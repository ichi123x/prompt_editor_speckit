# クイックスタート検証ガイド: ダイアログのリサイズ追従

**Feature**: 004-resizable-dialog
**Date**: 2026-09-09

本フィーチャーは**画面の見た目と操作感が成果物**である。その大部分はウィンドウを実際にドラッグしないと確認できないため、**動的テストの比重が 001〜003 より大きい**。静的テストで確認できるのは実装の構造に限られる。

判定基準は [spec.md](./spec.md) の「テスト方針」、契約は [contracts/ui-contract.md](./contracts/ui-contract.md)、設計根拠は [research.md](./research.md) を参照する。

---

## 前提条件

| 項目 | 内容 |
|---|---|
| 実行ファイル | `C:\work\claude\prompt_editor_speckit\MFCApplication1\x64\Debug\MFCApplication1.exe` |
| テスト対象 | `C:\work\test\test01`（MFC アプリ） |

**プラットフォーム名の注意**: `.sln` に渡すのは `x86`（`Win32` は `MSB4126` になる）。**ビルド前にアプリを終了させること**（`LNK1104`）。

---

## 手順0: テスト用フォルダの準備

```powershell
$t='C:\work\test\test01'
foreach($x in '.git','.specify','.claude'){ $p=Join-Path $t $x; if(Test-Path $p){ Remove-Item -LiteralPath $p -Recurse -Force } }
foreach($x in '.gitignore','package.json'){ $p=Join-Path $t $x; if(Test-Path $p){ Remove-Item -LiteralPath $p -Force } }
Get-ChildItem $t -Force -Name | Where-Object { $_ -like '.*' -or $_ -like '*.sln' }
```

`.vs` と `test01.sln` だけが残っていること。

---

## 手順1: ビルド（ST-304）

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

---

## 手順2: 静的検査（ST-301, ST-302, ST-303, ST-305）

### ST-301 / ST-302: 追従処理と下限設定の存在

```powershell
$src = (Get-ChildItem MFCApplication1\*.h,MFCApplication1\*.cpp | ForEach-Object {
  [System.IO.File]::ReadAllText($_.FullName,[System.Text.Encoding]::UTF8) }) -join "`n"
"--- ST-301: 追従処理（すべて1以上）---"
foreach($w in 'OnSize','ON_WM_SIZE','SetWindowPos','m_anchors','m_layoutReady'){
  "  {0,-18} {1}" -f $w, ([regex]::Matches($src,[regex]::Escape($w))).Count }
"--- ST-302: 下限設定（すべて1以上）---"
foreach($w in 'OnGetMinMaxInfo','ON_WM_GETMINMAXINFO','ptMinTrackSize','m_minWindowSize'){
  "  {0,-22} {1}" -f $w, ([regex]::Matches($src,[regex]::Escape($w))).Count }
```

### ST-303: 追従方針の網羅（**要所**）

追従テーブルに載っているコントロール数と、`.rc` のコントロール数を突き合わせる。

```powershell
$src = (Get-ChildItem MFCApplication1\*.h,MFCApplication1\*.cpp | ForEach-Object {
  [System.IO.File]::ReadAllText($_.FullName,[System.Text.Encoding]::UTF8) }) -join "`n"
$rc = [System.IO.File]::ReadAllText((Resolve-Path 'MFCApplication1\MFCApplication1.rc'),[System.Text.Encoding]::Unicode)

# .rc のダイアログ内コントロールを列挙
$dlg = ($rc -split "`r`n" | Select-String 'IDD_MFCAPPLICATION1_DIALOG DIALOGEX' -Context 0,25).Context.PostContext
$rcIds = $dlg | Where-Object { $_ -match 'IDC_|IDOK|IDCANCEL' } |
  ForEach-Object { if($_ -match '(IDC_[A-Z_]+|IDOK|IDCANCEL)'){ $Matches[1] } } | Sort-Object -Unique

"--- .rc のコントロール ({0}件) ---" -f $rcIds.Count
$rcIds | ForEach-Object { "  $_" }

