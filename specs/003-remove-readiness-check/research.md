# Phase 0 リサーチ: 準備状況の診断機能の廃止

**Feature**: 003-remove-readiness-check
**Date**: 2026-09-08
**前提**: [001 の research.md](../001-sdd-project-bootstrap/research.md) R-001〜R-009 と [002 の research.md](../002-remove-kind-selection/research.md) R-101〜R-104 は引き続き有効。本書はその差分のみを扱う。

本フィーチャーは既存実装からの削除であり、新しい外部依存も未知の技術要素も無い。調査項目は「何をどこまで消すか」と「消したあとに孤立するものは何か」の2点である。

---

## R-201: 削除範囲

**Decision**: 診断に関わる要素を**完全に削除**する。内部的にも残さない。

現在の参照箇所を実コードから洗い出した結果は次のとおり。

| ファイル | 削除する要素 |
|---|---|
| `ReadinessChecker.h` / `.cpp` | **ファイルごと削除**（合計 約430行）。`ReadinessVerdict` 列挙、`ReadinessItem` 構造、`Check()`、`VerdictDisplayName()` |
| `Resource.h` | `IDC_BUTTON_DIAGNOSE`（1005）の定義 |
| `MFCApplication1.rc` | 診断ボタンのコントロール定義 |
| `MFCApplication1Dlg.h` | `#include "ReadinessChecker.h"`、`OnBnClickedButtonDiagnose()` 宣言、`ShowReadinessGuide()` 宣言 |
| `MFCApplication1Dlg.cpp` | メッセージマップ1行、無効化リストの1要素、`OnBnClickedButtonDiagnose()` 実装、`ShowReadinessGuide()` 実装（約50行） |
| `MFCApplication1.vcxproj` / `.filters` | `ReadinessChecker.h` / `.cpp` の登録 |

**Rationale**: 仕様 FR-204 が「診断のためだけに存在していたコード・ファイル・メッセージ文字列を残してはならない」と定めている。列挙や構造だけを残しても、後から読んだときに「どこで使われるのか」を探す無駄が生じる。

**削除しないもの**: `ShowCompletionGuide()` と完了案内の文字列（FR-205）。この関数は `OnSetupCompletedMessage()` から呼ばれ、診断結果を参照しない。`ShowReadinessGuide()` と名前が似ているため、削除時に取り違えないこと。

---

## R-202: 削除によって孤立する要素

**実測で1件見つかった。**

`TargetFolderState::hasGitignore` は、現在次の3箇所にしか現れない。

| 箇所 | 種別 |
|---|---|
| `TargetFolderState.h:30` | フィールド宣言 |
| `TargetFolderState.cpp:96` | **書き込み**（`.gitignore` の存在を判定して代入） |
| `ReadinessChecker.cpp:234, 242` | **読み出し**（項目2の判定と、不足しているものの列挙） |

診断を削除すると読み出し側が消え、**書き込みのみで誰も読まないフィールド**になる。

**Decision**: `hasGitignore` フィールドと、`TargetFolderState::Read()` 内の代入処理を削除する。

**Rationale**: FR-204 の「診断以外から参照されない処理」に該当する。憲章の原則III（YAGNI）も「現時点で必要とされていない拡張ポイントを導入してはならない」と定めており、将来使うかもしれないという理由で残すのは反する。

**混同しやすい点**: `GitignoreWriter::Ensure()` は `.gitignore` の存在を**自分で `GetFileAttributes` を呼んで**判定しており、`TargetFolderState::hasGitignore` を使っていない。したがってこのフィールドを消しても `.gitignore` の生成・追記には影響しない。

**孤立しないもの**（誤って消さないこと）:

| 要素 | 診断以外の読み出し元 |
|---|---|
| `hasSpecifyDir` / `hasClaudeDir` | `SetupRunner.cpp:159, 186`（手順4のスキップ判定と配置確認）、`HasPartialScaffold()` |
| `HasPartialScaffold()` | `MFCApplication1Dlg.cpp:317`（検証チェーン4段目、足場が中途半端なときの警告） |
| `commitCount` | `SetupRunner.cpp:210, 250`（手順5のスキップ判定と、未コミット変更の検出条件） |
| `gitState` / `gitRoot` | `SetupRunner.cpp`、検証チェーン5段目（入れ子リポジトリの警告） |
| `hasProjectMarker` | 検証チェーン3段目 |

