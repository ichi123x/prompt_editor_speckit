# クイックスタート（検証手順）: 失敗理由が装飾バナーに埋もれて見えなくなる問題を解消する

**Feature**: 011-fix-error-message-truncation
**Date**: 2026-09-13

静的テスト ST-1101 は自動化する。動的テスト DT-1101 は、008・010で確立した「対象の`.cpp`を
`#include`して内部関数を直接呼ぶ」ホワイトボックス方式で、`Brief()`に合成した文字列を直接渡して
自動化する。実際に`specify`等の外部コマンドを実行する必要はない。

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

## 手順2: 静的テスト（ST-1101）

### ST-1101: `Brief()`のシグネチャ不変・FRの追跡可能性・変更範囲

```powershell
$git = (Get-Command git -ErrorAction SilentlyContinue).Source
if (-not $git) { $git = 'C:\Users\user\AppData\Local\GitHubDesktop\app-3.6.3\resources\app\git\cmd\git.exe' }

Write-Host "=== Brief()のシグネチャが変更されていないこと ==="
Select-String -Path MFCApplication1\SetupRunner.cpp -Pattern 'CString Brief\(const CString& output, int maxLength = 160\)'

Write-Host "`n=== ProcessRunner.cpp/.h に011による追加差分が無いこと ==="
& $git diff --stat -- MFCApplication1/ProcessRunner.cpp MFCApplication1/ProcessRunner.h

Write-Host "`n=== FRによる追跡可能性(コメント行を除外しない) ==="
$textRaw = Get-Content MFCApplication1\SetupRunner.cpp -Raw
foreach ($i in 1101..1105) {
  $n = "FR-$i"
  "{0}: {1}" -f $n, ([regex]::Matches($textRaw, $n)).Count
}

Write-Host "`n=== SetupRunner.cpp内の変更が Brief() 関数の範囲に閉じていること ==="
& $git diff -U0 -- MFCApplication1/SetupRunner.cpp | Select-String '^@@'
```

**合格条件**: `Brief()`のシグネチャ行が変更前と一致する（引数・既定値が同じ）こと。
`ProcessRunner.cpp`・`ProcessRunner.h`の差分が008・009からの差分のみ（011による追加差分がない）こと。
FR-1101〜1105がいずれも1件以上出現すること。`git diff`のハンクが`Brief()`関数の行範囲
（008〜010時点で25〜37行目）に収まっており、他の関数（`RunInstallUv`等）に変更が及んでいないこと。

---

## 手順3: DT-1101（`Brief()`の整形結果を直接確認、ホワイトボックス）

`SetupRunner.cpp`を`#include`で直接取り込み、`Brief()`を合成した文字列で直接呼ぶ。実際の外部コマンド
は一切実行しない。

1. スクラッチパッドに検証用の`.cpp`を作成し、`SetupRunner.cpp`を`#include`で直接取り込む
2. **ケースA（装飾＋エラー、実機で確認した`specify init`の実際の失敗出力そのもの）**: `/speckit-analyze`
   の指摘を受けて実機で取得した実際の`specify init`出力（[research.md](./research.md) R-1101・R-1102）
   をそのまま合成文字列として`Brief()`に渡す。**重要**: 単に「ロゴ＋`claude not found`」のような
   単純化した文字列を使ってはならない。実際の出力は「見出し（Agent Detection Error）→エラー本文
   （claude not found）→説明文（Install from: ...）→注意書き（Tip: ...）」という**エラー見出しの
   後に160文字を超える説明文が続く**構造になっており、この構造を再現しないと、末尾優先など誤った
   実装でも見かけ上テストが通ってしまう（`/speckit-analyze`で実際に確認された失敗モード）。
   戻り値に`"claude not found"`が含まれることを確認する（SC-1101）
3. **ケースB（装飾なしの短い失敗メッセージ、既存の典型例を模した合成文字列）**: 例えば
   `"git のユーザー設定 (user.name / user.email) が必要です。"`（`maxLength`以下の長さ）を
   `Brief()`に渡す。戻り値が入力とまったく同じ（省略記号が付かない）であることを確認する（SC-1102）
4. **ケースC（装飾のみ、エラー本文が全く無い合成文字列）**: 罫線・ブロック文字だけで構成された
   文字列を`Brief()`に渡す。戻り値が空文字列にならず、何らかの文字列（装飾除去前の元テキストを
   先頭から切り詰めたもの）が返ることを確認する（FR-1103のフォールバック）

**合格条件**: ケースAで`"claude not found"`が戻り値に含まれること（[research.md](./research.md) R-1102
で実測済み: 最後の段落・214文字を先頭から160文字に切り詰めた結果に含まれる）。ケースBで入力と戻り値が
完全一致すること。ケースCで戻り値が空文字列でないこと。いずれのケースでも戻り値の長さが`maxLength`
から大きく超過しないこと（省略記号を含めてもおよそ163文字程度、FR-1104）。

---

## 完了条件

- [ ] ST-1101の静的テストが合格
- [ ] DT-1101の3ケース（A・B・C）すべてが合格
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
