// $Id$

#ifndef SEARCHDLG_H_INC
#define SEARCHDLG_H_INC

#include "viewframe.h"

class SearchDlg {
public:
	static bool create(HWND hwndParent, ViewFrame* pViewFrame);
	static void close();

	static bool search(int dir);
	static void enableControls(int dir, bool);

	static BOOL isDialogMessage(MSG* pmsg)
	{
		if (!m_hwndDlg) return FALSE;
		return ::IsDialogMessage(m_hwndDlg, pmsg);
	}

private:
	static bool m_bSearching, m_bShowGrepDialog;
	static HWND m_hwndDlg, m_hwndSearch, m_hwndGrep, m_hwndParent;
	static ViewFrame* m_pViewFrame;

	static void adjustClientRect(HWND hDlg, const RECT& rctClient);

	static BOOL CALLBACK SearchMainDlgProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK SearchDlgProc(HWND, UINT, WPARAM, LPARAM);
	static BOOL CALLBACK SearchGrepDlgProc(HWND, UINT, WPARAM, LPARAM);
	static void FindCallbackProc(void* arg);
};

#endif
