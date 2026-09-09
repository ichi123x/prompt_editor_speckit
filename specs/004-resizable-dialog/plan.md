# Implementation Plan: ダイアログのリサイズ追従

**Branch**: `ver.0.2` | **Date**: 2026-09-09 | **Spec**: [spec.md](./spec.md)

**Input**: `/specs/004-resizable-dialog/spec.md` からの機能仕様

## Summary（概要）

ダイアログは `WS_THICKFRAME` を持つためサイズ変更できるが、追従処理が一切実装されていない。ウィザードが生成した状態のまま 001〜003 のどのフィーチャーも触れてこなかった箇所である。結果として、広げても各コントロールが元の大きさのまま取り残され、右と下に大きな空白ができる。

`WM_SIZE` を受け取り、**コントロールごとに定めた追従方針に従って再配置する**。方針は処理本体の分岐ではなく**テーブルで宣言的に持つ**。あわせて `WM_GETMINMAXINFO` で最小サイズを初期サイズに固定し、縮小時の破綻を防ぐ。

**001〜003 が削除中心だったのに対し、本フィーチャーは初めて機能を追加する。** 主リスクも変わる。これまでの「消し過ぎ」に代わって、**定義漏れ**（コントロールを1つ追従テーブルに載せ忘れる）と**計算方式の誤り**（前回サイズからの相対計算による累積ずれ）が2大リスクになる。設計はこの2点への対処に集中している。

## Technical Context（技術コンテキスト）

**Language/Version**: C++（MSVC v143 ツールセット、Unicode 文字セット）— 001〜003 から変更なし

**Primary Dependencies**: MFC（動的リンク）。**外部依存の追加なし**。MFC 標準の `WM_SIZE` / `WM_GETMINMAXINFO` ハンドラと `SetWindowPos` のみを使う

**Storage**: N/A（永続ストレージを持たない。ウィンドウサイズの記憶も行わない）

**Testing**: 自動テストランナーは導入しない。静的テスト ST-301〜305、動的テスト DT-301〜310 を手順として実施する。手順は [quickstart.md](./quickstart.md) に記載

**Target Platform**: Windows 11 — 変更なし

**Project Type**: desktop-app（MFC ダイアログベースアプリケーション）

**Performance Goals**: N/A。ただしリサイズ中に体感できるちらつきが出ないこと。8コントロールの再配置であり、通常の `SetWindowPos` で十分（[research.md](./research.md) R-301）

**Constraints**:
- ビルドは MSBuild のみ。4構成すべてが警告0で通ること
- 新規の PowerShell スクリプトを追加しない
- 001〜003 で実装した機能の動作を変えない（contracts「7. 既存機能への影響」が列挙）
- ワーカースレッド実行中のリサイズに対し、**追加の排他制御を導入しない**

**Scale/Scope**: 追加 — 列挙1件（6値）、構造1件、メンバ4件、ハンドラ2件（`OnSize` / `OnGetMinMaxInfo`）、再配置処理1件、追従テーブル8行。変更 — ラベルへのID割り当て1件、結果一覧の列幅計算の共通化

## Constitution Check（憲法チェック）

*ゲート: Phase 0 のリサーチ前に通過する必要がある。Phase 1 の設計後に再チェックすること。*

### Phase 0 前の評価

| 原則 | 評価 | 根拠 |
|---|---|---|
| I. Spec Kit ワークフロー厳守 | PASS | `/speckit-specify` → `/speckit-plan` の順で進行中 |
| II. 日本語での一貫性 | PASS | 設計成果物とコメントを日本語とする。本フィーチャーは表示文字列を追加しない |
| III. シンプルさ優先（YAGNI） | **要注意** | 初めての機能追加であり、原則IIIが最も試される。下記「設計上の判断」を参照 |
| IV. 変更の追跡可能性 | PASS | 追加した各箇所に FR 番号（FR-301〜310）をコメントで記載する |
| 技術的制約: MSBuild のみ | PASS | ビルド構成を変更しない |
| 技術的制約: PowerShell スクリプトのみ | PASS | 新規スクリプトを追加しない |
| 技術的制約: テストスイート不在 | PASS | テストランナーを新規導入しない |

**ゲート結果**: 違反なし。Phase 0 に進む。

### Phase 1 設計後の再評価

| 原則 | 評価 | 設計を経ての確認 |
|---|---|---|
| I. Spec Kit ワークフロー厳守 | PASS | research.md → data-model.md → contracts/ → quickstart.md を順に生成した |
| II. 日本語での一貫性 | PASS | 表示文字列の追加は無い。コメントはすべて日本語 |
| III. シンプルさ優先（YAGNI） | PASS | 下記「設計上の判断」を参照 |
| IV. 変更の追跡可能性 | PASS | 001〜003 の成果物との差分として全設計文書を構成し、変更しない部分は「003 のまま」と明記した |
| 技術的制約 | PASS | 依存関係・ビルドシステム・スクリプトのいずれも追加していない |

