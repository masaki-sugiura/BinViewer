// $Id$

#ifndef DC_MANAGER_H_INC
#define DC_MANAGER_H_INC

#include "bgb_manager.h"
#include "DrawInfo.h"

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

	void repaint();

	// return: cursor offset bytes
	virtual int setCursorByCoordinate(int x, int y) = 0;
	virtual int getPositionByCoordinate(int x, int y) const = 0;

protected:
	int  m_nCursorPos;
	int  m_nSelectedPos, m_nSelectedSize;

	virtual void invertRegionInBuffer(int offset, int size) = 0;
};

// 擬似的に（縦長の）メモリDCを表現するクラス
//
//	<---- width ---->
//	+---------------+↑
//	|				|
//	|	+-------+	|height
//	+---|--view-|---+↓
//	|	|		|	|
//	|	+-------+	|
//	+---------------+
//	|				|
//
class DC_Manager : public BGB_Manager {
public:
	DC_Manager(int nBufSize, int nBufCount);

	bool onLoadFile(LF_Acceptor* pLFAcceptor);
	void onUnloadFile();

	// メモリDCの幅を返す
	int width() const { return m_nWidth; }
//	int height() const { return m_nHeight; }

	// view 領域の座標を返す
	int getXOffset() const { return m_nXOffset; }
	filesize_t getYOffset() const { return m_qYOffset; }

	// view 領域のサイズを返す
	int getViewWidth() const { return m_nViewWidth; }
	int getViewHeight() const { return m_nViewHeight; }

	// view 領域のサイズを変更する
	void setViewSize(int nViewWidth, int nViewHeight)
	{
		m_nViewWidth  = nViewWidth;
		m_nViewHeight = nViewHeight;
	}

	// view 領域の座標を変更する
	void setViewPosition(int nXOffset, filesize_t qYOffset);

	// view 領域のx座標を変更する
	void setViewPositionX(int nXOffset)
	{
//		setViewPosition(nXOffset, m_qYOffset);
		m_nXOffset = nXOffset;
	}
	// view 領域のy座標を変更する
	void setViewPositionY(filesize_t qYOffset)
	{
		setViewPosition(m_nXOffset, qYOffset);
	}

	// view 領域の座標とサイズを変更する
	void setViewRect(int nXOffset, filesize_t qYOffset, int nWidth, int nHeight)
	{
		setViewSize(nWidth, nHeight);
		setViewPosition(nXOffset, qYOffset);
	}

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

	filesize_t setCursorByViewCoordinate(const POINTS& pt);
	filesize_t getPositionByViewCoordinate(const POINTS& pt);

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
