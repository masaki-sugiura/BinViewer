// $Id$

#ifndef HEXVIEW_H_INC
#define HEXVIEW_H_INC

#include "View.h"
#include "DrawInfo.h"

#define DATA_HEX_WIDTH				2
#define DATA_CENTER_WIDTH			3

#define ADDRESS_REGION_START_OFFSET	0
#define ADDRESS_START_OFFSET		0
#define ADDRESS_END_OFFSET			(ADDRESS_START_OFFSET + 16)
#define ADDRESS_REGION_END_OFFSET	(ADDRESS_END_OFFSET + 1)
#define DATA_REGION_START_OFFSET	ADDRESS_REGION_END_OFFSET
#define DATA_FORMAR_START_OFFSET	(DATA_REGION_START_OFFSET + 1)
#define DATA_FORMAR_END_OFFSET		(DATA_FORMAR_START_OFFSET + (DATA_HEX_WIDTH + 1) * 8 - 1)
#define DATA_LATTER_START_OFFSET	(DATA_FORMAR_END_OFFSET + DATA_CENTER_WIDTH)
#define DATA_LATTER_END_OFFSET		(DATA_LATTER_START_OFFSET + (DATA_HEX_WIDTH + 1) * 8 - 1)
#define DATA_REGION_END_OFFSET		(DATA_LATTER_END_OFFSET + 1)
#define STRING_REGION_START_OFFSET	DATA_REGION_END_OFFSET
#define STRING_START_OFFSET			STRING_REGION_START_OFFSET
#define STRING_END_OFFSET			(STRING_START_OFFSET + 16)

enum TCI_INDEX {
	TCI_ADDRESS = 0,
	TCI_DATA	= 1,
	TCI_STRING	= 2
};

class InvalidIndexError : public exception {
};

class HV_DrawInfo : public DrawInfo {
public:
	HV_DrawInfo(HDC hDC, float fontsize,
				const char* faceName, bool bBoldFace,
				COLORREF crFgColorAddr, COLORREF crBkColorAddr,
				COLORREF crFgColorData, COLORREF crBkColorData,
				COLORREF crFgColorStr, COLORREF crBkColorStr,
				CARET_MOVE caretMove, WHEEL_SCROLL wheelScroll);

	FontInfo m_FontInfo;
	TextColorInfo m_tciAddress, m_tciData, m_tciString;
	ScrollConfig m_ScrollConfig;

	TextColorInfo& getTextColorInfo(TCI_INDEX index)
	{
		switch (index) {
		case TCI_ADDRESS:
			return m_tciAddress;
		case TCI_DATA:
			return m_tciData;
		case TCI_STRING:
			return m_tciString;
		default:
			assert(0);
			throw InvalidIndexError();
		}
	}
	const TextColorInfo& getTextColorInfo(TCI_INDEX index) const
	{
		return const_cast<HV_DrawInfo*>(this)->getTextColorInfo(index);
	}

private:
	HV_DrawInfo(const HV_DrawInfo&);
	HV_DrawInfo& operator=(const HV_DrawInfo&);
};

class HV_DCBuffer : public DCBuffer {
public:
	HV_DCBuffer(int nBufSize);

	bool prepareDC(DrawInfo* pDrawInfo);
	int render();
	int setCursorByCoordinate(int x, int y);
	int getPositionByCoordinate(int x, int y) const;

protected:
	HV_DrawInfo* m_pHVDrawInfo;
	int m_anXPitch[16];
	
	void invertRegionInBuffer(int offset, int size);
	void invertOneLineRegion(int start_column, int end_column, int line);
};


class HV_DCManager : public DC_Manager {
public:
	HV_DCManager();

#ifdef _DEBUG
	void bitBlt(HDC hDCDst, const RECT& rcDst);
#endif

protected:
	BGBuffer* createBGBufferInstance();
};


class HexView : public View {
public:
	HexView(HWND hwndParent, const RECT& rctWindow, HV_DrawInfo* pDrawInfo);
	~HexView();

	bool setDrawInfo(DrawInfo* pDrawInfo);

protected:
	LRESULT viewWndProcMain(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif // HEXVIEW_H_INC
