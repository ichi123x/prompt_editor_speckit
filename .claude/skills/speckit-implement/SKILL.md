---
name: "speckit-implement"
description: "Execute the implementation plan by processing and executing all tasks defined in tasks.md"
argument-hint: "Optional implementation guidance or task filter"
compatibility: "Requires spec-kit project structure with .specify/ directory"
metadata:
  author: "github-spec-kit"
  source: "templates/commands/implement.md"
user-invocable: true
disable-model-invocation: false
---


## User Input

```text
$ARGUMENTS
```

ユーザー入力がある場合は、それを検討してから進めなければなりません。

## Pre-Execution Checks（事前実行チェック）

**拡張フックの確認（実装の前）**:
- プロジェクトルートに `.specify/extensions.yml` が存在するか確認する。
- 存在する場合は読み込み、`hooks.before_implement` キー配下のエントリを探す
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

1. リポジトリのルートから `.specify/scripts/powershell/check-prerequisites.ps1 -Json -RequireTasks -IncludeTasks` を実行し、FEATURE_DIR と AVAILABLE_DOCS のリストをパースする。すべてのパスは絶対パスでなければならない。"I'm Groot" のような引数中のシングルクォートには、エスケープ構文を使用すること: 例 'I'\''m Groot'（可能であればダブルクォートを使用: "I'm Groot"）。

2. **チェックリストの状態を確認する**（`FEATURE_DIR/checklists/` が存在する場合）:
   - チェックリストのマーカーは読み取り専用のゲートとして扱う: チェックボックスの状態をスキャンし、状態を報告し、必要な場合は進める前に確認を求める。チェックリストファイルやマーカーを変更しては**いけない**
   - `checklists/requirements.md` は `/speckit-specify` と `/speckit-clarify` が管理するビルトインのspec品質チェックリストであり、`/speckit-checklist` が生成するカスタムチェックリストはレビュアーが所有する要件品質レビューの成果物である
   - カスタムチェックリストにおいて `[x]` は、レビュアーが要件品質の基準が満たされていると判断したことを意味する。実装作業が完了したことを意味する**わけではない**
   - checklists/ ディレクトリ内のすべてのチェックリストファイルをスキャンする
   - 各チェックリストについて、以下をカウントする:
     - 項目総数: `- [ ]` または `- [X]` または `- [x]` に一致するすべての行
     - チェック済み項目: `- [X]` または `- [x]` に一致する行
     - 未チェック項目: `- [ ]` に一致する行
   - 状態テーブルを作成する:

     ```text
     | Checklist | Total | Checked | Unchecked | Status |
     |-----------|-------|---------|-----------|--------|
     | ux.md     | 12    | 12      | 0         | ✓ PASS |
     | test.md   | 8     | 5       | 3         | ✗ FAIL |
     | security.md | 6   | 6       | 0         | ✓ PASS |
     ```

   - 全体の状態を計算する:
     - **PASS**: すべてのチェックリストの未チェック項目が0件
     - **FAIL**: 1つ以上のチェックリストに未チェック項目がある

   - **いずれかのチェックリストに未チェック項目がある場合**:
     - 未チェック項目数を含むテーブルを表示する
     - **停止し**、次のように尋ねる: 「一部のチェックリストに未チェック項目があります。それでも実装を進めますか？（yes/no）」
     - 続行する前にユーザーの回答を待つ
     - ユーザーが「no」「wait」「stop」と言った場合は実行を停止する
     - ユーザーが「yes」「proceed」「continue」と言った場合はステップ3に進む

   - **すべてのチェックリストがチェック済みの場合**:
     - すべてのチェックリストが合格したことを示すテーブルを表示する
     - 自動的にステップ3に進む

3. 実装コンテキストをロードして分析する:
   - **必須**: 完全なタスクリストと実行計画について tasks.md を読む
   - **必須**: 技術スタック、アーキテクチャ、ファイル構造について plan.md を読む
   - **存在する場合**: エンティティと関係について data-model.md を読む
   - **存在する場合**: API仕様とテスト要件について contracts/ を読む
   - **存在する場合**: 技術的決定と制約について research.md を読む
   - **存在する場合**: ガバナンス制約について .specify/memory/constitution.md を読む
   - **存在する場合**: 統合シナリオについて quickstart.md を読む

