# Phase 0 リサーチ: リポジトリ登録の要否を選択できるようにする

**Feature**: 016-repo-registration-toggle
**Date**: 2026-09-17

本書の結論はすべて**実測**に基づく（憲章 原則V）。

---

## R-1601: 手順5・手順6の依存関係は変更不要

**調査対象**: `SetupRunner.cpp` の `DependencyFailed()`（L174-187）と `RunAll()`（L604-642）。

```cpp
bool DependencyFailed(std::vector<SetupStep>& steps, const SetupStep& step)
{
    ...
    return parent->result == StepResult::Failed
        || parent->result == StepResult::SkippedUnmetDependency;
}
```

`GenerateClaudeMd`（手順6）の `dependsOn = StepId::InitGit`（手順5）は、親の結果が `Failed` または `SkippedUnmetDependency` の場合のみ「依存未達」としてブロックする。`StepResult::Skipped`（通常のスキップ。例: 「既に初期化済み」）は**依存を満たしたものとして扱われ、後続手順は実行される**。これは既存の「Git初期化済みならスキップしてもCLAUDE.md生成は実行される」という現行の動作（013で確立）そのものである。

**Decision**: 「NG」が選択されたとき、`RunInitGit()` の先頭で早期に `Finish(step, StepResult::Skipped, ...)` を返せば、既存の依存関係チェックの仕組みを**一切変更せずに**、手順6（CLAUDE.mdの生成）がそのまま実行される。`StepId`・`dependsOn`・`DependencyFailed()` のいずれにも変更は不要。

**Rationale**: 依存関係グラフの再設計（例: `dependsOn` を撤廃する等）は、004で確立した「1箇所に定義し複数箇所から参照する」設計と、006・013で踏襲されてきた `SetupStep`/`RunAll` の構造を壊す。既存の `Skipped` の意味論をそのまま使うことで、YAGNI（憲章 原則III）にも合致する最小の変更で済む。

**Alternatives considered**:
- `GenerateClaudeMd.dependsOn` を `StepId::None` に変更する — 却下。手順6は依然として「手順5が失敗（Failed）した場合はスキップすべき」という既存要件（013 FR-1301）を持つため、依存関係自体は維持する必要がある
- `RunAll()` に手順間の特別分岐を追加する — 却下。`RunInitGit()` 内で完結する変更のほうが影響範囲が小さい

---

## R-1602: 選択状態をワーカースレッドへ渡す方法

**調査対象**: `OnBnClickedButtonGenerate()`（`MFCApplication1Dlg.cpp` L517-543）と、既存の `m_workerPath` パターン。

既存コードは `m_targetPath`（UIから随時更新されるメンバ）を、生成ボタン押下時に `m_workerPath`（ワーカースレッド専用のコピー）へ複製し、以後ワーカースレッドは `m_workerPath` のみを参照する（`contracts/ui-contract.md`「2.」の既存契約）。ラジオボタンの選択も同型の競合状態を持ちうる（生成中に利用者がラジオボタンを操作できないよう無効化するが、念のため同じパターンを踏襲する）。

**Decision**: `m_workerPath` と対になる `bool m_workerRegisterRepo` をダイアログクラスに追加する。`OnBnClickedButtonGenerate()` の中で `IsDlgButtonChecked(IDC_RADIO_REPO_OK)` を読み取り、生成ボタン押下の瞬間に `m_workerRegisterRepo` へコピーしてからワーカースレッドを起動する。

**Rationale**: 既存の `m_workerPath` パターンと完全に対称であり、新しい設計判断を必要としない。

---

## R-1603: `SetupRunner` のシグネチャ変更

**調査対象**: `SetupRunner.h`（`CreateSteps()`・`RunAll()`）、`SetupRunner.cpp` の `RunInitGit()`・`RunGenerateClaudeMd()`。

FR-1605（手順6の項目名がOK/NGで変わる）を満たすには、一覧に行を並べる時点（`CreateSteps()`）で選択が分かっていなければならない。FR-1603・FR-1604（手順5の実行有無・手順6のコミット有無）を満たすには、実行時（`RunAll()` 経由で呼ばれる `RunInitGit()`・`RunGenerateClaudeMd()`）でも選択が分かっていなければならない。

