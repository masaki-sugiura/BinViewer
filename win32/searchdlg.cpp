// $Id$

#define _WIN32_WINNT  0x500  // to support mouse wheel

#include "searchdlg.h"
#include "strutils.h"
#include "resource.h"
#include <assert.h>

bool SearchDlg::m_bSearching;
bool SearchDlg::m_bShowGrepDialog;
HWND SearchDlg::m_hwndDlg;
HWND SearchDlg::m_hwndSearch;
HWND SearchDlg::m_hwndGrep;
HWND SearchDlg::m_hwndParent;
ViewFrame* SearchDlg::m_pViewFrame;

bool
SearchDlg::create(HWND hwndParent, ViewFrame* pViewFrame)
{
	if (m_hwndDlg) return true;
	m_hwndParent = hwndParent;
	m_pViewFrame = pViewFrame;
	m_bShowGrepDialog = false;
	HINSTANCE hInst = (HINSTANCE)::GetWindowLong(hwndParent, GWL_HINSTANCE);
	m_hwndDlg = ::CreateDialog(hInst, MAKEINTRESOURCE(IDD_SEARCH_MAIN),
							   hwndParent,
							   (DLGPROC)SearchDlg::SearchMainDlgProc);
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
	HWND hEdit = ::GetDlgItem(m_hwndSearch, IDC_SEARCHDATA);
	int len = ::GetWindowTextLength(hEdit);
	BYTE* buf = new BYTE[len + 1];
	::GetWindowText(hEdit, (char*)buf, len + 1);

	// get datatype
	if (::SendMessage(::GetDlgItem(m_hwndSearch, IDC_DT_HEX), BM_GETCHECK, 0, 0)) {
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

	if (m_hwndSearch) {
		if (pArg->m_qFindAddress >= 0) {
			m_pViewFrame->onJump(pArg->m_qFindAddress + pArg->m_nBufSize);
			m_pViewFrame->onJump(pArg->m_qFindAddress);
			m_pViewFrame->select(pArg->m_qFindAddress, pArg->m_nBufSize);
		} else {
			::MessageBeep(MB_ICONEXCLAMATION);
			m_pViewFrame->select(pArg->m_qOrgAddress, pArg->m_nOrgSelectedSize);
		}
		::PostMessage(m_hwndSearch, WM_USER_FIND_FINISH, 0, 0);
	}

	delete [] pArg->m_pData;
	delete pArg;
}

void
SearchDlg::enableControls(int dir, bool enable)
{
	if (enable) {
		::SetWindowText(::GetDlgItem(m_hwndSearch, IDC_SEARCH_FORWARD),
						"後方検索");
		::SetWindowText(::GetDlgItem(m_hwndSearch, IDC_SEARCH_BACKWARD),
						"前方検索");
		::EnableWindow(::GetDlgItem(m_hwndSearch, IDC_SEARCH_FORWARD), TRUE);
		::EnableWindow(::GetDlgItem(m_hwndSearch, IDC_SEARCH_BACKWARD), TRUE);
	} else {
		if (dir == FIND_FORWARD) {
			::SetWindowText(::GetDlgItem(m_hwndSearch, IDC_SEARCH_FORWARD), "中断");
			::EnableWindow(::GetDlgItem(m_hwndSearch, IDC_SEARCH_BACKWARD), FALSE);
		} else {
			::SetWindowText(::GetDlgItem(m_hwndSearch, IDC_SEARCH_BACKWARD), "中断");
			::EnableWindow(::GetDlgItem(m_hwndSearch, IDC_SEARCH_FORWARD), FALSE);
		}
	}
	::EnableWindow(::GetDlgItem(m_hwndSearch, IDC_SEARCHDATA), enable);
	::EnableWindow(::GetDlgItem(m_hwndSearch, IDC_DT_HEX), enable);
	::EnableWindow(::GetDlgItem(m_hwndSearch, IDC_DT_STRING), enable);
	::EnableWindow(::GetDlgItem(m_hwndSearch, IDOK), enable);
}

void
SearchDlg::adjustClientRect(HWND hDlg, const RECT& rctClient)
{
	RECT rctDlg, rctOrgClient;
	::GetWindowRect(hDlg, &rctDlg);
	::GetClientRect(hDlg, &rctOrgClient);
	rctDlg.right  += rctClient.right - rctOrgClient.right;
	rctDlg.bottom += rctClient.bottom - rctOrgClient.bottom;
	::SetWindowPos(hDlg, NULL, 0, 0,
				   rctDlg.right - rctDlg.left,
				   rctDlg.bottom - rctDlg.top,
				   SWP_NOZORDER | SWP_NOMOVE);
}

BOOL CALLBACK
SearchDlg::SearchMainDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			HINSTANCE hInst = (HINSTANCE)::GetWindowLong(hDlg, GWL_HINSTANCE);
			m_hwndSearch = ::CreateDialog(hInst, MAKEINTRESOURCE(IDD_SEARCH),
										  hDlg,
										  SearchDlg::SearchDlgProc);
			if (!m_hwndSearch) {
				::DestroyWindow(hDlg);
				return TRUE;
			}
			RECT rctSearch;
			::GetWindowRect(m_hwndSearch, &rctSearch);
			rctSearch.right -= rctSearch.left;
			rctSearch.left = 0;
			rctSearch.bottom -= rctSearch.top;
			rctSearch.top = 0;
			adjustClientRect(hDlg, rctSearch);
			::ShowWindow(m_hwndSearch, SW_SHOW);
		}
		break;

	case WM_SYSCOMMAND:
		if (wParam == SC_CLOSE) ::DestroyWindow(hDlg);
		break;

	case WM_DESTROY:
		m_hwndDlg = NULL;
		break;

	default:
		return FALSE;
	}
	return TRUE;
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
			if (HIWORD(wParam) != BN_CLICKED) break;
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
			if (HIWORD(wParam) != BN_CLICKED) break;
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

		case IDC_GREP:
			if (HIWORD(wParam) != BN_CLICKED) break;
			if (!m_bShowGrepDialog) {
				if (!m_hwndGrep) {
					m_hwndGrep = ::CreateDialog((HINSTANCE)::GetWindowLong(hDlg, GWL_HINSTANCE),
												MAKEINTRESOURCE(IDD_SEARCH_GREP),
												m_hwndDlg,
												SearchDlg::SearchGrepDlgProc);
					if (!m_hwndGrep) break;
				}
				RECT rctSearch, rctGrep;
				::GetWindowRect(hDlg, &rctSearch);
				::GetWindowRect(m_hwndGrep, &rctGrep);
				int y = rctSearch.bottom - rctSearch.top;
				rctSearch.right -= rctSearch.left;
				rctSearch.left = 0;
				rctSearch.bottom += rctGrep.bottom - rctGrep.top - rctSearch.top;
				adjustClientRect(m_hwndDlg, rctSearch);
				::SetWindowPos(m_hwndGrep, HWND_TOP,
							   0, y, 0, 0,
							   SWP_NOSIZE);
				::ShowWindow(m_hwndGrep, SW_SHOW);
			} else {
				assert(m_hwndGrep);
				::ShowWindow(m_hwndGrep, SW_HIDE);
				RECT rctSearch;
				::GetWindowRect(hDlg, &rctSearch);
				rctSearch.right -= rctSearch.left;
				rctSearch.left = 0;
				rctSearch.bottom -= rctSearch.top;
				adjustClientRect(m_hwndDlg, rctSearch);
			}
			m_bShowGrepDialog = !m_bShowGrepDialog;
			break;

		case IDOK:
			if (HIWORD(wParam) != BN_CLICKED) break;
			::DestroyWindow(m_hwndDlg);
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
			m_hwndSearch = NULL;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK
SearchDlg::SearchGrepDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

