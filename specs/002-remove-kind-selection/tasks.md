---

description: "プロジェクト種別選択の廃止のタスクリスト"
---

# Tasks: プロジェクト種別選択の廃止

**Input**: `/specs/002-remove-kind-selection/` からの設計ドキュメント

**Prerequisites（前提条件）**: [plan.md](./plan.md)、[spec.md](./spec.md)、[research.md](./research.md)、[data-model.md](./data-model.md)、[contracts/ui-contract.md](./contracts/ui-contract.md)、[quickstart.md](./quickstart.md)

**Tests（テスト）**: 仕様の「テスト方針」がテストを明示的に要求している。静的テスト（ST-101〜107）と動的テスト（DT-101〜109）を検証タスクとして含める。自動テストランナーは導入しないため、これらはコードではなく**実施手順**として遂行するタスクである。

**Organization（構成）**: タスクはユーザーストーリーごとにグループ化される。ただし本フィーチャーは**既存実装からの削除**が中心であり、種別概念の除去は両ストーリーが依存する共通基盤となるため Phase 2 に置く。

**⚠️ 本フィーチャー固有の注意**: 主リスクは「消し過ぎ」である。削除タスクには、**隣接する何を残すか**を明記してある。特に T013（実行中の無効化リスト）と T015（検証チェーン）は巻き添えが起きやすい。

## Format（形式）: `[ID] [P?] [Story] Description`

- **[P]**: 並行実行可能（異なるファイル、依存関係なし）
- **[Story]**: このタスクがどのユーザーストーリーに属するか（US1、US2）
- 説明には正確なファイルパスを含める

## Path Conventions（パスの慣例）

既存の `MFCApplication1/` を変更する。新規ソースファイルは作らない。すべてのパスはリポジトリルートからの相対パスで記載する。

---

## Phase 1: Setup（リソースの統合）

**目的**: `.gitignore` の埋め込みリソースを1件へ統合し、ID を整理する。コードの変更に入る前の土台

- [X] T001 `MFCApplication1/res/gitignore_all.txt` を新規作成する。既存の `gitignore_vs.txt` と `gitignore_node.txt` を統合し、[research.md](./research.md) R-101 の4セクション構成（共通 / Visual Studio・.NET / Node・Web / 追記ブロックは含めない）とする。**重複していた16パターン（`$RECYCLE.BIN/`・`*.log`・`*.rsuser`・`*.suo`・`*.swp`・`*.user`・`.DS_Store`・`.env`・`.vs/`・`Desktop.ini`・`Thumbs.db`・`[Bb]in/`・`[Oo]bj/`・`artifacts/`・`ehthumbs.db`・`project.lock.json`）は共通セクションへ集約し、他のセクションからは取り除く**。同じパターンが2回現れてはならない（FR-106）
- [X] T002 `MFCApplication1/Resource.h` を更新する。`IDC_RADIO_KIND_MFC`（1002）と `IDC_RADIO_KIND_WEB`（1003）を削除し、`IDR_GITIGNORE_VS`（129）と `IDR_GITIGNORE_NODE`（130）を `IDR_GITIGNORE`（129）1件へ統合する。`_APS_NEXT_RESOURCE_VALUE` を 130 に戻す。`_APS_NEXT_CONTROL_VALUE` は 1008 のまま変更しない（削除した ID の再利用を避けるため）。FR-101、FR-107
- [X] T003 `MFCApplication1/MFCApplication1.rc` の RCDATA セクションを、`IDR_GITIGNORE` → `res\gitignore_all.txt` の1件のみに書き換える（T001、T002 に依存）
- [X] T004 `MFCApplication1/res/gitignore_vs.txt` と `MFCApplication1/res/gitignore_node.txt` を削除する（FR-107。T003 に依存）

**チェックポイント**: リソースが1件に統合された状態。この時点ではコードがまだ `IDR_GITIGNORE_VS` を参照しているためビルドは通らない

---

## Phase 2: Foundational（種別概念の除去）

