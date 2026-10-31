// MFCApplication1Dlg.h : ヘッダー ファイル
//

#pragma once

#include <vector>
#include "TargetFolderState.h"
#include "SetupRunner.h"

//不具合を確認するための定義
#define T_CHECK_BUG_01		//OK, CANCEL, ✖時のm_runningの状態確認

// ワーカースレッドからダイアログへ進捗を通知するためのメッセージ (research.md R-002)。
// SendMessage ではなく PostMessage を使い、ワーカーが UI スレッドの応答を待つことによる
// デッドロックを避ける。
#define WM_APP_STEP_FINISHED   (WM_APP + 1)
#define WM_APP_SETUP_COMPLETED (WM_APP + 2)

// 004 FR-301〜305: ウィンドウのサイズ変更に対する追従方針。
// 各コントロールはいずれか1つの方針を持つ (data-model.md「1. AnchorMode」)。
//
// dx / dy は「現在のクライアントサイズ − 初期クライアントサイズ」であり、
// 前回サイズからの相対値ではない。相対計算は丸め誤差が累積し、拡大と縮小を
// 繰り返すと位置がずれる。初期値からの絶対差分なら、同じウィンドウサイズには
// 常に同じ配置が対応する (004 SC-304 / research.md R-303)。
enum class AnchorMode
{
	Fixed,                 // 位置も大きさも変えない (FR-305)
	StretchH,              // 左上固定、幅だけ増減            w += dx
	AnchorRight,           // 右端との距離を保って移動 (FR-304) x += dx
	StretchBoth,           // 幅と高さの両方が増減            w += dx, h += dy
	AnchorBottomStretchH,  // 下端との距離を保ち幅は増減       y += dy, w += dx
	AnchorBottomRight      // 右端・下端との距離を保って移動    x += dx, y += dy
};

// 004: 追従テーブルの1行 (data-model.md「2. AnchorEntry」)。
// initialRect は OnInitDialog で記録し、以後変更しない。
struct AnchorEntry
{
	int id = 0;
	AnchorMode mode = AnchorMode::Fixed;
	CRect initialRect;
};

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
#ifdef	T_CHECK_BUG_01
	virtual void OnOK() override;		// OK ボタン押下時の動作を確認するためオーバーライド
	virtual void OnCancel() override;	// Cancel ボタン押下時の動作を確認する
#endif
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
	bool m_workerRegisterRepo = true;   // 016 R-1602: ワーカーへ渡す選択のコピー。既定 true = 「OK」

	// --- 004: リサイズ追従の状態 ---
	std::vector<AnchorEntry> m_anchors;   // 追従テーブル。8コントロールすべてを登録する
	CSize m_initialClientSize;            // 初期のクライアントサイズ。dx / dy の基準
	CSize m_minWindowSize;                // ウィンドウ全体の最小サイズ (FR-306)
	                                      // クライアントではなくウィンドウ全体。
	                                      // ptMinTrackSize がウィンドウ全体の寸法を指すため
	bool m_layoutReady = false;           // OnInitDialog が完了したか。
	                                      // false の間 OnSize は何もしない (research.md R-301)

	// --- イベントハンドラ ---
	afx_msg void OnBnClickedButtonBrowse();     // 001 FR-001, FR-002
	afx_msg void OnBnClickedButtonGenerate();   // 001 FR-004〜FR-016 / 002 FR-102, FR-103, FR-105
	afx_msg LRESULT OnStepFinishedMessage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSetupCompletedMessage(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSize(UINT nType, int cx, int cy);              // 004 FR-301, FR-310, FR-311
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);              // 004 FR-306, FR-307

	// --- ヘルパー ---
	void ApplyCaptionColor();                                // 005 FR-401〜404 / 007 FR-701, FR-702（DoModal() 前後でも呼ぶ）
	void InitResultList();                                   // 001 FR-011, FR-019
	void ClearResultList();
	void AppendResultRow(const CString& name, const CString& status, const CString& detail);
	void SetGuideText(const CString& text);
	void InitAnchors();                                      // 004 FR-301（追従テーブルの初期化）
	void ApplyAnchors();                                     // 004 FR-302〜305（再配置）
	void UpdateResultListColumns();                          // 004 FR-308（列幅の追従）
	void SetControlsEnabled(bool enabled);                   // 002 contracts「3.2 実行中に無効化するコントロール」
	bool ValidateBeforeGenerate(TargetFolderState& state);    // 002 FR-102, FR-103 / 001 FR-015, FR-016
	void ShowCompletionGuide();                              // 001 FR-011（定型の完了メッセージ）

	static UINT AFX_CDECL SetupThreadProc(LPVOID param);
};
