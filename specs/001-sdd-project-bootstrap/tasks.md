---

description: "SDDプロジェクトのベース作成ツールのタスクリスト"
---

# Tasks: SDDプロジェクトのベース作成ツール

**Input**: `/specs/001-sdd-project-bootstrap/` からの設計ドキュメント

**Prerequisites（前提条件）**: [plan.md](./plan.md)、[spec.md](./spec.md)、[research.md](./research.md)、[data-model.md](./data-model.md)、[contracts/ui-contract.md](./contracts/ui-contract.md)、[quickstart.md](./quickstart.md)

**Tests（テスト）**: 本フィーチャーの仕様は「テスト方針」節でテストを明示的に要求している。静的テスト（ST-001〜007）と動的テスト（DT-001〜009）を検証タスクとして含める。自動テストランナーは導入しないため、これらはコードではなく**実施手順**として遂行するタスクである。

**Organization（構成）**: タスクはユーザーストーリーごとにグループ化され、各ストーリーの独立した実装とテストを可能にする。

## Format（形式）: `[ID] [P?] [Story] Description`

- **[P]**: 並行実行可能（異なるファイル、依存関係なし）
- **[Story]**: このタスクがどのユーザーストーリーに属するか（US1、US2、US3）
- 説明には正確なファイルパスを含める

## Path Conventions（パスの慣例）

本フィーチャーは既存の `MFCApplication1/` にファイルを平坦に追加する（[plan.md](./plan.md) の Structure Decision を参照）。すべてのパスはリポジトリルートからの相対パスで記載する。

---

## Phase 1: Setup（共有インフラ）

**目的**: リソース定義とプロジェクトへのファイル登録。コードを書き始める前の土台

- [X] T001 `MFCApplication1/Resource.h` に新規コントロールID（`IDC_EDIT_TARGET_PATH`、`IDC_BUTTON_BROWSE`、`IDC_RADIO_KIND_MFC`、`IDC_RADIO_KIND_WEB`、`IDC_BUTTON_GENERATE`、`IDC_BUTTON_DIAGNOSE`、`IDC_LIST_RESULT`、`IDC_STATIC_GUIDE`）とリソースID（`IDR_GITIGNORE_VS`、`IDR_GITIGNORE_NODE`）を定義し、`_APS_NEXT_CONTROL_VALUE` と `_APS_NEXT_RESOURCE_VALUE` を更新する
- [X] T002 [P] `MFCApplication1/res/gitignore_vs.txt` を作成し、GitHub 公式の Visual Studio 用 `.gitignore` の内容を格納する（`[Dd]ebug/`、`[Rr]elease/`、`[Oo]bj/`、`.vs/`、`*.vcxproj.user`、`*.aps` を含むこと）
- [X] T003 [P] `MFCApplication1/res/gitignore_node.txt` を作成し、GitHub 公式の Node 用 `.gitignore` の内容を格納する（`node_modules/`、`dist/`、`.env` を含むこと）
- [X] T004 `MFCApplication1/MFCApplication1.rc` に T002・T003 の2ファイルを `RCDATA` リソースとして登録する（T001〜T003 に依存）
- [X] T005 `MFCApplication1/MFCApplication1.rc` の `IDD_MFCAPPLICATION1_DIALOG` に [contracts/ui-contract.md](./contracts/ui-contract.md) 「1. ダイアログのコントロール」の8コントロールを配置し（FR-001、FR-003、FR-004、FR-017）、既存の TODO 静的テキストを削除する。ラジオボタン2つはどちらも初期状態で未選択とし、`IDC_RADIO_KIND_MFC` に `WS_GROUP` を付与する（T001 に依存）
- [X] T006 `MFCApplication1/MFCApplication1.vcxproj` と `MFCApplication1/MFCApplication1.vcxproj.filters` に、Phase 2 以降で追加する新規ソースファイルとリソースファイルを登録する

**チェックポイント**: リソースが定義され、ビルドが通る状態

---

## Phase 2: Foundational（ブロッキングとなる前提条件）

**目的**: すべてのユーザーストーリーが依存する共通基盤。外部コマンドの実行と対象フォルダの状態取得

**⚠️ 重要**: このフェーズが完了するまで、どのユーザーストーリーの作業も開始できない

