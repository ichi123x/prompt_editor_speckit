# Quickstart: リポジトリ登録の要否を選択できるようにする

**Feature**: 016-repo-registration-toggle
**Date**: 2026-09-17

## 前提条件

- ビルド済みの `MFCApplication1.exe`（測定前に必ず `msbuild /t:Rebuild` を実行する）
- 空のテスト用フォルダを2つ用意する（「OK」用・「NG」用で対象を分ける。同一フォルダを使い回すと`CLAUDE.md`の存在チェック等で2回目以降の実行結果が変わる）

## ST-1601: `.rc`定義と列幅への影響の静的確認

1. `MFCApplication1.rc` を開き、`IDC_STATIC_REPO_LABEL`・`IDC_RADIO_REPO_OK`・`IDC_RADIO_REPO_NG` が[data-model.md](./data-model.md)の座標・スタイルで定義されていることを確認する
2. ダイアログ本体の寸法（570 x 233 DLU）が015から変わっていないことを確認する
3. `Resource.h` で3つの新規IDが1009・1010・1011（廃止済みの1002・1003・1005を再利用していない）ことを確認する
4. `MFCApplication1Dlg.cpp` の `InitAnchors()` に3行追加され、いずれも `AnchorMode::Fixed` であることを確認する
5. `SetupRunner.h`・`SetupRunner.cpp` のシグネチャが[data-model.md](./data-model.md)「3. 変更する構造体・関数シグネチャ」の通りであることを確認する
6. FR番号のコメント（FR-1601〜FR-1609）が該当箇所に付いていることを確認する（憲章 原則IV）

## ST-1602: コード分析（既存の静的解析をそのまま実行）

```
msbuild MFCApplication1/MFCApplication1.vcxproj -p:Configuration=Debug -p:Platform=x64 -t:Rebuild -p:RunCodeAnalysis=true -p:EnablePREfast=true
```

**期待結果**: 警告・指摘ゼロ。

## ST-1603: ビルド4構成の確認

CLAUDE.mdに記載の4構成ビルドスクリプトを実行し、警告・エラーがゼロであることを確認する。

## DT-1601: 既定値の確認（FR-1601）

1. アプリを起動する
2. `IsDlgButtonChecked(IDC_RADIO_REPO_OK)` を`GetDlgItem`/`SendMessage`（`BM_GETCHECK`）で取得し、`BST_CHECKED` であることを確認する（値渡しメッセージなのでクロスプロセスでも安全）

## DT-1602: 「OK」（既定）での回帰確認（FR-1602、SC-1601）

1. 何も操作せず、空のテスト用フォルダA（未初期化）を対象に「生成」を実行する
2. 完了後、結果一覧の「Git 初期化と初回コミット」「CLAUDE.md の生成とコミット」がいずれも「成功」で表示されることを確認する
3. フォルダAに `.git` ディレクトリと `CLAUDE.md` が存在し、`git log` で2件のコミット（Spec Kit 導入・CLAUDE.md を追加）があることを確認する
4. 001〜015の既存の動的テスト手順（リサイズ追従・タイトルバー色等）が変わらず成立することを確認する（SC-1601相当の回帰確認）

## DT-1603: 「NG」選択時の確認（FR-1603・FR-1604・FR-1605・FR-1606、SC-1602・SC-1603）

1. アプリを起動し、`IDC_RADIO_REPO_NG` を選択する（`BM_CLICK`をポストするか、`CheckRadioButton`相当の状態にする）
2. 空のテスト用フォルダB（未初期化）を対象に「生成」を実行する
3. 完了後、結果一覧を確認する：
   - 「uv の導入」「specify CLI の導入」「VS Code 拡張の導入」「Spec Kit 足場の配置」の4行: 「NG」選択の影響を受けず、既存基準（成功またはスキップ）のまま実行されていることを確認する（FR-1607、spec.md US1 AC3）
   - 「Git 初期化と初回コミット」の行: 「結果」列が「スキップ」、「内容」列に「NG」選択に起因する旨の文言が表示される
   - 「CLAUDE.md の生成」の行（項目名が「CLAUDE.md の生成とコミット」ではなく「CLAUDE.md の生成」になっていること自体を確認する）: 「結果」列が「成功」、「内容」列が「CLAUDE.md を生成しました。」（コミット済みの旨を含まない）
4. フォルダBを確認する：
   - `.git` ディレクトリが**存在しない**こと
   - `CLAUDE.md` が**存在する**こと

## DT-1604: 処理中もラジオボタンが操作可能なままであることの確認（FR-1608、2026-09-17改訂）

1. 「生成」を押した直後（処理完了前）に、`IDC_RADIO_REPO_OK`・`IDC_RADIO_REPO_NG` の `IsWindowEnabled` を確認し、いずれも `TRUE`（無効化されていない）であることを確認する。生成・参照ボタンは従来通り `FALSE` になる

## DT-1605: 列幅への影響が無いことの確認（FR-1609、015の列margin維持）

1. フォルダA（DT-1602）・フォルダB（DT-1603）それぞれの完了後、`LVM_GETCOLUMNWIDTH` で3列の実際の幅を取得する
2. 015の[research.md](../015-support-high-dpi/research.md) R-1503が示す最小余裕（96 DPIで+6px等）と比較し、悪化していないことを確認する。新規追加された文字列（「CLAUDE.md の生成」等）は既存の最長文字列より短いため、理論上は列幅計算に影響しないはずだが、実測で裏付ける

## DT-1606: 既存機能の非破壊確認（FR-1609、回帰確認）

001〜015のquickstart.mdに定義された既存の動作確認手順を一通り再実施し、いずれも崩れていないことを確認する。特に:

- リサイズ時の8方向の追従が引き続き機能すること（004）。新規3コントロールも `Fixed` としてウィンドウ左上隅からの相対位置を保つこと
- タイトルバーの色が変わらないこと（005）
- 100%〜150%のDPIスケールで列margin・ウィンドウ幅（1024px制約）が015のまま成立すること（015）
