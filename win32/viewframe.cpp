// $Id$

#include "viewframe.h"

#include <assert.h>
#include <algorithm>

ViewFrame::ViewFrame(HWND hWnd, const DrawInfo* pDrawInfo,
					 LargeFileReader* pLFReader)
	: m_pDC_Manager(NULL), m_pDrawInfo(pDrawInfo)
{
	HDC hDC = ::GetDC(hWnd);

	initParams();

	m_pDC_Manager = new DC_Manager(hDC, m_pDrawInfo, pLFReader);

	m_nLineHeight = m_pDrawInfo->m_FontInfo.getYPitch();
	m_nCharWidth  = m_pDrawInfo->m_FontInfo.getXPitch();

	RECT rctClient;
	::GetClientRect(hWnd, &rctClient);
	setFrameRect(rctClient);

	::ReleaseDC(hWnd, hDC);
}

ViewFrame::~ViewFrame()
{
}

static inline bool
is_overlapped(int y_offset_line_num, int page_line_num)
{
	return (y_offset_line_num + page_line_num >= MAX_DATASIZE_PER_BUFFER / 16);
}

void
ViewFrame::setFrameRect(const RECT& rctClient)
{
	m_rctClient = rctClient;

	m_nPageLineNum = (m_rctClient.bottom - m_rctClient.top + m_nLineHeight - 1)
					  / m_nLineHeight - 1 /* ヘッダの分は除く */;

	if (!m_pDC_Manager->isLoaded()) return;

	// ウィンドウを広げた結果次のバッファとオーバーラップした場合に必要
	m_bOverlapped = is_overlapped(m_nTopOffset / m_nLineHeight, m_nPageLineNum);
}

void
ViewFrame::setCurrentLine(filesize_t newline)
{
	if (!m_pDC_Manager->isLoaded()) return;

	if (newline > m_qMaxLine)
		newline = m_qMaxLine;
	else if (newline < 0) newline = 0;

	m_qCurrentLine = newline;

	filesize_t buffer_top = m_pDC_Manager->setPosition(m_qCurrentLine * 16);

	m_nTopOffset = (int)(m_qCurrentLine - buffer_top / 16) * m_nLineHeight;
	assert(m_nTopOffset < HEIGHT_PER_YPITCH * m_nLineHeight);

	// 次のバッファとオーバーラップしているかどうか
	m_bOverlapped = is_overlapped(m_nTopOffset / m_nLineHeight, m_nPageLineNum);

	m_pCurBuf  = m_pDC_Manager->getCurrentBuffer(0);
	m_pNextBuf = m_pDC_Manager->getCurrentBuffer(1);
}

void
ViewFrame::setPosition(filesize_t pos)
{
	if (!m_pDC_Manager->isLoaded()) return;

	filesize_t fsize = this->getFileSize();
	if (fsize == 0) return;

	if (pos >= fsize)
		pos = fsize - 1;
	if (pos < 0) pos = 0; // else では NG!! (filesize == 0 の場合)

	m_qCurrentPos = pos;

	int top_offset_by_size = m_nTopOffset / m_nLineHeight * 16;
	if (!m_pCurBuf ||
		pos < m_pCurBuf->m_qAddress + top_offset_by_size ||
		pos >= m_pCurBuf->m_qAddress + top_offset_by_size + m_nPageLineNum * 16) {
		// 最初の描画またはカーソルが表示領域をはみ出る
		filesize_t newline = pos / 16;
		int line_diff = pos / 16 - m_qCurrentLine;
		if (line_diff >= m_nPageLineNum) {
			// 下にジャンプ/スクロール
			newline = newline - m_nPageLineNum + 1;
		}
		setCurrentLine(newline);
	}

	unselect();
	select(pos, 1);
}

