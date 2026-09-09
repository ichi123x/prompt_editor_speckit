---

description: "準備状況の診断機能の廃止のタスクリスト"
---

# Tasks: 準備状況の診断機能の廃止

**Input**: `/specs/003-remove-readiness-check/` からの設計ドキュメント

**Prerequisites（前提条件）**: [plan.md](./plan.md)、[spec.md](./spec.md)、[research.md](./research.md)、[data-model.md](./data-model.md)、[contracts/ui-contract.md](./contracts/ui-contract.md)、[quickstart.md](./quickstart.md)

**Tests（テスト）**: 仕様の「テスト方針」がテストを明示的に要求している。静的テスト（ST-201〜206）と動的テスト（DT-201〜209）を検証タスクとして含める。自動テストランナーは導入しないため、これらはコードではなく**実施手順**として遂行するタスクである。

**Organization（構成）**: タスクはユーザーストーリーごとにグループ化される。ただし本フィーチャーは**既存実装からの削除**が中心であり、削除作業そのものは分割できないため Phase 2 にまとめる。ユーザーストーリーのフェーズは主に検証で構成される。

**⚠️ 本フィーチャー固有の注意**: 主リスクは「消し過ぎ」である。削除タスクには、**隣接する何を残すか**を明記してある。特に T007（無効化リスト）と T008（案内生成の取り違え）は巻き添えが起きやすい。

## Format（形式）: `[ID] [P?] [Story] Description`

- **[P]**: 並行実行可能（異なるファイル、依存関係なし）
- **[Story]**: このタスクがどのユーザーストーリーに属するか（US1、US2）
- 説明には正確なファイルパスを含める

## Path Conventions（パスの慣例）

既存の `MFCApplication1/` を変更する。新規ソースファイルは作らない。すべてのパスはリポジトリルートからの相対パスで記載する。

---

## Phase 1: Setup（テスト環境の準備）

**目的**: 検証に使うフォルダを初期状態へ戻す

- [X] T001 [quickstart.md](./quickstart.md) 手順0に従い、`C:\work\test\test01` から `.git`・`.gitignore`・`.specify`・`.claude` を削除して初期状態へ戻す。**ソリューション本体（`test01.sln` 等）を消さないよう、実行前にパスを確認すること**

---

## Phase 2: Foundational（診断機能の除去）

**目的**: 診断に関わるコード・コントロール・ファイルを取り除く。両ユーザーストーリーが依存する

**⚠️ 重要**: 削除は**ダイアログ側（利用者）から始め、最後に `ReadinessChecker` 本体と `vcxproj` の登録を外す**。逆順にすると、ダイアログが参照したままビルドが通らない期間が長くなる（[plan.md](./plan.md) の Structure Decision）

