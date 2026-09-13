# クイックスタート（検証手順）: CLAUDE.md生成とコミットの自動化

**Feature**: 013-automate-claude-init
**Date**: 2026-09-13

静的テスト ST-1301〜1303 は自動化する。動的テスト DT-1301（実機でのフルフロー確認、
`claude`によるCLAUDE.md生成とコミットまでの確認を含む）はフォルダ選択ダイアログを伴うため
008以降と同じ理由で自動化できず、利用者による任意の手動実行とする。

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

## 手順2: 静的テスト

### ST-1301: 新しい手順（手順6）の構造・依存関係・既存5手順の無変更

```powershell
$hText = Get-Content MFCApplication1\SetupRunner.h -Raw
$cText = Get-Content MFCApplication1\SetupRunner.cpp -Raw

Write-Host "=== StepId に GenerateClaudeMd が追加されていること ==="
[regex]::IsMatch($hText, 'GenerateClaudeMd')

Write-Host "`n=== 手順6が InitGit に依存していること ==="
[regex]::IsMatch($cText, 'dependsOn\s*=\s*StepId::InitGit')

Write-Host "`n=== claude /init のコマンドラインが実装されていること ==="
[regex]::IsMatch($cText, 'claude\s+--print\s+/init\s+--permission-mode\s+bypassPermissions')

Write-Host "`n=== CLAUDE.mdの単体ステージング（git add -A ではないこと）==="
[regex]::IsMatch($cText, 'git add -- CLAUDE\.md')

Write-Host "`n=== 既存5手順のコマンドラインが変更されていないこと ==="
$existing = @(
  'winget install --id=astral-sh\.uv',
  'uv tool install specify-cli',
  'cmd\.exe /c code --install-extension anthropic\.claude-code',
  'specify init \. --integration claude --script ps --force --non-interactive',
  'git commit -m \\"Spec Kit 導入\\"'
)
foreach ($p in $existing) {
  "{0,-70}: {1}" -f $p, ([regex]::IsMatch($cText, $p))
}

Write-Host "`n=== FRによる追跡可能性(コメント行を除外しない) ==="
foreach ($i in 1301..1312) {
  $n = "FR-$i"
  "{0}: {1}" -f $n, (([regex]::Matches($hText + $cText, $n)).Count)
}

Write-Host "`n=== ProcessRunner.cpp / TargetFolderState.* / GitignoreWriter.* に013による追加差分が無いこと ==="
$git = (Get-Command git -ErrorAction SilentlyContinue).Source
if (-not $git) { $git = 'C:\Program Files\Git\cmd\git.exe' }
& $git diff --stat -- MFCApplication1/ProcessRunner.cpp MFCApplication1/ProcessRunner.h MFCApplication1/TargetFolderState.cpp MFCApplication1/TargetFolderState.h MFCApplication1/GitignoreWriter.cpp MFCApplication1/GitignoreWriter.h
```

**合格条件**: `GenerateClaudeMd`が定義されていること。手順6の依存が`InitGit`であること。`claude /init`の
コマンドラインと`git add -- CLAUDE.md`が実装されていること。既存5手順のコマンドラインがすべて`True`
（変更なし）であること。FR-1301〜1312がいずれも1件以上出現すること。`ProcessRunner`・`TargetFolderState`・
`GitignoreWriter`に013による追加差分が無いこと。

### ST-1302: 完了案内の文言（012からの2手順削除・番号振り直し）

```powershell
$codeOnly = (Get-Content MFCApplication1\MFCApplication1Dlg.cpp) | Where-Object { $_.TrimStart() -notmatch '^//' }
$text = $codeOnly -join "`n"

Write-Host "=== 012で追加した2手順の文言が削除されていること ==="
$removed = @(
  '/init を実行し、CLAUDE\.md を生成する',
  '生成された CLAUDE\.md をコミットする'
)
foreach ($p in $removed) {
  "{0,-45}: 削除済み={1}" -f $p, (-not [regex]::IsMatch($text, $p))
}

Write-Host "`n=== 残りの手順の文言が維持されていること（番号のみ変わる） ==="
$kept = @(
  'VS Code で %s を開く',
  'テスト方針を決め、/speckit-constitution で憲章を制定する',
  '/init を再実行し、憲章を反映した CLAUDE\.md に更新する'
)
foreach ($c in $kept) {
  "{0,-55}: {1}" -f $c, ([regex]::IsMatch($text, $c))
}

