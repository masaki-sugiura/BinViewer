// $Id$

#ifndef MAINWINDOW_H_INC
#define MAINWINDOW_H_INC

#include "HexView.h"
#include "BitmapView.h"
#include "StatusBar.h"
#include "auto_ptr.h"

class MainWindow {
public:
	MainWindow(HINSTANCE hInstance, LPCSTR lpszFileName);
	~MainWindow();

	bool show(int nCmdShow);

	int doModal();

private:
	LF_Notifier m_lfNotifier;
	Auto_Ptr<HexView> m_pHexView;
	Auto_Ptr<LargeFileReader> m_pLFReader;
	Auto_Ptr<HV_DrawInfo> m_pHVDrawInfo;
	Auto_Ptr<BitmapViewWindow> m_pBitmapViewWindow;
	Auto_Ptr<StatusBar> m_pStatusBar;
	HWND m_hWnd;
	HACCEL m_hAccel;

	void onCreate(HWND hWnd);
	void onResize();
	void onResizing(RECT* pRect);
	void onQuit();
	void onOpenFile();
	void onCloseFile();

	HV_DrawInfo* loadDrawInfo(HWND hWnd);

	void adjustWindowSize(HWND hWnd, const RECT& rctClient);

	bool getImageFileName(LPSTR buf, int bufsize);
	bool openFile(LPCSTR pszFileName);
	void enableMenuForOpenFile(bool bEnable);

	static int registerWndClass(HINSTANCE hInstance);

	static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

class CreateWindowError {};
class CreateHexViewError {};

#endif
