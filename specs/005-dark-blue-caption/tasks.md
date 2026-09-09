---

description: "タイトルバーを濃い青にする（005）の実装タスク"
---

# Tasks: タイトルバーを濃い青にする

**Input**: `/specs/005-dark-blue-caption/` からの設計ドキュメント

**Prerequisites（前提条件）**: [plan.md](./plan.md)、[spec.md](./spec.md)、[research.md](./research.md)、[data-model.md](./data-model.md)、[contracts/ui-contract.md](./contracts/ui-contract.md)、[quickstart.md](./quickstart.md)

**Tests（テスト）**: 本リポジトリに自動テストスイートは存在しない（憲章の技術的制約）。代わりに静的テスト ST-401〜406 と動的テスト DT-401〜409 を、[quickstart.md](./quickstart.md) の手順を実施するタスクとして定義する。

**Organization（構成）**: タスクはユーザーストーリーごとにグループ化する。

## Format（形式）: `[ID] [P?] [Story] Description`

- **[P]**: 並行実行可能（異なるファイル、依存関係なし）
- **[Story]**: US1 = タイトルバーが濃い青になる（P1）／US2 = 着色できない環境でも今までどおり使える（P2）

---

## Phase 1: Setup（共有インフラ）

**目的**: 実装前の状態を記録し、後で「004 を壊していないこと」を突き合わせられるようにする

- [X] T001 アプリを終了させ（`Get-Process MFCApplication1 | Stop-Process -Force`、起動中だとビルドが `LNK1104` で失敗する）、実装前の 004 のキーワード件数を [quickstart.md](./quickstart.md) 手順2の ST-403 後半のスクリプトで取得して控えておく。**測定対象は `MFCApplication1Dlg.h` と `.cpp` の両方**で、**行頭が `//` のコメント行は除外する**。実測済みの期待値は OnSize 3、ON_WM_SIZE 1、OnGetMinMaxInfo 3、ptMinTrackSize 2、ApplyAnchors 3、InitAnchors 3、m_layoutReady 5、SIZE_MINIMIZED 1。**実装後にこの値と一致することが 004 無傷の根拠になる**

---

## Phase 2: Foundational（ブロッキングとなる前提条件）

**目的**: 着色 API を呼べる状態にする。ここが済むまで US1・US2 の実装に入れない

- [X] T002 `MFCApplication1/MFCApplication1Dlg.cpp` の先頭に `#include <dwmapi.h>` と `#pragma comment(lib, "dwmapi.lib")` を追加する。**`.vcxproj` は編集しない** — 4構成（Debug/Release × x64/Win32）すべてに書き足す必要があり、1つでも漏れるとその構成だけリンクエラーになる（[research.md](./research.md) R-406）。SDK 10.0.26100.0 には x64・x86 両方の `dwmapi.lib` があることを確認済み
- [X] T003 `MFCApplication1/MFCApplication1Dlg.cpp` にファイルスコープの色定数を2つ追加する — キャプション背景 `#1F3864`、キャプション文字 白。**COLORREF は `0x00BBGGRR` の順**であり、`RGB(0x1F, 0x38, 0x64)` マクロを使うことで並べ替えの取り違えを構造的に防ぐ。リテラルで書く場合は `0x0064381F`。`0x001F3864` と書くと R と B が入れ替わって茶色寄りになる（[research.md](./research.md) R-402、[data-model.md](./data-model.md) 3章）。**由来を示すコメントを必ず添えること**。T002 に依存

**チェックポイント**: 着色 API を呼ぶ準備が整った

---

## Phase 3: User Story 1 - タイトルバーが濃い青になる (Priority: P1) 🎯 MVP

**目標**: 開いているウィンドウの中から、タイトルの文字を読まずに色だけでこのツールを見つけられるようにする

**独立したテスト**: アプリを起動し、タイトルバーのピクセルを採取して `#1F3864` と一致することを確認する。非アクティブ・最大化・最小化からの復帰でも一致し続けること

### User Story 1 の実装

