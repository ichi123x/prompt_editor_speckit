# Phase 1 Data Model: 新規インストール直後のPATH未反映を解消する

**Feature**: [spec.md](./spec.md) | **Date**: 2026-09-13

本フィーチャーは新しい永続データエンティティを導入しない。以下は`ProcessRunner::Run()`が呼ばれるたびに
その場で計算され、どこにも保存されない実行時の合成規則である（[research.md](./research.md) R-901〜904）。

## PATH合成規則（`ProcessRunner::Run()` 呼び出しのたびに再計算）

| 入力 | 取得方法 | 失敗時の扱い |
|---|---|---|
| 現在のプロセスのPATH | `GetEnvironmentVariable(L"PATH", ...)` | 取得できない状況は通常想定しない（プロセスは必ずPATHを持つ） |
| システムPATH | `RegGetValueW(HKEY_LOCAL_MACHINE, "SYSTEM\CurrentControlSet\Control\Session Manager\Environment", "Path", ...)` | 失敗時は空文字列として扱う（R-904） |
| ユーザーPATH | `RegGetValueW(HKEY_CURRENT_USER, "Environment", "Path", ...)` | 失敗時は空文字列として扱う（R-904） |

**合成結果** = 現在のプロセスのPATH ＋ `;` ＋ システムPATH ＋ `;` ＋ ユーザーPATH
（空の入力は連結せずスキップし、先頭・末尾に余計な`;`が残らないようにする）

システムPATH・ユーザーPATHの両方が空だった場合は、合成処理自体を行わず（`SetEnvironmentVariable`を
呼ばず）、プロセスの既存PATHをそのまま維持する。

## 状態

この合成結果は、呼び出しのたびに`SetEnvironmentVariable(L"PATH", 合成結果)`でプロセス全体の環境変数へ
反映される。以降に`CreateProcess`される子プロセス（外部コマンド）は、この更新後のPATHを継承する。
プロセスを跨いで保持される状態はなく、次回の`Run()`呼び出し時に再び同じ手順でゼロから計算し直す。
