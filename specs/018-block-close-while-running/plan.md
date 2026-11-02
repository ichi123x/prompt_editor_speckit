# Implementation Plan: 導入処理の実行中はダイアログを閉じられないようにする

**Branch**: `018-block-close-while-running` | **Date**: 2026-11-02 | **Spec**: [spec.md](./spec.md)

**Input**: `/specs/018-block-close-while-running/spec.md` からの機能仕様

## Summary（概要）

導入処理の実行中に「OK」「キャンセル」、✖ 等でダイアログを閉じられると、ワーカースレッドが結果を通知
しようとしている最中にダイアログが破棄され、実行中の外部コマンドも途中で強制終了される。コミット
`1d49765` で `#ifdef T_CHECK_BUG_01` に囲んで先行導入した修正を、憲章 v1.5.0 T-2 に基づく正式な機能
とする。

技術的アプローチ:

1. `CMFCApplication1Dlg::OnOK()` / `OnCancel()` のオーバーライドを常に有効にする。`m_running` が `true` の
   間は案内を出して基底クラスを呼ばない（FR-1801〜1805）。閉じる操作は Enter・Esc・✖・Alt+F4 を含めて
   すべてこの2関数に集まるため、追加の処理は要らない（[research.md](./research.md) R-1801）。
2. 手順の結果をダイアログへ渡す `OnStepFinished` / `OnStepFinishedMessage` を、生の `new` / `delete` から
   憲章 O-3 の `unique_ptr` + `release()` 方式に書き換える（FR-1806・FR-1807、R-1804）。
3. 実験用の目印 `T_CHECK_BUG_01` をコメントも含めて取り除き、FR 番号のコメントに置き換える（R-1805）。
4. CLAUDE.md の既存機能の表・憲章の説明・壊しやすい箇所を更新する（R-1805）。

## Technical Context（技術コンテキスト）

**Language/Version**: C++（MSVC v143、Unicode、既定の言語標準 = C++14。`std::make_unique` が使える）

**Primary Dependencies**: MFC（`CDialogEx` の `OnOK` / `OnCancel`、`AfxMessageBox`）、C++ 標準ライブラリ
`<memory>`（`std::unique_ptr` / `std::make_unique`。本フィーチャーで明示的に include する）

**Storage**: N/A

**Testing**: 自動テストスイートは存在しない（憲章の技術的制約）。静的テスト ST-1801〜1806、動的テスト
DT-1801〜1802 を [quickstart.md](./quickstart.md) の手順として定義する

**Target Platform**: Windows 10/11 デスクトップ

**Project Type**: desktop-app（MFC ダイアログアプリケーション）

**Performance Goals**: N/A（閉じる操作の判定は `bool` 1つの比較のみ）

**Constraints**: 実行中も「OK」「キャンセル」ボタンは無効化しない（R-1801）。新しい状態変数を追加せず、既存の
`m_running` を転用する（R-1803、原則III）。`.rc` のコントロール構成・寸法は変更しない。ソースコードに
`T_CHECK_BUG_01` の文字列をコメントも含めて残さない（ST-1803）

**Scale/Scope**: 変更するファイルは `MFCApplication1Dlg.h`・`MFCApplication1Dlg.cpp`・`CLAUDE.md` の3つ

## Constitution Check（憲法チェック）

*ゲート: Phase 0 のリサーチ前に通過する必要がある。Phase 1 の設計後に再チェックすること。*

### 基本原則

| 原則 | 評価 |
|---|---|
| **I. Spec Kit ワークフロー厳守** | 適合。先行修正（`1d49765`）は Spec Kit を経由していなかったが、本フィーチャーで正式化している。実施順は `/speckit-specify` → `/speckit-plan` → `/speckit-tasks` → `/speckit-analyze` → `/speckit-clarify` → 指摘の反映 → `/speckit-implement`。当初は clarify を省略していたが、`/speckit-analyze` が原則I（clarify を含む順序が MUST）との矛盾として CRITICAL（C1）を検出したため、実装前に実施した。clarify の結果は質問ゼロ（specify 時点の [NEEDS CLARIFICATION] 1件は利用者が回答済み）で、spec.md の変更は無い。なお clarify を省略した前例は 016・017 などにあるが、015 は実施している |
| **II. 日本語での一貫性** | 適合。追加する UI 文字列は案内1種類（日本語）のみ。追加・変更するコメントも日本語 |
| **III. シンプルさ優先（YAGNI）** | 適合。新しいクラス・ファイル・状態変数は追加しない。`m_running` の意味論（完了通知の受信で `false`）を読み解いて T-2 の判定に転用した（R-1803）。案内表示の3行の重複は共通化しない（R-1802） |
| **IV. 変更の追跡可能性** | 適合。変更箇所に FR-1801〜1807 のコメントを付ける（ST-1802 で検査）。CLAUDE.md の表に 018 を追加する |
| **V. 検証は実測に基づく** | 適合。ST-1803・ST-1804・ST-1806 は実装前の現状で不合格になることを確認済み（`T_CHECK_BUG_01` が存在、`new SetupStep` が1件、CLAUDE.md に `4つの原則` が存在）。DT-1802 は自動化、DT-1801 は利用者の実機実行。FR-1806 は画面操作で再現できないため、コード検査で確認することを quickstart.md に明記した |

