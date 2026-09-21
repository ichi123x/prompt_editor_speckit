// TargetFolderState.cpp
// 対象フォルダの状態取得 (data-model.md「1. TargetFolderState」/ research.md R-006)

#include "pch.h"
#include "TargetFolderState.h"
#include "ProcessRunner.h"

namespace
{
	// パスを比較可能な形へ正規化する。
	// git は区切りに '/' を返すため、比較前に '\' へ統一し、末尾の区切りを除去する
	// (research.md R-006)。大文字小文字は比較側で無視する。
	CString NormalizePath(const CString& path)
	{
		CString normalized = path;
		normalized.Trim();
		normalized.Replace(_T('/'), _T('\\'));
		while (normalized.GetLength() > 0 && normalized[normalized.GetLength() - 1] == _T('\\'))
		{
			normalized.Truncate(normalized.GetLength() - 1);
		}
		return normalized;
	}

	CString CombinePath(const CString& folder, LPCTSTR name)
	{
		CString combined = NormalizePath(folder);
		combined += _T('\\');
		combined += name;
		return combined;
	}

	bool DirectoryExists(const CString& path)
	{
		DWORD attr = ::GetFileAttributes(path);
		return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	bool FileExists(const CString& path)
	{
		DWORD attr = ::GetFileAttributes(path);
		return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}

	// フォルダ直下に、指定されたワイルドカードに一致するファイルがあるか。
	bool HasFileMatching(const CString& folder, LPCTSTR pattern)
	{
		CFileFind finder;
		BOOL found = finder.FindFile(CombinePath(folder, pattern));
		while (found)
		{
			found = finder.FindNextFile();
			if (!finder.IsDirectory() && !finder.IsDots())
			{
				finder.Close();
				return true;
			}
		}
		finder.Close();
		return false;
	}

	// folder 自身、またはその配下のサブフォルダにプロジェクトマーカーがあるか。
	// Visual Studio のウィザードは対象フォルダの直下ではなく、ソリューション名と
	// 同じサブフォルダにプロジェクト一式を配置することがあるため、直下だけでなく
	// 配下も再帰的に探索する (017 FR-1701)。種別は区別せず、既存の4条件
	// (HasFileMatching が直下だけを見るのと同じ集合) のいずれかが見つかればよい
	// (017 FR-1702)。深さ優先・早期リターンで、最初に見つかった時点で打ち切る。
	bool HasProjectMarkerRecursive(const CString& folder)
	{
		if (HasFileMatching(folder, _T("*.sln"))
			|| HasFileMatching(folder, _T("*.vcxproj"))
			|| HasFileMatching(folder, _T("*.csproj"))
			|| FileExists(CombinePath(folder, _T("package.json"))))
		{
			return true;
		}

		CFileFind finder;
		BOOL found = finder.FindFile(CombinePath(folder, _T("*")));
		while (found)
		{
			found = finder.FindNextFile();
			if (finder.IsDirectory() && !finder.IsDots())
			{
				// .git の内部は探索しない (017 FR-1703)。Git 自身の内部ファイルに
				// よる誤検出を避けるため。
				if (finder.GetFileName().CompareNoCase(_T(".git")) == 0)
				{
					continue;
				}
				if (HasProjectMarkerRecursive(finder.GetFilePath()))
				{
					finder.Close();
					return true;
				}
			}
		}
		finder.Close();
		return false;
	}

	// 対象フォルダを作業ディレクトリとして git コマンドを実行する。
	ProcessResult RunGit(const CString& folder, LPCTSTR args)
	{
		CString commandLine;
		commandLine.Format(_T("git %s"), args);
		return ProcessRunner::Run(commandLine, folder, 30 * 1000);
	}
}

namespace TargetFolder
{
	// 対象フォルダの状態を読み取る (FR-006, FR-018)。読み取り専用。
	TargetFolderState Read(const CString& folderPath)
	{
		TargetFolderState state;
		if (folderPath.IsEmpty() || !DirectoryExists(folderPath))
		{
			return state;
		}

		// マーカーがどこで見つかっても、対象フォルダそのものは差し替えない (017 FR-1704)。
		// 以降の .gitignore 配置・Git 初期化・Spec Kit 足場配置は、常にこの path に対して行う。
		state.path = NormalizePath(folderPath);

		// プロジェクトマーカーの検出 (002 FR-103、017 FR-1701〜FR-1703)。
		// 対象フォルダの直下だけでなく、その配下のサブフォルダも探索する。見つからなければ
		// 従来どおり false のままとなり、「対象のプロジェクトが見つかりません」の表示ロジック
		// (呼び出し元の MFCApplication1Dlg.cpp) には手を入れない (017 FR-1705)。
		state.hasProjectMarker = HasProjectMarkerRecursive(state.path);

		// Spec Kit の足場と .gitignore。
		state.hasSpecifyDir = DirectoryExists(CombinePath(state.path, _T(".specify")));
		state.hasClaudeDir = DirectoryExists(CombinePath(state.path, _T(".claude")));

		// Git の状態 (research.md R-006)。
		// --is-inside-work-tree は親ディレクトリを遡って探索するため、これだけでは
		// 入れ子リポジトリを検出できない。--show-toplevel との組み合わせが必要。
		ProcessResult inside = RunGit(state.path, _T("rev-parse --is-inside-work-tree"));
		if (!inside.Succeeded())
		{
			state.gitState = GitState::NotRepository;
			return state;
		}

		ProcessResult toplevel = RunGit(state.path, _T("rev-parse --show-toplevel"));
		if (toplevel.Succeeded())
		{
			CString root = toplevel.output;
			root.Trim();
			state.gitRoot = NormalizePath(root);
			state.gitState = (state.gitRoot.CompareNoCase(state.path) == 0)
				? GitState::RepositoryAtTarget
				: GitState::NestedUnderParent;
		}
		else
		{
			// リポジトリ内と判定されたのにルートが取得できない場合は、
			// 安全側に倒して入れ子の可能性ありとして扱う (FR-016 の確認を経由させる)。
			state.gitState = GitState::NestedUnderParent;
		}

		// コミット件数。コミットが1件も無い場合、git rev-list は非0終了する。
		ProcessResult count = RunGit(state.path, _T("rev-list --count HEAD"));
		if (count.Succeeded())
		{
			CString text = count.output;
			text.Trim();
			state.commitCount = _ttoi(text);
		}

		return state;
	}

	// 書き込み権限の判定 (FR-015)。一時ファイルを作成するため診断からは呼ばない (FR-020)。
	bool IsWritable(const CString& folderPath)
	{
		if (folderPath.IsEmpty() || !DirectoryExists(folderPath))
		{
			return false;
		}

		CString probe;
		probe.Format(_T("%s\\.sddtool_write_probe_%lu.tmp"),
					 static_cast<LPCTSTR>(NormalizePath(folderPath)), ::GetCurrentProcessId());

		HANDLE handle = ::CreateFile(probe, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
									 FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
		if (handle == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		// FILE_FLAG_DELETE_ON_CLOSE により、ハンドルを閉じた時点で削除される。
		::CloseHandle(handle);
		return true;
	}
}
