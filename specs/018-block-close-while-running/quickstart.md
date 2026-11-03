# クイックスタート（検証手順）: 導入処理の実行中はダイアログを閉じられないようにする

**Feature**: 018-block-close-while-running
**Date**: 2026-11-02

静的テスト ST-1801〜1806 は自動化する。動的テストのうち DT-1802（待機中は閉じられる）はフォルダ選択を
必要としないため自動化する。DT-1801（実行中は閉じられない）は「生成」の前にフォルダ選択が必要であり、
フォルダ選択ダイアログの自動操作ができないという既知の制限（CLAUDE.md）により、**利用者による実機実行が
必須**である。

**FR-1806（送信失敗時の解放）について**: 018 の適用後は実行中にダイアログが閉じられなくなるため、送信が
失敗する場面を画面操作で作れない（spec Assumptions）。この要件は ST-1804（コードの検査）と、ST-1804 の
目視レビューで確認する。

---

## 手順0: 準備

```powershell
Get-Process MFCApplication1 -ErrorAction SilentlyContinue | Stop-Process -Force
```

---

## 手順1: ビルド（ST-1801）

```powershell
$msb='C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe'
foreach($cfg in 'Debug','Release'){ foreach($plat in 'x64','x86'){
  $out = & $msb MFCApplication1\MFCApplication1.sln /p:Configuration=$cfg /p:Platform=$plat /t:Rebuild /v:minimal /nologo 2>&1
  "{0,-8} {1,-5} 警告={2} エラー={3}" -f $cfg,$plat,
    ($out | Select-String ' warning ' | Measure-Object).Count,
    ($out | Select-String ' error '   | Measure-Object).Count
}}
```

**合格条件**: 4行すべてで警告=0 エラー=0。あわせて付録のコード分析で指摘が0件であること。

---

## 手順2: 静的テスト（ST-1802〜1806）

### ST-1802: FR による追跡可能性

FR 番号はコメントとして記載する設計のため、この検査は**コメント行を除外しない**。

```powershell
$text = (Get-Content MFCApplication1\MFCApplication1Dlg.cpp -Raw) + (Get-Content MFCApplication1\MFCApplication1Dlg.h -Raw)
1801..1808 | ForEach-Object { $n = "FR-$_"; "{0}: {1}" -f $n, ([regex]::Matches($text, $n)).Count }
```

**合格条件**: FR-1801〜1808 がいずれも1件以上出現すること。

### ST-1803: 実験用の目印が残っていないこと（FR-1805）

ソースコードには経緯の説明も書かない設計（research.md R-1805）のため、この検査は**コメント行も含めて**
探す。

```powershell
Select-String -Path MFCApplication1\*.cpp, MFCApplication1\*.h -Pattern 'T_CHECK_BUG_01'
```

**合格条件**: 出力が空であること。

### ST-1804: 手順の結果の受け渡しが憲章 O-1・O-3 に従っていること（FR-1806・FR-1807）

CLAUDE.md の注意に従い、**コメント行を除外して**探す（「`new` を使わない」旨のコメントを拾って誤判定
しないため）。

```powershell
$code = Get-Content MFCApplication1\MFCApplication1Dlg.cpp | Where-Object { $_ -notmatch '^\s*//' }
"new SetupStep : " + ($code | Select-String 'new\s+SetupStep' | Measure-Object).Count
"delete step   : " + ($code | Select-String 'delete\s+step'   | Measure-Object).Count
"make_unique   : " + ($code | Select-String 'make_unique<SetupStep>' | Measure-Object).Count
"release()     : " + ($code | Select-String '\.release\(\)'  | Measure-Object).Count
"unique_ptr受取: " + ($code | Select-String 'unique_ptr<SetupStep>' | Measure-Object).Count
Select-String -Path MFCApplication1\MFCApplication1Dlg.cpp -Pattern '#include <memory>'
```

**合格条件**: `new SetupStep` と `delete step` が 0、`make_unique` と `release()` と `unique_ptr受取` が
それぞれ 1 以上、`#include <memory>` が見つかること。

