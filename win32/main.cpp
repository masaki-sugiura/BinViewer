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
#define W_HEIGHT 760

#define COLOR_WHITE  RGB(255, 255, 255)
#define COLOR_BLACK  RGB(0, 0, 0)
#define COLOR_YELLOW RGB(255, 255, 0)

#define WM_USER_FIND_FINISH  (WM_USER + 1000)

// resources
static HINSTANCE g_hInstance;
static HWND g_hwndMain;
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
UpdateClientRect()
{
	RECT rctClient;
	::GetClientRect(g_hwndMain, &rctClient);
	rctClient.top = g_pViewFrame->getLineHeight();
	::InvalidateRect(g_hwndMain, &rctClient, FALSE);
	::UpdateWindow(g_hwndMain);
}

static void OnJump(HWND hWnd, filesize_t pos);

static void
FindCallbackProc(void* arg)
{
	assert(arg);

	FindCallbackArg* pArg = (FindCallbackArg*)arg;

	if (pArg->m_qFindAddress >= 0 && ::IsWindow(g_hSearchDlg)) {
		g_pViewFrame->select(pArg->m_qFindAddress, pArg->m_nBufSize);
		OnJump(g_hwndMain, pArg->m_qFindAddress);
	}

	if (::IsWindow(g_hSearchDlg))
		::PostMessage(g_hSearchDlg, WM_USER_FIND_FINISH, 0, 0);

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
	pFindCallbackArg->m_qStartAddress = g_pViewFrame->getPosition();
	pFindCallbackArg->m_pfnCallback = FindCallbackProc;
	pFindCallbackArg->m_qFindAddress = -1;

	g_pViewFrame->unselect();
	UpdateClientRect();

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
			g_pViewFrame->unselect();
			UpdateClientRect();
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
	RECT rctWnd, rctClient;
	::GetWindowRect(hWnd, &rctWnd);
	::GetClientRect(hWnd, &rctClient);

	rctWnd.right  += rctFrame.right  - rctClient.right;
	rctWnd.bottom += rctFrame.bottom - rctClient.bottom;
	::SetWindowPos(hWnd, NULL,
				   rctWnd.left, rctWnd.top,
				   rctWnd.right - rctWnd.left,
				   rctWnd.bottom - rctWnd.top,
				   SWP_NOZORDER);
}

static void
InitScrollInfo(HWND hWnd)
{
	int nPageLineNum = g_pViewFrame->getPageLineNum();

	SCROLLINFO sinfo;
	sinfo.cbSize = sizeof(sinfo);
	sinfo.fMask = SIF_DISABLENOSCROLL | SIF_POS | SIF_RANGE | SIF_PAGE;
	sinfo.nPos = 0;
	sinfo.nMin = 0;

	if (g_pViewFrame->getFileSize() < 0) {
		// ファイルが読み込まれていない状態
		sinfo.nMax  = 1;
		sinfo.nPage = 2;
	} else {
		filesize_t pos = g_pViewFrame->getPosition();
		filesize_t qMaxLine = g_pViewFrame->getMaxLine();
		assert(qMaxLine >= 0);

		if (qMaxLine & ~0x7FFFFFFF) {
			// スクロールバーが native に扱えるファイルサイズを越えている
			g_bMapScrollBarLinearly = false;
			sinfo.nMax = 0x7FFFFFFF;
			sinfo.nPage = (DWORD)(((filesize_t)nPageLineNum << 32) / qMaxLine);
			sinfo.nPos = (pos << 32) / qMaxLine;
		} else {
			g_bMapScrollBarLinearly = true;
			// nPageLineNum は半端な最下行を含むので -1 しておく
			sinfo.nPage = nPageLineNum - 1;
			sinfo.nMax = (int)qMaxLine + sinfo.nPage - 1;
			sinfo.nPos = (int)pos;
		}
		if (!sinfo.nPage) sinfo.nPage = 1;
	}

	::SetScrollInfo(hWnd, SB_VERT, &sinfo, TRUE);
}