- [X] T007 `MFCApplication1/ProcessRunner.h` に、外部コマンドの実行結果を表す構造体（終了コード、標準出力、実行できたか）と実行関数のインターフェースを定義する
- [X] T008 `MFCApplication1/ProcessRunner.cpp` に `CreateProcess` + 匿名パイプによる外部コマンド実行を実装する。作業ディレクトリは `lpCurrentDirectory` で指定し、`SetCurrentDirectory` は使用しない。`CREATE_NO_WINDOW` でコンソールを非表示にする。**パイプを読み出しながら終了を待つ**こと（読み出さずに待つとパイプバッファが埋まってデッドロックする。[research.md](./research.md) R-001）。T007 に依存
- [X] T009 `MFCApplication1/TargetFolderState.h` に [data-model.md](./data-model.md) 「1. TargetFolderState」の全フィールドと `GitState` 列挙を定義する
- [X] T010 `MFCApplication1/TargetFolderState.cpp` にフォルダ走査によるマーカー・足場・`.gitignore` の検出を実装する（T009 に依存）
- [X] T011 `MFCApplication1/TargetFolderState.cpp` に Git 状態の判定を実装する。`git rev-parse --is-inside-work-tree` の終了コードで `NotRepository` を判定し、`git rev-parse --show-toplevel` の出力を対象フォルダと比較して `RepositoryAtTarget` と `NestedUnderParent` を区別する。パス比較は区切り文字の正規化・大文字小文字の無視・末尾区切りの除去を行ってから実施する（[research.md](./research.md) R-006）。T008、T010 に依存
- [X] T012 `MFCApplication1/TargetFolderState.cpp` に書き込み権限の判定（一時ファイルの作成と削除を試行）を実装する。**この関数は診断からは呼び出さない**（FR-020 の読み取り専用要件）。T010 に依存
- [X] T013 `MFCApplication1/MFCApplication1Dlg.h` に、対象フォルダパス・選択された種別・処理実行中フラグのメンバ変数と、新規コントロールのメンバ変数を宣言する
- [X] T014 `MFCApplication1/MFCApplication1Dlg.cpp` の `DoDataExchange` にコントロールの関連付けを追加し、`OnInitDialog` で `IDC_LIST_RESULT` をレポートビューに設定して3列（項目 / 結果 / 内容）を作成する。ラジオボタンをどちらも未選択のままにし、生成・診断ボタンを有効な状態で初期化する（[contracts/ui-contract.md](./contracts/ui-contract.md) 「1. 初期状態の契約」）。T013 に依存
- [X] T015 `MFCApplication1/MFCApplication1Dlg.cpp` に、結果一覧へ1行（項目名・状態・内容）を追加するヘルパー関数と、一覧をクリアする関数を実装する。手順結果（FR-011）と診断結果（FR-019）の両方がこれを共用する（[research.md](./research.md) R-009）。T014 に依存

**チェックポイント**: 基盤が完了 — 外部コマンドの実行と対象フォルダの状態取得ができ、結果を一覧表示できる

---

## Phase 3: User Story 1 - 配置済みプロジェクトを選んでSDD化する (Priority: P1) 🎯 MVP

**目標**: フォルダと種別を選んで「生成」を押すと、uv・specify CLI・VS Code 拡張の導入、Spec Kit 足場の配置、Git 初期化と初回コミットまでが完了する

**独立したテスト**: `C:\work\test\test01` を選択し種別に「MFCアプリ」を選んで「生成」を押し、`.specify`・`.claude`・`.git`・`.gitignore` が揃いコミットが1件以上作成されることを確認する

### 対象の選択（FR-001〜FR-005）

- [X] T016 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` に「参照...」ボタンのハンドラを実装する。`CFolderPickerDialog` を表示し、`IDOK` のときのみ `IDC_EDIT_TARGET_PATH` を更新する。**キャンセル時は表示中のパスを変更しない**（FR-002、US1-6）
- [X] T017 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` にラジオボタン2つのハンドラを実装し、選択された `ProjectKind` をメンバ変数へ保持する（FR-003）

### 生成前の検証チェーン（FR-005〜FR-008、FR-015、FR-016）

