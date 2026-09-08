# クイックスタート検証ガイド: SDDプロジェクトのベース作成ツール

**Feature**: 001-sdd-project-bootstrap
**Date**: 2026-09-08

本フィーチャーがエンドツーエンドで動作することを確認する手順。仕様の「テスト方針」節が定める静的テスト（ST-001〜007）と動的テスト（DT-001〜009）を、実施可能な順序に並べたものである。

各項目の判定基準は [spec.md](./spec.md) の「テスト方針」、コントロールの仕様は [contracts/ui-contract.md](./contracts/ui-contract.md)、判定ロジックの根拠は [research.md](./research.md) を参照する。

---

## 前提条件

| 項目 | 内容 |
|---|---|
| OS | Windows 11 |
| ビルド | Visual Studio 2022（v143 ツールセット、MFC） |
| テスト対象フォルダ | `C:\work\test\test01`（VS2022 ウィザード生成の MFC ダイアログアプリ） |
| 必要なコマンド | `git`、`winget`。`uv`・`specify`・`code` は導入済みでもよい（スキップ経路の確認になる） |

**テスト対象フォルダの初期状態を確認する**:

```powershell
Get-ChildItem C:\work\test\test01 -Force -Name
```

`.git`・`.gitignore`・`.specify`・`.claude` が**存在しない**こと。存在する場合は「付録: テスト対象フォルダのリセット」を先に実施する。

---

## 手順1: ビルド（ST-001）

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" `
    MFCApplication1\MFCApplication1.sln /p:Configuration=Debug /p:Platform=x64
