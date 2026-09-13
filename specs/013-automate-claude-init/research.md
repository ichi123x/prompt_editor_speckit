# Phase 0 Research: CLAUDE.md生成とコミットの自動化

**Feature**: [spec.md](./spec.md) | **Date**: 2026-09-13

## R-1301: `claude` CLIを非対話的に`/init`実行する方法

**Decision**: 次のコマンドを、対象フォルダをカレントディレクトリとして実行する。

```
claude --print /init --permission-mode bypassPermissions --output-format text
```

**Rationale**:

- `--print`（`-p`）は応答を出力してプロセスを終了する非対話モード。ヘルプにも「pipesに便利」と明記されており、本アプリの`ProcessRunner`（パイプで出力を読み取りながら終了を待つ設計）とそのまま組み合わせられる
- `/init`はスラッシュコマンドで、通常は対話セッション内でしか使わないが、`--print`と組み合わせた「1回きりのプロンプト」としてそのまま渡せることを実機で確認した（後述の実測）
- `/init`はCLAUDE.mdの新規作成というファイル書き込みを伴うため、既定の権限確認モードのままでは非対話実行時にプロンプト待ちで止まる（008で確認した`winget`のToS同意プロンプトと同種の問題）。`--permission-mode bypassPermissions`はツール確認を全てスキップするモードで、ヘルプ上は「インターネットに接続しないサンドボックス向け」という注記があるが、対象は利用者が明示的に選んだフォルダに閉じており、対象フォルダに対する処理をノンストップで進めるという本アプリ全体の設計方針（`specify init --force --non-interactive`等）と一致する
- `--output-format text`は既定値だが明示し、将来の既定値変更に影響されないようにする

**実機検証**（`C:\work2\test\MFCApplication1`、うちCLAUDE.mdが存在しない状態で実施）:

| 試行 | コマンド | 経過時間 | 終了コード | 結果 |
|---|---|---|---|---|
| 1回目 | `claude.exe -p "/init" --permission-mode bypassPermissions --output-format text`（クオート付き） | 58.3秒 | 0 | CLAUDE.md生成成功 |
| 2回目 | `claude.exe --print /init --permission-mode bypassPermissions --output-format text`（クオート無し） | 34.4秒 | 0 | CLAUDE.md生成成功（既存の`.bak`ファイルを検出し復元。詳細後述） |

クオートの有無で挙動が変わらないことを確認した。`ProcessRunner::Run()`は`CreateProcess`にコマンドライン文字列をそのまま渡す設計（PowerShell/cmd.exeを経由しない）ため、クオート無しの単純な文字列で実装する。

**認証状態への依存**: `claude auth status`で`"loggedIn": true`であることを確認済み。認証されていない環境では`/init`の実行前に認証が必要になり失敗する（Assumptions・Edge Cases参照）。

**Alternatives considered**:

- `--dangerously-skip-permissions`（トップレベルのグローバルフラグ）: `--permission-mode bypassPermissions`とほぼ同義。後者はセッション単位のオプションとして明示的であり、`--permission-mode`という名前のほうが「このセッションだけ確認を省略する」という意図を表しやすいため採用した
- `--permission-mode acceptEdits`（ファイル編集のみ自動承認）: `/init`はコードベースの探索に`Bash`等の非編集ツールも使う可能性があり、それらは`acceptEdits`では自動承認されず非対話実行がハングするリスクがある。実機で`bypassPermissions`により問題なく完走することを確認済みのため、未検証の`acceptEdits`は採用しない

## R-1302: 結果一覧（`IDC_LIST_RESULT`）に6行目を追加しても寸法変更が不要か

**Decision**: `.rc`の変更は不要。

**Rationale**: 006 `spec.md` FR-507a には次の見積もり表が既に記載されている（006自身の成果物であり、本フィーチャーでは書き換えない）。

| 項目 | 現在 | 上限 | 余裕 |
|---|---|---|---|
| 結果一覧の行数 | 5手順 = 5行 | **7行**（一覧の高さ158px） | 2行 |

`IDC_STATIC_GUIDE`と異なり、結果一覧（`IDC_LIST_RESULT`）の高さ（`.rc`上90 DLU = 158px、[006 research.md](../006-readable-at-startup/research.md) R-501）は変更していない。本フィーチャーで手順が5→6に増えても、上限7行に対し1行の余裕が残るため、012のケース（余裕0行で即座に見切れる）とは異なり、`.rc`の変更なしで収まると判断する。

