// $Id$

#include "configdlg.h"
#include "resource.h"
#include <commctrl.h>
#include <assert.h>

ConfigDialog::ConfigDialog(HWND hwndParent, DrawInfo* pDrawInfo)
{
	if (!pDrawInfo) {
		throw CreateDialogError();
	}

	m_hwndParent = hwndParent;
	m_hwndDlg = NULL;
	m_pDrawInfo = pDrawInfo;

	m_hInstance = (HINSTANCE)::GetWindowLong(hwndParent, GWL_HINSTANCE);
	if (!m_hInstance) {
		throw CreateDialogError();
	}
}

ConfigDialog::~ConfigDialog()
{
}

BOOL CALLBACK
ConfigDialog::configDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ConfigDialog* pCD = NULL;

	if (uMsg == WM_INITDIALOG) {
		pCD = (ConfigDialog*)lParam;
		assert(pCD);
		::SetWindowLong(hDlg, DWL_USER, (LONG)lParam);
		pCD->m_hwndDlg = hDlg;
		if (!pCD->initDialog(hDlg)) {
			::SendMessage(hDlg, WM_CLOSE, 0, 0);
			pCD->m_hwndDlg = NULL;
		}
		return FALSE;
	}

	pCD = (ConfigDialog*)::GetWindowLong(hDlg, DWL_USER);
	if (!pCD) return FALSE;

	if (uMsg == WM_DESTROY) {
		pCD->destroyDialog();
		pCD->m_hwndDlg = NULL;
		return TRUE;
	}

	return pCD->dialogProcMain(uMsg, wParam, lParam);
}

BOOL
ConfigDialog::initDialog(HWND hDlg)
{
	m_hwndDlg = hDlg;
	return TRUE;
}

ConfigPage::ConfigPage(HWND hwndParent, DrawInfo* pDrawInfo,
					   int nPageTemplateID)
	: ConfigDialog(hwndParent, pDrawInfo)
{
	m_nPageTemplateID = nPageTemplateID;
}

bool
ConfigPage::create(const RECT& rctPage)
{
	assert(m_hInstance);

	m_hwndDlg = ::CreateDialogParam(m_hInstance,
									MAKEINTRESOURCE(m_nPageTemplateID),
									m_hwndParent,
									(DLGPROC)ConfigDialog::configDialogProc,
									(LPARAM)this);
	if (!m_hwndDlg) return false;

	::SetWindowPos(m_hwndDlg, HWND_TOP,
				   rctPage.left, rctPage.top,
				   0, 0,
				   SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);

	return true;
}

void
ConfigPage::destroyDialog()
{
//	::MessageBox(NULL, "ConfigPage::destroyDialog()", NULL, MB_OK);
}

ConfigMainDlg::ConfigMainDlg(HWND hwndParent, DrawInfo* pDrawInfo)
	: ConfigDialog(hwndParent, pDrawInfo)
{
	::ZeroMemory(m_pConfigPages, sizeof(m_pConfigPages));

	m_pszTabText[0] = "フォント";
	m_pszTabText[1] = "カーソル";
}

bool
ConfigMainDlg::doModal()
{
	return !::DialogBoxParam(m_hInstance,
							 MAKEINTRESOURCE(IDD_CONFIG_MAIN),
							 m_hwndParent,
							 (DLGPROC)ConfigDialog::configDialogProc,
							 (LPARAM)this);
}

void
ConfigMainDlg::applyChanges()
{
	for (int i = 0; i < CONFIG_DIALOG_PAGE_NUM; i++) {
		if (m_pConfigPages[i])
			m_pConfigPages[i]->applyChanges();
	}
}

BOOL
ConfigMainDlg::initDialog(HWND hDlg)
{
	HWND hwndTab = ::GetDlgItem(hDlg, IDC_CONFIG_TAB);
	if (!hwndTab) return FALSE;

	TCITEM tcItem;
	tcItem.mask = TCIF_TEXT | TCIF_PARAM;
	for (int i = 0; i < CONFIG_DIALOG_PAGE_NUM; i++) {
		tcItem.pszText = (LPSTR)m_pszTabText[i];
		tcItem.lParam  = i;
		if (TabCtrl_InsertItem(hwndTab, i, &tcItem) != i) {
			return FALSE;
		}
	}

	// 何か項目を追加した後でないとこの計算が失敗する
	::GetWindowRect(hwndTab, &m_rctPage);
	TabCtrl_AdjustRect(hwndTab, FALSE, &m_rctPage);
	POINT ptOrg;
	ptOrg.x = m_rctPage.left;
	ptOrg.y = m_rctPage.top;
	::ScreenToClient(hDlg, &ptOrg);
	m_rctPage.right  = ptOrg.x + (m_rctPage.right - m_rctPage.left);
	m_rctPage.bottom = ptOrg.y + (m_rctPage.bottom - m_rctPage.top);
	m_rctPage.left   = ptOrg.x;
	m_rctPage.top    = ptOrg.y;

	return createPage(0);
}

