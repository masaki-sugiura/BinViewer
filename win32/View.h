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

class BGB_Manager;

struct Metrics {
	DWORD width;
	DWORD height;
};

class View : public LF_Acceptor {
public:
	View(HWND hwndParent, DWORD dwStyle, DWORD dwExStyle,
		 const RECT& rctWindow,
		 DC_Manager* pDCManager,
		 DrawInfo* pDrawInfo,
		 int nPixelsPerLine);
	virtual ~View();

	virtual void ensureVisible(filesize_t pos, bool bRedraw);
	virtual void setCurrentLine(filesize_t newline, bool bRedraw);
	virtual void setPosition(filesize_t pos, bool bRedraw);

	// 
	virtual void setFrameRect(const RECT& rctFrame);

	// ウィンドウサイズを行高・文字幅の整数倍に調整
	void adjustWindowRect(RECT& rctFrame);

	// View クラスが投げる例外の基底クラス
	class ViewException {};
	// ウィンドウクラスの登録に失敗したときに投げられる例外
	class RegisterClassError : public ViewException {};
	// ウィンドウの作成に失敗したときに投げられる例外
	class CreateWindowError : public ViewException {};

protected:
	DC_Manager* m_pDCManager;
	DrawInfo* m_pDrawInfo;
	int m_nPixelsPerLine;
	int m_nBytesPerLine;
	filesize_t m_qYOffset;
	ScrollManager<int> m_smHorz;
	ScrollManager<filesize_t> m_smVert;
	HWND m_hwndView, m_hwndParent;

	bool onLoadFile();
	void onUnloadFile();

	void bitBlt(const RECT& rcPaint);

	virtual void onHScroll(WPARAM wParam, LPARAM lParam);
	virtual void onVScroll(WPARAM wParam, LPARAM lParam);

	virtual LRESULT viewWndProcMain(UINT, WPARAM, LPARAM);

	static LRESULT CALLBACK viewWndProc(HWND, UINT, WPARAM, LPARAM);
};

#endif
