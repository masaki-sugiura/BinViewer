// $Id$

#define _WIN32_WINNT  0x500  // to support mouse wheel

#include "grepdlg.h"
#include "strutils.h"
#include "resource.h"
#include <assert.h>

HWND GrepDlg::m_hwndDlg;
HWND GrepDlg::m_hwndParent;
ViewFrame* GrepDlg::m_pViewFrame;

GrepDlg::GrepDlg(ViewFrame* pViewFrame)
	: m_pViewFrame(pViewFrame),
	  m_hwndParent(NULL), m_hwndDlg(NULL)
{
	assert(pViewFrame);
}

bool
GrepDlg::create(HWND hwndParent)
{
	if (m_hwndDlg) return true;
	m_hwndParent = hwndParent;
	HINSTANCE hInst = (HINSTANCE)::GetWindowLong(hwndParent, GWL_HINSTANCE);
	m_hwndDlg = ::CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_SEARCH_RESULT),
									hwndParent,
									(DLGPROC)GrepDlg::GrepDlgProc,
									(LPARAM)this);
	return m_hwndDlg != NULL;
}

void
GrepDlg::close()
{
	if (!m_hwndDlg) return;

	::SendMessage(m_hwndDlg, WM_CLOSE, 0, 0);
}

bool
GrepDlg::search(int dir)
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

bool
GrepDlg::initDialog(HWND hDlg)
{
}

BOOL
GrepDlg::handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_JUMP:
			break;

		case IDC_CLOSE:
			::DestroyWindow(m_hwndDlg);
			break;

		default:
			return FALSE;
		}
		break;

	case WM_MOUSEWHEEL:
		m_pViewFrame->onMouseWheel(wParam, lParam);
		break;

	case WM_CLOSE:
		::DestroyWindow(hDlg);
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
GrepDlg::GrepDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	GrepDlg* pDlg;

	if (uMsg == WM_INITDIALOG) {
		pDlg = (GrepDlg*)lParam;
		assert(pDlg);
		::SetWindowLong(hDlg, DWL_USER, lParam);
		if (!pDlg->initDialog(hDlg)) ::DestroyWindow(hDlg);
		return FALSE;
	}

	pDlg = (GrepDlg*)::GetWindowLong(hDlg, DWL_USER);
	if (!pDlg) return FALSE;

	return pDlg->handleMessage(uMsg, wParam, lParam);
}

