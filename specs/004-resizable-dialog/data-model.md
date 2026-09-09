# Phase 1 データモデル: ダイアログのリサイズ追従

**Feature**: 004-resizable-dialog
**Date**: 2026-09-09
**前提**: 001〜003 の data-model.md を基準とし、本書はその**差分のみ**を定義する。既存の構造（`TargetFolderState`・`SetupStep`・`StepResult`・`GitState`）は一切変更しない。

本フィーチャーは画面の再配置のみを扱い、SDD化の処理には触れない。

---

## 1. 新設するエンティティ: AnchorMode（追従方針）

コントロールがウィンドウのサイズ変更にどう振る舞うかを表す列挙。5種類。

| 値 | 意味 | 横方向 (dx) | 縦方向 (dy) |
|---|---|---|---|
| `Fixed` | 位置も大きさも変えない | — | — |
| `StretchH` | 左上固定、幅だけ増減 | `w += dx` | — |
| `AnchorRight` | 右端との距離を保って移動 | `x += dx` | — |
| `StretchBoth` | 幅と高さの両方が増減 | `w += dx` | `h += dy` |
| `AnchorBottomStretchH` | 下端との距離を保って移動し、幅は増減 | `w += dx` | `y += dy` |
| `AnchorBottomRight` | 右端・下端との距離を保って移動 | `x += dx` | `y += dy` |

`dx` / `dy` は**初期クライアントサイズからの差分**である（[research.md](./research.md) R-303）。前回サイズからの相対値ではない。この定義により、同じウィンドウサイズには常に同じ配置が対応し、拡大と縮小を繰り返しても位置がずれない（SC-304）。

---

## 2. 新設するエンティティ: AnchorEntry（追従テーブルの1行）

コントロール1つ分の追従定義。

| フィールド | 型 | 意味 |
|---|---|---|
| `id` | 整数 | コントロールID |
| `mode` | `AnchorMode` | 追従方針 |
| `initialRect` | 矩形 | 初期化時に記録した位置と大きさ（ダイアログのクライアント座標） |

`initialRect` は `OnInitDialog` の時点で `GetWindowRect` + `ScreenToClient` により取得し、以後は変更しない。再配置は常にこの初期値と `dx` / `dy` から計算する。

### テーブルの内容（8行）

| # | id | mode | 対応FR |
|---|---|---|---|
| 1 | ラベル「対象フォルダ:」 | `Fixed` | FR-305 |
| 2 | `IDC_EDIT_TARGET_PATH` | `StretchH` | FR-302 |
| 3 | `IDC_BUTTON_BROWSE` | `AnchorRight` | FR-304 |
| 4 | `IDC_BUTTON_GENERATE` | `Fixed` | FR-305 |
| 5 | `IDC_LIST_RESULT` | `StretchBoth` | FR-302, FR-303 |
| 6 | `IDC_STATIC_GUIDE` | `AnchorBottomStretchH` | FR-302, FR-303 |
| 7 | `IDOK` | `AnchorBottomRight` | FR-303, FR-304 |
| 8 | `IDCANCEL` | `AnchorBottomRight` | FR-303, FR-304 |

**不変条件（FR-303 の担保 / ST-303）**: 画面上のすべてのコントロールが、このテーブルにちょうど1行を持たなければならない。定義漏れがあるとそのコントロールだけ元の位置に取り残され、他と重なるかウィンドウの外へはみ出す。

ラベルは `IDC_STATIC`（-1）であり `GetDlgItem` で取得できないため、**専用のコントロールIDを割り当てる**必要がある。詳細は [contracts/ui-contract.md](./contracts/ui-contract.md)「2. ラベルのID」を参照。

---

## 3. 新設する状態: ダイアログのメンバ

| フィールド | 型 | 意味 |
|---|---|---|
| `m_anchors` | `AnchorEntry` の配列 | 追従テーブル。`OnInitDialog` で初期矩形を埋める |
| `m_initialClientSize` | サイズ | 初期のクライアント領域の幅と高さ。`dx` / `dy` の基準 |
| `m_minWindowSize` | サイズ | ウィンドウ全体の最小サイズ。`OnGetMinMaxInfo` が返す |
| `m_layoutReady` | 真偽 | `OnInitDialog` が完了したか。false の間 `OnSize` は何もしない |

**`m_layoutReady` が必要な理由**: `OnSize` はダイアログの初期化中にも呼ばれる。その時点ではコントロールが未生成で `GetDlgItem` が `nullptr` を返すため、フラグで早期リターンしないとアクセス違反になる（[research.md](./research.md) R-301）。

**`m_minWindowSize` はクライアントサイズではなくウィンドウ全体のサイズ**を保持する。`OnGetMinMaxInfo` の `ptMinTrackSize` がウィンドウ全体の寸法を指すため（[research.md](./research.md) R-304）。

---

## 4. 変更するもの: 結果一覧の列幅

`InitResultList()` は列幅を一覧のクライアント幅の **40% / 20% / 40%** で設定している。004 ではリサイズのたびに同じ比率で再計算する（FR-308）。

| | 001〜003 | 004 |
|---|---|---|
| 列幅の決定 | `InitResultList()` で1回だけ | 初期化時と、リサイズのたび |
| 比率 | 40 / 20 / 40 | 変更なし |

**比率は1箇所にまとめて両方から参照する。** 初期化と再配置で別々に持つと、列を追加したり比率を変えたりしたときに片方だけ直す誤りが起きる（[research.md](./research.md) R-305(2)）。

---

## 5. 変更しないもの

以下は本フィーチャーで一切変更しない。回帰テスト DT-310 と SC-305 が確認する。

- `TargetFolderState`・`SetupStep`・`StepResult`・`GitState` の各構造
- 5手順の実行、検証チェーンの5段階とその順序
- 実行中のコントロール無効化（生成・参照の2つ）と、完了時に有効へ戻す契約
- 完了案内の内容、読みやすさ、コピー可能性（002 FR-110〜112）
- `.gitignore` の生成と追記、べき等性
- ワーカースレッドとの通信方式（`PostMessage`）

**ワーカースレッドとの排他制御は不要である。** ワーカーはコントロールを直接操作せず `PostMessage` で結果を送るだけなので、`OnSize` による再配置と競合しない（[research.md](./research.md) R-305(1)）。001 でこの設計を採ったことが、004 で追加の同期を不要にしている。

---

## エンティティ間の関係

```
OnSize（WM_SIZE）
  └→ dx, dy を初期クライアントサイズから算出
       └→ AnchorEntry[8] を走査
            └→ mode に応じて SetWindowPos で再配置
                 └→ IDC_LIST_RESULT なら列幅も再計算

OnGetMinMaxInfo（WM_GETMINMAXINFO）
  └→ m_minWindowSize を ptMinTrackSize に設定
```
