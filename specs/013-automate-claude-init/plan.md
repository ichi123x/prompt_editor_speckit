# Implementation Plan: CLAUDE.md生成とコミットの自動化

**Branch**: `013-automate-claude-init` | **Date**: 2026-09-13 | **Spec**: [spec.md](./spec.md)

**Input**: `/specs/013-automate-claude-init/spec.md` からの機能仕様

## Summary（概要）

012では、完了案内に「`/init`を実行してCLAUDE.mdを生成する」「生成されたCLAUDE.mdをコミットする」という2つの**手動**手順を追加した。今回、利用者から「/initを自動実行するように」という明示的な指示を受け、この2手順をアプリ自身が実行する**6番目の導入手順**に置き換える。

実機検証により、Claude Code CLI（`claude.exe`）は次のコマンドで非対話的に`/init`を実行できることを確認済み。

```
claude --print /init --permission-mode bypassPermissions --output-format text
```

対象フォルダをカレントディレクトリとして実行すると、認証済み環境ではプロンプト待ちにならず、CLAUDE.mdが生成される（実測 34〜58 秒）。技術的アプローチは、既存の`SetupRunner.cpp`に手順6「CLAUDE.md の生成とコミット」を追加し、既存の`ProcessRunner`経由でこのコマンドを実行する。成功したらCLAUDE.mdのみを`git add`／`git commit`する。既存5手順の実装（`RunInstallUv`〜`RunInitGit`）・`ProcessRunner`・`Brief()`には一切手を加えない。

完了案内（`ShowCompletionGuide()`）からは、012で追加した2手順の文言を削除し、残りの手順の番号を振り直す。

**寸法変更は不要と見込む**（012とは逆の結論）: 006 FR-507aの見積もり表により、結果一覧は現在5行の内容に対し7行分（158px）の高さの余裕があり、6行目を追加しても`.rc`の変更は不要と見込まれる。また完了案内は5行から3行へ減るため、012で拡張した案内欄の高さ（61 DLU）はそのままで問題ない。[research.md](./research.md) R-1301・R-1302で裏付ける。

## Technical Context（技術コンテキスト）

**Language/Version**: C++（MSVC v143、Unicode、C++17）

**Primary Dependencies**: なし。既存の`ProcessRunner`・`CString`のみを使う

**Storage**: N/A（対象フォルダへのファイル生成とGitコミットのみ、001以来の既存パターン）

**Testing**: 自動テストスイートは存在しない（憲章の技術的制約）。静的テスト ST-1301〜1303 を [quickstart.md](./quickstart.md) の手順として定義する。動的テスト DT-1301（実機でのフルフロー確認、CLAUDE.md生成とコミットまでの確認を含む）は、フォルダ選択ダイアログを伴い008以降と同じ理由で自動化できないため、利用者による任意の手動実行とする

**Target Platform**: Windows 10/11。`claude` CLIは対象マシンに認証済みで導入されていることを前提とする（Assumptions）

**Project Type**: desktop-app（MFC ダイアログアプリケーション）

**Performance Goals**: N/A。ただし`/init`はLLM呼び出しを伴い実行時間が変動する（実測 34〜58 秒、対象リポジトリが大きい場合はさらに長くなりうる）ため、タイムアウトは他手順より長く設定する

**Constraints**: `ProcessRunner`・`TargetFolderState`・`GitignoreWriter`には一切変更を加えない（新しい手順はこれらの既存部品を呼び出すだけ）。既存5手順（`RunInstallUv`〜`RunInitGit`）の実装・順序・依存関係は変更しない。新しいクラス・抽象化は追加しない（原則III YAGNI）。004のリサイズ追従8方針・006の列幅比率は変更しない

**Scale/Scope**: 変更するファイルは`SetupRunner.h`（`StepId`に1件追加）・`SetupRunner.cpp`（新しい手順関数を1つ追加し`CreateSteps`/`RunAll`に組み込む）・`MFCApplication1Dlg.cpp`（`ShowCompletionGuide()`の文言変更）の3ファイル。`.rc`の変更は不要と見込む（research.mdで裏付け後に確定）

## Constitution Check（憲法チェック）

*ゲート: Phase 0 のリサーチ前に通過する必要がある。Phase 1 の設計後に再チェックすること。*

