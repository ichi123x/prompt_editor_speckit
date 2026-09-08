# クイックスタート検証ガイド: 準備状況の診断機能の廃止

**Feature**: 003-remove-readiness-check
**Date**: 2026-09-08

本フィーチャーは削除が中心である。検証の重点は「消したものが消えたか」より、**「消し過ぎていないか」**にある。動的テスト8件のうち5件（DT-203〜205、DT-207、DT-208）を回帰確認に充てているのはそのためである。

判定基準は [spec.md](./spec.md) の「テスト方針」、契約は [contracts/ui-contract.md](./contracts/ui-contract.md)、削除範囲の根拠は [research.md](./research.md) を参照する。

---

## 前提条件

| 項目 | 内容 |
|---|---|
| 実行ファイル | `C:\work\claude\prompt_editor_speckit\MFCApplication1\x64\Debug\MFCApplication1.exe` |
| テスト対象 | `C:\work\test\test01`（MFC アプリ） |

**プラットフォーム名の注意**: `.sln` に渡すのは `x86`（`Win32` は `MSB4126` になる）。**ビルド前にアプリを終了させること**（起動したままだと `LNK1104`）。

---

## 手順0: テスト用フォルダの準備

```powershell
$t='C:\work\test\test01'
foreach($x in '.git','.specify','.claude'){ $p=Join-Path $t $x; if(Test-Path $p){ Remove-Item -LiteralPath $p -Recurse -Force } }
$gi=Join-Path $t '.gitignore'; if(Test-Path $gi){ Remove-Item -LiteralPath $gi -Force }
Get-ChildItem $t -Force -Name | Where-Object { $_ -like '.*' -or $_ -like '*.sln' }
```

`.git`・`.gitignore`・`.specify`・`.claude` が**無い**こと。`test01.sln` は**残っている**こと。

**注意**: `Remove-Item` の対象を取り違えるとソリューション本体を失う。実行前にパスを確認すること。

---

## 手順1: ビルド（ST-203）

```powershell
$msb='C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe'
foreach($cfg in 'Debug','Release'){ foreach($plat in 'x64','x86'){
  $out = & $msb MFCApplication1\MFCApplication1.sln /p:Configuration=$cfg /p:Platform=$plat /t:Rebuild /v:minimal /nologo 2>&1
  "{0,-8} {1,-5} 警告={2} エラー={3}" -f $cfg,$plat,
    ($out | Select-String ' warning ' | Measure-Object).Count,
    ($out | Select-String ' error '   | Measure-Object).Count
}}
```

**期待される結果**: 4構成すべてで警告0・エラー0。

**特に注意する警告**: 削除作業では「未使用の変数」「未使用の関数」「参照されない静的関数」が出やすい。出た場合は消し残しである（FR-204 / ST-202）。

---

## 手順2: 静的検査（ST-201, ST-202, ST-204, ST-205）

### ST-201: UIリソースの検査

```powershell
$rc=[System.IO.File]::ReadAllText((Resolve-Path 'MFCApplication1\MFCApplication1.rc'),[System.Text.Encoding]::Unicode)
$rh=[System.IO.File]::ReadAllText((Resolve-Path 'MFCApplication1\Resource.h'),[System.Text.Encoding]::UTF8)

"--- 削除されているべきもの（両方 False）---"
"IDC_BUTTON_DIAGNOSE   Resource.h={0} .rc={1}" -f ($rh -match 'IDC_BUTTON_DIAGNOSE'), ($rc -match 'IDC_BUTTON_DIAGNOSE')
"準備状況を診断（ラベル）: " + ($rc -match '準備状況を診断')

"--- 残っているべきもの（両方 True）---"
foreach($id in 'IDC_EDIT_TARGET_PATH','IDC_BUTTON_BROWSE','IDC_BUTTON_GENERATE','IDC_LIST_RESULT','IDC_STATIC_GUIDE'){
  "{0,-22} Resource.h={1,-5} .rc={2}" -f $id, ($rh -match $id), ($rc -match $id) }
```

