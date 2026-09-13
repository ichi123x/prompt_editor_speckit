# クイックスタート（検証手順）: 完了案内にCLAUDE.mdのコミット手順を追加する

**Feature**: 012-guide-claude-md-commit
**Date**: 2026-09-13

静的テスト ST-1201〜1202 は自動化する。ST-1202は`/speckit-analyze`で判明した006 FR-507aとの
矛盾（案内欄に増やした行数を表示する余裕が無い問題）への対応（`.rc`の寸法変更）を検証する。
動的テスト DT-1201（実機でのフルフロー確認、案内欄が実際に7行表示できることの目視確認を含む）は
フォルダ選択ダイアログを伴うため008以降と同じ理由で自動化できず、利用者による任意の手動実行とする。

---

## 手順0: 準備

```powershell
Get-Process MFCApplication1 -ErrorAction SilentlyContinue | Stop-Process -Force
```

---

## 手順1: ビルド

```powershell
$msb='C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe'
foreach($cfg in 'Debug','Release'){ foreach($plat in 'x64','x86'){
  $out = & $msb MFCApplication1\MFCApplication1.sln /p:Configuration=$cfg /p:Platform=$plat /t:Rebuild /v:minimal /nologo 2>&1
  "{0,-8} {1,-5} 警告={2} エラー={3}" -f $cfg,$plat,
    ($out | Select-String ' warning ' | Measure-Object).Count,
    ($out | Select-String ' error '   | Measure-Object).Count
}}
```

**合格条件**: 8行すべてで警告=0 エラー=0。動作確認は`MFCApplication1\x64\Debug\MFCApplication1.exe`（Debug構成）でよい。

---

## 手順2: 静的テスト（ST-1201）

### ST-1201: 案内文の手順順序・既存文言の維持・FRの追跡可能性

```powershell
$codeOnly = (Get-Content MFCApplication1\MFCApplication1Dlg.cpp) | Where-Object { $_.TrimStart() -notmatch '^//' }
$text = $codeOnly -join "`n"

Write-Host "=== 手順の順序: /init実行 → CLAUDE.mdコミット → テスト方針決定 ==="
$m = [regex]::Match($text, '/init[\s\S]{0,60}?CLAUDE\.md[\s\S]{0,150}?git (add|commit)[\s\S]{0,200}?/speckit-constitution')
"順序どおりになっている: $($m.Success)"

Write-Host "`n=== 既存の文言が維持されていること ==="
$checks = @(
  'VS Code で %s を開く',
  '/init を実行し、CLAUDE\.md を生成する',
  'テスト方針を決め、/speckit-constitution で憲章を制定する',
  '/init を再実行し、憲章を反映した CLAUDE\.md に更新する'
)
foreach ($c in $checks) {
  "{0,-55}: {1}" -f $c, ([regex]::IsMatch($text, $c))
}

Write-Host "`n=== 失敗時の案内文が変更されていないこと ==="
Select-String -Path MFCApplication1\MFCApplication1Dlg.cpp -Pattern '一部の手順が失敗しました'

Write-Host "`n=== FRによる追跡可能性(コメント行を除外しない) ==="
$textRaw = Get-Content MFCApplication1\MFCApplication1Dlg.cpp -Raw
foreach ($i in 1201..1204) {
  $n = "FR-$i"
  "{0}: {1}" -f $n, ([regex]::Matches($textRaw, $n)).Count
}

$git = (Get-Command git -ErrorAction SilentlyContinue).Source
if (-not $git) { $git = 'C:\Program Files\Git\cmd\git.exe' }
Write-Host "`n=== SetupRunner.cpp / ProcessRunner.cpp に012による追加差分が無いこと ==="
& $git diff --stat -- MFCApplication1/SetupRunner.cpp MFCApplication1/ProcessRunner.cpp MFCApplication1/ProcessRunner.h
```

**合格条件**: 手順の順序確認が`True`であること。既存4文言がすべて`True`（維持されている）こと。
失敗時の案内文（「一部の手順が失敗しました...」）がそのまま存在すること。FR-1201〜1204がいずれも
1件以上出現すること。`SetupRunner.cpp`・`ProcessRunner.cpp`・`ProcessRunner.h`に012による追加差分が
ないこと（008〜011からの差分のみ）。

### ST-1202: `.rc`の寸法変更（006 FR-507aへの対応）

```powershell
$rc = [System.IO.File]::ReadAllText("MFCApplication1\MFCApplication1.rc", [System.Text.Encoding]::Unicode)

