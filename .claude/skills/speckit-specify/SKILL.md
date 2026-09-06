---
name: "speckit-specify"
description: "Create or update the feature specification from a natural language feature description."
argument-hint: "Describe the feature you want to specify"
compatibility: "Requires spec-kit project structure with .specify/ directory"
metadata:
  author: "github-spec-kit"
  source: "templates/commands/specify.md"
user-invocable: true
disable-model-invocation: false
---


## User Input

```text
$ARGUMENTS
```

ユーザー入力がある場合は、それを検討してから進めなければなりません。

## Pre-Execution Checks（事前実行チェック）

**拡張フックの確認（specify実行の前）**:
- プロジェクトルートに `.specify/extensions.yml` が存在するか確認する。
- 存在する場合は読み込み、`hooks.before_specify` キー配下のエントリを探す
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

トリガーとなったメッセージ内で、ユーザーが `/speckit-specify` の後に入力したテキスト**が**フィーチャーの説明です。以下に `$ARGUMENTS` が文字通り現れていても、この会話の中でそれが常に利用可能であると想定してください。ユーザーが空のコマンドを送った場合を除き、繰り返し入力するよう求めないでください。

そのフィーチャーの説明に基づいて、以下を行います:

1. **簡潔な短い名前を生成する**（2〜4語）フィーチャーのための:
   - フィーチャーの説明を分析し、最も意味のあるキーワードを抽出する
   - フィーチャーの本質を捉えた2〜4語の短い名前を作成する
   - 可能な限り「動詞-名詞」形式を使用する（例: "add-user-auth"、"fix-payment-bug"）
   - 技術用語や頭字語（OAuth2、API、JWTなど）を保持する
   - 一目でフィーチャーを理解できる程度に簡潔かつ説明的にする
   - 例:
     - 「ユーザー認証を追加したい」 → "user-auth"
     - 「APIにOAuth2統合を実装する」 → "oauth2-api-integration"
     - 「アナリティクス用のダッシュボードを作成する」 → "analytics-dashboard"
     - 「支払い処理のタイムアウトバグを修正する」 → "fix-payment-timeout"

2. **ブランチの作成**（任意、フック経由）:

   上記の Pre-Execution Checks で `before_specify` フックが正常に実行された場合、それはgitブランチを作成/切り替え、`BRANCH_NAME` と `FEATURE_NUM` を含むJSONを出力しています。これらの値は参考のためメモしておきますが、ブランチ名はspecディレクトリ名を規定する**わけではありません**。

   ユーザーが `GIT_BRANCH_NAME` を明示的に指定した場合、それをフックに渡し、ブランチスクリプトがその正確な値をブランチ名として使うようにする（すべての接頭辞/接尾辞生成をバイパスする）。

3. **specフィーチャーディレクトリの作成**:

   ユーザーが明示的に `SPECIFY_FEATURE_DIRECTORY` を提供しない限り、specはデフォルトの `specs/` ディレクトリ以下に置かれます。

   **`SPECIFY_FEATURE_DIRECTORY` の解決順序**:
   1. ユーザーが（環境変数、引数、設定などを通じて）明示的に `SPECIFY_FEATURE_DIRECTORY` を提供した場合、それをそのまま使用する
   2. そうでない場合は、`specs/` 以下に自動生成する:
      - `.specify/init-options.json` の `feature_numbering`（推奨）または `branch_numbering`（非推奨、移行専用 — 将来のリリースで削除予定）を確認する
      - `"timestamp"` の場合: 接頭辞は `YYYYMMDD-HHMMSS`（現在のタイムスタンプ）
      - `"sequential"` または未指定の場合: 接頭辞は `NNN`（`specs/` 内の既存ディレクトリをスキャンした後の次の利用可能な3桁の番号）
      - ディレクトリ名を構築する: `<prefix>-<short-name>`（例: `003-user-auth` または `20260319-143022-user-auth`）
      - `SPECIFY_FEATURE_DIRECTORY` を `specs/<directory-name>` に設定する
      - `feature_numbering` が未指定で `branch_numbering` が使われた場合、一行の警告を出す: 「⚠️ init-options.json の `branch_numbering` は非推奨です。`feature_numbering` に名称変更してください。」

   **ディレクトリとspecファイルの作成**:
   - `mkdir -p SPECIFY_FEATURE_DIRECTORY`
   - Spec Kit のプリセット/テンプレート解決スタック（`specify preset resolve spec-template` と同等）を通じてアクティブな `spec-template` を解決する
   - 解決された `spec-template` ファイルを、開始点として `SPECIFY_FEATURE_DIRECTORY/spec.md` にコピーする
   - `SPEC_FILE` を `SPECIFY_FEATURE_DIRECTORY/spec.md` に設定する
   - 解決されたパスを `.specify/feature.json` に永続化する:
     ```json
     {
       "feature_directory": "<resolved feature dir>"
     }
     ```
     文字列 `SPECIFY_FEATURE_DIRECTORY` そのものではなく、解決された実際のディレクトリパスの値を書くこと（例: `specs/003-user-auth`）。
     これにより、後続のコマンド（`/speckit-plan`、`/speckit-tasks` など）が、gitブランチ名の慣例に頼らずにフィーチャーディレクトリを見つけられるようになる。

   **重要**:
   - `/speckit-specify` の1回の呼び出しにつき、作成するフィーチャーは1つだけでなければならない
   - specディレクトリ名とgitブランチ名は独立している — 同じにすることもできるが、それはユーザーの選択次第である
   - specディレクトリとファイルは常にこのコマンドが作成し、フックが作成することは決してない

