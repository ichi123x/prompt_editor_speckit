# Implementation Plan: リポジトリ登録の要否を選択できるようにする

**Branch**: `016-repo-registration-toggle` | **Date**: 2026-09-17 | **Spec**: [spec.md](./spec.md)

**Input**: `/specs/016-repo-registration-toggle/spec.md` からの機能仕様

**注記**: このテンプレートは `/speckit-plan` コマンドによって記入されます。その定義には実行ワークフローが記載されています。

## Summary（概要）

「生成」ボタンの右側に「リポジトリの登録：」ラベルと「OK」/「NG」ラジオボタン（既定「OK」）を追加する。「OK」選択時は001〜015の動作を完全に維持する。「NG」選択時は、手順5「Git 初期化と初回コミット」を実行せず、手順6は「CLAUDE.md の生成」のみを行い（`/init`は実行するが`git add`/`git commit`は行わない）、結果一覧の項目名・内容メッセージもそれに応じて変わる（FR-1601〜FR-1607）。手順1〜4（uv・specify CLI・VS Code拡張・Spec Kit足場）は選択に関わらず不変。

技術的アプローチは、既存の`SetupRunner`の依存関係チェック（`DependencyFailed()`が`StepResult::Skipped`を「依存充足」として扱う既存の意味論）をそのまま利用し、依存関係グラフ自体（`StepId`・`dependsOn`）には手を入れない（[research.md](./research.md) R-1601）。`CreateSteps()`・`RunAll()`・`RunInitGit()`・`RunGenerateClaudeMd()`に`bool registerRepo`引数を追加し、`OnBnClickedButtonGenerate()`で選択を読み取って`m_workerPath`と対称の`m_workerRegisterRepo`にコピーする（004以来のワーカースレッド分離パターンを踏襲）。新規コントロールは生成ボタン右側の既存の空きスペース（486 DLU）に収まるため、ダイアログの寸法変更は不要（R-1604）。

## Technical Context（技術コンテキスト）

**Language/Version**: C++17（MFC、既存コードベースを継承）

**Primary Dependencies**: MFC（Visual Studio 2022、既存）。新規の外部ライブラリ・パッケージの追加なし

**Storage**: N/A

**Testing**: 自動テストスイート無し（憲章の技術的制約）。`specs/016-repo-registration-toggle/quickstart.md`の静的テストST-1601〜1603・動的テストDT-1601〜1606で検証

**Target Platform**: Windows 11（開発機実DPI 115）。015で確立した100%〜150%のDPI保証範囲に影響しない（新規コントロールは既存の空きスペースに収まる）

**Project Type**: desktop-app（既存のMFCダイアログアプリ、単一プロジェクト構成）

**Performance Goals**: N/A（UI操作と条件分岐の追加のみ。実行時間に影響する処理の追加なし）

**Constraints**:
- 依存関係グラフ（`StepId`・`dependsOn`）は変更しない（[research.md](./research.md) R-1601、YAGNI）
- 廃止済みリソースID（1002・1003・1005）は再利用しない（`Resource.h`の既存規約）
- 新規コントロールはダイアログの寸法変更を伴わずに配置する（既存の空きスペースを利用）

**Scale/Scope**: 変更対象は`.rc`（新規3コントロール追加）、`Resource.h`（新規3ID）、`SetupRunner.h`/`SetupRunner.cpp`（4関数へのbool引数追加、`RunInitGit`/`RunGenerateClaudeMd`内の分岐追加）、`MFCApplication1Dlg.h`/`MFCApplication1Dlg.cpp`（新規メンバ1つ、`OnInitDialog`・`OnBnClickedButtonGenerate`・`SetControlsEnabled`・`InitAnchors`への追記）。既存の001〜015の文言・処理ロジック本体は変更しない（FR-1602）

## Constitution Check（憲法チェック）

*ゲート: Phase 0 のリサーチ前に通過する必要がある。Phase 1 の設計後に再チェックすること。*

| 原則 | 評価 |
|---|---|
| **I. Spec Kit ワークフロー厳守** | 適合。`/speckit-specify`（NEEDS CLARIFICATIONをユーザーに確認し解決済み）→`/speckit-plan`の順で実施中 |
| **II. 日本語での一貫性** | 適合。新規追加する文字列（ラベル・行の項目名・内容メッセージ）はすべて日本語。ラジオボタンのキャプション「OK」/「NG」は英字だが、既存の`IDOK`ボタンのキャプション「OK」と同様にこのアプリで既に使われている表記であり、ユーザーが明示的に指定・確認した文字列である |
| **III. シンプルさ優先（YAGNI）** | 適合。依存関係グラフの再設計・選択状態を表す専用クラスの導入等、要求されていない抽象化は行わない（[research.md](./research.md) R-1601, R-1607のAlternatives consideredで明示的に却下） |
| **IV. 変更の追跡可能性** | 適合。`.rc`・`SetupRunner.cpp`・`MFCApplication1Dlg.cpp`の追加・変更箇所にFR-1601〜FR-1609のコメントを付与する（tasks.mdで具体化） |
| **V. 検証は実測に基づく** | 適合。新規コントロールのDLU配置はDPI相当フォントでの実測（[research.md](./research.md) R-1604）に基づく。列幅への影響も実測で確認する（quickstart.md DT-1605） |

**Phase 1設計後の再評価**: 上表の判定に変更なし。`data-model.md`・`contracts/ui-contract.md`・`quickstart.md`の作成により具体化されたのみで、新たな憲法上の懸念は生じていない。

## Project Structure（プロジェクト構造）

### Documentation（このフィーチャー）

```text
specs/016-repo-registration-toggle/
├── plan.md              # このファイル（/speckit-plan コマンドの出力）
├── research.md          # Phase 0 の出力（/speckit-plan コマンド）
├── data-model.md         # Phase 1 の出力（/speckit-plan コマンド）
├── quickstart.md         # Phase 1 の出力（/speckit-plan コマンド）
├── contracts/
│   └── ui-contract.md   # Phase 1 の出力（/speckit-plan コマンド）
├── checklists/
│   └── requirements.md  # /speckit-specify の出力（全項目合格）
└── tasks.md              # Phase 2 の出力（/speckit-tasks コマンド - /speckit-plan では作成されない）
```

### Source Code（リポジトリのルート）

```text
MFCApplication1/
├── MFCApplication1.rc         # 変更対象: 新規3コントロール（ラベル・ラジオボタン2個）を追加
├── Resource.h                  # 変更対象: 新規リソースID 3個（1009〜1011）を追加
├── SetupRunner.h                # 変更対象: CreateSteps()・RunAll() に bool 引数を追加
├── SetupRunner.cpp              # 変更対象: RunInitGit()・RunGenerateClaudeMd() に分岐を追加
├── MFCApplication1Dlg.h        # 変更対象: m_workerRegisterRepo メンバを追加
└── MFCApplication1Dlg.cpp      # 変更対象: OnInitDialog・OnBnClickedButtonGenerate・
                                 #           SetControlsEnabled・InitAnchors に追記
```

**Structure Decision**: 既存の単一MFCプロジェクト構成（`MFCApplication1/`）をそのまま使用する。新規クラス・新規ファイルは追加しない。既存の`SetupRunner`（手順実行）と`MFCApplication1Dlg`（UI結線）の責務分担をそのまま維持し、両者への差分変更のみで完結する。

## Complexity Tracking（複雑さの追跡）

該当なし。Constitution Checkで違反は検出されなかった。
