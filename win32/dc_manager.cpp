// $Id$

#include "dc_manager.h"
#include "strutils.h"
#include <assert.h>

static const char* const hex = "0123456789ABCDEF";

Renderer::Renderer(HDC hDC,
				   const TextColorInfo* pTCInfo,
				   const FontInfo* pFontInfo,
				   int w_per_xpitch, int h_per_ypitch)
	: m_hDC(::CreateCompatibleDC(hDC)),
	  m_pTextColorInfo(pTCInfo),
	  m_pFontInfo(pFontInfo),
	  m_hBitmap(NULL),
	  m_nWidth_per_XPitch(w_per_xpitch),
	  m_nHeight_per_YPitch(h_per_ypitch)
{
	assert(hDC && pTCInfo && pFontInfo);
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
	int width  = m_pFontInfo->getXPitch() * m_nWidth_per_XPitch,
		height = m_pFontInfo->getYPitch() * m_nHeight_per_YPitch;

	if (m_hBitmap != NULL) ::DeleteObject(m_hBitmap);
	m_hBitmap = ::CreateCompatibleBitmap(hDC, width, height);

	::SelectObject(m_hDC, (HGDIOBJ)m_hBitmap);
	::SelectObject(m_hDC, (HGDIOBJ)m_pFontInfo->getFont());
	::SetTextColor(m_hDC, m_pTextColorInfo->getFgColor());
	::SetBkColor(m_hDC, m_pTextColorInfo->getBkColor());

	m_rctDC.left = m_rctDC.top = 0;
	m_rctDC.right  = width;
	m_rctDC.bottom = height;

	int nXPitch = m_pFontInfo->getXPitch();
	for (int i = 0; i < sizeof(m_anXPitch) / sizeof(m_anXPitch[0]); i++) {
		m_anXPitch[i] = nXPitch;
	}
}

void
Renderer::setDrawInfo(HDC hDC,
					  const TextColorInfo* pTCInfo,
					  const FontInfo* pFontInfo)
{
	assert(pTCInfo || pFontInfo);

	if (pFontInfo) {
		m_pFontInfo = pFontInfo;
		prepareDC(hDC);
	}
	if (pTCInfo) m_pTextColorInfo = pTCInfo;

	render();
}


DCBuffer::DCBuffer(HDC hDC,
				   const TextColorInfo* pTCInfo,
				   const FontInfo* pFontInfo)
	: BGBuffer(),
	  Renderer(hDC, pTCInfo, pFontInfo, WIDTH_PER_XPITCH, HEIGHT_PER_YPITCH)
{
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
		} else if (!IsCharReadable((TCHAR)src[i]) || (src[i] & 0x80)) {
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
	// ���̎��_�� m_DataBuf �Ƀf�[�^���ǂݍ��܂�Ă���

	return render();
}

void
DCBuffer::uninit()
{
	BGBuffer::uninit();
	render(); // �w�i�F�œh��Ԃ�
}

