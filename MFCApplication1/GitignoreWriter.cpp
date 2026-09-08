// GitignoreWriter.cpp
// .gitignore の生成と追記 (research.md R-007)

#include "pch.h"
#include "GitignoreWriter.h"
#include "Resource.h"

#include <string>

namespace
{
	// 追記する2ブロックの本文。目印行を含む (contracts/ui-contract.md「6.」)。
	LPCTSTR kSpecKitBlock =
		_T("\r\n")
		GITIGNORE_MARK_SPECKIT _T("\r\n")
		_T("# feature.json は作業中の機能を指すマシンローカルな状態。\r\n")
		_T("# 複数PCで同じ機能を続けるなら、除外せずコミットする選択もある\r\n")
		_T("# .specify/feature.json\r\n");

	LPCTSTR kProjectBlock =
		_T("\r\n")
		GITIGNORE_MARK_PROJECT _T("\r\n")
		_T("*.pdf\r\n");

	// RCDATA リソースを UTF-8 のバイト列として読み出す (FR-010)。
	bool LoadGitignoreResource(UINT resourceId, std::string& bytes)
	{
		HMODULE module = ::AfxGetResourceHandle();
		HRSRC found = ::FindResource(module, MAKEINTRESOURCE(resourceId), RT_RCDATA);
		if (found == nullptr)
		{
			return false;
		}

		DWORD size = ::SizeofResource(module, found);
		HGLOBAL loaded = ::LoadResource(module, found);
		if (loaded == nullptr || size == 0)
		{
			return false;
		}

		const void* data = ::LockResource(loaded);
		if (data == nullptr)
		{
			return false;
		}

		bytes.assign(static_cast<const char*>(data), size);
		return true;
	}

	// ファイル全体をテキストとして読み出す。追記済みかの判定にのみ用いる。
	bool ReadAllText(const CString& path, CStringA& content)
	{
		CFile file;
		CFileException ex;
		if (!file.Open(path, CFile::modeRead | CFile::shareDenyNone, &ex))
		{
			return false;
		}

		ULONGLONG length = file.GetLength();
		if (length > 8 * 1024 * 1024)
		{
			// 想定外に大きい .gitignore は扱わない。
			file.Close();
			return false;
		}

		content = CStringA();
		if (length > 0)
		{
			char* buffer = content.GetBuffer(static_cast<int>(length));
			file.Read(buffer, static_cast<UINT>(length));
			content.ReleaseBuffer(static_cast<int>(length));
		}
		file.Close();
		return true;
	}

	// UTF-8 のバイト列としてファイルへ追記する。
	bool AppendBytes(const CString& path, const std::string& bytes, bool createNew)
	{
		CFile file;
		CFileException ex;
		UINT mode = CFile::modeWrite | CFile::shareExclusive;
		mode |= createNew ? CFile::modeCreate : CFile::modeNoTruncate;
		if (!file.Open(path, mode, &ex))
		{
			return false;
		}

		if (!createNew)
		{
			file.SeekToEnd();
		}
		if (!bytes.empty())
		{
			file.Write(bytes.c_str(), static_cast<UINT>(bytes.size()));
		}
		file.Close();
		return true;
	}

	std::string ToUtf8(LPCTSTR text)
	{
		int len = ::WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
		if (len <= 1)
		{
			return std::string();
		}
		std::string bytes(static_cast<size_t>(len - 1), '\0');
		::WideCharToMultiByte(CP_UTF8, 0, text, -1, &bytes[0], len - 1, nullptr, nullptr);
		return bytes;
	}

	std::string ToUtf8Narrow(LPCTSTR text)
	{
		return ToUtf8(text);
	}
}

namespace GitignoreWriter
{
	bool Ensure(const CString& folderPath, bool& changed, CString& errorMessage)
	{
		changed = false;
		errorMessage.Empty();

		CString path = folderPath;
		path.TrimRight(_T('\\'));
		path += _T("\\.gitignore");

		DWORD attr = ::GetFileAttributes(path);
		const bool exists = (attr != INVALID_FILE_ATTRIBUTES)
			&& ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0);

		CStringA existing;
		if (exists && !ReadAllText(path, existing))
		{
			errorMessage = _T(".gitignore を読み取れませんでした。");
			return false;
		}

		// 既存ファイルには本体を書き込まない。未記載の追記ブロックのみを足す
		// (001 FR-010 / 002 FR-106a)。既存の本体が統合版と異なっていても置き換えない。
		std::string payload;
		if (!exists)
		{
			// 002 FR-106: 新規作成時は、対象フォルダの内容にかかわらず
			// 常に同じ統合リソースを書き出す。種別による分岐は存在しない。
			std::string body;
			if (!LoadGitignoreResource(IDR_GITIGNORE, body))
			{
				errorMessage = _T(".gitignore の埋め込みリソースを読み出せませんでした。");
				return false;
			}
			payload += body;
		}

		// 目印行の有無で追記済みかを判定する。これによりべき等性が成立する (001 FR-013 / 002 FR-106b)。
		const std::string markSpecKit = ToUtf8Narrow(GITIGNORE_MARK_SPECKIT);
		const std::string markProject = ToUtf8Narrow(GITIGNORE_MARK_PROJECT);
		const std::string current(existing.GetString(), static_cast<size_t>(existing.GetLength()));

		if (exists ? (current.find(markSpecKit) == std::string::npos) : true)
		{
			payload += ToUtf8Narrow(kSpecKitBlock);
		}
		if (exists ? (current.find(markProject) == std::string::npos) : true)
		{
			payload += ToUtf8Narrow(kProjectBlock);
		}

		if (payload.empty())
		{
			// 既に本体も2ブロックも揃っている。何もしない (001 FR-013 / 002 FR-106b)。
			return true;
		}

		if (!AppendBytes(path, payload, !exists))
		{
			errorMessage = _T(".gitignore を書き込めませんでした。");
			return false;
		}

		changed = true;
		return true;
	}
}
