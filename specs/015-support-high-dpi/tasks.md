---

description: "機能実装用のタスクリストテンプレート"
---

# Tasks: 高DPI環境でレイアウトが崩れない

**Input**: `/specs/015-support-high-dpi/` からの設計ドキュメント

**Prerequisites（前提条件）**: [plan.md](./plan.md)、[spec.md](./spec.md)、[research.md](./research.md)、[data-model.md](./data-model.md)、[contracts/ui-contract.md](./contracts/ui-contract.md)、[quickstart.md](./quickstart.md)

**Tests（テスト）**: 自動テストスイートは存在しない（憲章の技術的制約）。検証はquickstart.mdのST-xxx/DT-xxxをタスクとして組み込む。

**Organization（構成）**: タスクはユーザーストーリーごとにグループ化する。ただしFR-1505（013由来の列margin枯渇の是正）は両ユーザーストーリーの前提となるため、Foundationalフェーズに置く。

## Format（形式）: `[ID] [P?] [Story] Description`

- **[P]**: 並行実行可能（異なるファイル、依存関係なし）
- **[Story]**: US1、US2
- 説明には正確なファイルパスを含める

## Path Conventions（パスの慣例）

既存の単一MFCプロジェクト構成をそのまま使用する。変更対象は `MFCApplication1/MFCApplication1.rc` と `MFCApplication1/MFCApplication1Dlg.cpp` の2ファイルのみ（[plan.md](./plan.md) Project Structure参照）。

---

## Phase 1: Setup（共有インフラ）

**目的**: 測定の前提となるビルド環境を整える

- [X] T001 `MFCApplication1.exe` を終了させたうえで `MFCApplication1.sln` をDebug x64でリビルドし、以後のすべての測定がこのビルドを基準にすることを確認する（[research.md](./research.md)に記載のとおり、古いexeを掴んだ測定は無意味になるため必須）

---

## Phase 2: Foundational（ブロッキングとなる前提条件）

**目的**: FR-1505（013由来の「項目」列の安全余裕枯渇）の是正。US1・US2の両方が、この是正済みレイアウトを前提とする

**⚠️ 重要**: このフェーズが完了するまで、どのユーザーストーリーの検証も開始できません

- [X] T002 `MFCApplication1/MFCApplication1.rc` の `IDD_MFCAPPLICATION1_DIALOG` の幅を545→570 DLUに変更する。ダイアログ定義の直前（または同じ行）に `// FR-1505: 013由来の列margin枯渇の是正` のコメントを付ける（[data-model.md](./data-model.md) 「ダイアログ」表。当初は583 DLUで実装したが、実機実測でFR-506の1024px制約超過が判明し570 DLUへ修正した — [research.md](./research.md) R-1503）
- [X] T003 `MFCApplication1/MFCApplication1.rc` の6コントロール（`IDC_EDIT_TARGET_PATH`・`IDC_BUTTON_BROWSE`・`IDC_LIST_RESULT`・`IDC_STATIC_GUIDE`・`IDOK`・`IDCANCEL`）の座標を [data-model.md](./data-model.md) 「コントロール」表のとおりに変更する（T002に依存、同一ファイル）
- [X] T004 `MFCApplication1/MFCApplication1Dlg.cpp` の `kColumnRatio` を `{ 20, 17, 63 }` から `{ 22, 17, 61 }` に変更し、定義箇所のコメントに `FR-1503`・`FR-1505` を追記する（[research.md](./research.md) R-1503 Decision）
- [X] T005 `MFCApplication1.sln` をDebug x64でリビルドし、警告・エラーが0件であることを確認する（T002〜T004完了後）

**チェックポイント**: レイアウトの是正が完了 - これでUS1・US2の検証を開始できます

---

## Phase 3: User Story 1 - 固定スケールで起動しても崩れない (Priority: P1) 🎯 MVP

**目標**: 表示スケール100%〜150%相当（96/115/120/144 DPI）で、起動直後のウィンドウ操作なしに結果一覧の3列・完了案内・初期案内が全文読め、コントロールが重ならないことを実測で確認する

**独立したテスト**: 各DPI相当のフォント計量とビルド済みexeからの実測（`LVM_GETCOLUMNWIDTH`・`GetClientRect`）を突き合わせ、[research.md](./research.md) R-1503の検算表と一致することを確認する

### User Story 1 の検証

- [X] T006 [US1] ST-1501: `MFCApplication1.rc` の寸法・`kColumnRatio`が [data-model.md](./data-model.md) の値と一致し、FR番号コメントが付いていることを確認する（[quickstart.md](./quickstart.md) ST-1501）
- [X] T007 [US1] ST-1502: `msbuild ... -p:RunCodeAnalysis=true -p:EnablePREfast=true` を実行し、指摘0件を確認する
- [X] T008 [US1] ST-1503: Debug/Release × x64/x86の4構成をリビルドし、警告・エラー0件を確認する
- [X] T009 [US1] DT-1501〜1503: 96/115/120/144 DPI相当それぞれについて、DPI相当フォントでの文字幅計測と実行中アプリからの列幅実測を行い、余裕がすべて正の値（[research.md](./research.md) R-1503の表以上）であることを確認する（[quickstart.md](./quickstart.md) DT-1501〜1503の手順）
- [X] T010 [US1] DT-1504: 開発機の実DPI（115）でウィンドウ全体の幅を`GetWindowRect`で実測し、1024px以下（想定1016px、余裕8px）であることを確認する
- [X] T011 [US1] DT-1505: 192 DPI（200%）相当で列余裕が負になり得ることを確認しつつ、アプリがクラッシュしないことを目視確認し、既知の制限として結果を記録する
- [X] T012 [US1] DT-1507: 001〜014の既存動作確認手順（5〜6手順の実行、リサイズ追従8方向、タイトルバー色、完了案内・初期案内の文言、`/init`自動化）を再実施し、SC-1504（開発機環境ですべて再現）を確認する

