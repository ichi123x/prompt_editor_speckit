# Implementation Plan: 新規インストール直後のPATH未反映を解消する

**Branch**: `009-refresh-path-after-install` | **Date**: 2026-09-13 | **Spec**: [spec.md](./spec.md)

**Input**: `/specs/009-refresh-path-after-install/spec.md` からの機能仕様

## Summary（概要）

008でwingetの対話プロンプト起因のハングを解消した結果、実機で新たに顕在化した問題を修正する。`winget install`や`uv tool install`でツールを新規インストールしても、それらのインストーラーが更新するのはレジストリ上のPATHであり、既に起動済みの本アプリのプロセスが保持するPATHのコピーには反映されない。そのため、同一実行内の後続手順（例: 手順1でuvを新規導入した直後の手順2）が、ディスク上には存在するコマンドを「見つかりません」として誤って失敗させる。

技術的アプローチは、`ProcessRunner::Run()`（[ProcessRunner.cpp](../../MFCApplication1/ProcessRunner.cpp)）の先頭、`CreateProcess`を呼ぶ**前**に、レジストリ（`HKEY_CURRENT_USER\Environment`と`HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment`の両方の`Path`値）を読み直し、**現在のプロセスが既に持っているPATHの末尾に追記する**形で`SetEnvironmentVariable`を呼ぶ。置き換えではなく追記にすることで、プロセス起動時に持っていた情報を一切失わず、レジストリ読み取りが失敗しても既存の動作を壊さない（FR-903・FR-905）。呼び出し箇所を`ProcessRunner::Run()`の1箇所に集約するため、`SetupRunner.cpp`は一切変更しない — これにより5手順の実行順序・依存関係・べき等性（FR-906）が変わらないことを構造的に保証する。

## Technical Context（技術コンテキスト）

**Language/Version**: C++（MSVC v143、Unicode、C++17）

**Primary Dependencies**: Win32 API のみ。`RegGetValueW`（`advapi32.lib`）と`SetEnvironmentVariable`（`kernel32.lib`）を新規に使用するが、いずれもVisual Studioの既定リンクライブラリに含まれており、`.vcxproj`への追加設定は不要

**Storage**: N/A（本フィーチャーはプロセスの環境変数という実行時状態のみを扱い、何も永続化しない）

**Testing**: 自動テストスイートは存在しない（憲章の技術的制約）。静的テスト ST-901〜902、動的テスト DT-901〜903 を [quickstart.md](./quickstart.md) の手順として定義する（DT-903はUS2の耐障害性をホワイトボックス方式で確認するテストで、`/speckit-tasks`での検証方法検討時に追加）

**Target Platform**: Windows 10/11。レジストリの2キー（`HKCU\Environment`、`HKLM\SYSTEM\...\Session Manager\Environment`）はいずれのバージョンでも標準的に存在する

**Project Type**: desktop-app（MFC ダイアログアプリケーション）

**Performance Goals**: N/A。`ProcessRunner::Run()`は本アプリの1回の実行で最大でも十数回呼ばれる程度であり、レジストリ読み取り2回の追加コストは体感できない

**Constraints**: `ProcessRunner::Run()`のシグネチャは変更しない（008の契約を継続）。`SetupRunner.cpp`は変更しない（FR-906を構造的に満たすため）。新しいクラス・ヘッダファイルは追加せず、`ProcessRunner.cpp`内部の実装のみで完結させる（原則III YAGNI）。新しい`.ps1`は追加しない（憲章の技術的制約）

**Scale/Scope**: 変更するファイルは`ProcessRunner.cpp`の1ファイルのみ。追加は非公開ヘルパー関数2つ（レジストリ1キー読み取り・PATH追記）で、合計40〜60行程度

## Constitution Check（憲法チェック）

*ゲート: Phase 0 のリサーチ前に通過する必要がある。Phase 1 の設計後に再チェックすること。*

