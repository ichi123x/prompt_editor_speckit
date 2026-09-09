// MFCApplication1Dlg.h : ヘッダー ファイル
//

#pragma once

#include <vector>
#include "TargetFolderState.h"
#include "SetupRunner.h"

// ワーカースレッドからダイアログへ進捗を通知するためのメッセージ (research.md R-002)。
// SendMessage ではなく PostMessage を使い、ワーカーが UI スレッドの応答を待つことによる
// デッドロックを避ける。
#define WM_APP_STEP_FINISHED   (WM_APP + 1)
#define WM_APP_SETUP_COMPLETED (WM_APP + 2)

// CMFCApplication1Dlg ダイアログ
class CMFCApplication1Dlg : public CDialogEx, public ISetupProgressSink
{
// コンストラクション
public:
	CMFCApplication1Dlg(CWnd* pParent = nullptr);	// 標準コンストラクター

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFCAPPLICATION1_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV サポート


// 実装
protected:
	HICON m_hIcon;

	// 生成された、メッセージ割り当て関数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

// SDDプロジェクトのベース作成ツール
public:
	// ISetupProgressSink: ワーカースレッドから呼ばれる。UI は直接触らず PostMessage する。
	virtual void OnStepFinished(const SetupStep& step) override;

protected:
	// --- コントロール ---
	CListCtrl m_listResult;      // 手順結果と診断結果を共用する一覧 (research.md R-009)

	// --- 状態 ---
	CString m_targetPath;        // 選択された対象フォルダ。空なら未選択 (001 FR-002)
	bool m_running = false;      // 導入処理の実行中か (001 FR-014)
	// 002 FR-101: 種別を保持するメンバは廃止。判定は /speckit-constitution が行う。

	// ワーカースレッドが操作する。UI スレッドは実行中に触らない。
	std::vector<SetupStep> m_steps;
	CString m_workerPath;        // ワーカーへ渡す対象フォルダのコピー

	// --- イベントハンドラ ---
	afx_msg void OnBnClickedButtonBrowse();     // 001 FR-001, FR-002
	afx_msg void OnBnClickedButtonGenerate();   // 001 FR-004〜FR-016 / 002 FR-102, FR-103, FR-105
	afx_msg LRESULT OnStepFinishedMessage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSetupCompletedMessage(WPARAM wParam, LPARAM lParam);

	// --- ヘルパー ---
	void InitResultList();                                   // 001 FR-011, FR-019
	void ClearResultList();
	void AppendResultRow(const CString& name, const CString& status, const CString& detail);
	void SetGuideText(const CString& text);
	void SetControlsEnabled(bool enabled);                   // 002 contracts「3.2 実行中に無効化するコントロール」
	bool ValidateBeforeGenerate(TargetFolderState& state);    // 002 FR-102, FR-103 / 001 FR-015, FR-016
	void ShowCompletionGuide();                              // 001 FR-011（定型の完了メッセージ）

	static UINT AFX_CDECL SetupThreadProc(LPVOID param);
};
