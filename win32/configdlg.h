// $Id$

#ifndef CONFIGDLG_H_INC
#define CONFIGDLG_H_INC

#include "drawinfo.h"
#include "messages.h"
#include <string>
#include <exception>
#include <map>
using std::string;
using std::exception;
using std::map;
using std::make_pair;

#define CONFIG_DIALOG_PAGE_NUM  2

class CreateDialogError : public exception {};

class ConfigDialog {
public:
	ConfigDialog(DrawInfo* pDrawInfo);
	virtual ~ConfigDialog();

	HWND getHWND() const { return m_hwndDlg; }
	HWND getParentHWND() const { return m_hwndParent; }

	virtual bool applyChanges() = 0;

protected:
	HWND m_hwndParent, m_hwndDlg;
	DrawInfo* m_pDrawInfo;

	ConfigDialog(const ConfigDialog&);
	ConfigDialog& operator=(const ConfigDialog&);

	virtual BOOL initDialog(HWND hDlg) = 0;
	virtual void destroyDialog() = 0;

	virtual BOOL dialogProcMain(UINT, WPARAM, LPARAM) = 0;

	static BOOL CALLBACK configDialogProc(HWND, UINT, WPARAM, LPARAM);
};

class ConfigPage : public ConfigDialog {
public:
	ConfigPage(DrawInfo* pDrawInfo,
			   int nPageTemplateID, const char* pszTabText);

	const char* getTabText() const { return m_strTabText.c_str(); }

	bool create(HWND hwndParent, const RECT& rctPage);
	void close()
	{
		::SendMessage(m_hwndDlg, WM_CLOSE, 0, 0);
		m_bShown = false;
	}
	void show(BOOL bShow)
	{
		::ShowWindow(m_hwndDlg, bShow ? SW_SHOW : SW_HIDE);
	}

	bool isShown() const
	{
		return m_bShown;
	}

protected:
	int m_nPageTemplateID;
	string m_strTabText;
	bool m_bShown;

	void destroyDialog();
};

struct ColorConfig {
	COLORREF m_crFgColor;
	COLORREF m_crBkColor;
};

struct FontConfig {
	char  m_pszFontFace[LF_FACESIZE];
	float m_fFontSize;
	bool  m_bBoldFace;
	ColorConfig m_ColorConfig[4];
};

#define CC_HEADER  0
#define CC_ADDRESS 1
#define CC_DATA    2
#define CC_STRING  3

struct Icon {
	HICON   m_hIcon;
	HBITMAP m_hbmColor;
	HBITMAP m_hbmMask;
};

class FontConfigPage : public ConfigPage {
public:
	FontConfigPage(DrawInfo* pDrawInfo);

	bool applyChanges();

protected:
	bool m_bShowPropFonts;
	map<string, DWORD> m_mapFontName;
	Icon m_icFgColor, m_icBkColor;
	FontConfig m_FontConfig;

	BOOL initDialog(HWND hDlg);
	void destroyDialog();
	BOOL dialogProcMain(UINT, WPARAM, LPARAM);

	bool initFontConfig();
	void prepareFontList(bool bShowPropFonts);
	void addFont(const LOGFONT& logFont, DWORD fontType);
	void prepareFontSize();
	void addSize(int size);

	void selectFontPageList(int index);
	bool chooseColor(COLORREF& cref);

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
	CursorConfigPage(DrawInfo* pDrawInfo);

	bool applyChanges();

protected:
	BOOL initDialog(HWND hDlg);
	BOOL dialogProcMain(UINT, WPARAM, LPARAM);
};

class ConfigMainDlg : public ConfigDialog {
public:
	ConfigMainDlg(DrawInfo* pDrawInfo);
	~ConfigMainDlg();

	bool doModal(HWND hwndParent);

	bool applyChanges();

protected:
	RECT m_rctPage;
	ConfigPage* m_pConfigPages[CONFIG_DIALOG_PAGE_NUM];

	BOOL initDialog(HWND hDlg);
	void destroyDialog();

	int getCurrentPage() const;

	BOOL dialogProcMain(UINT, WPARAM, LPARAM);

	bool createPage(int i);
};

#endif