**設計上の判断（原則III との関係）**:

本フィーチャーで原則IIIが問われたのは3箇所ある。いずれも「必要最小限か、過剰か」を検討した。

**(1) 外部のリサイズ用ライブラリを使わない** — 8コントロールの再配置のために依存を増やすのは過剰。自前の再配置処理で足りる（research.md R-301）。

**(2) 追従方針をテーブルで持つ** — 処理本体に `if` を並べるほうがコード量は少ない。それでもテーブルにしたのは、**ST-303（定義漏れの検出）がテーブルでしか成立しない**ため。分岐が散らばっていると、どのコントロールが未対応かを機械的に調べられない。将来の拡張のためではなく、いま必要な検査のための構造である。

**(3) `Fixed` のコントロールにもIDを与える** — ラベルは追従処理として何もしないため、技術的にはIDが不要。それでも `IDC_STATIC_TARGET_LABEL` を割り当てるのは、**「意図的に固定」と「定義を忘れた」を区別する**ため。除外設計にすると ST-303 が本来の目的（漏れの検出）を果たせなくなる（contracts「2. ラベルのID」）。

いずれも「将来使うかもしれない」ではなく「いま必要な検査を成立させる」ための判断であり、原則IIIが禁じる時期尚早な抽象化にはあたらない。

**設計文書の書き方**: 002・003 と同じく、001〜003 の全文コピーを作らず**差分のみ**を記述して参照する。

**ゲート結果**: 違反なし。Complexity Tracking への記載事項なし。

## Project Structure（プロジェクト構造）

### Documentation（このフィーチャー）

```text
specs/004-resizable-dialog/
├── plan.md              # このファイル
├── spec.md              # 機能仕様（FR-301〜310、ST-301〜305、DT-301〜310）
├── research.md          # Phase 0 の出力（R-301〜305）
├── data-model.md        # Phase 1 の出力（001〜003 からの差分）
├── quickstart.md        # Phase 1 の出力（検証手順、動的テスト中心）
├── contracts/
│   └── ui-contract.md   # Phase 1 の出力（003 からの差分）
├── checklists/
│   └── requirements.md  # 仕様品質チェックリスト（16項目合格）
└── tasks.md             # Phase 2 の出力（/speckit-tasks が作成。未作成）
```

### Source Code（リポジトリのルート）

新規ファイルは作らない。既存ファイルへの追加のみ。

```text
MFCApplication1/
├── Resource.h                     # 変更: IDC_STATIC_TARGET_LABEL (1008) を追加
├── MFCApplication1.rc             # 変更: ラベルの IDC_STATIC を新IDへ差し替え
├── MFCApplication1Dlg.h           # 変更: AnchorMode 列挙、AnchorEntry 構造、
│                                  #       メンバ4件、OnSize / OnGetMinMaxInfo の宣言
├── MFCApplication1Dlg.cpp         # 変更: メッセージマップ2行、追従テーブルの初期化、
│                                  #       再配置処理、列幅の共通化
├── ProcessRunner.h / .cpp         # 変更なし
├── TargetFolderState.h / .cpp     # 変更なし
├── GitignoreWriter.h / .cpp       # 変更なし
└── SetupRunner.h / .cpp           # 変更なし
```

**Structure Decision**: 変更は `MFCApplication1Dlg` とリソース定義に閉じる。SDD化の処理を担う4クラス（`ProcessRunner`・`TargetFolderState`・`GitignoreWriter`・`SetupRunner`）はいずれも無変更である。

001 で「UI の結線はダイアログ、処理は別クラス」と分けておいたことが、ここでも効いている。リサイズは純粋に UI の関心事であり、処理側に一切波及しない。003 で `ProcessRunner` が無変更だったのと同じ構図である。

**新規クラスを作らない判断**: 追従テーブルと再配置処理を専用クラス（例: `DialogAnchor`）へ切り出す設計も考えられる。しかし利用者はこのダイアログ1つだけであり、切り出しても再利用先が無い。憲章の原則III（YAGNI）に従い、ダイアログのメンバとして持つ。ダイアログが2枚以上になった時点で再検討すればよい。

## Complexity Tracking（複雑さの追跡）

Constitution Check に違反はないため、記載事項なし。

本フィーチャーは 001〜003 と異なり複雑さを**増やす**変更である。ただし増加分は追従テーブル8行と再配置処理1件に収まっており、SDD化の処理には一切波及しない。