int
DCBuffer::render()
{
	assert(m_pTextColorInfo && m_pFontInfo);

	::FillRect(m_hDC, &m_rctDC, m_pTextColorInfo->getBkBrush());

	// �o�b�t�@�̃f�[�^�͕s��
	if (m_qAddress < 0) return 0;

	int nXPitch = m_pFontInfo->getXPitch(),
		nYPitch = m_pFontInfo->getYPitch();
	int linenum = m_nDataSize >> 4;
	int xoffset, yoffset;

	TCHAR linebuf[WIDTH_PER_XPITCH + 1], strbuf[17];
	for (int i = 0; i < linenum; i++) {
		yoffset = i * nYPitch;
		int idx_top = (i << 4);
#if 1
		filesize_t qCurAddress = m_qAddress + idx_top;
		wsprintf(linebuf, "%08X%08X", (int)(qCurAddress >> 32), (int)qCurAddress);
		::ExtTextOut(m_hDC, nXPitch, yoffset, 0, NULL, linebuf, 16, m_anXPitch);
		xoffset = nXPitch * 18;
		linebuf[2] = 0;
		int j = 0;
		while (j < 8) {
			BYTE data = m_DataBuf[idx_top + j++];
			linebuf[0] = hex[(data >> 4) & 0x0F];
			linebuf[1] = hex[(data >> 0) & 0x0F];
			::ExtTextOut(m_hDC, xoffset, yoffset, 0, NULL, linebuf, 2, m_anXPitch);
			xoffset += nXPitch * 3;
		}
		::ExtTextOut(m_hDC, xoffset, yoffset, 0, NULL, "-", 1, m_anXPitch);
		xoffset += nXPitch * 2;
		while (j < 16) {
			BYTE data = m_DataBuf[idx_top + j++];
			linebuf[0] = hex[(data >> 4) & 0x0F];
			linebuf[1] = hex[(data >> 0) & 0x0F];
			::ExtTextOut(m_hDC, xoffset, yoffset, 0, NULL, linebuf, 2, m_anXPitch);
			xoffset += nXPitch * 3;
		}
		xoffset += nXPitch;
		TranslateToString((BYTE*)strbuf, m_DataBuf + idx_top, 16);
		::ExtTextOut(m_hDC, xoffset, yoffset, 0, NULL, strbuf, 16, m_anXPitch);
#else
		TranslateToString((BYTE*)strbuf, m_DataBuf + idx_top, 16);
		strbuf[16] = '\0';
		filesize_t qCurAddress = m_qAddress + idx_top;
		wsprintf(linebuf,
				 " %08X%08X %02x %02x %02x %02x %02x %02x %02x %02x - %02x %02x %02x %02x %02x %02x %02x %02x %s",
				 (int)(qCurAddress >> 32),
				 (int)qCurAddress,
				 m_DataBuf[idx_top +  0],
				 m_DataBuf[idx_top +  1],
				 m_DataBuf[idx_top +  2],
				 m_DataBuf[idx_top +  3],
				 m_DataBuf[idx_top +  4],
				 m_DataBuf[idx_top +  5],
				 m_DataBuf[idx_top +  6],
				 m_DataBuf[idx_top +  7],
				 m_DataBuf[idx_top +  8],
				 m_DataBuf[idx_top +  9],
				 m_DataBuf[idx_top + 10],
				 m_DataBuf[idx_top + 11],
				 m_DataBuf[idx_top + 12],
				 m_DataBuf[idx_top + 13],
				 m_DataBuf[idx_top + 14],
				 m_DataBuf[idx_top + 15],
				 strbuf);
		::TextOut(m_hDC, 0, yoffset, linebuf, WIDTH_PER_XPITCH);
#endif
	}

	if (m_nDataSize & 0x0F) {
		yoffset = linenum * nYPitch;
		filesize_t qCurAddress = m_qAddress + (linenum << 4);
		wsprintf(linebuf, "%08X%08X", (int)(qCurAddress >> 32), (int)qCurAddress);
		::ExtTextOut(m_hDC, nXPitch, yoffset, 0, NULL, linebuf, 16, m_anXPitch);
		xoffset = nXPitch * 18;
		linebuf[2] = 0;
		int idx_top = (linenum << 4), j = 0;
		while (j < 8) {
			if (idx_top + j < m_nDataSize) {
				BYTE data = m_DataBuf[idx_top + j++];
				linebuf[0] = hex[(data >> 4) & 0x0F];
				linebuf[1] = hex[(data >> 0) & 0x0F];
				::ExtTextOut(m_hDC, xoffset, yoffset, 0, NULL,
							 linebuf, 2, m_anXPitch);
			}
			xoffset += nXPitch * 3;
		}
		if (idx_top + 8 < m_nDataSize)
			::ExtTextOut(m_hDC, xoffset, yoffset, 0, NULL, "-", 1, m_anXPitch);
		xoffset += nXPitch * 2;
		while (j < 16) {
			if (idx_top + j < m_nDataSize) {
				BYTE data = m_DataBuf[idx_top + j];
				linebuf[0] = hex[(data >> 4) & 0x0F];
				linebuf[1] = hex[(data >> 0) & 0x0F];
				::ExtTextOut(m_hDC, xoffset, yoffset, 0, NULL,
							 linebuf, 2, m_anXPitch);
			}
			xoffset += nXPitch * 3;
			j++;
		}
		xoffset += nXPitch;
		TranslateToString((BYTE*)strbuf, m_DataBuf + idx_top, m_nDataSize & 0x0F);
		::ExtTextOut(m_hDC, xoffset, yoffset, 0, NULL,
					 strbuf, m_nDataSize & 0x0F, m_anXPitch);
//		linenum++;
	}

	return m_nDataSize;
}

