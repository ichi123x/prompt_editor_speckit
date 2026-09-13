# Phase 0 Research: specify CLI導入後のPATH未登録を解消する

**Feature**: [spec.md](./spec.md) | **Date**: 2026-09-13

## R-1001: PATH登録手段の選定

**Decision**: `uv tool update-shell`を、`uv tool install specify-cli`が成功した直後に呼ぶ。

**事実確認（実機）**: この開発機（uv 0.12.13）で手動実行し、効果を確認した。

```text
> uv tool update-shell
Updated PATH to include executable directory C:\Users\user\.local\bin
Restart your shell to apply changes
```

実行前後でユーザーPATH（`HKCU\Environment\Path`）を比較したところ、`C:\Users\user\.local\bin`が
新たに追加されていた。これは`uv tool dir`が返す`%APPDATA%\uv\tools`（ツール本体の格納場所）とは別の、
実行可能ファイルの配置場所（既定`%USERPROFILE%\.local\bin`）である。

**Rationale**:
- uv自身が「このバージョンのuvが実際にどこへexeを配置し、どのディレクトリをPATHに登録すべきか」を
  正確に知っている。既定ディレクトリをアプリ側でハードコードして自前でレジストリへ書き込む案は、
  ユーザーが`UV_TOOL_BIN_DIR`環境変数等でuvの既定ディレクトリを変更している場合に追従できず、
  uv側の将来的な既定値変更にも追従できない
- `uv tool install --help`を確認したが、インストールと同時にPATH登録まで行うフラグは見当たらなかった
  （`--no-sources`等はあるが、シェル環境更新に関するものはない）。別コマンドとして明示的に呼ぶ必要がある
- 009で実装した「`ProcessRunner::Run()`の先頭でレジストリのシステム/ユーザーPATHを読み直し、
  現在のプロセスのPATHへ追記する」仕組みと自然に組み合わさる。`uv tool update-shell`は
  レジストリを書き換えるだけで、このアプリ自身のプロセスのPATHをその場で更新するわけではないが、
  次に`ProcessRunner::Run()`が呼ばれる際（手順4）に009の仕組みがレジストリを読み直すため、
  結果的にアプリを再起動しなくても手順4から検出できる

**Alternatives considered**:
- 既定ディレクトリ（`%USERPROFILE%\.local\bin`）を本アプリがハードコードし、自前でレジストリの
  ユーザーPATHへ追記する: uvの実装詳細への依存が生じ、uvのバージョンアップや設定変更で
  ディレクトリが変わった場合に追従できなくなる。uv公式のコマンドに任せる方が保守性が高いため却下
- `uv tool install`の代わりに`pip install`等、PATH登録を自動で行う別のインストール手段に変更する:
  spec Kit公式のインストール手順から逸脱し、影響範囲が本フィーチャーのスコープを超える。却下

## R-1002: 呼び出し箇所

**Decision**: `SetupRunner.cpp`の`RunInstallSpecify`関数内、`uv tool install specify-cli`の
`ProcessResult`が`Succeeded()`と判定された**直後**、`Finish(step, StepResult::Success, ...)`を
呼ぶ**前**に、`ProcessRunner::Run(_T("uv tool update-shell"), CString(), 60 * 1000)`を追加で呼ぶ。

**Rationale**:
- specify CLIの新規インストールが発生したのはこの分岐だけであり、PATH登録が必要になるのも
  この場合だけ（FR-1002: スキップ時は不要）。この分岐に直接書くのが最も単純
- `ProcessRunner.cpp`・`ProcessRunner.h`には一切触れない。008・009の成果物を無傷のまま利用するのみ
- 他の手順（winget・code・git）はいずれもこの問題を持たない（wingetは既にレジストリPATHへ登録する
  ことを008調査時点で確認済み、codeはVS Code拡張の追加でありPATH登録を伴わない、gitはこのアプリが
  インストールしない）。したがって`ProcessRunner::Run()`側に汎用的な仕組みを追加する必要はなく、
  `RunInstallSpecify`固有のロジックとして閉じ込める（FR-1005、YAGNI）

**Alternatives considered**:
- `ProcessRunner::Run()`に「インストール系コマンドの後は自動でシェル更新する」ような汎用フックを
  追加する: 対象がuvのtool installに固有の挙動であり、他の呼び出し（`git`、`code`等）には
  無関係な処理を汎用層に持ち込むことになる。原則III（YAGNI）に反するため却下
- `RunInitSpecKit`（手順4）側で、`specify`が見つからない場合にフォールバックとして
  `uv tool update-shell`を呼んでからリトライする: 問題の発生源（手順2でPATH登録をしていないこと）
  ではなく症状が出た場所で対処療法をする形になり、手順4のロジックに手順2の都合を持ち込むことになる。
  発生源である手順2で完結させる方が単純で分かりやすいため却下

## R-1003: 失敗時の扱い

**Decision**: `uv tool update-shell`の`ProcessResult`（`launched`・`Succeeded()`等）は一切判定に使わず、
呼びっぱなしにする。

**Rationale**:
- FR-1003が要求するとおり、この呼び出しが失敗しても手順2自体の成否（`uv tool install`の結果）には
  影響させない。仮に`uv tool update-shell`が失敗しても、起きることは「これまでと同じ、手順4が
  『見つかりません』で失敗する」という、本フィーチャー導入前から存在した挙動に留まり、新しい
  壊れ方を持ち込まない
- 結果を無視することで、新しいエラー分岐・新しいUI表示を追加する必要がなくなる（原則III YAGNI）。
  `uv tool update-shell`はuv公式のドキュメントに載っている安定したコマンドであり、通常時に
  失敗するケースは想定しにくい

**Alternatives considered**:
- 失敗時に結果一覧へ警告を追加する: 本処理は手順2の内部的な後処理であり、独立した結果行を持たない。
  失敗しても既存の「見つかりません」表示が手順4で自然にフォールバックするため、新しい通知を
  追加する必要性が薄い（原則III YAGNI）。却下