```

**期待される結果**: ビルドが成功し、新規追加コードに起因する警告が出力されないこと。

**全構成の確認**（ST-001 の完了条件）: `Configuration` を `Debug` / `Release`、`Platform` を `x64` / `x86` に変えた4通りすべてでビルドが通ること。

**プラットフォーム名の注意**: `.sln` に渡す場合は **`x86`** である。`Win32` を渡すと `error MSB4126: 指定されたソリューション構成 "Debug|Win32" は無効です` になる（実測）。`Win32` は `.vcxproj` を直接ビルドする場合のプラットフォーム名で、`.sln` はそれを `x86` というラベルで公開している。手順書には「MSBuild に渡すのは Win32」とあるが、それは vcxproj を直接指定する場合の話であり、`.sln` 経由では当てはまらない。

```powershell
# 4構成をまとめて確認する
$msb='C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe'
foreach($cfg in 'Debug','Release'){ foreach($plat in 'x64','x86'){
  $out = & $msb MFCApplication1\MFCApplication1.sln /p:Configuration=$cfg /p:Platform=$plat /t:Rebuild /v:minimal /nologo 2>&1
  "{0,-8} {1,-5} 警告={2} エラー={3}" -f $cfg,$plat,
    ($out | Select-String ' warning ' | Measure-Object).Count,
    ($out | Select-String ' error '   | Measure-Object).Count
}}
```

---

## 手順2: 静的検査（ST-002〜ST-005）

アプリケーションを実行せずに実施する。

| ID | 実施内容 | 期待される結果 |
|---|---|---|
| ST-002 | Visual Studio で「分析」→「コード分析の実行」を新規追加コードに対して実行 | 未初期化変数・リソースリーク・バッファ関連の指摘がない |
| ST-003 | 新規追加した各関数のコメントを確認 | 各関数がどの FR に対応するかを追跡できる |
| ST-004 | `MFCApplication1.rc` と `Resource.h` を確認 | [ui-contract.md](./contracts/ui-contract.md) 「1. ダイアログのコントロール」の8コントロールがすべて定義されている |
| ST-005 | ソース中の表示文字列を確認 | すべて日本語。ui-contract.md 「4. メッセージ文字列の契約」の9メッセージが定義されている |

**ST-004 の重点確認**: ラジオボタン `IDC_RADIO_KIND_MFC` / `IDC_RADIO_KIND_WEB` が**どちらも初期状態で未選択**であること。既定で選択済みだと DT-004 が実施できなくなる。

---

## 手順3: 未選択ガードの確認（DT-003, DT-004, DT-005）

**ここを最初の動的テストとする。** ファイルシステムを変更しない経路なので、以降のテストの前提を壊さずに実施できる。

### DT-003: フォルダ未選択で「生成」を押下する（最重要）

1. アプリケーションを起動する
2. パス表示欄が空であることを目視で確認する
3. 「参照...」を**押さずに**「生成」を押下する

**期待される結果** — 以下すべてを確認すること:

| # | 確認項目 |
|---|---|
| (a) | 「対象フォルダを選択してください。」という日本語メッセージが表示される |
| (b) | 結果一覧に手順が1行も表示されない（導入手順が1つも実行されていない） |
| (c) | どのフォルダにもファイルが作成・変更されない（下記コマンドで確認） |
| (d) | メッセージを閉じた後、「生成」ボタンが**押下可能な状態のまま**である |
| (e) | アプリケーションが異常終了しない |

(c) の確認:

```powershell
Get-ChildItem C:\work\test\test01 -Force -Name
```

`.git`・`.gitignore`・`.specify`・`.claude` が**依然として存在しない**こと。

(d) の確認方法: メッセージを閉じた後に「参照...」でフォルダを選択し、「生成」が押下できることを確かめる。押下できない場合、未選択エラーの経路でボタンを無効化したまま戻していない実装バグである（[ui-contract.md](./contracts/ui-contract.md) 「2. ボタンの有効・無効の契約」に違反）。

### DT-004: 種別未選択で「生成」を押下する

1. 「参照...」で `C:\work\test\test01` を選択する
2. 種別（MFCアプリ / Webアプリ）を**選ばずに**「生成」を押下する

**期待される結果**: 「プロジェクトの種別を選択してください。」が表示され、手順が1つも実行されず、フォルダが変化しない。

### DT-005: キャンセルしても選択が保持される

1. 「参照...」で `C:\work\test\test01` を選択する
2. 再度「参照...」を押し、フォルダ選択ダイアログを**キャンセル**する
3. パス表示欄を確認する

**期待される結果**: `C:\work\test\test01` が表示されたままである。空になっていないこと。この状態で「生成」を押しても未選択エラーにならないこと。

---

## 手順4: 診断の初期状態（DT-008, DT-009 前半）

1. `C:\work\test\test01` を選択した状態で「準備状況を診断」を押下する

**期待される結果**:

| 項目 | 期待される判定 |
|---|---|
| 1. ビルドが通る雛形がある | 達成（`test01.sln` を検出） |
| 2. Spec Kit の足場が同じ階層にある | 未達 |
| 3. コミットが1件以上入っている | 未達 |
| 4. テスト方針が決まっている | 判定不能 |
| 5. 憲法がある | 未達 |
| 6. CLAUDE.md が憲法を反映している | 未達 |
| 7. 技術スタックが記録されている | 判定不能 |

未達の各項目に、次に取るべき行動が表示されていること。

**DT-008（読み取り専用の確認）**: 診断の前後でフォルダが変化しないこと。

```powershell
Get-ChildItem C:\work\test\test01 -Force -Recurse | Measure-Object | Select-Object Count
```

診断の実行前後でこの件数が一致すること。

---

## 手順5: SDD化の実行（DT-001）

1. `C:\work\test\test01` を選択し、種別に「MFCアプリ」を選ぶ
2. 「生成」を押下する
3. 処理中、「生成」ボタンが無効化され、ダイアログが応答を保つことを確認する

**期待される結果**: 5手順が順に一覧へ追加され、それぞれ「成功」または「スキップ」となる。`uv`・`specify`・`code` が導入済みの環境では、最初の3手順が「スキップ」になる。

**完了後の確認**:

```powershell
Get-ChildItem C:\work\test\test01 -Force -Name
```

`.git`・`.gitignore`・`.specify`・`.claude` の4つが揃っていること。

```powershell
Push-Location C:\work\test\test01
git log --oneline
git rev-list --count HEAD
Pop-Location
```

コミットが1件以上あること。

処理完了後、「生成」ボタンが**有効に戻っている**こと（[ui-contract.md](./contracts/ui-contract.md) 「2. ボタンの有効・無効の契約」）。

---

## 手順6: 生成物の静的検査（ST-006, ST-007）

手順5の実行後に、生成されたファイルをアプリケーションを介さずに検査する。

### ST-006: `.gitignore` の内容

```powershell
Select-String -Path C:\work\test\test01\.gitignore -Pattern '---- Spec Kit ----','---- このプロジェクト固有 ----','\[Dd\]ebug/'
```

**期待される結果**: 3つのパターンすべてが見つかり、追記ブロックの目印行が**それぞれ1回だけ**出現すること。

### ST-007: 初回コミットの内容

```powershell
Push-Location C:\work\test\test01
git show --stat --name-only HEAD | Select-String -Pattern '\.vs/|\.vcxproj\.user|\.aps$'
Pop-Location
```

**期待される結果**: 1件も一致しないこと。Visual Studio の作業ファイルが初回コミットに含まれていないことを意味する。

```powershell
Push-Location C:\work\test\test01
git show --stat --name-only HEAD | Select-String -Pattern 'test01\.sln|test01\.cpp'
Pop-Location
```

**期待される結果**: ソリューションとソースは含まれていること。

---

## 手順7: べき等性の確認（DT-002）

1. 同じフォルダ・同じ種別で、再度「生成」を押下する

**期待される結果**: 5手順すべてが「スキップ」と表示される。「成功」が1件も出ないこと。

**副作用がないことの確認**:

```powershell
Push-Location C:\work\test\test01
git status --short
git rev-list --count HEAD
Pop-Location
```

**期待される結果**: `git status` に変更が出ないこと。コミット数が手順5の時点から増えていないこと。`.gitignore` の追記ブロックが重複していないこと（ST-006 を再実施して確認する）。

---

## 手順8: 診断の変化（DT-009 後半）

1. 再度「準備状況を診断」を押下する

**期待される結果**: 手順4の結果と比べて、以下が変化していること。

| 項目 | 手順4 | 手順8 |
|---|---|---|
| 2. Spec Kit の足場が同じ階層にある | 未達 | **達成** |
| 3. コミットが1件以上入っている | 未達 | **達成** |
| 5. 憲法がある | 未達 | **未達のまま** |
| 6. CLAUDE.md が憲法を反映している | 未達 | **未達のまま** |

項目5・6が未達のまま残るのは正しい挙動である。憲法の制定（`/speckit-constitution`）と `CLAUDE.md` の生成（`/init`）は Claude Code との対話が必要で、本ツールの対象外だからである。

**FR-022 の確認**: 完了案内に、VS Code で対象フォルダを開き `/init` を実行する手順が表示されていること。

---

## 手順9: 異常系（DT-006, DT-007）

### DT-006: 対象プロジェクトが存在しないフォルダ

```powershell
New-Item -ItemType Directory C:\work\test\empty01 -Force
```

1. `C:\work\test\empty01` を選択し、種別に「MFCアプリ」を選んで「生成」を押下する

**期待される結果**: ウィザードでプロジェクトを作成するよう促すメッセージが表示され、導入手順が1つも実行されない。**このフォルダに雛形が生成されないこと**（FR-007）。

```powershell
Get-ChildItem C:\work\test\empty01 -Force -Name
```

空のままであること。

### DT-007: 種別の不一致

1. `C:\work\test\test01` を選択し、種別に「**Webアプリ**」を選んで「生成」を押下する

**期待される結果**: 選択された種別とフォルダ内容が一致しない旨の警告が表示され、確認を求められる。「いいえ」を選ぶと処理が実行されないこと。

---

## 付録: テスト対象フォルダのリセット

手順5以降は `C:\work\test\test01` を破壊的に変更する。初期状態から再実行するには以下を実施する。

```powershell
Remove-Item C:\work\test\test01\.git -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item C:\work\test\test01\.specify -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item C:\work\test\test01\.claude -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item C:\work\test\test01\.gitignore -Force -ErrorAction SilentlyContinue
Get-ChildItem C:\work\test\test01 -Force -Name
```

`.git`・`.gitignore`・`.specify`・`.claude` が消えていること。ソリューション本体（`test01.sln` 等）は残っていること。

**注意**: `Remove-Item` の対象を取り違えるとソリューション本体を失う。実行前にパスを確認すること。

---

## 完了条件

以下がすべて満たされたとき、本フィーチャーは検証済みとする。

- [ ] ST-001〜ST-007 の静的テストがすべて合格
- [ ] DT-001〜DT-009 の動的テストがすべて合格
- [ ] 特に DT-003（フォルダ未選択での「生成」押下）の確認項目 (a)〜(e) がすべて合格
- [ ] 手順9で、雛形が1つも生成されないことを確認済み（FR-007 のスコープ境界）
