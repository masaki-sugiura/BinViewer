// $Id$

#include "configdlg.h"
#include "resource.h"
#include <commctrl.h>
#include <assert.h>

HINSTANCE ConfigDlg::m_hInstance;
HWND ConfigDlg::m_hwndMain;
HWND ConfigDlg::m_hwndFontPage;
DrawInfo* ConfigDlg::m_pDrawInfo;

bool
ConfigDlg::doModal(HWND hwndParent, DrawInfo* pDrawInfo)
{
	assert(pDrawInfo);

	m_hInstance = (HINSTANCE)::GetWindowLong(hwndParent, GWL_HINSTANCE);
	m_pDrawInfo = pDrawInfo;

	bool ret = !::DialogBox(m_hInstance,
							MAKEINTRESOURCE(IDD_CONFIG_MAIN),
							hwndParent,
							(DLGPROC)ConfigDlg::ConfigDlgProc);
//	::SetForegroundWindow(hwndParent);
	return ret;
}

bool
ConfigDlg::initMainDlg(HWND hDlg)
{
	assert(m_hInstance);

	HWND hwndTab = ::GetDlgItem(hDlg, IDC_CONFIG_TAB);
	if (!hwndTab) {
		::SendMessage(m_hwndFontPage, WM_CLOSE, 0, 0);
		m_hwndFontPage = NULL;
		return false;
	}

	m_hwndFontPage = ::CreateDialog(m_hInstance,
									MAKEINTRESOURCE(IDD_CONFIG_FONT),
									hDlg,
									(DLGPROC)ConfigDlg::FontPageProc);
	if (!m_hwndFontPage) return false;

	TCITEM tcItem;
	tcItem.mask = TCIF_TEXT | TCIF_PARAM;
	tcItem.pszText = "フォント";
	tcItem.lParam  = (LPARAM)m_hwndFontPage;
	if (TabCtrl_InsertItem(hwndTab, 0, &tcItem) != 0) {
		::SendMessage(m_hwndFontPage, WM_CLOSE, 0, 0);
		m_hwndFontPage = NULL;
		return false;
	}

	// 何か項目を追加した後でないとこの計算が失敗する
	RECT rctFrame;
	::GetWindowRect(hwndTab, &rctFrame);
	TabCtrl_AdjustRect(hwndTab, FALSE, &rctFrame);
	POINT ptOrg;
	ptOrg.x = rctFrame.left;
	ptOrg.y = rctFrame.top;
	::ScreenToClient(hDlg, &ptOrg);
	rctFrame.right  = ptOrg.x + (rctFrame.right - rctFrame.left);
	rctFrame.bottom = ptOrg.y + (rctFrame.bottom - rctFrame.top);
	rctFrame.left   = ptOrg.x;
	rctFrame.top    = ptOrg.y;
	
	::SetWindowPos(m_hwndFontPage, HWND_TOP,
				   rctFrame.left, rctFrame.top,
				   0, 0,
				   SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);

	m_hwndMain = hDlg;

	return true;
}

void
ConfigDlg::applyChanges()
{
}

BOOL CALLBACK
ConfigDlg::ConfigDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		if (!initMainDlg(hDlg)) ::SendMessage(hDlg, WM_CLOSE, 0, 0);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_APPLY:
			applyChanges();
			break;
		case IDOK:
			applyChanges();
			// through down
		case IDCANCEL:
			::SendMessage(hDlg, WM_CLOSE, 0, 0);
			break;
		}
		break;

	case WM_NOTIFY:
		if (((NMHDR*)lParam)->idFrom == IDC_CONFIG_TAB) {
			HWND hwndTab = ::GetDlgItem(hDlg, IDC_CONFIG_TAB);
			NMHDR* phdr = (NMHDR*)lParam;
			int iItem;
			TCITEM tcItem;
			tcItem.mask = TCIF_PARAM;
			switch (phdr->code) {
			case TCN_SELCHANGING:
				iItem = TabCtrl_GetCurSel(hwndTab);
				TabCtrl_GetItem(hwndTab, iItem, &tcItem);
				::ShowWindow((HWND)tcItem.lParam, SW_HIDE);
				break;
			case TCN_SELCHANGE:
				iItem = TabCtrl_GetCurSel(hwndTab);
				TabCtrl_GetItem(hwndTab, iItem, &tcItem);
				::ShowWindow((HWND)tcItem.lParam, SW_SHOW);
				break;
			default:
				break;
			}
		}
		break;

	case WM_CLOSE:
		::SetForegroundWindow(::GetParent(hDlg));
		::DestroyWindow(hDlg);
		break;

	case WM_DESTROY:
		::EndDialog(hDlg, 0);
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

bool
ConfigDlg::initFontPage(HWND hDlg)
{
	HWND hwndList = ::GetDlgItem(hDlg, IDC_PART_LIST);
	::SendMessage(hwndList, LB_INSERTSTRING, 0, (LPARAM)"ヘッダ");
	::SendMessage(hwndList, LB_INSERTSTRING, 1, (LPARAM)"アドレス");
	::SendMessage(hwndList, LB_INSERTSTRING, 2, (LPARAM)"データ");
	::SendMessage(hwndList, LB_INSERTSTRING, 3, (LPARAM)"文字表示");

	return true;
}

void
ConfigDlg::selectFontPageList(HWND hDlg, int index)
{
	assert(index >= 0 && index < 4);

	HWND hwndFontName = ::GetDlgItem(hDlg, IDC_CONFIG_FONT_NAME);
	
}

BOOL CALLBACK
ConfigDlg::FontPageProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		initFontPage(hDlg);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PART_LIST:
			break;
		case IDC_CONFIG_FONT_BOLD:
			break;
		case IDC_CONFIG_FONT_FGCOLOR:
			break;
		case IDC_CONFIG_FONT_BGCOLOR:
			break;
		default:
			break;
		}
		break;
	case WM_NOTIFY:
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

