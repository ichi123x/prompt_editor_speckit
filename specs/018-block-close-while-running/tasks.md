---

description: "導入処理の実行中はダイアログを閉じられないようにする（018）の実装タスク"
---

# Tasks: 導入処理の実行中はダイアログを閉じられないようにする

**Input**: `/specs/018-block-close-while-running/` からの設計ドキュメント

**Prerequisites（前提条件）**: [plan.md](./plan.md)、[spec.md](./spec.md)、[research.md](./research.md)、[data-model.md](./data-model.md)、[contracts/ui-contract.md](./contracts/ui-contract.md)、[quickstart.md](./quickstart.md)

**Tests（テスト）**: 本リポジトリに自動テストスイートは存在しない（憲章の技術的制約）。代わりに静的テスト ST-1801〜1806、動的テスト DT-1801（実機、利用者による実行が**必須**）・DT-1802（完全自動）を [quickstart.md](./quickstart.md) の手順を実施するタスクとして定義する。

**Organization（構成）**: タスクはユーザーストーリーごとにグループ化する。US1（実行中は閉じない）と US2（待機中はこれまでどおり閉じる）は同じ `OnOK` / `OnCancel` の実装を共有するが、`m_running` の真偽それぞれの観点として検証タスクを分ける。

## Format（形式）: `[ID] [P?] [Story] Description`

- **[P]**: 並行実行可能（異なるファイル、依存関係なし）
- **[Story]**: US1 = 実行中に誤って閉じても処理が守られる（P1）、US2 = 処理が終わればこれまでどおり閉じられる（P2）

---

## Phase 1: Setup（共有インフラ）

**目的**: 実装前の状態を記録する

- [X] T001 アプリを終了させ（`Get-Process MFCApplication1 -ErrorAction SilentlyContinue | Stop-Process -Force`）、[quickstart.md](./quickstart.md) の ST-1803・ST-1804・ST-1806 を実装前の状態で実行し、いずれも不合格になることを記録する（憲章 原則V「検査そのものが正しく動くことを確かめる」）。期待値: ST-1803 = `T_CHECK_BUG_01` が3件、ST-1804 = `new SetupStep` 1件・`delete step` 1件、ST-1806 = `4つの原則` 1件。あわせて `MFCApplication1/MFCApplication1Dlg.cpp`・`MFCApplication1/MFCApplication1Dlg.h` に `FR-1801`〜`FR-1807` がいずれも0件であることを控える（ST-1802 のベースライン） — **結果: 合格**（ST-1803=3件、ST-1804=new 1件/delete 1件、ST-1806=1件、FR-1801〜1807はいずれも0件。期待どおり不合格）

---

## Phase 2: Foundational（ブロッキングとなる前提条件）

**目的**: 両ユーザーストーリーが依存する共有の基盤コードを用意する

本フィーチャーには該当タスクがない。US1・US2 はいずれも `OnOK` / `OnCancel` の同じ実装に対する異なるテスト観点であり、別途の基盤整備は不要（[plan.md](./plan.md) Project Structure）。Setup の後、直接 User Story 1 の実装へ進める。

---

## Phase 3: User Story 1 - 実行中に誤って閉じても処理が守られる (Priority: P1) 🎯 MVP

**目標**: 導入処理の実行中は、どの閉じる操作でもダイアログを閉じず、案内を表示して処理を最後まで続ける。あわせて、手順の結果を受け渡す箇所を憲章 O-1・O-3 に従わせる

**独立したテスト**: `C:\work\test\test01` を対象に「生成」を押し、完了前に「キャンセル」「OK」✖・Esc・Alt+F4・Enter を試して、案内が出てダイアログが閉じず、手順が最後まで進むことを確認する（[quickstart.md](./quickstart.md) DT-1801）

### User Story 1 の実装

