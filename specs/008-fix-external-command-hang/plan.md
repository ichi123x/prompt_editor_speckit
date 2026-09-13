# Implementation Plan: 外部コマンドの無応答ハング修正

**Branch**: `008-fix-external-command-hang` | **Date**: 2026-09-13 | **Spec**: [spec.md](./spec.md)

**Input**: `/specs/008-fix-external-command-hang/spec.md` からの機能仕様

## Summary（概要）

uv導入手順の `winget install --id=astral-sh.uv -e` が、初回実行時に winget が出す `msstore` ソースの利用規約同意プロンプトで無期限にハングする不具合（実機で確認済み）を修正する。あわせて、その根本原因である `ProcessRunner::Run`（[ProcessRunner.cpp](../../MFCApplication1/ProcessRunner.cpp)）の設計上の欠陥 — `ReadFile` が子プロセスからの出力を待って無条件にブロックするため、子プロセスが出力を止めて入力待ちになると、コード上の `timeoutMs` チェックが一度も評価されず実質的に無効化される — を修正し、`ProcessRunner` を経由する全ての外部コマンド呼び出し（uv・specify・code・git）に共通して効く形にする。

技術的アプローチは2段構え。(1) `winget install` に `--accept-source-agreements --accept-package-agreements --source winget` を追加し、対話プロンプトの発生源そのものを断つ対症療法（FR-801）。(2) `ProcessRunner::Run` に、子プロセスをジョブオブジェクトへ割り当てた上で、タイムアウトを監視する別スレッドを追加し、超過時に `TerminateJobObject` でプロセスツリーごと確実に打ち切る根本対応（FR-802〜804、FR-806）。(1) だけでは今回確認できた事例にしか効かず、(2) がなければ将来別の未知のハングを防げないため、両方を実施する。

## Technical Context（技術コンテキスト）

**Language/Version**: C++（MSVC v143、Unicode、C++17）

**Primary Dependencies**: Win32 API のみ（`kernel32.lib`）。ジョブオブジェクト（`CreateJobObject` / `AssignProcessToJobObject` / `SetInformationJobObject` / `TerminateJobObject`）とスレッド（`CreateThread` / `WaitForSingleObject`）を新規に使用するが、いずれも既存の `windows.h` 経由で利用可能であり、新しいライブラリ・NuGetパッケージの追加は不要

**Storage**: N/A（本フィーチャーは何も永続化しない）

**Testing**: 自動テストスイートは存在しない（憲章の技術的制約）。静的テスト ST-801〜803 と動的テスト DT-801〜802 を [quickstart.md](./quickstart.md) の手順として定義し実施する

**Target Platform**: Windows 10/11（winget = App Installer が導入済みの環境）。ジョブオブジェクトはWindows 10以降で常時利用可能なため追加の環境要件は生じない

**Project Type**: desktop-app（MFC ダイアログアプリケーション）

**Performance Goals**: N/A。監視スレッドは各 `ProcessRunner::Run` 呼び出しにつき1つ生成され、子プロセスの終了と同時に速やかに終了する。体感できる追加コストはない

**Constraints**: 既存のタイムアウト値（各手順ごとの既定値）は変更しない（spec.md Assumptions）。`ProcessRunner::Run` のシグネチャ（引数・戻り値）は変更しない — 呼び出し元（`SetupRunner.cpp` の5手順すべて）への影響をゼロにする。新しいクラス・ヘッダファイルは追加せず、`ProcessRunner.cpp` 内部の実装のみで完結させる（原則III YAGNI）。新しい `.ps1` は追加しない（憲章の技術的制約）

**Scale/Scope**: 変更するファイルは `SetupRunner.cpp`（winget呼び出しの1行）と `ProcessRunner.cpp`（`Run` 関数の内部実装）の2ファイルのみ。`ProcessRunner.h` のインターフェースは変更しない

## Constitution Check（憲法チェック）

*ゲート: Phase 0 のリサーチ前に通過する必要がある。Phase 1 の設計後に再チェックすること。*

| 原則 | 判定 | 根拠 |
|---|---|---|
| **I. Spec Kit ワークフロー厳守** | 合格 | `/speckit-specify` → `/speckit-plan` の順で進めている。実装は `/speckit-tasks`・`/speckit-analyze` の後 |
| **II. 日本語での一貫性** | 合格 | 本フィーチャーは新規UI文字列を追加しない（既存の失敗理由表示の仕組みをそのまま使う）。コメントと成果物はすべて日本語 |
| **III. シンプルさ優先（YAGNI）** | 条件付き合格（要根拠、[research.md](./research.md) R-802 参照） | 監視スレッド＋ジョブオブジェクトという新しい仕組みを `ProcessRunner.cpp` 内部に追加するが、新しいクラス・公開インターフェース・状態は増やさない。既存の「タイムアウトで打ち切る」という契約を**実際に守れるようにする**ための最小限の実装であり、抽象化の追加ではなく既存契約の是正と位置づける |
| **IV. 変更の追跡可能性** | 合格 | 追加・変更する各箇所に spec.md の FR-801〜806 に対応するコメントを付す。ST-802 がコメントの存在を機械的に検査する |

