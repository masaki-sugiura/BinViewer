// $Id$

#define _WIN32_WINNT  0x500  // to support mouse wheel

#include "searchdlg.h"
#include "strutils.h"
#include "resource.h"
#include <assert.h>
#include <commctrl.h>

bool SearchDlg::m_bSearching;
bool SearchDlg::m_bShowGrepDialog;
HWND SearchDlg::m_hwndDlg;
HWND SearchDlg::m_hwndSearch;
HWND SearchDlg::m_hwndGrep;
HWND SearchDlg::m_hwndParent;
ViewFrame* SearchDlg::m_pViewFrame;

struct GrepResult {
	filesize_t m_qAddress;
	int m_nSize;
};

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
SearchDlg::prepareFindCallbackArg(FindCallbackArg*& pFindCallbackArg)
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
	pFindCallbackArg = new FindCallbackArg;
	pFindCallbackArg->m_pData = buf;
	pFindCallbackArg->m_nBufSize = len;
	pFindCallbackArg->m_qFindAddress = -1;
	pFindCallbackArg->m_qOrgAddress = m_pViewFrame->getPosition();
	pFindCallbackArg->m_nOrgSelectedSize = m_pViewFrame->getSelectedSize();

	return true;
}

bool
SearchDlg::search(int dir)
{
	FindCallbackArg* pFindCallbackArg = NULL;
	if (!prepareFindCallbackArg(pFindCallbackArg)) return false;

	pFindCallbackArg->m_nDirection  = dir;
	pFindCallbackArg->m_pfnCallback = FindCallbackProc;

	pFindCallbackArg->m_qStartAddress = pFindCallbackArg->m_qOrgAddress;
	if (dir == FIND_FORWARD) {
		pFindCallbackArg->m_qStartAddress += pFindCallbackArg->m_nOrgSelectedSize;
	}

	m_pViewFrame->unselect();

	if (m_pViewFrame->findCallback(pFindCallbackArg)) return true;

	delete [] pFindCallbackArg->m_pData;
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

bool
SearchDlg::grep()
{
	FindCallbackArg* pFindCallbackArg = NULL;
	if (!prepareFindCallbackArg(pFindCallbackArg)) return false;

	pFindCallbackArg->m_nDirection  = FIND_FORWARD;
	pFindCallbackArg->m_pfnCallback = GrepCallbackProc;

	pFindCallbackArg->m_qStartAddress = 0;

	if (m_pViewFrame->findCallback(pFindCallbackArg)) return true;

	delete [] pFindCallbackArg->m_pData;
	delete pFindCallbackArg;

	assert(0);

	return false;
}

void
SearchDlg::GrepCallbackProc(void* arg)
{
	assert(arg);

	FindCallbackArg* pArg = (FindCallbackArg*)arg;

	if (m_hwndGrep) {
		if (pArg->m_qFindAddress >= 0) {
			GrepResult gr;
			gr.m_qAddress = pArg->m_qFindAddress;
			gr.m_nSize    = pArg->m_nBufSize;
			::SendMessage(m_hwndGrep, WM_USER_ADD_ITEM, 0, (LPARAM)&gr);
			pArg->m_qStartAddress = pArg->m_qFindAddress + pArg->m_nBufSize;
			pArg->m_qFindAddress = -1;
			::PostMessage(m_hwndGrep, WM_USER_GREP_NEXT, 0, (LPARAM)pArg);
			return;
		}
	}
	delete [] pArg->m_pData;
	delete pArg;
	::PostMessage(m_hwndGrep, WM_USER_GREP_FINISH, 0, 0);
}

void
SearchDlg::enableControls(int dir, bool enable)
{
	if (enable) {
		::SetWindowText(::GetDlgItem(m_hwndSearch, IDC_SEARCH_FORWARD),
						"�������");
		::SetWindowText(::GetDlgItem(m_hwndSearch, IDC_SEARCH_BACKWARD),
						"�O������");
		::EnableWindow(::GetDlgItem(m_hwndSearch, IDC_SEARCH_FORWARD), TRUE);
		::EnableWindow(::GetDlgItem(m_hwndSearch, IDC_SEARCH_BACKWARD), TRUE);
	} else {
		if (dir == FIND_FORWARD) {
			::SetWindowText(::GetDlgItem(m_hwndSearch, IDC_SEARCH_FORWARD), "���f");
			::EnableWindow(::GetDlgItem(m_hwndSearch, IDC_SEARCH_BACKWARD), FALSE);
		} else {
			::SetWindowText(::GetDlgItem(m_hwndSearch, IDC_SEARCH_BACKWARD), "���f");
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
		if (wParam == SC_CLOSE) {
			::DestroyWindow(hDlg);
			break;
		}
		return FALSE;

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
				// �R�[���o�b�N�֐��ɂ�� WM_USER_FIND_FINISH ����������
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
				// �R�[���o�b�N�֐��ɂ�� WM_USER_FIND_FINISH ����������
			}
			break;

		case IDC_GREP:
			if (HIWORD(wParam) != BN_CLICKED) break;
			if (!::GetWindowTextLength(::GetDlgItem(hDlg, IDC_SEARCHDATA))) {
				::MessageBeep(MB_ICONEXCLAMATION);
				break;
			}
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
#if 0
			} else {
				assert(m_hwndGrep);
				::ShowWindow(m_hwndGrep, SW_HIDE);
				RECT rctSearch;
				::GetWindowRect(hDlg, &rctSearch);
				rctSearch.right -= rctSearch.left;
				rctSearch.left = 0;
				rctSearch.bottom -= rctSearch.top;
				adjustClientRect(m_hwndDlg, rctSearch);
#endif
				m_bShowGrepDialog = true;
			}
//			m_bShowGrepDialog = !m_bShowGrepDialog;
			::SendMessage(m_hwndGrep, WM_USER_GREP_START, 0, 0);
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
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			HWND hwndLView = ::GetDlgItem(hDlg, IDC_SEARCH_GREP_RESULT);
			RECT rctLView;
			::GetClientRect(hwndLView, &rctLView);
			LVCOLUMN lvc;
			lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
			lvc.fmt  = LVCFMT_RIGHT;
			lvc.cx   = rctLView.right * 2 / 3;
			lvc.pszText = "�A�h���X";
			lvc.iSubItem = 0;
			ListView_InsertColumn(hwndLView, 0, &lvc);
			lvc.cx       = rctLView.right / 3;
			lvc.pszText  = "�T�C�Y";
			lvc.iSubItem = 1;
			ListView_InsertColumn(hwndLView, 1, &lvc);
		}
		break;

	case WM_USER_GREP_START:
		::SendMessage(hDlg, WM_USER_CLEAR_ITEM, 0, 0);
		::EnableWindow(::GetDlgItem(hDlg, IDC_JUMP), FALSE);
		grep();
		break;

	case WM_USER_GREP_NEXT:
		{
			FindCallbackArg* pArg = (FindCallbackArg*)lParam;
			m_pViewFrame->cleanupCallback();
			if (!m_pViewFrame->findCallback(pArg)) {
				delete [] pArg->m_pData;
				delete pArg;
				::SendMessage(hDlg, WM_USER_GREP_FINISH, 0, 0);
			}
		}
		break;

	case WM_USER_GREP_FINISH:
		m_pViewFrame->cleanupCallback();
		::EnableWindow(::GetDlgItem(hDlg, IDC_JUMP), TRUE);
		break;

	case WM_USER_ADD_ITEM:
		{
			HWND hwndLView = ::GetDlgItem(hDlg, IDC_SEARCH_GREP_RESULT);
			LVITEM lvitem;
			lvitem.mask = LVIF_PARAM | LVIF_TEXT;
			lvitem.iItem = ListView_GetItemCount(hwndLView);
			lvitem.iSubItem = 0;
			lvitem.lParam = (LPARAM)(new GrepResult);
			GrepResult* gr = (GrepResult*)lParam;
			*((GrepResult*)lvitem.lParam) = *gr;
			char buf[24];
			wsprintf(buf, "0x%08x%08x",
					 (int)(gr->m_qAddress >> 32),
					 (int)gr->m_qAddress);
			lvitem.pszText = buf;
			lvitem.iItem = ListView_InsertItem(hwndLView, &lvitem);
			lvitem.mask = LVIF_TEXT;
			wsprintf(buf, "0x%08x", gr->m_nSize);
			lvitem.iSubItem = 1;
			ListView_SetItem(hwndLView, &lvitem);
		}
		break;

	case WM_USER_CLEAR_ITEM:
		{
			HWND hwndLView = ::GetDlgItem(hDlg, IDC_SEARCH_GREP_RESULT);
			int num = ListView_GetItemCount(hwndLView);
			LVITEM lvitem;
			lvitem.mask = LVIF_PARAM;
			lvitem.iSubItem = 0;
			for (int i = 0; i < num; i++) {
				lvitem.iItem = i;
				ListView_GetItem(hwndLView, &lvitem);
				delete (GrepResult*)lvitem.lParam;
			}
			ListView_DeleteAllItems(hwndLView);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_JUMP:
			if (HIWORD(wParam) == BN_CLICKED) {
				HWND hwndLView = ::GetDlgItem(hDlg, IDC_SEARCH_GREP_RESULT);
				LVITEM lvitem;
				lvitem.mask = LVIF_STATE | LVIF_PARAM;
				lvitem.stateMask = LVIS_SELECTED;
				lvitem.state = 0;
				lvitem.iSubItem = 0;
				int num = ListView_GetItemCount(hwndLView);
				for (int i = 0; i < num; i++) {
					lvitem.iItem = i;
					ListView_GetItem(hwndLView, &lvitem);
					if (lvitem.state & LVIS_SELECTED) break;
				}
				if (lvitem.state & LVIS_SELECTED) {
					GrepResult* gr = (GrepResult*)lvitem.lParam;
					m_pViewFrame->onJump(gr->m_qAddress + gr->m_nSize);
					m_pViewFrame->onJump(gr->m_qAddress);
					m_pViewFrame->select(gr->m_qAddress, gr->m_nSize);
				}
			}
			break;
		}
		break;

	case WM_NOTIFY:
		{
			switch (((NMHDR*)lParam)->code) {
			case NM_DBLCLK:
				{
					NMITEMACTIVATE* pia = (NMITEMACTIVATE*)lParam;
					LVITEM lvitem;
					lvitem.mask = LVIF_PARAM;
					lvitem.iItem = pia->iItem;
					lvitem.iSubItem = 0;
					ListView_GetItem(::GetDlgItem(hDlg, IDC_SEARCH_GREP_RESULT), &lvitem);
					GrepResult* gr = (GrepResult*)lvitem.lParam;
					m_pViewFrame->onJump(gr->m_qAddress + gr->m_nSize);
					m_pViewFrame->onJump(gr->m_qAddress);
					m_pViewFrame->select(gr->m_qAddress, gr->m_nSize);
				}
				break;
			}
		}
		break;

	case WM_DESTROY:
		::SendMessage(hDlg, WM_USER_CLEAR_ITEM, 0, 0);
		m_hwndGrep = NULL;
		break;
	}
	return FALSE;
}