| 原則 | 判定 | 根拠 |
|---|---|---|
| **I. Spec Kit ワークフロー厳守** | 合格 | `/speckit-specify` → `/speckit-plan` の順で進めている。実装は `/speckit-tasks`・`/speckit-analyze` の後 |
| **II. 日本語での一貫性** | 合格 | 本フィーチャーは新規UI文字列を追加しない。コメントと成果物はすべて日本語 |
| **III. シンプルさ優先（YAGNI）** | 合格 | 新しいクラス・公開インターフェース・永続状態を一切追加しない。非公開ヘルパー関数2つのみを`ProcessRunner.cpp`内部に追加する。呼び出し箇所を1箇所（`Run()`の先頭）に集約し、`SetupRunner.cpp`への変更を避けることで、必要以上の変更範囲の拡大を避けている |
| **IV. 変更の追跡可能性** | 合格 | 追加する各箇所に FR-901〜906 に対応するコメントを付す。ST-902 がコメントの存在を機械的に検査する |

**技術的制約との整合**:

- MSBuild のみを使用し、CMake 等は導入しない — 合格（変更なし）
- 新しい `.ps1` をプロジェクトへ追加しない — 合格
- テストランナーを新規導入しない — 合格（ST/DT の手順として定義）

**設計後の再チェック（Phase 1 完了時点）**: Phase 1 の成果物（[data-model.md](./data-model.md)・[contracts/path-refresh-contract.md](./contracts/path-refresh-contract.md)・[quickstart.md](./quickstart.md)）を作成した後も、上記の判定は変わらない。**違反ゼロのため Complexity Tracking セクションは記入しない。**

## Project Structure（プロジェクト構造）

### Documentation（このフィーチャー）

```text
specs/009-refresh-path-after-install/
├── plan.md              # このファイル
├── research.md          # Phase 0 の出力（R-901〜R-904）
├── data-model.md        # Phase 1 の出力（新規永続エンティティなし。実行時の合成規則を記述）
├── quickstart.md        # Phase 1 の出力（ST-901〜902、DT-901〜903）
├── contracts/
│   └── path-refresh-contract.md  # Phase 1 の出力（PATH追記の振る舞い契約、008の契約への追補）
├── checklists/
│   └── requirements.md  # spec 品質チェックリスト（全項目合格）
└── tasks.md             # Phase 2 の出力（/speckit-tasks が作成。本コマンドでは作らない）
```

### Source Code（リポジトリのルート）

```text
MFCApplication1/
├── ProcessRunner.cpp        # 変更するファイル
│                            #   - Run() の先頭、CreateProcess を呼ぶ前に、レジストリの
│                            #     ユーザー/システムPATHを読み直し、現在のプロセスのPATH末尾へ
│                            #     追記する非公開ヘルパーを呼ぶ (FR-901〜905)
├── ProcessRunner.h          # 変更しない（Run() のシグネチャは維持）
├── SetupRunner.cpp          # 変更しない（FR-906を構造的に満たすため、呼び出し箇所を増やさない）
├── MFCApplication1Dlg.*     # 変更しない
├── TargetFolderState.*      # 変更しない
├── GitignoreWriter.*        # 変更しない
└── res/gitignore_all.txt    # 変更しない
```

**Structure Decision**: 既存の平坦な構成をそのまま維持する。PATH追記は「外部コマンドを実行する直前に、実行環境を最新化する」という`ProcessRunner`の既存の責務（外部コマンド実行の窓口）の自然な延長として、その内部にのみ追加する。新しいファイル・新しいクラスは作らない（[research.md](./research.md) R-903）。

## Complexity Tracking（複雑さの追跡）

違反なし。上記のとおり、本フィーチャーは非公開ヘルパー関数2つを`ProcessRunner.cpp`内部に追加するのみで、新しい抽象化・公開インターフェース・永続状態を一切導入しない。
