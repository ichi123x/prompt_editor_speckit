// ReadinessChecker.cpp
// 準備完了チェックリスト7項目の判定 (FR-018〜FR-021 / research.md R-008)

#include "pch.h"
#include "ReadinessChecker.h"

#include <string>

namespace
{
	CString Combine(const CString& folder, LPCTSTR name)
	{
		CString path = folder;
		path.TrimRight(_T('\\'));
		path += _T('\\');
		path += name;
		return path;
	}

	bool FileExists(const CString& path)
	{
		DWORD attr = ::GetFileAttributes(path);
		return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}

	bool GetWriteTime(const CString& path, FILETIME& time)
	{
		WIN32_FILE_ATTRIBUTE_DATA data = {};
		if (!::GetFileAttributesEx(path, GetFileExInfoStandard, &data))
		{
			return false;
		}
		time = data.ftLastWriteTime;
		return true;
	}

	// テキストファイルを読み出す。UTF-8 として解釈し、失敗したら ANSI にフォールバックする。
	bool ReadText(const CString& path, CString& text)
	{
		CFile file;
		CFileException ex;
		if (!file.Open(path, CFile::modeRead | CFile::shareDenyNone, &ex))
		{
			return false;
		}

		ULONGLONG length = file.GetLength();
		if (length > 4 * 1024 * 1024)
		{
			file.Close();
			return false;
		}

		std::string bytes(static_cast<size_t>(length), '\0');
		if (length > 0)
		{
			file.Read(&bytes[0], static_cast<UINT>(length));
		}
		file.Close();

		if (bytes.empty())
		{
			text.Empty();
			return true;
		}

		int len = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
										bytes.c_str(), static_cast<int>(bytes.size()), nullptr, 0);
		UINT codePage = CP_UTF8;
		if (len <= 0)
		{
			codePage = CP_ACP;
			len = ::MultiByteToWideChar(CP_ACP, 0,
										bytes.c_str(), static_cast<int>(bytes.size()), nullptr, 0);
			if (len <= 0)
			{
				return false;
			}
		}

		LPWSTR buffer = text.GetBuffer(len);
		::MultiByteToWideChar(codePage, 0,
							  bytes.c_str(), static_cast<int>(bytes.size()), buffer, len);
		text.ReleaseBuffer(len);
		return true;
	}

	// HTML コメント (<!-- ... -->) を取り除いたテキストを返す。
	//
	// これを行わないと、憲法冒頭の Sync Impact Report に「削除した項目」として引用された
	// プレースホルダー名 (例: [PRINCIPLE_5_NAME]) を未置換とみなし、正しく制定された憲法を
	// 「未達」と誤判定する。research.md R-008 に実測例を記載。
	CString StripHtmlComments(const CString& text)
	{
		CString result;
		int pos = 0;
		for (;;)
		{
			int start = text.Find(_T("<!--"), pos);
			if (start < 0)
			{
				result += text.Mid(pos);
				break;
			}
			result += text.Mid(pos, start - pos);

			int end = text.Find(_T("-->"), start + 4);
			if (end < 0)
			{
				// 閉じられていないコメントは、以降すべてをコメントとみなす。
				break;
			}
			pos = end + 3;
		}
		return result;
	}

	// [大文字・数字・アンダースコア] 形式の未置換プレースホルダーが残っているか (research.md R-008)。
	bool HasUnreplacedPlaceholder(const CString& text)
	{
		int pos = 0;
		while (pos < text.GetLength())
		{
			int start = text.Find(_T('['), pos);
			if (start < 0)
			{
				return false;
			}
			int end = text.Find(_T(']'), start + 1);
			if (end < 0)
			{
				return false;
			}

			const int innerLength = end - start - 1;
			if (innerLength > 0)
			{
				bool matches = true;
				for (int i = start + 1; i < end; ++i)
				{
					const TCHAR ch = text[i];
					const bool allowed = (ch >= _T('A') && ch <= _T('Z'))
						|| (ch >= _T('0') && ch <= _T('9'))
						|| ch == _T('_');
					if (!allowed)
					{
						matches = false;
						break;
					}
				}
				if (matches)
				{
					return true;
				}
			}
			pos = start + 1;
		}
		return false;
	}

	bool ContainsAny(const CString& text, const std::vector<CString>& keywords)
	{
		for (const auto& keyword : keywords)
		{
			if (text.Find(keyword) >= 0)
			{
				return true;
			}
		}
		return false;
	}

	ReadinessItem MakeItem(int id, LPCTSTR name)
	{
		ReadinessItem item;
		item.id = id;
		item.displayName = name;
		return item;
	}

	void SetVerdict(ReadinessItem& item, ReadinessVerdict verdict, LPCTSTR nextAction)
	{
		item.verdict = verdict;
		item.nextAction = (verdict == ReadinessVerdict::Met) ? _T("") : nextAction;
	}
}

