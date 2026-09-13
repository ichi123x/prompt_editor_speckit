
// MFCApplication1Dlg.cpp : 実装ファイル
//

#include "pch.h"
#include "framework.h"
#include "MFCApplication1.h"
#include "MFCApplication1Dlg.h"
#include "afxdialogex.h"

#include "GitignoreWriter.h"
#include "ProcessRunner.h"

// 005 FR-401: タイトルバーの着色に DWM のウィンドウ属性を使う。
// dwmapi.lib は .vcxproj ではなく #pragma でリンクする。.vcxproj に書くと
// Debug/Release × x64/Win32 の4構成すべてに足す必要があり、1つ漏らすと
// その構成だけリンクエラーになる (research.md R-406)。
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 005 FR-402, FR-403: タイトルバーの配色。
// COLORREF のバイト順は 0x00BBGGRR であり #RRGGBB とは逆。RGB マクロを
// 使うことで並べ替えの取り違えを構造的に防ぐ。#1F3864 をリテラルで書くと
// 0x0064381F になる。0x001F3864 と書くと R と B が入れ替わり、濃い青の
// つもりが茶色寄りの色になる (research.md R-402)。
static const COLORREF kCaptionColor = RGB(0x1F, 0x38, 0x64);	// ネイビー #1F3864
static const COLORREF kCaptionTextColor = RGB(0xFF, 0xFF, 0xFF);	// 白。背景とのコントラスト比は約 11.5:1 (SC-402)


// アプリケーションのバージョン情報に使われる CAboutDlg ダイアログ

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

// 実装
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFCApplication1Dlg ダイアログ



CMFCApplication1Dlg::CMFCApplication1Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MFCAPPLICATION1_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCApplication1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_RESULT, m_listResult);
}

BEGIN_MESSAGE_MAP(CMFCApplication1Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CMFCApplication1Dlg::OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_BUTTON_GENERATE, &CMFCApplication1Dlg::OnBnClickedButtonGenerate)
	ON_MESSAGE(WM_APP_STEP_FINISHED, &CMFCApplication1Dlg::OnStepFinishedMessage)
	ON_MESSAGE(WM_APP_SETUP_COMPLETED, &CMFCApplication1Dlg::OnSetupCompletedMessage)
	ON_WM_SIZE()                                    // 004 FR-301
	ON_WM_GETMINMAXINFO()                           // 004 FR-306
END_MESSAGE_MAP()


// CMFCApplication1Dlg メッセージ ハンドラー

// 005 FR-401〜404: タイトルバーを濃紺・文字を白にする。戻り値は意図的に見ない
// (FR-408) — 着色できない環境では E_INVALIDARG が返るだけで何も起こらない
// (research.md R-405)。
//
// 007 FR-701, FR-702: OnInitDialog だけでなく、フォルダ選択ダイアログ
// (CFolderPickerDialog::DoModal()) を開く直前・閉じた直後にも呼ぶ。実機報告
// では、この経路を通るとタイトルバーの色が既定色に戻ったまま復帰しなかった
// (007 research.md R-701)。原因は特定できていないが、症状そのものを直接
// 打ち消すため、呼び出し箇所を増やす形で対応する (R-702, R-705)。
// OnActivate や WM_NCACTIVATE のような汎用的な契機では呼ばない — 対象を
// メインダイアログとフォルダ選択ダイアログの経路だけに絞る (FR-704, FR-705)。
//
// 007 FR-703: 呼び出し箇所が増えても戻り値を判定する分岐は一切追加しない。
// 着色できない環境で新たなエラー・警告が生じないことは、この方針そのもの
// によって構造的に保証される。
// 007 FR-706: 変更・追加した各箇所に対応する FR 番号をコメントで記載する
// (憲章 原則IV)。
void CMFCApplication1Dlg::ApplyCaptionColor()
{
	COLORREF captionColor = kCaptionColor;
	COLORREF captionTextColor = kCaptionTextColor;
	DwmSetWindowAttribute(GetSafeHwnd(), DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));
	DwmSetWindowAttribute(GetSafeHwnd(), DWMWA_TEXT_COLOR, &captionTextColor, sizeof(captionTextColor));
}

