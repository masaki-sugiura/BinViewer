// $Id$

#ifndef DIALOG_H_INC
#define DIALOG_H_INC

#include <windows.h>
#include <map>
using std::map;

typedef map<HWND, class Dialog*> DialogMap;

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

	static BOOL isDialogMessage(MSG* msg);

protected:
	int  m_nDialogID;
	HWND m_hwndDlg, m_hwndParent;
	BOOL m_bModal;

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
