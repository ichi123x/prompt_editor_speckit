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

	// 出力の先頭部分を理由欄へ載せるために切り詰める。
	CString Brief(const CString& output, int maxLength = 160)
	{
		CString text = output;
		text.Replace(_T("\r\n"), _T(" "));
		text.Replace(_T("\n"), _T(" "));
		text.Replace(_T("\t"), _T(" "));
		text.Trim();
		if (text.GetLength() > maxLength)
		{
			text = text.Left(maxLength) + _T("...");
		}
		return text;
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
	void RunInitGit(SetupStep& step, const CString& folderPath, ISetupProgressSink* sink)
	{
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
}

namespace SetupRunner
{
	std::vector<SetupStep> CreateSteps()
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

		return steps;
	}

	void RunAll(const CString& folderPath,
				std::vector<SetupStep>& steps, ISetupProgressSink* sink)
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
				RunInitGit(step, folderPath, sink);
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
