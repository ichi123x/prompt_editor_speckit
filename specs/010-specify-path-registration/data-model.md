# Phase 1 Data Model: specify CLI導入後のPATH未登録を解消する

**Feature**: [spec.md](./spec.md) | **Date**: 2026-09-13

本フィーチャーは新しい永続データエンティティを導入しない。`uv tool update-shell`はレジストリの
ユーザーPATH（`HKCU\Environment\Path`）を書き換えるが、これはuv自身が管理する既存の状態であり、
本アプリはその値の構造やスキーマに関与しない（呼び出すだけで、結果を読み取ったり解釈したりしない、
[research.md](./research.md) R-1003）。

新しいメンバ変数・構造体・状態遷移は発生しない。`RunInstallSpecify`関数のローカルな制御フローに
1行（`ProcessRunner::Run()`の追加呼び出し）が増えるのみである。
