// $Id$

#pragma warning(disable : 4786)

#include "configdlg.h"
#include "resource.h"
#include <commctrl.h>
#include <Tmschema.h>
#include <assert.h>

#ifndef WM_THEMECHANGED
#define WM_THEMECHANGED  0x031A
#endif

ConfigDialog::ConfigDialog(int nDialogID, Auto_Ptr<AppConfig>& pAppConfig)
	: Dialog(nDialogID),
	  m_pAppConfig(pAppConfig)
{
	if (!pAppConfig.ptr()) {
		throw CreateDialogError();
	}
}

ConfigDialog::~ConfigDialog()
{
}

ConfigPage::ConfigPage(int nDialogID, Auto_Ptr<AppConfig>& pAppConfig,
					   const char* pszTabText)
	: ConfigDialog(nDialogID, pAppConfig),
	  m_strTabText(pszTabText),
	  m_bShown(false)
{
}

BOOL
ConfigPage::initDialog(HWND hDlg)
{
	Dialog::setTextureToTabColor();
	return TRUE;
}

void
ConfigPage::destroyDialog()
{
	m_bShown = false;
//	::MessageBox(NULL, "ConfigPage::destroyDialog()", NULL, MB_OK);
}

ConfigMainDlg::ConfigMainDlg(Auto_Ptr<AppConfig>& pAppConfig)
	: ConfigDialog(IDD_CONFIG_MAIN, pAppConfig)
{
	m_pConfigPages[0] = new FontConfigPage(pAppConfig);
//	m_pConfigPages[1] = new CursorConfigPage(pAppConfig);
}

ConfigMainDlg::~ConfigMainDlg()
{
	for (int i = 0; i < CONFIG_DIALOG_PAGE_NUM; i++)
		delete m_pConfigPages[i];
}

bool
ConfigMainDlg::applyChanges()
{
	if (!m_bDirty) return true;
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
	m_bDirty = FALSE;
	return true;
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

BOOL
ConfigMainDlg::initDialog(HWND hDlg)
{
	m_bDirty = FALSE;

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
//	::EndDialog(m_hwndDlg, 0);
}

bool
ConfigMainDlg::createPage(int i)
{
	assert(i >= 0 && i < CONFIG_DIALOG_PAGE_NUM);

	HWND hDlg = m_pConfigPages[i]->create(m_hwndDlg);
	if (!hDlg) return false;

	::SetWindowPos(hDlg, HWND_TOP,
				   m_rctPage.left, m_rctPage.top,
				   0, 0,
				   SWP_NOSIZE | SWP_NOOWNERZORDER);

	m_pConfigPages[i]->show(TRUE);

	return true;
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
		::EndDialog(m_hwndDlg, 0);
		break;

	case WM_USER_ENABLE_APPLY_BUTTON:
		if (wParam) m_bDirty = TRUE;
		::EnableWindow(::GetDlgItem(m_hwndDlg, IDC_APPLY), wParam);
		break;

	case WM_USER_SET_FONT_CONFIG:
		::SendMessage(m_hwndParent, uMsg, wParam, lParam);
		break;

	case WM_USER_SET_SCROLL_CONFIG:
		::SendMessage(m_hwndParent, uMsg, wParam, lParam);
		break;

	default:
		return FALSE;
	}

	return TRUE;
}


Icon::Icon(HDC hDC)
	: m_hDC(NULL), m_hbmColor(NULL)
{
	m_hDC = ::CreateCompatibleDC(hDC);

	m_rctIcon.left = m_rctIcon.top = 0;
	m_rctIcon.right  = 44;
	m_rctIcon.bottom = 16;

	m_hbmColor = ::CreateCompatibleBitmap(hDC, 44, 16);

//	::SelectObject(m_hDC, m_hbmColor);
}

Icon::~Icon()
{
	::DeleteDC(m_hDC);
	::DeleteObject(m_hbmColor);
}

HBITMAP
Icon::setColor(COLORREF cref)
{
	HGDIOBJ hOrgBitmap = ::SelectObject(m_hDC, m_hbmColor);
	HBRUSH hBrush = ::CreateSolidBrush(cref);
	::SelectObject(m_hDC, hBrush);
	::FillRect(m_hDC, &m_rctIcon, hBrush);
	::DeleteObject(hBrush);

	return (HBITMAP)::SelectObject(m_hDC, hOrgBitmap);
}

SampleView::SampleView(HWND hwndSample,
					   FontConfig& fontConfig,
					   ColorConfig* colorConfig)
	: m_hwndSample(hwndSample)
{
	::SetWindowLong(m_hwndSample, GWL_USERDATA, (LONG)this);
	m_pfnOrgWndProc = (WNDPROC)::SetWindowLong(m_hwndSample, GWL_WNDPROC,
											   (LONG)SampleViewProc);
	::GetClientRect(m_hwndSample, &m_rctSample);
	HDC hDC = ::GetDC(m_hwndSample);
	m_hdcSample = ::CreateCompatibleDC(hDC);
	m_hbmSample = ::CreateCompatibleBitmap(hDC,
										   m_rctSample.right - m_rctSample.left,
										   m_rctSample.bottom - m_rctSample.top);
	::SelectObject(m_hdcSample, m_hbmSample);
	::ReleaseDC(m_hwndSample, hDC);
	updateSample(fontConfig, colorConfig);
}

SampleView::~SampleView()
{
	::SetWindowLong(m_hwndSample, GWL_WNDPROC, (LONG)m_pfnOrgWndProc);
	::DeleteObject(m_hbmSample);
	::DeleteDC(m_hdcSample);
}

