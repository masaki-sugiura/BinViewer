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
	  m_strTabText(pszTabText),
	  m_bShown(false)
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

	m_bShown = true;

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

int
ConfigMainDlg::getCurrentPage() const
{
	if (!m_hwndDlg) return -1;

	HWND hwndTab = ::GetDlgItem(m_hwndDlg, IDC_CONFIG_TAB);
	int iItem = TabCtrl_GetCurSel(hwndTab);
	TCITEM tcItem;
	tcItem.mask = TCIF_PARAM;
	tcItem.lParam = -1;
	TabCtrl_GetItem(hwndTab, iItem, &tcItem);

	assert((UINT)tcItem.lParam < CONFIG_DIALOG_PAGE_NUM);
	assert(m_pConfigPages[tcItem.lParam]);

	return tcItem.lParam;
}

bool
ConfigMainDlg::applyChanges()
{
	for (int i = 0; i < CONFIG_DIALOG_PAGE_NUM; i++) {
		ConfigPage* pPage = m_pConfigPages[i];
		if (pPage && pPage->isShown()) {
			if (!pPage->applyChanges()) {
				int pnum = getCurrentPage();
				if (pnum >= 0) m_pConfigPages[pnum]->show(FALSE);
				pPage->show(TRUE);
				return false;
			}
		}
	}
	return true;
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
			if (HIWORD(wParam) == BN_CLICKED && applyChanges()) {
				::SendMessage(m_hwndDlg, WM_USER_ENABLE_APPLY_BUTTON, FALSE, 0);
			}
			break;
		case IDOK:
			if (HIWORD(wParam) == BN_CLICKED && applyChanges()) {
				::SendMessage(m_hwndDlg, WM_CLOSE, 0, 0);
			}
			break;
		case IDCANCEL:
			if (HIWORD(wParam) == BN_CLICKED) {
				::SendMessage(m_hwndDlg, WM_CLOSE, 0, 0);
			}
			break;
		default:
			break;
		}
		break;

	case WM_NOTIFY:
		if (((NMHDR*)lParam)->idFrom == IDC_CONFIG_TAB) {
			NMHDR* phdr = (NMHDR*)lParam;
			int pnum;
			switch (phdr->code) {
			case TCN_SELCHANGING:
				pnum = getCurrentPage();
				if (pnum >= 0) m_pConfigPages[pnum]->show(FALSE);
				break;
			case TCN_SELCHANGE:
				pnum = getCurrentPage();
				if (pnum < 0) break;
				if (!createPage(pnum)) {
					::MessageBox(m_hwndDlg, "ダイアログの作成に失敗しました。",
								 NULL, MB_OK);
					break;
				}
				m_pConfigPages[pnum]->show(TRUE);
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

	case WM_USER_ENABLE_APPLY_BUTTON:
		::EnableWindow(::GetDlgItem(m_hwndDlg, IDC_APPLY), wParam);
		break;

	case WM_USER_SET_FONT_CONFIG:
		::SendMessage(m_hwndParent, uMsg, wParam, lParam);
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

static inline void
get_font_pt_from_pixel(HDC hDC, int pixel, LPSTR buf)
{
	float pt_size = (float)pixel * 72 / ::GetDeviceCaps(hDC, LOGPIXELSY);
	sprintf(buf, "%4.1f", pt_size);
}

FontConfigPage::FontConfigPage(DrawInfo* pDrawInfo)
	: ConfigPage(pDrawInfo, IDD_CONFIG_FONT, "フォント")
{
}

BOOL
FontConfigPage::initDialog(HWND hDlg)
{
	if (!ConfigPage::initDialog(hDlg) ||
		!initFontConfig()) return FALSE;

	prepareFontList(m_pDrawInfo->m_FontInfo.isProportional());

	HWND hwndFontList = ::GetDlgItem(hDlg, IDC_CONFIG_FONT_NAME);
	int pos = ::SendMessage(hwndFontList,
							CB_FINDSTRINGEXACT,
							-1,
							(LPARAM)m_FontConfig.m_pszFontFace);
	if (pos == CB_ERR) return FALSE;

	::SendMessage(hwndFontList, CB_SETCURSEL, pos, 0);

	if (m_pDrawInfo->m_FontInfo.isProportional())
		::CheckDlgButton(hDlg, IDC_CONFIG_SHOW_PROPFONT, BST_CHECKED);

	prepareFontSize();

	TEXTMETRIC tm;
	::GetTextMetrics(m_pDrawInfo->m_hDC, &tm);
	char buf[8];
	get_font_pt_from_pixel(m_pDrawInfo->m_hDC, tm.tmHeight, buf);
	HWND hwndFontSize = ::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_SIZE);
	pos = ::SendMessage(hwndFontSize, CB_FINDSTRINGEXACT,
						(WPARAM)-1, (LPARAM)buf);
	if (pos != CB_ERR) {
		::SendMessage(hwndFontSize, CB_SETCURSEL, pos, 0);
	} else {
		::SetWindowText(hwndFontSize, buf);
	}

	HWND hwndList = ::GetDlgItem(hDlg, IDC_PART_LIST);

	::SendMessage(hwndList, LB_INSERTSTRING,
				  CC_HEADER, (LPARAM)"ヘッダ");
	::SendMessage(hwndList, LB_INSERTSTRING,
				  CC_ADDRESS, (LPARAM)"アドレス");
	::SendMessage(hwndList, LB_INSERTSTRING,
				  CC_DATA, (LPARAM)"データ");
	::SendMessage(hwndList, LB_INSERTSTRING,
				  CC_STRING, (LPARAM)"文字表示");

	m_icFgColor.m_hIcon = NULL;
	m_icBkColor.m_hIcon = NULL;

	return TRUE;
}

void
FontConfigPage::destroyDialog()
{
	if (m_icFgColor.m_hIcon) {
		::DestroyIcon(m_icFgColor.m_hIcon);
		::DestroyIcon(m_icBkColor.m_hIcon);
		m_icFgColor.m_hIcon = m_icBkColor.m_hIcon = NULL;
		::DeleteObject(m_icFgColor.m_hbmColor);
		::DeleteObject(m_icFgColor.m_hbmMask);
		::DeleteObject(m_icBkColor.m_hbmColor);
		::DeleteObject(m_icBkColor.m_hbmMask);
	}
}

bool
FontConfigPage::initFontConfig()
{
	lstrcpy(m_FontConfig.m_pszFontFace, m_pDrawInfo->m_FontInfo.getFaceName());
	m_FontConfig.m_fFontSize = m_pDrawInfo->m_FontInfo.getFontSize();
	m_FontConfig.m_bBoldFace = m_pDrawInfo->m_FontInfo.isBoldFace();

	m_FontConfig.m_ColorConfig[CC_HEADER].m_crFgColor
		= m_pDrawInfo->m_tciHeader.getFgColor();
	m_FontConfig.m_ColorConfig[CC_HEADER].m_crBkColor
		= m_pDrawInfo->m_tciHeader.getBkColor();

	m_FontConfig.m_ColorConfig[CC_ADDRESS].m_crFgColor
		= m_pDrawInfo->m_tciAddress.getFgColor();
	m_FontConfig.m_ColorConfig[CC_ADDRESS].m_crBkColor
		= m_pDrawInfo->m_tciAddress.getBkColor();

	m_FontConfig.m_ColorConfig[CC_DATA].m_crFgColor
		= m_pDrawInfo->m_tciData.getFgColor();
	m_FontConfig.m_ColorConfig[CC_DATA].m_crBkColor
		= m_pDrawInfo->m_tciData.getBkColor();

	m_FontConfig.m_ColorConfig[CC_STRING].m_crFgColor
		= m_pDrawInfo->m_tciString.getFgColor();
	m_FontConfig.m_ColorConfig[CC_STRING].m_crBkColor
		= m_pDrawInfo->m_tciString.getBkColor();

	return true;
}

void
FontConfigPage::prepareFontList(bool bShowPropFonts)
{
	m_bShowPropFonts = bShowPropFonts;

	::SendDlgItemMessage(m_hwndDlg, IDC_CONFIG_FONT_NAME,
						 CB_RESETCONTENT, 0, 0);

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

	_this->addFont(lpelfe->elfLogFont, FontType);

	return 1;
}

void
FontConfigPage::addFont(const LOGFONT& logFont, DWORD fontType)
{
	if (logFont.lfFaceName[0] == '@') return;

	if (m_mapFontName.find(logFont.lfFaceName) != m_mapFontName.end())
		return;

	if (!m_bShowPropFonts) {
		if ((logFont.lfPitchAndFamily & 0x03) == VARIABLE_PITCH)
			return;
	}

	int pos = ::SendDlgItemMessage(m_hwndDlg, IDC_CONFIG_FONT_NAME,
								   CB_ADDSTRING,
								   0,
								   (LPARAM)logFont.lfFaceName);

	m_mapFontName.insert(make_pair(string(logFont.lfFaceName), fontType));
}

void
FontConfigPage::prepareFontSize()
{
	HWND hwndFontSize = ::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_SIZE);
	char sizebuf[8];
	::GetWindowText(hwndFontSize, sizebuf, 7);

	HWND hwndFontList = ::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_NAME);
	int sel = ::SendMessage(hwndFontList, CB_GETCURSEL, 0, 0);
	if (sel == CB_ERR) return;

	LOGFONT logFont;
	logFont.lfCharSet = DEFAULT_CHARSET;
	logFont.lfPitchAndFamily = 0;

	::SendMessage(hwndFontList, CB_GETLBTEXT, sel,
				  (LPARAM)logFont.lfFaceName);

	::SendMessage(hwndFontSize, CB_RESETCONTENT, 0, 0);

	DWORD fontType = m_mapFontName[logFont.lfFaceName];
	if ((fontType & TRUETYPE_FONTTYPE) != 0 ||
		(fontType & RASTER_FONTTYPE) == 0) {
		HWND hwndFontSize = ::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_SIZE);
		::SendMessage(hwndFontSize, CB_ADDSTRING, 0, (LPARAM)" 8.0");
		::SendMessage(hwndFontSize, CB_ADDSTRING, 0, (LPARAM)" 9.0");
		::SendMessage(hwndFontSize, CB_ADDSTRING, 0, (LPARAM)" 9.5");
		::SendMessage(hwndFontSize, CB_ADDSTRING, 0, (LPARAM)"10.0");
		::SendMessage(hwndFontSize, CB_ADDSTRING, 0, (LPARAM)"10.5");
		::SendMessage(hwndFontSize, CB_ADDSTRING, 0, (LPARAM)"11.0");
		::SendMessage(hwndFontSize, CB_ADDSTRING, 0, (LPARAM)"12.0");
		::SendMessage(hwndFontSize, CB_ADDSTRING, 0, (LPARAM)"14.0");
		::SendMessage(hwndFontSize, CB_ADDSTRING, 0, (LPARAM)"16.0");
		::SendMessage(hwndFontSize, CB_ADDSTRING, 0, (LPARAM)"18.0");
		::SendMessage(hwndFontSize, CB_ADDSTRING, 0, (LPARAM)"20.0");
		::SendMessage(hwndFontSize, CB_ADDSTRING, 0, (LPARAM)"22.0");
		::SendMessage(hwndFontSize, CB_ADDSTRING, 0, (LPARAM)"24.0");
		::SendMessage(hwndFontSize, CB_ADDSTRING, 0, (LPARAM)"26.0");
		::SendMessage(hwndFontSize, CB_ADDSTRING, 0, (LPARAM)"28.0");
		::SendMessage(hwndFontSize, CB_ADDSTRING, 0, (LPARAM)"36.0");
		::SendMessage(hwndFontSize, CB_ADDSTRING, 0, (LPARAM)"48.0");
		::SendMessage(hwndFontSize, CB_ADDSTRING, 0, (LPARAM)"72.0");
		::SetWindowText(hwndFontSize, sizebuf);
	} else {
		::EnumFontFamiliesEx(m_pDrawInfo->m_hDC,
							 &logFont,
							 (FONTENUMPROC)FontConfigPage::enumSpecificFontProc,
							 (LPARAM)this,
							 0);
		if (sizebuf[0] != '\0') {
			int pos = ::SendMessage(hwndFontSize, CB_FINDSTRINGEXACT,
									-1, (LPARAM)sizebuf);
			if (pos != CB_ERR) {
				::SetWindowText(hwndFontSize, sizebuf);
			}
		}
	}
}

