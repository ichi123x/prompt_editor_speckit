# クイックスタート検証ガイド: プロジェクト種別選択の廃止

**Feature**: 002-remove-kind-selection
**Date**: 2026-09-08

本フィーチャーは既存実装からの**削除**が中心である。したがって検証の重点は「消したものが消えたか」より、**「消し過ぎていないか」**にある。動的テストの半分（DT-105〜DT-108）を回帰確認に充てているのはそのためである。

判定基準は [spec.md](./spec.md) の「テスト方針」、契約は [contracts/ui-contract.md](./contracts/ui-contract.md)、削除範囲の根拠は [research.md](./research.md) を参照する。

---

## 前提条件

| 項目 | 内容 |
|---|---|
| ビルド | Visual Studio 2022（v143 ツールセット、MFC） |
| 実行ファイル | `C:\work\claude\prompt_editor_speckit\MFCApplication1\x64\Debug\MFCApplication1.exe` |
| テスト対象（MFC） | `C:\work\test\test01` |
| テスト対象（Web） | `C:\work\test\web01`（本ガイドの手順0で作成） |
| テスト対象（空） | `C:\work\test\empty01`（本ガイドの手順0で作成） |

**プラットフォーム名の注意**: `.sln` に渡すのは `x86` である。`Win32` を渡すと `error MSB4126` になる（001 の quickstart.md で実測済み）。

---

## 手順0: テスト用フォルダの準備

`C:\work\test\test01` を初期状態に戻し、Web アプリ用と空のフォルダを用意する。

```powershell
# test01 を初期状態へ（001 の検証で SDD化していた場合）
Remove-Item C:\work\test\test01\.git -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item C:\work\test\test01\.specify -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item C:\work\test\test01\.claude -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item C:\work\test\test01\.gitignore -Force -ErrorAction SilentlyContinue

# Web アプリ相当のフォルダ（package.json だけ置けばマーカーとして成立する）
New-Item -ItemType Directory C:\work\test\web01 -Force | Out-Null
'{ "name": "web01", "version": "1.0.0" }' | Out-File C:\work\test\web01\package.json -Encoding utf8

# 空のフォルダ
New-Item -ItemType Directory C:\work\test\empty01 -Force | Out-Null

# 確認
Get-ChildItem C:\work\test\test01 -Force -Name
Get-ChildItem C:\work\test\web01 -Force -Name
Get-ChildItem C:\work\test\empty01 -Force -Name
```

`test01` に `.git`・`.gitignore`・`.specify`・`.claude` が**無い**こと。`test01.sln` などのソリューション本体は**残っている**こと。

**注意**: `Remove-Item` の対象を取り違えるとソリューション本体を失う。実行前にパスを確認すること。

---

## 手順1: ビルド（ST-103）

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

**特に注意する警告**: 削除作業では「未使用の変数」「未使用の関数」「参照されない静的関数」が出やすい。これらが出た場合は消し残しである（FR-107 / ST-102）。

---

## 手順2: 静的検査（ST-101, ST-102, ST-105）

### ST-101: UIリソースの検査

```powershell
$rc=[System.IO.File]::ReadAllText('MFCApplication1\MFCApplication1.rc',[System.Text.Encoding]::Unicode)
$rh=[System.IO.File]::ReadAllText('MFCApplication1\Resource.h',[System.Text.Encoding]::UTF8)

"--- 削除されているべきもの ---"
foreach($id in 'IDC_RADIO_KIND_MFC','IDC_RADIO_KIND_WEB','IDR_GITIGNORE_VS','IDR_GITIGNORE_NODE'){
  "{0,-22} Resource.h={1} .rc={2}  (両方 False であること)" -f $id, ($rh -match $id), ($rc -match $id)
}
"--- 残っているべきもの ---"
foreach($id in 'IDC_EDIT_TARGET_PATH','IDC_BUTTON_BROWSE','IDC_BUTTON_GENERATE','IDC_BUTTON_DIAGNOSE','IDC_LIST_RESULT','IDC_STATIC_GUIDE','IDR_GITIGNORE'){
  "{0,-22} Resource.h={1} .rc={2}  (両方 True であること)" -f $id, ($rh -match $id), ($rc -match $id)
}
"--- グループボックス ---"
"プロジェクト種別: " + ($rc -match 'プロジェクト種別') + "  (False であること)"
```

