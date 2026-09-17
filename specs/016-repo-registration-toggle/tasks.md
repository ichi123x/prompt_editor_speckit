---

description: "機能実装用のタスクリストテンプレート"
---

# Tasks: リポジトリ登録の要否を選択できるようにする

**Input**: `/specs/016-repo-registration-toggle/` からの設計ドキュメント

**Prerequisites（前提条件）**: [plan.md](./plan.md)、[spec.md](./spec.md)、[research.md](./research.md)、[data-model.md](./data-model.md)、[contracts/ui-contract.md](./contracts/ui-contract.md)、[quickstart.md](./quickstart.md)

**Tests（テスト）**: 自動テストスイートは存在しない（憲章の技術的制約）。検証はquickstart.mdのST-xxx/DT-xxxをタスクとして組み込む。

**Organization（構成）**: 実際のコード変更（コントロール追加・`SetupRunner`のシグネチャ変更）はFoundationalフェーズに集約する。両ユーザーストーリーがこの同一の変更に依存し、コードを「OK専用」「NG専用」に分けて実装することはできないためである。User Storyフェーズはそれぞれの経路（NG＝US1、OK＝US2）の検証に専念する。

## Format（形式）: `[ID] [P?] [Story] Description`

- **[P]**: 並行実行可能（異なるファイル、依存関係なし）
- **[Story]**: US1、US2
- 説明には正確なファイルパスを含める

## Path Conventions（パスの慣例）

既存の単一MFCプロジェクト構成をそのまま使用する。変更対象は `MFCApplication1/MFCApplication1.rc`・`Resource.h`・`SetupRunner.h`・`SetupRunner.cpp`・`MFCApplication1Dlg.h`・`MFCApplication1Dlg.cpp` の6ファイル（[plan.md](./plan.md) Project Structure参照）。

---

## Phase 1: Setup（共有インフラ）

**目的**: 測定・ビルドの前提となる環境を整える

- [X] T001 `MFCApplication1.exe` を終了させたうえで `MFCApplication1.sln` をDebug x64でリビルドし、以後の変更がこのビルドを基準にすることを確認する

---

## Phase 2: Foundational（ブロッキングとなる前提条件）

**目的**: 新規コントロールの追加と`SetupRunner`のシグネチャ変更。US1（NG経路）・US2（OK経路）の両方がこの変更を前提とする

**⚠️ 重要**: このフェーズが完了するまで、どのユーザーストーリーの検証も開始できません

### UIコントロールの追加

- [X] T002 `MFCApplication1/Resource.h` に `IDC_STATIC_REPO_LABEL`(1009)・`IDC_RADIO_REPO_OK`(1010)・`IDC_RADIO_REPO_NG`(1011) を追加する。既存コメントの「削除したIDは再利用しない」規約に従い1002・1003・1005は使わない（[research.md](./research.md) R-1606, FR-1601）
- [X] T003 `MFCApplication1/MFCApplication1.rc` の `IDD_MFCAPPLICATION1_DIALOG` に、`IDC_STATIC_REPO_LABEL`（LTEXT「リポジトリの登録：」, 90,34,65,10）・`IDC_RADIO_REPO_OK`（AUTORADIOBUTTON「OK」, 157,34,34,10, `WS_GROUP`）・`IDC_RADIO_REPO_NG`（AUTORADIOBUTTON「NG」, 195,34,34,10）を追加する。ダイアログ本体の寸法（570 x 233 DLU）は変更しない（[data-model.md](./data-model.md) 2節, FR-1601, FR-1609）。**2026-09-18改訂**: 当初90,34,90,10／184,34,40,10／228,34,40,10で実装したが、実機目視で余白が過大との指摘を受け詰めた（[research.md](./research.md) R-1604参照）
- [X] T004 `MFCApplication1/MFCApplication1Dlg.cpp` の `InitAnchors()` に、`IDC_STATIC_REPO_LABEL`・`IDC_RADIO_REPO_OK`・`IDC_RADIO_REPO_NG` を `AnchorMode::Fixed` として3行追加する（[research.md](./research.md) R-1605, FR-1609）
- [X] T005 `MFCApplication1/MFCApplication1Dlg.cpp` の `OnInitDialog()` に `CheckRadioButton(IDC_RADIO_REPO_OK, IDC_RADIO_REPO_NG, IDC_RADIO_REPO_OK)` を追加し、既定で「OK」が選択された状態にする（FR-1601）
- [X] T006 `MFCApplication1/MFCApplication1Dlg.h` に `bool m_workerRegisterRepo = true;` メンバを追加する（[data-model.md](./data-model.md) 3節, R-1602）

