// $Id$

#pragma warning(disable : 4786)

#define _WIN32_WINNT  0x500  // to support mouse wheel

#include <windows.h>
#include <commctrl.h>
#include <assert.h>

#include "resource.h"
#include "strutils.h"
#include "viewframe.h"
#include "searchdlg.h"
#include "configdlg.h"
#include "messages.h"

#define ADDR_WIDTH   100
#define BYTE_WIDTH    32
#define DATA_WIDTH   160

#define W_WIDTH  (ADDR_WIDTH + BYTE_WIDTH * 16 + 20 + DATA_WIDTH + 20)
#define W_HEIGHT 720

#define COLOR_BLACK      RGB(0, 0, 0)
#define COLOR_GRAY       RGB(128, 128, 128)
#define COLOR_LIGHTGRAY  RGB(192, 192, 192)
#define COLOR_WHITE      RGB(255, 255, 255)
#define COLOR_YELLOW     RGB(255, 255, 0)

#define STATUSBAR_HEIGHT  20

#define IDC_STATUSBAR  10

#define STATUS_POS_HEADER    "カーソルの現在位置： 0x"

#define MAINWND_CLASSNAME    "BinViewerClass32"

#define REG_ROOT        "Software\\SugiApp\\BinViewer"
#define RK_FONTNAME     "FontFace"
#define RK_FONTSIZE     "FontSize"
#define RK_IS_BOLD      "IsBoldFont"
#define RK_HEADER_FGC   "Header_FgColor"
#define RK_HEADER_BKC   "Header_BkColor"
#define RK_ADDRESS_FGC  "Address_FgColor"
#define RK_ADDRESS_BKC  "Address_BkColor"
#define RK_DATA_FGC     "Data_FgColor"
#define RK_DATA_BKC     "Data_BkColor"
#define RK_STRING_FGC   "String_FgColor"
#define RK_STRING_BKC   "String_BkColor"
#define RK_CARET_MOVE   "Caret_Move"
#define RK_WHEEL_SCROLL "Wheel_Scroll"

// resources
static HINSTANCE g_hInstance;
static HWND g_hwndMain, g_hwndStatusBar;
static string g_strAppName;

// window properties
static Auto_Ptr<DrawInfo> g_pDrawInfo(NULL);
#define DEFAULT_FONT_SIZE  12
#define DEFAULT_FG_COLOR_ADDRESS COLOR_WHITE
#define DEFAULT_BK_COLOR_ADDRESS COLOR_GRAY
#define DEFAULT_FG_COLOR_DATA    COLOR_BLACK
#define DEFAULT_BK_COLOR_DATA    COLOR_WHITE
#define DEFAULT_FG_COLOR_STRING  COLOR_BLACK
#define DEFAULT_BK_COLOR_STRING  COLOR_LIGHTGRAY
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

