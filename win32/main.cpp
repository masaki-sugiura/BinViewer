// $Id$

#define _WIN32_WINNT  0x500  // to support mouse wheel

#include <windows.h>
#include <commctrl.h>
#include <assert.h>

#include "resource.h"
#include "strutils.h"
#include "viewframe.h"

#define ADDR_WIDTH   100
#define BYTE_WIDTH    32
#define DATA_WIDTH   160

#define W_WIDTH  (ADDR_WIDTH + BYTE_WIDTH * 16 + 20 + DATA_WIDTH + 20)
#define W_HEIGHT 720

#define COLOR_WHITE  RGB(255, 255, 255)
#define COLOR_BLACK  RGB(0, 0, 0)
#define COLOR_YELLOW RGB(255, 255, 0)

#define STATUSBAR_HEIGHT  20

#define IDC_STATUSBAR  10

#define MAINWND_CLASSNAME    "BinViewerClass32"

// resources
static HINSTANCE g_hInstance;
static HWND g_hwndMain, g_hwndStatusBar;
static string g_strAppName;
static HWND g_hSearchDlg;

// window properties
static Auto_Ptr<DrawInfo> g_pDrawInfo(NULL);
#define DEFAULT_FONT_SIZE  12
#define DEFAULT_FG_COLOR   COLOR_WHITE
#define DEFAULT_BK_COLOR   COLOR_BLACK
#define DEFAULT_FG_COLOR_HEADER  COLOR_BLACK
#define DEFAULT_BK_COLOR_HEADER  COLOR_YELLOW

static Auto_Ptr<ViewFrame> g_pViewFrame(NULL);

static string g_strImageFile;
static Auto_Ptr<LargeFileReader> g_pLFReader(NULL);

static bool g_bMapScrollBarLinearly;

static void
GetParameters()
{
	g_strAppName = __argv[0];
	if (__argc > 1) g_strImageFile = __argv[1];
}

static inline BYTE
xdigit(BYTE ch)
{
	assert(IsCharXDigit(ch));
	return IsCharDigit(ch) ? (ch - '0') : ((ch & 0x5F) - 'A' + 10);
}

static filesize_t
ParseNumber(LPCSTR str)
{
	while (*str && IsCharSpace(*str)) str++;
	if (!*str) return -1;
	else if (*str == '0') {
		str++;
		filesize_t num = 0;
		if (*str == 'x' || *str == 'X') {
			// hex
			for (;;) {
				str++;
				if (!IsCharXDigit(*str)) break;
				num <<= 4;
				num += xdigit(*str);
			}
		} else {
			// octal
			while (*str >= '0' && *str <= '7') {
				num <<= 3;
				num += *str - '0';
				str++;
			}
		}
		return num;
	} else if (IsCharDigit(*str)) {
		// decimal
		filesize_t num = 0;
		while (IsCharDigit(*str)) {
			num *= 10;
			num += *str - '0';
			str++;
		}
		return num;
	}
	return -1;
}

static void
FindCallbackProc(void* arg)
{
	assert(arg);

	FindCallbackArg* pArg = (FindCallbackArg*)arg;

	if (::IsWindow(g_hSearchDlg)) {
		if (pArg->m_qFindAddress >= 0) {
			g_pViewFrame->onJump(pArg->m_qFindAddress + pArg->m_nBufSize);
			g_pViewFrame->onJump(pArg->m_qFindAddress);
			g_pViewFrame->select(pArg->m_qFindAddress, pArg->m_nBufSize);
		} else {
			::MessageBeep(MB_ICONEXCLAMATION);
			g_pViewFrame->select(pArg->m_qOrgAddress, pArg->m_nOrgSelectedSize);
		}
		::PostMessage(g_hSearchDlg, WM_USER_FIND_FINISH, 0, 0);
	}

	delete [] pArg->m_pData;
	delete pArg;
}