**検証方法**（[quickstart.md](./quickstart.md) ST-1302）: ビルド後、結果一覧に6行分のデータをセットしても縦スクロールバーが出ないことを、006の検証手法（`LVM_GETCOLUMNWIDTH`等、GUI自動操作の制約内で可能な範囲）に倣い確認する。完全な目視確認はDT-1301（実機・任意）で行う。

## R-1303: 完了案内の行数減少が006の要件に影響しないか

**Decision**: `.rc`の`IDC_STATIC_GUIDE`の高さ（012で52→61 DLUへ拡張済み）は変更しない。

**Rationale**: 012では完了案内が4手順→5手順（見出し込み6論理行）に増え、006 FR-507aの「余裕0行」を使い切ったため`.rc`を拡張した。本フィーチャーは逆に、012で追加した2手順（`/init`実行・CLAUDE.mdコミット）を案内文から削除するため、案内は5手順→3手順（見出し込み4論理行）に**減る**。61 DLUという现有の高さは6論理行＋折り返し1回（7物理行相当）を収める寸法のまま変わらないため、4論理行に減った新しい案内文は当然収まる。寸法を縮小する必然性（憲法上の要求）は無く、YAGNIの観点からも「動いているものを理由なく変更しない」を優先し、`.rc`の`IDC_STATIC_GUIDE`関連の数値は一切変更しない。

## R-1304: CLAUDE.mdのコミット方法

**Decision**: `git add -- CLAUDE.md`でCLAUDE.md単体のみをステージし、`git commit -m "CLAUDE.md を追加"`でコミットする。`RunInitGit`が行う「`git status --porcelain`を見て他人の変更を検出したら限定ステージングに切り替える」という分岐ロジックは複製しない。

**Rationale**: `RunInitGit`が`git add -A`と限定ステージングを使い分けているのは、**どのファイルが変更されるか実行時までわからない**（`.gitignore`の新規作成 or 追記、その他の未知の差分）ためである。一方、本手順が扱う対象は**常にCLAUDE.md 1ファイルのみ**という設計上の制約があり、`git add -- CLAUDE.md`と対象を明示すれば、他にどんな未コミット変更が対象フォルダにあっても巻き込むことは原理的に無い。`git status`を読んで分岐する仕組みを複製するよりも单纯であり、原則III（YAGNI）に沿う。

**Edge Case**: `git commit`が「対象ファイルは無い」ため失敗するケースは、本手順の実行順序上発生しない。手順6は「CLAUDE.mdが対象フォルダに存在しない」ことを確認してから`/init`を実行し、実行後に存在確認をしてから`git add`に進むため、コミット対象が空になることはない（FR-1303・FR-1305参照）。

## R-1305: タイムアウト値

**Decision**: 15分（`15 * 60 * 1000`ミリ秒）とする。

**Rationale**: 実測は34〜58秒だが、これは小規模なテスト用プロジェクト（MFCのテンプレートそのまま、ソースファイル数十個）での結果である。`/init`は対象コードベース全体を探索して要約するため、実際の利用者のプロジェクト規模によっては数分かかることが見込まれる。既存の手順のうち最も長いタイムアウト（`specify init`・`winget install`・`uv tool install`の10分）より長く設定し、LLM呼び出しを伴う本手順の変動幅を吸収する。008で確立したジョブオブジェクト+監視スレッドによるタイムアウト監視の仕組みがそのまま使えるため、値を変えるだけで実装できる。

## R-1306: 手順の依存関係

**Decision**: 新しい手順（`GenerateClaudeMd`）は`InitGit`（手順5）に依存する（`dependsOn = StepId::InitGit`）。

**Rationale**: 本手順はCLAUDE.mdの生成に加えGitコミットを行うため、Gitリポジトリが初期化されていることが前提になる。既存の`DependencyFailed()`（FR-012、`SetupRunner.cpp`）は「依存元手順が`Failed`または`SkippedUnmetDependency`のときのみスキップする」という判定を既に実装しており、`Skipped`（既にGit初期化・コミット済み）は正常系として扱われる。この既存の仕組みをそのまま使うだけで、新しいコードを1行も追加せずに「Git初期化に失敗していたら手順6もスキップする」という挙動が得られる。