- [X] T002 [P] [US1] `MFCApplication1/MFCApplication1Dlg.h` から、`//不具合を確認するための定義` のコメント行と `#define T_CHECK_BUG_01 ...` の行（現在の11行目付近）を削除する。`virtual void OnOK() override;` / `virtual void OnCancel() override;` の宣言（現在の67〜70行目付近）を囲む `#ifdef T_CHECK_BUG_01` と `#endif` を削除し、宣言は残す。各宣言の行末コメント（「OK ボタン押下時の動作を確認するためオーバーライド」等）を、`// 018 FR-1801: 実行中は閉じない` のような FR 番号付きの日本語コメントに置き換える。`FR-1805`（実験用の目印を残さず常に有効）にもこのどちらかのコメントで触れる。**`T_CHECK_BUG_01` という文字列はコメントにも残さない**（ST-1803 がコメントも含めて検査するため。[research.md](./research.md) R-1805） — **結果: 実装済み**（`#define`・`#ifdef`/`#endif` を削除し、宣言に FR-1801・FR-1805 のコメントを付与）
- [X] T003 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` の `OnOK()` / `OnCancel()` の定義（現在の424〜447行目付近）を囲む `#ifdef T_CHECK_BUG_01` と `#endif`、および `//OK, CANCEL, ✖時の動作チェック` のコメントを削除する。関数の本体（`m_running` が `true` なら `AfxMessageBox(_T("導入処理の実行中は閉じられません。完了までお待ちください。"), MB_OK | MB_ICONINFORMATION)` を表示して `return`、そうでなければ `CDialogEx::OnOK()` / `CDialogEx::OnCancel()` を呼ぶ）は変更しない。2関数の直前に、閉じる操作がこの2関数に集まること（Enter・Esc・✖・Alt+F4 を含む）と、完了通知の受信で `m_running` が `false` に戻るまでワーカーがダイアログを使っていること（憲章 T-2）を説明する日本語コメントを付け、`FR-1801`・`FR-1802`・`FR-1803`・`FR-1804`・`FR-1805` を記載する。案内表示の3行の重複は共通化しない（R-1802） — **結果: 実装済み**（`#ifdef`/`#endif` と実験用コメントを削除。本体は無変更。直前に FR-1801〜1805 と T-2 の説明コメントを付与）
- [X] T004 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` の手順の結果の受け渡しを憲章 O-3 の方式に書き換える（[research.md](./research.md) R-1804、[data-model.md](./data-model.md) 2）。(1) include 群（現在の18行目 `#include <dwmapi.h>` 付近、`#define new DEBUG_NEW` より前）に `#include <memory>` を追加する。(2) `OnStepFinished`（現在の599〜602行目付近）を、`auto copy = std::make_unique<SetupStep>(step);` で作り、`PostMessage(WM_APP_STEP_FINISHED, 0, reinterpret_cast<LPARAM>(copy.get()))` が成功（非0）したときだけ `copy.release();` する形にする。`release()` の箇所に「所有権は OnStepFinishedMessage へ移った（憲章 O-1 の例外・O-3）」旨のコメントを付ける。失敗時は何もせず自動解放に任せる（FR-1806）。(3) `OnStepFinishedMessage`（現在の604〜615行目付近）の先頭で `std::unique_ptr<SetupStep> step(reinterpret_cast<SetupStep*>(lParam));` と受け取り直し、`delete step;` を削除する（FR-1807）。`step->` による参照箇所はそのまま使える。コメントに `FR-1806`・`FR-1807` を記載する。**`new` を使わない旨の警告コメントは書かない**（ST-1804 はコメント行を除外して検査するが、行末コメントは除外されないため）。T003 と同じファイルのため T003 の後に行う — **結果: 実装済み**（`#include <memory>` を追加。`OnStepFinished` を make_unique + 送信成功時のみ release に、`OnStepFinishedMessage` を先頭で unique_ptr に受け直す形に変更）

### User Story 1 の検証

