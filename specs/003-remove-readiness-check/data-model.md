# Phase 1 データモデル: 準備状況の診断機能の廃止

**Feature**: 003-remove-readiness-check
**Date**: 2026-09-08
**前提**: [001 の data-model.md](../001-sdd-project-bootstrap/data-model.md) と [002 の data-model.md](../002-remove-kind-selection/data-model.md) を基準とし、本書はその**差分のみ**を定義する。記載のない構造（`SetupStep`・`StepResult`・`GitState`・表示行）は変更しない。

---

## 1. 削除するエンティティ: ReadinessItem / ReadinessVerdict

001 の「4. ReadinessItem」を**完全に削除**する。

| 削除する要素 | 001 での役割 |
|---|---|
| `ReadinessVerdict::Met` / `NotMet` / `Indeterminate` | 7項目の判定結果の3状態 |
| `ReadinessItem`（`id`・`displayName`・`verdict`・`nextAction`） | 判定結果1件を表す構造 |
| `ReadinessChecker::Check()` | 7項目の判定処理 |
| `ReadinessChecker::VerdictDisplayName()` | 判定結果の表示文字列 |

ファイル `ReadinessChecker.h` / `ReadinessChecker.cpp` をファイルごと削除する（FR-202、FR-204 / SC-204）。

**「判定不能」という状態も消える。** 001 の FR-021 は「機械的に断定できない項目を未達と区別して表示する」ことを求めていたが、判定そのものが無くなるため区別の必要もなくなる。

---

## 2. 変更するエンティティ: TargetFolderState

001・002 で定義した構造から、**診断のためだけに存在していたフィールドを1つ削除**する。それ以外は変更しない。

| フィールド | 002 まで | 003 | 理由 |
|---|---|---|---|
| `path` | あり | 変更なし | |
| `hasProjectMarker` | あり | 変更なし | 検証チェーン3段目が使う |
| `hasSpecifyDir` | あり | 変更なし | `SetupRunner` 手順4のスキップ判定と配置確認、`HasPartialScaffold()` |
| `hasClaudeDir` | あり | 変更なし | 同上 |
| `hasGitignore` | あり | **削除** | **読み出しが `ReadinessChecker` にしか無く、診断削除により書き込みのみの孤立フィールドになる**（[research.md](./research.md) R-202） |
| `gitState` | あり | 変更なし | 検証チェーン5段目、`SetupRunner` 手順5 |
| `gitRoot` | あり | 変更なし | 入れ子リポジトリの警告メッセージ |
| `commitCount` | あり | 変更なし | `SetupRunner` 手順5のスキップ判定と未コミット変更の検出条件 |

メソッド `IsSelected()` と `HasPartialScaffold()` は**いずれも変更しない**。`HasPartialScaffold()` は診断ではなく検証チェーン4段目（足場が中途半端なときの警告）が使っている。

**`.gitignore` の存在判定への影響はない。** `GitignoreWriter::Ensure()` は自分で `GetFileAttributes` を呼んで判定しており、`hasGitignore` を参照していない。

### `TargetFolderState::Read()` の変更

`.gitignore` の存在を調べて `hasGitignore` へ代入する処理を削除する。ディスクアクセスが1回減る。

---

## 3. 削除する関数

| 関数 | 所在 | 呼び出し元 |
|---|---|---|
| `OnBnClickedButtonDiagnose()` | `MFCApplication1Dlg` | 診断ボタンのメッセージマップ |
| `ShowReadinessGuide()` | `MFCApplication1Dlg` | `OnBnClickedButtonDiagnose()` のみ |

**`ShowCompletionGuide()` は削除しない。** 名前が似ており宣言も隣接しているが、こちらは `OnSetupCompletedMessage()` から呼ばれ、診断結果を参照しない。SDD化の後に次の手順を伝える唯一の手段であり、FR-205 が明示的に存続を要求している。

`AppendResultRow()`・`ClearResultList()`・`InitResultList()`・`SetGuideText()` はいずれも手順結果の表示でも使うため削除しない。

---

## 4. 削除するコントロール

| コントロールID | 種類 |
|---|---|
| `IDC_BUTTON_DIAGNOSE`（1005） | プッシュボタン「準備状況を診断」 |

削除した ID は再利用しない（001・002 と同じ方針）。

---

## 5. 変更しないもの（FR-207）

以下は本フィーチャーで一切変更しない。回帰テスト DT-203〜205・DT-207・DT-208 が確認する。

- `SetupStep` の5手順、その依存関係、`StepResult` の5状態
- 検証チェーンの5段階と、その順序
- `GitState` の3状態と入れ子リポジトリの判定
- 書き込み権限の確認
- 追記ブロックのべき等な適用、統合 `.gitignore` の生成（002 FR-106〜106b）
- 未コミット変更を巻き込まないステージング（001 FR-013a）
- 実行中のコントロール無効化と、完了時に必ず有効へ戻す契約。**ただし対象は3つから2つ（生成・参照）へ減る**
- 完了案内の内容、読みやすさ、コピー可能性（002 FR-110〜112）

---

## エンティティ間の関係（変更後）

```
TargetFolderState ──→ SetupStep[5]   （導入処理）
                      ✂ ReadinessItem[7] は削除
```

001 では `TargetFolderState` が導入処理と診断の**双方**が参照する唯一の事実源だったが、003 では参照元が導入処理だけになる。「双方が参照する」という設計上の位置づけは失われるが、構造そのものを変える必要はない。