### ST-102: 未使用コードの検査

```powershell
"--- ソース中に残っていてはならない語 ---"
foreach($w in 'ProjectKind','RADIO_KIND','hasSlnMarker','hasWebMarker','HasMarkerFor','KindDisplayName','m_kind','m_workerKind','種別を選択してください','種別と、フォルダ内のプロジェクトが一致しません'){
  $hits = Select-String -Path MFCApplication1\*.h,MFCApplication1\*.cpp -Pattern $w -SimpleMatch
  "{0,-36} 出現={1}  (0 であること)" -f $w, ($hits | Measure-Object).Count
}
```

**期待される結果**: すべて 0。1件でも残っていれば FR-107 違反。

### ST-105: 仕様と実装の対応

変更した各箇所に対応する FR 番号（FR-101〜FR-109）がコメントで追跡できること。

```powershell
$all=@()
foreach($f in (Get-ChildItem MFCApplication1\*.h,MFCApplication1\*.cpp)){
  $t=[System.IO.File]::ReadAllText($f.FullName,[System.Text.Encoding]::UTF8)
  $all += [regex]::Matches($t,'FR-1\d{2}a?') | ForEach-Object { $_.Value }
}
"実装から参照されている 002 の FR: " + (($all | Sort-Object -Unique) -join ' ')
```

---

## 手順3: 画面の確認（DT-101）

アプリケーションを起動する。

```powershell
Start-Process 'C:\work\claude\prompt_editor_speckit\MFCApplication1\x64\Debug\MFCApplication1.exe'
```

**期待される結果**:

- 種別を選ぶラジオボタン、グループボックスが**存在しない**
- 「生成」「準備状況を診断」ボタンが**左端（パス欄の左端と同じ位置）**に並んでいる
- 結果一覧が上に広がっており、削除による空白が残っていない
- Tab キーで巡回すると `パス欄 → 参照 → 生成 → 診断 → 一覧 → OK → キャンセル` の順に移動する（FR-109）

---

## 手順4: 未選択ガード（DT-103）

「参照...」を押さずに「生成」を押す。

**期待される結果**:

| # | 確認項目 |
|---|---|
| (a) | 「対象フォルダを選択してください。」が表示される |
| (b) | 結果一覧に何も表示されない |
| (c) | どのフォルダにもファイルが作成・変更されない |
| (d) | メッセージを閉じた後、「生成」ボタンが**押下可能なまま** |
| (e) | アプリケーションが異常終了しない |

**(d) が重要**: 001 から継承した契約である。押下できない場合、未選択エラーの経路でボタンを無効化したまま戻していない。

---

## 手順5: MFCフォルダでSDD化（DT-102, DT-107）

1. 「参照...」で `C:\work\test\test01` を選ぶ
2. **種別を選ぶ操作が無いことを確認**する
3. 「生成」を押す

**DT-107（回帰確認・重要）**: 処理中に以下を目視する。

| コントロール | 期待 |
|---|---|
| 生成ボタン | グレーアウト |
| 診断ボタン | グレーアウト |
| **参照...ボタン** | **グレーアウト** |

**参照ボタンが押せる状態なら不合格。** ラジオを削除する際に巻き添えで無効化対象から外した誤りである（research.md R-104）。

完了後、3つすべてが有効に戻ること。

```powershell
Get-ChildItem C:\work\test\test01 -Force -Name
Push-Location C:\work\test\test01; git rev-list --count HEAD; Pop-Location
```

`.git`・`.gitignore`・`.specify`・`.claude` が揃い、コミットが1件以上あること。

---

## 手順6: 生成物の検査（ST-104）

