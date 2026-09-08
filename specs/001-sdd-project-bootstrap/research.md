# Phase 0 リサーチ: SDDプロジェクトのベース作成ツール

**Feature**: 001-sdd-project-bootstrap
**Date**: 2026-09-08
**検証環境**: Windows 11 Pro 10.0.26200 / Visual Studio 2022 (v143) / Spec Kit v1.0.4

本ドキュメントの調査結果は、実行環境で実際にコマンドを実行して確認した実測値に基づく。

---

## R-001: 外部コマンドの実行方式

**Decision**: `CreateProcess` に標準出力・標準エラーのパイプを接続してワーカースレッド上で同期実行し、終了コードと出力文字列を取得する。PowerShell は介在させず、各実行ファイルを直接起動する。`.cmd` ファイル（`code`）のみ `cmd.exe /c` 経由とする。

**Rationale**:
- `ShellExecute` は出力を取得できないため、手順の成否判定（FR-011）に使えない。
- PowerShell を介在させないことで、実行ポリシーによるブロック（Assumptions に記載の制約）を構造的に回避できる。「実行ポリシーを自動変更しない」という前提を、そもそも PowerShell を使わないことで満たす。
- `CreateProcess` は `lpApplicationName` を NULL にすると `lpCommandLine` の実行ファイルを PATH から解決する。これにより「PATH 上に存在するか」の判定と実行を1回の呼び出しで兼ねられる。実行ファイルが見つからない場合は `ERROR_FILE_NOT_FOUND` で失敗するため、FR-011 の「失敗」判定にそのまま対応づけられる。
- `code` は `code.cmd`（実測: `C:\Users\...\Microsoft VS Code\bin\code.cmd`）であり、`CreateProcess` は `.cmd` を直接実行できないため `cmd.exe /c code ...` とする必要がある。

**Alternatives considered**:
- *PowerShell スクリプト（.ps1）を同梱して呼び出す*: 憲法の技術的制約「Spec Kit 側のスクリプトは PowerShell のみをサポートする。bash版スクリプトを新規追加してはならない」は `.specify/scripts/` を対象とした記述であり、本ツール専用の `.ps1` 追加を禁じてはいない。しかし実行ポリシーの問題を持ち込むうえ、配布ファイルが増える。却下。
- *`system()` / `_popen()`*: コンソールウィンドウが可視化され、終了コードの取得も限定的。却下。

**実装上の注意**: パイプの読み出しをせずに `WaitForSingleObject` すると、子プロセスがパイプバッファ（既定 4KB）を埋めた時点でデッドロックする。読み出しループを回しながら終了を待つこと。

---

## R-002: UI をブロックしない実行

**Decision**: 導入処理全体を `AfxBeginThread` によるワーカースレッドで実行し、各手順の完了ごとに `PostMessage` でダイアログへ進捗を通知する。処理中は「生成」ボタンを無効化し、完了時に必ず再有効化する。

**Rationale**:
- winget や uv によるインストールは数十秒かかるため、UIスレッドで実行するとダイアログが応答不能になる。
- FR-014（二重実行防止）はボタンの無効化で実現する。ただし DT-003 の確認項目(d)が要求するとおり、**未選択エラーで早期リターンする経路ではボタンを無効化してはならない**。無効化はワーカースレッド起動の直前に行い、再有効化は完了通知の受信時に行う。
- `SendMessage` ではなく `PostMessage` を使うのは、ワーカースレッドがUIスレッドの応答を待つことによるデッドロックを避けるため。

**Alternatives considered**:
- *UIスレッドで実行し `PeekMessage` でポンプを回す*: 実装は単純だが、処理中に他のUI操作を受け付けてしまい再入の危険がある。却下。

---

## R-003: フォルダ選択ダイアログ

**Decision**: MFC の `CFolderPickerDialog` を使用する。

**Rationale**:
- `CFolderPickerDialog` は内部で Vista 以降の `IFileDialog`（`FOS_PICKFOLDERS`）を使う MFC のラッパーであり、追加の COM 記述なしにモダンなフォルダ選択UIが得られる。
- `DoModal()` が `IDOK` 以外を返した場合は表示中のパスを変更しないことで、FR-002 の「キャンセル時は表示を変更しない」と US1-6 を同時に満たせる。

