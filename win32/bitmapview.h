// $Id$

#ifndef BITMAPVIEW_H_INC
#define BITMAPVIEW_H_INC

#include "LargeFileReader.h"
#include "LF_Notify.h"
#include "scroll.h"

#include <exception>
using std::exception;

class ViewFrame;

class BitmapView : public LF_Acceptor {
public:
	BitmapView(HWND hwndOwner, ViewFrame* pViewFrame);
	~BitmapView();

	bool onLoadFile();
	void onUnloadFile();

	bool show();
	bool hide();

	bool setPosition(filesize_t pos);

private:
	HWND m_hwndOwner, m_hwndView;
	ViewFrame* m_pViewFrame;
	UINT m_uWindowWidth;
	RECT m_rctClient;
	HDC  m_hdcView;
	HBITMAP m_hbmView;
	BITMAPINFO* m_pbmInfo;
	filesize_t m_qCurrentPos, m_qHeadPos;
	ScrollManager<filesize_t>* m_pScrollManager;
	bool m_bLoaded;

	int registerWndClass(HINSTANCE hInst);

	void drawDC(const RECT& rctDraw);
	void invertPos();

	bool onCreate(HWND hWnd);
	void onPaint(HWND hWnd);
	bool onResize(HWND hWnd);
	void onScroll(WPARAM, LPARAM);
	void onLButtonDown(WPARAM, LPARAM);

	static LRESULT CALLBACK BitmapViewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

class CreateBitmapViewError : public exception {};

#endif
