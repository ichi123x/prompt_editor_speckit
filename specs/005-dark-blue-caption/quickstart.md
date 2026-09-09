# クイックスタート（検証手順）: タイトルバーを濃い青にする

**Feature**: 005-dark-blue-caption
**Date**: 2026-09-09

本フィーチャーの検証は、004 と違って**ほぼ全自動で回せる**。フォルダ選択ダイアログを通らないため、CLAUDE.md に記録した GUI 自動操作の制約に掛からない。目視が要るのは DT-405 の1件だけである。

---

## 手順0: 準備

アプリが起動したままだとビルドが `LNK1104` で失敗する。

```powershell
Get-Process MFCApplication1 -ErrorAction SilentlyContinue | Stop-Process -Force
```

---

## 手順1: ビルド（ST-404）

4構成すべてで、警告0件・エラー0件であること。`.sln` に渡すプラットフォーム名は **`x86`**（`Win32` では `error MSB4126`）。

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

`dwmapi.lib` のリンクは `#pragma comment` で行うため、**x86 構成でもリンクエラーが出ないこと**がここで確認される（[research.md](./research.md) R-406）。

---

## 手順2: 静的テスト（ST-401〜403、ST-405、ST-406）

### ST-401: 着色処理が存在する

```powershell
Select-String -Path MFCApplication1\MFCApplication1Dlg.cpp -Pattern 'DwmSetWindowAttribute|DWMWA_CAPTION_COLOR|DWMWA_TEXT_COLOR|dwmapi' |
  ForEach-Object { "{0,-5} {1}" -f $_.LineNumber, $_.Line.Trim() }
```

**合格条件**: `DwmSetWindowAttribute` が2回（背景・文字）、`DWMWA_CAPTION_COLOR` が1回、`DWMWA_TEXT_COLOR` が1回、`dwmapi` の include と `#pragma comment` が現れること。

**この「2回ちょうど」が FR-411（着色対象はメインダイアログ1枚のみ）の根拠でもある。** 3回以上あれば、他のウィンドウにも着色を広げたか、共通化の仕組みを作ったことを意味する（[contracts/ui-contract.md](./contracts/ui-contract.md) 4章）。

### ST-402: 色の値が正しい

`0x001F3864`（RとBが逆）になっていないことを機械的に検出する。

```powershell
$src = Get-Content MFCApplication1\MFCApplication1Dlg.cpp -Raw
"誤った並び 0x001F3864 の出現: " + ([regex]::Matches($src,'0x001F3864')).Count + "  （0 であること）"
"正しい並び 0x0064381F の出現: " + ([regex]::Matches($src,'0x0064381F')).Count
"RGB マクロの使用: " + ([regex]::Matches($src,'RGB\(\s*0x1F\s*,\s*0x38\s*,\s*0x64\s*\)')).Count
"文字色（白）の定義: " + ([regex]::Matches($src,'0x00FFFFFF|RGB\(\s*0xFF\s*,\s*0xFF\s*,\s*0xFF\s*\)|RGB\(\s*255\s*,\s*255\s*,\s*255\s*\)')).Count
```

**合格条件**: 誤った並びが0件で、かつ「正しい並び」か「RGB マクロ」のどちらかが1件以上あり、**文字色（白）の定義が1件以上ある**こと。

**SC-402（コントラスト比 4.5:1 以上）はこの検査で担保される。** 背景 `#1F3864` と文字 `#FFFFFF` の2つが確定すれば比は約 11.5:1 に定まり、実行時に測り直す余地が無い。白以外の色に変えられた場合はこの検査が落ちる。

### ST-403: 再設定していない

```powershell
Select-String -Path MFCApplication1\MFCApplication1Dlg.cpp -Pattern 'DwmSetWindowAttribute' -Context 8,0 |
  ForEach-Object { $_.Context.PreContext -join "`n" }
