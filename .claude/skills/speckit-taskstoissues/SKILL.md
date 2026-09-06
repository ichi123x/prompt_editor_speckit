---
name: "speckit-taskstoissues"
description: "Convert existing tasks into actionable, dependency-ordered GitHub issues for the feature based on available design artifacts."
argument-hint: "Optional filter or label for GitHub issues"
compatibility: "Requires spec-kit project structure with .specify/ directory"
metadata:
  author: "github-spec-kit"
  source: "templates/commands/taskstoissues.md"
user-invocable: true
disable-model-invocation: false
---


## User Input

```text
$ARGUMENTS
```

ユーザー入力がある場合は、それを検討してから進めなければなりません。

## Pre-Execution Checks（事前実行チェック）

**拡張フックの確認（tasks-to-issues変換の前）**:
- プロジェクトルートに `.specify/extensions.yml` が存在するか確認する。
- 存在する場合は読み込み、`hooks.before_taskstoissues` キー配下のエントリを探す
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
1. **存在する場合**: プロジェクトの原則とガバナンス制約について `.specify/memory/constitution.md` をロードする。
1. 実行したスクリプトから、**tasks** へのパスを抽出する。
1. 以下を実行してGitリモートを取得する:

```bash
git config --get remote.origin.url
```

> [!CAUTION]
> リモートがGitHubのURLである場合にのみ、次のステップに進んでください

1. **重複排除のために既存のissueを取得する**: 何かを作成する前に、`tasks.md` から処理しようとしているタスクIDの集合を構築する（各IDは `T` に続いて**少なくとも**3桁の数字、例: `T001` — `/speckit-converge` は `T{M+1:03d}` で新しいIDを割り当てるが、これは上限ではなく下限なので、1つのファイルに1000件を超えるタスクがあるとIDは4桁以上になる）。次にGitHub MCPサーバーの `list_issues` ツールを使って、それらのIDをすでにカバーしているissueを探す。`state` の値は渡さないこと。省略することでツールがオープンとクローズ両方のissueを返すようになる。呼び出し回数を抑えるため `perPage: 100` を要求し、このツールはカーソルベースのページネーションを使うため、（前回レスポンスの `endCursor` を使って）`after` パラメータでページを要求する。各issueのタイトルについて、タスクIDのパターン `\bT\d{3,}\b` と照合する（`{3,}` は4桁以上のIDも受け入れる — `\d{3}` のままだと、末尾の `\b` が2つの数字の間には成立しないため、`T1000` を含むタイトルはまったくマッチせず、そのタスクは重複排除もされず作成もされないまま黙って見逃されてしまう。単語境界は依然として `ST001` のようなトークンがマッチするのを防ぎ、数字の並び全体を消費させることで `T100` が `T1000` の内部にマッチすることも決してない。これは `T001 ...`、`T001: ...`、`[T001] ...` のように書かれたタイトルも認識する）。マッチした場合、そのIDにはすでにissueがあるものとしてマークする。すべてのタスクIDがマッチしたら、あるいはそれ以上ページがなくなったら、ページネーションを止める。これにより、すべてのタスクIDが判明した後もリポジトリ全体のissue履歴を取得し続けることがなくなる。これによりissue履歴の多いリポジトリでの呼び出し回数が制限され、`tasks.md` が再生成されたりスキルが再度呼び出されたりした後にコマンドが再実行された場合でも重複が防止される。
1. リスト内の各タスクについて、GitHub MCPサーバーを使い、Gitリモートに対応するリポジトリに新しいissueを作成する。`tasks.md` 内のタスク行はMarkdownのチェックボックスで始まるため、まず先頭の `- [ ]`（および `[P]` / `[US#]` マーカー）を取り除いてタスクIDと説明を復元する。issueは `T001: <description>` という単一の正規のタイトル形式で作成し、IDを一度だけ書いた後にタスクの説明を続ける（例えば `- [ ] T001 Create project structure` という行は、タイトル `T001: Create project structure` になる）。
   - 前のステップで既存issueの集合にすでにIDが存在するタスクは**スキップ**し、それを報告する（例: `T001 already has an issue, skipping`）。
   - まだ対応するissueがないタスクについてのみissueを作成する。

> [!CAUTION]
> リモートURLと一致しないリポジトリにissueを作成することは、いかなる状況においても絶対に行わないでください

## Post-Execution Checks（事後実行チェック）

**拡張フックの確認（tasks-to-issues変換の後）**:
プロジェクトルートに `.specify/extensions.yml` が存在するか確認する。
- 存在する場合は読み込み、`hooks.after_taskstoissues` キー配下のエントリを探す
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

    **Optional Hook**: {extension}
    Command: `/{command}`
    Description: {description}

    Prompt: {prompt}
    To execute: `/{command}`
    ```
  - **必須フック** (`optional: false`):
    ```
    ## Extension Hooks

    **Automatic Hook**: {extension}
    Executing: `/{command}`
    EXECUTE_COMMAND: {command}
    ```
    上のブロックを出力した後、実際にそのフックを呼び出し、完了を待ってから続行しなければなりません。このエージェント/セッションで自分自身がコマンドを実行するのと同じ方法で実行してください（呼び出し方法は上記の文字通りの `{command}` id とは異なる場合があります。例えばスキルモードのエージェントでは `/skill:speckit-...` や `$speckit-...` として実行されます）。上記のブロックを出力するだけではフックは実行されません。
- フックが登録されていない、または `.specify/extensions.yml` が存在しない場合は、黙ってスキップする
