# クイックスタート（検証手順）: specify CLI導入後のPATH未登録を解消する

**Feature**: 010-specify-path-registration
**Date**: 2026-09-13

静的テスト ST-1001 は自動化する。動的テスト DT-1001 は、`uv tool update-shell`という**実在のuv公式
コマンド**をレジストリ操作の安全なリハーサル環境で直接呼び、その効果（PATH登録）を確認する形で
自動化する（実際にspecify-cliをインストール/アンインストールする必要はない）。DT-1002（GUIでの
完全なエンドツーエンド確認）はフォルダ選択ダイアログと、specify CLIを実際にアンインストールする
操作を伴うため、利用者による任意の手動実行とする。

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

## 手順2: 静的テスト（ST-1001）

### ST-1001: `uv tool update-shell` が成功パスに正しく組み込まれている

```powershell
$codeOnly = (Get-Content MFCApplication1\SetupRunner.cpp) | Where-Object { $_.TrimStart() -notmatch '^//' }
$text = $codeOnly -join "`n"
$m = [regex]::Match($text, 'RunInstallSpecify[\s\S]*?install\.Succeeded\(\)[\s\S]{0,400}?uv tool update-shell[\s\S]{0,300}?StepResult::Success')
"install成功 → uv tool update-shell → Finish(Success) の順になっている: $($m.Success)"

$git = (Get-Command git -ErrorAction SilentlyContinue).Source
if (-not $git) { $git = 'C:\Users\user\AppData\Local\GitHubDesktop\app-3.6.3\resources\app\git\cmd\git.exe' }
& $git diff --stat -- MFCApplication1/ProcessRunner.cpp MFCApplication1/ProcessRunner.h
```

**合格条件**: 正規表現マッチが`True`であること（`uv tool install`の成功判定後、`Finish`で成功と
確定する前に`uv tool update-shell`が呼ばれている）。`ProcessRunner.cpp`・`ProcessRunner.h`の差分は
008・009からの差分のみ（010時点でもコミットされていないため両方が含まれる）で、010による追加差分が
ないこと（`git diff`の出力が009実装完了時点から変わらないこと）。

### FRによる追跡可能性

FR番号はコメントとして記載する設計のため、コメント行を除外しない（008 T010・009 ST-902の教訓）。

```powershell
$text = Get-Content MFCApplication1\SetupRunner.cpp -Raw
1001..1005 | ForEach-Object {
  $n = "FR-$_"
  "{0}: {1}" -f $n, ([regex]::Matches($text, $n)).Count
}
```

**合格条件**: FR-1001〜1005がいずれも1件以上出現すること。

### FR-1002・FR-1004・SC-1003 の静的確認（追加）

```powershell
$codeOnly = (Get-Content MFCApplication1\SetupRunner.cpp) | Where-Object { $_.TrimStart() -notmatch '^//' }
$text = $codeOnly -join "`n"

# FR-1002: スキップ分岐（既に導入済み）に uv tool update-shell が入り込んでいないこと
$skip = [regex]::Match($text, 'probe\.Succeeded\(\)[\s\S]{0,150}?StepResult::Skipped[\s\S]{0,50}?return;')
"スキップ分岐にupdate-shellが無い: $(-not ($skip.Value -match 'uv tool update-shell'))"

# SC-1003: update-shellの呼び出しから成功確定までの間に if 分岐が無い（結果を判定に使っていない証拠）
$between = [regex]::Match($text, 'uv tool update-shell[\s\S]{0,300}?StepResult::Success')
"update-shellの結果を判定するif文が無い: $(-not ($between.Value -match '\bif\s*\('))"

