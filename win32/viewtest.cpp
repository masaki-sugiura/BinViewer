// $Id$

#pragma warning(disable : 4786)

#define _WIN32_WINNT  0x500  // to support mouse wheel

#include <windows.h>
#include <commctrl.h>
#include <assert.h>

#include "view.h"
#include "strutils.h"

#define ADDR_WIDTH   100
#define BYTE_WIDTH    32
#define DATA_WIDTH   160

#define W_WIDTH  (ADDR_WIDTH + BYTE_WIDTH * 16 + 20 + DATA_WIDTH + 20)
#define W_HEIGHT 720

#define STATUSBAR_HEIGHT  20

#define IDC_STATUSBAR  10

#define STATUS_POS_HEADER    "カーソルの現在位置： 0x"

#define MAINWND_CLASSNAME    "BinViewerClass32"

// resources
static HINSTANCE g_hInstance;
static HWND g_hwndMain, g_hwndStatusBar;
static string g_strAppName;

class TestDCBuffer : public DCBuffer {
public:
	TestDCBuffer(int nBufSize,
				 HDC hDC, int nWidth, int nHeight,
				 HBRUSH hbrBackground)
		: DCBuffer(nBufSize, hDC, nWidth, nHeight, hbrBackground)
	{}

	int render()
	{
		RECT rctRegion;
		rctRegion.left = 0;
		rctRegion.right = m_nWidth;
		rctRegion.top = 0;
		rctRegion.bottom = 1;
		for ( ; rctRegion.top < m_nHeight; rctRegion.top++, rctRegion.bottom++) {
			int GrayScale = rctRegion.top % 256;
			HBRUSH hBrush = ::CreateSolidBrush(RGB(GrayScale, GrayScale, GrayScale));
			::FillRect(m_hDC, &rctRegion, hBrush);
			::DeleteObject(hBrush);
		}
		::TextOut(m_hDC, 0, m_nHeight / 2, "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF", 64);
		return m_nBufSize;
	}

protected:
	void invertRegionInBuffer(int offset, int size)
	{
	}
};

class TestDCManager : public DC_Manager {
public:
	TestDCManager()
		: DC_Manager(32 * 1024, 3)
	{
	}

	int getXPositionByCoordinate(int x)
	{
		return 0;
	}

protected:
	BGBuffer* createBGBufferInstance()
	{
		if (m_hDC == NULL || m_nWidth == 0 || m_nHeight == 0 || m_hbrBackground == NULL) {
			return NULL;
		}
		return new TestDCBuffer(m_nBufSize, m_hDC, m_nWidth, m_nHeight, m_hbrBackground);
	}
};

class ViewFrame : public View {
public:
	ViewFrame(HWND hwndParent, const RECT& rctClient)
		: View(hwndParent,
			   WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
			   WS_EX_CLIENTEDGE,
			   rctClient,
			   new TestDCManager(),
			   800, 1024, 1, RGB(255, 255, 255))
	{
	}
	~ViewFrame()
	{
		delete m_pDCManager;
	}

	void setFrameRect(const RECT& rctFrame)
	{
		View::setFrameRect(rctFrame);
		::InvalidateRect(m_hwndView, NULL, FALSE);
		::UpdateWindow(m_hwndView);
	}

protected:
	void onHScroll(WPARAM wParam, LPARAM lParam)
	{
		View::onHScroll(wParam, lParam);
		::InvalidateRect(m_hwndView, NULL, FALSE);
		::UpdateWindow(m_hwndView);
	}
	void onVScroll(WPARAM wParam, LPARAM lParam)
	{
		View::onVScroll(wParam, lParam);
		::InvalidateRect(m_hwndView, NULL, FALSE);
		::UpdateWindow(m_hwndView);
	}

	LRESULT viewWndProcMain(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return View::viewWndProcMain(uMsg, wParam, lParam);
	}
};


// window properties
static Auto_Ptr<ViewFrame> g_pViewFrame(NULL);

static Auto_Ptr<LargeFileReader> g_pLFReader(NULL);
static LF_Notifier g_lfNotifier;

static void
AdjustWindowSize(HWND hWnd, const RECT& rctFrame)
{
	RECT rctWnd, rctClient = rctFrame;
	::GetWindowRect(hWnd, &rctWnd);
	int x_diff = rctWnd.right - rctWnd.left - (rctFrame.right - rctFrame.left),
		y_diff = rctWnd.bottom - rctWnd.top - (rctFrame.bottom - rctFrame.top);

	g_pViewFrame->adjustWindowRect(rctClient);

	rctWnd.right  = rctWnd.left + rctClient.right  - rctClient.left + x_diff;
	rctWnd.bottom = rctWnd.top + rctClient.bottom - rctClient.top + y_diff;

	RECT rctDesktop;
	::GetWindowRect(::GetDesktopWindow(), &rctDesktop);

	int dWidth = rctDesktop.right - rctDesktop.left;
	if (rctWnd.right > dWidth) {
		if (rctWnd.right - rctWnd.left > dWidth) {
			// 最大化する必要あり
		} else {
			// デスクトップに収まるように位置を調整
			int over = rctWnd.right - dWidth;
			rctWnd.left  -= over;
			rctWnd.right -= over;
		}
	}

	int dHeight = rctDesktop.bottom - rctDesktop.top;
	if (rctWnd.bottom > rctDesktop.bottom - rctDesktop.top) {
		if (rctWnd.bottom - rctWnd.top > dHeight) {
			// 最大化する必要あり
		} else {
			// デスクトップに収まるように位置を調整
			int over = rctWnd.bottom - dHeight;
			rctWnd.top    -= over;
			rctWnd.bottom -= over;
		}
	}

	::SetWindowPos(hWnd, NULL,
				   rctWnd.left, rctWnd.top,
				   rctWnd.right - rctWnd.left,
				   rctWnd.bottom - rctWnd.top,
				   SWP_NOZORDER);

	g_pViewFrame->setFrameRect(rctClient);
}

