// $Id$

#ifndef STATUSBAR_H_INC
#define STATUSBAR_H_INC

#include "LF_Notify.h"

#define IDC_STATUSBAR  10
#define STATUSBAR_HEIGHT  20

class StatusBar : public LF_Acceptor {
public:
	StatusBar(LF_Notifier& lfNotifier,
			  HWND hwndParent, const RECT& rctBar);
	~StatusBar();

	bool onLoadFile();
	void onUnloadFile();

	void setWindowPos(const RECT& rctBar);

private:
	HWND m_hwndParent, m_hwndStatusBar;

	void setStatus(filesize_t newpos);
};

class CreateStatusBarError {};

#endif
