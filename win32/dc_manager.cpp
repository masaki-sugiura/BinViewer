// $Id$

#include "dc_manager.h"
#include "strutils.h"
#include <assert.h>

Renderer::Renderer(HDC hDC,
				   const DrawInfo* pDrawInfo,
				   int w_per_xpitch, int h_per_ypitch)
	: m_hDC(::CreateCompatibleDC(hDC)),
	  m_pDrawInfo(pDrawInfo),
	  m_hBitmap(NULL),
	  m_nWidth_per_XPitch(w_per_xpitch),
	  m_nHeight_per_YPitch(h_per_ypitch)
{
	assert(hDC && pDrawInfo);
	prepareDC(hDC);
}

Renderer::~Renderer()
{
	::DeleteObject(m_hBitmap);
	::DeleteDC(m_hDC);
}

void
Renderer::prepareDC(HDC hDC)
{
	const FontInfo& FontInfo = m_pDrawInfo->m_FontInfo;

	int width  = FontInfo.getXPitch() * m_nWidth_per_XPitch,
		height = FontInfo.getYPitch() * m_nHeight_per_YPitch;

	if (m_hBitmap != NULL) ::DeleteObject(m_hBitmap);
	m_hBitmap = ::CreateCompatibleBitmap(hDC, width, height);

	::SelectObject(m_hDC, (HGDIOBJ)m_hBitmap);
	::SelectObject(m_hDC, (HGDIOBJ)FontInfo.getFont());
//	::SetTextColor(m_hDC, m_pTextColorInfo->getFgColor());
//	::SetBkColor(m_hDC, m_pTextColorInfo->getBkColor());

	m_rctDC.left = m_rctDC.top = 0;
	m_rctDC.right  = width;
	m_rctDC.bottom = height;

	int nXPitch = FontInfo.getXPitch();
	for (int i = 0; i < sizeof(m_anXPitch) / sizeof(m_anXPitch[0]); i++) {
		m_anXPitch[i] = nXPitch;
	}
}

void
Renderer::setDrawInfo(HDC hDC, const DrawInfo* pDrawInfo)
{
	assert(pDrawInfo);

	prepareDC(hDC);

	render();
}


DCBuffer::DCBuffer(HDC hDC, const DrawInfo* pDrawInfo)
	: BGBuffer(),
	  Renderer(hDC, pDrawInfo, WIDTH_PER_XPITCH, HEIGHT_PER_YPITCH),
	  m_bHasSelectedRegion(false),
	  m_nSelectedPos(-1), m_nSelectedSize(0)
{
	int nXPitch = m_pDrawInfo->m_FontInfo.getXPitch(),
		nYPitch = m_pDrawInfo->m_FontInfo.getYPitch();

	RECT rct = m_rctDC;
	rct.right = nXPitch * 18;
	::FillRect(m_hDC, &rct, m_pDrawInfo->m_tciAddress.getBkBrush());
	rct.left  = rct.right;
	rct.right = nXPitch * (18 + 3 * 16 + 2 + 1);
	::FillRect(m_hDC, &rct, m_pDrawInfo->m_tciData.getBkBrush());
	rct.left  = rct.right;
	rct.right = m_rctDC.right;
	::FillRect(m_hDC, &rct, m_pDrawInfo->m_tciString.getBkBrush());
}

static void
TranslateToString(LPBYTE dst, const BYTE* src, int size)
{
	for (int i = 0; i < size; i++) {
		if (IsCharLeadByte((TCHAR)src[i])) {
			if (i == size - 1 || !IsCharTrailByte((TCHAR)src[i + 1])) {
				dst[i] = '.';
			} else {
				dst[i] = src[i];
				i++;
				dst[i] = src[i];
			}
		} else if (!IsCharReadable((TCHAR)src[i]) ||
				   src[i] < 0x20 || (src[i] & 0x80)) {
			dst[i] = '.';
		} else {
			dst[i] = src[i];
		}
	}
}

int
DCBuffer::init(LargeFileReader& LFReader, filesize_t offset)
{
	BGBuffer::init(LFReader, offset);
	// この時点で m_DataBuf にデータが読み込まれている

	return render();
}

void
DCBuffer::uninit()
{
	if (m_bHasSelectedRegion) // 選択を解除
		invertRegionInBuffer(m_nSelectedPos, m_nSelectedSize);
	BGBuffer::uninit();
//	render(); // 背景色で塗りつぶし
}

