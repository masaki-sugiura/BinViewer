// $Id$

#pragma warning(disable : 4786)

#include "HexView.h"
#include "strutils.h"

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

HV_DCBuffer::HV_DCBuffer()
{
}

int
HV_DCBuffer::render()
{
}

int
HV_DCBuffer::render()
{
	assert(m_pDrawInfo);

	m_bHasSelectedRegion = false;

	int nXPitch = m_pDrawInfo->m_FontInfo.getXPitch(),
		nYPitch = m_pDrawInfo->m_FontInfo.getYPitch();

#if 1
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
	DwordToStr((UINT)(m_qAddress >> 32), linebuf);
	UINT addr = (UINT)m_qAddress;
	for (i = 0; i < linenum; i++) {
		DwordToStr(addr, linebuf + 8);
		::ExtTextOut(m_hDC, nXPitch, i * nYPitch, ETO_OPAQUE, NULL,
					 linebuf, 16, m_anXPitch);
		addr += 16;
	}
	if (m_nDataSize & 15) {
		DwordToStr(addr, linebuf + 8);
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
			BYTE data = m_pDataBuf[idx_top + j++];
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
			BYTE data = m_pDataBuf[idx_top + j++];
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
			BYTE data = m_pDataBuf[idx_top + j];
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
			BYTE data = m_pDataBuf[idx_top + j];
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
		TranslateToString((BYTE*)strbuf, m_pDataBuf + (i << 4), 16);
		::ExtTextOut(m_hDC, xoffset, i * nYPitch, ETO_OPAQUE, NULL,
					 strbuf, 16, m_anXPitch);
	}
	if (m_nDataSize & 0x0F) {
		TranslateToString((BYTE*)strbuf, m_pDataBuf + (linenum << 4),
						  m_nDataSize & 0x0F);
		::ExtTextOut(m_hDC, xoffset, linenum * nYPitch, ETO_OPAQUE, NULL,
					 strbuf, m_nDataSize & 0x0F, m_anXPitch);
	}

	return m_nDataSize;
}

