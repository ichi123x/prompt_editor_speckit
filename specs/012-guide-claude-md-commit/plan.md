# Implementation Plan: 完了案内にCLAUDE.mdのコミット手順を追加する

**Branch**: `012-guide-claude-md-commit` | **Date**: 2026-09-13 | **Spec**: [spec.md](./spec.md)

**Input**: `/specs/012-guide-claude-md-commit/spec.md` からの機能仕様

## Summary（概要）

ユーザー提供の導入手順書（SpecKitVSCodesetup.pdf 手順08）と、本アプリの完了案内（`MFCApplication1Dlg.cpp`の`ShowCompletionGuide()`）を突き合わせたところ、`/init`実行によるCLAUDE.md生成の直後にそれをGitへコミットする手順が、案内文から欠落していることが判明した。

技術的アプローチは、`ShowCompletionGuide()`内の成功時の案内文字列（`guide.Format(...)`）に1手順を追加する。現行の手順2（`/init`実行）と手順3（テスト方針決定・憲章制定）の間に、「生成されたCLAUDE.mdをコミットする」手順を挿入し、以降の手順番号を1つずつ繰り下げる。既存の各手順の文言・`m_workerPath`の埋め込み方法は変更しない。失敗時の案内文（`anyFailed`が真の場合の文言）は本フィーチャーの対象外であり、一切変更しない（FR-1203）。

**`/speckit-analyze`による指摘とスコープの訂正**: 当初はこの文字列変更のみで完結する計画だったが、
006（起動直後のまま結果と案内が読める）のFR-507aが「案内欄の表示行数の余裕は0行であり、案内を1行
増やす変更はそのままでは表示が切れる。文言を足す前にこの表を確認すること」と明記していることが
`/speckit-analyze`で判明した。本フィーチャーは案内を5行から6行（見出し含む）に増やすため、このまま
文字列だけを変更すると、追加した新しい手順自体が画面から見切れる可能性が高い。そこで、`.rc`のダイアログ
リソースにある案内欄（`IDC_STATIC_GUIDE`）の高さも合わせて調整することをスコープに追加した
（FR-1205、[research.md](./research.md) R-1202）。004・006の前例により、`.rc`上の初期矩形を変更する
だけで004のリサイズ追従ロジック（`OnSize`/`OnGetMinMaxInfo`）は自動的に追従するため、`.cpp`側の
追従ロジックへの変更は不要である。

## Technical Context（技術コンテキスト）

**Language/Version**: C++（MSVC v143、Unicode、C++17）

**Primary Dependencies**: なし（既存の`CString::Format`を使うのみ）

**Storage**: N/A

**Testing**: 自動テストスイートは存在しない（憲章の技術的制約）。静的テスト ST-1201〜1202 を [quickstart.md](./quickstart.md) の手順として定義する。動的テスト DT-1201（実機でのフルフロー確認）は、フォルダ選択ダイアログを伴い008以降と同じ理由で自動化できないため、利用者による任意の手動実行とする

**Target Platform**: Windows 10/11、開発機の表示環境（実測115 DPI、006と同じ基準）

**Project Type**: desktop-app（MFC ダイアログアプリケーション）

**Performance Goals**: N/A（文字列リテラルの追記と、`.rc`の初期矩形の数値変更のみ）

**Constraints**: `ShowCompletionGuide()`が参照する`m_steps`・`m_workerPath`等のメンバー変数、関数のシグネチャ・呼び出し箇所（`OnSetupCompletedMessage`からの1箇所）は変更しない。`SetupRunner.cpp`・`ProcessRunner.cpp`（008〜011の成果）には一切触れない（FR-1203）。新しいクラス・メンバー変数は追加しない（原則III YAGNI）。004のリサイズ追従8方針（`AnchorMode`の割り当て）・`OnSize`/`OnGetMinMaxInfo`のロジックは変更しない（006 FR-508の継承）。文字の大きさ・既存の列幅比率（006の成果）は変更しない

**Scale/Scope**: 変更するファイルは`MFCApplication1Dlg.cpp`と`MFCApplication1.rc`の2ファイル。`ShowCompletionGuide()`内の`guide.Format(...)`呼び出し1箇所と、`.rc`内の`IDC_STATIC_GUIDE`の高さ・`IDOK`/`IDCANCEL`のY座標・ダイアログ全体の高さ（計4箇所の数値、006と同じ「差分がそのままダイアログの高さに乗る」関係）を書き換える程度

## Constitution Check（憲法チェック）

*ゲート: Phase 0 のリサーチ前に通過する必要がある。Phase 1 の設計後に再チェックすること。*

