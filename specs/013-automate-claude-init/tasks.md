---

description: "CLAUDE.md生成とコミットの自動化（013）の実装タスク"
---

# Tasks: CLAUDE.md生成とコミットの自動化

**Input**: `/specs/013-automate-claude-init/` からの設計ドキュメント

**Prerequisites（前提条件）**: [plan.md](./plan.md)、[spec.md](./spec.md)、[research.md](./research.md)、[data-model.md](./data-model.md)、[contracts/claude-init-contract.md](./contracts/claude-init-contract.md)、[quickstart.md](./quickstart.md)

**Tests（テスト）**: 本リポジトリに自動テストスイートは存在しない（憲章の技術的制約）。代わりに静的テスト ST-1301〜1303 を [quickstart.md](./quickstart.md) の手順を実施するタスクとして定義する。動的テスト DT-1301 はフォルダ選択ダイアログを伴うため008以降と同じ理由で自動化できず、利用者による任意の手動実行とする。

**Organization（構成）**: spec.mdにはUser Story 1（P1）とUser Story 2（P2）が定義されている。

## Format（形式）: `[ID] [P?] [Story] Description`

- **[P]**: 並行実行可能（異なるファイル、依存関係なし）
- **[Story]**: US1 = 手順6（CLAUDE.md生成とコミット）の自動実行（P1）、US2 = 完了案内から不要になった手動手順を削除（P2）

---

## Phase 1: Setup（共有インフラ）

**目的**: 実装前の状態を記録する

- [X] T001 アプリを終了させ（`Get-Process MFCApplication1 -ErrorAction SilentlyContinue | Stop-Process -Force`）、実装前の状態を控えておく: (a) `MFCApplication1/SetupRunner.h`の`StepId`が5値のみであること、(b) `MFCApplication1/SetupRunner.cpp`の`CreateSteps()`が5手順のみを返すこと、(c) `MFCApplication1/MFCApplication1Dlg.cpp`の`ShowCompletionGuide()`の案内文が012時点の5手順（`/init`実行・CLAUDE.mdコミットを含む）のままであること（[quickstart.md](./quickstart.md) ST-1301〜1302 のベースライン）

---

## Phase 2: Foundational（ブロッキングとなる前提条件）

**目的**: User Story 1・2 が依存する共有の基盤コードを用意する

本フィーチャーには該当タスクがない。User Story 1（`SetupRunner.h`/`.cpp`）と User Story 2
（`MFCApplication1Dlg.cpp`）は異なるファイルへの変更であり、共有の新しい型・関数を必要としない
（[plan.md](./plan.md) Project Structure）。Setup の後、直接 User Story 1 の実装へ進める。

---

## Phase 3: User Story 1 - 手順6（CLAUDE.md生成とコミット）の自動実行 (Priority: P1) 🎯 MVP

**目標**: 既存5手順に続けて、Claude Code CLIによる`/init`実行でCLAUDE.mdを自動生成し、そのままコミットする6番目の手順を追加する

**独立したテスト**: CLAUDE.mdが存在しない対象フォルダで「生成」を実行し、完走後に対象フォルダ直下に
CLAUDE.mdが存在し、`git log`に「CLAUDE.md を追加」のコミットが記録されていることを確認する（案内文の
更新（US2）が無くても、この動作自体は単独で検証できる）

### User Story 1 の実装

