---

description: "生成されるCLAUDE.mdの日本語化（014）の実装タスク"
---

# Tasks: 生成されるCLAUDE.mdの日本語化

**Input**: `/specs/014-japanese-claude-md/` からの設計ドキュメント

**Prerequisites（前提条件）**: [plan.md](./plan.md)、[spec.md](./spec.md)、[research.md](./research.md)、[data-model.md](./data-model.md)、[quickstart.md](./quickstart.md)

**Tests（テスト）**: 本リポジトリに自動テストスイートは存在しない（憲章の技術的制約）。代わりに静的テスト ST-1401 を [quickstart.md](./quickstart.md) の手順として定義する。動的テスト DT-1401 は生成される自然言語の内容を機械的に判定する手段が無いため、利用者による任意の手動実行とする。

**Organization（構成）**: spec.mdにはUser Story 1のみが定義されている。

## Format（形式）: `[ID] [P?] [Story] Description`

- **[Story]**: US1 = 生成されるCLAUDE.mdが日本語で書かれている（P1）

---

## Phase 1: Setup（共有インフラ）

**目的**: 実装前の状態を記録する

- [X] T001 アプリを終了させ（`Get-Process MFCApplication1 -ErrorAction SilentlyContinue | Stop-Process -Force`）、実装前の`MFCApplication1/SetupRunner.cpp`の`RunGenerateClaudeMd`が013時点のコマンドライン（`claude --print /init --permission-mode bypassPermissions --output-format text`、日本語指示なし）のままであることを控えておく（[quickstart.md](./quickstart.md) ST-1401 のベースライン）

---

## Phase 2: Foundational（ブロッキングとなる前提条件）

本フィーチャーには該当タスクがない。変更は`SetupRunner.cpp`の`RunGenerateClaudeMd`内のコマンドライン文字列1箇所に閉じており、共有の新しい型・関数を必要としない。Setup の後、直接 User Story 1 の実装へ進める。

---

## Phase 3: User Story 1 - 生成されるCLAUDE.mdが日本語で書かれている (Priority: P1) 🎯 MVP

**目標**: 手順6（`RunGenerateClaudeMd`）が`/init`を実行する際のコマンドラインに日本語指示を追加する

**独立したテスト**: CLAUDE.mdが存在しない対象フォルダで「生成」を実行し、生成されたCLAUDE.mdの本文（定型ヘッダー2行を除く）が日本語で書かれていることを確認する

### User Story 1 の実装

- [X] T002 [US1] `MFCApplication1/SetupRunner.cpp`の`RunGenerateClaudeMd`（013で追加）内、`ProcessRunner::Run`に渡すコマンドライン文字列を次のように変更する: `_T("claude --print /init --permission-mode bypassPermissions --output-format text")` → `_T("claude --print \"/init のCLAUDE.mdは日本語で書いてください。\" --permission-mode bypassPermissions --output-format text")`（FR-1401、R-1401）。タイムアウト値（15分）・作業ディレクトリ引数・以降の`git add -- CLAUDE.md`／コミット処理は一切変更しない（FR-1402、FR-1403）。コメントに`FR-1401`〜`FR-1403`を付す

### User Story 1 の検証

- [X] T003 [US1] [quickstart.md](./quickstart.md) 手順1の**ビルド**を実施する。Debug/Release × x64/x86 の8行すべてで警告=0 エラー=0。T002 に依存 — **結果: 合格**（4構成すべて警告=0 エラー=0）
- [X] T004 [US1] [quickstart.md](./quickstart.md) 手順2の **ST-1401**（日本語指示の追加、`--permission-mode bypassPermissions`・`--output-format text`・`git add -- CLAUDE.md`の維持、FR-1401〜1403の追跡可能性、`SetupRunner.h`・`MFCApplication1Dlg.cpp`・`MFCApplication1.rc`・`ProcessRunner.*`に014による追加差分が無いこと）を実施する。T003 に依存 — **結果: 全項目合格**（日本語指示True、既存2フラグTrue、git add単体ステージングTrue、FR-1401〜1403すべて1件。他ファイルの差分は008〜013由来のみで014による追加なし）
- [X] T005 [US1] [quickstart.md](./quickstart.md) 手順3の **DT-1401**（実際に生成されたCLAUDE.mdが日本語で書かれているかの目視確認、任意・利用者による手動実行）を実施する。実行するかどうかは利用者の判断に委ねる。T004 に依存 — **結果: 部分的に実施**（フォルダ選択ダイアログはGUI自動化不能のため未実施。013のT010と同じホワイトボックス手法で、更新後の`RunGenerateClaudeMd()`を実プロジェクト構成の対象フォルダ`C:\work2\test\MFCApplication1`に対して実行し確認した。1回目: `StepResult::Success`、生成された`CLAUDE.md`の本文（見出し・説明文）はすべて正しいUTF-8の日本語（定型ヘッダー2行のみ英語のまま、想定通り）。`git log`に「CLAUDE.md を追加」の単独コミットとして記録。2回目: `StepResult::Skipped`でべき等性を確認。**未確認のまま残る項目**: フォルダ選択ダイアログを介したGUI操作そのものは利用者による目視確認を引き続き推奨する**

**チェックポイント**: ここまでで US1 は完結する。「生成」を実行すると、手順6が生成するCLAUDE.mdの本文が日本語になる

---

## Phase 4: Polish & Cross-Cutting Concerns（仕上げと横断的関心事）

- [X] T006 [P] [quickstart.md](./quickstart.md) 付録の**コード分析**（`RunCodeAnalysis=true` / `EnablePREfast=true`）を実施し、指摘が0件であること。T004 に依存 — **結果: 合格**（出力0件）
- [X] T007 T004が合格したら、`CLAUDE.md`（プロジェクト直下）の「既存機能」の表に014を追加し、「設計上、壊しやすい箇所」の013で追加した`claude --print /init ...`の説明に、日本語指示を含む最新のコマンドラインを反映する。**T004が不合格の場合はこのタスクを実施しない**。T006 に依存 — **結果: 済**。既存機能の表に014を追加し、013の`claude --print /init ...`のコマンド例を日本語指示付きに更新。あわせて「/initのプロンプトに日本語指示を加えないと英語になる」という発見を新規の壊しやすい箇所として追記した

---

## Dependencies & Execution Order（依存関係と実行順序）

- **Setup (Phase 1)**: 依存なし。**T001 は実装を始める前に必ず実施する**
- **Foundational (Phase 2)**: タスクなし
- **User Story 1 (Phase 3)**: T001 の後
- **Polish (Phase 4)**: US1 の完了後
- T005（DT-1401）は利用者の任意判断。T007（CLAUDE.md更新）はT004の合格が前提

---

## Notes（注記）

- 本フィーチャーは単一ファイル・単一箇所（コマンドライン文字列1つ）の変更であるため、並行実行可能な[P]タスクは検証・仕上げ以外に存在しない
- 検証タスク（T003〜T005）はコードを書くタスクではなく、[quickstart.md](./quickstart.md) の手順を実施して結果を確認するタスクである
