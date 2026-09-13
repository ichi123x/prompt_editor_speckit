# クイックスタート（検証手順）: 外部コマンドの無応答ハング修正

**Feature**: 008-fix-external-command-hang
**Date**: 2026-09-13

静的テスト ST-801〜803 は自動化する。動的テスト DT-801（実際のuv導入シナリオ）は実機で再現できる範囲で
自動的に確認できるが、環境依存（winget の同意状態は一度受理すると恒久的に残る）のため**利用者による
確認を推奨**する。DT-802（無応答コマンドの汎用タイムアウト確認）は、検証専用の一時的なコマンド差し替え
を伴うため手動実行とする。

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

**動作確認に使う実行ファイル**: `MFCApplication1\x64\Debug\MFCApplication1.exe`（Debug構成で可、ユーザー指示）。

---

## 手順2: 静的テスト（ST-801〜803）

コメント行を対象から除外すること（CLAUDE.md — 過去に004・005で誤判定が発生している）。以下の正規表現は
`//` 始まりの行を除外する簡易フィルタを使う。

### ST-801: winget コマンドラインに必要な3フラグが揃っている

```powershell
Select-String -Path MFCApplication1\SetupRunner.cpp -Pattern 'winget install' -Context 0,0 |
  ForEach-Object { $_.Line.Trim() }
```

**合格条件**: 出力の1行に `--accept-source-agreements`・`--accept-package-agreements`・`--source winget` の
3つがすべて含まれていること。

### ST-802: FR による追跡可能性

FR番号はコメントとして記載される設計（原則IV）のため、この検査は**コメント行を除外しない**
（コメント行を除外するのはST-803のようにAPIの実使用を検査する場合であり、除外すると
FRコメント自体を検出できなくなる — ST-802固有の注意点）。

```powershell
$files = 'MFCApplication1\SetupRunner.cpp','MFCApplication1\ProcessRunner.cpp'
foreach ($f in $files) {
  $text = Get-Content $f -Raw
  801..806 | ForEach-Object {
    $n = "FR-$_"
    "{0,-30} {1}: {2}" -f $f, $n, ([regex]::Matches($text, $n)).Count
  }
}
```

**合格条件**: FR-801（winget フラグ）は `SetupRunner.cpp` に、FR-802・FR-804（タイムアウト監視・ジョブ
オブジェクト）は `ProcessRunner.cpp` に、それぞれ1件以上コメントとして現れること。FR-803・FR-805・FR-806
は「既存動作の維持」を表す要件のため、関連コメントに1箇所ずつまとめて記載されていればよい。

### ST-803: `ProcessRunner.cpp` にジョブオブジェクトと監視スレッドの構成要素が揃っている

```powershell
$codeOnly = (Get-Content MFCApplication1\ProcessRunner.cpp) | Where-Object { $_.TrimStart() -notmatch '^//' }
$text = $codeOnly -join "`n"
$checks = 'CreateJobObject','AssignProcessToJobObject','JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE',
          'CREATE_SUSPENDED','ResumeThread','CreateThread','TerminateJobObject'
foreach ($c in $checks) {
  "{0,-40}: {1}" -f $c, ([regex]::Matches($text, [regex]::Escape($c))).Count
}
```

**合格条件**: 全項目が1件以上。`ProcessRunner.h` のシグネチャ（引数・戻り値・デフォルト値）が変更されて
いないことも確認する。

```powershell
git diff --stat -- MFCApplication1/ProcessRunner.h
```

**合格条件**: 出力が空（無変更）であること。

---

## 手順3: 既存機能の回帰確認

既存の5手順（uv導入・specify CLI導入・VS Code拡張導入・Spec Kit足場配置・Git初期化）の呼び出しコードが
`ProcessRunner::Run()` の呼び出し以外で変更されていないことを確認する。

```powershell
git diff --stat -- MFCApplication1/SetupRunner.cpp
```

**合格条件**: `winget install` のコマンドライン文字列（ST-801の1行）以外に差分がないこと。

対象フォルダを `C:\work\test\test01`（uvが既に導入済みの状態）で「生成」を実行し、5手順すべてが従来
どおり成功またはスキップで完了することを確認する（SC-803）。

---

## 手順4: 動的テスト DT-801（実際のuv導入シナリオ、実機推奨）

winget の `msstore` ソース利用規約に**未同意**の環境でのみ再現するため、既に同意済みの開発機では
効果を直接確認しにくい。同意状態を確認・リセットする一般的な手段はなく、環境依存性が高いため、
**利用者の実機（未同意の状態、または新しいユーザーアカウント）での確認を推奨**する。

1. `winget list` を一度も対話的に完了させたことのない環境、またはuvが未導入の新しい環境を用意する
2. 空の対象フォルダ（例: `C:\work\test\empty01` 相当の新規フォルダ）を用意し、アプリで選択する
3. 「生成」を押す
4. **手順1（uv導入）が、ユーザー入力を求めることなく、成功・スキップ・失敗のいずれかで確定すること**
   を確認する（SC-801）
5. 結果確定後、`winget.exe` および付随する `conhost.exe` が残存していないことを確認する

```powershell
Get-Process winget,conhost -ErrorAction SilentlyContinue |
  Where-Object { (Get-Date) - $_.StartTime -lt [TimeSpan]::FromMinutes(15) }
