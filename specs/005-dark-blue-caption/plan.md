# Implementation Plan: タイトルバーを濃い青にする

**Branch**: `005-dark-blue-caption` | **Date**: 2026-09-09 | **Spec**: [spec.md](./spec.md)

**Input**: `/specs/005-dark-blue-caption/spec.md` からの機能仕様

## Summary（概要）

メインダイアログのタイトルバーをネイビー `#1F3864`、タイトル文字を白で表示し、開いているウィンドウの中からこのツールを色だけで見分けられるようにする。着色できない環境では既定の外観のまま何事もなく動く。

技術的アプローチは **`DwmSetWindowAttribute` に `DWMWA_CAPTION_COLOR`（35）と `DWMWA_TEXT_COLOR`（36）を渡すだけ**である。`OnInitDialog` で1回呼び、戻り値は見ない。実測により、一度設定すれば最大化・最小化・非アクティブを経ても色が保たれること（再設定が不要であること）が確認できているため、**004 の `OnSize` / `OnGetMinMaxInfo` には一切触れない**。

実質的な変更は `MFCApplication1Dlg.cpp` の十数行のみで、新しいクラス・メンバ変数・状態は増えない。

## Technical Context（技術コンテキスト）

**Language/Version**: C++（MSVC v143、Unicode、C++17）

**Primary Dependencies**: MFC、Windows SDK 10.0.26100.0 の `dwmapi.h` / `dwmapi.lib`（新規に使用する唯一の追加要素）

**Storage**: N/A（本フィーチャーは何も永続化しない）

**Testing**: 自動テストスイートは存在しない（憲章の技術的制約）。静的テスト ST-401〜405 と動的テスト DT-401〜408 を [quickstart.md](./quickstart.md) の手順として定義し実施する

**Target Platform**: Windows 11（着色の対象）。Windows 10 以前では着色されないが動作する

**Project Type**: desktop-app（MFC ダイアログアプリケーション）

**Performance Goals**: N/A。API 呼び出し2回のみで、起動時間・描画性能に影響しない

**Constraints**: ダイアログのサイズ・コントロール・001〜004 の機能を一切変更しないこと（FR-410）。新しい `.ps1` をプロジェクトへ追加しないこと（憲章）

**Scale/Scope**: 変更するファイルは `MFCApplication1Dlg.cpp` の1ファイルのみ。追加行数は十数行

## Constitution Check（憲法チェック）

*ゲート: Phase 0 のリサーチ前に通過する必要がある。Phase 1 の設計後に再チェックすること。*

| 原則 | 判定 | 根拠 |
|---|---|---|
| **I. Spec Kit ワークフロー厳守** | 合格 | `/speckit-specify` → `/speckit-plan` の順で進めている。実装は `/speckit-tasks`・`/speckit-analyze` の後 |
| **II. 日本語での一貫性** | 合格 | 本フィーチャーは UI 文字列を追加しない。コメントと成果物はすべて日本語 |
| **III. シンプルさ優先（YAGNI）** | 合格 | 新しいクラス・メンバ変数・共通関数を作らない。着色の成否を記録する変数も持たない（[data-model.md](./data-model.md) 2章）。`GetProcAddress` による動的解決も採用しない（[research.md](./research.md) R-405）。色を選べる設定機能も作らない |
| **IV. 変更の追跡可能性** | 合格 | 追加する各箇所に FR-401〜FR-412 のコメントを付す。ST-405 が機械的に検査する |

**技術的制約との整合**:

- MSBuild のみを使用し、CMake 等は導入しない — 合格
- 新しい `.ps1` をプロジェクトへ追加しない — 合格（検証スクリプトはスクラッチパッドに置き、リポジトリには含めない）
- テストランナーを新規導入しない — 合格（ST/DT の手順として定義）