Header::Header(HDC hDC,
			   const TextColorInfo* pTCInfo,
			   const FontInfo* pFontInfo)
	: Renderer(hDC, pTCInfo, pFontInfo, WIDTH_PER_XPITCH, 1)
{
	render();
}

int
Header::render()
{
	assert(m_pTextColorInfo && m_pFontInfo);

	::FillRect(m_hDC, &m_rctDC, m_pTextColorInfo->getBkBrush());

	int nXPitch = m_pFontInfo->getXPitch(),
		nYPitch = m_pFontInfo->getYPitch();

	::ExtTextOut(m_hDC, nXPitch, 0, 0, NULL,
				 "0000000000000000", 16, m_anXPitch);

	int i = 0, xoffset = nXPitch * 18;
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
	  m_Header(hDC, &pDrawInfo->m_tciHeader, &pDrawInfo->m_FontInfo)
{
//	assert(m_hDC && m_pDrawInfo);
}

int
DC_Manager::getXPositionByCoordinate(int x)
{
	int nXPitch = m_pDrawInfo->m_FontInfo.getXPitch();

	int x_offset = 0;
	if (x >= nXPitch * 18 && x < nXPitch * (18 + 3 * 8 - 1)) {
		// left half
		int i;
		for (i = 0; i < 8; i++) {
			int xx = nXPitch * (18 + i * 3);
			if (x < xx) {
				// not in a region of number
				return -1;
			} else if (x < xx + nXPitch * 2) {
				// in a region of number
				return i;
			}
		}
	} else if (x >= nXPitch * (18 + 3 * 8 + 2) &&
			   x < nXPitch * (18 + 3 * 16 + 2 - 1)) {
		// right half
		int i;
		for (i = 0; i < 8; i++) {
			int xx = nXPitch * (18 + 3 * 8 + 2 + i * 3);
			if (x < xx) {
				// not in a region of number
				return -1;
			} else if (x < xx + nXPitch * 2) {
				// in a region of number
				return 8 + i;
			}
		}
	}

	return -1;
}

void
DC_Manager::setDrawInfo(HDC hDC, const DrawInfo* pDrawInfo)
{
	assert(pDrawInfo);

	m_pDrawInfo = pDrawInfo;

	m_Header.setDrawInfo(hDC, &pDrawInfo->m_tciHeader, &pDrawInfo->m_FontInfo);

	// �����O�o�b�t�@�̗v�f�ɂ��Ă��ύX��`�d
	if (m_bRBInit) {
		int count = m_rbBuffers.count();
		for (int i = 0; i < count; i++) {
			DCBuffer* ptr = static_cast<DCBuffer*>(m_rbBuffers.elementAt(i));
			ptr->setDrawInfo(hDC, &pDrawInfo->m_tciData, &pDrawInfo->m_FontInfo);
		}
	}
}

BGBuffer*
DC_Manager::createBGBufferInstance()
{
	return new DCBuffer(m_Header.m_hDC,
						&m_pDrawInfo->m_tciData, &m_pDrawInfo->m_FontInfo);
}