- [X] T004 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` の `OnInitDialog()` に `DwmSetWindowAttribute(GetSafeHwnd(), DWMWA_CAPTION_COLOR, ...)` を追加する（FR-401、FR-402、FR-404）。**呼び出し位置は `OnInitDialog` の先頭付近**とし、004 が末尾で `m_layoutReady` を立てる構造には触れない。`OnInitDialog` はウィンドウが画面に出る前に走るため、既定色が一瞬見えてから塗り替わることはない（[research.md](./research.md) R-404）。T003 に依存
- [X] T005 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` の同じ場所に `DWMWA_TEXT_COLOR` で文字色を白に設定する処理を追加する（FR-403）。これを省くとライトモードでは文字が濃色で描かれ、濃紺の背景に埋もれて読めなくなる。背景 `#1F3864` と白のコントラスト比は約 11.5:1 で SC-402 の下限 4.5:1 を大きく上回る（[research.md](./research.md) R-403）。T004 に依存
- [X] T006 [US1] `MFCApplication1/MFCApplication1Dlg.cpp` の追加箇所に FR 番号のコメントを付す（FR-412）。着色処理に直接対応するのは FR-401〜FR-404・FR-408・FR-411。あわせて **FR-405〜FR-407・FR-409・FR-410 は「何もしないことが要件」である**ため、`OnInitDialog` に**なぜ再設定しないのかを説明するコメントを1箇所にまとめて記載する**。これが無いと、後から `OnSize` で塗り直す修正を「抜けていた処理の追加」と誤認して入れられてしまう。**このコメントは語 `OnSize` を含むため、ST-403 がコメント行を除外して数えていないと必ず不合格になる** — 検査側は対処済みだが、ST-403 のスクリプトを書き換える際はこの前提を壊さないこと。T005 に依存

### User Story 1 の検証

- [X] T007 [US1] [quickstart.md](./quickstart.md) 手順2の **ST-401**（着色処理の存在）を実施する。`DwmSetWindowAttribute` が2回、`DWMWA_CAPTION_COLOR` が1回、`DWMWA_TEXT_COLOR` が1回、`dwmapi` の include と `#pragma comment` が現れること。T006 に依存
- [X] T008 [US1] [quickstart.md](./quickstart.md) 手順2の **ST-402（色の並びの取り違え検出）** を実施する。誤った並び `0x001F3864` の出現が **0件**であり、かつ正しい並び `0x0064381F` か `RGB(0x1F, 0x38, 0x64)` のどちらかが1件以上あること。あわせて**文字色（白）の定義が1件以上ある**ことを確認する。**この検査が本フィーチャー最大の要所** — 取り違えても「濃い色のタイトルバー」に見えてしまい、目視では気づきにくい。背景と文字の2つの定数が確定することで **SC-402（コントラスト比 4.5:1 以上）も同時に担保される**（約 11.5:1）。T006 に依存
- [X] T009 [US1] [quickstart.md](./quickstart.md) 手順2の **ST-403**（再設定していない／004 が無傷）を実施する。`DwmSetWindowAttribute` の呼び出しが `OnInitDialog` の中にのみあり、`OnSize` / `OnGetMinMaxInfo` / `OnPaint` に無いこと。あわせて 004 のキーワード8件のカウントが **T001 で控えた値と完全に一致**すること（OnSize 3、ON_WM_SIZE 1、OnGetMinMaxInfo 3、ptMinTrackSize 2、ApplyAnchors 3、InitAnchors 3、m_layoutReady 5、SIZE_MINIMIZED 1）。**スクリプトは `.h` と `.cpp` の両方を読み、コメント行を除外すること** — どちらか一方でも外すと検査が壊れる（`.cpp` だけでは実装前から不合格、コメントを含めると T006 のコメントで不合格）。あわせて FR-404 の合否もここで判定する（着色処理が `OnInitDialog` にあること。[quickstart.md](./quickstart.md) 手順3の注記を参照）。T006、T001 に依存
- [X] T010 [US1] [quickstart.md](./quickstart.md) 手順2の **ST-405**（FR による追跡可能性）を実施する。FR-401〜FR-404・FR-408・FR-411 がコメントとして必ず現れ、FR-405〜407・FR-409・FR-410 が「再設定しない理由」のコメントに現れること。T006 に依存
- [X] T011 [US1] [quickstart.md](./quickstart.md) 手順1の **ST-404**（4構成ビルド）を実施する。Debug/Release × x64/x86 の8行すべてで警告=0 エラー=0。**`#pragma comment` によるリンクが x86 構成でも効いていることがここで確認される**。`.sln` に渡すプラットフォーム名は `x86`（`Win32` では `error MSB4126`）。T006 に依存
- [X] T012 [US1] [quickstart.md](./quickstart.md) 手順3の **DT-401〜DT-404、DT-406** を自動計測で実施する。起動直後・非アクティブ・最大化・復元・最小化からの復帰の5点すべてでタイトルバーのピクセルが `#1F3864` と**完全一致**すること。設定した色はプログラムから読み戻せない（`DwmGetWindowAttribute` は `E_INVALIDARG`）ため、画面のピクセル採取で判定する（[research.md](./research.md) R-409）。**非アクティブ状態はタスクバーにフォーカスを移して作る** — 別アプリを起動すると後片付けが要る。T011 に依存
- [X] T013 [US1] [quickstart.md](./quickstart.md) 手順4の **DT-407**（ライト／ダークモード）を実施する。Windows の「個人用設定」→「色」でモードを切り替え、それぞれで手順3のスクリプトを実行して5行すべて `#1F3864` であること（FR-407）。**確認後は元の設定（`AppsUseLightTheme=1`・`SystemUsesLightTheme=0`）に戻すこと**。T012 に依存
- [ ] T014 [US1] [quickstart.md](./quickstart.md) 手順5の **DT-405（アクセントカラー設定、目視・要手動）** を実施する。「タイトル バーとウィンドウの境界線にアクセント カラーを表示する」をオンにして、アクセントカラーではなく `#1F3864` で表示されること。**本フィーチャー唯一の未測定項目**（[research.md](./research.md) R-410）。**不合格の場合は実装の誤りではなく仕様の前提が崩れたことを意味するため、コードを直す前に報告し、FR-401 の見直しを検討する**。確認後は設定をオフに戻すこと。T012 に依存

