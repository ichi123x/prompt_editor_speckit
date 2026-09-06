# CLAUDE.md

このファイルは、このリポジトリでコードを扱う際に Claude Code (claude.ai/code) が従うガイダンスを提供します。

## リポジトリ構成

このリポジトリには独立した2つの部分があります:

- **`MFCApplication1/`** — Visual Studio の MFC (Microsoft Foundation Class) デスクトップダイアログアプリケーション。現状はプロジェクトウィザードが生成したデフォルトの雛形のまま（`MFCApplication1Dlg.cpp` / `MFCApplication1.cpp` のロジックはすべて `// TODO:` のプレースホルダー）。コメントとUI文字列は日本語。
- **`.specify/` + `.claude/skills/`** — このリポジトリでの機能開発をスラッシュコマンドで進める Spec Kit（仕様駆動開発）一式。`MFCApplication1`（またはこのリポジトリに追加される他のもの）へ機能を追加する際の主要なワークフローとして扱うこと。

## ビルド

MFC プロジェクトは Visual Studio 専用（CMake/Makefile なし）:

```
msbuild MFCApplication1/MFCApplication1.sln -p:Configuration=Debug -p:Platform=x64
```

有効な `Configuration|Platform` の組み合わせ: `Debug|x64`、`Debug|Win32`、`Release|x64`、`Release|Win32`（`.sln` は `x86` ラベルを vcxproj の `Win32` プラットフォームにマッピングしている）。このリポジトリには既存のテストスイートや lint 設定はない。

## Spec Kit ワークフロー

機能開発は `.claude/skills/speckit-*` 配下のスキルによって進められ、以下の厳密な順序でスラッシュコマンドとして呼び出す:

```
/speckit-constitution → /speckit-specify → /speckit-clarify → /speckit-plan
  → /speckit-tasks → /speckit-analyze / /speckit-checklist → /speckit-implement
```

主要な仕組み（すべて個別の `SKILL.md` に定義されている — このワークフローを変更する前に該当ファイルを読むこと）:

- **`/speckit-specify`** はテンプレート（`.specify/templates/spec-template.md`）から `specs/<NNN>-<short-name>/spec.md` を作成し、アクティブな機能ディレクトリを `.specify/feature.json` に記録する。採番は連番（`init-options.json` → `feature_numbering`）。機能ディレクトリを作成するのはここだけ。
- **`/speckit-plan`** は `.specify/scripts/powershell/setup-plan.ps1 -Json` を実行し、spec と `.specify/memory/constitution.md` を読み込み、機能ディレクトリ内に `research.md`、`data-model.md`、`contracts/`、`quickstart.md` を生成する。
- **`/speckit-tasks`** は `.specify/scripts/powershell/setup-tasks.ps1 -Json` を実行し、`tasks.md` をユーザーストーリー単位（レイヤー単位ではない）で生成する。厳密なチェックリスト行形式（`- [ ] T001 [P] [US1] Description with file path`）が必要で、これは `/speckit-implement` がパースするために必須。
- **`/speckit-implement`** は `.specify/scripts/powershell/check-prerequisites.ps1 -Json -RequireTasks -IncludeTasks` を実行し、`tasks.md` をフェーズごとに実行して、完了したタスクを `[X]` としてマークする。`checklists/*.md` は読み取り専用のゲートとして扱い（未チェック項目があれば進める前に確認する）、チェックリストファイル自体を編集することはない。
- **`.specify/memory/constitution.md`** はまだ未記入の `[PROJECT_NAME] Constitution` テンプレートのまま — プロジェクト固有の原則/ガバナンスはまだ制定されていない。`/speckit-plan` の憲法ゲートに頼る前に `/speckit-constitution` を実行すること。
- すべての `speckit-*` スキルは、任意の `.specify/extensions.yml`（`hooks.before_*`/`hooks.after_*` エントリ、gitブランチ自動化など）の有無を確認する。現在このリポジトリには存在しないため、フック関連セクションは何もしない。
- スクリプトは PowerShell のみ（`.specify/scripts/powershell/`） — このインストールには bash スクリプト版は存在しない。

Windows 環境: これらの `.ps1` スクリプトを直接呼び出す場合は、Windows PowerShell / pwsh スクリプトなので Bash ではなく PowerShell ツールを使うこと。
