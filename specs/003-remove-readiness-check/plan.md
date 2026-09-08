# Implementation Plan: 準備状況の診断機能の廃止

**Branch**: `ver.0.2` | **Date**: 2026-09-08 | **Spec**: [spec.md](./spec.md)

**Input**: `/specs/003-remove-readiness-check/spec.md` からの機能仕様

## Summary（概要）

001 で実装した「準備状況を診断」ボタンと、準備完了チェックリスト7項目の判定機能を廃止する。判定結果を入力とする手順案内（001 FR-022）も、判定材料を失うため同時に廃止する。ツールの役割を「配置済みプロジェクトをSDD化する」一点に絞る。

技術的アプローチは削除のみである。`ReadinessChecker.h` / `.cpp` をファイルごと消し（約430行）、ダイアログからボタン・ハンドラ・案内生成を取り除き、`vcxproj` の登録を解除する。加えて、削除によって孤立する `TargetFolderState::hasGitignore` を取り除く。

**新しい技術要素は無く、外部依存も増減しない。本フィーチャーの主リスクは削除の巻き添えである。** 設計上の注意は、消してよいものと消してはならないものの境界を明確にすることに集中している。特に `ShowCompletionGuide()`（残す）と `ShowReadinessGuide()`（削除）の取り違えと、無効化リストから前後の要素を巻き添えにする誤りの2つを名指しで警戒している。

## Technical Context（技術コンテキスト）

**Language/Version**: C++（MSVC v143 ツールセット、Unicode 文字セット）— 001・002 から変更なし

**Primary Dependencies**: MFC（動的リンク）。**外部依存の追加・削除ともに無し**。実行時に起動する外部コマンドも変更なし

**Storage**: N/A（永続ストレージを持たない）

**Testing**: 自動テストランナーは導入しない。静的テスト ST-201〜205、動的テスト DT-201〜208 を手順として実施する。手順は [quickstart.md](./quickstart.md) に記載。**動的テスト8件のうち5件（DT-203〜205、DT-207、DT-208）を回帰確認に充てる**

**Target Platform**: Windows 11 — 変更なし

**Project Type**: desktop-app（MFC ダイアログベースアプリケーション）

**Performance Goals**: N/A。ただし `TargetFolder::Read()` から `.gitignore` の存在確認が1つ減るため、ディスクアクセスがわずかに減る

**Constraints**:
- ビルドは MSBuild のみ。4構成すべてが通り、**未使用変数・未使用関数の警告が出ないこと**（削除の消し残しを検出する）
- 001・002 で実装した機能の動作を変えない（FR-207 が列挙）
- 完了案内を失わない（FR-205）
- 実行中の無効化対象から生成・参照ボタンを外さない（FR-209）

**Scale/Scope**: 削除 — ソースファイル2件（約430行）、コントロール1件、関数2件、構造・列挙2件、フィールド1件。変更 — `vcxproj` / `.filters` の登録解除、`.rc` のコントロール定義、ダイアログのメッセージマップと無効化リスト

## Constitution Check（憲法チェック）

*ゲート: Phase 0 のリサーチ前に通過する必要がある。Phase 1 の設計後に再チェックすること。*

### Phase 0 前の評価

| 原則 | 評価 | 根拠 |
|---|---|---|
| I. Spec Kit ワークフロー厳守 | PASS | `/speckit-specify` → `/speckit-plan` の順で進行中 |
| II. 日本語での一貫性 | PASS | 設計成果物・UI文字列・コメントをすべて日本語とする。削除する文字列も日本語であり、残るものとの一貫性は保たれる |
| III. シンプルさ優先（YAGNI） | **強く適合** | 機能の削除そのものであり、原則IIIが最も直接的に効く。「判定不能」を含む7項目の判定という、実効性の乏しい仕組みを取り除く |
| IV. 変更の追跡可能性 | PASS | 削除する各箇所に対応する FR 番号（FR-201〜209）をコメントで残す。spec 冒頭に 001 のどの要件を廃止するかの表を掲載済み |
| 技術的制約: MSBuild のみ | PASS | ビルド構成を変更しない |
| 技術的制約: PowerShell スクリプトのみ | PASS | 新規スクリプトを追加しない |
| 技術的制約: テストスイート不在 | PASS | テストランナーを新規導入しない |

**ゲート結果**: 違反なし。Phase 0 に進む。

### Phase 1 設計後の再評価