**目的**: `ProjectKind` を実装から消し、`kind` を受け渡していた関数シグネチャから引数を落とす。両ユーザーストーリーが依存する

**⚠️ 重要**: このフェーズが完了するまで、どのユーザーストーリーの作業も開始できない。またフェーズ途中はビルドが通らない期間が発生する。T005〜T009 は一続きの作業として扱うこと

- [X] T005 `MFCApplication1/TargetFolderState.h` から `enum class ProjectKind` を削除する。`hasSlnMarker` と `hasWebMarker` を `hasProjectMarker`（bool、1つ）へ統合する。`HasMarkerFor(ProjectKind)` と `KindDisplayName(ProjectKind)` の宣言を削除する。`IsSelected()` と `HasPartialScaffold()` は変更しない（[data-model.md](./data-model.md)「2. 変更するエンティティ」）。FR-103、FR-107
- [X] T006 `MFCApplication1/TargetFolderState.cpp` から `HasMarkerFor()` と `KindDisplayName()` の実装を削除する。`Read()` 内のマーカー検出を `hasProjectMarker` への集約に変更する。**探すファイルの集合（`*.sln`・`*.vcxproj`・`*.csproj`・`package.json`）は変更しない**。結果を2つのフラグに分けて保持するのをやめるだけで、検出ロジック自体は 001 のまま。FR-103。T005 に依存
- [X] T007 `MFCApplication1/GitignoreWriter.h` と `MFCApplication1/GitignoreWriter.cpp` を更新する。`Ensure()` から `ProjectKind kind` 引数を削除し、`kind == ProjectKind::None` の早期リターンと `IDR_GITIGNORE_VS` / `IDR_GITIGNORE_NODE` の三項分岐を削除して、常に `IDR_GITIGNORE` を読み出すようにする。**追記ブロックの目印行による判定とべき等な追記の挙動は変更しない**（FR-106a）。FR-106。T002、T005 に依存
- [X] T008 `MFCApplication1/SetupRunner.h` と `MFCApplication1/SetupRunner.cpp` を更新する。`RunAll()` と `RunInitGit()` から `ProjectKind kind` 引数を削除し、`GitignoreWriter::Ensure()` の呼び出しから `kind` を落とす。**5手順の定義・依存関係・スキップ条件は変更しない**（FR-108）。T007 に依存
- [X] T009 `MFCApplication1/ReadinessChecker.cpp` の項目1の判定を `state.hasSlnMarker || state.hasWebMarker` から `state.hasProjectMarker` へ置き換える。**両者は同値であり判定結果は変わらない**。項目1〜7の判定方法・取りうる Verdict・NextAction の文言はいずれも変更しない（FR-108）。T005 に依存

**チェックポイント**: `ProjectKind` が実装から消えた。ダイアログ側がまだ `m_kind` を持つためビルドは通らない

---

## Phase 3: User Story 1 - フォルダを選ぶだけでSDD化する (Priority: P1) 🎯 MVP

**目標**: 種別を選ぶ操作を画面から消し、フォルダ選択と「生成」の2操作だけでSDD化が完了する

**独立したテスト**: `C:\work\test\test01` を選択し、種別を選ぶ操作が存在しないことを確認したうえで「生成」をクリックし、`.specify`・`.claude`・`.git`・`.gitignore` が揃うことを確認する

### ダイアログの変更（FR-101、FR-102、FR-105、FR-109）

