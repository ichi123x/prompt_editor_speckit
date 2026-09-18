---

description: "外部コマンドの無応答ハング修正（008）の実装タスク"
---

# Tasks: 外部コマンドの無応答ハング修正

**Input**: `/specs/008-fix-external-command-hang/` からの設計ドキュメント

**Prerequisites（前提条件）**: [plan.md](./plan.md)、[spec.md](./spec.md)、[research.md](./research.md)、[data-model.md](./data-model.md)、[contracts/process-runner-contract.md](./contracts/process-runner-contract.md)、[quickstart.md](./quickstart.md)

**Tests（テスト）**: 本リポジトリに自動テストスイートは存在しない（憲章の技術的制約）。代わりに静的テスト ST-801〜803、動的テスト DT-801〜802 を [quickstart.md](./quickstart.md) の手順を実施するタスクとして定義する。DT-801 は `m_targetPath` がフォルダ選択ダイアログ経由でしか設定できずUI自動化できないため利用者による実機実行が必要（環境依存のため必須ではない）。DT-802 は当初GUI経由の手動タスクとして計画したが、同じ制約に阻まれると判明したため、`ProcessRunner::Run()` を直接呼ぶスクラッチパッド限定のスタンドアロンハーネスに設計変更し自動実施した（実装時に判明・是正、詳細はT012参照）。

**Organization（構成）**: タスクはユーザーストーリーごとにグループ化する。

## Format（形式）: `[ID] [P?] [Story] Description`

- **[P]**: 並行実行可能（異なるファイル、依存関係なし）
- **[Story]**: US1 = uv導入(winget)が対話プロンプトで止まらない（P1）、US2 = 外部コマンドが無応答でも確実にタイムアウトする（P2）

---

## Phase 1: Setup（共有インフラ）

**目的**: 実装前の状態を記録し、後で「変更されるべきでない部分が変わっていないこと」を突き合わせられるようにする

- [X] T001 アプリを終了させ（`Get-Process MFCApplication1 -ErrorAction SilentlyContinue | Stop-Process -Force`、起動中だとビルドが `LNK1104` で失敗する）、実装前の `MFCApplication1/SetupRunner.cpp` の `winget install` 行と、`MFCApplication1/ProcessRunner.cpp` に `CreateJobObject`・`CREATE_SUSPENDED`・`CreateThread`・`TerminateJobObject` がいずれも0件であることを控えておく（[quickstart.md](./quickstart.md) ST-801・ST-803 のベースライン）— **結果: 記録済み**（`winget install --id=astral-sh.uv -e` のみ、ProcessRunner.cppは4項目とも0件）

---

## Phase 2: Foundational（ブロッキングとなる前提条件）

**目的**: 両ユーザーストーリーが依存する共有の基盤コードを用意する

本フィーチャーには該当タスクがない。US1（`SetupRunner.cpp` のみ）と US2（`ProcessRunner.cpp` のみ）は別ファイル・別関数に閉じており、共有の新しい型・関数を必要としない（[plan.md](./plan.md) Project Structure）。Setup の後、直接それぞれのユーザーストーリーへ進める。

---

## Phase 3: User Story 1 - uv導入が対話プロンプトで止まらない (Priority: P1) 🎯 MVP

**目標**: `winget install --id=astral-sh.uv -e` が `msstore` ソースの利用規約同意プロンプトで無期限にハングしないようにする

**独立したテスト**: uv未導入かつwinget利用規約未同意の環境で「生成」を実行し、手順1（uv導入）がユーザー入力なしに成功・スキップ・失敗のいずれかで確定することを確認する

### User Story 1 の実装