static void
LoadConfig(Auto_Ptr<DrawInfo>& pDrawInfo)
{
	assert(!pDrawInfo.ptr());

	HKEY hKeyRoot = NULL;
	if (::RegOpenKeyEx(HKEY_CURRENT_USER,
						REG_ROOT,
						0, KEY_ALL_ACCESS,
						&hKeyRoot) != ERROR_SUCCESS) {
		pDrawInfo = new DrawInfo(NULL, DEFAULT_FONT_SIZE, "FixedSys", false,
								 DEFAULT_FG_COLOR_ADDRESS, DEFAULT_BK_COLOR_ADDRESS,
								 DEFAULT_FG_COLOR_DATA, DEFAULT_BK_COLOR_DATA,
								 DEFAULT_FG_COLOR_STRING, DEFAULT_BK_COLOR_STRING,
								 DEFAULT_FG_COLOR_HEADER, DEFAULT_BK_COLOR_HEADER,
								 CARET_STATIC, WHEEL_AS_ARROW_KEYS);
		return;
	}

	DWORD dwType, dwSize;

	dwType = REG_BINARY;
	dwSize = sizeof(float);
	float fontsize;
	if (::RegQueryValueEx(hKeyRoot, RK_FONTSIZE, 0,
						  &dwType, (BYTE*)&fontsize, &dwSize) != ERROR_SUCCESS) {
		fontsize = DEFAULT_FONT_SIZE;
	}

	dwType = REG_SZ;
	dwSize = LF_FACESIZE;
	char fontface[LF_FACESIZE];
	if (::RegQueryValueEx(hKeyRoot, RK_FONTNAME, 0,
						  &dwType, (BYTE*)&fontface, &dwSize) != ERROR_SUCCESS) {
		lstrcpy(fontface, "FixedSys");
	}

	dwType = REG_DWORD;
	dwSize = sizeof(DWORD);
	DWORD dwBoldFace;
	if (::RegQueryValueEx(hKeyRoot, RK_IS_BOLD, 0,
						  &dwType, (BYTE*)&dwBoldFace, &dwSize) != ERROR_SUCCESS) {
		dwBoldFace = 0;
	}

	COLORREF crFgHeader, crBkHeader;
	if (::RegQueryValueEx(hKeyRoot, RK_HEADER_FGC, 0,
						  &dwType, (BYTE*)&crFgHeader, &dwSize) != ERROR_SUCCESS) {
		crFgHeader = DEFAULT_FG_COLOR_HEADER;
	}
	if (::RegQueryValueEx(hKeyRoot, RK_HEADER_BKC, 0,
						  &dwType, (BYTE*)&crBkHeader, &dwSize) != ERROR_SUCCESS) {
		crBkHeader = DEFAULT_BK_COLOR_HEADER;
	}
	COLORREF crFgAddress, crBkAddress;
	if (::RegQueryValueEx(hKeyRoot, RK_ADDRESS_FGC, 0,
						  &dwType, (BYTE*)&crFgAddress, &dwSize) != ERROR_SUCCESS) {
		crFgAddress = DEFAULT_FG_COLOR_ADDRESS;
	}
	if (::RegQueryValueEx(hKeyRoot, RK_ADDRESS_BKC, 0,
						  &dwType, (BYTE*)&crBkAddress, &dwSize) != ERROR_SUCCESS) {
		crBkAddress = DEFAULT_BK_COLOR_ADDRESS;
	}
	COLORREF crFgData, crBkData;
	if (::RegQueryValueEx(hKeyRoot, RK_DATA_FGC, 0,
						  &dwType, (BYTE*)&crFgData, &dwSize) != ERROR_SUCCESS) {
		crFgData = DEFAULT_FG_COLOR_DATA;
	}
	if (::RegQueryValueEx(hKeyRoot, RK_DATA_BKC, 0,
						  &dwType, (BYTE*)&crBkData, &dwSize) != ERROR_SUCCESS) {
		crBkData = DEFAULT_BK_COLOR_DATA;
	}
	COLORREF crFgString, crBkString;
	if (::RegQueryValueEx(hKeyRoot, RK_STRING_FGC, 0,
						  &dwType, (BYTE*)&crFgString, &dwSize) != ERROR_SUCCESS) {
		crFgString = DEFAULT_FG_COLOR_STRING;
	}
	if (::RegQueryValueEx(hKeyRoot, RK_STRING_BKC, 0,
						  &dwType, (BYTE*)&crBkString, &dwSize) != ERROR_SUCCESS) {
		crBkString = DEFAULT_BK_COLOR_STRING;
	}

	DWORD dwCaretMove;
	if (::RegQueryValueEx(hKeyRoot, RK_CARET_MOVE, 0,
						  &dwType, (BYTE*)&dwCaretMove, &dwSize) != ERROR_SUCCESS) {
		dwCaretMove = CARET_STATIC;
	}
	DWORD dwWheelScroll;
	if (::RegQueryValueEx(hKeyRoot, RK_WHEEL_SCROLL, 0,
						  &dwType, (BYTE*)&dwWheelScroll, &dwSize) != ERROR_SUCCESS) {
		dwWheelScroll = WHEEL_AS_ARROW_KEYS;
	}

	::RegCloseKey(hKeyRoot);

	pDrawInfo = new DrawInfo(NULL, fontsize, fontface, dwBoldFace != 0,
							 crFgAddress, crBkAddress,
							 crFgData, crBkData,
							 crFgString, crBkString,
							 crFgHeader, crBkHeader,
							 (CARET_MOVE)dwCaretMove,
							 (WHEEL_SCROLL)dwWheelScroll);
}