### `SetupRunner` のシグネチャ変更 [P]

- [X] T007 [P] `MFCApplication1/SetupRunner.h` の `CreateSteps()` に `bool registerRepo` 引数を、`RunAll()` に `bool registerRepo` 引数を追加する（[data-model.md](./data-model.md) 3節, R-1603）
- [X] T008 `MFCApplication1/SetupRunner.cpp` の `CreateSteps(bool registerRepo)` で、`claudeMd.displayName` を `registerRepo` に応じて「CLAUDE.md の生成とコミット」（true）／「CLAUDE.md の生成」（false）に出し分ける（T007に依存。FR-1605）
- [X] T009 `MFCApplication1/SetupRunner.cpp` の `RunInitGit()` に `bool registerRepo` 引数を追加し、関数の先頭で `!registerRepo` の場合に `Finish(step, StepResult::Skipped, "「NG」が選択されたため、Gitの初期化を行いませんでした。", sink); return;` する分岐を追加する。既存の本体ロジックは変更しない（T007に依存。FR-1603, R-1601）
- [X] T010 `MFCApplication1/SetupRunner.cpp` の `RunGenerateClaudeMd()` に `bool registerRepo` 引数を追加し、`git add -- CLAUDE.md` / `git commit` の直前で `!registerRepo` の場合は `Finish(step, StepResult::Success, "CLAUDE.md を生成しました。", sink); return;` で終える分岐を追加する。CLAUDE.md生成・存在確認のロジックは変更しない（T007に依存。FR-1604）
- [X] T011 `MFCApplication1/SetupRunner.cpp` の `RunAll(..., bool registerRepo)` に引数を追加し、`RunInitGit`・`RunGenerateClaudeMd` の呼び出しにそのまま渡す（T007, T009, T010に依存。R-1603）

### 結線

- [X] T012 `MFCApplication1/MFCApplication1Dlg.cpp` の `OnBnClickedButtonGenerate()` で、`m_workerPath = m_targetPath;` と同じ箇所に `m_workerRegisterRepo = (IsDlgButtonChecked(IDC_RADIO_REPO_OK) == BST_CHECKED);` を追加し、`SetupRunner::CreateSteps(m_workerRegisterRepo)` を呼ぶ。ワーカースレッド側（`SetupThreadProc`）の `SetupRunner::RunAll(...)` 呼び出しにも `dialog->m_workerRegisterRepo` を渡す（T002, T006, T007, T011に依存。R-1602, R-1603）
- [X] T013 ~~`MFCApplication1/MFCApplication1Dlg.cpp` の `SetControlsEnabled()` の `controls[]` 配列に `GetDlgItem(IDC_RADIO_REPO_OK)`・`GetDlgItem(IDC_RADIO_REPO_NG)` を追加する（T002に依存。FR-1608）~~ **2026-09-17改訂**: 実機確認の結果、ラジオボタンは無効化しない方針に変更（FR-1608改訂）。`controls[]` は既存の生成・参照ボタン2つのみに戻した
- [X] T014 `MFCApplication1.sln` をDebug x64でリビルドし、警告・エラーが0件であることを確認する（T002〜T013完了後）。T013の2026-09-17改訂後も再リビルドし、警告・エラー0件を再確認済み

**チェックポイント**: コントロールの追加とロジックの分岐が完了 - これでUS1・US2の検証を開始できます

---

## Phase 3: User Story 1 - Gitに触れさせずにSpec Kit足場とCLAUDE.mdだけを用意する (Priority: P1) 🎯 MVP

**目標**: 「NG」を選択して生成した場合、Git初期化を一切行わず、CLAUDE.mdは生成されることを実測で確認する

**独立したテスト**: 空の対象フォルダに対し「NG」を選択して生成し、`.git`が作成されていないこと・`CLAUDE.md`は存在すること・結果一覧の表示が[contracts/ui-contract.md](./contracts/ui-contract.md)「4.」の通りであることを確認する