**Alternatives considered**:
- *`SHBrowseForFolder`*: 旧来のツリー表示UIで、パス入力ができず操作性が劣る。却下。

---

## R-004: 各ツールの検出と導入コマンド

実行環境で各コマンドの実挙動を確認した結果を以下に示す。

| 手順 | 検出コマンド | 検出の判定 | 導入コマンド |
|---|---|---|---|
| uv | `uv --version` | 終了コード 0（実測出力: `uv 0.12.6 (...)`） | `winget install --id=astral-sh.uv -e` |
| specify CLI | `specify version` | 終了コード 0 | `uv tool install specify-cli` |
| VS Code 拡張 | `cmd /c code --list-extensions` | 標準出力に `anthropic.claude-code` の行が含まれる（実測で確認） | `cmd /c code --install-extension anthropic.claude-code` |

**Decision**: 検出は上表のとおり、**終了コードを一次判定**とする。`specify version` の出力は Rich ライブラリによる罫線付きボックス（実測: `┌───── Specify CLI Information ─────┐`）であり、バージョン文字列のパースは表示形式の変更に脆いため行わない。

**Rationale**:
- 「導入済みかどうか」の判定に必要なのは存在確認のみで、バージョン値は本フィーチャーの要件にない（YAGNI）。
- VS Code 拡張のみ、コマンドの存在だけでは拡張の導入有無が分からないため、`--list-extensions` の出力を行単位で照合する。拡張ID `anthropic.claude-code` は大文字小文字を区別せず比較する。

**Alternatives considered**:
- *`where.exe` で存在確認*: PATH 上の存在は分かるが、コマンドが実際に動作するかまでは分からない。`--version` 実行のほうが確実。却下。

---

## R-005: `specify init` の非対話実行

**Decision**: `specify init . --integration claude --script ps --force --non-interactive` を対象フォルダをカレントディレクトリとして実行する。

**Rationale**（`specify init --help` の実測結果に基づく）:
- `--non-interactive`: ヘルプに「Never prompt. ... **fail instead of hanging** when a choice has no safe default」と明記されている。パイプ接続で標準入力が TTY でない状態から実行するため、これが無いと**プロンプト待ちでハングする危険がある**。本フィーチャーでは必須。
- `--integration claude`: ヘルプに「Let you choose your coding agent integration, or **default to Copilot** in non-interactive sessions」とある。明示しないと Copilot 用の足場が作られてしまうため必須。
- `--force`: ヘルプに「Skip confirmation when current directory not empty」とある。対象フォルダには既にソリューションが配置済み（＝非空）なので必須。
- `--script ps`: Windows 環境では既定で PowerShell 形式になるが、明示して決定性を確保する。
- **重要な発見**: ヘルプに「Project files are scaffolded from assets bundled inside the specify-cli package, so **initialization does not need network access**」とある。したがって手順4（Spec Kit 足場の配置）はオフラインで完了する。ネットワーク不通時に失敗しうるのは手順1〜3（インストール）のみであり、FR-012 の依存関係の設計に直接影響する。

**Alternatives considered**:
- *`--here` を使う*: `specify init --here` は `specify init .` と等価（ヘルプに「Alternative syntax for current directory」と記載）。どちらでもよいが、カレントディレクトリを明示的に対象フォルダへ設定する実装と対応が取りやすい `.` を選ぶ。

**実装上の注意**: カレントディレクトリの指定は `CreateProcess` の `lpCurrentDirectory` で行う。プロセス全体の `SetCurrentDirectory` を変更してはならない（他の処理に影響するため）。

---

## R-006: Git の状態検出と操作

実行環境（`C:\work\test\test01`、Git 未初期化）で確認した実測結果に基づく。

| 判定したいこと | コマンド | 判定方法 |
|---|---|---|
| Gitリポジトリか | `git rev-parse --is-inside-work-tree` | 終了コード 0 なら真。未初期化では**終了コード 128** と `fatal: not a git repository (or any of the parent directories): .git` を実測 |
| リポジトリのルート | `git rev-parse --show-toplevel` | 標準出力のパスを対象フォルダと比較 |
| コミットが1件以上あるか | `git rev-list --count HEAD` | 終了コード 0 かつ出力が `0` より大きい。コミットが無い場合は非0終了 |

**Decision**: 上記3コマンドで状態を判定し、以下のとおり分岐する。

