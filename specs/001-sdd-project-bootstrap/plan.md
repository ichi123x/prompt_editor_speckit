# Implementation Plan: SDDプロジェクトのベース作成ツール

**Branch**: `ver.0.2` | **Date**: 2026-09-08 | **Spec**: [spec.md](./spec.md)

**Input**: `/specs/001-sdd-project-bootstrap/spec.md` からの機能仕様

## Summary（概要）

既存の `MFCApplication1` のダイアログを拡張し、ユーザーが VS2022 のウィザードで生成・配置済みのプロジェクト（MFCアプリ／Webアプリ）を、GUI操作だけで SDD の土台を持った状態にする。ツールは雛形を一切持たず、生成もしない。

技術的アプローチは、外部プロセスの実行結果を積み上げる素直な構成とする。`CreateProcess` にパイプを接続して `uv`・`specify`・`code`・`git` を直接起動し、終了コードで成否を判定する。PowerShell を介在させないことで、実行ポリシーによるブロックを構造的に回避する。処理はワーカースレッドで実行し、`PostMessage` で1手順ずつ結果をリストへ追加する。`.gitignore` は埋め込みリソースとし、`specify init` がオフラインで完結する（リサーチで確認）ことと合わせて、ネットワーク不通でも Spec Kit 足場の配置と Git 初期化までは到達できる設計とする。

## Technical Context（技術コンテキスト）

**Language/Version**: C++（MSVC v143 ツールセット、Unicode 文字セット）

**Primary Dependencies**: MFC（動的リンク）。外部依存の追加なし。実行時に外部コマンド `uv` / `specify` / `code` / `git` / `winget` を起動する

**Storage**: N/A（永続ストレージを持たない。状態はダイアログの生存期間中のみメモリ上に保持する）

**Testing**: 自動テストランナーは導入しない。静的テスト ST-001〜007（MSBuild の警告検査、Visual Studio のコード分析、リソース・文字列・生成物のファイル検査）と動的テスト DT-001〜009（ダイアログ操作）を手順として実施する。手順は [quickstart.md](./quickstart.md) に記載

**Target Platform**: Windows 11（Windows 10 でも動作しうるが検証対象外）

**Project Type**: desktop-app（MFC ダイアログベースアプリケーション）

**Performance Goals**: N/A。導入処理は外部インストーラの所要時間に支配されるため、本フィーチャー固有の性能目標は設けない。ただし処理中もダイアログが応答を保つこと（ワーカースレッド化）は必須

**Constraints**:
- ビルドは MSBuild のみ。`Debug`/`Release` × `x64`/`Win32` の4構成すべてが通ること
- 新規の PowerShell スクリプト（`.ps1`）を追加しない
- 診断処理は対象フォルダへの書き込みを一切行わない（一時ファイル含む）
- 対象プロジェクトの雛形を保持も生成もしない

**Scale/Scope**: 1画面、8コントロール、5つのセットアップ手順、7つの診断項目。同時に扱う対象フォルダは常に1つ

## Constitution Check（憲法チェック）

*ゲート: Phase 0 のリサーチ前に通過する必要がある。Phase 1 の設計後に再チェックすること。*

### Phase 0 前の評価

| 原則 | 評価 | 根拠 |
|---|---|---|
| I. Spec Kit ワークフロー厳守 | PASS | `/speckit-constitution`（憲法 v1.0.0 を配置）→ `/speckit-specify` → `/speckit-plan` の順で進行中。段階を飛ばしていない |
| II. 日本語での一貫性 | PASS | 設計成果物・UI文字列・コメントをすべて日本語とする。UIコントラクトにメッセージ文字列を日本語で定義済み |
| III. シンプルさ優先（YAGNI） | PASS | インターフェース・ファクトリ・抽象基底クラスを導入しない。具象クラスのみで構成する。手順と診断項目は固定配列で持ち、プラグイン機構を設けない |
| IV. 変更の追跡可能性 | PASS | 各関数に対応する FR 番号をコメントで記載する（静的テスト ST-003 で検査） |
| 技術的制約: MSBuild のみ | PASS | CMake/Makefile を導入しない |
| 技術的制約: PowerShell スクリプトのみ | PASS | 新規スクリプトを追加しない。外部コマンドは `CreateProcess` で直接起動する |
| 技術的制約: テストスイート不在 | PASS | テストランナーを新規導入しない。静的・動的テストは手順として定義し、既存ツール（MSBuild、VS のコード分析）のみで実施する |

