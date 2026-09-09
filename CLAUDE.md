# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

このファイルは、このリポジトリでコードを扱う際に Claude Code (claude.ai/code) が従うガイダンスを提供します。

## リポジトリ構成

このリポジトリには相互に関連する3つの部分があります:

- **`MFCApplication1/`** — Visual Studio の MFC デスクトップダイアログアプリケーション。**「SDDプロジェクトのベース作成ツール」**として実装されている。ユーザーが指定したフォルダに対し、uv / specify CLI / VS Code 拡張の導入、Spec Kit 足場の配置、Git 初期化と初回コミットを一括で行い、SDD を始められる状態にする。コメントとUI文字列はすべて日本語。
- **`.specify/` + `.claude/skills/`** — Spec Kit（仕様駆動開発）一式。このリポジトリでの機能開発は**必ず**このワークフローを通す。
- **`specs/`** — 機能ごとの仕様・設計・タスク。`/speckit-specify` のみが作成する。

## ビルド

Visual Studio 専用（CMake/Makefile なし）。

```
msbuild MFCApplication1/MFCApplication1.sln -p:Configuration=Debug -p:Platform=x64
```

**プラットフォーム名の落とし穴**: `.sln` に渡すのは `x64` または **`x86`**。`Win32` を渡すと `error MSB4126: 指定されたソリューション構成 "Debug|Win32" は無効です` になる。`Win32` は `.vcxproj` を直接ビルドする場合の名前で、`.sln` はそれを `x86` というラベルで公開している。

4構成すべてを確認するとき:

```powershell
$msb='C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe'
foreach($cfg in 'Debug','Release'){ foreach($plat in 'x64','x86'){
  $out = & $msb MFCApplication1\MFCApplication1.sln /p:Configuration=$cfg /p:Platform=$plat /t:Rebuild /v:minimal /nologo 2>&1
  "{0,-8} {1,-5} 警告={2} エラー={3}" -f $cfg,$plat,
    ($out | Select-String ' warning ' | Measure-Object).Count,
    ($out | Select-String ' error '   | Measure-Object).Count
}}
```

**出力先と中間ファイルの置き場は別**。`.exe` を探すときに間違えやすい。

