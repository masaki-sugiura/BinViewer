// $Id$

#include "view.h"

#define VIEW_CLASSNAME  "BinViewerViewClass32"

View::View(HWND hwndParent, DWORD dwStyle, DWORD dwExStyle,
		   const RECT& rctWindow,
		   DC_Manager* pDCManager,
		   int nWidth, int nHeight,
		   int nPixelsPerLine,
		   COLORREF crBkColor)
	: m_pDCManager(pDCManager),
	  m_nPixelsPerLine(nPixelsPerLine),
	  m_nXOffset(0), m_nTopOffset(0), m_qYOffset(0),
	  m_smHorz(NULL, SB_HORZ), m_smVert(NULL, SB_VERT),
	  m_bMapScrollBarLinearly(true),
	  m_hwndParent(hwndParent),
	  m_bOverlapped(false),
	  m_pCurBuf(NULL), m_pNextBuf(NULL)
{
	static bool bRegisterClass;
	static WORD wNextID;
	HINSTANCE hInstance = (HINSTANCE)::GetWindowLong(hwndParent, GWL_HINSTANCE);

	if (!bRegisterClass) {
		WNDCLASS wc;
		::ZeroMemory(&wc, sizeof(wc));
		wc.hInstance = hInstance;
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

	m_hwndView = ::CreateWindowEx(WS_EX_CLIENTEDGE,
								  VIEW_CLASSNAME, "",
								  WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
								  rctWindow.left, rctWindow.top,
								  rctWindow.right - rctWindow.left,
								  rctWindow.bottom - rctWindow.top,
								  hwndParent, (HMENU)wNextID++,
								  hInstance, (LPVOID)this);
	if (!m_hwndView) {
		throw CreateWindowError();
	}

	m_smHorz.setHWND(m_hwndView);
	m_smVert.setHWND(m_hwndView);

	m_hdcView = ::GetDC(m_hwndView);

	m_hbrBackground = ::CreateSolidBrush(crBkColor);

	m_pDCManager->setDCInfo(m_hdcView, nWidth, nHeight, m_hbrBackground);

	m_nBytesPerLine = m_pDCManager->bufSize() * nPixelsPerLine / nHeight;
}

View::~View()
{
	::DeleteObject(m_hbrBackground);
	::SetWindowLong(m_hwndView, GWL_USERDATA, 0);
}

bool
View::onLoadFile()
{
	if (!m_pDCManager->onLoadFile(this)) return false;
	m_smHorz.setPosition(0);
	m_smVert.setPosition(0);
	setCurrentLine(0, true);
	return true;
}

void
View::onUnloadFile()
{
	m_pDCManager->onUnloadFile();
	setCurrentLine(0, true);
}

void
View::bitBlt(const RECT& rcPaint)
{
	int nHeaderHeight = 0;

	int nXOffset = m_smHorz.getCurrentPos();
	if (m_bOverlapped) {
		int height = m_pDCManager->height() - m_nTopOffset;

		assert(m_pCurBuf);

		if (rcPaint.bottom - nHeaderHeight <= height) {
			m_pCurBuf->bitBlt(m_hdcView,
							  rcPaint.left, rcPaint.top,
							  rcPaint.right - rcPaint.left,
							  rcPaint.bottom - rcPaint.top,
							  rcPaint.left + nXOffset,
							  m_nTopOffset + rcPaint.top - nHeaderHeight);
		} else if (rcPaint.top - nHeaderHeight >= height) {
			if (m_pNextBuf) {
				m_pNextBuf->bitBlt(m_hdcView,
								   rcPaint.left, rcPaint.top,
								   rcPaint.right - rcPaint.left,
								   rcPaint.bottom - rcPaint.top,
								   rcPaint.left + nXOffset,
								   rcPaint.top - height - nHeaderHeight);
			} else {
				::FillRect(m_hdcView, &rcPaint, m_hbrBackground);
			}
		} else {
			m_pCurBuf->bitBlt(m_hdcView,
							  rcPaint.left, rcPaint.top,
							  rcPaint.right - rcPaint.left,
							  height - rcPaint.top + nHeaderHeight,
							  rcPaint.left + nXOffset,
							  rcPaint.top + m_nTopOffset - nHeaderHeight);
			if (m_pNextBuf) {
				m_pNextBuf->bitBlt(m_hdcView,
								   rcPaint.left, height + nHeaderHeight,
								   rcPaint.right - rcPaint.left,
								   rcPaint.bottom - height,
								   rcPaint.left + nXOffset, 0);
			} else {
				RECT rctTemp = rcPaint;
				rctTemp.top    = height + nHeaderHeight;
				rctTemp.bottom = rcPaint.bottom;
				::FillRect(m_hdcView, &rctTemp, m_hbrBackground);
			}
		}
	} else if (m_pCurBuf) {
		m_pCurBuf->bitBlt(m_hdcView,
						  rcPaint.left, rcPaint.top,
						  rcPaint.right - rcPaint.left,
						  rcPaint.bottom - rcPaint.top,
						  rcPaint.left + nXOffset,
						  m_nTopOffset + rcPaint.top - nHeaderHeight);
	} else {
		::FillRect(m_hdcView, &rcPaint, (HBRUSH)(COLOR_APPWORKSPACE + 1));
	}
}

void
View::ensureVisible(filesize_t pos, bool bRedraw)
{
	filesize_t newline = pos / m_nBytesPerLine;
	filesize_t line_diff = newline - m_smVert.getCurrentPos();
	filesize_t valid_page_line_num = m_smVert.getGripWidth() - 1;
	if (line_diff < 0) {
		m_smVert.setPosition(newline);
		setCurrentLine(newline, bRedraw);
	} else if (line_diff >= valid_page_line_num) {
		newline -= valid_page_line_num;
		m_smVert.setPosition(newline);
		setCurrentLine(newline, bRedraw);
	}
}

void
View::setCurrentLine(filesize_t newline, bool bRedraw)
{
	if (!m_pDCManager->isLoaded()) {
		m_nTopOffset = 0;
		m_bOverlapped = false;
		m_pCurBuf = m_pNextBuf = NULL;
		return;
	}

	filesize_t qMaxLine = m_smVert.getMaxPos();
	if (newline > qMaxLine) {
		newline = qMaxLine;
	} else if (newline < 0) {
		newline = 0;
	}

	// DCBuffer の内容を表示領域に変更する
	filesize_t buffer_top = m_pDCManager->setPosition(newline * m_nBytesPerLine);

	m_nTopOffset = (int)(newline - buffer_top / m_nBytesPerLine) * m_nPixelsPerLine;
	assert(m_nTopOffset < m_pDCManager->height());

	// 次のバッファとオーバーラップしているかどうか
	RECT rctClient;
	::GetClientRect(m_hwndView, &rctClient);
	int actual_linenum = (rctClient.bottom - rctClient.top + m_nPixelsPerLine - 1)
							/ m_nPixelsPerLine - 1;
	m_bOverlapped = isOverlapped(m_nTopOffset / m_nPixelsPerLine, actual_linenum);

	m_pCurBuf  = m_pDCManager->getCurrentBuffer(0);
	m_pNextBuf = m_pDCManager->getCurrentBuffer(1);
}

void
View::setPosition(filesize_t pos, bool bRedraw)
{
	if (!m_pDCManager->isLoaded()) return;

	filesize_t fsize = m_pDCManager->getFileSize();
	if (fsize == 0) return;

	if (pos >= fsize)
		pos = fsize - 1;
	if (pos < 0) pos = 0; // else では NG!! (filesize == 0 の場合)
}

void
View::adjustWindowRect(RECT& rctFrame)
{
	RECT rctWindow, rctClient;
	::GetWindowRect(m_hwndView, &rctWindow);
	::GetClientRect(m_hwndView, &rctClient);

	rctFrame.left = rctFrame.top = 0;
	rctFrame.right = rctWindow.right - rctWindow.left - rctClient.right
					 + m_pDCManager->width();
	rctFrame.bottom = ((rctFrame.bottom + m_nPixelsPerLine - 1) / m_nPixelsPerLine)
					   * m_nPixelsPerLine
					+ (rctWindow.bottom - rctWindow.top - rctClient.bottom);
}

void
View::setFrameRect(const RECT& rctFrame)
{
	::SetWindowPos(m_hwndView, NULL,
				   rctFrame.left, rctFrame.top,
				   rctFrame.right - rctFrame.left,
				   rctFrame.bottom - rctFrame.top,
				   SWP_NOZORDER);

	int nPageLineNum = (rctFrame.bottom - rctFrame.top) / m_nPixelsPerLine;

	// ウィンドウを広げた結果次のバッファとオーバーラップした場合に必要
	m_bOverlapped = isOverlapped(m_nTopOffset / m_nPixelsPerLine, nPageLineNum);
	m_smHorz.setInfo(m_pDCManager->width(),
					 (rctFrame.right - rctFrame.left),
					 m_smHorz.getCurrentPos());
	filesize_t size = m_pDCManager->getFileSize();
	if (size < 0)
		m_smVert.disable();
	else
		m_smVert.setInfo(size / m_nBytesPerLine, nPageLineNum, m_smVert.getCurrentPos());
}

void
View::onVScroll(WPARAM wParam, LPARAM lParam)
{
	m_qYOffset = m_smVert.onScroll(LOWORD(wParam));

	// 表示領域の更新
	setCurrentLine(m_qYOffset, false);
}

void
View::onHScroll(WPARAM wParam, LPARAM lParam)
{
	// prepare the correct BGBuffer
	m_nXOffset = m_smHorz.onScroll(LOWORD(wParam));
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

LRESULT
View::viewWndProcMain(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return ::DefWindowProc(m_hwndView, uMsg, wParam, lParam);
}
