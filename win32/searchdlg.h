// $Id$

#ifndef SEARCHDLG_H_INC
#define SEARCHDLG_H_INC

#include "viewframe.h"
#include "dialog.h"

class SearchDlg : public Dialog {
public:
	SearchDlg(Dialog* pParentDlg, ViewFrame& viewFrame);
	~SearchDlg();

	bool prepareFindCallbackArg(FindCallbackArg*& pFindCallbackArg);

	bool isSearching() const { return m_bSearching; }

protected:
	BOOL initDialog(HWND hDlg);
	void destroyDialog();
	BOOL dialogProcMain(UINT, WPARAM, LPARAM);

private:
	Dialog* m_pParentDlg;
	ViewFrame& m_ViewFrame;
	bool m_bSearching;

	void enableControls(int dir, bool);

	bool search(int dir);

	static void FindCallbackProc(void* arg);
};

class GrepDlg : public Dialog {
public:
	GrepDlg(Dialog* pParentDlg, SearchDlg* pSearchDlg,
			ViewFrame& viewFrame);
	~GrepDlg();

	bool grep();

protected:
	BOOL initDialog(HWND hDlg);
	void destroyDialog();
	BOOL dialogProcMain(UINT, WPARAM, LPARAM);

private:
	Dialog* m_pParentDlg;
	SearchDlg* m_pSearchDlg;
	ViewFrame& m_ViewFrame;

	static void GrepCallbackProc(void* arg);
};

class SearchMainDlg : public Dialog {
public:
	SearchMainDlg(ViewFrame& viewFrame);
	~SearchMainDlg();

protected:
	BOOL initDialog(HWND hDlg);
	void destroyDialog();
	BOOL dialogProcMain(UINT, WPARAM, LPARAM);

private:
	ViewFrame& m_ViewFrame;
	SearchDlg* m_pSearchDlg;
	GrepDlg*   m_pGrepDlg;
	bool m_bShowGrepDialog;

	void adjustClientRect(HWND hDlg, const RECT& rctClient);
};

#endif
