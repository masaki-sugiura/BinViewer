// $Id$

#include "view.h"

#define VIEW_CLASSNAME  "BinViewerViewClass32"

View::View(LF_Notifier& lfNotifier,
		   HWND hwndParent, DWORD dwStyle, DWORD dwExStyle,
		   const RECT& rctWindow,
		   DC_Manager* pDCManager,
		   DrawInfo* pDrawInfo)
	: LF_Acceptor(lfNotifier),
	  m_pDCManager(pDCManager),
	  m_pDrawInfo(pDrawInfo),
	  m_qYOffset(0),
	  m_smHorz(NULL, SB_HORZ), m_smVert(NULL, SB_VERT),
	  m_hwndParent(hwndParent)
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

	int nViewWidth  = rctWindow.right - rctWindow.left,
		nViewHeight = rctWindow.bottom - rctWindow.top;

	m_hwndView = ::CreateWindowEx(dwExStyle,
								  VIEW_CLASSNAME, "",
								  dwStyle,
								  rctWindow.left, rctWindow.top,
								  nViewWidth, nViewHeight,
								  hwndParent, (HMENU)wNextID++,
								  hInstance, (LPVOID)this);
	if (!m_hwndView) {
		throw CreateWindowError();
	}

	if (dwStyle & WS_HSCROLL) {
		m_smHorz.setHWND(m_hwndView);
	}
	if (dwStyle & WS_VSCROLL) {
		m_smVert.setHWND(m_hwndView);
	}

	m_pDrawInfo->setDC(::GetDC(m_hwndView));

	if (!m_pDCManager->setDrawInfo(m_pDrawInfo)) {
		throw CreateWindowError();
	}

	m_nBytesPerLine = m_pDCManager->bufSize() * m_pDrawInfo->getPixelsPerLine() / m_pDrawInfo->getHeight();

	m_pDCManager->setViewRect(0, 0, nViewWidth, nViewHeight);
}

View::~View()
{
//	::SetWindowLong(m_hwndView, GWL_USERDATA, 0);
	if (m_hwndView) {
		::DestroyWindow(m_hwndView);
	}
}

bool
View::onLoadFile()
{
	if (!m_pDCManager->onLoadFile(this)) return false;
	RECT rctView;
	::GetWindowRect(m_hwndView, &rctView);
	setFrameRect(rctView, false);
	m_smHorz.setPosition(0);
	m_smVert.setPosition(0);
	setCurrentLine(0, true);
	return true;
}

void
View::onUnloadFile()
{
	m_pDCManager->onUnloadFile();
	RECT rctView;
	::GetWindowRect(m_hwndView, &rctView);
	setFrameRect(rctView, false);
	setCurrentLine(0, true);
}

void
View::ensureVisible(filesize_t pos, bool bRedraw)
{
	filesize_t newline = pos / m_nBytesPerLine;
	filesize_t line_diff = newline - m_smVert.getCurrentPos();
	filesize_t valid_page_line_num = m_smVert.getGripWidth();
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
		return;
	}

	filesize_t qMaxLine = m_smVert.getMaxPos();
	if (newline > qMaxLine) {
		newline = qMaxLine;
	} else if (newline < 0) {
		newline = 0;
	}

	m_pDCManager->setViewPositionY(newline * m_pDrawInfo->getPixelsPerLine());

	if (bRedraw) {
		redrawView();
	}
}

void
View::setPosition(filesize_t pos, bool bRedraw)
{
	if (!m_pDCManager->isLoaded()) {
		return;
	}

	filesize_t fsize = m_pDCManager->getFileSize();
	if (fsize == 0) return;

	if (pos >= fsize)
		pos = fsize - 1;
	if (pos < 0) pos = 0; // else �ł� NG!! (filesize == 0 �̏ꍇ)

	setCurrentLine(pos / m_nBytesPerLine, bRedraw);
}