**目視レビュー（必須）**: `OnStepFinished` を開き、次の3点を確認する。
1. `release()` が `PostMessage` の戻り値が成功（非 0）のときだけ実行される分岐の中にある（FR-1806）。
2. `release()` の箇所に、所有権がメッセージの受け取り側へ移ることを示すコメントがある（憲章 O-1）。
3. `OnStepFinishedMessage` の先頭で `lParam` を直ちに `unique_ptr` で受け取り直している（FR-1807）。

### ST-1805: 案内の文言（FR-1802、憲章 原則II）

```powershell
(Select-String -Path MFCApplication1\MFCApplication1Dlg.cpp -Pattern '導入処理の実行中は閉じられません。完了までお待ちください。' | Measure-Object).Count
```

**合格条件**: 2 であること（`OnOK` と `OnCancel` に1件ずつ。research.md R-1802）。

### ST-1806: CLAUDE.md の更新（spec Assumptions、research.md R-1805）

```powershell
Select-String -Path CLAUDE.md -Pattern '018-block-close-while-running', '4つの原則', 'v1\.0\.0', 'v1\.5\.0'
```

**合格条件**: `018-block-close-while-running` と `v1.5.0` が見つかり、`4つの原則` と `v1.0.0` が見つから
ないこと。

---

## 手順3: DT-1802（待機中は閉じられること、完全自動）

フォルダを選ばずに起動し、閉じる操作を送ってプロセスが終了することを確かめる。閉じる操作ごとにアプリを
起動し直す。

- ケース1〜3: 起動直後に閉じる（FR-1804、spec User Story 2 シナリオ1）
- ケース4: フォルダ未選択のまま「生成」を送り、検証で中止させる。表示される「対象フォルダを選択して
  ください。」の案内を閉じてから「キャンセル」を送る（FR-1804、spec User Story 2 シナリオ3）。検証で
  中止した場合は `m_running` が `true` にならないことを、フォルダ選択なしで確かめられる

```powershell
Add-Type @"
using System; using System.Text; using System.Collections.Generic; using System.Runtime.InteropServices;
public static class W {
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc f, IntPtr l);
  [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetClassName(IntPtr h, StringBuilder s, int n);
  public static List<IntPtr> Dialogs(uint pid) {
    var r = new List<IntPtr>();
    EnumWindows((h, l) => { uint p; GetWindowThreadProcessId(h, out p);
      var sb = new StringBuilder(64); GetClassName(h, sb, 64);
      if (p == pid && IsWindowVisible(h) && sb.ToString() == "#32770") r.Add(h); return true; }, IntPtr.Zero);
    return r;
  }
}
"@
$exe = 'MFCApplication1\x64\Debug\MFCApplication1.exe'
$WM_COMMAND = 0x0111; $WM_SYSCOMMAND = 0x0112
$cases = @(
  @{ name='1 OK (IDOK)';                msg=$WM_COMMAND;    w=1;      pre=$false },
  @{ name='2 キャンセル (IDCANCEL)';    msg=$WM_COMMAND;    w=2;      pre=$false },
  @{ name='3 ✖ (SC_CLOSE)';             msg=$WM_SYSCOMMAND; w=0xF060; pre=$false },
  @{ name='4 検証中止後にキャンセル';   msg=$WM_COMMAND;    w=2;      pre=$true  }
)
foreach ($c in $cases) {
  $p = Start-Process $exe -PassThru
  $null = $p.WaitForInputIdle(5000); Start-Sleep -Milliseconds 500; $p.Refresh()
  $main = $p.MainWindowHandle
  if ($c.pre) {
    [W]::PostMessage($main, $WM_COMMAND, [IntPtr]1004, [IntPtr]::Zero) | Out-Null   # IDC_BUTTON_GENERATE
    Start-Sleep -Milliseconds 800
    $boxes = [W]::Dialogs([uint32]$p.Id) | Where-Object { $_ -ne $main }
    "   検証の案内ボックス数={0}" -f @($boxes).Count
    $boxes | ForEach-Object { [W]::PostMessage($_, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null }  # WM_CLOSE で案内を閉じる
    Start-Sleep -Milliseconds 800
    "   案内を閉じた後の残り={0}" -f @([W]::Dialogs([uint32]$p.Id) | Where-Object { $_ -ne $main }).Count
  }
  [W]::PostMessage($main, $c.msg, [IntPtr]$c.w, [IntPtr]::Zero) | Out-Null
  $closed = $p.WaitForExit(3000)
  "{0,-28} 閉じた={1}" -f $c.name, $closed
  if (-not $closed) { $p | Stop-Process -Force }
}
```