void
ViewFrame::setPositionByCoordinate(const POINTS& pos)
{
	// header
	if (pos.y < m_nLineHeight) return;
	int y_pos = pos.y - m_nLineHeight + m_nTopOffset;
	int height = HEIGHT_PER_YPITCH * m_nLineHeight;

	filesize_t qPos = 0;
	if (!m_bOverlapped || y_pos < height) {
		// in m_pCurBuf
		qPos = m_pCurBuf->m_qAddress + (y_pos / m_nLineHeight) * 16;
	} else {
		// in m_pNextBuf
		qPos = m_pNextBuf->m_qAddress + ((y_pos - height) / m_nLineHeight) * 16;
	}

	int x_pos = m_pDC_Manager->getXPositionByCoordinate(pos.x + m_nXOffset);
	if (x_pos < 0) return;

	qPos += x_pos;

	if (qPos >= this->getFileSize()) {
		qPos = this->getFileSize() - 1;
		if (qPos < 0) qPos = 0;
	}

	this->setPosition(qPos);
}

void
ViewFrame::bitBlt(HDC hDC, const RECT& rcPaint)
{
	int width = WIDTH_PER_XPITCH * m_nCharWidth - m_nXOffset;
	if (m_bOverlapped) {
		int height = HEIGHT_PER_YPITCH * m_nLineHeight - m_nTopOffset;

		assert(m_pCurBuf);

		if (rcPaint.bottom - m_nLineHeight <= height) {
			::BitBlt(hDC,
					 rcPaint.left, rcPaint.top,
					 rcPaint.right - rcPaint.left,
					 rcPaint.bottom - rcPaint.top,
					 m_pCurBuf->m_hDC,
					 rcPaint.left + m_nXOffset,
					 m_nTopOffset + rcPaint.top - m_nLineHeight,
					 SRCCOPY);
		} else if (rcPaint.top - m_nLineHeight >= height) {
			if (m_pNextBuf) {
				::BitBlt(hDC,
						 rcPaint.left, rcPaint.top,
						 rcPaint.right - rcPaint.left,
						 rcPaint.bottom - rcPaint.top,
						 m_pNextBuf->m_hDC,
						 rcPaint.left + m_nXOffset,
						 rcPaint.top - height - m_nLineHeight,
						 SRCCOPY);
			} else {
				::FillRect(hDC, &rcPaint, m_pDrawInfo->m_tciData.getBkBrush());
			}
		} else {
			::BitBlt(hDC,
					 rcPaint.left, rcPaint.top,
					 rcPaint.right - rcPaint.left,
					 height - rcPaint.top + m_nLineHeight,
					 m_pCurBuf->m_hDC,
					 rcPaint.left + m_nXOffset,
					 rcPaint.top + m_nTopOffset - m_nLineHeight,
					 SRCCOPY);
			if (m_pNextBuf) {
				::BitBlt(hDC,
						 rcPaint.left, height + m_nLineHeight,
						 rcPaint.right - rcPaint.left,
						 rcPaint.bottom - height,
						 m_pNextBuf->m_hDC,
						 rcPaint.left + m_nXOffset, 0,
						 SRCCOPY);
			} else {
				RECT rctTemp = rcPaint;
				rctTemp.top    = height + m_nLineHeight;
				rctTemp.bottom = rcPaint.bottom;
				::FillRect(hDC, &rctTemp, m_pDrawInfo->m_tciData.getBkBrush());
			}
		}
		if (rcPaint.right > width) {
			RECT rctTemp = rcPaint;
			rctTemp.left  = max(rcPaint.left, width);
			::FillRect(hDC, &rctTemp,
					   m_pDrawInfo->m_tciData.getBkBrush());
		}
	} else if (m_pCurBuf) {
		::BitBlt(hDC,
				 rcPaint.left, rcPaint.top,
				 rcPaint.right - rcPaint.left,
				 rcPaint.bottom - rcPaint.top,
				 m_pCurBuf->m_hDC,
				 rcPaint.left + m_nXOffset,
				 m_nTopOffset + rcPaint.top - m_nLineHeight,
				 SRCCOPY);
		if (rcPaint.right > width) {
			RECT rctTemp = rcPaint;
			rctTemp.left  = max(rcPaint.left, width);
			::FillRect(hDC, &rctTemp,
					   m_pDrawInfo->m_tciData.getBkBrush());
		}
	} else {
		::FillRect(hDC, &rcPaint, (HBRUSH)(COLOR_APPWORKSPACE + 1));
	}

	// bitblt header
	if (rcPaint.top < m_nLineHeight &&
		rcPaint.left < width) {
		::BitBlt(hDC, rcPaint.left, rcPaint.top,
				 min(rcPaint.right, width) - rcPaint.left,
				 min(m_nLineHeight, rcPaint.bottom - rcPaint.top),
				 m_pDC_Manager->getHeaderDC(),
				 rcPaint.left + m_nXOffset, rcPaint.top,
				 SRCCOPY);
	}
}

