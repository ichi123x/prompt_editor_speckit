# Phase 0 Research: 外部コマンドの無応答ハング修正

**Feature**: [spec.md](./spec.md) | **Date**: 2026-09-13

## R-801: winget の対話プロンプト対策

**Decision**: [SetupRunner.cpp:90](../../MFCApplication1/SetupRunner.cpp#L90) の `winget install --id=astral-sh.uv -e` に
`--accept-source-agreements --accept-package-agreements --source winget` を追加する。

**事実確認（実機ログ）**: 今回の不具合発生時の `winget` ログ
（`%LOCALAPPDATA%\Packages\Microsoft.DesktopAppInstaller_8wekyb3d8bbwe\LocalState\DiagOutputDir\WinGet-*.log`）
を確認したところ、`winget` ソースのインデックスを正常に開いた直後でログの出力が完全に停止していた。
これは `-e`（完全一致）指定であっても、対象コマンドが最初に「複数ソースを既定で検索対象に追加する」
（ログ: `Default source requested, multiple sources available, adding all to source references.` /
`Adding to source references msstore`）ため、`msstore` ソースへの初回アクセス時に表示される
利用規約同意プロンプト（「このソースの使用には利用規約への同意が必要です。同意しますか？ [Y] Yes [N] No」）
で入力待ちになったと判断できる。[ProcessRunner.cpp:75](../../MFCApplication1/ProcessRunner.cpp#L75) は
`hStdInput` に親（GUI サブシステムで実コンソールを持たない）の `STD_INPUT_HANDLE` をそのまま渡しており、
`CREATE_NO_WINDOW` により生成される非表示コンソールの入力は誰からも到達しないため、この待ちは自然には解消しない。

**Rationale**:
- `--source winget` を明示すれば `msstore` ソースを検索対象から外せるため、この同意プロンプト自体が発生しなくなる
- `--accept-source-agreements --accept-package-agreements` を併せて指定することで、`winget` ソース自体や
  パッケージ側に将来別の同意プロンプトが追加された場合にも備える（winget の一般的な非対話運用の定石）
- `specify init` に `--non-interactive` が必要だったのと同じ性質の対症療法であり、CLAUDE.md
  「`specify init` には4つのフラグがすべて必要」の記述と一貫する

**Alternatives considered**:
- `winget source reset --force`等で事前にソース同意状態をリセット/確定させる: マシン全体のwinget設定を書き換える副作用があり、対象フォルダにしか作用しないはずの本ツールの責務を超える。却下
- `<CTRL-Z>`（EOF）を子プロセスのstdinへ明示的に書き込んで「同意しない」扱いにする: `winget`はEOFを受けても待ち続ける可能性があり実測で確認できておらず、かつ`--source winget`で問題の発生源を断つ方が確実で単純。却下

## R-802: ProcessRunner のタイムアウト実効化

**Decision**: [ProcessRunner.cpp:48-150](../../MFCApplication1/ProcessRunner.cpp#L48-L150) の `Run()` を、
子プロセスをジョブオブジェクトへ割り当てた上で、別スレッドがプロセスの生死をタイムアウト付きで監視し、
超過時に `TerminateJobObject` でプロセスツリーごと打ち切る構成に変更する。

**現状の欠陥の再現条件**: [ProcessRunner.cpp:111-127](../../MFCApplication1/ProcessRunner.cpp#L111-L127) の
読み出しループは、`ReadFile` が返ってきた後（＝子プロセスが何か書き込んだ後）にしか `GetTickCount64` による
経過時間の比較を行わない。子プロセスが**出力を止めたまま**終了もしない場合（R-801 の `winget` のように
入力待ちで停止するケースが典型）、`ReadFile` はそれ自体がブロックし続けるため、後続の比較コードは
一度も実行されない。実機でも、コード上は10分（`SetupRunner.cpp:90`）でタイムアウトする設計であるにもかかわらず、
5分弱経過してもプロセスが残存し続けていることを確認した（手動で `Stop-Process` するまで終了しなかった）。

**Rationale**:
- **監視スレッド**: `ReadFile` のブロッキングとは独立に、`WaitForSingleObject(hProcess, timeoutMs)` で
  タイムアウトを判定できる。子プロセスが正常終了すればこの待機はすぐに `WAIT_OBJECT_0` で戻り、
  スレッドは何もせず終了する（追加のキャンセル信号は不要）。子プロセスがタイムアウトまで生存していれば
  `WAIT_TIMEOUT` が返り、その時点でジョブオブジェクトを終了させる
- **ジョブオブジェクト**: `TerminateProcess` は対象プロセス単体しか終了させず、そのプロセスがさらに
  子プロセスを起動していた場合（`winget` が起動した関連プロセスなど）は残存し得る。子プロセスを
  `CREATE_SUSPENDED` で起動し、`ResumeThread` する前に `AssignProcessToJobObject` で
  `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` 付きのジョブへ割り当てておけば、`TerminateJobObject`
  一発でそのプロセスが（実行再開後に）生成した子孫プロセスも含めて確実に終了できる。これはFR-804
  （残存プロセスなし）を、`winget` に限らず `ProcessRunner` を使う全コマンドに対して満たす
- **1つの仕組みで完結**: 監視スレッドとジョブオブジェクトはいずれも `Run()` の内部にのみ存在し、
  呼び出し元（`SetupRunner.cpp`）や `ProcessRunner.h` のインターフェースには一切影響しない。
  FR-806（単一の仕組みで全コマンドに適用、個別コマンドの特別扱いをしない）を満たす

**Alternatives considered**:
- **オーバーラップI/O**（`FILE_FLAG_OVERLAPPED` な名前付きパイプ＋`GetOverlappedResultEx`によるタイムアウト付き読み出し）:
  `ReadFile`のブロッキング自体は解消できるが、匿名パイプ（`CreatePipe`）を名前付きパイプへ置き換える書き直しが
  必要になり実装量が増える。加えてFR-804（プロセスツリーの残存防止）はこの方式単独では解決せず、
  結局ジョブオブジェクトが別途必要になる。1つの仕組みでFR-802とFR-804の両方を満たせる監視スレッド方式を採用
- **`WaitForSingleObject`のみを先に呼び、パイプは非ブロッキングでポーリングする**: 匿名パイプは
  `PeekNamedPipe`でのポーリングは可能だが、ビジーウェイトのポーリング間隔とCPU消費のトレードオフが生じる。
  監視スレッドは`WaitForSingleObject`によるイベント駆動のため、この種のポーリングを避けられる
- **タイムアウト値そのものを短縮する**: 今回のspec（Assumptions）でスコープ外と明記済み。値の妥当性ではなく
  「値が守られない」ことが問題であるため、値の変更では解決しない

## R-803: 既存の終了コード互換性

**Decision**: タイムアウトで打ち切られた場合の `exitCode` は、現行実装と同じ `WAIT_TIMEOUT`（`258`）を
`TerminateJobObject` の第2引数（終了コード）として渡すことで維持する。

**Rationale**: [ProcessRunner.cpp:124](../../MFCApplication1/ProcessRunner.cpp#L124) の現行実装は
`TerminateProcess(pi.hProcess, static_cast<UINT>(WAIT_TIMEOUT))` によって、タイムアウト時の `exitCode` を
`WAIT_TIMEOUT` に固定している。`ProcessResult::Succeeded()` は `exitCode == 0` のみを成功とするため、
`258` はいずれにせよ「失敗」に分類され、既存の呼び出し元（`SetupRunner.cpp` の各手順）の分岐ロジックに
影響しない。値そのものを変える理由がないため、そのまま踏襲する。

**Alternatives considered**: 専用のタイムアウトフラグを `ProcessResult` に追加する案も検討したが、
`Succeeded()` の判定にもUI表示にも新しい分岐は不要（既存の「失敗」表示で十分、FR-803）であり、
`ProcessResult` の構造を変えるとインターフェース変更になり FR-806 の「既存呼び出し元に影響を与えない」
という制約を超える。却下

## R-804: 起動時のプロセス生成方法変更（CREATE_SUSPENDED）

**Decision**: [ProcessRunner.cpp:86-87](../../MFCApplication1/ProcessRunner.cpp#L86-L87) の `CreateProcess`
呼び出しに `CREATE_SUSPENDED` フラグを追加し、`AssignProcessToJobObject` の完了後に `ResumeThread` する。

**Rationale**: `CREATE_SUSPENDED` なしで子プロセスをジョブに割り当てると、割り当てが完了するまでの
わずかな時間に子プロセスが実行を開始し、孫プロセスを起動してしまう競合状態がありうる。孫プロセスは
ジョブの割り当てより前に生成された場合、ジョブに属さない可能性がある。`CREATE_SUSPENDED` で
主スレッドを止めたまま確実にジョブへ割り当ててから `ResumeThread` することで、この競合を構造的に防ぐ。
これはWindowsでの「プロセスツリーを確実にジョブ管理下に置く」際の定石。

**Alternatives considered**: 割り当てを省略し実行後即座に行う: 上記の競合状態を残したままになり、
FR-804の「残存プロセスなし」を保証できないケースが理論上残るため却下。