- [X] T018 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` に「生成」ボタンのハンドラの検証部を実装する。[contracts/ui-contract.md](./contracts/ui-contract.md) 「3.2 生成ボタン」の順序（フォルダ未選択 → 種別未選択 → 書き込み権限 → マーカー → 足場の中途半端 → 入れ子リポジトリ）で検証し（FR-005、FR-006、FR-007、FR-015）、いずれかで中止する場合は**ファイルシステムへの変更を一切行わない**。**未選択エラーの経路で生成ボタンを無効化してはならない**（DT-003 の確認項目(d)）。T014、T016、T017 に依存
- [X] T019 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` に種別の不一致判定を実装する。選択種別のマーカーが無く、もう一方の種別のマーカーがある場合は確認ダイアログを表示し、「いいえ」なら中止する（FR-008）。T018 に依存
- [X] T020 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` に入れ子リポジトリの確認を実装する。`GitState` が `NestedUnderParent` のとき警告して確認を求め、「いいえ」なら中止する（FR-016）。T011、T018 に依存

### `.gitignore` の生成（FR-010）

- [X] T021 [P] [US1] `MFCApplication1/GitignoreWriter.h` に、埋め込みリソースの読み出しと `.gitignore` の生成・追記のインターフェースを定義する
- [X] T022 [US1] `MFCApplication1/GitignoreWriter.cpp` に `RCDATA` リソースからの `.gitignore` 本体の読み出しを実装する（`FindResource` / `LoadResource` / `LockResource`）。種別に応じて `IDR_GITIGNORE_VS` と `IDR_GITIGNORE_NODE` を切り替える。T004、T021 に依存
- [X] T023 [US1] `MFCApplication1/GitignoreWriter.cpp` に追記ブロックのべき等な適用を実装する。目印行 `# ---- Spec Kit ----` と `# ---- このプロジェクト固有 ----` の有無を判定し、**未記載のブロックのみ**を末尾に追記する。既存の `.gitignore` の内容は書き換えない（FR-010、FR-013、[contracts/ui-contract.md](./contracts/ui-contract.md) 「6. 生成されるファイルのコントラクト」）。T022 に依存

### セットアップ手順の実行（FR-009、FR-011〜FR-014）

- [X] T024 [P] [US1] `MFCApplication1/SetupRunner.h` に [data-model.md](./data-model.md) 「3. SetupStep」の構造（`Id`、`DisplayName`、`DependsOn`、`Result`、`Reason`）と `StepResult` 列挙、5手順の固定配列を定義する
- [X] T025 [US1] `MFCApplication1/SetupRunner.cpp` に手順1（uv の導入）を実装する。`uv --version` が0終了なら「スキップ」、そうでなければ `winget install --id=astral-sh.uv -e` を実行する。T008、T024 に依存
- [X] T026 [US1] `MFCApplication1/SetupRunner.cpp` に手順2（specify CLI の導入）を実装する。`specify version` が0終了なら「スキップ」、そうでなければ `uv tool install specify-cli` を実行する。**バージョン文字列のパースは行わない**（出力が罫線付きボックスのため。[research.md](./research.md) R-004）。T025 に依存
- [X] T027 [US1] `MFCApplication1/SetupRunner.cpp` に手順3（VS Code 拡張の導入）を実装する。`cmd.exe /c code --list-extensions` の出力に `anthropic.claude-code` の行があれば「スキップ」、なければ `cmd.exe /c code --install-extension anthropic.claude-code` を実行する。拡張IDの比較は大文字小文字を区別しない。T008、T024 に依存
- [X] T028 [US1] `MFCApplication1/SetupRunner.cpp` に手順4（Spec Kit 足場の配置）を実装する。`.specify` と `.claude` が両方あれば「スキップ」、なければ対象フォルダを作業ディレクトリとして `specify init . --integration claude --script ps --force --non-interactive` を実行する。**4つのフラグはいずれも省略不可**（`--non-interactive` を省くとハング、`--integration claude` を省くと Copilot が既定、`--force` を省くと非空ディレクトリで確認待ち。[research.md](./research.md) R-005）。T008、T024 に依存
- [X] T029 [US1] `MFCApplication1/SetupRunner.cpp` に手順5（Git 初期化・`.gitignore`・初回コミット）を実装する。`GitState` に応じて `git init` の要否を決め、`.gitignore` を T023 で生成する。**ステージングの前に `git status --porcelain` で既存の未コミット変更を確認し、存在する場合は `git add -A` を使わず `.gitignore` のみをステージする**（FR-013a）。未コミット変更が無い場合のみ `git add -A` を使ってよい。その後 `git commit -m "Spec Kit 導入"` を実行する。`git commit` が非0終了した場合は `user.name` / `user.email` の設定が必要である旨を `Reason` に格納する（[research.md](./research.md) R-006）。T011、T023、T024 に依存
- [X] T029a [US1] `MFCApplication1/SetupRunner.cpp` で、未コミット変更が存在したために限定的なステージングを行った場合、その旨を手順5の `Reason` に記録して結果一覧に表示する（例: 「既存の未コミット変更を検出したため、.gitignore のみをコミットしました」）。ユーザーが自分の作業が巻き込まれていないことを確認できるようにする（FR-013a）。T029 に依存
- [X] T030 [US1] `MFCApplication1/SetupRunner.cpp` に依存関係の解決を実装する。`DependsOn` に指定された先行手順が `Failed` の場合、当該手順を実行せず `SkippedUnmetDependency` とする。依存しない手順（`InstallVsCodeExt`、`InitGit`）は先行手順の失敗にかかわらず実行する（FR-012、[data-model.md](./data-model.md) 「依存関係の設計根拠」）。T025〜T029 に依存
- [X] T031 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` にワーカースレッドの起動と完了処理を実装する。`AfxBeginThread` で `SetupRunner` を実行し、各手順の完了ごとに `PostMessage` で結果を1行ずつ `IDC_LIST_RESULT` へ追加する。**起動直前に生成ボタンと診断ボタンを無効化し、完了通知の受信時に必ず有効へ戻す**（成功・失敗・中断のいずれでも。FR-014、[contracts/ui-contract.md](./contracts/ui-contract.md) 「2. ボタンの有効・無効の契約」）。T015、T018、T030 に依存
- [X] T032 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` に、全手順の完了後に `IDC_STATIC_GUIDE` へ**定型の**完了メッセージ（[contracts/ui-contract.md](./contracts/ui-contract.md) 「完了時の案内」の4行）を表示する処理を実装する。これは導入処理の実行結果報告の一部であり（FR-011）、診断結果に応じて内容が変わる手順案内（FR-022、US3 の T048）とは別物である。US1 単独で完結させるため、ここでは診断結果を参照しない。T031 に依存

