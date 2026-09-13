# Implementation Plan: specify CLI導入後のPATH未登録を解消する

**Branch**: `010-specify-path-registration` | **Date**: 2026-09-13 | **Spec**: [spec.md](./spec.md)

**Input**: `/specs/010-specify-path-registration/spec.md` からの機能仕様

## Summary（概要）

009でレジストリPATHの反映遅延を解消した直後、実機で新たに確認された問題を修正する。`uv tool install specify-cli`はspecify.exeを既定のディレクトリ（`%USERPROFILE%\.local\bin`）へ配置するが、**レジストリのPATHにそのディレクトリを登録しない**。そのため、009の対策（`ProcessRunner::Run()`でのレジストリPATH再読み込み）を持ってしても、レジストリに元々存在しないものは読み直しても見つからず、続く手順4（Spec Kit足場配置）が「specifyが見つかりません」として失敗する。

技術的アプローチは、`SetupRunner.cpp`の`RunInstallSpecify`（手順2）内、`uv tool install specify-cli`が成功した直後に、`uv tool update-shell`を追加で呼び出す。これはuv自身が提供する「シェル環境（PATH）を更新する」公式コマンドであり、実機（uv 0.12.13）で効果を確認済み（`Updated PATH to include executable directory ...`という出力とともにレジストリのユーザーPATHへ追記される）。この呼び出し結果（成否）は手順2自体の判定に一切影響させない（FR-1003） — 失敗しても、これまでと同じ「見つかりません」という既存の失敗表示にフォールバックするだけであり、新しい壊れ方を持ち込まない。

009で実装済みの「`ProcessRunner::Run()`の先頭でレジストリPATHを読み直し既存PATHへ追記する」仕組みと組み合わさることで、`uv tool update-shell`がレジストリへ書き込んだ内容を、続く手順4の`ProcessRunner::Run()`呼び出しが正しく拾える。**本フィーチャーは009に依存する。**

## Technical Context（技術コンテキスト）

**Language/Version**: C++（MSVC v143、Unicode、C++17）

**Primary Dependencies**: なし（新しいWin32 API・ライブラリは追加しない。既存の`ProcessRunner::Run()`を1回追加で呼ぶのみ）

**Storage**: N/A

**Testing**: 自動テストスイートは存在しない（憲章の技術的制約）。静的テスト ST-1001、動的テスト DT-1001 を [quickstart.md](./quickstart.md) の手順として定義する

**Target Platform**: Windows 10/11、uvが導入済みの環境。`uv tool update-shell`サブコマンドの存在が前提（実機のuv 0.12.13で確認済み。仕様上安定したuvの公式コマンドであり、将来的な破壊的変更のリスクは低いと判断）

**Project Type**: desktop-app（MFC ダイアログアプリケーション）

**Performance Goals**: N/A。specify CLIが新規インストールされた場合にのみ、追加で1回の軽量なプロセス起動が発生する（既に導入済みでスキップされる場合は発生しない）

**Constraints**: `SetupRunner.cpp`の`RunInstallSpecify`内に閉じる変更とし、`ProcessRunner.cpp`・`ProcessRunner.h`は変更しない（009の成果物に一切手を加えない）。新しいクラス・ヘッダファイルは追加しない（原則III YAGNI）。008・009とは異なり、他の手順（winget・code・git）への汎用化は行わない（FR-1005、uv tool install固有の挙動のため）

**Scale/Scope**: 変更するファイルは`SetupRunner.cpp`の1ファイルのみ。`RunInstallSpecify`関数内に数行追加する程度

## Constitution Check（憲法チェック）

*ゲート: Phase 0 のリサーチ前に通過する必要がある。Phase 1 の設計後に再チェックすること。*

| 原則 | 判定 | 根拠 |
|---|---|---|
| **I. Spec Kit ワークフロー厳守** | 合格 | `/speckit-specify` → `/speckit-plan` の順で進めている。実装は `/speckit-tasks`・`/speckit-analyze` の後 |
| **II. 日本語での一貫性** | 合格 | 本フィーチャーは新規UI文字列を追加しない。コメントと成果物はすべて日本語 |
| **III. シンプルさ優先（YAGNI）** | 合格 | 新しいクラス・関数・状態を追加せず、既存の`RunInstallSpecify`内に数行追加するのみ。他手順への汎用化も意図的に行わない（FR-1005で既に正当化済み） |
| **IV. 変更の追跡可能性** | 合格 | 追加する箇所に FR-1001〜1005 に対応するコメントを付す。ST-1001 がコメントの存在を機械的に検査する |

**技術的制約との整合**:

- MSBuild のみを使用し、CMake 等は導入しない — 合格（変更なし）
- 新しい `.ps1` をプロジェクトへ追加しない — 合格
- テストランナーを新規導入しない — 合格（ST/DT の手順として定義）

**設計後の再チェック（Phase 1 完了時点）**: Phase 1 の成果物（[data-model.md](./data-model.md)・[quickstart.md](./quickstart.md)）を作成した後も、上記の判定は変わらない。**違反ゼロのため Complexity Tracking セクションは記入しない。**

## Project Structure（プロジェクト構造）

### Documentation（このフィーチャー）

```text
specs/010-specify-path-registration/
├── plan.md              # このファイル
├── research.md          # Phase 0 の出力（R-1001〜R-1003）
├── data-model.md        # Phase 1 の出力（新規永続エンティティなし）
├── quickstart.md        # Phase 1 の出力（ST-1001、DT-1001）
├── checklists/
│   └── requirements.md  # spec 品質チェックリスト（全項目合格）
└── tasks.md             # Phase 2 の出力（/speckit-tasks が作成。本コマンドでは作らない）
```

**`contracts/`を作成しない理由**: 本フィーチャーは`RunInstallSpecify`という内部関数1つに閉じた変更であり、`ProcessRunner::Run()`のような複数箇所から参照される内部インターフェースの契約を変更しない。008・009とは異なり、外部から観測可能な振る舞い契約の変更がないため、プロジェクトが純粋に内部向けの変更である場合の規定（Phase 1手順3）に従いスキップする。

### Source Code（リポジトリのルート）

```text
MFCApplication1/
├── SetupRunner.cpp          # 変更するファイル
│                            #   - RunInstallSpecify 内、uv tool install specify-cli が
│                            #     成功した直後（Finish(Success)の前）に
│                            #     uv tool update-shell を追加で呼ぶ (FR-1001〜1005)
│                            #     結果は判定に使わない（呼びっぱなし、FR-1003）
├── ProcessRunner.cpp        # 変更しない（009の成果物、無変更で利用するのみ）
├── ProcessRunner.h          # 変更しない
├── MFCApplication1Dlg.*     # 変更しない
├── TargetFolderState.*      # 変更しない
├── GitignoreWriter.*        # 変更しない
└── res/gitignore_all.txt    # 変更しない
```

**Structure Decision**: 既存の平坦な構成をそのまま維持する。`uv tool update-shell`の呼び出しは、それを必要とする唯一の手順（specify CLI導入）のロジックの一部として、その関数内に直接書く。新しいファイル・新しいクラスは作らない（[research.md](./research.md) R-1002）。

## Complexity Tracking（複雑さの追跡）

違反なし。本フィーチャーは既存関数内への数行の追加のみで、新しい抽象化・公開インターフェース・永続状態を一切導入しない。
