# Phase 1 Data Model: 完了案内にCLAUDE.mdのコミット手順を追加する

**Feature**: [spec.md](./spec.md) | **Date**: 2026-09-13

本フィーチャーは新しい永続データエンティティを導入しない。変更対象は`ShowCompletionGuide()`が
組み立てる、UI表示用の1つの`CString`（案内文字列）のみであり、状態や構造を持たない。

## 案内文字列の構成（変更後）

| 行 | 内容 | 変更 |
|---|---|---|
| 見出し | 「Spec Kit の導入が完了しました。次の手順は Claude Code との対話が必要です。」 | 変更なし |
| 手順1 | VS Code で対象フォルダを開く | 変更なし（番号も変わらない） |
| 手順2 | `/init` を実行し CLAUDE.md を生成する | 変更なし（番号も変わらない） |
| 手順3（新規） | 生成された CLAUDE.md をコミットする | **新規追加** |
| 手順4（旧3） | テスト方針を決め `/speckit-constitution` で憲章を制定する | 文言は変更なし、番号のみ3→4 |
| 手順5（旧4） | `/init` を再実行し、憲章を反映した CLAUDE.md に更新する | 文言は変更なし、番号のみ4→5 |

`m_workerPath`が埋め込まれるのは手順1のみであり、この埋め込み方法（`static_cast<LPCTSTR>`）は
変更しない。

## 案内欄（`IDC_STATIC_GUIDE`）の寸法変更（`/speckit-analyze`での指摘を受けて追加、R-1202）

| 項目 | 変更前 | 変更後 |
|---|---|---|
| `IDC_STATIC_GUIDE` の高さ | 52 DLU | **61 DLU** |
| `IDOK` のY座標 | 203 | **212** |
| `IDCANCEL` のY座標 | 203 | **212** |
| ダイアログ全体の高さ | 224 DLU | **233 DLU** |

いずれも`MFCApplication1.rc`内の数値のみの変更であり、004の追従ロジック（`AnchorMode`の割り当て、
`OnSize`/`OnGetMinMaxInfo`の実装）は無修正のまま、初期矩形の変更に自動的に追従する
（[research.md](./research.md) R-1202、006 R-507の前例）。