### User Story 1 の検証

- [X] T033 [US1] [quickstart.md](./quickstart.md) 手順3の DT-003（フォルダ未選択で「生成」を押下）を実施し、確認項目(a)〜(e)がすべて満たされることを確認する。特に(d)「メッセージを閉じた後も生成ボタンが押下可能」を必ず検証する (SC-003)
- [ ] T034 [P] [US1] [quickstart.md](./quickstart.md) 手順3の DT-004（種別未選択）と DT-005（キャンセル後も選択が保持される）を実施する (SC-003)
- [ ] T035 [US1] [quickstart.md](./quickstart.md) 手順5の DT-001（`C:\work\test\test01` でSDD化を完走）を実施し、`.specify`・`.claude`・`.git`・`.gitignore` が揃いコミットが1件以上あることを確認する (SC-001)
- [ ] T036 [P] [US1] [quickstart.md](./quickstart.md) 手順6の ST-006（`.gitignore` の内容検査。3パターンが各1回だけ出現）と ST-007（初回コミットに `.vs/`・`*.vcxproj.user`・`*.aps` が含まれない）を実施する。T035 に依存
- [ ] T037 [US1] [quickstart.md](./quickstart.md) 手順7の DT-002（べき等性。再実行で全手順が「スキップ」、`git status` に変更なし、追記ブロックが重複しない）を実施する。T035 に依存 (SC-002)
- [ ] T038 [P] [US1] [quickstart.md](./quickstart.md) 手順9の DT-006（プロジェクト不在のフォルダで案内が表示され、雛形が生成されないこと。FR-007）と DT-007（種別の不一致で確認を求められること）を実施する (SC-003)

**チェックポイント**: この時点で User Story 1 は独立して完全に機能する。配置済みプロジェクトのSDD化がGUI操作だけで完了する（MVP 達成）

---

## Phase 4: User Story 2 - 準備完了チェックリストの達成状況を診断する (Priority: P2)

**目標**: 対象フォルダの準備状況7項目を判定し、未達項目と次に取るべき行動を一覧表示する

**独立したテスト**: SDD化前後の `C:\work\test\test01` を診断し、判定が正しく変化すること、かつどちらの実行でもフォルダが一切変化しないことを確認する

