// $Id$

#include "MainWindow.h"
#include "resource.h"

#include <commctrl.h>

#define MAINWND_CLASSNAME    "BinViewerClass32"

#define ADDR_WIDTH   100
#define BYTE_WIDTH    32
#define DATA_WIDTH   160

#define W_WIDTH  (ADDR_WIDTH + BYTE_WIDTH * 16 + 20 + DATA_WIDTH + 20)
#define W_HEIGHT 720

#define IDC_STATUSBAR  10
#define STATUSBAR_HEIGHT  20
#define STATUS_POS_HEADER    "カーソルの現在位置： 0x"

MainWindow::MainWindow(HINSTANCE hInstance, LPCSTR lpszFileName)
	: m_pHexView(NULL),
	  m_pLFReader(NULL),
	  m_pHVDrawInfo(NULL),
	  m_pBitmapViewWindow(NULL),
	  m_pStatusBar(NULL),
	  m_hWnd(NULL)
{
	// window class の登録
	static int bRegWndClass = registerWndClass(hInstance);
	if (!bRegWndClass) {
		throw CreateWindowError();
	}

	m_hAccel = ::LoadAccelerators(hInstance,
								  MAKEINTRESOURCE(IDR_KEYACCEL));

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
		openFile(lpszFileName);
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
		if (!::TranslateAccelerator(m_hWnd, m_hAccel, &msg)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}

void
MainWindow::onCreate(HWND hWnd)
{
	m_pHVDrawInfo = loadDrawInfo(hWnd);

	RECT rctClient;
	::GetClientRect(hWnd, &rctClient);

	rctClient.top = rctClient.bottom - STATUSBAR_HEIGHT;

	m_pStatusBar = new StatusBar(m_lfNotifier, hWnd, rctClient);

	rctClient.top = 0;
	rctClient.bottom -= STATUSBAR_HEIGHT;

	m_pHexView = new HexView(m_lfNotifier, hWnd, rctClient, m_pHVDrawInfo.ptr());

	adjustWindowSize(hWnd, rctClient);

	// BitmapView
	m_pBitmapViewWindow = new BitmapViewWindow(m_lfNotifier, hWnd);
}

void
MainWindow::onResize()
{
	RECT rctClient;
	::GetClientRect(m_hWnd, &rctClient);

	rctClient.top = rctClient.bottom - STATUSBAR_HEIGHT;
	if (m_pStatusBar.ptr()) {
		m_pStatusBar->setWindowPos(rctClient);
	}

	rctClient.bottom -= STATUSBAR_HEIGHT;
	rctClient.top = 0;
	m_pHexView->setFrameRect(rctClient, true);
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
					  - (rctWnd.bottom - rctWnd.top);
//					  - STATUSBAR_HEIGHT;

	rctClient.top = rctClient.bottom - STATUSBAR_HEIGHT;
	if (m_pStatusBar.ptr()) {
		m_pStatusBar->setWindowPos(rctClient);
	}

	rctClient.bottom -= STATUSBAR_HEIGHT;
	rctClient.top = 0;
	m_pHexView->setFrameRect(rctClient, true);
}

void
MainWindow::onQuit()
{
	m_lfNotifier.unloadFile();
	m_pLFReader = NULL;
	m_pBitmapViewWindow = NULL;
	m_pStatusBar = NULL;
	m_pHexView = NULL;
	m_pHVDrawInfo = NULL;
}

void
MainWindow::onOpenFile()
{
	char buf[MAX_PATH];
	if (!getImageFileName(buf, MAX_PATH)) {
		return;
	}

	openFile(buf);
}

void
MainWindow::onCloseFile()
{
	m_lfNotifier.unloadFile();
	m_pLFReader = NULL;
	::SetWindowText(m_hWnd, "BinViewer");
	enableMenuForOpenFile(false);
}

HV_DrawInfo*
MainWindow::loadDrawInfo(HWND hWnd)
{
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

	return pDrawInfo;
}

void
MainWindow::adjustWindowSize(HWND hWnd, const RECT& rctFrame)
{
	RECT rctWnd, rctClient = rctFrame;
	::GetWindowRect(hWnd, &rctWnd);
	int x_diff = rctWnd.right - rctWnd.left - (rctFrame.right - rctFrame.left),
		y_diff = rctWnd.bottom - rctWnd.top - (rctFrame.bottom - rctFrame.top);

	m_pHexView->adjustWindowRect(rctClient);

	rctWnd.right  = rctWnd.left + rctClient.right - rctClient.left + x_diff;
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

	m_pHexView->setFrameRect(rctClient, true);
}

bool
MainWindow::getImageFileName(LPSTR buf, int bufsize)
{
	buf[0] = '\0';

	OPENFILENAME ofn;

	::ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize  = sizeof(OPENFILENAME);
	ofn.hInstance    = (HINSTANCE)::GetWindowLong(m_hWnd, GWL_HINSTANCE);
	ofn.hwndOwner    = m_hWnd;
	ofn.lpstrFilter  = "全てのファイル\0*.*\0\0";
	ofn.lpstrFile    = buf;
	ofn.nMaxFile     = bufsize;
	ofn.lpstrTitle   = "ファイルの選択";
	ofn.Flags        = OFN_FILEMUSTEXIST | OFN_EXPLORER;
	ofn.lpstrInitialDir = "";

	return ::GetOpenFileName(&ofn) != 0;
}

bool
MainWindow::openFile(LPCSTR pszFileName)
{
	if (m_pLFReader.ptr()) {
		// 既にファイルを開いている
		char cmdline[3 * MAX_PATH];
		::GetModuleFileName(NULL, cmdline, 3 * MAX_PATH);
		int len = lstrlen(cmdline);
		cmdline[len++] = ' ';
		wsprintf(cmdline + len, "\"%s\"", pszFileName);
		// 別プロセスを起動
		return ::WinExec(cmdline, SW_SHOW) < 31;
	}

	try {
		m_pLFReader = new LargeFileReader(pszFileName);
		m_lfNotifier.loadFile(m_pLFReader.ptr());
	} catch (FileOpenError&) {
		::MessageBox(m_hWnd, "指定されたファイルは存在しません。", "BinViewer", MB_OK | MB_ICONERROR);
		return false;
	}

	char titlebuf[1024];
	wsprintf(titlebuf, "BinViewer - %s", pszFileName);
	::SetWindowText(m_hWnd, titlebuf);

	enableMenuForOpenFile(true);

	return true;
}

void
MainWindow::enableMenuForOpenFile(bool bEnable)
{
	HMENU hMenu = ::GetMenu(m_hWnd);
	assert(hMenu);
	HMENU hFileMenu = ::GetSubMenu(hMenu, 0);
	assert(hFileMenu);
	::EnableMenuItem(hFileMenu, IDM_CLOSE,
					 MF_BYCOMMAND | (bEnable ? MF_ENABLED : MF_GRAYED));
	::EnableMenuItem(hMenu, 1,
					 MF_BYPOSITION | (bEnable ? MF_ENABLED : MF_GRAYED));
	::SendMessage(m_hWnd, WM_NCPAINT, 1, 0);
}

int
MainWindow::registerWndClass(HINSTANCE hInstance)
{
	// Window class の登録
	WNDCLASS wc;
	::ZeroMemory(&wc, sizeof(WNDCLASS));
	wc.hInstance = hInstance;
	wc.style = CS_OWNDC;
	wc.hIcon = ::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wc.lpfnWndProc = (WNDPROC)MainWndProc;
	wc.lpszClassName = MAINWND_CLASSNAME;
	wc.lpszMenuName = MAKEINTRESOURCE(IDM_MAINMENU);

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
		switch (LOWORD(wParam)) {
		case IDM_OPEN:
			pMainWindow->onOpenFile();
			break;

		case IDM_CLOSE:
			pMainWindow->onCloseFile();
			break;

		case IDM_QUIT:
			::DestroyWindow(hWnd);
			break;

		case IDM_HELP:
			break;

		case IDM_CONFIG:
//			ConfigMainDlg(g_pDrawInfo).doModal(hWnd);
			break;

		case IDM_SEARCH:
#if 0
			assert(g_pSearchDlg.ptr());
			if (!g_pSearchDlg->create(hWnd)) {
				::MessageBox(hWnd, "検索ダイアログの表示に失敗しました", NULL, MB_OK);
			}
			EnableSearchMenu(hWnd, FALSE);
#endif
			break;

		case IDM_JUMP:
//			JumpDlg(*g_pViewFrame).doModal(hWnd);
			break;

		case IDM_BITMAPVIEW:
			pMainWindow->m_pBitmapViewWindow->show();
			break;

		case IDK_LINEDOWN:
//			m_pHexView->onVerticalMove(1);
			break;

		case IDK_LINEUP:
//			g_pViewFrame->onVerticalMove(-1);
			break;

		case IDK_PAGEDOWN:
		case IDK_SPACE:
//			g_pViewFrame->onVScroll(SB_PAGEDOWN, 0);
			break;

		case IDK_PAGEUP:
		case IDK_S_SPACE:
//			g_pViewFrame->onVScroll(SB_PAGEUP, 0);
			break;

		case IDK_BOTTOM:
		case IDK_C_SPACE:
//			g_pViewFrame->onVScroll(SB_BOTTOM, 0);
			break;

		case IDK_TOP:
		case IDK_C_S_SPACE:
//			g_pViewFrame->onVScroll(SB_TOP, 0);
			break;

		case IDK_RIGHT:
//			g_pViewFrame->onHorizontalMove(1);
			break;

		case IDK_LEFT:
//			g_pViewFrame->onHorizontalMove(-1);
			break;
		}
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