- **未初期化**（`--is-inside-work-tree` が非0終了）→ `git init` を実行する。
- **初期化済みでルートが対象フォルダと一致** → `git init` は「スキップ」とし、コミット有無のみ判定する。
- **初期化済みでルートが対象フォルダと不一致**（＝対象フォルダが既存リポジトリのサブフォルダ）→ 入れ子リポジトリになるため、FR-016 に従い警告してユーザーの確認を求める。この判定は `--show-toplevel` の出力パスと対象フォルダパスを正規化して比較することで行う（`git` はパス区切りに `/` を用いるため、比較前に正規化が必要）。

**Rationale**: `--is-inside-work-tree` は親ディレクトリを遡って探索するため、単に「対象フォルダに `.git` があるか」を調べるだけでは入れ子リポジトリを検出できない。`--show-toplevel` との組み合わせが必要。

**初回コミット**: `git add -A` の後 `git commit -m "Spec Kit 導入"` を実行する。Git のユーザー設定は Assumptions により設定済みを前提とするが、未設定の場合 `git commit` は非0終了するため、FR-011 の「失敗」として理由（`user.name` / `user.email` の設定が必要）を表示する。

---

## R-007: `.gitignore` の入手方法

**Decision**: Visual Studio 用と Node 用の `.gitignore` 本体を、アプリケーションに `RCDATA` リソースとして**埋め込む**。ネットワークからは取得しない。

**Rationale**:
- 手順書は `curl -o .gitignore https://raw.githubusercontent.com/github/gitignore/main/VisualStudio.gitignore` を提示しているが、これはネットワーク依存を持ち込む。R-005 で確認したとおり `specify init` 自体はオフラインで完了するため、`.gitignore` の取得だけがネットワークを要求する構図は望ましくない。
- 埋め込みにより結果が決定的になり、静的テスト ST-006（生成された `.gitignore` の内容検査）が安定して実施できる。
- 上流の更新に追随しないという欠点はあるが、`.gitignore` の内容は年単位でしか変化せず、更新が必要なら埋め込みリソースを差し替えればよい。YAGNI（憲法 原則III）に照らして、自動更新機構は設けない。

**追記ブロック**（FR-010、種別によらず共通。既存 `.gitignore` がある場合は未記載のものだけを末尾に追記する）:

```
# ---- Spec Kit ----
# feature.json は作業中の機能を指すマシンローカルな状態。
# 複数PCで同じ機能を続けるなら、除外せずコミットする選択もある
# .specify/feature.json

# ---- このプロジェクト固有 ----
*.pdf
```

**Alternatives considered**:
- *`curl.exe` で都度ダウンロード*: Windows 10 以降には `curl.exe` が同梱されるため実行自体は可能だが、オフライン環境で失敗し、取得内容も上流の変更で変わるため ST-006 が不安定になる。却下。

**べき等性の実現（FR-010, FR-013）**: 追記ブロックの有無は、ブロック先頭の目印行（`# ---- Spec Kit ----` および `# ---- このプロジェクト固有 ----`）がファイル内に既に存在するかで判定する。存在すれば追記しない。

---

## R-008: 準備完了チェックリスト7項目の判定方法

すべて読み取り専用で実施する（FR-020）。判定不能を独立した状態として扱う（FR-021）。

| # | 項目 | 判定方法 | 取りうる結果 |
|---|---|---|---|
| 1 | ビルドが通る雛形がある | プロジェクトマーカー（`*.sln` / `*.vcxproj` / `*.csproj` / `package.json`）の存在。実ビルドは行わない | 達成 / 判定不能 |
| 2 | 足場が同階層に並んでいる | `.git`・`.gitignore`・`.specify`・`.claude` の4つが対象フォルダ直下に存在するか | 達成 / 未達 |
| 3 | コミットが1件以上ある | `git rev-list --count HEAD`（R-006 参照） | 達成 / 未達 |
| 4 | テスト方針が決まっている | `.specify/memory/constitution.md` にテスト関連の記述があるか | 達成 / 判定不能 |
| 5 | 憲法がある | `.specify/memory/constitution.md` が存在し、未置換プレースホルダーが残っていないか | 達成 / 未達 |
| 6 | CLAUDE.md が憲法を反映 | `CLAUDE.md` が存在し、最終更新時刻が憲法より新しいか | 達成 / 未達 / 判定不能 |
| 7 | 技術スタックが記録されている | `CLAUDE.md` に言語・ビルド構成の記述があるか | 達成 / 判定不能 |

