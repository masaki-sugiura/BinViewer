// $Id$

#include "dc_manager.h"
#include <assert.h>

DrawInfo::DrawInfo()
	: m_hDC(NULL),
	  m_nWidth(0),
	  m_nHeight(0),
	  m_nPixelsPerLine(0),
	  m_hbrBackground(NULL)
{
}

DrawInfo::~DrawInfo()
{
	if (m_hbrBackground) {
		::DeleteObject(m_hbrBackground);
	}
}


Renderer::Renderer()
	: m_hDC(NULL),
	  m_hBitmap(NULL),
	  m_nWidth(0), m_nHeight(0),
	  m_hbrBackground(NULL)
{
}

Renderer::~Renderer()
{
	::DeleteObject(m_hBitmap);
	::DeleteDC(m_hDC);
}

bool
Renderer::prepareDC(DrawInfo* pDrawInfo)
{
	if (pDrawInfo == NULL ||
		pDrawInfo->getDC() == NULL  ||
		pDrawInfo->getWidth() <= 0  ||
		pDrawInfo->getHeight() <= 0 ||
		pDrawInfo->getBkBrush() == NULL) {
		return false;
	}

	if (m_hBitmap != NULL) {
		::DeleteObject(m_hBitmap);
	}
	if (m_hDC != NULL) {
		::DeleteDC(m_hDC);
	}

	m_nWidth  = pDrawInfo->getWidth();
	m_nHeight = pDrawInfo->getHeight();

	m_hDC = ::CreateCompatibleDC(pDrawInfo->getDC());
	if (m_hDC == NULL) {
		return false;
	}

	m_hBitmap = ::CreateCompatibleBitmap(pDrawInfo->getDC(),
										 pDrawInfo->getWidth(),
										 pDrawInfo->getHeight());
	if (m_hBitmap == NULL) {
		return false;
	}

	HGDIOBJ hOrgBitmap = ::SelectObject(m_hDC, (HGDIOBJ)m_hBitmap);
	if (hOrgBitmap == NULL) {
		return false;
	}

	m_hbrBackground = pDrawInfo->getBkBrush();

	return true;
}


DCBuffer::DCBuffer(int nBufSize)
	: BGBuffer(nBufSize), Renderer(),
	  m_nCursorPos(-1),
	  m_nSelectedPos(-1), m_nSelectedSize(0)
{
	assert(nBufSize > 0);
}

int
DCBuffer::init(LargeFileReader& LFReader, filesize_t offset)
{
	BGBuffer::init(LFReader, offset);
	// この時点で m_DataBuf にデータが読み込まれている

	m_nCursorPos = -1;
	m_nSelectedPos = -1;
	m_nSelectedSize = 0;

	return render();
}

void
DCBuffer::uninit()
{
	if (hasCursor()) {
		// カーソルを消去
		setCursor(-1);
	}
	if (hasSelectedRegion()) {
		// 選択を解除
		select(-1, 0);
	}
	BGBuffer::uninit();
//	render(); // 背景色で塗りつぶし
}

void
DCBuffer::bitBlt(HDC hDC, int x, int y, int cx, int cy,
				 int sx, int sy) const
{
	::BitBlt(hDC, x, y, cx, cy, m_hDC, sx, sy, SRCCOPY);
	if (sx + cx > m_nWidth) {
		RECT rct;
		rct.left   = x + (m_nWidth - sx);
		rct.top    = y;
		rct.right  = x + cx;
		rct.bottom = y + cy;
		::FillRect(hDC, &rct, m_hbrBackground);
	}
}

void
DCBuffer::setCursor(int offset)
{
	if (offset < 0) {
		// カーソルの消去
		if (hasCursor()) {
			invertRegionInBuffer(m_nCursorPos, 1);
			m_nCursorPos = offset;
		}
	} else {
		// カーソルのセット
		if (!hasCursor() && offset < m_nDataSize) {
			m_nCursorPos = offset;
			invertRegionInBuffer(m_nCursorPos, 1);
		}
	}
}

void
DCBuffer::select(int offset, int size)
{
	if (offset < 0) {
		// 選択領域の消去
		if (hasSelectedRegion()) {
			invertRegionInBuffer(m_nSelectedPos, m_nSelectedSize);
			m_nSelectedPos = offset;
			m_nSelectedSize = size;
		}
	} else {
		// 選択領域のセット
		if (!hasSelectedRegion() &&
			offset + size <= m_nDataSize) {
			m_nSelectedPos = offset;
			m_nSelectedSize = size;
			invertRegionInBuffer(m_nSelectedPos, m_nSelectedSize);
		}
	}
}

