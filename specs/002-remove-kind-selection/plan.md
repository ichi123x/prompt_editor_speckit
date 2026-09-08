# Implementation Plan: プロジェクト種別選択の廃止

**Branch**: `ver.0.2` | **Date**: 2026-09-08 | **Spec**: [spec.md](./spec.md)

**Input**: `/specs/002-remove-kind-selection/spec.md` からの機能仕様

## Summary（概要）

001 で設けた「MFCアプリ / Webアプリ」のラジオボタンを廃止し、種別という概念を実装から完全に取り除く。種別の判定は本ツールの役割ではなく、`/speckit-constitution` が憲章を制定する段階で行われる。

技術的アプローチは削除が中心である。`ProjectKind` 列挙とその参照箇所（実測で6ファイル・約20箇所）を消し、関数シグネチャから `kind` 引数を落とし、ダイアログからコントロール3件を削除する。`.gitignore` の埋め込みリソース2件は、重複16パターンを排除した単一ファイルへ統合する。統合により「書く時点で種別を知る必要」自体が消え、生成される内容が入力に依存しなくなる。

**本フィーチャーの主リスクは削除の巻き添えである。** 新しい技術要素は無く、外部依存も増えない。設計上の注意は、消してよいものと消してはならないものの境界を明確にすることに集中している。

## Technical Context（技術コンテキスト）

**Language/Version**: C++（MSVC v143 ツールセット、Unicode 文字セット）— 001 から変更なし

**Primary Dependencies**: MFC（動的リンク）。**外部依存の追加・削除ともに無し**。実行時に起動する外部コマンド（`uv` / `specify` / `code` / `git` / `winget`）も 001 から変更なし

**Storage**: N/A（永続ストレージを持たない）

**Testing**: 自動テストランナーは導入しない。静的テスト ST-101〜106、動的テスト DT-101〜108 を手順として実施する。手順は [quickstart.md](./quickstart.md) に記載。**動的テスト8件のうち4件（DT-105〜108）を回帰確認に充てる**

**Target Platform**: Windows 11 — 001 から変更なし

**Project Type**: desktop-app（MFC ダイアログベースアプリケーション）

**Performance Goals**: N/A

**Constraints**:
- ビルドは MSBuild のみ。`Debug`/`Release` × `x64`/`x86` の4構成すべてが通り、**未使用変数・未使用関数の警告が出ないこと**（削除の消し残しを検出する）
- 新規の PowerShell スクリプト（`.ps1`）を追加しない
- 001 で実装した機能の動作を変えない（FR-108 が列挙）
- 生成される `.gitignore` の内容が対象フォルダに依存しないこと（FR-106）

**Scale/Scope**: 削除 — コントロール3件、列挙1件、メソッド2件、メンバ2件、メッセージ2件、埋め込みリソース1件。変更 — 関数シグネチャ3件、ダイアログのレイアウト3コントロール、統合リソース1件

## Constitution Check（憲法チェック）

*ゲート: Phase 0 のリサーチ前に通過する必要がある。Phase 1 の設計後に再チェックすること。*

### Phase 0 前の評価

| 原則 | 評価 | 根拠 |
|---|---|---|
| I. Spec Kit ワークフロー厳守 | PASS | `/speckit-specify` → `/speckit-plan` の順で進行中。001 も同じ順序を踏んでいる |
| II. 日本語での一貫性 | PASS | 設計成果物・UI文字列・コメントをすべて日本語とする。削除するメッセージも日本語だったため、残るものとの一貫性は保たれる |
| III. シンプルさ優先（YAGNI） | **強く適合** | 本フィーチャーは機能の削除であり、原則IIIが最も直接的に効く。特に「`hasSlnMarker` / `hasWebMarker` を将来のために両方残す」案を、YAGNI を根拠に却下している（research.md R-102） |
| IV. 変更の追跡可能性 | PASS | 削除する各箇所に対応する FR 番号（FR-101〜109）をコメントで残す。spec 冒頭に 001 のどの要件を置き換えるかの表を掲載済み |
| 技術的制約: MSBuild のみ | PASS | ビルド構成を変更しない |
| 技術的制約: PowerShell スクリプトのみ | PASS | 新規スクリプトを追加しない |
| 技術的制約: テストスイート不在 | PASS | テストランナーを新規導入しない |

**ゲート結果**: 違反なし。Phase 0 に進む。

### Phase 1 設計後の再評価

| 原則 | 評価 | 設計を経ての確認 |
|---|---|---|
| I. Spec Kit ワークフロー厳守 | PASS | research.md → data-model.md → contracts/ → quickstart.md を順に生成した |
| II. 日本語での一貫性 | PASS | [ui-contract.md](./contracts/ui-contract.md)「4. メッセージ文字列の契約」で、削除する2件と残る7件を明示した |
| III. シンプルさ優先（YAGNI） | PASS | 下記「設計上の簡素化」を参照 |
| IV. 変更の追跡可能性 | PASS | 001 の成果物との差分として全設計文書を構成し、変更しない部分は「001 のまま」と明記した。何が変わって何が変わらないかが文書上で確定している |
| 技術的制約 | PASS | 依存関係・ビルドシステム・スクリプトのいずれも追加していない |

