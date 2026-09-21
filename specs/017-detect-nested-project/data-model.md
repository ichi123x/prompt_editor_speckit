# Phase 1 Data Model: サブフォルダ内のウィザードプロジェクトを検出できるようにする

**Feature**: [spec.md](./spec.md) | **Date**: 2026-09-21

本フィーチャーは新しい永続データエンティティ・新しい構造体フィールドを導入しない。既存の
`TargetFolderState`（[TargetFolderState.h](../../MFCApplication1/TargetFolderState.h)）の
`hasProjectMarker`フィールドの**計算方法だけ**が変わる（[research.md](./research.md) R-1701〜R-1704）。

## 既存エンティティ: TargetFolderState（変更なし・計算規則のみ変更）

| フィールド | 型 | 本フィーチャーでの扱い |
|---|---|---|
| `path` | `CString` | 変更なし。利用者が選んだ対象フォルダの完全パス。マーカーがサブフォルダで見つかっても差し替えない（FR-1704、R-1703） |
| `hasProjectMarker` | `bool` | **計算規則を拡張**。「対象フォルダの直下」のみを見る判定から、「対象フォルダ自身、およびその配下のサブフォルダ（`.git`内部を除く）のいずれか」を見る判定へ拡張する |
| その他（`hasSpecifyDir`・`hasClaudeDir`・`gitState`・`gitRoot`・`commitCount`） | — | 変更なし |

## `hasProjectMarker` の新しい計算規則

```text
hasProjectMarker =
    HasProjectMarkerRecursive(state.path)

HasProjectMarkerRecursive(folder):
    もし folder 自身に *.sln / *.vcxproj / *.csproj / package.json のいずれかがあれば true
    そうでなければ、folder 直下の各サブディレクトリ subfolder について（"." "" ".git" を除く）:
        もし HasProjectMarkerRecursive(subfolder) が true なら true を返す（早期リターン）
    どのサブフォルダでも見つからなければ false
```

- 探索は深さ優先・早期リターン（最初にマーカーが見つかった時点で走査を打ち切る）
- `.git`フォルダ自体は「サブディレクトリ」として列挙されるが、その**内部には**再帰しない（R-1702）
- この計算はメモリ上の一時的な走査であり、どこにも保存されない。`Read()`が呼ばれるたびにゼロから
  再計算する（既存の`hasSpecifyDir`等と同じ「読み取り専用・都度再計算」の性質を踏襲）

## 状態

新しい永続状態・新しい構造体・新しいファイルは追加しない。`TargetFolderState`インスタンスのライフタイム・
生成タイミング（`MFCApplication1Dlg`の検証チェーンから`TargetFolder::Read()`を呼んだ時点）も変更しない。
