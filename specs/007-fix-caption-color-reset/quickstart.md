# クイックスタート（検証手順）: フォルダ選択ダイアログ後もタイトルバーの色を保つ

**Feature**: 007-fix-caption-color-reset
**Date**: 2026-09-12

本フィーチャーは、フォルダ選択ダイアログの開閉を伴う。CLAUDE.md および [research.md](./research.md) R-701 のとおり、この種の動的テストは自動化できない。**静的テスト ST-701〜703 は自動化し、動的テスト DT-701 は利用者による手動実行とする。**

---

## 手順0: 準備

アプリが起動したままだとビルドが `LNK1104` で失敗する。

```powershell
Get-Process MFCApplication1 -ErrorAction SilentlyContinue | Stop-Process -Force
```

---

## 手順1: ビルド

4構成すべてで、警告0件・エラー0件であること。

```powershell
$msb='C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe'
foreach($cfg in 'Debug','Release'){ foreach($plat in 'x64','x86'){
  $out = & $msb MFCApplication1\MFCApplication1.sln /p:Configuration=$cfg /p:Platform=$plat /t:Rebuild /v:minimal /nologo 2>&1
  "{0,-8} {1,-5} 警告={2} エラー={3}" -f $cfg,$plat,
    ($out | Select-String ' warning ' | Measure-Object).Count,
    ($out | Select-String ' error '   | Measure-Object).Count
}}
```

**合格条件**: 8行すべてで警告=0 エラー=0。

---

## 手順2: 静的テスト（ST-701〜703）

### ST-701: `ApplyCaptionColor` が定義され、3箇所から呼ばれている

```powershell
$c = Get-Content MFCApplication1\MFCApplication1Dlg.cpp -Raw
"定義（ApplyCaptionColor()\s*\{ 相当）  : " + ([regex]::Matches($c,'void\s+CMFCApplication1Dlg::ApplyCaptionColor\s*\(')).Count
"呼び出し合計                          : " + ([regex]::Matches($c,'ApplyCaptionColor\s*\(\s*\)\s*;')).Count
```

**合格条件**: 定義が1件、呼び出しが**3件**（`OnInitDialog` に1件、`OnBnClickedButtonBrowse` に `DoModal()` の直前・直後で2件）。

呼び出しがそれぞれどの関数の中にあるかも確認する:

```powershell
Select-String -Path MFCApplication1\MFCApplication1Dlg.cpp -Pattern 'ApplyCaptionColor\(\);' -Context 20,0 |
  ForEach-Object {
    $pre = $_.Context.PreContext -join "`n"
    $fn = [regex]::Matches($pre, '(?:BOOL|void)\s+CMFCApplication1Dlg::(\w+)') | Select-Object -Last 1
    "{0,-4} 呼び出し元関数: {1}" -f $_.LineNumber, $fn.Groups[1].Value
  }
```

**合格条件**: 呼び出し元が `OnInitDialog`（1件）と `OnBnClickedButtonBrowse`（2件）の**ちょうど3件**であること。

### ST-702: 再適用が `DoModal()` の直前・直後にあり、早期 return より前にある

```powershell
$c = Get-Content MFCApplication1\MFCApplication1Dlg.cpp -Raw
$mBefore = [regex]::Match($c, 'CFolderPickerDialog\s+dialog[\s\S]{0,300}?ApplyCaptionColor\(\);[\s\S]{0,150}?dialog\.DoModal\(\)')
$mAfter  = [regex]::Match($c, 'dialog\.DoModal\(\)[\s\S]{0,300}?ApplyCaptionColor\(\);[\s\S]{0,200}?(?:dialog\.DoModal\(\)|result)\s*!=\s*IDOK')
"CFolderPickerDialog 構築 → ApplyCaptionColor → DoModal の順になっている: " + $mBefore.Success
"DoModal → ApplyCaptionColor → 早期returnの順になっている            : " + $mAfter.Success
```

**実装メモ**: `DoModal()` を2回呼ぶことは避け、戻り値を一度変数（例: `INT_PTR result`）に受けてから、返った直後の `ApplyCaptionColor()` を呼び、その後 `result != IDOK` で早期 return を判定する形でよい。上記の正規表現は `dialog.DoModal() != IDOK` と `result != IDOK` のどちらの書き方にも一致する。

**合格条件**: 両方とも `True`。

- 1つ目は「ダイアログを開く**直前**の再適用」がある証拠。ダイアログを開いた瞬間に色が崩れる可能性（FR-701）に備える呼び出しが漏れていないかを確認する
- 2つ目は「返った直後の再適用」が、フォルダ未選択・キャンセル時の早期 `return` より**前**にあることの確認（[contracts/ui-contract.md](./contracts/ui-contract.md) 2章）。これが後になっていると、キャンセル時に再適用されず FR-701 のシナリオ3を満たさない

あわせて、戻り値を判定していないこと（005 の方針を継承）を確認する:

```powershell
Select-String -Path MFCApplication1\MFCApplication1Dlg.cpp -Pattern 'DwmSetWindowAttribute' -Context 2,2 |
  ForEach-Object { ($_.Context.PreContext + $_.Line + $_.Context.PostContext) -join "`n"; '---' }
