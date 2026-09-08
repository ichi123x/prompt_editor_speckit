<!--
Sync Impact Report
- Version change: (template) → 1.0.0
- Modified principles: N/A（プレースホルダーテンプレートからの初回制定）
- Added sections:
  - Core Principles: I. Spec Kit ワークフロー厳守 / II. 日本語での一貫性 / III. シンプルさ優先（YAGNI） / IV. 変更の追跡可能性
  - 技術的制約
  - 開発ワークフロー
  - Governance
- Removed sections: [PRINCIPLE_5_NAME]（テンプレートの5番目の原則スロットは本プロジェクトでは未使用のため削除）
- Deferred TODOs: なし
-->

# prompt_editor_speckit Constitution

## Core Principles（基本原則）

### I. Spec Kit ワークフロー厳守
機能開発は必ず `/speckit-constitution` → `/speckit-specify` → `/speckit-clarify` → `/speckit-plan`
→ `/speckit-tasks` → `/speckit-analyze` / `/speckit-checklist` → `/speckit-implement` の順序で進めなければ
ならない（MUST）。いずれかの段階を飛ばして直接コード実装やビルドに進んではならない。各段階の成果物
（spec.md、plan.md、tasks.md 等）は次段階が読み込む前提の情報源であり、勝手に省略・改変してはならない。
`.specify/extensions.yml` にフックが定義されている場合は、各スキルの Pre/Post-Execution Checks に従って
必ず確認しなければならない。
**理由**: 段階を飛ばすと仕様と実装が乖離し、後工程（tasks/implement）が前提とする情報が欠落する。

### II. 日本語での一貫性
リポジトリ内のドキュメント（CLAUDE.md、spec/plan/tasks 等の Spec Kit 成果物）、UI文字列、コメントは
日本語で統一しなければならない（MUST）。既存の `MFCApplication1` の日本語UI・日本語コメントの慣習を
新規追加コードでも踏襲すること。技術用語・識別子（変数名、関数名、コマンド名等）は英語のままでよいが、
説明文・コメント・ドキュメントは日本語を用いる。
**理由**: 既存コードベースとドキュメントが日本語で統一されており、混在は可読性と保守性を損なう。

### III. シンプルさ優先（YAGNI）
`MFCApplication1` はダイアログベースの小規模MFCデスクトップアプリケーションである。現時点で必要とされて
いない抽象化・共通化・将来を見越した拡張ポイントを導入してはならない（MUST NOT）。3行程度の重複コードは、
時期尚早な共通化より許容される。新しい依存関係やレイヤーを追加する場合は、既存の `tasks.md` / `plan.md`
に明記された要求に基づくものでなければならない。
**理由**: 小規模なデスクトップアプリの規模に対して過剰設計は保守コストを増大させるだけで便益がない。

### IV. 変更の追跡可能性
`MFCApplication1` に対するすべての機能追加・変更は、対応する `specs/<NNN>-<short-name>/spec.md` に遡れ
なければならない（MUST）。spec/plan/tasks を経由しないアドホックな変更は、緊急のバグ修正など明確な例外
理由がない限り行ってはならない。現時点でテストスイートは存在しないため、`tasks.md` と `spec.md` の対応
関係がその代替として変更の妥当性を担保する一次情報源となる。
**理由**: テストによる裏付けがない状態では、仕様との対応関係だけが変更意図を将来検証する手段になる。

## 技術的制約

- `MFCApplication1` は Visual Studio 専用のMFCプロジェクトであり、CMake/Makefile等の代替ビルドシステム
  を導入してはならない（MUST NOT）。ビルドは `msbuild MFCApplication1/MFCApplication1.sln
  -p:Configuration=<Debug|Release> -p:Platform=<x64|Win32>` を用いる。
- Spec Kit 側のスクリプトは PowerShell（`.specify/scripts/powershell/`）のみをサポートする。bash版スクリ
  プトを新規追加してはならない。Windows環境では `.ps1` の実行にBashツールではなくPowerShellツールを使用
  すること。
- 現状このリポジトリには自動テストスイートおよびlint設定が存在しない。導入する場合は、その必要性を対応
  する `spec.md`/`plan.md` に明記すること。

## 開発ワークフロー

- 新機能は必ず `/speckit-specify` によって作成された `specs/<NNN>-<short-name>/` 配下から開始する。
  機能ディレクトリを手動で作成してはならない。
- `checklists/*.md` は読み取り専用のゲートとして扱う。`/speckit-implement` はチェック未了の項目があれば
  進行前にユーザーへ確認しなければならず、チェックリスト自体を編集してはならない。
- `.specify/memory/constitution.md` の変更は本コマンド（`/speckit-constitution`）経由でのみ行う。他の
  Spec Kit コマンドの実行中に憲法ファイルを書き換えてはならない。

## Governance（ガバナンス）

本憲法はこのリポジトリにおける他のすべての開発慣行・ガイダンスに優先する。CLAUDE.md や各 `SKILL.md` の
記述が本憲法と矛盾する場合は、本憲法を優先し、矛盾箇所は発見次第修正しなければならない。

- **改訂手続き**: 憲法の変更は `/speckit-constitution` を通じてのみ行う。変更内容は必ずファイル冒頭の
  Sync Impact Report に記録する。
- **バージョニングポリシー**: セマンティックバージョニングに従う。
  - MAJOR: 既存原則の後方互換性のない削除・再定義。
  - MINOR: 新しい原則・セクションの追加、ガイダンスの実質的拡張。
  - PATCH: 文言の明確化・誤字修正・非意味論的な改善。
- **コンプライアンスレビュー**: `/speckit-plan` の憲法ゲートおよび `/speckit-analyze` の整合性チェックに
  おいて、本憲法の原則との矛盾がないかを確認しなければならない（MUST）。矛盾が見つかった場合は、実装を
  進める前に spec/plan を修正するか、本憲法の改訂を検討する。

**Version**: 1.0.0 | **Ratified**: 2026-09-06 | **Last Amended**: 2026-09-06
