// $Id$

#pragma warning(disable : 4786)

#include "configdlg.h"
#include "resource.h"
#include <commctrl.h>
#include <assert.h>

ConfigDialog::ConfigDialog(DrawInfo* pDrawInfo)
	: m_hwndParent(NULL), m_hwndDlg(NULL),
	  m_pDrawInfo(pDrawInfo)
{
	if (!pDrawInfo) {
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

ConfigPage::ConfigPage(DrawInfo* pDrawInfo,
					   int nPageTemplateID, const char* pszTabText)
	: ConfigDialog(pDrawInfo),
	  m_nPageTemplateID(nPageTemplateID),
	  m_strTabText(pszTabText)
{
}

bool
ConfigPage::create(HWND hwndParent, const RECT& rctPage)
{
	if (m_hwndDlg) return true;

	m_hwndParent = hwndParent;

	m_hwndDlg = ::CreateDialogParam((HINSTANCE)::GetWindowLong(hwndParent,
															   GWL_HINSTANCE),
									MAKEINTRESOURCE(m_nPageTemplateID),
									hwndParent,
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

ConfigMainDlg::ConfigMainDlg(DrawInfo* pDrawInfo)
	: ConfigDialog(pDrawInfo)
{
	m_pConfigPages[0] = new FontConfigPage(pDrawInfo);
	m_pConfigPages[1] = new CursorConfigPage(pDrawInfo);
}

ConfigMainDlg::~ConfigMainDlg()
{
	for (int i = 0; i < CONFIG_DIALOG_PAGE_NUM; i++)
		delete m_pConfigPages[i];
}

bool
ConfigMainDlg::doModal(HWND hwndParent)
{
	m_hwndParent = hwndParent;
	return !::DialogBoxParam((HINSTANCE)::GetWindowLong(hwndParent,
														GWL_HINSTANCE),
							 MAKEINTRESOURCE(IDD_CONFIG_MAIN),
							 hwndParent,
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
	if (!ConfigDialog::initDialog(hDlg)) return FALSE;

	HWND hwndTab = ::GetDlgItem(hDlg, IDC_CONFIG_TAB);
	if (!hwndTab) return FALSE;

	TCITEM tcItem;
	tcItem.mask = TCIF_TEXT | TCIF_PARAM;
	for (int i = 0; i < CONFIG_DIALOG_PAGE_NUM; i++) {
		tcItem.pszText = (LPSTR)m_pConfigPages[i]->getTabText();
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

	return m_pConfigPages[i]->create(m_hwndDlg, m_rctPage);
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
				if (!createPage(tcItem.lParam)) {
					::MessageBox(m_hwndDlg, "", NULL, MB_OK);
					break;
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

FontConfigPage::FontConfigPage(DrawInfo* pDrawInfo)
	: ConfigPage(pDrawInfo, IDD_CONFIG_FONT, "フォント")
{
}

BOOL
FontConfigPage::initDialog(HWND hDlg)
{
	if (!ConfigPage::initDialog(hDlg)) return FALSE;

	prepareFontList();

	char faceName[LF_FACESIZE];
	faceName[0] = '\0';
	if (::GetTextFace(m_pDrawInfo->m_hDC, LF_FACESIZE, faceName)) {
		HWND hwndFontList = ::GetDlgItem(hDlg, IDC_CONFIG_FONT_NAME);
		int pos = ::SendMessage(hwndFontList,
								CB_FINDSTRINGEXACT,
								-1, (LPARAM)faceName);
		if (pos != CB_ERR) {
			::SendMessage(hwndFontList, CB_SETCURSEL,
						  pos, 0);
		}
	}

	HWND hwndList = ::GetDlgItem(hDlg, IDC_PART_LIST);

	::SendMessage(hwndList, LB_INSERTSTRING, 0, (LPARAM)"ヘッダ");
	::SendMessage(hwndList, LB_INSERTSTRING, 1, (LPARAM)"アドレス");
	::SendMessage(hwndList, LB_INSERTSTRING, 2, (LPARAM)"データ");
	::SendMessage(hwndList, LB_INSERTSTRING, 3, (LPARAM)"文字表示");

	return TRUE;
}

void
FontConfigPage::prepareFontList()
{
	m_bShowPropFonts
		= ::SendMessage(::GetDlgItem(m_hwndDlg, IDC_CONFIG_SHOW_PROPFONT),
						BM_GETCHECK,
						0, 0) == BST_CHECKED;

	::SendMessage(::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_NAME),
				  CB_RESETCONTENT,
				  0, 0);

	m_mapFontName.clear();

	LOGFONT logFont;
	logFont.lfCharSet = DEFAULT_CHARSET;
	logFont.lfFaceName[0] = '\0';
	logFont.lfPitchAndFamily = 0;
	::EnumFontFamiliesEx(m_pDrawInfo->m_hDC,
						 &logFont,
						 (FONTENUMPROC)FontConfigPage::enumFontProc,
						 (LPARAM)this,
						 0);
}

int CALLBACK
FontConfigPage::enumFontProc(ENUMLOGFONTEX *lpelfe,
							 NEWTEXTMETRICEX *lpntme,
							 DWORD FontType,
							 LPARAM lParam)
{
	FontConfigPage* _this = (FontConfigPage*)lParam;

	if (!_this) return 0;

	_this->addFont(lpelfe->elfLogFont);

	return 1;
}

void
FontConfigPage::addFont(const LOGFONT& logFont)
{
	if (logFont.lfFaceName[0] == '@') return;

	if (m_mapFontName.find(logFont.lfFaceName) != m_mapFontName.end())
		return;

	if (!m_bShowPropFonts) {
		if ((logFont.lfPitchAndFamily & 0x03) == VARIABLE_PITCH)
			return;
	}

	int pos = ::SendMessage(::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_NAME),
							CB_ADDSTRING,
							0,
							(LPARAM)logFont.lfFaceName);

	// ソートされるため、pos の値は意味がなくなる
	m_mapFontName.insert(make_pair(string(logFont.lfFaceName), pos));
}

void
FontConfigPage::prepareFontSize()
{
	HWND hwndFontList = ::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_NAME);
	int sel = ::SendMessage(hwndFontList, CB_GETCURSEL, 0, 0);
	if (sel == CB_ERR) return;

	char faceName[LF_FACESIZE];
	::SendMessage(hwndFontList, CB_GETLBTEXT, sel, (LPARAM)faceName);


}

BOOL
FontConfigPage::dialogProcMain(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CONFIG_FONT_NAME:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
			}
			break;
		case IDC_CONFIG_SHOW_PROPFONT:
			if (HIWORD(wParam) == BN_CLICKED) {
				char faceName[LF_FACESIZE];
				faceName[0] = '\0';
				HWND hwndFontList = ::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_NAME);
				int sel = ::SendMessage(hwndFontList, CB_GETCURSEL, 0, 0);
				if (sel != CB_ERR) {
					::SendMessage(hwndFontList, CB_GETLBTEXT,
								  sel, (LPARAM)faceName);
				}
				prepareFontList();
				if (sel != CB_ERR) {
					int pos = ::SendMessage(hwndFontList,
											CB_FINDSTRINGEXACT,
											-1, (LPARAM)faceName);
					if (pos != CB_ERR) {
						::SendMessage(hwndFontList, CB_SETCURSEL,
									  pos, 0);
					}
				}
			}
			break;
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

CursorConfigPage::CursorConfigPage(DrawInfo* pDrawInfo)
	: ConfigPage(pDrawInfo, IDD_CONFIG_CURSOR, "カーソル")
{
}

BOOL
CursorConfigPage::initDialog(HWND hDlg)
{
	if (!ConfigPage::initDialog(hDlg)) return FALSE;

	return TRUE;
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

