# クイックスタート（検証手順）: 起動直後のまま結果と案内が読める

**Feature**: 006-readable-at-startup
**Date**: 2026-09-10

検証は**ほぼ全自動で回せる**。フォルダ選択を伴う最終確認だけが手動である。

---

## 手順0: 準備

```powershell
Get-Process MFCApplication1 -ErrorAction SilentlyContinue | Stop-Process -Force
```

---

## 手順1: ビルド（ST-504）

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

## 手順2: 静的テスト（ST-501〜503、ST-505、ST-506）

### ST-501: `.rc` の寸法とコントロール数

```powershell
$rc = [System.IO.File]::ReadAllText('MFCApplication1\MFCApplication1.rc', [System.Text.Encoding]::Unicode)
# 空白の数に依存しない照合にする。実際の .rc は "DIALOGEX  0, 0, ..." と空白が2つある。
$exp = @(
  @('ダイアログ',   'IDD_MFCAPPLICATION1_DIALOG\s+DIALOGEX\s+0,\s*0,\s*545,\s*224'),
  @('パス欄',       'IDC_EDIT_TARGET_PATH,54,7,430,14'),
  @('参照ボタン',   'IDC_BUTTON_BROWSE,490,7,48,14'),
  @('結果一覧',     'IDC_LIST_RESULT.*,7,52,531,90'),
  @('案内欄',       'IDC_STATIC_GUIDE,7,147,531,52'),
  @('OK',           'IDOK,434,203,50,14'),
  @('キャンセル',   'IDCANCEL,488,203,50,14')
)
$ng = 0
foreach($e in $exp){
  $hit = [regex]::IsMatch($rc, $e[1])
  if(-not $hit){ $ng++ }
  "{0,-12} {1}  {2}" -f $e[0], $(if($hit){'一致'}else{'不一致'}), $e[1]
}
# FR-510: ダイアログ内のコントロール行を数える
$body = [regex]::Match($rc, 'IDD_MFCAPPLICATION1_DIALOG\s+DIALOGEX.*?
BEGIN
?
(.*?)
?
END', 'Singleline').Groups[1].Value
$ctrl = ($body -split "`r?`n" | Where-Object { $_ -match '^\s*(LTEXT|EDITTEXT|PUSHBUTTON|DEFPUSHBUTTON|CONTROL|COMBOBOX|GROUPBOX)' }).Count
"コントロール行数: $ctrl  （8 であること）"
if($ctrl -ne 8){ $ng++ }
"判定: " + $(if($ng -eq 0){'ST-501 合格'}else{"不合格 $ng 件"})
```

**合格条件**: 7項目すべて一致し、**コントロール行数が 8**（FR-510）。

> **T017（2026-09-11）で案内欄を 44 → 52 DLU、ダイアログを 216 → 224 DLU、ボタンの y を 195 → 203 に変更した。** ここの照合値も併せて更新してある。`.rc` を変えたら**この検査の期待値も必ず同時に直すこと** — 直し忘れると、正しい実装に対して不合格が出る（`/speckit-analyze` C1）。

> `.rc` は **UTF-16LE BOM 付き**なので `[System.Text.Encoding]::Unicode` で読むこと（CLAUDE.md 既出）。`Get-Content` では文字化けする。
>
> **`DIALOGEX` の後の空白は1つではなく2つある。** 文字列の完全一致で照合すると、正しく実装しても不合格になる（`/speckit-analyze` B1）。空白の数に依存しない正規表現を使う。

### ST-502: 列幅の比率

