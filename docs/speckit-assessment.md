# Spec Kit SDD 評価レポート

**対象**: `C:\work\claude\prompt_editor_speckit`（hp-aero）
**Spec Kit バージョン**: 1.0.4（導入日 2026-09-06）
**評価日**: 2026-09-07

---

## 結論

- **ツールとしての Spec Kit**: 「テンプレート＋ゲートで LLM を規律づける」段階。仕様書を実行可能な一次成果物として扱うが、検証は最終的に LLM の自己レビューであり、形式的な保証はない。
- **本プロジェクトの SDD レベル**: **L1 — 導入完了・運用未開始**。道具は揃っているが、まだ一度も回していない。

---

## 1. ツールとしての Spec Kit v1.0.4

導入コマンドは 10 個（`constitution` / `specify` / `clarify` / `plan` / `tasks` / `analyze` / `checklist` / `implement` / `converge` / `taskstoissues`）。仕組みは 3 層。

| 層 | 実体 | 強さ |
|---|---|---|
| テンプレート | `spec-template.md` 等。User Story を P1/P2/P3 で優先度付けし、各ストーリーを「独立してテスト・デプロイ可能な MVP スライス」として書かせる | 強い。曖昧な要求を構造化する力は本物 |
| ゲート | `[NEEDS CLARIFICATION]` 上限 3 個、`checklists/requirements.md` 自動生成、plan の Constitution Check（設計前後で 2 回）、tasks.md の厳密行フォーマット | 中。LLM による自己採点 |
| スクリプト | PowerShell 6 本。機能ディレクトリ採番、前提チェック、JSON 出力 | 弱い（配管のみ）。ロジック検証はしない |

### 限界

- `analyze` は spec / plan / tasks の記述矛盾を LLM が読んで判定するのみ。**コードとの整合は見ない**。コンパイラ的な保証はゼロ。
- `constitution` は単なる Markdown。`plan` 実行時に LLM が読んで自己判定するだけで、強制力はない。
- 公式の方法論文書自体が「既存ツールの寄せ集めと規律の維持が必要」と認めている。**運用の型であって、自動化された品質保証機構ではない**。

→ 「AI が書いたものを経験のあるエンジニアが検証する」の**検証側の足場を作るツール**。検証そのものは代替しない。

---

## 2. 本プロジェクトの現状（調査で確認した事実）

- `.specify/` + `.claude/skills/` は v1.0.4 で導入済み。設定は `integration: claude` / `script: ps` / `feature_numbering: sequential` / `ai_skills: true`。ほぼ最新版。
- `.specify/memory/constitution.md` は **`[PROJECT_NAME] Constitution` のプレースホルダーのまま**。原則が一つも書かれていない。
- `specs/` ディレクトリなし、`.specify/feature.json` なし → **機能を 1 件も起票していない**。
- `MFCApplication1/` は Visual Studio ウィザードの雛形のまま（ロジックは全て `// TODO:`）。
- `.specify/extensions.yml` なし → フック・git ブランチ自動化は未使用。
- コミットは 2 件（`Spec Kit 導入` → `CLAUDE.md を追加し、既存のMDファイルを日本語化`）。ブランチは `main`、リモートに `ver.0.1` あり。
- `CLAUDE.md` は内容がよく整理されており、それ自体が資産。

### 成熟度スケール

```
L0 場当たりプロンプト
L1 ツール導入・テンプレートあり     ← 今ここ
L2 constitution 記入 + spec 1本
L3 spec→plan→tasks→implement を1周
L4 反復運用・converge・checklist が回る
L5 仕様が真のソース（再生成／CI連携）
```

---

## 3. アドバイス（優先順）

### ① 最初にやるのは `/speckit-constitution`。ただしデフォルト原則は捨てる

Spec Kit の既定原則は「Library-First」「CLI Interface」「Test-First」で、Web/CLI サービス前提。MFC ダイアログアプリにそのまま適用すると `plan` の Constitution Check が毎回ノイズになる。書くべき内容:

- ビルド検証は `msbuild MFCApplication1/MFCApplication1.sln -p:Configuration=Debug -p:Platform=x64` を通すこと（`implement` に検証手段を与える）
- UI 文字列・コメントは日本語、リソース ID の命名規則
- x64 / Win32 両方をターゲットにするか、当面 x64 のみか
- テスト方針（下記 ②）

### ② 最大の穴はテスト。ここを先に決める

SDD の中核は Test-First だが、本リポジトリにはテストスイートも lint 設定もない。`tasks` は Setup / Tests / Core / Integration / Polish のフェーズでタスクを生成するため、Tests フェーズの受け皿がない状態。選択肢:

1. **推奨** — ロジックを MFC から切り離した静的ライブラリに置き、Google Test を別プロジェクトで回す。SDD と最も噛み合う。
2. テストなしと明記し、constitution で「受け入れシナリオの手動確認をもって完了とする」と定義する（正直な妥協）。
3. 曖昧なまま進める → `implement` が毎回勝手にテストを書こうとして荒れる。**非推奨**。

### ③ 最初の 1 本は小さく

いきなりアプリ全体を `/speckit-specify` に投げると spec が巨大化し `clarify` が発散する。「プロンプト一覧の表示」程度の 1 機能で 1 周回し、テンプレートの癖と PowerShell スクリプトの挙動を確認する。

### ④ `/speckit-converge` が効く

`MFCApplication1` は雛形が既に存在するため、0→1 ではなく「既存コードと spec の差分を洗って未実装タスクを追記する」converge の使いどころがある。2 周目以降で使う。

### ⑤ git が全ファイルを変更扱いにする

`.specify` 配下は CRLF、コミット済みブロブは LF のため、Windows 以外のクライアントから見ると 44 ファイル・5950 行が丸ごと差分に見える（内容は同一）。Windows の Git では `core.autocrlf` の設定次第で見えない。`.gitattributes` に以下を置いておくと後で差分を追うときに困らない。

```
* text=auto eol=crlf
```

---

## 参考

- [Spec Kit installation docs](https://github.com/github/spec-kit/blob/main/docs/installation.md)
- [Spec-Driven Development methodology](https://github.com/github/spec-kit/blob/main/spec-driven.md)
- [github/spec-kit](https://github.com/github/spec-kit)
- [Releases · github/spec-kit](https://github.com/github/spec-kit/releases)
