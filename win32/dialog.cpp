// $Id$

#pragma warning(disable : 4786)

#include "dialog.h"

#include <assert.h>

Lock Dialog::m_lckActiveDialogs;
int  Dialog::m_nActiveDialogNum;
DialogMap Dialog::m_activeDialogs;
HMODULE   Dialog::m_hmUxTheme;
PFN_ITA   Dialog::m_pfnIsThemeActive;
PFN_ETDT  Dialog::m_pfnEnableThemeDialogTexture;
PFN_OTD   Dialog::m_pfnOpenThemeData;
PFN_CTD   Dialog::m_pfnCloseThemeData;
PFN_DTB   Dialog::m_pfnDrawThemeBackground;
PFN_DTPB  Dialog::m_pfnDrawThemeParentBackground;
PFN_GTBCR Dialog::m_pfnGetThemeBackgroundContentRect;

Dialog::Dialog(int idd)
	: m_nDialogID(idd),
	  m_hwndDlg(NULL), m_hwndParent(NULL),
	  m_bModal(FALSE)
{
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

BOOL
Dialog::initializeTheme()
{
	if (m_hmUxTheme) return m_pfnEnableThemeDialogTexture != NULL;

	m_hmUxTheme = ::LoadLibrary("UxTheme.dll");
	if (!m_hmUxTheme) return FALSE;

	m_pfnIsThemeActive
		= (PFN_ITA)::GetProcAddress(m_hmUxTheme, "IsThemeActive");
	m_pfnEnableThemeDialogTexture
		= (PFN_ETDT)::GetProcAddress(m_hmUxTheme, "EnableThemeDialogTexture");
	m_pfnOpenThemeData
		= (PFN_OTD)::GetProcAddress(m_hmUxTheme, "OpenThemeData");
	m_pfnCloseThemeData
		= (PFN_CTD)::GetProcAddress(m_hmUxTheme, "CloseThemeData");
	m_pfnDrawThemeBackground
		= (PFN_DTB)::GetProcAddress(m_hmUxTheme, "DrawThemeBackground");
	m_pfnDrawThemeParentBackground
		= (PFN_DTPB)::GetProcAddress(m_hmUxTheme, "DrawThemeParentBackground");
	m_pfnGetThemeBackgroundContentRect
		= (PFN_GTBCR)::GetProcAddress(m_hmUxTheme, "GetThemeBackgroundContentRect");

	return m_pfnIsThemeActive != NULL &&
		   m_pfnEnableThemeDialogTexture != NULL &&
		   m_pfnOpenThemeData != NULL &&
		   m_pfnCloseThemeData != NULL &&
		   m_pfnDrawThemeBackground != NULL &&
		   m_pfnDrawThemeParentBackground != NULL &&
		   m_pfnGetThemeBackgroundContentRect != NULL;
}

void
Dialog::uninitializeTheme()
{
	if (m_hmUxTheme) ::FreeLibrary(m_hmUxTheme);
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

