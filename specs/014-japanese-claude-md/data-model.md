# Phase 1 Data Model: 生成されるCLAUDE.mdの日本語化

**Feature**: [spec.md](./spec.md) | **Date**: 2026-09-13

本フィーチャーは新しいデータエンティティ・状態・構造体を一切導入しない。`StepId`・`SetupStep`・
`StepResult`（013までの成果）に変更はない。変更されるのは`RunGenerateClaudeMd`内の
`ProcessRunner::Run`呼び出しに渡すコマンドライン文字列（`CString`リテラル）1つのみである。

## コマンドライン文字列の変更

| 項目 | 013までの値 | 014での値 |
|---|---|---|
| コマンドライン | `claude --print /init --permission-mode bypassPermissions --output-format text` | `claude --print "/init のCLAUDE.mdは日本語で書いてください。" --permission-mode bypassPermissions --output-format text` |

タイムアウト（15分）・作業ディレクトリ（対象フォルダ）・その他の引数は変更しない。