**合格条件**: 4ケースすべてで `閉じた=True`。ケース4では `検証の案内ボックス数=1` かつ
`案内を閉じた後の残り=0` であること（ボックス数が 0 の場合は「生成」が届いておらず、残りが 1 の場合は
案内が閉じておらず、いずれもシナリオ3を検証できていない）。閉じる操作の後に「実行中は閉じられません」の
案内が表示されないこと（表示されるとプロセスが終了せず `False` になる）。

**検査スクリプトの注意**: 案内（OK ボタンだけのメッセージボックス）は `WM_CLOSE` で閉じる。
`WM_COMMAND(IDOK)` を送っても閉じないことを 2026-11-02 に開発機（Windows 11）で実測した。この場合
メインウィンドウへの「キャンセル」がモーダルの案内に阻まれ、アプリ側が正しくても `閉じた=False` になる。

---

## 手順3b: DT-1803（結果一覧にフォーカスがあれば Esc・Enter が届くこと、完全自動）

FR-1808 の前提（[research.md](./research.md) R-1806）を確かめる。起動直後のアプリで、結果一覧へ
`WM_KEYDOWN` を直接送る。結果一覧が Enter・Esc を自分で受け取らない部品であれば、ダイアログマネージャが
それぞれ OK・キャンセルへ回し、待機中なのでダイアログが閉じる。

```powershell
Add-Type -Namespace K -Name U -MemberDefinition @'
[DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
[DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr h, int id);
[DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
'@
$id = (Select-String -Path MFCApplication1\Resource.h -Pattern 'IDC_LIST_RESULT\s+(\d+)').Matches[0].Groups[1].Value
foreach ($k in @(@{n='Enter';vk=0x0D}, @{n='Esc';vk=0x1B})) {
  $p = Start-Process MFCApplication1\x64\Debug\MFCApplication1.exe -PassThru
  $null = $p.WaitForInputIdle(5000); Start-Sleep -Milliseconds 500; $p.Refresh()
  $list = [K.U]::GetDlgItem($p.MainWindowHandle, [int]$id)
  $code = [K.U]::SendMessage($list, 0x0087, [IntPtr]$k.vk, [IntPtr]::Zero)          # WM_GETDLGCODE
  [K.U]::PostMessage($list, 0x0100, [IntPtr]$k.vk, [IntPtr]::Zero) | Out-Null        # WM_KEYDOWN
  [K.U]::PostMessage($list, 0x0101, [IntPtr]$k.vk, [IntPtr]0xC0000001) | Out-Null   # WM_KEYUP
  "{0}: WM_GETDLGCODE=0x{1:X} 閉じた={2}" -f $k.n, $code.ToInt64(), $p.WaitForExit(3000)
  if (-not $p.HasExited) { $p | Stop-Process -Force }
}
```

**合格条件**: Enter・Esc の両方で `閉じた=True`。`WM_GETDLGCODE` に `DLGC_WANTALLKEYS`（0x4）が含まれない
こと（2026-11-03 の実測値は `0x81`）。

**この検査の限界**: 「生成」を押した後に実際にフォーカスが結果一覧へ移ることは、フォルダ選択を伴うため
自動化できない。それは DT-1801 の Esc・Enter の項目（利用者の実機確認）で確かめる。

---

## 手順4: DT-1801（実行中は閉じられないこと、利用者による実機実行が必須）