- [X] T002 [P] [US1] `MFCApplication1/SetupRunner.h` の`enum class StepId`（[SetupRunner.h:12-20](../../MFCApplication1/SetupRunner.h)）に`GenerateClaudeMd`を追加する。コメントで「CLAUDE.md の生成とコミット (InitGit に依存)」と説明する（FR-1301、[data-model.md](./data-model.md)）
- [X] T003 [US1] `MFCApplication1/SetupRunner.cpp` を変更する。T002 に依存:
  - (a) 匿名名前空間に`RunGenerateClaudeMd(SetupStep& step, const CString& folderPath, ISetupProgressSink* sink)`関数を追加する。実装内容は[contracts/claude-init-contract.md](./contracts/claude-init-contract.md)のとおり: ① 対象フォルダ直下のCLAUDE.md存在確認（`PathFileExists`等）→存在すれば`StepResult::Skipped`（理由「CLAUDE.md は既に生成されています。」）(FR-1303) ② 存在しなければ`ProcessRunner::Run(_T("claude --print /init --permission-mode bypassPermissions --output-format text"), folderPath, 15 * 60 * 1000)`を実行 (FR-1304、FR-1309、FR-1310、R-1301、R-1305) ③ 起動失敗時は`CommandNotFoundReason(_T("claude"))`で失敗 (FR-1307) ④ 終了コード非0なら`Brief(output)`で失敗 ⑤ 終了コード0でもCLAUDE.md不在なら「`/init` は完了しましたが、CLAUDE.md を確認できませんでした。」で失敗 (FR-1305) ⑥ CLAUDE.md確認後、`git add -- CLAUDE.md`を実行 (FR-1306、R-1304) ⑦ 失敗なら`Brief(output)`で失敗 ⑧ `git commit -m \"CLAUDE.md を追加\"`を実行 (FR-1306) ⑨ 失敗なら`Brief(output)`で失敗、成功なら`StepResult::Success`（理由「CLAUDE.md を生成し、コミットしました。」）⑩ 上記①〜⑨のいずれの失敗経路も`StepResult::Failed`を返して`RunAll()`へ復帰するのみであり、それ以前の手順（Git初期化等）の結果を書き換えたり取り消したりする処理は一切行わない。既存4手順（`RunInstallUv`〜`RunInitSpecKit`・`RunInitGit`）のコードには触れないため、この非ロールバック特性は追加コードを書かないことで自然に満たされる。関数冒頭のコメントに「本関数はStepResult::Failedを返すのみで、以前の手順へのロールバックは行わない (FR-1308)」と明記する
  - (b) `CreateSteps()`（[SetupRunner.cpp:470-505](../../MFCApplication1/SetupRunner.cpp)）に6番目の`SetupStep`を追加する。`id = StepId::GenerateClaudeMd`、`displayName = _T("CLAUDE.md の生成とコミット")`、`dependsOn = StepId::InitGit`とする (FR-1301、FR-1302、R-1306)
  - (c) `RunAll()`の`switch`（[SetupRunner.cpp:521-540](../../MFCApplication1/SetupRunner.cpp)）に`case StepId::GenerateClaudeMd: RunGenerateClaudeMd(step, folderPath, sink); break;`を追加する
  - コメントに`FR-1301`〜`FR-1310`を付す。既存5手順の関数（`RunInstallUv`〜`RunInitGit`）・`Brief()`・`CommandNotFoundReason()`・`Finish()`・`FindStep()`・`DependencyFailed()`は一切変更しない

### User Story 1 の検証

- [X] T004 [US1] [quickstart.md](./quickstart.md) 手順1の**ビルド**を実施する。Debug/Release × x64/x86 の8行すべてで警告=0 エラー=0。T003 に依存 — **結果: 合格**（4構成すべて警告=0 エラー=0）
- [X] T005 [US1] [quickstart.md](./quickstart.md) 手順2の **ST-1301**（`GenerateClaudeMd`の追加、`dependsOn=InitGit`、`claude /init`のコマンドライン、`git add -- CLAUDE.md`単体ステージング、既存5手順のコマンドライン無変更、FR-1301〜1310の追跡可能性、`ProcessRunner`・`TargetFolderState`・`GitignoreWriter`に013による追加差分が無いこと）を実施する。T004 に依存 — **結果: 全項目合格**（GenerateClaudeMd追加True、dependsOn=InitGit True、claude /initコマンドラインTrue、git add単体ステージングTrue、既存5コマンド無変更すべてTrue、FR-1301〜1310すべて1件以上。ProcessRunner.cppの差分は008・009由来の既存差分のみで013による追加なし（前回コミットが007のため差分に008以降すべてが含まれる。013では同ファイルを一切編集していないことで確認））

**チェックポイント**: ここまでで US1 は独立して完結する。案内文はまだ012のままだが、「生成」を実行すると
実際に6手順目が動作し、CLAUDE.mdの生成とコミットが自動化されている

---

## Phase 4: User Story 2 - 完了案内から不要になった手動手順を削除 (Priority: P2)

**目標**: `ShowCompletionGuide()`の案内文から、012で追加した「`/init`実行」「CLAUDE.mdコミット」の
2手順を削除し、残りの手順の番号を振り直す

**独立したテスト**: SDD化を成功させ、表示される完了案内の文言を確認し、「/initを実行する」
「CLAUDE.mdをコミットする」という文言（012由来）が無く、残りの手順が正しい順序・番号で表示されている
ことを確認する

### User Story 2 の実装

