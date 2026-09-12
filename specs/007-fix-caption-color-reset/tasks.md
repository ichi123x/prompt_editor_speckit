---

description: "フォルダ選択ダイアログ後もタイトルバーの色を保つ（007）の実装タスク"
---

# Tasks: フォルダ選択ダイアログ後もタイトルバーの色を保つ

**Input**: `/specs/007-fix-caption-color-reset/` からの設計ドキュメント

**Prerequisites（前提条件）**: [plan.md](./plan.md)、[spec.md](./spec.md)、[research.md](./research.md)、[data-model.md](./data-model.md)、[contracts/ui-contract.md](./contracts/ui-contract.md)、[quickstart.md](./quickstart.md)

**Tests（テスト）**: 本リポジトリに自動テストスイートは存在しない（憲章の技術的制約）。代わりに静的テスト ST-701〜703 を [quickstart.md](./quickstart.md) の手順を実施するタスクとして定義する。動的テスト DT-701 はフォルダ選択ダイアログの開閉を伴うため自動化できず、**利用者による手動実行タスク**として定義する。

**Organization（構成）**: タスクはユーザーストーリーごとにグループ化する。本フィーチャーは spec.md に User Story 1 のみが定義されている。

## Format（形式）: `[ID] [P?] [Story] Description`

- **[P]**: 並行実行可能（異なるファイル、依存関係なし）
- **[Story]**: US1 = フォルダを選んでもツールのウィンドウを見失わない（P1）

---

## Phase 1: Setup（共有インフラ）

**目的**: 実装前の状態を記録し、既存の着色処理（005）が無傷であることを後で突き合わせられるようにする

- [X] T001 アプリを終了させ（`Get-Process MFCApplication1 | Stop-Process -Force`、起動中だとビルドが `LNK1104` で失敗する）、[quickstart.md](./quickstart.md) 手順3の検索コマンドで実装前の `DwmSetWindowAttribute` の出現箇所を控えておく。実装前は `OnInitDialog` 内に2回のみ現れるはずであり、**この値が実装後に「`ApplyCaptionColor()` の定義内に2回、それ以外の場所には0回」に変わることが 005 の処理を壊さず移設できたことの根拠になる**

---

## Phase 2: Foundational（ブロッキングとなる前提条件）

**目的**: `ApplyCaptionColor()` を呼べる状態にする。ここが済むまで US1 の実装に入れない

- [X] T002 `MFCApplication1/MFCApplication1Dlg.h` に非公開メンバ関数 `void ApplyCaptionColor();` の宣言を1行追加する（[data-model.md](./data-model.md) 3章）。メンバ変数は追加しない
- [X] T003 `MFCApplication1/MFCApplication1Dlg.cpp` に `void CMFCApplication1Dlg::ApplyCaptionColor()` を定義する。**中身は `OnInitDialog` に既存の2行（`DwmSetWindowAttribute` への `DWMWA_CAPTION_COLOR` と `DWMWA_TEXT_COLOR` の呼び出し）をそのまま移すだけ**とし、色の定数・並び・戻り値を判定しない方針（005 FR-408）を一切変えない。T002 に依存

**チェックポイント**: `ApplyCaptionColor()` を呼ぶ準備が整った。この時点ではまだ `OnInitDialog` から呼ばれておらず、着色処理が動いていない状態

---

## Phase 3: User Story 1 - フォルダを選んでもツールのウィンドウを見失わない (Priority: P1) 🎯 MVP

**目標**: 「参照...」でフォルダ選択ダイアログを開いて閉じても、メインダイアログのタイトルバーが濃紺のままであるようにする

**独立したテスト**: アプリを起動し、「参照...」でフォルダ選択ダイアログを開いて（フォルダを選択する場合・キャンセルする場合の両方で）閉じ、閉じた直後にタイトルバーが濃紺のままであることを確認する

### User Story 1 の実装

