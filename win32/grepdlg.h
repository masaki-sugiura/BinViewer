// $Id$

#ifndef GREPDLG_H_INC
#define GREPDLG_H_INC

#include "viewframe.h"

class GrepDlg {
public:
	GrepDlg(ViewFrame* pViewFrame);

	bool create(HWND hwndParent);
	void close();

	BOOL isDialogMessage(MSG* pmsg)
	{
		if (!m_hwndDlg) return FALSE;
		return ::IsDialogMessage(m_hwndDlg, pmsg);
	}

private:
	HWND m_hwndDlg, m_hwndParent;
	ViewFrame* m_pViewFrame;

	bool initDialog(HWND hDlg);
	BOOL handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	static BOOL CALLBACK GrepDlgProc(HWND, UINT, WPARAM, LPARAM);
};

#endif
