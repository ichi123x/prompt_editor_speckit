# Phase 0 Research: 生成されるCLAUDE.mdの日本語化

**Feature**: [spec.md](./spec.md) | **Date**: 2026-09-13

## R-1401: `/init`のプロンプトに日本語指示を加える方法

**Decision**: `/init`の直後に日本語での指示文を続けて1つの引数として渡す。

```
claude --print "/init のCLAUDE.mdは日本語で書いてください。" --permission-mode bypassPermissions --output-format text
```

**Rationale**: `--print`（`-p`）は与えられた引数全体を1つのプロンプトとしてClaude Codeへ渡す。スラッシュコマンド`/init`はメッセージの先頭にあれば認識されるため、その後ろに自然文で指示を続けても問題なく解釈される。

**実機検証**（`C:\work2\test\MFCApplication1`、CLAUDE.mdが存在しない状態、013のコミットを一時的に取り消して実施）:

| 項目 | 結果 |
|---|---|
| 実行コマンド | `claude --print "/init のCLAUDE.mdは日本語で書いてください。" --permission-mode bypassPermissions --output-format text` |
| 経過時間 | 97.2秒 |
| 終了コード | 0 |
| 生成されたCLAUDE.mdの本文 | 見出し（`## プロジェクト概要`等）・説明文がすべて正しいUTF-8の日本語。コード内の識別子（クラス名等）は英語のまま引用されており、これは許容範囲（Assumptions） |
| 定型ヘッダー2行（`# CLAUDE.md`、`This file provides guidance...`） | 英語のまま（`claude`が常に付与する固定ヘッダーであり、日本語指示の有無にかかわらず変化しない） |

なお、`Get-Content`（既定エンコーディング）でこのファイルを読むと文字化けして見えるが、これは
PowerShellの表示側の問題であり、`[System.Text.Encoding]::UTF8`で明示的に読み直すと正しい日本語が
確認できた。ファイル自体のエンコーディングに問題はない。

**Alternatives considered**:

- `--append-system-prompt`でシステムプロンプトに日本語指示を追加する方法: 動作する可能性はあるが、
  `/init`自体は変更せずに済む一方、コマンドラインへ渡す文字列が長くなり、`--print`引数に直接書く
  方法より複雑になる。実機検証済みの単純な方法（プロンプト文字列に直接続ける）を優先した
- 環境変数や設定ファイルで既定の応答言語を指定する方法: `claude --help`にはそのような専用オプションは
  無く、`CLAUDE.md`自体やユーザー設定に依存させると対象フォルダの状態に影響されるため、
  プロンプト文字列で明示する方が確実で単純である（原則III YAGNI）

## R-1402: 013からの変更範囲の確認

**Decision**: `RunGenerateClaudeMd`内の`ProcessRunner::Run`呼び出しのコマンドライン文字列（1つの
`CString`リテラル）のみを変更する。関数の構造・分岐・タイムアウト値（15分）・その後の
`git add -- CLAUDE.md`／コミット処理は一切変更しない。

**Rationale**: 013の実装は既にCLAUDE.mdの生成・確認・コミットという責務を1関数に閉じて実装して
おり、その関数が「何を生成させるか」を決めているのはコマンドライン文字列だけである。日本語化は
生成物の言語という1つの関心事の変更であり、それ以外のロジックには影響しない。
