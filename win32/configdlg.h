// $Id$

#ifndef CONFIGDLG_H_INC
#define CONFIGDLG_H_INC

#include "drawinfo.h"
#include <exception>
using std::exception;

#define CONFIG_DIALOG_PAGE_NUM  2

class CreateDialogError : public exception {};

class ConfigDialog {
public:
	ConfigDialog(HWND hwndParent, DrawInfo* pDrawInfo);
	virtual ~ConfigDialog();

	HWND getHWND() const { return m_hwndDlg; }
	HWND getParentHWND() const { return m_hwndParent; }

	virtual void applyChanges() = 0;

protected:
	HWND m_hwndParent, m_hwndDlg;
	HINSTANCE m_hInstance;
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
	ConfigPage(HWND hwndParent, DrawInfo* pDrawInfo, int nPageTemplateID);

	bool create(const RECT& rctPage);
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

	void destroyDialog();
};

class FontConfigPage : public ConfigPage {
public:
	FontConfigPage(HWND hwndParent, DrawInfo* pDrawInfo);

	void applyChanges();

protected:
	BOOL initDialog(HWND hDlg);
	BOOL dialogProcMain(UINT, WPARAM, LPARAM);

	void selectFontPageList(int index);
};

class CursorConfigPage : public ConfigPage {
public:
	CursorConfigPage(HWND hwndParent, DrawInfo* pDrawInfo);

	void applyChanges();

protected:
	BOOL initDialog(HWND hDlg);
	BOOL dialogProcMain(UINT, WPARAM, LPARAM);
};

class ConfigMainDlg : public ConfigDialog {
public:
	ConfigMainDlg(HWND hwndParent, DrawInfo* pDrawInfo);

	bool doModal();

	void applyChanges();

protected:
	RECT m_rctPage;
	ConfigPage* m_pConfigPages[CONFIG_DIALOG_PAGE_NUM];
	LPCSTR m_pszTabText[CONFIG_DIALOG_PAGE_NUM];

	BOOL initDialog(HWND hDlg);
	void destroyDialog();

	BOOL dialogProcMain(UINT, WPARAM, LPARAM);

	bool createPage(int i);
};

#endif