**チェックポイント**: この時点で、User Story 1 は独立して完全に検証済みであるはずです

---

## Phase 4: User Story 2 - モニタ間を移動しても表示が破綻しない (Priority: P2)

**目標**: 異なるDPIのモニタ間をウィンドウが移動した場合、レイアウトの比率は崩れず、ビットマップ引き伸ばしによるぼやけのみが既知の制限として発生することを文書化・（可能なら）確認する

**独立したテスト**: 複数モニタ環境がある場合、実機でウィンドウをドラッグ移動し目視確認する。無い場合は既知の制限としての文書化をもって完了とする

### User Story 2 の検証

- [X] T013 [US2] [spec.md](./spec.md) FR-1504・[contracts/ui-contract.md](./contracts/ui-contract.md) 5節の記載が、per-monitor DPI awareness化を行わないという決定と矛盾なく最終化されていることを確認する（コード変更なし）
- [X] T014 [US2] 複数モニタでDPI設定が異なる環境がある場合のみ、[quickstart.md](./quickstart.md) DT-1506を実施し、レイアウト比率が保たれることを目視確認する。単一モニタ環境の場合は「実機未確認・既知の制限」として結果を記録するに留める — **開発機は単一モニタ構成のため未実施。既知の制限として[research.md](./research.md) R-1505に記録済み**

**チェックポイント**: これでUser Story 1・2の両方が独立して検証済みであるはずです

---

## Phase 5: Polish & Cross-Cutting Concerns（仕上げと横断的関心事）

**目的**: ドキュメントの反映

- [X] T015 [P] `CLAUDE.md` の既存機能一覧表に `specs/015-support-high-dpi/` の行を追加し、013由来の列margin回帰とその是正（570 DLU・比率22/17/61）、実装時に発覚したウィンドウ幅見積もりの見落とし（`GetClientRect`と`GetWindowRect`の混同、非クライアント領域約18px）、100%〜150%保証・200%は既知の制限、マルチモニタ非対応の決定を追記する

---

## Dependencies & Execution Order（依存関係と実行順序）

### Phase Dependencies（フェーズの依存関係）

- **Setup (Phase 1)**: 依存関係なし
- **Foundational (Phase 2)**: Setup の完了に依存 - US1・US2の両方をブロックする（FR-1505が両ストーリーの前提のため）
- **User Story 1 (Phase 3)**: Foundational の完了に依存。US2への依存なし
- **User Story 2 (Phase 4)**: Foundational の完了に依存。US1への依存なし（並行実施可能）
- **Polish (Phase 5)**: US1・US2の完了に依存

### Within Each Phase（各フェーズ内）

- T002→T003は同一ファイル（`MFCApplication1.rc`）のため逐次実行
- T004は別ファイル（`MFCApplication1Dlg.cpp`）だが、T002・T003と合わせてFR-1505の是正一式としてT005のリビルド確認前に完了させる
- T009〜T012は同一の実行中アプリに対する実測のため、T006〜T008（静的確認）の後に行う

### Parallel Opportunities（並行実行の機会）

- Foundational完了後、User Story 1（Phase 3）とUser Story 2（Phase 4）は並行して進められる（US2はコード変更を伴わないため、US1の検証と競合しない）
- T015（CLAUDE.md更新）はUS1・US2のいずれの検証タスクとも独立して進められる

---

## Implementation Strategy（実装戦略）

### MVP First（User Story 1 のみ）

1. Phase 1: Setup を完了する
2. Phase 2: Foundational（FR-1505の是正）を完了する — これが実質的な唯一のコード変更
3. Phase 3: User Story 1 の検証を完了する
4. **停止して検証**: SC-1501・SC-1502・SC-1504が満たされることを確認する

### Incremental Delivery（段階的な提供）

1. Setup + Foundational を完了する → レイアウトの是正が完了
2. User Story 1 を検証する → 固定スケールでの可読性が保証される（MVP）
3. User Story 2 を検証する → マルチモニタでの既知の制限が文書化される
4. Polish（CLAUDE.md更新）で完了

---

## Notes（注記）

- 本フィーチャーはコード変更が非常に小さい（`.rc`の座標6箇所＋`kColumnRatio`定数1つ）。タスクの大半は実測による検証であり、これは憲章 原則V（検証は実測に基づく）に従うため
- US2はコード変更を伴わない（FR-1504により per-monitor 対応をしないと決定済み）。検証は文書化と、環境があれば目視確認のみ
- 各タスク完了後、`git add`は変更した実ファイルのみを明示指定すること（`git add -A`を使わない。CLAUDE.mdに記載の既存の注意点を継続する）
