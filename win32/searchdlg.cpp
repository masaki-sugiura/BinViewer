// $Id$

#define _WIN32_WINNT  0x500  // to support mouse wheel

#include "searchdlg.h"
#include "strutils.h"
#include "resource.h"
#include <assert.h>

bool SearchDlg::m_bSearching;
HWND SearchDlg::m_hwndDlg;
HWND SearchDlg::m_hwndParent;
ViewFrame* SearchDlg::m_pViewFrame;

bool
SearchDlg::create(HWND hwndParent, ViewFrame* pViewFrame)
{
	if (m_hwndDlg) return true;
	m_hwndParent = hwndParent;
	m_pViewFrame = pViewFrame;
	HINSTANCE hInst = (HINSTANCE)::GetWindowLong(hwndParent, GWL_HINSTANCE);
	m_hwndDlg = ::CreateDialog(hInst, MAKEINTRESOURCE(IDD_SEARCH),
							   hwndParent,
							   (DLGPROC)SearchDlg::SearchDlgProc);
	return m_hwndDlg != NULL;
}

void
SearchDlg::close()
{
	if (!m_hwndDlg) return;

	::SendMessage(m_hwndDlg, WM_CLOSE, 0, 0);
}

bool
SearchDlg::search(int dir)
{
	typedef enum {
		DATATYPE_HEX, DATATYPE_STRING
	} SEARCH_DATATYPE;

	if (!m_pViewFrame->isLoaded()) return false;

	// get raw data
	HWND hEdit = ::GetDlgItem(m_hwndDlg, IDC_SEARCHDATA);
	int len = ::GetWindowTextLength(hEdit);
	BYTE* buf = new BYTE[len + 1];
	::GetWindowText(hEdit, (char*)buf, len + 1);

	// get datatype
	if (::SendMessage(::GetDlgItem(m_hwndDlg, IDC_DT_HEX), BM_GETCHECK, 0, 0)) {
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
	pFindCallbackArg->m_qOrgAddress = m_pViewFrame->getPosition();
	pFindCallbackArg->m_nOrgSelectedSize = m_pViewFrame->getSelectedSize();

	pFindCallbackArg->m_qStartAddress = pFindCallbackArg->m_qOrgAddress;
	if (dir == FIND_FORWARD) {
		pFindCallbackArg->m_qStartAddress += pFindCallbackArg->m_nOrgSelectedSize;
	}

	m_pViewFrame->unselect();

	if (m_pViewFrame->findCallback(pFindCallbackArg)) return true;

	delete [] buf;
	delete pFindCallbackArg;

	assert(0);

	return false;
}

void
SearchDlg::FindCallbackProc(void* arg)
{
	assert(arg);

	FindCallbackArg* pArg = (FindCallbackArg*)arg;

	if (::IsWindow(m_hwndDlg)) {
		if (pArg->m_qFindAddress >= 0) {
			m_pViewFrame->onJump(pArg->m_qFindAddress + pArg->m_nBufSize);
			m_pViewFrame->onJump(pArg->m_qFindAddress);
			m_pViewFrame->select(pArg->m_qFindAddress, pArg->m_nBufSize);
		} else {
			::MessageBeep(MB_ICONEXCLAMATION);
			m_pViewFrame->select(pArg->m_qOrgAddress, pArg->m_nOrgSelectedSize);
		}
		::PostMessage(m_hwndDlg, WM_USER_FIND_FINISH, 0, 0);
	}

	delete [] pArg->m_pData;
	delete pArg;
}

void
SearchDlg::enableControls(int dir, bool enable)
{
	if (enable) {
		::SetWindowText(::GetDlgItem(m_hwndDlg, IDC_SEARCH_FORWARD),
						"後方検索");
		::SetWindowText(::GetDlgItem(m_hwndDlg, IDC_SEARCH_BACKWARD),
						"前方検索");
		::EnableWindow(::GetDlgItem(m_hwndDlg, IDC_SEARCH_FORWARD), TRUE);
		::EnableWindow(::GetDlgItem(m_hwndDlg, IDC_SEARCH_BACKWARD), TRUE);
	} else {
		if (dir == FIND_FORWARD) {
			::SetWindowText(::GetDlgItem(m_hwndDlg, IDC_SEARCH_FORWARD), "中断");
			::EnableWindow(::GetDlgItem(m_hwndDlg, IDC_SEARCH_BACKWARD), FALSE);
		} else {
			::SetWindowText(::GetDlgItem(m_hwndDlg, IDC_SEARCH_BACKWARD), "中断");
			::EnableWindow(::GetDlgItem(m_hwndDlg, IDC_SEARCH_FORWARD), FALSE);
		}
	}
	::EnableWindow(::GetDlgItem(m_hwndDlg, IDC_SEARCHDATA), enable);
	::EnableWindow(::GetDlgItem(m_hwndDlg, IDC_DT_HEX), enable);
	::EnableWindow(::GetDlgItem(m_hwndDlg, IDC_DT_STRING), enable);
	::EnableWindow(::GetDlgItem(m_hwndDlg, IDOK), enable);
}

BOOL CALLBACK
SearchDlg::SearchDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		::SendMessage(::GetDlgItem(hDlg, IDC_DT_HEX),
					  BM_SETCHECK, BST_CHECKED, 0);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SEARCH_FORWARD:
			if (!m_bSearching) {
				if (m_bSearching = search(FIND_FORWARD)) {
					enableControls(FIND_FORWARD, FALSE);
					::EnableWindow(m_hwndParent, FALSE);
				}
			} else {
				m_pViewFrame->stopFind();
				// コールバック関数により WM_USER_FIND_FINISH が投げられる
			}
			break;
		case IDC_SEARCH_BACKWARD:
			if (!m_bSearching) {
				if (m_bSearching = search(FIND_BACKWARD)) {
					enableControls(FIND_BACKWARD, FALSE);
					::EnableWindow(m_hwndParent, FALSE);
				}
			} else {
				m_pViewFrame->stopFind();
				// コールバック関数により WM_USER_FIND_FINISH が投げられる
			}
			break;
		case IDOK:
			::DestroyWindow(hDlg);
			break;
		}
		break;

	case WM_USER_FIND_FINISH:
		if (m_bSearching) {
			m_pViewFrame->cleanupCallback();
			enableControls(0, TRUE);
			::EnableWindow(m_hwndParent, TRUE);
			m_bSearching = false;
		}
		break;

	case WM_MOUSEWHEEL:
		if (!m_bSearching) {
			m_pViewFrame->onMouseWheel(wParam, lParam);
		}
		break;

	case WM_CLOSE:
		::DestroyWindow(hDlg);
		break;

	case WM_DESTROY:
		{
			if (m_bSearching) {
				m_bSearching = false;
				m_pViewFrame->stopFind();
				m_pViewFrame->cleanupCallback();
				::EnableWindow(m_hwndParent, TRUE);
			}
			m_hwndDlg = NULL;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

