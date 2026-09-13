# Contract: Claude Code CLIによる`/init`実行（手順6）

**Feature**: [../spec.md](../spec.md) | **Date**: 2026-09-13

001の[ui-contract.md](../../001-sdd-project-bootstrap/contracts/ui-contract.md)「5. 外部コマンドのコントラクト」に、
本フィーチャーで追加する行を示す。既存5手順の行は変更しない。

## 追加する外部コマンド

| 手順 | コマンド | 成功判定 | 作業ディレクトリ |
|---|---|---|---|
| CLAUDE.md の生成 | `claude --print /init --permission-mode bypassPermissions --output-format text` | 終了コード 0 **かつ** 対象フォルダ直下にCLAUDE.mdが存在する | **対象フォルダ** |
| CLAUDE.md のステージング | `git add -- CLAUDE.md` | 終了コード 0 | 対象フォルダ |
| CLAUDE.md のコミット | `git commit -m "CLAUDE.md を追加"` | 終了コード 0 | 対象フォルダ |

## 呼び出し順序と前提条件

1. 対象フォルダ直下にCLAUDE.mdが存在するかを確認する（`TargetFolder`相当の単純なファイル存在確認、既存の`TargetFolderState`は`.specify`/`.claude`/Gitの状態しか持たないため、この確認は新しいヘルパーを介さずファイル存在チェックのみで行う。R-1301〜1305、原則III YAGNI）
   - 存在する場合: 手順全体をスキップする（`StepResult::Skipped`、理由「CLAUDE.md は既に生成されています。」）
   - 存在しない場合: 手順2へ進む
2. `claude --print /init --permission-mode bypassPermissions --output-format text` を実行する（タイムアウト15分、R-1305）
   - 起動できない場合（`launched == false`）: `StepResult::Failed`、理由は既存の`CommandNotFoundReason(_T("claude"))`をそのまま使う
   - 終了コードが0以外: `StepResult::Failed`、理由は`Brief(output)`
   - 終了コード0だが、対象フォルダ直下にCLAUDE.mdが確認できない: `StepResult::Failed`、理由「`/init` は完了しましたが、CLAUDE.md を確認できませんでした。」
3. CLAUDE.mdの生成を確認できたら、`git add -- CLAUDE.md`を実行する
   - 失敗した場合: `StepResult::Failed`、理由は`Brief(output)`
4. `git commit -m "CLAUDE.md を追加"`を実行する
   - 失敗した場合: `StepResult::Failed`、理由は`Brief(output)`
   - 成功した場合: `StepResult::Success`、理由「CLAUDE.md を生成し、コミットしました。」

## 既存コマンドとの違い

- `specify init`等と異なり、成功判定に「終了コード0」だけでなく「生成物（CLAUDE.md）の存在確認」を組み合わせる。この二段判定は`RunInitSpecKit`（手順4、`.specify`/`.claude`の存在確認）と同じパターンであり、新しい設計を持ち込まない
- `git add`の対象を`-A`ではなく`-- CLAUDE.md`と明示する点が`RunInitGit`と異なる（R-1304参照）。対象ファイルが1つに定まっているため、`git status`を読んで分岐する処理は不要

## 変更しない既存コマンド

001〜012で定義済みの5手順（uv・specify CLI・VS Code拡張・Spec Kit足場・Git初期化）のコマンドライン・
成功判定・作業ディレクトリは一切変更しない。