**チェックポイント**: タイトルバーが濃い青になり、どの状態でも保たれる（MVP 達成）

---

## Phase 4: User Story 2 - 着色できない環境でも今までどおり使える (Priority: P2)

**目標**: 着色に失敗しても、色が付かないだけで機能は何も変わらない

**独立したテスト**: 着色が効かない状態を作ってアプリを起動し、エラーが出ず、既定の外観のまま 004 までの動作がそのまま働くことを確認する

- [X] T015 [US2] [quickstart.md](./quickstart.md) 手順2の **ST-406**（戻り値を判定していない）を実施する。着色の呼び出しが `if` / `SUCCEEDED` / `FAILED` で囲まれておらず、`HRESULT` を受ける変数も無いこと（FR-408）。**判定を書くと、そこから「失敗したら何かする」分岐が生えて FR-408 に反する**。未知の属性でも `E_INVALIDARG` が返るだけで例外にならないことは実測済み（[research.md](./research.md) R-405）。T006 に依存
- [X] T016 [US2] [quickstart.md](./quickstart.md) 手順7の **DT-409（着色できない環境の模擬）** を実施する。`MFCApplication1/MFCApplication1Dlg.cpp` の属性番号を一時的に存在しない値（999）へ差し替えてビルドし、(a) 起動時にエラーも警告も出ない、(b) タイトルバーが Windows 既定の外観になる、(c) 004 のリサイズ追従の全項目が 004 の実測値と一致する、の3点を確認する（FR-408、FR-409）。**確認後は必ず元の属性番号に戻して再ビルドし、T012 を再実行して `#1F3864` に戻っていることを確かめること**。T012 に依存

**チェックポイント**: 両ユーザーストーリーが独立して動作する

---

## Phase 5: Polish & Cross-Cutting Concerns（仕上げと横断的関心事）

- [X] T017 [P] [quickstart.md](./quickstart.md) 付録の**コード分析**（`RunCodeAnalysis=true` / `EnablePREfast=true`）を実施し、指摘が0件であること（SC-405）。T011 に依存
- [X] T018 [P] `CLAUDE.md` に本フィーチャーで得た知見を追記する — (a) タイトルバーの着色に `DwmSetWindowAttribute` を使っており `OnSize` で塗り直してはならないこと、(b) `DWMWA_CAPTION_COLOR` は設定専用で読み戻せないため検証は画面のピクセル採取で行うこと、(c) **PowerShell 5.1 は BOM の無い `.ps1` を ANSI として読むため日本語を含むスクリプトは UTF-8 BOM 付きで保存すること**（本セッションで2回つまずいた）。既存の「エンコーディング」節と「テスト」節に足す。**本タスクはどの FR にも紐づかない文書整備であり、spec の要件ではない**（憲章 原則IV の観点で明示しておく）。T016 に依存
- [X] T019 [quickstart.md](./quickstart.md) 手順6の **DT-408（004 リサイズ追従の回帰確認）を、属性番号を元に戻した正規ビルドで実施する**。004 の実測値（初期 578x395、横+200 でパス欄 359→559、縦+150 で一覧 158→308、縮小復帰で初期値と完全一致、下限 578x395、列幅 212/106/212→292/146/292、表示可能行数 7→15→46）と**完全に一致**すること。**T016 の回帰確認は属性を 999 に壊したビルドで行うため、正規ビルドでの確認はここでしか行われない**。T016 に依存
- [ ] T020 [quickstart.md](./quickstart.md) の「完了条件」8項目をすべて確認する。あわせて **SC-404（既存機能の回帰）** のうちフォルダ選択を伴う部分 — SDD化の完走、再実行で結果一覧の5行すべてスキップ、未選択ガード、完了案内のコピー — を `C:\work\test\test01` で手動確認する。**この部分だけは自動化できない**（CLAUDE.md の制約）。T013〜T019 に依存

