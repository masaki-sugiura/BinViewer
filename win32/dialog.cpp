// $Id$

#pragma warning(disable : 4786)

#include "dialog.h"

#include <assert.h>

Lock Dialog::m_lckActiveDialogs;
int  Dialog::m_nActiveDialogNum;
DialogMap Dialog::m_activeDialogs;

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
Dialog::isDialogMessage(MSG* msg)
{
	GetLock lock(m_lckActiveDialogs);

	if (!m_nActiveDialogNum) return FALSE;

	for (map<HWND, Dialog*>::iterator itr = m_activeDialogs.begin();
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
			if (This->m_bModal) ::EndDialog(-1);
			else This->close();
			return TRUE;
		}

		::SetWindowLong(hDlg, DWL_USER, lParam);

		GetLock lock(m_lckActiveDialogs);
		m_activeDialogs[hDlg] = (Dialog*)lParam;
		m_nActiveDialogNum++;
		return FALSE;
	}

	Dialog* This = (Dialog*)::GetWindowLong(hDlg, DWL_USER);
	if (!This) return FALSE;

	if (uMsg == WM_DESTROY) {
		This->destroyDialog();
		if (!This->m_bModal) {
			GetLock lock(m_lckActiveDialogs);
			m_activeDialogs.erase(hDlg);
			m_nActiveDialogNum--;
		}
		This->m_hwndDlg = This->m_hwndParent = NULL;
		return TRUE;
	}

	return This->dialogProcMain(uMsg, wParam, lParam);
}

