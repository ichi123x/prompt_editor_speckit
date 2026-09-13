# Contract: `ProcessRunner::Run()` へのPATH追記の振る舞い契約

**Feature**: [../spec.md](../spec.md) | **Date**: 2026-09-13

008で定義した[process-runner-contract.md](../../008-fix-external-command-hang/contracts/process-runner-contract.md)
の契約（シグネチャ、タイムアウト・残存プロセスに関する事後条件）は本フィーチャーでも一切変更しない。
本書はその契約に**追加**される、PATH追記に関する事後条件のみを記述する。

## シグネチャ（変更なし）

```cpp
ProcessResult Run(const CString& commandLine,
                   const CString& workingDir = CString(),
                   DWORD timeoutMs = 10 * 60 * 1000);
```

## 追加の事前処理（`CreateProcess`を呼ぶ前）

`Run()`は、子プロセスを起動する前に必ず一度、以下を行う:

1. 現在のプロセスのPATH環境変数を取得する
2. レジストリの`HKEY_CURRENT_USER\Environment`と
   `HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment`から
   それぞれ`Path`値を読み取る（[research.md](../research.md) R-901）
3. 「現在のプロセスのPATH」＋「システムPATH」＋「ユーザーPATH」の順に連結し、現在のプロセスの
   PATH環境変数へ反映する（[research.md](../research.md) R-902）

## 追加の事後条件

1. **既存のPATHは失われない**: レジストリから読み取れた値の有無にかかわらず、この処理の前に
   プロセスが持っていたPATHの内容は、合成後のPATHに引き続き含まれる（追記であり置き換えではない）
2. **レジストリ読み取りが両方失敗しても`Run()`は失敗しない**: `launched`・`exitCode`・`output`の
   意味は008の契約のまま変わらない。PATH追記の失敗は`ProcessResult`に一切現れない（内部的な
   ベストエフォート処理であり、呼び出し元に通知するインターフェースを持たない）
3. **`commandLine`・`workingDir`・`timeoutMs`の解釈は変わらない**: PATH追記は子プロセスの検索パスに
   影響するのみで、`commandLine`に明示的なパスが含まれる場合の挙動（PATH探索を経由しない）には
   影響しない

## 呼び出し元が守るべき前提（不変）

- `SetupRunner.cpp`は本フィーチャーによって一切変更されない。各手順のコードは無修正で動作し続ける
  （FR-906、[research.md](../research.md) R-903）
