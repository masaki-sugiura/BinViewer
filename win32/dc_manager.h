// $Id$

#ifndef DC_MANAGER_H_INC
#define DC_MANAGER_H_INC

#include "bgb_manager.h"

struct Renderer {
	Renderer(HDC hDC, int width, int height, HBRUSH hbrBackground);
	virtual ~Renderer();

	virtual int render() = 0;

	virtual void bitBlt(HDC hDC, int x, int y, int cx, int cy,
						int sx, int sy) const = 0;

	void prepareDC(HDC hDC, int width, int height, HBRUSH hbrBackground);

	HDC m_hDC;
	HBITMAP m_hBitmap;
	int m_nWidth, m_nHeight;
	HBRUSH m_hbrBackground;
};

struct DCBuffer : public BGBuffer, public Renderer {
	DCBuffer(int nBufSize,
			 HDC hDC, int nWidth, int nHeight,
			 HBRUSH hbrBackground);

	int init(LargeFileReader& LFReader, filesize_t offset);
	void uninit();

	void bitBlt(HDC hDC, int x, int y, int cx, int cy,
				int sx, int sy) const;

	void invertRegion(filesize_t pos, int size, bool bSelected);

	bool hasSelectedRegion() const { return m_bHasSelectedRegion; }

protected:
	bool m_bHasSelectedRegion;
	int  m_nSelectedPos, m_nSelectedSize;

	virtual void invertRegionInBuffer(int offset, int size) = 0;
};

class DC_Manager : public BGB_Manager {
public:
	DC_Manager(int nBufSize, int nBufCount);

	int width() const { return m_nWidth; }
	int height() const { return m_nHeight; }

	int getXOffset() const { return m_nXOffset; }
	filesize_t getYOffset() const { return m_qYOffset; }

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

	void setDCInfo(HDC hDC, int width, int height, HBRUSH hbrBackground);

	void setViewSize(int nViewWidth, int nViewHeight)
	{
		m_nViewWidth  = nViewWidth;
		m_nViewHeight = nViewHeight;
	}

	void setViewPosition(int nXOffset, filesize_t qYOffset);

	void setViewPosition(int nXOffset)
	{
//		setViewPosition(nXOffset, m_qYOffset);
		m_nXOffset = nXOffset;
	}
	void setViewPosition(filesize_t qYOffset)
	{
		setViewPosition(m_nXOffset, qYOffset);
	}

	void setViewRect(int nXOffset, filesize_t qYOffset, int nWidth, int nHeight)
	{
		setViewSize(nWidth, nHeight);
		setViewPosition(nXOffset, qYOffset);
	}

	void bitBlt(HDC hDCDst, const RECT& rcDst);

	virtual int getXPositionByCoordinate(int x) = 0;

protected:
	HDC m_hDC;
	int m_nWidth, m_nHeight;
	int m_nXOffset;
	int m_nYOffset;
	int m_nViewWidth, m_nViewHeight;
	filesize_t m_qYOffset;
	bool m_bOverlapped;
	DCBuffer* m_pCurBuf;
	DCBuffer* m_pNextBuf;
	HBRUSH m_hbrBackground;

	bool
	isOverlapped(int y_offset_line_num, int page_line_num)
	{
		return (y_offset_line_num + page_line_num >= m_nHeight / m_nWidth);
	}
};

#endif
