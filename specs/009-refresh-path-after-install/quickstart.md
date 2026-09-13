# クイックスタート（検証手順）: 新規インストール直後のPATH未反映を解消する

**Feature**: 009-refresh-path-after-install
**Date**: 2026-09-13

静的テスト ST-901〜902 は自動化する。動的テストのうち DT-901（実際のSDD化シナリオ）はフォルダ選択
ダイアログを伴うため自動化できず利用者による実機実行を推奨する（008と同じ制約）。DT-902（PATH追記の
汎用的な仕組みそのものの確認）は、008で確立した「`ProcessRunner::Run()`をGUIを介さず直接呼ぶ
スタンドアロンハーネス」の手法を流用し、レジストリを一時的に操作することで自動化する。DT-903
（レジストリ読み取り失敗時の堅牢性確認）は、実際のレジストリを壊さずに内部ヘルパーを直接呼び出す
ホワイトボックス方式で自動化する。

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

## 手順2: 静的テスト（ST-901〜902）

### ST-901: `ProcessRunner.cpp`にPATH追記の構成要素が揃っており、他ファイルは無変更

```powershell
$codeOnly = (Get-Content MFCApplication1\ProcessRunner.cpp) | Where-Object { $_.TrimStart() -notmatch '^//' }
$text = $codeOnly -join "`n"
foreach ($c in 'RegGetValue','SetEnvironmentVariable','GetEnvironmentVariable','HKEY_CURRENT_USER','HKEY_LOCAL_MACHINE') {
  "{0,-25}: {1}" -f $c, ([regex]::Matches($text, [regex]::Escape($c))).Count
}
$git = (Get-Command git -ErrorAction SilentlyContinue).Source
if (-not $git) { $git = 'C:\Users\user\AppData\Local\GitHubDesktop\app-3.6.3\resources\app\git\cmd\git.exe' }
& $git diff --stat -- MFCApplication1/SetupRunner.cpp MFCApplication1/ProcessRunner.h
```

**合格条件**: 5項目すべて1件以上。`SetupRunner.cpp`・`ProcessRunner.h`の差分はいずれも空であること（FR-906、008の契約の継続）。

### ST-902: FRによる追跡可能性

FR番号はコメントとして記載する設計のため、この検査は**コメント行を除外しない**（008 T010で判明した注意点）。

```powershell
$text = Get-Content MFCApplication1\ProcessRunner.cpp -Raw
901..906 | ForEach-Object {
  $n = "FR-$_"
  "{0}: {1}" -f $n, ([regex]::Matches($text, $n)).Count
}
```

**合格条件**: FR-901〜906がいずれも1件以上出現すること。

---

## 手順3: DT-901（実際のSDD化シナリオ、実機推奨）

008のDT-801と同じ制約（`m_targetPath`はフォルダ選択ダイアログ経由でしか設定できずUI自動化できない）
のため、利用者による実機実行を推奨する。

1. uvとspecify CLIの両方が未導入の環境（または既にuvのみ導入済みでspecify CLIが未導入の環境）を用意する
2. 対象フォルダを選択し「生成」を押す
3. 手順1（uv導入）が成功した直後、手順2（specify CLI導入）が「uvが見つかりません」で失敗しないことを確認する（SC-901）
4. 手順2が成功した場合、続く手順4（Spec Kit足場配置）が「specifyが見つかりません」で失敗しないことを確認する（SC-902）

**合格条件**: 新規インストールされたツールが原因の「見つかりません」失敗が発生しないこと。

---

## 手順4: DT-902（PATH追記の仕組みそのものの確認、スタンドアロンハーネス＋レジストリ一時操作）

**この手順はユーザー環境変数PATHを一時的に変更する。手順の最後で必ず元に戻すこと。**

1. 検証用の実行ファイルを配置する一時ディレクトリを作る（例: スクラッチパッド配下の`fakebin`）。
   その中に、実行されたことが分かる最小限のコンソールexe（`fake_tool.exe`、`printf("FOUND\n"); return 0;`
   程度のもの）を、008で使ったのと同じ`cl.exe`手順でビルドして置く
2. **現在のユーザーPATH（`HKCU\Environment\Path`）の値を読み取り、バックアップとして保存する**
   （`[Environment]::GetEnvironmentVariable('Path','User')`）
3. `[Environment]::SetEnvironmentVariable('Path', "$backup;<fakebinのパス>", 'User')`で、
   レジストリのユーザーPATHにだけ`fakebin`を追加する。**この時点でこのPowerShellセッション自身の
   `$env:PATH`には反映されない**（.NETの`SetEnvironmentVariable('User')`はレジストリを書き換える
   だけで、既に起動中のプロセスの環境には影響しない）。これが「新規インストール直後、既存プロセスの
   PATHには乗らない」という本フィーチャーが対象とする状況そのものを再現している
4. このPowerShellセッション（`fakebin`を含まない古いPATHを持つ）から、008で作成したスタンドアロン
   ハーネス（`ProcessRunner::Run()`を直接呼ぶ、[008/quickstart.md](../008-fix-external-command-hang/quickstart.md)
   手順5参照）を実行する。ハーネスの`main()`内で、`ProcessRunner::Run()`を呼ぶ**前**に
   `GetEnvironmentVariable(L"TEMP", ...)`等、PATH以外の環境変数を1つ読んで控えておく
5. `ProcessRunner::Run(_T("fake_tool.exe"))`をフルパスなしで呼ぶ。**修正が効いていれば**、`Run()`内部で
   レジストリのユーザーPATHを読み直して`fakebin`が追記され、`fake_tool.exe`が見つかって起動できる
   （`launched=1`かつ出力に`FOUND`が含まれる）
6. `Run()`が返った直後、手順4で控えた`TEMP`等の値をもう一度読み、**呼び出し前後で値が一切変化して
   いない**ことを確認する（FR-905）
7. **確認後、必ず`[Environment]::SetEnvironmentVariable('Path', $backup, 'User')`でユーザーPATHを
   手順2で保存した値に戻す**。`fakebin`ディレクトリもスクラッチパッドごと削除してよい

**合格条件**: 手順5で`fake_tool.exe`が見つかり実行できること（`launched=1`、出力に`FOUND`を含む）。
手順6でPATH以外の環境変数（`TEMP`等）が呼び出し前後で完全に一致すること（FR-905）。手順7でユーザーPATHの
レジストリ値が手順2の時点と完全に一致することを確認する。

```powershell
$after = [Environment]::GetEnvironmentVariable('Path','User')
"復元確認: $($after -eq $backup)"
```

---

## 手順5: DT-903（無効なレジストリキーでも異常終了しないこと、ホワイトボックス確認）

US2（FR-903: レジストリ読み取り失敗時も既存動作を壊さない）の検証。実際のレジストリを本物の失敗状態に
する安全な方法はない（権限操作等はリスクが高い）ため、内部ヘルパー関数を直接呼び出すホワイトボックス
方式で確認する。**レジストリの書き換えは一切行わない。**

1. スクラッチパッドに検証用の`.cpp`を作成し、`ProcessRunner.cpp`を（別ファイルとしてリンクするのではなく）
   `#include "ProcessRunner.cpp"`で直接取り込む。これにより、無名名前空間内にあるレジストリ読み取りの
   内部ヘルパー関数が同一翻訳単位内から直接呼び出せるようになる（実装コード自体は一切変更しない）
