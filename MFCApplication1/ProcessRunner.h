// ProcessRunner.h
// 外部コマンドを CreateProcess + 匿名パイプで実行し、終了コードと標準出力を取得する。
// 設計根拠: research.md R-001
//  - PowerShell を介在させないことで、実行ポリシーによるブロックを構造的に回避する。
//  - ShellExecute は出力を取得できないため、手順の成否判定 (FR-011) に使えない。

#pragma once

#include <afxstr.h>

// 外部コマンドの実行結果。
struct ProcessResult
{
	// プロセスを起動できたか。false の場合、コマンドが PATH 上に見つからない等で
	// 実行自体ができなかったことを表す (エッジケース: コマンド不在)。
	bool launched = false;

	// プロセスの終了コード。launched が false の場合は意味を持たない。
	DWORD exitCode = 0;

	// 標準出力と標準エラーを結合した文字列。
	CString output;

	// 起動に失敗した場合の Win32 エラーコード。
	DWORD launchError = 0;

	// 起動できて、かつ終了コードが 0 であること。
	// 各手順の「成功」判定に用いる (FR-011)。
	bool Succeeded() const { return launched && exitCode == 0; }
};

namespace ProcessRunner
{
	// コマンドラインを実行し、終了するまで待って結果を返す。
	//
	// commandLine   : 実行するコマンドライン。lpApplicationName は NULL とするため、
	//                 実行ファイルは PATH から解決される (research.md R-001)。
	// workingDir    : 作業ディレクトリ。空文字なら呼び出し元の既定を用いる。
	//                 CreateProcess の lpCurrentDirectory で指定し、プロセス全体の
	//                 カレントディレクトリ (SetCurrentDirectory) は変更しない。
	// timeoutMs     : タイムアウト (ミリ秒)。超過した場合は子プロセスを終了させる。
	ProcessResult Run(const CString& commandLine,
					  const CString& workingDir = CString(),
					  DWORD timeoutMs = 10 * 60 * 1000);

	// 出力の各行を走査し、needle と完全一致する行があるかを調べる (大文字小文字を区別しない)。
	// VS Code 拡張の導入判定に用いる (FR-009 手順3 / research.md R-004)。
	bool OutputHasLine(const CString& output, const CString& needle);
}
