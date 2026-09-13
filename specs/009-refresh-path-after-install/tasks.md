---

description: "新規インストール直後のPATH未反映を解消する（009）の実装タスク"
---

# Tasks: 新規インストール直後のPATH未反映を解消する

**Input**: `/specs/009-refresh-path-after-install/` からの設計ドキュメント

**Prerequisites（前提条件）**: [plan.md](./plan.md)、[spec.md](./spec.md)、[research.md](./research.md)、[data-model.md](./data-model.md)、[contracts/path-refresh-contract.md](./contracts/path-refresh-contract.md)、[quickstart.md](./quickstart.md)

**Tests（テスト）**: 本リポジトリに自動テストスイートは存在しない（憲章の技術的制約）。代わりに静的テスト ST-901〜902、動的テスト DT-901〜903 を [quickstart.md](./quickstart.md) の手順を実施するタスクとして定義する。DT-901はフォルダ選択ダイアログを伴うため実機推奨（必須ではない）。DT-902・DT-903はスタンドアロンハーネス（008で確立した手法）で自動化する。

**Organization（構成）**: タスクはユーザーストーリーごとにグループ化する。US1・US2は実装コードの大部分を共有する（同一関数内の設計であるため）が、独立したテスト観点として分離する。

## Format（形式）: `[ID] [P?] [Story] Description`

- **[P]**: 並行実行可能（異なるファイル、依存関係なし）
- **[Story]**: US1 = 新規インストールしたツールを同一実行内の後続手順から使える（P1）、US2 = PATHの再読み込みに失敗しても既存の動作を壊さない（P2）

---

## Phase 1: Setup（共有インフラ）

**目的**: 実装前の状態を記録する

- [X] T001 アプリを終了させ（`Get-Process MFCApplication1 -ErrorAction SilentlyContinue | Stop-Process -Force`）、実装前の `MFCApplication1/ProcessRunner.cpp` に `RegGetValue`・`SetEnvironmentVariable`・`HKEY_CURRENT_USER`・`HKEY_LOCAL_MACHINE` がいずれも0件であることを控えておく（[quickstart.md](./quickstart.md) ST-901 のベースライン）— **結果: 記録済み**（4項目とも0件）

---

## Phase 2: Foundational（ブロッキングとなる前提条件）

**目的**: 両ユーザーストーリーが依存する共有の基盤コードを用意する

本フィーチャーには該当タスクがない。変更は `ProcessRunner.cpp` 1ファイルの `Run()` に閉じており、US1・US2はいずれもこの同じ実装に対する異なるテスト観点であるため、別途の基盤整備は不要（[plan.md](./plan.md) Project Structure）。Setup の後、直接 User Story 1 の実装へ進める。

---

## Phase 3: User Story 1 - 新規インストールしたツールを同一実行内の後続手順から使える (Priority: P1) 🎯 MVP

**目標**: 手順1（uv導入）や手順2（specify CLI導入）で新規インストールされたツールを、続く手順が「見つかりません」と誤判定せずに検出できるようにする

**独立したテスト**: uvとspecify CLIが未導入の環境で「生成」を実行し、手順1成功直後の手順2が「uvが見つかりません」で失敗しないこと、手順2成功直後の手順4が「specifyが見つかりません」で失敗しないことを確認する

### User Story 1 の実装

