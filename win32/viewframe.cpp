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
	: m_pDC_Manager(NULL), m_pDrawInfo(pDrawInfo),
	  m_smHorz(NULL, SB_HORZ), m_smVert(NULL, SB_VERT),
	  m_hwndParent(hwndParent)
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

	m_smHorz.setHWND(m_hwndView);
	m_smVert.setHWND(m_hwndView);

	m_hDC = ::GetDC(m_hwndView);

	m_pDrawInfo->m_hDC = m_hDC;
	m_pDrawInfo->m_FontInfo.setFont(m_hDC,
									m_pDrawInfo->m_FontInfo.getFontSize(),
									m_pDrawInfo->m_FontInfo.getFaceName(),
									m_pDrawInfo->m_FontInfo.isBoldFace());

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
ViewFrame::recalcPageInfo()
{
	// -1 �̓w�b�_�̕�
	int nPageLineNum = (m_rctClient.bottom - m_rctClient.top) / m_nLineHeight - 1;

	// �E�B���h�E���L�������ʎ��̃o�b�t�@�ƃI�[�o�[���b�v�����ꍇ�ɕK�v
	m_bOverlapped = is_overlapped(m_nTopOffset / m_nLineHeight, nPageLineNum);
	m_smHorz.setInfo(WIDTH_PER_XPITCH * m_nCharWidth,
					 (m_rctClient.right - m_rctClient.left),
					 m_smHorz.getCurrentPos());
	filesize_t size = m_pDC_Manager->getFileSize();
	if (size < 0)
		m_smVert.disable();
	else
		m_smVert.setInfo(size / 16, nPageLineNum, m_smVert.getCurrentPos());
	::InvalidateRect(m_hwndView, NULL, FALSE);
	::UpdateWindow(m_hwndView);
}

void
ViewFrame::ensureVisible(filesize_t pos, bool bRedraw)
{
	filesize_t newline = pos / 16;
	filesize_t line_diff = newline - m_smVert.getCurrentPos();
	filesize_t valid_page_line_num = m_smVert.getGripWidth() - 1;
	if (line_diff < 0) {
		m_smVert.setPosition(newline);
		setCurrentLine(newline, bRedraw);
	} else if (line_diff >= valid_page_line_num) {
		newline -= valid_page_line_num;
		m_smVert.setPosition(newline);
		setCurrentLine(newline, bRedraw);
	}
}

void
ViewFrame::setCurrentLine(filesize_t newline, bool bRedraw)
{
	if (!m_pDC_Manager->isLoaded()) return;

	filesize_t qMaxLine = m_smVert.getMaxPos();
	if (newline > qMaxLine)
		newline = qMaxLine;
	else if (newline < 0) newline = 0;

	filesize_t qSelectedPos = m_qPrevSelectedPos;
	int nSelectedSize = m_nPrevSelectedSize;
	unselect(false);

	// DCBuffer �̓��e��\���̈�ɕύX����
	filesize_t buffer_top = m_pDC_Manager->setPosition(newline * 16);

	m_nTopOffset = (int)(newline - buffer_top / 16) * m_nLineHeight;
	assert(m_nTopOffset < HEIGHT_PER_YPITCH * m_nLineHeight);

	// ���̃o�b�t�@�ƃI�[�o�[���b�v���Ă��邩�ǂ���
	int actual_linenum = (m_rctClient.bottom - m_rctClient.top + m_nLineHeight - 1)
							/ m_nLineHeight - 1;
	m_bOverlapped = is_overlapped(m_nTopOffset / m_nLineHeight, actual_linenum);

	m_pCurBuf  = m_pDC_Manager->getCurrentBuffer(0);
	m_pNextBuf = m_pDC_Manager->getCurrentBuffer(1);

	select(qSelectedPos, nSelectedSize, false);

	if (bRedraw) updateWithoutHeader();
}

void
ViewFrame::setPosition(filesize_t pos, bool bRedraw)
{
	if (!m_pDC_Manager->isLoaded()) return;

	filesize_t fsize = this->getFileSize();
	if (fsize == 0) return;

	if (pos >= fsize)
		pos = fsize - 1;
	if (pos < 0) pos = 0; // else �ł� NG!! (filesize == 0 �̏ꍇ)

	unselect(false);

	m_qCurrentPos = pos;

	select(pos, 1, bRedraw);

	// �X�e�[�^�X�o�[�ւ̃t�B�[�h�o�b�N
	::SendMessage(m_hwndParent, WM_USER_SETPOSITION,
				  (WPARAM)(pos >> 32), (LPARAM)pos);
}