**Decision（項目5の判定）**: 憲法テンプレートの未置換プレースホルダーは `[` と `]` で囲まれた大文字・数字・アンダースコアのみの語（`[PROJECT_NAME]`、`[PRINCIPLE_1_NAME]`、`[CONSTITUTION_VERSION]` など）である。**ただし HTML コメント（`<!--` から `-->` まで）の内部は走査対象から除外し**、除外後に1つでも残っていれば「未達」とする。

**Rationale と、素朴な判定が失敗する実測例**:

本リポジトリの2ファイルを実測して判定条件を検証した。

| ファイル | 状態 | プレースホルダー検出数（除外なし） | 除外後 |
|---|---|---|---|
| `.specify/templates/constitution-template.md` | 未記入テンプレート | 18件（`[PROJECT_NAME]`・`[PRINCIPLE_1_NAME]`・`[GOVERNANCE_RULES]` 等） | 18件 |
| `.specify/memory/constitution.md` | 制定済み v1.0.0 | **1件**（`[PRINCIPLE_5_NAME]`） | **0件** |

制定済みの憲法にも1件が検出される。実体は10行目の

```
- Removed sections: [PRINCIPLE_5_NAME]（テンプレートの5番目の原則スロットは本プロジェクトでは未使用のため削除）
```

であり、これはファイル冒頭（1〜12行目）の Sync Impact Report を囲む HTML コメントの内部にある。**「削除した項目の名前として、プレースホルダー名が散文中に引用されている」**ケースである。

`/speckit-constitution` は改訂のたびに Sync Impact Report をコメントとして書き出す仕様であり、そこでは「どのプレースホルダーを埋めたか・削除したか」を記録するため、プレースホルダー名の出現は今後も繰り返し起こる。HTML コメントを除外しない判定は、**正しく制定された憲法を「未達」と誤判定する**。この誤判定は準備完了チェックリストの中核（項目5）で起きるため、除外処理は必須である。

**Alternatives considered**:
- *行頭が `- ` で始まる行を除外する*: Sync Impact Report の箇条書きだけを狙い撃つ方法だが、憲法本文にも箇条書きは多数あり、本文中の未置換プレースホルダーを見逃す。却下。
- *ファイル冒頭の固定行数を読み飛ばす*: Sync Impact Report の行数は改訂内容によって変動するため成立しない。却下。

**Decision（項目4・7の判定）**: 単純なキーワード照合で行い、**該当キーワードが見つからない場合は「未達」ではなく「判定不能」とする**。キーワードは項目4が「テスト」、項目7が「ビルド」および言語名。

**Rationale**: 「テスト方針が決まっている」は人間の判断であり、記述の有無から機械的に断定できない。憲法がテストについて何も述べていない場合、それは「方針が無い」のか「方針として不要と決めた」のか区別できない。誤って「未達」と断ずるより「判定不能（人間の確認が必要）」と示すほうが正確であり、FR-021 の要求そのものである。

**Decision（項目6の判定）**: `CLAUDE.md` が存在しない場合は「未達」、存在して憲法より新しい場合は「達成」、存在するが憲法より古い場合は「未達（`/init` の再実行が必要）」とする。ただしファイル時刻は Git のチェックアウトやファイルコピーで容易に変わるため、判定結果には「時刻に基づく参考判定である」旨を併記する。

---

## R-009: 結果表示のUI

**Decision**: レポートビュー形式の `CListCtrl` を用い、「手順名 / 結果 / 理由」の3列で表示する。診断結果も同じコントロールを「項目名 / 判定 / 次に取るべき行動」の3列として再利用する。

**Rationale**:
- FR-011 と FR-019 はいずれも「名称・状態・補足」の3項目を持つ行の集合であり、同じ表現で扱える。別々のコントロールを設けるのは重複（憲法 原則III のYAGNI）。
- 複数行の `CEdit` でも表示はできるが、列が揃わず SC-004（追加調査なしに判断できる）を満たしにくい。

**Alternatives considered**:
- *手順ごとにメッセージボックスを出す*: 手順数だけダイアログ操作が必要になり、一覧性が失われる。却下。

---

## 未解決の項目

なし。Technical Context に NEEDS CLARIFICATION は残っていない。
