// SetupRunner.cpp
// 5手順の実行 (001 FR-009〜FR-013a / research.md R-004〜R-007)
//
// 002 FR-108 / 003 FR-207: 5手順の定義・依存関係・スキップ条件は、種別選択の廃止
// (002) と診断の廃止 (003) のいずれによっても変更していない。この不変性は
// べき等性の回帰テスト (002 DT-105 / 003 DT-208) が確認する。

#include "pch.h"
#include "SetupRunner.h"
#include "ProcessRunner.h"
#include "GitignoreWriter.h"

namespace
{
	// 実行できなかった (コマンドが PATH 上に無い) 場合の理由文字列。
	CString CommandNotFoundReason(LPCTSTR commandName)
	{
		CString reason;
		reason.Format(_T("%s が見つかりません。インストールして PATH を通してから再実行してください。"),
					  commandName);
		return reason;
	}

	// FR-1102: 罫線描画文字 (Box Drawing, U+2500-257F) とブロック要素 (Block Elements,
	// U+2580-259F) かどうかを判定する (research.md R-1103)。
	bool IsDecorativeChar(WCHAR ch)
	{
		return (ch >= 0x2500 && ch <= 0x257F) || (ch >= 0x2580 && ch <= 0x259F);
	}

	// 1行から装飾文字を取り除いてTrimする。可視文字が残らなければ空文字列になる。
	CString StripDecorativeChars(const CString& line)
	{
		CString result;
		for (int i = 0; i < line.GetLength(); ++i)
		{
			WCHAR ch = line[i];
			if (!IsDecorativeChar(ch))
			{
				result += ch;
			}
		}
		result.Trim();
		return result;
	}

	// CString::Tokenize は strtok 相当で連続する区切り文字をまとめて読み飛ばしてしまい、
	// 空行を検出できない。段落の区切り判定に空行の連続数を使うため、空文字列の行も
	// そのまま保持する素朴な分割を自前で行う。
	std::vector<CString> SplitLines(const CString& text)
	{
		std::vector<CString> lines;
		int start = 0;
		int length = text.GetLength();
		for (int i = 0; i <= length; ++i)
		{
			if (i == length || text[i] == _T('\n'))
			{
				lines.push_back(text.Mid(start, i - start));
				start = i + 1;
			}
		}
		return lines;
	}

	CString JoinWithSpace(const std::vector<CString>& parts)
	{
		CString joined;
		for (size_t i = 0; i < parts.size(); ++i)
		{
			if (i > 0)
			{
				joined += _T(" ");
			}
			joined += parts[i];
		}
		return joined;
	}

