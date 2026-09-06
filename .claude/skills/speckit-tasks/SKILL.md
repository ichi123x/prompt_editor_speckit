---
name: "speckit-tasks"
description: "Generate an actionable, dependency-ordered tasks.md for the feature based on available design artifacts."
argument-hint: "Optional task generation constraints"
compatibility: "Requires spec-kit project structure with .specify/ directory"
metadata:
  author: "github-spec-kit"
  source: "templates/commands/tasks.md"
user-invocable: true
disable-model-invocation: false
---


## User Input

```text
$ARGUMENTS
```

ユーザー入力がある場合は、それを検討してから進めなければなりません。

## Pre-Execution Checks（事前実行チェック）

**拡張フックの確認（タスク生成の前）**:
- プロジェクトルートに `.specify/extensions.yml` が存在するか確認する。
- 存在する場合は読み込み、`hooks.before_tasks` キー配下のエントリを探す
- YAMLの解析に失敗する場合や不正な場合は、フックのチェックを黙ってスキップし、通常通り続行する
- `enabled` が明示的に `false` であるフックは除外する。`enabled` フィールドがないフックはデフォルトで有効として扱う
- 残りの各フックについて、フックの `condition` 式を解釈・評価しようとしては**いけない**:
  - フックに `condition` フィールドがない、またはそれが null/空である場合、そのフックは実行可能として扱う
  - フックが空でない `condition` を定義している場合、そのフックはスキップし、条件の評価は HookExecutor の実装に委ねる
- フックコマンド名からコマンド呼び出しを構築する際は、ドット（`.`）をハイフン（`-`）に置き換える。例: `speckit.git.commit` → `/speckit-git-commit`。
- 実行可能な各フックについて、`optional` フラグに基づいて以下を出力する:
  - **オプションフック** (`optional: true`):
    ```
    ## Extension Hooks

    **Optional Pre-Hook**: {extension}
    Command: `/{command}`
    Description: {description}

    Prompt: {prompt}
    To execute: `/{command}`
    ```
  - **必須フック** (`optional: false`):
    ```
    ## Extension Hooks

    **Automatic Pre-Hook**: {extension}
    Executing: `/{command}`
    EXECUTE_COMMAND: {command}

    Outline に進む前にフックコマンドの結果を待つこと。
    ```
    上のブロックを出力した後、実際にそのフックを呼び出し、完了を待ってから続行しなければなりません。このエージェント/セッションで自分自身がコマンドを実行するのと同じ方法で実行してください（呼び出し方法は上記の文字通りの `{command}` id とは異なる場合があります。例えばスキルモードのエージェントでは `/skill:speckit-...` や `$speckit-...` として実行されます）。上記のブロックを出力するだけではフックは実行されません。
- フックが登録されていない、または `.specify/extensions.yml` が存在しない場合は、黙ってスキップする

## Outline

1. **セットアップ**: リポジトリのルートから `.specify/scripts/powershell/setup-tasks.ps1 -Json` を実行し、FEATURE_DIR、TASKS_TEMPLATE_CONTENT、TASKS_TEMPLATE、AVAILABLE_DOCS のリストをパースする。`FEATURE_DIR` と `TASKS_TEMPLATE` は提供される場合、絶対パスでなければならない。`AVAILABLE_DOCS` は、FEATURE_DIR 以下で利用可能なドキュメントの名前/相対パスのリストである（例: `research.md` や `contracts/`）。"I'm Groot" のような引数中のシングルクォートには、エスケープ構文を使用すること: 例 'I'\''m Groot'（可能であればダブルクォートを使用: "I'm Groot"）。

2. **設計ドキュメントのロード**: FEATURE_DIR から読み込む:
   - **必須**: plan.md（技術スタック、ライブラリ、構造）、spec.md（優先度付きのユーザーストーリー）
   - **任意**: data-model.md（エンティティ）、contracts/（インターフェースコントラクト）、research.md（決定事項）、quickstart.md（テストシナリオ）
   - **存在する場合**: プロジェクトの原則とガバナンス制約について `.specify/memory/constitution.md` をロードする
   - 注記: すべてのプロジェクトがすべてのドキュメントを持っているわけではない。利用可能なものに基づいてタスクを生成する。

