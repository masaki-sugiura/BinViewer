// $Id$

#include "MainWindow.h"
#include <commctrl.h>

#define MAINWND_CLASSNAME    "BinViewerClass32"

#define ADDR_WIDTH   100
#define BYTE_WIDTH    32
#define DATA_WIDTH   160

#define W_WIDTH  (ADDR_WIDTH + BYTE_WIDTH * 16 + 20 + DATA_WIDTH + 20)
#define W_HEIGHT 720

#define IDC_STATUSBAR  10
#define STATUSBAR_HEIGHT  20
#define STATUS_POS_HEADER    "�J�[�\���̌��݈ʒu�F 0x"

MainWindow::MainWindow(HINSTANCE hInstance, LPCSTR lpszFileName)
	: m_pHexView(NULL),
	  m_pLFReader(NULL),
	  m_pHVDrawInfo(NULL),
	  m_pBitmapViewWindow(NULL),
	  m_hWnd(NULL),
	  m_hwndStatusBar(NULL)
{
	// window class �̓o�^
	static int bRegWndClass = registerWndClass(hInstance);
	if (!bRegWndClass) {
		throw CreateWindowError();
	}

	m_hWnd = ::CreateWindowEx(0,
							  MAINWND_CLASSNAME, "BinViewer",
							  WS_OVERLAPPEDWINDOW,
							  CW_USEDEFAULT, CW_USEDEFAULT,
							  W_WIDTH, W_HEIGHT,
							  NULL, NULL, hInstance, (LPVOID)this);
	if (!m_hWnd) {
		throw CreateWindowError();
	}

	if (lpszFileName) {
		m_pLFReader = new LargeFileReader(lpszFileName);
		m_lfNotifier.loadFile(m_pLFReader.ptr());
	}
}

MainWindow::~MainWindow()
{
}

bool
MainWindow::show(int nCmdShow)
{
	::ShowWindow(m_hWnd, nCmdShow);
	return true;
}

int
MainWindow::doModal()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	return msg.wParam;
}

void
MainWindow::onCreate(HWND hWnd)
{
	RECT rctClient;
	::GetClientRect(hWnd, &rctClient);
	rctClient.bottom -= STATUSBAR_HEIGHT;

#define COLOR_BLACK      RGB(0, 0, 0)
#define COLOR_GRAY       RGB(128, 128, 128)
#define COLOR_LIGHTGRAY  RGB(192, 192, 192)
#define COLOR_WHITE      RGB(255, 255, 255)
#define COLOR_YELLOW     RGB(255, 255, 0)

#define DEFAULT_FONT_SIZE  12
#define DEFAULT_FG_COLOR_ADDRESS COLOR_WHITE
#define DEFAULT_BK_COLOR_ADDRESS COLOR_GRAY
#define DEFAULT_FG_COLOR_DATA    COLOR_BLACK
#define DEFAULT_BK_COLOR_DATA    COLOR_WHITE
#define DEFAULT_FG_COLOR_STRING  COLOR_BLACK
#define DEFAULT_BK_COLOR_STRING  COLOR_LIGHTGRAY
#define DEFAULT_FG_COLOR_HEADER  COLOR_BLACK
#define DEFAULT_BK_COLOR_HEADER  COLOR_YELLOW

	HDC hDC = ::GetDC(hWnd);
	HV_DrawInfo* pDrawInfo = new HV_DrawInfo(hDC, DEFAULT_FONT_SIZE,
											 "FixedSys", false,
											 DEFAULT_FG_COLOR_ADDRESS, DEFAULT_BK_COLOR_ADDRESS,
											 DEFAULT_FG_COLOR_DATA, DEFAULT_BK_COLOR_DATA,
											 DEFAULT_FG_COLOR_STRING, DEFAULT_BK_COLOR_STRING,
											 DEFAULT_FG_COLOR_HEADER, DEFAULT_BK_COLOR_HEADER,
											 CARET_STATIC, WHEEL_AS_ARROW_KEYS);
	::ReleaseDC(hWnd, hDC);

	m_pHVDrawInfo = pDrawInfo;

	m_pHexView = new HexView(m_lfNotifier, hWnd, rctClient, pDrawInfo);

	adjustWindowSize(hWnd, rctClient);

	m_hwndStatusBar = ::CreateWindow(STATUSCLASSNAME,
									 "",
									 WS_CHILD | WS_VISIBLE |
									  SBARS_SIZEGRIP,
									 rctClient.left,
									 rctClient.bottom,
									 rctClient.right - rctClient.left,
									 STATUSBAR_HEIGHT,
									 hWnd,
									 (HMENU)IDC_STATUSBAR,
									 (HINSTANCE)::GetWindowLong(hWnd, GWL_HINSTANCE),
									 NULL);
	if (!m_hwndStatusBar) {
		::SendMessage(hWnd, WM_CLOSE, 0, 0);
		return;
	}

	hDC = ::GetDC(m_hwndStatusBar);
	char status[80];
	lstrcpy(status, STATUS_POS_HEADER);
	lstrcat(status, "0000000000000000");
	SIZE tsize;
	::GetTextExtentPoint32(hDC, status, lstrlen(status), &tsize);
	::SendMessage(m_hwndStatusBar, SB_SETPARTS, 1, (LPARAM)&tsize.cx);
	::ReleaseDC(m_hwndStatusBar, hDC);

	// BitmapView
	m_pBitmapViewWindow = new BitmapViewWindow(m_lfNotifier, hWnd);
}