static void
SaveConfig(const Auto_Ptr<DrawInfo>& pDrawInfo)
{
	HKEY hKeyRoot;
	if (::RegCreateKeyEx(HKEY_CURRENT_USER,
						 REG_ROOT,
						 0, NULL,
						 REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
						 NULL,
						 &hKeyRoot, NULL) != ERROR_SUCCESS) {
		assert(0);
		return;
	}

	DWORD dwType, dwSize;

	dwType = REG_BINARY;
	dwSize = sizeof(float);
	float fontsize = pDrawInfo->m_FontInfo.getFontSize();
	::RegSetValueEx(hKeyRoot, RK_FONTSIZE, 0,
					dwType, (BYTE*)&fontsize, dwSize);

	dwType = REG_SZ;
	dwSize = lstrlen(pDrawInfo->m_FontInfo.getFaceName());
	::RegSetValueEx(hKeyRoot, RK_FONTNAME, 0,
					dwType, (BYTE*)pDrawInfo->m_FontInfo.getFaceName(), dwSize);

	dwType = REG_DWORD;
	dwSize = sizeof(DWORD);
	DWORD dwBoldFace = pDrawInfo->m_FontInfo.isBoldFace();
	::RegSetValueEx(hKeyRoot, RK_IS_BOLD, 0,
					dwType, (BYTE*)&dwBoldFace, dwSize);

	COLORREF crFgHeader = pDrawInfo->m_tciHeader.getFgColor(),
			 crBkHeader = pDrawInfo->m_tciHeader.getBkColor();
	::RegSetValueEx(hKeyRoot, RK_HEADER_FGC, 0,
					dwType, (BYTE*)&crFgHeader, dwSize);
	::RegSetValueEx(hKeyRoot, RK_HEADER_BKC, 0,
					dwType, (BYTE*)&crBkHeader, dwSize);

	COLORREF crFgAddress = pDrawInfo->m_tciAddress.getFgColor(),
			 crBkAddress = pDrawInfo->m_tciAddress.getBkColor();
	::RegSetValueEx(hKeyRoot, RK_ADDRESS_FGC, 0,
					dwType, (BYTE*)&crFgAddress, dwSize);
	::RegSetValueEx(hKeyRoot, RK_ADDRESS_BKC, 0,
					dwType, (BYTE*)&crBkAddress, dwSize);

	COLORREF crFgData = pDrawInfo->m_tciData.getFgColor(),
			 crBkData = pDrawInfo->m_tciData.getBkColor();
	::RegSetValueEx(hKeyRoot, RK_DATA_FGC, 0,
					dwType, (BYTE*)&crFgData, dwSize);
	::RegSetValueEx(hKeyRoot, RK_DATA_BKC, 0,
					dwType, (BYTE*)&crBkData, dwSize);

	COLORREF crFgString = pDrawInfo->m_tciString.getFgColor(),
			 crBkString = pDrawInfo->m_tciString.getBkColor();
	::RegSetValueEx(hKeyRoot, RK_STRING_FGC, 0,
					dwType, (BYTE*)&crFgString, dwSize);
	::RegSetValueEx(hKeyRoot, RK_STRING_BKC, 0,
					dwType, (BYTE*)&crBkString, dwSize);

	DWORD dwCaretMove = pDrawInfo->m_ScrollConfig.m_caretMove;
	::RegSetValueEx(hKeyRoot, RK_CARET_MOVE, 0,
					dwType, (BYTE*)&dwCaretMove, dwSize);

	DWORD dwWheelScroll = pDrawInfo->m_ScrollConfig.m_wheelScroll;
	::RegSetValueEx(hKeyRoot, RK_WHEEL_SCROLL, 0,
					dwType, (BYTE*)&dwWheelScroll, dwSize);

	::RegCloseKey(hKeyRoot);
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
//			wsprintf(str, "0 - 0x%08x%08x",
//					 (int)(size >> 32), (int)size);
			::CopyMemory(str, "0 - 0x", 6);
			QuadToStr((UINT)size, (UINT)(size >> 32), str + 6);
			str[22] = '\0';
			::SetWindowText(::GetDlgItem(hDlg, IDC_JUMPINFO), str);
			HWND hEditCtrl = ::GetDlgItem(hDlg, IDC_JUMPADDRESS);
			filesize_t pos = g_pViewFrame->getPosition();
//			wsprintf(str, "0x%08x%08x",
//					 (int)(pos >> 32), (int)pos);
			str[0] = '0';  str[1] = 'x';
			QuadToStr((UINT)pos, (UINT)(pos >> 32), str + 2);
			str[18] = '\0';
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
	static int hlen = lstrlen(STATUS_POS_HEADER);
	char msgbuf[40];
	::CopyMemory(msgbuf, STATUS_POS_HEADER, hlen);
	QuadToStr((UINT)lParam, (UINT)wParam, msgbuf + hlen);
	msgbuf[hlen + 16] = '\0';
//	::SetWindowText(g_hwndStatusBar, msgbuf);
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

	// create default DrawInfo
	LoadConfig(g_pDrawInfo);
	assert(g_pDrawInfo.ptr());

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

	HDC hDC = ::GetDC(g_hwndStatusBar);
	char status[80];
	lstrcpy(status, STATUS_POS_HEADER);
	lstrcat(status, "0000000000000000");
	SIZE tsize;
	::GetTextExtentPoint32(hDC, status, lstrlen(status), &tsize);
	::SendMessage(g_hwndStatusBar, SB_SETPARTS, 1, (LPARAM)&tsize.cx);
	::ReleaseDC(g_hwndStatusBar, hDC);

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
	SearchDlg::close();
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

void
OnSetFontConfig(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	FontConfig*  pFontConfig  = (FontConfig*)wParam;
	ColorConfig* pColorConfig = (ColorConfig*)lParam;

	if (pFontConfig)
		g_pDrawInfo->m_FontInfo.setFont(g_pDrawInfo->m_hDC, *pFontConfig);

	if (pColorConfig) {
		g_pDrawInfo->m_tciHeader.setColor(pColorConfig[TCI_HEADER]);
		g_pDrawInfo->m_tciAddress.setColor(pColorConfig[TCI_ADDRESS]);
		g_pDrawInfo->m_tciData.setColor(pColorConfig[TCI_DATA]);
		g_pDrawInfo->m_tciString.setColor(pColorConfig[TCI_STRING]);
	}

	if (pFontConfig || pColorConfig)
		g_pViewFrame->setDrawInfo(g_pDrawInfo.ptr());

	SaveConfig(g_pDrawInfo);
}

void
OnSetScrollConfig(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if (!lParam) return;

	g_pDrawInfo->m_ScrollConfig = *(ScrollConfig*)lParam;
	g_pViewFrame->setDrawInfo(g_pDrawInfo.ptr());

	SaveConfig(g_pDrawInfo);
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
		case IDM_OPEN:
			OnLoadFile(hWnd);
			break;

		case IDM_CLOSE:
			OnUnloadFile(hWnd);
			break;

		case IDM_QUIT:
			::DestroyWindow(hWnd);
			break;

		case IDM_HELP:
			break;

		case IDM_CONFIG:
			ConfigMainDlg(g_pDrawInfo.ptr()).doModal(hWnd);
			break;

		case IDM_SEARCH:
			if (!SearchDlg::create(hWnd, g_pViewFrame.ptr())) {
				::MessageBox(hWnd, "検索ダイアログの表示に失敗しました", NULL, MB_OK);
			}
			break;

		case IDM_JUMP:
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

	case WM_USER_SET_FONT_CONFIG:
		OnSetFontConfig(hWnd, wParam, lParam);
		break;

	case WM_USER_SET_SCROLL_CONFIG:
		OnSetScrollConfig(hWnd, wParam, lParam);
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
	wc.lpszMenuName = MAKEINTRESOURCE(IDM_MAINMENU);
	if (!::RegisterClass(&wc)) return -1;

	HACCEL hAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_KEYACCEL));

	::InitCommonControls();

	g_hwndMain = ::CreateWindowEx(/*WS_EX_OVERLAPPEDWINDOW |*/ WS_EX_ACCEPTFILES,
								  wc.lpszClassName, "BinViewer",
//								  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
//								   WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
								  WS_OVERLAPPEDWINDOW,
								  CW_USEDEFAULT, CW_USEDEFAULT,
								  W_WIDTH, W_HEIGHT,
								  NULL, NULL, hInstance, NULL);
	if (!g_hwndMain) return -1;

	::ShowWindow(g_hwndMain, SW_SHOW);

	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		if (!SearchDlg::isDialogMessage(&msg) &&
			!::TranslateAccelerator(g_hwndMain, hAccel, &msg)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}