```

**合格条件**: `DwmSetWindowAttribute` の呼び出しが `if` / `SUCCEEDED` / `FAILED` で囲まれていないこと。

**この確認は FR-703（着色できない環境で新たな警告・エラーを出さない）も兼ねる**（[contracts/ui-contract.md](./contracts/ui-contract.md) 3章）。呼び出し箇所が3つに増えても戻り値を判定する分岐が一切無いことが確認できれば、着色できない環境での新たなエラー発生経路が存在しないことを構造的に保証できるため、別途の模擬テストは追加しない。

### ST-703: FR による追跡可能性

```powershell
$c = Get-Content MFCApplication1\MFCApplication1Dlg.cpp -Raw
701..706 | ForEach-Object { $n="FR-$_"; "{0}: {1}" -f $n, ([regex]::Matches($c,$n)).Count }
```

**合格条件**: FR-701〜706 のうち、実装に直接対応するもの（FR-701, FR-702, FR-704）が**必ず**コメントとして現れること。FR-703, FR-705, FR-706 は「何もしない／変更しない」という要件のため、関連コメントに1箇所ずつまとめて記載する。

---

## 手順3: 既存機能の回帰確認（自動、SC-702）

005・006 の静的テストがそのまま通ることを確認する。005 の [quickstart.md](../005-dark-blue-caption/quickstart.md) 手順2（ST-401〜406）を再実行し、`OnInitDialog` 内の着色処理が `ApplyCaptionColor()` 呼び出しに置き換わった点を除き、判定基準が変わらないことを確認する。

```powershell
Select-String -Path MFCApplication1\MFCApplication1Dlg.cpp -Pattern 'DwmSetWindowAttribute|DWMWA_CAPTION_COLOR|DWMWA_TEXT_COLOR|dwmapi' |
  ForEach-Object { "{0,-5} {1}" -f $_.LineNumber, $_.Line.Trim() }
```

**合格条件**: `DwmSetWindowAttribute` が `ApplyCaptionColor()` の定義内にのみ2回（背景・文字）現れ、それ以外の場所には現れないこと。呼び出し元（`ApplyCaptionColor()` を呼ぶ側）が3箇所に増えても、`DwmSetWindowAttribute` 自体は `ApplyCaptionColor()` の定義内の2回のみである。005 の ST-401 と異なり、`OnInitDialog` 自体には `DwmSetWindowAttribute` が直接は現れなくなる（`ApplyCaptionColor()` 経由になるため）。

---

## 手順4: 動的テスト（DT-701、**利用者による手動実行が必須**）

CLAUDE.md のとおり、フォルダ選択ダイアログ（`CFolderPickerDialog`）は合成入力を受け付けないため、この手順は自動化できない。

1. `MFCApplication1\x64\Debug\MFCApplication1.exe` を起動し、タイトルバーが濃紺であることを確認する
2. 「参照...」を押してフォルダ選択ダイアログを開く
3. **フォルダ選択ダイアログを開いたまま**、メインダイアログのタイトルバーが（ダイアログの陰に隠れている部分を除き）濃紺のままであることを確認する（spec.md User Story 1 Acceptance Scenario 1）
4. 適当なフォルダへ移動し、「フォルダーの選択」を押して閉じる
5. **メインダイアログのタイトルバーが濃紺のままであることを確認する**（白に変わっていないこと）
6. 再度「参照...」を押してフォルダ選択ダイアログを開き、今度は「キャンセル」を押して閉じる
7. **タイトルバーが濃紺のままであることを確認する**
8. 手順2〜7を、選択・キャンセルを織り交ぜて**10回以上**繰り返す（SC-701）
9. 毎回、タイトルバーが濃紺のまま変化しないことを確認する

**合格条件**: 10回以上のすべての開閉後、タイトルバーが一度も既定色（白）に変わらないこと（開いている最中を含む）。

**もし手順4で不合格になった場合**: [plan.md](./plan.md)「未解決のリスク」に記載のとおり、今回の修正は対症療法であり、根本原因が別にある可能性がある。その場合は、実際に不具合が再現した操作手順（フォルダの階層をどう移動したか、他のウィンドウとの切り替えを挟んだか等）をできるだけ詳しく記録し、原因の再調査を行う。

---

## 完了条件

- [ ] ST-701〜ST-703 の静的テストがすべて合格
- [ ] 手順3（既存機能の回帰確認）が合格
- [ ] **DT-701 を利用者が実機で手動実行し、合格していること**
- [ ] 手順1のビルドで警告が0件
- [ ] コード分析（`RunCodeAnalysis=true`）の指摘が0件

---

## 付録: コード分析

```powershell
$msb='C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe'
& $msb MFCApplication1\MFCApplication1.vcxproj /p:Configuration=Debug /p:Platform=x64 /t:Rebuild `
  /p:RunCodeAnalysis=true /p:EnablePREfast=true /v:minimal /nologo 2>&1 | Select-String 'warning'
```

**合格条件**: 出力が空であること。