**Decision**:
- `CreateSteps(bool registerRepo)` — `registerRepo` に応じて `claudeMd.displayName` を「CLAUDE.md の生成とコミット」（true）または「CLAUDE.md の生成」（false）に出し分ける。他の5手順の `displayName` は変更しない
- `RunAll(const CString& folderPath, std::vector<SetupStep>& steps, ISetupProgressSink* sink, bool registerRepo)` — 引数を1つ追加し、`RunInitGit`・`RunGenerateClaudeMd` の呼び出しにそのまま渡す
- `RunInitGit(SetupStep& step, const CString& folderPath, ISetupProgressSink* sink, bool registerRepo)` — 関数の先頭で `if (!registerRepo) { Finish(step, StepResult::Skipped, "「NG」が選択されたため、Gitの初期化を行いませんでした。", sink); return; }` を追加する。既存の本体ロジック（`.gitignore` 生成・`git init`・`git add`・`git commit`）には一切手を入れない
- `RunGenerateClaudeMd(SetupStep& step, const CString& folderPath, ISetupProgressSink* sink, bool registerRepo)` — `git add -- CLAUDE.md` / `git commit` の直前で分岐する。`registerRepo` が false の場合はコミットを行わず `Finish(step, StepResult::Success, "CLAUDE.md を生成しました。", sink);` で終える。それ以外（CLAUDE.md生成・存在確認）のロジックは変更しない

**Rationale**: 影響範囲を「関数シグネチャへの引数追加」と「各関数の先頭・末尾の分岐追加」に限定し、既存の外部コマンド呼び出し・エラー処理・メッセージ文言をそのまま再利用する。これにより011（`Brief()`）・008（タイムアウト監視）・009（PATH再読込）等、`ProcessRunner::Run()` に集約された既存の対策をすべてそのまま享受できる。

**Alternatives considered**:
- `SetupStep` 構造体に `bool registerRepo` フィールドを追加する — 却下。6手順すべてに共通のフラグを個々の手順の構造体へ持たせるのは責務が合わず、`RunAll()`の引数として渡すほうが影響範囲が明確

---

## R-1604: 新規コントロールのDLU配置

**測定環境**: 開発機実DPI 115、`MS UI Gothic` 9pt（既存踏襲）。

**方法**: 015で確立した手法（`TextRenderer.MeasureText` によるDPI相当フォントでの文字幅計測）をそのまま用いる。

**実測（新規文字列）**:

| DPI | 「リポジトリの登録：」 | 「OK」 | 「NG」 |
|---|---|---|---|
| 96（100%） | 87 px | 20 px | 21 px |
| 115（開発機） | 110 px | 27 px | 27 px |
| 120（125%） | 110 px | 27 px | 27 px |
| 144（150%） | 133 px | 32 px | 32 px |

参考（既存ラベル「対象フォルダ:」、115 DPIで実測83px、`.rc`定義幅45 DLU）: 83/45 ≈ 1.84 px/DLU。006・015で採用してきた保守的な変換係数 1.75 px/DLU（実際より小さい値）を安全側の基準として用いる。

**配置場所の空き**: 生成ボタン（`x=7, y=30, w=70, h=18`）の右側から、キャンセルボタンの右端が定める右余白（`x=563`）までの間に **486 DLU** の空きがある。015の列幅と異なり、余白を切り詰める必要は無い。

**当初のDecision（2026-09-17、計画時）**:

| コントロール | x, y, w, h (DLU) | 根拠 |
|---|---|---|
| `IDC_STATIC_REPO_LABEL`（「リポジトリの登録：」） | `90, 34, 90, 10` | 144 DPI必要幅133px+安全余裕(20+8)=161px → 92 DLU相当。90 DLUの幅は115 DPIで158px相当となり、144 DPIでの133px要求に対しても余裕を確保できるよう92→**90**に丸めたうえで実測を伴うST/DTで確認する（下記quickstart） |
| `IDC_RADIO_REPO_OK`（「OK」） | `184, 34, 40, 10` | ラジオボタンは文字幅に加えて丸印＋余白が必要なため、テキスト分(20+8)よりさらに広い40 DLU幅を確保 |
| `IDC_RADIO_REPO_NG`（「NG」） | `228, 34, 40, 10` | 同上 |

