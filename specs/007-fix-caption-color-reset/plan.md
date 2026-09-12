# Implementation Plan: フォルダ選択ダイアログ後もタイトルバーの色を保つ

**Branch**: `007-fix-caption-color-reset` | **Date**: 2026-09-12 | **Spec**: [spec.md](./spec.md)

**Input**: `/specs/007-fix-caption-color-reset/spec.md` からの機能仕様

## Summary（概要）

「参照...」ボタンでフォルダ選択ダイアログ（`CFolderPickerDialog`）を開いて閉じると、005 で設定したメインダイアログのタイトルバーの濃紺 `#1F3864` が既定色（白）に戻り、二度と濃紺に復帰しない不具合を修正する。

利用者の実機報告（スクリーンショット）により不具合の実在は確認できたが、同一開発機上で実際のマウス入力による自動再現を複数パターン試みても再現できず（[research.md](./research.md) R-701）、根本原因（DWM が非クライアント領域の再描画を促す何らかのメッセージを受けて色をリセットする、という仮説）を実機で確定させることはできなかった。

そのため、原因の完全特定を追求するのではなく、**症状に直接効く防御的な修正**を採用する。技術的アプローチは、005 で `OnInitDialog` に実装した着色処理（`DwmSetWindowAttribute` への2回の呼び出し）を非公開メンバ関数 `ApplyCaptionColor()` に切り出し、`OnInitDialog` に加えて `CFolderPickerDialog::DoModal()` を呼ぶ**直前**と、**返った直後**（`IDOK` / それ以外を問わず）の**両方**で呼び出す。開く直前にも呼ぶのは、利用者の報告どおりダイアログを開いた瞬間に色が崩れる可能性（spec.md User Story 1 Acceptance Scenario 1）にも備えるためである。

実質的な変更は `MFCApplication1Dlg.cpp`／`.h` の数行のみで、新しいクラス・メンバ変数・状態は増えない。

## Technical Context（技術コンテキスト）

**Language/Version**: C++（MSVC v143、Unicode、C++17）

**Primary Dependencies**: MFC、Windows SDK の `dwmapi.h` / `dwmapi.lib`（005 で導入済み、新規追加なし）

**Storage**: N/A（本フィーチャーは何も永続化しない）

**Testing**: 自動テストスイートは存在しない（憲章の技術的制約）。静的テスト ST-701〜703 を [quickstart.md](./quickstart.md) の手順として定義し実施する。動的テスト DT-701 はフォルダ選択ダイアログの開閉を伴うため、**利用者による手動実行**とする（[research.md](./research.md) R-704）

**Target Platform**: Windows 11（005 の着色が効く環境）。着色できない環境では元々何も起こらないため対象外

**Project Type**: desktop-app（MFC ダイアログアプリケーション）

**Performance Goals**: N/A。API 呼び出しが2回（`DoModal()` 前後で1回ずつ）追加されるのみで、体感できる遅延は生じない

**Constraints**: フォルダ選択ダイアログの見た目・動作を変更しないこと（FR-704）。005〜006 で確立した機能を一切変更しないこと（FR-705）。新しい `.ps1` をプロジェクトへ追加しないこと（憲章の技術的制約）

**Scale/Scope**: 変更するファイルは `MFCApplication1Dlg.cpp` と `MFCApplication1Dlg.h` の2ファイルのみ。追加行数は十数行

## Constitution Check（憲法チェック）

*ゲート: Phase 0 のリサーチ前に通過する必要がある。Phase 1 の設計後に再チェックすること。*

| 原則 | 判定 | 根拠 |
|---|---|---|
| **I. Spec Kit ワークフロー厳守** | 合格 | `/speckit-specify` → `/speckit-plan` の順で進めている。実装は `/speckit-tasks`・`/speckit-analyze` の後 |
| **II. 日本語での一貫性** | 合格 | 本フィーチャーは UI 文字列を追加しない。コメントと成果物はすべて日本語 |
| **III. シンプルさ優先（YAGNI）** | 合格 | 新しいクラス・メンバ変数・状態を作らない。`ApplyCaptionColor()` という非公開関数1つのみ追加する（呼び出し箇所が3箇所に増えるための最小限の重複排除であり、[research.md](./research.md) R-703 で理由を明記）。`WM_ACTIVATE` など汎用的な再適用の仕組みは採用しない（原因を特定できていない状態で対象を広げるのは YAGNI に反すると判断） |
| **IV. 変更の追跡可能性** | 合格 | 追加する各箇所に FR-701〜706 のコメントを付す。ST-703 が機械的に検査する |