- [X] T002 `MFCApplication1/MFCApplication1Dlg.cpp` のメッセージマップから `ON_BN_CLICKED(IDC_BUTTON_DIAGNOSE, ...)` の1行を削除する（FR-201）
- [X] T003 `MFCApplication1/MFCApplication1Dlg.cpp` から `OnBnClickedButtonDiagnose()` の実装を削除する（FR-201、FR-202）。T002 に依存
- [X] T004 `MFCApplication1/MFCApplication1Dlg.cpp` から `ShowReadinessGuide()` の実装（約50行）を削除する。**`ShowCompletionGuide()` は削除してはならない** — 名前が似ており宣言も隣接しているが、こちらは `OnSetupCompletedMessage()` から呼ばれ診断結果を参照しない。SDD化の後に次の手順を伝える唯一の手段であり、002 の FR-110〜112 で読みやすさとコピー可能性を確保したばかりである（FR-203、FR-205 / [research.md](./research.md) R-204(2)）。T003 に依存
- [X] T005 `MFCApplication1/MFCApplication1Dlg.h` から `#include "ReadinessChecker.h"`、`OnBnClickedButtonDiagnose()` の宣言、`ShowReadinessGuide()` の宣言を削除する。**`#include <vector>` は残す**（`m_steps` が `std::vector<SetupStep>` を使う。[research.md](./research.md) R-204(4)）。`ShowCompletionGuide()` の宣言も残す。T004 に依存
- [X] T006 `MFCApplication1/MFCApplication1.rc` の `IDD_MFCAPPLICATION1_DIALOG` から診断ボタンのコントロール定義（`PUSHBUTTON "準備状況を診断",IDC_BUTTON_DIAGNOSE,85,30,70,18`）を削除する。**残るコントロールの位置は変更しない** — 診断ボタンは「生成」と同じ行にあったため縦の空きは生じず、「生成」は既に x=7 で左端に揃っている（FR-208 / [contracts/ui-contract.md](./contracts/ui-contract.md)「2. レイアウト」）
- [X] T007 `MFCApplication1/MFCApplication1Dlg.cpp` の `SetControlsEnabled()` のコントロール配列から `GetDlgItem(IDC_BUTTON_DIAGNOSE)` の**1要素のみ**を削除する。**配列の前後（`IDC_BUTTON_GENERATE` と `IDC_BUTTON_BROWSE`）を巻き添えで削除してはならない** — 生成ボタンの無効化は二重実行防止（001 FR-014）、参照ボタンの無効化は 001 の analyze で HIGH として指摘し追加した項目であり、処理中にフォルダを変更されるとワーカースレッドの処理対象と画面表示が食い違う。002 の DT-107 と同じ構図の誤りである（FR-209 / [research.md](./research.md) R-204(1)）。削除後は生成・参照の2コントロールが対象として残る。T002 に依存
- [X] T008 `MFCApplication1/Resource.h` から `IDC_BUTTON_DIAGNOSE`（1005）の定義を削除する。削除した ID は再利用しない。`_APS_NEXT_CONTROL_VALUE` は 1008 のまま変更しない（FR-201、FR-204）。T006、T007 に依存
- [X] T009 `MFCApplication1/TargetFolderState.h` から `hasGitignore` フィールドを削除し、`MFCApplication1/TargetFolderState.cpp` の `Read()` から `.gitignore` の存在確認と代入処理を削除する。**このフィールドは診断削除により書き込みのみの孤立フィールドになる**（読み出しが `ReadinessChecker` にしか無い。[research.md](./research.md) R-202）。`hasSpecifyDir`・`hasClaudeDir`・`commitCount`・`gitState`・`HasPartialScaffold()` は**いずれも削除してはならない** — `SetupRunner` と検証チェーンが使っている（FR-204）
- [X] T010 `MFCApplication1/ReadinessChecker.h` と `MFCApplication1/ReadinessChecker.cpp` を**ファイルごと削除**する（FR-202、FR-204 / SC-204）。T005、T009 に依存
- [X] T011 `MFCApplication1/MFCApplication1.vcxproj` と `MFCApplication1/MFCApplication1.vcxproj.filters` から `ReadinessChecker.h` / `ReadinessChecker.cpp` の登録を解除する。T010 に依存
- [X] T012 削除した各箇所に、対応する FR 番号（FR-201〜FR-209）をコメントで記載する。憲法 原則IV（変更の追跡可能性）に基づき、静的テスト ST-205 で検査する。T002〜T011 に依存

**チェックポイント**: 診断機能が実装から消え、ビルドが通る状態

---

## Phase 3: User Story 1 - SDD化だけに専念した画面で操作する (Priority: P1) 🎯 MVP

**目標**: 画面から診断ボタンが消え、フォルダ選択と「生成」だけの一本道になる

**独立したテスト**: アプリを起動して診断ボタンが存在しないことを確認し、`C:\work\test\test01` を選んで「生成」を押してSDD化が完走することを確認する