void
View::adjustWindowRect(RECT& rctFrame)
{
	RECT rctWindow, rctClient;
	::GetWindowRect(m_hwndView, &rctWindow);
	::GetClientRect(m_hwndView, &rctClient);

	int nPixelsPerLine = m_pDrawInfo->getPixelsPerLine();

	rctFrame.left = rctFrame.top = 0;
	rctFrame.right = rctWindow.right - rctWindow.left - rctClient.right
					 + m_pDCManager->width();
	rctFrame.bottom = ((rctFrame.bottom + nPixelsPerLine - 1) / nPixelsPerLine)
					   * nPixelsPerLine
					+ (rctWindow.bottom - rctWindow.top - rctClient.bottom);
}

void
View::setFrameRect(const RECT& rctFrame, bool bRedraw)
{
	::SetWindowPos(m_hwndView, NULL,
				   rctFrame.left, rctFrame.top,
				   rctFrame.right - rctFrame.left,
				   rctFrame.bottom - rctFrame.top,
				   SWP_NOZORDER);

	RECT rctClient;
	::GetClientRect(m_hwndView, &rctClient);

	int nViewWidth  = rctClient.right - rctClient.left,
		nViewHeight = rctClient.bottom - rctClient.top;

	m_pDCManager->setViewSize(nViewWidth, nViewHeight);

	m_smHorz.setInfo(m_pDCManager->width(),
					 nViewWidth,
					 m_smHorz.getCurrentPos());

	int nPageLineNum = nViewHeight / m_pDrawInfo->getPixelsPerLine();
	filesize_t size = m_pDCManager->getFileSize();
	if (size < 0)
		m_smVert.disable();
	else
		m_smVert.setInfo(size / m_nBytesPerLine, nPageLineNum, m_smVert.getCurrentPos());

	if (bRedraw) {
		redrawView();
	}
}

void
View::getFrameRect(RECT& rctFrame)
{
	::GetWindowRect(m_hwndView, &rctFrame);
}

void
View::setViewSize(int width, int height)
{
	RECT rctWindow, rctClient;
	::GetWindowRect(m_hwndView, &rctWindow);
	::GetClientRect(m_hwndView, &rctClient);

	if (width > 0) {
		rctWindow.right = rctWindow.left + width
						+ (rctWindow.right - rctWindow.left)
						- (rctClient.right - rctClient.left);
	} else {
		width = rctClient.right - rctClient.left;
	}
	if (height > 0) {
		rctWindow.bottom = rctWindow.top + height
						+ (rctWindow.bottom - rctWindow.top)
						- (rctClient.bottom - rctClient.top);
	} else {
		height = rctClient.bottom - rctClient.top;
	}

	::SetWindowPos(m_hwndView, NULL,
				   rctWindow.left, rctWindow.top,
				   rctWindow.right - rctWindow.left,
				   rctWindow.bottom - rctWindow.top,
				   SWP_NOZORDER);

	m_pDCManager->setViewSize(width, height);
}

void
View::onVScroll(WPARAM wParam, LPARAM lParam)
{
	// �\���̈�̍X�V
	setCurrentLine(m_smVert.onScroll(LOWORD(wParam)), true);
}

void
View::onHScroll(WPARAM wParam, LPARAM lParam)
{
	// �\���̈�̍X�V
	m_pDCManager->setViewPositionX(m_smHorz.onScroll(LOWORD(wParam)));
	redrawView();
}

void
View::onLButtonDown(WPARAM wParam, LPARAM lParam)
{
	if (!m_pDCManager->isLoaded()) return;

	m_pDCManager->setCursorByViewCoordinate(MAKEPOINTS(lParam));

	redrawView();
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
			This->m_pDCManager->bitBlt(ps.hdc, ps.rcPaint);
			::EndPaint(hWnd, &ps);
		}
		break;

	case WM_VSCROLL:
		This->onVScroll(wParam, lParam);
		break;

	case WM_HSCROLL:
		This->onHScroll(wParam, lParam);
		break;

	case WM_LBUTTONDOWN:
		This->onLButtonDown(wParam, lParam);
		break;

	case WM_DROPFILES:
		::SendMessage(::GetParent(hWnd), uMsg, wParam, lParam);
		break;

	case WM_DESTROY:
		This->m_hwndView = NULL;
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
