// $Id$

#ifndef JUMPDLG_H_INC
#define JUMPDLG_H_INC

#include "viewframe.h"

class JumpDlg {
public:
	static void doModal(HWND hwndParent, ViewFrame* pViewFrame);

private:
	static HWND m_hwndParent;
	static ViewFrame* m_pViewFrame;

	static BOOL CALLBACK JumpDlgProc(HWND, UINT, WPARAM, LPARAM);
};

#endif
