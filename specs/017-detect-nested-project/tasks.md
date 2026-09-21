---

description: "サブフォルダ内のウィザードプロジェクトを検出できるようにする（017）の実装タスク"
---

# Tasks: サブフォルダ内のウィザードプロジェクトを検出できるようにする

**Input**: `/specs/017-detect-nested-project/` からの設計ドキュメント

**Prerequisites（前提条件）**: [plan.md](./plan.md)、[spec.md](./spec.md)、[research.md](./research.md)、[data-model.md](./data-model.md)、[quickstart.md](./quickstart.md)

**Tests（テスト）**: 本リポジトリに自動テストスイートは存在しない（憲章の技術的制約）。代わりに静的テスト ST-1701〜1702、動的テスト DT-1701（スタンドアロンハーネス、完全自動）・DT-1702（実機、フォルダ選択ダイアログを伴うため推奨だが必須ではない）を [quickstart.md](./quickstart.md) の手順を実施するタスクとして定義する。

**Organization（構成）**: タスクはユーザーストーリーごとにグループ化する。US1・US2は同一の実装コード（`TargetFolderState.cpp`のマーカー検出ロジック）を共有するが、「検出できる」（US1）と「誤検出しない」（US2）という独立した観点として検証タスクを分離する。

## Format（形式）: `[ID] [P?] [Story] Description`

- **[P]**: 並行実行可能（異なるファイル、依存関係なし）
- **[Story]**: US1 = プロジェクトの親フォルダを選んでもSDD化できる（P1）、US2 = 無関係なフォルダは引き続き未検出として扱われる（P2）

---

## Phase 1: Setup（共有インフラ）

**目的**: 実装前の状態を記録する

- [X] T001 アプリを終了させ（`Get-Process MFCApplication1 -ErrorAction SilentlyContinue | Stop-Process -Force`）、実装前の `MFCApplication1/TargetFolderState.cpp` に `FR-1701`〜`FR-1705` がいずれも0件であることを控えておく（[quickstart.md](./quickstart.md) ST-1701 のベースライン）— **結果: 記録済み**（5項目とも0件）

---

## Phase 2: Foundational（ブロッキングとなる前提条件）

**目的**: 両ユーザーストーリーが依存する共有の基盤コードを用意する

本フィーチャーには該当タスクがない。変更は `TargetFolderState.cpp` 1ファイルの `Read()` 内マーカー確認に閉じており、US1・US2はいずれもこの同じ実装に対する異なるテスト観点であるため、別途の基盤整備は不要（[plan.md](./plan.md) Project Structure）。Setup の後、直接 User Story 1 の実装へ進める。

---

## Phase 3: User Story 1 - プロジェクトの親フォルダを選んでもSDD化できる (Priority: P1) 🎯 MVP

**目標**: ウィザードが作成した`.sln`/`.vcxproj`等が対象フォルダの直下ではなくサブフォルダにあっても、プロジェクトを検出して後続の手順を進められるようにする

**独立したテスト**: 直下にはプロジェクトファイルが無く、1階層下のサブフォルダに`.sln`があるフォルダを対象フォルダとして選び、「対象のプロジェクトが見つかりません」で中止されずに手順が進行することを確認する

### User Story 1 の実装

