// $Id$

#ifndef DC_MANAGER_H_INC
#define DC_MANAGER_H_INC

#include "bgb_manager.h"

class DrawInfo {
public:
	DrawInfo();
	virtual ~DrawInfo();

	HDC getDC() const { return m_hDC; }
	int getWidth() const { return m_nWidth; }
	int getHeight() const { return m_nHeight; }
	int getPixelsPerLine() const { return m_nPixelsPerLine; }
	COLORREF getBkColor() const { return m_crBkColor; }
	HBRUSH getBkBrush() const { return m_hbrBackground; }

	void setDC(HDC hDC) { m_hDC = hDC; }
	void setWidth(int width) { m_nWidth = width; }
	void setHeight(int height) { m_nHeight = height; }
	void setPixelsPerLine(int nPixelsPerLine)
	{
		m_nPixelsPerLine = nPixelsPerLine;
	}
	void setBkColor(COLORREF crBkColor)
	{
		::DeleteObject(m_hbrBackground);
		m_hbrBackground = ::CreateSolidBrush(crBkColor);
		m_crBkColor = crBkColor;
	}

protected:
	HDC m_hDC;
	int m_nWidth;
	int m_nHeight;
	int m_nPixelsPerLine;
	COLORREF m_crBkColor;
	HBRUSH m_hbrBackground;
};


struct Renderer {
	Renderer();
	virtual ~Renderer();

	virtual int render() = 0;

	virtual void bitBlt(HDC hDC, int x, int y, int cx, int cy,
						int sx, int sy) const = 0;

	virtual bool prepareDC(DrawInfo* pDrawInfo);

	HDC m_hDC;
	HBITMAP m_hBitmap;
	int m_nWidth, m_nHeight;
	HBRUSH m_hbrBackground;
};

struct DCBuffer : public BGBuffer, public Renderer {
	DCBuffer(int nBufSize);

	int init(LargeFileReader& LFReader, filesize_t offset);
	void uninit();

	void bitBlt(HDC hDC, int x, int y, int cx, int cy,
				int sx, int sy) const;

	void setCursor(int offset);
	void select(int offset, int size);

	bool hasCursor() const { return m_nCursorPos >= 0; }
	bool hasSelectedRegion() const
	{
		return m_nSelectedPos >= 0 && m_nSelectedSize > 0;
	}

	// return: cursor offset bytes
	virtual int setCursorByCoordinate(int x, int y) = 0;

protected:
	int  m_nCursorPos;
	int  m_nSelectedPos, m_nSelectedSize;

	virtual void invertRegionInBuffer(int offset, int size) = 0;
};

class DC_Manager : public BGB_Manager {
public:
	DC_Manager(int nBufSize, int nBufCount);

	bool onLoadFile(LF_Acceptor* pLFAcceptor);
	void onUnloadFile();

	int width() const { return m_nWidth; }
	int height() const { return m_nHeight; }

	int getXOffset() const { return m_nXOffset; }
	filesize_t getYOffset() const { return m_qYOffset; }

	int getViewWidth() const { return m_nViewWidth; }
	int getViewHeight() const { return m_nViewHeight; }

	// 仮想関数ではなく、BGB_Manager::getCurrentBuffer() を隠す
	DCBuffer* getCurrentBuffer(int offset = 0)
	{
		return static_cast<DCBuffer*>(BGB_Manager::getCurrentBuffer(offset));
	}

	// 仮想関数ではなく、BGB_Manager::getBuffer() を隠す
	DCBuffer* getBuffer(filesize_t offset)
	{
		return static_cast<DCBuffer*>(BGB_Manager::getBuffer(offset));
	}

	bool setDrawInfo(DrawInfo* pDrawInfo);

	void setViewSize(int nViewWidth, int nViewHeight)
	{
		m_nViewWidth  = nViewWidth;
		m_nViewHeight = nViewHeight;
	}

	void setViewPosition(int nXOffset, filesize_t qYOffset);

	void setViewPositionX(int nXOffset)
	{
//		setViewPosition(nXOffset, m_qYOffset);
		m_nXOffset = nXOffset;
	}
	void setViewPositionY(filesize_t qYOffset)
	{
		setViewPosition(m_nXOffset, qYOffset);
	}

	void setViewRect(int nXOffset, filesize_t qYOffset, int nWidth, int nHeight)
	{
		setViewSize(nWidth, nHeight);
		setViewPosition(nXOffset, qYOffset);
	}

	void setCursorByViewCoordinate(const POINTS& pt);

#ifdef _DEBUG
	virtual void bitBlt(HDC hDCDst, const RECT& rcDst);
#else
	void bitBlt(HDC hDCDst, const RECT& rcDst);
#endif

	virtual void setCursor(filesize_t pos);
	virtual void select(filesize_t pos, filesize_t size);

protected:
	DrawInfo* m_pDrawInfo;
	int m_nWidth, m_nHeight;
	int m_nXOffset;
	int m_nYOffset;
	int m_nViewWidth, m_nViewHeight;
	filesize_t m_qYOffset;
	filesize_t m_qCursorPos;
	filesize_t m_qStartSelected;
	filesize_t m_qSelectedSize;
	bool m_bOverlapped;
	DCBuffer* m_pCurBuf;
	DCBuffer* m_pNextBuf;

	bool
	isOverlapped(int y_offset_line_num, int page_line_num)
	{
		return (y_offset_line_num + page_line_num >= m_nHeight / m_nWidth);
	}
};

#endif
