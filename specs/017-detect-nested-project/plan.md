# Implementation Plan: サブフォルダ内のウィザードプロジェクトを検出できるようにする

**Branch**: `017-detect-nested-project` | **Date**: 2026-09-21 | **Spec**: [spec.md](./spec.md)

**Input**: `/specs/017-detect-nested-project/spec.md` からの機能仕様

## Summary（概要）

現在の`TargetFolder::Read()`（[TargetFolderState.cpp](../../MFCApplication1/TargetFolderState.cpp)）は、プロジェクトマーカー（`*.sln`・`*.vcxproj`・`*.csproj`・`package.json`）の存在確認を対象フォルダの**直下**にしか行っていない。Visual Studio のプロジェクトウィザードは、選んだ場所の直下ではなく「ソリューション名と同名のサブフォルダ」を新規作成してその中にプロジェクト一式を配置することがあり（本リポジトリ自身の`MFCApplication1/`フォルダがまさにこの構造）、利用者がその一つ上の親フォルダを対象フォルダとして選ぶと、現状ではマーカーが見つからず「対象のプロジェクトが見つかりません」で誤って中止される。

技術的アプローチは、`HasFileMatching`によるマーカー確認（[TargetFolderState.cpp:88-91](../../MFCApplication1/TargetFolderState.cpp#L88-L91)）を、対象フォルダ自身に加えてその配下のサブフォルダも探索する再帰版に置き換える。既存の`HasFileMatching`（フォルダ直下のみを見る関数）はそのまま残し、新たに非公開の再帰ヘルパー関数を1つ追加して、各階層で`HasFileMatching`を呼び出しながらサブフォルダへ降りていく。`.git`フォルダの内部だけは明示的に除外する（FR-1703）。マーカーが実際に見つかった場所に関わらず、`state.path`（対象フォルダそのもの）は変更しない（FR-1704） — これにより`.gitignore`配置・Git初期化・Spec Kit足場配置などの後続処理は今まで通り対象フォルダに対して行われる。

## Technical Context（技術コンテキスト）

**Language/Version**: C++（MSVC v143、Unicode、既定の言語標準）

**Primary Dependencies**: MFCの`CFileFind`（既存の`HasFileMatching`が既に使用中で、追加の依存関係やライブラリ設定は不要）

**Storage**: N/A（本フィーチャーは対象フォルダの読み取り専用スキャンのみを行い、何も永続化しない）

**Testing**: 自動テストスイートは存在しない（憲章の技術的制約）。静的テスト ST-1701〜、動的テスト DT-1701〜 を [quickstart.md](./quickstart.md) の手順として定義する

**Target Platform**: Windows 10/11 デスクトップ

**Project Type**: desktop-app（MFC ダイアログアプリケーション）

**Performance Goals**: 生成ボタン押下から結果一覧の表示開始までの体感の遅延を増やさない。通常の開発プロジェクト規模（数百〜数千ファイル）のフォルダで、マーカー探索が数秒以上の待たされ感を生まないこと（SC-1703）

**Constraints**: 対象フォルダを実際のマーカー位置へすり替えない（FR-1704、検証チェーンの後続処理は既存のまま）。`.git`内部を探索から除外する（FR-1703）。新しいクラス・新しいファイルは追加せず、`TargetFolderState.cpp`内部の変更のみで完結させる（原則III YAGNI）。既存の`HasFileMatching`のシグネチャ・挙動（直下のみを見る）は変更しない — 他の呼び出し箇所（`.specify`・`.claude`ディレクトリの直下確認など）に影響を与えないため

**Scale/Scope**: 変更するファイルは`TargetFolderState.cpp`の1ファイルのみ。追加は非公開の再帰ヘルパー関数1つと、`Read()`内のマーカー確認4行の置き換え

## Constitution Check（憲法チェック）

*ゲート: Phase 0 のリサーチ前に通過する必要がある。Phase 1 の設計後に再チェックすること。*

| 原則 | 評価 |
|---|---|
| **I. Spec Kit ワークフロー厳守** | 適合。`/speckit-specify`（NEEDS CLARIFICATIONなしで完了）→`/speckit-plan`の順で実施中 |
| **II. 日本語での一貫性** | 適合。本フィーチャーは新規UI文字列を追加しない。既存の「対象のプロジェクトが見つかりません」メッセージも変更しない（マーカーが見つかった場合の挙動のみを変える）。追加コードのコメントは日本語 |
| **III. シンプルさ優先（YAGNI）** | 適合。新しいクラス・ヘッダ・公開インターフェースは追加しない。既存の`HasFileMatching`（直下のみ）を転用しつつ、再帰ヘルパー関数1つを`TargetFolderState.cpp`内部（無名namespace）に追加するのみ。除外対象は`.git`のみに限定し、`node_modules`等の追加除外は現時点で見送る（[research.md](./research.md) R-1701のAlternatives consideredで検討・却下） |
| **IV. 変更の追跡可能性** | 適合。`TargetFolderState.cpp`の追加・変更箇所にFR-1701〜FR-1705のコメントを付与する（tasks.mdで具体化） |
| **V. 検証は実測に基づく** | 適合。「配下」の探索範囲拡大が既存の空フォルダ判定に回帰を起こさないことは、実機ビルドと実際のテストフォルダ（`test01`にサブフォルダ構成を作る／`empty01`）で実測する（quickstart.md DT-1701〜）。一方、SC-1703（大規模フォルダでの体感速度）は実測での検証手順を設けず、[research.md](./research.md) R-1704の見積もり（早期リターン＋`.git`除外で十分という判断）に基づく受容とし、その旨をquickstart.mdに明記する（原則Vの「測れないものは安全側に見積もり、見積もりであることを明記する」を適用） |

**Phase 1設計後の再評価**: 上表の判定に変更なし。`data-model.md`・`quickstart.md`の作成により具体化されたのみで、新たな憲法上の懸念は生じていない。本フィーチャーはユーザーや他システムに公開する外部インターフェースを持たない内部ロジックの変更のみのため、`contracts/`は作成しない。

## Project Structure（プロジェクト構造）

### Documentation（このフィーチャー）

```text
specs/017-detect-nested-project/
├── plan.md              # このファイル（/speckit-plan コマンドの出力）
├── research.md          # Phase 0 の出力（/speckit-plan コマンド）
├── data-model.md         # Phase 1 の出力（/speckit-plan コマンド）
├── quickstart.md         # Phase 1 の出力（/speckit-plan コマンド）
├── checklists/
│   └── requirements.md  # /speckit-specify の出力（全項目合格）
└── tasks.md              # Phase 2 の出力（/speckit-tasks コマンド - /speckit-plan では作成されない）
```

### Source Code（リポジトリのルート）

```text
MFCApplication1/
├── TargetFolderState.cpp   # 変更対象: マーカー確認をサブフォルダ探索対応の
│                           #           再帰ヘルパーへ置き換える (FR-1701〜FR-1705)
├── TargetFolderState.h     # 変更しない（TargetFolderState構造体・公開関数シグネチャは維持）
├── MFCApplication1Dlg.*    # 変更しない（検証チェーンの順序・メッセージは既存のまま）
├── SetupRunner.*           # 変更しない
├── ProcessRunner.*         # 変更しない
└── GitignoreWriter.*       # 変更しない
```

**Structure Decision**: 既存の単一MFCプロジェクト構成（`MFCApplication1/`）をそのまま使用する。新規クラス・新規ファイルは追加しない。マーカー検出は`TargetFolderState`という既存の「対象フォルダから読み取った事実を表す唯一の事実源」の内部実装のみを変更し、`TargetFolderState`を参照する側（`MFCApplication1Dlg`の検証チェーン、`SetupRunner`）には一切手を入れない。

## Complexity Tracking（複雑さの追跡）

該当なし。Constitution Checkで違反は検出されなかった。