### 診断ロジック（FR-018、FR-021）

- [X] T039 [P] [US2] `MFCApplication1/ReadinessChecker.h` に [data-model.md](./data-model.md) 「4. ReadinessItem」の構造（`Id`、`DisplayName`、`Verdict`、`NextAction`）と `ReadinessVerdict` 列挙、7項目の固定配列を定義する
- [X] T040 [US2] `MFCApplication1/ReadinessChecker.cpp` に項目1〜3の判定を実装する。項目1はプロジェクトマーカーの存在（実ビルドはしない。`Met` / `Indeterminate` のみ）、項目2は `.git`・`.gitignore`・`.specify`・`.claude` の4つが直下に揃っているか、項目3は `git rev-list --count HEAD` の結果。T008、T010、T039 に依存
- [X] T041 [US2] `MFCApplication1/ReadinessChecker.cpp` に項目5（憲法がある）の判定を実装する。`.specify/memory/constitution.md` を読み、`[大文字・数字・アンダースコア]` 形式の未置換プレースホルダーを検出する。**HTML コメント（`<!--` 〜 `-->`）の内部は走査対象から除外する** — 除外しないと Sync Impact Report 中に引用されたプレースホルダー名により、制定済みの憲法を誤って「未達」と判定する（[research.md](./research.md) R-008 に実測例あり）。T039 に依存
- [X] T042 [US2] `MFCApplication1/ReadinessChecker.cpp` に項目4・6・7の判定を実装する。項目4は憲法内のテスト関連記述の有無、項目6は `CLAUDE.md` の存在と憲法との更新時刻の比較、項目7は `CLAUDE.md` 内の言語・ビルド構成の記述の有無。**項目4と7は `NotMet` を取らず、条件を満たさない場合は必ず `Indeterminate` とする**（FR-021）。T041 に依存
- [X] T043 [US2] `MFCApplication1/ReadinessChecker.cpp` に、`NotMet` および `Indeterminate` の各項目へ次に取るべき行動を設定する処理を実装する（[data-model.md](./data-model.md) 「7項目の判定」の NextAction 列）。FR-019。T040、T042 に依存

### 診断のUI（FR-017、FR-020）

- [X] T044 [US2] `MFCApplication1/MFCApplication1Dlg.cpp` に「準備状況を診断」ボタンのハンドラを実装する。フォルダ未選択なら案内して中止し、そうでなければ7項目を判定して `IDC_LIST_RESULT` に7行を表示する。**書き込み権限の判定関数（T012）を呼び出してはならない** — 一時ファイルの作成が FR-020 の読み取り専用要件に違反するため。T015、T043 に依存
- [X] T045 [US2] `MFCApplication1/MFCApplication1Dlg.cpp` で、導入処理の実行中は診断ボタンを無効化する（[contracts/ui-contract.md](./contracts/ui-contract.md) 「2. ボタンの有効・無効の契約」）。T031、T044 に依存

### User Story 2 の検証

- [ ] T046 [US2] [quickstart.md](./quickstart.md) 手順4の DT-008（診断が読み取り専用。実行前後でフォルダの項目数が一致）を実施する (SC-005)
- [ ] T047 [US2] [quickstart.md](./quickstart.md) 手順4と手順8の DT-009（SDD化の前後で判定が正しく変化し、項目5・6は未達のまま残る）を実施する。T035 に依存 (SC-006)

**チェックポイント**: User Story 1 と 2 の両方が独立して動作する

---

## Phase 5: User Story 3 - 憲法・CLAUDE.md までの準備手順を案内される (Priority: P3)

**目標**: 自動実行できない工程について、実行すべきコマンドと順序を、完了状況を区別して表示する

**独立したテスト**: 憲法が未制定のフォルダを診断した状態で案内を表示し、`/speckit-constitution` → `/init` → 技術スタック記録の順に提示されることを確認する

- [X] T048 [US3] `MFCApplication1/MFCApplication1Dlg.cpp` に、診断結果に基づく手順案内の生成を実装する。テスト方針の決定 → `/speckit-constitution` → `/init` の再実行 → 技術スタックの記録 の4工程を順に並べ、診断で `Met` と判定済みの工程は完了として区別して示す（FR-022）。T043、T044 に依存
- [X] T049 [US3] `MFCApplication1/MFCApplication1Dlg.cpp` で、生成した案内を `IDC_STATIC_GUIDE` に表示する。コマンド文字列をそのまま Claude Code のパネルへ貼り付けられる形式にする。T048 に依存
- [ ] T050 [US3] [spec.md](./spec.md) の User Story 3 受け入れシナリオ1・2を実施する。憲法未制定のフォルダでは4工程すべてが未完了として、憲法制定済みで `CLAUDE.md` が古いフォルダでは `/init` 以降のみが未完了として提示されることを確認する

