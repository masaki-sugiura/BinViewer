// $Id$

#ifndef MAINWINDOW_H_INC
#define MAINWINDOW_H_INC

#include "HexView.h"
#include "BitmapView.h"
#include "StatusBar.h"
#include "auto_ptr.h"

class HD_DrawInfo : public DrawInfo {
public:
	HD_DrawInfo(HDC hDC, float fontsize,
				const char* faceName, bool bBoldFace,
				COLORREF crFgColor, COLORREF crBkColor);

	FontInfo m_FontInfo;
	TextColorInfo m_tciHeader;
};

struct AppConfig {
	Auto_Ptr<HV_DrawInfo> m_pHVDrawInfo;
	Auto_Ptr<HD_DrawInfo> m_pHDDrawInfo;
//	Auto_Ptr<BV_DrawInfo> m_pBVDrawInfo;

	AppConfig()
		: m_pHVDrawInfo(NULL),
		  m_pHDDrawInfo(NULL)
	{}
};

struct Header : public Renderer {
	Header();

	bool prepareDC(DrawInfo* pDrawInfo);

	void bitBlt(HDC hDC, int x, int y, int cx, int cy,
				int sx, int sy) const;

	int render();

	void setDrawInfo(HD_DrawInfo* pHDDrawInfo);

protected:
	HD_DrawInfo* m_pDrawInfo;
	int m_anXPitch[16];
};

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
//	Auto_Ptr<HV_DrawInfo> m_pHVDrawInfo;
//	Auto_Ptr<HD_DrawInfo> m_pHDDrawInfo;
	Auto_Ptr<AppConfig> m_pAppConfig;
	Auto_Ptr<BitmapViewWindow> m_pBitmapViewWindow;
	Auto_Ptr<StatusBar> m_pStatusBar;
	HWND m_hWnd;
	HACCEL m_hAccel;
	Header m_Header;

	bool onCreate(HWND hWnd);
	void onPaint(HWND hWnd);
	void onResize(HWND hWnd);
	void onResizing(HWND hWnd, RECT* pRect);
	void onQuit();
	void onOpenFile();
	void onCloseFile();
	void onShowBitmapView();
	void onJump();
	void onConfig();
	void onSetFontConfig(FontConfig* pFontConfig, ColorConfig* pColorConfig);
	void onSetScrollConfig(ScrollConfig* pScrollConfig);

	AppConfig* loadDrawInfo(HWND hWnd);

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
