// $Id$

#ifndef DIALOG_H_INC
#define DIALOG_H_INC

#include <windows.h>

#include <UxTheme.h>

#include <map>
using std::map;

typedef map<HWND, class Dialog*> DialogMap;

// IsThemeActive()
typedef BOOL (STDAPICALLTYPE *PFN_ITA)();
// EnableThemeDialogTexture()
typedef HRESULT (STDAPICALLTYPE *PFN_ETDT)(HWND hwnd, DWORD dwFlags);
// OpenThemeData()
typedef HTHEME  (STDAPICALLTYPE *PFN_OTD)(HWND hWnd, LPCWSTR pszwClassName);
// CloseThemeData()
typedef HRESULT (STDAPICALLTYPE *PFN_CTD)(HTHEME hTheme);
// DrawThemeBackground()
typedef HRESULT (STDAPICALLTYPE *PFN_DTB)(HTHEME hTheme, HDC hDC,
										  int iPart, int iState,
										  const RECT* pRect,
										  const RECT* pClipRect);
// DrawThemeParentBackground()
typedef HRESULT (STDAPICALLTYPE *PFN_DTPB)(HWND hWnd, HDC hDC, const RECT* pRect);
// GetThemeBackgroundContentRect
typedef HRESULT (STDAPICALLTYPE *PFN_GTBCR)(HTHEME hTheme, HDC hDC,
											int iPart, int iState,
											const RECT* pRect,
											RECT* pContentRect);

#include "lock.h"

class Dialog {
public:
	Dialog(int idd);
	virtual ~Dialog();

	HWND getHWND() const { return m_hwndDlg; }
	HWND getParentHWND() const { return m_hwndParent; }

	HWND create(HWND hwndParent);
	void close();

	int doModal(HWND hwndParent);

	static BOOL initializeTheme();
	static void uninitializeTheme();
	static BOOL isDialogMessage(MSG* msg);

protected:
	int  m_nDialogID;
	HWND m_hwndDlg, m_hwndParent;
	BOOL m_bModal;
	static HMODULE  m_hmUxTheme;
	static PFN_ITA  m_pfnIsThemeActive;
	static PFN_ETDT m_pfnEnableThemeDialogTexture;
	static PFN_OTD  m_pfnOpenThemeData;
	static PFN_CTD  m_pfnCloseThemeData;
	static PFN_DTB  m_pfnDrawThemeBackground;
	static PFN_DTPB m_pfnDrawThemeParentBackground;
	static PFN_GTBCR m_pfnGetThemeBackgroundContentRect;

	static BOOL addToMessageLoop(Dialog*);
	static BOOL removeFromMessageLoop(Dialog*);

	BOOL setTextureToTabColor()
	{
		if (!m_pfnEnableThemeDialogTexture) return FALSE;
		return !(*m_pfnEnableThemeDialogTexture)(m_hwndDlg, ETDT_ENABLETAB);
	}

	virtual BOOL initDialog(HWND hDlg) = 0;
	virtual void destroyDialog() = 0;
	virtual BOOL dialogProcMain(UINT, WPARAM, LPARAM) = 0;

private:
	static Lock m_lckActiveDialogs;
	static int m_nActiveDialogNum;
	static DialogMap m_activeDialogs;

	Dialog(const Dialog&);
	Dialog& operator=(const Dialog&);

	static BOOL CALLBACK dialogProc(HWND, UINT, WPARAM, LPARAM);
};

#endif