namespace ReadinessChecker
{
	std::vector<ReadinessItem> Check(const CString& folderPath)
	{
		std::vector<ReadinessItem> items;

		// TargetFolder::Read は読み取り専用。IsWritable は呼ばない (FR-020)。
		const TargetFolderState state = TargetFolder::Read(folderPath);

		const CString constitutionPath =
			Combine(folderPath, _T(".specify\\memory\\constitution.md"));
		const CString claudeMdPath = Combine(folderPath, _T("CLAUDE.md"));

		CString constitutionText;
		const bool hasConstitution = FileExists(constitutionPath)
			&& ReadText(constitutionPath, constitutionText);

		CString claudeMdText;
		const bool hasClaudeMd = FileExists(claudeMdPath) && ReadText(claudeMdPath, claudeMdText);

		// --- 項目1: ビルドが通る雛形がある ---------------------------------
		// 実ビルドは行わない。マーカーがあれば「達成」、無ければ「判定不能」。
		// この「達成」はマーカーの存在を示すもので、ビルドの成功を保証しない。
		{
			ReadinessItem item = MakeItem(1, _T("ビルドが通る雛形がある"));
			if (state.hasSlnMarker || state.hasWebMarker)
			{
				SetVerdict(item, ReadinessVerdict::Met, _T(""));
				item.nextAction = _T("プロジェクトを検出しました（実ビルドは未確認です）。");
			}
			else
			{
				SetVerdict(item, ReadinessVerdict::Indeterminate,
						   _T("Visual Studio 2022 などのウィザードでプロジェクトを作成し、このフォルダに配置してください。"));
			}
			items.push_back(item);
		}

		// --- 項目2: Spec Kit の足場が同じ階層にある -------------------------
		{
			ReadinessItem item = MakeItem(2, _T("Spec Kit の足場が同じ階層にある"));
			const bool hasGit = (state.gitState == GitState::RepositoryAtTarget);
			if (hasGit && state.hasGitignore && state.hasSpecifyDir && state.hasClaudeDir)
			{
				SetVerdict(item, ReadinessVerdict::Met, _T(""));
			}
			else
			{
				CString missing;
				if (!hasGit) { missing += _T(".git "); }
				if (!state.hasGitignore) { missing += _T(".gitignore "); }
				if (!state.hasSpecifyDir) { missing += _T(".specify "); }
				if (!state.hasClaudeDir) { missing += _T(".claude "); }
				missing.Trim();

				CString action;
				action.Format(_T("不足: %s ／「生成」ボタンで Spec Kit を導入してください。"),
							  static_cast<LPCTSTR>(missing));
				SetVerdict(item, ReadinessVerdict::NotMet, action);
			}
			items.push_back(item);
		}

		// --- 項目3: コミットが1件以上入っている ------------------------------
		{
			ReadinessItem item = MakeItem(3, _T("コミットが1件以上入っている"));
			if (state.commitCount > 0)
			{
				SetVerdict(item, ReadinessVerdict::Met, _T(""));
			}
			else
			{
				SetVerdict(item, ReadinessVerdict::NotMet,
						   _T("「生成」ボタンで Git 初期化と初回コミットを行ってください。"));
			}
			items.push_back(item);
		}

		// --- 項目4: テスト方針が決まっている --------------------------------
		// 人間の判断であり機械的に断定できない。憲法にテスト関連の記述があるかを
		// 手がかりとし、無ければ「未達」ではなく「判定不能」とする (FR-021)。
		{
			ReadinessItem item = MakeItem(4, _T("テスト方針が決まっている"));
			std::vector<CString> keywords;
			keywords.push_back(_T("テスト"));
			keywords.push_back(_T("Test"));
			keywords.push_back(_T("test"));

			if (hasConstitution && ContainsAny(constitutionText, keywords))
			{
				SetVerdict(item, ReadinessVerdict::Indeterminate,
						   _T("憲章にテストへの言及があります。内容が方針として妥当か確認してください。"));
			}
			else
			{
				SetVerdict(item, ReadinessVerdict::Indeterminate,
						   _T("何をテスト対象にし、何を対象外にするかを決め、憲章に記載してください。"));
			}
			items.push_back(item);
		}

		// --- 項目5: 憲法がある ----------------------------------------------
		{
			ReadinessItem item = MakeItem(5, _T("憲章がある（未対応TODOがゼロ）"));
			if (!hasConstitution)
			{
				SetVerdict(item, ReadinessVerdict::NotMet,
						   _T("Claude Code で /speckit-constitution を実行して憲章を制定してください。"));
			}
			else
			{
				// HTML コメントを除外してから走査する。除外しないと Sync Impact Report 中に
				// 引用されたプレースホルダー名で誤判定する (research.md R-008)。
				const CString scanned = StripHtmlComments(constitutionText);
				if (HasUnreplacedPlaceholder(scanned))
				{
					SetVerdict(item, ReadinessVerdict::NotMet,
							   _T("憲章に未置換のプレースホルダーが残っています。/speckit-constitution を実行してください。"));
				}
				else
				{
					SetVerdict(item, ReadinessVerdict::Met, _T(""));
				}
			}
			items.push_back(item);
		}

		// --- 項目6: CLAUDE.md が憲法を反映している ---------------------------
		// 内容の実質的な整合は判定せず、ファイルの更新時刻で判定する。
		{
			ReadinessItem item = MakeItem(6, _T("CLAUDE.md が憲章を反映している"));
			if (!hasClaudeMd)
			{
				SetVerdict(item, ReadinessVerdict::NotMet,
						   _T("Claude Code で /init を実行して CLAUDE.md を生成してください。"));
			}
			else if (!hasConstitution)
			{
				SetVerdict(item, ReadinessVerdict::Indeterminate,
						   _T("憲章が無いため比較できません。先に /speckit-constitution を実行してください。"));
			}
			else
			{
				FILETIME claudeTime = {};
				FILETIME constitutionTime = {};
				if (GetWriteTime(claudeMdPath, claudeTime)
					&& GetWriteTime(constitutionPath, constitutionTime))
				{
					if (::CompareFileTime(&claudeTime, &constitutionTime) >= 0)
					{
						SetVerdict(item, ReadinessVerdict::Met, _T(""));
						item.nextAction = _T("更新時刻に基づく参考判定です。");
					}
					else
					{
						SetVerdict(item, ReadinessVerdict::NotMet,
								   _T("CLAUDE.md が憲章より古いままです。/init を再実行してください。"));
					}
				}
				else
				{
					SetVerdict(item, ReadinessVerdict::Indeterminate,
							   _T("ファイルの更新時刻を取得できませんでした。内容を目視で確認してください。"));
				}
			}
			items.push_back(item);
		}

		// --- 項目7: 技術スタックが記録されている ------------------------------
		// キーワードが見つからない場合は「未達」ではなく「判定不能」とする (FR-021)。
		{
			ReadinessItem item = MakeItem(7, _T("技術スタックが記録されている"));
			std::vector<CString> keywords;
			keywords.push_back(_T("ビルド"));
			keywords.push_back(_T("Build"));
			keywords.push_back(_T("build"));
			keywords.push_back(_T("msbuild"));
			keywords.push_back(_T("MSBuild"));

			if (hasClaudeMd && ContainsAny(claudeMdText, keywords))
			{
				SetVerdict(item, ReadinessVerdict::Indeterminate,
						   _T("CLAUDE.md にビルド構成の記述があります。言語とテストランナーも揃っているか確認してください。"));
			}
			else
			{
				SetVerdict(item, ReadinessVerdict::Indeterminate,
						   _T("言語・ビルド構成・テストランナーを CLAUDE.md に明記してください。"));
			}
			items.push_back(item);
		}

		return items;
	}

	CString VerdictDisplayName(ReadinessVerdict verdict)
	{
		switch (verdict)
		{
		case ReadinessVerdict::Met:
			return _T("達成");
		case ReadinessVerdict::NotMet:
			return _T("未達");
		default:
			return _T("判定不能");
		}
	}
}