**技術的制約との整合**:

- MSBuild のみを使用し、CMake 等は導入しない — 合格
- 新しい `.ps1` をプロジェクトへ追加しない — 合格（調査・検証スクリプトはスクラッチパッドに置き、リポジトリには含めない）
- テストランナーを新規導入しない — 合格（ST/DT の手順として定義）

**設計後の再チェック（Phase 1 完了時点）**: 上記4原則および技術的制約について、Phase 1 の成果物（data-model.md・contracts/ui-contract.md・quickstart.md）を作成した後も**判定は変わらない**。**違反ゼロのため Complexity Tracking セクションは記入しない。**

## Project Structure（プロジェクト構造）

### Documentation（このフィーチャー）

```text
specs/007-fix-caption-color-reset/
├── plan.md              # このファイル
├── research.md          # Phase 0 の出力（R-701〜R-704）
├── data-model.md         # Phase 1 の出力（新規エンティティなし）
├── quickstart.md        # Phase 1 の出力（ST-701〜703、DT-701）
├── contracts/
│   └── ui-contract.md   # Phase 1 の出力（005・006 からの差分のみ）
├── checklists/
│   └── requirements.md  # spec 品質チェックリスト（全項目合格）
└── tasks.md             # Phase 2 の出力（/speckit-tasks が作成。本コマンドでは作らない）
```

### Source Code（リポジトリのルート）

```text
MFCApplication1/
├── MFCApplication1Dlg.cpp   # 変更するファイル
│                            #   - OnInitDialog 内の着色2行を ApplyCaptionColor() に切り出す
│                            #   - OnBnClickedButtonBrowse で DoModal() を呼ぶ直前と直後の両方で
│                            #     ApplyCaptionColor() を呼ぶ
├── MFCApplication1Dlg.h     # 変更するファイル
│                            #   - ApplyCaptionColor() の非公開宣言を1行追加
├── MFCApplication1.rc       # 変更しない
├── Resource.h               # 変更しない
├── MFCApplication1.vcxproj  # 変更しない
├── ProcessRunner.*          # 変更しない
├── TargetFolderState.*      # 変更しない
├── GitignoreWriter.*        # 変更しない
├── SetupRunner.*            # 変更しない
└── res/gitignore_all.txt    # 変更しない
```

**Structure Decision**: 既存の平坦な構成をそのまま維持する。着色処理は元々 `OnInitDialog` の中に直書きされていたが、呼び出し箇所が3箇所になるため、状態を持たない非公開メンバ関数として括り出す。新しいファイル・新しいクラスは作らない（[research.md](./research.md) R-703）。

## 実装方針の要点

1. **`ApplyCaptionColor()` は状態を持たない**。005 の2定数（ネイビー・白）を使い、`DwmSetWindowAttribute` を2回呼ぶだけの純粋な処理とする。戻り値は引き続き判定しない（005 FR-408 を継承）
2. **呼び出し箇所は3つに限定する**。`OnInitDialog`（既存、置き換え）、`OnBnClickedButtonBrowse` で `CFolderPickerDialog::DoModal()` を呼ぶ**直前**（新規）、同じく **返った直後**（新規）。`DoModal()` の戻り値が `IDOK` かどうかに関わらず、直前・直後のどちらも必ず呼ぶ — キャンセルで閉じた場合も色が既定に戻っている可能性があるため（FR-701 は「開いている間も含めて常に」を要求する）
3. **`OnSize` などには一切触れない**。005 の禁止事項（[contracts/ui-contract.md](../005-dark-blue-caption/contracts/ui-contract.md) 2章）を継承する。今回追加する契機は「フォルダ選択ダイアログを開閉する」という1点のみ
4. **フォルダ未選択時の早期 return より前に置く**。`OnBnClickedButtonBrowse` は `dialog.DoModal() != IDOK` の分岐で早期 return するが、この分岐の**前**に `ApplyCaptionColor()` を呼び、キャンセル時にも確実に実行されるようにする

## 未解決のリスク

| 項目 | 内容 | 対応 |
|---|---|---|
| **根本原因が未特定** | [research.md](./research.md) R-701 のとおり、自動操作では不具合を再現できなかった。今回の修正は症状に対する対症療法であり、原因が別にある場合は効果が無い可能性がある | 動的テスト DT-701 を利用者の実機で手動実行し、実際に直っているかを確認する。直っていなければ、原因の再調査（例: 利用者の操作手順をより詳細にヒアリングする）が必要になる |
