# Quickstart: 高DPI環境でレイアウトが崩れない

**Feature**: 015-support-high-dpi
**Date**: 2026-09-14
**前提**: 006・013までのquickstart.mdの手法（DPI相当フォントによるシミュレーション、`LVM_GETCOLUMNWIDTH`等の値渡しメッセージによる実測）をそのまま踏襲する。

## 前提条件

- ビルド済みの `MFCApplication1.exe`（**測定前に必ず `msbuild /t:Rebuild` を実行し、最新のソースを反映していることを確認する**。古いexeを掴んだまま測定すると無意味になる — 本フィーチャーの調査で実際に発生した）
- 開発機の実DPI（`GetDpiForWindow`で確認。本フィーチャーでは115を前提）
- PowerShell（`System.Drawing`アセンブリのロードに使用）

## ST-1501: `.rc` 寸法の静的確認

1. `MFCApplication1.rc` を開く
2. ダイアログ `IDD_MFCAPPLICATION1_DIALOG` の幅が `570`、高さが `233` であることを確認する
3. [data-model.md](./data-model.md) の表と、8個の各コントロールの座標・幅が一致することを確認する（`IDC_STATIC_TARGET_LABEL`と`IDC_BUTTON_GENERATE`は変更なし）
4. `MFCApplication1Dlg.cpp` の `kColumnRatio` が `{ 22, 17, 61 }` であることを確認する（定義箇所は1箇所のみであること）
5. FR番号のコメント（FR-1503・FR-1505）が該当箇所に付いていることを確認する（憲章 原則IV）

**期待結果**: すべて一致。不一致があれば実装ミス。

## ST-1502: コード分析（既存の静的解析をそのまま実行）

```
msbuild MFCApplication1/MFCApplication1.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Rebuild -p:RunCodeAnalysis=true -p:EnablePREfast=true
```

**期待結果**: 警告・指摘ゼロ（既存基準を維持）。

## ST-1503: ビルド4構成の確認（既存の確認をそのまま実行）

CLAUDE.mdに記載の4構成ビルドスクリプトを実行し、警告・エラーがゼロであることを確認する。

## DT-1501〜DT-1503: DPIスケール別の可読性確認（96/115/120/144 DPI、FR-1501・FR-1502）

各DPIについて、以下の手順で列幅の余裕を検証する。

1. アプリを終了させてから再ビルドする（`msbuild /t:Rebuild`）
2. アプリを起動し、`GetWindowRect`・`GetClientRect`でウィンドウ全体とリストビューのクライアント幅を取得する
3. `LVM_GETCOLUMNWIDTH`で3列（項目・結果・内容）それぞれの実際の列幅を取得する
4. 対象DPI相当のフォントを生成して最長文字列の描画幅を測る:
   ```powershell
   Add-Type -AssemblyName System.Drawing
   $dpi = 115  # 96 / 115 / 120 / 144 に差し替え
   $pxSize = 9.0 * $dpi / 72.0
   $font = New-Object System.Drawing.Font('MS UI Gothic', $pxSize, [System.Drawing.GraphicsUnit]::Pixel)
   $bmp = New-Object System.Drawing.Bitmap 1,1
   $g = [System.Drawing.Graphics]::FromImage($bmp)
   $g.MeasureString('Git初期化はスキップ。既存の未コミット変更を検出したため、.gitignore のみをコミットしました。', $font)
   ```
5. 各列について「実際の列幅 − (最長文字列幅 + 20px + 8px)」を計算し、**正の値**であることを確認する（0px以下は不合格。憲章 原則V）

**期待結果**（[research.md](./research.md) R-1503の検算表と一致すること）:

| DPI | 項目 余裕 | 結果 余裕 | 内容 余裕 |
|---|---|---|---|
| 96 | +6px以上 | +9px以上 | +23px以上 |
| 115（開発機） | +7px以上 | +10px以上 | +11px以上 |
| 120 | +16px以上 | +17px以上 | +36px以上 |
| 144 | +24px以上 | +27px以上 | +49px以上 |

## DT-1504: 1024x768制約の確認（FR-1506、開発機実DPIで判定）

1. 開発機の実DPI（115）で起動する
2. `GetWindowRect`でウィンドウ全体の幅を取得する
3. 1024px以下であることを確認する（期待値: 1016px、余裕8px）。**必ず`GetWindowRect`（非クライアント領域込み）を使うこと。`GetClientRect`で代用すると約18px分過小評価し、制約違反を見逃す（[research.md](./research.md) R-1503）**

## DT-1505: 200%（192 DPI）は既知の制限であることの確認（FR-1502）

1. 192 DPI相当でフォントを生成し、同様に列幅の余裕を計算する（DT-1501の手順を流用）
2. 余裕が負になる可能性があることを確認し、**保証対象外として記録するに留める**（是正は行わない）
3. アプリが実際にクラッシュしないことのみを目視確認する（可能な環境がある場合。無ければ「実機未確認・既知の制限」として記録する）

## DT-1506: マルチモニタでのDPI変更（FR-1504、任意項目）

複数モニタで異なるDPIスケールが設定された環境がある場合のみ実施する。

1. 低DPIモニタでアプリを起動する
2. ウィンドウを高DPIモニタへドラッグして移動する
3. レイアウトの**比率**が崩れていないこと（ビットマップの引き伸ばしによるぼやけは許容）を目視確認する

**開発機は単一モニタ構成のため、本セッションでは未実施。既知の制限として文書化することをもって完了とする**（[research.md](./research.md) R-1506）。

## DT-1507: 既存機能の非破壊確認（FR-1506、回帰確認）

001〜014のquickstart.mdに定義された既存の動作確認手順（5〜6手順の実行、リサイズ追従、タイトルバー色、完了案内・初期案内の文言）を一通り再実施し、いずれも崩れていないことを確認する。特に:

- リサイズ時の8方向の追従が引き続き機能すること（004）
- タイトルバーの色・文字色が変わらないこと（005）
- 完了案内・初期案内の文言が一切変わっていないこと（006）
- `/init`自動化・CLAUDE.md生成が変わらず動作すること（013・014）
