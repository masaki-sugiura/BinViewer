// $Id$

#ifndef DRAWINFO_H_INC
#define DRAWINFO_H_INC

#include <windows.h>
#include <assert.h>
#include <exception>
#include <string>
using std::exception;
using std::string;

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

#define TCI_HEADER  0
#define TCI_ADDRESS 1
#define TCI_DATA    2
#define TCI_STRING  3

class InvalidIndexError : public exception {
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
	TextColorInfo m_tciHeader, m_tciAddress, m_tciData, m_tciString;

	TextColorInfo& getTextColorInfo(int index)
	{
		switch (index) {
		case TCI_HEADER:
			return m_tciHeader;
		case TCI_ADDRESS:
			return m_tciAddress;
		case TCI_DATA:
			return m_tciData;
		case TCI_STRING:
			return m_tciString;
		default:
			assert(0);
			throw InvalidIndexError();
		}
	}
	const TextColorInfo& getTextColorInfo(int index) const
	{
		return const_cast<DrawInfo*>(this)->getTextColorInfo(index);
	}

private:
	DrawInfo(const DrawInfo&);
	DrawInfo& operator=(const DrawInfo&);
};

#endif