int CALLBACK
FontConfigPage::enumSpecificFontProc(ENUMLOGFONTEX* lpelfe,
									 NEWTEXTMETRICEX* lpntme,
									 DWORD FontType,
									 LPARAM lParam)
{
	FontConfigPage* _this = (FontConfigPage*)lParam;

	if (!_this) return 0;

	_this->addSize(lpelfe->elfLogFont.lfHeight);

	return 1;
}

void
FontConfigPage::addSize(int size)
{
	char buf[16];
	get_font_pt_from_pixel(m_pDrawInfo->m_hDC, size, buf);

	HWND hwndSizeList = ::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_SIZE);
	if (::SendMessage(hwndSizeList, CB_FINDSTRINGEXACT,
					  (WPARAM)-1, (LPARAM)buf) == CB_ERR) {
		::SendDlgItemMessage(m_hwndDlg, IDC_CONFIG_FONT_SIZE,
							 CB_ADDSTRING, 0, (LPARAM)buf);
	}
}

BOOL
FontConfigPage::dialogProcMain(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CONFIG_FONT_NAME:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				::SendMessage(m_hwndParent, WM_USER_ENABLE_APPLY_BUTTON, TRUE, 0);
				prepareFontSize();
			}
			break;
		case IDC_CONFIG_FONT_SIZE:
			if (HIWORD(wParam) == CBN_SELCHANGE ||
				HIWORD(wParam) == CBN_EDITCHANGE) {
				::SendMessage(m_hwndParent, WM_USER_ENABLE_APPLY_BUTTON,
							  TRUE, 0);
			}
			break;
		case IDC_CONFIG_SHOW_PROPFONT:
			if (HIWORD(wParam) == BN_CLICKED) {
				char faceName[LF_FACESIZE];
//				faceName[0] = '\0';
				HWND hwndFontList = ::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_NAME);
				int sel = ::SendMessage(hwndFontList, CB_GETCURSEL, 0, 0);
				if (sel != CB_ERR) {
					::SendMessage(hwndFontList, CB_GETLBTEXT,
								  sel, (LPARAM)faceName);
				}
				bool bShowPropFonts
					= ::SendDlgItemMessage(m_hwndDlg, IDC_CONFIG_SHOW_PROPFONT,
										   BM_GETCHECK, 0, 0) == BST_CHECKED;
				prepareFontList(bShowPropFonts);
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
			if (HIWORD(wParam) == LBN_SELCHANGE) {
				int pos = ::SendDlgItemMessage(m_hwndDlg, IDC_PART_LIST,
											   LB_GETCURSEL, 0, 0);
				if (pos != LB_ERR) {
					selectFontPageList(pos);
				}
			}
			break;
		case IDC_CONFIG_FONT_BOLD:
			if (HIWORD(wParam) == BN_CLICKED) {
				::SendMessage(m_hwndParent, WM_USER_ENABLE_APPLY_BUTTON, TRUE, 0);
			}
			break;
		case IDC_CONFIG_FONT_FGCOLOR:
			if (HIWORD(wParam) == BN_CLICKED) {
				int pos = ::SendDlgItemMessage(m_hwndDlg, IDC_PART_LIST,
											   LB_GETCURSEL, 0, 0);
				if (pos != LB_ERR) {
					if (chooseColor(m_FontConfig.m_ColorConfig[pos].m_crFgColor)) {
						::SendMessage(m_hwndParent, WM_USER_ENABLE_APPLY_BUTTON,
									  TRUE, 0);
						selectFontPageList(pos);
					}
				}
			}
			break;
		case IDC_CONFIG_FONT_BKCOLOR:
			if (HIWORD(wParam) == BN_CLICKED) {
				int pos = ::SendDlgItemMessage(m_hwndDlg, IDC_PART_LIST,
											   LB_GETCURSEL, 0, 0);
				if (pos != LB_ERR) {
					if (chooseColor(m_FontConfig.m_ColorConfig[pos].m_crBkColor)) {
						::SendMessage(m_hwndParent, WM_USER_ENABLE_APPLY_BUTTON,
									  TRUE, 0);
						selectFontPageList(pos);
					}
				}
			}
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