- [X] T013 [US1] [quickstart.md](./quickstart.md) 手順2の ST-201（UIリソース）と ST-202（未使用コード）を実施する。削除されているべき語（`ReadinessChecker`・`ReadinessItem`・`ReadinessVerdict`・`BUTTON_DIAGNOSE`・`OnBnClickedButtonDiagnose`・`ShowReadinessGuide`・`hasGitignore`）の出現数が**すべて0**であること。`ReadinessChecker.h` / `.cpp` が存在せず、`vcxproj` からも登録が消えていること。T011 に依存
- [X] T014 [US1] [quickstart.md](./quickstart.md) 手順3の DT-201・DT-202 を実施する。画面に診断ボタンが存在せず、「生成」ボタンが左端に1つだけ並び、レイアウトが崩れておらず、縦方向に不自然な空きが無いこと。Tab キーの巡回順序が `パス欄 → 参照 → 生成 → 一覧 → OK → キャンセル` であること（FR-208）。T012 に依存
- [X] T015 [P] [US1] [quickstart.md](./quickstart.md) 手順4の DT-204（フォルダ未選択で「生成」を押下・回帰確認）を実施し、確認項目(a)〜(e)を確認する。特に(d)「メッセージを閉じた後も生成ボタンが押下可能」は 001 から継承した契約
- [X] T016 [US1] [quickstart.md](./quickstart.md) 手順5の DT-203（SDD化が完走・回帰確認）と **DT-207（処理中に生成・参照ボタンが無効化される・回帰確認）** を実施する。DT-207 は T007 の巻き添えを検出する要所であり、参照ボタンが押せる状態なら不合格。T001、T012 に依存
- [X] T017 [US1] [quickstart.md](./quickstart.md) 手順7の DT-208（べき等性・回帰確認）を実施する。再実行で5手順すべてが「スキップ」、`git status` に変更が出ず、コミット数が増えないこと。DT-203・DT-204・DT-207・DT-208 がすべて合格することで、001・002 の既存機能が本変更で壊れていない（FR-207）といえる。T016 に依存

**チェックポイント**: 診断ボタンが画面から消え、SDD化は従来どおり動作する（MVP 達成）

---

## Phase 4: User Story 2 - 生成後の案内は引き続き表示される (Priority: P2)

**目標**: 診断削除の巻き添えで完了案内が失われていないことを確認する

**独立したテスト**: 「生成」を実行した後、案内欄に5行の定型メッセージが表示され、末尾まで読めてコピーできることを確認する

- [X] T018 [US2] [quickstart.md](./quickstart.md) 手順2の ST-204（完了案内の残存検査）を実施する。`ShowCompletionGuide`・`Spec Kit の導入が完了しました`・`init を再実行し`・`AppendResultRow`・`ClearResultList` の出現数が**すべて1以上**であること。**0 が出たら不合格** — `ShowCompletionGuide()` を `ShowReadinessGuide()` と取り違えて削除している。T011 に依存
- [X] T019 [US2] [quickstart.md](./quickstart.md) 手順6の DT-205（完了案内の回帰確認）を実施する。確認項目(a)〜(d) — 5行の定型案内が表示され、最後の行「`4. /init を再実行し、憲章を反映した CLAUDE.md に更新する`」まで読め、ドラッグ選択して Ctrl+C でコピーでき、文字を入力しても編集できないこと。**002 の FR-110〜112 で確保したばかりの機能である**。T016 に依存
- [X] T020 [P] [US2] [quickstart.md](./quickstart.md) 手順3の DT-206 を実施する。初期案内が「対象フォルダを選び、「生成」を押してください。…」であり、**診断への言及が無い**こと（FR-206）。T014 に依存

**チェックポイント**: 両ユーザーストーリーが独立して動作する

---

## Phase 5: Polish & Cross-Cutting Concerns（仕上げと横断的関心事）

- [X] T021 [quickstart.md](./quickstart.md) 手順1の ST-203 を実施する。`Debug`/`Release` × `x64`/`x86` の4構成すべてで MSBuild が成功し、**未使用変数・未使用関数の警告が0件**であること。削除の消し残しはこの警告に現れる。`.sln` に渡すプラットフォーム名は `x86`（`Win32` では `error MSB4126`）。**ビルド前にアプリを終了させること**（`LNK1104` になる）。T011 に依存
- [X] T022 [P] [quickstart.md](./quickstart.md) 手順2の ST-205 を実施する。削除・変更した各箇所が FR-201〜FR-209 のどれに対応するかコード上で追跡できること。不足していればコメントを追加する。T012 に依存
---

