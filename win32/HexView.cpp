// $Id$

#pragma warning(disable : 4786)

#define _WIN32_WINNT  0x500  // to support mouse wheel

#include "HexView.h"
#include "strutils.h"

#define MAX_DATASIZE_PER_BUFFER  1024 // 1KB
#define BUFFER_NUM  3

#define ADDR_WIDTH   100
#define BYTE_WIDTH    32
#define DATA_WIDTH   160

#define W_WIDTH  (ADDR_WIDTH + BYTE_WIDTH * 16 + 20 + DATA_WIDTH + 20)
#define W_HEIGHT 720

static void
TranslateToString(LPBYTE dst, const BYTE* src, int size)
{
	for (int i = 0; i < size; i++) {
		if (IsCharLeadByte((CHAR)src[i])) {
			if (i == size - 1 || !IsCharTrailByte((CHAR)src[i + 1])) {
				dst[i] = '.';
			} else {
				dst[i] = src[i];
				i++;
				dst[i] = src[i];
			}
		} else if (!IsCharReadable((CHAR)src[i]) ||
				   src[i] < 0x20 || (src[i] & 0x80)) {
			dst[i] = '.';
		} else {
			dst[i] = src[i];
		}
	}
}

HV_DCBuffer::HV_DCBuffer(int nBufSize)
	: DCBuffer(nBufSize),
	  m_pHVDrawInfo(NULL)
{
}

bool
HV_DCBuffer::prepareDC(DrawInfo* pDrawInfo)
{
	HV_DrawInfo* pHVDrawInfo = dynamic_cast<HV_DrawInfo*>(pDrawInfo);
	if (!pHVDrawInfo) {
		return false;
	}

	if (!DCBuffer::prepareDC(pDrawInfo)) {
		return false;
	}

	m_pHVDrawInfo = pHVDrawInfo;

	::SelectObject(m_hDC, (HGDIOBJ)m_pHVDrawInfo->m_FontInfo.getFont());

	int nXPitch = m_pHVDrawInfo->m_FontInfo.getXPitch();
	for (int i = 0; i < sizeof(m_anXPitch) / sizeof(m_anXPitch[0]); i++) {
		m_anXPitch[i] = nXPitch;
	}

	return true;
}