| 種類 | パス |
|---|---|
| 実行ファイル (Debug x64) | `MFCApplication1\x64\Debug\MFCApplication1.exe` |
| 中間ファイル (.obj 等) | `MFCApplication1\MFCApplication1\x64\Debug\` |

x86 構成の出力は `x86\` ではなく `MFCApplication1\Debug\` に出る（MFC プロジェクトの既定）。

**ビルド前にアプリを終了させること**。起動したままだと `LNK1104: ファイルを開くことができません` になる。

### コード分析（静的解析）

```
msbuild MFCApplication1/MFCApplication1.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Rebuild -p:RunCodeAnalysis=true -p:EnablePREfast=true
```

## テスト

**自動テストスイートも lint 設定も存在しない**（憲章の技術的制約で明記）。テストランナーを新規導入しないこと。

代わりに、各機能の仕様が**静的テスト（ST-xxx）と動的テスト（DT-xxx）を手順として定義**している。実施手順は `specs/<機能>/quickstart.md` にある。

- **静的テスト** — アプリを実行せずに行う検査。ビルド警告ゼロ、コード分析の指摘ゼロ、リソース定義の確認、表示文字列がすべて日本語であること、FR 番号によるコードの追跡可能性、生成された `.gitignore` の内容
- **動的テスト** — ダイアログを操作して行う検査。テスト対象フォルダは `C:\work\test\test01`（MFC）、`web01`（Web）、`empty01`（空）

**Win32 API による GUI 自動操作は部分的にしかできない。** ボタン押下・コントロール状態の取得・メッセージボックスの読み取りは `PostMessage` / `GetDlgItem` で自動化できるが、**フォルダ選択ダイアログ（`CFolderPickerDialog` / `IFileDialog`）は合成入力を受け付けない**。`WM_SETTEXT`・SendKeys・キーメッセージの直接送信・UI Automation の `ValuePattern`・`WM_CHAR` の逐次送信をいずれも試して失敗している。フォルダ選択を伴う動的テストはユーザーによる手動実行が必要。

埋め込みリソースの検証は、ビルド済み exe から `FindResource` / `LoadResource` で取り出して行える（GUI 不要）。

**画面の色は1ピクセルを採取して機械判定できる。** `DWMWA_CAPTION_COLOR` は設定専用で `DwmGetWindowAttribute` では読み戻せない（`E_INVALIDARG` が返る）ため、タイトルバーの色は `Graphics.CopyFromScreen` で採取して突き合わせる。採取位置はウィンドウ矩形の右端から 200・上端から 15 とし、タイトル文字とキャプションボタンの両方を避ける。最大化するとウィンドウ矩形は不可視の枠の分だけ画面外へ出る（実測 `L=-9 T=-9`）ので、座標は 0 以上へ丸めること。

**ソースコードを文字列検索する検査は、コメント行を除外すること。** 「この書き方をしてはいけない」という警告コメントを実装側に残すと、検査がそのコメントを拾って必ず不合格になる。005 で実際に発生した。004 の ST-303 も同種の失敗をしている。

## アプリケーションのアーキテクチャ

ダイアログ1枚（`IDD_MFCAPPLICATION1_DIALOG`）に、責務ごとの具象クラスを平坦に追加した構成。抽象基底クラス・インターフェース・ファクトリは意図的に導入していない（憲章 原則III のYAGNI）。

| ファイル | 責務 |
|---|---|
| `ProcessRunner` | 外部コマンドを `CreateProcess` + 匿名パイプで実行し、終了コードと標準出力を取得 |
| `TargetFolderState` | 対象フォルダから読み取った事実を表す構造。**導入処理と診断の双方が参照する唯一の事実源** |
| `GitignoreWriter` | 埋め込みリソースから `.gitignore` を生成し、追記ブロックをべき等に適用 |
| `SetupRunner` | SDD化の5手順を順に確認・実行。ワーカースレッドから呼ばれる |
| `MFCApplication1Dlg` | UI の結線、検証チェーン、ワーカースレッドの起動と完了処理、リサイズ追従、タイトルバーの着色 |

### 設計上、壊しやすい箇所

複数ファイルを読まないと分からない不変条件がいくつかある。変更時に注意すること。

**外部コマンドは PowerShell を経由せず、各 exe を直接起動する。** 実行ポリシーによるブロックを構造的に回避するための設計。`.ps1` を新規に追加しないこと（憲章の技術的制約）。`code` だけは `code.cmd` のため `cmd.exe /c` を経由する。

**`ProcessRunner` はパイプを読み出しながら子プロセスの終了を待つ。** 読み出さずに `WaitForSingleObject` すると、子プロセスがパイプバッファ（既定 4KB）を埋めた時点でデッドロックする。

**`specify init` には4つのフラグがすべて必要**。1つでも欠けると壊れる。

```
specify init . --integration claude --script ps --force --non-interactive
```

`--non-interactive` を省くとパイプ接続時にプロンプト待ちでハングする。`--integration claude` を省くと非対話セッションでは Copilot が既定になる。`--force` を省くと非空ディレクトリで確認待ちになる。なお `specify init` 自体はネットワークを必要としない（テンプレートが CLI パッケージに同梱されている）。

**処理中は2コントロールを無効化する** — 生成ボタンと参照ボタン（ラジオボタンは 002 で、診断ボタンは 003 で廃止）。参照ボタンを外すと、処理中にフォルダを変更されてワーカースレッドの処理対象と画面表示が食い違う。

**未選択エラーの経路で生成ボタンを無効化してはならない。** 二重実行防止はワーカースレッドが動作している間のみ適用される。ここで無効化すると、ユーザーがフォルダを選択してもボタンが押せなくなりアプリの再起動が必要になる。

**検証チェーンの順序を変えないこと** — フォルダ未選択 → 書き込み権限 → マーカー → 足場が中途半端 → 入れ子リポジトリ。特に書き込み権限をマーカー確認より先に置く点（権限が無いフォルダでマーカーを探しても意味がない）。

**診断は対象フォルダに一切書き込まない。** `TargetFolder::IsWritable()` は一時ファイルを作るため、診断の経路から呼び出してはならない。

**Git 手順は既存の未コミット変更を巻き込まない。** ステージング前に `git status --porcelain` を確認し、他人の変更があれば `git add -A` を使わず `.gitignore` のみをステージする。

**タイトルバーの色は `OnInitDialog` で1回設定するだけでよい。** `DwmSetWindowAttribute` に `DWMWA_CAPTION_COLOR` / `DWMWA_TEXT_COLOR` を渡す。DWM がウィンドウ属性として色を保持するため、非アクティブ・最大化・最小化からの復帰・ライト/ダークモードの切り替えのいずれでも色は保たれる。**`OnSize` などで塗り直してはならない** — 004 のリサイズ追従に無関係な責務が混ざる。戻り値は意図的に見ない。着色できない環境では `E_INVALIDARG` が返るだけで例外にならないため、無視すれば「何も起こらなかった」のと同じになる。

**憲章の未対応 TODO を判定する際は HTML コメントを除外する。** `/speckit-constitution` は冒頭の Sync Impact Report に「削除した項目」としてプレースホルダー名（`[PRINCIPLE_5_NAME]` 等）を引用するため、除外しないと正しく制定された憲章を「未達」と誤判定する。

**`.gitignore` は種別を問わず常に同じ統合版を新規作成する。** ただし既存ファイルがある場合は本体を書き換えず、未記載の追記ブロックのみを末尾へ足す。追記済みかは目印行（`# ---- Spec Kit ----` 等）で判定するため、この文字列を変えるとブロックが重複する。

### ファイルのエンコーディング

