// $Id$

#ifndef SCROLL_H_INC
#define SCROLL_H_INC

#include <windows.h>
#include <assert.h>

template<class SIZE_TYPE>
class ScrollManager {
public:
	ScrollManager(HWND hWnd, int axis)
		: m_hWnd(hWnd), m_nAxis(axis)
	{
		// disable scroll bar initially
//		disable();
	}

	void setHWND(HWND hWnd)
	{
		m_hWnd = hWnd;
	}

	void disable()
	{
		setInfo(1, 2, 0);
	}

	void setInfo(SIZE_TYPE stMaxPos, SIZE_TYPE stGripWidth, SIZE_TYPE stCurrPos = 0)
	{
		assert(m_hWnd);

		m_stMaxPos     = stMaxPos;
		m_stGripWidth  = stGripWidth;
		m_stCurrentPos = stCurrPos;

		SCROLLINFO sinfo;
		sinfo.cbSize = sizeof(sinfo);
		sinfo.fMask  = SIF_DISABLENOSCROLL | SIF_POS | SIF_RANGE | SIF_PAGE;
		sinfo.nMin   = 0;

		if ((__int64)stMaxPos >= 0x100000000) {
			m_bMapScrollBarLinearly = false;
			sinfo.nMax  = 0x7FFFFFFF;
			sinfo.nPage = (DWORD)(((__int64)stGripWidth << 32) / stMaxPos);
			sinfo.nPos  = ((__int64)stCurrPos << 32) / stMaxPos;
		} else {
			m_bMapScrollBarLinearly = true;
			sinfo.nMax  = stMaxPos - 1;
			sinfo.nPage = stGripWidth;
			sinfo.nPos  = stCurrPos;
		}
		if (sinfo.nMax <= 0)  sinfo.nMax  = 1;
		if (sinfo.nPage == 0) sinfo.nPage = 1;

		::SetScrollInfo(m_hWnd, m_nAxis, &sinfo, TRUE);
	}

	void setPosition(SIZE_TYPE pos)
	{
		assert(m_hWnd);

		m_stCurrentPos = pos;

		SCROLLINFO sinfo;
		sinfo.cbSize = sizeof(sinfo);
		sinfo.fMask  = SIF_POS;
		if (m_bMapScrollBarLinearly)
			sinfo.nPos = pos;
		else
			sinfo.nPos = ((__int64)pos << 32) / m_stMaxPos;

		::SetScrollInfo(m_hWnd, m_nAxis, &sinfo, TRUE);
	}

	SIZE_TYPE onScroll(int cmd)
	{
		SCROLLINFO sinfo;
		sinfo.cbSize = sizeof(SCROLLINFO);
		sinfo.fMask = SIF_ALL;

		::GetScrollInfo(m_hWnd, m_nAxis, &sinfo);

		switch (cmd) {
		case SB_LINEDOWN:
			if (m_stCurrentPos < m_stMaxPos) {
				m_stCurrentPos++;
				if (m_bMapScrollBarLinearly) {
					sinfo.nPos++;
				} else {
					sinfo.nPos = ((__int64)m_stCurrentPos << 32) / m_stMaxPos;
				}
			}
			break;

		case SB_LINEUP:
			if (m_stCurrentPos > 0) {
				m_stCurrentPos--;
				if (m_bMapScrollBarLinearly) {
					sinfo.nPos--;
				} else {
					sinfo.nPos = ((__int64)m_stCurrentPos << 32) / m_stMaxPos;
				}
			}
			break;

		case SB_PAGEDOWN:
			if ((m_stCurrentPos += m_stGripWidth) > m_stMaxPos) {
				m_stCurrentPos = m_stMaxPos;
				sinfo.nPos = sinfo.nMax;
			} else {
				if (m_bMapScrollBarLinearly) {
					sinfo.nPos += sinfo.nPage;
				} else {
					sinfo.nPos = ((__int64)m_stCurrentPos << 32) / m_stMaxPos;
				}
			}
			break;

		case SB_PAGEUP:
			if ((m_stCurrentPos -= m_stGripWidth) < 0) {
				m_stCurrentPos = 0;
				sinfo.nPos = 0;
			} else {
				if (m_bMapScrollBarLinearly) {
					sinfo.nPos -= sinfo.nPage;
				} else {
					sinfo.nPos = ((__int64)m_stCurrentPos << 32) / m_stMaxPos;
				}
			}
			break;

		case SB_TOP:
			m_stCurrentPos = 0;
			sinfo.nPos = 0;
			break;

		case SB_BOTTOM:
			m_stCurrentPos = m_stMaxPos;
			sinfo.nPos = sinfo.nMax;
			break;

		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			if (!m_bMapScrollBarLinearly) {
				// ファイルサイズが大きい場合
				m_stCurrentPos = (sinfo.nTrackPos * m_stMaxPos) >> 32;
			} else {
				m_stCurrentPos = sinfo.nTrackPos;
			}
			if (m_bMapScrollBarLinearly)
				sinfo.nPos = m_stCurrentPos;
			else
				sinfo.nPos = ((__int64)m_stCurrentPos << 32) / m_stMaxPos;
			break;
		}
		::SetScrollInfo(m_hWnd, m_nAxis, &sinfo, TRUE);

		return m_stCurrentPos;
	}

	SIZE_TYPE getCurrentPos() const { return m_stCurrentPos; }
	SIZE_TYPE getMaxPos() const { return m_stMaxPos; }
	SIZE_TYPE getGripWidth() const { return m_stGripWidth; }

private:
	HWND m_hWnd;
	int m_nAxis;
	SIZE_TYPE m_stCurrentPos, m_stMaxPos, m_stGripWidth;
	bool m_bMapScrollBarLinearly;
};

template<typename T>
inline T count(T v, int denom)
{
	return (v + 1) / denom;
}

#endif // SCROLL_H_INC
