// $Id$

#ifndef VIEW_H_INC
#define VIEW_H_INC

/*
	DC_Manager を利用するウィンドウを表す抽象基底クラス

	・画面の描画（バックグラウンドバッファからのビットマップ転送）
	・描画領域のサイズ管理
	・カーソル位置の管理
	・マウスイベントのハンドリング
	・キー操作のハンドリング

						+-----------------------+
						|						|
	+-----------+		|	+-----------+		|
	|			|		|	|			|		|
	|			|		|	|			|		|
	+-----------+		|	+-----------+		|
						|						|
						+-----------------------+

 */

#include "auto_ptr.h"
#include "scroll.h"
#include "dc_manager.h"
#include "DrawInfo.h"

class View : public LF_Acceptor {
public:
	View(HWND hwndParent, DWORD dwStyle, DWORD dwExStyle,
		 const RECT& rctWindow,
		 DC_Manager* pDCManager,
		 DrawInfo* pDrawInfo);
	virtual ~View();

	// ウィンドウを指定された短形に変形
	void setFrameRect(const RECT& rctFrame, bool bRedraw);
	// ウィンドウ短形を取得
	void getFrameRect(RECT& rctFrame);

	// クライアント領域を指定サイズに変更
	void setViewSize(int width, int height);

	// ウィンドウサイズを行高・文字幅の整数倍に調整
	void adjustWindowRect(RECT& rctFrame);

	void redrawView()
	{
		::InvalidateRect(m_hwndView, NULL, FALSE);
		::UpdateWindow(m_hwndView);
	}

	bool onLoadFile();
	void onUnloadFile();
	void onSetCursorPos(filesize_t pos);

	void onMouseWheel(short nMouseMove);
	void onHScroll(WPARAM wParam, LPARAM lParam);
	void onVScroll(WPARAM wParam, LPARAM lParam);
	void onHorizontalMove(int nMove);
	void onVerticalMove(int nMove);

	virtual bool setDrawInfo(DrawInfo* pDrawInfo);

	// View クラスが投げる例外の基底クラス
	class ViewException {};
	// ウィンドウクラスの登録に失敗したときに投げられる例外
	class RegisterClassError : public ViewException {};
	// ウィンドウの作成に失敗したときに投げられる例外
	class CreateWindowError : public ViewException {};

protected:
	DC_Manager* m_pDCManager;
	DrawInfo* m_pDrawInfo;
	int m_nBytesPerLine;
	filesize_t m_qYOffset;
	ScrollManager<int> m_smHorz;
	ScrollManager<filesize_t> m_smVert;
	HWND m_hwndView, m_hwndParent;

	void bitBlt(const RECT& rcPaint);

	void initScrollInfo();

	void ensureVisible(filesize_t pos, bool bRedraw);
	void setCurrentLine(filesize_t newline, bool bRedraw);

	void onLButtonDown(WPARAM wParam, LPARAM lParam);

	virtual LRESULT viewWndProcMain(UINT, WPARAM, LPARAM);

	static LRESULT CALLBACK viewWndProc(HWND, UINT, WPARAM, LPARAM);
};

#endif
