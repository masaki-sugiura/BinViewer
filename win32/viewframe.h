// $Id$

#ifndef VIEWFRAME_H_INC
#define VIEWFRAME_H_INC

#include "dc_manager.h"
#include "messages.h"
#include <assert.h>
#include <exception>
using std::exception;

static inline bool
is_overlapped(int y_offset_line_num, int page_line_num)
{
	return (y_offset_line_num + page_line_num >= MAX_DATASIZE_PER_BUFFER / 16);
}

class ViewFrame {
public:
	ViewFrame(HWND hWnd, const RECT& rct,
			  DrawInfo* pDrawInfo,
			  LargeFileReader* pLFReader = NULL);
	~ViewFrame();

	bool loadFile(LargeFileReader* pLFReader)
	{
		initParams();
		if (!m_pDC_Manager->loadFile(pLFReader)) return false;
		calcMaxLine();
		modifyVScrollInfo();
		::InvalidateRect(m_hwndView, NULL, FALSE);
		::UpdateWindow(m_hwndView);
		return true;
	}

	void unloadFile()
	{
		m_pDC_Manager->unloadFile();
		initParams();
		modifyVScrollInfo();
		::InvalidateRect(m_hwndView, NULL, FALSE);
		::UpdateWindow(m_hwndView);
	}

	bool isLoaded() const { return m_pDC_Manager->isLoaded(); }

	filesize_t getFileSize() const
	{
		return m_pDC_Manager->getFileSize();
	}

	filesize_t getCurrentLine() const
	{
		return m_qCurrentLine;
	}
	void setCurrentLine(filesize_t newline);

	filesize_t getMaxLine() const
	{
		return m_qMaxLine;
	}

	void setPosition(filesize_t offset);
	filesize_t getPosition() const
	{
		return m_qCurrentPos;
	}

	void setXOffset(int nXOffset)
	{
		assert(nXOffset >= 0);
		m_nXOffset = nXOffset;
	}
	int getXOffset() const
	{
		return m_nXOffset;
	}

	void setPositionByCoordinate(const POINTS& pos);

	bool findCallback(FindCallbackArg* pArg)
	{
		return m_pDC_Manager->findCallback(pArg);
	}
	bool stopFind()
	{
		return m_pDC_Manager->stopFind();
	}
	bool cleanupCallback()
	{
		return m_pDC_Manager->cleanupCallback();
	}

	void select(filesize_t pos, int size);
	void unselect();
	int getSelectedSize() const
	{
		return m_nPrevSelectedSize;
	}

	void setDrawInfo(DrawInfo* pDrawInfo)
	{
		assert(pDrawInfo);
		m_pDrawInfo = pDrawInfo;
		m_nLineHeight = m_pDrawInfo->m_FontInfo.getYPitch();
		m_nCharWidth  = m_pDrawInfo->m_FontInfo.getXPitch();
		m_pDC_Manager->setDrawInfo(m_hDC, pDrawInfo);
		recalcPageInfo();
	}

	int getCharWidth() const
	{
		return m_nCharWidth;
	}
	int getLineHeight() const
	{
		return m_nLineHeight;
	}

	int getPageLineNum() const
	{
		return m_nPageLineNum;
	}

	void setFrameRect(const RECT& rctClient);
	const RECT& getFrameRect() const { return m_rctFrame; }
	void adjustWindowRect(RECT& rctFrame);

	void updateWithoutHeader();

	void onPaint(WPARAM, LPARAM);
	void onHScroll(WPARAM, LPARAM);
	void onVScroll(WPARAM, LPARAM);
	void onLButtonDown(WPARAM, LPARAM);
	void onMouseWheel(WPARAM, LPARAM);

	void onJump(filesize_t);
	void onHorizontalMove(int diff);
	void onVerticalMove(int diff);

private:
	Auto_Ptr<DC_Manager> m_pDC_Manager;
	DrawInfo* m_pDrawInfo;
	int m_nLineHeight;
	int m_nCharWidth;
	int m_nPageLineNum;
	RECT m_rctFrame, m_rctClient;
	bool m_bOverlapped;
	filesize_t m_qCurrentLine;
	filesize_t m_qMaxLine;
	int m_nTopOffset, m_nXOffset;
	DCBuffer *m_pCurBuf, *m_pNextBuf;
	filesize_t m_qCurrentPos;
	filesize_t m_qPrevSelectedPos;
	int m_nPrevSelectedSize;

	HWND m_hwndView;
	HDC  m_hDC;
	bool m_bMapScrollBarLinearly;

	void initParams()
	{
		m_qCurrentLine = m_qMaxLine = 0;
		m_qCurrentPos = m_qPrevSelectedPos = -1;
		m_nPrevSelectedSize = 0;
		m_nTopOffset = m_nXOffset = 0;
		m_bOverlapped = false;
		m_pCurBuf = m_pNextBuf = NULL;
	}

	void recalcPageInfo()
	{
		m_nPageLineNum = (m_rctClient.bottom - m_rctClient.top + m_nLineHeight - 1)
						  / m_nLineHeight - 1 /* ヘッダの分は除く */;
		// ウィンドウを広げた結果次のバッファとオーバーラップした場合に必要
		m_bOverlapped = is_overlapped(m_nTopOffset / m_nLineHeight, m_nPageLineNum);
		modifyVScrollInfo();
		modifyHScrollInfo(m_rctClient.right - m_rctClient.left);
		::InvalidateRect(m_hwndView, NULL, FALSE);
		::UpdateWindow(m_hwndView);
	}

	void calcMaxLine()
	{
		m_qMaxLine = (m_pDC_Manager->getFileSize() >> 4);
		if (m_qMaxLine <= 0) m_qMaxLine = 1;
	}

	void modifyVScrollInfo();
	void modifyHScrollInfo(int width);
	void bitBlt(const RECT& rcPaint);

	void invertOneLineRegion(HDC hDC, int column, int lineno, int n_char);
	void invertOneBufferRegion(DCBuffer* pDCBuffer, filesize_t pos, int size);
	void invertRegion(filesize_t pos, int size);

	static bool m_bRegisterClass;
	static WORD m_wNextID;
	static HINSTANCE m_hInstance;
	static LRESULT CALLBACK ViewFrameWndProc(HWND, UINT, WPARAM, LPARAM);
};

class ViewFrameError : public exception {};
class RegisterClassError : public ViewFrameError {};
class CreateWindowError  : public ViewFrameError {};

#endif