- [X] T006 [P] [US2] `MFCApplication1/MFCApplication1Dlg.cpp` の `ShowCompletionGuide()`（[MFCApplication1Dlg.cpp:585](../../MFCApplication1/MFCApplication1Dlg.cpp#L585)）内、成功時の `guide.Format(...)` の文字列リテラルを変更する。012で追加した「2. `/init` を実行し CLAUDE.md を生成する」「3. 生成された CLAUDE.md をコミットする」の2行を削除し、残りの手順（VS Codeを開く／テスト方針決定と憲章制定／`/init`再実行）を1から順に振り直す（FR-1311）。見出し・手順1（VS Codeを開く）の文言、`m_workerPath`の埋め込み方法、失敗時の案内文（`anyFailed`が真の場合の分岐）は一切変更しない（FR-1312）。コメントに`FR-1311`・`FR-1312`を付す。`MFCApplication1/SetupRunner.h`・`SetupRunner.cpp`への変更（T002・T003）とは異なるファイルのため、それらと並行実装可能

### User Story 2 の検証

- [X] T007 [US2] [quickstart.md](./quickstart.md) 手順1の**ビルド**を実施する（T003・T006を反映した状態での再ビルド）。8行すべてで警告=0 エラー=0。T003・T006 に依存 — **結果: 合格**（4構成すべて警告=0 エラー=0）
- [X] T008 [US2] [quickstart.md](./quickstart.md) 手順2の **ST-1302**（012で追加した2手順の文言削除、残り3手順の文言維持と順序、失敗時案内文の無変更）を実施する。T007 に依存 — **結果: 全項目合格**（012の2手順の文言は両方とも削除済み、残り3手順の文言と順序はTrue、失敗時案内文は無変更、FR-1311・1312とも1件）

**チェックポイント**: ここまでで US1・US2 とも完結する。「生成」を実行すると6手順すべてが動作し、
完了案内も自動化を反映した最新の文言になる

---

## Phase 5: Polish & Cross-Cutting Concerns（仕上げと横断的関心事）

- [X] T009 [quickstart.md](./quickstart.md) 手順2の **ST-1303**（`.rc`の`IDC_LIST_RESULT`・`IDC_STATIC_GUIDE`・ダイアログ全体の高さが012の値から変化していないこと、research.md R-1302・R-1303の「寸法変更不要」という判断の裏付け）を実施する。**もし寸法変更が必要と判明した場合は、このタスクを未完了のまま報告し、`.rc`変更タスクを追加すること。** T007・T008 に依存 — **結果: 合格**（90・61・233のいずれも012から変化なし。寸法変更不要の判断が裏付けられた。`.rc`自体は013で一切編集していない）
- [X] T010 [quickstart.md](./quickstart.md) 手順3の **DT-1301**（実機でのフルフロー確認、任意・利用者による手動実行）を実施する。認証済みの`claude`が導入された環境で「生成」を実行し、6手順すべての成功、CLAUDE.mdの生成とコミット、完了案内の文言更新、起動直後の大きさのままでの表示（SC-1301〜1303）を確認する。実行するかどうかは利用者の判断に委ねる。T009 に依存 — **結果: 部分的に実施**（フォルダ選択ダイアログはCLAUDE.mdの規約どおり自動化不能なため、GUI操作そのものは未実施。代わりに008〜011と同じホワイトボックス手法で、実際にビルドされた`SetupRunner.cpp`の`RunGenerateClaudeMd()`を、本物の`ProcessRunner::Run()`経由で実プロジェクト構成の対象フォルダ（`C:\work2\test\MFCApplication1`）に対して2回実行した。1回目: `claude --print /init --permission-mode bypassPermissions`が実際にCLAUDE.mdを生成し、`git add -- CLAUDE.md`→`git commit -m "CLAUDE.md を追加"`まで完走（`StepResult::Success`、`git log`に単独コミットとして記録、`.claude`/`.specify`は未ステージのまま＝他の変更を巻き込んでいないことを確認）。2回目: 再実行すると`StepResult::Skipped`（べき等性を確認）。また、ほぼ空のリポジトリ（README.mdのみ）に対して実行した際は、`claude`が「実体のないCLAUDE.mdは作らない」と判断してCLAUDE.mdを生成せず終了コード0で終わるケースを発見し、この場合も本実装が`StepResult::Failed`（「/init は完了しましたが、CLAUDE.md を確認できませんでした。」）として正しく検出することを確認した（FR-1305の二段判定が実際に機能することの実証）。**未確認のまま残る項目**: 完了案内の実際の画面表示（起動直後の大きさでのスクロール有無、SC-1203）は、フォルダ選択ダイアログを介したGUI操作でしか確認できないため、利用者による目視確認を引き続き推奨する**
- [X] T011 [P] [quickstart.md](./quickstart.md) 付録の**コード分析**（`RunCodeAnalysis=true` / `EnablePREfast=true`）を実施し、指摘が0件であること（憲章の技術的制約）。T009 に依存 — **結果: 合格**（出力0件）
- [X] T012 T009が合格したら、`CLAUDE.md`（プロジェクト直下）の「既存機能」の表に013を追加する。あわせて、006 FR-507aの見積もり表（結果一覧の行数）が「5手順=5行、上限7行、余裕2行」から「6手順=6行、上限7行、余裕1行」に変わったことを、013自身の成果物（本ファイルや research.md）に記録済みであることを確認する（006自体の成果物は書き換えない、012までの慣例を踏襲）。**T009が不合格の場合はこのタスクを実施しない**。T011 に依存 — **結果: 済**。既存機能の表に013を追加し、あわせて「設計上、壊しやすい箇所」に`claude --print /init --permission-mode bypassPermissions`の必須フラグを008〜012と同じ様式で追記した。006の見積もり表は013のresearch.md R-1302に記録済みのため006自体は書き換えていない

---

## Dependencies & Execution Order（依存関係と実行順序）

### Phase Dependencies（フェーズの依存関係）

- **Setup (Phase 1)**: 依存なし。**T001 は実装を始める前に必ず実施する**
- **Foundational (Phase 2)**: タスクなし（該当なし）
- **User Story 1 (Phase 3)**: T001 の後
- **User Story 2 (Phase 4)**: T001 の後（T002・T003 とはファイルが異なるため並行可能。ただしビルド・検証は US1 の実装が先に完了している前提で進める）
- **Polish (Phase 5)**: US1・US2 の完了後

### 重要な順序

| タスク | 依存 | 理由 |
|---|---|---|
| T002 → T003 | T002 | `StepId`に`GenerateClaudeMd`が無いと`CreateSteps()`/`RunAll()`から参照できない |
| T003・T006 | — | `SetupRunner.cpp`（手順の実処理）と`MFCApplication1Dlg.cpp`（案内文）は別ファイルへの独立した変更であり、互いへの依存も無い |
| T010（DT-1301） | 利用者の任意判断 | 実機でのフルフロー確認は環境依存のため、実行するかは利用者に委ねる。ただしR-1301〜1303の見積もりを実機で裏付ける唯一の手段でもあるため実行を推奨する |
| T012（CLAUDE.md更新） | T009の合格 | 効果が確認できていない変更を既存機能表に記録しない |

### Parallel Opportunities（並行実行の機会）

- T002（`SetupRunner.h`）とT006（`MFCApplication1Dlg.cpp`）は異なるファイルへの変更のため並行実行可能 [P]
- T011（コード分析）はT009完了後、T010（任意）の結果を待たずに実施してよい

---

## Implementation Strategy（実装戦略）

### MVP First（User Story 1 のみ）

1. Phase 1: T001（ベースライン取得）
2. Phase 3: T002〜T005（手順6の実装と検証）
3. **停止して検証**: ST-1301の結果を確認する。この時点で「生成」は6手順を実行しCLAUDE.mdを自動生成・コミットするが、案内文はまだ012のまま古い
4. Phase 4: T006〜T008（案内文の更新と検証）
5. Phase 5: ST-1303、コード分析、既存機能表の更新。DT-1301（実機フルフロー）は利用者の任意だが推奨

### 実装順序に関する注意

- **T003は`RunGenerateClaudeMd()`を既存5手順の関数（`RunInstallUv`〜`RunInitGit`）と同じ様式（`SetupStep&`を受け取り`Finish()`で終える）で実装すること。** 新しいヘルパークラスや戻り値の型を作らない（原則III YAGNI）
- **T003の`git add`は`-A`ではなく`-- CLAUDE.md`と対象を明示すること。** `RunInitGit`が持つ「`git status`を見て他人の変更を検出する」分岐ロジックは複製しない（R-1304）
- **T006は成功時の分岐（`anyFailed`が偽の場合）にのみ変更する。** 失敗時の案内文（「一部の手順が失敗しました...」）には触れない（FR-1312、012からの継続方針）
- **T009で寸法変更が不要と確認できなかった場合は、`.rc`の変更タスクを追加してから先に進むこと。** research.md R-1302・R-1303はビルド前の見積もりであり、実装後の数値確認で裏付けを取るまでは確定していない

---

## Notes（注記）

- [P] タスク = 異なるファイル、依存関係なし
- [Story] ラベルは、トレーサビリティのためにタスクを特定のユーザーストーリーに対応付ける
- 検証タスク（T004〜T005、T007〜T010）はコードを書くタスクではなく、[quickstart.md](./quickstart.md) の手順を実施して結果を確認するタスクである
- T010（DT-1301）は環境依存のため必須ではないが、`/init`の実行時間・CLAUDE.mdの実際の生成内容・完了案内の見え方を実機で確認できる唯一の手段でもあるため、実行を強く推奨する。T005・T008・T009（ST-1301〜1303）が本フィーチャーの自動検証手段である
