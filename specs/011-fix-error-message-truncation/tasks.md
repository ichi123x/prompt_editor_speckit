---

description: "失敗理由が装飾バナーに埋もれて見えなくなる問題を解消する（011）の実装タスク"
---

# Tasks: 失敗理由が装飾バナーに埋もれて見えなくなる問題を解消する

**Input**: `/specs/011-fix-error-message-truncation/` からの設計ドキュメント

**Prerequisites（前提条件）**: [plan.md](./plan.md)、[spec.md](./spec.md)、[research.md](./research.md)、[data-model.md](./data-model.md)、[contracts/brief-contract.md](./contracts/brief-contract.md)、[quickstart.md](./quickstart.md)

**Tests（テスト）**: 本リポジトリに自動テストスイートは存在しない（憲章の技術的制約）。代わりに静的テスト ST-1101、動的テスト DT-1101 を [quickstart.md](./quickstart.md) の手順を実施するタスクとして定義する。DT-1101はホワイトボックス方式（`SetupRunner.cpp`を`#include`して`Brief()`を直接呼ぶ）で自動化し、実際の外部コマンド実行は不要。

**Organization（構成）**: spec.mdにはUser Story 1のみが定義されている。

## Format（形式）: `[ID] [P?] [Story] Description`

- **[P]**: 並行実行可能（異なるファイル、依存関係なし）
- **[Story]**: US1 = 失敗理由から実際のエラー内容を読み取れる（P1）

---

## Phase 1: Setup（共有インフラ）

**目的**: 実装前の状態を記録する

- [X] T001 アプリを終了させ（`Get-Process MFCApplication1 -ErrorAction SilentlyContinue | Stop-Process -Force`）、実装前の `MFCApplication1/SetupRunner.cpp` の `Brief()` 関数の内容（25〜37行目付近、装飾除去や `Right()` 相当の処理が無いこと）を控えておく（[quickstart.md](./quickstart.md) ST-1101 のベースライン）— **結果: 記録済み**（段落分割等の処理は無かった）

---

## Phase 2: Foundational（ブロッキングとなる前提条件）

**目的**: User Story 1 が依存する共有の基盤コードを用意する

本フィーチャーには該当タスクがない。変更は `SetupRunner.cpp` の `Brief()` 関数1つに閉じており、共有の新しい型・関数を必要としない（[plan.md](./plan.md) Project Structure）。Setup の後、直接 User Story 1 の実装へ進める。

---

## Phase 3: User Story 1 - 失敗理由から実際のエラー内容を読み取れる (Priority: P1) 🎯 MVP

**目標**: `Brief()` を、装飾文字を除去したうえで段落単位に分割し最後の段落を先頭優先で切り詰めるように変更し、`specify init` 等が出す装飾バナーに実際のエラー内容が埋もれないようにする

**独立したテスト**: 装飾的なバナーの後に具体的なエラーメッセージを出力する合成文字列を `Brief()` に直接渡し、戻り値にそのエラーメッセージが含まれることを確認する

### User Story 1 の実装

