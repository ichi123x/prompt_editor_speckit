# Phase 1 データモデル: SDDプロジェクトのベース作成ツール

**Feature**: 001-sdd-project-bootstrap
**Date**: 2026-09-08

本フィーチャーは永続ストレージを持たない。ここで定義するのは、ダイアログの生存期間中にメモリ上に保持される状態と、対象フォルダから読み取った事実を表す構造である。

---

## 1. TargetFolderState（対象フォルダの状態）

ユーザーが選択した1つのフォルダについて、ファイルシステムから読み取った事実。導入処理・診断のいずれでも、この構造を1度作ってから判断に使う。

| フィールド | 型 | 意味 | 取得方法 |
|---|---|---|---|
| `Path` | 文字列 | 対象フォルダの完全パス。未選択なら空 | `CFolderPickerDialog` の結果 |
| `IsSelected` | 真偽 | フォルダが選択済みか | `Path` が空でないか |
| `IsWritable` | 真偽 | 書き込み権限があるか | 一時ファイルの作成と削除を試行 |
| `HasSlnMarker` | 真偽 | `*.sln` / `*.vcxproj` が存在するか | フォルダ走査 |
| `HasWebMarker` | 真偽 | `package.json` / `*.csproj` が存在するか | フォルダ走査 |
| `HasSpecifyDir` | 真偽 | `.specify` が存在するか | フォルダ走査 |
| `HasClaudeDir` | 真偽 | `.claude` が存在するか | フォルダ走査 |
| `HasGitignore` | 真偽 | `.gitignore` が存在するか | フォルダ走査 |
| `GitState` | 列挙 | Git の状態（下記） | R-006 のコマンド群 |
| `GitRoot` | 文字列 | リポジトリのルートパス。非リポジトリなら空 | `git rev-parse --show-toplevel` |
| `CommitCount` | 整数 | コミット件数。取得できなければ 0 | `git rev-list --count HEAD` |

### GitState（列挙）

| 値 | 意味 | 判定 |
|---|---|---|
| `NotRepository` | Gitリポジトリではない | `git rev-parse --is-inside-work-tree` が非0終了（実測: 終了コード128） |
| `RepositoryAtTarget` | 対象フォルダ自身がリポジトリのルート | 上記が0終了、かつ `--show-toplevel` が対象フォルダと一致 |
| `NestedUnderParent` | 対象フォルダが既存リポジトリのサブフォルダ | 上記が0終了、かつ `--show-toplevel` が対象フォルダと不一致 |

**バリデーションルール**:
- `GitRoot` と `Path` の比較は、パス区切りを統一し（`git` は `/` を返す）、大文字小文字を区別せず、末尾の区切り文字を除去してから行う。
- `NestedUnderParent` の場合、Git初期化を実行する前にユーザーの確認を得なければならない（FR-016）。

### 状態遷移

`TargetFolderState` は導入処理の前後で再取得する。導入処理は次の遷移を意図する。

```
[未選択]
   │ 参照ボタンでフォルダ選択
   ▼
[選択済み・SDD化前]  HasSpecifyDir=false, HasClaudeDir=false, GitState=NotRepository
   │ 生成ボタン（全手順が成功）
   ▼
[SDD化済み]          HasSpecifyDir=true, HasClaudeDir=true, HasGitignore=true,
                     GitState=RepositoryAtTarget, CommitCount>=1
   │ 生成ボタン再押下
   ▼
[SDD化済み]（変化なし。全手順がスキップ。FR-013 のべき等性）
```

---

## 2. ProjectKind（プロジェクト種別）

ユーザーが選択する対象の種別。2値のみ。

| 値 | 表示名 | 対応マーカー | 埋め込む `.gitignore` |
|---|---|---|---|
| `MfcApp` | MFCアプリ | `*.sln`、`*.vcxproj` | Visual Studio 用 |
| `WebApp` | Webアプリ | `package.json`、`*.csproj` | Node 用 |
| `None` | （未選択） | — | — |

**バリデーションルール**:
- `None` の状態で生成ボタンが押された場合、種別の選択を促して処理を中止する（FR-005、US1-5）。
- 選択された種別に対応するマーカーが `TargetFolderState` に無い場合は、処理を中止して案内する（FR-007、US1-7）。
- 選択された種別のマーカーが無く、**もう一方の種別のマーカーがある**場合は、種別の不一致として警告し、ユーザーの確認を得るまで進まない（FR-008）。

---

## 3. SetupStep（セットアップ手順）

導入処理を構成する5つの工程。実行順に固定で並ぶ。

| フィールド | 型 | 意味 |
|---|---|---|
| `Id` | 列挙 | 手順の識別子（下表） |
| `DisplayName` | 文字列 | 一覧に表示する日本語の手順名 |
| `DependsOn` | 列挙 | 先行して成功している必要がある手順。無い場合は `None` |
| `Result` | 列挙 | 実行結果（下記 `StepResult`） |
| `Reason` | 文字列 | 結果の理由。一覧の3列目に表示する |

### 手順の定義と依存関係

