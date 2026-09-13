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

	// FR-802: タイムアウト監視スレッドへ渡すパラメータ。子プロセス・ジョブオブジェクトの
	// ハンドルと、呼び出し元が指定したタイムアウトのみを保持する (research.md R-802)。
	struct TimeoutWatcherParams
	{
		HANDLE hProcess;
		HANDLE hJob;
		DWORD timeoutMs;
	};

	// FR-802・FR-804: 子プロセスの生死だけを監視する。パイプの読み出し (ReadFile) とは
	// 完全に独立しているため、子プロセスが出力を止めて入力待ちになっても、この待機は
	// timeoutMs で必ず戻る。タイムアウトした場合のみ、ジョブオブジェクト経由でプロセス
	// ツリーごと終了させる (research.md R-802・R-804)。ジョブオブジェクトの作成に失敗して
	// いた場合 (hJob が null) は、直接プロセスのみを終了させる縮退動作とする。
	DWORD WINAPI TimeoutWatcherThreadProc(LPVOID param)
	{
		TimeoutWatcherParams* params = static_cast<TimeoutWatcherParams*>(param);
		if (::WaitForSingleObject(params->hProcess, params->timeoutMs) == WAIT_TIMEOUT)
		{
			if (params->hJob != nullptr)
			{
				::TerminateJobObject(params->hJob, static_cast<UINT>(WAIT_TIMEOUT));
			}
			else
			{
				::TerminateProcess(params->hProcess, static_cast<UINT>(WAIT_TIMEOUT));
			}
		}
		return 0;
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

		// FR-804: CREATE_SUSPENDED で起動し、ジョブオブジェクトへの割り当てが終わってから
		// ResumeThread する。先に実行を始めてしまうと、割り当てが完了する前に子プロセスが
		// 孫プロセスを起動する競合が起こり得て、その孫プロセスがジョブに含まれない
		// (=タイムアウト時に終了させられない) おそれがある (research.md R-804)。
		BOOL created = ::CreateProcess(nullptr, cmdBuffer, nullptr, nullptr,
									   TRUE, CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr, dir, &si, &pi);
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

		// FR-804: プロセスツリーごと確実に終了させるためのジョブオブジェクト。
		// JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE を付けることで、子プロセスが起動した子孫プロセスも
		// 含めて一度に終了できる (research.md R-802・R-804)。作成に失敗しても起動自体は続行し、
		// 監視スレッド側で単一プロセスのみを終了させる縮退動作に委ねる。
		HANDLE hJob = ::CreateJobObject(nullptr, nullptr);
		if (hJob != nullptr)
		{
			JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo = {};
			jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
			::SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jobInfo, sizeof(jobInfo));
			::AssignProcessToJobObject(hJob, pi.hProcess);
		}
		::ResumeThread(pi.hThread);

		// FR-802: 子プロセスの生死をタイムアウト付きで監視する別スレッド。下の読み出しループは
		// 子プロセスが出力を止めると ReadFile が無条件にブロックし続けるため、パイプの読み出しとは
		// 独立にタイムアウトを判定する主体がここで必要になる (research.md R-802)。
		//
		// FR-803・FR-805・FR-806: タイムアウト時の終了コードは従来どおり WAIT_TIMEOUT を用い、
		// 呼び出し元の Succeeded() 判定・結果一覧の「失敗」表示をそのまま流用する。この関数の
		// シグネチャ・呼び出し元 (SetupRunner.cpp の5手順すべて) は変更せず、uv・specify・code・git
		// のいずれの呼び出しにも等しく適用される単一の仕組みとする。
		TimeoutWatcherParams watcherParams = { pi.hProcess, hJob, timeoutMs };
		HANDLE hWatcherThread = ::CreateThread(nullptr, 0, TimeoutWatcherThreadProc, &watcherParams, 0, nullptr);

		// パイプを読み出しながら終了を待つ。読み出さずに WaitForSingleObject すると、
		// 子プロセスがパイプバッファ (既定 4KB) を埋めた時点でデッドロックする
		// (research.md R-001 の実装上の注意)。タイムアウト判定は上の監視スレッドが行うため、
		// ここでは書き込み端がすべて閉じられる (ERROR_BROKEN_PIPE) までひたすら読み出す。
		std::string collected;
		char buffer[4096];
		for (;;)
		{
			DWORD bytesRead = 0;
			if (!::ReadFile(readPipe, buffer, sizeof(buffer), &bytesRead, nullptr) || bytesRead == 0)
			{
				// 書き込み端がすべて閉じられた (子プロセスが終了した、またはタイムアウトで
				// 終了させられた) 場合、ReadFile は ERROR_BROKEN_PIPE で失敗する。
				break;
			}
			collected.append(buffer, bytesRead);
		}

		::WaitForSingleObject(pi.hProcess, 5000);

		DWORD exitCode = 0;
		if (::GetExitCodeProcess(pi.hProcess, &exitCode))
		{
			result.exitCode = exitCode;
		}

		// 監視スレッドは子プロセスの終了 (正常・タイムアウトいずれか) とともに自然に終了する。
		if (hWatcherThread != nullptr)
		{
			::WaitForSingleObject(hWatcherThread, INFINITE);
			::CloseHandle(hWatcherThread);
		}
		if (hJob != nullptr)
		{
			::CloseHandle(hJob);
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
