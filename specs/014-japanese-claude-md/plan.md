# Implementation Plan: 生成されるCLAUDE.mdの日本語化

**Branch**: `014-japanese-claude-md` | **Date**: 2026-09-13 | **Spec**: [spec.md](./spec.md)

**Input**: `/specs/014-japanese-claude-md/spec.md` からの機能仕様

## Summary（概要）

013で自動化した手順6（`RunGenerateClaudeMd`、`SetupRunner.cpp`）は`claude --print /init --permission-mode bypassPermissions --output-format text`を実行してCLAUDE.mdを生成するが、生成される本文の言語は`claude`自身の判断に委ねていたため、実機では英語になった。

実機検証により、プロンプトに日本語指示を1文加えるだけで、生成されるCLAUDE.mdの本文（定型ヘッダー2行を除く）が正しいUTF-8の日本語になることを確認済み。

```
claude --print "/init のCLAUDE.mdは日本語で書いてください。" --permission-mode bypassPermissions --output-format text
```

技術的アプローチは、`RunGenerateClaudeMd`内のコマンドライン文字列1箇所をこの形に変更するのみ。013が定めたそれ以外のロジック（存在確認によるスキップ、生成確認、`git add -- CLAUDE.md`、コミット、`InitGit`への依存、失敗時の扱い）は一切変更しない。

## Technical Context（技術コンテキスト）

**Language/Version**: C++（MSVC v143、Unicode、C++17）

**Primary Dependencies**: なし（既存の`ProcessRunner::Run`をそのまま使う）

**Storage**: N/A

**Testing**: 自動テストスイートは存在しない（憲章の技術的制約）。静的テスト ST-1401 を [quickstart.md](./quickstart.md) の手順として定義する。動的テスト DT-1401（実際に生成されたCLAUDE.mdが日本語で書かれているかの目視確認）は、生成される自然言語の内容を機械的に判定する手段が無いため（Assumptions）、フォルダ選択を伴う008以降のDTとは別の理由で自動化できず、利用者による任意の手動実行とする

**Target Platform**: Windows 10/11。`claude` CLIが認証済みで導入されていることを前提とする（013から継承）

**Project Type**: desktop-app（MFC ダイアログアプリケーション）

**Performance Goals**: N/A

**Constraints**: `SetupRunner.h`・`ProcessRunner.*`・`MFCApplication1Dlg.cpp`・`MFCApplication1.rc`には一切変更を加えない（FR-1403）。013が実装した`RunGenerateClaudeMd`の構造（`SetupStep&`を受け取り`Finish()`で終える、既存の分岐・エラーメッセージ）は変更せず、コマンドライン文字列のみを書き換える

**Scale/Scope**: 変更するファイルは`SetupRunner.cpp`1つ。`RunGenerateClaudeMd`内の`ProcessRunner::Run(...)`呼び出し1箇所のコマンドライン文字列を書き換える

## Constitution Check（憲法チェック）

*ゲート: Phase 0 のリサーチ前に通過する必要がある。Phase 1 の設計後に再チェックすること。*

| 原則 | 判定 | 根拠 |
|---|---|---|
| **I. Spec Kit ワークフロー厳守** | 合格 | `/speckit-specify` → `/speckit-plan` の順で進めている |
| **II. 日本語での一貫性** | 合格 | 本フィーチャーの目的そのものが、生成物を含めた日本語での一貫性を確保すること |
| **III. シンプルさ優先（YAGNI）** | 合格 | 新しい関数・状態・検証手段を追加せず、既存のコマンドライン文字列1箇所を書き換えるのみ |
| **IV. 変更の追跡可能性** | 合格 | 変更箇所に FR-1401〜1403 に対応するコメントを付す。ST-1401 がコメントの存在とコマンドライン文字列を機械的に検査する |

**設計後の再チェック（Phase 1 完了時点）**: Phase 1 の成果物（[data-model.md](./data-model.md)・[quickstart.md](./quickstart.md)）を作成した後も、上記の判定は変わらない。**違反ゼロのため Complexity Tracking セクションは記入しない。**

## Project Structure（プロジェクト構造）

### Documentation（このフィーチャー）

```text
specs/014-japanese-claude-md/
├── plan.md              # このファイル
├── research.md          # Phase 0 の出力（R-1401）
├── data-model.md         # Phase 1 の出力（変更なしの確認のみ）
├── quickstart.md         # Phase 1 の出力（ST-1401、DT-1401）
├── checklists/
│   └── requirements.md  # spec 品質チェックリスト（全項目合格）
└── tasks.md              # Phase 2 の出力（/speckit-tasks が作成。本コマンドでは作らない）
```

**`contracts/`を作成しない理由**: 本フィーチャーは013の[contracts/claude-init-contract.md](../013-automate-claude-init/contracts/claude-init-contract.md)が定義済みの「CLAUDE.md の生成」コマンドの、コマンドライン文字列という1パラメータだけを変更するものであり、成功判定・作業ディレクトリ・呼び出し順序といったコントラクトの構造自体は変更しない。012・010の前例に倣い、新しいコントラクトファイルは作らず、research.mdに変更後のコマンドラインを記録する。

### Source Code（リポジトリのルート）

```text
MFCApplication1/
├── SetupRunner.cpp          # 変更するファイル
│                            #   - RunGenerateClaudeMd() 内のコマンドライン文字列に
│                            #     日本語指示を追加 (FR-1401)
├── SetupRunner.h            # 変更しない
├── MFCApplication1Dlg.cpp   # 変更しない
├── MFCApplication1.rc       # 変更しない
├── ProcessRunner.cpp/.h     # 変更しない
├── TargetFolderState.*      # 変更しない
└── GitignoreWriter.*        # 変更しない
```

**Structure Decision**: 013で実装した`RunGenerateClaudeMd`関数の構造はそのまま維持し、`ProcessRunner::Run(...)`に渡すコマンドライン文字列リテラル1つだけを書き換える。新しいファイル・新しい関数は作らない。

## Complexity Tracking（複雑さの追跡）

違反なし。既存関数内の文字列リテラル1箇所を書き換えるのみで、新しい抽象化・公開インターフェース・永続状態を一切導入しない。