- [X] T002 [US1] `MFCApplication1/TargetFolderState.cpp` の無名名前空間に `bool HasProjectMarkerRecursive(const CString& folder)` を追加する。まず既存の4条件（`HasFileMatching(folder, "*.sln")` / `"*.vcxproj"` / `"*.csproj"` / `FileExists(folder + "\\package.json")`）を`folder`自身に対して確認し（FR-1701, FR-1702）、いずれも無ければ`CFileFind`で`folder`直下のサブディレクトリを列挙し、`.`・`..`・`.git`を除く各サブフォルダに対して自身を再帰呼び出しする（FR-1701）。`.git`はサブディレクトリとして列挙はするが再帰対象から除外する（FR-1703）。いずれか1箇所で見つかった時点で`true`を返す早期リターンとする（[research.md](./research.md) R-1701, R-1702）。コメントに`FR-1701`・`FR-1702`・`FR-1703`を付す — **結果: 実装済み**
- [X] T003 [US1] `MFCApplication1/TargetFolderState.cpp` の `Read()`（[TargetFolderState.cpp:85-91](../../MFCApplication1/TargetFolderState.cpp#L85-L91)）で、`state.hasProjectMarker`の代入をT002の`HasProjectMarkerRecursive(state.path)`の呼び出しに置き換える。`state.path`への代入（[TargetFolderState.cpp:83](../../MFCApplication1/TargetFolderState.cpp#L83)）はマーカーの検出結果に関わらず変更しない（FR-1704、R-1703）。マーカーが1つも見つからない場合は従来どおり`hasProjectMarker = false`のままとなり、呼び出し元（`MFCApplication1Dlg.cpp`）の「対象のプロジェクトが見つかりません」の表示ロジックには触れない（FR-1705）。T002 に依存。コメントに`FR-1704`・`FR-1705`を付す — **結果: 実装済み**

### User Story 1 の検証

- [X] T004 [US1] [quickstart.md](./quickstart.md) 手順1の**ビルド**を実施する。Debug/Release × x64/x86 の8行すべてで警告=0 エラー=0。T003 に依存 — **結果: 合格**（4構成×2（実質8行相当）すべて警告0 エラー0）
- [X] T005 [US1] [quickstart.md](./quickstart.md) 手順2の **ST-1701**（FR-1701〜1705がいずれも1件以上出現、コメント行を除外しない）を実施する。T004 に依存 — **結果: 合格**（FR-1701=2, FR-1702=1, FR-1703=2, FR-1704=1, FR-1705=1）
- [X] T006 [US1] [quickstart.md](./quickstart.md) 手順2の **ST-1702**（変更が`TargetFolderState.cpp`1ファイルに限定され、`.h`・`Dlg.cpp`・`SetupRunner.cpp`・`ProcessRunner.cpp`に差分がないこと）を実施する。T004 に依存 — **結果: 合格**（他4ファイルの差分は空。`TargetFolderState.cpp`のみ+47/-7行の差分）
- [X] T007 [US1] [quickstart.md](./quickstart.md) 手順3の **DT-1701** のうち、`case_nested`（1階層下のサブフォルダに`.sln`）・`case_direct`（直下に`.vcxproj`、回帰確認）・`case_deep`（3階層下に`package.json`）の3ケースを実施し、いずれも`hasProjectMarker = true`となることを確認する（FR-1701、SC-1701、Acceptance Scenario 1・2）。スタンドアロンハーネス（`TargetFolderState.cpp`を`#include`する検証用`.cpp`、009 DT-903と同じ手法）を新規に作成する。T004 に依存 — **結果: 合格**（`case_nested=1`, `case_direct=1`, `case_deep=1`。ハーネスは`vcvars64.bat`＋`cl.exe /MD /D_UNICODE /DUNICODE /D_AFXDLL`で`TargetFolderState.cpp`・`ProcessRunner.cpp`を直接コンパイル・リンクする方式（008 DT-802と同じ手法）で作成し、スクラッチパッドのみに配置、リポジトリには追加していない）
- [ ] T008 [US1] [quickstart.md](./quickstart.md) 手順4の **DT-1702** のうち、`test01`型のネスト構成（親フォルダ直下にはプロジェクトが無く、`test01\MFCApplication1\`にある想定）での確認を実施する。あわせて`.gitignore`・`.specify`・`.claude`・`.git`が対象フォルダ（`test01`）直下に生成され、マーカーのあったサブフォルダには生成されないことを確認する（FR-1704）。フォルダ選択ダイアログの自動操作ができないため実機推奨（必須ではない）。T007 に依存 — **未実施**: CLAUDE.md記載の既知の制限（`CFolderPickerDialog::DoModal()`は合成入力を受け付けずUI自動化できない）により、このセッションでは実行できない。加えてこの開発機には`C:\work\test\test01`がまだ用意されていない。T007のスタンドアロンハーネス（マーカー検出ロジックそのもの）とST-1701/1702（コード上の裏付け）で本フィーチャーの正しさは確認済みであり、本タスクは利用者による任意の実機確認として`[ ]`のまま残す

**チェックポイント**: この時点で、User Story 1 は独立して完全に機能し、テスト可能であるはずです

---

## Phase 4: User Story 2 - 無関係なフォルダは引き続き未検出として扱われる (Priority: P2)

**目標**: サブフォルダの探索を追加した後も、プロジェクトを一切含まないフォルダや`.git`内部にしかマーカーが無いフォルダは、誤って「検出された」と判定されないことを保証する

**独立したテスト**: T007のハーネスへ`case_empty`（マーカーなし）・`case_git_only`（`.git`内部にのみマーカー類似ファイル）を渡し、いずれも`hasProjectMarker = false`となることを確認する（US1の実装コードに対する独立した検証観点であり、US1の検証タスクの完了を待たずに実施できる）

### User Story 2 の実装

本ストーリーが必要とする「誤検出しない」という性質（マーカーが無ければ`false`のまま、`.git`内部は探索しない）は、T002・T003の設計に最初から組み込まれている（[research.md](./research.md) R-1702）。したがって本ストーリー専用の追加実装タスクはない。以下は検証タスクのみ。

### User Story 2 の検証

- [X] T009 [US2] [quickstart.md](./quickstart.md) 手順3の **DT-1701** のうち、`case_empty`（マーカーなし）・`case_git_only`（`.git`内部にのみ`.sln`類似ファイル）の2ケースを実施し、いずれも`hasProjectMarker = false`となることを確認する（FR-1703、FR-1705、SC-1702、Edge Cases、Acceptance Scenario）。T007で作成したハーネスを再利用する。T003 に依存 — **結果: 合格**（`case_empty=0`, `case_git_only=0`）
- [ ] T010 [US2] [quickstart.md](./quickstart.md) 手順4の **DT-1702** のうち、`empty01`（プロジェクトを一切含まない空フォルダ）での回帰確認を実施する。「対象のプロジェクトが見つかりません」で従来どおり中止されることを確認する（SC-1702）。実機推奨（必須ではない）。T008 に依存 — **未実施**: T008と同じ理由（フォルダ選択ダイアログの自動操作不可、`empty01`未整備）による。T009のスタンドアロンハーネス（`case_empty`）で同じ判定ロジックの正しさは確認済みであり、本タスクは利用者による任意の実機確認として`[ ]`のまま残す

**チェックポイント**: ここまでで US1・US2 の両方が独立して機能する。サブフォルダの検出と誤検出防止の両方が実測で確認できる

---

## Phase 5: Polish & Cross-Cutting Concerns（仕上げと横断的関心事）

- [X] T011 [P] [quickstart.md](./quickstart.md) 付録の**コード分析**（`RunCodeAnalysis=true` / `EnablePREfast=true`）を実施し、指摘が0件であること（憲章の技術的制約）。T009・T010 に依存 — **結果: 合格**（出力0件）
- [X] T012 T007・T009が合格したら、`CLAUDE.md` の「設計上、壊しやすい箇所」節に本フィーチャーで得た知見を追記する — マーカー検出（002 FR-103）は実装上は対象フォルダの直下しか見ておらず、Visual Studioウィザードが作る「プロジェクト名と同じサブフォルダ」構成では検出漏れが起きること、017でサブフォルダを再帰的に探索する（`.git`内部のみ除外）ように拡張したこと、マーカーの発見場所に関わらず対象フォルダ自体はすり替えないこと。あわせて「既存機能」の表に017を追加する。**T007またはT009が不合格の場合はこのタスクを実施しない**。T011 に依存 — **結果: 済**（「設計上、壊しやすい箇所」に1項目追加、既存機能の表に017を追加）

---

## Dependencies & Execution Order（依存関係と実行順序）

### Phase Dependencies（フェーズの依存関係）

- **Setup (Phase 1)**: 依存なし。**T001 は実装を始める前に必ず実施する**
- **Foundational (Phase 2)**: タスクなし（該当なし）
- **User Story 1 (Phase 3)**: T001 の後
- **User Story 2 (Phase 4)**: US1の実装（T002・T003）に依存する（同じコードを異なる観点で検証するため、US1のコードが存在しないとUS2のテストが書けない）。ただしUS2の検証タスク（T009）はUS1の検証タスク（T004〜T008）の完了は待たずに並行して進められる
- **Polish (Phase 5)**: US1・US2 の両方の完了後

### User Story Dependencies（ユーザーストーリーの依存関係）

- **User Story 1 (P1)**: Setup の後に開始可能。他のストーリーへの依存なし
- **User Story 2 (P2)**: US1の実装タスク（T002・T003）の完了に依存する。検証タスク（T009）はT003完了後、US1の検証（T004〜T008）と並行して進めてよい

### 重要な順序

| タスク | 依存 | 理由 |
|---|---|---|
| T002→T003（`TargetFolderState.cpp` の変更順） | — | `Read()`内の呼び出しがT002のヘルパーを使うため、ヘルパーを先に実装する必要がある |
| T007（DT-1701の検出ケース） | T004 | ビルドが通ってからハーネスを作成する（ハーネスは`TargetFolderState.cpp`を`#include`するため、本体のビルドが壊れていないことを先に確認する） |
| T009（DT-1701の未検出ケース） | T003のみ | US1の検証（ST-1701等）の完了を待たずに、T003の実装が終わっていれば独立して実施できる |
| T012（CLAUDE.md更新） | T007・T009の合格 | 効果が確認できていない知見を書き残さない |

### Parallel Opportunities（並行実行の機会）

- T002・T003 は同一ファイルの同一関数群のため逐次実行が必要（並行不可）
- T005・T006（ST-1701・1702）は互いに独立して並行実行可能
- T009（US2のDT-1701未検出ケース）はT003完了後、T004〜T008（US1の検証）と並行して進めてよい

---

## Implementation Strategy（実装戦略）

### MVP First（User Story 1 のみ）

1. Phase 1: T001（ベースライン取得）
2. Phase 3: T002〜T008（サブフォルダ探索の実装と検証）
3. **停止して検証**: プロジェクトの親フォルダを選んでもSDD化できることを確認する

US1だけでも実機で確認された問題（スクリーンショットの「対象のプロジェクトが見つかりません」誤判定）は解消するが、US2の検証（誤検出しないことの実測確認）まで行うことで、サブフォルダ探索の拡大が既存の空フォルダ判定を壊していないことを裏付けられる。

### Incremental Delivery（段階的な提供）

1. Setup → ベースライン取得
2. User Story 1 → サブフォルダ探索の実装（**MVP**）
3. User Story 2 → 誤検出しないことの検証（実装はUS1に内包済み）
4. Phase 5 → コード分析、知見の記録

### 実装順序に関する注意

- **T002で`.git`の除外を忘れない。** サブディレクトリの列挙自体は`.git`も含めて行われるが、その内部へは再帰しないこと。ここを忘れると、Git管理下のフォルダで誤検出が発生しうる（FR-1703に反する重大な回帰）
- **T003で`state.path`の代入順序を変えない。** マーカーの発見場所に関わらず対象フォルダをすり替えないこと（FR-1704）。ここを誤ると、後続の`.gitignore`配置やGit初期化が意図しないサブフォルダに対して行われる
- **T007のハーネスは`.cpp`・`.exe`ともにスクラッチパッドに置き、リポジトリには追加しない**（憲章の技術的制約）

---

## Notes（注記）

- [P] タスク = 異なるファイル、依存関係なし
- [Story] ラベルは、トレーサビリティのためにタスクを特定のユーザーストーリーに対応付ける
- 検証タスク（T004〜T010）はコードを書くタスクではなく、[quickstart.md](./quickstart.md) の手順を実施して結果を確認するタスクである
- 検証用の `.cpp`・`.exe` はスクラッチパッドに置き、**リポジトリには追加しない**（憲章の技術的制約）
- T008・T010（DT-1702）は環境依存のため必須ではないが推奨。T007・T009（DT-1701）は本フィーチャーの根本価値（検出できる）と安全性（誤検出しない）を検証する手段であり必須
