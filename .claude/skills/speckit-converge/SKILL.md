---
name: "speckit-converge"
description: "Assess the current codebase against the feature's spec, plan, and tasks, then append any remaining unbuilt work as new tasks to tasks.md so implement can complete it."
compatibility: "Requires spec-kit project structure with .specify/ directory"
metadata:
  author: "github-spec-kit"
  source: "templates/commands/converge.md"
user-invocable: true
disable-model-invocation: false
---


## User Input

```text
$ARGUMENTS
```

ユーザー入力がある場合は、それを検討してから進めなければなりません。

## Pre-Execution Checks（事前実行チェック）

**拡張フックの確認（収束の前）**:

- プロジェクトルートに `.specify/extensions.yml` が存在するか確認する。
- 存在する場合は読み込み、`hooks.before_converge` キー配下のエントリを探す
- YAMLの解析に失敗する場合や不正な場合は、フックのチェックを黙ってスキップし、通常通り続行する
- `enabled` が明示的に `false` であるフックは除外する。`enabled` フィールドがないフックはデフォルトで有効として扱う
- 残りの各フックについて、フックの `condition` 式を解釈・評価しようとしては**いけない**:
  - フックに `condition` フィールドがない、またはそれが null/空である場合、そのフックは実行可能として扱う
  - フックが空でない `condition` を定義している場合、そのフックはスキップし、条件の評価は HookExecutor の実装に委ねる
- フックコマンド名からコマンド呼び出しを構築する際は、ドット（`.`）をハイフン（`-`）に置き換える。例: `speckit.git.commit` → `/speckit-git-commit`。
- 実行可能な各フックについて、`optional` フラグに基づいて以下を出力する:
  - **オプションフック** (`optional: true`):

    ```text
    ## Extension Hooks

    **Optional Pre-Hook**: {extension}
    Command: `/{command}`
    Description: {description}

    Prompt: {prompt}
    To execute: `/{command}`
    ```

  - **必須フック** (`optional: false`):

    ```text
    ## Extension Hooks

    **Automatic Pre-Hook**: {extension}
    Executing: `/{command}`
    EXECUTE_COMMAND: {command}

    Goal に進む前にフックコマンドの結果を待つこと。
    ```
    上のブロックを出力した後、実際にそのフックを呼び出し、完了を待ってから続行しなければなりません。このエージェント/セッションで自分自身がコマンドを実行するのと同じ方法で実行してください（呼び出し方法は上記の文字通りの `{command}` id とは異なる場合があります。例えばスキルモードのエージェントでは `/skill:speckit-...` や `$speckit-...` として実行されます）。上記のブロックを出力するだけではフックは実行されません。

- フックが登録されていない、または `.specify/extensions.yml` が存在しない場合は、黙ってスキップする

## Goal（目的）

フィーチャーのspec・plan・tasksが要求する内容と、コードベースが現在実装している内容とのギャップを埋める。`spec.md`、`plan.md`、`tasks.md` を**唯一の意図の情報源**として読み込み（憲法を統治的な制約として扱う）、コードの現状を評価し、どの要件・受け入れ基準・plan上の決定・既存タスクが未達・未完了・部分的にしか満たされていないかを判断し、残っている作業をそれぞれ `tasks.md` の末尾に**新しいトレーサブルなタスクとして追加**し、`/speckit-implement` がそれを完了できるようにする。このコマンドは、現在の `tasks.md` に対して `/speckit-implement` が実行された後、かつ `/speckit-tasks` が完全な `tasks.md` を生成した後にのみ実行しなければならない。

これは差分ツールでは**なく**、変更履歴を追跡することも**ない**。フィーチャーの成果物に対するコードの現在の状態を評価するものであり、gitもブランチ比較も履歴も使わない。

## Operating Constraints（運用上の制約）

**追記のみ、書き直しは絶対にしない**: このコマンドの唯一の書き込みは、`tasks.md` に新しい `## Phase N: Convergence` セクションを追記することです。以下を行っては**いけません**:

- `spec.md` や `plan.md` をいかなる形でも変更すること
- 既存のタスク（以前の Convergence フェーズのタスクを含む）を書き直す、番号を振り直す、並べ替える、削除すること
- アプリケーションコードを変更・作成・削除すること — 追記されたタスクを完了させるのは `/speckit-implement` の仕事です。

コードベードがすでにすべてを満たしている場合、このコマンドは `tasks.md` を**1バイトも変更せず**（空のConvergence見出しも作らない）維持し、クリーンな結果を報告しなければならない。

**Constitution Authority（憲法の権威）**: プロジェクトの憲法（`.specify/memory/constitution.md`）は**交渉の余地がありません**。MUST原則に違反するコードは最も重大度の高い所見であり、対応する是正タスクを生成する。憲法が未記入のテンプレートである場合は、失敗するのではなく、憲法チェックを穏当にスキップすること。