# FR-1004: SetupRunner.cpp内の変更が RunInstallSpecify 関数（109〜133行目付近）に閉じていること
$git = (Get-Command git -ErrorAction SilentlyContinue).Source
if (-not $git) { $git = 'C:\Users\user\AppData\Local\GitHubDesktop\app-3.6.3\resources\app\git\cmd\git.exe' }
& $git diff -U0 -- MFCApplication1/SetupRunner.cpp | Select-String '^@@'
```

**合格条件**: 上2行がいずれも`True`。`git diff -U0`のハンク（`@@`行）が示す変更行番号が、すべて
`RunInstallSpecify`関数の範囲内（008・009時点の行番号から見て110行目台〜130行目台の範囲）に
収まっていること。他の関数（`RunInstallUv`・`RunInstallVsCodeExt`・`RunInitSpecKit`・`RunInitGit`・
`RunAll`等）に変更が及んでいないこと。

---

## 手順3: DT-1001（`uv tool update-shell`のPATH登録効果を確認、安全なレジストリ操作）

**この手順はユーザー環境変数PATHを一時的に変更する可能性がある。手順の最後で必ず元の状態に戻すこと。**
specify-cliのインストール状態そのものには一切触れない。

1. 現在のユーザーPATH（`[Environment]::GetEnvironmentVariable('Path','User')`）を読み取り、
   バックアップとして保存する
2. バックアップの中に`.local\bin`（uvの既定インストール先）が含まれているか確認する
   - 含まれている場合: 検証のため一時的に取り除いたPATH文字列を作り、`'User'`スコープへ設定する
   - 含まれていない場合: そのまま次へ進む
3. `uv.exe`（`C:\Users\<ユーザー>\AppData\Local\Microsoft\WinGet\Packages\astral-sh.uv_*\uv.exe`、
   または`where.exe uv`で解決できるパス）を使い、`& $uvExe tool update-shell`を実行する
4. 実行後のユーザーPATHを読み取り、`.local\bin`が含まれるようになったことを確認する
5. **手順1で保存したバックアップへユーザーPATHを戻す**（手順2で一時的に取り除いていた場合は元の
   状態に、そうでない場合は同じ値に戻すだけで実質変化なし）

```powershell
$after = [Environment]::GetEnvironmentVariable('Path','User')
"復元確認: $($after -eq $backup)"
```

**合格条件**: 手順4で`.local\bin`が新たに含まれるようになること。手順5でユーザーPATHが手順1の
バックアップと完全に一致すること。

---

## 手順4: DT-1003（スキップパスの回帰確認、ホワイトボックス・specify-cliへの変更なし）

SC-1002（specify CLIが既に導入済みの環境での回帰なし）を確認する。008・009で確立した
「`#include "対象.cpp"`で内部関数を直接呼ぶ」ホワイトボックス方式を使う。**specify-cliの
インストール状態には一切触れない**（この開発機は既にspecify-cliが導入済みのため、この手順は
自然に「スキップ」分岐を通る）。

1. スクラッチパッドに検証用の`.cpp`を作成し、`SetupRunner.cpp`を`#include`で直接取り込む
2. `ISetupProgressSink`の最小実装（`OnStepFinished`で受け取った`SetupStep`を保存するだけ）を用意する
3. `SetupRunner::CreateSteps()`で取得した手順一覧から`StepId::InstallSpecify`の`SetupStep`を取り出し、
   `RunInstallSpecify(step, &sink)`を直接呼ぶ
4. 結果が`StepResult::Skipped`であり、理由が`"specify CLI は導入済みです。"`と一致することを確認する

**合格条件**: 結果が`Skipped`、理由文字列が完全一致すること。これはこの開発機の現在の状態（specify-cli
導入済み）でのみ意味を持つ確認であり、specify-cliが未導入の環境ではこの手順自体が成立しない（その
場合はDT-1002で新規インストール側の経路を確認する）。

---

## 手順5: DT-1002（GUIでの完全なエンドツーエンド確認、任意・利用者による手動実行）

**この手順は実際にspecify-cliをアンインストールする。実行するかどうかは利用者の判断に委ねる。**

1. `uv tool uninstall specify-cli`を実行し、意図的にspecify CLIを未導入の状態に戻す
2. アプリを起動し、対象フォルダを選択して「生成」を押す
3. 手順2（specify CLI導入）が成功し、続く手順4（Spec Kit足場配置）が「specifyが見つかりません」で
   失敗しないことを確認する（SC-1001）
4. 確認後、必要であれば`uv tool install specify-cli`で元の状態に戻す

**合格条件**: 手順3で「specifyが見つかりません」という失敗が発生しないこと。

---

## 完了条件

- [ ] ST-1001の静的テストが合格（FR-1002・FR-1004・SC-1003の追加確認を含む）
- [ ] DT-1001を実行し合格していること。**手順5でユーザーPATHが完全に復元されていること**
- [ ] DT-1003を実行し合格していること（SC-1002、スキップパスの回帰確認）
- [ ] DT-1002は任意。実行できる場合は合格していること
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