void
DCBuffer::repaint()
{
	render();
	if (hasCursor()) {
		int offset = m_nCursorPos;
		m_nCursorPos = -1;
		setCursor(offset);
	}
	if (hasSelectedRegion()) {
		int offset = m_nSelectedPos, size = m_nSelectedSize;
		m_nSelectedPos = -1;
		m_nSelectedSize = 0;
		select(offset, size);
	}
}


DC_Manager::DC_Manager(int nBufSize, int nBufCount)
	: BGB_Manager(nBufSize, nBufCount),
	  m_nWidth(0), m_nHeight(0),
	  m_nXOffset(0), m_nYOffset(0),
	  m_nViewWidth(0), m_nViewHeight(0),
	  m_qYOffset(0),
	  m_qCursorPos(-1),
	  m_qStartSelected(-1),
	  m_qSelectedSize(0),
	  m_bOverlapped(false),
	  m_pCurBuf(NULL), m_pNextBuf(NULL)
{
}

bool
DC_Manager::onLoadFile(LF_Acceptor* pLFAcceptor)
{
	return BGB_Manager::onLoadFile(pLFAcceptor);
}

void
DC_Manager::onUnloadFile()
{
	BGB_Manager::onUnloadFile();
	m_nXOffset = m_nYOffset = 0;
	m_qYOffset = 0;
	m_qCursorPos = m_qStartSelected = -1;
	m_qSelectedSize = 0;
	m_bOverlapped = false;
	m_pCurBuf = m_pNextBuf = NULL;
}

bool
DC_Manager::setDrawInfo(DrawInfo* pDrawInfo)
{
	for (int i = getMinBufferIndex(); i <= getMaxBufferIndex(); i++) {
		DCBuffer* pBuf = static_cast<DCBuffer*>(m_rbBuffers.elementAt(i));
		if (!pBuf) continue;
		if (!pBuf->prepareDC(pDrawInfo)) {
			return false;
		}
		pBuf->repaint();
	}

	m_pDrawInfo = pDrawInfo;
	m_nWidth = pDrawInfo->getWidth();
	m_nHeight = pDrawInfo->getHeight();

	return true;
}

void
DC_Manager::setViewPosition(int nXOffset, filesize_t qYOffset)
{
	m_nXOffset = nXOffset;
	m_qYOffset = qYOffset;

	m_nYOffset = qYOffset - (qYOffset / m_nHeight) * m_nHeight;

	m_bOverlapped = m_nYOffset + m_nViewHeight > m_nHeight;

	filesize_t qCursorPos = m_qCursorPos;
	setCursor(-1);

	setPosition((qYOffset / m_nHeight) * m_nBufSize);

	m_pCurBuf  = getCurrentBuffer(0);
	m_pNextBuf = getCurrentBuffer(1);

	setCursor(qCursorPos);
}

void
DC_Manager::bitBlt(HDC hdcDst, const RECT& rcPaint)
{
	int nHeaderHeight = 0;

//	int nXOffset = m_smHorz.getCurrentPos();
	if (m_bOverlapped) {
		int height = m_nHeight - m_nYOffset;

		assert(m_pCurBuf);

		if (rcPaint.bottom - nHeaderHeight <= height) {
			m_pCurBuf->bitBlt(hdcDst,
							  rcPaint.left, rcPaint.top,
							  rcPaint.right - rcPaint.left,
							  rcPaint.bottom - rcPaint.top,
							  rcPaint.left + m_nXOffset,
							  m_nYOffset + rcPaint.top - nHeaderHeight);
		} else if (rcPaint.top - nHeaderHeight >= height) {
			if (m_pNextBuf) {
				m_pNextBuf->bitBlt(hdcDst,
								   rcPaint.left, rcPaint.top,
								   rcPaint.right - rcPaint.left,
								   rcPaint.bottom - rcPaint.top,
								   rcPaint.left + m_nXOffset,
								   rcPaint.top - height - nHeaderHeight);
			} else {
				::FillRect(hdcDst, &rcPaint, m_pDrawInfo->getBkBrush());
			}
		} else {
			m_pCurBuf->bitBlt(hdcDst,
							  rcPaint.left, rcPaint.top,
							  rcPaint.right - rcPaint.left,
							  height - rcPaint.top + nHeaderHeight,
							  rcPaint.left + m_nXOffset,
							  rcPaint.top + m_nYOffset - nHeaderHeight);
			if (m_pNextBuf) {
				m_pNextBuf->bitBlt(hdcDst,
								   rcPaint.left, height + nHeaderHeight,
								   rcPaint.right - rcPaint.left,
								   rcPaint.bottom - height,
								   rcPaint.left + m_nXOffset, 0);
			} else {
				RECT rctTemp = rcPaint;
				rctTemp.top    = height + nHeaderHeight;
				rctTemp.bottom = rcPaint.bottom;
				::FillRect(hdcDst, &rctTemp, m_pDrawInfo->getBkBrush());
			}
		}
	} else if (m_pCurBuf) {
		m_pCurBuf->bitBlt(hdcDst,
						  rcPaint.left, rcPaint.top,
						  rcPaint.right - rcPaint.left,
						  rcPaint.bottom - rcPaint.top,
						  rcPaint.left + m_nXOffset,
						  m_nYOffset + rcPaint.top - nHeaderHeight);
	} else {
		::FillRect(hdcDst, &rcPaint, (HBRUSH)(COLOR_APPWORKSPACE + 1));
	}
}