BOOL CMFCApplication1Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// "バージョン情報..." メニューをシステム メニューに追加します。

	// IDM_ABOUTBOX は、システム コマンドの範囲内になければなりません。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
	//  Framework は、この設定を自動的に行います。
	SetIcon(m_hIcon, TRUE);			// 大きいアイコンの設定
	SetIcon(m_hIcon, FALSE);		// 小さいアイコンの設定

	SetWindowText(_T("SDDプロジェクトのベース作成ツール"));

	// 005 FR-401, FR-402: タイトルバーを濃い青 (#1F3864) に、
	// 005 FR-403: タイトル文字を白にする。文字色を指定しないと、利用者が
	// ライトモードのときに文字が濃色で描かれ、濃紺の背景に埋もれて読めなくなる。
	//
	// 005 FR-404: ここで設定するのは、OnInitDialog がダイアログの表示より前に
	// 走るため。既定色が一瞬見えてから塗り替わる、という見え方にならない。
	//
	// 005 FR-408, FR-409: 戻り値を意図的に見ない。着色できない環境
	// (Windows 10 や Windows 11 ビルド 22000 未満) では E_INVALIDARG が返るだけで
	// 例外も異常終了も起きないことを実測済み (research.md R-405)。無視すれば
	// 「何も起こらなかった」のと同じになり、エラー表示もアプリの停止も生じない。
	// 判定を書くと、そこから「失敗したら何かする」分岐が生えて FR-408 に反する。
	//
	// 005 FR-405, FR-406, FR-407, FR-410: ここで1回設定するだけでよく、
	// 塗り直してはならない。DWM がウィンドウ属性として色を保持するため、
	// 非アクティブ・最大化・最小化からの復帰・ライト/ダークモードの切り替えの
	// いずれでも色は保たれる (research.md R-407, R-408 で実測)。
	// したがって 004 の OnSize / OnGetMinMaxInfo には一切手を入れない。
	// ここに再設定処理を足すと、004 のリサイズ追従に無関係な責務が混ざる。
	//
	// 005 FR-411: 着色するのはこのメインダイアログ1枚のみ。メッセージボックス、
	// フォルダ選択ダイアログ、バージョン情報ダイアログは対象外であり、
	// 他のウィンドウへ広げる共通の仕組みも作らない (contracts/ui-contract.md 4章)。
	//
	// 007 FR-701: 着色処理そのものは ApplyCaptionColor() に切り出した。
	// OnBnClickedButtonBrowse からも呼ぶため (FR-701, FR-702)。
	ApplyCaptionColor();

	// 結果一覧を3列のレポートビューとして初期化する (FR-011, FR-019 / research.md R-009)。
	InitResultList();

	// 初期状態の契約 (contracts/ui-contract.md「1. 初期状態の契約」):
	//  - パス表示欄は空。これが「フォルダ未選択」を表す (US1-4 / DT-003 の前提条件)
	//  - 生成・診断ボタンは有効。未選択でも押下でき、押下時に検証する (001 FR-005)
	//  - 002 FR-101: 種別を選ぶコントロールは存在しない
	SetDlgItemText(IDC_EDIT_TARGET_PATH, _T(""));
	m_targetPath.Empty();
	m_running = false;
	SetControlsEnabled(true);

	// 002 FR-107: 種別に言及する初期案内文は削除済み。
	// 003 FR-206: 診断への言及も含まない。
	SetGuideText(_T("対象フォルダを選び、「生成」を押してください。")
				 _T("\r\n対象のプロジェクトは、あらかじめ Visual Studio 2022 などのウィザードで作成し、そのフォルダに配置しておいてください。"));

	// 004 FR-301: 追従テーブルの初期化。すべての記録が済んだ最後に m_layoutReady を立てる。
	InitAnchors();

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

void CMFCApplication1Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// ダイアログに最小化ボタンを追加する場合、アイコンを描画するための
//  下のコードが必要です。ドキュメント/ビュー モデルを使う MFC アプリケーションの場合、
//  これは、Framework によって自動的に設定されます。

