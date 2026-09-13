---

description: "完了案内にCLAUDE.mdのコミット手順を追加する（012）の実装タスク"
---

# Tasks: 完了案内にCLAUDE.mdのコミット手順を追加する

**Input**: `/specs/012-guide-claude-md-commit/` からの設計ドキュメント

**Prerequisites（前提条件）**: [plan.md](./plan.md)、[spec.md](./spec.md)、[research.md](./research.md)、[data-model.md](./data-model.md)、[quickstart.md](./quickstart.md)

**Tests（テスト）**: 本リポジトリに自動テストスイートは存在しない（憲章の技術的制約）。代わりに静的テスト ST-1201〜1202 を [quickstart.md](./quickstart.md) の手順を実施するタスクとして定義する。動的テスト DT-1201 はフォルダ選択ダイアログを伴うため008以降と同じ理由で自動化できず、利用者による任意の手動実行とする。

**`/speckit-analyze`によるスコープ訂正**: 当初は`MFCApplication1Dlg.cpp`の文字列変更のみを計画していたが、006 FR-507aとの矛盾（案内欄に増やす行数を表示する余裕が無い）が判明したため、`MFCApplication1.rc`の寸法変更（T003、検証はT006）を追加した。

**Organization（構成）**: spec.mdにはUser Story 1のみが定義されている。

## Format（形式）: `[ID] [P?] [Story] Description`

- **[P]**: 並行実行可能（異なるファイル、依存関係なし）
- **[Story]**: US1 = 完了案内から生成直後のCLAUDE.mdをコミットする手順が分かる（P1）

---

## Phase 1: Setup（共有インフラ）

**目的**: 実装前の状態を記録する

- [X] T001 アプリを終了させ（`Get-Process MFCApplication1 -ErrorAction SilentlyContinue | Stop-Process -Force`）、実装前の `MFCApplication1/MFCApplication1Dlg.cpp` の `ShowCompletionGuide()` の案内文（4手順、CLAUDE.mdのコミット手順が無いこと）を控えておく（[quickstart.md](./quickstart.md) ST-1201 のベースライン）— **結果: 記録済み**（4手順、コミット手順なし）

---

## Phase 2: Foundational（ブロッキングとなる前提条件）

**目的**: User Story 1 が依存する共有の基盤コードを用意する

本フィーチャーには該当タスクがない。変更は `MFCApplication1Dlg.cpp` の `ShowCompletionGuide()` 関数1つに閉じており、共有の新しい型・関数を必要としない（[plan.md](./plan.md) Project Structure）。Setup の後、直接 User Story 1 の実装へ進める。

---

## Phase 3: User Story 1 - 完了案内から生成直後のCLAUDE.mdをコミットする手順が分かる (Priority: P1) 🎯 MVP

**目標**: `ShowCompletionGuide()` の成功時の案内に、`/init`実行の直後にCLAUDE.mdをコミットする手順を追加する

**独立したテスト**: SDD化を成功させ、表示される完了案内の文言を確認し、「`/init`実行」の直後に「CLAUDE.mdをコミットする」手順が示されていることを確認する

### User Story 1 の実装