int
HV_DCBuffer::render()
{
	assert(m_pHVDrawInfo);

	int nXPitch = m_pHVDrawInfo->m_FontInfo.getXPitch(),
		nYPitch = m_pHVDrawInfo->m_FontInfo.getYPitch();

#if 1
	RECT rct;
	rct.left = rct.top = 0;
	rct.bottom = m_nHeight;
	rct.right = nXPitch * ADDRESS_REGION_END_OFFSET;
	::FillRect(m_hDC, &rct, m_pHVDrawInfo->m_tciAddress.getBkBrush());
	rct.left  = rct.right;
	rct.right = nXPitch * DATA_REGION_END_OFFSET;
	::FillRect(m_hDC, &rct, m_pHVDrawInfo->m_tciData.getBkBrush());
	rct.left  = rct.right;
	rct.right = m_nWidth;
	::FillRect(m_hDC, &rct, m_pHVDrawInfo->m_tciString.getBkBrush());
#endif

	// �o�b�t�@�̃f�[�^�͕s��
	if (m_qAddress < 0) {
		return 0;
	}

	int i, linenum = m_nDataSize >> 4, xoffset, yoffset;
	CHAR linebuf[STRING_END_OFFSET + 2], strbuf[17];

	// �A�h���X
	m_pHVDrawInfo->m_tciAddress.setColorToDC(m_hDC);
	DwordToStr((UINT)(m_qAddress >> 32), linebuf);
	UINT addr = (UINT)m_qAddress;
	for (i = 0; i < linenum; i++) {
		DwordToStr(addr, linebuf + 8);
		::ExtTextOut(m_hDC, nXPitch * ADDRESS_START_OFFSET, i * nYPitch,
					 ETO_OPAQUE, NULL,
					 linebuf, 16, m_anXPitch);
		addr += 16;
	}
	if (m_nDataSize & 0x0F) {
		DwordToStr(addr, linebuf + 8);
		::ExtTextOut(m_hDC, nXPitch * ADDRESS_START_OFFSET, linenum * nYPitch,
					 ETO_OPAQUE, NULL,
					 linebuf, 16, m_anXPitch);
	}

	// �f�[�^
	m_pHVDrawInfo->m_tciData.setColorToDC(m_hDC);
	for (i = 0; i < linenum; i++) {
		int idx_top = (i << 4), j = 0;
		xoffset = nXPitch * DATA_FORMAR_START_OFFSET;
		yoffset = i * nYPitch;
		linebuf[2] = 0;
		while (j < 8) {
			BYTE data = m_pDataBuf[idx_top + j++];
			linebuf[0] = hex[(data >> 4) & 0x0F];
			linebuf[1] = hex[(data >> 0) & 0x0F];
			::ExtTextOut(m_hDC, xoffset, yoffset, ETO_OPAQUE, NULL,
						 linebuf, 2, m_anXPitch);
			xoffset += nXPitch * (DATA_HEX_WIDTH + 1);
		}
		::ExtTextOut(m_hDC, xoffset, yoffset, ETO_OPAQUE, NULL,
					 "-", 1, m_anXPitch);
		xoffset += nXPitch * (DATA_CENTER_WIDTH - 1);
		while (j < 16) {
			BYTE data = m_pDataBuf[idx_top + j++];
			linebuf[0] = hex[(data >> 4) & 0x0F];
			linebuf[1] = hex[(data >> 0) & 0x0F];
			::ExtTextOut(m_hDC, xoffset, yoffset, ETO_OPAQUE, NULL,
						 linebuf, 2, m_anXPitch);
			xoffset += nXPitch * (DATA_HEX_WIDTH + 1);
		}
	}
	if (m_nDataSize & 0x0F) {
		int idx_top = (linenum << 4), j = 0;
		xoffset = nXPitch * DATA_FORMAR_START_OFFSET;
		yoffset = linenum * nYPitch;
		linebuf[2] = 0;
		while (j < 8) {
			BYTE data = m_pDataBuf[idx_top + j];
			linebuf[0] = hex[(data >> 4) & 0x0F];
			linebuf[1] = hex[(data >> 0) & 0x0F];
			::ExtTextOut(m_hDC, xoffset, yoffset, ETO_OPAQUE, NULL,
						 linebuf, 2, m_anXPitch);
			xoffset += nXPitch * (DATA_HEX_WIDTH + 1);
			if (idx_top + ++j >= m_nDataSize) goto _data_end;
		}
		::ExtTextOut(m_hDC, xoffset, yoffset, ETO_OPAQUE, NULL,
					 "-", 1, m_anXPitch);
		xoffset += nXPitch * (DATA_CENTER_WIDTH - 1);
		while (j < 16) {
			BYTE data = m_pDataBuf[idx_top + j];
			linebuf[0] = hex[(data >> 4) & 0x0F];
			linebuf[1] = hex[(data >> 0) & 0x0F];
			::ExtTextOut(m_hDC, xoffset, yoffset, ETO_OPAQUE, NULL,
						 linebuf, 2, m_anXPitch);
			xoffset += nXPitch * (DATA_HEX_WIDTH + 1);
			if (idx_top + ++j >= m_nDataSize) goto _data_end;
		}
_data_end:
		/* jump point */;
	}

	// ������
	xoffset = nXPitch * STRING_START_OFFSET;
	m_pHVDrawInfo->m_tciString.setColorToDC(m_hDC);
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

int
HV_DCBuffer::setCursorByCoordinate(int x, int y)
{
	// �o�b�t�@�̃f�[�^�͕s��
	if (m_qAddress < 0) {
		return -1;
	}

	assert(m_pHVDrawInfo);

	int nXPitch = m_pHVDrawInfo->m_FontInfo.getXPitch(),
		nYPitch = m_pHVDrawInfo->m_FontInfo.getYPitch();

	int line = y / nYPitch, column = -1;

	if (x >= nXPitch * DATA_FORMAR_START_OFFSET &&
		x < nXPitch * DATA_FORMAR_END_OFFSET) {
		// �f�[�^�̈�̑O������
		int w = (x - nXPitch * DATA_FORMAR_START_OFFSET) / nXPitch;
		if (w % (DATA_HEX_WIDTH + 1) != DATA_HEX_WIDTH) {
			column = w / (DATA_HEX_WIDTH + 1);
		}
	} else if (x >= nXPitch * DATA_LATTER_START_OFFSET &&
			   x < nXPitch * DATA_LATTER_END_OFFSET) {
		// �f�[�^�̈�̌㔼����
		int w = (x - nXPitch * DATA_LATTER_START_OFFSET) / nXPitch;
		if (w % (DATA_HEX_WIDTH + 1) != DATA_HEX_WIDTH) {
			column = w / (DATA_HEX_WIDTH + 1) + 8;
		}
	} else if (x >= nXPitch * STRING_START_OFFSET &&
			   x < nXPitch * STRING_END_OFFSET) {
		// ������̈�
		column = (x - nXPitch * STRING_START_OFFSET) / nXPitch;
	}

	if (column < 0) {
		return -1;
	}

	// �f�[�^�̈�ƕ�����̈�ɃJ�[�\�����Z�b�g����
	invertRegionInBuffer(column + line * 16, 1);

	return column + line * 16;
}

void
HV_DCBuffer::invertRegionInBuffer(int offset, int size)
{
	// �o�b�t�@�̃f�[�^�͕s��
	if (m_qAddress < 0) {
		return;
	}

	int start_line = offset / 16, start_column = offset % 16,
		end_line = (offset + size) / 16, end_column = (offset + size) % 16;

	if (end_column == 0) {
		assert(end_line > 0);
		end_line--;
		end_column = 16;
	}

	if (start_line == end_line) {
		invertOneLineRegion(start_column, end_column, start_line);
		return;
	}

	if (start_column > 0) {
		invertOneLineRegion(start_column, 16, start_line);
		start_line++;
	}

	for (int i = start_line; i < end_line; i++) {
		invertOneLineRegion(0, 16, i);
	}

	invertOneLineRegion(0, end_column, end_line);
}

void
HV_DCBuffer::invertOneLineRegion(int start_column, int end_column, int line)
{
	assert(m_pHVDrawInfo && start_column < end_column && line >= 0);

	int nXPitch = m_pHVDrawInfo->m_FontInfo.getXPitch(),
		nYPitch = m_pHVDrawInfo->m_FontInfo.getYPitch();

	RECT rctRegion;
	rctRegion.top    = line * nYPitch;
	rctRegion.bottom = (line + 1) * nYPitch;

	// 16�i���̗̈�
	if (end_column <= 8) {
		// �̈悪�������Ɏ��܂��Ă���
		rctRegion.left   = (DATA_FORMAR_START_OFFSET + start_column * (DATA_HEX_WIDTH + 1)) * nXPitch;
		rctRegion.right  = (DATA_FORMAR_START_OFFSET + end_column * (DATA_HEX_WIDTH + 1) - 1) * nXPitch;
		::InvertRect(m_hDC, &rctRegion);
	} else if (start_column >= 8) {
		// �̈悪�E�����Ɏ��܂��Ă���
		rctRegion.left   = (DATA_LATTER_START_OFFSET + (start_column - 8) * (DATA_HEX_WIDTH + 1)) * nXPitch;
		rctRegion.right  = (DATA_LATTER_START_OFFSET + (end_column - 8) * (DATA_HEX_WIDTH + 1) - 1) * nXPitch;
		::InvertRect(m_hDC, &rctRegion);
	} else {
		// �̈悪���E�����ɂ܂������Ă���
		rctRegion.left   = (DATA_FORMAR_START_OFFSET + start_column * (DATA_HEX_WIDTH + 1)) * nXPitch;
		rctRegion.right  = (DATA_FORMAR_START_OFFSET + 7 * (DATA_HEX_WIDTH + 1) - 1) * nXPitch;
		::InvertRect(m_hDC, &rctRegion);
		rctRegion.left   = (DATA_LATTER_START_OFFSET) * nXPitch;
		rctRegion.right  = (DATA_LATTER_START_OFFSET + (end_column - 8) * (DATA_HEX_WIDTH + 1) - 1) * nXPitch;
		::InvertRect(m_hDC, &rctRegion);
	}

	// ������̈�
	rctRegion.left   = (STRING_START_OFFSET + start_column) * nXPitch;
	rctRegion.right  = (STRING_START_OFFSET + end_column) * nXPitch;
	::InvertRect(m_hDC, &rctRegion);
}

HV_DCManager::HV_DCManager()
	: DC_Manager(MAX_DATASIZE_PER_BUFFER, BUFFER_NUM)
{
}

BGBuffer*
HV_DCManager::createBGBufferInstance()
{
	if (m_pDrawInfo == NULL) {
		return NULL;
	}
	DCBuffer* pBuf = new HV_DCBuffer(m_nBufSize);
	if (!pBuf->prepareDC(m_pDrawInfo)) {
		delete pBuf;
		return NULL;
	}
	return pBuf;
}

HexView::HexView(HWND hwndParent, const RECT& rctWindow, HV_DrawInfo* pDrawInfo)
	: View(hwndParent,
		   WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
		   WS_EX_CLIENTEDGE,
		   rctWindow,
		   new HV_DCManager(),
		   pDrawInfo)
{
}

HexView::~HexView()
{
}

bool
HexView::setDrawInfo(DrawInfo* pDrawInfo)
{
	HV_DrawInfo* pHVDrawInfo = dynamic_cast<HV_DrawInfo*>(pDrawInfo);
	if (!pHVDrawInfo) {
		return false;
	}

	if (!m_pDCManager->setDrawInfo(pDrawInfo)) {
		return false;
	}

	m_pDrawInfo = pHVDrawInfo;

	redrawView();

	return true;
}

LRESULT
HexView::viewWndProcMain(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return View::viewWndProcMain(uMsg, wParam, lParam);
}
