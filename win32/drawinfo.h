// $Id$

#ifndef DRAWINFO_H_INC
#define DRAWINFO_H_INC

#include <windows.h>
#include <exception>
using std::exception;

class InvalidFontError : public exception {
};

class TextColorInfo {
public:
	TextColorInfo(COLORREF crFgColor, COLORREF crBkColor);
	~TextColorInfo();

	void setColor(COLORREF crFgColor, COLORREF crBkColor);
	COLORREF getFgColor() const { return m_crFgColor; }
	COLORREF getBkColor() const { return m_crBkColor; }
	HBRUSH   getBkBrush() const { return m_hbrBackground; }

	void setColorToDC(HDC hDC) const
	{
		::SetTextColor(hDC, m_crFgColor);
		::SetBkColor(hDC, m_crBkColor);
	}

private:
	HBRUSH m_hbrBackground;
	COLORREF m_crFgColor, m_crBkColor;

	TextColorInfo(const TextColorInfo&);
	TextColorInfo& operator=(const TextColorInfo&);
};

class FontInfo {
public:
	FontInfo(HDC hDC, float fontsize,
			 const char* faceName = NULL, bool bBoldFace = false);
	~FontInfo();

	int getXPitch() const
	{
		return m_nXPitch;
	}
	int getYPitch() const
	{
		return m_nYPitch;
	}

	float getFontSize() const
	{
		return m_fFontSize;
	}
	const char* getFaceName() const
	{
		return m_pszFontFace;
	}
	bool isBoldFace() const
	{
		return m_bBoldFace;
	}
	bool isProportional() const
	{
		return m_bProportional;
	}

	void setFont(HDC hDC, float fontsize,
				 const char* faceName = NULL,
				 bool bBoldFace = false);
	HFONT getFont() const
	{
		return m_hFont;
	}

private:
	int m_nXPitch, m_nYPitch;
	HFONT  m_hFont;
	float  m_fFontSize;
	bool   m_bBoldFace;
	bool   m_bProportional;
	char   m_pszFontFace[LF_FACESIZE];

	FontInfo(const FontInfo&);
	FontInfo& operator=(const FontInfo&);
};

class DrawInfo {
public:
	DrawInfo(HDC hDC, float fontsize,
			 const char* faceName, bool bBoldFace,
			 COLORREF crFgColorAddr, COLORREF crBkColorAddr,
			 COLORREF crFgColorData, COLORREF crBkColorData,
			 COLORREF crFgColorStr, COLORREF crBkColorStr,
			 COLORREF crFgColorHeader, COLORREF crBkColorHeader);

	HDC m_hDC;
	FontInfo m_FontInfo;
	TextColorInfo m_tciAddress, m_tciData, m_tciString, m_tciHeader;

private:
	DrawInfo(const DrawInfo&);
	DrawInfo& operator=(const DrawInfo&);
};

#endif