## Phase 6: 欠陥修正 — コミット対象が無いときの結果表示（FR-210、FR-211）

**目的**: 002 の手動テスト中に発見した、Git 手順の結果表示の誤りを修正する

**発見の経緯**: `test01` に対する「生成」で Git 手順が「失敗」と表示され、内容欄に `On branch main Untracked files:...` が出た。データは何も壊れておらず（コミット1件、追記ブロック各1回のまま）、表示だけが誤っていた。実装は `nothing to commit` を探していたが、Git が実際に出力するのは `nothing added to commit but untracked files present` であり、前者は後者の部分文字列ではない。001 の実装時に Git の出力を実測せず推測で書いた箇所である。

**このフェーズは診断削除と独立している。** `SetupRunner.cpp` は Phase 2 で触らないファイルのため、Phase 2〜5 と並行して進めてよい。

- [X] T024 `MFCApplication1/SetupRunner.cpp` の `RunInitGit()` で、コミットの結果判定に用いる文字列を実測値に合わせて修正する。`nothing to commit` は Git の実出力 `nothing added to commit but untracked files present` の部分文字列ではないため一致しない。**両方の文言に対応させること** — リポジトリの状態によって Git は `nothing to commit, working tree clean` と `nothing added to commit but untracked files present` を出し分けるため、`nothing added to commit` と `nothing to commit` の双方を判定対象とする。該当時は `StepResult::Skipped` とし、理由欄に「コミットすべき変更がありませんでした。」を表示する（FR-210、FR-211）
- [X] T025 `MFCApplication1/SetupRunner.cpp` の該当箇所に、**判定文字列は推測ではなく実測に基づくこと**をコメントで明記する。実測した Git の出力例を添え、文字列を変更する場合は再度実測するよう促す（FR-211 / [spec.md](./spec.md)「コミット対象が無いときの結果表示について」）。T024 に依存
- [X] T026 ST-206 を実施する。`MFCApplication1/SetupRunner.cpp` の判定文字列を、実環境の Git が出力する文言と突き合わせる。コミット対象が無い状態を実際に作って `git commit` を実行し、出力に判定文字列が含まれることを確認する。T024 に依存
- [X] T027 DT-209 を実施する。既にコミット済みで `.gitignore` の内容がコミット済みのものと同一、かつ未追跡ファイルが存在するフォルダに対して「生成」を押し、Git 手順が**「失敗」ではなく「スキップ」**と表示されることを確認する。理由欄に「コミットすべき変更がありませんでした。」が出ること。T024、T016 に依存

---

## Phase 7: 通し実行

- [X] T023 [quickstart.md](./quickstart.md) の付録に従って `C:\work\test\test01` をリセットし、手順1〜7を通しで実行して「完了条件」の6項目をすべて確認する。T013〜T022 および Phase 6 の全タスクに依存

---

## Dependencies & Execution Order（依存関係と実行順序）

### Phase Dependencies（フェーズの依存関係）

- **Setup (Phase 1)**: 依存関係なし
- **Foundational (Phase 2)**: すべてのユーザーストーリーをブロックする
- **User Stories (Phase 3〜4)**: Foundational の完了に依存
- **Polish (Phase 5)**: 両ユーザーストーリーの完了に依存

### ビルドが通らない期間について

削除の順序をダイアログ側から始めるため、**ビルドが通らない期間は T005 完了から T011 完了までに限られる**。

| 時点 | ビルド | 理由 |
|---|---|---|
| T002〜T004 完了 | ✓ | ダイアログから参照が減っただけ。`ReadinessChecker` はまだ存在する |
| T005 完了 | ✓ | include を外したが、`ReadinessChecker` への参照も同時に消えている |
| T010 完了 | ✗ | ファイルを消したが `vcxproj` にまだ登録が残っている |
| **T011 完了** | **✓** | 登録を解除して整合する |

T010 と T011 は一続きの作業として扱うこと。

### User Story Dependencies（ユーザーストーリーの依存関係）