static void
ModifyHScrollInfo(HWND hWnd, int width)
{
	SCROLLINFO sinfo;
	sinfo.cbSize = sizeof(sinfo);
	sinfo.fMask = SIF_DISABLENOSCROLL | SIF_POS | SIF_RANGE | SIF_PAGE;
	sinfo.nPos = 0;
	sinfo.nMin = 0;

	width /= g_pViewFrame->getCharWidth();

	if (width >= WIDTH_PER_XPITCH) {
		// HSCROLL を無効化
		sinfo.nMax  = 1;
		sinfo.nPage = 2;
	} else {
		sinfo.nMax  = WIDTH_PER_XPITCH - 1;
		sinfo.nPage = width;
	}

	::SetScrollInfo(hWnd, SB_HORZ, &sinfo, TRUE);
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

	InitScrollInfo(g_hwndMain);

//	UpdateClientRect();
	::InvalidateRect(g_hwndMain, NULL, FALSE);
	::UpdateWindow(g_hwndMain);

	EnableEditMenu(g_hwndMain, bRet);

	return bRet;
}

static void
UnloadFile()
{
	g_pViewFrame->unloadFile();
	g_pLFReader = NULL;

	g_strImageFile = "";
	::SetWindowText(g_hwndMain, "BinViewer");

	InitScrollInfo(g_hwndMain);

	//	UpdateClientRect();
	::InvalidateRect(g_hwndMain, NULL, FALSE);
	::UpdateWindow(g_hwndMain);

	EnableEditMenu(g_hwndMain, FALSE);
}

static void
OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hDC = ::BeginPaint(hWnd, &ps);
	g_pViewFrame->bitBlt(hDC, ps.rcPaint);
	::EndPaint(hWnd, &ps);
}

static void
OnResizing(HWND hWnd, RECT* prctNew)
{
	RECT rctWnd, rctClient;

	::GetWindowRect(hWnd, &rctWnd);
	::GetClientRect(hWnd, &rctClient);

	int width = rctClient.right - rctClient.left
				+ (prctNew->right - prctNew->left)
				- (rctWnd.right - rctWnd.left);

	ModifyHScrollInfo(hWnd, width);
}

static void
OnResize(HWND hWnd, WPARAM fwSizeType)
{
	RECT rctClient;
	::GetClientRect(hWnd, &rctClient);
	g_pViewFrame->setFrameRect(rctClient);
	InitScrollInfo(hWnd);
	ModifyHScrollInfo(hWnd, rctClient.right - rctClient.left);
}

static void
OnJump(HWND hWnd, filesize_t pos)
{
	if (!g_pViewFrame->isLoaded()) return;

	filesize_t qMaxLine = g_pViewFrame->getMaxLine();

	pos >>= 4;

	SCROLLINFO sinfo;
	sinfo.cbSize = sizeof(SCROLLINFO);
	sinfo.fMask = SIF_ALL;

	::GetScrollInfo(hWnd, SB_VERT, &sinfo);
	if (pos < 0) {
		pos = 0;
		sinfo.nPos = sinfo.nMin;
	} else if (pos > qMaxLine) {
		pos = qMaxLine;
		sinfo.nPos = sinfo.nMax;
	} else {
		if (!g_bMapScrollBarLinearly) {
			// ファイルサイズが大きい場合
			sinfo.nPos = (pos << 32) / qMaxLine;
		} else {
			sinfo.nPos = (int)pos;
		}
	}
	::SetScrollInfo(hWnd, SB_VERT, &sinfo, TRUE);

	// prepare the correct BGBuffer
	g_pViewFrame->setPosition(pos << 4);

	// get the region of BGBuffer to be drawn
	UpdateClientRect();
}