void
ViewFrame::invertOneLineRegion(HDC hDC, int column, int lineno, int n_char)
{
	assert(column < 16 && column + n_char <= 16);

	RECT rect;

	rect.top = lineno * m_nLineHeight;
	rect.bottom = rect.top + m_nLineHeight - 1;

	// 文字列表現部
	rect.left  = (1 + 16 + 1 + 16 * 3 + 2 + 1 + column) * m_nCharWidth;
	rect.right = rect.left + n_char * m_nCharWidth;

	::InvertRect(hDC, &rect);

	// データ本体
	int column_end = column + n_char;
	rect.left  = (1 + 16 + 1 + column * 3 + (column >= 8 ? 2 : 0)) * m_nCharWidth;
	rect.right = (1 + 16 + 1 + column_end * 3 + (column_end > 8 ? 2 : 0) - 1)
				  * m_nCharWidth;

	::InvertRect(hDC, &rect);
}

void
ViewFrame::invertOneBufferRegion(DCBuffer* pDCBuffer, filesize_t pos, int size)
{
	assert(pDCBuffer);

	if (pDCBuffer->m_qAddress >= pos + size ||
		pDCBuffer->m_qAddress + MAX_DATASIZE_PER_BUFFER <= pos)
		return;

	int offset = (int)(pos - pDCBuffer->m_qAddress);
	if (offset < 0) offset = 0;
	if (offset + size > MAX_DATASIZE_PER_BUFFER)
		size = MAX_DATASIZE_PER_BUFFER - offset;
	HDC hDC = pDCBuffer->m_hDC;

	int b_x = offset & 15, b_y = offset >> 4;
	
	if (b_x + size <= 16) {
		// 同一行内
		invertOneLineRegion(hDC, b_x, b_y, size);
	} else {
		// 複数行

		// 最初の行の最後まで選択
		invertOneLineRegion(hDC, b_x, b_y, 16 - b_x);

		// 途中の行
		offset += size;
		int e_x = offset & 15, e_y = offset >> 4;
		while (++b_y < e_y) {
			invertOneLineRegion(hDC, 0, b_y, 16);
		}

		// 最終行
		if (e_x > 0) {
			invertOneLineRegion(hDC, 0, e_y, e_x);
		}
	}
}

void
ViewFrame::invertRegion(filesize_t pos, int size)
{
	for (int i = -1; i < 2; i++) {
		DCBuffer* pDCBuffer = m_pDC_Manager->getCurrentBuffer(i);
		if (pDCBuffer) {
			invertOneBufferRegion(pDCBuffer, pos, size);
		}
	}
}

void
ViewFrame::select(filesize_t pos, int size)
{
	assert(pos >= 0 && size > 0);

	// unselect previously selected region
	unselect();

	invertRegion(pos, size);

	m_qPrevSelectedPos  = pos;
	m_nPrevSelectedSize = size;
}

void
ViewFrame::unselect()
{
	if (m_qPrevSelectedPos >= 0 && m_nPrevSelectedSize > 0) {
		invertRegion(m_qPrevSelectedPos, m_nPrevSelectedSize);
		m_qPrevSelectedPos  = -1;
		m_nPrevSelectedSize = 0;
	}
}

