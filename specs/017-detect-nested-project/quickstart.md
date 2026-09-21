# クイックスタート（検証手順）: サブフォルダ内のウィザードプロジェクトを検出できるようにする

**Feature**: 017-detect-nested-project
**Date**: 2026-09-21

静的テスト ST-1701〜1702 は自動化する。動的テスト DT-1701（マーカー検出ロジックそのものの確認）は、
`TargetFolder::Read()`がフォルダ選択ダイアログを介さずに直接呼び出せる公開関数であることを利用し、
GUIを一切介さないスタンドアロンハーネスで完全に自動化する（009 DT-903と同様の`#include`方式）。
DT-1702（実際のGUI操作での確認）は、フォルダ選択ダイアログの自動操作ができないという既知の制限
（CLAUDE.md）により実機推奨とする。

**SC-1703（大規模フォルダでの体感速度）について**: 本フィーチャーは実測によるパフォーマンス検証手順を
設けない。[research.md](./research.md) R-1704の見積もり（深さ優先の早期リターンと`.git`除外だけで
通常規模の開発フォルダには十分）に基づく受容であり、憲章 原則V「測れないものは安全側に見積もり、
見積もりであることを明記する」を適用したものとして、ここに明記する。実機で体感できる遅延が確認された
場合は、その実測結果に基づいて別フィーチャーで対応する。

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

## 手順2: 静的テスト（ST-1701〜1702）

### ST-1701: FRによる追跡可能性

FR番号はコメントとして記載する設計のため、この検査は**コメント行を除外しない**（008 T010で判明した注意点）。

```powershell
$text = Get-Content MFCApplication1\TargetFolderState.cpp -Raw
1701..1705 | ForEach-Object {
  $n = "FR-$_"
  "{0}: {1}" -f $n, ([regex]::Matches($text, $n)).Count
}
```

**合格条件**: FR-1701〜1705がいずれも1件以上出現すること。

### ST-1702: 変更範囲が`TargetFolderState.cpp`1ファイルに限定されていること

```powershell
$git = (Get-Command git -ErrorAction SilentlyContinue).Source
if (-not $git) { $git = 'C:\Users\user\AppData\Local\GitHubDesktop\app-3.6.3\resources\app\git\cmd\git.exe' }
& $git diff --stat -- MFCApplication1/TargetFolderState.h MFCApplication1/MFCApplication1Dlg.cpp MFCApplication1/SetupRunner.cpp MFCApplication1/ProcessRunner.cpp
& $git diff --stat -- MFCApplication1/TargetFolderState.cpp
```

**合格条件**: 1つ目のコマンド（`.h`・`Dlg.cpp`・`SetupRunner.cpp`・`ProcessRunner.cpp`）の出力が空。
2つ目のコマンド（`TargetFolderState.cpp`）は非空（変更がある）こと（plan.md Structure Decision）。

---

## 手順3: DT-1701（マーカー検出ロジックの確認、スタンドアロンハーネス、完全自動）

`TargetFolder::Read()`は`CFolderPickerDialog`を経由せず、フォルダパスの文字列を直接引数に取る公開関数
であるため、GUIを一切起動せずに検証できる。

1. スクラッチパッドに検証用フォルダ構成を作る:

   ```powershell
   $root = "$env:TEMP\dt1701_$([guid]::NewGuid().ToString('N').Substring(0,8))"
   New-Item -ItemType Directory -Force "$root\case_nested\Wizard" | Out-Null
   New-Item -ItemType File "$root\case_nested\Wizard\App.sln" | Out-Null

   New-Item -ItemType Directory -Force "$root\case_direct" | Out-Null
   New-Item -ItemType File "$root\case_direct\App.vcxproj" | Out-Null

   New-Item -ItemType Directory -Force "$root\case_empty" | Out-Null

   New-Item -ItemType Directory -Force "$root\case_git_only\.git" | Out-Null
   New-Item -ItemType File "$root\case_git_only\.git\fake.sln" | Out-Null

   New-Item -ItemType Directory -Force "$root\case_deep\a\b\c" | Out-Null
   New-Item -ItemType File "$root\case_deep\a\b\c\package.json" | Out-Null

   $root
   ```

2. スクラッチパッドに検証用の`.cpp`を作成し、`TargetFolderState.cpp`を（別ファイルとしてリンクするので
   はなく）`#include "TargetFolderState.cpp"`で直接取り込むスタンドアロンハーネスをビルドする
   （009 DT-903と同じ手法）。ハーネスの`main()`から`TargetFolder::Read(folder)`を各ケースについて
   呼び出し、`hasProjectMarker`の値を出力する。
3. 5ケースそれぞれについて`TargetFolder::Read()`を呼び、`hasProjectMarker`を確認する:
   - `case_nested`（1階層下のサブフォルダに`.sln`）→ `true`（FR-1701、User Story 1）
   - `case_direct`（直下に`.vcxproj`、従来どおりの構成）→ `true`（回帰なし、Acceptance Scenario 2）
   - `case_empty`（マーカーなし）→ `false`（FR-1705、User Story 2）
   - `case_git_only`（`.git`内部にのみ`.sln`類似ファイル）→ `false`（FR-1703、Edge Case）
   - `case_deep`（3階層下に`package.json`）→ `true`（FR-1701、Edge Case「さらに深い場合」）
4. 検証後、`$root`をスクラッチパッドごと削除する。

**合格条件**: 5ケースすべてで期待どおりの`hasProjectMarker`値が得られること。

---

## 手順4: DT-1702（実際のGUI操作での確認、実機推奨）

フォルダ選択ダイアログの自動操作ができないため、利用者による実機実行を推奨する。

1. `C:\work\test\test01`のようなテスト対象フォルダの直下にあるプロジェクトファイルを、いったん
   `test01\MFCApplication1\`のようなサブフォルダへ移動する（Visual Studioウィザードが作る典型的な
   構造を再現する）
2. `test01`（親フォルダ、直下にはプロジェクトファイルが無い状態）を対象フォルダとして選び、「生成」を
   クリックする
3. 「対象のプロジェクトが見つかりません」で中止されず、手順1（uv導入確認）以降が実行されることを
   確認する（SC-1701）
4. 生成完了後、`.gitignore`・`.specify`・`.claude`・`.git`が`test01`（利用者が選んだ親フォルダ）
   **直下**に生成されており、マーカーが実際にあった`test01\MFCApplication1\`（サブフォルダ）には
   生成されていないことを確認する（FR-1704）
5. 別途、`C:\work\test\empty01`（プロジェクトを一切含まない空フォルダ）を対象フォルダとして選び、
   「生成」をクリックして、これまでどおり「対象のプロジェクトが見つかりません」で中止されることを
   確認する（SC-1702、回帰確認）

**合格条件**: 手順3で誤って中止されないこと。手順4で生成先が対象フォルダ直下であること。手順5で従来どおり中止されること。

---

## 完了条件

- [ ] ST-1701〜1702の静的テストがすべて合格
- [ ] DT-1701（スタンドアロンハーネス）を実行し、5ケースすべて合格していること
- [ ] DT-1702を実機で確認できる環境がある場合は実行し合格していること（環境依存のため必須ではないが推奨）
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
