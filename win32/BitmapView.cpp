// $Id$

#include "BitmapView.h"
#include "ViewFrame.h"
#include "messages.h"

BitmapView::BitmapView(HWND hwndOwner, ViewFrame* pViewFrame)
	: m_hwndOwner(hwndOwner),
	  m_pViewFrame(pViewFrame),
	  m_pLFReader(NULL),
	  m_qCurrentPos(-1),
	  m_qHeadPos(-1),
	  m_pScrollManager(NULL)
{
	if (!pViewFrame) throw CreateBitmapViewError();

	m_pLFReader = pViewFrame->getReader();

	HINSTANCE hInstance = (HINSTANCE)::GetWindowLong(hwndOwner, GWL_HINSTANCE);

	// Window class �̓o�^
	WNDCLASS wc;
	::ZeroMemory(&wc, sizeof(WNDCLASS));
	wc.hInstance = hInstance;
	wc.style = CS_OWNDC;
	wc.hIcon = NULL;
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wc.lpfnWndProc = (WNDPROC)BitmapViewWndProc;
	wc.lpszClassName = "BinViewer_BitmapViewClass32";
	wc.lpszMenuName = NULL;
	if (!::RegisterClass(&wc)) throw CreateBitmapViewError();

	RECT rctOwner;
	::GetWindowRect(hwndOwner, &rctOwner);

	m_hwndView = ::CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_OVERLAPPEDWINDOW,
								  wc.lpszClassName, "BitmapView",
								  WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU |
								   WS_VSCROLL,
								  rctOwner.right, rctOwner.top,
								  128, 512,
								  NULL, NULL, hInstance,
								  (LPVOID)this);
	if (!m_hwndView) {
		throw CreateBitmapViewError();
	}

	m_pScrollManager = new ScrollManager<filesize_t>(m_hwndView, SB_VERT);
	m_pScrollManager->disable();
}

BitmapView::~BitmapView()
{
	::DeleteObject(m_hbmView);
	::DeleteDC(m_hdcView);
	delete m_pScrollManager;
	::SendMessage(m_hwndView, WM_CLOSE, 0, 0);
}

bool
BitmapView::loadFile(LargeFileReader* pLFReader)
{
	if (!pLFReader) return false;
	m_pLFReader = pLFReader;
	m_qCurrentPos = 0;
	m_qHeadPos = 0;
	m_pScrollManager->setInfo(pLFReader->size() / 128,
							  m_rctClient.bottom,
							  0);
	drawDC(m_rctClient);
	invertPos();
	::InvalidateRect(m_hwndView, NULL, FALSE);
	::UpdateWindow(m_hwndView);
	return true;
}

void
BitmapView::unloadFile()
{
	m_pScrollManager->disable();
	m_pLFReader = NULL;
	m_qHeadPos = -1;
	m_qCurrentPos = -1;
	drawDC(m_rctClient);
}

bool
BitmapView::show()
{
	::ShowWindow(m_hwndView, SW_SHOW);
	return true;
}

bool
BitmapView::hide()
{
	::ShowWindow(m_hwndView, SW_HIDE);
	return true;
}

bool
BitmapView::setPosition(filesize_t pos)
{
	if (m_qCurrentPos == -1) return false;
	invertPos();
	m_qCurrentPos = pos;
	if (m_qHeadPos > m_qCurrentPos ||
		m_qHeadPos + 128 * m_rctClient.bottom <= m_qCurrentPos) {
		m_qHeadPos = (pos / 128) * 128;
		drawDC(m_rctClient);
	}
	invertPos();
	::InvalidateRect(m_hwndView, NULL, FALSE);
	::UpdateWindow(m_hwndView);
	return true;
}

void
BitmapView::drawDC(const RECT& rctPaint)
{
	if (m_pLFReader) {
		filesize_t qOffset = m_qHeadPos + rctPaint.top * 128;
		static BYTE databuf[128 * 2048]; // max size
		int size = 128 * (rctPaint.bottom - rctPaint.top);
		if (!size) return;
		size = m_pLFReader->readFrom(qOffset, FILE_BEGIN,
									 databuf, size);
		int x = 0, y = rctPaint.top;
		for (int i = 0; i < size; i++) {
			BYTE val = 255 - databuf[i]; // 0 �����ɂȂ�悤��
			::SetPixel(m_hdcView, x, y, RGB(val, val, val));
			if (++x == 128) {
				x = 0; y++;
			}
		}
		while (y < rctPaint.bottom) {
			::SetPixel(m_hdcView, x, y, RGB(255, 255, 255));
			if (++x == 128) {
				x = 0; y++;
			}
		}
	} else {
		::FillRect(m_hdcView, &rctPaint, (HBRUSH)(COLOR_APPWORKSPACE + 1));
	}
}

inline void invertPixel(HDC hDC, int x, int y)
{
	COLORREF cref = ::GetPixel(hDC, x, y);
	cref ^= RGB(0, 255, 255);
	::SetPixel(hDC, x, y, cref);
}