4. 必須セクションを理解するため、解決されたアクティブな `spec-template` ファイルをロードする。

5. **存在する場合**: プロジェクトの原則とガバナンス制約について `.specify/memory/constitution.md` をロードする。

6. 以下の実行フローに従う:
    1. 引数からユーザーの説明をパースする
       空の場合: ERROR「フィーチャーの説明が提供されていません」
    2. 説明から主要な概念を抽出する
       特定するもの: アクター、アクション、データ、制約
    3. 不明確な点について:
       - コンテキストと業界標準に基づいて合理的な推測を行う
       - 以下の場合にのみ `[NEEDS CLARIFICATION: specific question]` でマークする:
         - その選択がフィーチャーのスコープやユーザー体験に大きく影響する
         - 異なる含意を持つ複数の合理的な解釈が存在する
         - 合理的なデフォルトが存在しない
       - **上限: 合計最大3つの [NEEDS CLARIFICATION] マーカー**
       - 影響度で明確化事項を優先順位付けする: スコープ > セキュリティ/プライバシー > ユーザー体験 > 技術的詳細
    4. User Scenarios & Testing セクションを記入する
       明確なユーザーフローがない場合: ERROR「ユーザーシナリオを判断できません」
    5. Functional Requirements を生成する
       各要件はテスト可能でなければならない
       未指定の詳細には合理的なデフォルトを使用する（Assumptionsセクションに前提を文書化する）
    6. Success Criteria を定義する
       測定可能で技術に依存しない成果を作成する
       定量的指標（時間、パフォーマンス、量）と定性的指標（ユーザー満足度、タスク完了率）の両方を含める
       各基準は実装の詳細を知らなくても検証可能でなければならない
    7. （データが関わる場合）Key Entities を特定する
    8. 戻り値: SUCCESS（specがプランニングの準備完了）

7. テンプレート構造を使ってSPEC_FILEにspecを書く。セクションの順序と見出しは維持したまま、プレースホルダーをフィーチャーの説明（引数）から得た具体的な詳細に置き換える。

