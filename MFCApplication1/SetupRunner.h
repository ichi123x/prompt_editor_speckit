// SetupRunner.h
// SDD化の5手順を順に確認・実行する。ワーカースレッドから呼ばれる。
// 設計根拠: data-model.md「3. SetupStep」/ contracts/ui-contract.md「5. 外部コマンドのコントラクト」

#pragma once

#include <afxstr.h>
#include <vector>
#include "TargetFolderState.h"

// 手順の識別子 (data-model.md「手順の定義と依存関係」)。
enum class StepId
{
	None,
	InstallUv,          // uv の導入
	InstallSpecify,     // specify CLI の導入 (InstallUv に依存)
	InstallVsCodeExt,   // VS Code 拡張の導入 (依存なし)
	InitSpecKit,        // Spec Kit 足場の配置 (InstallSpecify に依存)
	InitGit             // Git 初期化・.gitignore・初回コミット (依存なし)
};

// 手順の実行結果 (data-model.md「StepResult」)。
enum class StepResult
{
	NotRun,                 // まだ実行していない
	Success,                // 実行して完了した
	Skipped,                // 既に完了していたため実行しなかった
	SkippedUnmetDependency, // 依存する先行手順が失敗した (FR-012)
	Failed                  // 実行したが完了しなかった
};

struct SetupStep
{
	StepId id = StepId::None;
	CString displayName;
	StepId dependsOn = StepId::None;
	StepResult result = StepResult::NotRun;
	CString reason;
};

// 1手順が終わるたびに呼ばれる通知。ダイアログへ PostMessage するために用いる。
class ISetupProgressSink
{
public:
	virtual ~ISetupProgressSink() = default;
	virtual void OnStepFinished(const SetupStep& step) = 0;
};

namespace SetupRunner
{
	// 5手順を実行順に並べた初期状態を返す。
	std::vector<SetupStep> CreateSteps();

	// 5手順を順に実行する。各手順の完了ごとに sink へ通知する。
	// folderPath はワーカースレッド起動時にコピーされた値であり、
	// 実行中にダイアログのメンバ変数を参照しない (contracts/ui-contract.md「2.」)。
	void RunAll(const CString& folderPath,
				std::vector<SetupStep>& steps, ISetupProgressSink* sink);

	// 結果の表示文字列 (contracts/ui-contract.md「4. メッセージ文字列の契約」)。
	CString ResultDisplayName(StepResult result);
}