`TargetFolderState` の構造そのものは `hasGitignore` を除いて変更しない（仕様 Key Entities）。

---

## R-203: ダイアログのレイアウト

**Decision**: 診断ボタンを削除し、**残るコントロールの位置は変更しない**。

**変更前**:

```
y=7    対象フォルダ: [____________] [参照...]
y=30   [生成]  [準備状況を診断]
y=52   ┌ 結果一覧 (90 DLU) ─────────────────┐
y=147  案内欄（読み取り専用エディット）
y=179  [OK] [キャンセル]
```

**変更後**:

```
y=7    対象フォルダ: [____________] [参照...]
y=30   [生成]
y=52   ┌ 結果一覧 (90 DLU) ─────────────────┐
y=147  案内欄（読み取り専用エディット）
y=179  [OK] [キャンセル]
```

**Rationale**:
- 「生成」ボタンは x=7 にあり、上のパス欄・下の一覧と左端が揃っている。診断ボタンを消しても左揃えの構造は保たれる。**操作ボタンが1つだけの行は不自然ではない**。
- 縦方向に空きは生じない。診断ボタンは「生成」と同じ行にあったため、行そのものは残る。
- 002 でボタンを左詰めにしておいたことが効いている。もし右寄せのままだったら、削除後に左半分が空いて不自然になっていた。

**Alternatives considered**:
- *「生成」ボタンを横に伸ばして空きを埋める*: 押しやすくはなるが、ボタン幅が他と揃わなくなる。また幅の広いボタンは「主要な操作」を強調する意図に見え、実際そうであるとはいえ、001・002 で確立したレイアウトを必要以上に動かすことになる。却下。
- *「生成」ボタンを中央や右へ寄せる*: 左端の揃いが崩れる。却下。

**タブ順序（FR-208）**: `.rc` 内の記述順がタブ順序を決める。変更後は次のとおり。

```
ラベル → IDC_EDIT_TARGET_PATH → IDC_BUTTON_BROWSE
      → IDC_BUTTON_GENERATE → IDC_LIST_RESULT → IDOK → IDCANCEL
```

案内欄は 002 で `NOT WS_TABSTOP` を指定済みのため、元から巡回に含まれない。

---

## R-204: 回帰を防ぐうえで注意する箇所

削除作業で壊しやすい箇所を、実コードを見て特定した。

**(1) 実行中に無効化するコントロールの配列**（`MFCApplication1Dlg.cpp:243-246`）

```cpp
CWnd* controls[] = {
    GetDlgItem(IDC_BUTTON_GENERATE),    // ← 残す
    GetDlgItem(IDC_BUTTON_DIAGNOSE),    // ← 削除
    GetDlgItem(IDC_BUTTON_BROWSE)       // ← 残す
};
```

削除するのは**真ん中の1要素のみ**。前後を巻き添えにしないこと。生成ボタンの無効化は二重実行防止（001 FR-014）、参照ボタンの無効化は 001 の analyze で HIGH として指摘し追加した項目である。002 の DT-107 と同じ構図の誤りであり、DT-207 がこれを検出する。

**(2) `ShowCompletionGuide()` と `ShowReadinessGuide()` の取り違え**

名前が似ており、宣言も隣接している。削除するのは後者だけである。

| 関数 | 呼び出し元 | 扱い |
|---|---|---|
| `ShowCompletionGuide()` | `OnSetupCompletedMessage()` | **残す**（FR-205） |
| `ShowReadinessGuide()` | `OnBnClickedButtonDiagnose()` | 削除 |

前者を消すと、SDD化の後に次の手順が一切案内されなくなる。002 で読みやすさとコピー可能性を確保したばかりの機能であり、DT-205 がこれを検出する。

**(3) 結果一覧の共用をやめるだけで、表示処理は変えない**

`AppendResultRow()` と `ClearResultList()` は手順結果と診断結果の双方が使っていた（001 research.md R-009 の設計判断）。診断が無くなっても、手順結果からの呼び出しは残る。これらの関数を消さないこと。

**(4) `<vector>` インクルードの扱い**

`MFCApplication1Dlg.h` は `ReadinessChecker.h` を通じて `std::vector` を使っていたが、`m_steps`（`std::vector<SetupStep>`）でも使う。`#include <vector>` は残す。

---

## 未解決の項目

なし。