	// 出力の先頭部分ではなく、実際に診断の役に立つ内容を理由欄へ載せるために整形する。
	//
	// FR-1101・FR-1102: specify等の一部の外部コマンドは、罫線・ブロック文字による装飾バナーを
	// 出力の先頭に付けるため、単純な先頭切り詰めでは実際のエラー本文が失われる
	// (research.md R-1101、実機の specify init で確認)。出力を行に分割し、各行から装飾文字を
	// 取り除いたうえで、可視文字が残る行（内容行）だけを段落にまとめ、最後の段落を採用する。
	// 罫線ボックス内部の余白として現れる空行1行だけでは段落の区切りとみなさず、2行以上
	// 連続した空行でのみ区切る（research.md R-1102 — 区切りを空行1行にすると、タイトル行と
	// 本文行が別の段落に分断され、最後の段落に肝心の見出しが含まれなくなることを実機データで確認済み）。
	// FR-1104: 採用した段落は maxLength を超える場合その段落の先頭から切り詰める。
	// FR-1105: 本関数の変更は SetupRunner.cpp 内に閉じ、ProcessRunner 側には一切影響しない。
	CString Brief(const CString& output, int maxLength = 160)
	{
		// FR-1103: 装飾除去が効かない場合のフォールバック用に、既存どおりの正規化結果も用意する。
		CString normalizedWhole = output;
		normalizedWhole.Replace(_T("\r\n"), _T(" "));
		normalizedWhole.Replace(_T("\n"), _T(" "));
		normalizedWhole.Replace(_T("\t"), _T(" "));
		normalizedWhole.Trim();

		CString forSplit = output;
		forSplit.Replace(_T("\r\n"), _T("\n"));
		forSplit.Replace(_T("\t"), _T(" "));

		std::vector<CString> paragraphs;
		std::vector<CString> current;
		int emptyRun = 0;
		for (const CString& line : SplitLines(forSplit))
		{
			CString stripped = StripDecorativeChars(line);
			if (stripped.IsEmpty())
			{
				++emptyRun;
				if (emptyRun >= 2 && !current.empty())
				{
					paragraphs.push_back(JoinWithSpace(current));
					current.clear();
				}
			}
			else
			{
				emptyRun = 0;
				current.push_back(stripped);
			}
		}
		if (!current.empty())
		{
			paragraphs.push_back(JoinWithSpace(current));
		}

		if (paragraphs.empty())
		{
			// FR-1103: 内容行が1つも無かった場合 (出力全体が装飾のみ) は、既存どおり
			// 先頭から切り詰めたものにフォールバックする。
			if (normalizedWhole.GetLength() > maxLength)
			{
				return normalizedWhole.Left(maxLength) + _T("...");
			}
			return normalizedWhole;
		}

		CString lastParagraph = paragraphs.back();
		if (lastParagraph.GetLength() > maxLength)
		{
			return lastParagraph.Left(maxLength) + _T("...");
		}
		return lastParagraph;
	}

	void Finish(SetupStep& step, StepResult result, LPCTSTR reason, ISetupProgressSink* sink)
	{
		step.result = result;
		step.reason = reason;
		if (sink != nullptr)
		{
			sink->OnStepFinished(step);
		}
	}

	SetupStep* FindStep(std::vector<SetupStep>& steps, StepId id)
	{
		for (auto& step : steps)
		{
			if (step.id == id)
			{
				return &step;
			}
		}
		return nullptr;
	}

	// 依存する先行手順が失敗しているか (FR-012)。
	// 依存が無い手順 (InstallVsCodeExt, InitGit) は常に false を返し、
	// 他の手順の失敗にかかわらず実行される。
	bool DependencyFailed(std::vector<SetupStep>& steps, const SetupStep& step)
	{
		if (step.dependsOn == StepId::None)
		{
			return false;
		}
		const SetupStep* parent = FindStep(steps, step.dependsOn);
		if (parent == nullptr)
		{
			return false;
		}
		return parent->result == StepResult::Failed
			|| parent->result == StepResult::SkippedUnmetDependency;
	}

	// --- 手順1: uv の導入 (research.md R-004) -------------------------------
	void RunInstallUv(SetupStep& step, ISetupProgressSink* sink)
	{
		ProcessResult probe = ProcessRunner::Run(_T("uv --version"), CString(), 60 * 1000);
		if (probe.Succeeded())
		{
			Finish(step, StepResult::Skipped, _T("uv は導入済みです。"), sink);
			return;
		}

		// FR-801: winget は初回実行時、msstore ソースの利用規約同意プロンプトを表示することがあり、
		// 本アプリは非対話実行のため入力待ちで無期限にハングする (research.md R-801)。
		// --source winget でそのソースへのアクセス自体を避け、残る2フラグで将来の同意プロンプトにも備える。
		ProcessResult install = ProcessRunner::Run(
			_T("winget install --id=astral-sh.uv -e --accept-source-agreements --accept-package-agreements --source winget"),
			CString(), 10 * 60 * 1000);
		if (!install.launched)
		{
			Finish(step, StepResult::Failed, CommandNotFoundReason(_T("winget")), sink);
			return;
		}
		if (!install.Succeeded())
		{
			Finish(step, StepResult::Failed, Brief(install.output), sink);
			return;
		}
		Finish(step, StepResult::Success, _T("uv をインストールしました。"), sink);
	}