- [X] T010 [US1] `MFCApplication1/MFCApplication1.rc` の `IDD_MFCAPPLICATION1_DIALOG` から、グループボックス「プロジェクト種別」とラジオボタン2件を削除する。[contracts/ui-contract.md](./contracts/ui-contract.md)「2. レイアウト」に従い、`IDC_BUTTON_GENERATE` を (7, 30, 70, 18)、`IDC_BUTTON_DIAGNOSE` を (85, 30, 70, 18)、`IDC_LIST_RESULT` を (7, 52, 306, 90) へ変更する。記述順を「ラベル → エディット → 参照 → 生成 → 診断 → 一覧 → 案内 → OK → キャンセル」とし、タブ順序を保つ（FR-109）。T002 に依存
- [X] T011 [US1] `MFCApplication1/MFCApplication1Dlg.h` から `m_kind` と `m_workerKind` のメンバ、`OnBnClickedRadioKindMfc()` と `OnBnClickedRadioKindWeb()` の宣言を削除する。`m_targetPath`・`m_running`・`m_workerPath` は残す。T005 に依存
- [X] T012 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` のメッセージマップから `ON_BN_CLICKED(IDC_RADIO_KIND_MFC, ...)` と `ON_BN_CLICKED(IDC_RADIO_KIND_WEB, ...)` の2行を削除し、`OnBnClickedRadioKindMfc()` と `OnBnClickedRadioKindWeb()` の実装を削除する。T011 に依存
- [X] T013 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` の `OnInitDialog()` から `CheckDlgButton(IDC_RADIO_KIND_MFC, BST_UNCHECKED)` と `CheckDlgButton(IDC_RADIO_KIND_WEB, BST_UNCHECKED)` の2行、および `m_kind = ProjectKind::None;` を削除する。初期案内文から種別への言及を取り除き、[contracts/ui-contract.md](./contracts/ui-contract.md)「4. 初期表示の案内文」の内容に差し替える。T011 に依存
- [X] T014 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` の `SetControlsEnabled()` のコントロール配列から、`GetDlgItem(IDC_RADIO_KIND_MFC)` と `GetDlgItem(IDC_RADIO_KIND_WEB)` の**2要素のみ**を削除する。**`GetDlgItem(IDC_BUTTON_BROWSE)` を巻き添えで削除してはならない** — 参照ボタンの無効化は 001 の analyze で HIGH として指摘し追加した項目であり、処理中にフォルダを変更されるとワーカースレッドの処理対象と画面表示が食い違う（[research.md](./research.md) R-104(1)）。削除後は生成・診断・参照の3コントロールが対象として残る。T011 に依存
- [X] T015 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` の `ValidateBeforeGenerate()` から、種別未選択の検証（`if (m_kind == ProjectKind::None)`）と種別不一致の検証（`HasMarkerFor(other)` の分岐と確認ダイアログ）を削除する。マーカーの確認を `state.hasProjectMarker` による単純な有無判定へ置き換える。**残る検証の順序を変えてはならない** — フォルダ未選択 → 書き込み権限 → マーカー → 足場が中途半端 → 入れ子リポジトリ の順を維持する。特に「書き込み権限の確認」が「マーカーの確認」より先である点は、権限が無いフォルダでマーカーを探しても意味がないため（[research.md](./research.md) R-104(2)）。FR-102、FR-103、FR-105。T005、T011 に依存
- [X] T016 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` の `OnBnClickedButtonGenerate()` から `m_workerKind = m_kind;` を削除し、`SetupThreadProc()` の `SetupRunner::RunAll()` 呼び出しから `dialog->m_workerKind` を落とす。**ワーカースレッドへ対象フォルダパスのコピーを渡す挙動は維持する**（実行中に UI が変更されても処理対象が変わらないため）。T008、T011 に依存
- [X] T017 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` から、種別に関する2つのメッセージ文字列（「プロジェクトの種別を選択してください。」「選択された種別と、フォルダ内のプロジェクトが一致しません。…」）が残っていないことを確認し、残っていれば削除する（FR-107）。T015 に依存
- [X] T018 [US1] 変更した各箇所に、対応する FR 番号（FR-101〜FR-109）をコメントで記載する。憲法 原則IV（変更の追跡可能性）に基づき、静的テスト ST-105 で検査する。T010〜T017 に依存

### User Story 1 の検証