3. **タスク生成ワークフローの実行**:
   - plan.md をロードし、技術スタック、ライブラリ、プロジェクト構造を抽出する
   - spec.md をロードし、優先度（P1、P2、P3など）付きのユーザーストーリーを抽出する
   - data-model.md が存在する場合: エンティティを抽出し、ユーザーストーリーにマッピングする
   - contracts/ が存在する場合: インターフェースコントラクトをユーザーストーリーにマッピングする
   - research.md が存在する場合: セットアップタスクのための決定事項を抽出する
   - ユーザーストーリーごとにタスクを生成する（下記のTask Generation Rulesを参照）
   - ユーザーストーリーの完了順序を示す依存関係グラフを生成する
   - ユーザーストーリーごとの並行実行例を作成する
   - タスクの完全性を検証する（各ユーザーストーリーが必要なタスクをすべて持ち、独立してテスト可能であること）

4. **tasks.md の生成**: （上記のJSON出力からの）TASKS_TEMPLATE_CONTENT を構造として使用する。TASKS_TEMPLATE_CONTENT を省略する古いセットアップスクリプトとの互換性のため、代わりにTASKS_TEMPLATEを読む。以下を記入する:
   - plan.md からの正しいフィーチャー名
   - Phase 1: Setup タスク（プロジェクトの初期化）
   - Phase 2: Foundational タスク（すべてのユーザーストーリーをブロックする前提条件）
   - Phase 3+: ユーザーストーリーごとに1フェーズ（spec.mdの優先度順）
   - 各フェーズには以下を含める: ストーリーの目標、独立したテスト基準、テスト（要求された場合）、実装タスク
   - Final Phase: 仕上げと横断的関心事
   - すべてのタスクは厳密なチェックリスト形式に従わなければならない（下記のTask Generation Rulesを参照）
   - 各タスクの明確なファイルパス
   - ストーリーの完了順序を示すDependenciesセクション
   - ストーリーごとの並行実行例
   - 実装戦略セクション（まずMVP、段階的な提供）

## Mandatory Post-Execution Hooks（必須の事後実行フック）

**ユーザーに完了を報告する前に、このセクションを完了しなければなりません。**

プロジェクトルートに `.specify/extensions.yml` が存在するか確認する。
- 存在しない場合、または `hooks.after_tasks` の下にフックが登録されていない場合は、Completion Report にスキップする。
- 存在する場合は読み込み、`hooks.after_tasks` キー配下のエントリを探す。
- YAMLの解析に失敗する場合や不正な場合は、フックのチェックを黙ってスキップし、Completion Report に進む。
- `enabled` が明示的に `false` であるフックは除外する。`enabled` フィールドがないフックはデフォルトで有効として扱う。
- 残りの各フックについて、フックの `condition` 式を解釈・評価しようとしては**いけない**:
  - フックに `condition` フィールドがない、またはそれが null/空である場合、そのフックは実行可能として扱う
  - フックが空でない `condition` を定義している場合、そのフックはスキップし、条件の評価は HookExecutor の実装に委ねる
- フックコマンド名からコマンド呼び出しを構築する際は、ドット（`.`）をハイフン（`-`）に置き換える。例: `speckit.git.commit` → `/speckit-git-commit`。
- 実行可能な各フックについて、`optional` フラグに基づいて以下を出力する:
  - **必須フック** (`optional: false`) — **必須フックごとに `EXECUTE_COMMAND:` を出力しなければならない**:
    ```
    ## Extension Hooks

    **Automatic Hook**: {extension}
    Executing: `/{command}`
    EXECUTE_COMMAND: {command}
    ```
    上のブロックを出力した後、実際にそのフックを呼び出し、完了を待ってから続行しなければなりません。このエージェント/セッションで自分自身がコマンドを実行するのと同じ方法で実行してください（呼び出し方法は上記の文字通りの `{command}` id とは異なる場合があります。例えばスキルモードのエージェントでは `/skill:speckit-...` や `$speckit-...` として実行されます）。上記のブロックを出力するだけではフックは実行されません。
  - **オプションフック** (`optional: true`):
    ```
    ## Extension Hooks

    **Optional Hook**: {extension}
    Command: `/{command}`
    Description: {description}

    Prompt: {prompt}
    To execute: `/{command}`
    ```

## Completion Report

生成された tasks.md へのパスとサマリーを出力する:
- タスクの総数
- ユーザーストーリーごとのタスク数
- 特定された並行実行の機会
- 各ストーリーの独立したテスト基準
- 推奨されるMVPスコープ（通常は User Story 1 のみ）
- フォーマットの検証: すべてのタスクがチェックリスト形式（チェックボックス、ID、ラベル、ファイルパス）に従っていることを確認する

タスク生成のためのコンテキスト: $ARGUMENTS

