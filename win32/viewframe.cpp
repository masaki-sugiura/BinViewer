// $Id$

#define _WIN32_WINNT  0x500  // to support mouse wheel

#include "viewframe.h"

#include <assert.h>
#include <algorithm>

#define VIEWFRAME_CLASSNAME  "BinViewer_ViewFrameClass32"

bool ViewFrame::m_bRegisterClass;
WORD ViewFrame::m_wNextID;
HINSTANCE ViewFrame::m_hInstance;

ViewFrame::ViewFrame(HWND hwndParent, const RECT& rct,
					 DrawInfo* pDrawInfo,
					 LargeFileReader* pLFReader)
	: m_pDC_Manager(NULL), m_pDrawInfo(pDrawInfo)
{
	if (!m_bRegisterClass) {
		WNDCLASS wc;
		::ZeroMemory(&wc, sizeof(wc));
		wc.hInstance = (HINSTANCE)::GetWindowLong(hwndParent, GWL_HINSTANCE);
		wc.style = CS_OWNDC | CS_SAVEBITS;
		wc.hIcon = NULL;
		wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		wc.lpfnWndProc = (WNDPROC)ViewFrame::ViewFrameWndProc;
		wc.lpszClassName = VIEWFRAME_CLASSNAME;
		wc.lpszMenuName = NULL;
		if (!::RegisterClass(&wc)) {
			assert(0);
			throw RegisterClassError();
		}
		m_wNextID = 0x10;
		m_bRegisterClass = true;
		m_hInstance = wc.hInstance;
	}

	m_hwndView = ::CreateWindowEx(WS_EX_CLIENTEDGE,
								  VIEWFRAME_CLASSNAME, "",
								  WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
								  rct.left, rct.top,
								  rct.right - rct.left,
								  rct.bottom - rct.top,
								  hwndParent, (HMENU)m_wNextID++,
								  m_hInstance, (LPVOID)this);
	if (!m_hwndView) {
		throw CreateWindowError();
	}

	m_hDC = ::GetDC(m_hwndView);

	m_pDrawInfo->m_hDC = m_hDC;
	m_pDrawInfo->m_FontInfo.setFont(m_hDC, m_pDrawInfo->m_nFontSize);

	initParams();

	m_pDC_Manager = new DC_Manager(m_hDC, m_pDrawInfo, pLFReader);

	m_nLineHeight = m_pDrawInfo->m_FontInfo.getYPitch();
	m_nCharWidth  = m_pDrawInfo->m_FontInfo.getXPitch();

	setFrameRect(rct);
}

ViewFrame::~ViewFrame()
{
}

void
ViewFrame::setFrameRect(const RECT& rctFrame)
{
	m_rctFrame = rctFrame;

	::SetWindowPos(m_hwndView, NULL,
				   rctFrame.left, rctFrame.top,
				   rctFrame.right - rctFrame.left,
				   rctFrame.bottom - rctFrame.top,
				   SWP_NOZORDER);

	::GetClientRect(m_hwndView, &m_rctClient);

	recalcPageInfo();
}