- **User Story 1 (P1)**: Foundational の後に開始可能
- **User Story 2 (P2)**: 検証タスク T019 は US1 の T016（SDD化の実行）に依存する。T018・T020 は独立して実施できる

### 重要な依存関係

| タスク | 依存先 | 理由 |
|---|---|---|
| T008（Resource.h） | T006, T007 | ID を使う箇所を先に消す |
| T010（ファイル削除） | T005, T009 | 参照元をすべて消してから本体を消す |
| T011（vcxproj） | T010 | ファイルが無くなってから登録を解除する |
| T016（DT-203/207） | T001, T012 | 初期状態のフォルダと、完成した実装の両方が必要 |
| T019（DT-205） | T016 | SDD化を実行してから案内を確認する |
| T023（通し実行） | T013〜T022 | 全検証が揃った状態で実施する |

### Parallel Opportunities（並行実行の機会）

- **Phase 2**: T002〜T005・T007 は同一ファイル（`MFCApplication1Dlg.cpp` / `.h`）のため**並行不可**。T006（`.rc`）と T009（`TargetFolderState`）は別ファイルのため他と並行可能
- **Phase 3**: T015 は他の検証と独立
- **Phase 4**: T018 と T020 は互いに独立
- **Phase 5**: T022 は他と並行可能

---

## Parallel Example: Phase 2（並行実行の例）

```text
Task: "MFCApplication1/MFCApplication1.rc から診断ボタンの定義を削除する"
Task: "MFCApplication1/TargetFolderState から hasGitignore を削除する"
```

---

## Implementation Strategy（実装戦略）

### MVP First（User Story 1 のみ）

1. Phase 1: テスト環境を初期状態へ戻す（T001）
2. Phase 2: 診断機能を除去する（T002〜T012）
3. **T011 の時点でビルドが通る。** ここで一度確認する
4. Phase 3: User Story 1 を検証する（T013〜T017）
5. **停止して検証**: T014（DT-201/202）と T016（DT-203/207）を実施する

### 実装順序に関する注意

- **T004 で `ShowCompletionGuide()` を巻き添えで消さない。** `ShowReadinessGuide()` とは名前が似ており宣言も隣接している。消すと SDD化の後に次の手順が一切案内されなくなる。この誤りは T018（ST-204）と T019（DT-205）でしか検出できない
- **T007 で配列の前後を巻き添えにしない。** 削除するのは真ん中の1要素のみ。参照ボタンを外すと、処理中にフォルダを変更したときだけ食い違うという再現条件の限られた不具合になる
- **T009 で `hasSpecifyDir` / `hasClaudeDir` / `commitCount` を消さない。** `hasGitignore` だけが孤立する。他は `SetupRunner` と検証チェーンが使っている
- **T005 で `#include <vector>` を消さない。** `ReadinessChecker.h` 経由で使っていたが、`m_steps` でも使う

### Incremental Delivery（段階的な提供）

1. Setup + Foundational → 診断機能が実装から消える
2. User Story 1 → 画面が単純になり、SDD化は従来どおり動く（**MVP**）
3. User Story 2 → 完了案内が失われていないことを確認
4. Phase 5 → ビルドの健全性と通し実行

---

## Notes（注記）

- [P] タスク = 異なるファイル、依存関係なし
- [Story] ラベルは、トレーサビリティのためにタスクを特定のユーザーストーリーに対応付ける
- 検証タスク（T013〜T023）はコードを書くタスクではなく、[quickstart.md](./quickstart.md) の手順を実施して結果を確認するタスクである
- 番号体系が 001・002 と並存する。本書で 001・002 の要件を参照する場合は「001 FR-014」のように接頭辞を付けている。接頭辞の無い `FR-2xx` はすべて 003 の要件である
- 本フィーチャーの主リスクは「消し過ぎ」である。削除タスクには隣接する何を残すかを明記してある
- `C:\work\test\test01` は検証で破壊的に変更される。リセット手順は [quickstart.md](./quickstart.md) の付録を参照
- 各タスクまたは論理的なまとまりごとにコミットする。ただし T010〜T011 はビルドが通らない期間があるため1つのまとまりとして扱ってよい