### ST-202: 未使用コードの検査

```powershell
"--- ソース中に残っていてはならない語（すべて0）---"
foreach($w in 'ReadinessChecker','ReadinessItem','ReadinessVerdict','BUTTON_DIAGNOSE','OnBnClickedButtonDiagnose','ShowReadinessGuide','hasGitignore'){
  $c = (Get-ChildItem MFCApplication1\*.h,MFCApplication1\*.cpp | ForEach-Object {
    ([regex]::Matches([System.IO.File]::ReadAllText($_.FullName,[System.Text.Encoding]::UTF8), [regex]::Escape($w))).Count
  } | Measure-Object -Sum).Sum
  "{0,-28} {1}" -f $w, $c
}
"--- ファイルが削除されているか（両方 False）---"
"ReadinessChecker.h  : " + (Test-Path MFCApplication1\ReadinessChecker.h)
"ReadinessChecker.cpp: " + (Test-Path MFCApplication1\ReadinessChecker.cpp)
"--- vcxproj からの登録解除（両方 0）---"
$vc = Get-Content MFCApplication1\MFCApplication1.vcxproj -Raw
"vcxproj 内の ReadinessChecker: " + ([regex]::Matches($vc,'ReadinessChecker')).Count
```

**`hasGitignore` を検査語に含めている理由**: 診断を消すと読み出し側が消え、書き込みのみの孤立フィールドになる（[research.md](./research.md) R-202）。コンパイラは書き込みのみのメンバ変数を警告しないため、ここで検出する。

### ST-204: 完了案内の残存検査（**削除禁止のものが残っているか**）

```powershell
"--- 残っていなければならないもの（すべて 1 以上）---"
foreach($w in 'ShowCompletionGuide','Spec Kit の導入が完了しました','init を再実行し','AppendResultRow','ClearResultList'){
  $c = (Get-ChildItem MFCApplication1\*.h,MFCApplication1\*.cpp | ForEach-Object {
    ([regex]::Matches([System.IO.File]::ReadAllText($_.FullName,[System.Text.Encoding]::UTF8), [regex]::Escape($w))).Count
  } | Measure-Object -Sum).Sum
  "{0,-34} {1}" -f $w, $c
}
```

**0 が出たら不合格。** `ShowCompletionGuide` と `ShowReadinessGuide` は名前が似ており、削除時に取り違えやすい。

### ST-205: 仕様と実装の対応

```powershell
$all=@()
Get-ChildItem MFCApplication1\*.h,MFCApplication1\*.cpp | ForEach-Object {
  $all += [regex]::Matches([System.IO.File]::ReadAllText($_.FullName,[System.Text.Encoding]::UTF8),'FR-2\d{2}') | ForEach-Object { $_.Value } }
$rcTxt=[System.IO.File]::ReadAllText((Resolve-Path 'MFCApplication1\MFCApplication1.rc'),[System.Text.Encoding]::Unicode)
$all += [regex]::Matches($rcTxt,'FR-2\d{2}') | ForEach-Object { $_.Value }
"実装から参照されている 003 の FR: " + (($all | Sort-Object -Unique) -join ' ')
```

---

## 手順3: 画面の確認（DT-201, DT-202, DT-206）

アプリケーションを起動する。

```powershell
Start-Process 'C:\work\claude\prompt_editor_speckit\MFCApplication1\x64\Debug\MFCApplication1.exe'
```

**期待される結果**:

| No. | 確認項目 |
|---|---|
| DT-201 | 「準備状況を診断」ボタンが**存在しない** |
| DT-202 | 「生成」ボタンが左端に1つだけ並び、レイアウトが崩れていない。縦方向に不自然な空きが無い |
| DT-206 | 初期案内が「対象フォルダを選び、「生成」を押してください。…」であり、**診断への言及が無い** |