tasks.mdは即座に実行可能でなければならない - 各タスクは、追加のコンテキストなしにLLMが完了できる程度に具体的でなければならない。

## Task Generation Rules（タスク生成のルール）

**重要**: タスクは、各ストーリーの独立した実装とテストを可能にするため、ユーザーストーリーごとに整理**しなければならない**。

**テストは任意**: テストタスクは、フィーチャー仕様で明示的に要求されている場合、またはユーザーがTDDアプローチを要求した場合にのみ生成する。

### Checklist Format（必須）

すべてのタスクは、この形式に厳密に従わなければならない:

```text
- [ ] [TaskID] [P?] [Story?] Description with file path
```

**形式の構成要素**:

1. **チェックボックス**: 常に `- [ ]`（Markdownチェックボックス）で始める
2. **タスクID**: 実行順の連番（T001、T002、T003...）
3. **[P] マーカー**: タスクが並行実行可能な場合（異なるファイル、未完了のタスクへの依存なし）にのみ含める
4. **[Story] ラベル**: ユーザーストーリーフェーズのタスクにのみ必須
   - 形式: [US1]、[US2]、[US3]など（spec.mdのユーザーストーリーに対応）
   - Setupフェーズ: ストーリーラベルなし
   - Foundationalフェーズ: ストーリーラベルなし
   - User Storyフェーズ: ストーリーラベルが必須
   - Polishフェーズ: ストーリーラベルなし
5. **説明**: 正確なファイルパスを伴う明確なアクション

**例**:

- ✅ 正しい: `- [ ] T001 Create project structure per implementation plan`
- ✅ 正しい: `- [ ] T005 [P] Implement authentication middleware in src/middleware/auth.py`
- ✅ 正しい: `- [ ] T012 [P] [US1] Create User model in src/models/user.py`
- ✅ 正しい: `- [ ] T014 [US1] Implement UserService in src/services/user_service.py`
- ❌ 誤り: `- [ ] Create User model`（IDとStoryラベルが欠けている）
- ❌ 誤り: `T001 [US1] Create model`（チェックボックスが欠けている）
- ❌ 誤り: `- [ ] [US1] Create User model`（タスクIDが欠けている）
- ❌ 誤り: `- [ ] T001 [US1] Create model`（ファイルパスが欠けている）

### Task Organization（タスクの整理）

1. **ユーザーストーリー（spec.md）から** - 主要な整理軸:
   - 各ユーザーストーリー（P1、P2、P3...）は独自のフェーズを持つ
   - 関連するすべてのコンポーネントをそのストーリーにマッピングする:
     - そのストーリーに必要なモデル
     - そのストーリーに必要なサービス
     - そのストーリーに必要なインターフェース/UI
     - テストが要求されている場合: そのストーリー固有のテスト
   - ストーリーの依存関係をマークする（ほとんどのストーリーは独立しているべき）

2. **コントラクトから**:
   - 各インターフェースコントラクトを、それが提供するユーザーストーリーにマッピングする
   - テストが要求されている場合: 各インターフェースコントラクトについて、そのストーリーのフェーズにおける実装の前にコントラクトテストタスク [P] を作成する

3. **データモデルから**:
   - 各エンティティを、それを必要とするユーザーストーリーにマッピングする
   - エンティティが複数のストーリーに使われる場合: 最も早いストーリーかSetupフェーズに含める
   - 関係 → 適切なストーリーフェーズにおけるサービス層のタスク

4. **セットアップ/インフラから**:
   - 共有インフラ → Setupフェーズ（Phase 1）
   - 基盤/ブロッキングタスク → Foundationalフェーズ（Phase 2）
   - ストーリー固有のセットアップ → そのストーリーのフェーズ内

### Phase Structure（フェーズ構造）

- **Phase 1**: Setup（プロジェクトの初期化）
- **Phase 2**: Foundational（ブロッキングとなる前提条件 - すべてのユーザーストーリーの前に完了しなければならない）
- **Phase 3+**: 優先度順のユーザーストーリー（P1、P2、P3...）
  - 各ストーリー内: テスト（要求された場合）→ モデル → サービス → エンドポイント → 統合
  - 各フェーズは完全で独立してテスト可能な増分であるべき
- **Final Phase**: 仕上げと横断的関心事

## Done When

- [ ] tasks.md が、すべてのフェーズ、タスクID、ファイルパスとともに生成されている
- [ ] 上記の Mandatory Post-Execution Hooks のルールに従って拡張フックが発行またはスキップされている
- [ ] タスク数、ストーリーごとの内訳、MVPスコープとともに、完了がユーザーに報告されている
