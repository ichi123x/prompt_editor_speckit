# Implementation Plan: 失敗理由が装飾バナーに埋もれて見えなくなる問題を解消する

**Branch**: `011-fix-error-message-truncation` | **Date**: 2026-09-13 | **Spec**: [spec.md](./spec.md)

**Input**: `/specs/011-fix-error-message-truncation/spec.md` からの機能仕様

## Summary（概要）

008〜010で実行時のハング・PATH関連の不具合を解消した結果、実機で初めて顕在化した問題を修正する。`SetupRunner.cpp`の`Brief()`（[SetupRunner.cpp:25](../../MFCApplication1/SetupRunner.cpp#L25)）は、外部コマンドの出力を先頭から`maxLength`（既定160文字）で単純に切り詰めるだけであり、`specify init`のように罫線・ブロック文字によるロゴを最初に出力するツールでは、その装飾だけで文字数予算を使い切ってしまい、実際のエラー本文（実機で確認した例: 「claude not found」を含む`Agent Detection Error`ボックス）が一切表示されない。

技術的アプローチは、`Brief()`を次のように変更する。(1) 出力を行に分割し、各行からUnicodeの罫線描画文字・ブロック要素（U+2500〜U+259F）を除去して、可視文字が残るかどうかで「内容行」と「空行」に分類する。(2) 内容行を段落にまとめる。**空行が1行だけの場合は区切りとみなさず**（罫線ボックス内部の余白として無視する）、**2行以上連続した場合にのみ**段落の区切りとする。(3) 最後の段落を採用し、`maxLength`を超える場合は**その段落の先頭から**切り詰める。(4) 内容行が1つも無かった場合（FR-1103）は、既存どおり元の出力を先頭から切り詰めた結果にフォールバックする。

当初は「装飾除去後のテキスト全体をひとつながりの文字列として末尾優先で切り詰める」という単純な設計を検討したが、`/speckit-analyze`での指摘を受けて実機データ（[research.md](./research.md) R-1101で取得した実際の`specify init`出力）で検証したところ、「claude not found」という見出しの後に続く説明文だけで`maxLength`の予算を使い切ってしまい、末尾優先では肝心の見出しが失われることが実測で判明した。段落単位への分割によって、無関係な先行ボックス（バナー・進行状況等）を自然に除外しつつ、最後のボックス（多くの場合エラーボックス）の内容を丸ごと1つの段落として扱えるため、その段落を先頭から切り詰めるだけで見出しを含む診断情報を確保できる（[research.md](./research.md) R-1102、実機データで再検証済み）。

出力全体が装飾も空行も持たない短い失敗メッセージ（既存の git・winget 等の典型的な失敗）は、内容行がすべて1つの段落にまとまり、かつ`maxLength`以下であるため、**この変更による影響を受けない**（SC-1102）。

## Technical Context（技術コンテキスト）

**Language/Version**: C++（MSVC v143、Unicode、C++17）

**Primary Dependencies**: なし（新しいWin32 API・ライブラリは追加しない。`CString`の既存機能とUnicodeコードポイント比較のみで実装する）

**Storage**: N/A

**Testing**: 自動テストスイートは存在しない（憲章の技術的制約）。静的テスト ST-1101、動的テスト DT-1101 を [quickstart.md](./quickstart.md) の手順として定義する。DT-1101は008・010で確立した「対象の`.cpp`を`#include`して内部関数を直接呼ぶ」ホワイトボックス方式で自動化する

**Target Platform**: Windows 10/11（既存と同じ）

**Project Type**: desktop-app（MFC ダイアログアプリケーション）

**Performance Goals**: N/A。`Brief()`は各手順の失敗時に高々1回、短い文字列（最大でも出力全体の長さ、実務上は数千文字程度）に対して呼ばれるのみで、体感できるコストは生じない

**Constraints**: `Brief()`のシグネチャ（引数・戻り値・既定の`maxLength`）は変更しない — 既存の5箇所の呼び出し元（`RunInstallUv`・`RunInstallSpecify`・`RunInitSpecKit`・`RunInitGit`内の各失敗分岐）を無修正で動作させる。`ProcessRunner.cpp`・`ProcessRunner.h`は変更しない（FR-1105）。新しいクラス・ヘッダファイルは追加しない（原則III YAGNI）

**Scale/Scope**: 変更するファイルは`SetupRunner.cpp`の1ファイルのみ。`Brief()`関数本体（現在13行）を書き換える程度

## Constitution Check（憲法チェック）

*ゲート: Phase 0 のリサーチ前に通過する必要がある。Phase 1 の設計後に再チェックすること。*

| 原則 | 判定 | 根拠 |
|---|---|---|
| **I. Spec Kit ワークフロー厳守** | 合格 | `/speckit-specify` → `/speckit-plan` の順で進めている。実装は `/speckit-tasks`・`/speckit-analyze` の後 |
| **II. 日本語での一貫性** | 合格 | 本フィーチャーは新規UI文字列を追加しない。コメントと成果物はすべて日本語 |
| **III. シンプルさ優先（YAGNI）** | 合格 | 新しいクラス・関数・状態を追加せず、既存の`Brief()`1関数の内部実装を書き換えるのみ。ログファイル・詳細表示UI等の新機能は追加しない（spec.md Assumptions） |
| **IV. 変更の追跡可能性** | 合格 | 変更箇所に FR-1101〜1105 に対応するコメントを付す。ST-1101 がコメントの存在を機械的に検査する |

**技術的制約との整合**:

- MSBuild のみを使用し、CMake 等は導入しない — 合格（変更なし）
- 新しい `.ps1` をプロジェクトへ追加しない — 合格
- テストランナーを新規導入しない — 合格（ST/DT の手順として定義）

**設計後の再チェック（Phase 1 完了時点）**: Phase 1 の成果物（[data-model.md](./data-model.md)・[contracts/brief-contract.md](./contracts/brief-contract.md)・[quickstart.md](./quickstart.md)）を作成した後も、上記の判定は変わらない。**違反ゼロのため Complexity Tracking セクションは記入しない。**

## Project Structure（プロジェクト構造）

### Documentation（このフィーチャー）

```text
specs/011-fix-error-message-truncation/
├── plan.md              # このファイル
├── research.md          # Phase 0 の出力（R-1101〜R-1103）
├── data-model.md        # Phase 1 の出力（新規永続エンティティなし）
├── quickstart.md        # Phase 1 の出力（ST-1101、DT-1101）
├── contracts/
│   └── brief-contract.md  # Phase 1 の出力（Brief() の振る舞い契約）
├── checklists/
│   └── requirements.md  # spec 品質チェックリスト（全項目合格）
└── tasks.md             # Phase 2 の出力（/speckit-tasks が作成。本コマンドでは作らない）
```

### Source Code（リポジトリのルート）

```text
MFCApplication1/
├── SetupRunner.cpp          # 変更するファイル
│                            #   - Brief() 内に装飾文字除去・段落分割・最後の段落の
│                            #     先頭優先切り詰めを追加
│                            #     (FR-1101〜1105)
├── ProcessRunner.cpp        # 変更しない
├── ProcessRunner.h          # 変更しない
├── MFCApplication1Dlg.*     # 変更しない
├── TargetFolderState.*      # 変更しない
├── GitignoreWriter.*        # 変更しない
└── res/gitignore_all.txt    # 変更しない
```

**Structure Decision**: 既存の平坦な構成をそのまま維持する。`Brief()`は既に5箇所の失敗分岐から共有される唯一の「失敗理由の整形窓口」であり、修正はその内部実装に閉じる。新しいファイル・新しいクラスは作らない（[research.md](./research.md) R-1102）。

## Complexity Tracking（複雑さの追跡）

違反なし。本フィーチャーは既存関数`Brief()`の内部実装を書き換えるのみで、新しい抽象化・公開インターフェース・永続状態を一切導入しない。