2. 存在しないレジストリサブキー（例: `Environment\NonExistentSubkeyForTest009`）を指定してそのヘルパーを
   呼び出し、例外を投げず、クラッシュせず、空文字列相当の結果が返ることを確認する
3. 実在するサブキー（`Environment`）を指定して同じヘルパーを呼び出し、通常どおり値が返ることを確認する
   （異常系の確認だけでなく、正常系が壊れていないことも合わせて確認する）
4. **FR-902（`%VAR%`展開）の確認**: `HKEY_CURRENT_USER`配下に一時的なテスト用サブキー
   （例: `Software\SpecKit009Test`）を作成し、`REG_EXPAND_SZ`型の値（例: `%TEMP%\dummy`）を書き込む。
   ヘルパーをこのテストキーに対して呼び出し、返ってきた文字列に`%`が残っておらず、実際のパスへ
   展開されていることを確認する。確認後、このテスト用サブキーを`RegDeleteKey`等で必ず削除する
   （`HKCU\Environment`本体には一切触れない、独立したテスト専用キーのため復元操作は削除のみでよい）

**合格条件**: 手順2・3のいずれもクラッシュ・例外なく完了し、手順2は空、手順3は非空の結果を返すこと。
手順4で展開後の文字列に`%`が含まれておらず、`%TEMP%`が実際の一時ディレクトリのパスに置き換わっている
こと。手順4のテスト用サブキーが確認後に削除されていること。

---

## 完了条件

- [ ] ST-901〜902の静的テストがすべて合格
- [ ] DT-901を実機で確認できる環境がある場合は実行し合格していること（環境依存のため必須ではないが推奨）
- [ ] DT-902を実行し合格していること。**手順7でユーザーPATHのレジストリ値が完全に復元されていること**
- [ ] DT-903を実行し合格していること
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
