// $Id$

#pragma warning(disable : 4786)

#include "dialog.h"

#include <assert.h>

Lock Dialog::m_lckActiveDialogs;
int  Dialog::m_nActiveDialogNum;
DialogMap Dialog::m_activeDialogs;
DWORD Dialog::m_dwEnableTheme = 0xFFFF0000;

Dialog::Dialog(int idd)
	: m_nDialogID(idd),
	  m_hwndDlg(NULL), m_hwndParent(NULL),
	  m_bModal(FALSE)
{
	if (m_dwEnableTheme & 0xFFFF0000) {
		m_dwEnableTheme = 0x0000FFFF;
		try {
			ThemeWrapper& tw = GetTW();
			tw.SetThemeAppProperties(STAP_ALLOW_NONCLIENT |
									 STAP_ALLOW_CONTROLS  |
									 STAP_ALLOW_WEBCONTENT);
		} catch (ThemeNotSupportedError&) {
			m_dwEnableTheme = 0;
		}
	}
}

Dialog::~Dialog()
{
}

HWND
Dialog::create(HWND hwndParent)
{
	HINSTANCE hInst = (HINSTANCE)::GetWindowLong(hwndParent, GWL_HINSTANCE);
	assert(hInst);
	m_bModal = FALSE;
	m_hwndParent = hwndParent;
	m_hwndDlg = ::CreateDialogParam(hInst,
									MAKEINTRESOURCE(m_nDialogID),
									hwndParent,
									(DLGPROC)dialogProc,
									(LPARAM)this);
	if (!m_hwndDlg) m_hwndParent = NULL;

	return m_hwndDlg;
}

void
Dialog::close()
{
	if (!m_hwndDlg) return;
	::SendMessage(m_hwndDlg, WM_CLOSE, 0, 0);
}

int
Dialog::doModal(HWND hwndParent)
{
	HINSTANCE hInst = (HINSTANCE)::GetWindowLong(hwndParent, GWL_HINSTANCE);
	assert(hInst);
	m_bModal = TRUE;
	m_hwndParent = hwndParent;
	return ::DialogBoxParam(hInst,
							MAKEINTRESOURCE(m_nDialogID),
							hwndParent,
							(DLGPROC)dialogProc,
							(LPARAM)this);
}

BOOL
Dialog::addToMessageLoop(Dialog* that)
{
	if (!that->m_hwndDlg) return FALSE;
	GetLock lock(m_lckActiveDialogs);
	m_activeDialogs[that->m_hwndDlg] = that;
	m_nActiveDialogNum++;
	return TRUE;
}

BOOL
Dialog::removeFromMessageLoop(Dialog* that)
{
	GetLock lock(m_lckActiveDialogs);
	if (m_activeDialogs.find(that->m_hwndDlg) == m_activeDialogs.end())
		return FALSE;
	m_activeDialogs.erase(that->m_hwndDlg);
	m_nActiveDialogNum--;
	return TRUE;
}

ThemeWrapper&
Dialog::GetTW()
{
	static ThemeWrapper theThemeWrapper;
	return theThemeWrapper;
}

BOOL
Dialog::isDialogMessage(MSG* msg)
{
	GetLock lock(m_lckActiveDialogs);

	if (!m_nActiveDialogNum) return FALSE;

	for (DialogMap::iterator itr = m_activeDialogs.begin();
		 itr != m_activeDialogs.end();
		 ++itr) {
		 if (::IsDialogMessage(itr->first, msg)) return TRUE;
	}

	return FALSE;
}

BOOL CALLBACK
Dialog::dialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_INITDIALOG) {
		Dialog* This = (Dialog*)lParam;

		This->m_hwndDlg = hDlg;

		if (!This->initDialog(hDlg)) {
			if (This->m_bModal) ::EndDialog(hDlg, -1);
			else This->close();
			return TRUE;
		}

		::SetWindowLong(hDlg, DWL_USER, lParam);

		return FALSE;
	}

	Dialog* This = (Dialog*)::GetWindowLong(hDlg, DWL_USER);
	if (!This) return FALSE;

	if (uMsg == WM_DESTROY) {
		This->destroyDialog();
		This->m_hwndDlg = This->m_hwndParent = NULL;
		return TRUE;
	}

	return This->dialogProcMain(uMsg, wParam, lParam);
}