**チェックポイント**: これですべてのユーザーストーリーが独立して機能する

---

## Phase 6: Polish & Cross-Cutting Concerns（仕上げと横断的関心事）

**目的**: 静的テストの完遂と、検証ガイド全体の通し実行

- [X] T051 [P] `MFCApplication1/` の全ソースについて ST-005（表示文字列がすべて日本語であること、[contracts/ui-contract.md](./contracts/ui-contract.md) 「4. メッセージ文字列の契約」の9メッセージが定義されていること）を検査する
- [X] T052 [P] `MFCApplication1/MFCApplication1.rc` と `MFCApplication1/Resource.h` について ST-004（8コントロールの定義、ラジオボタンが初期状態で未選択であること）を検査する
- [X] T053 [P] `MFCApplication1/` の新規追加コードについて ST-003（各関数が対応する FR 番号をコメントで追跡できること。憲法 原則IV）を検査し、不足していればコメントを追加する
- [X] T054 ST-001（`Debug`/`Release` × `x64`/`Win32` の4構成すべてで MSBuild が成功し、新規コード起因の警告が出ないこと）を実施する。[quickstart.md](./quickstart.md) 手順1
- [X] T055 ST-002（Visual Studio のコード分析で未初期化変数・リソースリーク・バッファ関連の指摘が出ないこと）を実施する。T054 に依存
- [ ] T056 SC-004 を検証する。SDD化の実行結果一覧と診断結果一覧を表示した状態で、この機能を知らない第三者が各行を見て「どの手順・項目が完了・スキップ・失敗・未達か」と「次に何をすべきか」を、口頭の補足なしに判断できることを確認する。判断できない行があれば表示文言を修正する
- [ ] T057 F1 を検証する。導入処理の実行中に「参照...」ボタンとラジオボタンが無効化されていること、および完了後に4コントロールすべてが有効へ戻ることを確認する（[contracts/ui-contract.md](./contracts/ui-contract.md) 「実行中に無効化するコントロール」）。T031 に依存
- [ ] T058 FR-013a を検証する。SDD化済みの `C:\work\test\test01` で `.gitignore` から `# ---- Spec Kit ----` ブロックを手で削除し、任意のソースファイルにも未コミットの変更を加えたうえで「生成」を再実行する。**その未コミット変更がコミットに含まれず**、`.gitignore` のみがコミットされ、その旨が結果一覧に表示されることを確認する。確認後 `git log -1 --stat` と `git status --short` で裏付ける。T029a に依存
- [ ] T059 [quickstart.md](./quickstart.md) の付録に従って `C:\work\test\test01` を初期状態にリセットし、手順1〜9を通しで実行して「完了条件」の4項目をすべて確認する。T051〜T058、および Phase 3〜5 の全検証タスクに依存

---

## Dependencies & Execution Order（依存関係と実行順序）

### Phase Dependencies（フェーズの依存関係）

- **Setup (Phase 1)**: 依存関係なし — すぐに開始可能
- **Foundational (Phase 2)**: Setup の完了に依存 — すべてのユーザーストーリーをブロックする
- **User Stories (Phase 3〜5)**: すべて Foundational の完了に依存
- **Polish (Phase 6)**: 対象とするすべてのユーザーストーリーの完了に依存

### User Story Dependencies（ユーザーストーリーの依存関係）

- **User Story 1 (P1)**: Foundational の後に開始可能。他のストーリーへの依存なし
- **User Story 2 (P2)**: Foundational の後に開始可能。ただし**検証タスク T047 のみ** US1 の T035（SDD化の実行）に依存する。判定ロジックの実装自体は US1 と独立して進められる
- **User Story 3 (P3)**: Foundational の後に開始可能だが、案内の生成が診断結果を入力とするため、**実装は US2 の T043・T044 に依存**する

### 重要な依存関係（フェーズをまたぐもの）