void
ConfigMainDlg::destroyDialog()
{
	for (int i = 0; i < CONFIG_DIALOG_PAGE_NUM; i++) {
		if (m_pConfigPages[i]) {
			m_pConfigPages[i]->close();
		}
	}
	::EndDialog(m_hwndDlg, 0);
}

bool
ConfigMainDlg::createPage(int i)
{
	assert(i >= 0 && i < CONFIG_DIALOG_PAGE_NUM);

	if (m_pConfigPages[i]) return true;

	ConfigPage* pCP = NULL;
	switch (i) {
	case 0:
		pCP = new FontConfigPage(m_hwndDlg, m_pDrawInfo);
		break;
	case 1:
		pCP = new CursorConfigPage(m_hwndDlg, m_pDrawInfo);
		break;
	default:
		break;
	}
	m_pConfigPages[i] = pCP;

	return pCP != NULL && pCP->create(m_rctPage);
}

BOOL
ConfigMainDlg::dialogProcMain(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_APPLY:
			applyChanges();
			break;
		case IDOK:
			applyChanges();
			// through down
		case IDCANCEL:
			::SendMessage(m_hwndDlg, WM_CLOSE, 0, 0);
			break;
		}
		break;

	case WM_NOTIFY:
		if (((NMHDR*)lParam)->idFrom == IDC_CONFIG_TAB) {
			HWND hwndTab = ::GetDlgItem(m_hwndDlg, IDC_CONFIG_TAB);
			NMHDR* phdr = (NMHDR*)lParam;
			int iItem;
			TCITEM tcItem;
			tcItem.mask = TCIF_PARAM;
			tcItem.lParam = -1;
			switch (phdr->code) {
			case TCN_SELCHANGING:
				iItem = TabCtrl_GetCurSel(hwndTab);
				TabCtrl_GetItem(hwndTab, iItem, &tcItem);
				assert((UINT)tcItem.lParam < CONFIG_DIALOG_PAGE_NUM);
				assert(m_pConfigPages[tcItem.lParam]);
				m_pConfigPages[tcItem.lParam]->show(FALSE);
				break;
			case TCN_SELCHANGE:
				iItem = TabCtrl_GetCurSel(hwndTab);
				TabCtrl_GetItem(hwndTab, iItem, &tcItem);
				assert((UINT)tcItem.lParam < CONFIG_DIALOG_PAGE_NUM);
				if (!m_pConfigPages[tcItem.lParam]) {
					if (!createPage(tcItem.lParam)) {
						::MessageBox(m_hwndDlg, "", NULL, MB_OK);
						break;
					}
				}
				assert(m_pConfigPages[tcItem.lParam]);
				m_pConfigPages[tcItem.lParam]->show(TRUE);
				break;
			default:
				break;
			}
		}
		break;

	case WM_CLOSE:
		::SetForegroundWindow(m_hwndParent);
		::DestroyWindow(m_hwndDlg);
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

FontConfigPage::FontConfigPage(HWND hwndParent, DrawInfo* pDrawInfo)
	: ConfigPage(hwndParent, pDrawInfo, IDD_CONFIG_FONT)
{
}

BOOL
FontConfigPage::initDialog(HWND hDlg)
{
	HWND hwndList = ::GetDlgItem(hDlg, IDC_PART_LIST);

	::SendMessage(hwndList, LB_INSERTSTRING, 0, (LPARAM)"ヘッダ");
	::SendMessage(hwndList, LB_INSERTSTRING, 1, (LPARAM)"アドレス");
	::SendMessage(hwndList, LB_INSERTSTRING, 2, (LPARAM)"データ");
	::SendMessage(hwndList, LB_INSERTSTRING, 3, (LPARAM)"文字表示");

	return true;
}

BOOL
FontConfigPage::dialogProcMain(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
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

void
FontConfigPage::applyChanges()
{
}

void
FontConfigPage::selectFontPageList(int index)
{
	assert(index >= 0 && index < 4);

	HWND hwndFontName = ::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_NAME);
	
}

CursorConfigPage::CursorConfigPage(HWND hwndParent, DrawInfo* pDrawInfo)
	: ConfigPage(hwndParent, pDrawInfo, IDD_CONFIG_CURSOR)
{
}

BOOL
CursorConfigPage::initDialog(HWND hDlg)
{
	return true;
}

BOOL
CursorConfigPage::dialogProcMain(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_COMMAND:
#if 0
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
#endif
		break;

	case WM_NOTIFY:
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

void
CursorConfigPage::applyChanges()
{
}