導入処理の手順6（CLAUDE.md の生成）は 30〜60 秒程度かかるため、その間に操作するとよい。

1. `MFCApplication1\x64\Debug\MFCApplication1.exe` を起動し、「参照...」で `C:\work\claude\test\test01` を選ぶ。
2. 「生成」を押す。結果一覧に手順が並び始め、案内欄が「導入処理を実行しています...」になったことを確認する。
3. 完了する前に、次の操作を1つずつ行い、そのたびに確認する。
   - 「キャンセル」を押す
   - 「OK」を押す
   - タイトルバーの ✖ を押す
   - Esc キーを押す
   - Alt+F4 を押す
   - Enter キーを押す（既定のボタン「OK」が押されたとみなされる操作）
4. 各操作で確認すること:
   - (a) 「導入処理の実行中は閉じられません。完了までお待ちください。」が表示される（FR-1802）
   - (b) 案内の「OK」を押すと、ダイアログは表示されたまま残る（FR-1801）
   - (c) 結果一覧への手順の追加が止まらずに続く（FR-1803）
5. 全手順が完了し、完了案内が表示されたら、✖ を押す。案内が出ずにダイアログが閉じることを確認する
   （FR-1804、spec User Story 2 シナリオ2）。
6. （任意）もう一度起動して「生成」を押し、実行中に「キャンセル」を押して案内を表示させたまま、案内を
   閉じずに導入処理の完了まで待つ。完了後に案内の「OK」を押し、結果一覧と完了案内が表示されていること、
   続けて ✖ を押すと案内が出ずに閉じることを確認する（spec Edge Case「案内を表示している間に導入処理が
   完了した場合」）。

**合格条件**: 手順3の6操作すべてで (a)〜(c) を満たし、手順5でダイアログが閉じること。手順6は任意。
Esc・Enter は、「生成」を押した後に**画面のどこもクリックせずに**押すこと（クリックするとフォーカスが
戻り、FR-1808 の確認にならない）。手順5も、クリックせずに Esc で閉じられることを追加で確かめる。

**注意**: test01 のように既に SDD 化したフォルダでは、6手順すべてが「導入済み」で数秒のうちに終わる。
閉じる操作が実行中に間に合わず、正しく閉じてしまったのを「不合格」と誤認しやすい（2026-11-03 に実際に
起きた）。`.sln` だけを置いた新しいフォルダ（例: `C:\work\claude\test\test02`）を使うと、手順6だけで
30〜60 秒かかる。

**手順3に含めない閉じる操作について**: システムメニュー（タイトルバー左端のアイコン、または Alt+Space）の
「閉じる」と、タスクバーのウィンドウ一覧からの「ウィンドウを閉じる」は、✖・Alt+F4 と同じ
`WM_SYSCOMMAND(SC_CLOSE)` の経路を通る（[research.md](./research.md) R-1801）。そのため ✖ と Alt+F4 の
確認で代える。

**補足**: 手順1〜4（uv・specify・VS Code 拡張・Spec Kit 足場）が「既に導入済み」で一瞬で終わる環境では、
操作が間に合わないことがある。その場合は手順6（CLAUDE.md の生成）の実行中に操作する。

---

## 完了条件

- [ ] ST-1801: 4構成のビルドで警告0・エラー0、コード分析の指摘0
- [ ] ST-1802〜1806 がすべて合格（ST-1804 の目視レビューを含む）
- [ ] DT-1802（自動）が4ケースすべて合格
- [ ] DT-1803（自動）が Enter・Esc の両方で合格
- [ ] DT-1801（利用者による実機実行）が合格

---

## 付録: コード分析

```powershell
$msb='C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe'
& $msb MFCApplication1\MFCApplication1.vcxproj /p:Configuration=Debug /p:Platform=x64 /t:Rebuild `
  /p:RunCodeAnalysis=true /p:EnablePREfast=true /v:minimal /nologo 2>&1 | Select-String 'warning'
```

**合格条件**: 出力が空であること。