## Execution Steps（実行手順）

### 1. 収束コンテキストの初期化

リポジトリのルートから `.specify/scripts/powershell/check-prerequisites.ps1 -Json -RequireSpec -RequireTasks -IncludeTasks` を一度実行し、FEATURE_DIR と AVAILABLE_DOCS のJSONをパースする。絶対パスを導出する:

- SPEC = FEATURE_DIR/spec.md
- PLAN = FEATURE_DIR/plan.md
- TASKS = FEATURE_DIR/tasks.md
- CONSTITUTION = `.specify/memory/constitution.md`（存在する場合）

`spec.md`、`plan.md`、`tasks.md` のいずれかが欠けている場合、実行すべき前提コマンド名（specが欠けているなら `/speckit-specify`、planが欠けているなら `/speckit-plan`、tasksが欠けているなら `/speckit-tasks`）を挙げた明確で実行可能なメッセージとともに停止する。部分的な出力は生成しないこと。
"I'm Groot" のような引数中のシングルクォートには、エスケープ構文を使用すること: 例 'I'\''m Groot'（可能であればダブルクォートを使用: "I'm Groot"）。

### 2. 成果物のロード（プログレッシブ・ディスクロージャー）

各成果物から必要最小限のコンテキストのみをロードする:

**spec.md から:**

- 機能要件（FR-###）
- 成功基準（SC-###） — ビルド可能な作業を必要とする項目のみを含め、公開後の成果指標やビジネスKPIは除外する
- ユーザーストーリーとその受け入れシナリオ
- エッジケース（存在する場合）

**plan.md から:**

- アーキテクチャ/スタックの選択と技術的決定
- データモデルの参照
- フェーズと、名指しされたタッチポイント（planで作成・編集されると述べられているファイル/コンポーネント）
- 技術的制約

**tasks.md から:**

- タスクID（次のIDと次のフェーズ番号を計算するため）
- 説明、フェーズのグルーピング、参照されているファイルパス

**constitution から（未記入のテンプレートでない場合）:**

- 原則名とMUST/SHOULDの規範的記述

### 3. 意図のインベントリを構築する

内部モデルを作成する（生の成果物をそのまま出力しないこと）:

- **要件インベントリ**: FR-### / SC-### / ユーザーストーリーの受け入れシナリオ（例: `US1/AC2`）ごとに安定したキーを1つ、さらにビルド可能な義務を課すplanの決定と憲法の原則を加える。
- **コードスコープマップ**: `plan.md` と `tasks.md` に記載されたファイルパスに加え、各要件が説明する概念のキーワード検索から、評価対象となるソースファイルとコンポーネントの集合を導出する。評価はこれらに限定する — 成果物が定義する範囲を超えてスコープを推測しては**いけない**。

### 4. コードベースを評価し、所見を分類する

意図インベントリの各項目について、対象範囲の現在のコードを検査し、ギャップがある場合にのみ `Finding`（所見）を作成する。すべての所見を**ギャップの種類**によって分類する:

- **`missing`**: 必要な作業がコードにまったく存在しない。
- **`partial`**: 作業は存在するが、要件/受け入れ基準/plan上の決定をまだ完全には満たしていない。
- **`contradicts`**: コードが、明示された意図や憲法のMUST原則と矛盾することを行っている。
- **`unrequested`**: コードに、spec・plan・tasksが要求していない作業が含まれている（認識のために表面化させる — convergeはコードを削除せず、レビュー/正当化または削除するためのタスクを追記するのみ）。

各 `Finding` には以下を記録する: 安定したid、トレースする先の `source-ref`、`gap-type`、重大度、観察された証拠（ファイル/領域）を伴う短い人間可読の説明。

**エッジケース:**

- **コードがほとんど、またはまったくない**: 仕様化されたスコープ全体を、失敗としてではなく `missing` の残作業として扱う。
- **何も残っていない**: 所見をゼロとして、ステップ7の収束済みブランチに従う。

### 5. 重大度の割り当て

- **CRITICAL**: 憲法のMUST原則に違反している、またはP1ユーザーストーリーのベースライン機能をブロックする `missing`/`contradicts` のギャップ。
- **HIGH**: コアとなる機能要件や受け入れ基準における `missing` または `partial` のギャップ。
- **MEDIUM**: 副次的な要件における `partial` のギャップ、または正当化が不明確な `unrequested` の追加。
- **LOW**: 軽微な部分的ギャップ、仕上げ、または低リスクな `unrequested` の追加。

### 6. セッション内の所見サマリーを提示する

何かを追記する前に、コンパクトで重大度別のサマリーを出力する（まだファイルへの書き込みは行わない）:

## Convergence Findings