void
DCBuffer::bitBlt(HDC hDC, int x, int y, int cx, int cy,
				 int sx, int sy) const
{
	::BitBlt(hDC, x, y, cx, cy, m_hDC, sx, sy, SRCCOPY);
	if (sx + cx > m_rctDC.right) {
		RECT rct;
		rct.left   = x + (m_rctDC.right - sx);
		rct.top    = y;
		rct.right  = x + cx;
		rct.bottom = y + cy;
		::FillRect(hDC, &rct, m_pDrawInfo->m_tciString.getBkBrush());
	}
}

int
DCBuffer::render()
{
	assert(m_pDrawInfo);

	m_bHasSelectedRegion = false;

	int nXPitch = m_pDrawInfo->m_FontInfo.getXPitch(),
		nYPitch = m_pDrawInfo->m_FontInfo.getYPitch();

#if 0
	RECT rct = m_rctDC;
	rct.right = nXPitch * 18;
	::FillRect(m_hDC, &rct, m_pDrawInfo->m_tciAddress.getBkBrush());
	rct.left  = rct.right;
	rct.right = nXPitch * (18 + 3 * 16 + 2 + 1);
	::FillRect(m_hDC, &rct, m_pDrawInfo->m_tciData.getBkBrush());
	rct.left  = rct.right;
	rct.right = m_rctDC.right;
	::FillRect(m_hDC, &rct, m_pDrawInfo->m_tciString.getBkBrush());
#endif

	// バッファのデータは不正
	if (m_qAddress < 0) return 0;

	int i, linenum = m_nDataSize >> 4, xoffset, yoffset;
	TCHAR linebuf[WIDTH_PER_XPITCH + 1], strbuf[17];

	// アドレス
	m_pDrawInfo->m_tciAddress.setColorToDC(m_hDC);
	DoubleToStr((UINT)(m_qAddress >> 32), linebuf);
	UINT addr = (UINT)m_qAddress;
	for (i = 0; i < linenum; i++) {
		DoubleToStr(addr, linebuf + 8);
		::ExtTextOut(m_hDC, nXPitch, i * nYPitch, ETO_OPAQUE, NULL,
					 linebuf, 16, m_anXPitch);
		addr += 16;
	}
	if (m_nDataSize & 15) {
		DoubleToStr(addr, linebuf + 8);
		::ExtTextOut(m_hDC, nXPitch, linenum * nYPitch, ETO_OPAQUE, NULL,
					 linebuf, 16, m_anXPitch);
	}

	// データ
	m_pDrawInfo->m_tciData.setColorToDC(m_hDC);
	for (i = 0; i < linenum; i++) {
		int idx_top = (i << 4), j = 0;
		xoffset = nXPitch * 19;
		yoffset = i * nYPitch;
		linebuf[2] = 0;
		while (j < 8) {
			BYTE data = m_DataBuf[idx_top + j++];
			linebuf[0] = hex[(data >> 4) & 0x0F];
			linebuf[1] = hex[(data >> 0) & 0x0F];
			::ExtTextOut(m_hDC, xoffset, yoffset, ETO_OPAQUE, NULL,
						 linebuf, 2, m_anXPitch);
			xoffset += nXPitch * 3;
		}
		::ExtTextOut(m_hDC, xoffset, yoffset, ETO_OPAQUE, NULL,
					 "-", 1, m_anXPitch);
		xoffset += nXPitch * 2;
		while (j < 16) {
			BYTE data = m_DataBuf[idx_top + j++];
			linebuf[0] = hex[(data >> 4) & 0x0F];
			linebuf[1] = hex[(data >> 0) & 0x0F];
			::ExtTextOut(m_hDC, xoffset, yoffset, ETO_OPAQUE, NULL,
						 linebuf, 2, m_anXPitch);
			xoffset += nXPitch * 3;
		}
	}
	if (m_nDataSize & 15) {
		int idx_top = (linenum << 4), j = 0;
		xoffset = nXPitch * 19;
		yoffset = linenum * nYPitch;
		linebuf[2] = 0;
		while (j < 8) {
			BYTE data = m_DataBuf[idx_top + j];
			linebuf[0] = hex[(data >> 4) & 0x0F];
			linebuf[1] = hex[(data >> 0) & 0x0F];
			::ExtTextOut(m_hDC, xoffset, yoffset, ETO_OPAQUE, NULL,
						 linebuf, 2, m_anXPitch);
			xoffset += nXPitch * 3;
			if (idx_top + ++j >= m_nDataSize) goto _data_end;
		}
		::ExtTextOut(m_hDC, xoffset, yoffset, ETO_OPAQUE, NULL,
					 "-", 1, m_anXPitch);
		xoffset += nXPitch * 2;
		while (j < 16) {
			BYTE data = m_DataBuf[idx_top + j];
			linebuf[0] = hex[(data >> 4) & 0x0F];
			linebuf[1] = hex[(data >> 0) & 0x0F];
			::ExtTextOut(m_hDC, xoffset, yoffset, ETO_OPAQUE, NULL,
						 linebuf, 2, m_anXPitch);
			xoffset += nXPitch * 3;
			if (idx_top + ++j >= m_nDataSize) goto _data_end;
		}
_data_end:
		/* jump point */;
	}

	// 文字列
	xoffset = nXPitch * (1 + 16 + 2 + 8 * 3 + 2 + 8 * 3 + 1);
	m_pDrawInfo->m_tciString.setColorToDC(m_hDC);
	for (i = 0; i < linenum; i++) {
		TranslateToString((BYTE*)strbuf, m_DataBuf + (i << 4), 16);
		::ExtTextOut(m_hDC, xoffset, i * nYPitch, ETO_OPAQUE, NULL,
					 strbuf, 16, m_anXPitch);
	}
	if (m_nDataSize & 0x0F) {
		TranslateToString((BYTE*)strbuf, m_DataBuf + (linenum << 4),
						  m_nDataSize & 0x0F);
		::ExtTextOut(m_hDC, xoffset, linenum * nYPitch, ETO_OPAQUE, NULL,
					 strbuf, m_nDataSize & 0x0F, m_anXPitch);
	}

	return m_nDataSize;
}