| 種類 | エンコーディング |
|---|---|
| `.h` / `.cpp` | UTF-8 **BOM 付き**（日本語コメントのため必須） |
| `.rc` | **UTF-16LE BOM 付き** |

新規ファイルを作ったら BOM を確認すること。`.rc` を PowerShell で編集する場合は `[System.Text.Encoding]::Unicode` で読み書きする。

**PowerShell の `Add-Type` に渡す C# コードには日本語コメントを書かないこと。** 一時ファイル生成時に文字化けして改行が壊れ、コンパイルエラーになる。

**日本語を含む `.ps1` は UTF-8 BOM 付きで保存すること。** Windows PowerShell 5.1 は BOM の無い `.ps1` をシステムの ANSI コードページとして読むため、UTF-8 のまま保存すると日本語が化けて構文エラーになる。Bash のヒアドキュメントで書いた場合は BOM が付かないので、後から付け直す必要がある。

## Spec Kit ワークフロー

機能開発は `.claude/skills/speckit-*` のスキルをスラッシュコマンドとして、以下の順序で呼び出す。

```
/speckit-constitution → /speckit-specify → /speckit-clarify → /speckit-plan
  → /speckit-tasks → /speckit-analyze / /speckit-checklist → /speckit-implement
```

主要な仕組み（各 `SKILL.md` に定義。ワークフローを変更する前に該当ファイルを読むこと）:

- **`/speckit-specify`** はテンプレートから `specs/<NNN>-<short-name>/spec.md` を作成し、アクティブな機能ディレクトリを `.specify/feature.json` に記録する。採番は連番。**機能ディレクトリを作成するのはここだけ。**
- **`/speckit-plan`** は `setup-plan.ps1 -Json` を実行し、spec と憲章を読み込んで `research.md`・`data-model.md`・`contracts/`・`quickstart.md` を生成する。
- **`/speckit-tasks`** は `setup-tasks.ps1 -Json` を実行し、`tasks.md` をユーザーストーリー単位（レイヤー単位ではない）で生成する。厳密なチェックリスト行形式（`- [ ] T001 [P] [US1] Description with file path`）が必須。
- **`/speckit-implement`** は `check-prerequisites.ps1 -Json -RequireTasks -IncludeTasks` を実行し、`tasks.md` をフェーズごとに実行して完了タスクを `[X]` にする。`checklists/*.md` は**読み取り専用のゲート**として扱い、チェックリストファイル自体を編集しない。

**`.specify/feature.json` は `/speckit-specify` を実行するたびに書き換わる。** 別の機能に戻るには手で書き戻すか、環境変数 `SPECIFY_FEATURE_DIRECTORY` で明示する。このファイルは `.specify/.gitignore` により除外されるマシンローカル状態なのでコミットされない。

`.specify/extensions.yml` は存在しないため、各スキルのフック関連セクションは何もしない。

**Windows 環境では `.ps1` の実行に Bash ツールではなく PowerShell ツールを使うこと。**

### 既存機能

| 機能 | 内容 |
|---|---|
| `specs/001-sdd-project-bootstrap/` | 本体の実装。5手順のSDD化、7項目の診断、手順案内 |
| `specs/002-remove-kind-selection/` | プロジェクト種別（MFC/Web）の選択を廃止し、`.gitignore` を統合 |
| `specs/003-remove-readiness-check/` | 「準備状況を診断」ボタンと7項目の診断機能を削除 |
| `specs/004-resizable-dialog/` | ウィンドウのサイズ変更に全コントロールが追従。最小サイズを設定 |
| `specs/005-dark-blue-caption/` | タイトルバーをネイビー `#1F3864`、文字を白にする |

**後の機能が前の機能の要件を一部置き換えている。** 各 spec の冒頭に対応表がある。設計文書は前機能の全文コピーではなく**差分のみ**を記述しているため、新しい番号から読むときは前の番号も併せて参照すること。

001 の「7項目の診断」は 003 で廃止済みであり、現在のアプリには存在しない。

## 憲章

`.specify/memory/constitution.md`（v1.0.0）が**他のすべての開発慣行に優先する**。CLAUDE.md と矛盾する場合は憲章が優先。

4つの原則: **I. Spec Kit ワークフロー厳守** / **II. 日本語での一貫性** / **III. シンプルさ優先（YAGNI）** / **IV. 変更の追跡可能性**

実務上、特に効いてくるのは次の2つ。

**原則IV（変更の追跡可能性）** — テストスイートが無いため、`spec.md` と `tasks.md` の対応関係が変更妥当性を担保する一次情報源になる。新規追加・変更した各箇所に、対応する FR 番号をコメントで記載すること。静的テスト ST-xxx がこれを検査する。

**原則III（YAGNI）** — 現時点で必要とされていない抽象化・共通化・拡張ポイントを導入しない。3行程度の重複は時期尚早な共通化より許容される。

憲章の変更は `/speckit-constitution` を通じてのみ行う。