### User Story 1 の検証

- [X] T015 [US1] ST-1601: `.rc`・`Resource.h`・`InitAnchors()`・`SetupRunner`のシグネチャが[data-model.md](./data-model.md)の通りであり、FR番号コメントが付いていることを確認する（[quickstart.md](./quickstart.md) ST-1601）
- [X] T016 [US1] ST-1602: `msbuild ... -p:RunCodeAnalysis=true -p:EnablePREfast=true` を実行し、指摘0件を確認する
- [X] T017 [US1] ST-1603: Debug/Release × x64/x86の4構成をリビルドし、警告・エラー0件を確認する
- [X] T018 [US1] DT-1603: 空のテスト用フォルダBに対し「NG」を選択して生成し、手順1〜4（uv・specify CLI・VS Code拡張・Spec Kit足場）が選択の影響を受けず従来通り実行されること（FR-1607、spec.md US1 AC3）・`.git`が作成されないこと・`CLAUDE.md`は生成されること・結果一覧の「Git 初期化と初回コミット」が「スキップ」・「CLAUDE.md の生成」の行が「成功」（内容「CLAUDE.md を生成しました。」）であることを確認する（[quickstart.md](./quickstart.md) DT-1603）
- [X] T019 [US1] DT-1604（2026-09-17改訂）: 「NG」を選択した状態で生成を実行した直後（処理完了前）に、`IDC_RADIO_REPO_OK`・`IDC_RADIO_REPO_NG`が`IsWindowEnabled() == TRUE`（無効化されていない）であることを確認する（[quickstart.md](./quickstart.md) DT-1604）。ユーザーによる`test016-ng2`実機確認（処理中グレーアウトなし）と、`SetControlsEnabled()`が生成・参照ボタンの2つしか触らないことのコード確認の両方で担保

**チェックポイント**: この時点で、User Story 1 は独立して完全に検証済みであるはずです

---

## Phase 4: User Story 2 - 何も操作しなければこれまで通りGitまで自動化される (Priority: P2)

**目標**: ラジオボタンに触れず（既定「OK」のまま）生成した場合、001〜015の動作が完全に再現されることを実測で確認する

**独立したテスト**: ラジオボタンを操作せず生成し、001〜015の既存の動的テスト手順がすべて変わらず成立することを確認する

### User Story 2 の検証

- [X] T020 [US2] DT-1601: アプリ起動直後、`IDC_RADIO_REPO_OK`の`BM_GETCHECK`が`BST_CHECKED`であることを確認する（[quickstart.md](./quickstart.md) DT-1601, FR-1601）
- [X] T021 [US2] DT-1602: 何も操作せず、空のテスト用フォルダAに対し生成を実行し、「Git 初期化と初回コミット」「CLAUDE.md の生成とコミット」がいずれも「成功」、`.git`と`CLAUDE.md`が存在し`git log`に2件のコミットがあることを確認する（[quickstart.md](./quickstart.md) DT-1602, SC-1601）。ユーザーによる`test016-ok`実機確認（スクリーンショット）と、`git log`（`9a90e26 CLAUDE.md を追加` / `e4e2fe0 Spec Kit 導入`の2件）・`CLAUDE.md`存在のディスク確認の両方で担保

**チェックポイント**: これでUser Story 1・2の両方が独立して検証済みであるはずです

---

## Phase 5: Polish & Cross-Cutting Concerns（仕上げと横断的関心事）

**目的**: US1・US2双方の結果を突き合わせる検証と、ドキュメントの反映