- [X] T002 [US1] `MFCApplication1/ProcessRunner.cpp` の無名名前空間に `CString ReadRegistryPath(HKEY hive, LPCWSTR subKey)` を追加する。`RegGetValueW`（`RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ`、`RRF_NOEXPAND`は指定しない）でサブキー配下の`Path`値を読み取り、`REG_EXPAND_SZ`は自動展開された状態で返す（[research.md](./research.md) R-901）。取得できない場合（キー/値が存在しない、サイズ0等）は空の`CString`を返す。コメントに`FR-901`・`FR-902`・`FR-904`を付す
- [X] T003 [US1] `MFCApplication1/ProcessRunner.cpp` の `Run()`（[ProcessRunner.cpp:79](../../MFCApplication1/ProcessRunner.cpp#L79)付近、`CreatePipe`より前）に、PATH追記処理を追加する: (a) `GetEnvironmentVariable(_T("PATH"), ...)` で現在のプロセスのPATHを取得する、(b) T002のヘルパーで `HKEY_LOCAL_MACHINE` の `SYSTEM\CurrentControlSet\Control\Session Manager\Environment` と `HKEY_CURRENT_USER` の `Environment` から、それぞれシステムPATH・ユーザーPATHを取得する、(c) 「現在のPATH」＋「システムPATH」＋「ユーザーPATH」の順で空でない部分だけを`;`で連結する（[research.md](./research.md) R-902）、(d) システムPATH・ユーザーPATHの**少なくとも一方が空でない**場合に`SetEnvironmentVariable(_T("PATH"), 連結結果)`を呼ぶ（片方だけ読めた場合でも、読めた方は反映する）。**両方とも空の場合のみ**何もしない（[research.md](./research.md) R-904、FR-903のフォールバックを兼ねる）。T002 に依存。コメントに`FR-901`・`FR-903`・`FR-905`・`FR-906`を付す

### User Story 1 の検証

- [X] T004 [US1] [quickstart.md](./quickstart.md) 手順1の**ビルド**を実施する。Debug/Release × x64/x86 の8行すべてで警告=0 エラー=0。T003 に依存 — **結果: 合格**
- [X] T005 [US1] [quickstart.md](./quickstart.md) 手順2の **ST-901**（`RegGetValue`・`SetEnvironmentVariable`・`HKEY_CURRENT_USER`・`HKEY_LOCAL_MACHINE`がいずれも1件以上、かつ`SetupRunner.cpp`・`ProcessRunner.h`に差分がないこと）を実施する。T004 に依存 — **結果: 合格**（RegGetValue=2, SetEnvironmentVariable=1, HKEY_CURRENT_USER=1, HKEY_LOCAL_MACHINE=1。`ProcessRunner.h`差分0、`SetupRunner.cpp`の差分は008由来のもののみで009による追加差分なし）
- [X] T006 [US1] [quickstart.md](./quickstart.md) 手順2の **ST-902**（FR-901〜906の追跡可能性、コメント行を除外しない）を実施する。T004 に依存 — **結果: 合格**（FR-901〜906すべて1件以上出現）
- [ ] T007 [US1] [quickstart.md](./quickstart.md) 手順3の **DT-901**（実際のSDD化シナリオ、実機推奨）を実施する。uv・specify CLIが未導入の環境が用意できる場合に実行し、SC-901・SC-902を確認する。環境依存のため必須ではないが強く推奨。T005・T006 に依存 — **未実施**: 008のDT-801と同じ理由（`m_targetPath`はフォルダ選択ダイアログ経由でしか設定できずUI自動化できない）に加え、この開発機は前回の実機確認でuvが既に導入済みになったため、「uv未導入」の前提自体がもう再現できない。specify CLI未導入の前提はまだ残っているため、利用者が実機で「生成」を実行すれば、少なくともSC-902（specify CLI新規導入直後の手順4）は自然に検証できる
- [X] T008 [US1] [quickstart.md](./quickstart.md) 手順4の **DT-902**（PATH追記の仕組みそのものの確認、スタンドアロンハーネス＋レジストリ一時操作）を実施する。`fake_tool.exe`をユーザーPATH（レジストリのみ）に一時追加し、既存プロセスのPATHには乗っていない状態から`ProcessRunner::Run()`経由で発見・起動できることを確認する。あわせて`Run()`呼び出し前後で`TEMP`等PATH以外の環境変数が変化していないことを確認する（FR-905、M2対応）。最後に必ずユーザーPATHのレジストリ値を元に戻す。T005・T006 に依存 — **結果: 合格**。`fake_tool.exe`をレジストリのユーザーPATHのみに追加した状態（このPowerShellセッション自身の`$env:PATH`には未反映であることを確認済み）から、`ProcessRunner::Run(_T("fake_tool.exe"))`が`launched=1 exitCode=0 succeeded=1 output=[FOUND]`で発見・実行できた。`TEMP`環境変数は呼び出し前後で完全一致（FR-905）。ユーザーPATHのレジストリ値は復元確認済み（`復元確認: True`）

**チェックポイント**: ここまでで US1 は独立して完結する。新規インストールしたツールが同一実行内で検出できることが実測で確認できる

---

## Phase 4: User Story 2 - PATHの再読み込みに失敗しても既存の動作を壊さない (Priority: P2)

**目標**: レジストリ読み取りが失敗する状況でも、アプリが異常終了せず、既存のコマンド検出が引き続き機能することを確認する

**独立したテスト**: T002のヘルパーへ存在しないレジストリサブキーを直接渡し、クラッシュせず空の結果が返ることを確認する（US1のレジストリ書き換えを伴う検証とは独立して実施できる）

### User Story 2 の実装

本ストーリーが必要とする耐障害性（レジストリ読み取り失敗時に空文字列へフォールバックし、両方失敗時は`SetEnvironmentVariable`自体を呼ばない）は、T002・T003の設計に最初から組み込まれている（[research.md](./research.md) R-904）。したがって本ストーリー専用の追加実装タスクはない。以下は検証タスクのみ。

### User Story 2 の検証

- [X] T009 [US2] [quickstart.md](./quickstart.md) 手順5の **DT-903**（無効なレジストリキーでも異常終了しないこと、ホワイトボックス確認）を実施する。スクラッチパッドの検証用`.cpp`で`ProcessRunner.cpp`を`#include`し、T002の`ReadRegistryPath`を存在しないサブキーと実在するサブキーの両方で直接呼び出し、前者が空・後者が非空の結果をクラッシュなく返すことを確認する。あわせて`HKCU`配下の一時テスト用サブキーに`REG_EXPAND_SZ`型の値（`%TEMP%\dummy`等）を書き込んで同ヘルパーで読み取り、展開後に`%`が残っていないことを確認する（FR-902、L1対応）。確認後、テスト用サブキーを削除する。T003 に依存 — **結果: 合格**。存在しないサブキー`Environment\NonExistentSubkeyForTest009`→`isEmpty=1`（クラッシュなし）。実在する`Environment`→`isEmpty=0 length=293`。`REG_EXPAND_SZ`の`%TEMP%\dummy`→展開後`C:\Users\user\AppData\Local\Temp\dummy`（`%`残存なし、FR-902確認）。テスト用サブキーは削除済み（残存確認: False）
- [X] T010 [US2] T008で使用したハーネスに、通常どおり短時間で正常終了するコマンド（例: `cmd.exe /c echo hello`）の実行も含め、PATH追記処理を経由しても既存のコマンド実行が壊れていないこと（`launched=1 exitCode=0 succeeded=1`）を確認する（SC-903・SC-904）。T008 に依存 — **結果: 合格**（T008のハーネス内`[REGRESSION]`ログ: `launched=1 exitCode=0 succeeded=1 output=[hello]`）

**チェックポイント**: ここまでで US1・US2 の両方が独立して機能する。新規インストールの検出とレジストリ読み取り失敗への耐性の両方が実測で確認できる

---

## Phase 5: Polish & Cross-Cutting Concerns（仕上げと横断的関心事）

- [X] T011 [P] [quickstart.md](./quickstart.md) 付録の**コード分析**（`RunCodeAnalysis=true` / `EnablePREfast=true`）を実施し、指摘が0件であること（憲章の技術的制約）。T009・T010 に依存 — **結果: 合格**（出力0件）
- [X] T012 T009・T010が合格したら、`CLAUDE.md` の「設計上、壊しやすい箇所」節に本フィーチャーで得た知見を追記する — winget/uvで新規インストールした直後、レジストリのPATHは更新されても既に起動済みの本アプリのプロセスにはそのコピーが反映されず、後続手順が「見つかりません」と誤判定すること、対策として`ProcessRunner::Run()`の先頭でレジストリの2箇所のPATHを読み直し既存PATHへ追記していること。あわせて「既存機能」の表に009を追加する。**T009が不合格の場合はこのタスクを実施しない**。T011 に依存 — **結果: 済**（「設計上、壊しやすい箇所」に1項目追加、既存機能の表に009を追加）

---

## Dependencies & Execution Order（依存関係と実行順序）

### Phase Dependencies（フェーズの依存関係）

- **Setup (Phase 1)**: 依存なし。**T001 は実装を始める前に必ず実施する**
- **Foundational (Phase 2)**: タスクなし（該当なし）
- **User Story 1 (Phase 3)**: T001 の後
- **User Story 2 (Phase 4)**: US1の実装（T002・T003）に依存する（同じコードを異なる観点で検証するため、US1のコードが存在しないとUS2のテストが書けない）。この点で008（US1/US2が完全に独立したファイル）とは事情が異なる
- **Polish (Phase 5)**: US1・US2 の両方の完了後

### User Story Dependencies（ユーザーストーリーの依存関係）

- **User Story 1 (P1)**: Setup の後に開始可能。他のストーリーへの依存なし
- **User Story 2 (P2)**: US1の実装タスク（T002・T003）の完了に依存する。ただしUS2の検証タスク（T009）はUS1の検証タスク（T004〜T008）の完了は待たずに並行して進められる

### 重要な順序

| タスク | 依存 | 理由 |
|---|---|---|
| T002→T003（`ProcessRunner.cpp` の変更順） | — | `Run()`内の呼び出しがT002のヘルパーを使うため、ヘルパーを先に実装する必要がある |
| T008（DT-902） | T005・T006 | 静的検証が揃ってから、レジストリを一時操作する動的確認に進む（安全のため） |
| T009（DT-903） | T003のみ | US1の検証（ST-901等）の完了を待たずに、T003の実装が終わっていれば独立して実施できる |
| T012（CLAUDE.md更新） | T009・T010の合格 | 効果が確認できていない知見を書き残さない |

### Parallel Opportunities（並行実行の機会）

- T002・T003 は同一ファイルの同一関数群のため逐次実行が必要（並行不可）
- T005・T006（ST-901・902）は互いに独立して並行実行可能
- T009（US2のDT-903）はT003完了後、T004〜T008（US1の検証）と並行して進めてよい

---

## Implementation Strategy（実装戦略）

### MVP First（User Story 1 のみ）

1. Phase 1: T001（ベースライン取得）
2. Phase 3: T002〜T008（PATH追記の実装と検証）
3. **停止して検証**: 新規インストールしたツールが同一実行内で使えることを確認する

US1だけでも実機で確認された問題は解消するが、US2の検証（レジストリ読み取り失敗への耐性の実測確認）まで行うことで、設計上組み込んだ耐障害性が実際に機能することを裏付けられる。

### Incremental Delivery（段階的な提供）

1. Setup → ベースライン取得
2. User Story 1 → PATH追記の実装（**MVP**）
3. User Story 2 → 耐障害性の検証（実装はUS1に内包済み）
4. Phase 5 → コード分析、知見の記録

### 実装順序に関する注意

- **T003で「両方空なら`SetEnvironmentVariable`を呼ばない」分岐を忘れない。** ここを忘れると、レジストリ読み取りが両方失敗した環境で、既存のPATHを空文字列で上書きしてしまうおそれがある（FR-903に反する重大な回帰）
- **T008の一時的なレジストリ変更は必ず元に戻す。** `[Environment]::GetEnvironmentVariable('Path','User')`で復元確認してからでないと完了とみなさない
- **T009はレジストリを一切書き換えない。** ホワイトボックス方式で内部ヘルパーを直接呼ぶだけであり、T008のような復元手順は不要

---

## Notes（注記）

- [P] タスク = 異なるファイル、依存関係なし
- [Story] ラベルは、トレーサビリティのためにタスクを特定のユーザーストーリーに対応付ける
- 検証タスク（T004〜T010）はコードを書くタスクではなく、[quickstart.md](./quickstart.md) の手順を実施して結果を確認するタスクである
- 検証用の `.cpp`・`.exe` はスクラッチパッドに置き、**リポジトリには追加しない**（憲章の技術的制約）
- T007（DT-901）は環境依存のため必須ではないが推奨。T008（DT-902）・T009（DT-903）は本フィーチャーの根本価値（発見）と安全性（耐障害性）を検証する手段であり必須
