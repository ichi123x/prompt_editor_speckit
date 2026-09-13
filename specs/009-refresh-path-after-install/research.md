# Phase 0 Research: 新規インストール直後のPATH未反映を解消する

**Feature**: [spec.md](./spec.md) | **Date**: 2026-09-13

## R-901: レジストリ読み取りAPIの選定

**Decision**: `RegGetValueW`（`RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ`、`RRF_NOEXPAND`は指定しない）を使い、
`HKEY_CURRENT_USER\Environment`と`HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment`
それぞれの`Path`値を読み取る。

**Rationale**:
- `RegGetValueW`はキーのオープン・クローズを内部で行い、呼び出し側で`RegOpenKeyEx`/`RegCloseKey`の対を
  管理する必要がない
- `RRF_NOEXPAND`を指定しなければ、値の型が`REG_EXPAND_SZ`（システムPATHは通常この型）であっても
  自動的に`%SystemRoot%`等を展開して返すため、`ExpandEnvironmentStrings`を別途呼ぶコードが不要になる
  （FR-902）。値が`REG_SZ`（ユーザーPATHはこちらのことが多い）であっても同じ呼び出しでそのまま扱える
- サイズ取得→バッファ確保→再取得という2段階呼び出し（`lpData=nullptr`で`pcbData`だけ得る）は
  `CString::GetBuffer`/`ReleaseBuffer`とそのまま組み合わせられ、`ProcessRunner.cpp`の既存の
  `BytesToString`と同様の書き方に馴染む

**Alternatives considered**:
- `RegOpenKeyEx` + `RegQueryValueEx` + 型が`REG_EXPAND_SZ`のときだけ`ExpandEnvironmentStrings`を呼ぶ:
  動作は同じだが分岐とハンドル管理が増える。`RegGetValueW`1呼び出しに集約できるなら、その方が
  シンプル（原則III YAGNI）
- レジストリではなく`GetEnvironmentVariable`で現在のプロセスのPATHを見る: これは今回の問題の原因
  そのもの（プロセス起動時のコピーが古い）であり、対象にならない

## R-902: 合成方法 — 追記であって置き換えではない

**Decision**: 新しいPATHは「現在のプロセスが**既に持っている**PATH」＋「システムPATH（レジストリ）」＋
「ユーザーPATH（レジストリ）」の順に`;`で連結したものとし、`SetEnvironmentVariable`で丸ごと置き換える。
**置き換えではなく追記**という性質が本質であり、実装上は結果を1本の文字列にまとめて`SetEnvironmentVariable`
に渡す形になるが、既存の値を切り捨てない。

**Rationale**:
- 本アプリの利用者は、コマンドプロンプトやPowerShellから`set PATH=...`のようにセッション限定でPATHを
  一時的に変更した上で本アプリを起動している可能性がある。これらの値はレジストリには存在しないため、
  「レジストリの2値だけで丸ごと置き換える」設計を取ると、こうしたセッション限定のカスタマイズを
  消してしまい、FR-903（既存動作を壊さない）・FR-905（PATH以外は変更しないが、PATHについても
  「壊れていない部分は保つ」という意図を含む）の精神に反する
- 「既存のプロセスPATH」＋「レジストリの最新値」という追記型にすれば、新しく登録されたパスを
  取りこぼすことなく追加しつつ、プロセスが元々持っていた情報を一切失わない。純粋な加算であり、
  副作用が最も小さい
- 重複が生じても実害はない（spec.md Edge Cases: 重複除去はスコープ外）。Windowsのコマンド探索は
  PATHを先頭から順に探すだけであり、同じディレクトリが複数回現れても速度以外への影響はなく、
  今回のような小規模な呼び出し回数では体感できない

**Alternatives considered**:
- レジストリの2値だけで置き換える（追記しない）: 上記の理由（セッション限定のPATH変更を消してしまう
  リスク）により却下
- 重複を除去してから結合する: 実装量が増える割に得られる価値が小さい（spec.md Assumptionsが明示的に
  重複除去をスコープ外としている）ため却下

## R-903: 呼び出し箇所を `ProcessRunner::Run()` の先頭1箇所に集約する

**Decision**: PATH追記処理は`ProcessRunner::Run()`の先頭、`CreateProcess`を呼ぶ前に1箇所だけ呼ぶ。
`SetupRunner.cpp`は一切変更しない。

**Rationale**:
- `SetupRunner.cpp`の5手順はいずれも`ProcessRunner::Run()`を介してのみ外部コマンドを実行するため
  （008 FR-806で確認済みの構造）、ここに1箇所置くだけで、5手順のどの組み合わせ・どの呼び出し順でも
  もれなく最新のPATHが使われる
- `SetupRunner::RunAll()`のループ先頭に置く案も検討したが、1つの手順の中で複数回`Run()`を呼ぶケース
  （例: `RunInstallSpecify`は`specify version`のprobeと`uv tool install specify-cli`のinstallの
  2回呼ぶ）では、probe時点ではまだ古いPATHのままになる。`Run()`自体の先頭に置けば、同一手順内の
  2回目以降の呼び出しも含めて、あらゆる外部コマンド実行の直前に必ず最新化される
- `SetupRunner.cpp`を変更しないことで、FR-906（5手順の実行順序・依存関係・べき等性を変更しない）を
  「変更していないことを確認する」までもなく、変更対象に含まれないという形で構造的に保証できる

**Alternatives considered**:
- `SetupRunner::RunAll()`のループ先頭で手順ごとに1回だけ呼ぶ: 上記の理由（同一手順内の2回目の呼び出しに
  間に合わない）により却下
- 各`RunInstallXxx`関数の冒頭で個別に呼ぶ: 呼び出し箇所が5箇所（またはそれ以上）に増え、将来手順が
  追加されるたびに呼び忘れるリスクがある。`Run()`1箇所に集約する方が確実で、原則III（YAGNI）にも
  適う

## R-904: レジストリ読み取り失敗時のフォールバック

**Decision**: システムPATH・ユーザーPATHのいずれかの読み取りに失敗した場合、そのハイブの値を空文字列
として扱い、結合処理を続行する。両方とも失敗（空）の場合は、`SetEnvironmentVariable`を呼ばず、
プロセスが元々持っていたPATHをそのまま使う。

**Rationale**:
- R-902の「追記」設計により、片方だけ読めた場合でも、既存のPATHを失うことなく、読めた方だけを
  追記すればよい。特別なエラーハンドリング分岐を増やす必要がない
- 両方失敗した場合に何もしない（＝既存のPATHをそのまま使う）ことで、FR-903（読み取り失敗時も
  既存のコマンド検出動作を壊さない）を、例外を投げたり`AfxMessageBox`で利用者に通知したりすることなく
  満たせる。この処理はUIから見えない内部的な最適化であり、失敗してもユーザー体験上は「PATHの
  自動反映が効かないだけ」で、既存の「コマンドが見つかりません」という表示に自然にフォールバックする

**Alternatives considered**:
- 読み取り失敗時に結果一覧へ警告を出す: 本機能はPATH反映という内部的な補助機能であり、失敗しても
  既存の「コマンドが見つかりません」という表示が引き続き機能するため、利用者に新しい通知を追加する
  必要性が薄い（原則III YAGNI）。却下