	// --- 手順2: specify CLI の導入 (research.md R-004) ----------------------
	void RunInstallSpecify(SetupStep& step, ISetupProgressSink* sink)
	{
		// specify version の出力は罫線付きボックスのため、バージョン文字列はパースせず
		// 終了コードのみで判定する (research.md R-004)。
		ProcessResult probe = ProcessRunner::Run(_T("specify version"), CString(), 60 * 1000);
		if (probe.Succeeded())
		{
			Finish(step, StepResult::Skipped, _T("specify CLI は導入済みです。"), sink);
			return;
		}

		ProcessResult install = ProcessRunner::Run(
			_T("uv tool install specify-cli"), CString(), 10 * 60 * 1000);
		if (!install.launched)
		{
			Finish(step, StepResult::Failed, CommandNotFoundReason(_T("uv")), sink);
			return;
		}
		if (!install.Succeeded())
		{
			Finish(step, StepResult::Failed, Brief(install.output), sink);
			return;
		}

		// FR-1001・FR-1004・FR-1005: uv tool install はインストール先ディレクトリ
		// (既定 %USERPROFILE%\.local\bin) をレジストリのPATHへ自動登録しないため、
		// 明示的に uv tool update-shell を呼んで登録させる (research.md R-1001)。
		// specify CLI 導入手順に固有の対応であり、他の手順 (winget・code・git) には
		// 汎用化しない。009 で実装した ProcessRunner::Run() のPATH再読み込みと組み合わさり、
		// 続く手順4がアプリを再起動しなくても specify を検出できるようになる。
		//
		// FR-1002・FR-1003: この呼び出しは install が成功した場合のみ行い (スキップ・失敗の
		// 分岐には追加しない)、戻り値は判定に使わない。失敗しても手順2自体の成否には影響させず、
		// 既存の「見つかりません」という失敗表示に自然にフォールバックさせる。
		ProcessRunner::Run(_T("uv tool update-shell"), CString(), 60 * 1000);

		Finish(step, StepResult::Success, _T("specify CLI をインストールしました。"), sink);
	}

	// --- 手順3: VS Code 拡張の導入 (research.md R-004) ----------------------
	void RunInstallVsCodeExt(SetupStep& step, ISetupProgressSink* sink)
	{
		// code は code.cmd のため、CreateProcess から直接起動できず cmd /c を経由する
		// (research.md R-001)。
		ProcessResult list = ProcessRunner::Run(
			_T("cmd.exe /c code --list-extensions"), CString(), 60 * 1000);
		if (!list.launched)
		{
			Finish(step, StepResult::Failed, CommandNotFoundReason(_T("code")), sink);
			return;
		}
		if (list.Succeeded() && ProcessRunner::OutputHasLine(list.output, _T("anthropic.claude-code")))
		{
			Finish(step, StepResult::Skipped, _T("Claude Code 拡張は導入済みです。"), sink);
			return;
		}

		ProcessResult install = ProcessRunner::Run(
			_T("cmd.exe /c code --install-extension anthropic.claude-code"), CString(), 5 * 60 * 1000);
		if (!install.Succeeded())
		{
			Finish(step, StepResult::Failed, Brief(install.output), sink);
			return;
		}
		Finish(step, StepResult::Success, _T("Claude Code 拡張をインストールしました。"), sink);
	}

