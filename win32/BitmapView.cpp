// $Id$

#include "BitmapView.h"

BV_DrawInfo::BV_DrawInfo()
{
	initDrawInfo();
}

void
BV_DrawInfo::initDrawInfo()
{
	setWidth(BV_WIDTH);
	setHeight(BV_HEIGHT);
	setPixelsPerLine(1);
	setBkColor(RGB(255, 255, 255));
}

BV_DCBuffer::BV_DCBuffer(int nBufSize)
	: DCBuffer(nBufSize),
	  m_pBVDrawInfo(NULL)
{
	m_hbrSelectMask = ::CreateSolidBrush(RGB(0, 255, 255));
}

BV_DCBuffer::~BV_DCBuffer()
{
	::DeleteObject(m_hbrSelectMask);
}

bool
BV_DCBuffer::prepareDC(DrawInfo* pDrawInfo)
{
	BV_DrawInfo* pBVDrawInfo = dynamic_cast<BV_DrawInfo*>(pDrawInfo);
	if (!pBVDrawInfo) {
		return false;
	}

	if (!DCBuffer::prepareDC(pDrawInfo)) {
		return false;
	}

	m_pBVDrawInfo = pBVDrawInfo;

	return true;
}

int
BV_DCBuffer::render()
{
	assert(m_pBVDrawInfo);

	int width = m_pBVDrawInfo->getWidth(), height = m_pBVDrawInfo->getHeight();

	assert(width * height == m_nBufSize);

	int x = 0, y = 0;
	for (int i = 0; i < m_nDataSize; i++) {
		BYTE val = 255 - m_pDataBuf[i]; // 0 が白になるように
		::SetPixelV(m_hDC, x, y, RGB(val, val, val));
		if (++x == width) {
			x = 0; y++;
		}
	}

	COLORREF crBkColor = m_pBVDrawInfo->getBkColor();
	while (y < height) {
		::SetPixelV(m_hDC, x, y, crBkColor);
		if (++x == width) {
			x = 0; y++;
		}
	}

	return m_nDataSize;
}

int
BV_DCBuffer::setCursorByCoordinate(int x, int y)
{
	int offset = getPositionByCoordinate(x, y);
	if (offset >= 0) {
//		invertRegionInBuffer(offset, 1);
		setCursor(offset);
	}

	return offset;
}

int
BV_DCBuffer::getPositionByCoordinate(int x, int y) const
{
	// バッファのデータは不正
	if (m_qAddress < 0) {
		return -1;
	}

	assert(m_pBVDrawInfo);

	return x + y * m_pBVDrawInfo->getWidth();
}

void
BV_DCBuffer::invertRegionInBuffer(int offset, int size)
{
	// バッファのデータは不正
	if (m_qAddress < 0) {
		return;
	}

	int width = m_pBVDrawInfo->getWidth(), height = m_pBVDrawInfo->getHeight();

	int start_line = offset / width, start_column = offset % width,
		end_line = (offset + size) / width, end_column = (offset + size) % width;

	if (end_column == 0) {
		assert(end_line > 0);
		end_line--;
		end_column = width;
	}

	if (start_line == end_line) {
		invertOneLineRegion(start_column, end_column, start_line);
		return;
	}

	if (start_column > 0) {
		invertOneLineRegion(start_column, width, start_line);
		start_line++;
	}

	for (int i = start_line; i < end_line; i++) {
		invertOneLineRegion(0, width, i);
	}

	invertOneLineRegion(0, end_column, end_line);
}

void
BV_DCBuffer::invertOneLineRegion(int start_column, int end_column, int line)
{
	assert(m_pBVDrawInfo && start_column < end_column && line >= 0);

#if 0
	int width = m_pBVDrawInfo->getWidth();
	for (int i = start_column; i < end_column; i++) {
		COLORREF cRef = ::GetPixel(m_hDC, i, line);
		::SetPixel(m_hDC, i, line, cRef ^ RGB(0, 255, 255));
	}
#else
	HGDIOBJ hOrgBrush = ::SelectObject(m_hDC, m_hbrSelectMask);
	::PatBlt(m_hDC, start_column, line, end_column - start_column, 1, PATINVERT);
	::SelectObject(m_hDC, hOrgBrush);
#endif
}

BV_DCManager::BV_DCManager()
	: DC_Manager(BV_WIDTH * BV_HEIGHT, BV_BUFCOUNT)
{
}

#ifdef _DEBUG
void
BV_DCManager::bitBlt(HDC hDCDst, const RECT& rcDst)
{
	DC_Manager::bitBlt(hDCDst, rcDst);
}
#endif

BGBuffer*
BV_DCManager::createBGBufferInstance()
{
	if (m_pDrawInfo == NULL) {
		return NULL;
	}
	DCBuffer* pBuf = new BV_DCBuffer(m_nBufSize);
	if (!pBuf->prepareDC(m_pDrawInfo)) {
		delete pBuf;
		return NULL;
	}
	return pBuf;
}

BitmapView::BitmapView(HWND hwndParent, const RECT& rctWindow, BV_DrawInfo* pDrawInfo)
	: View(hwndParent,
		   WS_CHILD | WS_VISIBLE | WS_VSCROLL,
		   0,//WS_EX_CLIENTEDGE,
		   rctWindow,
		   new BV_DCManager(),
		   pDrawInfo)
{
	setViewSize(BV_WIDTH, -1);
}