```powershell
$src = (Get-Content MFCApplication1\MFCApplication1Dlg.cpp | Where-Object { $_.TrimStart() -notlike '//*' }) -join "`n"
"新しい比率 { 20, 17, 63 } : " + ([regex]::Matches($src,'kColumnRatio\[3\]\s*=\s*\{\s*20\s*,\s*17\s*,\s*63\s*\}')).Count + "  （1 であること）"
"旧比率 { 40, 20, 40 }     : " + ([regex]::Matches($src,'\{\s*40\s*,\s*20\s*,\s*40\s*\}')).Count + "  （0 であること）"
"kColumnRatio の定義箇所   : " + ([regex]::Matches($src,'kColumnRatio\[3\]\s*=')).Count + "  （1 であること）"
"kColumnRatio の参照箇所   : " + ([regex]::Matches($src,'kColumnRatio\[')).Count + "  （5 であること。実装前に実測した値）"
```

**合格条件**: 新比率が1件、旧比率が0件、**定義は1箇所だけ**（004 FR-308 の「1箇所にまとめる」契約を維持）、参照は**5件**。

> 参照数の期待値 5 は実装前に実測した値である。「定義1 + 初期化3 + 再配置3 = 7」と数えると誤り（`/speckit-analyze` B2）。

> コメント行を除外している。研究の経緯で旧比率 `{40,20,40}` に言及するコメントを残す場合があり、含めると誤検出する（005 の ST-402 で同じ失敗をした）。

### ST-503: 004・005 の実装が無傷

```powershell
$files = 'MFCApplication1\MFCApplication1Dlg.h','MFCApplication1\MFCApplication1Dlg.cpp'
$code = ($files | ForEach-Object { Get-Content $_ | Where-Object { $_.TrimStart() -notlike '//*' } }) -join "`n"
$exp = @{ 'OnSize'=3; 'ON_WM_SIZE'=1; 'OnGetMinMaxInfo'=3; 'ptMinTrackSize'=2; 'ApplyAnchors'=3; 'InitAnchors'=3;
          'm_layoutReady'=5; 'SIZE_MINIMIZED'=1; 'DwmSetWindowAttribute'=2; 'DWMWA_CAPTION_COLOR'=1; 'DWMWA_TEXT_COLOR'=1 }