- [X] T019 [US1] [quickstart.md](./quickstart.md) 手順2の ST-101（UIリソース）と ST-102（未使用コード）を実施する。削除されているべき語（`ProjectKind`・`RADIO_KIND`・`hasSlnMarker`・`hasWebMarker`・`HasMarkerFor`・`KindDisplayName`・`m_kind`・`m_workerKind`・種別関連の2メッセージ）の出現数が**すべて0**であることを確認する
- [X] T020 [US1] [quickstart.md](./quickstart.md) 手順3の DT-101 を実施する。画面に種別選択のコントロールが存在せず、ボタンが左端に並び、削除による空白が残っていないこと。Tab キーの巡回順序が `パス欄 → 参照 → 生成 → 診断 → 一覧 → OK → キャンセル` であること（FR-109）。種別を選ぶ操作が存在しないこと自体が SC-101（操作が3回から2回へ）と SC-102（種別判断が不要）の達成を意味する
- [X] T021 [P] [US1] [quickstart.md](./quickstart.md) 手順4の DT-103（フォルダ未選択で「生成」を押下）を実施し、確認項目(a)〜(e)を確認する。特に(d)「メッセージを閉じた後も生成ボタンが押下可能」は 001 から継承した契約
- [ ] T022 [US1] [quickstart.md](./quickstart.md) 手順5の DT-102（フォルダを選ぶだけでSDD化が完走）と **DT-107（処理中に参照ボタンが無効化される）** を実施する。DT-107 は T014 の巻き添えを検出する要所であり、参照ボタンが押せる状態なら不合格。フォルダ選択と「生成」の2操作だけで完走することが SC-101・SC-102 の検証にあたる

**チェックポイント**: 種別選択が画面から消え、フォルダ選択と「生成」の2操作でSDD化が完了する（MVP 達成）

---

## Phase 4: User Story 2 - 対象プロジェクトが無いフォルダで安全に停止する (Priority: P2)

**目標**: 種別選択の廃止後も、プロジェクトが無いフォルダでは案内して停止し、雛形を生成しない

**独立したテスト**: 空のフォルダを選択して「生成」をクリックし、案内メッセージが表示されて `.specify`・`.git` などが作成されないことを確認する

- [X] T023 [US2] `MFCApplication1/MFCApplication1Dlg.cpp` で、`hasProjectMarker` が false のときにウィザードでの作成を促すメッセージを表示して中止する経路が、T015 の変更後も維持されていることを確認する。**失われていれば復元する**（T015 で不一致の分岐を削除する際に、マーカー不在の分岐まで巻き添えで消す誤りが起こりうる）。メッセージ文言（「対象のプロジェクトが見つかりません。Visual Studio 2022 などのウィザードで…」）は 001 から変更しない。**本ツールは引き続き雛形を保持も生成もしない**（FR-104）。T015 に依存
- [ ] T024 [US2] [quickstart.md](./quickstart.md) 手順10の DT-104 を実施する。`C:\work\test\empty01` で「生成」を押し、案内が表示され、**フォルダが空のままである**ことを確認する（雛形が生成されないこと）
- [ ] T025 [US2] [quickstart.md](./quickstart.md) 手順8の DT-106（診断の回帰確認）を実施する。7項目の判定が 001 と同じであること。項目1の判定条件を `hasSlnMarker || hasWebMarker` から `hasProjectMarker` へ置き換えたが同値であり、結果が変わっていたら統合時に条件を誤っている。診断の前後でフォルダが変化しないことも確認する（001 FR-020 の回帰）。T009 に依存
- [ ] T026 [P] [US2] [quickstart.md](./quickstart.md) 手順11（US1-5）を実施する。`test01` に `package.json` を追加して MFC と Web の両方のマーカーがある状態にし、「生成」を押して**不一致の警告や確認ダイアログが出ない**ことを確認する（FR-105）。確認後 `package.json` を削除する

**チェックポイント**: 両ユーザーストーリーが独立して動作する

---

## Phase 5: Polish & Cross-Cutting Concerns（仕上げと横断的関心事）

**目的**: ビルドの健全性、統合 `.gitignore` の検証、回帰の総点検

