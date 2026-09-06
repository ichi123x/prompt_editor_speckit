---
name: "speckit-analyze"
description: "Perform a non-destructive cross-artifact consistency and quality analysis across spec.md, plan.md, and tasks.md after task generation."
argument-hint: "Optional focus areas for analysis"
compatibility: "Requires spec-kit project structure with .specify/ directory"
metadata:
  author: "github-spec-kit"
  source: "templates/commands/analyze.md"
user-invocable: true
disable-model-invocation: false
---


## User Input

```text
$ARGUMENTS
```

ユーザー入力がある場合は、それを検討してから進めなければなりません。

## Pre-Execution Checks（事前実行チェック）

**拡張フックの確認（解析の前）**:
- プロジェクトルートに `.specify/extensions.yml` が存在するか確認する。
- 存在する場合は読み込み、`hooks.before_analyze` キー配下のエントリを探す
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

    Goal に進む前にフックコマンドの結果を待つこと。
    ```
    上のブロックを出力した後、実際にそのフックを呼び出し、完了を待ってから続行しなければなりません。このエージェント/セッションで自分自身がコマンドを実行するのと同じ方法で実行してください（呼び出し方法は上記の文字通りの `{command}` id とは異なる場合があります。例えばスキルモードのエージェントでは `/skill:speckit-...` や `$speckit-...` として実行されます）。上記のブロックを出力するだけではフックは実行されません。
- フックが登録されていない、または `.specify/extensions.yml` が存在しない場合は、黙ってスキップする

## Goal（目的）

実装前に、3つのコア成果物（`spec.md`、`plan.md`、`tasks.md`）の間の不整合、重複、曖昧さ、仕様不足の項目を特定する。このコマンドは `/speckit-tasks` が完全な `tasks.md` を正常に生成した後にのみ実行しなければならない。

## Operating Constraints（運用上の制約）

**厳密に読み取り専用**: いかなるファイルも変更しては**いけません**。構造化された分析レポートを出力してください。修正計画（オプション）を提示すること（後続の編集コマンドが手動で呼び出される前に、ユーザーが明示的に承認する必要がある）。

**Constitution Authority（憲法の権威）**: プロジェクトの憲法（`.specify/memory/constitution.md`）は、この分析のスコープ内では**交渉の余地がありません**。憲法との矛盾は自動的にCRITICALとなり、原則を薄める、再解釈する、あるいは黙って無視するのではなく、spec・plan・tasksの調整が必要です。原則自体を変更する必要がある場合は、`/speckit-analyze` の外で別途明示的な憲法更新として行わなければなりません。

## Execution Steps（実行手順）

### 1. 分析コンテキストの初期化

リポジトリのルートから `.specify/scripts/powershell/check-prerequisites.ps1 -Json -RequireSpec -RequireTasks -IncludeTasks` を一度実行し、FEATURE_DIR と AVAILABLE_DOCS のJSONをパースする。絶対パスを導出する:

- SPEC = FEATURE_DIR/spec.md
- PLAN = FEATURE_DIR/plan.md
- TASKS = FEATURE_DIR/tasks.md

必須ファイルが欠けている場合はエラーメッセージで中止する（不足している前提コマンドを実行するようユーザーに指示する）。
"I'm Groot" のような引数中のシングルクォートには、エスケープ構文を使用すること: 例 'I'\''m Groot'（可能であればダブルクォートを使用: "I'm Groot"）。

### 2. 成果物のロード（プログレッシブ・ディスクロージャー）

各成果物から必要最小限のコンテキストのみをロードする:

**spec.md から:**

- 概要/コンテキスト
- 機能要件
- 成功基準（測定可能な成果 — 例: パフォーマンス、セキュリティ、可用性、ユーザーの成功、ビジネスへの影響）
- ユーザーストーリー
- エッジケース（存在する場合）

**plan.md から:**

- アーキテクチャ/スタックの選択
- データモデルの参照
- フェーズ
- 技術的制約

**tasks.md から:**

- タスクID
- 説明
- フェーズのグルーピング
- 並行マーカー [P]
- 参照されているファイルパス

**constitution から:**

- 原則の検証のために `.specify/memory/constitution.md` をロードする

### 3. セマンティックモデルの構築

内部表現を作成する（出力に生の成果物を含めないこと）:

- **要件インベントリ**: 各機能要件（FR-###）と成功基準（SC-###）について、安定したキーを記録する。存在する場合は明示的な FR-/SC- 識別子を主キーとして使用し、任意で読みやすさのために命令形フレーズのスラッグも導出する（例: "User can upload file" → `user-can-upload-file`）。ビルド可能な作業を必要とする成功基準の項目のみを含め（例: 負荷テストインフラ、セキュリティ監査ツール）、公開後の成果指標やビジネスKPI（例: "サポートチケットを50%削減する"）は除外する。
- **ユーザーストーリー/アクションインベントリ**: 受け入れ基準を伴う個別のユーザーアクション
- **タスクカバレッジマッピング**: 各タスクを1つ以上の要件やストーリーにマッピングする（キーワードや、IDやキーフレーズなどの明示的な参照パターンによる推論）
- **憲法ルールセット**: 原則名とMUST/SHOULDの規範的記述を抽出する

### 4. 検出パス（トークン効率の良い分析）

シグナルの強い所見に焦点を当てる。所見は合計50件に制限し、残りはオーバーフローサマリーに集約する。

#### A. 重複の検出

- ほぼ重複している要件を特定する
- より質の低い言い回しを統合対象としてマークする

#### B. 曖昧さの検出

- 測定可能な基準を欠く曖昧な形容詞（速い、スケーラブル、安全、直感的、堅牢など）にフラグを立てる
- 未解決のプレースホルダー（TODO、TKTK、???、`<placeholder>` など）にフラグを立てる

#### C. 仕様不足

- 動詞はあるが目的語や測定可能な成果を欠く要件
- 受け入れ基準との整合が取れていないユーザーストーリー
- spec/planに定義されていないファイルやコンポーネントを参照するタスク

#### D. 憲法との整合

- MUST原則と矛盾する要件やplanの要素
- 憲法から義務付けられたセクションや品質ゲートの欠落

#### E. カバレッジのギャップ

- 関連タスクがゼロの要件
- マッピングされた要件/ストーリーがないタスク
- ビルド可能な作業を必要とする成功基準（パフォーマンス、セキュリティ、可用性）がタスクに反映されていない

#### F. 不整合

- 用語のドリフト（同じ概念がファイルごとに異なる名前で呼ばれている）
- planで参照されているがspecには存在しないデータエンティティ（またはその逆）
- タスクの順序の矛盾（例: 依存関係の注記なしに、基盤セットアップタスクより前に統合タスクがある）
- 矛盾する要件（例: 一方はNext.jsを要求し、他方はVueを指定している）

### 5. 重大度の割り当て

所見の優先順位付けには以下のヒューリスティックを使用する:

- **CRITICAL**: 憲法のMUSTに違反している、コアとなるspec成果物が欠けている、またはベースライン機能をブロックするカバレッジゼロの要件
- **HIGH**: 重複または矛盾する要件、曖昧なセキュリティ/パフォーマンス属性、テスト不可能な受け入れ基準
- **MEDIUM**: 用語のドリフト、非機能タスクのカバレッジ欠如、仕様不足のエッジケース
- **LOW**: スタイル/言い回しの改善、実行順序に影響しない軽微な冗長性

### 6. コンパクトな分析レポートの作成

以下の構造でMarkdownレポートを出力する（ファイル書き込みは行わない）:

## Specification Analysis Report

| ID | Category | Severity | Location(s) | Summary | Recommendation |
|----|----------|----------|-------------|---------|----------------|
| A1 | Duplication | HIGH | spec.md:L120-134 | 類似した2つの要件... | 言い回しを統合し、より明確な方を残す |

（所見1件につき1行を追加する。カテゴリの頭文字を接頭辞とした安定したIDを生成する。）

**Coverage Summary Table:**

| Requirement Key | Has Task? | Task IDs | Notes |
|-----------------|-----------|----------|-------|

**Constitution Alignment Issues:**（あれば）

**Unmapped Tasks:**（あれば）

**Metrics:**

- 要件の総数
- タスクの総数
- カバレッジ%（1件以上のタスクを持つ要件）
- 曖昧さの件数
- 重複の件数
- CRITICALな問題の件数

### 7. 次のアクションの提示

レポートの最後に、簡潔な Next Actions ブロックを出力する:

- CRITICALな問題が存在する場合: `/speckit-implement` の前に解決することを推奨する
- LOW/MEDIUMのみの場合: ユーザーは進めてよいが、改善案を提示する
- 明示的なコマンド案を提示する: 例: 「`/speckit-specify` を修正付きで実行する」「`/speckit-plan` を実行してアーキテクチャを調整する」「`tasks.md` を手動編集して 'performance-metrics' のカバレッジを追加する」

### 8. 是正措置の提示

ユーザーに尋ねる: 「上位N件の問題について、具体的な是正編集を提案しましょうか？」（自動的には適用しないこと。）

### 9. 拡張フックの確認

レポート後、プロジェクトルートに `.specify/extensions.yml` が存在するか確認する。
- 存在する場合は読み込み、`hooks.after_analyze` キー配下のエントリを探す
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

## Operating Principles（運用原則）

### Context Efficiency（コンテキスト効率）

- **最小限の高シグナルなトークン**: 網羅的な文書化ではなく、実行可能な所見に焦点を当てる
- **プログレッシブ・ディスクロージャー**: 成果物を段階的にロードする。すべての内容を分析にダンプしない
- **トークン効率の良い出力**: 所見テーブルは50行に制限し、残りは要約する
- **決定論的な結果**: 変更なしで再実行した場合、一貫したIDと件数を生成すること

### Analysis Guidelines（分析ガイドライン）

- **ファイルを絶対に変更しない**（これは読み取り専用の分析）
- **欠落しているセクションを絶対に捏造しない**（存在しない場合は正確に報告する）
- **憲法違反を優先する**（これらは常にCRITICAL）
- **網羅的なルールより具体例を使う**（一般的なパターンではなく、具体的な事例を引用する）
- **問題ゼロの場合も適切に報告する**（カバレッジ統計とともに成功レポートを出力する）

## Context

$ARGUMENTS
