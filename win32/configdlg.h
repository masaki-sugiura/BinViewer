// $Id$

#ifndef CONFIGDLG_H_INC
#define CONFIGDLG_H_INC

#include "drawinfo.h"

class ConfigDlg {
public:
	static bool doModal(HWND hwndParent, DrawInfo* pDrawInfo);

private:
	static HINSTANCE m_hInstance;
	static HWND m_hwndMain, m_hwndFontPage;
	static DrawInfo* m_pDrawInfo;

	static bool initMainDlg(HWND hDlg);
	static bool initFontPage(HWND hDlg);
	static void selectFontPageList(HWND hDlg, int index);
	static void applyChanges();

	static BOOL CALLBACK ConfigDlgProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK FontPageProc(HWND, UINT, WPARAM, LPARAM);
};

#endif
