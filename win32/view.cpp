// $Id$

#include "view.h"

#define VIEW_CLASSNAME  "BinViewerViewClass32"

View::View(HWND hwndParent, DWORD dwStyle, DWORD dwExStyle,
		   const RECT& rctWindow,
		   int nBytesPerLine, int nPixelsPerLine)
	: m_smHorz(NULL, SB_HORZ), m_smVert(NULL, SB_VERT),
	  m_hwndParent(hwndParent),
	  m_nBytesPerLine(nBytesPerLine),
	  m_nPixelsPerLine(nPixelsPerLine)
{
	static bool bRegisterClass;
	static WORD wNextID;

	if (!bRegisterClass) {
		WNDCLASS wc;
		::ZeroMemory(&wc, sizeof(wc));
		wc.hInstance = (HINSTANCE)::GetWindowLong(hwndParent, GWL_HINSTANCE);
		wc.style = CS_OWNDC | CS_SAVEBITS;
//		wc.hIcon = NULL;
		wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		wc.lpfnWndProc = (WNDPROC)View::viewWndProc;
		wc.lpszClassName = VIEW_CLASSNAME;
//		wc.lpszMenuName = NULL;
		if (!::RegisterClass(&wc)) {
			assert(0);
			throw RegisterClassError();
		}
		wNextID = 0x10;
		bRegisterClass = true;
	}

	m_nXOffset = 0;
	m_qYOffset = 0;
	m_mtView.width = rctWindow.right - rctWindow.left;
	m_mtView.height = rctWindow.bottom - rctWindow.top;

	m_hwndView = ::CreateWindowEx(WS_EX_CLIENTEDGE,
								  VIEW_CLASSNAME, "",
								  WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
								  rctWindow.left, rctWindow.top,
								  m_mtView.width,
								  m_mtView.height,
								  hwndParent, (HMENU)wNextID++,
								  wc.hInstance, (LPVOID)this);
	if (!m_hwndView) {
		throw CreateWindowError();
	}

	m_smHorz.setHWND(m_hwndView);
	m_smVert.setHWND(m_hwndView);

	m_hdcView = ::GetDC(m_hwndView);
}

View::~View()
{
}

void
View::bitBlt(const RECT& rcPaint)
{
	m_pDC_Manager->bitBlt(m_hdcView, rcPaint);
}

void
View::onVScroll(WPARAM wParam, LPARAM lParam)
{
	m_qYOffset = m_smVert.onScroll(LOWORD(wParam));

	m_pDC_Manager->ensureVisible(m_qYOffset,
								 count(m_mtView.height, m_nPixelsPerLine)
								  * m_nBytesPerLine);

}

void
View::onHScroll(WPARAM wParam, LPARAM lParam)
{
	// prepare the correct BGBuffer
	m_nXOffset = m_smHorz.onScroll(LOWORD(wParam));

	// get the region of BGBuffer to be drawn
	::InvalidateRect(m_hwndView, NULL, FALSE);
	::UpdateWindow(m_hwndView);
}

LRESULT CALLBACK
View::viewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CREATESTRUCT* pCs = (CREATESTRUCT*)lParam;
			::SetWindowLong(hWnd, GWL_USERDATA, (LONG)pCs->lpCreateParams);
			return 0;
		}
	case WM_NCCREATE:
		return TRUE;
	}

	View* This = (View*)::GetWindowLong(hWnd, GWL_USERDATA);
	if (!This) {
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg) {
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			::BeginPaint(hWnd, &ps);
			This->bitBlt(ps.rcPaint);
			::EndPaint(hWnd, &ps);
		}
		break;

	case WM_VSCROLL:
		This->onVScroll(wParam, lParam);
		break;

	case WM_HSCROLL:
		This->onHScroll(wParam, lParam);
		break;

	default:
		return This->viewWndProcMain(uMsg, wParam, lParam);
	}

	return 0;
}