void
SampleView::updateSample(FontConfig& fontConfig, ColorConfig* colorConfig)
{
	LOGFONT lfont;
	lfont.lfHeight = - (int) (fontConfig.m_fFontSize
							  * ::GetDeviceCaps(m_hdcSample, LOGPIXELSY) / 72);
	lfont.lfWidth  = 0;
	lfont.lfEscapement = 0;
	lfont.lfOrientation = 0;
	lfont.lfWeight = fontConfig.m_bBoldFace ? FW_BOLD : FW_NORMAL;
	lfont.lfItalic = FALSE;
	lfont.lfUnderline = FALSE;
	lfont.lfStrikeOut = FALSE;
	lfont.lfCharSet = DEFAULT_CHARSET;
	lfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lfont.lfQuality = DEFAULT_QUALITY;
	lfont.lfPitchAndFamily = FF_DONTCARE;
	lstrcpy(lfont.lfFaceName, fontConfig.m_pszFontFace);
	HFONT hFont = ::CreateFontIndirect(&lfont);
	if (!hFont) return;

	HGDIOBJ hOrgFont = ::SelectObject(m_hdcSample, hFont);
	TEXTMETRIC tm;
	if (!::GetTextMetrics(m_hdcSample, &tm)) {
		::SelectObject(m_hdcSample, hOrgFont);
		::DeleteObject(hFont);
		return;
	}

	int anXPitch[9];
	for (int i = 0; i < 9; i++) {
		anXPitch[i] = tm.tmAveCharWidth;
	}

	int height = m_rctSample.bottom - m_rctSample.top + tm.tmHeight + 1,
		a_offset = tm.tmAveCharWidth / 2,
		d_offset = tm.tmAveCharWidth * 5 + tm.tmAveCharWidth / 2,
		s_offset = m_rctSample.right - m_rctSample.left - tm.tmAveCharWidth * 4
					+ tm.tmAveCharWidth / 2;

	HBRUSH hbrBackground = ::CreateSolidBrush(colorConfig[3].m_crBkColor);
	RECT rcPaint = m_rctSample;
	rcPaint.bottom = tm.tmHeight + 1;
	::FillRect(m_hdcSample, &rcPaint, hbrBackground);
	::DeleteObject(hbrBackground);
	::SetTextColor(m_hdcSample, colorConfig[3].m_crFgColor);
	::SetBkColor(m_hdcSample, colorConfig[3].m_crBkColor);
	::ExtTextOut(m_hdcSample, a_offset, 0,
				 0, NULL, "0000", 4, anXPitch);
	::ExtTextOut(m_hdcSample, d_offset, 0,
				 0, NULL, "00 01 ...", 9, anXPitch);
	::ExtTextOut(m_hdcSample, s_offset, 0,
				 0, NULL, "012", 3, anXPitch);

	hbrBackground = ::CreateSolidBrush(colorConfig[0].m_crBkColor);
	rcPaint.top    = tm.tmHeight + 1;
	rcPaint.bottom = m_rctSample.bottom;
	rcPaint.right  = tm.tmAveCharWidth * 5;
	::FillRect(m_hdcSample, &rcPaint, hbrBackground);
	::DeleteObject(hbrBackground);
	::SetTextColor(m_hdcSample, colorConfig[0].m_crFgColor);
	::SetBkColor(m_hdcSample, colorConfig[0].m_crBkColor);
	int addr = 0, y;
	for (y = tm.tmHeight + 1; y < height; y += tm.tmHeight + 1) {
		char buf[5];
		wsprintf(buf, "%04x", addr);
		::ExtTextOut(m_hdcSample, a_offset, y,
					 0, NULL, buf, 4, anXPitch);
		addr += 16;
	}

	hbrBackground = ::CreateSolidBrush(colorConfig[1].m_crBkColor);
	rcPaint.left  = rcPaint.right;
	rcPaint.right = s_offset;
	::FillRect(m_hdcSample, &rcPaint, hbrBackground);
	::DeleteObject(hbrBackground);
	::SetTextColor(m_hdcSample, colorConfig[1].m_crFgColor);
	::SetBkColor(m_hdcSample, colorConfig[1].m_crBkColor);
	for (y = tm.tmHeight + 1; y < height; y += tm.tmHeight + 1) {
		::ExtTextOut(m_hdcSample, d_offset, y,
					 0, NULL, "AB CD ...", 9, anXPitch);
	}

	hbrBackground = ::CreateSolidBrush(colorConfig[2].m_crBkColor);
	rcPaint.left  = m_rctSample.right - m_rctSample.left - tm.tmAveCharWidth * 4;
	rcPaint.right = m_rctSample.right - m_rctSample.left;
	::FillRect(m_hdcSample, &rcPaint, hbrBackground);
	::DeleteObject(hbrBackground);
	::SetTextColor(m_hdcSample, colorConfig[2].m_crFgColor);
	::SetBkColor(m_hdcSample, colorConfig[2].m_crBkColor);
	for (y = tm.tmHeight + 1; y < height; y += tm.tmHeight + 1) {
		::ExtTextOut(m_hdcSample, s_offset, y,
					 0, NULL, "...", 3, anXPitch);
	}

	::SelectObject(m_hdcSample, hOrgFont);
	::DeleteObject(hFont);

	::InvalidateRect(m_hwndSample, NULL, FALSE);
	::UpdateWindow(m_hwndSample);
}

int CALLBACK
SampleView::SampleViewProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SampleView* _this = (SampleView*)::GetWindowLong(hWnd, GWL_USERDATA);
	assert(_this);

	if (uMsg == WM_PAINT) {
		PAINTSTRUCT ps;
		::BeginPaint(hWnd, &ps);
		::BitBlt(ps.hdc, ps.rcPaint.left, ps.rcPaint.top,
				 ps.rcPaint.right - ps.rcPaint.left,
				 ps.rcPaint.bottom - ps.rcPaint.top,
				 _this->m_hdcSample, ps.rcPaint.left, ps.rcPaint.top,
				 SRCCOPY);
		::EndPaint(hWnd, &ps);
		return 0;
	}

	return ::CallWindowProc(_this->m_pfnOrgWndProc, hWnd, uMsg, wParam, lParam);
}

static inline void
get_font_pt_from_pixel(HDC hDC, int pixel, LPSTR buf)
{
	float pt_size = (float)pixel * 72 / ::GetDeviceCaps(hDC, LOGPIXELSY);
	sprintf(buf, "%4.1f", pt_size);
}

FontConfigPage::FontConfigPage(Auto_Ptr<AppConfig>& pAppConfig)
	: ConfigPage(IDD_CONFIG_FONT, pAppConfig, "フォント"),
	  m_icFgColor(pAppConfig->m_pHVDrawInfo->getDC()),
	  m_icBkColor(pAppConfig->m_pHVDrawInfo->getDC()),
	  m_pSampleView(NULL)
{
}