```

**合格条件**: 呼び出しが `OnInitDialog` の中にのみあり、`OnSize` / `OnGetMinMaxInfo` / `OnPaint` の中に無いこと（[contracts/ui-contract.md](./contracts/ui-contract.md) 2章）。

あわせて、004 の処理が無傷であること:

```powershell
# 004 の実装は .h（宣言・メンバ）と .cpp（定義）に分かれているため、両方を対象にする。
# また、行頭が // のコメント行は除外する。
$files = 'MFCApplication1\MFCApplication1Dlg.h','MFCApplication1\MFCApplication1Dlg.cpp'
$code = ($files | ForEach-Object { Get-Content $_ | Where-Object { $_.TrimStart() -notlike '//*' } }) -join "`n"
foreach($k in 'OnSize','ON_WM_SIZE','OnGetMinMaxInfo','ptMinTrackSize','ApplyAnchors','InitAnchors','m_layoutReady','SIZE_MINIMIZED'){
  "{0,-18} {1}" -f $k, ([regex]::Matches($code,[regex]::Escape($k))).Count
}
```

**合格条件**: 次の件数と完全に一致すること。

| キーワード | 期待値 |
|---|---|
| `OnSize` | 3 |
| `ON_WM_SIZE` | 1 |
| `OnGetMinMaxInfo` | 3 |
| `ptMinTrackSize` | 2 |
| `ApplyAnchors` | 3 |
| `InitAnchors` | 3 |
| `m_layoutReady` | 5 |
| `SIZE_MINIMIZED` | 1 |

**この2点（`.h` も読む／コメント行を除く）を外すと検査が壊れる。**

- **`.cpp` だけを読むと実装前から不合格になる。** 004 の実装は宣言が `.h`、定義が `.cpp` に分かれており、`.cpp` 単独では `OnSize` は 3 ではなく 2 になる（実測値）
- **コメント行を除かないと、本フィーチャーの実装によって必ず不合格になる。** T006 は「`OnSize` で塗り直さない理由」のコメントを要求しており、そのコメントは語 `OnSize` を含むため出現数が増える。004 の ST-303 で見つかったのと同じ「自分で自分を壊す検査」であり、実測でコメント1行の追加により件数が増えることを確認済み

### ST-405: FR による追跡可能性

```powershell
$c = Get-Content MFCApplication1\MFCApplication1Dlg.cpp -Raw
401..412 | ForEach-Object { $n="FR-$_"; "{0}: {1}" -f $n, ([regex]::Matches($c,$n)).Count }
```

**合格条件**: 実装に対応する FR がコメントとして現れること。着色の処理に直接対応するのは FR-401〜FR-404、FR-408、FR-411 であり、これらは**必ず**現れなければならない。FR-405〜407・FR-409・FR-410 は「何もしない」ことが要件なので、`OnInitDialog` に**なぜ再設定しないのかを説明するコメント**として1箇所にまとめて記載する。

### ST-406: 戻り値を判定していない

```powershell
Select-String -Path MFCApplication1\MFCApplication1Dlg.cpp -Pattern 'DwmSetWindowAttribute' -Context 2,2 |
  ForEach-Object { ($_.Context.PreContext + $_.Line + $_.Context.PostContext) -join "`n"; '---' }