- [X] T002 [P] [US1] `MFCApplication1/MFCApplication1Dlg.cpp` の `ShowCompletionGuide()`（[MFCApplication1Dlg.cpp:585](../../MFCApplication1/MFCApplication1Dlg.cpp#L585)）内、成功時の `guide.Format(...)` の文字列リテラルを変更する。現行の手順2（`/init`実行）と手順3（テスト方針決定）の間に「3. 生成された CLAUDE.md をコミットする（git add -A / git commit -m "CLAUDE.md を追加"）」を挿入し、以降の手順番号を1つずつ繰り下げる（旧3→4、旧4→5）（[research.md](./research.md) R-1201）。既存の手順1・2・4・5（振り直し後）の文言、`m_workerPath` の埋め込み方法、失敗時の案内文（`anyFailed` が真の場合の分岐）は一切変更しない（FR-1202・FR-1203）。コメントに `FR-1201`・`FR-1202`・`FR-1203`・`FR-1204` を付す
- [X] T003 [P] [US1] `MFCApplication1/MFCApplication1.rc` を変更する（006 FR-507a対応、[research.md](./research.md) R-1202）: (a) `IDC_STATIC_GUIDE`（`EDITTEXT`行、`7,147,531,52`）の高さを`52`から`61`に変更する。(b) `IDOK`（`DEFPUSHBUTTON`、`434,203,50,14`）と`IDCANCEL`（`PUSHBUTTON`、`488,203,50,14`）のY座標を`203`から`212`に変更する。(c) `IDD_MFCAPPLICATION1_DIALOG DIALOGEX`の全体サイズ（`0, 0, 545, 224`）の高さを`224`から`233`に変更する。3箇所の増分がいずれも`+9`で一致することを確認する。ファイルは既存どおりUTF-16LE BOM付きで保存する（CLAUDE.md記載のエンコーディング規則）。近傍にFR-1205のコメントを1行追加する

### User Story 1 の検証

- [X] T004 [US1] [quickstart.md](./quickstart.md) 手順1の**ビルド**を実施する。Debug/Release × x64/x86 の8行すべてで警告=0 エラー=0。T002・T003 に依存 — **結果: 合格**（4構成すべて警告=0 エラー=0）
- [X] T005 [US1] [quickstart.md](./quickstart.md) 手順2の **ST-1201**（手順順序: `/init`実行→CLAUDE.mdコミット→テスト方針決定、既存4文言の維持、失敗時案内文の無変更、FR-1201〜1204の追跡可能性、`SetupRunner.cpp`・`ProcessRunner.cpp`・`ProcessRunner.h`に012による追加差分が無いこと）を実施する。T004 に依存 — **結果: 全項目合格**（順序True、既存4文言すべてTrue、失敗時案内文は無変更、FR-1201〜1204すべて1件、無関係ファイルへの追加差分なし）
- [X] T006 [US1] [quickstart.md](./quickstart.md) 手順2の **ST-1202**（`.rc`の寸法変更: `IDC_STATIC_GUIDE`高さ61、`IDOK`/`IDCANCEL`のY座標212、ダイアログ全体の高さ233、3箇所の増分が`+9`で一致すること、FR-1205の追跡可能性）を実施する。T004 に依存 — **結果: 全項目合格**（61・212・212・233すべて一致、増分3箇所とも+9、FR-1205 1件）
- [ ] T007 [US1] [quickstart.md](./quickstart.md) 手順3の **DT-1201**（実機でのフルフロー確認、任意・利用者による手動実行）を実施する。uv・specify CLI・claude CLI・gitがすべて導入済みの環境で「生成」を実行し、5手順すべての成功後に表示される完了案内でSC-1201・SC-1202・SC-1203（7行すべてがスクロールなしで見えること、R-1202の見積もりの実機裏付け）を確認する。実行するかどうかは利用者の判断に委ねる。T005・T006 に依存 — **未実施（推奨）**: 008以降と同じ理由（`m_targetPath`はフォルダ選択ダイアログ経由でしか設定できずUI自動化できない）に加え、`.rc`の寸法見積もり（61 DLU）が実機で実際に7行を収められるかは、この手動確認でのみ裏付けられる。利用者による実機確認を推奨する

**チェックポイント**: ここまでで US1 は独立して完結する。完了案内から「`/init`実行→CLAUDE.mdコミット」の順序が読み取れ、かつ006が保証した「スクロールなしで全文が読める」水準が維持されていることが確認できる

---

## Phase 4: Polish & Cross-Cutting Concerns（仕上げと横断的関心事）

- [X] T008 [P] [quickstart.md](./quickstart.md) 付録の**コード分析**（`RunCodeAnalysis=true` / `EnablePREfast=true`）を実施し、指摘が0件であること（憲章の技術的制約）。T005・T006 に依存 — **結果: 合格**（出力0件）
- [X] T009 T005・T006が合格したら、`CLAUDE.md` の「既存機能」の表に012を追加する。あわせて、案内欄の高さが「いまの中身に合わせて決めた値」（006 FR-507a）であり、今回0行だった余裕を使い切ったことを踏まえ、006の該当表の記載更新が必要かどうかを確認する（006自体の成果物を書き換えるかは、006のオーナーシップに関わるため本タスクでは判断のみとし、変更は行わない）。**T005またはT006が不合格の場合はこのタスクを実施しない**。T008 に依存 — **結果: 済**。既存機能の表に012を追加した。006自体の成果物（spec.md/research.md）は、プロジェクトの規約（後続機能は差分のみを記述し前機能の文書を書き換えない）に従い変更しないと判断した。差分は012/spec.md・research.mdに記録済み

---

## Dependencies & Execution Order（依存関係と実行順序）

### Phase Dependencies（フェーズの依存関係）

- **Setup (Phase 1)**: 依存なし。**T001 は実装を始める前に必ず実施する**
- **Foundational (Phase 2)**: タスクなし（該当なし）
- **User Story 1 (Phase 3)**: T001 の後
- **Polish (Phase 4)**: US1 の完了後

### 重要な順序

| タスク | 依存 | 理由 |
|---|---|---|
| T002・T003 | — | `MFCApplication1Dlg.cpp`（文字列）と`MFCApplication1.rc`（寸法）は別ファイルへの独立した変更であり、互いへの依存も他フィーチャーへの依存もない |
| T007（DT-1201） | 利用者の任意判断 | 実機でのフルフロー確認は環境依存のため、実行するかは利用者に委ねる。ただしR-1202の見積もりを実機で裏付ける唯一の手段でもあるため実行を推奨する |
| T009（CLAUDE.md更新） | T005・T006の合格 | 効果が確認できていない変更を既存機能表に記録しない |

### Parallel Opportunities（並行実行の機会）

- T002（`MFCApplication1Dlg.cpp`）とT003（`MFCApplication1.rc`）は異なるファイルへの変更のため並行実行可能 [P]
- T005（ST-1201）とT006（ST-1202）は互いに独立して並行実行可能
- T008（コード分析）はT005・T006完了後、T007（任意）の結果を待たずに実施してよい

---

## Implementation Strategy（実装戦略）

### MVP First（User Story 1 のみ、本フィーチャーの全体）

1. Phase 1: T001（ベースライン取得）
2. Phase 3: T002〜T006（実装と自動検証。T007は任意）
3. **停止して検証**: ST-1201・ST-1202の結果を確認する。DT-1201（実機フルフロー、7行表示の実機確認を含む）は利用者の任意だが推奨
4. Phase 4: コード分析、既存機能表の更新

### 実装順序に関する注意

- **T002は成功時の分岐（`anyFailed`が偽の場合）にのみ変更する。** 失敗時の案内文（「一部の手順が失敗しました...」）には触れない（FR-1203）
- **T002で既存の手順1・2の文言、`m_workerPath`の埋め込みをそのまま残す。** 変更するのは新しい手順の挿入と、それに伴う後続手順の番号のみ
- **T003は3箇所（案内欄高さ・ボタンY座標×2・ダイアログ高さ）をすべて同じ`+9`で変更する。** 1箇所でも変更を忘れると、ボタンが案内欄と重なる、またはダイアログ下端の余白が崩れる
- **T003は`.rc`のエンコーディング（UTF-16LE BOM付き）を保つこと。** PowerShellで編集する場合は`[System.Text.Encoding]::Unicode`で読み書きする（CLAUDE.md記載の規則）

---

## Notes（注記）

- [P] タスク = 異なるファイル、依存関係なし
- [Story] ラベルは、トレーサビリティのためにタスクを特定のユーザーストーリーに対応付ける
- 検証タスク（T004〜T007）はコードを書くタスクではなく、[quickstart.md](./quickstart.md) の手順を実施して結果を確認するタスクである
- T007（DT-1201）は環境依存のため必須ではないが、`.rc`の寸法見積もり（R-1202）が実機で正しいかを確認できる唯一の手段でもあるため、実行を強く推奨する。T005・T006（ST-1201・1202）が本フィーチャーの自動検証手段である
