// $Id$

#include "drawinfo.h"

TextColorInfo::TextColorInfo(COLORREF crFgColor, COLORREF crBkColor)
	: m_hbrBackground(NULL)
{
	setColor(crFgColor, crBkColor);
}

TextColorInfo::~TextColorInfo()
{
	if (m_hbrBackground) ::DeleteObject(m_hbrBackground);
}

void
TextColorInfo::setColor(COLORREF crFgColor, COLORREF crBkColor)
{
	m_crFgColor = crFgColor;
	m_crBkColor = crBkColor;

	if (m_hbrBackground) ::DeleteObject(m_hbrBackground);
	m_hbrBackground = ::CreateSolidBrush(crBkColor);
}

FontInfo::FontInfo(HDC hDC, int fontsize)
	: m_hFont(NULL)
{
	if (hDC) setFont(hDC, fontsize);
}

FontInfo::~FontInfo()
{
	if (m_hFont != NULL) ::DeleteObject(m_hFont);
}

void
FontInfo::setFont(HDC hDC, int fontsize)
{
	if (m_hFont != NULL) ::DeleteObject(m_hFont);

	// �t�H���g�̐���
	LOGFONT lfont;
	lfont.lfHeight = - MulDiv(fontsize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	lfont.lfWidth  = 0;
	lfont.lfEscapement = 0;
	lfont.lfOrientation = 0;
	lfont.lfWeight = FW_NORMAL;
	lfont.lfItalic = FALSE;
	lfont.lfUnderline = FALSE;
	lfont.lfStrikeOut = FALSE;
	lfont.lfCharSet = DEFAULT_CHARSET;
	lfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lfont.lfQuality = DEFAULT_QUALITY;
	lfont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
//	lfont.lfFaceName[0] = '\0';
	lstrcpy(lfont.lfFaceName, "FixedSys");

	m_hFont = ::CreateFontIndirect(&lfont);
	if (!m_hFont) throw InvalidFontError();

	HGDIOBJ orgFont = ::SelectObject(hDC, m_hFont);

	// "M" �̉��������ɕ����Ԋu��ݒ�
	SIZE tsize;
	if (!::GetTextExtentPoint32(hDC, "M", 1, &tsize)) {
		::SelectObject(hDC, orgFont);
		throw InvalidFontError();
	}
//	::SelectObject(hDC, orgFont);

	// �����Ԋu�̐ݒ�
//	m_nXPitch = tsize.cx + 1;
	m_nXPitch = tsize.cx;
	// �s�Ԋu�̐ݒ�
	m_nYPitch = tsize.cy + 1;
}

DrawInfo::DrawInfo(HDC hDC, int fontsize,
				   COLORREF crFgColorAddr, COLORREF crBkColorAddr,
				   COLORREF crFgColorData, COLORREF crBkColorData,
				   COLORREF crFgColorStr, COLORREF crBkColorStr,
				   COLORREF crFgColorHeader, COLORREF crBkColorHeader)
	: m_hDC(hDC),
	  m_nFontSize(fontsize),
	  m_FontInfo(hDC, fontsize),
	  m_tciAddress(crFgColorAddr, crBkColorAddr),
	  m_tciData(crFgColorData, crBkColorData),
	  m_tciString(crFgColorStr, crBkColorStr),
	  m_tciHeader(crFgColorHeader, crBkColorHeader)
{
}