void
DCBuffer::invertOneLineRegion(int column, int lineno, int n_char)
{
	assert(column < 16 && column + n_char <= 16);

	RECT rect;
	int nXPitch = m_pDrawInfo->m_FontInfo.getXPitch(),
		nYPitch = m_pDrawInfo->m_FontInfo.getYPitch();

	rect.top = lineno * nYPitch;
	rect.bottom = rect.top + nYPitch - 1;

	// 文字列表現部
	rect.left  = (1 + 16 + 2 + 16 * 3 + 2 + 1 + column) * nXPitch;
	rect.right = rect.left + n_char * nXPitch;

	::InvertRect(m_hDC, &rect);

	// データ本体
	int column_end = column + n_char;
	rect.left  = (1 + 16 + 2 + column * 3 + (column >= 8 ? 2 : 0)) * nXPitch;
	rect.right = (1 + 16 + 2 + column_end * 3 + (column_end > 8 ? 2 : 0) - 1)
				  * nXPitch;

	::InvertRect(m_hDC, &rect);
}

void
DCBuffer::invertRegionInBuffer(int offset, int size)
{
	m_nSelectedPos  = offset;
	m_nSelectedSize = size;

	int b_x = offset & 15, b_y = offset >> 4;
	
	if (b_x + size <= 16) {
		// 同一行内
		invertOneLineRegion(b_x, b_y, size);
	} else {
		// 複数行

		// 最初の行の最後まで選択
		invertOneLineRegion(b_x, b_y, 16 - b_x);

		// 途中の行
		offset += size;
		int e_x = offset & 15, e_y = offset >> 4;
		while (++b_y < e_y) {
			invertOneLineRegion(0, b_y, 16);
		}

		// 最終行
		if (e_x > 0) {
			invertOneLineRegion(0, e_y, e_x);
		}
	}
}

void
DCBuffer::invertRegion(filesize_t pos, int size, bool bSelected)
{
//	if (m_bHasSelectedRegion == bSelected) return;

	if (m_qAddress >= pos + size ||
		m_qAddress + MAX_DATASIZE_PER_BUFFER <= pos)
		return;

	m_bHasSelectedRegion = bSelected;

	int offset = (int)(pos - m_qAddress);

	if (offset < 0) offset = 0;
	if (offset + size > MAX_DATASIZE_PER_BUFFER)
		size = MAX_DATASIZE_PER_BUFFER - offset;

	invertRegionInBuffer(offset, size);
}

Header::Header(HDC hDC, const DrawInfo* pDrawInfo)
	: Renderer(hDC, pDrawInfo, WIDTH_PER_XPITCH, 1)
{
	m_pDrawInfo->m_tciHeader.setColorToDC(m_hDC);
	render();
}

void
Header::setDrawInfo(HDC hDC, const DrawInfo* pDrawInfo)
{
	m_pDrawInfo = pDrawInfo;
	pDrawInfo->m_tciHeader.setColorToDC(m_hDC);
	Renderer::setDrawInfo(hDC, pDrawInfo);
}

