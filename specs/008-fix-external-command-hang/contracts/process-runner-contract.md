# Contract: `ProcessRunner::Run()` の振る舞い契約

**Feature**: [../spec.md](../spec.md) | **Date**: 2026-09-13

本フィーチャーはUIやAPIの外部公開契約を持たない（デスクトップアプリ内部のみで完結する）。
代わりに、`SetupRunner.cpp` の5手順すべてが依存する内部インターフェース `ProcessRunner::Run()`
（[ProcessRunner.h](../../../MFCApplication1/ProcessRunner.h)）について、本フィーチャーで**変えない部分**と
**変える部分**を明示する。

## シグネチャ（変更なし）

```cpp
ProcessResult Run(const CString& commandLine,
                   const CString& workingDir = CString(),
                   DWORD timeoutMs = 10 * 60 * 1000);
```

引数・戻り値の型、デフォルト値は一切変更しない。既存の全呼び出し元（`SetupRunner.cpp` の
`RunInstallUv` / `RunInstallSpecify` / `RunInstallVsCodeExt` / `RunInitSpecKit` / `RunInitGit`）は
無修正のまま動作する。

## 事前条件（Preconditions）

- `commandLine` は空でない（既存と同じ）
- `timeoutMs` は0より大きい（既存と同じ）

## 事後条件（Postconditions）— 太字が本フィーチャーで新たに保証する内容

1. `launched == false` の場合: プロセスは起動できなかった（PATH上に存在しない等）。既存と同じ
2. `launched == true` かつ子プロセスが `timeoutMs` 以内に自ら終了した場合: `exitCode` はその終了コード、
   `output` は標準出力・標準エラーの結合文字列。既存と同じ
3. `launched == true` かつ子プロセスが `timeoutMs` を超えても終了しなかった場合（**出力を出し続けている場合と、
   出力を止めて無応答になっている場合の両方を含む** — ここが本フィーチャーによる保証の拡大点）:
   - **`Run()` は `timeoutMs` から大きく超過することなく（同オーダーの時間内に）呼び出し元へ制御を返す**
   - `exitCode == WAIT_TIMEOUT`（`258`）であり、`Succeeded()` は `false` を返す（既存の値を踏襲、[research.md](../research.md) R-803）
   - **子プロセスおよびその子プロセスが起動した子孫プロセスは、`Run()` が返った時点で終了している**
     （[research.md](../research.md) R-802・R-804）
4. `Run()` はいずれの経路でも、生成した全てのハンドル（プロセス・スレッド・パイプ・ジョブオブジェクト）を
   関数を抜けるまでに解放する。呼び出し元から見て残留ハンドルは存在しない

## 呼び出し元が守るべき前提（不変）

- `SetupRunner.cpp` の各手順は、`Run()` が返す `ProcessResult` の `launched` / `Succeeded()` / `output` /
  `exitCode` のみを見て分岐しており、本フィーチャーはこれらのセマンティクスを変えないため、
  各手順のコードは無修正で動作し続ける（FR-805）