**技術的制約との整合**:

- MSBuild のみを使用し、CMake 等は導入しない — 合格（変更なし）
- 新しい `.ps1` をプロジェクトへ追加しない — 合格
- テストランナーを新規導入しない — 合格（ST/DT の手順として定義）

**設計後の再チェック（Phase 1 完了時点）**: Phase 1 の成果物（[data-model.md](./data-model.md)・[contracts/process-runner-contract.md](./contracts/process-runner-contract.md)・[quickstart.md](./quickstart.md)）を作成した後も、上記の判定は変わらない。**Complexity Tracking セクションに1件のみ記入する（監視スレッド＋ジョブオブジェクトの追加について）。**

## Project Structure（プロジェクト構造）

### Documentation（このフィーチャー）

```text
specs/008-fix-external-command-hang/
├── plan.md              # このファイル
├── research.md          # Phase 0 の出力（R-801〜R-804）
├── data-model.md        # Phase 1 の出力（新規永続エンティティなし。実行時状態を記述）
├── quickstart.md        # Phase 1 の出力（ST-801〜803、DT-801〜802）
├── contracts/
│   └── process-runner-contract.md  # Phase 1 の出力（ProcessRunner::Run の振る舞い契約）
├── checklists/
│   └── requirements.md  # spec 品質チェックリスト（全項目合格）
└── tasks.md             # Phase 2 の出力（/speckit-tasks が作成。本コマンドでは作らない）
```

### Source Code（リポジトリのルート）

```text
MFCApplication1/
├── SetupRunner.cpp          # 変更するファイル
│                            #   - RunInstallUv 内の winget install コマンドラインに
│                            #     --accept-source-agreements --accept-package-agreements
│                            #     --source winget を追加 (FR-801)
├── ProcessRunner.cpp        # 変更するファイル
│                            #   - Run() 内でジョブオブジェクトを作成し、CREATE_SUSPENDED で
│                            #     起動した子プロセスを ResumeThread 前に割り当てる (FR-804)
│                            #   - タイムアウト監視スレッドを追加し、超過時に
│                            #     TerminateJobObject でプロセスツリーごと打ち切る (FR-802, FR-804)
│                            #   - 監視スレッドの終了を待ってからハンドルを閉じる
├── ProcessRunner.h          # 変更しない（Run() のシグネチャは維持）
├── MFCApplication1Dlg.*     # 変更しない
├── TargetFolderState.*      # 変更しない
├── GitignoreWriter.*        # 変更しない
└── res/gitignore_all.txt    # 変更しない
```

**Structure Decision**: 既存の平坦な構成をそのまま維持する。`ProcessRunner` は既にこのアプリの「外部コマンド実行」を一手に引き受ける唯一の窓口であり、修正はその内部実装に閉じる。新しいファイル・新しいクラスは作らない（[research.md](./research.md) R-802）。

## Complexity Tracking（複雑さの追跡）

> **Constitution Check に違反があり、正当化が必要な場合にのみ記入すること**

| Violation（違反） | Why Needed（必要な理由） | Simpler Alternative Rejected Because（よりシンプルな代替案を却下した理由） |
|-----------|------------|-------------------------------------|
| `ProcessRunner::Run` 内に監視スレッド＋ジョブオブジェクトを追加する | `ReadFile` が子プロセスの出力を待って無条件にブロックするため、子プロセスが入力待ちで停止すると既存の `timeoutMs` チェック（`GetTickCount64` 比較）が一度も評価されない。タイムアウトを実際に機能させるには、パイプの読み出しとは独立にプロセスの生死を監視する主体が必要（[research.md](./research.md) R-802） | オーバーラップI/O（`FILE_FLAG_OVERLAPPED` な名前付きパイプ＋`GetOverlappedResultEx`）も検討したが、匿名パイプ生成部分を名前付きパイプに置き換える書き直しが必要な上、FR-804（プロセスツリーごとの残存プロセスなし）はどのみちジョブオブジェクトが別途必要になる。1つの仕組み（ジョブオブジェクト＋監視スレッド）でFR-802とFR-804の両方を満たせる方を選んだ（[research.md](./research.md) R-802） |