4. **プロジェクトセットアップの検証**:
   - **必須**: 実際のプロジェクトセットアップに基づき、ignoreファイルを作成/確認する:

   **検出と作成のロジック**:
   - 以下のコマンドが成功するかどうかでリポジトリがgitリポジトリか判断する（成功する場合は .gitignore を作成/確認する）:

     ```sh
     git rev-parse --git-dir 2>/dev/null
     ```

   - Dockerfile* が存在する、またはplan.mdにDockerの記載がある → .dockerignore を作成/確認する
   - .eslintrc* が存在する → .eslintignore を作成/確認する
   - eslint.config.* が存在する → 設定の `ignores` エントリが必要なパターンをカバーしていることを確認する
   - .prettierrc* が存在する → .prettierignore を作成/確認する
   - .npmrc または package.json が存在する → （公開する場合）.npmignore を作成/確認する
   - terraformファイル（*.tf）が存在する → .terraformignore を作成/確認する
   - .helmignore が必要（helmチャートが存在する） → .helmignore を作成/確認する

   **既存のignoreファイルがある場合**: 必須パターンが含まれているか確認し、欠けている重要なパターンのみを追加する
   **ignoreファイルがない場合**: 検出された技術の完全なパターンセットで新規作成する

   **技術別の一般的なパターン**（plan.mdの技術スタックから）:
   - **Node.js/JavaScript/TypeScript**: `node_modules/`、`dist/`、`build/`、`*.log`、`.env*`
   - **Python**: `__pycache__/`、`*.pyc`、`.venv/`、`venv/`、`dist/`、`*.egg-info/`
   - **Java**: `target/`、`*.class`、`*.jar`、`.gradle/`、`build/`
   - **C#/.NET**: `bin/`、`obj/`、`*.user`、`*.suo`、`packages/`
   - **Go**: `*.exe`、`*.test`、`vendor/`、`*.out`
   - **Ruby**: `.bundle/`、`log/`、`tmp/`、`*.gem`、`vendor/bundle/`
   - **PHP**: `vendor/`、`*.log`、`*.cache`、`*.env`
   - **Rust**: `target/`、`debug/`、`release/`、`*.rs.bk`、`*.rlib`、`*.prof*`、`.idea/`、`*.log`、`.env*`
   - **Kotlin**: `build/`、`out/`、`.gradle/`、`.idea/`、`*.class`、`*.jar`、`*.iml`、`*.log`、`.env*`
   - **C++**: `build/`、`bin/`、`obj/`、`out/`、`*.o`、`*.so`、`*.a`、`*.exe`、`*.dll`、`.idea/`、`*.log`、`.env*`
   - **C**: `build/`、`bin/`、`obj/`、`out/`、`*.o`、`*.a`、`*.so`、`*.exe`、`*.dll`、`autom4te.cache/`、`config.status`、`config.log`、`.idea/`、`*.log`、`.env*`
   - **Swift**: `.build/`、`DerivedData/`、`*.swiftpm/`、`Packages/`
   - **R**: `.Rproj.user/`、`.Rhistory`、`.RData`、`.Ruserdata`、`*.Rproj`、`packrat/`、`renv/`
   - **共通**: `.DS_Store`、`Thumbs.db`、`*.tmp`、`*.swp`、`.vscode/`、`.idea/`

   **ツール固有のパターン**:
   - **Docker**: `node_modules/`、`.git/`、`Dockerfile*`、`.dockerignore`、`*.log*`、`.env*`、`coverage/`
   - **ESLint**: `node_modules/`、`dist/`、`build/`、`coverage/`、`*.min.js`
   - **Prettier**: `node_modules/`、`dist/`、`build/`、`coverage/`、`package-lock.json`、`yarn.lock`、`pnpm-lock.yaml`
   - **Terraform**: `.terraform/`、`*.tfstate*`、`*.tfvars`、`.terraform.lock.hcl`
   - **Kubernetes/k8s**: `*.secret.yaml`、`secrets/`、`.kube/`、`kubeconfig*`、`*.key`、`*.crt`

5. tasks.md の構造をパースし、以下を抽出する:
   - **タスクフェーズ**: Setup、Tests、Core、Integration、Polish
   - **タスクの依存関係**: 逐次実行か並行実行かのルール
   - **タスクの詳細**: ID、説明、ファイルパス、並行マーカー [P]
   - **実行フロー**: 順序と依存関係の要件

6. タスクプランに従って実装を実行する:
   - **フェーズごとの実行**: 各フェーズを完了してから次に進む
   - **依存関係を尊重する**: 逐次タスクは順序通りに実行し、並行タスク [P] は一緒に実行できる
   - **TDDアプローチに従う**: 対応する実装タスクの前にテストタスクを実行する
   - **ファイル単位の調整**: 同じファイルに影響するタスクは逐次実行しなければならない
   - **検証チェックポイント**: 次に進む前に各フェーズの完了を確認する

7. 実装実行のルール:
   - **まずセットアップ**: プロジェクト構造、依存関係、設定を初期化する
   - **コードの前にテスト**: コントラクト、エンティティ、統合シナリオのテストを書く必要があれば書く
   - **コア開発**: モデル、サービス、CLIコマンド、エンドポイントを実装する
   - **統合作業**: データベース接続、ミドルウェア、ロギング、外部サービス
   - **仕上げと検証**: 単体テスト、パフォーマンス最適化、ドキュメント作成

8. 進捗のトラッキングとエラー処理:
   - 各タスク完了後に進捗を報告する
   - 並行不可のタスクが失敗した場合は実行を停止する
   - 並行タスク [P] については、成功したタスクを続行し、失敗したものを報告する
   - デバッグ用のコンテキストを含む明確なエラーメッセージを提供する
   - 実装を進められない場合は次のステップを提案する
   - **重要** 完了したタスクについては、tasksファイル内で `[X]` としてマークすること

9. 完了の検証:
   - すべての必須タスクが完了していることを確認する
   - 実装された機能が元の仕様と一致していることを確認する
   - テストが合格し、カバレッジが要件を満たしていることを検証する
   - 実装が技術計画に従っていることを確認する

注記: このコマンドは、tasks.md に完全なタスク分解が存在することを前提としています。タスクが不完全または欠けている場合は、まず `/speckit-tasks` を実行してタスクリストを再生成することを提案してください。

## Mandatory Post-Execution Hooks（必須の事後実行フック）

**ユーザーに完了を報告する前に、このセクションを完了しなければなりません。**

プロジェクトルートに `.specify/extensions.yml` が存在するか確認する。
- 存在しない場合、または `hooks.after_implement` の下にフックが登録されていない場合は、Completion Report にスキップする。
- 存在する場合は読み込み、`hooks.after_implement` キー配下のエントリを探す。
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

完了した作業のサマリーとともに最終ステータスを報告する。

## Done When

- [ ] tasks.md のすべてのタスクが完了し `[X]` としてマークされている
- [ ] 実装がspec・plan・テストカバレッジに対して検証されている
- [ ] 上記の Mandatory Post-Execution Hooks のルールに従って拡張フックが発行またはスキップされている
- [ ] 完了した作業のサマリーとともに、完了がユーザーに報告されている