	// --- 手順4: Spec Kit 足場の配置 (research.md R-005) ---------------------
	void RunInitSpecKit(SetupStep& step, const CString& folderPath, ISetupProgressSink* sink)
	{
		TargetFolderState state = TargetFolder::Read(folderPath);
		if (state.hasSpecifyDir && state.hasClaudeDir)
		{
			Finish(step, StepResult::Skipped, _T(".specify と .claude は配置済みです。"), sink);
			return;
		}

		// 4つのフラグはいずれも省略不可 (research.md R-005):
		//  --non-interactive : 省くとパイプ接続時にプロンプト待ちでハングする
		//  --integration claude : 省くと非対話セッションでは Copilot が既定になる
		//  --force : 対象フォルダはプロジェクト配置済みで非空のため確認待ちになる
		//  --script ps : Windows の既定だが明示して決定性を確保する
		ProcessResult init = ProcessRunner::Run(
			_T("specify init . --integration claude --script ps --force --non-interactive"),
			folderPath, 10 * 60 * 1000);

		if (!init.launched)
		{
			Finish(step, StepResult::Failed, CommandNotFoundReason(_T("specify")), sink);
			return;
		}
		if (!init.Succeeded())
		{
			Finish(step, StepResult::Failed, Brief(init.output), sink);
			return;
		}

		TargetFolderState after = TargetFolder::Read(folderPath);
		if (!after.hasSpecifyDir || !after.hasClaudeDir)
		{
			Finish(step, StepResult::Failed,
				   _T("specify init は完了しましたが、.specify と .claude を確認できませんでした。"), sink);
			return;
		}
		Finish(step, StepResult::Success, _T(".specify と .claude を配置しました。"), sink);
	}