void
ViewFrame::setPositionByCoordinate(const POINTS& pos, bool bRedraw)
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

	int x_pos = m_pDC_Manager->getXPositionByCoordinate(pos.x + m_smHorz.getCurrentPos());
	if (x_pos < 0) return;

	qPos += x_pos;

	filesize_t fsize = this->getFileSize();
	if (qPos >= fsize) {
		qPos = fsize - 1;
		if (qPos < 0) qPos = 0;
	}

	this->setPosition(qPos, bRedraw);
}

void
ViewFrame::bitBlt(const RECT& rcPaint)
{
	int nXOffset = m_smHorz.getCurrentPos();
	int width = WIDTH_PER_XPITCH * m_nCharWidth - nXOffset;
	if (m_bOverlapped) {
		int height = HEIGHT_PER_YPITCH * m_nLineHeight - m_nTopOffset;

		assert(m_pCurBuf);

		if (rcPaint.bottom - m_nLineHeight <= height) {
			m_pCurBuf->bitBlt(m_hDC,
							  rcPaint.left, rcPaint.top,
							  rcPaint.right - rcPaint.left,
							  rcPaint.bottom - rcPaint.top,
							  rcPaint.left + nXOffset,
							  m_nTopOffset + rcPaint.top - m_nLineHeight);
		} else if (rcPaint.top - m_nLineHeight >= height) {
			if (m_pNextBuf) {
				m_pNextBuf->bitBlt(m_hDC,
								   rcPaint.left, rcPaint.top,
								   rcPaint.right - rcPaint.left,
								   rcPaint.bottom - rcPaint.top,
								   rcPaint.left + nXOffset,
								   rcPaint.top - height - m_nLineHeight);
			} else {
				::FillRect(m_hDC, &rcPaint, m_pDrawInfo->m_tciData.getBkBrush());
			}
		} else {
			m_pCurBuf->bitBlt(m_hDC,
							  rcPaint.left, rcPaint.top,
							  rcPaint.right - rcPaint.left,
							  height - rcPaint.top + m_nLineHeight,
							  rcPaint.left + nXOffset,
							  rcPaint.top + m_nTopOffset - m_nLineHeight);
			if (m_pNextBuf) {
				m_pNextBuf->bitBlt(m_hDC,
								   rcPaint.left, height + m_nLineHeight,
								   rcPaint.right - rcPaint.left,
								   rcPaint.bottom - height,
								   rcPaint.left + nXOffset, 0);
			} else {
				RECT rctTemp = rcPaint;
				rctTemp.top    = height + m_nLineHeight;
				rctTemp.bottom = rcPaint.bottom;
				::FillRect(m_hDC, &rctTemp, m_pDrawInfo->m_tciData.getBkBrush());
			}
		}
	} else if (m_pCurBuf) {
		m_pCurBuf->bitBlt(m_hDC,
						  rcPaint.left, rcPaint.top,
						  rcPaint.right - rcPaint.left,
						  rcPaint.bottom - rcPaint.top,
						  rcPaint.left + nXOffset,
						  m_nTopOffset + rcPaint.top - m_nLineHeight);
	} else {
		::FillRect(m_hDC, &rcPaint, (HBRUSH)(COLOR_APPWORKSPACE + 1));
	}

	// bitblt header
	if (rcPaint.top < m_nLineHeight) {
		Header& hdr = m_pDC_Manager->getHeader();
		hdr.bitBlt(m_hDC,
				   rcPaint.left, rcPaint.top,
				   rcPaint.right - rcPaint.left,
				   min(m_nLineHeight, rcPaint.bottom - rcPaint.top),
				   rcPaint.left + nXOffset, rcPaint.top);
	}
}

void
ViewFrame::invertRegion(filesize_t pos, int size, bool bSelected)
{
	int min = m_pDC_Manager->getMinBufferIndex(),
		max = m_pDC_Manager->getMaxBufferIndex();
	for (int i = min; i <= max; i++) {
		DCBuffer* pDCBuffer = m_pDC_Manager->getCurrentBuffer(i);
		if (pDCBuffer && pDCBuffer->hasSelectedRegion() != bSelected) {
			pDCBuffer->invertRegion(pos, size, bSelected);
		}
	}
}

void
ViewFrame::select(filesize_t pos, int size, bool bRedraw)
{
//	assert(pos >= 0 && size > 0);
	if (pos < 0 || size <= 0) return;

	// unselect previously selected region
	unselect(false);

	invertRegion(pos, size, true);

	m_qPrevSelectedPos  = pos;
	m_nPrevSelectedSize = size;

	if (bRedraw) updateWithoutHeader();
}

