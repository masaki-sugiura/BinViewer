// $Id$

#ifndef MAINWINDOW_H_INC
#define MAINWINDOW_H_INC

#include "HexView.h"
#include "BitmapView.h"
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
	HWND m_hWnd, m_hwndStatusBar;

	void onCreate(HWND hWnd);
	void onResize();
	void onResizing(RECT* pRect);
	void onQuit();

	void adjustWindowSize(HWND hWnd, const RECT& rctClient);

	static int registerWndClass(HINSTANCE hInstance);

	static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

class CreateWindowError {};
class CreateHexViewError {};

#endif