| ID | Gap Type | Severity | Source | Evidence | Remaining Work |
|----|----------|----------|--------|----------|----------------|
| F1 | missing  | HIGH     | FR-008 | 例: tasks.md書き込み時、path/to/module.py に追記専用のガードが検出されない | 追記専用の強制を追加する |

**Summary metrics（サマリー指標）:**

- 確認した要件/受け入れ基準の数
- 確認したplan上の決定の数
- 確認した憲法の原則の数（または「skipped — template」）
- ギャップの種類別の所見（missing / partial / contradicts / unrequested）
- 重大度別の所見

### 7. Convergence タスクを追記する（または収束済みと報告する）

**実行可能な所見が1件以上ある場合**（`tasks_appended` の結果）:

追記契約に従って `tasks.md` の**末尾**に追記する:

1. 既存のすべてのタスクIDをスキャンし、最大値を `M` とする。次のフェーズ番号 `N`（既存の最大フェーズ+1）を決定する。
2. 新しいセクション見出し `## Phase N: Convergence` を1つ書く。
3. 実行可能な所見1件につき1つのチェックリスト項目を、CRITICAL/HIGHを先頭にした順序で発行し、ゼロ埋めされたID `T{M+1:03d}, T{M+2:03d}, …` を割り当てる:

   ```markdown
   - [ ] T042 <命令形の説明> per <source-ref> (<gap-type>)
   ```

   `<source-ref>` はタスクをその出所にトレースする。例: `FR-003`、`SC-002`、`US1/AC2`、`plan: storage decision`、`Constitution II`。

   `<gap-type>` は `missing`、`partial`、`contradicts`、`unrequested` のいずれか。

   憲法違反のタスクは必ず最初に発行し、`CRITICAL` として記述しなければならない。
4. 既存のIDを再利用したり番号を振り直したりしない。以前のConvergenceフェーズが存在する場合は、その下に別途番号を振った新しいものを追加する — 古いものには触れないこと。

**実行可能な所見が1件もない場合**（`converged` の結果）:

- `tasks.md` を一切変更**しない** — 空のフェーズ見出しも作らない。
- 報告する: **「✅ Converged — 実装はspec・plan・tasksを満たしています。」**
- 確認した内容のサマリー件数を含める。

### 8. 次のアクションを提示する（引き継ぎ）

- `tasks_appended` の場合: どのフェーズの下に何件のタスクが追記されたかを述べ、それらを完了させるために `/speckit-implement` を実行することを推奨する。後続のconverge実行では残りの項目が減る、あるいはなくなることを注記する。
- `converged` の場合: レビューへの移行やPRのオープンを推奨する。このフィーチャーの仕様化されたスコープについては、これ以上のimplementパスは不要。

### 9. 拡張フックの確認

結果を出力した後、プロジェクトルートに `.specify/extensions.yml` が存在するか確認する。

- 存在する場合は読み込み、`hooks.after_converge` キー配下のエントリを探す
- YAMLの解析に失敗する場合や不正な場合は、フックのチェックを黙ってスキップし、通常通り続行する
- `enabled` が明示的に `false` であるフックは除外する。`enabled` フィールドがないフックはデフォルトで有効として扱う
- 残りの各フックについて、フックの `condition` 式を解釈・評価しようとしては**いけない**:
  - フックに `condition` フィールドがない、またはそれが null/空である場合、そのフックは実行可能として扱う
  - フックが空でない `condition` を定義している場合、そのフックはスキップし、条件の評価は HookExecutor の実装に委ねる
- どのフックを列挙するよりも前に、収束の結果（`converged` または `tasks_appended`）をセッション内で報告し、ユーザーが任意のフォローアップコマンドを実行するかどうかを判断できるようにする。
- フックコマンド名からコマンド呼び出しを構築する際は、ドット（`.`）をハイフン（`-`）に置き換える。例: `speckit.git.commit` → `/speckit-git-commit`。
- 実行可能な各フックについて、`optional` フラグに基づいて以下を出力する:
  - **オプションフック** (`optional: true`):

    ```text
    ## Extension Hooks

    **Optional Hook**: {extension}
    Command: `/{command}`
    Description: {description}

    Prompt: {prompt}
    To execute: `/{command}`
    ```

  - **必須フック** (`optional: false`):

    ```text
    ## Extension Hooks

    **Automatic Hook**: {extension}
    Executing: `/{command}`
    EXECUTE_COMMAND: {command}
    ```
    上のブロックを出力した後、実際にそのフックを呼び出し、完了を待ってから続行しなければなりません。このエージェント/セッションで自分自身がコマンドを実行するのと同じ方法で実行してください（呼び出し方法は上記の文字通りの `{command}` id とは異なる場合があります。例えばスキルモードのエージェントでは `/skill:speckit-...` や `$speckit-...` として実行されます）。上記のブロックを出力するだけではフックは実行されません。

- フックが登録されていない、または `.specify/extensions.yml` が存在しない場合は、黙ってスキップする