```powershell
$gi='C:\work\test\test01\.gitignore'
"--- Visual Studio 系 ---"
Select-String -Path $gi -Pattern '\[Dd\]ebug/','\.vs/','\*\.user','\*\.aps' | ForEach-Object { $_.Line }
"--- Node 系（MFCプロジェクトでも含まれること）---"
Select-String -Path $gi -Pattern 'node_modules/','dist/','\.env$' | ForEach-Object { $_.Line }
"--- 追記ブロックの目印行（各1回のみ）---"
"Spec Kit ブロック: " + (Select-String -Path $gi -Pattern '---- Spec Kit ----' | Measure-Object).Count
"プロジェクト固有ブロック: " + (Select-String -Path $gi -Pattern '---- このプロジェクト固有 ----' | Measure-Object).Count
"--- 重複パターンの検査（research.md R-101）---"
$dup = Get-Content $gi | Where-Object { $_ -notmatch '^\s*(#|$)' } | Group-Object | Where-Object Count -gt 1
if($dup){ "重複あり: " + ($dup.Name -join ', ') } else { "重複なし（OK）" }
```

**期待される結果**:

- Visual Studio 系と **Node 系の両方**が含まれる（MFCプロジェクトでも `node_modules/` が入るのが正しい）
- 目印行はそれぞれ **1回のみ**
- 同じパターンの重複が **無い**

**注意**: 統合ファイルは `*.vcxproj.user` ではなく、より広い **`*.user`** を持つ。`test01.vcxproj.user` は `*.user` にマッチするため除外される。パターン名の直接照合ではなく、実ファイルが除外されるかで確認するほうが確実である。

```powershell
Push-Location C:\work\test\test01
foreach($f in 'test01.vcxproj.user','test01.aps','.vs/test01','x64/Debug/a.obj','node_modules/x'){
  $r = git check-ignore -v -- $f 2>&1
  "{0,-24} {1}" -f $f, $(if($LASTEXITCODE -eq 0){ ($r -split "`t")[0] } else { '除外されない（NG）' })
}
Pop-Location
```

5件すべてが除外され、どの行が効いたかが表示されること。

---

## 手順7: べき等性（DT-105・回帰確認）

同じフォルダで再度「生成」を押す。

**期待される結果**: 5手順すべてが「スキップ」。「成功」が1件も出ないこと。

```powershell
Push-Location C:\work\test\test01
git status --short      # 変更が出ないこと
git rev-list --count HEAD   # 手順5から増えていないこと
Pop-Location
```

手順6の重複検査を再実行し、追記ブロックが増えていないことも確認する。

---

## 手順8: 診断の回帰確認（DT-106）

「準備状況を診断」を押す。

**期待される結果**（SDD化済みの状態）:

| 項目 | 期待 |
|---|---|
| 1. ビルドが通る雛形がある | 達成 |
| 2. Spec Kit の足場が同じ階層にある | **達成** |
| 3. コミットが1件以上入っている | **達成** |
| 4. テスト方針が決まっている | 判定不能 |
| 5. 憲章がある | 未達 |
| 6. CLAUDE.md が憲章を反映している | 未達 |
| 7. 技術スタックが記録されている | 判定不能 |

**回帰確認の要点**: 項目1の判定を `hasSlnMarker || hasWebMarker` から `hasProjectMarker` へ置き換えたが、両者は同値なので**判定結果は 001 と変わらない**。ここが変わっていたら統合の際に条件を間違えている。

診断の前後でフォルダが変化しないことも確認する（001 FR-020 の回帰）。

```powershell
Get-ChildItem C:\work\test\test01 -Force -Recurse | Measure-Object | Select-Object Count
```

---

## 手順9: Webフォルダで同一の `.gitignore`（DT-108, ST-106）

1. 「参照...」で `C:\work\test\web01` を選ぶ
2. 「生成」を押す（**MFCのときと操作手順が完全に同じ**であることを確認 — SC-104）

完了後、2つの `.gitignore` を突き合わせる。

```powershell
$a = Get-FileHash C:\work\test\test01\.gitignore -Algorithm SHA256
$b = Get-FileHash C:\work\test\web01\.gitignore  -Algorithm SHA256
"test01: " + $a.Hash
"web01 : " + $b.Hash
"判定: " + $(if($a.Hash -eq $b.Hash){'PASS（完全に同一）'}else{'FAIL（種別分岐が残っている）'})
```

**期待される結果**: ハッシュが完全に一致（SC-105）。

**不一致だった場合**: フォルダの内容によって `.gitignore` を切り替える分岐が実装に残っている。FR-106 違反である。

---

## 手順9b: 既存 `.gitignore` の保持（ST-107, DT-109）

FR-106a の検証。**独自の内容**を持つ `.gitignore` があるフォルダで、本体が保持されることを確認する。

```powershell
# 既存の .gitignore を消し、独自内容のものを置く
Remove-Item C:\work\test\web01\.gitignore -Force -ErrorAction SilentlyContinue
@"
# 独自の除外設定
my-secret-folder/
*.mykeep
"@ | Out-File C:\work\test\web01\.gitignore -Encoding utf8
Get-FileHash C:\work\test\web01\.gitignore -Algorithm SHA256 | Select-Object -ExpandProperty Hash
```

`web01` を選んで「生成」を押す。完了後に確認する。

```powershell
Get-Content C:\work\test\web01\.gitignore
```

**期待される結果**:

| # | 確認項目 |
|---|---|
| (a) | `# 独自の除外設定`・`my-secret-folder/`・`*.mykeep` の3行が**そのまま残っている** |
| (b) | 統合版の内容（`node_modules/` など）で**丸ごと上書きされていない** |
| (c) | 末尾に Spec Kit 用とプロジェクト固有の追記ブロックが足されている |
| (d) | 目印行がそれぞれ1回のみ（FR-106b） |