**ゲート結果**: 違反なし。Phase 0 に進む。

### Phase 1 設計後の再評価

| 原則 | 評価 | 設計を経ての確認 |
|---|---|---|
| I. Spec Kit ワークフロー厳守 | PASS | research.md → data-model.md → contracts/ → quickstart.md を順に生成した。tasks.md は `/speckit-tasks` に委ねる |
| II. 日本語での一貫性 | PASS | [ui-contract.md](./contracts/ui-contract.md) の「4. メッセージ文字列の契約」に9件のメッセージと表示文字列をすべて日本語で定義。ST-005 で検査する |
| III. シンプルさ優先（YAGNI） | PASS | 下記「設計上の簡素化」を参照 |
| IV. 変更の追跡可能性 | PASS | UIコントラクトの各コントロール・各操作フローに対応 FR を明記した。データモデルの各判定にも FR/R 番号を対応づけた |
| 技術的制約 | PASS | 設計で新たな依存関係・ビルドシステム・スクリプトを追加していない |

**設計上の簡素化（原則III への適合）**:
- 手順結果（FR-011）と診断結果（FR-019）は、いずれも「名称／状態／補足」の3列構造であるため、**1つのリストコントロールを再利用**する。専用コントロールを2つ設けない
- `SetupStep` と `ReadinessItem` は共通の基底クラスを作らない。3行程度の類似コードは、時期尚早な共通化より許容する（原則III の明文）
- `.gitignore` の埋め込みリソースに自動更新機構を設けない。上流の更新が必要になったらリソースを差し替える

**ゲート結果**: 違反なし。Complexity Tracking への記載事項なし。

## Project Structure（プロジェクト構造）

### Documentation（このフィーチャー）

```text
specs/001-sdd-project-bootstrap/
├── plan.md              # このファイル
├── spec.md              # 機能仕様（FR-001〜022、ST-001〜007、DT-001〜009）
├── research.md          # Phase 0 の出力（R-001〜009）
├── data-model.md        # Phase 1 の出力
├── quickstart.md        # Phase 1 の出力（検証手順）
├── contracts/
│   └── ui-contract.md   # Phase 1 の出力（UI・外部コマンド・生成物の契約）
├── checklists/
│   └── requirements.md  # 仕様品質チェックリスト
└── tasks.md             # Phase 2 の出力（/speckit-tasks が作成。未作成）
```

### Source Code（リポジトリのルート）

```text
MFCApplication1/
├── MFCApplication1.sln
├── MFCApplication1.vcxproj        # 新規ファイルとリソースの登録を追記
├── MFCApplication1.rc             # ダイアログに8コントロールを追加、RCDATA を2件追加
├── Resource.h                     # 新規コントロールIDを追加
├── MFCApplication1Dlg.h / .cpp    # ダイアログ本体。イベントハンドラと画面更新
├── ProcessRunner.h / .cpp         # 新規: CreateProcess + パイプによる外部コマンド実行
├── TargetFolderState.h / .cpp     # 新規: 対象フォルダの状態取得
├── SetupRunner.h / .cpp           # 新規: 5手順の実行（ワーカースレッド）
├── ReadinessChecker.h / .cpp      # 新規: 7項目の診断（読み取り専用）
├── GitignoreWriter.h / .cpp       # 新規: .gitignore の生成と追記ブロックのべき等な適用
└── res/
    ├── gitignore_vs.txt           # 新規: Visual Studio 用 .gitignore（埋め込みリソース）
    └── gitignore_node.txt         # 新規: Node 用 .gitignore（埋め込みリソース）
```

**Structure Decision**: 既存の `MFCApplication1/` に、責務ごとの具象クラスを平坦に追加する。サブディレクトリを切らないのは、ファイル数が6組程度に収まり、既存プロジェクトがフラット構成だからである（既存の慣習を踏襲）。

責務の分割は、テスト容易性ではなく**再利用の実態**に基づく。`ProcessRunner` は5手順と診断の両方から呼ばれ、`TargetFolderState` は導入処理と診断の両方が参照する唯一の事実源である。一方 `SetupRunner`（書き込みあり・ワーカースレッド）と `ReadinessChecker`（読み取り専用・UIスレッド）は、副作用の有無が根本的に異なるため分離する。この分離は FR-020（診断は読み取り専用）を構造で担保する意味を持つ。

## Complexity Tracking（複雑さの追跡）

Constitution Check に違反はないため、記載事項なし。