bool
FontConfigPage::applyChanges()
{
	::GetWindowText(::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_NAME),
					m_FontConfig.m_pszFontFace, LF_FACESIZE - 1);
	if (m_FontConfig.m_pszFontFace[0] == '\0') {
		::MessageBox(m_hwndDlg, "フォント名が指定されていません。",
					 NULL, MB_OK);
		return false;
	}
	char fontSize[8];
	::GetWindowText(::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_SIZE),
					fontSize, 7);
	float font_size = strtod(fontSize, NULL);
	if (font_size <= 0.0) {
		::MessageBox(m_hwndDlg, "フォントサイズの指定が不正です。",
					 NULL, MB_OK);
		return false;
	}
	m_FontConfig.m_fFontSize = font_size;
	m_FontConfig.m_bBoldFace
		= ::SendDlgItemMessage(m_hwndDlg, IDC_CONFIG_FONT_BOLD,
							   BM_GETCHECK, 0, 0) == BST_CHECKED;

	::SendMessage(m_hwndParent, WM_USER_SET_FONT_CONFIG,
				  0, (LPARAM)&m_FontConfig);

	return true;
}

void
FontConfigPage::selectFontPageList(int index)
{
	assert(index >= 0 && index < 4);

	RECT rctIcon;
	rctIcon.left = rctIcon.top = 0;
	rctIcon.right = rctIcon.bottom = 16;

	HDC hDC = ::GetDC(m_hwndDlg);

	HDC hCDC = ::CreateCompatibleDC(hDC);

	if (!m_icFgColor.m_hIcon) {
		m_icFgColor.m_hbmColor = ::CreateCompatibleBitmap(hDC, 16, 16);
		m_icFgColor.m_hbmMask  = ::CreateCompatibleBitmap(hDC, 16, 16);
		m_icBkColor.m_hbmColor = ::CreateCompatibleBitmap(hDC, 16, 16);
		m_icBkColor.m_hbmMask  = ::CreateCompatibleBitmap(hDC, 16, 16);
		::SelectObject(hCDC, m_icFgColor.m_hbmMask);
		HBRUSH hbrMask = ::CreateSolidBrush(RGB(0, 0, 0));
		::FillRect(hCDC, &rctIcon, hbrMask);
		::SelectObject(hCDC, m_icBkColor.m_hbmMask);
		::FillRect(hCDC, &rctIcon, hbrMask);
		::DeleteObject(hbrMask);
	} else {
		::DestroyIcon(m_icFgColor.m_hIcon);
		::DestroyIcon(m_icBkColor.m_hIcon);
	}
	::ReleaseDC(m_hwndDlg, hDC);

	::SelectObject(hCDC, m_icFgColor.m_hbmColor);

	HBRUSH hBrush = ::CreateSolidBrush(m_FontConfig.m_ColorConfig[index].m_crFgColor);
	::FillRect(hCDC, &rctIcon, hBrush);
	::DeleteObject(hBrush);
	hBrush = ::CreateSolidBrush(m_FontConfig.m_ColorConfig[index].m_crBkColor);
	::SelectObject(hCDC, m_icBkColor.m_hbmColor);
	::FillRect(hCDC, &rctIcon, hBrush);
	::DeleteObject(hBrush);

	::DeleteDC(hCDC);

	ICONINFO iconInfo;
	iconInfo.fIcon = TRUE;
	iconInfo.hbmColor = m_icFgColor.m_hbmColor;
	iconInfo.hbmMask  = m_icFgColor.m_hbmMask;
	m_icFgColor.m_hIcon = ::CreateIconIndirect(&iconInfo);
	iconInfo.hbmColor = m_icBkColor.m_hbmColor;
	iconInfo.hbmMask  = m_icBkColor.m_hbmMask;
	m_icBkColor.m_hIcon = ::CreateIconIndirect(&iconInfo);

	::SendDlgItemMessage(m_hwndDlg, IDC_CONFIG_FONT_FGCOLOR,
						 BM_SETIMAGE, IMAGE_ICON,
						 (LPARAM)m_icFgColor.m_hIcon);
	::SendDlgItemMessage(m_hwndDlg, IDC_CONFIG_FONT_BKCOLOR,
						 BM_SETIMAGE, IMAGE_ICON,
						 (LPARAM)m_icBkColor.m_hIcon);
}

bool
FontConfigPage::chooseColor(COLORREF& cref)
{
	CHOOSECOLOR cc;

	COLORREF cref_user[16];
	::ZeroMemory(cref_user, sizeof(cref_user));

	::ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = m_hwndDlg;
	cc.rgbResult = cref;
	cc.lpCustColors = cref_user;
	cc.Flags = CC_ANYCOLOR | CC_RGBINIT;

	if (::ChooseColor(&cc)) {
		cref = cc.rgbResult;
		return true;
	}
	return false;
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

bool
CursorConfigPage::applyChanges()
{
	return true;
}