- [X] T022 DT-1605: フォルダA（T021、OK）・フォルダB（T018、NG）それぞれの完了後の列幅（`LVM_GETCOLUMNWIDTH`）を実測し、015の[research.md](../015-support-high-dpi/research.md) R-1503の最小余裕から悪化していないことを確認する（T018, T021に依存。[quickstart.md](./quickstart.md) DT-1605, FR-1609）。ライブ再測定の代わりに、96/115/120/144 DPI全てでTextRenderer.MeasureTextにより新規文字列（「CLAUDE.md の生成」「CLAUDE.md を生成しました。」「「NG」が選択されたため、Gitの初期化を行いませんでした。」）を実測し、いずれも015が確立した各列の既存最長文字列（178/126/552px @115DPI）を下回ることを確認した。016は`kColumnRatio`・ダイアログ寸法を変更していないため、これで列margin悪化なしと判定できる
- [X] T023 DT-1606: 001〜015の既存の動作確認手順（リサイズ追従8方向、タイトルバー色、100%〜150%のDPI列margin・ウィンドウ幅1024px制約）を再実施し、崩れていないことを確認する（T014に依存。[quickstart.md](./quickstart.md) DT-1606）。実機で以下を確認: タイトルバー色は伸縮前後とも`#1F3864`(RGB 31,56,100)、ウィンドウ幅は115 DPIで1016px（1024px制約維持）、参照/OK/キャンセルボタンと内容列がリサイズに追従、新規3コントロール（ラベル・OK・NGラジオ）は`Fixed`の通り位置を維持（座標不変）
- [X] T024 [P] `CLAUDE.md` の既存機能一覧表に `specs/016-repo-registration-toggle/` の行を追加し、リポジトリ登録トグルの追加、「NG」選択時の手順5・6の挙動変化、既存の依存関係チェック機構（`StepResult::Skipped`）をそのまま再利用できた設計判断を追記する

---

## Dependencies & Execution Order（依存関係と実行順序）

### Phase Dependencies（フェーズの依存関係）

- **Setup (Phase 1)**: 依存関係なし
- **Foundational (Phase 2)**: Setup の完了に依存 - US1・US2の両方をブロックする（コード変更自体が両ストーリー共通のため）
- **User Story 1 (Phase 3)**: Foundational の完了に依存。US2への依存なし
- **User Story 2 (Phase 4)**: Foundational の完了に依存。US1への依存なし（並行実施可能）
- **Polish (Phase 5)**: US1・US2の両方の完了に依存（T022はフォルダA・Bの両方の実行結果を使うため）

### Within Each Phase（各フェーズ内）

- T002→T003→T004→T005→T006 はUIコントロール系（`.rc`・`Resource.h`・`Dlg.h/.cpp`）で、T003以降はT002が定義するIDCシンボルに依存するため逐次実行
- T007（`SetupRunner.h`）はUIコントロール系（T002〜T006）と異なるファイル・異なる関心事のため並行実行可能。T008〜T011はT007のシグネチャ変更に依存し、かつ同一ファイル（`SetupRunner.cpp`）のため逐次実行
- T012（結線）はUIコントロール系（T002, T006）と`SetupRunner`系（T007, T011）の両方の完了に依存する合流点
- T013はT002（IDCシンボル）に依存
- T014（リビルド確認）はT002〜T013すべてに依存

### Parallel Opportunities（並行実行の機会）

- Foundational内: T007（`SetupRunner.h`のシグネチャ変更）はT002〜T006（UIコントロール系）と並行して進められる
- Foundational完了後、User Story 1（Phase 3）とUser Story 2（Phase 4）は並行して進められる（異なるテスト用フォルダを使うため干渉しない）
- T024（CLAUDE.md更新）はT022・T023の完了を待たずに進められる

---

## Implementation Strategy（実装戦略）

### MVP First（User Story 1 のみ）

1. Phase 1: Setup を完了する
2. Phase 2: Foundational（コントロール追加とロジック分岐）を完了する — これが実質的な全コード変更
3. Phase 3: User Story 1（NG経路）の検証を完了する
4. **停止して検証**: 「NG」選択でGitに触れずCLAUDE.mdだけが生成されることを確認する

### Incremental Delivery（段階的な提供）

1. Setup + Foundational を完了する → コントロールとロジックの追加が完了
2. User Story 1 を検証する → 「NG」経路が保証される（MVP）
3. User Story 2 を検証する → 「OK」経路（既存動作）の回帰が無いことが保証される
4. Polish（列幅の突き合わせ、既存機能の再回帰確認、CLAUDE.md更新）で完了

---

## Notes（注記）

- 本フィーチャーのコード変更はFoundationalフェーズに集約されている。US1・US2はいずれも「OKかNGか」という同一条件分岐の異なる枝を検証するものであり、コードを分割して段階的に実装することはできない
- 各タスク完了後、`git add`は変更した実ファイルのみを明示指定すること（`git add -A`を使わない）