```

**合格条件**: 手順4が数分〜10分程度（既存タイムアウト値のオーダー）で確定し、無期限に「導入処理を実行して
います...」のまま止まらないこと。手順5でプロセスが残存していないこと。ここでタスクマネージャー等での
手動終了が一切不要であることも、あわせてSC-804の裏付けとなる。

---

## 手順5: 動的テスト DT-802（無応答コマンドの汎用タイムアウト確認、スタンドアロンハーネス）

**設計変更の経緯**: 当初は `SetupRunner.cpp` を一時的に書き換えてGUIから「生成」を押す案だったが、
`m_targetPath` は `CFolderPickerDialog::DoModal()` の戻り値からのみ設定され（[MFCApplication1Dlg.cpp:425](../../MFCApplication1/MFCApplication1Dlg.cpp#L425)）、
フォルダ選択ダイアログはCLAUDE.mdの制約により合成入力を受け付けないため、この方法はそもそも自動化できない。
そこで、`ProcessRunner::Run()` をGUIを介さず直接呼び出すスタンドアロンの検証用exeを**スクラッチパッドに**
作成する（**リポジトリには追加しない**）。spec.md User Story 2 の独立したテスト方法定義
（「ダミープロセスを短いタイムアウト値で `ProcessRunner` 経由で実行する」）と厳密に一致する方法でもある。

1. スクラッチパッドに検証用の `.cpp`（例: `pr_harness.cpp`）を作成する。`MFCApplication1/framework.h` を
   include してから `ProcessRunner.h` を include し、`main()` で以下の2パターンを `ProcessRunner::Run()`
   に直接渡す:
   - 通常完了: `cmd.exe /c echo hello`（タイムアウト5000ms 程度）
   - 無応答の模擬: `cmd.exe /c ping -n 30 127.0.0.1 >NUL`（タイムアウト3000ms）。
     **注意**: このツールの実行環境ではstdinがNULデバイスに固定されているため、`cmd /c pause` のような
     「標準入力待ち」コマンドはEOFを即座に受け取って正常終了してしまい、無応答を再現できない。
     出力そのものを一切生成せず、タイムアウトより長く動き続けるコマンド（上記のping）を使うこと
   - それぞれ `GetTickCount64()` で実測経過時間を測り、`ProcessResult` の `launched`・`exitCode`・
     `Succeeded()`・`output` を表示する
2. Visual Studio の `vcvars64.bat` で環境を整えたうえで、`cl.exe /EHsc /std:c++17 /MD /D_UNICODE /DUNICODE
   /D_AFXDLL /I"MFCApplication1" pr_harness.cpp "MFCApplication1\ProcessRunner.cpp" /Fe:pr_harness.exe`
   でビルドする（`ProcessRunner.cpp` 自体は無変更のまま参照するだけで、リポジトリへの変更は一切生じない）
3. 生成した exe を実行し、次を確認する:
   - 通常完了パターン: `launched=1`・`exitCode=0`・`Succeeded()=true`・`output` に期待どおりの文字列が入っている
   - 無応答パターン: `launched=1`・`exitCode=258`（`WAIT_TIMEOUT`）・`Succeeded()=false`・実測経過時間が
     タイムアウト値から大きく超過していない（SC-802）
4. 実行直後、`cmd.exe`・`ping.exe` の残存プロセスがないことを確認する（`ping.exe` はジョブオブジェクトの
   割り当てが正しく機能していれば `cmd.exe` の子として一緒に終了する。ここが本フィーチャーの核心）

```powershell
Get-Process ping -ErrorAction SilentlyContinue
```

**合格条件**: 無応答パターンの実測経過時間がタイムアウト値と同オーダー（数十ms〜数百ms超過程度、
無期限ではない）であり、`exitCode=258`・`Succeeded()=false` であること。実行後に `ping.exe` が
残存していないこと（`cmd.exe` 単体を `TerminateProcess` するだけでは孫プロセスの `ping.exe` が
残存し得るため、ここが通ることがジョブオブジェクトの効果を直接裏付ける）。通常完了パターンも
問題なく成功すること（SC-803 の一部を兼ねる、回帰なしの確認）。

---

## 完了条件

- [ ] ST-801〜803 の静的テストがすべて合格
- [ ] 手順3（既存機能の回帰確認）が合格
- [ ] DT-801 を実機で確認できる環境がある場合は実行し、合格していること（環境依存のため必須ではないが推奨）
- [ ] DT-802 を手動実行し、合格していること。検証用の一時的なコード変更が最終的に残っていないこと
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
