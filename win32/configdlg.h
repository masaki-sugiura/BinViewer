// $Id$

#ifndef CONFIGDLG_H_INC
#define CONFIGDLG_H_INC

#include "MainWindow.h"
#include "dialog.h"
#include "messages.h"
#include "auto_ptr.h"
#include <string>
#include <exception>
#include <map>
using std::string;
using std::exception;
using std::map;
using std::make_pair;

#define COLOR_BLACK      RGB(0, 0, 0)
#define COLOR_GRAY       RGB(128, 128, 128)
#define COLOR_LIGHTGRAY  RGB(192, 192, 192)
#define COLOR_WHITE      RGB(255, 255, 255)
#define COLOR_YELLOW     RGB(255, 255, 0)

#define REG_ROOT        "Software\\SugiApp\\BinViewer"
#define RK_FONTNAME     "FontFace"
#define RK_FONTSIZE     "FontSize"
#define RK_IS_BOLD      "IsBoldFont"
#define RK_HEADER_FGC   "Header_FgColor"
#define RK_HEADER_BKC   "Header_BkColor"
#define RK_ADDRESS_FGC  "Address_FgColor"
#define RK_ADDRESS_BKC  "Address_BkColor"
#define RK_DATA_FGC     "Data_FgColor"
#define RK_DATA_BKC     "Data_BkColor"
#define RK_STRING_FGC   "String_FgColor"
#define RK_STRING_BKC   "String_BkColor"
#define RK_CARET_MOVE   "Caret_Move"
#define RK_WHEEL_SCROLL "Wheel_Scroll"

#define DEFAULT_FONT_SIZE  12
#define DEFAULT_FG_COLOR_ADDRESS COLOR_WHITE
#define DEFAULT_BK_COLOR_ADDRESS COLOR_GRAY
#define DEFAULT_FG_COLOR_DATA    COLOR_BLACK
#define DEFAULT_BK_COLOR_DATA    COLOR_WHITE
#define DEFAULT_FG_COLOR_STRING  COLOR_BLACK
#define DEFAULT_BK_COLOR_STRING  COLOR_LIGHTGRAY
#define DEFAULT_FG_COLOR_HEADER  COLOR_BLACK
#define DEFAULT_BK_COLOR_HEADER  COLOR_YELLOW

//#define CONFIG_DIALOG_PAGE_NUM  2
#define CONFIG_DIALOG_PAGE_NUM  1

class CreateDialogError : public exception {};

class ConfigDialog : public Dialog {
public:
	ConfigDialog(int nDialogID, Auto_Ptr<AppConfig>& pAppConfig);
	~ConfigDialog();

	virtual bool applyChanges() = 0;

protected:
	Auto_Ptr<AppConfig>& m_pAppConfig;
};

class ConfigPage : public ConfigDialog {
public:
	ConfigPage(int nDialogID, Auto_Ptr<AppConfig>& pAppConfig,
			   const char* pszTabText);

	const char* getTabText() const { return m_strTabText.c_str(); }

	void show(BOOL bShow)
	{
		m_bShown = (bShow != 0);
		::ShowWindow(m_hwndDlg, bShow ? SW_SHOW : SW_HIDE);
	}

	bool isShown() const
	{
		return m_bShown;
	}

protected:
	string m_strTabText;
	bool m_bShown;

	BOOL initDialog(HWND hDlg);
	void destroyDialog();
};

struct Icon {
	HDC     m_hDC;
	HBITMAP m_hbmColor;
	RECT    m_rctIcon;

	Icon(HDC hDC);
	~Icon();

	HBITMAP setColor(COLORREF cref);
};

class SampleView {
public:
	SampleView(HWND hwndSample, FontConfig& fontConfig, ColorConfig* colorConfig);
	~SampleView();

	void updateSample(FontConfig& fontConfig, ColorConfig* colorConfig);

private:
	HWND m_hwndSample;
	RECT m_rctSample;
	HDC  m_hdcSample;
	HBITMAP m_hbmSample;
	WNDPROC m_pfnOrgWndProc;

	static int CALLBACK SampleViewProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

class FontConfigPage : public ConfigPage {
public:
	FontConfigPage(Auto_Ptr<AppConfig>& pAppConfig);

	bool applyChanges();

protected:
	bool m_bShowPropFonts;
	map<string, DWORD> m_mapFontName;
	Icon m_icFgColor, m_icBkColor;
	FontConfig m_FontConfig;
	ColorConfig m_ColorConfig[4];
	Auto_Ptr<SampleView> m_pSampleView;

	BOOL initDialog(HWND hDlg);
	void destroyDialog();
	BOOL dialogProcMain(UINT, WPARAM, LPARAM);

	void prepareFontList(bool bShowPropFonts, const char* pszFontFace);
	void addFont(const LOGFONT& logFont, DWORD fontType);
	void prepareFontSize(float fFontSize);
	void addSize(int size);

	bool chooseColor(COLORREF& cref);

	struct ColorSelButtonInfo {
		FontConfigPage* m_pFCPage;
		WNDPROC m_pfnOrgWndProc;
		HTHEME  m_hTheme;
		HDC     m_hDC;
		HBITMAP m_hBitmap;
		HBRUSH  m_hbrColor;
		int m_iState;
		BOOL m_bPressed;
	};

	struct ComboBoxInfo {
		HWND m_hwndCombo;
		WNDPROC m_pfnOrgWndProc;
	};
	ComboBoxInfo m_cbiFontName, m_cbiFontSize;

	BOOL onSetImage(HWND hWnd, ColorSelButtonInfo* pInfo, int iNewState, HBITMAP hbmColor = NULL);

	static LRESULT CALLBACK colorSelBtnProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK comboBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static int CALLBACK enumFontProc(ENUMLOGFONTEX *lpelfe,
									 NEWTEXTMETRICEX *lpntme,
									 DWORD FontType,
									 LPARAM lParam);
	static int CALLBACK enumSpecificFontProc(ENUMLOGFONTEX* lpelfe,
											 NEWTEXTMETRICEX* lpntme,
											 DWORD FontType,
											 LPARAM lParam);
};

class CursorConfigPage : public ConfigPage {
public:
	CursorConfigPage(Auto_Ptr<AppConfig>& pAppConfig);

	bool applyChanges();

protected:
	ScrollConfig m_ScrollConfig;

	BOOL initDialog(HWND hDlg);
	BOOL dialogProcMain(UINT, WPARAM, LPARAM);
};

class ConfigMainDlg : public ConfigDialog {
public:
	ConfigMainDlg(Auto_Ptr<AppConfig>& pAppConfig);
	~ConfigMainDlg();

	bool applyChanges();

protected:
	BOOL m_bDirty;
	RECT m_rctPage;
	ConfigPage* m_pConfigPages[CONFIG_DIALOG_PAGE_NUM];

	BOOL initDialog(HWND hDlg);
	void destroyDialog();

	int getCurrentPage() const;

	BOOL dialogProcMain(UINT, WPARAM, LPARAM);

	bool createPage(int i);
};

void LoadConfig(HWND hWnd, Auto_Ptr<AppConfig>& pAppConfig);
void SaveConfig(const Auto_Ptr<AppConfig>& pAppConfig);

#endif