- [X] T005 [US1] [quickstart.md](./quickstart.md) 手順1の**ビルド**（ST-1801）を実施する。Debug/Release × x64/x86 の4構成すべてで警告=0 エラー=0。T002・T003・T004 に依存 — **結果: 合格**（4構成すべて警告0・エラー0）
- [X] T006 [P] [US1] [quickstart.md](./quickstart.md) の **ST-1802**（`FR-1801`〜`FR-1807` がいずれも1件以上出現、コメント行を除外しない）を実施する。T005 に依存 — **結果: 合格**（FR-1801=2, FR-1802=1, FR-1803=1, FR-1804=1, FR-1805=2, FR-1806=2, FR-1807=2）
- [X] T007 [P] [US1] [quickstart.md](./quickstart.md) の **ST-1803**（`MFCApplication1\*.cpp`・`*.h` に `T_CHECK_BUG_01` が0件、コメントも含める）を実施する。T005 に依存 — **結果: 合格**（0件）
- [X] T008 [P] [US1] [quickstart.md](./quickstart.md) の **ST-1804**（`new SetupStep`・`delete step` が0件、`make_unique<SetupStep>`・`.release()`・`unique_ptr<SetupStep>` が各1件以上、`#include <memory>` あり）を実施し、続けて**目視レビュー**の3点（`release()` が PostMessage 成功時の分岐内にあること、所有者のコメントがあること、受信側が先頭で `unique_ptr` に受け取り直していること）を確認する。T005 に依存 — **結果: 合格**（new SetupStep=0, delete step=0, make_unique=1, release()=1, unique_ptr受取=1, #include <memory> は22行目。目視レビュー3点も合格）
- [X] T009 [P] [US1] [quickstart.md](./quickstart.md) の **ST-1805**（案内の文言が `MFCApplication1Dlg.cpp` に2件）を実施する。T005 に依存 — **結果: 合格**（2件）
- [ ] T010 [US1] [quickstart.md](./quickstart.md) 手順4の **DT-1801**（実行中に「キャンセル」「OK」✖・Esc・Alt+F4・Enter の6操作を試し、それぞれ案内が表示されること・ダイアログが閉じないこと・手順の追加が続くことを確認し、完了後に ✖ で閉じられることを確認する）を**利用者が実機で実施する**。フォルダ選択ダイアログの自動操作ができないため、エージェントは手順を案内して結果の報告を受ける。本フィーチャーの根本価値（FR-1801〜1803、SC-1801）を確かめる唯一の手段のため**必須**。T005 に依存

**チェックポイント**: この時点で、User Story 1 は独立して完全に機能し、テスト可能であるはずです

---

## Phase 4: User Story 2 - 処理が終わればこれまでどおり閉じられる (Priority: P2)

**目標**: 閉じる操作を止めるのは実行中に限り、待機中（起動直後・完了後・検証による中止後）はこれまでどおり1回の操作で閉じられることを保証する

**独立したテスト**: フォルダを選ばずに起動し、「OK」「キャンセル」✖ に相当するメッセージを送ってプロセスが終了することを確認する（[quickstart.md](./quickstart.md) DT-1802）

### User Story 2 の実装

本ストーリーが必要とする「待機中は閉じる」という性質（`m_running` が `false` なら基底クラスの `OnOK` / `OnCancel` を呼ぶ）は、T003 の実装に最初から含まれている。したがって本ストーリー専用の追加実装タスクはない。以下は検証タスクのみ。

### User Story 2 の検証

- [X] T011 [US2] [quickstart.md](./quickstart.md) 手順3の **DT-1802** を自動で実施する。ケース1〜3は起動直後のアプリへ `WM_COMMAND(IDOK)`・`WM_COMMAND(IDCANCEL)`・`WM_SYSCOMMAND(SC_CLOSE)` を1ケースずつ送る（User Story 2 シナリオ1）。ケース4はフォルダ未選択のまま「生成」（`IDC_BUTTON_GENERATE` = 1004）を送って検証で中止させ、表示された案内を `WM_CLOSE` で閉じてから `WM_COMMAND(IDCANCEL)` を送る（シナリオ3。案内は `WM_COMMAND(IDOK)` では閉じないことを実測済み）。4ケースすべてで3秒以内にプロセスが終了し、ケース4で案内ボックスが1つ検出されること（FR-1804、SC-1802）。完了後に閉じられること（シナリオ2）は T010 の手順5で確認する。T005 に依存 — **結果: 合格**（4ケースとも閉じた=True。ケース4は案内ボックス数=1、閉じた後の残り=0）

