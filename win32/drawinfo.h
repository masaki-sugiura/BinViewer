// $Id$

#ifndef DRAWINFO_H_INC
#define DRAWINFO_H_INC

#include <windows.h>
#include <assert.h>
#include <exception>
#include <string>
using std::exception;
using std::string;

class DrawInfo {
public:
	DrawInfo();
	virtual ~DrawInfo();

	HDC getDC() const { return m_hDC; }
	int getWidth() const { return m_nWidth; }
	int getHeight() const { return m_nHeight; }
	int getPixelsPerLine() const { return m_nPixelsPerLine; }
	COLORREF getBkColor() const { return m_crBkColor; }
	HBRUSH getBkBrush() const { return m_hbrBackground; }

	void setDC(HDC hDC) { m_hDC = hDC; }
	void setWidth(int width) { m_nWidth = width; }
	void setHeight(int height) { m_nHeight = height; }
	void setPixelsPerLine(int nPixelsPerLine)
	{
		m_nPixelsPerLine = nPixelsPerLine;
	}
	void setBkColor(COLORREF crBkColor)
	{
		::DeleteObject(m_hbrBackground);
		m_hbrBackground = ::CreateSolidBrush(crBkColor);
		m_crBkColor = crBkColor;
	}

protected:
	HDC m_hDC;
	int m_nWidth;
	int m_nHeight;
	int m_nPixelsPerLine;
	COLORREF m_crBkColor;
	HBRUSH m_hbrBackground;
};

class InvalidFontError : public exception {
};

struct ColorConfig {
	COLORREF m_crFgColor, m_crBkColor;
};

class TextColorInfo {
public:
	TextColorInfo(const string& name, COLORREF crFgColor, COLORREF crBkColor);
	~TextColorInfo();

	void setColor(const ColorConfig& cc);
	void setColor(COLORREF crFgColor, COLORREF crBkColor);
	COLORREF getFgColor() const { return m_ColorConfig.m_crFgColor; }
	COLORREF getBkColor() const { return m_ColorConfig.m_crBkColor; }
	const ColorConfig& getColorConfig() const
	{
		return m_ColorConfig;
	}

	const string& getName() const { return m_strName; }
	HBRUSH getBkBrush() const { return m_hbrBackground; }

	void setColorToDC(HDC hDC) const
	{
		::SetTextColor(hDC, m_ColorConfig.m_crFgColor);
		::SetBkColor(hDC, m_ColorConfig.m_crBkColor);
	}

private:
	HBRUSH m_hbrBackground;
	const string m_strName;
	ColorConfig m_ColorConfig;

	TextColorInfo(const TextColorInfo&);
	TextColorInfo& operator=(const TextColorInfo&);
};

struct FontConfig {
	float  m_fFontSize;
	bool   m_bBoldFace;
	bool   m_bProportional;
	char   m_pszFontFace[LF_FACESIZE];
};

class FontInfo {
public:
	FontInfo(HDC hDC, float fontsize,
			 const char* faceName = NULL, bool bBoldFace = false);
	~FontInfo();

	float getFontSize() const
	{
		return m_FontConfig.m_fFontSize;
	}
	const char* getFaceName() const
	{
		return m_FontConfig.m_pszFontFace;
	}
	bool isBoldFace() const
	{
		return m_FontConfig.m_bBoldFace;
	}
	bool isProportional() const
	{
		return m_FontConfig.m_bProportional;
	}
	const FontConfig& getFontConfig() const
	{
		return m_FontConfig;
	}

	void setFont(HDC hDC, float fontsize,
				 const char* faceName = NULL,
				 bool bBoldFace = false);
	void setFont(HDC hDC, const FontConfig& fc);

	HFONT getFont() const
	{
		return m_hFont;
	}
	int getXPitch() const
	{
		return m_nXPitch;
	}
	int getYPitch() const
	{
		return m_nYPitch;
	}

private:
	int m_nXPitch, m_nYPitch;
	HFONT  m_hFont;
	FontConfig m_FontConfig;

	FontInfo(const FontInfo&);
	FontInfo& operator=(const FontInfo&);
};

typedef enum {
	CARET_STATIC = 0,
	CARET_ENSURE_VISIBLE = 1,
	CARET_SCROLL = 2,
	CARET_LAST
} CARET_MOVE;

typedef enum {
	WHEEL_AS_ARROW_KEYS = 0,
	WHEEL_AS_SCROLL_BAR = 1,
	WHEEL_LAST
} WHEEL_SCROLL;

struct ScrollConfig {
	CARET_MOVE   m_caretMove;
	WHEEL_SCROLL m_wheelScroll;
	ScrollConfig(CARET_MOVE caretMove, WHEEL_SCROLL wheelScroll)
		: m_caretMove(caretMove), m_wheelScroll(wheelScroll)
	{
//		m_caretMove = CARET_SCROLL;
//		m_wheelScroll = WHEEL_AS_SCROLL_BAR;
	}
};

#endif
