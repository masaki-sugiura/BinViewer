// $Id$

#include "drawinfo.h"

TextColorInfo::TextColorInfo(const string& name,
							 COLORREF crFgColor, COLORREF crBkColor)
	: m_strName(name), m_hbrBackground(NULL)
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
	m_ColorConfig.m_crFgColor = crFgColor;
	m_ColorConfig.m_crBkColor = crBkColor;

	if (m_hbrBackground) ::DeleteObject(m_hbrBackground);
	m_hbrBackground = ::CreateSolidBrush(crBkColor);
}

void
TextColorInfo::setColor(const ColorConfig& cc)
{
	m_ColorConfig = cc;

	if (m_hbrBackground) ::DeleteObject(m_hbrBackground);
	m_hbrBackground = ::CreateSolidBrush(cc.m_crBkColor);
}


FontInfo::FontInfo(HDC hDC, float fontsize,
				   const char* faceName, bool bBoldFace)
	: m_hFont(NULL)
{
	if (hDC) {
		setFont(hDC, fontsize, faceName, bBoldFace);
	} else {
		m_FontConfig.m_fFontSize = fontsize;
		if (faceName)
			lstrcpy(m_FontConfig.m_pszFontFace, faceName);
		else
			m_FontConfig.m_pszFontFace[0] = '\0';
		m_FontConfig.m_bBoldFace = bBoldFace;
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
	int lpy = ::GetDeviceCaps(hDC, LOGPIXELSY);

	// フォントの生成
	LOGFONT lfont;
	lfont.lfHeight = - (int) (fontsize * lpy / 72);
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

	if (m_hFont != NULL) ::DeleteObject(m_hFont);
	m_hFont = hFont;

	lstrcpy(m_FontConfig.m_pszFontFace, lfont.lfFaceName);

	m_FontConfig.m_fFontSize = (float)tm.tmHeight * 72 / lpy;

	m_FontConfig.m_bBoldFace = bBoldFace;
	// 下記の？？については SDK マニュアル参照
	m_FontConfig.m_bProportional
		= ((tm.tmPitchAndFamily & TMPF_FIXED_PITCH) != 0);

	// 文字間隔の設定
	m_nXPitch = tm.tmAveCharWidth;
	// 行間隔の設定
	m_nYPitch = tm.tmHeight + 1;
}

void
FontInfo::setFont(HDC hDC, const FontConfig& fc)
{
	setFont(hDC, fc.m_fFontSize, fc.m_pszFontFace, fc.m_bBoldFace);
}

DrawInfo::DrawInfo(HDC hDC, float fontsize,
				   const char* faceName, bool bBoldFace,
				   COLORREF crFgColorAddr, COLORREF crBkColorAddr,
				   COLORREF crFgColorData, COLORREF crBkColorData,
				   COLORREF crFgColorStr, COLORREF crBkColorStr,
				   COLORREF crFgColorHeader, COLORREF crBkColorHeader)
	: m_hDC(hDC),
	  m_FontInfo(hDC, fontsize, faceName, bBoldFace),
	  m_tciHeader("ヘッダ", crFgColorHeader, crBkColorHeader),
	  m_tciAddress("アドレス", crFgColorAddr, crBkColorAddr),
	  m_tciData("データ", crFgColorData, crBkColorData),
	  m_tciString("文字列", crFgColorStr, crBkColorStr)
{
}