Write-Host "`n=== 手順の順序: VS Codeを開く(1) → テスト方針決定(2) → /init再実行(3) ==="
$m = [regex]::Match($text, 'VS Code で[\s\S]{0,80}?テスト方針を決め[\s\S]{0,150}?/init を再実行')
"順序どおりになっている: $($m.Success)"

Write-Host "`n=== 失敗時の案内文が変更されていないこと ==="
Select-String -Path MFCApplication1\MFCApplication1Dlg.cpp -Pattern '一部の手順が失敗しました'
```

**合格条件**: 012で追加した2手順の文言が両方とも削除されていること。残りの3手順（VS Codeを開く／
テスト方針決定と憲章制定／`/init`再実行）の文言が維持され、この順序で並んでいること。失敗時の案内文
（「一部の手順が失敗しました...」）がそのまま存在すること。

### ST-1303: `.rc`の寸法が変更されていないこと（research.md R-1302・R-1303の裏付け）

```powershell
$rc = [System.IO.File]::ReadAllText("MFCApplication1\MFCApplication1.rc", [System.Text.Encoding]::Unicode)

$list = [regex]::Match($rc, 'IDC_LIST_RESULT[\s\S]{0,120}?,7,52,531,(\d+)')
"IDC_LIST_RESULT の高さ: $($list.Groups[1].Value) (期待値: 90、012までと同じ)"

$guide = [regex]::Match($rc, 'IDC_STATIC_GUIDE,7,147,531,(\d+)')
"IDC_STATIC_GUIDE の高さ: $($guide.Groups[1].Value) (期待値: 61、012と同じ)"

$dialog = [regex]::Match($rc, 'IDD_MFCAPPLICATION1_DIALOG DIALOGEX\s+0,\s*0,\s*545,\s*(\d+)')
"ダイアログ全体の高さ: $($dialog.Groups[1].Value) (期待値: 233、012と同じ)"
```

**合格条件**: 3つの数値がいずれも012の値（90・61・233）から変化していないこと。research.md R-1302・
R-1303の「寸法変更は不要」という判断が実装後も裏付けられることを確認する。**もしST-1301〜1302の
実装過程で寸法変更が必要と判明した場合は、この期待値と`.rc`を合わせて更新しtasks.mdに記録すること。**

---

## 手順3: DT-1301（実機でのフルフロー確認、任意・利用者による手動実行）

1. `claude auth status`で認証済み（`loggedIn: true`）であることを確認する
2. CLAUDE.mdが存在しない対象フォルダを選択し、「生成」を実行する
3. 結果一覧に6手順分の結果が表示され、6手順目「CLAUDE.md の生成とコミット」が「成功」になることを確認する（数十秒〜数分待つ）
4. 対象フォルダ直下にCLAUDE.mdが生成されていることを確認する
5. 対象フォルダで`git log --oneline`を実行し、「CLAUDE.md を追加」というコミットが記録されていることを確認する
6. 表示された完了案内を確認し、「/initを実行する」「CLAUDE.mdをコミットする」という文言（012由来）が**無く**、
   「VS Codeを開く」→「テスト方針決定・憲章制定」→「/init再実行」の3手順が1から順番に表示されていることを確認する（SC-1301）
7. **起動直後の大きさのまま**、結果一覧6行・完了案内3手順（見出し含め4行、パス折り返し1回を含め最大5行）が
   縦スクロールなしで同時に見えることを確認する（SC-1303、006 FR-501・504の水準を維持できているかの実機確認）
8. 再度「生成」を実行し、6手順すべてが「スキップ」になり、CLAUDE.mdが上書きされないことを確認する（べき等性）
9. `claude`が導入されていない、または未認証の環境がもしあれば、6手順目のみが「失敗」になり、5手順目までの結果に影響が無いことを確認する（SC-1302、任意）

**合格条件**: 手順3〜8がすべて確認できること。**手順7で行が見切れていた場合、research.md R-1302の
「寸法変更不要」という判断が実機で誤りだったことを意味する。その場合はST-1303の期待値と`.rc`を
調整し、本手順を再実行すること。**

---

## 完了条件

- [ ] ST-1301〜1303の静的テストが合格
- [ ] DT-1301は任意。実行できる場合は合格していること
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
