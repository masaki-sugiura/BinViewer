// $Id$

#ifndef BITMAPVIEW_H_INC
#define BITMAPVIEW_H_INC

#include "View.h"
#include "auto_ptr.h"

#define BV_WIDTH   128
#define BV_HEIGHT 1024
#define BV_BUFCOUNT  3

#define BV_WNDCLASSNAME  "BinViewer_BitmapViewClass32"

class BV_DrawInfo : public DrawInfo {
public:
	BV_DrawInfo();
};

class BV_DCBuffer : public DCBuffer {
public:
	BV_DCBuffer(int nBufSize);

	bool prepareDC(DrawInfo* pDrawInfo);
	int render();
	int setCursorByCoordinate(int x, int y);

protected:
	BV_DrawInfo* m_pBVDrawInfo;
	
	void invertRegionInBuffer(int offset, int size);
	void invertOneLineRegion(int start_column, int end_column, int line);
};

class BV_DCManager : public DC_Manager {
public:
	BV_DCManager();

#ifdef _DEBUG
	void bitBlt(HDC hDCDst, const RECT& rcDst);
#endif

protected:
	BGBuffer* createBGBufferInstance();
};

class BitmapView : public View {
public:
	BitmapView(LF_Notifier& lfNotifier,
			   HWND hwndParent, const RECT& rctWindow, BV_DrawInfo* pDrawInfo);
	~BitmapView();

	bool setDrawInfo(DrawInfo* pDrawInfo);

protected:
	LRESULT viewWndProcMain(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

class BitmapViewWindow {
public:
	BitmapViewWindow(LF_Notifier& lfNotify, HWND hwndOwner);
	~BitmapViewWindow();

	bool show();
	bool hide();

private:
	LF_Notifier& m_lfNotify;
	Auto_Ptr<BitmapView> m_pBitmapView;
	Auto_Ptr<BV_DrawInfo> m_pBVDrawInfo;
	HWND m_hwndOwner, m_hWnd;
	UINT m_uWindowWidth;

	void onCreate(HWND hWnd);
	void onResize(HWND hWnd);

	static int registerWndClass(HINSTANCE hInstance);

	static LRESULT CALLBACK BitmapViewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

class CreateBitmapViewError {};

#endif
