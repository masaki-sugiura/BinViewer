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
		 int nWidth, int nHeight,
		 int nPixelsPerLine,
		 COLORREF crBkColor);
	virtual ~View();

	// View クラスが投げる例外の基底クラス
	class ViewException {};
	// ウィンドウクラスの登録に失敗したときに投げられる例外
	class RegisterClassError : public ViewException {};
	class CreateWindowError : public ViewException {};

protected:
	DC_Manager* m_pDCManager;
	int m_nPixelsPerLine;
	int m_nXOffset;
	filesize_t m_qYOffset;
	ScrollManager<int> m_smHorz;
	ScrollManager<filesize_t> m_smVert;
	bool m_bMapScrollBarLinearly;
	HWND m_hwndView, m_hwndParent;
	HDC m_hdcView;
	HBRUSH m_hbrBackground;
	bool m_bOverlapped;

	bool onLoadFile();
	void onUnloadFile();

	void bitBlt(const RECT& rcPaint);

	void onHScroll(WPARAM wParam, LPARAM lParam);
	void onVScroll(WPARAM wParam, LPARAM lParam);

	virtual LRESULT viewWndProcMain(UINT, WPARAM, LPARAM) = 0;

	static LRESULT CALLBACK viewWndProc(HWND, UINT, WPARAM, LPARAM);
};

#endif
