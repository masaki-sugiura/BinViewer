// $Id$

#ifndef CONFIGDLG_H_INC
#define CONFIGDLG_H_INC

#include "drawinfo.h"
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

	virtual void applyChanges() = 0;

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
	}
	void show(BOOL bShow)
	{
		::ShowWindow(m_hwndDlg, bShow ? SW_SHOW : SW_HIDE);
	}

protected:
	int m_nPageTemplateID;
	string m_strTabText;

	void destroyDialog();
};

class FontConfigPage : public ConfigPage {
public:
	FontConfigPage(DrawInfo* pDrawInfo);

	void applyChanges();

protected:
	bool m_bShowPropFonts;
	map<string, int> m_mapFontName;

	BOOL initDialog(HWND hDlg);
	BOOL dialogProcMain(UINT, WPARAM, LPARAM);

	void prepareFontList();
	void addFont(const LOGFONT& logFont);
	void prepareFontSize();

	void selectFontPageList(int index);

	static int CALLBACK enumFontProc(ENUMLOGFONTEX *lpelfe,
									 NEWTEXTMETRICEX *lpntme,
									 DWORD FontType,
									 LPARAM lParam);
};

class CursorConfigPage : public ConfigPage {
public:
	CursorConfigPage(DrawInfo* pDrawInfo);

	void applyChanges();

protected:
	BOOL initDialog(HWND hDlg);
	BOOL dialogProcMain(UINT, WPARAM, LPARAM);
};

class ConfigMainDlg : public ConfigDialog {
public:
	ConfigMainDlg(DrawInfo* pDrawInfo);
	~ConfigMainDlg();

	bool doModal(HWND hwndParent);

	void applyChanges();

protected:
	RECT m_rctPage;
	ConfigPage* m_pConfigPages[CONFIG_DIALOG_PAGE_NUM];

	BOOL initDialog(HWND hDlg);
	void destroyDialog();

	BOOL dialogProcMain(UINT, WPARAM, LPARAM);

	bool createPage(int i);
};

#endif