- [X] T002 [US1] `MFCApplication1/SetupRunner.cpp` の `Brief()`（[SetupRunner.cpp:25](../../MFCApplication1/SetupRunner.cpp#L25)）を変更する: (a) `output`を`\r\n`・`\n`で行に分割する。(b) 各行からUnicode Box Drawing（U+2500〜U+257F）・Block Elements（U+2580〜U+259F）の範囲に該当する文字を取り除いてTrimし、可視文字が残れば「内容行」、残らなければ「空行」に分類する（[research.md](./research.md) R-1102・R-1103）。(c) 内容行を段落に連結する。**空行が1行だけなら区切りとみなさず**内容行として連結を続け、**空行が2行以上連続した場合にのみ**そこまでを1段落として確定し次の段落を開始する（[research.md](./research.md) R-1102、ボックス内部の余白を段落区切りと誤認しないための措置）。(d) 段落が1つ以上あれば**最後の段落**（各行を半角スペースで連結）を採用し、`maxLength`以下ならそのまま、超える場合は**その段落の先頭から**`maxLength`文字を切り詰めて返す（FR-1101・FR-1102）。(e) 段落が1つも無ければ、`output`を`\r\n`・`\n`・タブのみ半角スペースに正規化した文字列を対象に、既存どおり先頭から`maxLength`文字を切り詰めて返す（FR-1103のフォールバック）。`Brief()`のシグネチャ・既定の`maxLength`は変更しない（FR-1105、[contracts/brief-contract.md](./contracts/brief-contract.md)）。コメントに `FR-1101`・`FR-1102`・`FR-1103`・`FR-1104`・`FR-1105` を付す

### User Story 1 の検証

- [X] T003 [US1] [quickstart.md](./quickstart.md) 手順1の**ビルド**を実施する。Debug/Release × x64/x86 の8行すべてで警告=0 エラー=0。T002 に依存 — **結果: 合格**（4構成すべて警告=0 エラー=0）
- [X] T004 [US1] [quickstart.md](./quickstart.md) 手順2の **ST-1101**（`Brief()`のシグネチャ不変、`ProcessRunner.cpp`・`ProcessRunner.h`に011による追加差分がない、FR-1101〜1105の追跡可能性、`SetupRunner.cpp`の変更が`Brief()`関数の範囲に閉じていること）を実施する。T003 に依存 — **結果: 合格**（シグネチャ一致、`ProcessRunner.cpp/.h`は008・009からの差分のみ、FR-1101〜1105すべて1件以上、`git diff`の新規ハンクは`Brief()`関数の範囲内のみで他の関数への変更なし）
- [X] T005 [US1] [quickstart.md](./quickstart.md) 手順3の **DT-1101**（`Brief()`の整形結果をホワイトボックスで直接確認、ケースA・B・C）を実施する。**ケースAは実機で取得した実際の`specify init`失敗出力そのものを使う**（単純化した合成文字列では末尾優先等の誤った実装でも見かけ上テストが通ってしまうため、[research.md](./research.md) R-1101・R-1102参照）。戻り値に`"claude not found"`が含まれること（SC-1101）、ケースB（短い装飾なしメッセージ）で入力と戻り値が完全一致すること（SC-1102）、ケースC（装飾のみ）で戻り値が空文字列にならないこと（FR-1103）を確認する。T004 に依存 — **結果: 全ケース合格**。ケースA: `containsClaudeNotFound=1`（`length=163`、`research.md`での事前検証と完全一致する結果文字列）。ケースB: `matches=1`（入力と完全一致）。ケースC: `isEmpty=0`

**チェックポイント**: ここまでで US1 は独立して完結する。装飾バナーに埋もれていた実際のエラー内容が結果一覧から読み取れるようになったことが実測で確認できる

---

## Phase 4: Polish & Cross-Cutting Concerns（仕上げと横断的関心事）

- [X] T006 [P] [quickstart.md](./quickstart.md) 付録の**コード分析**（`RunCodeAnalysis=true` / `EnablePREfast=true`）を実施し、指摘が0件であること（憲章の技術的制約）。T005 に依存 — **結果: 合格**（出力0件）
- [X] T007 T005が合格したら、`CLAUDE.md` の「設計上、壊しやすい箇所」節に本フィーチャーで得た知見を追記する — `specify init`等の一部の外部コマンドは、罫線・ブロック文字による装飾バナーを出力の先頭に付けるため、単純な先頭切り詰めでは実際のエラー本文が失われること、対策として装飾文字の除去・段落単位への分割・最後の段落を先頭優先で切り詰める処理を`Brief()`に導入したこと（当初検討した単純な末尾優先切り詰めでは実機データ上エラー見出しが失われることが判明したため設計変更したこと）。あわせて「既存機能」の表に011を追加する。**T005が不合格の場合はこのタスクを実施しない**。T006 に依存 — **結果: 済**（「設計上、壊しやすい箇所」に1項目追加、既存機能の表に011を追加）

---

## Dependencies & Execution Order（依存関係と実行順序）

### Phase Dependencies（フェーズの依存関係）

- **Setup (Phase 1)**: 依存なし。**T001 は実装を始める前に必ず実施する**
- **Foundational (Phase 2)**: タスクなし（該当なし）
- **User Story 1 (Phase 3)**: T001 の後
- **Polish (Phase 4)**: US1 の完了後

### 重要な順序

| タスク | 依存 | 理由 |
|---|---|---|
| T002 | — | `Brief()`の内部実装のみの変更であり、他フィーチャーへの依存はない（008〜010とは独立） |
| T005（DT-1101） | T004 | 静的検証（シグネチャ・差分範囲）が済んでから、整形結果の動的確認に進む |
| T007（CLAUDE.md更新） | T005の合格 | 効果が確認できていない知見を書き残さない |

### Parallel Opportunities（並行実行の機会）

- 本フィーチャーは変更ファイルが1つ（`SetupRunner.cpp`）のみのため、T002〜T005は逐次実行が基本
- T006（コード分析）はT005完了後に実施する

---

## Implementation Strategy（実装戦略）

### MVP First（User Story 1 のみ、本フィーチャーの全体）

1. Phase 1: T001（ベースライン取得）
2. Phase 3: T002〜T005（実装と自動検証）
3. **停止して検証**: DT-1101の3ケースすべての結果を確認する
4. Phase 4: コード分析、知見の記録

### 実装順序に関する注意

- **T002で空行1行を段落の区切りと誤判定しない。** `specify`のボックス描画はタイトルと本文の間に1行の余白を挟むため、空行1行ごとに段落を区切ると、エラー見出し（例: `Agent Detection Error`）と本文（例: `claude not found`）が別々の段落に分かれてしまい、「最後の段落」に見出しが含まれなくなる。区切りは**2行以上連続した空行**のみで判定する
- **T002は「除去後のテキスト全体の末尾を採る」という設計にしない。** `/speckit-analyze`での指摘により、この単純な設計では実機データ上「claude not found」が失われることが実測で確認されている（[research.md](./research.md) R-1102）。必ず段落分割→最後の段落→その段落の**先頭**から切り詰める、という手順にする
- **T002で段落が1つも無い場合のフォールバック（FR-1103）を忘れない。** ここを忘れると、罫線のみで構成される出力に対して空文字列が理由として表示され、「常に何らかの理由文字列を表示する」という既存の前提（FR-1103、004以来の結果一覧の設計）が崩れる
- **T005のケースAは実機データをそのまま使う。** 単純化した合成文字列（ロゴ＋見出しのみで説明文を続けない等）では、誤った実装（例: 単純な末尾切り詰め）でもテストが通ってしまい、実際のバグを検出できない

---

## Notes（注記）

- [P] タスク = 異なるファイル、依存関係なし（本フィーチャーは実質1ファイルのため該当タスクは少ない）
- [Story] ラベルは、トレーサビリティのためにタスクを特定のユーザーストーリーに対応付ける
- 検証タスク（T003〜T005）はコードを書くタスクではなく、[quickstart.md](./quickstart.md) の手順を実施して結果を確認するタスクである
- 検証用の `.cpp` はスクラッチパッドに置き、**リポジトリには追加しない**（憲章の技術的制約）
