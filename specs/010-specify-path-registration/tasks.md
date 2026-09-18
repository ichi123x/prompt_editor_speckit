---

description: "specify CLI導入後のPATH未登録を解消する（010）の実装タスク"
---

# Tasks: specify CLI導入後のPATH未登録を解消する

**Input**: `/specs/010-specify-path-registration/` からの設計ドキュメント

**Prerequisites（前提条件）**: [plan.md](./plan.md)、[spec.md](./spec.md)、[research.md](./research.md)、[data-model.md](./data-model.md)、[quickstart.md](./quickstart.md)。**009（`specs/009-refresh-path-after-install/`）の実装が前提**（[plan.md](./plan.md) Summary参照）。

**Tests（テスト）**: 本リポジトリに自動テストスイートは存在しない（憲章の技術的制約）。代わりに静的テスト ST-1001、動的テスト DT-1001〜1002 を [quickstart.md](./quickstart.md) の手順を実施するタスクとして定義する。DT-1001は安全なレジストリ操作で自動化する。DT-1002はspecify-cliの実際のアンインストールとフォルダ選択ダイアログを伴うため、利用者による任意の手動実行とする。

**Organization（構成）**: spec.mdにはUser Story 1のみが定義されている。

## Format（形式）: `[ID] [P?] [Story] Description`

- **[P]**: 並行実行可能（異なるファイル、依存関係なし）
- **[Story]**: US1 = specify CLI新規導入直後、後続手順からspecifyを使える（P1）

---

## Phase 1: Setup（共有インフラ）

**目的**: 実装前の状態を記録する

- [X] T001 アプリを終了させ（`Get-Process MFCApplication1 -ErrorAction SilentlyContinue | Stop-Process -Force`）、実装前の `MFCApplication1/SetupRunner.cpp` に `uv tool update-shell` の文字列が0件であることを控えておく（[quickstart.md](./quickstart.md) ST-1001 のベースライン）— **結果: 記録済み**（0件）

---

## Phase 2: Foundational（ブロッキングとなる前提条件）

**目的**: User Story 1 が依存する共有の基盤コードを用意する

本フィーチャーには該当タスクがない。変更は `SetupRunner.cpp` の `RunInstallSpecify` 関数1つに閉じており、共有の新しい型・関数を必要としない（[plan.md](./plan.md) Project Structure）。Setup の後、直接 User Story 1 の実装へ進める。

---

## Phase 3: User Story 1 - specify CLI新規導入直後、後続手順からspecifyを使える (Priority: P1) 🎯 MVP

**目標**: `uv tool install specify-cli` が成功した直後に `uv tool update-shell` を呼び、続く手順4（Spec Kit足場配置）がspecifyを検出できるようにする

**独立したテスト**: specify CLIが未導入の環境で「生成」を実行し、手順2成功直後の手順4が「specifyが見つかりません」で失敗しないことを確認する

### User Story 1 の実装