static bool
SearchData(HWND hDlg, int dir)
{
	typedef enum {
		DATATYPE_HEX, DATATYPE_STRING
	} SEARCH_DATATYPE;

	if (!g_pViewFrame->isLoaded()) return false;

	// get raw data
	HWND hEdit = ::GetDlgItem(hDlg, IDC_SEARCHDATA);
	int len = ::GetWindowTextLength(hEdit);
	BYTE* buf = new BYTE[len + 1];
	::GetWindowText(hEdit, (char*)buf, len + 1);

	// get datatype
	if (::SendMessage(::GetDlgItem(hDlg, IDC_DT_HEX), BM_GETCHECK, 0, 0)) {
		// convert hex string data to the actual data
		BYTE data = 0;
		int j = 0;
		for (int i = 0; i < len; i++) {
			if (!IsCharXDigit(buf[i])) break;
			if (i & 1) {
				data <<= 4;
				data += xdigit(buf[i]);
				buf[j++] = data;
			} else {
				data = xdigit(buf[i]);
			}
		}
		if (len & 1) {
			buf[j++] = data;
		}
		len = j;
	}

	// prepare a callback arg
	FindCallbackArg* pFindCallbackArg = new FindCallbackArg;
	pFindCallbackArg = new FindCallbackArg;
	pFindCallbackArg->m_pData = buf;
	pFindCallbackArg->m_nBufSize = len;
	pFindCallbackArg->m_nDirection = dir;
	pFindCallbackArg->m_pfnCallback = FindCallbackProc;
	pFindCallbackArg->m_qFindAddress = -1;
	pFindCallbackArg->m_qOrgAddress = g_pViewFrame->getPosition();
	pFindCallbackArg->m_nOrgSelectedSize = g_pViewFrame->getSelectedSize();

	pFindCallbackArg->m_qStartAddress = pFindCallbackArg->m_qOrgAddress;
	if (dir == FIND_FORWARD) {
		pFindCallbackArg->m_qStartAddress += pFindCallbackArg->m_nOrgSelectedSize;
	}

	g_pViewFrame->unselect();

	if (g_pViewFrame->findCallback(pFindCallbackArg)) return true;

	delete [] buf;
	delete pFindCallbackArg;

	assert(0);

	return false;
}

static void
EnableSearchDlgControls(HWND hDlg, int dir, BOOL bEnable)
{
	if (bEnable) {
		::SetWindowText(::GetDlgItem(hDlg, IDC_SEARCH_FORWARD),
						"後方検索");
		::SetWindowText(::GetDlgItem(hDlg, IDC_SEARCH_BACKWARD),
						"前方検索");
		::EnableWindow(::GetDlgItem(hDlg, IDC_SEARCH_FORWARD), TRUE);
		::EnableWindow(::GetDlgItem(hDlg, IDC_SEARCH_BACKWARD), TRUE);
	} else {
		if (dir == FIND_FORWARD) {
			::SetWindowText(::GetDlgItem(hDlg, IDC_SEARCH_FORWARD), "中断");
			::EnableWindow(::GetDlgItem(hDlg, IDC_SEARCH_BACKWARD), FALSE);
		} else {
			::SetWindowText(::GetDlgItem(hDlg, IDC_SEARCH_BACKWARD), "中断");
			::EnableWindow(::GetDlgItem(hDlg, IDC_SEARCH_FORWARD), FALSE);
		}
	}
	::EnableWindow(::GetDlgItem(hDlg, IDC_SEARCHDATA), bEnable);
	::EnableWindow(::GetDlgItem(hDlg, IDC_DT_HEX), bEnable);
	::EnableWindow(::GetDlgItem(hDlg, IDC_DT_STRING), bEnable);
	::EnableWindow(::GetDlgItem(hDlg, IDOK), bEnable);
}