static void
OnVScroll(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if (!g_pViewFrame->isLoaded()) return;

	SCROLLINFO sinfo;
	sinfo.cbSize = sizeof(SCROLLINFO);
	sinfo.fMask = SIF_ALL;

	::GetScrollInfo(hWnd, SB_VERT, &sinfo);
	if (sinfo.nMax <= sinfo.nPage) return;

	filesize_t qPosition = g_pViewFrame->getPosition(),
			   qMaxLine = g_pViewFrame->getMaxLine();
	int nPageLineNum = g_pViewFrame->getPageLineNum();

	switch (LOWORD(wParam)) {
	case SB_LINEDOWN:
		if ((qPosition / 16) < qMaxLine) {
			qPosition += 16;
			if (!g_bMapScrollBarLinearly) {
				// ファイルサイズが大きい場合
				sinfo.nPos = ((qPosition / 16) << 32) / qMaxLine;
			} else {
				sinfo.nPos++;
			}
		}
		break;

	case SB_LINEUP:
		if ((qPosition / 16) > 0) {
			qPosition -= 16;
			if (!g_bMapScrollBarLinearly) {
				// ファイルサイズが大きい場合
				sinfo.nPos = ((qPosition / 16) << 32) / qMaxLine;
			} else {
				sinfo.nPos--;
			}
		}
		break;

	case SB_PAGEDOWN:
		if (((qPosition += 16 * nPageLineNum) / 16) > qMaxLine) {
			qPosition = qMaxLine * 16;
			sinfo.nPos = sinfo.nMax;
		} else {
			if (!g_bMapScrollBarLinearly) {
				// ファイルサイズが大きい場合
				sinfo.nPos = ((qPosition / 16) << 32) / qMaxLine;
			} else {
				sinfo.nPos += sinfo.nPage;
			}
		}
		break;

	case SB_PAGEUP:
		if (((qPosition -= nPageLineNum * 16) / 16) < 0) {
			qPosition = 0;
			sinfo.nPos = sinfo.nMin;
		} else {
			if (!g_bMapScrollBarLinearly) {
				// ファイルサイズが大きい場合
				sinfo.nPos = ((qPosition / 16) << 32) / qMaxLine;
			} else {
				sinfo.nPos -= sinfo.nPage;
			}
		}
		break;

	case SB_TOP:
		qPosition = 0;
		sinfo.nPos = sinfo.nMin;
		break;

	case SB_BOTTOM:
		qPosition = qMaxLine * 16;
		sinfo.nPos = sinfo.nMax;
		break;

	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
		sinfo.nPos = sinfo.nTrackPos;
		if (!g_bMapScrollBarLinearly) {
			// ファイルサイズが大きい場合
			qPosition = (sinfo.nPos * qMaxLine * 16) >> 32;
		} else {
			qPosition = sinfo.nPos * 16;
		}
		break;

	default:
		return;
	}
	::SetScrollInfo(hWnd, SB_VERT, &sinfo, TRUE);

	// prepare the correct BGBuffer
	g_pViewFrame->setPosition(qPosition);

	// get the region of BGBuffer to be drawn
	UpdateClientRect();
}

static void
OnHScroll(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	SCROLLINFO sinfo;
	sinfo.cbSize = sizeof(SCROLLINFO);
	sinfo.fMask = SIF_ALL;

	::GetScrollInfo(hWnd, SB_HORZ, &sinfo);
	if (sinfo.nMax <= sinfo.nPage) return;

	const RECT& rctClient = g_pViewFrame->getFrameRect();
	int cw = g_pViewFrame->getCharWidth();
	int nXOffset = g_pViewFrame->getXOffset(),
		nMaxXOffset = cw * WIDTH_PER_XPITCH - (rctClient.right - rctClient.left);

	switch (LOWORD(wParam)) {
	case SB_LINEDOWN:
		if ((nXOffset += cw) >= nMaxXOffset) {
			nXOffset = nMaxXOffset;
			sinfo.nPos = sinfo.nMax;
		} else {
			sinfo.nPos++;
		}
		break;

	case SB_LINEUP:
		if ((nXOffset -= cw) <= 0) {
			nXOffset = 0;
			sinfo.nPos = 0;
		} else {
			sinfo.nPos--;
		}
		break;

	case SB_PAGEDOWN:
		if ((sinfo.nPos += sinfo.nPage) > sinfo.nMax) {
			nXOffset = nMaxXOffset;
			sinfo.nPos = sinfo.nMax;
		} else {
			nXOffset += sinfo.nPage * cw;
		}
		break;

	case SB_PAGEUP:
		if ((sinfo.nPos -= sinfo.nPage) < 0) {
			nXOffset = 0;
			sinfo.nPos = sinfo.nMin;
		} else {
			nXOffset -= sinfo.nPage * cw;
		}
		break;

	case SB_TOP:
		nXOffset = 0;
		sinfo.nPos = sinfo.nMin;
		break;

	case SB_BOTTOM:
		nXOffset = nMaxXOffset;
		sinfo.nPos = sinfo.nMax;
		break;

	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
		sinfo.nPos = sinfo.nTrackPos;
		nXOffset = cw * sinfo.nPos;
		break;

	default:
		return;
	}

	::SetScrollInfo(hWnd, SB_HORZ, &sinfo, TRUE);

	// prepare the correct BGBuffer
	g_pViewFrame->setXOffset(nXOffset);

	// get the region of BGBuffer to be drawn
	::InvalidateRect(g_hwndMain, NULL, FALSE);
	::UpdateWindow(g_hwndMain);
}

static void
OnHorizontalMove(HWND hWnd, int diff)
{
	g_pViewFrame->setPosition(g_pViewFrame->getPosition() + diff);
	UpdateClientRect();
}