- [X] T002 [US1] `MFCApplication1/SetupRunner.cpp` の `RunInstallSpecify`（[SetupRunner.cpp:109](../../MFCApplication1/SetupRunner.cpp#L109)、挿入点は132行目付近）内、`uv tool install specify-cli` の `ProcessResult`（`install`）が `install.Succeeded()` と判定された直後、`Finish(step, StepResult::Success, ...)` を呼ぶ**前**に、`ProcessRunner::Run(_T("uv tool update-shell"), CString(), 60 * 1000);` を追加で呼ぶ（[research.md](./research.md) R-1001・R-1002）。戻り値（`ProcessResult`）は変数に受けるだけで、`launched`・`Succeeded()`等の判定には一切使わない（FR-1003、[research.md](./research.md) R-1003）。`install.Succeeded()` が false（インストール自体が失敗）の分岐、および冒頭の `probe.Succeeded()`（スキップ）の分岐には追加しない（FR-1002）。コメントに `FR-1001`・`FR-1002`・`FR-1003`・`FR-1004`・`FR-1005` を付す

### User Story 1 の検証

- [X] T003 [US1] [quickstart.md](./quickstart.md) 手順1の**ビルド**を実施する。Debug/Release × x64/x86 の8行すべてで警告=0 エラー=0。T002 に依存 — **結果: 合格**（4構成すべて警告=0 エラー=0）
- [X] T004 [US1] [quickstart.md](./quickstart.md) 手順2の **ST-1001**（`install.Succeeded()` → `uv tool update-shell` → `Finish(Success)` の順になっている、`ProcessRunner.cpp`・`ProcessRunner.h`に010による追加差分がない、FR-1001〜1005の追跡可能性、**加えてFR-1002・FR-1004・SC-1003の追加静的確認**: スキップ分岐に`uv tool update-shell`が入り込んでいないこと、その呼び出し結果を判定する`if`文が無いこと、`SetupRunner.cpp`の差分が`RunInstallSpecify`関数の範囲に閉じていること）を実施する。T003 に依存 — **結果: 全項目合格**（順序確認True、FR-1001〜1005すべて1件、スキップ分岐にupdate-shellなし、if文での判定なし、`ProcessRunner.cpp/.h`は008・009からの差分のみ、`SetupRunner.cpp`の新規ハンクは132行目付近の13行追加のみ＝`RunInstallSpecify`内に限定）
- [X] T005 [US1] [quickstart.md](./quickstart.md) 手順3の **DT-1001**（`uv tool update-shell`のPATH登録効果を安全なレジストリ操作で確認）を実施する。ユーザーPATHから一時的に`.local\bin`を取り除いた状態から実行して`.local\bin`が復活することを確認したうえで、必ず元のユーザーPATHへ復元する。T004 に依存 — **結果: 合格**（取り除き後`False`→`uv tool update-shell`実行後`True`→復元確認`True`）
- [X] T006 [US1] [quickstart.md](./quickstart.md) 手順4の **DT-1003**（スキップパスの回帰確認、ホワイトボックス、SC-1002）を実施する。`SetupRunner.cpp`を`#include`で直接取り込み、`RunInstallSpecify`をこの開発機の現在の状態（specify-cli導入済み）で直接呼び、結果が`Skipped`・理由文字列が完全一致することを確認する。specify-cliのインストール状態には一切触れない。T004 に依存 — **結果: 合格**（`isSkipped=1 reasonMatches=1`、`uv tool update-shell`は呼ばれない）
- [ ] T007 [US1] [quickstart.md](./quickstart.md) 手順5の **DT-1002**（GUIでの完全なエンドツーエンド確認、任意・利用者による手動実行）を実施する。`uv tool uninstall specify-cli`で意図的に未導入へ戻し、実機でSC-1001を確認する。実行するかどうかは利用者の判断に委ねる。T004 に依存 — **未実施（任意）**: T005・T006（DT-1001・DT-1003）で仕組みの効果とスキップパスの回帰なしを実測済みのため必須ではない。利用者がspecify-cliの実アンインストールを許容できる場合に任意で実施。**2026-09-18判断**: 個々の利用者環境に依存するため検証不要と判断し、以後実施しない（`spec.md` Status は `Implemented`。本タスクの`[ ]`は未実施の事実を保持するため意図的に残す）

**チェックポイント**: ここまでで US1 は独立して完結する。specify CLIの新規インストール直後でも後続手順から検出できることが実測で確認できる

---

## Phase 4: Polish & Cross-Cutting Concerns（仕上げと横断的関心事）

- [X] T008 [P] [quickstart.md](./quickstart.md) 付録の**コード分析**（`RunCodeAnalysis=true` / `EnablePREfast=true`）を実施し、指摘が0件であること（憲章の技術的制約）。T005・T006 に依存 — **結果: 合格**（出力0件）
- [X] T009 T005・T006が合格したら、`CLAUDE.md` の「設計上、壊しやすい箇所」節に本フィーチャーで得た知見を追記する — `uv tool install`はインストール先ディレクトリ（既定`%USERPROFILE%\.local\bin`）をレジストリのPATHへ自動登録しないため、009のPATH再読み込みだけでは効果がなく、`uv tool update-shell`を明示的に呼ぶ必要があること。あわせて「既存機能」の表に010を追加する。**T005またはT006が不合格の場合はこのタスクを実施しない**。T008 に依存 — **結果: 済**（「設計上、壊しやすい箇所」に1項目追加、009の記述の古い注記も訂正、既存機能の表に010を追加）

---

## Dependencies & Execution Order（依存関係と実行順序）

### Phase Dependencies（フェーズの依存関係）

- **Setup (Phase 1)**: 依存なし。**T001 は実装を始める前に必ず実施する**
- **Foundational (Phase 2)**: タスクなし（該当なし）
- **User Story 1 (Phase 3)**: T001 の後。**009の実装が前提**（`ProcessRunner::Run()`のPATH再読み込みが無いと、`uv tool update-shell`がレジストリへ書いた内容を手順4が拾えない）
- **Polish (Phase 4)**: US1 の完了後

### 重要な順序

| タスク | 依存 | 理由 |
|---|---|---|
| T002 | 009の実装 | `uv tool update-shell`単体では、書き込み先のレジストリを次の`ProcessRunner::Run()`が読み直す仕組み（009）がないと効果が手順4に届かない |
| T006（DT-1003） | T004 | ホワイトボックス方式のため、実装（T002）とST-1001の静的確認が済んでから行う |
| T007（DT-1002） | 利用者の任意判断 | specify-cliを実際にアンインストールするため、実行するかは利用者に委ねる |
| T009（CLAUDE.md更新） | T005・T006の合格 | 効果が確認できていない知見を書き残さない |

### Parallel Opportunities（並行実行の機会）

- 本フィーチャーは変更ファイルが1つ（`SetupRunner.cpp`）のみのため、T002〜T006は逐次実行が基本
- T008（コード分析）はT005・T006完了後、T007（任意）の結果を待たずに実施してよい

---

## Implementation Strategy（実装戦略）

### MVP First（User Story 1 のみ、本フィーチャーの全体）

1. Phase 1: T001（ベースライン取得）
2. Phase 3: T002〜T006（実装と自動検証）
3. **停止して検証**: DT-1001・DT-1003の結果を確認する。DT-1002（GUIでの完全確認）は利用者の任意
4. Phase 4: コード分析、知見の記録

### 実装順序に関する注意

- **T002はinstallの成功分岐にのみ追加する。** スキップ（既に導入済み）や失敗の分岐に追加すると、不要な呼び出しが増えたりFR-1002に反したりする
- **T002で`uv tool update-shell`の戻り値を判定に使わない。** `if`文で結果を分岐させると、それ自体が新しいエラー経路になりFR-1003に反する
- **T005の一時的なPATH変更は必ず元に戻す。** `.local\bin`を取り除いた状態で終わらないよう、`[Environment]::GetEnvironmentVariable('Path','User')`で復元確認してから完了とする
- **T006はspecify-cliのインストール状態に一切触れない。** DT-1002（T007）とは異なり、現在の状態（導入済み）でスキップ分岐を確認するだけであり、アンインストール等の破壊的操作は行わない

---

## Notes（注記）

- [P] タスク = 異なるファイル、依存関係なし（本フィーチャーは実質1ファイルのため該当タスクは少ない）
- [Story] ラベルは、トレーサビリティのためにタスクを特定のユーザーストーリーに対応付ける
- 検証タスク（T003〜T007）はコードを書くタスクではなく、[quickstart.md](./quickstart.md) の手順を実施して結果を確認するタスクである
- T007（DT-1002）はspecify-cliの実際のアンインストールを伴うため必須ではない。T005（DT-1001）・T006（DT-1003）が本フィーチャーの主たる自動検証手段である
