// $Id$

#ifndef DC_MANAGER_H_INC
#define DC_MANAGER_H_INC

#include "bgb_manager.h"
#include "drawinfo.h"

#define WIDTH_PER_XPITCH  (1 + 16 + 1 + 8 * 3 + 2 + 8 * 3 + 1 + 16 + 1)
#define HEIGHT_PER_YPITCH (MAX_DATASIZE_PER_BUFFER / 16)

class DC_Manager;

class Renderer {
public:
	Renderer(HDC hDC,
			 const TextColorInfo* pTCInfo,
			 const FontInfo* pFontInfo,
			 int w_per_xpitch, int h_per_ypitch);
	virtual ~Renderer();

	virtual const TextColorInfo* getTextColorInfo() const
	{
		return m_pTextColorInfo;
	}
	virtual const FontInfo* getFontInfo() const
	{
		return m_pFontInfo;
	}
	virtual void setDrawInfo(HDC hDC,
							 const TextColorInfo* pTCInfo,
							 const FontInfo* pFontInfo);

	HDC m_hDC;
	HBITMAP m_hBitmap;
	RECT m_rctDC;

protected:
	const TextColorInfo* m_pTextColorInfo;
	const FontInfo* m_pFontInfo;
	const int m_nWidth_per_XPitch, m_nHeight_per_YPitch;
	int m_anXPitch[16];

	virtual void prepareDC(HDC);
	virtual int render() = 0;
};

struct DCBuffer : public BGBuffer, public Renderer {
	DCBuffer(HDC hDC,
			 const TextColorInfo* pTCInfo,
			 const FontInfo* pFontInfo);

	int init(LargeFileReader& LFReader, filesize_t offset);
	void uninit();

	void invertRegion(filesize_t pos, int size);

protected:
	int render();

	void invertOneLineRegion(int column, int lineno, int n_char);
};

struct Header : public Renderer {
	Header(HDC hDC,
		   const TextColorInfo* pTCInfo,
		   const FontInfo* pFontInfo);

protected:
	int render();
};

class DC_Manager : public BGB_Manager {
public:
	DC_Manager(HDC hDC, const DrawInfo* pDrawInfo,
			   LargeFileReader* pLFReader = NULL);

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

	int getXPositionByCoordinate(int x);

	void setDrawInfo(HDC hDC, const DrawInfo* pDrawInfo);

	HDC getHeaderDC() { return m_Header.m_hDC; }

protected:
	const DrawInfo* m_pDrawInfo;
	Header m_Header;

	BGBuffer* createBGBufferInstance();
};

#endif