$guide = [regex]::Match($rc, 'IDC_STATIC_GUIDE,7,147,531,(\d+)')
$guideHeight = [int]$guide.Groups[1].Value
"IDC_STATIC_GUIDE の高さ: $guideHeight (期待値: 61)"

$ok = [regex]::Match($rc, 'IDOK,434,(\d+)')
$okY = [int]$ok.Groups[1].Value
"IDOK のY座標: $okY (期待値: 212)"

$cancel = [regex]::Match($rc, 'IDCANCEL,488,(\d+)')
$cancelY = [int]$cancel.Groups[1].Value
"IDCANCEL のY座標: $cancelY (期待値: 212)"

$dialog = [regex]::Match($rc, 'IDD_MFCAPPLICATION1_DIALOG DIALOGEX\s+0,\s*0,\s*545,\s*(\d+)')
$dialogHeight = [int]$dialog.Groups[1].Value
"ダイアログ全体の高さ: $dialogHeight (期待値: 233)"

Write-Host "`n=== 差分の整合性(すべて+9であること) ==="
"案内欄の増分   : $($guideHeight - 52)"
"ボタンY座標の増分: $($okY - 203)"
"ダイアログ高さの増分: $($dialogHeight - 224)"

Write-Host "`n=== FR-1205の追跡可能性(.rcのコメント行を含めて確認) ==="
$rcText = $rc
"FR-1205: $(([regex]::Matches($rcText, 'FR-1205')).Count)"
```

**合格条件**: `IDC_STATIC_GUIDE`の高さが61、`IDOK`・`IDCANCEL`のY座標が212、ダイアログ全体の高さが
233であること。3つの増分（案内欄・ボタンY座標・ダイアログ高さ）がすべて`+9`で一致していること
（[research.md](./research.md) R-1202、006 R-507と同じ「差分がそのまま乗る」関係）。`.rc`に
`FR-1205`のコメントが1件以上存在すること。

---

## 手順3: DT-1201（実機でのフルフロー確認、任意・利用者による手動実行）

1. uv・specify CLI・claude CLI・gitがいずれも導入済みの環境で、対象フォルダを選択し「生成」を実行する
2. 5手順すべてが成功したことを確認する
3. 表示された完了案内を確認し、「`/init`実行」の直後に「CLAUDE.mdをコミットする」手順が示されていることを確認する（SC-1201）
4. **起動直後の大きさのまま**、完了案内の6行（見出し＋手順5つ、フォルダパスの折り返し1回を含む）
   すべてが縦スクロールなしで同時に見えることを確認する（SC-1203、006 FR-504・FR-504aの水準を
   維持できているかの実機確認。ST-1202は`.rc`の数値の整合性のみを確認しており、実際に文字が
   欠けずに収まるかはこの手順でしか確認できない）
5. 案内表示領域にレイアウト崩れ（文字の欠け、スクロールバーの不整合、他コントロールとの重なり等）が
   無いことを確認する（SC-1202）

**合格条件**: 手順3・4・5がすべて確認できること。**手順4で7行目（Tipに相当する最終行）が見切れて
いた場合、R-1202の見積もり（61 DLU）が実機で不足していたことを意味する。その場合はST-1202の
期待値と`.rc`の値をさらに大きく調整し、本手順を再実行すること。**

---

## 完了条件

- [ ] ST-1201〜1202の静的テストが合格
- [ ] DT-1201は任意。実行できる場合は合格していること。**特に手順4（7行すべてが見えること）は
      R-1202の見積もりの妥当性を実機で裏付ける唯一の手段であるため、実行を強く推奨する**
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
