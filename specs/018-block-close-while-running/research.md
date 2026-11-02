# Research: 導入処理の実行中はダイアログを閉じられないようにする

**Feature**: 018-block-close-while-running
**Date**: 2026-11-02

Technical Context に NEEDS CLARIFICATION は無い。以下は設計判断の記録である。

---

## R-1801: 閉じる操作をどこで止めるか

**Decision**: `CMFCApplication1Dlg` の `OnOK()` と `OnCancel()`（`CDialog` の仮想関数）をオーバーライドし、
`m_running` が `true` のときは案内を表示して基底クラスを呼ばずに戻る。コミット `1d49765` の実験用修正の
構造をそのまま採用する。

**Rationale**: モーダルダイアログを閉じる操作は、最終的にすべてこの2つの仮想関数に集まる。

| 利用者の操作 | Windows / MFC 内部の経路 | 到達する関数 |
|---|---|---|
| 「OK」ボタン | `WM_COMMAND(IDOK)` | `OnOK()` |
| Enter キー | ダイアログマネージャが既定ボタン（`DEFPUSHBUTTON` = `IDOK`）の `WM_COMMAND(IDOK)` を送る | `OnOK()` |
| 「キャンセル」ボタン | `WM_COMMAND(IDCANCEL)` | `OnCancel()` |
| Esc キー | ダイアログマネージャが `WM_COMMAND(IDCANCEL)` を送る | `OnCancel()` |
| タイトルバーの ✖ / Alt+F4 / システムメニューの「閉じる」/ タスクバーの「ウィンドウを閉じる」 | `WM_SYSCOMMAND(SC_CLOSE)` → `WM_CLOSE` → `DefDlgProc` が `WM_COMMAND(IDCANCEL)` を送る | `OnCancel()` |