"--- 追従テーブルに定義されているか ---"
$missing = @()
foreach($id in $rcIds){
  $found = $src -match ($id + '\s*,\s*AnchorMode::')
  "  {0,-26} {1}" -f $id, $(if($found){'定義あり'}else{'★未定義'})
  if(-not $found){ $missing += $id }
}
"--- 判定: " + $(if($missing.Count -eq 0){'PASS（全コントロールに方針あり）'}else{'FAIL 未定義: ' + ($missing -join ', ')}) + " ---"
```

**期待される結果**: `.rc` のコントロール8件すべてに追従方針が定義されている。

**このテストの狙い**: コントロールごとに方針を定めるため、**1つ定義し忘れるとそのコントロールだけ元の位置に取り残され**、他と重なるかウィンドウの外へはみ出す。`Fixed` のコントロールもテーブルに載せているのは、「意図的に固定」と「定義を忘れた」を区別するためである（[contracts/ui-contract.md](./contracts/ui-contract.md)「2. ラベルのID」）。

### ST-305: 仕様と実装の対応

```powershell
$src = (Get-ChildItem MFCApplication1\*.h,MFCApplication1\*.cpp | ForEach-Object {
  [System.IO.File]::ReadAllText($_.FullName,[System.Text.Encoding]::UTF8) }) -join "`n"
