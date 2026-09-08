// ReadinessChecker.h
// 準備完了チェックリスト7項目を判定する。読み取り専用 (FR-020)。
// 設計根拠: data-model.md「4. ReadinessItem」/ research.md R-008

#pragma once

#include <afxstr.h>
#include <vector>
#include "TargetFolderState.h"

// 判定結果 (data-model.md「ReadinessVerdict」)。
enum class ReadinessVerdict
{
	Met,           // 完了条件を満たしている
	NotMet,        // 満たしていないと断定できる
	Indeterminate  // 機械的に断定できない。人間の確認が必要 (FR-021)
};

struct ReadinessItem
{
	int id = 0;                // 1〜7。手順書の準備完了チェックリストの項番
	CString displayName;
	ReadinessVerdict verdict = ReadinessVerdict::NotMet;
	CString nextAction;        // 未達・判定不能のとき次に取るべき行動 (FR-019)
};

namespace ReadinessChecker
{
	// 7項目を判定する。
	// 対象フォルダへの書き込みを一切行わない。書き込み権限の判定 (TargetFolder::IsWritable) は
	// 一時ファイルを作るため、ここから呼び出してはならない (FR-020 / SC-005)。
	std::vector<ReadinessItem> Check(const CString& folderPath);

	// 判定結果の表示文字列 (contracts/ui-contract.md「4.」)。
	CString VerdictDisplayName(ReadinessVerdict verdict);
}