BOOL
FontConfigPage::initDialog(HWND hDlg)
{
	ConfigPage::initDialog(hDlg);

	const HV_DrawInfo* pDrawInfo = m_pAppConfig->m_pHVDrawInfo.ptr();

	m_FontConfig = pDrawInfo->m_FontInfo.getFontConfig();

	prepareFontList(m_FontConfig.m_bProportional, m_FontConfig.m_pszFontFace);

	if (m_FontConfig.m_bProportional)
		::CheckDlgButton(hDlg, IDC_CONFIG_SHOW_PROPFONT, BST_CHECKED);

	prepareFontSize(m_FontConfig.m_fFontSize);

	HWND hwndList = ::GetDlgItem(hDlg, IDC_PART_LIST);

	for (int i = 0; i < 3; i++) {
		const TextColorInfo& tci = pDrawInfo->getTextColorInfo((TCI_INDEX)i);
		::SendMessage(hwndList, LB_INSERTSTRING,
					  i, (LPARAM)tci.getName().c_str());
		m_ColorConfig[i] = tci.getColorConfig();
	}
	const TextColorInfo& tci = m_pAppConfig->m_pHDDrawInfo->m_tciHeader;
	::SendMessage(hwndList, LB_INSERTSTRING,
				  3, (LPARAM)tci.getName().c_str());
	m_ColorConfig[3] = tci.getColorConfig();

	m_pSampleView = new SampleView(::GetDlgItem(hDlg, IDC_CONFIG_FONT_SAMPLE),
								   m_FontConfig, m_ColorConfig);

	if (isThemeActive()) {
		// テーマが有効 -> カラー選択ボタンを自分で描画するために
		//                 ボタンウィンドウをサブクラス化
		HWND hCtrl = ::GetDlgItem(hDlg, IDC_CONFIG_FONT_FGCOLOR);
		ColorSelButtonInfo* pInfo = new ColorSelButtonInfo;
		pInfo->m_pFCPage = this;
		pInfo->m_hTheme = NULL;
		pInfo->m_hDC = NULL;
		pInfo->m_hBitmap = NULL;
		pInfo->m_hbrColor = NULL;
		pInfo->m_iState = PBS_NORMAL;
		pInfo->m_bPressed = FALSE;
		pInfo->m_pfnOrgWndProc = (WNDPROC)::SetWindowLong(hCtrl, GWL_WNDPROC,
														  (LONG)colorSelBtnProc);
		::SendMessage(hCtrl, WM_USER_INIT_COLORSELBUTTON, 0, (LPARAM)pInfo);

		hCtrl = ::GetDlgItem(hDlg, IDC_CONFIG_FONT_BKCOLOR);
		pInfo = new ColorSelButtonInfo;
		pInfo->m_pFCPage = this;
		pInfo->m_hTheme = NULL;
		pInfo->m_hDC = NULL;
		pInfo->m_hBitmap = NULL;
		pInfo->m_hbrColor = NULL;
		pInfo->m_iState = PBS_NORMAL;
		pInfo->m_bPressed = FALSE;
		pInfo->m_pfnOrgWndProc = (WNDPROC)::SetWindowLong(hCtrl, GWL_WNDPROC,
														  (LONG)colorSelBtnProc);
		::SendMessage(hCtrl, WM_USER_INIT_COLORSELBUTTON, 0, (LPARAM)pInfo);
	}

	// フォント名コンボリストをサブクラス化
	m_cbiFontName.m_hwndCombo = ::GetDlgItem(hDlg, IDC_CONFIG_FONT_NAME);
	m_cbiFontName.m_pfnOrgWndProc
		= (WNDPROC)::GetWindowLong(m_cbiFontName.m_hwndCombo, GWL_WNDPROC);
	::SetWindowLong(m_cbiFontName.m_hwndCombo, GWL_USERDATA,
					(LONG)&m_cbiFontName);
	::SetWindowLong(m_cbiFontName.m_hwndCombo, GWL_WNDPROC,
					(LONG)comboBoxProc);
	// フォントサイズコンボボックスのエディットコントロールをサブクラス化
	m_cbiFontSize.m_hwndCombo = ::GetDlgItem(hDlg, IDC_CONFIG_FONT_SIZE);
	HWND hwndEdit = ::GetTopWindow(m_cbiFontSize.m_hwndCombo);
	m_cbiFontSize.m_pfnOrgWndProc = (WNDPROC)::GetWindowLong(hwndEdit, GWL_WNDPROC);
	::SetWindowLong(hwndEdit, GWL_USERDATA, (LONG)&m_cbiFontSize);
	::SetWindowLong(hwndEdit, GWL_WNDPROC, (LONG)comboBoxProc);

	return TRUE;
}

void
FontConfigPage::destroyDialog()
{
	m_pSampleView = NULL;
}