8. **Specification Quality Validation**: 初期specを書いた後、それを品質基準に照らして検証する:

   a. **Spec Quality Checklist の作成**: 以下の検証項目を持つ、チェックリストテンプレートの構造を使って `SPECIFY_FEATURE_DIRECTORY/checklists/requirements.md` にチェックリストファイルを生成する:

      ```markdown
      # Specification Quality Checklist: [FEATURE NAME]

      **Purpose**: プランニングに進む前に、仕様の完全性と品質を検証する
      **Created**: [DATE]
      **Feature**: [spec.mdへのリンク]

      ## Content Quality

      - [ ] 実装の詳細（言語、フレームワーク、API）が含まれていない
      - [ ] ユーザー価値とビジネスニーズに焦点を当てている
      - [ ] 非技術者のステークホルダー向けに書かれている
      - [ ] すべての必須セクションが記入されている

      ## Requirement Completeness

      - [ ] [NEEDS CLARIFICATION] マーカーが残っていない
      - [ ] 要件はテスト可能で曖昧さがない
      - [ ] 成功基準は測定可能である
      - [ ] 成功基準は技術に依存しない（実装の詳細がない）
      - [ ] すべての受け入れシナリオが定義されている
      - [ ] エッジケースが特定されている
      - [ ] スコープが明確に境界付けられている
      - [ ] 依存関係と前提事項が特定されている

      ## Feature Readiness

      - [ ] すべての機能要件に明確な受け入れ基準がある
      - [ ] ユーザーシナリオが主要なフローをカバーしている
      - [ ] フィーチャーはSuccess Criteriaで定義された測定可能な成果を満たしている
      - [ ] 実装の詳細がspecに漏れ出していない

      ## Notes

      - 不完全とマークされた項目は、`/speckit-clarify` や `/speckit-plan` の前にspecの更新が必要
      ```

   b. **検証チェックの実行**: チェックリストの各項目についてspecをレビューする:
      - 各項目について、合格か不合格かを判断する
      - 見つかった具体的な問題を文書化する（specの関連セクションを引用する）

   c. **検証結果の処理**:

      - **すべての項目が合格した場合**: チェックリストを完了とマークし、Mandatory Post-Execution Hooks セクションに進む

      - **項目が不合格の場合（[NEEDS CLARIFICATION]を除く）**:
        1. 不合格の項目と具体的な問題を列挙する
        2. 各問題に対処するようspecを更新する
        3. すべての項目が合格するまで検証を再実行する（最大3回まで）
        4. 3回試行しても不合格が残る場合は、残っている問題をチェックリストの注記に記録し、ユーザーに警告する

      - **[NEEDS CLARIFICATION] マーカーが残っている場合**:
        1. specからすべての [NEEDS CLARIFICATION: ...] マーカーを抽出する
        2. **上限チェック**: 3つを超えるマーカーが存在する場合は、（スコープ/セキュリティ/UXへの影響度で）最も重要な3つのみを残し、残りは合理的な推測を行う
        3. 必要な明確化事項（最大3つ）ごとに、以下の形式で選択肢をユーザーに提示する:

           ```markdown
           ## Question [N]: [Topic]

           **Context**: [specの関連セクションを引用]

           **What we need to know**: [NEEDS CLARIFICATIONマーカーからの具体的な質問]

           **Suggested Answers**:

           | Option | Answer | Implications |
           |--------|--------|--------------|
           | A      | [1つ目の提案する回答] | [このフィーチャーにとっての意味] |
           | B      | [2つ目の提案する回答] | [このフィーチャーにとっての意味] |
           | C      | [3つ目の提案する回答] | [このフィーチャーにとっての意味] |
           | Custom | 独自の回答を提供する | [独自の入力を提供する方法の説明] |

           **Your choice**: _[ユーザーの回答を待つ]_
           ```

        4. **重要 - テーブルのフォーマット**: Markdownのテーブルが正しくフォーマットされていることを確認する:
           - パイプを揃えた一貫した間隔を使う
           - 各セルは内容の前後にスペースを入れる: `|Content|` ではなく `| Content |`
           - ヘッダー区切りは少なくとも3つのダッシュを持たなければならない: `|--------|`
           - Markdownプレビューでテーブルが正しく表示されることをテストする
        5. 質問には順番に番号を振る（Q1、Q2、Q3 - 最大3つ）
        6. すべての質問を、回答を待つ前にまとめて提示する
        7. ユーザーがすべての質問に対する選択を返信するまで待つ（例: 「Q1: A、Q2: Custom - [details]、Q3: B」）
        8. 各 [NEEDS CLARIFICATION] マーカーを、ユーザーが選択または提供した回答に置き換えてspecを更新する
        9. すべての明確化が解決された後、検証を再実行する

   d. **チェックリストの更新**: 各検証の反復の後、現在の合否状態でチェックリストファイルを更新する

## Mandatory Post-Execution Hooks（必須の事後実行フック）

**ユーザーに完了を報告する前に、このセクションを完了しなければなりません。**

プロジェクトルートに `.specify/extensions.yml` が存在するか確認する。
- 存在しない場合、または `hooks.after_specify` の下にフックが登録されていない場合は、Completion Report にスキップする。
- 存在する場合は読み込み、`hooks.after_specify` キー配下のエントリを探す。
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