Generate ボタン（`y=30, h=18`）に対し、新規コントロールは `y=34, h=10` とし、行の上下中央に寄せる（`(18-10)/2=4`）。既存のリストビュー（`y=52`）との間に4 DLUの隙間が残るため、**ダイアログの高さ・幅は変更不要**。

**検算**: NG右端 = 228+40 = 268 DLU。ダイアログ内側の右余白（563 DLU）まで 295 DLU の余裕があり、96〜144 DPIのいずれでも同じ結論になる（このマージンはDPIに依存しないほど大きい）。

**Rationale**: 015のような限界設計ではなく、既存の空きスペースに収める設計であるため、実測はDLU値の決定根拠として使うに留め、厳密な境界計算（0px余裕の回避等）は不要と判断した。

---

### 2026-09-18改訂: 実機の目視により幅を詰めた

計画時のDecisionは「DPI安全マージンの確保」を優先し、006/015のListView列と同じ考え方（144 DPIで必要な最大幅に安全余裕20+8pxを足す）で幅を決めた。この結果、開発機の実DPI（115）ではラベル・ラジオボタンとも文字の実測幅よりボックス自体がかなり広くなり（例: ラベルは文字110pxに対し箱158px、48pxの余白が箱の内部だけで生じる）、実機で見るとラベルと「OK」の間に不自然な空白が生じた。

ユーザーからの実機フィードバック（2回）を受け、目視で許容できるレベルまで箱を詰めた。**最終的な採用値**:

| コントロール | x, y, w, h (DLU) | 変更 |
|---|---|---|
| `IDC_STATIC_REPO_LABEL` | `90, 34, 65, 10` | 幅 90→65 |
| `IDC_RADIO_REPO_OK` | `157, 34, 34, 10`（`WS_GROUP`） | x 184→157、幅 40→34 |
| `IDC_RADIO_REPO_NG` | `195, 34, 34, 10` | x 228→195、幅 40→34 |

**検算（96/115/120/144 DPIでの余裕、実測）**:

| DPI | ラベル必要幅 | ラベル箱(65 DLU) | ラベル余裕 | OK/NG必要幅(文字+glyph概算18px) | OK/NG箱(34 DLU) | OK/NG余裕 |
|---|---|---|---|---|---|---|
| 96 | 87px | 94.9px | +7.9px | 38px | 49.7px | +11.7px |
| 115（開発機） | 110px | 113.8px | +3.8px | 45px | 59.5px | +14.5px |
| 120 | 110px | 118.7px | +8.7px | 45px | 62.1px | +17.1px |
| 144 | 133px | 142.4px | +9.4px | 50px | 74.5px | +24.5px |

いずれのDPIでも余裕は正（最小+3.8px@115 DPI）であり、憲章 原則Vの「0px余裕を避ける」を満たす。ラジオボタンの「glyph+gap」オーバーヘッドは実測できないため18pxと見積もった（見積もりであることを明記。006 R-505と同じ考え方）。

**Rationale**: この行は015のListView列のような「厳密な境界での安全設計」を必要としない（周囲に486 DLUの空きがある）。当初の保守的な余白は、機能上は正しいが視覚的には過大だった。ユーザーの実機確認を優先し、視覚密度を詰める方向へ改めた。DPI保証範囲（100%〜150%）での正の余裕は維持している。

**Alternatives considered**:
- ラベル幅70・OK/NG幅34（1回目の調整、ユーザーが「まだ空きすぎ」と指摘） — 却下
- ラベルの箱をテキストぴったりに自動サイズする（`SS_LEFTNOWORDWRAP`等の動的サイズ） — 却下。既存の全コントロールが固定DLU座標で設計されており（004以来の座標系）、1コントロールだけ動的サイズにすると一貫性が崩れる。YAGNI（原則III）

## R-1605: アンカーモード

**調査対象**: `InitAnchors()`（`MFCApplication1Dlg.cpp` L649-662）。

生成ボタン（`IDC_BUTTON_GENERATE`）は `AnchorMode::Fixed`（リサイズで移動・拡縮しない）。新規コントロールは生成ボタンと同じ行に相対配置されるため、同じく `AnchorMode::Fixed` とする。

