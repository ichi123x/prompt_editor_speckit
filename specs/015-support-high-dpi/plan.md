# Implementation Plan: 高DPI環境でレイアウトが崩れない

**Branch**: `015-support-high-dpi` | **Date**: 2026-09-14 | **Spec**: [spec.md](./spec.md)

**Input**: `/specs/015-support-high-dpi/spec.md` からの機能仕様

**注記**: このテンプレートは `/speckit-plan` コマンドによって記入されます。その定義には実行ワークフローが記載されています。

## Summary（概要）

100%〜150%の表示スケール（96〜144 DPI相当）で、起動直後のウィンドウ操作なしに結果一覧の3列・完了案内・初期案内が全文読めることを保証する（FR-1501・FR-1502）。200%は保証対象外の既知の制限とする。併せて、013で追加された手順名により発生していた「項目」列の安全余裕の枯渇（開発機実DPI 115で-21px）を、本フィーチャーの前提条件として是正する（FR-1505）。

技術的アプローチは、004・006で確立済みのDLU/比率ベースの座標系をそのまま用いる。`.rc`のダイアログ幅を545→570 DLUへ拡張し、右方向のコントロール座標を一律+25 DLUシフトし、`kColumnRatio`を`{20,17,63}`から`{22,17,61}`へ変更する（[research.md](./research.md) R-1503、[data-model.md](./data-model.md)）。C++コードの変更は`kColumnRatio`の定数値のみで、004の`AnchorMode`/`InitAnchors`/`ApplyAnchors`が実行時に新しい初期矩形を読み取るため追従処理自体の変更は不要。per-monitor DPI awareness化・`WM_DPICHANGED`対応は行わない（FR-1504。YAGNI、憲章 原則III）。

**実装時の追記**: 計画段階では583 DLUを想定していたが、実機ビルドで`GetWindowRect`を実測したところウィンドウ幅が1038pxとなりFR-506（1024px制約、開発機実DPI 115判定）を超過することが判明した。紙上計算がダイアログのクライアント幅（非クライアント領域＝タイトルバー・枠 約18pxを含まない）を誤ってウィンドウ全体の幅として扱っていたことが原因。570 DLUに修正し、実機再ビルド・実測で1024px制約（余裕8px）と全DPIでの列margin（最小+6px）の両方を満たすことを確認した上で確定した（[research.md](./research.md) R-1503）。

## Technical Context（技術コンテキスト）

**Language/Version**: C++17（MFC、既存コードベースを継承）

**Primary Dependencies**: MFC（Visual Studio 2022、既存）。新規の外部ライブラリ・パッケージの追加なし

**Storage**: N/A

**Testing**: 自動テストスイート無し（憲章の技術的制約）。`specs/015-support-high-dpi/quickstart.md`の静的テストST-1501〜1503・動的テストDT-1501〜1507で検証

**Target Platform**: Windows 11（開発機実DPI 115）。保証範囲は表示スケール100%〜150%（96/115/120/144 DPI相当）

**Project Type**: desktop-app（既存のMFCダイアログアプリ、単一プロジェクト構成）

**Performance Goals**: N/A（UIレイアウトのみの変更。実行時の追加処理なし）

**Constraints**:
- 006 FR-506由来の制約: 初期ウィンドウサイズは開発機の実DPI（115）判定で1024x768の作業領域に収まること（継承。FR-1506）
- 006 R-505由来の制約: 列内部余白の見積もり（20px）に対し、検査と実装が同じ見積もりを共有しないよう独立した安全余裕（8px）を確保すること。加えて憲章 原則Vにより、この安全余裕自体が実測でちょうど0pxになる解を採用してはならない

**Scale/Scope**: 変更対象は`.rc`の1ダイアログ（8コントロールのうち6個の座標）と`MFCApplication1Dlg.cpp`の定数1つ（`kColumnRatio`）のみ。既存の001〜014の機能・文言には一切手を入れない（FR-1506）

## Constitution Check（憲法チェック）

*ゲート: Phase 0 のリサーチ前に通過する必要がある。Phase 1 の設計後に再チェックすること。*

| 原則 | 評価 |
|---|---|
| **I. Spec Kit ワークフロー厳守** | 適合。`/speckit-specify`→`/speckit-clarify`→`/speckit-plan`の順で実施済み。この後`/speckit-tasks`に進む |
| **II. 日本語での一貫性** | 適合。UI文字列・コメントの変更は無し（座標値と定数のみの変更）。既存の日本語文言はFR-1506・FR-1507により変更しない |
| **III. シンプルさ優先（YAGNI）** | 適合。per-monitor DPI awareness化・動的列幅計算・`WM_DPICHANGED`対応など、要求されていない拡張は行わない（FR-1504。[research.md](./research.md) R-1503のAlternatives consideredで明示的に却下） |
| **IV. 変更の追跡可能性** | 適合。`.rc`のコメントと`kColumnRatio`定義箇所にFR-1503・FR-1505番号を付与する（tasks.mdで具体化） |
| **V. 検証は実測に基づく** | 適合。計画段階の紙上計算では583 DLU案を採用する予定だったが、`/speckit-implement`での実機ビルド・`GetWindowRect`実測により、その計算がウィンドウの非クライアント領域（約18px）を見落としており1024px制約に違反することを発見し、570 DLUへ修正した。この修正自体も実機ビルド・実測（列margin最小+6px、ウィンドウ幅余裕8px）で裏付けている。ビルド済みexeの陳腐化にも本フィーチャーの調査中に気づき、リビルド確認をquickstart.mdに明文化した |

**Phase 1設計後の再評価**: 上表の判定に変更なし。`contracts/ui-contract.md`・`data-model.md`・`quickstart.md`の作成により具体化されたのみで、新たな憲法上の懸念は生じていない。

## Project Structure（プロジェクト構造）

### Documentation（このフィーチャー）

```text
specs/015-support-high-dpi/
├── plan.md              # このファイル（/speckit-plan コマンドの出力）
├── research.md          # Phase 0 の出力（/speckit-plan コマンド）
├── data-model.md        # Phase 1 の出力（/speckit-plan コマンド）
├── quickstart.md        # Phase 1 の出力（/speckit-plan コマンド）
├── contracts/
│   └── ui-contract.md   # Phase 1 の出力（/speckit-plan コマンド）
├── checklists/
│   └── requirements.md  # /speckit-specify の出力（16/16合格）
└── tasks.md             # Phase 2 の出力（/speckit-tasks コマンド - /speckit-plan では作成されない）
```

### Source Code（リポジトリのルート）

```text
MFCApplication1/
├── MFCApplication1.rc         # 変更対象: ダイアログ幅545→570 DLU、6コントロールの座標
├── MFCApplication1Dlg.h       # 変更なし
└── MFCApplication1Dlg.cpp     # 変更対象: kColumnRatio定数値のみ（{20,17,63} → {22,17,61}）
```

**Structure Decision**: 既存の単一MFCプロジェクト構成（`MFCApplication1/`）をそのまま使用する。本フィーチャーは新規ファイル・新規クラスを追加しない（004の座標系・アンカー機構をそのまま再利用するため）。上記2ファイルの差分変更のみで完結する。

## Complexity Tracking（複雑さの追跡）

該当なし。Constitution Checkで違反は検出されなかった。