	// --- 手順5: Git 初期化・.gitignore・初回コミット (research.md R-006) ----
	void RunInitGit(SetupStep& step, const CString& folderPath, ISetupProgressSink* sink,
					 bool registerRepo)
	{
		// 016 FR-1603: 「NG」が選択された場合、本手順は一切実行しない。
		// StepResult::Skipped で終えることで、既存の DependencyFailed() の仕組み
		// （Skipped は依存充足として扱われる）により手順6はそのまま実行される
		// (research.md R-1601)。
		if (!registerRepo)
		{
			Finish(step, StepResult::Skipped,
				   _T("「NG」が選択されたため、Gitの初期化を行いませんでした。"), sink);
			return;
		}

		TargetFolderState state = TargetFolder::Read(folderPath);

		// .gitignore の追記ブロックまで揃っていて、かつコミット済みなら手順全体をスキップする。
		bool gitignoreChanged = false;
		CString gitignoreError;
		if (!GitignoreWriter::Ensure(folderPath, gitignoreChanged, gitignoreError))
		{
			Finish(step, StepResult::Failed, gitignoreError, sink);
			return;
		}

		const bool alreadyRepo = (state.gitState == GitState::RepositoryAtTarget);
		if (alreadyRepo && state.commitCount > 0 && !gitignoreChanged)
		{
			Finish(step, StepResult::Skipped,
				   _T("Git は初期化済みで、コミットと .gitignore も揃っています。"), sink);
			return;
		}

		CString note;

		if (!alreadyRepo)
		{
			ProcessResult init = ProcessRunner::Run(_T("git init"), folderPath, 60 * 1000);
			if (!init.launched)
			{
				Finish(step, StepResult::Failed, CommandNotFoundReason(_T("git")), sink);
				return;
			}
			if (!init.Succeeded())
			{
				Finish(step, StepResult::Failed, Brief(init.output), sink);
				return;
			}
		}
		else
		{
			note += _T("Git初期化はスキップ。");
		}

		// FR-013a: ステージングの前に既存の未コミット変更を確認する。
		// 存在する場合、git add -A はユーザーの作業を巻き込んでしまうため使わず、
		// 本ツールが作成・変更した .gitignore のみをステージする。
		ProcessResult status = ProcessRunner::Run(
			_T("git status --porcelain"), folderPath, 60 * 1000);
		if (!status.launched)
		{
			Finish(step, StepResult::Failed, CommandNotFoundReason(_T("git")), sink);
			return;
		}

		bool hasForeignChanges = false;
		if (alreadyRepo && state.commitCount > 0)
		{
			// 既にコミットのあるリポジトリでのみ問題になる。
			// .gitignore 以外の変更が1件でもあれば、限定的なステージングへ切り替える。
			int pos = 0;
			CString text = status.output;
			while (pos >= 0 && pos <= text.GetLength())
			{
				CString line = text.Tokenize(_T("\r\n"), pos);
				if (pos < 0)
				{
					break;
				}
				line.Trim();
				if (line.IsEmpty())
				{
					continue;
				}
				// 「XY パス」形式。パス部分に .gitignore 以外が現れたら他人の変更とみなす。
				if (line.Find(_T(".gitignore")) < 0)
				{
					hasForeignChanges = true;
					break;
				}
			}
		}

		ProcessResult add = hasForeignChanges
			? ProcessRunner::Run(_T("git add -- .gitignore"), folderPath, 60 * 1000)
			: ProcessRunner::Run(_T("git add -A"), folderPath, 5 * 60 * 1000);
		if (!add.Succeeded())
		{
			Finish(step, StepResult::Failed, Brief(add.output), sink);
			return;
		}

		if (hasForeignChanges)
		{
			// T029a: 限定的なステージングを行ったことを結果一覧で明示し、
			// ユーザーが自分の作業を巻き込んでいないことを確認できるようにする (FR-013a)。
			note += _T("既存の未コミット変更を検出したため、.gitignore のみをコミットしました。");
		}

		ProcessResult commit = ProcessRunner::Run(
			_T("git commit -m \"Spec Kit 導入\""), folderPath, 5 * 60 * 1000);
		if (!commit.Succeeded())
		{
			CString output = commit.output;
			if (output.Find(_T("user.name")) >= 0 || output.Find(_T("user.email")) >= 0)
			{
				Finish(step, StepResult::Failed,
					   _T("Git のユーザー設定 (user.name / user.email) が必要です。"), sink);
				return;
			}
			// 003 FR-210, FR-211: 判定文字列は推測ではなく実測に基づく。
			//
			// git はコミット対象が無いとき、リポジトリの状態によって2通りの文言を
			// 出し分ける。実測した出力は次のとおり。
			//
			//  (a) 未追跡ファイルがある場合:
			//        nothing added to commit but untracked files present (use "git add" to track)
			//  (b) 作業ツリーがクリーンな場合:
			//        nothing to commit, working tree clean
			//
			// 001 では (b) だけを想定して "nothing to commit" を探していたが、
			// これは (a) の "nothing added to commit" の部分文字列ではないため
			// 一致せず、「スキップ」とすべきところが「失敗」と表示されていた。
			// 文字列を変更する場合は、実際に git を動かして出力を採取すること。
			if (output.Find(_T("nothing added to commit")) >= 0
				|| output.Find(_T("nothing to commit")) >= 0)
			{
				Finish(step, StepResult::Skipped,
					   _T("コミットすべき変更がありませんでした。"), sink);
				return;
			}
			Finish(step, StepResult::Failed, Brief(output), sink);
			return;
		}

		if (note.IsEmpty())
		{
			note = _T("Git を初期化し、初回コミットを作成しました。");
		}
		Finish(step, StepResult::Success, note, sink);
	}

	// 対象フォルダ直下に指定した名前のファイルが存在するか (013)。
	// TargetFolderState は .specify/.claude 等の既存の事実しか持たないため、
	// CLAUDE.md 用に拡張せず、GitignoreWriter.cpp/TargetFolderState.cpp と同じ
	// GetFileAttributes の使い方でここに閉じたヘルパーとして持つ (原則III YAGNI)。
	bool FileExists(const CString& folderPath, LPCTSTR fileName)
	{
		CString path = folderPath;
		if (!path.IsEmpty() && path[path.GetLength() - 1] != _T('\\'))
		{
			path += _T('\\');
		}
		path += fileName;
		DWORD attr = ::GetFileAttributes(path);
		return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}

