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
	static bool m_bSearching;
	static HWND m_hwndDlg, m_hwndParent;
	static ViewFrame* m_pViewFrame;

	static BOOL CALLBACK SearchDlgProc(HWND, UINT, WPARAM, LPARAM);
	static void FindCallbackProc(void* arg);
};

#endif