以下を含めて完了をユーザーに報告する:
- `SPECIFY_FEATURE_DIRECTORY` — フィーチャーディレクトリのパス
- `SPEC_FILE` — specファイルのパス
- チェックリスト結果のサマリー
- 次のフェーズ（`/speckit-clarify` または `/speckit-plan`）への準備状況

**注記:** ブランチの作成は `before_specify` フック（gitエクステンション）によって処理されます。specディレクトリとファイルの作成は常にこのコアコマンドが処理します。

## Quick Guidelines

- ユーザーが**何を**必要としていて、**なぜ**必要なのかに焦点を当てる。
- **どう**実装するかは避ける（技術スタック、API、コード構造なし）。
- 開発者ではなく、ビジネスステークホルダー向けに書く。
- specに埋め込まれたチェックリストは作成**しない**こと。それは別のコマンドになる。

### Section Requirements

- **必須セクション**: すべてのフィーチャーで記入されなければならない
- **任意セクション**: フィーチャーに関連する場合のみ含める
- セクションが該当しない場合は「N/A」として残すのではなく、完全に削除する

### For AI Generation

ユーザーのプロンプトからこのspecを作成する際:

1. **合理的な推測をする**: コンテキスト、業界標準、一般的なパターンを使ってギャップを埋める
2. **前提事項を文書化する**: Assumptionsセクションに合理的なデフォルトを記録する
3. **明確化事項を制限する**: 最大3つの [NEEDS CLARIFICATION] マーカー - 以下に該当する重要な決定にのみ使用する:
   - フィーチャーのスコープやユーザー体験に大きく影響する
   - 異なる含意を持つ複数の合理的な解釈がある
   - 合理的なデフォルトを欠く
4. **明確化事項を優先順位付けする**: スコープ > セキュリティ/プライバシー > ユーザー体験 > 技術的詳細
5. **テスターのように考える**: 曖昧な要件はすべて「テスト可能で曖昧さがない」チェックリスト項目に不合格になるべき
6. **明確化が必要になりがちな領域**（合理的なデフォルトが存在しない場合のみ）:
   - フィーチャーのスコープと境界（特定のユースケースを含める/除外する）
   - ユーザータイプと権限（複数の矛盾する解釈がありうる場合）
   - セキュリティ/コンプライアンス要件（法的/財務的に重要な場合）

**合理的なデフォルトの例**（これらについて尋ねないこと）:

- データ保持: ドメインにおける業界標準の慣行
- パフォーマンス目標: 特に指定がなければ標準的なWeb/モバイルアプリの想定
- エラー処理: 適切なフォールバックを伴うユーザーフレンドリーなメッセージ
- 認証方式: Webアプリなら標準的なセッションベースまたはOAuth2
- 統合パターン: プロジェクトに適したパターンを使用する（Webサービスなら REST/GraphQL、ライブラリなら関数呼び出し、ツールなら CLI引数など）

### Success Criteria Guidelines

Success Criteriaは以下でなければならない:

1. **測定可能**: 具体的な指標（時間、パーセンテージ、件数、割合）を含む
2. **技術に依存しない**: フレームワーク、言語、データベース、ツールへの言及がない
3. **ユーザー中心**: システム内部ではなく、ユーザー/ビジネスの視点からの成果を説明する
4. **検証可能**: 実装の詳細を知らなくてもテスト/検証できる

**良い例**:

- 「ユーザーは3分以内にチェックアウトを完了できる」
- 「システムは1万人の同時ユーザーをサポートする」
- 「95%の検索は1秒以内に結果を返す」
- 「タスク完了率が40%向上する」

**悪い例**（実装に焦点を当てている）:

- 「APIのレスポンスタイムが200ms未満」（技術的すぎる。「ユーザーは即座に結果を見る」を使う）
- 「データベースは1000TPSを処理できる」（実装の詳細。ユーザー向けの指標を使う）
- 「Reactコンポーネントが効率的にレンダリングされる」（フレームワーク固有）
- 「Redisのキャッシュヒット率が80%以上」（技術固有）

## Done When

- [ ] specが `SPEC_FILE` に書かれ、品質チェックリストに対して検証されている
- [ ] 上記の Mandatory Post-Execution Hooks のルールに従って拡張フックが発行またはスキップされている
- [ ] フィーチャーディレクトリ、specファイルのパス、チェックリスト結果とともに、完了がユーザーに報告されている
