// $Id$

#ifndef SEARCHDLG_H_INC
#define SEARCHDLG_H_INC

#include "viewframe.h"
#include "dialog.h"

class SearchMainDlg;

class SearchDlg : public Dialog {
public:
	SearchDlg(SearchMainDlg* pParentDlg, ViewFrame& viewFrame);
	~SearchDlg();

	bool prepareFindCallbackArg(FindCallbackArg*& pFindCallbackArg);

	bool isSearching() const { return m_bSearching; }

protected:
	BOOL initDialog(HWND hDlg);
	void destroyDialog();
	BOOL dialogProcMain(UINT, WPARAM, LPARAM);

private:
	SearchMainDlg* m_pParentDlg;
	ViewFrame& m_ViewFrame;
	bool m_bSearching;
	string m_strSearchStr;
	int m_nStringType;

	void enableControls(FIND_DIRECTION dir, bool);

	bool search(FIND_DIRECTION dir);

	static void FindCallbackProc(FindCallbackArg* pArg);
};

class GrepDlg : public Dialog {
public:
	GrepDlg(SearchMainDlg* pParentDlg, SearchDlg* pSearchDlg,
			ViewFrame& viewFrame);
	~GrepDlg();

	bool grep();

protected:
	BOOL initDialog(HWND hDlg);
	void destroyDialog();
	BOOL dialogProcMain(UINT, WPARAM, LPARAM);

private:
	SearchMainDlg* m_pParentDlg;
	SearchDlg* m_pSearchDlg;
	ViewFrame& m_ViewFrame;

	static void GrepCallbackProc(FindCallbackArg* pArg);
};

class SearchMainDlg : public Dialog {
public:
	SearchMainDlg(ViewFrame& viewFrame);
	~SearchMainDlg();

	bool findCallback(FindCallbackArg* pArg);
	bool stopFind();
	bool waitStopFind();
	bool cleanupCallback();

protected:
	BOOL initDialog(HWND hDlg);
	void destroyDialog();
	BOOL dialogProcMain(UINT, WPARAM, LPARAM);

private:
	ViewFrame& m_ViewFrame;
	SearchDlg* m_pSearchDlg;
	GrepDlg*   m_pGrepDlg;
	bool m_bShowGrepDialog;
	Auto_Ptr<Thread> m_pThread;
	Lock m_lockFindCallbackData;

	void adjustClientRect(HWND hDlg, const RECT& rctClient);
};

#endif