void
FontConfigPage::prepareFontList(bool bShowPropFonts,
								const char* pszFontFace)
{
	m_bShowPropFonts = bShowPropFonts;

	HWND hwndFontList = ::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_NAME);

	::SendMessage(hwndFontList, CB_RESETCONTENT, 0, 0);

	m_mapFontName.clear();

	LOGFONT logFont;
	logFont.lfCharSet = DEFAULT_CHARSET;
	logFont.lfFaceName[0] = '\0';
	logFont.lfPitchAndFamily = 0;
	::EnumFontFamiliesEx(m_pAppConfig->m_pHVDrawInfo->getDC(),
						 &logFont,
						 (FONTENUMPROC)FontConfigPage::enumFontProc,
						 (LPARAM)this,
						 0);

	if (pszFontFace) {
		int pos = ::SendMessage(hwndFontList,
								CB_FINDSTRINGEXACT,
								-1,
								(LPARAM)pszFontFace);
		if (pos != CB_ERR) {
			::SendMessage(hwndFontList, CB_SETCURSEL, pos, 0);
		} else {
			::SendDlgItemMessage(m_hwndDlg, IDC_CONFIG_FONT_SIZE,
								 CB_RESETCONTENT, 0, 0);
		}
	}
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
FontConfigPage::prepareFontSize(float fFontSize)
{
	HWND hwndFontList = ::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_NAME);
	int sel = ::SendMessage(hwndFontList, CB_GETCURSEL, 0, 0);
	if (sel == CB_ERR) return;

	LOGFONT logFont;
	logFont.lfCharSet = DEFAULT_CHARSET;
	logFont.lfPitchAndFamily = 0;

	::SendMessage(hwndFontList, CB_GETLBTEXT, sel,
				  (LPARAM)logFont.lfFaceName);

	HWND hwndFontSize = ::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_SIZE);
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
	} else {
		::EnumFontFamiliesEx(m_pAppConfig->m_pHVDrawInfo->getDC(),
							 &logFont,
							 (FONTENUMPROC)FontConfigPage::enumSpecificFontProc,
							 (LPARAM)this,
							 0);
	}

	if (fFontSize > 0.0) {
		char buf[8];
		sprintf(buf, "%4.1f", fFontSize);
		int pos = ::SendMessage(hwndFontSize, CB_FINDSTRINGEXACT,
								(WPARAM)-1, (LPARAM)buf);
		if (pos != CB_ERR) {
			::SendMessage(hwndFontSize, CB_SETCURSEL, pos, 0);
		} else {
			::SetWindowText(hwndFontSize, buf);
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
	char buf[8];
	get_font_pt_from_pixel(m_pAppConfig->m_pHVDrawInfo->getDC(), size, buf);

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
				HWND hwndFontSize = ::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_SIZE);
				char sizebuf[8];
				::GetWindowText(hwndFontSize, sizebuf, 7);
				float fontsize = (float)strtod(sizebuf, NULL);
				prepareFontSize(fontsize);
				if (fontsize > 0.0) m_FontConfig.m_fFontSize = fontsize;
				char fontface[LF_FACESIZE];
				::GetWindowText(::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_NAME),
								fontface, LF_FACESIZE);
				if (fontface[0]) {
					lstrcpy(m_FontConfig.m_pszFontFace, fontface);
					m_pSampleView->updateSample(m_FontConfig, m_ColorConfig);
				}
			}
			break;

		case IDC_CONFIG_FONT_SIZE:
			{
				char sizebuf[8] = { 0 };
				HWND hwndFontSize = ::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_SIZE);
				switch (HIWORD(wParam)) {
				case CBN_SELCHANGE:
					{
						int pos = ::SendMessage(hwndFontSize, CB_GETCURSEL, 0, 0);
						if (pos != CB_ERR) {
							::SendMessage(hwndFontSize, CB_GETLBTEXT,
										  pos, (LPARAM)sizebuf);
						}
					}
					break;

				case CBN_EDITCHANGE:
					::GetWindowText(hwndFontSize, sizebuf, 7);
					break;
				}
				if (sizebuf[0]) {
					char* ptr;
					float fontsize = (float)strtod(sizebuf, &ptr);
					if (ptr != sizebuf && fontsize > 0.0) {
						m_FontConfig.m_fFontSize = fontsize;
						m_pSampleView->updateSample(m_FontConfig, m_ColorConfig);
					}
				}
				::SendMessage(m_hwndParent, WM_USER_ENABLE_APPLY_BUTTON,
							  TRUE, 0);
			}
			break;

		case IDC_CONFIG_SHOW_PROPFONT:
			if (HIWORD(wParam) == BN_CLICKED) {
				char faceName[LF_FACESIZE];
				HWND hwndFontList = ::GetDlgItem(m_hwndDlg, IDC_CONFIG_FONT_NAME);
				int sel = ::SendMessage(hwndFontList, CB_GETCURSEL, 0, 0);
				if (sel != CB_ERR) {
					::SendMessage(hwndFontList, CB_GETLBTEXT,
								  sel, (LPARAM)faceName);
				}
				bool bShowPropFonts
					= ::SendDlgItemMessage(m_hwndDlg, IDC_CONFIG_SHOW_PROPFONT,
										   BM_GETCHECK, 0, 0) == BST_CHECKED;
				prepareFontList(bShowPropFonts, (sel != CB_ERR) ? faceName : NULL);
			}
			break;

		case IDC_PART_LIST:
			if (HIWORD(wParam) == LBN_SELCHANGE) {
				int pos = ::SendDlgItemMessage(m_hwndDlg, IDC_PART_LIST,
											   LB_GETCURSEL, 0, 0);
				if (pos != LB_ERR) {
					COLORREF crFg = m_ColorConfig[pos].m_crFgColor,
							 crBk = m_ColorConfig[pos].m_crBkColor;
					::SendDlgItemMessage(m_hwndDlg, IDC_CONFIG_FONT_FGCOLOR,
										 BM_SETIMAGE, IMAGE_BITMAP,
										 (LPARAM)m_icFgColor.setColor(crFg));
					::SendDlgItemMessage(m_hwndDlg, IDC_CONFIG_FONT_BKCOLOR,
										 BM_SETIMAGE, IMAGE_BITMAP,
										 (LPARAM)m_icBkColor.setColor(crBk));
				}
			}
			break;

		case IDC_CONFIG_FONT_BOLD:
			if (HIWORD(wParam) == BN_CLICKED) {
				::SendMessage(m_hwndParent, WM_USER_ENABLE_APPLY_BUTTON, TRUE, 0);
				m_FontConfig.m_bBoldFace
					= ::SendDlgItemMessage(m_hwndDlg, IDC_CONFIG_FONT_BOLD,
										   BM_GETCHECK, 0, 0) == BST_CHECKED;
				m_pSampleView->updateSample(m_FontConfig, m_ColorConfig);
			}
			break;

		case IDC_CONFIG_FONT_FGCOLOR:
			if (HIWORD(wParam) == BN_CLICKED) {
				int pos = ::SendDlgItemMessage(m_hwndDlg, IDC_PART_LIST,
											   LB_GETCURSEL, 0, 0);
				if (pos != LB_ERR) {
					COLORREF& cref = m_ColorConfig[pos].m_crFgColor;
					if (chooseColor(cref)) {
						::SendMessage(m_hwndParent, WM_USER_ENABLE_APPLY_BUTTON,
									  TRUE, 0);
						::SendDlgItemMessage(m_hwndDlg, IDC_CONFIG_FONT_FGCOLOR,
											 BM_SETIMAGE, IMAGE_BITMAP,
											 (LPARAM)m_icFgColor.setColor(cref));
						m_pSampleView->updateSample(m_FontConfig, m_ColorConfig);
					}
				}
			}
			break;

		case IDC_CONFIG_FONT_BKCOLOR:
			if (HIWORD(wParam) == BN_CLICKED) {
				int pos = ::SendDlgItemMessage(m_hwndDlg, IDC_PART_LIST,
											   LB_GETCURSEL, 0, 0);
				if (pos != LB_ERR) {
					COLORREF& cref = m_ColorConfig[pos].m_crBkColor;
					if (chooseColor(cref)) {
						::SendMessage(m_hwndParent, WM_USER_ENABLE_APPLY_BUTTON,
									  TRUE, 0);
						::SendDlgItemMessage(m_hwndDlg, IDC_CONFIG_FONT_BKCOLOR,
											 BM_SETIMAGE, IMAGE_BITMAP,
											 (LPARAM)m_icBkColor.setColor(cref));
						m_pSampleView->updateSample(m_FontConfig, m_ColorConfig);
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
				  (WPARAM)&m_FontConfig, (LPARAM)m_ColorConfig);

	return true;
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

BOOL
FontConfigPage::onSetImage(HWND hCtrl,
						   ColorSelButtonInfo* pInfo,
						   int iNewState,
						   HBITMAP hbmColor)
{
	ThemeWrapper& tw = GetTW();

	if (!pInfo->m_hTheme) {
		pInfo->m_hTheme = tw.OpenThemeData(hCtrl, L"Button");
		if (!pInfo->m_hTheme) return FALSE;
	}

	RECT rctCtrl;
	::GetWindowRect(hCtrl, &rctCtrl);
	rctCtrl.right -= rctCtrl.left;
	rctCtrl.bottom -= rctCtrl.top;
	rctCtrl.left = rctCtrl.top = 0;

	HDC hDC = ::GetDC(hCtrl);

	if (!pInfo->m_hDC) {
		pInfo->m_hDC = ::CreateCompatibleDC(hDC);
		if (!pInfo->m_hDC) {
			::ReleaseDC(hCtrl, hDC);
			return FALSE;
		}
		pInfo->m_hBitmap = ::CreateCompatibleBitmap(hDC, rctCtrl.right, rctCtrl.bottom);
		if (!pInfo->m_hBitmap) {
			::DeleteDC(pInfo->m_hDC);
			::ReleaseDC(hCtrl, hDC);
			return FALSE;
		}
		::SelectObject(pInfo->m_hDC, pInfo->m_hBitmap);
	}

	::ReleaseDC(hCtrl, hDC);

	if (hbmColor) {
		if (pInfo->m_hbrColor) ::DeleteObject(pInfo->m_hbrColor);
		HGDIOBJ hOrgBitmap = ::SelectObject(pInfo->m_hDC, hbmColor);
		pInfo->m_hbrColor = ::CreateSolidBrush(::GetPixel(pInfo->m_hDC, 0, 0));
		::SelectObject(pInfo->m_hDC, hOrgBitmap);
	}

	pInfo->m_iState = iNewState;

	HRESULT hr = tw.DrawThemeParentBackground(hCtrl, pInfo->m_hDC, &rctCtrl);
	if (hr != S_OK) return FALSE;

	hr = tw.DrawThemeBackground(pInfo->m_hTheme, pInfo->m_hDC,
								BP_PUSHBUTTON, pInfo->m_iState,
								&rctCtrl, NULL);
	if (hr != S_OK) return FALSE;

	RECT rctContent;
	hr = tw.GetThemeBackgroundContentRect(pInfo->m_hTheme, pInfo->m_hDC,
										  BP_PUSHBUTTON, pInfo->m_iState,
										  &rctCtrl, &rctContent);
	if (hr != S_OK) return FALSE;

	// content 領域の 70% を描画
	int w = (rctContent.right - rctContent.left) * 7 / 10,
		h = (rctContent.bottom - rctContent.top) * 7 / 10;
	rctContent.left   = (rctContent.left + rctContent.right - w) / 2;
	rctContent.right  = rctContent.left + w;
	rctContent.top    = (rctContent.top + rctContent.bottom - h) / 2;
	rctContent.bottom = rctContent.top + h;

	::FillRect(pInfo->m_hDC, &rctContent, pInfo->m_hbrColor);

	return TRUE;
}

LRESULT CALLBACK
FontConfigPage::colorSelBtnProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_USER_INIT_COLORSELBUTTON) {
		::SetWindowLong(hWnd, GWL_USERDATA, lParam);
		return 0;
	}

	ColorSelButtonInfo*
		pInfo = (ColorSelButtonInfo*)::GetWindowLong(hWnd, GWL_USERDATA);
	if (!pInfo) return ::DefWindowProc(hWnd, uMsg, wParam, lParam);

	ThemeWrapper& tw = GetTW();

	switch (uMsg) {
	case WM_PAINT:
		if (tw.IsThemeActive() && pInfo->m_hDC) {
			PAINTSTRUCT ps;
			::BeginPaint(hWnd, &ps);
			::BitBlt(ps.hdc,
					 ps.rcPaint.left, ps.rcPaint.top,
					 ps.rcPaint.right - ps.rcPaint.left,
					 ps.rcPaint.bottom - ps.rcPaint.top,
					 pInfo->m_hDC,
					 ps.rcPaint.left, ps.rcPaint.top,
					 SRCCOPY);
			::EndPaint(hWnd, &ps);
			return 0;
		}
		break;

	case WM_LBUTTONDOWN:
		if (tw.IsThemeActive() && pInfo->m_hDC) {
			pInfo->m_bPressed = TRUE;
			if (pInfo->m_iState != PBS_PRESSED)
				pInfo->m_pFCPage->onSetImage(hWnd, pInfo, PBS_PRESSED);
			::InvalidateRect(hWnd, NULL, FALSE);
			::UpdateWindow(hWnd);
			if (::GetCapture() != hWnd) ::SetCapture(hWnd);
			return 0;
		}
		break;

	case WM_LBUTTONUP:
		if (tw.IsThemeActive() && pInfo->m_hDC) {
			pInfo->m_bPressed = FALSE;
			if (::GetCapture() == hWnd) ::ReleaseCapture();
			RECT rctWnd;
			::GetWindowRect(hWnd, &rctWnd);
			rctWnd.right -= rctWnd.left;
			rctWnd.bottom -= rctWnd.top;
			rctWnd.left = rctWnd.top = 0;
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			if (::PtInRect(&rctWnd, pt)) {
				HWND hwndParent = ::GetParent(hWnd);
				::SendMessage(hwndParent, WM_COMMAND,
							  (BN_CLICKED << 16) | ::GetDlgCtrlID(hWnd),
							  0);
			}
		}
		break;

	case WM_CAPTURECHANGED:
		if (::GetCapture() != hWnd) {
			if (tw.IsThemeActive() && pInfo->m_hDC) {
				if (pInfo->m_iState != PBS_NORMAL)
					pInfo->m_pFCPage->onSetImage(hWnd, pInfo, PBS_NORMAL);
				::InvalidateRect(hWnd, NULL, FALSE);
				::UpdateWindow(hWnd);
				return 0;
			}
		}
		break;

	case WM_MOUSEMOVE:
		if (tw.IsThemeActive() && pInfo->m_hDC) {
			RECT rctWnd;
			::GetWindowRect(hWnd, &rctWnd);
			rctWnd.right -= rctWnd.left;
			rctWnd.bottom -= rctWnd.top;
			rctWnd.left = rctWnd.top = 0;
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			int iState;
			if (::PtInRect(&rctWnd, pt)) {
				if (pInfo->m_bPressed) {
					iState = PBS_PRESSED;
				} else {
					if (::GetCapture() != hWnd) ::SetCapture(hWnd);
					iState = PBS_HOT;
				}
			} else {
				if (pInfo->m_bPressed) {
					iState = PBS_HOT;
				} else {
					if (::GetCapture() == hWnd) {
						::ReleaseCapture();
						return 0;
					} else {
						break;
					}
				}
			}
			if (pInfo->m_iState != iState)
				pInfo->m_pFCPage->onSetImage(hWnd, pInfo, iState);
			::InvalidateRect(hWnd, NULL, FALSE);
			::UpdateWindow(hWnd);
			return 0;
		}
		break;

	case BM_SETIMAGE:
		if (tw.IsThemeActive()) {
			pInfo->m_pFCPage->onSetImage(hWnd, pInfo, PBS_NORMAL, (HBITMAP)lParam);
			::InvalidateRect(hWnd, NULL, FALSE);
			::UpdateWindow(hWnd);
			return 0;
		}
		break;

	case WM_THEMECHANGED:
		tw.CloseThemeData(pInfo->m_hTheme);
		pInfo->m_hTheme = NULL;
		if (tw.IsThemeActive()) {
			pInfo->m_pFCPage->onSetImage(hWnd, pInfo, pInfo->m_iState);
			::InvalidateRect(hWnd, NULL, FALSE);
			::UpdateWindow(hWnd);
			return 0;
		}
		break;

	case WM_DESTROY:
		{
			WNDPROC pfnOrgWndProc = pInfo->m_pfnOrgWndProc;
			if (pInfo->m_hDC) {
				::DeleteObject(pInfo->m_hbrColor);
				::DeleteObject(pInfo->m_hBitmap);
				::DeleteDC(pInfo->m_hDC);
			}
			if (pInfo->m_hTheme) tw.CloseThemeData(pInfo->m_hTheme);
			delete pInfo;
			::SetWindowLong(hWnd, GWL_WNDPROC, (LONG)pfnOrgWndProc);
			return ::CallWindowProc(pfnOrgWndProc, hWnd, uMsg, wParam, lParam);
		}

	default:
		break;
	}

	return ::CallWindowProc(pInfo->m_pfnOrgWndProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK
FontConfigPage::comboBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ComboBoxInfo* pcbInfo = (ComboBoxInfo*)::GetWindowLong(hWnd, GWL_USERDATA);
	if (uMsg == WM_KEYDOWN && wParam == VK_DOWN &&
		!::SendMessage(pcbInfo->m_hwndCombo, CB_GETDROPPEDSTATE, 0, 0)) {
		::SendMessage(pcbInfo->m_hwndCombo, CB_SHOWDROPDOWN, TRUE, 0);
		int cursel = ::SendMessage(pcbInfo->m_hwndCombo, CB_GETCURSEL, 0, 0);
		if (cursel == CB_ERR) cursel = 0;
		::SendMessage(pcbInfo->m_hwndCombo, CB_SETCURSEL, cursel, 0);
		return 0;
	}
	return ::CallWindowProc(pcbInfo->m_pfnOrgWndProc,
							hWnd, uMsg, wParam, lParam);
}

CursorConfigPage::CursorConfigPage(Auto_Ptr<AppConfig>& pAppConfig)
	: ConfigPage(IDD_CONFIG_CURSOR, pAppConfig, "カーソル"),
	  m_ScrollConfig(pAppConfig->m_pHVDrawInfo->m_ScrollConfig)
{
//	m_ScrollConfig = m_pAppConfig->m_ScrollConfig;
}

BOOL
CursorConfigPage::initDialog(HWND hDlg)
{
	ConfigPage::initDialog(hDlg);

	switch (m_ScrollConfig.m_caretMove) {
	case CARET_STATIC:
		::SendDlgItemMessage(hDlg, IDC_CONFIG_CARET_STATIC,
							 BM_SETCHECK, BST_CHECKED, 0);
		break;

	case CARET_ENSURE_VISIBLE:
		::SendDlgItemMessage(hDlg, IDC_CONFIG_CARET_ENSURE_VISIBLE,
							 BM_SETCHECK, BST_CHECKED, 0);
		break;

	case CARET_SCROLL:
		::SendDlgItemMessage(hDlg, IDC_CONFIG_CARET_SCROLL,
							 BM_SETCHECK, BST_CHECKED, 0);
		break;
	}

	if (m_ScrollConfig.m_wheelScroll == WHEEL_AS_ARROW_KEYS)
		::SendDlgItemMessage(hDlg, IDC_CONFIG_WHEEL_AS_ARROW_KEYS,
							 BM_SETCHECK, BST_CHECKED, 0);
	else
		::SendDlgItemMessage(hDlg, IDC_CONFIG_WHEEL_AS_SCROLL_BAR,
							 BM_SETCHECK, BST_CHECKED, 0);

	return TRUE;
}

BOOL
CursorConfigPage::dialogProcMain(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_COMMAND:
		if (HIWORD(wParam) != BN_CLICKED) break;
		switch (LOWORD(wParam)) {
		case IDC_CONFIG_CARET_STATIC:
			m_ScrollConfig.m_caretMove = CARET_STATIC;
			break;
		case IDC_CONFIG_CARET_ENSURE_VISIBLE:
			m_ScrollConfig.m_caretMove = CARET_ENSURE_VISIBLE;
			break;
		case IDC_CONFIG_CARET_SCROLL:
			m_ScrollConfig.m_caretMove = CARET_SCROLL;
			break;
		case IDC_CONFIG_WHEEL_AS_ARROW_KEYS:
			m_ScrollConfig.m_wheelScroll = WHEEL_AS_ARROW_KEYS;
			break;
		case IDC_CONFIG_WHEEL_AS_SCROLL_BAR:
			m_ScrollConfig.m_wheelScroll = WHEEL_AS_SCROLL_BAR;
			break;
		default:
			break;
		}
		::SendMessage(m_hwndParent, WM_USER_ENABLE_APPLY_BUTTON, TRUE, 0);
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
//	m_pAppConfig->m_ScrollConfig = m_ScrollConfig;
	::SendMessage(m_hwndParent, WM_USER_SET_SCROLL_CONFIG,
				  0, (LPARAM)&m_ScrollConfig);

	return true;
}


void
LoadConfig(HWND hWnd, Auto_Ptr<AppConfig>& pAppConfig)
{
	assert(!pAppConfig.ptr());

	HKEY hKeyRoot = NULL;
	if (::RegOpenKeyEx(HKEY_CURRENT_USER,
						REG_ROOT,
						0, KEY_ALL_ACCESS,
						&hKeyRoot) != ERROR_SUCCESS) {
		pAppConfig = new AppConfig();
		HDC hDC = ::GetDC(hWnd);
		pAppConfig->m_pHVDrawInfo = new HV_DrawInfo(hDC, DEFAULT_FONT_SIZE,
													"FixedSys", false,
													DEFAULT_FG_COLOR_ADDRESS, DEFAULT_BK_COLOR_ADDRESS,
													DEFAULT_FG_COLOR_DATA, DEFAULT_BK_COLOR_DATA,
													DEFAULT_FG_COLOR_STRING, DEFAULT_BK_COLOR_STRING,
													CARET_STATIC, WHEEL_AS_ARROW_KEYS);

		pAppConfig->m_pHDDrawInfo = new HD_DrawInfo(hDC, DEFAULT_FONT_SIZE,
													"FixedSys", false,
													DEFAULT_FG_COLOR_HEADER, DEFAULT_BK_COLOR_HEADER);
		return;
	}

	DWORD dwType, dwSize;

	dwType = REG_BINARY;
	dwSize = sizeof(float);
	float fontsize;
	if (::RegQueryValueEx(hKeyRoot, RK_FONTSIZE, 0,
						  &dwType, (BYTE*)&fontsize, &dwSize) != ERROR_SUCCESS) {
		fontsize = DEFAULT_FONT_SIZE;
	}

	dwType = REG_SZ;
	dwSize = LF_FACESIZE;
	char fontface[LF_FACESIZE];
	if (::RegQueryValueEx(hKeyRoot, RK_FONTNAME, 0,
						  &dwType, (BYTE*)&fontface, &dwSize) != ERROR_SUCCESS) {
		lstrcpy(fontface, "FixedSys");
	}

	dwType = REG_DWORD;
	dwSize = sizeof(DWORD);
	DWORD dwBoldFace;
	if (::RegQueryValueEx(hKeyRoot, RK_IS_BOLD, 0,
						  &dwType, (BYTE*)&dwBoldFace, &dwSize) != ERROR_SUCCESS) {
		dwBoldFace = 0;
	}

	COLORREF crFgHeader, crBkHeader;
	if (::RegQueryValueEx(hKeyRoot, RK_HEADER_FGC, 0,
						  &dwType, (BYTE*)&crFgHeader, &dwSize) != ERROR_SUCCESS) {
		crFgHeader = DEFAULT_FG_COLOR_HEADER;
	}
	if (::RegQueryValueEx(hKeyRoot, RK_HEADER_BKC, 0,
						  &dwType, (BYTE*)&crBkHeader, &dwSize) != ERROR_SUCCESS) {
		crBkHeader = DEFAULT_BK_COLOR_HEADER;
	}
	COLORREF crFgAddress, crBkAddress;
	if (::RegQueryValueEx(hKeyRoot, RK_ADDRESS_FGC, 0,
						  &dwType, (BYTE*)&crFgAddress, &dwSize) != ERROR_SUCCESS) {
		crFgAddress = DEFAULT_FG_COLOR_ADDRESS;
	}
	if (::RegQueryValueEx(hKeyRoot, RK_ADDRESS_BKC, 0,
						  &dwType, (BYTE*)&crBkAddress, &dwSize) != ERROR_SUCCESS) {
		crBkAddress = DEFAULT_BK_COLOR_ADDRESS;
	}
	COLORREF crFgData, crBkData;
	if (::RegQueryValueEx(hKeyRoot, RK_DATA_FGC, 0,
						  &dwType, (BYTE*)&crFgData, &dwSize) != ERROR_SUCCESS) {
		crFgData = DEFAULT_FG_COLOR_DATA;
	}
	if (::RegQueryValueEx(hKeyRoot, RK_DATA_BKC, 0,
						  &dwType, (BYTE*)&crBkData, &dwSize) != ERROR_SUCCESS) {
		crBkData = DEFAULT_BK_COLOR_DATA;
	}
	COLORREF crFgString, crBkString;
	if (::RegQueryValueEx(hKeyRoot, RK_STRING_FGC, 0,
						  &dwType, (BYTE*)&crFgString, &dwSize) != ERROR_SUCCESS) {
		crFgString = DEFAULT_FG_COLOR_STRING;
	}
	if (::RegQueryValueEx(hKeyRoot, RK_STRING_BKC, 0,
						  &dwType, (BYTE*)&crBkString, &dwSize) != ERROR_SUCCESS) {
		crBkString = DEFAULT_BK_COLOR_STRING;
	}

	DWORD dwCaretMove;
	if (::RegQueryValueEx(hKeyRoot, RK_CARET_MOVE, 0,
						  &dwType, (BYTE*)&dwCaretMove, &dwSize) != ERROR_SUCCESS) {
		dwCaretMove = CARET_STATIC;
	}
	DWORD dwWheelScroll;
	if (::RegQueryValueEx(hKeyRoot, RK_WHEEL_SCROLL, 0,
						  &dwType, (BYTE*)&dwWheelScroll, &dwSize) != ERROR_SUCCESS) {
		dwWheelScroll = WHEEL_AS_ARROW_KEYS;
	}

	::RegCloseKey(hKeyRoot);

	pAppConfig = new AppConfig();
	HDC hDC = ::GetDC(hWnd);
	pAppConfig->m_pHVDrawInfo = new HV_DrawInfo(hDC, fontsize,
												fontface, dwBoldFace != 0,
												crFgAddress, crBkAddress,
												crFgData, crBkData,
												crFgString, crBkString,
												(CARET_MOVE)dwCaretMove,
												(WHEEL_SCROLL)dwWheelScroll);

	pAppConfig->m_pHDDrawInfo = new HD_DrawInfo(hDC, fontsize,
												fontface, dwBoldFace != 0,
												crFgHeader, crBkHeader);
}

void
SaveConfig(const Auto_Ptr<AppConfig>& pAppConfig)
{
	HKEY hKeyRoot;
	if (::RegCreateKeyEx(HKEY_CURRENT_USER,
						 REG_ROOT,
						 0, NULL,
						 REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
						 NULL,
						 &hKeyRoot, NULL) != ERROR_SUCCESS) {
		assert(0);
		return;
	}

	const HV_DrawInfo* pDrawInfo = pAppConfig->m_pHVDrawInfo.ptr();

	DWORD dwType, dwSize;

	dwType = REG_BINARY;
	dwSize = sizeof(float);
	float fontsize = pDrawInfo->m_FontInfo.getFontSize();
	::RegSetValueEx(hKeyRoot, RK_FONTSIZE, 0,
					dwType, (BYTE*)&fontsize, dwSize);

	dwType = REG_SZ;
	dwSize = lstrlen(pDrawInfo->m_FontInfo.getFaceName());
	::RegSetValueEx(hKeyRoot, RK_FONTNAME, 0,
					dwType, (BYTE*)pDrawInfo->m_FontInfo.getFaceName(), dwSize);

	dwType = REG_DWORD;
	dwSize = sizeof(DWORD);
	DWORD dwBoldFace = pDrawInfo->m_FontInfo.isBoldFace();
	::RegSetValueEx(hKeyRoot, RK_IS_BOLD, 0,
					dwType, (BYTE*)&dwBoldFace, dwSize);

	COLORREF crFgAddress = pDrawInfo->m_tciAddress.getFgColor(),
			 crBkAddress = pDrawInfo->m_tciAddress.getBkColor();
	::RegSetValueEx(hKeyRoot, RK_ADDRESS_FGC, 0,
					dwType, (BYTE*)&crFgAddress, dwSize);
	::RegSetValueEx(hKeyRoot, RK_ADDRESS_BKC, 0,
					dwType, (BYTE*)&crBkAddress, dwSize);

	COLORREF crFgData = pDrawInfo->m_tciData.getFgColor(),
			 crBkData = pDrawInfo->m_tciData.getBkColor();
	::RegSetValueEx(hKeyRoot, RK_DATA_FGC, 0,
					dwType, (BYTE*)&crFgData, dwSize);
	::RegSetValueEx(hKeyRoot, RK_DATA_BKC, 0,
					dwType, (BYTE*)&crBkData, dwSize);

	COLORREF crFgString = pDrawInfo->m_tciString.getFgColor(),
			 crBkString = pDrawInfo->m_tciString.getBkColor();
	::RegSetValueEx(hKeyRoot, RK_STRING_FGC, 0,
					dwType, (BYTE*)&crFgString, dwSize);
	::RegSetValueEx(hKeyRoot, RK_STRING_BKC, 0,
					dwType, (BYTE*)&crBkString, dwSize);

	DWORD dwCaretMove = pDrawInfo->m_ScrollConfig.m_caretMove;
	::RegSetValueEx(hKeyRoot, RK_CARET_MOVE, 0,
					dwType, (BYTE*)&dwCaretMove, dwSize);

	DWORD dwWheelScroll = pDrawInfo->m_ScrollConfig.m_wheelScroll;
	::RegSetValueEx(hKeyRoot, RK_WHEEL_SCROLL, 0,
					dwType, (BYTE*)&dwWheelScroll, dwSize);

	const HD_DrawInfo* pHDDrawInfo = pAppConfig->m_pHDDrawInfo.ptr();

	COLORREF crFgHeader = pHDDrawInfo->m_tciHeader.getFgColor(),
			 crBkHeader = pHDDrawInfo->m_tciHeader.getBkColor();
	::RegSetValueEx(hKeyRoot, RK_HEADER_FGC, 0,
					dwType, (BYTE*)&crFgHeader, dwSize);
	::RegSetValueEx(hKeyRoot, RK_HEADER_BKC, 0,
					dwType, (BYTE*)&crBkHeader, dwSize);

	::RegCloseKey(hKeyRoot);
}

