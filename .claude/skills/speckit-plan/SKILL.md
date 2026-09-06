---
name: "speckit-plan"
description: "Execute the implementation planning workflow using the plan template to generate design artifacts."
argument-hint: "Optional guidance for the planning phase"
compatibility: "Requires spec-kit project structure with .specify/ directory"
metadata:
  author: "github-spec-kit"
  source: "templates/commands/plan.md"
user-invocable: true
disable-model-invocation: false
---


## User Input

```text
$ARGUMENTS
```

ユーザー入力がある場合は、それを検討してから進めなければなりません。

## Pre-Execution Checks（事前実行チェック）

**拡張フックの確認（プランニングの前）**:
- プロジェクトルートに `.specify/extensions.yml` が存在するか確認する。
- 存在する場合は読み込み、`hooks.before_plan` キー配下のエントリを探す
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

1. **セットアップ**: リポジトリのルートから `.specify/scripts/powershell/setup-plan.ps1 -Json` を実行し、FEATURE_SPEC、IMPL_PLAN、SPECS_DIR、BRANCH のJSONをパースする。"I'm Groot" のような引数中のシングルクォートには、エスケープ構文を使用すること: 例 'I'\''m Groot'（可能であればダブルクォートを使用: "I'm Groot"）。

2. **コンテキストのロード**: FEATURE_SPEC と `.specify/memory/constitution.md` を読む。IMPL_PLAN テンプレート（すでにコピー済み）をロードする。

3. **プランワークフローの実行**: IMPL_PLAN テンプレートの構造に従って以下を行う:
   - Technical Context を記入する（不明点は「NEEDS CLARIFICATION」とマークする）
   - 憲法から Constitution Check セクションを記入する
   - ゲートを評価する（正当化されない違反があればERROR）
   - Phase 0: research.md を生成する（すべてのNEEDS CLARIFICATIONを解決する）
   - Phase 1: data-model.md、contracts/、quickstart.md を生成する
   - 設計後にConstitution Checkを再評価する

## Mandatory Post-Execution Hooks（必須の事後実行フック）

**ユーザーに完了を報告する前に、このセクションを完了しなければなりません。**

プロジェクトルートに `.specify/extensions.yml` が存在するか確認する。
- 存在しない場合、または `hooks.after_plan` の下にフックが登録されていない場合は、Completion Report にスキップする。
- 存在する場合は読み込み、`hooks.after_plan` キー配下のエントリを探す。
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

コマンドはPhase 1のデザインの後に終了する。ブランチ、IMPL_PLANのパス、生成された成果物を報告する。

## Phases

### Phase 0: Outline & Research

1. **上記の Technical Context から不明点を抽出する**:
   - NEEDS CLARIFICATION ごとに → リサーチタスク
   - 依存関係ごとに → ベストプラクティスタスク
   - 統合ごとに → パターンタスク

2. **リサーチエージェントを生成・派遣する**:

   ```text
   Technical Context の各不明点について:
     Task: "{feature context} 向けに {unknown} をリサーチする"
   各技術選定について:
     Task: "{domain} における {tech} のベストプラクティスを見つける"
   ```

3. **所見を以下の形式で `research.md` に統合する**:
   - Decision（決定）: [何が選択されたか]
   - Rationale（理由）: [なぜ選択されたか]
   - Alternatives considered（検討した代替案）: [他に何が評価されたか]

**Output**: すべてのNEEDS CLARIFICATIONが解決された research.md

### Phase 1: Design & Contracts

**前提条件:** `research.md` が完了していること

1. **フィーチャーspecからエンティティを抽出する** → `data-model.md`:
   - エンティティ名、フィールド、関係
   - 要件からのバリデーションルール
   - 該当する場合は状態遷移

2. **インターフェースコントラクトを定義する**（プロジェクトに外部インターフェースがある場合） → `/contracts/`:
   - プロジェクトがユーザーや他システムに公開しているインターフェースを特定する
   - プロジェクトタイプに適したコントラクト形式を文書化する
   - 例: ライブラリの場合は公開API、CLIツールの場合はコマンドスキーマ、Webサービスの場合はエンドポイント、パーサーの場合は文法、アプリケーションの場合はUIコントラクト
   - プロジェクトが純粋に内部向け（ビルドスクリプト、単発ツールなど）の場合はスキップする

3. **クイックスタート検証ガイドを作成する** → `quickstart.md`:
   - フィーチャーがエンドツーエンドで動作することを証明する、実行可能な検証シナリオを文書化する
   - 前提条件、セットアップコマンド、テスト/実行コマンド、期待される結果を含める
   - コントラクトやデータモデルの詳細を複製するのではなく、リンクや参照を使用する
   - 完全な実装コード、モデル/サービス/コントローラの本体、マイグレーション、完全なテストスイートは含めない
   - この成果物は検証/実行ガイドとして維持すること。実装の詳細は tasks.md と実装フェーズに属する

**Output**: data-model.md、/contracts/*、quickstart.md

## Key rules（主要なルール）

- ファイルシステム操作には絶対パスを、ドキュメント内の参照にはプロジェクト相対パスを使用する
- ゲートの失敗や未解決の明確化事項があればERRORとする

## Done When

- [ ] プランワークフローが実行され、設計成果物が生成されている
- [ ] 上記の Mandatory Post-Execution Hooks のルールに従って拡張フックが発行またはスキップされている
- [ ] ブランチ、planのパス、生成された成果物とともに、完了がユーザーに報告されている
