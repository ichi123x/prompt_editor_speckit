// ProcessRunner.cpp
// 外部コマンドの実行 (research.md R-001)

#include "pch.h"
#include "ProcessRunner.h"

#include <string>

namespace
{
	// 子プロセスの出力バイト列を CString へ変換する。
	// 子プロセス (git, uv, specify 等) の出力は UTF-8 のことが多いが、
	// Windows のコンソール既定コードページ (日本語環境では CP932) のこともある。
	// まず UTF-8 として解釈し、不正なバイト列で失敗した場合のみ ANSI として解釈する。
	CString BytesToString(const std::string& bytes)
	{
		if (bytes.empty())
		{
			return CString();
		}

		// MB_ERR_INVALID_CHARS を付けることで、UTF-8 として不正なら 0 が返る。
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
				return CString();
			}
		}

		CString result;
		LPWSTR buffer = result.GetBuffer(len);
		::MultiByteToWideChar(codePage, 0,
							  bytes.c_str(), static_cast<int>(bytes.size()), buffer, len);
		result.ReleaseBuffer(len);
		return result;
	}
}

namespace ProcessRunner
{
	ProcessResult Run(const CString& commandLine, const CString& workingDir, DWORD timeoutMs)
	{
		ProcessResult result;

		SECURITY_ATTRIBUTES sa = {};
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = TRUE;
		sa.lpSecurityDescriptor = nullptr;

		HANDLE readPipe = nullptr;
		HANDLE writePipe = nullptr;
		if (!::CreatePipe(&readPipe, &writePipe, &sa, 0))
		{
			result.launchError = ::GetLastError();
			return result;
		}

		// 読み出し側は子プロセスへ継承させない。継承させると、親が読み出しを終える前に
		// パイプが閉じられず、ReadFile が終端を検出できなくなる。
		::SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

		STARTUPINFO si = {};
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
		si.hStdOutput = writePipe;
		si.hStdError = writePipe;
		si.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);

		PROCESS_INFORMATION pi = {};

		// CreateProcess は lpCommandLine を書き換える可能性があるため、可変バッファを渡す。
		CString mutableCmd = commandLine;
		LPWSTR cmdBuffer = mutableCmd.GetBuffer();

		// lpApplicationName は NULL。実行ファイルは PATH から解決される (research.md R-001)。
		// 作業ディレクトリは lpCurrentDirectory で指定し、SetCurrentDirectory は使わない。
		LPCWSTR dir = workingDir.IsEmpty() ? nullptr : static_cast<LPCWSTR>(workingDir);
		BOOL created = ::CreateProcess(nullptr, cmdBuffer, nullptr, nullptr,
									   TRUE, CREATE_NO_WINDOW, nullptr, dir, &si, &pi);
		mutableCmd.ReleaseBuffer();

		if (!created)
		{
			result.launchError = ::GetLastError();
			::CloseHandle(readPipe);
			::CloseHandle(writePipe);
			return result;
		}

		result.launched = true;

		// 親側の書き込みハンドルを閉じる。これを閉じないと、子プロセスが終了しても
		// パイプの書き込み端が開いたままになり、ReadFile が終端を返さず無限に待つ。
		::CloseHandle(writePipe);
		writePipe = nullptr;

		// パイプを読み出しながら終了を待つ。読み出さずに WaitForSingleObject すると、
		// 子プロセスがパイプバッファ (既定 4KB) を埋めた時点でデッドロックする
		// (research.md R-001 の実装上の注意)。
		std::string collected;
		const ULONGLONG startTick = ::GetTickCount64();
		char buffer[4096];
		for (;;)
		{
			DWORD bytesRead = 0;
			if (!::ReadFile(readPipe, buffer, sizeof(buffer), &bytesRead, nullptr) || bytesRead == 0)
			{
				// 書き込み端がすべて閉じられた (子プロセスが終了した) 場合、
				// ReadFile は ERROR_BROKEN_PIPE で失敗する。これが正常な終端。
				break;
			}
			collected.append(buffer, bytesRead);

			if (::GetTickCount64() - startTick > timeoutMs)
			{
				::TerminateProcess(pi.hProcess, static_cast<UINT>(WAIT_TIMEOUT));
				break;
			}
		}

		const ULONGLONG elapsed = ::GetTickCount64() - startTick;
		const DWORD remaining = (elapsed >= timeoutMs)
			? 0 : static_cast<DWORD>(timeoutMs - elapsed);
		if (::WaitForSingleObject(pi.hProcess, remaining) == WAIT_TIMEOUT)
		{
			::TerminateProcess(pi.hProcess, static_cast<UINT>(WAIT_TIMEOUT));
			::WaitForSingleObject(pi.hProcess, 5000);
		}

		DWORD exitCode = 0;
		if (::GetExitCodeProcess(pi.hProcess, &exitCode))
		{
			result.exitCode = exitCode;
		}

		::CloseHandle(pi.hThread);
		::CloseHandle(pi.hProcess);
		::CloseHandle(readPipe);

		result.output = BytesToString(collected);
		return result;
	}

	bool OutputHasLine(const CString& output, const CString& needle)
	{
		CString target = needle;
		target.Trim();
		if (target.IsEmpty())
		{
			return false;
		}

		int pos = 0;
		while (pos >= 0 && pos <= output.GetLength())
		{
			CString line = output.Tokenize(_T("\r\n"), pos);
			if (pos < 0)
			{
				break;
			}
			line.Trim();
			if (line.CompareNoCase(target) == 0)
			{
				return true;
			}
		}
		return false;
	}
}