	// --- 手順6: CLAUDE.md の生成とコミット (013、contracts/claude-init-contract.md) ---
	//
	// 本関数はいずれの失敗経路でも StepResult::Failed を返して呼び出し元 (RunAll) へ
	// 復帰するのみであり、それ以前の手順 (Git初期化等、既存4手順) の結果を書き換えたり
	// 取り消したりする処理は一切行わない。既存4手順のコードには一切触れないため、この
	// 非ロールバック特性は追加コードを書かないことで自然に満たされる (FR-1308)。
	void RunGenerateClaudeMd(SetupStep& step, const CString& folderPath, ISetupProgressSink* sink,
							   bool registerRepo)
	{
		// FR-1303: 既に CLAUDE.md が存在する場合、内容の正しさは問わずスキップする。
		// 他手順 (RunInitSpecKit の .specify/.claude 存在確認) と同じ「存在すれば
		// 完了済みとみなす」設計に合わせる (research.md Assumptions)。
		if (FileExists(folderPath, _T("CLAUDE.md")))
		{
			Finish(step, StepResult::Skipped, _T("CLAUDE.md は既に生成されています。"), sink);
			return;
		}

		// FR-1304・FR-1309・FR-1310・R-1301: claude CLI を非対話・確認スキップモードで
		// 実行し /init を実行させる。ProcessRunner 経由のため、PATH再読み込み(009)・
		// タイムアウト監視(008)の仕組みをそのまま利用する。/init はLLM呼び出しを伴い
		// 実行時間が変動するため (実測34〜58秒)、他手順より長い15分をタイムアウトとする
		// (R-1305)。
		// FR-1401・FR-1402・FR-1403 (014、R-1401): プロンプトに日本語での指示を1つ
		// 加えるだけで、生成されるCLAUDE.mdの本文が日本語になることを実機で確認した。
		// それ以外のフラグ・タイムアウト・以降の処理は013から変更しない。
		ProcessResult init = ProcessRunner::Run(
			_T("claude --print \"/init のCLAUDE.mdは日本語で書いてください。\" --permission-mode bypassPermissions --output-format text"),
			folderPath, 15 * 60 * 1000);

		// FR-1307: claude が PATH 上に見つからない場合は他手順と同じ様式で失敗にする。
		if (!init.launched)
		{
			Finish(step, StepResult::Failed, CommandNotFoundReason(_T("claude")), sink);
			return;
		}
		if (!init.Succeeded())
		{
			Finish(step, StepResult::Failed, Brief(init.output), sink);
			return;
		}

		// FR-1305: コマンドが成功しても、実際に CLAUDE.md が生成されたことを確認する
		// (RunInitSpecKit の .specify/.claude 存在確認と同じ二段判定)。
		if (!FileExists(folderPath, _T("CLAUDE.md")))
		{
			Finish(step, StepResult::Failed,
				   _T("/init は完了しましたが、CLAUDE.md を確認できませんでした。"), sink);
			return;
		}

		// 016 FR-1604: 「NG」が選択された場合、CLAUDE.mdの生成まではここまでの処理で
		// 完了しているが、コミット（git add / git commit）は行わない。
		if (!registerRepo)
		{
			Finish(step, StepResult::Success, _T("CLAUDE.md を生成しました。"), sink);
			return;
		}

		// FR-1306・R-1304: CLAUDE.md のみを対象にステージする。RunInitGit が持つ
		// 「git status を見て他人の変更を検出したら限定ステージングに切り替える」という
		// 分岐は複製しない。対象ファイルが常に CLAUDE.md 1つに定まっているため、
		// git add -A ではなく対象を明示するだけで他の未コミット変更を巻き込まずに済む。
		ProcessResult add = ProcessRunner::Run(_T("git add -- CLAUDE.md"), folderPath, 60 * 1000);
		if (!add.Succeeded())
		{
			Finish(step, StepResult::Failed, Brief(add.output), sink);
			return;
		}

		ProcessResult commit = ProcessRunner::Run(
			_T("git commit -m \"CLAUDE.md を追加\""), folderPath, 60 * 1000);
		if (!commit.Succeeded())
		{
			Finish(step, StepResult::Failed, Brief(commit.output), sink);
			return;
		}

		Finish(step, StepResult::Success, _T("CLAUDE.md を生成し、コミットしました。"), sink);
	}
}