void
ViewFrame::unselect(bool bRedraw)
{
	if (m_qPrevSelectedPos >= 0 && m_nPrevSelectedSize > 0) {
		invertRegion(m_qPrevSelectedPos, m_nPrevSelectedSize, false);
		m_qPrevSelectedPos  = -1;
		m_nPrevSelectedSize = 0;
		if (bRedraw) updateWithoutHeader();
	}
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
	if (!m_pDC_Manager->isLoaded()) return;

	filesize_t qCurLine = m_smVert.getCurrentPos();
	filesize_t qCurDiff = m_qCurrentPos - qCurLine * 16;

	filesize_t qNewLine = m_smVert.onScroll(LOWORD(wParam));

	// �\���̈�̍X�V
	setCurrentLine(qNewLine, false);

	// m_qCurrentPos �̕ύX
	filesize_t qCaretLine = m_qCurrentPos / 16;

	switch (m_pDrawInfo->m_ScrollConfig.m_caretMove) {
	case CARET_ENSURE_VISIBLE:
		{
			// �����L�����b�g���r���[�t���[������͂ݏo�Ă�����
			// �r���[�t���[���̒��ɓ����
			filesize_t qBottomLine = qNewLine + m_smVert.getGripWidth();
			if (qNewLine > qCaretLine) {
				// ��ɂ͂ݏo���ꍇ
				setPosition(qNewLine * 16 + m_qCurrentPos % 16, false); 
			} else if (qBottomLine <= qCaretLine) {
				// ���ɂ͂ݏo���ꍇ
				setPosition((qBottomLine - 1) * 16 + m_qCurrentPos % 16, false);
			}
		}
		break;
	
	case CARET_SCROLL:
		{
			// �L�����b�g���r���[�t���[���ɑ΂��ČŒ肷��
			assert(qCurDiff >= 0);
			setPosition(qNewLine * 16 + qCurDiff, false);
		}
		break;

	default:
		// �L�����b�g���ړ����Ȃ�
		break;
	}

	updateWithoutHeader();
}

void
ViewFrame::onHScroll(WPARAM wParam, LPARAM lParam)
{
	// prepare the correct BGBuffer
	m_nXOffset = m_smHorz.onScroll(LOWORD(wParam));

	// get the region of BGBuffer to be drawn
	::InvalidateRect(m_hwndView, NULL, FALSE);
	::UpdateWindow(m_hwndView);
}

void
ViewFrame::onMouseWheel(WPARAM wParam, LPARAM lParam)
{
	if (!m_pDC_Manager->isLoaded()) return;

	int nLineDiff = - (short)HIWORD(wParam) / WHEEL_DELTA;

	switch (m_pDrawInfo->m_ScrollConfig.m_wheelScroll) {
	case WHEEL_AS_ARROW_KEYS:
		onVerticalMove(nLineDiff);
		break;
	case WHEEL_AS_SCROLL_BAR:
		{
			int absNLineDiff = abs(nLineDiff);
			for (int n = 0; n < absNLineDiff; n++)
				onVScroll(nLineDiff > 0 ? SB_LINEDOWN : SB_LINEUP, 0);
		}
		break;
	default:
		assert(0);
		break;
	}
}

void
ViewFrame::onLButtonDown(WPARAM wParam, LPARAM lParam)
{
	if (!m_pDC_Manager->isLoaded()) return;

	setPositionByCoordinate(MAKEPOINTS(lParam));
}

void
ViewFrame::onJump(filesize_t pos, int size)
{
	assert(size > 0);

	if (!m_pDC_Manager->isLoaded()) return;

	if (pos < 0) pos = 0;

	if (size > 1) {
		// pos �` pos + size �̃f�[�^���\�Ȍ���\������邱�Ƃ�ۏ؂���
		ensureVisible(pos + size, false);
	}

	// prepare the correct BGBuffer
	ensureVisible(pos, false);

	setPosition(pos);
}

void
ViewFrame::onHorizontalMove(int diff)
{
	filesize_t newpos = m_qCurrentPos + diff;
	ensureVisible(newpos, false);
	setPosition(newpos);
}

void
ViewFrame::onVerticalMove(int diff)
{
	filesize_t newpos = m_qCurrentPos + diff * 16;
	ensureVisible(newpos, false);
	setPosition(newpos);
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