- [X] T027 [quickstart.md](./quickstart.md) 手順1の ST-103 を実施する。`Debug`/`Release` × `x64`/`x86` の4構成すべてで MSBuild が成功し、**未使用変数・未使用関数の警告が0件**であること。削除の消し残しはこの警告に現れる。`.sln` に渡すプラットフォーム名は `x86`（`Win32` では `error MSB4126` になる）
- [X] T028 [quickstart.md](./quickstart.md) 手順6の ST-104 を実施する。生成された `.gitignore` に Visual Studio 系（`[Dd]ebug/`・`.vs/`・`*.vcxproj.user`・`*.aps`）と **Node 系（`node_modules/`・`dist/`・`.env`）の双方**が含まれること。MFCプロジェクトでも Node 系が入るのが正しい。追記ブロックの目印行が各1回のみで、同一パターンの重複が無いこと。T022 に依存
- [ ] T029 [quickstart.md](./quickstart.md) 手順9の **ST-106 / DT-108** を実施する。`C:\work\test\web01` でもSDD化を実行し、`test01` と `web01` の `.gitignore` を SHA256 で突き合わせて**完全に一致**することを確認する（SC-105）。不一致なら、フォルダの内容によって `.gitignore` を切り替える分岐が実装に残っている（FR-106 違反）。あわせて、MFC と Web で操作手順が完全に同一であったことを確認する（SC-104）。T022、T028 に依存
- [ ] T030 [quickstart.md](./quickstart.md) 手順7の DT-105（べき等性の回帰確認）を実施する。再実行で5手順すべてが「スキップ」、`git status` に変更が出ず、コミット数が増えず、追記ブロックが重複しないこと。DT-105・DT-106・DT-107 がすべて合格することで、001 の検証項目が再現したといえる（SC-103）。T022 に依存
- [X] T031 [P] [quickstart.md](./quickstart.md) 手順2の ST-105 を実施する。変更した各箇所が FR-101〜FR-109 のどれに対応するかコード上で追跡できること。不足していればコメントを追加する
- [ ] T032 [quickstart.md](./quickstart.md) 手順9bの **ST-107 / DT-109** を実施する。独自内容の `.gitignore` を置いた `C:\work\test\web01` で「生成」を実行し、確認項目(a)〜(d)を確認する。特に **(b)「統合版で丸ごと上書きされていない」** が要所であり、上書きしていれば FR-106a 違反でユーザーが手で書いた除外設定を失わせることになる。再実行で追記ブロックが増えないこと（FR-106b）も確認する。T029 に依存
- [ ] T033 [quickstart.md](./quickstart.md) の付録に従って `test01`・`web01`・`empty01` をリセットし、手順1〜11を通しで実行して「完了条件」の5項目をすべて確認する。T027〜T032 および Phase 3〜4 の全検証タスクに依存

---

## Dependencies & Execution Order（依存関係と実行順序）

### Phase Dependencies（フェーズの依存関係）

- **Setup (Phase 1)**: 依存関係なし — すぐに開始可能
- **Foundational (Phase 2)**: Setup の完了に依存 — すべてのユーザーストーリーをブロックする
- **User Stories (Phase 3〜4)**: Foundational の完了に依存
- **Polish (Phase 5)**: 両ユーザーストーリーの完了に依存

### ビルドが通らない期間について

本フィーチャーは協調的なシグネチャ変更を含むため、**Phase 1 の途中から Phase 3 の T016 完了までビルドが通らない**。これは削除作業の性質上避けられない。

| 時点 | ビルド | 理由 |
|---|---|---|
| T001〜T004 完了 | ✗ | コードが `IDR_GITIGNORE_VS` を参照している |
| T005〜T009 完了 | ✗ | ダイアログが `m_kind` を持っている |
| **T016 完了** | **✓** | `kind` の供給と消費が両方消えて整合する |

途中でビルドを確認したい場合は T016 まで進めること。T005〜T009 と T010〜T016 は一続きの作業として扱う。

### User Story Dependencies（ユーザーストーリーの依存関係）

- **User Story 1 (P1)**: Foundational の後に開始可能。他のストーリーへの依存なし
- **User Story 2 (P2)**: 実装タスク T023 は US1 の T015（検証チェーンの変更）に依存する。検証タスク T024〜T026 も US1 の実装完了後に実施する