- [X] T004 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` の `OnInitDialog()` にあった着色の2行を削除し、代わりに `ApplyCaptionColor();` の呼び出し1行に置き換える（FR-701 の前提となる既存動作の保持）。005 のコメント（着色対象・タイミング・再設定不要の理由）はそのまま残す。T003 に依存
- [X] T005 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` の `OnBnClickedButtonBrowse()` を次のように変更する: (a) `CFolderPickerDialog dialog(...)` の構築直後、`DoModal()` を呼ぶ**直前**に `ApplyCaptionColor();` を1回呼ぶ（FR-701 — ダイアログを開いた瞬間に色が崩れる可能性に備える。spec.md User Story 1 Acceptance Scenario 1）。(b) `dialog.DoModal()` の結果を一度変数に受け、**`!= IDOK` による早期 return の前**に `ApplyCaptionColor();` をもう1回呼ぶ（FR-701, FR-702）。**(a)(b) いずれも戻り値（選択／キャンセル）を問わず必ず呼ぶこと** — (b) がキャンセル時にも呼ばれないと FR-701 のシナリオ3を満たさない。既存の早期 return のコメントや、選択後の `m_targetPath` 代入・`SetDlgItemText` 呼び出しは変更しない（FR-704）。T003 に依存
- [X] T006 [US1] T004・T005 の追加箇所に FR 番号のコメントを付す（FR-706）。`ApplyCaptionColor()` の定義には「フォルダ選択ダイアログの開閉でも再適用するため関数化した」という趣旨のコメントを、`OnBnClickedButtonBrowse` の2箇所の呼び出し箇所にはそれぞれ FR-701（開く直前）・FR-701/FR-702（閉じた直後）を記載する。あわせて「他の契機（`OnActivate` 等）では呼ばない」という FR-704・FR-705 の趣旨を1箇所にまとめて記載する。T005 に依存

### User Story 1 の検証

- [X] T007 [US1] [quickstart.md](./quickstart.md) 手順1の**ビルド**を実施する。Debug/Release × x64/x86 の8行すべてで警告=0 エラー=0。T006 に依存 — **結果: 合格**（8行すべて警告=0 エラー=0）
- [X] T008 [US1] [quickstart.md](./quickstart.md) 手順2の **ST-701**（`ApplyCaptionColor` の定義1件・呼び出し3件、呼び出し元が `OnInitDialog`(1件)と `OnBnClickedButtonBrowse`(2件)のちょうど3件）を実施する。T007 に依存 — **結果: 合格**
- [X] T009 [US1] [quickstart.md](./quickstart.md) 手順2の **ST-702**（`DoModal()` の直前・直後の両方に `ApplyCaptionColor()` があり、直後の呼び出しが早期 return の順になっている、かつ戻り値を判定していない）を実施する。**直後の呼び出し順序が逆だとキャンセル時に再適用されない** — 本フィーチャー最大の要所。この確認は FR-703 の検証も兼ねる。T007 に依存 — **結果: 合格**（`DoModal()` の結果を `result` 変数に受ける実装に合わせ、quickstart.md の検査正規表現を調整のうえ確認）
- [X] T010 [US1] [quickstart.md](./quickstart.md) 手順2の **ST-703**（FR による追跡可能性）を実施する。FR-701, FR-702, FR-704 が実装コメントとして必ず現れること。T007 に依存 — **結果: 合格**（FR-701〜706 すべてコメントとして出現）
- [X] T011 [US1] [quickstart.md](./quickstart.md) 手順3の**既存機能の回帰確認**を実施する。`DwmSetWindowAttribute` が `ApplyCaptionColor()` の定義内にのみ2回現れ、それ以外の場所（`OnInitDialog` 直下や `OnBnClickedButtonBrowse` 直下）には現れないこと。T001 で控えた値と比較する。T007 に依存 — **結果: 合格**（004のキーワード8件も実測値と完全一致）
- [X] T012 [US1] [quickstart.md](./quickstart.md) 付録の**コード分析**（`RunCodeAnalysis=true` / `EnablePREfast=true`）を実施し、指摘が0件であること（SC-703）。T007 に依存 — **結果: 合格**（出力0件）

**チェックポイント**: ここまでで静的な検証はすべて完了。あとは実機での動的確認のみが残る

---

## Phase 4: Polish & Cross-Cutting Concerns（仕上げと横断的関心事）

