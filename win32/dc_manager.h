// $Id$

#ifndef DC_MANAGER_H_INC
#define DC_MANAGER_H_INC

#include "bgb_manager.h"
#include "drawinfo.h"

#define MAX_DATASIZE_PER_BUFFER  1024 // 1KB

#define BUFFER_NUM  3

#define WIDTH_PER_XPITCH  (1 + 16 + 2 + 8 * 3 + 2 + 8 * 3 + 1 + 16 + 1)
#define HEIGHT_PER_YPITCH (MAX_DATASIZE_PER_BUFFER / 16)

class DC_Manager;

class Renderer {
public:
	Renderer(HDC hDC,
			 const DrawInfo* pDrawInfo,
			 int w_per_xpitch, int h_per_ypitch);
	virtual ~Renderer();

#if 0
	virtual const TextColorInfo* getTextColorInfo() const
	{
		return m_pTextColorInfo;
	}
	virtual const FontInfo* getFontInfo() const
	{
		return m_pFontInfo;
	}
#endif
	virtual const DrawInfo* getDrawInfo() const
	{
		return m_pDrawInfo;
	}
	virtual void setDrawInfo(HDC hDC, const DrawInfo* pDrawInfo);

	virtual void bitBlt(HDC hDC, int x, int y, int cx, int cy,
						int sx, int sy) const = 0;

	HDC m_hDC;
	HBITMAP m_hBitmap;
	RECT m_rctDC;

protected:
	const DrawInfo* m_pDrawInfo;
	const int m_nWidth_per_XPitch, m_nHeight_per_YPitch;
	int m_anXPitch[16];

	virtual void prepareDC(HDC);
	virtual int render() = 0;
};

struct DCBuffer : public BGBuffer, public Renderer {
	DCBuffer(HDC hDC, const DrawInfo* pDrawInfo);

	int init(LargeFileReader& LFReader, filesize_t offset);
	void uninit();

	void invertRegion(filesize_t pos, int size, bool bSelected);

	void bitBlt(HDC hDC, int x, int y, int cx, int cy,
				int sx, int sy) const;

	bool hasSelectedRegion() const { return m_bHasSelectedRegion; }

protected:
	bool m_bHasSelectedRegion;
	int  m_nSelectedPos, m_nSelectedSize;

	int render();

	void invertOneLineRegion(int column, int lineno, int n_char);
	void invertRegionInBuffer(int offset, int size);
};

struct Header : public Renderer {
	Header(HDC hDC, const DrawInfo* pDrawInfo);

	void setDrawInfo(HDC hDC, const DrawInfo* pDrawInfo);

	void bitBlt(HDC hDC, int x, int y, int cx, int cy,
				int sx, int sy) const;

protected:
	int render();
};

class DC_Manager : public BGB_Manager {
public:
	DC_Manager(HDC hDC, const DrawInfo* pDrawInfo,
			   LargeFileReader* pLFReader = NULL);

	// âºëzä÷êîÇ≈ÇÕÇ»Ç≠ÅABGB_Manager::getCurrentBuffer() ÇâBÇ∑
	DCBuffer* getCurrentBuffer(int offset = 0)
	{
		return static_cast<DCBuffer*>(BGB_Manager::getCurrentBuffer(offset));
	}

	// âºëzä÷êîÇ≈ÇÕÇ»Ç≠ÅABGB_Manager::getBuffer() ÇâBÇ∑
	DCBuffer* getBuffer(filesize_t offset)
	{
		return static_cast<DCBuffer*>(BGB_Manager::getBuffer(offset));
	}

	int getXPositionByCoordinate(int x);

	void setDrawInfo(HDC hDC, const DrawInfo* pDrawInfo);

	Header& getHeader() { return m_Header; }

protected:
	const DrawInfo* m_pDrawInfo;
	Header m_Header;

	BGBuffer* createBGBufferInstance();
};

#endif