void
ViewFrame::adjustWindowRect(RECT& rctFrame)
{
	RECT rctWindow, rctClient;
	::GetWindowRect(m_hwndView, &rctWindow);
	::GetClientRect(m_hwndView, &rctClient);

	rctFrame.left = rctFrame.top = 0;
	rctFrame.right = rctWindow.right - rctWindow.left - rctClient.right
					 + WIDTH_PER_XPITCH * m_nCharWidth;
	rctFrame.bottom = ((rctFrame.bottom + m_nLineHeight - 1) / m_nLineHeight)
					   * m_nLineHeight
					+ (rctWindow.bottom - rctWindow.top - rctClient.bottom);
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

	unselect();

	m_qCurrentPos = pos;

	int top_offset_by_size = m_nTopOffset / m_nLineHeight * 16,
		page_line_num_by_size
		 = ((m_rctClient.bottom - m_rctClient.top) / m_nLineHeight - 1) * 16;
	if (!m_pCurBuf ||
		pos < m_pCurBuf->m_qAddress + top_offset_by_size ||
		pos >= m_pCurBuf->m_qAddress + top_offset_by_size
				+ page_line_num_by_size) {
		// 最初の描画またはカーソルが表示領域をはみ出る
		filesize_t newline = pos / 16;
		int line_diff = pos / 16 - m_qCurrentLine;
		if (line_diff >= page_line_num_by_size / 16) {
			// 下にジャンプ/スクロール
			newline -= page_line_num_by_size / 16 - 1;
		}
		setCurrentLine(newline);
	}
	modifyVScrollInfo();

	select(pos, 1);

	::SendMessage(::GetParent(m_hwndView), WM_USER_SETPOSITION,
				  (WPARAM)(m_qCurrentPos >> 32), (LPARAM)m_qCurrentPos);
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
ViewFrame::bitBlt(const RECT& rcPaint)
{
	int width = WIDTH_PER_XPITCH * m_nCharWidth - m_nXOffset;
	if (m_bOverlapped) {
		int height = HEIGHT_PER_YPITCH * m_nLineHeight - m_nTopOffset;

		assert(m_pCurBuf);

		if (rcPaint.bottom - m_nLineHeight <= height) {
			::BitBlt(m_hDC,
					 rcPaint.left, rcPaint.top,
					 rcPaint.right - rcPaint.left,
					 rcPaint.bottom - rcPaint.top,
					 m_pCurBuf->m_hDC,
					 rcPaint.left + m_nXOffset,
					 m_nTopOffset + rcPaint.top - m_nLineHeight,
					 SRCCOPY);
		} else if (rcPaint.top - m_nLineHeight >= height) {
			if (m_pNextBuf) {
				::BitBlt(m_hDC,
						 rcPaint.left, rcPaint.top,
						 rcPaint.right - rcPaint.left,
						 rcPaint.bottom - rcPaint.top,
						 m_pNextBuf->m_hDC,
						 rcPaint.left + m_nXOffset,
						 rcPaint.top - height - m_nLineHeight,
						 SRCCOPY);
			} else {
				::FillRect(m_hDC, &rcPaint, m_pDrawInfo->m_tciData.getBkBrush());
			}
		} else {
			::BitBlt(m_hDC,
					 rcPaint.left, rcPaint.top,
					 rcPaint.right - rcPaint.left,
					 height - rcPaint.top + m_nLineHeight,
					 m_pCurBuf->m_hDC,
					 rcPaint.left + m_nXOffset,
					 rcPaint.top + m_nTopOffset - m_nLineHeight,
					 SRCCOPY);
			if (m_pNextBuf) {
				::BitBlt(m_hDC,
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
				::FillRect(m_hDC, &rctTemp, m_pDrawInfo->m_tciData.getBkBrush());
			}
		}
		if (rcPaint.right > width) {
			RECT rctTemp = rcPaint;
			rctTemp.left  = max(rcPaint.left, width);
			::FillRect(m_hDC, &rctTemp,
					   m_pDrawInfo->m_tciData.getBkBrush());
		}
	} else if (m_pCurBuf) {
		::BitBlt(m_hDC,
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
			::FillRect(m_hDC, &rctTemp,
					   m_pDrawInfo->m_tciData.getBkBrush());
		}
	} else {
		::FillRect(m_hDC, &rcPaint, (HBRUSH)(COLOR_APPWORKSPACE + 1));
	}

	// bitblt header
	if (rcPaint.top < m_nLineHeight) {
		if (rcPaint.left < width)
			::BitBlt(m_hDC, rcPaint.left, rcPaint.top,
					 min(rcPaint.right, width) - rcPaint.left,
					 min(m_nLineHeight, rcPaint.bottom - rcPaint.top),
					 m_pDC_Manager->getHeaderDC(),
					 rcPaint.left + m_nXOffset, rcPaint.top,
					 SRCCOPY);
		if (rcPaint.right > width) {
			RECT rctTemp = rcPaint;
			rctTemp.left = width;
			rctTemp.bottom = min(m_nLineHeight, rcPaint.bottom - rcPaint.top);
			::FillRect(m_hDC, &rctTemp, m_pDrawInfo->m_tciHeader.getBkBrush());
		}
	}
}

void
ViewFrame::invertRegion(filesize_t pos, int size)
{
	int min = m_pDC_Manager->getMinBufferIndex(),
		max = m_pDC_Manager->getMaxBufferIndex();
	for (int i = min; i <= max; i++) {
		DCBuffer* pDCBuffer = m_pDC_Manager->getCurrentBuffer(i);
		if (pDCBuffer) {
			pDCBuffer->invertRegion(pos, size);
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

	updateWithoutHeader();
}

void
ViewFrame::unselect()
{
	if (m_qPrevSelectedPos >= 0 && m_nPrevSelectedSize > 0) {
		invertRegion(m_qPrevSelectedPos, m_nPrevSelectedSize);
		m_qPrevSelectedPos  = -1;
		m_nPrevSelectedSize = 0;
		updateWithoutHeader();
	}
}

LRESULT CALLBACK
ViewFrame::ViewFrameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CREATESTRUCT* pCs = (CREATESTRUCT*)lParam;
			::SetWindowLong(hWnd, GWL_USERDATA, (LONG)pCs->lpCreateParams);
			return 0;
		}
	case WM_NCCREATE:
		return TRUE;
	}

	ViewFrame* pViewFrame = (ViewFrame*)::GetWindowLong(hWnd, GWL_USERDATA);
//	assert(pViewFrame);
	if (!pViewFrame)
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);

	switch (uMsg) {
	case WM_PAINT:
		pViewFrame->onPaint(wParam, lParam);
		break;

	case WM_VSCROLL:
		pViewFrame->onVScroll(wParam, lParam);
		break;

	case WM_HSCROLL:
		pViewFrame->onHScroll(wParam, lParam);
		break;

	case WM_LBUTTONDOWN:
		pViewFrame->onLButtonDown(wParam, lParam);
		break;

	case WM_DROPFILES:
		::SendMessage(::GetParent(hWnd), uMsg, wParam, lParam);
		break;

	default:
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

void
ViewFrame::modifyVScrollInfo()
{
	SCROLLINFO sinfo;
	sinfo.cbSize = sizeof(sinfo);
	sinfo.fMask = SIF_DISABLENOSCROLL | SIF_POS | SIF_RANGE | SIF_PAGE;
	sinfo.nPos = 0;
	sinfo.nMin = 0;

	if (getFileSize() < 0) {
		// ファイルが読み込まれていない状態
		sinfo.nMax  = 1;
		sinfo.nPage = 2;
	} else {
		filesize_t pos = m_qCurrentPos;

		if (m_qMaxLine & ~0x7FFFFFFF) {
			// スクロールバーが native に扱えるファイルサイズを越えている
			m_bMapScrollBarLinearly = false;
			sinfo.nMax = 0x7FFFFFFF;
			sinfo.nPage = (DWORD)(((filesize_t)m_nPageLineNum << 32) / m_qMaxLine);
			sinfo.nPos = (pos << 32) / m_qMaxLine;
		} else {
			m_bMapScrollBarLinearly = true;
			// m_nPageLineNum は半端な最下行を含むので -1 しておく
			sinfo.nPage = m_nPageLineNum - 1;
			sinfo.nMax = (int)m_qMaxLine + sinfo.nPage - 1;
			sinfo.nPos = (int)(pos / 16);
		}
		if (!sinfo.nPage) sinfo.nPage = 1;
	}

	::SetScrollInfo(m_hwndView, SB_VERT, &sinfo, TRUE);
}

void
ViewFrame::modifyHScrollInfo(int width)
{
	SCROLLINFO sinfo;
	sinfo.cbSize = sizeof(sinfo);
	sinfo.fMask = SIF_DISABLENOSCROLL | SIF_POS | SIF_RANGE | SIF_PAGE;
	sinfo.nPos = 0;
	sinfo.nMin = 0;

	width /= m_nCharWidth;

	if (width >= WIDTH_PER_XPITCH) {
		// HSCROLL を無効化
		sinfo.nMax  = 1;
		sinfo.nPage = 2;
	} else {
		sinfo.nMax  = WIDTH_PER_XPITCH - 1;
		sinfo.nPage = width;
		sinfo.nPos  = m_nXOffset / m_nCharWidth;
	}

	::SetScrollInfo(m_hwndView, SB_HORZ, &sinfo, TRUE);
}

void
ViewFrame::updateWithoutHeader()
{
	RECT rctClient = m_rctClient;
	rctClient.top = m_nLineHeight;
	::InvalidateRect(m_hwndView, &rctClient, FALSE);
	::UpdateWindow(m_hwndView);
}

void
ViewFrame::onPaint(WPARAM, LPARAM)
{
	PAINTSTRUCT ps;
	::BeginPaint(m_hwndView, &ps);
	bitBlt(ps.rcPaint);
	::EndPaint(m_hwndView, &ps);
}

void
ViewFrame::onVScroll(WPARAM wParam, LPARAM lParam)
{
	if (!isLoaded()) return;

	SCROLLINFO sinfo;
	sinfo.cbSize = sizeof(SCROLLINFO);
	sinfo.fMask = SIF_ALL;

	::GetScrollInfo(m_hwndView, SB_VERT, &sinfo);
	if (sinfo.nMax <= sinfo.nPage) return;

	filesize_t qCurLine = m_qCurrentLine;

	switch (LOWORD(wParam)) {
	case SB_LINEDOWN:
		if (qCurLine < m_qMaxLine) {
			qCurLine++;
			if (!m_bMapScrollBarLinearly) {
				// ファイルサイズが大きい場合
				sinfo.nPos = (qCurLine << 32) / m_qMaxLine;
			} else {
				sinfo.nPos++;
			}
		}
		break;

	case SB_LINEUP:
		if (qCurLine > 0) {
			qCurLine--;
			if (!m_bMapScrollBarLinearly) {
				// ファイルサイズが大きい場合
				sinfo.nPos = (qCurLine << 32) / m_qMaxLine;
			} else {
				sinfo.nPos--;
			}
		}
		break;

	case SB_PAGEDOWN:
		if ((qCurLine += m_nPageLineNum) > m_qMaxLine) {
			qCurLine = m_qMaxLine;
			sinfo.nPos = sinfo.nMax;
		} else {
			if (!m_bMapScrollBarLinearly) {
				// ファイルサイズが大きい場合
				sinfo.nPos = (qCurLine << 32) / m_qMaxLine;
			} else {
				sinfo.nPos += sinfo.nPage;
			}
		}
		break;

	case SB_PAGEUP:
		if ((qCurLine -= m_nPageLineNum) < 0) {
			qCurLine = 0;
			sinfo.nPos = sinfo.nMin;
		} else {
			if (!m_bMapScrollBarLinearly) {
				// ファイルサイズが大きい場合
				sinfo.nPos = (qCurLine << 32) / m_qMaxLine;
			} else {
				sinfo.nPos -= sinfo.nPage;
			}
		}
		break;

	case SB_TOP:
		qCurLine = 0;
		sinfo.nPos = sinfo.nMin;
		break;

	case SB_BOTTOM:
		qCurLine = m_qMaxLine;
		sinfo.nPos = sinfo.nMax;
		break;

	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
		sinfo.nPos = sinfo.nTrackPos;
		if (!m_bMapScrollBarLinearly) {
			// ファイルサイズが大きい場合
			qCurLine = (sinfo.nPos * m_qMaxLine) >> 32;
		} else {
			qCurLine = sinfo.nPos;
		}
		break;

	default:
		return;
	}
	::SetScrollInfo(m_hwndView, SB_VERT, &sinfo, TRUE);

	// prepare the correct BGBuffer
	setCurrentLine(qCurLine);

	updateWithoutHeader();
}

void
ViewFrame::onHScroll(WPARAM wParam, LPARAM lParam)
{
	SCROLLINFO sinfo;
	sinfo.cbSize = sizeof(SCROLLINFO);
	sinfo.fMask = SIF_ALL;

	::GetScrollInfo(m_hwndView, SB_HORZ, &sinfo);
	if (sinfo.nMax <= sinfo.nPage) return;

	int nXOffset = m_nXOffset,
		nMaxXOffset = m_nCharWidth * WIDTH_PER_XPITCH
					  - (m_rctClient.right - m_rctClient.left);

	switch (LOWORD(wParam)) {
	case SB_LINEDOWN:
		if ((nXOffset += m_nCharWidth) > nMaxXOffset) {
			nXOffset = nMaxXOffset;
			sinfo.nPos = sinfo.nMax;
		} else {
			sinfo.nPos++;
		}
		break;

	case SB_LINEUP:
		if ((nXOffset -= m_nCharWidth) <= 0) {
			nXOffset = 0;
			sinfo.nPos = 0;
		} else {
			sinfo.nPos--;
		}
		break;

	case SB_PAGEDOWN:
		if ((sinfo.nPos += sinfo.nPage) > sinfo.nMax) {
			nXOffset = nMaxXOffset;
			sinfo.nPos = sinfo.nMax;
		} else {
			nXOffset += sinfo.nPage * m_nCharWidth;
		}
		break;

	case SB_PAGEUP:
		if ((sinfo.nPos -= sinfo.nPage) < 0) {
			nXOffset = 0;
			sinfo.nPos = sinfo.nMin;
		} else {
			nXOffset -= sinfo.nPage * m_nCharWidth;
		}
		break;

	case SB_TOP:
		nXOffset = 0;
		sinfo.nPos = sinfo.nMin;
		break;

	case SB_BOTTOM:
		nXOffset = nMaxXOffset;
		sinfo.nPos = sinfo.nMax;
		break;

	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
		sinfo.nPos = sinfo.nTrackPos;
		nXOffset = m_nCharWidth * sinfo.nPos;
		break;

	default:
		return;
	}

	::SetScrollInfo(m_hwndView, SB_HORZ, &sinfo, TRUE);

	// prepare the correct BGBuffer
	setXOffset(nXOffset);

	// get the region of BGBuffer to be drawn
	::InvalidateRect(m_hwndView, NULL, FALSE);
	::UpdateWindow(m_hwndView);
}

void
ViewFrame::onMouseWheel(WPARAM wParam, LPARAM lParam)
{
	if (!isLoaded()) return;

	onVerticalMove(- (short)HIWORD(wParam) / WHEEL_DELTA);
}

void
ViewFrame::onLButtonDown(WPARAM wParam, LPARAM lParam)
{
	if (!isLoaded()) return;

	setPositionByCoordinate(MAKEPOINTS(lParam));
	updateWithoutHeader();
}

void
ViewFrame::onJump(filesize_t pos)
{
	if (!isLoaded()) return;

	SCROLLINFO sinfo;
	sinfo.cbSize = sizeof(SCROLLINFO);
	sinfo.fMask = SIF_ALL;

	::GetScrollInfo(m_hwndView, SB_VERT, &sinfo);
	if (pos < 0) {
		pos = 0;
		sinfo.nPos = sinfo.nMin;
	} else if (pos / 16 > m_qMaxLine) {
		pos = m_qMaxLine * 16;
		sinfo.nPos = sinfo.nMax;
	} else {
		if (!m_bMapScrollBarLinearly) {
			// ファイルサイズが大きい場合
			sinfo.nPos = ((pos / 16) << 32) / m_qMaxLine;
		} else {
			sinfo.nPos = (int)(pos / 16);
		}
	}
	::SetScrollInfo(m_hwndView, SB_VERT, &sinfo, TRUE);

	// prepare the correct BGBuffer
	setPosition(pos);

	updateWithoutHeader();
}

void
ViewFrame::onHorizontalMove(int diff)
{
	setPosition(m_qCurrentPos + diff);
	updateWithoutHeader();
}

void
ViewFrame::onVerticalMove(int diff)
{
	setPosition(m_qCurrentPos + diff * 16);
	updateWithoutHeader();
}

