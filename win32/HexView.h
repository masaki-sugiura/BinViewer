// $Id$

#ifndef HEXVIEW_H_INC
#define HEXVIEW_H_INC

#include "View.h"
#include "DrawInfo.h"

class HV_DCBuffer : public DCBuffer {
public:
	HV_DCBuffer(DrawInfo* pDrawInfo);

	int render();

	bool setDrawInfo(DrawInfo* pDrawInfo);

protected:
	DrawInfo* m_pDrawInfo;
	
	void invertRegionInBuffer(int offset, int size);
};


class HexView : public View {
public:
	HexView(DrawInfo* pDrawInfo);
};

#endif // HEXVIEW_H_INC