static void
OnMouseWheel(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	if (!g_pViewFrame->isLoaded()) return;

	int delta = (short)HIWORD(wParam);

	if (delta > 0) {
		delta /= WHEEL_DELTA;
		for (int i = 0; i < delta; i++) {
			OnVScroll(hWnd, SB_LINEUP, 0);
		}
	} else {
		delta = - delta / WHEEL_DELTA;
		for (int i = 0; i < delta; i++) {
			OnVScroll(hWnd, SB_LINEDOWN, 0);
		}
	}
}

static void
OnLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	g_pViewFrame->setPositionByCoordinate(MAKEPOINTS(lParam));
	UpdateClientRect();
}

static void
OnCreate(HWND hWnd)
{
	g_hwndMain = hWnd;

	// create default DrawInfo
	HDC hDC = ::GetDC(hWnd);
	g_pDrawInfo = new DrawInfo(hDC, DEFAULT_FONT_SIZE,
							   DEFAULT_FG_COLOR, DEFAULT_BK_COLOR,
							   DEFAULT_FG_COLOR_HEADER, DEFAULT_BK_COLOR_HEADER);
	g_pViewFrame = new ViewFrame(hWnd, g_pDrawInfo.ptr(), NULL);
	::ReleaseDC(hWnd, hDC);

	int height = g_pViewFrame->getLineHeight();
	RECT rctClient;
	::GetClientRect(hWnd, &rctClient);
	rctClient.right  = g_pViewFrame->getCharWidth() * WIDTH_PER_XPITCH;
	rctClient.bottom = height * ((rctClient.bottom + height + 1) / height);

	AdjustWindowSize(hWnd, rctClient);

	// 必要？
	InitScrollInfo(hWnd);
	ModifyHScrollInfo(hWnd, rctClient.right - rctClient.left);

	if (g_strImageFile.length() == 0) {
		EnableEditMenu(hWnd, FALSE);
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

	case WM_PAINT:
		OnPaint(hWnd);
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
				if (pos >= 0) OnJump(hWnd, pos);
			}
			break;

		case IDK_LINEDOWN:
			OnVScroll(hWnd, SB_LINEDOWN, 0);
			break;

		case IDK_LINEUP:
			OnVScroll(hWnd, SB_LINEUP, 0);
			break;

		case IDK_PAGEDOWN:
		case IDK_SPACE:
			OnVScroll(hWnd, SB_PAGEDOWN, 0);
			break;

		case IDK_PAGEUP:
		case IDK_S_SPACE:
			OnVScroll(hWnd, SB_PAGEUP, 0);
			break;

		case IDK_BOTTOM:
		case IDK_C_SPACE:
			OnVScroll(hWnd, SB_BOTTOM, 0);
			break;

		case IDK_TOP:
		case IDK_C_S_SPACE:
			OnVScroll(hWnd, SB_TOP, 0);
			break;

		case IDK_RIGHT:
			OnHorizontalMove(hWnd, 1);
//			OnHScroll(hWnd, SB_LINEDOWN, 0);
			break;

		case IDK_LEFT:
			OnHorizontalMove(hWnd, -1);
//			OnHScroll(hWnd, SB_LINEUP, 0);
			break;
		}
		break;

	case WM_SIZING:
		OnResizing(hWnd, (RECT*)lParam);
		return TRUE;

	case WM_SIZE:
		OnResize(hWnd, wParam);
		break;

	case WM_VSCROLL:
		OnVScroll(hWnd, wParam, lParam);
		break;

	case WM_HSCROLL:
		OnHScroll(hWnd, wParam, lParam);
		break;

	case WM_MOUSEWHEEL:
		OnMouseWheel(hWnd, wParam, lParam);
		break;

	case WM_LBUTTONDOWN:
		OnLButtonDown(hWnd, wParam, lParam);
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

	::ZeroMemory(&wc, sizeof(WNDCLASS));
	wc.hInstance = hInstance;
	wc.style = CS_OWNDC | CS_SAVEBITS /* | CS_HREDRAW | CS_VREDRAW */;
	wc.hIcon = ::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
//	wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
	wc.lpfnWndProc = (WNDPROC)MainWndProc;
	wc.lpszClassName = "BinViewerClass32";
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);

	if (!::RegisterClass(&wc)) return -1;

	HACCEL hAccel = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_KEYACCEL));

	::InitCommonControls();

	g_hwndMain = ::CreateWindowEx(WS_EX_OVERLAPPEDWINDOW | WS_EX_ACCEPTFILES,
								  wc.lpszClassName, "BinViewer",
								  WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
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

