# Phase 1 Data Model: CLAUDE.md生成とコミットの自動化

**Feature**: [spec.md](./spec.md) | **Date**: 2026-09-13

本フィーチャーは新しい永続データエンティティを導入しない。既存の`SetupStep`（`SetupRunner.h`）に、
新しい`StepId`を1件追加するのみである。

## `StepId`の拡張

| 値 | 追加 | 依存 |
|---|---|---|
| `None` | 既存 | — |
| `InstallUv` | 既存 | なし |
| `InstallSpecify` | 既存 | `InstallUv` |
| `InstallVsCodeExt` | 既存 | なし |
| `InitSpecKit` | 既存 | `InstallSpecify` |
| `InitGit` | 既存 | なし |
| `GenerateClaudeMd` | **新規追加（FR-1301）** | `InitGit`（R-1306） |

`SetupStep`構造体自体（`displayName`・`dependsOn`・`result`・`reason`）・`StepResult`列挙型は変更しない。

## `CreateSteps()`が返す手順一覧（変更後）

| # | `displayName` | `dependsOn` |
|---|---|---|
| 1 | uv の導入 | なし |
| 2 | specify CLI の導入 | uv の導入 |
| 3 | VS Code 拡張の導入 | なし |
| 4 | Spec Kit 足場の配置 | specify CLI の導入 |
| 5 | Git 初期化と初回コミット | なし |
| 6（新規） | **CLAUDE.md の生成とコミット** | Git 初期化と初回コミット |

## 完了案内の文字列構成（変更後、`ShowCompletionGuide()`）

| 行 | 内容 | 変更（012からの差分） |
|---|---|---|
| 見出し | 「Spec Kit の導入が完了しました。次の手順は Claude Code との対話が必要です。」 | 変更なし |
| 手順1 | VS Code で対象フォルダを開く | 変更なし（番号も変わらない） |
| 手順2（旧4） | テスト方針を決め `/speckit-constitution` で憲章を制定する | 文言は変更なし、番号のみ4→2 |
| 手順3（旧5） | `/init` を再実行し、憲章を反映した CLAUDE.md に更新する | 文言は変更なし、番号のみ5→3 |
| ~~旧手順2~~ | ~~`/init` を実行し CLAUDE.md を生成する~~ | **削除**（手順6が自動実行するため、FR-1311） |
| ~~旧手順3~~ | ~~生成された CLAUDE.md をコミットする~~ | **削除**（手順6が自動実行するため、FR-1311） |

`m_workerPath`が埋め込まれるのは手順1のみであり、この埋め込み方法（`static_cast<LPCTSTR>`）は変更しない。
