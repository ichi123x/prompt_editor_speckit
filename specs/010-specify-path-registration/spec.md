# Feature Specification: specify CLI導入後のPATH未登録を解消する

**Feature Branch**: `010-specify-path-registration`

**Created**: 2026-09-13

**Status**: Draft

**Input**: User description: "specify CLI導入手順(uv tool install specify-cli)が成功しても、uvはインストール先ディレクトリ(既定では%USERPROFILE%\.local\bin)をPATHへ自動登録しないため、続くSpec Kit足場配置の手順がspecifyコマンドを「見つかりません」として失敗する問題を修正する。uv tool installの成功直後に、uv自身が提供するシェル環境更新の仕組み(uv tool update-shell相当)を実行してPATH登録まで完結させることで、アプリを再起動しなくても後続の手順からspecifyを使えるようにする。"

## User Scenarios & Testing（ユーザーシナリオとテスト） *(必須)*

### User Story 1 - specify CLI新規導入直後、後続手順からspecifyを使える (Priority: P1)

specify CLIが未導入の環境で「生成」を実行したユーザーは、手順2でspecify CLIが新規にインストールされた場合、続く手順4（Spec Kit足場配置）がspecifyコマンドを正しく検出し、アプリを再起動しなくても最後まで処理が進むことを期待する。

**Why this priority（この優先度である理由）**: 009でレジストリPATHの反映遅延を解消した結果、実機で新たに確認された問題である。`uv tool install`はインストール先ディレクトリ（既定`%USERPROFILE%\.local\bin`）をレジストリのPATHへ自動登録しないため、009の対策（レジストリ再読み込み）だけでは効果がない（レジストリに元々登録されていないものは読み直しても見つからない）。この問題が残る限り、specify CLIが未導入の環境ではSDD化が初回実行だけで完走できない。

**Independent Test（独立したテスト方法）**: specify CLIが未導入の環境（uvは導入済みでよい）で対象フォルダを選択し「生成」を実行するだけで検証できる。手順2（specify CLI導入）が成功した直後の手順4（Spec Kit足場配置）が「specifyが見つかりません」で失敗しないことを確認する。

**Acceptance Scenarios（受け入れシナリオ）**:

1. **Given**（前提） specify CLIが未導入の環境、**When**（動作） 「生成」を実行し手順2でspecify CLIが新規にインストールされる、**Then**（結果） 続く手順4はインストールされたspecifyを検出でき、「specifyが見つかりません」という失敗にならない。
2. **Given**（前提） 手順2の完了直後、**When**（動作） 手順4が実行される、**Then**（結果） specifyのインストール先ディレクトリが（レジストリ経由で）PATHに登録された状態になっている。

---

### Edge Cases（エッジケース）

- specify CLIが既に導入済み（PATHにも登録済み）の環境で実行した場合、手順2は「スキップ」となり、PATH登録処理自体を行う必要はない（既存動作を変えない）。
- uv自体は既に導入済み（PATH登録済み）でspecify CLIだけを新規インストールする場合も、同様に手順4から検出できること。
- PATH登録処理自体が何らかの理由で失敗した場合でも、手順2自体の成否判定（`uv tool install`の結果）には影響を与えず、既存の「見つかりません」という失敗表示にフォールバックし、アプリが異常終了しないこと。

## Requirements（要件） *(必須)*

### Functional Requirements（機能要件）

- **FR-1001**: システムはspecify CLI導入手順（`uv tool install specify-cli`）が成功した場合、続けてそのインストール先ディレクトリをPATHへ登録する処理（uvが提供するシェル環境更新の仕組み）を実行しなければならない。
- **FR-1002**: PATH登録処理は、specify CLI導入手順が「スキップ」（既に導入済み）だった場合には実行しなくてよい。
- **FR-1003**: PATH登録処理が失敗した場合でも、手順2自体の成否判定（`uv tool install`の結果）を変えず、処理を継続しなければならない。
- **FR-1004**: 本修正は既存の5手順の実行順序・依存関係・べき等性を変更してはならない。
- **FR-1005**: 本修正はspecify CLI導入手順に限定してよい。008・009が「`ProcessRunner`を使う全コマンドに単一の仕組みで適用する」ことを求めたのとは異なり、PATH自動登録の要否はツールのインストール方法（uvのtool install固有の挙動）に依存するため、他の手順（winget・code・git）へ汎用化する必要はない。

### Key Entities（主要エンティティ） *(データが関わる機能の場合に記載)*

N/A（本機能はコード実行フローの修正であり、新たな永続データエンティティを導入しない）

## Success Criteria（成功基準） *(必須)*

### Measurable Outcomes（測定可能な成果）

- **SC-1001**: specify CLIが未導入の環境で「生成」を実行し手順2でspecify CLIが新規インストールされたとき、100%のケースで続く手順4が「specifyが見つかりません」という誤った失敗にならない。
- **SC-1002**: specify CLIが既に導入済みの環境では、修正前と同じ結果表示・所要時間で成功する（回帰がない）。
- **SC-1003**: PATH登録処理が失敗する状況を人為的に再現しても、手順2の成否判定に影響を与えず、アプリが異常終了しない。

## Assumptions（前提事項）

- PATH登録には`uv`自身が提供するサブコマンド（`uv tool update-shell`相当）を利用する。実機で`uv 0.12.13`にてこのサブコマンドの存在と効果（レジストリのユーザーPATHへの追記、`Updated PATH to include executable directory ...`という出力）を確認済み。
- 009で実装した「`ProcessRunner::Run()`の先頭でレジストリのPATHを読み直し既存PATHへ追記する」仕組みと組み合わせて機能する。`uv tool update-shell`がレジストリへ書き込んだ内容を、後続の`ProcessRunner::Run()`呼び出しが009の仕組みで正しく拾える前提であり、本機能は009に依存する。
- `uv tool update-shell`を呼ぶタイミングや、呼び出しコマンドの具体的な組み立て方（`uv tool install`と`uv tool update-shell`を別々に呼ぶか等）は`/speckit-plan`で決定する技術判断であり、本specはその選定を指定しない。
- ビルド構成はDebugでよい（008・009に引き続きユーザー指示を踏襲）。