static void OnSetPosition(HWND, WPARAM, LPARAM);

static void
OnSetPosition(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	static int hlen = lstrlen(STATUS_POS_HEADER);
	static char msgbuf[40] = STATUS_POS_HEADER;

	QwordToStr((UINT)lParam, (UINT)wParam, msgbuf + hlen);
	::SendMessage(g_hwndStatusBar, SB_SETTEXT, 0, (LPARAM)msgbuf);
}

static void
OnResizing(HWND hWnd, RECT* prctNew)
{
	RECT rctWnd, rctClient;

	::GetWindowRect(hWnd, &rctWnd);
	::GetClientRect(hWnd, &rctClient);

	rctClient.right += (prctNew->right - prctNew->left)
					 - (rctWnd.right - rctWnd.left);
	rctClient.bottom += (prctNew->bottom - prctNew->top)
					  - (rctWnd.bottom - rctWnd.top)
					  - STATUSBAR_HEIGHT;

	g_pViewFrame->setFrameRect(rctClient);

	::SetWindowPos(g_hwndStatusBar, 0,
				   0, rctClient.bottom,
				   rctClient.right - rctClient.left,
				   STATUSBAR_HEIGHT,
				   SWP_NOZORDER);
}

static void
OnResize(HWND hWnd, WPARAM fwSizeType)
{
	RECT rctClient;
	::GetClientRect(hWnd, &rctClient);
	rctClient.bottom -= STATUSBAR_HEIGHT;

	g_pViewFrame->setFrameRect(rctClient);

	::SetWindowPos(g_hwndStatusBar, 0,
				   0, rctClient.bottom,
				   rctClient.right - rctClient.left,
				   STATUSBAR_HEIGHT,
				   SWP_NOZORDER);
}

static void
OnCreate(HWND hWnd)
{
	g_hwndMain = hWnd;

	RECT rctClient;
	::GetClientRect(hWnd, &rctClient);
	rctClient.bottom -= STATUSBAR_HEIGHT;

	g_pViewFrame = new ViewFrame(hWnd, rctClient);
	assert(g_pViewFrame.ptr());

	g_lfNotifier.registerAcceptor(g_pViewFrame.ptr());

	AdjustWindowSize(hWnd, rctClient);

	g_hwndStatusBar = ::CreateWindow(STATUSCLASSNAME,
									 "",
									 WS_CHILD | WS_VISIBLE |
									  SBARS_SIZEGRIP,
									 rctClient.left,
									 rctClient.bottom,
									 rctClient.right - rctClient.left,
									 STATUSBAR_HEIGHT,
									 hWnd,
									 (HMENU)IDC_STATUSBAR,
									 g_hInstance,
									 NULL);
	if (!g_hwndStatusBar) {
		::SendMessage(hWnd, WM_CLOSE, 0, 0);
		return;
	}

	HDC hDC = ::GetDC(g_hwndStatusBar);
	char status[80];
	lstrcpy(status, STATUS_POS_HEADER);
	lstrcat(status, "0000000000000000");
	SIZE tsize;
	::GetTextExtentPoint32(hDC, status, lstrlen(status), &tsize);
	::SendMessage(g_hwndStatusBar, SB_SETPARTS, 1, (LPARAM)&tsize.cx);
	::ReleaseDC(g_hwndStatusBar, hDC);

	g_pLFReader = new LargeFileReader("C:\\temp\\gt4.mpg");

	g_lfNotifier.loadFile(g_pLFReader.ptr());
}

static void
OnQuit(HWND)
{
	g_lfNotifier.unloadFile();
	g_pLFReader = NULL;
	g_lfNotifier.unregisterAcceptor(g_pViewFrame.ptr());
	g_pViewFrame = NULL;
}

LRESULT CALLBACK
MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		OnCreate(hWnd);
		break;

	case WM_COMMAND:
		break;

	case WM_SIZING:
		OnResizing(hWnd, (RECT*)lParam);
		return TRUE;

	case WM_SIZE:
		OnResize(hWnd, wParam);
		break;

	case WM_CLOSE:
		::DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		OnQuit(hWnd);
		::PostQuitMessage(0);
		break;

	default:
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}


int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
		LPSTR lpszCmdLine, int nCmdShow)
{
	g_hInstance = hInstance;

	// Main Window's WindowClass
	WNDCLASS wc;
	::ZeroMemory(&wc, sizeof(WNDCLASS));
	wc.hInstance = hInstance;
	wc.style = CS_OWNDC;
	wc.hIcon = NULL;
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wc.lpfnWndProc = (WNDPROC)MainWndProc;
	wc.lpszClassName = MAINWND_CLASSNAME;
	wc.lpszMenuName = NULL;
	if (!::RegisterClass(&wc)) return -1;

	::InitCommonControls();

	g_hwndMain = ::CreateWindowEx(0,
								  wc.lpszClassName, "BinViewer",
								  WS_OVERLAPPEDWINDOW,
								  CW_USEDEFAULT, CW_USEDEFAULT,
								  W_WIDTH, W_HEIGHT,
								  NULL, NULL, hInstance, NULL);
	if (!g_hwndMain) {
		return -1;
	}

	::ShowWindow(g_hwndMain, SW_SHOW);

	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	return msg.wParam;
}

