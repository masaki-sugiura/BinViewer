// $Id$

#ifndef HEXVIEW_H_INC
#define HEXVIEW_H_INC

#include "View.h"
#include "DrawInfo.h"

class HV_DCBuffer : public DCBuffer {
public:
	HV_DCBuffer(int nBufSize);

	bool prepareDC(DrawInfo* pDrawInfo);
	int render();
	int setCursorByCoordinate(int x, int y);

protected:
	HV_DrawInfo* m_pHVDrawInfo;
	int m_anXPitch[16];
	
	void invertRegionInBuffer(int offset, int size);
	void invertOneLineRegion(int start_column, int end_column, int line);
};


class HV_DCManager : public DC_Manager {
public:
	HV_DCManager();

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