| 原則 | 判定 | 根拠 |
|---|---|---|
| **I. Spec Kit ワークフロー厳守** | 合格 | `/speckit-specify` → `/speckit-plan` の順で進めている。実装は `/speckit-tasks`・`/speckit-analyze` の後 |
| **II. 日本語での一貫性** | 合格 | 追加するコメント・案内文言・結果表示はすべて既存と同じ日本語 |
| **III. シンプルさ優先（YAGNI）** | 合格 | 新しい手順は既存の`SetupStep`/`ProcessRunner`の枠組みにそのまま乗せる。新しい抽象化（インターフェース、ファクトリ等）は導入しない。CLAUDE.mdのコミットは`git add -- CLAUDE.md`という最小限のステージングで済ませ、`RunInitGit`が持つ「他人の変更を検出して分岐する」ロジックを複製しない（対象ファイルが1つに定まっているため不要） |
| **IV. 変更の追跡可能性** | 合格 | 変更箇所に FR-1301〜1312 に対応するコメントを付す。ST-1301〜1303 がコメントの存在とビルド結果を機械的に検査する |

**技術的制約との整合**:

- MSBuild のみを使用し、CMake 等は導入しない — 合格（変更なし）
- 新しい `.ps1` をプロジェクトへ追加しない — 合格
- 外部コマンドは各exeを直接起動し、PowerShellを経由しない — 合格（`claude.exe`を`ProcessRunner`経由で直接起動する）
- テストランナーを新規導入しない — 合格（ST/DT の手順として定義）

**設計後の再チェック（Phase 1 完了時点）**: Phase 1 の成果物（[data-model.md](./data-model.md)・[contracts/claude-init-contract.md](./contracts/claude-init-contract.md)・[quickstart.md](./quickstart.md)）を作成した後も、上記の判定は変わらない。**違反ゼロのため Complexity Tracking セクションは記入しない。**

## Project Structure（プロジェクト構造）

### Documentation（このフィーチャー）

```text
specs/013-automate-claude-init/
├── plan.md              # このファイル
├── research.md          # Phase 0 の出力（R-1301〜R-1304）
├── data-model.md        # Phase 1 の出力（StepId拡張のみ、新規永続エンティティなし）
├── contracts/
│   └── claude-init-contract.md  # 新しい外部コマンド（claude /init）のコントラクト
├── quickstart.md        # Phase 1 の出力（ST-1301〜1303、DT-1301）
├── checklists/
│   └── requirements.md  # spec 品質チェックリスト（全項目合格）
└── tasks.md             # Phase 2 の出力（/speckit-tasks が作成。本コマンドでは作らない）
```

**`contracts/`を作成する理由**: 本フィーチャーは001の`ui-contract.md`「5. 外部コマンドのコントラクト」に載っている表に、新しい外部コマンド（`claude`）の行を1件追加する性質のものである。008〜012とは異なり、既存コマンドの実行方法・タイムアウト・成功判定を新規に定義するため、001の前例に倣い専用のコントラクトファイルを作成する。

### Source Code（リポジトリのルート）

```text
MFCApplication1/
├── SetupRunner.h            # 変更するファイル
│                            #   - StepId に GenerateClaudeMd を追加 (FR-1301)
├── SetupRunner.cpp          # 変更するファイル
│                            #   - RunGenerateClaudeMd() を追加し、claude /init 実行と
│                            #     CLAUDE.md の git add / commit を行う (FR-1303〜1308)
│                            #   - CreateSteps() に6番目の手順として追加、
│                            #     dependsOn = InitGit (FR-1302)
│                            #   - RunAll() の switch に1ケース追加
├── MFCApplication1Dlg.cpp   # 変更するファイル
│                            #   - ShowCompletionGuide() の成功時の案内文字列から、
│                            #     012で追加した2手順の文言を削除し番号を振り直す (FR-1311〜1312)
├── MFCApplication1Dlg.h     # 変更しない
├── ProcessRunner.cpp/.h     # 変更しない（既存のPATH再読み込み・タイムアウト監視をそのまま利用）
├── TargetFolderState.*      # 変更しない
├── GitignoreWriter.*        # 変更しない
├── MFCApplication1.rc       # 変更しない見込み（research.mdで裏付け後に確定。変更が必要と
│                            #   判明した場合はtasks.mdに追加する）
└── res/gitignore_all.txt    # 変更しない
```

**Structure Decision**: 既存の平坦な構成をそのまま維持する。新しい手順は`SetupRunner.cpp`内に既存の5手順と同じ形（`Run<StepName>(SetupStep&, ...)`関数 + `Finish()`呼び出し）で追加するのみで、新しいファイル・新しいクラスは作らない。完了案内の文言変更も012と同じ箇所（`guide.Format(...)`の1呼び出し）に閉じる。

## Complexity Tracking（複雑さの追跡）

違反なし。新しい手順は既存の`SetupStep`/`ProcessRunner`/`Finish`/`FindStep`/`DependencyFailed`という既存の枠組みをそのまま再利用し、新しい抽象化・公開インターフェース・永続状態を一切導入しない。CLAUDE.mdの単体ステージング（`git add -- CLAUDE.md`）は、`RunInitGit`が行う「他人の変更を検出して分岐する」ロジックより単純であり、複雑さを増やしていない。
