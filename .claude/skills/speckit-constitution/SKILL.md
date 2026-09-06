---
name: "speckit-constitution"
description: "Create or update the project constitution from interactive or provided principle inputs."
argument-hint: "Principles or values for the project constitution"
compatibility: "Requires spec-kit project structure with .specify/ directory"
metadata:
  author: "github-spec-kit"
  source: "templates/commands/constitution.md"
user-invocable: true
disable-model-invocation: false
---


## User Input

```text
$ARGUMENTS
```

ユーザー入力がある場合は、それを検討してから進めなければなりません。

## Scope Guard（スコープの防御）

このコマンド自体の作業は、プロジェクトの憲法そのものを更新することに限定されます。依存するテンプレートやコマンドは実行時に憲法を読み込みますが、ここでは変更されません。

- ユーザー入力のすべての部分を、憲法の内容か、それとは別のガバナンス以外の意図かに分類する。
- 入力に機能実装、コード生成、リファクタリング、ビルド、デプロイの要求が含まれる場合、それらを実行しては**いけません**。代わりに、それらを先送りされた意図として抽出してください。
- アプリケーションのソースファイル、フィーチャールート、コンポーネント、テスト、デプロイファイル、その他憲法ワークフローと無関係な成果物を、作成・変更・削除しては**いけません**。
- ある指示が憲法の内容かどうか不明な場合は、変更を加える前に明確化を求めること。
- 憲法の更新を完了した後、先送りされた各意図について `Next Actions` セクションを含める。元の意図を列挙し、`/speckit-specify` のような適切なフォローアップのSpec Kitコマンドを、実際には呼び出さずに提案する。
- ガバナンス以外の意図がない場合は、`Next Actions` セクションを省略する。

## Pre-Execution Checks（事前実行チェック）

**拡張フックの確認（憲法更新の前）**:
- プロジェクトルートに `.specify/extensions.yml` が存在するか確認する。
- 存在する場合は読み込み、`hooks.before_constitution` キー配下のエントリを探す
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

あなたは `.specify/memory/constitution.md` にあるプロジェクトの憲法を更新しています。アクティブな憲法の骨組みは、Spec Kit のプリセット/テンプレート解決スタックを通じて `constitution-template` からコマンド実行時に解決されます。

以下の実行フローに従ってください:

1. リポジトリのルートから `.specify/scripts/powershell/resolve-template.ps1 constitution-template -Json` を実行し、`TEMPLATE_CONTENT` をアクティブなテンプレートとしてパースする。
   - 共有のリゾルバーは、コアテンプレートのフォールバックの前に、プロジェクトのオーバーライド、プリセットレイヤーの合成、拡張レイヤーを適用します。これは続行する前に成功しなければなりません。
   - 失敗した場合は停止し、解決エラーを報告する。1つの寄与テンプレートレイヤーだけで続行しないこと。
   - `.specify/memory/constitution.md` が存在する場合は、それを現在のプロジェクト固有の値と改訂内容の情報源としてロードする。新しく解決された骨組みを適用する際、まだ適用可能な情報は保持する。
   - 存在しない場合は、解決されたテンプレートを初期ドキュメントとして使用する。
   - バージョン管理されたテンプレートレイヤーに書き戻さないこと。
   - `[ALL_CAPS_IDENTIFIER]` 形式のすべてのプレースホルダートークンを特定する。
   **重要**: ユーザーはテンプレートで使われているものより少ない、あるいは多い原則を必要とする場合があります。数が指定されている場合はそれを尊重し、一般的なテンプレートに従ってください。それに応じてドキュメントを更新します。

2. プレースホルダーの値を収集/導出する:
   - ユーザー入力（会話）が値を提供している場合は、それを使用する。
   - そうでなければ、既存のリポジトリのコンテキスト（README、ドキュメント、埋め込まれている場合は以前の憲法バージョン）から推測する。
   - ガバナンスの日付について: `RATIFICATION_DATE` は最初の採用日（不明な場合は尋ねるかTODOとしてマークする）、`LAST_AMENDED_DATE` は変更が行われた場合は今日の日付、そうでなければ以前の値を維持する。
   - `CONSTITUTION_VERSION` はセマンティックバージョニングのルールに従って増分しなければならない:
     - MAJOR: 後方互換性のないガバナンス/原則の削除や再定義。
     - MINOR: 新しい原則/セクションの追加や、ガイダンスの実質的な拡張。
     - PATCH: 明確化、言い回し、誤字修正、非意味論的な改善。
   - バージョンアップの種類が曖昧な場合は、確定する前に理由を提示する。

3. 解決されたテンプレートを必須の構造として使用し、更新後の憲法の内容を起草する:
   - すべてのプレースホルダーを具体的なテキストに置き換える（プロジェクトがまだ定義しないことを選んだ、意図的に残されたテンプレートのスロットを除き、角括弧のトークンを残さないこと — 残す場合は明示的に理由を述べる）。
   - 見出しの階層は保持し、置き換え後もまだ明確化のガイダンスを加えるものでない限り、コメントは削除してよい。
   - 各原則セクションには以下を確保する: 簡潔な名前の行、非交渉可能なルールを記載した段落（または箇条書き）、自明でない場合は明示的な理由。
   - Governance セクションには、改訂手続き、バージョニングポリシー、コンプライアンスレビューへの期待が列挙されていることを確保する。

4. Sync Impact Report を作成する（更新後、憲法ファイルの先頭にHTMLコメントとして追記する）:
   - バージョンの変更: 旧 → 新
   - 変更された原則の一覧（改名された場合は 旧タイトル → 新タイトル）
   - 追加されたセクション
   - 削除されたセクション
   - 意図的に先送りされたプレースホルダーがある場合のフォローアップTODO。

5. 最終出力前の検証:
   - 未説明の角括弧トークンが残っていないこと。
   - バージョン行がレポートと一致していること。
   - 日付はISO形式 YYYY-MM-DD であること。
   - 原則は宣言的で、テスト可能で、曖昧な言葉を含まないこと（"should" → 適切な場合はMUST/SHOULDと理由に置き換える）。

6. 完成した憲法を `.specify/memory/constitution.md` に書き戻す（上書き）。

7. 以下を含む最終サマリーをユーザーに出力する:
   - 新しいバージョンとバージョンアップの理由。
   - 手動でのフォローアップが必要なTODOプレースホルダーや先送りされた項目。
   - 推奨するコミットメッセージ（例: `docs: amend constitution to vX.Y.Z (principle additions + governance update)`）。
   - 先送りされたガバナンス以外の意図がある場合の `Next Actions` セクション。

フォーマットとスタイルの要件:

- テンプレートと全く同じMarkdown見出しを使用する（レベルを下げたり上げたりしない）。
- 読みやすさを保つため長い理由の行は折り返す（理想的には100文字未満）が、不自然な改行で無理に強制しない。
- セクション間は空行1行を保つ。
- 行末の余分な空白を避ける。

ユーザーが部分的な更新（例: 原則1つのみの改訂）を提供した場合でも、検証とバージョン決定の手順は実行すること。

重要な情報が欠けている場合（例: 制定日が本当に不明）、`TODO(<FIELD_NAME>): explanation` を挿入し、Sync Impact Report の先送り項目に含めること。

`.specify/memory/constitution.md` のみを書き込むこと。テンプレートのソースファイルは作成・変更しないこと。

## Post-Execution Checks（事後実行チェック）

**拡張フックの確認（憲法更新の後）**:
プロジェクトルートに `.specify/extensions.yml` が存在するか確認する。
- 存在する場合は読み込み、`hooks.after_constitution` キー配下のエントリを探す
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