static BOOL
SearchDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static bool bSearching = false;

	switch (uMsg) {
	case WM_INITDIALOG:
		::SendMessage(::GetDlgItem(hDlg, IDC_DT_HEX),
					  BM_SETCHECK, BST_CHECKED, 0);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SEARCH_FORWARD:
			if (!bSearching) {
				if (bSearching = SearchData(hDlg, FIND_FORWARD)) {
					EnableSearchDlgControls(hDlg, FIND_FORWARD, FALSE);
					::EnableWindow(g_hwndMain, FALSE);
				}
			} else {
				g_pViewFrame->stopFind();
				// コールバック関数により WM_USER_FIND_FINISH が投げられる
			}
			break;
		case IDC_SEARCH_BACKWARD:
			if (!bSearching) {
				if (bSearching = SearchData(hDlg, FIND_BACKWARD)) {
					EnableSearchDlgControls(hDlg, FIND_BACKWARD, FALSE);
					::EnableWindow(g_hwndMain, FALSE);
				}
			} else {
				g_pViewFrame->stopFind();
				// コールバック関数により WM_USER_FIND_FINISH が投げられる
			}
			break;
		case IDOK:
			::DestroyWindow(hDlg);
			break;
		}
		break;

	case WM_USER_FIND_FINISH:
		if (bSearching) {
			g_pViewFrame->cleanupCallback();
			EnableSearchDlgControls(hDlg, 0, TRUE);
			::EnableWindow(g_hwndMain, TRUE);
			bSearching = false;
		}
		break;

	case WM_MOUSEWHEEL:
		if (!bSearching) {
			g_pViewFrame->onMouseWheel(wParam, lParam);
		}
		break;

	case WM_CLOSE:
		::DestroyWindow(hDlg);
		break;

	case WM_DESTROY:
		{
			if (bSearching) {
				bSearching = false;
				g_pViewFrame->stopFind();
				g_pViewFrame->cleanupCallback();
				::EnableWindow(g_hwndMain, TRUE);
			}
			g_hSearchDlg = NULL;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

static BOOL
JumpAddrDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			::SetWindowLong(hDlg, DWL_USER, (LONG)lParam);
			filesize_t size = g_pViewFrame->getFileSize();
			char str[32];
			wsprintf(str, "0 - 0x%08x%08x",
					 (int)(size >> 32), (int)size);
			::SetWindowText(::GetDlgItem(hDlg, IDC_JUMPINFO), str);
			HWND hEditCtrl = ::GetDlgItem(hDlg, IDC_JUMPADDRESS);
			filesize_t pos = g_pViewFrame->getPosition();
			wsprintf(str, "0x%08x%08x",
					 (int)(pos >> 32), (int)pos);
			::SetWindowText(hEditCtrl, str);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				HWND hCtrl = ::GetDlgItem(hDlg, IDC_JUMPADDRESS);
				char buf[32];
				::GetWindowText(hCtrl, buf, 32);
				filesize_t* ppos = (filesize_t*)::GetWindowLong(hDlg, DWL_USER);
				*ppos = ParseNumber(buf);
			}
			// through down
		case IDCANCEL:
			::EndDialog(hDlg, 0);
			break;
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

static bool
GetImageFileName(LPSTR buf, int bufsize)
{
	buf[0] = '\0';

	OPENFILENAME ofn;

	::ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize  = sizeof(OPENFILENAME);
	ofn.hInstance    = g_hInstance;
	ofn.hwndOwner    = g_hwndMain;
	ofn.lpstrFilter  = "全てのファイル\0*.*\0\0";
	ofn.lpstrFile    = buf;
	ofn.nMaxFile     = bufsize;
	ofn.lpstrTitle   = "ファイルの選択";
	ofn.Flags        = OFN_FILEMUSTEXIST | OFN_EXPLORER;
	ofn.lpstrInitialDir = "";

	return ::GetOpenFileName(&ofn) != 0;
}

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
	::SetWindowPos(hWnd, NULL,
				   rctWnd.left, rctWnd.top,
				   rctWnd.right - rctWnd.left,
				   rctWnd.bottom - rctWnd.top,
				   SWP_NOZORDER);

	g_pViewFrame->setFrameRect(rctClient);
}