| Id | DisplayName | DependsOn | 完了判定（スキップ条件） |
|---|---|---|---|
| `InstallUv` | uv の導入 | なし | `uv --version` が0終了 |
| `InstallSpecify` | specify CLI の導入 | `InstallUv` | `specify version` が0終了 |
| `InstallVsCodeExt` | VS Code 拡張の導入 | なし | `code --list-extensions` に `anthropic.claude-code` を含む |
| `InitSpecKit` | Spec Kit 足場の配置 | `InstallSpecify` | `.specify` と `.claude` が両方存在 |
| `InitGit` | Git初期化・`.gitignore`・初回コミット | なし | `GitState=RepositoryAtTarget` かつ `CommitCount>=1` かつ `.gitignore` に追記ブロックあり |

**依存関係の設計根拠**（R-005 の発見に基づく）:
- `InstallVsCodeExt` と `InitGit` は他の手順に依存しない。VS Code 拡張の導入に失敗しても Spec Kit の足場配置と Git 初期化は完遂できる。FR-012 が要求する「依存しない手順は続行する」はこの独立性によって実現される。
- `InitSpecKit` が依存するのは `InstallSpecify` のみ。`specify init` はネットワークを必要としない（R-005）ため、ネットワーク不通時でも specify CLI さえ導入済みなら成功する。

### StepResult（列挙）

| 値 | 表示 | 意味 |
|---|---|---|
| `Success` | 成功 | 実行して完了した |
| `Skipped` | スキップ | 既に完了していたため実行しなかった |
| `SkippedUnmetDependency` | スキップ（前提未達） | 依存する先行手順が失敗したため実行しなかった（FR-012） |
| `Failed` | 失敗 | 実行したが完了しなかった。`Reason` に理由を格納 |
| `NotRun` | — | まだ実行していない（初期値） |

---

## 4. ReadinessItem（準備完了チェック項目）

診断が判定する7項目。読み取り専用の判定結果（FR-020）。

| フィールド | 型 | 意味 |
|---|---|---|
| `Id` | 整数 | 1〜7。手順書の準備完了チェックリストの項番に対応 |
| `DisplayName` | 文字列 | 項目名（日本語） |
| `Verdict` | 列挙 | 判定結果（下記 `ReadinessVerdict`） |
| `NextAction` | 文字列 | 未達・判定不能のとき次に取るべき行動。達成なら空 |

### ReadinessVerdict（列挙）

| 値 | 表示 | 意味 |
|---|---|---|
| `Met` | 達成 | 完了条件を満たしている |
| `NotMet` | 未達 | 完了条件を満たしていないと断定できる |
| `Indeterminate` | 判定不能 | 機械的に断定できない。人間の確認が必要（FR-021） |

### 7項目の判定（R-008 に対応）

| Id | DisplayName | 取りうる Verdict | NextAction の例 |
|---|---|---|---|
| 1 | ビルドが通る雛形がある | `Met` / `Indeterminate` | Visual Studio でビルドを一度通してください |
| 2 | Spec Kit の足場が同じ階層にある | `Met` / `NotMet` | 生成ボタンで Spec Kit を導入してください |
| 3 | コミットが1件以上入っている | `Met` / `NotMet` | 生成ボタンで Git 初期化と初回コミットを行ってください |
| 4 | テスト方針が決まっている | `Met` / `Indeterminate` | 何をテスト対象にするかを決め、憲法に記載してください |
| 5 | 憲法がある | `Met` / `NotMet` | Claude Code で `/speckit-constitution` を実行してください |
| 6 | CLAUDE.md が憲法を反映している | `Met` / `NotMet` / `Indeterminate` | Claude Code で `/init` を再実行してください |
| 7 | 技術スタックが記録されている | `Met` / `Indeterminate` | 言語・ビルド構成・テストランナーを CLAUDE.md に明記してください |

**バリデーションルール**:
- 項目5の判定では、憲法ファイルの HTML コメント（`<!--` 〜 `-->`）内部を走査対象から除外する。除外しないと、Sync Impact Report 中に引用されたプレースホルダー名により、制定済みの憲法を `NotMet` と誤判定する（R-008 に実測例を記載）。
- 項目1・4・7 は `NotMet` を取らない。条件を満たさない場合は必ず `Indeterminate` とする（FR-021）。

---

## 5. 表示行（ResultRow）

FR-011 の手順結果と FR-019 の診断結果は、いずれも同一のリストコントロールで表示する（R-009）。両者を共通の3列構造として扱う。

| 列 | 導入処理での内容 | 診断での内容 |
|---|---|---|
| 1列目 | `SetupStep.DisplayName` | `ReadinessItem.DisplayName` |
| 2列目 | `StepResult` の表示文字列 | `ReadinessVerdict` の表示文字列 |
| 3列目 | `SetupStep.Reason` | `ReadinessItem.NextAction` |

---

## エンティティ間の関係

```
TargetFolderState ──┬─→ SetupStep[5]     （導入処理: 状態を読んで各手順のスキップ可否を決める）
        ▲           └─→ ReadinessItem[7] （診断: 状態を読んで7項目を判定する）
        │
   ProjectKind ─────→ マーカー判定、埋め込む .gitignore の選択
```

`TargetFolderState` は導入処理・診断の双方が参照する唯一の事実源である。`ProjectKind` は判定の分岐にのみ関与し、フォルダの状態そのものには影響しない。
