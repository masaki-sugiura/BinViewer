// $Id$

#include "dc_manager.h"
#include <assert.h>

Renderer::Renderer(HDC hDC, int width, int height, HBRUSH hbrBackground)
	: m_hDC(::CreateCompatibleDC(hDC))
{
	assert(hDC);

	prepareDC(hDC, width, height, hbrBackground);
}

Renderer::~Renderer()
{
	::DeleteObject(m_hBitmap);
	::DeleteDC(m_hDC);
}

void
Renderer::prepareDC(HDC hDC, int width, int height, HBRUSH hbrBackground)
{
	if (m_hBitmap != NULL) ::DeleteObject(m_hBitmap);

	m_nWidth  = width;
	m_nHeight = height;

	m_hBitmap = ::CreateCompatibleBitmap(hDC, width, height);
	::SelectObject(m_hDC, (HGDIOBJ)m_hBitmap);

	m_hbrBackground = hbrBackground;
}


DCBuffer::DCBuffer(int nBufSize,
				   HDC hDC, int nWidth, int nHeight,
				   HBRUSH hbrBackground)
	: BGBuffer(nBufSize),
	  Renderer(hDC, nWidth, nHeight, hbrBackground),
	  m_bHasSelectedRegion(false),
	  m_nSelectedPos(-1), m_nSelectedSize(0)
{
}

int
DCBuffer::init(LargeFileReader& LFReader, filesize_t offset)
{
	BGBuffer::init(LFReader, offset);
	// ‚±‚ÌŽž“_‚Å m_DataBuf ‚Éƒf[ƒ^‚ª“Ç‚Ýž‚Ü‚ê‚Ä‚¢‚é

	return render();
}

void
DCBuffer::uninit()
{
	if (m_bHasSelectedRegion) // ‘I‘ð‚ð‰ðœ
		invertRegionInBuffer(m_nSelectedPos, m_nSelectedSize);
	BGBuffer::uninit();
//	render(); // ”wŒiF‚Å“h‚è‚Â‚Ô‚µ
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
DCBuffer::invertRegion(filesize_t pos, int size, bool bSelected)
{
//	if (m_bHasSelectedRegion == bSelected) return;

	if (m_qAddress >= pos + size ||
		m_qAddress + m_nBufSize <= pos)
		return;

	m_bHasSelectedRegion = bSelected;

	int offset = (int)(pos - m_qAddress);

	if (offset < 0) offset = 0;
	if (offset + size > m_nBufSize)
		size = m_nBufSize - offset;

	invertRegionInBuffer(offset, size);
}


DC_Manager::DC_Manager(int nBufSize, int nBufCount)
	: BGB_Manager(nBufSize, nBufCount),
	  m_hDC(NULL),
	  m_nWidth(0), m_nHeight(0),
	  m_nXOffset(0), m_nYOffset(0),
	  m_nViewWidth(0), m_nViewHeight(0),
	  m_bOverlapped(false),
	  m_pCurBuf(NULL), m_pNextBuf(NULL),
	  m_hbrBackground(NULL)
{
}

void
DC_Manager::setDCInfo(HDC hDC, int width, int height, HBRUSH hbrBackground)
{
	m_hDC = hDC;
	m_nWidth = width;
	m_nHeight = height;
	m_hbrBackground = hbrBackground;
}

void
DC_Manager::setViewPosition(int nXOffset, filesize_t qYOffset)
{
	m_nXOffset = nXOffset;
	m_qYOffset = qYOffset;

	m_nYOffset = qYOffset - (qYOffset / m_nHeight) * m_nHeight;

	m_bOverlapped = m_nYOffset + m_nViewHeight > m_nHeight;

	setPosition((qYOffset / m_nHeight) * m_nBufSize);

	m_pCurBuf  = getCurrentBuffer(0);
	m_pNextBuf = getCurrentBuffer(1);
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
				::FillRect(hdcDst, &rcPaint, m_hbrBackground);
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
				::FillRect(hdcDst, &rctTemp, m_hbrBackground);
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