static void
EnableEditMenu(HWND hWnd, BOOL bEnable)
{
	HMENU hMenu = ::GetMenu(hWnd);
	assert(hMenu);
	::EnableMenuItem(hMenu, 1,
					 MF_BYPOSITION | (bEnable ? MF_ENABLED : MF_GRAYED));
	::SendMessage(hWnd, WM_NCPAINT, 1, 0);
}

static BOOL
LoadFile(const string& filename)
{
//	assert(!g_pViewFrame->isLoaded());
	if (g_pViewFrame->isLoaded()) {
		HMODULE hModule = ::GetModuleHandle(g_strAppName.c_str());
		assert(hModule);
		char cmdline[3 * MAX_PATH];
		::GetModuleFileName(hModule, cmdline, 3 * MAX_PATH);
		int len = lstrlen(cmdline);
		cmdline[len++] = ' ';
		wsprintf(cmdline + len, "\"%s\"", filename.c_str());
		return ::WinExec(cmdline, SW_SHOW) < 31;
	}

	BOOL bRet = TRUE;
	try {
		g_strImageFile = filename;
		g_pLFReader = new LargeFileReader(filename);
		g_pViewFrame->loadFile(g_pLFReader.ptr());
		g_pViewFrame->setPosition(0);

		char buf[1024];
		wsprintf(buf, "BinViewer - %s", g_strImageFile.c_str());
		::SetWindowText(g_hwndMain, buf);
	} catch (...) {
		::MessageBox(g_hwndMain,
					 "ファイルが開けませんでした。",
					 "BinViewer エラー",
					 MB_OK);
		g_strImageFile = "";
		::SetWindowText(g_hwndMain, "BinViewer");
		bRet = FALSE;
	}

	EnableEditMenu(g_hwndMain, bRet);

	return bRet;
}

static void OnSetPosition(HWND, WPARAM, LPARAM);

static void
UnloadFile()
{
	g_pViewFrame->unloadFile();
	g_pLFReader = NULL;

	g_strImageFile = "";
	::SetWindowText(g_hwndMain, "BinViewer");

	EnableEditMenu(g_hwndMain, FALSE);
	OnSetPosition(g_hwndMain, 0, 0);
}

static void
OnSetPosition(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	char msgbuf[80];
	wsprintf(msgbuf, "カーソルの現在位置： 0x%08X%08X", wParam, lParam);
	::SetWindowText(g_hwndStatusBar, msgbuf);
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

	// create default DrawInfo
	HDC hDC = ::GetDC(hWnd);
	g_pDrawInfo = new DrawInfo(hDC, DEFAULT_FONT_SIZE,
							   DEFAULT_FG_COLOR, DEFAULT_BK_COLOR,
							   DEFAULT_FG_COLOR_HEADER, DEFAULT_BK_COLOR_HEADER);
	::ReleaseDC(hWnd, hDC);

	g_pViewFrame = new ViewFrame(hWnd, rctClient, g_pDrawInfo.ptr(), NULL);
	assert(g_pViewFrame.ptr());

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

	if (g_strImageFile.length() == 0) {
		EnableEditMenu(hWnd, FALSE);
		OnSetPosition(hWnd, 0, 0);
		return;
	}

	assert(!g_pViewFrame->isLoaded());

	LoadFile(g_strImageFile);
}

static void
OnQuit(HWND)
{
	g_pLFReader = NULL;
	g_pViewFrame = NULL;
}

static void
OnUnloadFile(HWND)
{
	UnloadFile();
}

static void
OnLoadFile(HWND hWnd)
{
	char buf[MAX_PATH];
	if (GetImageFileName(buf, MAX_PATH)) {
		LoadFile(buf);
	}
}

static void
OnDropFiles(HWND hWnd, WPARAM wParam)
{
	HDROP hDrop = (HDROP)wParam;

	UINT nFiles = ::DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);

	for (UINT i = 0; i < nFiles; i++) {
		UINT nSize = ::DragQueryFile(hDrop, i, NULL, 0) + 1;
		char* buf = new char[nSize];

		::DragQueryFile(hDrop, i, buf, nSize);

		LoadFile(buf);

		delete [] buf;
	}

	::DragFinish(hDrop);
}