BitmapView::~BitmapView()
{
}

bool
BitmapView::setDrawInfo(DrawInfo* pDrawInfo)
{
	if (!dynamic_cast<BV_DrawInfo*>(pDrawInfo)) {
		return false;
	}

	return View::setDrawInfo(pDrawInfo);
}

LRESULT
BitmapView::viewWndProcMain(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return View::viewWndProcMain(uMsg, wParam, lParam);
}

BitmapViewWindow::BitmapViewWindow(LF_Notifier& lfNotify, HWND hwndOwner)
	: m_lfNotify(lfNotify),
	  m_pBitmapView(NULL),
	  m_pBVDrawInfo(NULL),
	  m_hwndOwner(hwndOwner),
	  m_hWnd(NULL),
	  m_uWindowWidth(0)
{
	HINSTANCE hInstance = (HINSTANCE)::GetWindowLong(hwndOwner, GWL_HINSTANCE);

	// window class の登録
	static int bRegWndClass = registerWndClass(hInstance);
	if (!bRegWndClass) throw CreateBitmapViewError();

	RECT rctOwner;
	::GetWindowRect(hwndOwner, &rctOwner);

	m_hWnd = ::CreateWindowEx(WS_EX_PALETTEWINDOW,
							  BV_WNDCLASSNAME, "BitmapView",
							  WS_OVERLAPPED | WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU,
							  rctOwner.right, rctOwner.top,
							  BV_WIDTH, rctOwner.bottom - rctOwner.top,
							  hwndOwner, NULL, hInstance,
							  (LPVOID)this);
	if (!m_hWnd) {
		throw CreateBitmapViewError();
	}

//	::SetWindowPos(m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
//				   SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

BitmapViewWindow::~BitmapViewWindow()
{
}

bool
BitmapViewWindow::show()
{
	m_pBitmapView->registTo(m_lfNotify);
	::ShowWindow(m_hWnd, SW_SHOW);
	onResize(m_hWnd);
	return true;
}

bool
BitmapViewWindow::hide()
{
	::ShowWindow(m_hWnd, SW_HIDE);
	m_pBitmapView->unregist();
	return true;
}

void
BitmapViewWindow::onCreate(HWND hWnd)
{
	RECT rctWindow, rctClient;
	::GetWindowRect(hWnd, &rctWindow);
	::GetClientRect(hWnd, &rctClient);

	rctWindow.right = rctWindow.left
					+ (rctWindow.right - rctWindow.left)
					- (rctClient.right - rctClient.left);

	rctClient.right = BV_WIDTH;

	m_pBVDrawInfo = new BV_DrawInfo();
	m_pBitmapView = new BitmapView(hWnd, rctClient, m_pBVDrawInfo.ptr());

	m_pBitmapView->getFrameRect(rctClient);

	rctWindow.right += rctClient.right - rctClient.left;

	m_uWindowWidth = rctWindow.right - rctWindow.left;

	::SetWindowPos(hWnd, NULL,
				   rctWindow.left, rctWindow.top,
				   m_uWindowWidth,
				   rctWindow.bottom - rctWindow.top,
				   SWP_NOZORDER);

	m_pBitmapView->setFrameRect(rctClient, true);
}

void
BitmapViewWindow::onResize(HWND hWnd)
{
	RECT rctNew;
	::GetClientRect(hWnd, &rctNew);
	if (m_pBitmapView.ptr()) {
		m_pBitmapView->setFrameRect(rctNew, true);
	}
}

int
BitmapViewWindow::registerWndClass(HINSTANCE hInst)
{
	// Window class の登録
	WNDCLASS wc;
	::ZeroMemory(&wc, sizeof(WNDCLASS));
	wc.hInstance = hInst;
	wc.style = CS_OWNDC;
	wc.hIcon = NULL;
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wc.lpfnWndProc = (WNDPROC)BitmapViewWndProc;
	wc.lpszClassName = BV_WNDCLASSNAME;
	wc.lpszMenuName = NULL;

	return ::RegisterClass(&wc);
}

LRESULT CALLBACK
BitmapViewWindow::BitmapViewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CREATESTRUCT* pCs = (CREATESTRUCT*)lParam;
			::SetWindowLong(hWnd, GWL_USERDATA, (LONG)pCs->lpCreateParams);
			BitmapViewWindow* pBitmapViewWindow = (BitmapViewWindow*)pCs->lpCreateParams;
			pBitmapViewWindow->onCreate(hWnd);
			return 0;
		}
	case WM_NCCREATE:
		return TRUE;
	}

	BitmapViewWindow*
		pBitmapViewWindow = (BitmapViewWindow*)::GetWindowLong(hWnd, GWL_USERDATA);
	if (!pBitmapViewWindow) {
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg) {
	case WM_SIZE:
		pBitmapViewWindow->onResize(hWnd);
		break;

	case WM_SIZING:
		{
			RECT* pRect = (RECT*)lParam;
			pRect->right = pRect->left + pBitmapViewWindow->m_uWindowWidth;
		}
		break;

	case WM_CLOSE:
		::DestroyWindow(hWnd);
//		pBitmapViewWindow->hide();
		break;

	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_CLOSE) {
//			::ShowWindow(hWnd, SW_HIDE);
			pBitmapViewWindow->hide();
			break;
		}
		// through down.

	default:
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