void
Header::bitBlt(HDC hDC, int x, int y, int cx, int cy,
			   int sx, int sy) const
{
	::BitBlt(hDC, x, y, cx, cy, m_hDC, sx, sy, SRCCOPY);
	if (sx + cx > m_rctDC.right) {
		RECT rct;
		rct.left   = x + (m_rctDC.right - sx);
		rct.top    = y;
		rct.right  = x + cx;
		rct.bottom = y + cy;
		::FillRect(hDC, &rct, m_pDrawInfo->m_tciHeader.getBkBrush());
	}
}

int
Header::render()
{
	assert(m_pDrawInfo);

	::FillRect(m_hDC, &m_rctDC, m_pDrawInfo->m_tciHeader.getBkBrush());

	int nXPitch = m_pDrawInfo->m_FontInfo.getXPitch(),
		nYPitch = m_pDrawInfo->m_FontInfo.getYPitch();

	::ExtTextOut(m_hDC, nXPitch, 0, 0, NULL,
				 "0000000000000000", 16, m_anXPitch);

	int i = 0, xoffset = nXPitch * 19;
	char buf[3];
	buf[2];
	while (i < 8) {
		buf[0] = hex[(i >> 4) & 0x0F];
		buf[1] = hex[(i >> 0) & 0x0F];
		::ExtTextOut(m_hDC, xoffset, 0, 0, NULL, buf, 2, m_anXPitch);
		xoffset += nXPitch * 3;
		i++;
	}
	::ExtTextOut(m_hDC, xoffset, 0, 0, NULL, "-", 1, m_anXPitch);
	xoffset += nXPitch * 2;
	while (i < 16) {
		buf[0] = hex[(i >> 4) & 0x0F];
		buf[1] = hex[(i >> 0) & 0x0F];
		::ExtTextOut(m_hDC, xoffset, 0, 0, NULL, buf, 2, m_anXPitch);
		xoffset += nXPitch * 3;
		i++;
	}
	xoffset += nXPitch;
	::ExtTextOut(m_hDC, xoffset, 0, 0, NULL, hex, 16, m_anXPitch);

	return 0; // dummy
}

DC_Manager::DC_Manager(HDC hDC, const DrawInfo* pDrawInfo,
					   LargeFileReader* pLFReader)
	: BGB_Manager(pLFReader),
	  m_pDrawInfo(pDrawInfo),
	  m_Header(hDC, pDrawInfo)
{
//	assert(m_hDC && m_pDrawInfo);
}

int
DC_Manager::getXPositionByCoordinate(int x)
{
	int nXPitch = m_pDrawInfo->m_FontInfo.getXPitch();

	int x_offset = 0;
	if (x >= nXPitch * 19 && x < nXPitch * (19 + 3 * 8 - 1)) {
		// left half
		int i;
		for (i = 0; i < 8; i++) {
			int xx = nXPitch * (19 + i * 3);
			if (x < xx) {
				// not in a region of number
				return -1;
			} else if (x < xx + nXPitch * 2) {
				// in a region of number
				return i;
			}
		}
	} else if (x >= nXPitch * (19 + 3 * 8 + 2) &&
			   x < nXPitch * (19 + 3 * 16 + 2 - 1)) {
		// right half
		int i;
		for (i = 0; i < 8; i++) {
			int xx = nXPitch * (19 + 3 * 8 + 2 + i * 3);
			if (x < xx) {
				// not in a region of number
				return -1;
			} else if (x < xx + nXPitch * 2) {
				// in a region of number
				return 8 + i;
			}
		}
	} else if (x >= nXPitch * (19 + 3 * 16 + 2 + 1) &&
			   x < nXPitch * (19 + 3 * 16 + 2 + 1 + 16)) {
		// string region
		return (x - nXPitch * (19 + 3 * 16 + 2 + 1)) / nXPitch;
	}

	return -1;
}

void
DC_Manager::setDrawInfo(HDC hDC, const DrawInfo* pDrawInfo)
{
	assert(pDrawInfo);

	m_pDrawInfo = pDrawInfo;

	m_Header.setDrawInfo(hDC, pDrawInfo);

	// リングバッファの要素についても変更を伝播
	if (m_bRBInit) {
		int count = m_rbBuffers.count();
		for (int i = 0; i < count; i++) {
			DCBuffer* ptr = static_cast<DCBuffer*>(m_rbBuffers.elementAt(i));
			ptr->setDrawInfo(hDC, pDrawInfo);
		}
	}
}

BGBuffer*
DC_Manager::createBGBufferInstance()
{
	return new DCBuffer(m_Header.m_hDC, m_pDrawInfo);
}