| 原則 | 評価 | 設計を経ての確認 |
|---|---|---|
| I. Spec Kit ワークフロー厳守 | PASS | research.md → data-model.md → contracts/ → quickstart.md を順に生成した |
| II. 日本語での一貫性 | PASS | [ui-contract.md](./contracts/ui-contract.md)「4. メッセージ文字列の契約」で、削除する文字列と**残す文字列**を明示した |
| III. シンプルさ優先（YAGNI） | PASS | 下記「設計上の簡素化」を参照 |
| IV. 変更の追跡可能性 | PASS | 001・002 の成果物との差分として全設計文書を構成し、変更しない部分は「002 のまま」と明記した |
| 技術的制約 | PASS | 依存関係・ビルドシステム・スクリプトのいずれも追加していない |

**設計上の簡素化（原則III への適合）**:
- `ReadinessChecker` を**ファイルごと削除**する。列挙や構造だけを残すと、後から読んだときに「どこで使われるのか」を探す無駄が生じる
- 孤立する `hasGitignore` も削除する。将来使うかもしれないという理由で残すのは、原則IIIが禁じる「現時点で必要とされていない拡張ポイント」にあたる
- レイアウトを動かさない。診断ボタンは「生成」と同じ行にあったため縦の空きは生じず、「生成」は既に左端に揃っている。002 でボタンを左詰めにしておいたことがここで効いている

**設計文書の書き方についての判断**: 002 と同じく、001・002 の全文コピーを作らず**差分のみ**を記述して参照する形にした。全文を複製すると元を修正したときに食い違い、どちらが正なのか分からなくなる。

**ゲート結果**: 違反なし。Complexity Tracking への記載事項なし。

## Project Structure（プロジェクト構造）

### Documentation（このフィーチャー）

```text
specs/003-remove-readiness-check/
├── plan.md              # このファイル
├── spec.md              # 機能仕様（FR-201〜209、ST-201〜205、DT-201〜208）
├── research.md          # Phase 0 の出力（R-201〜204）
├── data-model.md        # Phase 1 の出力（001・002 からの差分）
├── quickstart.md        # Phase 1 の出力（検証手順、回帰確認を重視）
├── contracts/
│   └── ui-contract.md   # Phase 1 の出力（002 からの差分）
├── checklists/
│   └── requirements.md  # 仕様品質チェックリスト（16項目合格）
└── tasks.md             # Phase 2 の出力（/speckit-tasks が作成。未作成）
```

### Source Code（リポジトリのルート）

新規ファイルは作らない。ファイルの削除と既存ファイルの変更のみ。

```text
MFCApplication1/
├── ReadinessChecker.h             # 削除
├── ReadinessChecker.cpp           # 削除（約400行）
├── MFCApplication1.vcxproj        # 変更: 上記2件の登録を解除
├── MFCApplication1.vcxproj.filters # 変更: 同上
├── Resource.h                     # 変更: IDC_BUTTON_DIAGNOSE の定義を削除
├── MFCApplication1.rc             # 変更: 診断ボタンのコントロール定義を削除
├── MFCApplication1Dlg.h           # 変更: include、ハンドラ宣言、ShowReadinessGuide 宣言を削除
├── MFCApplication1Dlg.cpp         # 変更: メッセージマップ1行、無効化リストの1要素、
│                                  #       ハンドラ実装、ShowReadinessGuide 実装を削除
├── TargetFolderState.h / .cpp     # 変更: hasGitignore フィールドと代入処理を削除
├── ProcessRunner.h / .cpp         # 変更なし
├── GitignoreWriter.h / .cpp       # 変更なし
└── SetupRunner.h / .cpp           # 変更なし
```

**Structure Decision**: 001 で確立したファイル構成を維持する。クラスの追加も分割も行わない。

責務ごとに分けておいたことで、削除の影響範囲が明確になっている。`ProcessRunner`・`GitignoreWriter`・`SetupRunner` はいずれも診断に依存していないため無変更である。診断だけが `ReadinessChecker` に閉じており、ファイルごと消せる。

**削除作業の順序についての判断**: 依存の末端（`ReadinessChecker` の実体）から消すと、ダイアログ側が参照したままビルドが通らない期間が長くなる。**ダイアログ側（利用者）から消し始め、最後に `ReadinessChecker` 本体と `vcxproj` の登録を外す**順序とする。詳細は `/speckit-tasks` に委ねる。

## Complexity Tracking（複雑さの追跡）

Constitution Check に違反はないため、記載事項なし。

本フィーチャーは複雑さを**減らす**方向の変更である。ソースファイル2件（約430行）、列挙1件、構造1件、関数2件、コントロール1件、フィールド1件が消える。`TargetFolderState` の参照元が2つから1つへ減り、結果一覧の共用も解消される。