LRESULT CALLBACK
MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		OnCreate(hWnd);
		break;

	case WM_DROPFILES:
		OnDropFiles(hWnd, wParam);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDR_OPEN:
			OnLoadFile(hWnd);
			break;

		case IDR_CLOSE:
			OnUnloadFile(hWnd);
			break;

		case IDR_QUIT:
			::DestroyWindow(hWnd);
			break;

		case IDR_HELP:
			break;

		case IDR_SEARCH:
			{
				if (g_hSearchDlg) break;
				g_hSearchDlg = ::CreateDialogParam(g_hInstance, MAKEINTRESOURCE(IDD_SEARCH),
												   hWnd, (DLGPROC)SearchDlgProc,
												   (LPARAM)NULL);
				if (!g_hSearchDlg) {
					::MessageBox(hWnd, "検索ダイアログの表示に失敗しました", NULL, MB_OK);
					break;
				}
			}
			break;

		case IDR_JUMP:
			{
				// show dialog box
				// parse string and return filesize_t pos
				// set position to pos
				filesize_t pos = -1;
				::DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_JUMP),
								 hWnd, (DLGPROC)JumpAddrDlgProc, (LPARAM)&pos);
				if (pos >= 0) g_pViewFrame->onJump(pos);
			}
			break;

		case IDK_LINEDOWN:
			g_pViewFrame->onVerticalMove(1);
			break;

		case IDK_LINEUP:
			g_pViewFrame->onVerticalMove(-1);
			break;

		case IDK_PAGEDOWN:
		case IDK_SPACE:
			g_pViewFrame->onVScroll(SB_PAGEDOWN, 0);
			break;

		case IDK_PAGEUP:
		case IDK_S_SPACE:
			g_pViewFrame->onVScroll(SB_PAGEUP, 0);
			break;

		case IDK_BOTTOM:
		case IDK_C_SPACE:
			g_pViewFrame->onVScroll(SB_BOTTOM, 0);
			break;

		case IDK_TOP:
		case IDK_C_S_SPACE:
			g_pViewFrame->onVScroll(SB_TOP, 0);
			break;

		case IDK_RIGHT:
			g_pViewFrame->onHorizontalMove(1);
			break;

		case IDK_LEFT:
			g_pViewFrame->onHorizontalMove(-1);
			break;
		}
		break;

	case WM_USER_SETPOSITION:
		OnSetPosition(hWnd, wParam, lParam);
		break;

	case WM_SIZING:
		OnResizing(hWnd, (RECT*)lParam);
		return TRUE;

	case WM_SIZE:
		OnResize(hWnd, wParam);
		break;

	case WM_MOUSEWHEEL:
		g_pViewFrame->onMouseWheel(wParam, lParam);
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

	GetParameters();

	WNDCLASS wc;

	// Main Window's WindowClass
	::ZeroMemory(&wc, sizeof(WNDCLASS));
	wc.hInstance = hInstance;
	wc.style = CS_OWNDC /* | CS_HREDRAW | CS_VREDRAW */;
	wc.hIcon = ::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
//	wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
	wc.lpfnWndProc = (WNDPROC)MainWndProc;
	wc.lpszClassName = MAINWND_CLASSNAME;
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);
	if (!::RegisterClass(&wc)) return -1;

	HACCEL hAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_KEYACCEL));

	::InitCommonControls();

	g_hwndMain = ::CreateWindowEx(WS_EX_OVERLAPPEDWINDOW | WS_EX_ACCEPTFILES,
								  wc.lpszClassName, "BinViewer",
								  WS_OVERLAPPEDWINDOW,
								  CW_USEDEFAULT, CW_USEDEFAULT,
								  W_WIDTH, W_HEIGHT,
								  NULL, NULL, hInstance, NULL);
	if (!g_hwndMain) return -1;

	::ShowWindow(g_hwndMain, SW_SHOW);

	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		if (!::IsDialogMessage(g_hSearchDlg, &msg) &&
			!::TranslateAccelerator(g_hwndMain, hAccel, &msg)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}