### 重要な依存関係

| タスク | 依存先 | 理由 |
|---|---|---|
| T003（RCDATA 更新） | T001, T002 | 統合ファイルと ID の両方が必要 |
| T007（GitignoreWriter） | T002, T005 | 新 ID と `ProjectKind` 削除の両方が必要 |
| T015（検証チェーン） | T005, T011 | `hasProjectMarker` と `m_kind` 削除の両方が必要 |
| T022（DT-102/DT-107） | T014 | 無効化リストの変更を検証する |
| T029（ST-106/DT-108） | T022, T028 | 2つのフォルダでSDD化を済ませてから比較する |
| T032（通し実行） | Phase 3〜5 の全検証 | 全機能が揃った状態で実施する |

### Parallel Opportunities（並行実行の機会）

- **Phase 1**: T001（統合ファイル作成）と T002（Resource.h）は別ファイルのため並行可能
- **Phase 2**: T005〜T009 は依存が直列に連なるため**並行不可**
- **Phase 3**: T010（`.rc`）と T011（`Dlg.h`）は別ファイルのため並行可能。T012〜T017 は同一ファイル（`MFCApplication1Dlg.cpp`）のため**並行不可**
- **検証**: T021 と T026 は互いに独立。T031 は他の検証と並行可能

---

## Parallel Example: Phase 1（並行実行の例）

```text
Task: "MFCApplication1/res/gitignore_all.txt を統合して作成する"
Task: "MFCApplication1/Resource.h の ID を整理する"
```

---

## Implementation Strategy（実装戦略）

### MVP First（User Story 1 のみ）

1. Phase 1: Setup を完了する（T001〜T004）
2. Phase 2: Foundational を完了する（T005〜T009）— すべてのストーリーをブロックする
3. Phase 3: User Story 1 を完了する（T010〜T022）
4. **T016 の時点で初めてビルドが通る。** ここで一度ビルドを確認する
5. **停止して検証**: T020（DT-101）と T022（DT-102/DT-107）を実施する

### 実装順序に関する注意

- **T014 で `IDC_BUTTON_BROWSE` を巻き添えで消さない。** ラジオ2件のみを配列から取り除く。この誤りは DT-107 でしか検出できず、実害は「処理中にフォルダを変更するとワーカーの処理対象と画面表示が食い違う」という再現条件の限られた不具合になる
- **T015 で残る検証の順序を変えない。** 特に書き込み権限の確認をマーカーの確認より後ろへ動かさない
- **T001 で重複16パターンを排除する。** 単純連結すると ST-104 の重複検査が意味を失う
- **T006 でマーカー検出の対象ファイル集合を変えない。** 変えると診断の項目1の判定が 001 と食い違い、DT-106 が落ちる

### Incremental Delivery（段階的な提供）

1. Setup + Foundational → 種別概念が実装から消える（ビルドは通らない）
2. User Story 1 → ビルドが通り、2操作でSDD化できる（**MVP**）
3. User Story 2 → 安全弁の維持を確認
4. Phase 5 → 統合 `.gitignore` の検証と回帰の総点検

---

## Notes（注記）

- [P] タスク = 異なるファイル、依存関係なし
- [Story] ラベルは、トレーサビリティのためにタスクを特定のユーザーストーリーに対応付ける
- 検証タスク（T019〜T022、T024〜T026、T027〜T033）はコードを書くタスクではなく、[quickstart.md](./quickstart.md) の手順を実施して結果を確認するタスクである
- 本フィーチャーの主リスクは「消し過ぎ」である。削除タスクには隣接する何を残すかを明記してある
- 番号体系が 001 と並存する。本書で `FR-020` のように 001 の要件を参照する場合は「001 FR-020」と明示している。接頭辞の無い `FR-1xx` はすべて 002 の要件である。リセット手順は [quickstart.md](./quickstart.md) の付録を参照
- 各タスクまたは論理的なまとまりごとにコミットする。ただしビルドが通らない期間があるため、T016 までは1つのまとまりとして扱ってよい
