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

//! �A���t�@�x�b�g�啶�����ǂ����`�F�b�N
inline BOOL
IsCharUpperCase(TCHAR ch)
{
	return (char_property[(UCHAR)ch] & IS_UPPERCASE) != 0;
}

//! �A���t�@�x�b�g���������ǂ����`�F�b�N
inline BOOL
IsCharLowerCase(TCHAR ch)
{
	return (char_property[(UCHAR)ch] & IS_LOWERCASE) != 0;
}

//! �A���t�@�x�b�g���ǂ����`�F�b�N
inline BOOL
IsCharAlphabet(TCHAR ch)
{
	return (char_property[(UCHAR)ch] & (IS_UPPERCASE|IS_LOWERCASE)) != 0;
}

//! �������ǂ����`�F�b�N
inline BOOL
IsCharDigit(TCHAR ch)
{
	return (char_property[(UCHAR)ch] & IS_DIGIT) != 0;
}

//! 16�i���������ǂ����`�F�b�N
inline BOOL
IsCharXDigit(TCHAR ch)
{
	return (char_property[(UCHAR)ch] & IS_XDIGIT) != 0;
}

//! �󔒂��ǂ����`�F�b�N
inline BOOL
IsCharSpace(TCHAR ch)
{
	return (char_property[(UCHAR)ch] & IS_SPACE) != 0;
}

//! �Q�o�C�g�����̂P�o�C�g�ڂ��ǂ����`�F�b�N
inline BOOL
IsCharLeadByte(TCHAR ch)
{
	return (char_property[(UCHAR)ch] & IS_LEADBYTE) != 0;
}

//! �Q�o�C�g�����̂Q�o�C�g�ڂ��ǂ����`�F�b�N
inline BOOL
IsCharTrailByte(TCHAR ch)
{
	return (char_property[(UCHAR)ch] & IS_TRAILBYTE) != 0;
}

//! �Ǖ������ǂ����`�F�b�N
inline BOOL
IsCharReadable(TCHAR ch)
{
	return (char_property[(UCHAR)ch] & IS_READABLE) != 0;
}

//! ���̕����փ|�C���^��i�߂�
inline LPSTR
ToNextChar(LPCSTR ptr)
{
	return (LPSTR)(ptr + 1 + IsCharLeadByte(*ptr));
}

//! 16�i�������ɑΉ����鐔�l��Ԃ�
inline BYTE
xdigit(BYTE ch)
{
	assert(IsCharXDigit(ch));
	return IsCharDigit(ch) ? (ch - '0') : ((ch & 0x5F) - 'A' + 10);
}

//! ���l��\��������𐔒l�ɕϊ�
filesize_t ParseNumber(LPCSTR str);

extern const char* const hex;

//! 32bit �����Ȃ�������16�i��������ɕϊ�
inline void
DwordToStr(UINT val, LPSTR buf)
{
	LPSTR ptr = buf + 8;
	while (ptr-- != buf) {
		*ptr = hex[0x0F & val];
		val >>= 4;
	}
}

//! 64bit �����Ȃ�������16�i��������ɕϊ�
inline void
QwordToStr(UINT lo, UINT hi, LPSTR buf)
{
	DwordToStr(hi, buf);
	DwordToStr(lo, buf + 8);
}

#endif