| タスク | 依存先 | 理由 |
|---|---|---|
| T031（ワーカースレッド） | T030 | 5手順の実行ロジックが揃っている必要がある |
| T045（診断ボタンの無効化） | T031 | 導入処理の実行中フラグを参照する |
| T047（DT-009） | T035 | SDD化の前後の判定を比較するため |
| T048（案内の生成） | T043 | 診断結果を入力とするため |
| T056（通し実行） | Phase 3〜5 の全検証タスク | 全機能が揃った状態で実施する |

### Parallel Opportunities（並行実行の機会）

- **Phase 1**: T002 と T003（2つの `.gitignore` リソースファイル）は並行可能
- **Phase 2**: T007〜T008（ProcessRunner）と T013〜T014（ダイアログのメンバ）は別ファイルのため並行可能
- **Phase 3**: T021（GitignoreWriter.h）と T024（SetupRunner.h）は並行可能。T025〜T029 は同一ファイル（`SetupRunner.cpp`）のため**並行不可**
- **Phase 3 の検証**: T034、T036、T038 は互いに独立
- **Phase 4**: T039 は Phase 3 と並行可能（`ReadinessChecker.h` は US1 が触らない）
- **Phase 6**: T051、T052、T053 は互いに独立

### Within Each User Story（各ユーザーストーリー内）

- ヘッダ定義 → 実装 → ダイアログへの結線 → 検証 の順
- `SetupRunner.cpp` と `ReadinessChecker.cpp` は単一ファイルに複数タスクが集中するため、順次実行する
- 次の優先度に進む前にストーリーの検証タスクまで完了させる

---

## Parallel Example: Phase 1（並行実行の例）

```text
# 2つの .gitignore 埋め込みリソースを一緒に作成する:
Task: "MFCApplication1/res/gitignore_vs.txt に Visual Studio 用 .gitignore を格納する"
Task: "MFCApplication1/res/gitignore_node.txt に Node 用 .gitignore を格納する"
```

## Parallel Example: Phase 6（並行実行の例）

```text
# 3つの静的検査を一緒に実施する:
Task: "ST-005 表示文字列がすべて日本語であることを検査する"
Task: "ST-004 リソース定義の8コントロールを検査する"
Task: "ST-003 各関数の FR 番号コメントを検査する"
```

---

## Implementation Strategy（実装戦略）

### MVP First（User Story 1 のみ）

1. Phase 1: Setup を完了する（T001〜T006）
2. Phase 2: Foundational を完了する（T007〜T015）— すべてのストーリーをブロックする
3. Phase 3: User Story 1 を完了する（T016〜T038）
4. **停止して検証**: T033（DT-003）と T035（DT-001）を単独で実施する
5. この時点で「配置済みプロジェクトをGUI操作だけでSDD化する」という中心的価値が動く

### Incremental Delivery（段階的な提供）

1. Setup + Foundational → 基盤が準備完了
2. User Story 1 → 検証 → **MVP**（SDD化が動く）
3. User Story 2 → 検証 → 準備状況の可視化が加わる
4. User Story 3 → 検証 → 残作業の案内が加わる
5. Phase 6 → 静的テストと通し実行で全体を確認

### 実装順序に関する注意

- **T028（`specify init`）は4つのフラグをすべて付けること。** 1つでも欠けるとハング・誤った足場・確認待ちのいずれかが起きる
- **T018 と T031 は生成ボタンの有効・無効を逆向きに扱う。** T018（検証で中止）は無効化しない、T031（処理開始）は無効化して必ず戻す。この2つを取り違えるとアプリの再起動が必要になる
- **T041 の HTML コメント除外を省略しない。** 省略すると準備完了チェックリストの中核である項目5が、正しく制定された憲法を「未達」と誤判定する

---

## Notes（注記）

- [P] タスク = 異なるファイル、依存関係なし
- [Story] ラベルは、トレーサビリティのためにタスクを特定のユーザーストーリーに対応付ける
- 検証タスク（T033〜T038、T046、T047、T050、T051〜T056）はコードを書くタスクではなく、[quickstart.md](./quickstart.md) の手順を実施して結果を確認するタスクである
- `C:\work\test\test01` は T035 以降で破壊的に変更される。初期状態から再実行するには [quickstart.md](./quickstart.md) の付録を参照する
- 各タスクまたは論理的なまとまりごとにコミットする
- 各チェックポイントで一旦止まり、ストーリーが独立していることを検証する