filesize_t
DC_Manager::setCursorByViewCoordinate(const POINTS& pt)
{
	filesize_t qYCoord = m_qYOffset + pt.y;
	filesize_t qByteOffset = (qYCoord / m_nHeight) * m_nBufSize;
	DCBuffer* pCurBuf = getBuffer(qByteOffset);
	if (!pCurBuf) return -1;

	// 既にカーソルを持っていた場合、それを一度消去
	setCursor(-1);

	int offset = pCurBuf->setCursorByCoordinate(pt.x, (int)(qYCoord % m_nHeight));

	return m_qCursorPos = qByteOffset + offset;
}

filesize_t
DC_Manager::getPositionByViewCoordinate(const POINTS& pt)
{
	filesize_t qYCoord = m_qYOffset + pt.y;
	filesize_t qByteOffset = (qYCoord / m_nHeight) * m_nBufSize;
	DCBuffer* pCurBuf = getBuffer(qByteOffset);
	if (!pCurBuf) return -1;

	int offset = pCurBuf->getPositionByCoordinate(pt.x, (int)(qYCoord % m_nHeight));

	return qByteOffset + offset;
}

void
DC_Manager::setCursor(filesize_t pos)
{
	if (!isLoaded()) return;

	filesize_t fsize = getFileSize();
	if (fsize <= pos) return;

	for (int i = getMinBufferIndex(); i <= getMaxBufferIndex(); i++) {
		DCBuffer* pBuf = static_cast<DCBuffer*>(m_rbBuffers.elementAt(i));
		if (!pBuf || pBuf->m_qAddress == -1) continue;
		// 既にカーソルを持っていた場合、それを一度消去
		if (pBuf->hasCursor()) {
			pBuf->setCursor(-1);
		}
		if (pBuf->m_qAddress <= pos &&
			pos < pBuf->m_qAddress + pBuf->m_nDataSize) {
			pBuf->setCursor((int)(pos - pBuf->m_qAddress));
//			break;
		}
	}
	m_qCursorPos = pos;
}

void
DC_Manager::select(filesize_t pos, filesize_t size)
{
	if (pos < 0 || size <= 0 || !isLoaded()) return;

	filesize_t fsize = getFileSize();
	if (fsize <= pos) return;

	if (pos + size > fsize) {
		size = fsize - pos;
	}

	for (int i = getMinBufferIndex(); i < getMaxBufferIndex(); i++) {
		DCBuffer* pBuf = static_cast<DCBuffer*>(m_rbBuffers.elementAt(i));
		if (!pBuf || pBuf->m_qAddress == -1) {
			continue;
		}
		// 選択領域を持っていた場合、一度それを消去
		if (pBuf->hasSelectedRegion()) {
			pBuf->select(-1, 0);
		}
		if (pBuf->m_qAddress >= pos + size ||
			pBuf->m_qAddress + pBuf->m_nDataSize <= pos) {
			continue;
		}
		filesize_t qHead = max(pBuf->m_qAddress, pos),
				   qTail = min(pBuf->m_qAddress + pBuf->m_nDataSize, pos + size);
		pBuf->select((int)(qHead - pBuf->m_qAddress), (int)(qTail - qHead));
	}

	m_qStartSelected = pos;
	m_qSelectedSize = size;
}