**Decision**: `IDC_STATIC_REPO_LABEL`・`IDC_RADIO_REPO_OK`・`IDC_RADIO_REPO_NG` はいずれも `AnchorMode::Fixed`。`InitAnchors()` のテーブルに3行追加する。

---

## R-1606: リソースIDの割り当て

**調査対象**: `Resource.h` のコメント（L16-19）。

```
// 1002 と 1003 は種別選択のラジオボタンだったが 002 FR-101 で廃止し、
// 1005 は診断ボタンだったが 003 FR-201 で廃止し、
// 定義も削除した (002 FR-107: 不要となったリソース定義を残さない)。
// 削除した ID は再利用しない。
```

**Decision**: 廃止済みID（1002・1003・1005）は再利用せず、既存の最大値1008の続きとして新規に採番する。

| 定数 | 値 |
|---|---|
| `IDC_STATIC_REPO_LABEL` | 1009 |
| `IDC_RADIO_REPO_OK` | 1010 |
| `IDC_RADIO_REPO_NG` | 1011 |

---

## R-1607: ラジオボタンのグループ化とMFCの標準機構

MFCのラジオボタンは、グループの先頭に `WS_GROUP` スタイルを付けることで相互排他になる（Windows標準ダイアログの挙動）。`IDC_RADIO_REPO_OK` に `WS_GROUP` を付け、直後に `IDC_RADIO_REPO_NG` を続ける。選択状態の読み取りは `IsDlgButtonChecked(IDC_RADIO_REPO_OK)`（1=OK選択、0=NG選択）で行う。初期選択（既定「OK」）は `OnInitDialog()` で `CheckRadioButton(IDC_RADIO_REPO_OK, IDC_RADIO_REPO_NG, IDC_RADIO_REPO_OK)` を1回呼ぶ。

**Decision**: 上記の標準的なMFCラジオボタン機構をそのまま用いる。新しい抽象化（例: 選択状態を表す専用クラス）は導入しない（YAGNI、憲章 原則III）。

---

## R-1608: 処理中の無効化対象（2026-09-17改訂: 追加しない）

**調査対象**: `SetControlsEnabled()`（`MFCApplication1Dlg.cpp` L399-415）。

既存は `IDC_BUTTON_GENERATE`・`IDC_BUTTON_BROWSE` の2つを配列で無効化している。

**当初の実装**: 計画時はラジオボタン2個も配列に加えて4コントロールを無効化する予定だった（FR-1608初版）。実装しビルド・実機確認した結果、機能上は問題なかった。

**実機確認後の修正**: ユーザーが実機で「NG」経路（`claude --print /init`呼び出しを含むため実測数十秒かかる）を通しで確認したところ、ラジオボタンが無効化されていることに気づかず、「グレーアウトは不要。仕様・実装に含まれているなら削除してほしい」との指示があった。

**Decision**: `SetControlsEnabled()` の対象からラジオボタン2個を除外し、既存の2コントロール（生成・参照ボタン）のみ無効化する（FR-1608、2026-09-17改訂）。

**Rationale**: `m_workerRegisterRepo` は「生成」押下の瞬間に一度だけ読み取ってワーカーへコピーする設計（R-1602）のため、処理中にラジオボタンの選択を変更しても**実行中の処理には一切影響しない**（次回の「生成」押下時に新しい選択が使われるだけ）。無効化は安全のための予防措置だったが、実害が無い操作を禁止する意味は薄く、ユーザーの明示的な指示により撤回した。

---

## R-1609: 完了案内（`IDC_STATIC_GUIDE`）は変更不要

**調査対象**: `ShowCompletionGuide()`（`MFCApplication1Dlg.cpp` L599-633）。

現在の案内文言（成功時の3行）はいずれもGit固有の内容を含まない（VS Codeを開く／`/speckit-constitution`／`/init`再実行）。「NG」選択時もこれらの案内はそのまま成立するため、本フィーチャーでは変更しない（spec.md Assumptionsのとおり）。

**Decision**: `ShowCompletionGuide()` は変更しない。

---

## 測定に使ったもの

一時的なPowerShellスクリプト（本セッション内で使用、リポジトリには追加しない。憲章の技術的制約による）。

- `TextRenderer.MeasureText` によるDPI相当フォントでの文字幅計測（R-1604）