Tab キーで巡回すると `パス欄 → 参照 → 生成 → 一覧 → OK → キャンセル` の順に移動すること（FR-208）。案内欄は巡回に含まれない。

---

## 手順4: 未選択ガード（DT-204・回帰確認）

「参照...」を押さずに「生成」を押す。

| # | 確認項目 |
|---|---|
| (a) | 「対象フォルダを選択してください。」が表示される |
| (b) | 結果一覧に何も表示されない |
| (c) | どのフォルダにもファイルが作成・変更されない |
| (d) | メッセージを閉じた後、「生成」ボタンが**押下可能なまま** |
| (e) | アプリケーションが異常終了しない |

**(d) が重要**: 001 から継承した契約である。

---

## 手順5: SDD化の実行（DT-203, DT-207・回帰確認）

1. 「参照...」で `C:\work\test\test01` を選ぶ
2. 「生成」を押す

**DT-207（回帰確認・最重要）**: 処理中に以下を目視する。

| コントロール | 期待 |
|---|---|
| 生成ボタン | グレーアウト |
| **参照...ボタン** | **グレーアウト** |

**参照ボタンが押せる状態なら不合格。** 診断ボタンを配列から取り除く際に、前後を巻き添えで外した誤りである（[research.md](./research.md) R-204(1)）。完了後、両方が有効に戻ること。

**DT-203**: 5手順が順に一覧へ追加され、それぞれ「成功」または「スキップ」となること。

```powershell
Get-ChildItem C:\work\test\test01 -Force -Name | Where-Object { $_ -like '.*' }
Push-Location C:\work\test\test01; git rev-list --count HEAD; Pop-Location
```

`.git`・`.gitignore`・`.specify`・`.claude` が揃い、コミットが1件以上あること。

---

## 手順6: 完了案内の確認（DT-205・回帰確認）

手順5の直後、画面下の案内欄を確認する。

| # | 確認項目 |
|---|---|
| (a) | 5行の定型案内が表示されている |
| (b) | 最後の行「`4. /init を再実行し、憲章を反映した CLAUDE.md に更新する`」まで読める（スクロールを含む） |
| (c) | 案内文をドラッグ選択して Ctrl+C でコピーできる |
| (d) | 文字を入力しても編集できない（読み取り専用） |

**この手順が不合格なら、`ShowCompletionGuide()` を `ShowReadinessGuide()` と取り違えて削除している。** 002 の FR-110〜112 で確保したばかりの機能である。

---

## 手順7: べき等性（DT-208・回帰確認）

同じフォルダで再度「生成」を押す。

**期待される結果**: 5手順すべてが「スキップ」。「成功」が1件も出ないこと。

```powershell
Push-Location C:\work\test\test01
git status --short          # 変更が出ないこと
git rev-list --count HEAD   # 手順5から増えていないこと
Pop-Location
```

---

## 完了条件

- [ ] ST-201〜ST-205 の静的テストがすべて合格
- [ ] DT-201〜DT-208 の動的テストがすべて合格
- [ ] 特に **DT-207**（実行中に参照ボタンが無効化される）が合格 — 削除の巻き添えを検出する要所
- [ ] 特に **DT-205**（完了案内が残っている）が合格 — 取り違え削除を検出する要所
- [ ] 手順1のビルドで、未使用変数・未使用関数の警告が0件
- [ ] `ReadinessChecker.h` / `.cpp` が存在せず、vcxproj からも登録が消えている

---

## 付録: テスト用フォルダのリセット

```powershell
$t='C:\work\test\test01'
foreach($x in '.git','.specify','.claude'){ $p=Join-Path $t $x; if(Test-Path $p){ Remove-Item -LiteralPath $p -Recurse -Force } }
$gi=Join-Path $t '.gitignore'; if(Test-Path $gi){ Remove-Item -LiteralPath $gi -Force }
Get-ChildItem $t -Force -Name
```

ソリューション本体（`test01.sln` 等）が残っていること。