void CMFCApplication1Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 描画のデバイス コンテキスト

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// クライアントの四角形領域内の中央
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// アイコンの描画
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// ユーザーが最小化したウィンドウをドラッグしているときに表示するカーソルを取得するために、
//  システムがこの関数を呼び出します。
HCURSOR CMFCApplication1Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


/////////////////////////////////////////////////////////////////////////////
// SDDプロジェクトのベース作成ツール
/////////////////////////////////////////////////////////////////////////////

// 004 FR-308: 結果一覧の列幅の比率（%）。初期化時と再配置時の両方から参照する。
// 2箇所に分けて持つと、列を追加したり比率を変えたりしたときに片方だけ直す誤りが起きる
// (research.md R-305(2))。
//
// 006 FR-501, FR-502: 起動直後の大きさのまま「内容」欄を最後まで読めるようにするため、
// 配分を見直した。各列に必要な幅は実測に基づく (006 research.md R-505)。
//
//   項目  最長「Git 初期化と初回コミット」                       153 px
//   結果  最長「スキップ（前提未達）」                           126 px
//   内容  最長「Git初期化はスキップ。既存の未コミット変更を…」   552 px
//
// 「内容」の最長は失敗時のメッセージではなく、Git 手順が「完了」したときの
// メッセージである。条件に応じて先頭に「Git初期化はスキップ。」が連結されて伸びるため、
// 単独の文字列リテラルだけを測ると最長を見落とす (006 research.md R-504)。
//
// 006 FR-506: ダイアログは 545 x 216 DLU（ウィンドウ 971 x 423 px）とし、
// 1024x768 の画面の作業領域に収まる範囲に留める。一覧のクライアント幅は 925 px、
// 実際の列幅は 185 / 157 / 582 px となり、必要幅（余白 20 px 込み）を
// それぞれ 12 / 11 / 10 px 上回る。
//
// 006 FR-504: 案内欄は 52 DLU に広げ、完了案内の5行が折り返しもスクロールも
// なしに収まるようにした。この配分でなければ成立しない — 従前の配分のままで
// 「内容」に 572 px を確保するには一覧幅が 1430 px 必要になり、上の制約を超える。
//
// 006 FR-504a: 案内欄の高さを 5 行分ではなく 6 行分にしてあるのは、完了案内の
// 2行目が利用者の選んだフォルダのパスを含み、パスが長いと折り返して表示が
// 6 行になるため。折り返し 1 回分を吸収することで、長いパスでも 5 行すべてが
// スクロールなしに見える。
//
//   案内欄のクライアント幅 905 px − 固定文 412 px = パスに使える幅 493 px
//   （半角で約 82 文字。OneDrive の同期フォルダと日本語のフォルダ名を
//     組み合わせたパスは 1033 px に達し、現実的に到達する）
//
// 44 DLU（クライアント 73 px）では 5 行しか入らない。6 行には 85 px が必要で
// 51 DLU 以上、丸めの余裕を見て 52 DLU（クライアント 87 px）とした。
// ダイアログの高さはこれに連動して 216 → 224 DLU（ウィンドウ 437 px）になり、
// FR-506 の 1024x768 制約には収まったままである。
//
// 006 FR-509: 比率を1箇所で定義する仕組みは 004 のまま維持する。値だけを変えた。
static const int kColumnRatio[3] = { 20, 17, 63 };

