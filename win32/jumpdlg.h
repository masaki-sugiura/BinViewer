// $Id$

#ifndef JUMPDLG_H_INC
#define JUMPDLG_H_INC

#include "LF_Notify.h"
#include "dialog.h"

class JumpDlg : public Dialog {
public:
	JumpDlg(LF_Notifier& lfNotifier);

protected:
	BOOL initDialog(HWND hDlg);
	void destroyDialog();
	BOOL dialogProcMain(UINT, WPARAM, LPARAM);

private:
	LF_Notifier& m_lfNotifier;
};

#endif