**設計上の簡素化（原則III への適合）**:
- `ProjectKind` を内部的にも残さず**消し切る**。片方を固定で渡す実装にすると「なぜ選択肢があるのに固定なのか」という疑問を後から読む人に残す
- `hasSlnMarker` / `hasWebMarker` を `hasProjectMarker` 1つへ統合する。将来どちらかを使う可能性を見越して両方残すのは、原則IIIが禁じる「現時点で必要とされていない拡張ポイント」にあたる
- `.gitignore` を統合し、フォルダの内容による分岐そのものを無くす。分岐が無ければ分岐のテストも要らない

**設計文書の書き方についての判断**: 本フィーチャーの設計文書は 001 の完全なコピーを作らず、**差分のみ**を記述して 001 を参照する形にした。全文を複製すると、001 側を修正したときに 002 側と食い違い、どちらが正なのか分からなくなる。原則IV（変更の追跡可能性）の観点でも、差分表現のほうが「何を変えたか」が明確になる。

**ゲート結果**: 違反なし。Complexity Tracking への記載事項なし。

## Project Structure（プロジェクト構造）

### Documentation（このフィーチャー）

```text
specs/002-remove-kind-selection/
├── plan.md              # このファイル
├── spec.md              # 機能仕様（FR-101〜109、ST-101〜106、DT-101〜108）
├── research.md          # Phase 0 の出力（R-101〜104）
├── data-model.md        # Phase 1 の出力（001 からの差分）
├── quickstart.md        # Phase 1 の出力（検証手順、回帰確認を重視）
├── contracts/
│   └── ui-contract.md   # Phase 1 の出力（001 からの差分）
├── checklists/
│   └── requirements.md  # 仕様品質チェックリスト（16項目合格）
└── tasks.md             # Phase 2 の出力（/speckit-tasks が作成。未作成）
```

### Source Code（リポジトリのルート）

新規ファイルは作らない。既存ファイルの変更と、リソースファイルの統合のみ。

```text
MFCApplication1/
├── MFCApplication1.rc             # 変更: グループボックスとラジオ2件を削除、
│                                  #       ボタンを左詰めへ移動、一覧を拡大、RCDATA を1件へ
├── Resource.h                     # 変更: ラジオ2件の ID を削除、
│                                  #       IDR_GITIGNORE_VS/NODE を IDR_GITIGNORE へ
├── MFCApplication1Dlg.h / .cpp    # 変更: m_kind / m_workerKind、ラジオのハンドラ、
│                                  #       種別未選択と不一致の検証を削除
├── TargetFolderState.h / .cpp     # 変更: ProjectKind 列挙、HasMarkerFor、KindDisplayName を削除。
│                                  #       hasSlnMarker/hasWebMarker を hasProjectMarker へ統合
├── GitignoreWriter.h / .cpp       # 変更: Ensure から kind 引数を削除、リソース分岐を削除
├── SetupRunner.h / .cpp           # 変更: RunAll / RunInitGit から kind 引数を削除
├── ReadinessChecker.cpp           # 変更: hasSlnMarker || hasWebMarker → hasProjectMarker
├── ProcessRunner.h / .cpp         # 変更なし
└── res/
    ├── gitignore_all.txt          # 新規: 重複を排除した統合版
    ├── gitignore_vs.txt           # 削除
    └── gitignore_node.txt         # 削除
```

**Structure Decision**: 001 で確立したファイル構成（責務ごとの具象クラスを平坦に配置）を維持する。本フィーチャーはクラスの追加も分割も行わない。

`ProcessRunner` だけが無変更である点は、責務分割が妥当だったことの傍証といえる。外部コマンドの実行は種別に一切依存しないため、種別の廃止によって影響を受けない。逆に `GitignoreWriter`・`SetupRunner`・`TargetFolderState`・`ReadinessChecker`・ダイアログの5つはいずれも `kind` を受け取っていたため、すべて変更対象になる。

**削除作業の順序についての判断**: 依存の末端から消すと中間状態でビルドが通らない期間が長くなる。ダイアログ側（`kind` の供給元）から消し始め、下位のクラスへ向かって引数を落としていく順序とする。詳細は `/speckit-tasks` に委ねる。

## Complexity Tracking（複雑さの追跡）

Constitution Check に違反はないため、記載事項なし。

本フィーチャーは複雑さを**減らす**方向の変更である。列挙1件、メソッド2件、分岐2箇所、埋め込みリソース1件が消え、関数シグネチャから引数が1つずつ減る。