**設計後の再チェック（Phase 1 完了時点）**: 上記4原則および技術的制約について、Phase 1 の成果物（data-model.md・contracts/ui-contract.md・quickstart.md）を作成した後も**判定は変わらない**。設計の過程で抽象化や新しい依存を導入する必要は生じなかった。**違反ゼロのため Complexity Tracking セクションは記入しない。**

## Project Structure（プロジェクト構造）

### Documentation（このフィーチャー）

```text
specs/005-dark-blue-caption/
├── plan.md              # このファイル
├── research.md          # Phase 0 の出力（R-401〜R-410）
├── data-model.md        # Phase 1 の出力（新規エンティティなし）
├── quickstart.md        # Phase 1 の出力（ST-401〜405、DT-401〜408）
├── contracts/
│   └── ui-contract.md   # Phase 1 の出力（004 からの差分のみ）
├── checklists/
│   └── requirements.md  # spec 品質チェックリスト（16項目合格）
└── tasks.md             # Phase 2 の出力（/speckit-tasks が作成。本コマンドでは作らない）
```

### Source Code（リポジトリのルート）

```text
MFCApplication1/
├── MFCApplication1Dlg.cpp   # 変更する唯一のファイル
│                            #   - <dwmapi.h> の include
│                            #   - #pragma comment(lib, "dwmapi.lib")
│                            #   - 色の定数2つ
│                            #   - OnInitDialog 内の着色処理（2回の API 呼び出し）
├── MFCApplication1Dlg.h     # 変更しない（メンバも宣言も増えない）
├── MFCApplication1.rc       # 変更しない
├── Resource.h               # 変更しない
├── MFCApplication1.vcxproj  # 変更しない（#pragma comment でリンクするため）
├── ProcessRunner.*          # 変更しない
├── TargetFolderState.*      # 変更しない
├── GitignoreWriter.*        # 変更しない
├── SetupRunner.*            # 変更しない
└── res/gitignore_all.txt    # 変更しない
```

**Structure Decision**: 既存の平坦な構成をそのまま維持する。本フィーチャーは新しい責務を持つクラスを必要とする規模ではない（API 呼び出し2回）。着色処理を `CaptionStyler` のようなクラスに切り出すことも考えられるが、呼び出し元が1箇所しかなく、状態も持たないため、憲章 原則III に照らして**`OnInitDialog` に直接書く**。

## 実装方針の要点

実装時に踏み外しやすい点を、根拠とともに記す。

1. **COLORREF は `0x00BBGGRR`**。`#1F3864` は `0x0064381F`。`RGB(0x1F, 0x38, 0x64)` マクロを使えば並べ替えを間違えないため、そちらを推奨する。ST-402 が誤った並び `0x001F3864` を機械的に検出する（[research.md](./research.md) R-402）

2. **戻り値を判定しない**。`if (SUCCEEDED(...))` と書きたくなるが、書いてはならない。判定した結果として何かをするわけではなく、判定すること自体が FR-408 に反する分岐を呼び込む（R-405）

3. **再設定の処理を書かない**。`OnSize` や `OnPaint` で塗り直したくなるが不要である。DWM が保持することを実測済みで、ST-403 がこれを検査する（R-407）

4. **`OnInitDialog` の戻り値と `m_layoutReady` の順序を壊さない**。004 は `m_layoutReady` を `OnInitDialog` の**最後**に立てる契約になっている。着色処理はこれと無関係なので先頭付近に置き、末尾の構造には触れない

5. **`.vcxproj` を編集しない**。`#pragma comment(lib, "dwmapi.lib")` で足りる。4構成への書き漏れという典型的な失敗を構造的に避ける（R-406）

## 未解決のリスク

| 項目 | 内容 | 対応 |
|---|---|---|
| **DT-405（アクセントカラー設定）** | Windows の「タイトルバーにアクセントカラーを表示する」が有効なとき、本ツールの指定色が勝つかどうかが**未測定**（R-410） | 実装後に利用者の手で確認する。もし負ける場合は実装の誤りではなく**仕様の前提が崩れた**ことを意味するため、修正ではなく FR-401 の見直しを報告する |
