// $Id$

#ifndef TYPES_H_INC
#define TYPES_H_INC

#ifndef __GNUC__

#error this project cannot compile with this environment

#endif

#include <stdio.h>

typedef FILE* FILE_HANDLE;
typedef long long filesize_t;

typedef unsigned long  DWORD;
typedef unsigned int   UINT;
typedef unsigned short WORD;
typedef unsigned char  BYTE;

typedef char* LPSTR;
typedef const char* LPCSTR;

#endif // TYPES_H_INC