- [X] T013 [US1] [quickstart.md](./quickstart.md) 手順4の **DT-701（フォルダ選択ダイアログの開閉、利用者による手動実行が必須）** を実施する。「参照...」でフォルダ選択ダイアログを**開いている最中**にタイトルバーが濃紺のままであることを確認し（Acceptance Scenario 1）、続けて選択・キャンセルの両方で閉じる操作を**10回以上**繰り返し（SC-701）、毎回タイトルバーが濃紺のままであることを確認する。**この確認は自動化できないため、利用者が実機で行う**（[research.md](./research.md) R-701, R-704）。T008〜T012 に依存 — **結果: 合格**（利用者がフォルダ選択ダイアログを開いた状態のスクリーンショットで確認。開いている最中もタイトルバーは濃紺のまま）
- [X] T014 [P] T013 が合格したら、`CLAUDE.md` の「設計上、壊しやすい箇所」節に本フィーチャーで得た知見を追記する — タイトルバーの着色処理（005）は `OnInitDialog` だけでなく `CFolderPickerDialog::DoModal()` の直前・直後にも再適用する必要があったこと、根本原因は自動操作では特定できず対症療法で対応したこと。**T013 が不合格の場合はこのタスクを実施しない**（原因が別にあることが判明した時点で、判明した原因に基づいて記載し直す）。T013 に依存 — **結果: 済**（既存の該当節を書き換え、既存機能一覧の表にも007を追加）

---

## Dependencies & Execution Order（依存関係と実行順序）

### Phase Dependencies（フェーズの依存関係）

- **Setup (Phase 1)**: 依存なし。**T001 は実装を始める前に必ず実施する**
- **Foundational (Phase 2)**: T001 の後。US1 をブロックする
- **User Story 1 (Phase 3)**: Foundational の後
- **Polish (Phase 4)**: US1 の完了後。**T013（実機での手動確認）が本フィーチャーの真の合否判定**であり、T007〜T012 の静的な合格だけでは完了とみなさない

### 重要な順序

| タスク | 依存 | 理由 |
|---|---|---|
| T001（ベースライン取得） | — | 実装前でなければ T011 の突き合わせが意味を失う |
| T005（`DoModal()` 直前・直後の2箇所に再適用を置く） | T004 | 直前を忘れると開いた瞬間に効かず、直後を早期 return の後に置くとキャンセル時に効かない |
| T013（実機での手動確認） | T008〜T012 | 静的な検証がすべて揃ってから実機確認に進む |
| T014（CLAUDE.md 更新） | T013 の合格 | 効果が確認できていない知見を書き残さない |

### Parallel Opportunities（並行実行の機会）

- **Phase 3**: T008〜T010（静的テスト）は互いに独立して並行実行可能
- **Phase 4**: T014 は T013 の合否を待つため、実質的に並行実行の余地はない

---

## Implementation Strategy（実装戦略）

### MVP First（User Story 1 のみ）

1. Phase 1: T001（実装前のベースライン取得）
2. Phase 2: T002〜T003（`ApplyCaptionColor()` の追加）
3. Phase 3: T004〜T012（呼び出し箇所の変更と静的検証）
4. **停止して検証**: T013（実機での手動確認）を必ず利用者に依頼すること

### 実装順序に関する注意

- **T005 で2箇所とも忘れない。** `DoModal()` を呼ぶ直前の1回を忘れると、ダイアログを開いた瞬間に色が崩れるケース（Acceptance Scenario 1）に対応できない。`dialog.DoModal()` の戻り値を一度変数で受けてから判定する形に変えないと、返った直後の呼び出しを早期 return の前に置きにくい
- **T004 で005 のコメントを消さない。** 「なぜ `OnSize` などで再設定しないか」の説明は本フィーチャーでも変わらず有効
- **T013 の結果を待たずに T014 を書かない。** 効果不明の知見を CLAUDE.md に残すと、後で誤った前提として参照される

### Incremental Delivery（段階的な提供）

1. Setup + Foundational → `ApplyCaptionColor()` を呼べる状態
2. User Story 1 → フォルダ選択ダイアログの開閉後もタイトルバーが濃紺のまま（**MVP**、ただし実機確認が済むまでは未完了扱い）
3. Phase 4 → 実機確認、知見の記録

---

## Notes（注記）

- [P] タスク = 異なるファイル、依存関係なし
- [Story] ラベルは、トレーサビリティのためにタスクを特定のユーザーストーリーに対応付ける
- 検証タスク（T007〜T013）はコードを書くタスクではなく、[quickstart.md](./quickstart.md) の手順を実施して結果を確認するタスクである
- 検証用の `.ps1` はスクラッチパッドに置き、**リポジトリには追加しない**（憲章の技術的制約）
- T013 は利用者による実機操作が必須。Claude Code 単独では完了できない