---

## Dependencies & Execution Order（依存関係と実行順序）

### Phase Dependencies（フェーズの依存関係）

- **Setup (Phase 1)**: 依存なし。ただし **T001 は実装を始める前に必ず実施する** — 実装後では 004 のベースラインが取れなくなる
- **Foundational (Phase 2)**: T001 の後。US1・US2 の両方をブロックする
- **User Story 1 (Phase 3)**: Foundational の後
- **User Story 2 (Phase 4)**: Foundational の後。ただし T016 は T012 の結果と突き合わせるため US1 の検証後に行う
- **Polish (Phase 5)**: US1・US2 の完了後

### 重要な順序

| タスク | 依存 | 理由 |
|---|---|---|
| T001（ベースライン取得） | — | **実装前でなければ意味がない** |
| T009（ST-403） | T001 | T001 で控えた値と突き合わせる |
| T016（着色失敗の模擬） | T012 | 正常時の値を先に確定させてから壊す |
| T012 の再実行 | T016 の後 | 属性番号を戻したことの確認 |
| T020（通し確認） | T013〜T019 | 全検証が揃った状態で実施する |

### Parallel Opportunities（並行実行の機会）

- **Phase 2**: T002 と T003 は同一ファイルの別箇所だが、include とファイルスコープ定数で場所が近く、逐次に行うほうが安全
- **Phase 3**: T007〜T011（静的テストとビルド）は互いに独立。T012〜T014 は逐次（T013・T014 は T012 の結果を前提にする）
- **Phase 5**: T017 と T018 は互いに独立

---

## Parallel Example: Phase 3（並行実行の例）

```text
Task: "ST-401 — 着色処理が存在する"
Task: "ST-402 — 色の並びの取り違えを検出する"
Task: "ST-405 — FR による追跡可能性"
```

---

## Implementation Strategy（実装戦略）

### MVP First（User Story 1 のみ）

1. Phase 1: T001（**実装前のベースライン取得**）
2. Phase 2: T002〜T003（include・リンク・定数）
3. Phase 3: T004〜T014（着色と検証）
4. **停止して検証**: T008（色の並び）と T009（004 無傷）を必ず通すこと

### 実装順序に関する注意

- **T001 を飛ばさない。** 実装後に 004 のカウントを取っても「元からその値だったか」が分からず、ST-403 が意味を失う
- **T003 では `RGB` マクロを使う。** リテラルで `0x0064381F` と書いても正しいが、`#1F3864` を見ながら手で並べ替える工程が入る分だけ取り違えやすい
- **T004・T005 を `OnInitDialog` の先頭付近に置く。** 末尾は 004 が `m_layoutReady` を立てる場所であり、そこに割り込むと初期化順序の契約が読みにくくなる
- **T006 のコメントを省略しない。** 「なぜ再設定しないのか」を書き残さないと、後から `OnSize` に塗り直しを足す修正が入る
- **T016 の後始末を忘れない。** 属性番号を 999 のままコミットすると、着色が効かないまま完成扱いになる。戻した後に T012 を再実行すること

### Incremental Delivery（段階的な提供）

1. Setup + Foundational → API を呼べる状態
2. User Story 1 → タイトルバーが濃い青になる（**MVP**）
3. User Story 2 → 着色できない環境での安全性を確認
4. Phase 5 → コード分析、CLAUDE.md 更新、通し確認

---

## Notes（注記）

- [P] タスク = 異なるファイル、依存関係なし
- [Story] ラベルは、トレーサビリティのためにタスクを特定のユーザーストーリーに対応付ける
- 検証タスク（T007〜T017、T019、T020）はコードを書くタスクではなく、[quickstart.md](./quickstart.md) の手順を実施して結果を確認するタスクである
- 検証用の `.ps1` はスクラッチパッドに置き、**リポジトリには追加しない**（憲章の技術的制約）
