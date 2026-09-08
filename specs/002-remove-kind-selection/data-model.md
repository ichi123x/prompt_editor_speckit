# Phase 1 データモデル: プロジェクト種別選択の廃止

**Feature**: 002-remove-kind-selection
**Date**: 2026-09-08
**前提**: [001 の data-model.md](../001-sdd-project-bootstrap/data-model.md) を基準とし、本書はその**差分のみ**を定義する。記載のない構造（`SetupStep`・`StepResult`・`ReadinessItem`・`ReadinessVerdict`・`GitState`・表示行）は 001 のまま変更しない。

---

## 1. 削除するエンティティ: ProjectKind

001 の「2. ProjectKind」を**完全に削除**する。

| 削除する要素 | 001 での役割 | 削除後 |
|---|---|---|
| `ProjectKind::None` | 未選択。生成時に中止の判定に使う | 概念ごと消滅 |
| `ProjectKind::MfcApp` | MFCアプリ。マーカーは `*.sln` / `*.vcxproj` | 概念ごと消滅 |
| `ProjectKind::WebApp` | Webアプリ。マーカーは `package.json` / `*.csproj` | 概念ごと消滅 |
| 種別ごとの `.gitignore` の対応 | 埋め込みリソースの選択 | 統合された単一リソースへ（FR-106） |

種別という概念は本ツールの実装から消える。プロジェクトが MFC なのか Web なのかは `/speckit-constitution` が判定し、憲章の技術的制約として記録される。

---

## 2. 変更するエンティティ: TargetFolderState

001 の「1. TargetFolderState」から、マーカー関連のフィールドとメソッドを変更する。それ以外のフィールド（`path`・`hasSpecifyDir`・`hasClaudeDir`・`hasGitignore`・`gitState`・`gitRoot`・`commitCount`）は変更しない。

### フィールドの変更

| 001 | 002 | 変更内容 |
|---|---|---|
| `hasSlnMarker` （`*.sln` / `*.vcxproj` の有無） | — | **削除** |
| `hasWebMarker` （`package.json` / `*.csproj` の有無） | — | **削除** |
| — | `hasProjectMarker` | **新設**。認識可能なマーカーが1つ以上あるか（種別を区別しない） |

**検出ロジックは変更しない。** 探すファイルの集合（`*.sln`・`*.vcxproj`・`*.csproj`・`package.json`）は 001 と同じで、結果を2つのフラグに分けて保持するのをやめて1つにするだけである。

```
hasProjectMarker = (*.sln または *.vcxproj が存在) または (package.json または *.csproj が存在)
```

つまり 001 における `hasSlnMarker || hasWebMarker` と同値である。

### メソッドの変更

| 001 | 002 | 変更内容 |
|---|---|---|
| `HasMarkerFor(ProjectKind)` | — | **削除**。種別ごとの判定が不要になったため |
| `KindDisplayName(ProjectKind)` | — | **削除**。表示すべき種別名が存在しないため |
| `IsSelected()` | `IsSelected()` | 変更なし |
| `HasPartialScaffold()` | `HasPartialScaffold()` | 変更なし |

### バリデーションルールの変更

| 001 のルール | 002 での扱い |
|---|---|
| `None` の状態で生成ボタンが押されたら種別選択を促して中止 | **廃止**（FR-102） |
| 選択種別に対応するマーカーが無ければ中止して案内 | **変更**。`hasProjectMarker` が false のときに中止して案内（FR-103, FR-104） |
| 選択種別のマーカーが無く他方のマーカーがあれば不一致として警告 | **廃止**（FR-105）。種別を区別しないため不一致が定義できない |

---

## 3. 変更するエンティティ: 埋め込み `.gitignore` リソース

| 001 | 002 |
|---|---|
| `IDR_GITIGNORE_VS` → `res/gitignore_vs.txt` | **統合** |
| `IDR_GITIGNORE_NODE` → `res/gitignore_node.txt` | **統合** |
| — | `IDR_GITIGNORE` → `res/gitignore_all.txt` |

統合ファイルは4つのセクションで構成する（[research.md](./research.md) R-101）。

| # | セクション | パターン数の目安 |
|---|---|---|
| 1 | 共通（Windows のシェル生成物・エディタ一時ファイル・環境変数） | 16 |
| 2 | Visual Studio / .NET | 67 |
| 3 | Node / Web | 37 |
| 4 | 追記ブロック（Spec Kit 用 / プロジェクト固有） | — |

重複していた16パターンはセクション1へ集約し、セクション2・3からは取り除く。実測の重複一覧は research.md R-101 を参照。

**不変条件**: 生成される `.gitignore` の内容は、対象フォルダの状態にかかわらず常に同一でなければならない（FR-106 / SC-105）。この不変条件が破れた場合、種別による分岐が実装に残っていることを意味する。ST-106 と DT-108 が検証する。

---

## 4. 変更する関数シグネチャ

種別を引数として受け渡していた箇所から、その引数を取り除く。

| 001 | 002 |
|---|---|
| `GitignoreWriter::Ensure(folderPath, kind, changed, errorMessage)` | `GitignoreWriter::Ensure(folderPath, changed, errorMessage)` |
| `SetupRunner::RunAll(folderPath, kind, steps, sink)` | `SetupRunner::RunAll(folderPath, steps, sink)` |
| `RunInitGit(step, folderPath, kind, sink)` | `RunInitGit(step, folderPath, sink)` |

ダイアログ側のメンバ `m_kind` と `m_workerKind` も削除する。ワーカースレッドへ渡すのは対象フォルダパスのコピーのみとなる。

---

## 5. 変更しないもの（FR-108）

以下は本フィーチャーで一切変更しない。回帰テスト DT-105〜DT-107 が確認する。

- `SetupStep` の5手順、その依存関係、`StepResult` の5状態
- `ReadinessItem` の7項目、`ReadinessVerdict` の3状態、各項目の判定方法
  - ただし項目1の判定は `hasSlnMarker || hasWebMarker` から `hasProjectMarker` へ置き換わる。両者は同値であり判定結果は変わらない
- `GitState` の3状態と入れ子リポジトリの判定
- 書き込み権限の確認（診断からは呼ばない、という制約を含む）
- 追記ブロックのべき等な適用（目印行による判定）
- 実行中のコントロール無効化と、完了時に必ず有効へ戻す契約
- 未コミット変更を巻き込まないステージング（001 FR-013a）

---

## エンティティ間の関係（変更後）

```
TargetFolderState ──┬─→ SetupStep[5]      （導入処理）
                    └─→ ReadinessItem[7]  （診断）

統合 .gitignore リソース ──→ 常に同一の内容を出力（入力に依存しない）
```

001 では `ProjectKind` が判定の分岐に関与していたが、002 ではその矢印が消える。`TargetFolderState` が唯一の事実源である点は変わらない。
