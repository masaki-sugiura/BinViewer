// $Id$

#ifndef VIEW_H_INC
#define VIEW_H_INC

#include "dc_manager.h"
#include "scroll.h"

class View {
public:
	View(HWND hwndParent, DWORD dwStyle, DWORD dwExStyle,
		 const RECT& rctWindow);
	virtual ~View();

protected:
	Auto_Ptr<DC_Manager> m_pDC_Manager;
	RECT m_rctView, m_rctClient;
	ScrollManager<int> m_smHorz;
	ScrollManager<filesize_t> m_smVert;
	int m_nXOffset, m_nYOffset;
	DCBuffer *m_pCurBuf, *m_pNextBuf;
	filesize_t m_qCurrentPos;
	HWND m_hwndView, m_hwndParent;
	HDC m_hDC;
	bool m_bMapScrollBarLinearly;

	virtual void bitBlt(const RECT& rcPaint);

	virtual LRESULT viewWndProcMain(UINT, WPARAM, LPARAM) = 0;

	static LRESULT CALLBACK viewWndProc(HWND, UINT, WPARAM, LPARAM);
};

#endif
