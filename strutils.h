// $Id$

#ifndef	STRUTILS_H_INC
#define	STRUTILS_H_INC

#include "types.h"
#include <assert.h>

#define IS_LOWERCASE 0x01
#define IS_UPPERCASE 0x02
#define IS_DIGIT     0x04
#define IS_XDIGIT    0x08
#define IS_SPACE     0x10
#define IS_LEADBYTE  0x20
#define IS_TRAILBYTE 0x40
#define IS_READABLE  0x80

extern UCHAR char_property[];

//! アルファベット大文字かどうかチェック
inline BOOL
IsCharUpperCase(TCHAR ch)
{
	return (char_property[(UCHAR)ch] & IS_UPPERCASE) != 0;
}

//! アルファベット小文字かどうかチェック
inline BOOL
IsCharLowerCase(TCHAR ch)
{
	return (char_property[(UCHAR)ch] & IS_LOWERCASE) != 0;
}

//! アルファベットかどうかチェック
inline BOOL
IsCharAlphabet(TCHAR ch)
{
	return (char_property[(UCHAR)ch] & (IS_UPPERCASE|IS_LOWERCASE)) != 0;
}

//! 数字かどうかチェック
inline BOOL
IsCharDigit(TCHAR ch)
{
	return (char_property[(UCHAR)ch] & IS_DIGIT) != 0;
}

//! 16進数文字かどうかチェック
inline BOOL
IsCharXDigit(TCHAR ch)
{
	return (char_property[(UCHAR)ch] & IS_XDIGIT) != 0;
}

//! 空白かどうかチェック
inline BOOL
IsCharSpace(TCHAR ch)
{
	return (char_property[(UCHAR)ch] & IS_SPACE) != 0;
}

//! ２バイト文字の１バイト目かどうかチェック
inline BOOL
IsCharLeadByte(TCHAR ch)
{
	return (char_property[(UCHAR)ch] & IS_LEADBYTE) != 0;
}

//! ２バイト文字の２バイト目かどうかチェック
inline BOOL
IsCharTrailByte(TCHAR ch)
{
	return (char_property[(UCHAR)ch] & IS_TRAILBYTE) != 0;
}

//! 可読文字かどうかチェック
inline BOOL
IsCharReadable(TCHAR ch)
{
	return (char_property[(UCHAR)ch] & IS_READABLE) != 0;
}

//! 次の文字へポインタを進める
inline LPSTR
ToNextChar(LPCSTR ptr)
{
	return (LPSTR)(ptr + 1 + IsCharLeadByte(*ptr));
}

//! 16進数文字に対応する数値を返す
inline BYTE
xdigit(BYTE ch)
{
	assert(IsCharXDigit(ch));
	return IsCharDigit(ch) ? (ch - '0') : ((ch & 0x5F) - 'A' + 10);
}

//! 数値を表す文字列を数値に変換
filesize_t ParseNumber(LPCSTR str);

extern const char* const hex;

//! 32bit 符号なし整数を16進数文字列に変換
inline void
DwordToStr(UINT val, LPSTR buf)
{
	LPSTR ptr = buf + 8;
	while (ptr-- != buf) {
		*ptr = hex[0x0F & val];
		val >>= 4;
	}
}

//! 64bit 符号なし整数を16進数文字列に変換
inline void
QwordToStr(UINT lo, UINT hi, LPSTR buf)
{
	DwordToStr(hi, buf);
	DwordToStr(lo, buf + 8);
}

#endif
