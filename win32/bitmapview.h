// $Id$

#ifndef BITMAPVIEW_H_INC
#define BITMAPVIEW_H_INC

#include "LargeFileReader.h"
#include "scroll.h"

#include <exception>
using std::exception;

class BitmapView {
public:
	BitmapView(HWND hwndOwner, LargeFileReader* pLFReader = NULL);
	~BitmapView();

	bool loadFile(LargeFileReader* pLFReader);
	void unloadFile();

	bool show();
	bool hide();

	bool setPosition(filesize_t pos);

private:
	HWND m_hwndOwner, m_hwndView;
	LargeFileReader* m_pLFReader;
	UINT m_uWindowWidth;
	RECT m_rctClient;
	HDC  m_hdcView;
	HBITMAP m_hbmView;
	filesize_t m_qCurrentPos, m_qHeadPos;
	ScrollManager<filesize_t>* m_pScrollManager;

	void drawDC(const RECT& rctDraw);
	void invertPos();

	bool onCreate(HWND hWnd);
	void onPaint(HWND hWnd);
	bool onResize(HWND hWnd);
	void onScroll(WPARAM, LPARAM);

	static LRESULT CALLBACK BitmapViewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

class CreateBitmapViewError : public exception {};

#endif