void
BitmapView::invertPos()
{
	int offset = m_qCurrentPos - m_qHeadPos;
	int x = offset % 128, y = offset / 128;
	invertPixel(m_hdcView, x - 1, y - 1);
	invertPixel(m_hdcView, x,     y - 1);
	invertPixel(m_hdcView, x + 1, y - 1);
	invertPixel(m_hdcView, x - 1, y);
	invertPixel(m_hdcView, x + 1, y);
	invertPixel(m_hdcView, x - 1, y + 1);
	invertPixel(m_hdcView, x,     y + 1);
	invertPixel(m_hdcView, x + 1, y + 1);
}

bool
BitmapView::onCreate(HWND hWnd)
{
	RECT rctWindow, rctClient;
	::GetWindowRect(hWnd, &rctWindow);
	::GetClientRect(hWnd, &rctClient);
	rctWindow.right += 128 - (rctClient.right - rctClient.left);
	m_uWindowWidth = rctWindow.right - rctWindow.left;

	m_rctClient.left = m_rctClient.top = 0;
	m_rctClient.right = 128;
	m_rctClient.bottom = rctClient.bottom;

	::SetWindowPos(hWnd, NULL,
				   rctWindow.left, rctWindow.top,
				   m_uWindowWidth,
				   rctWindow.bottom - rctWindow.top,
				   SWP_NOZORDER | SWP_NOMOVE);

	HDC hDC = ::GetDC(hWnd);
	m_hdcView = ::CreateCompatibleDC(hDC);
	m_hbmView = ::CreateCompatibleBitmap(hDC, 128, 2048);
	::SelectObject(m_hdcView, m_hbmView);
	::ReleaseDC(hWnd, hDC);

	drawDC(m_rctClient);

	return true;
}

void
BitmapView::onPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	::BeginPaint(hWnd, &ps);
	::BitBlt(ps.hdc,
			 ps.rcPaint.left, ps.rcPaint.top,
			 ps.rcPaint.right - ps.rcPaint.left,
			 ps.rcPaint.bottom - ps.rcPaint.top,
			 m_hdcView,
			 ps.rcPaint.left, ps.rcPaint.top,
			 SRCCOPY);
	::EndPaint(hWnd, &ps);
}

bool
BitmapView::onResize(HWND hWnd)
{
	RECT rctNew;
	::GetClientRect(hWnd, &rctNew);
	if (rctNew.bottom - rctNew.top > m_rctClient.bottom - m_rctClient.top) {
		// �`��̈悪�������ꍇ�̂�(�����������̂�)�ĕ`��
		RECT rctPaint = rctNew;
		rctPaint.top = m_rctClient.bottom;
		drawDC(rctPaint);
	}
	m_rctClient = rctNew;
	if (m_pLFReader) {
		m_pScrollManager->setInfo(m_pLFReader->size() / 128,
								  m_rctClient.bottom,
								  m_qHeadPos / 128);
	}
	return true;
}

void
BitmapView::onScroll(WPARAM wParam, LPARAM lParam)
{
	if (!m_pLFReader) return;
	filesize_t qNewPos = m_pScrollManager->onScroll(LOWORD(wParam)) * 128;
	setPosition(qNewPos);
}

void
BitmapView::onLButtonDown(WPARAM wParam, LPARAM lParam)
{
	if (!m_pLFReader) return;

	const POINTS& pt = MAKEPOINTS(lParam);

	filesize_t pos = m_qHeadPos + 128 * pt.y + pt.x;
	if (pos > m_pLFReader->size()) return;

	invertPos();
	m_qCurrentPos = pos;
	invertPos();

	m_pViewFrame->onJump(pos);
}

LRESULT CALLBACK
BitmapView::BitmapViewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CREATESTRUCT* pCs = (CREATESTRUCT*)lParam;
			::SetWindowLong(hWnd, GWL_USERDATA, (LONG)pCs->lpCreateParams);
			BitmapView* pBitmapView = (BitmapView*)pCs->lpCreateParams;
			pBitmapView->onCreate(hWnd);
			return 0;
		}
	case WM_NCCREATE:
		return TRUE;
	}

	BitmapView* pBitmapView = (BitmapView*)::GetWindowLong(hWnd, GWL_USERDATA);
	if (!pBitmapView) {
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg) {
	case WM_PAINT:
		pBitmapView->onPaint(hWnd);
		break;

	case WM_SIZE:
		pBitmapView->onResize(hWnd);
		break;

	case WM_SIZING:
		{
			RECT* pRect = (RECT*)lParam;
			pRect->right = pRect->left + pBitmapView->m_uWindowWidth;
		}
		break;

	case WM_VSCROLL:
		pBitmapView->onScroll(wParam, lParam);
		break;

	case WM_LBUTTONDOWN:
		pBitmapView->onLButtonDown(wParam, lParam);
		break;

	case WM_CLOSE:
		::DestroyWindow(hWnd);
		break;

	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_CLOSE) {
			::ShowWindow(hWnd, SW_HIDE);
			break;
		}
		// through down.

	default:
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

