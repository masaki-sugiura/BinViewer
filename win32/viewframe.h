// $Id$

#ifndef VIEWFRAME_H_INC
#define VIEWFRAME_H_INC

#include "dc_manager.h"
#include <assert.h>

class ViewFrame {
public:
	ViewFrame(HWND hWnd, const DrawInfo* pDrawInfo,
			  LargeFileReader* pLFReader = NULL);
	~ViewFrame();

	bool loadFile(LargeFileReader* pLFReader)
	{
		initParams();
		if (!m_pDC_Manager->loadFile(pLFReader)) return false;
		calcMaxLine();
		return true;
	}

	void unloadFile()
	{
		m_pDC_Manager->unloadFile();
		initParams();
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

	void bitBlt(HDC hDC, const RECT& rcPaint);

	void setDrawInfo(HDC hDC, const DrawInfo* pDrawInfo)
	{
		assert(pDrawInfo);
		m_pDrawInfo = pDrawInfo;
		m_nLineHeight = m_pDrawInfo->m_FontInfo.getYPitch();
		m_nCharWidth  = m_pDrawInfo->m_FontInfo.getXPitch();
		m_nPageLineNum = (m_rctClient.bottom - m_rctClient.top + m_nLineHeight - 1)
						  / m_nLineHeight - 1 /* ƒwƒbƒ_‚Ì•ª‚Íœ‚­ */;
		m_pDC_Manager->setDrawInfo(hDC, pDrawInfo);
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
	const RECT& getFrameRect() const { return m_rctClient; }

private:
	Auto_Ptr<DC_Manager> m_pDC_Manager;
	const DrawInfo* m_pDrawInfo;
	int m_nLineHeight;
	int m_nCharWidth;
	int m_nPageLineNum;
	RECT m_rctClient;
	bool m_bOverlapped;
	filesize_t m_qCurrentLine;
	filesize_t m_qMaxLine;
	int m_nTopOffset, m_nXOffset;
	DCBuffer *m_pCurBuf, *m_pNextBuf;
	filesize_t m_qCurrentPos;
	filesize_t m_qPrevSelectedPos;
	int m_nPrevSelectedSize;

	void initParams()
	{
		m_qCurrentLine = m_qMaxLine = 0;
		m_qCurrentPos = m_qPrevSelectedPos = -1;
		m_nPrevSelectedSize = 0;
		m_nTopOffset = m_nXOffset = 0;
		m_bOverlapped = false;
		m_pCurBuf = m_pNextBuf = NULL;
	}

	void calcMaxLine()
	{
		m_qMaxLine = (m_pDC_Manager->getFileSize() >> 4);
		if (m_qMaxLine <= 0) m_qMaxLine = 1;
	}

	void invertOneLineRegion(HDC hDC, int column, int lineno, int n_char);
	void invertOneBufferRegion(DCBuffer* pDCBuffer, filesize_t pos, int size);
	void invertRegion(filesize_t pos, int size);
};

#endif