`OnSysCommand`（[MFCApplication1Dlg.cpp:208](../../MFCApplication1/MFCApplication1Dlg.cpp#L208)）は
`IDM_ABOUTBOX` 以外を `CDialogEx::OnSysCommand` へ渡しており、`SC_CLOSE` を横取りしていない。また
メッセージマップに `ON_WM_CLOSE` は無い。したがって上表の経路は既存コードで遮られておらず、2関数の
オーバーライドだけで FR-1801 の全操作を網羅できる。利用者は実験用修正で OK・キャンセル・✖ の3操作を
実機確認済みである。

**Alternatives considered**:
- **`OnClose`（`WM_CLOSE`）を処理する**: ✖ 系の操作しか止められず、OK・キャンセル・Enter・Esc には別途
  対応が要る。2関数で済むものを3か所に分散させるため却下。
- **実行中に「OK」「キャンセル」ボタンを無効化し、システムメニューの「閉じる」も灰色にする**: ✖ は
  `EnableMenuItem(SC_CLOSE)` で無効化できるが、Enter・Esc は無効化したボタンとは別経路で `IDOK` /
  `IDCANCEL` を送るため、結局 `OnOK` / `OnCancel` での防止が必要になる。無効化と防止の二重管理になり、
  処理完了時に戻し忘れる危険も増えるため却下（spec Assumptions、憲章 原則III）。

---

## R-1802: 案内の表示方法と文言

**Decision**: `AfxMessageBox(_T("導入処理の実行中は閉じられません。完了までお待ちください。"),
MB_OK | MB_ICONINFORMATION)` を表示する。実験用修正と同じ文言・アイコンを使う。`OnOK` と `OnCancel` に
同じ3行（判定・表示・return）が重複するが、共通関数にはまとめない。

**Rationale**: 利用者が実験用修正で表示を確認済みである（spec Assumptions）。メッセージボックスは
モーダルだが、その間も UI スレッドのメッセージループは回り続けるため、ワーカーからの
`WM_APP_STEP_FINISHED` / `WM_APP_SETUP_COMPLETED` は処理され、結果一覧の更新と完了処理は止まらない
（spec Edge Case「案内を表示している間に導入処理が完了した場合」）。重複は3行であり、憲章 原則III が
「時期尚早な共通化より許容される」とする規模に収まる。

**Alternatives considered**:
- **案内を出さずに黙って無視する**: 利用者は閉じられない理由が分からず、アプリが固まったと誤解しうる。
  FR-1802 に反するため却下。
- **「中止しますか？」と確認して処理を取り消す**: 外部コマンドを安全に途中終了させる設計が別途必要で、
  spec の対象外（Assumptions）。却下。

---

## R-1803: 「ワーカーの終了を確認してから閉じる」（憲章 T-2）をどう満たすか

**Decision**: 既存の `m_running` フラグをそのまま判定に使う。`m_running` は「生成」で `true` になり
（[MFCApplication1Dlg.cpp:574](../../MFCApplication1/MFCApplication1Dlg.cpp#L574)）、ワーカーが最後に
送る `WM_APP_SETUP_COMPLETED` を UI スレッドが受け取ったとき `false` に戻る
（[同:621](../../MFCApplication1/MFCApplication1Dlg.cpp#L621)）。新しい状態は追加しない。

**Rationale**: ワーカー（`SetupThreadProc`）は `WM_APP_SETUP_COMPLETED` を送った直後に `return 0;` する
だけで、それ以降ダイアログ（`this`）に触れない。そのため「完了通知を受け取った」ことは「ワーカーが
ダイアログを使い終えた」ことと同じ意味になり、T-2 の「ワーカースレッドの処理の終了を確認してから」を
満たす。スレッドの OS 上の終了（`return` 後の後始末）まで待つ必要は無い。

`m_running` は UI スレッドだけが読み書きする（設定は `OnBnClickedButtonGenerate` と
`OnSetupCompletedMessage`、判定は `OnBnClickedButtonGenerate` と今回の `OnOK` / `OnCancel`。すべて UI
スレッドで動く）。2つのスレッドが同じ変数を触る状況ではないため、T-3 の `std::atomic` 化は不要。

**Alternatives considered**:
- **スレッドのハンドルを保持し、閉じる前に `WaitForSingleObject` で待つ**: 完了通知の時点でワーカーは
  既にダイアログを使い終えており、待つ意味が無い。待っている間 UI スレッドが止まる危険も生じるため却下
  （原則III「既存の仕組みの意味論を読み解いて転用する」）。
- **`m_running` を `std::atomic<bool>` にする**: 上記のとおり UI スレッドからしか触らないため不要。却下。

---

## R-1804: 手順の結果をダイアログへ渡す箇所の所有権（FR-1806・FR-1807、憲章 O-1・O-3）

**Decision**: 憲章 O-3 の手順どおりに書き換える。

- 送る側（`OnStepFinished`、ワーカースレッド）: `std::make_unique<SetupStep>(step)` で作り、
  `PostMessage` が成功した（戻り値が非 0）ときだけ `release()` する。失敗したときは何もせず、
  `unique_ptr` のスコープ終了で自動的に解放させる。
- 受ける側（`OnStepFinishedMessage`、UI スレッド）: `lParam` を関数の先頭で直ちに
  `std::unique_ptr<SetupStep>` で受け取り直す。`delete` は書かない。
- `MFCApplication1Dlg.cpp` に `#include <memory>` を明示的に追加する。

**Rationale**: 現在のコード（[MFCApplication1Dlg.cpp:601](../../MFCApplication1/MFCApplication1Dlg.cpp#L601)、
[605-614](../../MFCApplication1/MFCApplication1Dlg.cpp#L605-L614)）は `new` の結果を `PostMessage` へ直接
渡しており、送信に失敗すると誰も解放しない。送信が失敗する代表的な場面は送り先のウィンドウが破棄済みの
ときで、018 以前は「実行中にダイアログを閉じた」ときに実際に起こり得た。018 の適用後はこの場面が
起こらなくなるが、O-1・O-3 は送信失敗の頻度に関わらず適用されるルールである。

`release()` で `unique_ptr` の手を離れた時点から、受け取り側が `unique_ptr` で受け取り直すまでの間は、
O-1 の例外「O-3 のパターンで、PostMessage を通して所有権を渡すとき」に該当する。O-1 に従い、この箇所に
「誰が所有者か」をコメントで明記する。

`std::make_unique` は C++14 から使える。本プロジェクトは言語標準を指定しておらず MSVC の既定（C++14）で
ビルドされるため、そのまま使える。`<memory>` は現状 MFC ヘッダー経由で間接的に読み込まれている可能性が
あるが、どこから来た機能かを明確にするため明示的に include する。`MFCApplication1Dlg.cpp` は
`#define new DEBUG_NEW` を include の**後**で定義しているため、`<memory>` 内部の `new` には影響しない。

**Alternatives considered**:
- **`SetupStep` を値でコピーしてキューに積み、UI スレッドが取り出す**: 共有データとなり T-3 の
  `std::mutex` が必要になる。T-3 は「大きいデータは優先して共有しない設計（PostMessage で渡すなど）」を
  勧めており、現在の PostMessage 方式を維持するほうが合致する。却下。
- **`std::shared_ptr` を使う**: 所有者は常に1人（送る側 → 受ける側へ移る）であり、O-1 の「共有の所有者が
  必要な場合だけ」に該当しない。却下。

---

## R-1805: 実験用の目印の除去と CLAUDE.md の更新範囲（FR-1805、spec Assumptions）

**Decision**:
- `MFCApplication1Dlg.h` の `#define T_CHECK_BUG_01` とその説明コメント（「不具合を確認するための定義」）、
  および `.h` / `.cpp` の `#ifdef T_CHECK_BUG_01` / `#endif` を削除する。中身（`OnOK` / `OnCancel` の
  宣言と定義）は残す。
- 「OK ボタン押下時の動作を確認するため」等の実験用コメントは、FR 番号を含む正式なコメントへ置き換える。
- ソースコード中に `T_CHECK_BUG_01` という文字列を、コメントも含めて1つも残さない（ST-1803 で検査する
  ため。CLAUDE.md の「検査がコメントを拾って不合格になる」問題を避けるため、経緯の説明は spec.md にのみ
  書き、コードには書かない）。
- CLAUDE.md は次の3か所を更新する。
  1. 「既存機能」の表に 018 の行を追加する。
  2. 「憲章」節の「（v1.0.0）」「4つの原則」の記述を、v1.5.0 の内容（原則 I〜V と「所有権・スレッドモデル」節）
     に合わせる。
  3. 「設計上、壊しやすい箇所」に、実行中は閉じられないこと（`OnOK` / `OnCancel` の判定を外すと T-2 違反に
     戻ること）を1項目追加する。

**Rationale**: 憲章 Governance「CLAUDE.md の記述が本憲法と矛盾する場合は … 発見次第修正しなければ
ならない」に従う。3 は、複数ファイルを読まないと分からない不変条件（閉じる操作と `m_running` の関係）で
あり、CLAUDE.md のこの節の趣旨に合う。

**Alternatives considered**:
- **`#ifdef` を残して既定で有効にする**: FR-1805 に反する。却下。
