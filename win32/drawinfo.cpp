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

FontInfo::FontInfo(HDC hDC, float fontsize,
				   const char* faceName, bool bBoldFace)
	: m_hFont(NULL)
{
	if (hDC) {
		setFont(hDC, fontsize, faceName, bBoldFace);
	} else {
		m_fFontSize = fontsize;
		if (faceName)
			lstrcpy(m_pszFontFace, faceName);
		else
			m_pszFontFace[0] = '\0';
		m_bBoldFace = bBoldFace;
	}
}

FontInfo::~FontInfo()
{
	if (m_hFont != NULL) ::DeleteObject(m_hFont);
}

void
FontInfo::setFont(HDC hDC, float fontsize,
				  const char* faceName, bool bBoldFace)
{
	// フォントの生成
	LOGFONT lfont;
	lfont.lfHeight = - (int) (fontsize * GetDeviceCaps(hDC, LOGPIXELSY) / 72);
	lfont.lfWidth  = 0;
	lfont.lfEscapement = 0;
	lfont.lfOrientation = 0;
	lfont.lfWeight = bBoldFace ? FW_BOLD : FW_NORMAL;
	lfont.lfItalic = FALSE;
	lfont.lfUnderline = FALSE;
	lfont.lfStrikeOut = FALSE;
	lfont.lfCharSet = DEFAULT_CHARSET;
	lfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lfont.lfQuality = DEFAULT_QUALITY;
	lfont.lfPitchAndFamily = /* FIXED_PITCH | */ FF_DONTCARE;
	if (faceName)
		lstrcpy(lfont.lfFaceName, faceName);
	else
		lfont.lfFaceName[0] = '\0';

	HFONT hFont = ::CreateFontIndirect(&lfont);
	if (!hFont) throw InvalidFontError();

	HGDIOBJ orgFont = ::SelectObject(hDC, hFont);

	// facename, textmetrics を取得
	TEXTMETRIC tm;
	if (!::GetTextFace(hDC, LF_FACESIZE, lfont.lfFaceName) ||
		!::GetTextMetrics(hDC, &tm)) {
		::SelectObject(hDC, orgFont);
		::DeleteObject(hFont);
		throw InvalidFontError();
	}

	lstrcpy(m_pszFontFace, lfont.lfFaceName);

	if (m_hFont != NULL) ::DeleteObject(m_hFont);
	m_hFont = hFont;
	m_fFontSize = fontsize;

	m_bBoldFace = bBoldFace;
	// 下記の？？については SDK マニュアル参照
	m_bProportional = ((tm.tmPitchAndFamily & TMPF_FIXED_PITCH) != 0);

	// 文字間隔の設定
	m_nXPitch = tm.tmAveCharWidth;
	// 行間隔の設定
	m_nYPitch = tm.tmHeight + 1;
}

DrawInfo::DrawInfo(HDC hDC, float fontsize,
				   const char* faceName, bool bBoldFace,
				   COLORREF crFgColorAddr, COLORREF crBkColorAddr,
				   COLORREF crFgColorData, COLORREF crBkColorData,
				   COLORREF crFgColorStr, COLORREF crBkColorStr,
				   COLORREF crFgColorHeader, COLORREF crBkColorHeader)
	: m_hDC(hDC),
	  m_FontInfo(hDC, fontsize, faceName, bBoldFace),
	  m_tciAddress(crFgColorAddr, crBkColorAddr),
	  m_tciData(crFgColorData, crBkColorData),
	  m_tciString(crFgColorStr, crBkColorStr),
	  m_tciHeader(crFgColorHeader, crBkColorHeader)
{
}