namespace SetupRunner
{
	std::vector<SetupStep> CreateSteps(bool registerRepo)
	{
		std::vector<SetupStep> steps;

		SetupStep uv;
		uv.id = StepId::InstallUv;
		uv.displayName = _T("uv の導入");
		uv.dependsOn = StepId::None;
		steps.push_back(uv);

		SetupStep specify;
		specify.id = StepId::InstallSpecify;
		specify.displayName = _T("specify CLI の導入");
		specify.dependsOn = StepId::InstallUv;
		steps.push_back(specify);

		SetupStep ext;
		ext.id = StepId::InstallVsCodeExt;
		ext.displayName = _T("VS Code 拡張の導入");
		ext.dependsOn = StepId::None;
		steps.push_back(ext);

		SetupStep speckit;
		speckit.id = StepId::InitSpecKit;
		speckit.displayName = _T("Spec Kit 足場の配置");
		speckit.dependsOn = StepId::InstallSpecify;
		steps.push_back(speckit);

		SetupStep git;
		git.id = StepId::InitGit;
		git.displayName = _T("Git 初期化と初回コミット");
		git.dependsOn = StepId::None;
		steps.push_back(git);

		// FR-1301・FR-1302: Git初期化の後に実行し、InitGitに依存する。InitGitが失敗した
		// 場合は既存のDependencyFailed()/FR-012の仕組みにより自動的にスキップされる
		// (R-1306)。
		// 016 FR-1605: 「NG」選択時（registerRepo == false）はコミットを行わないため、
		// 項目名も「CLAUDE.md の生成とコミット」ではなく「CLAUDE.md の生成」にする。
		SetupStep claudeMd;
		claudeMd.id = StepId::GenerateClaudeMd;
		claudeMd.displayName = registerRepo ? _T("CLAUDE.md の生成とコミット") : _T("CLAUDE.md の生成");
		claudeMd.dependsOn = StepId::InitGit;
		steps.push_back(claudeMd);

		return steps;
	}

	void RunAll(const CString& folderPath,
				std::vector<SetupStep>& steps, ISetupProgressSink* sink,
				bool registerRepo)
	{
		for (auto& step : steps)
		{
			// FR-012: 依存する先行手順が失敗した場合のみスキップする。
			// 依存しない手順は先行手順の失敗にかかわらず実行する。
			if (DependencyFailed(steps, step))
			{
				Finish(step, StepResult::SkippedUnmetDependency,
					   _T("前提となる手順が失敗したため実行しませんでした。"), sink);
				continue;
			}

			switch (step.id)
			{
			case StepId::InstallUv:
				RunInstallUv(step, sink);
				break;
			case StepId::InstallSpecify:
				RunInstallSpecify(step, sink);
				break;
			case StepId::InstallVsCodeExt:
				RunInstallVsCodeExt(step, sink);
				break;
			case StepId::InitSpecKit:
				RunInitSpecKit(step, folderPath, sink);
				break;
			case StepId::InitGit:
				RunInitGit(step, folderPath, sink, registerRepo);
				break;
			case StepId::GenerateClaudeMd:
				RunGenerateClaudeMd(step, folderPath, sink, registerRepo);
				break;
			default:
				break;
			}
		}
	}

	CString ResultDisplayName(StepResult result)
	{
		switch (result)
		{
		case StepResult::Success:
			return _T("成功");
		case StepResult::Skipped:
			return _T("スキップ");
		case StepResult::SkippedUnmetDependency:
			return _T("スキップ（前提未達）");
		case StepResult::Failed:
			return _T("失敗");
		default:
			return _T("-");
		}
	}
}