| 原則 | 判定 | 根拠 |
|---|---|---|
| **I. Spec Kit ワークフロー厳守** | 合格 | `/speckit-specify` → `/speckit-plan` の順で進めている。実装は `/speckit-tasks`・`/speckit-analyze` の後 |
| **II. 日本語での一貫性** | 合格 | 追加する案内文言も既存と同じ日本語。コメントと成果物はすべて日本語 |
| **III. シンプルさ優先（YAGNI）** | 合格 | 新しいクラス・関数・状態を追加せず、既存の`guide.Format(...)`の文字列リテラルと、`.rc`の数値4箇所を書き換えるのみ。004・006が確立した追従の仕組みをそのまま利用し、新しい追従ロジックは作らない |
| **IV. 変更の追跡可能性** | 合格 | 変更箇所に FR-1201〜1205 に対応するコメントを付す（`.rc`側はコメント形式が`.cpp`と異なるため、006 R-503等の前例に倣いFR番号を`.rc`のコメント行に記載する）。ST-1201〜1202 がコメントの存在を機械的に検査する |

**技術的制約との整合**:

- MSBuild のみを使用し、CMake 等は導入しない — 合格（変更なし）
- 新しい `.ps1` をプロジェクトへ追加しない — 合格
- テストランナーを新規導入しない — 合格（ST/DT の手順として定義）

**設計後の再チェック（Phase 1 完了時点）**: Phase 1 の成果物（[data-model.md](./data-model.md)・[quickstart.md](./quickstart.md)）を作成した後も、上記の判定は変わらない。**違反ゼロのため Complexity Tracking セクションは記入しない。**

## Project Structure（プロジェクト構造）

### Documentation（このフィーチャー）

```text
specs/012-guide-claude-md-commit/
├── plan.md              # このファイル
├── research.md          # Phase 0 の出力（R-1201〜R-1202）
├── data-model.md        # Phase 1 の出力（新規永続エンティティなし）
├── quickstart.md        # Phase 1 の出力（ST-1201〜1202、DT-1201）
├── checklists/
│   └── requirements.md  # spec 品質チェックリスト（全項目合格）
└── tasks.md             # Phase 2 の出力（/speckit-tasks が作成。本コマンドでは作らない）
```

**`contracts/`を作成しない理由**: `ShowCompletionGuide()`はUIの表示文字列を組み立てる非公開メンバー関数
であり、呼び出し元は`OnSetupCompletedMessage`の1箇所のみ。008・010と同様、複数箇所から参照される
内部インターフェースの契約変更を伴わないため、Phase 1手順3の規定に従いスキップする。

### Source Code（リポジトリのルート）

```text
MFCApplication1/
├── MFCApplication1Dlg.cpp   # 変更するファイル
│                            #   - ShowCompletionGuide() の成功時の案内文字列に
│                            #     CLAUDE.mdコミット手順を1つ追加 (FR-1201〜1204)
├── MFCApplication1.rc       # 変更するファイル
│                            #   - IDC_STATIC_GUIDE の高さを 52→61 DLU に拡張 (FR-1205)
│                            #   - IDOK・IDCANCEL の Y座標を 203→212 に変更（案内欄の
│                            #     拡張分だけ下へずらす、006 R-507 と同じ関係）
│                            #   - ダイアログ全体の高さを 224→233 DLU に拡張（差分の
│                            #     9 DLU がそのままダイアログ高さに乗る、006 R-507 と同じ関係）
├── MFCApplication1Dlg.h     # 変更しない
├── SetupRunner.cpp          # 変更しない（008〜011の成果、無変更で利用するのみ）
├── ProcessRunner.cpp        # 変更しない
├── ProcessRunner.h          # 変更しない
├── TargetFolderState.*      # 変更しない
├── GitignoreWriter.*        # 変更しない
└── res/gitignore_all.txt    # 変更しない
```

**Structure Decision**: 既存の平坦な構成をそのまま維持する。案内文字列は元々この1関数に閉じており、
修正もその内部（文字列リテラルと引数）に閉じる。案内欄の寸法調整も、既存の`.rc`の初期矩形を数値で
変更するだけであり、004・006が確立した追従ロジック（`AnchorMode`、`OnSize`/`OnGetMinMaxInfo`）は
無修正のまま自動的に新しい寸法に追従する（006 R-507の前例）。新しいファイル・新しいクラスは作らない。

## Complexity Tracking（複雑さの追跡）

違反なし。本フィーチャーは既存関数`ShowCompletionGuide()`内の文字列リテラルと、`.rc`の初期矩形の
数値4箇所を書き換えるのみで、新しい抽象化・公開インターフェース・永続状態を一切導入しない。
`.rc`の変更が加わった点は当初のスコープからの拡大だが、006 FR-507aが明示的に義務付けている
対応であり、憲法違反ではなく既存フィーチャーの制約を満たすための必然的な変更である。
