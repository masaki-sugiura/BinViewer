// $Id$

#include "StatusBar.h"
#include "strutils.h"
#include <commctrl.h>

#define STATUS_POS_HEADER    "カーソルの現在位置： 0x"

StatusBar::StatusBar(LF_Notifier& lfNotifier,
					 HWND hwndParent, const RECT& rctBar)
	: LF_Acceptor(),
	  m_hwndParent(hwndParent),
	  m_hwndStatusBar(NULL)
{
	m_hwndStatusBar = ::CreateWindow(STATUSCLASSNAME,
									 "",
									 WS_CHILD | WS_VISIBLE |
									  SBARS_SIZEGRIP,
									 rctBar.left,
									 rctBar.top,
									 rctBar.right - rctBar.left,
									 rctBar.bottom - rctBar.top,
									 hwndParent,
									 (HMENU)IDC_STATUSBAR,
									 (HINSTANCE)::GetWindowLong(hwndParent, GWL_HINSTANCE),
									 NULL);
	if (!m_hwndStatusBar) {
		throw CreateStatusBarError();
	}

	HDC hDC = ::GetDC(m_hwndStatusBar);
	char status[80];
	lstrcpy(status, STATUS_POS_HEADER);
	lstrcat(status, "0000000000000000");
	SIZE tsize;
	::GetTextExtentPoint32(hDC, status, lstrlen(status), &tsize);
	::SendMessage(m_hwndStatusBar, SB_SETPARTS, 1, (LPARAM)&tsize.cx);
	::ReleaseDC(m_hwndStatusBar, hDC);

	if (!registTo(lfNotifier)) {
		::DestroyWindow(m_hwndStatusBar);
		throw CreateStatusBarError();
	}
}

StatusBar::~StatusBar()
{
	unregist();
}

bool
StatusBar::onLoadFile()
{
	setStatus(0);
	return true;
}

void
StatusBar::onUnloadFile()
{
	setStatus(0);
}

void
StatusBar::onSetCursorPos(filesize_t pos)
{
	setStatus(pos);
}

void
StatusBar::setWindowPos(const RECT& rctBar)
{
	::SetWindowPos(m_hwndStatusBar, 0,
				   rctBar.left, rctBar.top,
				   rctBar.right - rctBar.left,
				   rctBar.bottom - rctBar.top,
				   SWP_NOZORDER);
}

void
StatusBar::getWindowRect(RECT& rctWindow)
{
	::GetWindowRect(m_hwndStatusBar, &rctWindow);
}

void
StatusBar::setStatus(filesize_t newpos)
{
	static const int hlen = lstrlen(STATUS_POS_HEADER);
	static char msgbuf[40] = STATUS_POS_HEADER;

	QwordToStr((UINT)newpos, (UINT)(newpos >> 32), msgbuf + hlen);
	::SendMessage(m_hwndStatusBar, SB_SETTEXT, 0, (LPARAM)msgbuf);
}