// 結果一覧の初期化 (001 FR-011)。
//
// 003 FR-202, FR-203: 診断の7項目判定と、その結果に基づく手順案内は廃止した。
// 001 では手順結果と診断結果がこの一覧を共用していたが、003 以降は手順結果の
// 表示にのみ使う。共用をやめるだけで、列定義も表示処理も変更していない。
void CMFCApplication1Dlg::InitResultList()
{
	m_listResult.SetExtendedStyle(m_listResult.GetExtendedStyle()
								  | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	CRect rect;
	m_listResult.GetClientRect(&rect);
	const int width = rect.Width() > 0 ? rect.Width() : 600;

	m_listResult.InsertColumn(0, _T("項目"), LVCFMT_LEFT, width * kColumnRatio[0] / 100);
	m_listResult.InsertColumn(1, _T("結果"), LVCFMT_LEFT, width * kColumnRatio[1] / 100);
	m_listResult.InsertColumn(2, _T("内容"), LVCFMT_LEFT, width * kColumnRatio[2] / 100);
}

// 004 FR-308: 結果一覧の列幅を一覧の幅に追従させる。
//
// 再配置のたびに無条件で再計算する。「幅が変わったときだけ」の条件判定は設けない。
// 前回の幅を保持する状態が増えるうえ、「変わったのに走らない」誤りは表示の破綻として
// 現れるためである (contracts/ui-contract.md「6. 再計算のタイミング」)。
//
// この方式では、ユーザーが列の境界をドラッグして手で調整した列幅は次のリサイズで
// 既定の比率に戻る。列幅の手動調整を保持する要件は無いため許容する。
void CMFCApplication1Dlg::UpdateResultListColumns()
{
	if (m_listResult.GetSafeHwnd() == nullptr)
	{
		return;
	}

	CRect rect;
	m_listResult.GetClientRect(&rect);
	const int width = rect.Width();
	if (width <= 0)
	{
		return;
	}

	for (int i = 0; i < 3; ++i)
	{
		m_listResult.SetColumnWidth(i, width * kColumnRatio[i] / 100);
	}
}

void CMFCApplication1Dlg::ClearResultList()
{
	m_listResult.DeleteAllItems();
}

void CMFCApplication1Dlg::AppendResultRow(const CString& name, const CString& status,
										  const CString& detail)
{
	const int row = m_listResult.GetItemCount();
	m_listResult.InsertItem(row, name);
	m_listResult.SetItemText(row, 1, status);
	m_listResult.SetItemText(row, 2, detail);
	m_listResult.EnsureVisible(row, FALSE);
}

// 案内文の表示 (002 FR-110〜112 / contracts/ui-contract.md「4-2」)。
//
// IDC_STATIC_GUIDE は静的テキストではなく読み取り専用の複数行エディットである。
// 静的テキストだった頃、5行の案内が3行目で切り詰められて表示され、さらに
// 001 の User Story 3 が求める「コマンドをそのまま貼り付けられる形」を
// 満たしていなかった（静的テキストは選択もコピーもできない）。
//
// 案内文の行数は今後も変わりうるため、高さの調整ではなくスクロール可能な
// コントロールで解決している。この関数側の実装は変更不要で、エディットでも
// SetDlgItemText でそのまま設定できる。
void CMFCApplication1Dlg::SetGuideText(const CString& text)
{
	SetDlgItemText(IDC_STATIC_GUIDE, text);
}

// 実行中に無効化するコントロール (003 FR-209 / contracts/ui-contract.md「3.3」)。
//
// 生成ボタンだけでなく、**参照ボタンも無効化する**。処理中にフォルダを
// 変更されると、ワーカースレッドの処理対象と画面表示が食い違うためである。
//
// 003 で診断ボタンを配列から取り除いた際、IDC_BUTTON_BROWSE を巻き添えで
// 消してはならない (003 research.md R-204(1))。DT-207 がこれを検証する。
void CMFCApplication1Dlg::SetControlsEnabled(bool enabled)
{
	const BOOL flag = enabled ? TRUE : FALSE;

	CWnd* controls[] = {
		GetDlgItem(IDC_BUTTON_GENERATE),
		GetDlgItem(IDC_BUTTON_BROWSE)
	};

	for (CWnd* control : controls)
	{
		if (control != nullptr)
		{
			control->EnableWindow(flag);
		}
	}
}

// 「参照...」ボタン (FR-001, FR-002)。
void CMFCApplication1Dlg::OnBnClickedButtonBrowse()
{
	CFolderPickerDialog dialog(m_targetPath, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, this);

	// 007 FR-701: フォルダ選択ダイアログを開く直前にも再適用する。実機報告では
	// ダイアログを開いた瞬間にタイトルバーの色が既定色に戻ることがあった
	// (spec.md User Story 1 Acceptance Scenario 1 / research.md R-705)。
	ApplyCaptionColor();
	INT_PTR result = dialog.DoModal();

	// 007 FR-701, FR-702: 返った直後にも再適用する。選択・キャンセルのどちらで
	// 閉じた場合でも、早期 return より前に必ず呼ぶ (research.md R-702)。
	ApplyCaptionColor();

	if (result != IDOK)
	{
		// キャンセル時は表示中のパスを変更しない (FR-002 / US1-6 / DT-005)。
		// ここで m_targetPath を消すと、一度選んだフォルダが未選択に戻ってしまう。
		return;
	}

	m_targetPath = dialog.GetPathName();
	SetDlgItemText(IDC_EDIT_TARGET_PATH, m_targetPath);
}

// 「生成」ボタンの検証部 (FR-005〜FR-008, FR-015, FR-016)。
// contracts/ui-contract.md「3.2 生成ボタン」の順序で検証する。
// いずれかで中止する場合、ファイルシステムへの変更は一切行わない。
//
// 重要: この関数の中で生成ボタンを無効化してはならない。FR-014 の二重実行防止は
// ワーカースレッドが動作している間のみ適用される。ここで無効化すると、ユーザーが
// フォルダを選択してもボタンが押せなくなる (DT-003 の確認項目(d))。
bool CMFCApplication1Dlg::ValidateBeforeGenerate(TargetFolderState& state)
{
	// 003 FR-207: 検証チェーンの5段階とその順序は、診断の廃止によって
	// 変更してはならない。以下の順序は 001・002 から一切変えていない。

	// 1. フォルダ未選択 (US1-4 / DT-003)
	if (m_targetPath.IsEmpty())
	{
		AfxMessageBox(_T("対象フォルダを選択してください。"), MB_OK | MB_ICONINFORMATION);
		return false;
	}

	// 2. 書き込み権限 (001 FR-015)
	//    マーカーの確認より先に行う。権限が無いフォルダでマーカーを探しても意味がない。
	//    002 でこの順序を変えてはならない (002 research.md R-104(2))。
	if (!TargetFolder::IsWritable(m_targetPath))
	{
		AfxMessageBox(_T("対象フォルダに書き込みできません。フォルダの権限を確認してください。"),
					  MB_OK | MB_ICONWARNING);
		return false;
	}

	state = TargetFolder::Read(m_targetPath);

	// 3. マーカーの確認 (002 FR-103, FR-104, FR-105)
	//    種別を区別せず、認識可能なマーカーが1つ以上あるかだけを見る。
	//    複数種類のマーカーが共存していても警告は出さない (002 FR-105)。
	if (!state.hasProjectMarker)
	{
		// 本ツールは雛形を保持も生成もしない。ウィザードでの作成を促すに留める
		// (001 FR-007 / 002 FR-104)。
		AfxMessageBox(
			_T("対象のプロジェクトが見つかりません。\r\n")
			_T("Visual Studio 2022 などのウィザードでプロジェクトを作成し、")
			_T("このフォルダに配置してから再実行してください。"),
			MB_OK | MB_ICONINFORMATION);
		return false;
	}

	// 4. 足場が中途半端 (エッジケース)
	if (state.HasPartialScaffold())
	{
		AfxMessageBox(
			_T(".specify と .claude の一方だけが存在します。\r\n")
			_T("上書きせずに中止しました。手動で確認してください。"),
			MB_OK | MB_ICONWARNING);
		return false;
	}

	// 5. 入れ子リポジトリ (001 FR-016)
	if (state.gitState == GitState::NestedUnderParent)
	{
		CString message;
		message.Format(
			_T("親フォルダが既に Git リポジトリです（ルート: %s）。\r\n")
			_T("このまま初期化すると入れ子のリポジトリになります。\r\n\r\n続行しますか？"),
			static_cast<LPCTSTR>(state.gitRoot));
		if (AfxMessageBox(message, MB_YESNO | MB_ICONQUESTION) != IDYES)
		{
			return false;
		}
	}

	return true;
}

// 「生成」ボタン (FR-004〜FR-016)。
void CMFCApplication1Dlg::OnBnClickedButtonGenerate()
{
	if (m_running)
	{
		return;
	}

	TargetFolderState state;
	if (!ValidateBeforeGenerate(state))
	{
		// 検証で中止した。ボタンは有効なまま、ファイルの変更も行っていない。
		return;
	}

	ClearResultList();
	SetGuideText(_T("導入処理を実行しています..."));

	// ワーカーへ渡す値をここでコピーする。実行中にダイアログのメンバ変数を
	// 参照しないことで、UI の変更と処理対象の食い違いを防ぐ
	// (contracts/ui-contract.md「実行中に無効化するコントロール」)。
	m_workerPath = m_targetPath;
	m_steps = SetupRunner::CreateSteps();

	m_running = true;
	SetControlsEnabled(false);

	if (AfxBeginThread(SetupThreadProc, this) == nullptr)
	{
		// スレッドを起動できなかった場合も、必ずボタンを有効へ戻す。
		m_running = false;
		SetControlsEnabled(true);
		AfxMessageBox(_T("処理を開始できませんでした。"), MB_OK | MB_ICONERROR);
	}
}

UINT AFX_CDECL CMFCApplication1Dlg::SetupThreadProc(LPVOID param)
{
	CMFCApplication1Dlg* dialog = static_cast<CMFCApplication1Dlg*>(param);
	if (dialog != nullptr)
	{
		SetupRunner::RunAll(dialog->m_workerPath, dialog->m_steps, dialog);
		dialog->PostMessage(WM_APP_SETUP_COMPLETED, 0, 0);
	}
	return 0;
}

// ISetupProgressSink: ワーカースレッドから呼ばれる。
// UI を直接操作せず PostMessage で UI スレッドへ渡す (research.md R-002)。
void CMFCApplication1Dlg::OnStepFinished(const SetupStep& step)
{
	PostMessage(WM_APP_STEP_FINISHED, 0, reinterpret_cast<LPARAM>(new SetupStep(step)));
}

LRESULT CMFCApplication1Dlg::OnStepFinishedMessage(WPARAM, LPARAM lParam)
{
	SetupStep* step = reinterpret_cast<SetupStep*>(lParam);
	if (step != nullptr)
	{
		AppendResultRow(step->displayName,
						SetupRunner::ResultDisplayName(step->result),
						step->reason);
		delete step;
	}
	return 0;
}

LRESULT CMFCApplication1Dlg::OnSetupCompletedMessage(WPARAM, LPARAM)
{
	// 成功・失敗・中断のいずれでも、必ずボタンを有効へ戻す
	// (contracts/ui-contract.md「2. ボタンの有効・無効の契約」)。
	m_running = false;
	SetControlsEnabled(true);
	ShowCompletionGuide();
	return 0;
}

// 完了時の定型案内 (001 FR-011 / 003 FR-205)。
//
// 003 FR-205: 診断機能の廃止後も、この案内は残さなければならない。SDD化の後に
// 次の手順を伝える唯一の手段であり、002 FR-110〜112 で読みやすさとコピー可能性を
// 確保したばかりである。名前の似た診断結果向けの案内関数は 003 で削除したが、
// こちらは診断結果を参照しないため影響を受けない。
void CMFCApplication1Dlg::ShowCompletionGuide()
{
	bool anyFailed = false;
	for (const auto& step : m_steps)
	{
		if (step.result == StepResult::Failed)
		{
			anyFailed = true;
			break;
		}
	}

	CString guide;
	if (anyFailed)
	{
		guide = _T("一部の手順が失敗しました。一覧の「内容」欄で理由を確認してください。");
	}
	else
	{
		// FR-1201・FR-1202: 導入手順書(SpecKitVSCodesetup.pdf 手順08)では /init 実行の直後に
		// 生成物をコミットする。手順2と手順3(旧)の間にこのコミット手順を挿入し、以降の番号を
		// 1つずつ繰り下げる。既存の手順の文言・m_workerPath の埋め込み方法は変更しない。
		// FR-1204: コマンド表記は SetupRunner.cpp の RunInitGit が使う git add -A /
		// git commit -m "..." という既存の表記に合わせる。
		// FR-1203: この分岐は anyFailed が偽の場合のみであり、失敗時の案内文には触れない。
		guide.Format(
			_T("Spec Kit の導入が完了しました。次の手順は Claude Code との対話が必要です。\r\n")
			_T(" 1. VS Code で %s を開く（.specify と .claude が直下に見えるフォルダ）\r\n")
			_T(" 2. Claude パネルで /init を実行し、CLAUDE.md を生成する\r\n")
			_T(" 3. 生成された CLAUDE.md をコミットする（git add -A / git commit -m \"CLAUDE.md を追加\"）\r\n")
			_T(" 4. テスト方針を決め、/speckit-constitution で憲章を制定する\r\n")
			_T(" 5. /init を再実行し、憲章を反映した CLAUDE.md に更新する"),
			static_cast<LPCTSTR>(m_workerPath));
	}
	SetGuideText(guide);
}


/////////////////////////////////////////////////////////////////////////////
// 004: ダイアログのリサイズ追従
/////////////////////////////////////////////////////////////////////////////

// 追従テーブルの初期化 (004 FR-301 / data-model.md「テーブルの内容（8行）」)。
//
// OnInitDialog の最後から呼ぶ。各コントロールの初期矩形と、初期クライアントサイズ、
// 最小ウィンドウサイズを記録し、最後に m_layoutReady を立てる。
//
// 画面上の8コントロールを**すべて**登録する。1つでも漏れるとそのコントロールだけ
// 元の位置に取り残され、他と重なるかウィンドウの外へはみ出す。Fixed のラベルと
// 生成ボタンも省略しない。除外すると ST-303 が「意図的に固定」と「定義を忘れた」を
// 区別できなくなる (contracts/ui-contract.md「2. ラベルのID」)。
void CMFCApplication1Dlg::InitAnchors()
{
	m_anchors.clear();

	// 004 FR-302〜305: コントロールごとの追従方針。
	const AnchorEntry table[] = {
		{ IDC_STATIC_TARGET_LABEL, AnchorMode::Fixed,                CRect() },  // FR-305
		{ IDC_EDIT_TARGET_PATH,    AnchorMode::StretchH,             CRect() },  // FR-302
		{ IDC_BUTTON_BROWSE,       AnchorMode::AnchorRight,          CRect() },  // FR-304
		{ IDC_BUTTON_GENERATE,     AnchorMode::Fixed,                CRect() },  // FR-305
		{ IDC_LIST_RESULT,         AnchorMode::StretchBoth,          CRect() },  // FR-302, FR-303
		{ IDC_STATIC_GUIDE,        AnchorMode::AnchorBottomStretchH, CRect() },  // FR-302, FR-303
		{ IDOK,                    AnchorMode::AnchorBottomRight,    CRect() },  // FR-303, FR-304
		{ IDCANCEL,                AnchorMode::AnchorBottomRight,    CRect() }   // FR-303, FR-304
	};

	for (const AnchorEntry& src : table)
	{
		CWnd* control = GetDlgItem(src.id);
		if (control == nullptr)
		{
			continue;
		}

		AnchorEntry entry = src;
		control->GetWindowRect(&entry.initialRect);
		ScreenToClient(&entry.initialRect);
		m_anchors.push_back(entry);
	}

	CRect clientRect;
	GetClientRect(&clientRect);
	m_initialClientSize = CSize(clientRect.Width(), clientRect.Height());

	// 004 FR-306: 記録するのはウィンドウ全体のサイズ。ptMinTrackSize が
	// ウィンドウ全体の寸法を指すため、クライアントサイズを渡すと枠とタイトルバーの
	// 分だけ小さい下限になってしまう (research.md R-304)。
	CRect windowRect;
	GetWindowRect(&windowRect);
	m_minWindowSize = CSize(windowRect.Width(), windowRect.Height());

	// すべての記録が済んでから立てる。これ以前に OnSize が走ってはならない。
	m_layoutReady = true;
}

// 追従テーブルに従ってコントロールを再配置する (004 FR-302〜305)。
//
// dx / dy は**初期クライアントサイズからの差分**である。前回サイズからの相対計算に
// してはならない。相対計算は丸め誤差が累積し、拡大と縮小を繰り返すと位置がじわじわ
// ずれる。初期値からの絶対差分なら、どんな経路でリサイズしても同じサイズには同じ配置が
// 対応し、SC-304（元のサイズに戻すと配置も戻る）が構造的に保証される
// (research.md R-303)。
void CMFCApplication1Dlg::ApplyAnchors()
{
	if (!m_layoutReady || m_anchors.empty())
	{
		return;
	}

	CRect clientRect;
	GetClientRect(&clientRect);
	const int dx = clientRect.Width() - m_initialClientSize.cx;
	const int dy = clientRect.Height() - m_initialClientSize.cy;

	for (const AnchorEntry& entry : m_anchors)
	{
		CWnd* control = GetDlgItem(entry.id);
		if (control == nullptr)
		{
			continue;
		}

		CRect r = entry.initialRect;

		switch (entry.mode)
		{
		case AnchorMode::Fixed:
			continue;                                   // 何もしない (FR-305)
		case AnchorMode::StretchH:
			r.right += dx;                              // 幅だけ増減 (FR-302)
			break;
		case AnchorMode::AnchorRight:
			r.OffsetRect(dx, 0);                        // 右端との距離を保つ (FR-304)
			break;
		case AnchorMode::StretchBoth:
			r.right += dx;                              // 幅と高さの両方 (FR-302, FR-303)
			r.bottom += dy;
			break;
		case AnchorMode::AnchorBottomStretchH:
			r.OffsetRect(0, dy);                        // 下端との距離を保ち幅は増減
			r.right += dx;                              // (FR-302, FR-303)
			break;
		case AnchorMode::AnchorBottomRight:
			r.OffsetRect(dx, dy);                       // 右下との距離を保つ (FR-303, FR-304)
			break;
		}

		control->SetWindowPos(nullptr, r.left, r.top, r.Width(), r.Height(),
							  SWP_NOZORDER | SWP_NOACTIVATE);
	}

	// 004 FR-308: 一覧の幅が変わったので列幅も追従させる。
	UpdateResultListColumns();
}

// ウィンドウのサイズ変更 (004 FR-301, FR-310, FR-311)。
void CMFCApplication1Dlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// 004 FR-311: 最小化中は再配置しない。クライアント領域の幅と高さが 0 になり、
	// そのまま差分を計算するとコントロールの幅・高さが負になって復帰時に配置が
	// 破綻する。復帰時は SIZE_RESTORED で再度 OnSize が呼ばれるため支障はない
	// (contracts/ui-contract.md「5. 最小化時の早期リターン」)。
	if (nType == SIZE_MINIMIZED)
	{
		return;
	}

	// OnSize はダイアログの初期化中にも呼ばれる。その時点ではコントロールが未生成で
	// GetDlgItem が nullptr を返すため、フラグが立つまで何もしない (research.md R-301)。
	if (!m_layoutReady)
	{
		return;
	}

	// 004 FR-309: 導入処理の実行中にリサイズされても処理を中断しない。
	//
	// ワーカースレッドはコントロールを直接操作せず PostMessage で結果を送るだけなので
	// (001 research.md R-002)、この再配置と結果一覧への行追加は競合しない。
	// **追加の排他制御を導入してはならない** — 不要なロックは処理の停止を招く
	// (research.md R-305(1))。
	ApplyAnchors();
}

// ウィンドウの最小サイズ (004 FR-306, FR-307)。
//
// 下限は初期のウィンドウサイズと同一とする。初期サイズは 001 の設計時点で全コントロールが
// 収まるよう決めたものなので、これを下限にすれば FR-307（下限でもすべて操作可能）が
// 自動的に満たされる。
void CMFCApplication1Dlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	if (m_layoutReady && lpMMI != nullptr)
	{
		lpMMI->ptMinTrackSize.x = m_minWindowSize.cx;
		lpMMI->ptMinTrackSize.y = m_minWindowSize.cy;
	}

	CDialogEx::OnGetMinMaxInfo(lpMMI);
}