```

**合格条件**: 着色の呼び出しが `if` / `SUCCEEDED` / `FAILED` で囲まれておらず、戻り値を受ける変数も無いこと（FR-408）。

**根拠**: 判定を書くと、そこから「失敗したら何かする」分岐が生える。未知の属性でも `E_INVALIDARG` が返るだけで例外にならないため、無視するのが正しい（[research.md](./research.md) R-405）。

---

## 手順3: 動的テストの自動計測（DT-401〜404、DT-406）

検証用スクリプトをスクラッチパッドに作る。**リポジトリには追加しない**（憲章の技術的制約により `.ps1` をプロジェクトへ追加しない）。

> **保存時の注意**: PowerShell 5.1 は BOM の無い `.ps1` を ANSI として読むため、日本語を含むスクリプトは **UTF-8 BOM 付き**で保存すること。また `Add-Type` に渡す C# コードには**日本語コメントを書かない**（CLAUDE.md 既出）。

```powershell
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Threading;
using System.Runtime.InteropServices;
[StructLayout(LayoutKind.Sequential)]
public struct RECT { public int Left, Top, Right, Bottom; }
public static class Cap {
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern IntPtr FindWindowEx(IntPtr p, IntPtr a, string c, string n);
  [DllImport("user32.dll")] static extern IntPtr GetDlgItem(IntPtr h, int id);
  [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern IntPtr FindWindow(string c, string n);
  // タスクバーへフォーカスを移して非アクティブ状態を作る。別アプリを起動しないので後片付けが不要
  public static bool Deactivate() { return SetForegroundWindow(FindWindow("Shell_TrayWnd", null)); }
  [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr h, int cmd);
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
  public static void Show(IntPtr h, int c) { ShowWindow(h, c); Thread.Sleep(700); }
}
"@

function Get-CaptionColor($h) {
  $r = New-Object RECT
  [void][Cap]::GetWindowRect($h, [ref]$r)
  # 最大化するとウィンドウ矩形は不可視の枠の分だけ画面外へはみ出す（実測 L=-9 T=-9）。
  # 画面外を採取しないよう 0 以上へ丸める。
  $x = [Math]::Max(0, $r.Right - 200)
  $y = [Math]::Max(0, $r.Top + 15)
  $bmp = New-Object System.Drawing.Bitmap 1,1
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen($x, $y, 0, 0, (New-Object System.Drawing.Size 1,1))
  $c = $bmp.GetPixel(0,0); $g.Dispose(); $bmp.Dispose()
  '#{0:X2}{1:X2}{2:X2}' -f $c.R, $c.G, $c.B
}
function Check($h, $label) {
  $c = Get-CaptionColor $h
  "{0,-24} {1}  {2}" -f $label, $c, $(if ($c -eq '#1F3864') { '合格' } else { '不合格' })
}

Get-Process MFCApplication1 -ErrorAction SilentlyContinue | Stop-Process -Force
$p = Start-Process 'C:\work\claude\prompt_editor_speckit\MFCApplication1\x64\Debug\MFCApplication1.exe' -PassThru
$h = [Cap]::FindMain($p.Id, 1004, 10000)
if ($h -eq [IntPtr]::Zero) { throw 'ダイアログが見つかりません' }
[void][Cap]::SetForegroundWindow($h); Start-Sleep -Milliseconds 700

Check $h 'DT-401 起動直後'
[void][Cap]::Deactivate(); Start-Sleep -Milliseconds 1000
Check $h 'DT-402 非アクティブ'
[void][Cap]::SetForegroundWindow($h); Start-Sleep -Milliseconds 700
[Cap]::Show($h, 3);                      Check $h 'DT-403 最大化'
[Cap]::Show($h, 9);                      Check $h 'DT-403 復元'
[Cap]::Show($h, 6); [Cap]::Show($h, 9)
[void][Cap]::SetForegroundWindow($h); Start-Sleep -Milliseconds 500
Check $h 'DT-404 最小化からの復帰'
Stop-Process -Id $p.Id -Force
```

**合格条件**: 5行すべてが `#1F3864` で「合格」。

- **DT-401**: 起動直後に濃い青である（FR-401）

> **FR-404（既定色が一瞬見えてから塗り替わらないこと）は DT-401 では検証できない。** ウィンドウを見つけて 700 ミリ秒待ってから採取するため、その間のちらつきは捉えられない。FR-404 は測定ではなく**構造で担保している** — `OnInitDialog` はダイアログが画面に出る前に走るため、既定色が描かれる瞬間が存在しない（[research.md](./research.md) R-404）。したがって FR-404 の合否は、着色処理が `OnInitDialog` にあることを確認する ST-403 で判定する。
- **DT-402**: 非アクティブでも色が変わらない（FR-405）
- **DT-403**: 最大化・復元で色が保たれる（FR-406）。最大化時のウィンドウ矩形は実測で `L=-9 T=-9 R=1929 B=1089` となり、採取位置は `(1729, 6)` で画面内に収まることを確認済み
- **DT-404**: 最小化から戻しても色が保たれる（FR-406）
- **DT-406**: 上記が**再設定の処理を一切書いていない状態**で成立していること（R-407 の裏取り）。ST-403 と組み合わせて成立する

---

## 手順4: ダークモードでの確認（DT-407）

Windows の設定 →「個人用設定」→「色」→「モードを選ぶ」を**ライト／ダークで切り替え**、それぞれで手順3のスクリプトを実行する。

**合格条件**: どちらのモードでも5行すべて `#1F3864`（FR-407）。

> 現在の設定は `AppsUseLightTheme=1`・`SystemUsesLightTheme=0` である（[research.md](./research.md) の測定環境）。切り替えたら**元の設定に戻すこと**。

---

## 手順5: アクセントカラー設定での確認（DT-405、**目視・要手動**）

Windows の設定 →「個人用設定」→「色」→「タイトル バーとウィンドウの境界線にアクセント カラーを表示する」を**オンにする**。

その状態でアプリを起動し、タイトルバーを見る。

**合格条件**: アクセントカラー（現在の設定では `#0078D4` 系の水色）ではなく、**濃い青 `#1F3864`** で表示されること。

**この項目だけは結果が未測定である**（[research.md](./research.md) R-410）。**不合格だった場合は実装の誤りではなく、仕様の前提が崩れたことを意味する。** その場合は spec の FR-401 を見直す必要があるため、実装を直す前に報告すること。

確認後、設定を**元（オフ）に戻すこと**。

---

## 手順6: 既存機能の回帰確認（DT-408 / SC-404）

004 の検証がそのまま通ることを確認する。004 の quickstart 手順3〜9 のうち、自動計測できる部分を再実行する。

```powershell
# 004 の scratchpad/resize.ps1 と同等の計測を実行し、
# 初期 578x395 / 横+200 / 縦+150 / 縮小復帰 / 下限 / 最大化 / 列幅 / 表示可能行数 を突き合わせる
```

**合格条件**: 004 で得た値と**完全に一致**すること。

| 確認項目 | 004 での値 |
|---|---|
| 初期ウィンドウサイズ | 578x395（クライアント 560x350） |
| 横+200 時のパス欄の幅 | 359 → 559 |
| 縦+150 時の一覧の高さ | 158 → 308 |
| 縮小して戻したときの各コントロール | 初期値と完全一致 |
| 最小サイズ | 578x395 を下回らない |
| 列幅 | 212/106/212 → 292/146/292 |
| 表示可能行数 | 7 → 15 → 46 |

さらに、フォルダ選択を伴う部分（SDD化の完走、再実行で全手順スキップ、未選択ガード、案内のコピー）は**利用者による手動確認**が必要（CLAUDE.md の制約）。

---

## 手順7: 着色できない環境の模擬（DT-409）

Windows 10 以前を用意せずに FR-408・FR-409 を確かめるため、**属性番号を存在しない値に差し替えたビルド**で確認する。着色できない環境で起きることと同じ `E_INVALIDARG` が返る状態を作れる。

1. `MFCApplication1/MFCApplication1Dlg.cpp` の `DWMWA_CAPTION_COLOR` と `DWMWA_TEXT_COLOR` を、一時的に `999` へ差し替える
2. Debug x64 をビルドし、アプリを起動する
3. 以下を確認する

| 確認項目 | 合格条件 |
|---|---|
| 起動 | エラーメッセージも警告も出ず、ウィンドウが通常どおり表示される |
| タイトルバー | 手順3のスクリプトで採取した色が `#1F3864` **ではなく** Windows 既定の色になる |
| リサイズ追従 | 004 の実測値（手順6の表）と**完全に一致**する |

4. **属性番号を元に戻して再ビルドし、手順3を再実行して5行すべて `#1F3864` に戻ることを確認する**

> **この後始末を忘れないこと。** 999 のままコミットすると、着色が効かない状態で「完成」扱いになる。

---

## 完了条件

- [ ] ST-401〜ST-406 の静的テストがすべて合格
- [ ] DT-401〜DT-409 の動的テストがすべて合格
- [ ] 特に **ST-402**（色の並び）が合格 — `0x001F3864` と書く取り違えを機械的に検出する要所
- [ ] 特に **ST-403**（再設定していない）が合格 — 004 の `OnSize` に無関係な処理が混ざっていないことの確認
- [ ] 特に **DT-405**（アクセントカラー設定）の結果を確認 — 唯一の未測定項目
- [ ] 特に **DT-409** の後始末（属性番号を元に戻して再ビルド）が済んでいる
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