$src += [System.IO.File]::ReadAllText((Resolve-Path 'MFCApplication1\MFCApplication1.rc'),[System.Text.Encoding]::Unicode)
"参照されている 004 の FR: " + (([regex]::Matches($src,'FR-3\d{2}') | ForEach-Object { $_.Value } | Sort-Object -Unique) -join ' ')
```

FR-301〜310 の10件が揃っていること。

---

## 手順3: 横方向のリサイズ（DT-301, DT-304）

アプリケーションを起動する。

```powershell
Start-Process 'C:\work\claude\prompt_editor_speckit\MFCApplication1\x64\Debug\MFCApplication1.exe'
```

「参照...」で `C:\work\test\test01` を選び、「生成」を押して結果一覧に5行表示させておく（見え方の変化が分かりやすい）。

**ウィンドウの右端をドラッグして、横に大きく広げる。**

| No. | 確認項目 |
|---|---|
| DT-301 | パス表示欄の**幅が広がる** |
| DT-301 | 結果一覧の**幅が広がり**、右側に空白が残らない |
| DT-301 | 案内欄の**幅が広がる** |
| DT-304 | 「参照...」ボタンが**右へ移動**し、右端との距離が変わらない。大きさは変わらない |
| DT-304 | 「生成」ボタンは**動かない**。左端に留まる |

**以前の不具合の再現確認**: 修正前は、広げても各コントロールが元の大きさのまま取り残され、右側に大きな空白ができていた。それが解消していること。

---

## 手順4: 縦方向のリサイズ（DT-302, DT-305）

**ウィンドウの下端をドラッグして、縦に大きく広げる。**

| No. | 確認項目 |
|---|---|
| DT-302 | 結果一覧の**高さが伸びる**（スクロールなしで見える行数が増える） |
| DT-302 | 案内欄が**下へ移動**する。高さは変わらない |
| DT-302 | 下側に不自然な空白が残らない |
| DT-305 | OK・キャンセルが**下へ移動**し、右下の隅との距離が変わらない |

**案内欄の高さが変わらないのは正しい挙動**である。002 で縦スクロール可能にしてあり、増えた高さはすべて結果一覧に振ることで読める行数が最大になる（SC-302）。

---

## 手順5: 縮小と復元（DT-303, SC-304）

**ウィンドウを元のサイズまで縮める。**

| No. | 確認項目 |
|---|---|
| DT-303 | 各コントロールが**縮小に追従**する |
| DT-303 | コントロールがウィンドウの外へ**はみ出さない** |
| DT-303 | コントロールどうしが**重ならない** |
| SC-304 | 元のサイズに戻したとき、**変更前と同じ配置**になっている |

**SC-304 が重要**: 拡大と縮小を何度か繰り返してから元のサイズに戻し、位置がずれていないことを確認する。前回サイズからの相対計算で実装すると丸め誤差が累積してずれる。初期サイズからの差分で計算していれば構造的にずれない（[research.md](./research.md) R-303）。

---

## 手順6: 最小サイズ（DT-306, DT-307）

**ウィンドウの右下をつまんで、可能な限り小さくする。**

| No. | 確認項目 |
|---|---|
| DT-306 | 既定のサイズより**小さくならない**。そこで下限に達する |
| DT-307 | 下限でもすべてのコントロールが**見えている** |
| DT-307 | ボタンが**クリックできる** |
| DT-307 | 結果一覧に少なくとも数行が表示できる高さがある |

---

## 手順7: 結果一覧の列幅（DT-308）

横に広げた状態で、結果一覧のヘッダーを見る。

| # | 確認項目 |
|---|---|
| (a) | 「項目」「結果」「内容」の3列が、一覧の幅いっぱいに広がっている |
| (b) | 右側に無駄な余白ができていない |
| (c) | 不要な横スクロールバーが出ていない |
| (d) | 「内容」列で読める文字数が増えている（SC-301） |

**(d) が本フィーチャーの主目的**である。手順の失敗理由や `.gitignore` の状況が既定幅では切れることがあり、広げれば読めるようにするのが狙い。

---

## 手順8: 最大化・最小化（DT-309）

1. ウィンドウを**最大化**する → 通常のリサイズと同じ追従が行われること
2. **元に戻す** → 配置が崩れないこと
3. **最小化**してタスクバーへ格納する
4. **元に戻す** → 配置が崩れないこと

---

## 手順9: 実行中のリサイズ（DT-310・回帰確認）

1. [quickstart.md](./quickstart.md) 手順0で `test01` をリセットする
2. `test01` を選んで「生成」を押す
3. **処理が走っている最中に**ウィンドウのサイズを変更する

| # | 確認項目 |
|---|---|
| (a) | 処理が**中断しない** |
| (b) | 結果一覧への行の追加が**正しく続く**（5手順すべてが表示される） |
| (c) | リサイズ中もアプリが応答する |
| (d) | 処理中は生成ボタンと参照ボタンが**グレーアウトのまま**（003 FR-209 の回帰） |
| (e) | 完了後に両ボタンが**有効に戻る** |

**この確認が成立する理由**: ワーカースレッドはコントロールを直接操作せず `PostMessage` で結果を送るだけなので、再配置と行追加が競合しない。追加の排他制御は不要である（[research.md](./research.md) R-305(1)）。

---

## 手順10: 既存機能の回帰確認（SC-305）

001〜003 で合格していた項目が再現することを確認する。

```powershell
Get-ChildItem C:\work\test\test01 -Force -Name | Where-Object { $_ -like '.*' }
Push-Location C:\work\test\test01; git rev-list --count HEAD; git status --short; Pop-Location
```

- `.git`・`.gitignore`・`.specify`・`.claude` の4点セットが揃う
- コミットが1件以上
- もう一度「生成」を押すと5手順すべてが「スキップ」（べき等性）
- 「参照...」を押さずに「生成」→ 未選択メッセージが出て、生成ボタンは押下可能なまま
- 完了案内が5行読めて、選択してコピーできる

---

## 完了条件

- [ ] ST-301〜ST-305 の静的テストがすべて合格
- [ ] DT-301〜DT-310 の動的テストがすべて合格
- [ ] 特に **ST-303**（追従方針の網羅）が合格 — 定義漏れを機械的に検出する要所
- [ ] 特に **SC-304**（拡大縮小を繰り返しても元に戻る）が合格 — 相対計算による累積ずれを検出する要所
- [ ] 特に **DT-310**（実行中のリサイズ）が合格 — 001〜003 の挙動を壊していないことの確認
- [ ] 手順1のビルドで警告が0件

---

## 付録: テスト用フォルダのリセット

```powershell
$t='C:\work\test\test01'
foreach($x in '.git','.specify','.claude'){ $p=Join-Path $t $x; if(Test-Path $p){ Remove-Item -LiteralPath $p -Recurse -Force } }
foreach($x in '.gitignore','package.json'){ $p=Join-Path $t $x; if(Test-Path $p){ Remove-Item -LiteralPath $p -Force } }
Get-ChildItem $t -Force -Name
```

ソリューション本体（`test01.sln` 等）が残っていること。