**チェックポイント**: ここまでで US1・US2 の両方が独立して機能する。実行中は閉じず、待機中は閉じることが実測で確認できる

---

## Phase 5: Polish & Cross-Cutting Concerns（仕上げと横断的関心事）

- [X] T012 [P] [quickstart.md](./quickstart.md) 付録の**コード分析**（`RunCodeAnalysis=true` / `EnablePREfast=true`）を実施し、指摘が0件であること（憲章の技術的制約）。T005 に依存 — **結果: 合格**（指摘0件）
- [ ] T013 `CLAUDE.md` を3か所更新する（[research.md](./research.md) R-1805）。(1)「既存機能」の表の末尾に `specs/018-block-close-while-running/` の行を追加する（内容: 導入処理の実行中は OK・キャンセル・✖ 等でダイアログを閉じられないようにした。コミット `1d49765` の先行修正を憲章 T-2 に基づき正式化。あわせて手順の結果の受け渡しを憲章 O-3 の `unique_ptr` + `release()` 方式に変更）。(2)「憲章」節の「（v1.0.0）」を「（v1.5.0）」にし、「4つの原則: …」の行を、原則 I〜V と「所有権・スレッドモデル」節（O-1〜O-3、T-1〜T-4）がある旨の記述に置き換える。(3)「設計上、壊しやすい箇所」節に1項目を追加する（`OnOK` / `OnCancel` は `m_running` が `true` の間は閉じない。Enter・Esc・✖・Alt+F4 もすべてこの2関数を通る。この判定を外すと、ワーカーが通知している最中にダイアログが破棄される T-2 違反に戻る。実行中も「OK」「キャンセル」ボタンは無効化しない設計であり、「処理中は2コントロールを無効化する」の項目と矛盾しない）。**T010（DT-1801）が不合格の場合はこのタスクを実施しない**（効果が確認できていない知見を書き残さない）。T010・T011 に依存
- [ ] T014 [quickstart.md](./quickstart.md) の **ST-1806**（CLAUDE.md に `018-block-close-while-running` と `v1.5.0` があり、`4つの原則` と `v1.0.0` が無い）を実施する。T013 に依存
- [ ] T015 `specs/018-block-close-while-running/spec.md` 冒頭の `**Status**` を `Draft` から `Implemented` に更新する（憲章「開発ワークフロー」）。必須タスク（T001〜T014、すべて必須）が `[X]` になっていることを確認してから行う。T014 に依存

---

## Dependencies & Execution Order（依存関係と実行順序）

### Phase Dependencies（フェーズの依存関係）

- **Setup (Phase 1)**: 依存なし。**T001 は実装を始める前に必ず実施する**（実装後では「検査が不合格になること」を確かめられない）
- **Foundational (Phase 2)**: タスクなし（該当なし）
- **User Story 1 (Phase 3)**: T001 の後
- **User Story 2 (Phase 4)**: US1 の実装（T003）とビルド（T005）に依存する。US1 の検証タスク（T006〜T010）の完了は待たずに並行して進められる
- **Polish (Phase 5)**: US1・US2 の両方の完了後（T012 のみ T005 の後いつでもよい）

### User Story Dependencies（ユーザーストーリーの依存関係）

- **User Story 1 (P1)**: Setup の後に開始可能。他のストーリーへの依存なし
- **User Story 2 (P2)**: US1 の実装タスク（T003）の完了に依存する。検証タスク（T011）は T005 完了後、US1 の検証（T006〜T010）と並行して進めてよい

### 重要な順序

