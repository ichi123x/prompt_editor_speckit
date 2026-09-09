
// MFCApplication1Dlg.cpp : 実装ファイル
//

#include "pch.h"
#include "framework.h"
#include "MFCApplication1.h"
#include "MFCApplication1Dlg.h"
#include "afxdialogex.h"

#include "GitignoreWriter.h"
#include "ProcessRunner.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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
END_MESSAGE_MAP()


// CMFCApplication1Dlg メッセージ ハンドラー

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

	m_listResult.InsertColumn(0, _T("項目"), LVCFMT_LEFT, width * 40 / 100);
	m_listResult.InsertColumn(1, _T("結果"), LVCFMT_LEFT, width * 20 / 100);
	m_listResult.InsertColumn(2, _T("内容"), LVCFMT_LEFT, width * 40 / 100);
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
	if (dialog.DoModal() != IDOK)
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
		guide.Format(
			_T("Spec Kit の導入が完了しました。次の手順は Claude Code との対話が必要です。\r\n")
			_T(" 1. VS Code で %s を開く（.specify と .claude が直下に見えるフォルダ）\r\n")
			_T(" 2. Claude パネルで /init を実行し、CLAUDE.md を生成する\r\n")
			_T(" 3. テスト方針を決め、/speckit-constitution で憲章を制定する\r\n")
			_T(" 4. /init を再実行し、憲章を反映した CLAUDE.md に更新する"),
			static_cast<LPCTSTR>(m_workerPath));
	}
	SetGuideText(guide);
}