void
MainWindow::onResize()
{
	RECT rctClient;
	::GetClientRect(m_hWnd, &rctClient);
	rctClient.bottom -= STATUSBAR_HEIGHT;

	m_pHexView->setFrameRect(rctClient, false);

	::SetWindowPos(m_hwndStatusBar, 0,
				   0, rctClient.bottom,
				   rctClient.right - rctClient.left,
				   STATUSBAR_HEIGHT,
				   SWP_NOZORDER);

	m_pHexView->redrawView();
}

void
MainWindow::onResizing(RECT* prctNew)
{
	RECT rctWnd, rctClient;

	::GetWindowRect(m_hWnd, &rctWnd);
	::GetClientRect(m_hWnd, &rctClient);

	rctClient.right += (prctNew->right - prctNew->left)
					 - (rctWnd.right - rctWnd.left);
	rctClient.bottom += (prctNew->bottom - prctNew->top)
					  - (rctWnd.bottom - rctWnd.top)
					  - STATUSBAR_HEIGHT;

	m_pHexView->setFrameRect(rctClient, false);

	::SetWindowPos(m_hwndStatusBar, 0,
				   0, rctClient.bottom,
				   rctClient.right - rctClient.left,
				   STATUSBAR_HEIGHT,
				   SWP_NOZORDER);

	m_pHexView->redrawView();
}

void
MainWindow::onQuit()
{
	m_lfNotifier.unloadFile();
	m_pLFReader = NULL;
	m_pBitmapViewWindow = NULL;
	m_pHexView = NULL;
	m_pHVDrawInfo = NULL;
}

void
MainWindow::adjustWindowSize(HWND hWnd, const RECT& rctFrame)
{
	RECT rctWnd, rctClient = rctFrame;
	::GetWindowRect(hWnd, &rctWnd);
	int x_diff = rctWnd.right - rctWnd.left - (rctFrame.right - rctFrame.left),
		y_diff = rctWnd.bottom - rctWnd.top - (rctFrame.bottom - rctFrame.top);

	m_pHexView->adjustWindowRect(rctClient);

	rctWnd.right  = rctWnd.left + rctClient.right  - rctClient.left + x_diff;
	rctWnd.bottom = rctWnd.top + rctClient.bottom - rctClient.top + y_diff;

	RECT rctDesktop;
	::GetWindowRect(::GetDesktopWindow(), &rctDesktop);

	int dWidth = rctDesktop.right - rctDesktop.left;
	if (rctWnd.right > dWidth) {
		if (rctWnd.right - rctWnd.left > dWidth) {
			// �ő剻����K�v����
		} else {
			// �f�X�N�g�b�v�Ɏ��܂�悤�Ɉʒu�𒲐�
			int over = rctWnd.right - dWidth;
			rctWnd.left  -= over;
			rctWnd.right -= over;
		}
	}

	int dHeight = rctDesktop.bottom - rctDesktop.top;
	if (rctWnd.bottom > rctDesktop.bottom - rctDesktop.top) {
		if (rctWnd.bottom - rctWnd.top > dHeight) {
			// �ő剻����K�v����
		} else {
			// �f�X�N�g�b�v�Ɏ��܂�悤�Ɉʒu�𒲐�
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

	m_pHexView->setFrameRect(rctClient, true);
}

int
MainWindow::registerWndClass(HINSTANCE hInstance)
{
	// Window class �̓o�^
	WNDCLASS wc;
	::ZeroMemory(&wc, sizeof(WNDCLASS));
	wc.hInstance = hInstance;
	wc.style = CS_OWNDC;
	wc.hIcon = NULL;
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wc.lpfnWndProc = (WNDPROC)MainWndProc;
	wc.lpszClassName = MAINWND_CLASSNAME;
	wc.lpszMenuName = NULL;

	return ::RegisterClass(&wc);
}

LRESULT CALLBACK
MainWindow::MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CREATESTRUCT* pCs = (CREATESTRUCT*)lParam;
			::SetWindowLong(hWnd, GWL_USERDATA, (LONG)pCs->lpCreateParams);
			MainWindow* pMainWindow = (MainWindow*)pCs->lpCreateParams;
			pMainWindow->onCreate(hWnd);
			return 0;
		}
	case WM_NCCREATE:
		return TRUE;
	}

	MainWindow*
		pMainWindow = (MainWindow*)::GetWindowLong(hWnd, GWL_USERDATA);
	if (!pMainWindow) {
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg) {
	case WM_COMMAND:
		break;

	case WM_SIZE:
		pMainWindow->onResize();
		break;

	case WM_SIZING:
		pMainWindow->onResizing((RECT*)lParam);
		break;

	case WM_CLOSE:
		::DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		pMainWindow->onQuit();
		::PostQuitMessage(0);
		break;

	default:
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