| タスク | 依存 | 理由 |
|---|---|---|
| T001 | — | 実装前の状態でしか「検査が不合格になる」ことを確かめられない |
| T003→T004（`MFCApplication1Dlg.cpp` の変更順） | — | 同じファイルのため逐次に行う |
| T005（ビルド） | T002・T003・T004 | `.h` の宣言と `.cpp` の定義が揃ってからでないとビルドが通らない |
| T010（DT-1801、利用者） | T005 | ビルド済みの exe を利用者が操作する |
| T013（CLAUDE.md 更新） | T010・T011 の合格 | 効果が確認できていない知見を書き残さない |
| T015（Status 更新） | T014 | 必須タスクがすべて `[X]` になってから |

### Parallel Opportunities（並行実行の機会）

- T002（`.h`）と T003（`.cpp`）は異なるファイルのため並行して編集できる。T004 は T003 と同じファイルのため T003 の後
- T006・T007・T008・T009（ST-1802〜1805）は互いに独立した読み取り専用の検査であり並行実行可能
- T011（DT-1802、自動）と T012（コード分析）は、T010（利用者による実機確認）を待つ間に並行して進めてよい。ただし T011 と T012 はどちらもビルド成果物を使うため、T012 の Rebuild 中に T011 を走らせない

### Parallel Example: User Story 1

```text
# T005 のビルド合格後、静的テストをまとめて実行する:
Task: "T006 ST-1802 FR 追跡可能性"
Task: "T007 ST-1803 T_CHECK_BUG_01 が残っていないこと"
Task: "T008 ST-1804 O-1・O-3 の検査と目視レビュー"
Task: "T009 ST-1805 案内の文言"
```

---

## Implementation Strategy（実装戦略）

### MVP First（User Story 1 のみ）

1. Phase 1: T001（ベースライン取得）
2. Phase 3: T002〜T010（目印の除去、O-3 化、検証、利用者による実機確認）
3. **停止して検証**: 実行中に閉じられないことを利用者が確認する

US1 だけで本フィーチャーの目的（実行中に閉じられる不具合の解消と、先行修正の正式化）は達成される。ただし US2 の検証（待機中は閉じられること）まで行うことで、止める条件を誤って「アプリを終了できなくなる」回帰が起きていないことを裏付けられる。

### Incremental Delivery（段階的な提供）

1. Setup → ベースライン取得
2. User Story 1 → 実装と検証（**MVP**）
3. User Story 2 → 待機中に閉じられることの検証（実装は US1 に内包済み）
4. Phase 5 → コード分析、CLAUDE.md の更新、Status の更新

### 実装順序に関する注意

- **T002・T003 で `T_CHECK_BUG_01` の文字列をコメントにも残さない。** 「元は T_CHECK_BUG_01 だった」のような経緯のコメントを書くと ST-1803 が必ず不合格になる（CLAUDE.md の「検査がコメントを拾う」問題）。経緯は spec.md の背景に書いてある
- **T004 で `release()` を `PostMessage` の前に呼ばない。** 先に `release()` すると送信失敗時に誰も解放しなくなり、修正前と同じリークが残る（FR-1806 に反する）
- **T003 で `OnOK` / `OnCancel` の本体を変えない。** 利用者が実験で動作を確認済みのコードであり、本フィーチャーで変えるのは囲みとコメントだけである
- **T010 は利用者の操作が必要。** エージェントは T005 のビルド後に、quickstart.md 手順4を利用者へ案内し、結果の報告を待つ

---

## Notes（注記）

- [P] タスク = 異なるファイル、依存関係なし
- [Story] ラベルは、トレーサビリティのためにタスクを特定のユーザーストーリーに対応付ける
- 検証タスク（T005〜T012、T014）はコードを書くタスクではなく、[quickstart.md](./quickstart.md) の手順を実施して結果を確認するタスクである
- DT-1802 の PowerShell スクリプトはスクラッチパッドで実行し、**リポジトリには追加しない**（憲章の技術的制約: `.ps1` を新規に追加しない）
- T001〜T015 はすべて必須タスクである（「任意」と明記したタスクは無い）。T010 は利用者による実機実行だが、017 の DT-1702 と異なり任意ではない