- [X] T002 [US1] `MFCApplication1/SetupRunner.cpp` の `RunInstallUv`（[SetupRunner.cpp:90](../../MFCApplication1/SetupRunner.cpp#L90)付近）にある `winget install --id=astral-sh.uv -e` のコマンドラインに `--accept-source-agreements --accept-package-agreements --source winget` を追加する。`--source winget` により `msstore` ソースへのアクセス自体を回避し、残る2フラグで将来のソース/パッケージ同意プロンプトにも備える（FR-801、[research.md](./research.md) R-801）。コメントに `FR-801` を付す

### User Story 1 の検証

- [X] T003 [US1] [quickstart.md](./quickstart.md) 手順1の**ビルド**を実施する。Debug/Release × x64/x86 の8行すべてで警告=0 エラー=0。T002 に依存 — **結果: 合格**（4構成すべて警告=0 エラー=0）
- [X] T004 [US1] [quickstart.md](./quickstart.md) 手順2の **ST-801**（winget コマンドラインに3フラグすべてが含まれる）を実施する。T003 に依存 — **結果: 合格**（`--accept-source-agreements --accept-package-agreements --source winget` すべて含む1行を確認）
- [X] T005 [US1] [quickstart.md](./quickstart.md) 手順3の**既存機能の回帰確認**（`git diff --stat -- MFCApplication1/SetupRunner.cpp` が winget の1行以外に差分がないこと）を実施する。T003 に依存 — **結果: 合格**（差分は`RunInstallUv`内のFR-801コメント3行とwingetコマンドライン変更のみ。他の手順のコードは無変更）
- [ ] T006 [US1] [quickstart.md](./quickstart.md) 手順4の **DT-801**（実際のuv導入シナリオ、実機推奨）を実施する。未同意環境が用意できる場合は実行し、SC-801（ユーザー入力なしに確定）と、`winget`/`conhost` の残存プロセスがないことを確認する。プロセス強制終了が不要であること自体もSC-804の裏付けとなる。環境依存のため必須ではないが強く推奨。T004・T005 に依存 — **未実施**: `m_targetPath` は `CFolderPickerDialog::DoModal()` の戻り値からのみ設定され、`IDC_EDIT_TARGET_PATH` への `WM_SETTEXT` では更新されないため（[MFCApplication1Dlg.cpp:425](../../MFCApplication1/MFCApplication1Dlg.cpp#L425)）、CLAUDE.mdの制約（フォルダ選択ダイアログは合成入力を受け付けない）によりUI自動化での実施ができない。**利用者による実機での手動実行が必要**（004〜007の前例と同様）。**2026-09-18判断**: 個々の利用者環境に依存するため検証不要と判断し、以後実施しない（`spec.md` Status は `Implemented`。本タスクの`[ ]`は未実施の事実を保持するため意図的に残す）

**チェックポイント**: ここまでで US1 は独立して完結する。winget が対話プロンプトを出さなくなったことが確認できれば、今回実機で発生した事例は解消する

---

## Phase 4: User Story 2 - 外部コマンドが無応答でも確実にタイムアウトする (Priority: P2)

**目標**: `ProcessRunner::Run()` を経由するどの外部コマンド呼び出しが無応答になっても、指定タイムアウトで確実に打ち切られ、プロセスツリーが残存しないようにする

**独立したテスト**: 出力を一度出した後に入力待ちで無応答になり続けるコマンド（例: `cmd.exe /c pause`）を短いタイムアウト値で `ProcessRunner::Run()` に渡し、タイムアウト時間から大きく超過することなく処理が戻り、関連プロセスが残存しないことを確認する（US1 の変更や winget の状態に一切依存しない）

### User Story 2 の実装

- [X] T007 [US2] `MFCApplication1/ProcessRunner.cpp` の `Run()`（[ProcessRunner.cpp:48](../../MFCApplication1/ProcessRunner.cpp#L48)）を変更し、`CreateJobObject` で `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` を設定したジョブオブジェクトを作成する。`CreateProcess` の呼び出しに `CREATE_SUSPENDED` を追加し、`AssignProcessToJobObject` で子プロセスをジョブへ割り当てた**後**に `ResumeThread(pi.hThread)` を呼ぶ（生成直後に子孫プロセスがジョブ外で生まれる競合を防ぐため、[research.md](./research.md) R-804）。コメントに `FR-804` を付す
- [X] T008 [US2] 同じ `Run()` 内に、子プロセスの生死をタイムアウト付きで監視する別スレッドを追加する。スレッドは `WaitForSingleObject(pi.hProcess, timeoutMs)` を行い、`WAIT_TIMEOUT` が返った場合のみ `TerminateJobObject(hJob, WAIT_TIMEOUT)` を呼んでプロセスツリーごと打ち切る（`WAIT_TIMEOUT` の値をそのまま終了コードとして使うことで既存の失敗判定と互換を保つ、FR-803, [research.md](./research.md) R-803）。既存の読み出しループ内にあった `GetTickCount64` による毎回のタイムアウト比較（[ProcessRunner.cpp:122-126](../../MFCApplication1/ProcessRunner.cpp#L122-L126)）は、`ReadFile` がブロックしている間は評価されず無効だったため削除する（FR-802）。読み出しループが終わった後、既存の `WaitForSingleObject(pi.hProcess, remaining)` に続けて監視スレッドの終了を待ち（`WaitForSingleObject(hWatcherThread, ...)`）、ジョブオブジェクト・スレッドハンドルを含むすべてのハンドルを解放する（FR-804）。あわせて `Run()` の冒頭付近に、FR-803（タイムアウト時も既存の失敗表示の仕組みをそのまま使う）・FR-805（5手順の順序・依存・べき等性を変更しない）・FR-806（`ProcessRunner` を使う全呼び出しに等しく適用し特定コマンドを特別扱いしない）をまとめて1箇所に記載するコメントを追加する（quickstart.md ST-802 が個別に検査するため書き漏らさないこと）。T007 に依存。コメントに `FR-802`・`FR-804` を付す

### User Story 2 の検証

- [X] T009 [US2] [quickstart.md](./quickstart.md) 手順1の**ビルド**を実施する。Debug/Release × x64/x86 の8行すべてで警告=0 エラー=0。T008 に依存 — **結果: 合格**（4構成すべて警告=0 エラー=0）
- [X] T010 [US2] [quickstart.md](./quickstart.md) 手順2の **ST-802**（FR-801〜806 の追跡可能性、`SetupRunner.cpp`・`ProcessRunner.cpp` 両方。特にFR-803・FR-805・FR-806 がT008で追加したまとめコメントとして現れること）と **ST-803**（`CreateJobObject`・`AssignProcessToJobObject`・`JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`・`CREATE_SUSPENDED`・`ResumeThread`・`CreateThread`・`TerminateJobObject` がいずれも1件以上、かつ `ProcessRunner.h` に差分がないこと）を実施する。T009 に依存 — **結果: 合格**。ただしquickstart.md記載のST-802検査スクリプトにコメント行を誤って除外するバグを発見し修正した（FR番号はコメントとして記載する設計のため、コメント除外はST-803側にのみ必要）。修正後、SetupRunner.cppにFR-801、ProcessRunner.cppにFR-802〜806が1件以上ずつ出現することを確認。ST-803（7項目すべて1件以上）・ProcessRunner.h無変更も確認
- [X] T011 [US2] [quickstart.md](./quickstart.md) 手順3の**既存機能の回帰確認**（SC-803）を実施する。T009 に依存 — **結果: 合格（範囲限定）**。`ProcessRunner.h`が無変更（T010で確認済み）であることに加え、スタンドアロンハーネス（T012で作成）のTEST1で「通常どおり短時間で正常終了するコマンド」が`launched=1 exitCode=0 succeeded=1`・出力も正しく取得できることを実測確認した。ただし実際のGUIから5手順をフルに流す回帰確認は、`m_targetPath`がフォルダ選択ダイアログ経由でしか設定できないため自動化できず（T006と同じ制約）、**利用者による実機確認を推奨**として残る
- [X] T012 [US2] [quickstart.md](./quickstart.md) 手順5の **DT-802**（無応答コマンドの汎用タイムアウト確認）を実施する。T010・T011 に依存 — **結果: 合格**。当初想定していた「`SetupRunner.cpp`を一時改変してGUIから生成を押す」方式は、T006と同じ理由（`m_targetPath`がフォルダ選択ダイアログ経由でしか設定できない）で実行不可能と判明したため、quickstart.mdを是正し、`ProcessRunner::Run()`をGUIを介さず直接呼ぶスタンドアロンハーネス（スクラッチパッド限定、リポジトリには追加しない）に切り替えた。`cmd.exe /c ping -n 30 127.0.0.1 >NUL`（無応答の模擬、timeoutMs=3000）を実行した結果 `launched=1 exitCode=258(WAIT_TIMEOUT) succeeded=0 elapsedMs=3032`（SC-802: タイムアウト値からの超過はわずか32ms）。実行直後に`ping.exe`（`cmd.exe`が起動した孫プロセス）の残存が0件であることを確認し、ジョブオブジェクトによるプロセスツリー一括終了（FR-804）を実測で裏付けた（`TerminateProcess`単体では`ping.exe`が残存しうる点が、本フィーチャー導入前後の違い）

**チェックポイント**: ここまでで US1・US2 の両方が独立して機能する。`ProcessRunner` を使う既知・未知どちらのハングにも、根拠を持って対処できる状態になる

---

## Phase 5: Polish & Cross-Cutting Concerns（仕上げと横断的関心事）

- [X] T013 [P] [quickstart.md](./quickstart.md) 付録の**コード分析**（`RunCodeAnalysis=true` / `EnablePREfast=true`）を実施し、指摘が0件であること（憲章の技術的制約）。T012 に依存（T006は環境依存のため未完了だが、静的なコード分析はこれと無関係に実施可能と判断） — **結果: 合格**（出力0件）
- [X] T014 T012（DT-802）が合格したら、`CLAUDE.md` の「設計上、壊しやすい箇所」節に本フィーチャーで得た知見を追記する — `ProcessRunner` はパイプを読み出しながら子プロセスの終了を待つが、`ReadFile` 自体が無条件にブロックするため子プロセスが出力を止めると既存のタイムアウト判定が一度も評価されず無効化されること、対策としてジョブオブジェクト＋監視スレッドを導入したこと。あわせて「既存機能」の表に008を追加する。**T012 が不合格の場合はこのタスクを実施しない**。T013 に依存 — **結果: 済**（「設計上、壊しやすい箇所」に2項目追加、既存機能の表に008を追加）

---

## Dependencies & Execution Order（依存関係と実行順序）

### Phase Dependencies（フェーズの依存関係）

- **Setup (Phase 1)**: 依存なし。**T001 は実装を始める前に必ず実施する**
- **Foundational (Phase 2)**: タスクなし（該当なし）
- **User Story 1 (Phase 3)**: T001 の後。US2 に依存しない
- **User Story 2 (Phase 4)**: T001 の後。US1 に依存しない（Phase 3 と並行して進めることも可能）
- **Polish (Phase 5)**: US1・US2 の両方の完了後

### User Story Dependencies（ユーザーストーリーの依存関係）

- **User Story 1 (P1)**: Setup の後に開始可能。他のストーリーへの依存なし
- **User Story 2 (P2)**: Setup の後に開始可能。US1 と別ファイル（`ProcessRunner.cpp` vs `SetupRunner.cpp`）のため独立して実装・テスト可能

### 重要な順序

| タスク | 依存 | 理由 |
|---|---|---|
| T001（ベースライン取得） | — | ST-801・ST-803 が「実装前は0件」であることを前提にした差分確認のため |
| T007→T008（`ProcessRunner.cpp` の変更順） | — | 監視スレッドがジョブオブジェクトのハンドルを参照するため、ジョブオブジェクトの作成・割り当てを先に済ませる必要がある |
| T012（DT-802） | T010・T011 | 静的検証と回帰確認が揃ってから、根本原因（ReadFileのブロッキング）に対する動的確認に進む |
| T014（CLAUDE.md更新） | T012 の合格 | 効果が確認できていない知見を書き残さない |

### Parallel Opportunities（並行実行の機会）

- **Phase 3 と Phase 4**: US1（`SetupRunner.cpp`）と US2（`ProcessRunner.cpp`）は別ファイルのため、T002〜T006 と T007〜T012 は並行して進めてよい
- **Phase 5**: T013 は T006・T012 の両方が揃えば独立して実行可能

---

## Parallel Example: User Story 1 と User Story 2

```powershell
# 別ファイルのため同時に着手できる
# 開発者A: T002〜T006 (US1, SetupRunner.cpp)
# 開発者B: T007〜T012 (US2, ProcessRunner.cpp)
```

---

## Implementation Strategy（実装戦略）

### MVP First（User Story 1 のみ）

1. Phase 1: T001（ベースライン取得）
2. Phase 3: T002〜T006（winget フラグ追加と検証）
3. **停止して検証**: 今回実機で確認できた事例が解消したことを確認する

US1 だけでも今回確認済みの不具合は解消するが、`ProcessRunner` の根本的なタイムアウト欠陥（US2）は残るため、**両方の実施を推奨**する（spec.md User Story 2 の Why this priority を参照）。

### Incremental Delivery（段階的な提供）

1. Setup → ベースライン取得
2. User Story 1 → winget の対話プロンプト対策（**MVP**）
3. User Story 2 → `ProcessRunner` のタイムアウトを汎用的に実効化
4. Phase 5 → コード分析、知見の記録

### 実装順序に関する注意

- **T008 で読み出しループの `GetTickCount64` 比較を消し忘れない。** 監視スレッドと二重にタイムアウト判定を持つと、どちらが実際に効いているのか検証しづらくなる
- **T007 で `CREATE_SUSPENDED` を忘れると、T008 の監視スレッドがあっても孫プロセスの残存を防げないケースが理論上残る**（[research.md](./research.md) R-804）
- **T012 の一時的なコード変更は必ず元に戻す。** `git status` で `SetupRunner.cpp` に差分が残っていないことを確認してからコミットする

---

## Notes（注記）

- [P] タスク = 異なるファイル、依存関係なし
- [Story] ラベルは、トレーサビリティのためにタスクを特定のユーザーストーリーに対応付ける
- 検証タスク（T003〜T006、T009〜T012）はコードを書くタスクではなく、[quickstart.md](./quickstart.md) の手順を実施して結果を確認するタスクである
- 検証用の `.ps1` はスクラッチパッドに置き、**リポジトリには追加しない**（憲章の技術的制約）
- T006（DT-801）は環境依存のため必須ではないが推奨。T012（DT-802）は本フィーチャーの根本修正（US2）を検証する唯一の手段であり必須
