# クイックスタート（検証手順）: 生成されるCLAUDE.mdの日本語化

**Feature**: 014-japanese-claude-md
**Date**: 2026-09-13

静的テスト ST-1401 は自動化する。動的テスト DT-1401（生成されたCLAUDE.mdが実際に日本語で
書かれているかの目視確認）は、自然言語の内容を機械的に判定する手段が無いため、利用者による
任意の手動実行とする。

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

**合格条件**: 8行すべてで警告=0 エラー=0。

---

## 手順2: 静的テスト

### ST-1401: コマンドライン文字列の変更・013からの無変更範囲の確認

```powershell
$cText = Get-Content MFCApplication1\SetupRunner.cpp -Raw

Write-Host "=== プロンプトに日本語指示が含まれていること ==="
[regex]::IsMatch($cText, '/init のCLAUDE\.mdは日本語で書いてください')

Write-Host "`n=== 既存フラグが維持されていること ==="
[regex]::IsMatch($cText, '--permission-mode bypassPermissions')
[regex]::IsMatch($cText, '--output-format text')

Write-Host "`n=== git add -- CLAUDE.md 単体ステージングが維持されていること ==="
[regex]::IsMatch($cText, 'git add -- CLAUDE\.md')

Write-Host "`n=== FRによる追跡可能性 ==="
foreach ($i in 1401..1403) {
  "FR-$i : $(([regex]::Matches($cText, "FR-$i")).Count)"
}

Write-Host "`n=== SetupRunner.h・MFCApplication1Dlg.cpp・MFCApplication1.rc に014による追加差分が無いこと ==="
$git = 'C:\Program Files\Git\cmd\git.exe'
& $git diff --stat -- MFCApplication1/SetupRunner.h MFCApplication1/MFCApplication1Dlg.cpp MFCApplication1/MFCApplication1.rc MFCApplication1/ProcessRunner.cpp MFCApplication1/ProcessRunner.h
```

**合格条件**: 日本語指示が含まれること。`--permission-mode bypassPermissions`・
`--output-format text`・`git add -- CLAUDE.md`がいずれも維持されていること。FR-1401〜1403が
1件以上出現すること。`SetupRunner.h`等、`SetupRunner.cpp`以外のファイルに014による追加差分が
無いこと。

---

## 手順3: DT-1401（生成物の日本語確認、任意・利用者による手動実行）

1. CLAUDE.mdが存在しない対象フォルダで「生成」を実行し、手順6が成功することを確認する
2. 生成されたCLAUDE.mdを開き、本文（見出し・説明文）が日本語で書かれていることを確認する（定型
   ヘッダー2行が英語のままなのは正常）
3. `git log`で「CLAUDE.md を追加」のコミットが記録されていることを確認する（013からの継続）

**合格条件**: 手順1〜3がすべて確認できること。

---

## 完了条件

- [ ] ST-1401が合格
- [ ] DT-1401は任意。実行できる場合は合格していること
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
