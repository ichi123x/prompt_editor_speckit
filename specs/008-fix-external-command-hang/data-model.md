# Phase 1 Data Model: 外部コマンドの無応答ハング修正

**Feature**: [spec.md](./spec.md) | **Date**: 2026-09-13

本フィーチャーは新しい永続データエンティティを導入しない（`TargetFolderState` 等の既存構造には触れない）。
以下は `ProcessRunner::Run()` 内でのみ生きる実行時の状態であり、関数呼び出しを跨いで保持されない
（[research.md](./research.md) R-802・R-804）。

## 実行時状態（`ProcessRunner::Run()` のローカルスコープ内のみ）

| 項目 | 内容 | 生存期間 |
|---|---|---|
| ジョブオブジェクトハンドル (`HANDLE hJob`) | `CreateJobObject` で作成し、`JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` を設定。子プロセスをタイムアウト時にツリーごと終了させるための入れ物 | `Run()` の呼び出し中のみ。関数を抜ける際に `CloseHandle` する（`KILL_ON_JOB_CLOSE` により、その時点で生きている割り当て済みプロセスがあれば併せて終了する） |
| 監視スレッドハンドル (`HANDLE hWatcherThread`) | `WaitForSingleObject(pi.hProcess, timeoutMs)` を行い、タイムアウト時に `TerminateJobObject` を呼ぶだけの単純なスレッド | 子プロセスの終了（正常/タイムアウトいずれか早い方）とともに終了。主スレッドは読み出しループの後に必ず `WaitForSingleObject` でこのスレッドの終了を待ってから `CloseHandle` する |
| 既存の `PROCESS_INFORMATION` / パイプハンドル | 変更なし（既存実装のまま） | 既存実装のまま |

## 状態遷移

```text
CreateProcess(CREATE_SUSPENDED)
        │
        ▼
AssignProcessToJobObject(hJob, pi.hProcess)
        │
        ▼
ResumeThread(pi.hThread)  ──────────────┐
        │                                │
        ▼                                ▼
  [主スレッド]                     [監視スレッド]
  パイプを読み出しループ            WaitForSingleObject(hProcess, timeoutMs)
        │                                │
        │                        ┌───────┴───────┐
        │                        │               │
        │                   WAIT_OBJECT_0    WAIT_TIMEOUT
        │                   (正常終了)        (無応答)
        │                        │               │
        │                        │       TerminateJobObject(hJob, WAIT_TIMEOUT)
        │                        │               │
        │                        └───────┬───────┘
        │                                │
        ▼                                ▼
  ReadFile が ERROR_BROKEN_PIPE で終了  監視スレッド終了
        │
        ▼
  WaitForSingleObject(hProcess) → GetExitCodeProcess
        │
        ▼
  WaitForSingleObject(hWatcherThread) → CloseHandle 群（hWatcherThread, hJob, pi.*, パイプ）
```

正常終了・タイムアウトのどちらの経路でも、最終的に主スレッドが両方のハンドル（ジョブ・監視スレッド）を
確実に解放してから `Run()` を返す。新規に導入する状態はいずれも「1回の `Run()` 呼び出し限り」であり、
関数間で共有される状態や設定ファイルは増えない。