**(b) が重要**: 統合版で上書きしていたら FR-106a 違反である。ユーザーが手で書いた除外設定を失わせることになる。

もう一度「生成」を押し、追記ブロックが増えないこと（FR-106b のべき等性）も確認する。

確認後、次の手順のために元へ戻す。

```powershell
Remove-Item C:\work\test\web01\.gitignore -Force
```

---

## 手順10: 異常系（DT-104）

1. 「参照...」で `C:\work\test\empty01` を選ぶ
2. 「生成」を押す

**期待される結果**: ウィザードでプロジェクトを作成するよう促すメッセージが表示され、導入手順が実行されない。**雛形が生成されないこと**（001 FR-007 の継承）。

```powershell
Get-ChildItem C:\work\test\empty01 -Force -Name
```

空のままであること。

---

## 手順11: 複数マーカーの共存（US1-5）

`test01` に `package.json` を追加し、MFC と Web の両方のマーカーがある状態にする。

```powershell
'{ "name": "test01", "version": "1.0.0" }' | Out-File C:\work\test\test01\package.json -Encoding utf8
```

「生成」を押す。

**期待される結果**: 不一致の警告や確認ダイアログが**出ない**（FR-105）。そのまま処理が進み、全手順が「スキップ」になる。

001 では「選択種別と一致しない」として警告が出ていた状況だが、種別の概念が無くなったため異常ではなくなった。

確認後、追加したファイルを消す。

```powershell
Remove-Item C:\work\test\test01\package.json -Force
```

---

## 完了条件

- [ ] ST-101〜ST-106 の静的テストがすべて合格
- [ ] DT-101〜DT-108 の動的テストがすべて合格
- [ ] 特に **DT-107**（実行中に参照ボタンが無効化される）が合格 — 削除の巻き添えを検出する要所
- [ ] 特に **DT-108 / ST-106**（MFCとWebで `.gitignore` が同一）が合格 — 種別分岐の残存を検出する要所
- [ ] 手順1のビルドで、未使用変数・未使用関数の警告が0件

---

## 付録: テスト用フォルダのリセット

```powershell
foreach($d in 'test01','web01','empty01'){
  $p = "C:\work\test\$d"
  Remove-Item "$p\.git" -Recurse -Force -ErrorAction SilentlyContinue
  Remove-Item "$p\.specify" -Recurse -Force -ErrorAction SilentlyContinue
  Remove-Item "$p\.claude" -Recurse -Force -ErrorAction SilentlyContinue
  Remove-Item "$p\.gitignore" -Force -ErrorAction SilentlyContinue
  "$d : " + ((Get-ChildItem $p -Force -Name) -join ', ')
}
```

`test01` にソリューション本体（`test01.sln` 等）が残っていること。