$ng = 0
foreach($k in $exp.Keys | Sort-Object){
  $n = ([regex]::Matches($code,[regex]::Escape($k))).Count
  $r = if($n -eq $exp[$k]){'一致'}else{$ng++;'不一致'}
  "{0,-24} 実測={1} 期待={2}  {3}" -f $k, $n, $exp[$k], $r
}
"判定: " + $(if($ng -eq 0){'ST-503 合格'}else{"不合格 $ng 件"})
```

**合格条件**: 11項目すべて一致。**`.h` と `.cpp` の両方を読み、コメント行を除外すること** — どちらかを外すと検査が壊れる（005 の analyze で確認済み）。

### ST-506: 文言を変更していない（FR-511）

```powershell
git diff --stat -- MFCApplication1/SetupRunner.cpp MFCApplication1/SetupRunner.h
"上の出力が空であること（SetupRunner が未変更）"
git diff -- MFCApplication1/MFCApplication1Dlg.cpp | Select-String '^[+-].*_T\("' | Where-Object { $_ -notmatch 'kColumnRatio' }
"上の出力が空であること（表示文字列の増減が無い）"
```

**合格条件**: どちらの出力も空。

**根拠**: 「読める幅を確保する」代わりに**メッセージを短く言い換える**のは FR-511 違反である。結果一覧に出る文言はすべて `SetupRunner.cpp` にあるため、このファイルが未変更であることが最も直接的な証拠になる。

### ST-505: FR による追跡可能性

```powershell
$c = Get-Content MFCApplication1\MFCApplication1Dlg.cpp -Raw
501..513 | ForEach-Object { $n="FR-$_"; "{0}: {1}" -f $n, ([regex]::Matches($c,$n)).Count }
```

**合格条件**:

| FR | コードに現れるべきか | 理由 |
|---|---|---|
| FR-501・502・504・506・509 | **必須** | `kColumnRatio` の変更と `.rc` の寸法の根拠 |
| FR-503・505・507・508・510・511・512 | **現れなくてよい** | `.rc` 側の要件、または「変更しない」ことが要件のため、対応するコード変更が存在しない |
| FR-513 | 任意 | 本項目そのもの |

`.rc` にはコメントを書けないため、寸法変更の根拠は `kColumnRatio` のコメントにまとめて記載する。

---

## 手順3: 寸法の自動計測（DT-501〜504、DT-508）

スクラッチパッドにスクリプトを作る。**リポジトリには追加しない**。

> **保存時の注意**: 日本語を含む `.ps1` は **UTF-8 BOM 付き**で保存する。`Add-Type` に渡す C# には日本語コメントを書かない。関数名に `Cli` のような短い名前を使わない（`Clear-Item` のエイリアスと衝突する）。

```powershell
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Threading;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct RECT { public int Left, Top, Right, Bottom; }
public static class Mx {
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern IntPtr FindWindowEx(IntPtr p, IntPtr a, string c, string n);
  [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr h, int id);
  [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr SendMessageW(IntPtr h, uint m, IntPtr w, IntPtr l);
  public static IntPtr FindMain(int procId, int probeId, int timeoutMs) {
    int w = 0;
    while (w < timeoutMs) {
      IntPtr h = IntPtr.Zero;
      while ((h = FindWindowEx(IntPtr.Zero, h, null, null)) != IntPtr.Zero) {
        uint pid; GetWindowThreadProcessId(h, out pid);
        if ((int)pid == procId && IsWindowVisible(h) && GetDlgItem(h, probeId) != IntPtr.Zero) return h;
      }
      Thread.Sleep(200); w += 200;
    }
    return IntPtr.Zero;
  }
}
"@
function WinSize($h){ $r=New-Object RECT; [void][Mx]::GetWindowRect($h,[ref]$r); @(($r.Right-$r.Left),($r.Bottom-$r.Top)) }
function ClientSize($h){ $r=New-Object RECT; [void][Mx]::GetClientRect($h,[ref]$r); @($r.Right,$r.Bottom) }
$font = New-Object System.Drawing.Font('MS UI Gothic', 9)
function TW($s){ [System.Windows.Forms.TextRenderer]::MeasureText($s, $font).Width }

Get-Process MFCApplication1 -ErrorAction SilentlyContinue | Stop-Process -Force
$p = Start-Process 'C:\work\claude\prompt_editor_speckit\MFCApplication1\x64\Debug\MFCApplication1.exe' -PassThru
$h = [Mx]::FindMain($p.Id, 1004, 10000)
if($h -eq [IntPtr]::Zero){ throw 'ダイアログが見つかりません' }
$list = [Mx]::GetDlgItem($h, 1006); $guide = [Mx]::GetDlgItem($h, 1007)
$ng = 0
function Check($label, $actual, $expected, $cmp){
  $ok = if($cmp -eq 'ge'){ $actual -ge $expected } else { $actual -eq $expected }
  if(-not $ok){ $script:ng++ }
  "{0,-34} 実測={1,5}  期待{2}{3,5}  {4}" -f $label, $actual, $(if($cmp -eq 'ge'){'≧'}else{'='}), $expected, $(if($ok){'合格'}else{'不合格'})
}

# DT-501: ウィンドウサイズ
$ws = WinSize $h
Check 'DT-501 ウィンドウ幅'   $ws[0] 972 'eq'
Check 'DT-501 ウィンドウ高さ' $ws[1] 423 'eq'
# DT-502: 1024x768 に収まる
Check 'DT-502 幅 ≦ 1024'      1024   $ws[0] 'ge'
Check 'DT-502 高さ ≦ 728'     728    $ws[1] 'ge'
# DT-503: 列幅が必要幅以上
# 余白 20 px は実測できない見積もり。さらに 8 px 以上の余裕を要求する（analyze B3）
$need = @{
 0 = (TW 'Git 初期化と初回コミット') + 20 + 8
 1 = (TW 'スキップ（前提未達）') + 20 + 8
 2 = (TW 'Git初期化はスキップ。既存の未コミット変更を検出したため、.gitignore のみをコミットしました。') + 20 + 8
}
$sum = 0
foreach($i in 0..2){
  $w = [Mx]::SendMessageW($list, 0x101D, [IntPtr]$i, [IntPtr]::Zero).ToInt32()
  $sum += $w
  Check ("DT-503 列{0}の幅（余白20+余裕8）" -f $i) $w $need[$i] 'ge'
}
# FR-503: 横スクロールバーが出ないこと
$lc = ClientSize $list
Check 'DT-503 列幅の合計 ≦ 一覧クライアント幅' $lc[0] $sum 'ge'
# DT-504: 案内欄が5行入る高さと、折り返しの無い幅
$gc = ClientSize $guide
Check 'DT-504 案内欄のクライアント高さ' $gc[1] 71 'ge'
$maxLine = (@(
 'Spec Kit の導入が完了しました。次の手順は Claude Code との対話が必要です。',
 ' 1. VS Code で C:\work\test\test01 を開く（.specify と .claude が直下に見えるフォルダ）',
 ' 2. Claude パネルで /init を実行し、CLAUDE.md を生成する',
 ' 3. テスト方針を決め、/speckit-constitution で憲章を制定する',
 ' 4. /init を再実行し、憲章を反映した CLAUDE.md に更新する',
 '対象フォルダを選び、「生成」を押してください。',
 '対象のプロジェクトは、あらかじめ Visual Studio 2022 などのウィザードで作成し、そのフォルダに配置しておいてください。'
) | ForEach-Object { TW $_ } | Measure-Object -Maximum).Maximum
Check 'DT-504 案内欄のクライアント幅' $gc[0] $maxLine 'ge'
$lc = [Mx]::SendMessageW($guide, 0xBA, [IntPtr]::Zero, [IntPtr]::Zero).ToInt32()
Check 'DT-504 初期案内の表示行数（折返なし）' $lc 2 'eq'

# DT-508: 長いパスで折り返しても5行すべてが見えること（FR-504a / SC-502）
# 案内欄はパスを含む2行目が長いと折り返す。折り返し1回分を吸収できているかを見る。
$longPath = 'C:\Users\user\OneDrive - 会社名\ドキュメント\開発\顧客管理システム\CustomerManagementApp'
$line2 = " 1. VS Code で $longPath を開く（.specify と .claude が直下に見えるフォルダ）"
$w2 = TW $line2
$wrapped = [Math]::Ceiling($w2 / $gc[0])
$displayLines = 4 + $wrapped
Check 'DT-508 長いパスの2行目が折り返す（前提）' $wrapped 2 'eq'
Check 'DT-508 折返し後の行数ぶんの高さがある' $gc[1] ($displayLines * 14 + 1) 'ge'
"参考: 長いパスの2行目 = $w2 px / 案内欄クライアント幅 = $($gc[0]) px -> $wrapped 行に折り返し（全体 $displayLines 行）"
Stop-Process -Id $p.Id -Force
"判定: " + $(if($ng -eq 0){'DT-501〜504 すべて合格'}else{"不合格 $ng 件"})
```

**合格条件**: 全項目「合格」。

- **DT-501**: 起動直後のウィンドウが 972 x 437 px（FR-506）
- **DT-502**: 1024x768 の作業領域に収まる（SC-503）
- **DT-503**: 3列とも最長文字列 + 余白 20 px + 余裕 8 px 以上で、合計が一覧のクライアント幅以下（FR-501、FR-502、FR-503、SC-501）
- **DT-504**: 案内欄が 5 行分の高さを持ち、最長の行が折り返さない幅を持つ。初期案内が**2行**で表示される（現状は折り返して3行）（FR-504、FR-505）
- **DT-508**: **長いフォルダパスで完了案内の2行目が折り返しても、5行すべてが見える高さがある**（FR-504a、SC-502）。案内欄のクライアント幅 905 px から固定文 412 px を引いた **493 px**（半角約82文字）がパスに使える幅であり、これを超えると折り返す。`C:\Users\user\OneDrive - 会社名\ドキュメント\開発\顧客管理システム\CustomerManagementApp` は 1033 px に達し、OneDrive と日本語フォルダ名の組み合わせで現実的に到達する。折り返し1回で表示6行になるため、案内欄には 6 × 14 + 1 = **85 px 以上**のクライアント高さが必要

---

## 手順4: 004 のリサイズ追従の回帰（DT-505）

004 の `scratchpad/resize.ps1` を実行する。**期待値は初期サイズの変更に伴って変わる**ので、004 の値をそのまま使ってはならない。

**合格条件**:

| 確認項目 | 期待 |
|---|---|
| 初期ウィンドウ | 972 x 437 |
| 横 +200 でパス欄の幅 | +200 される |
| 縦 +150 で一覧の高さ | +150 される（案内欄の高さは不変） |
| 縮小して戻したとき | 全コントロールが初期値と完全一致 |
| 下限 | 972 x 437 を下回らない |
| 列幅 | 常に 20/17/63 の比率を保つ |
| 最大化・最小化からの復帰 | 配置が崩れない |

---

## 手順5: タイトルバーの色の回帰（DT-506）

005 の `scratchpad/caption.ps1` を実行する。

**合格条件**: 5点すべて `#1F3864`。寸法変更が着色に影響していないこと。

---

## 手順6: 通し確認（DT-507、**要手動**）

`C:\work\test\test01` をリセットしてから、アプリで通しに実行する。

1. 何も選ばずに「生成」→ 警告が出て、閉じたあとも「生成」が押せる
2. 「参照...」で `test01` を選び「生成」→ 結果一覧5行がすべて「完了」または「スキップ」
3. **結果一覧の「内容」欄が5行とも途中で切れていない**（本フィーチャーの主目的）
4. **完了案内の5行すべてが、スクロールせずに見えている**（本フィーチャーの主目的）
5. 案内欄の文字を選択して Ctrl+C で複写できる
6. もう一度「生成」→ 5行すべて「スキップ」
7. ウィンドウを大きく・小さくして追従を確認

リセット:

```powershell
$t='C:\work\test\test01'
foreach($x in '.git','.specify','.claude'){ $p=Join-Path $t $x; if(Test-Path $p){ Remove-Item -LiteralPath $p -Recurse -Force } }
foreach($x in '.gitignore','package.json'){ $p=Join-Path $t $x; if(Test-Path $p){ Remove-Item -LiteralPath $p -Force } }
Get-ChildItem $t -Force -Name
```

**未コミット変更を伴う経路（`Git初期化はスキップ。既存の未コミット変更を…` の 552 px）を確認したい場合**は、001 T058 と同じ手順で `test01Dlg.cpp` に変更を加え、`.gitignore` から Spec Kit ブロックを削除してから「生成」を押す。この文字列が切れずに表示されることが FR-501 の最も厳しい条件である。

---

## 完了条件

- [ ] ST-501〜ST-506 の静的テストがすべて合格
- [ ] DT-501〜DT-508 の動的テストがすべて合格
- [ ] 特に **DT-503**（列幅 ≧ 最長文字列 + 余白）が合格 — 本フィーチャーの主目的
- [ ] 特に **DT-504**（案内欄に5行入る）が合格 — 本フィーチャーの主目的
- [ ] 特に **DT-508**（長いパスで折り返しても5行見える）が合格 — 実運用で最も起きやすい破れ方
- [ ] 特に **DT-505**（004 の追従が無傷）が合格 — 寸法変更の巻き添えを検出する要所
- [ ] 特に **DT-503 の余裕 8 px** が確保されている — 余白の見積もり誤りを吸収する要所
- [ ] 特に **ST-506**（文言を変更していない）が合格 — 幅の確保を文言の短縮ですり替えていないことの確認
- [ ] 手順1のビルドで警告が0件
- [ ] コード分析の指摘が0件

---

## 付録: コード分析

```powershell
$msb='C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe'
& $msb MFCApplication1\MFCApplication1.vcxproj /p:Configuration=Debug /p:Platform=x64 /t:Rebuild `
  /p:RunCodeAnalysis=true /p:EnablePREfast=true /v:minimal /nologo 2>&1 | Select-String 'warning'
```

**合格条件**: 出力が空であること。
