// $Id$

#ifndef JUMPDLG_H_INC
#define JUMPDLG_H_INC

#include "viewframe.h"
#include "dialog.h"

class JumpDlg : public Dialog {
public:
	JumpDlg(ViewFrame* pViewFrame);

protected:
	BOOL initDialog(HWND hDlg);
	void destroyDialog();
	BOOL dialogProcMain(UINT, WPARAM, LPARAM);

private:
	ViewFrame* m_pViewFrame;
};

#endif
