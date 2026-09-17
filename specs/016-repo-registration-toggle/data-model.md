# Phase 1 データモデル: リポジトリ登録の要否を選択できるようにする

**Feature**: 016-repo-registration-toggle
**Date**: 2026-09-17
**前提**: 001〜015のdata-model.mdを基準とし、本書はその**差分のみ**を定義する。

---

## 1. 新設するリソースID

`Resource.h`（[research.md](./research.md) R-1606）:

| 定数 | 値 | 種別 |
|---|---|---|
| `IDC_STATIC_REPO_LABEL` | 1009 | 静的ラベル |
| `IDC_RADIO_REPO_OK` | 1010 | ラジオボタン（グループ先頭、`WS_GROUP`） |
| `IDC_RADIO_REPO_NG` | 1011 | ラジオボタン |

廃止済みID（1002・1003・1005）は再利用しない（既存の`Resource.h`コメント規約）。

## 2. 変更する`.rc`リソース

すべて新規追加。既存8コントロールの座標・サイズは変更しない（015時点の570 DLU幅・8コントロール構成を維持）。

| コントロール | 種別 | x, y, w, h (DLU) | スタイル | 根拠 |
|---|---|---|---|---|
| `IDC_STATIC_REPO_LABEL` | LTEXT | 90, 34, 65, 10 | 既定 | [research.md](./research.md) R-1604（2026-09-18改訂） |
| `IDC_RADIO_REPO_OK` | AUTORADIOBUTTON | 157, 34, 34, 10 | `WS_GROUP`（グループ先頭） | R-1604, R-1607（2026-09-18改訂） |
| `IDC_RADIO_REPO_NG` | AUTORADIOBUTTON | 195, 34, 34, 10 | 既定 | R-1604, R-1607（2026-09-18改訂） |

**2026-09-18改訂**: 当初は幅90/40/40 DLU・間隔4 DLUで実装したが、ユーザーの実機目視により「ラベルとOKの間が空きすぎ」との指摘を受け、詰めた。詳細は[research.md](./research.md) R-1604を参照。

ダイアログ本体の幅・高さ（570 x 233 DLU）は変更しない。

## 3. 変更する構造体・関数シグネチャ

### `SetupRunner.h`

```cpp
namespace SetupRunner
{
    std::vector<SetupStep> CreateSteps(bool registerRepo);   // 引数追加 (FR-1605)

    void RunAll(const CString& folderPath,
                std::vector<SetupStep>& steps, ISetupProgressSink* sink,
                bool registerRepo);                            // 引数追加 (FR-1603, FR-1604)

    CString ResultDisplayName(StepResult result);               // 変更なし
}
```

`SetupStep`・`StepId`・`StepResult`・`ISetupProgressSink` は変更しない（[research.md](./research.md) R-1601: 依存関係グラフ自体は無変更）。

### `SetupRunner.cpp`（内部関数）

```cpp
void RunInitGit(SetupStep& step, const CString& folderPath,
                 ISetupProgressSink* sink, bool registerRepo);   // 引数追加

void RunGenerateClaudeMd(SetupStep& step, const CString& folderPath,
                           ISetupProgressSink* sink, bool registerRepo); // 引数追加
```

### `MFCApplication1Dlg.h`

```cpp
bool m_workerRegisterRepo = true;   // ワーカーへ渡す選択のコピー (R-1602)。既定 true = 「OK」
```

## 4. 状態遷移

| 出来事 | 処理 |
|---|---|
| ダイアログの初期化（`OnInitDialog`） | `CheckRadioButton(IDC_RADIO_REPO_OK, IDC_RADIO_REPO_NG, IDC_RADIO_REPO_OK)` で既定「OK」を選択状態にする（FR-1601） |
| 「生成」ボタン押下 | `m_workerRegisterRepo = (IsDlgButtonChecked(IDC_RADIO_REPO_OK) == BST_CHECKED)` を`m_workerPath`と同じタイミングでコピーし、`CreateSteps(m_workerRegisterRepo)` で手順一覧を構築する（R-1602, R-1603） |
| ワーカースレッド実行中 | `SetControlsEnabled(false)` は既存の生成・参照ボタンのみを無効化する。新規2ラジオボタンは無効化しない（FR-1608、2026-09-17改訂。R-1608参照） |
| `RunAll` 実行 | `m_workerRegisterRepo` を`RunInitGit`・`RunGenerateClaudeMd`にそのまま渡す。それ以外の4手順は影響を受けない（FR-1607） |
| 手順5（Git初期化） | `registerRepo == false` の場合、既存ロジックを一切実行せず `StepResult::Skipped` で即終了する（FR-1603, R-1601） |
| 手順6（CLAUDE.md） | `registerRepo == false` の場合、生成・存在確認までは実行し、`git add`/`git commit`のみ省略する（FR-1604） |

## 5. 検証チェーンへの影響

**なし**。001の検証チェーン（フォルダ未選択→書き込み権限→マーカー→足場が中途半端→入れ子リポジトリ）は、対象フォルダの状態のみを見ており、ラジオボタンの選択を参照しない。本フィーチャーはこのチェーンより後（手順5・6の内部）にのみ影響する。