### 所有権・スレッドモデル

| ルール | 評価 |
|---|---|
| **O-1. 生の new / delete は使わない** | 適合（本フィーチャーで是正）。`new SetupStep` / `delete step` を `make_unique` / `unique_ptr` に置き換える。PostMessage で所有権を渡す区間は O-1 の例外に該当し、所有者をコメントに書く（R-1804）。`MFCApplication1.cpp` の `new CShellManager` は spec Assumptions で対象外とした既存の違反であり、本フィーチャーでは変更しない |
| **O-2. 引数の `T*` / `T&` は借用** | 適合。`SetupThreadProc` に渡す `this` は借用であり、渡した側（ダイアログ）は T-2 によりワーカーの完了まで破棄されなくなる。本フィーチャーはまさにこの条件を成立させるものである |
| **O-3. スレッド間の所有権は unique_ptr と release** | 適合（本フィーチャーで是正）。送信成功時のみ `release()`、受信側は直ちに `unique_ptr` で受け取り直す（R-1804、data-model.md 2） |
| **T-1. UI 部品は UI スレッドだけが触る** | 適合。ワーカーは既存どおり `PostMessage` のみを使う。ワーカー側に `SendMessage` は無い（`SendMessage` は `OnPaint` 内の1箇所のみで、UI スレッドで動く） |
| **T-2. ワーカー実行中はダイアログを閉じない** | 適合（本フィーチャーで充足）。完了通知の受信を「ワーカーの処理の終了の確認」とみなす根拠は R-1803 |
| **T-3. 共有変数の保護** | 適合。`m_running` は UI スレッドのみが読み書きする（R-1803）。`m_steps`・`m_workerPath`・`m_workerRegisterRepo` は実行中はワーカーのみが使い、UI スレッドは開始前の設定のみ行う既存の設計（ヘッダーのコメント「UI スレッドは実行中に触らない」）であり、本フィーチャーで変えない |
| **T-4. ハンドルは使用スレッドの終了後に閉じる** | 該当なし。本フィーチャーはハンドルを扱うコード（`ProcessRunner`）を変更しない |

**Phase 1設計後の再評価**: 上表の判定に変更なし。`data-model.md`・`contracts/ui-contract.md`・
`quickstart.md` の作成で具体化されたのみで、新たな憲法上の懸念は生じていない。

## Project Structure（プロジェクト構造）

### Documentation（このフィーチャー）

```text
specs/018-block-close-while-running/
├── plan.md              # このファイル（/speckit-plan コマンドの出力）
├── research.md          # Phase 0 の出力（/speckit-plan コマンド）
├── data-model.md        # Phase 1 の出力（/speckit-plan コマンド）
├── quickstart.md        # Phase 1 の出力（/speckit-plan コマンド）
├── contracts/
│   └── ui-contract.md   # Phase 1 の出力（閉じる操作の契約、017 からの差分のみ）
├── checklists/
│   └── requirements.md  # /speckit-specify の出力（全項目合格）
└── tasks.md             # Phase 2 の出力（/speckit-tasks コマンド - /speckit-plan では作成されない）
```

### Source Code（リポジトリのルート）

```text
MFCApplication1/
├── MFCApplication1Dlg.h     # 変更: T_CHECK_BUG_01 の #define・#ifdef を削除し、OnOK/OnCancel の
│                            #       宣言を常に有効にする。コメントを FR 番号付きに置き換える (FR-1801, FR-1805)
├── MFCApplication1Dlg.cpp   # 変更: OnOK/OnCancel の #ifdef を削除 (FR-1801〜1805)。
│                            #       OnStepFinished/OnStepFinishedMessage を unique_ptr 方式へ (FR-1806, FR-1807)。
│                            #       #include <memory> を追加
├── MFCApplication1.rc       # 変更しない
├── SetupRunner.*            # 変更しない
├── ProcessRunner.*          # 変更しない
├── TargetFolderState.*      # 変更しない
└── GitignoreWriter.*        # 変更しない
CLAUDE.md                    # 変更: 既存機能の表に 018、憲章の説明を v1.5.0 に、壊しやすい箇所に1項目
```

**Structure Decision**: 既存の単一MFCプロジェクト構成（`MFCApplication1/`）をそのまま使用する。変更は
ダイアログクラス（`MFCApplication1Dlg.*`）の内部に閉じ、ワーカー側（`SetupRunner`）が呼ぶ
`ISetupProgressSink::OnStepFinished` のシグネチャは変えない。

## Complexity Tracking（複雑さの追跡）

該当なし。Constitution Checkで違反は検出されなかった。
