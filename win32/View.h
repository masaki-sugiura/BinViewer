// $Id$

#ifndef VIEW_H_INC
#define VIEW_H_INC

//#include "dc_manager.h"
#include "auto_ptr.h"
#include "scroll.h"

class DC_Manager;

struct Metrics {
	DWORD width;
	DWORD height;
};

class View {
public:
	View(HWND hwndParent, DWORD dwStyle, DWORD dwExStyle,
		 const RECT& rctWindow,
		 int nBytesPerLine,
		 int nPixelsPerLine);
	virtual ~View();

	// View クラスが投げる例外の基底クラス
	class ViewException {};
	// ウィンドウクラスの登録に失敗したときに投げられる例外
	class RegisterClassError : public ViewException {};
	class CreateWindowError : public ViewException {};

protected:
	Auto_Ptr<DC_Manager> m_pDC_Manager;
	int m_nBytesPerLine;
	int m_nPixelsPerLine;
	int m_nXOffset;
	filesize_t m_qYOffset;
	Metrics m_mtView;
	ScrollManager<int> m_smHorz;
	ScrollManager<filesize_t> m_smVert;
	bool m_bMapScrollBarLinearly;
	HWND m_hwndView, m_hwndParent;
	HDC m_hdcView;

	void bitBlt(const RECT& rcPaint);
	void onHScroll(WPARAM wParam, LPARAM lParam);
	void onVScroll(WPARAM wParam, LPARAM lParam);

	virtual LRESULT viewWndProcMain(UINT, WPARAM, LPARAM) = 0;

	static LRESULT CALLBACK viewWndProc(HWND, UINT, WPARAM, LPARAM);
};

#endif
