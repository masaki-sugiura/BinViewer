// $Id$

#ifndef TYPES_H_INC
#define TYPES_H_INC

#ifndef WIN32

#error this project cannot compile with this environment

#endif

#include <windows.h>

typedef HANDLE FILE_HANDLE; //! ファイルハンドル
typedef CRITICAL_SECTION MUTEX_HANDLE; //! ミューテックスハンドル
typedef __int64 filesize_t; //! ファイルサイズ

typedef void* thread_arg_t; //! スレッドの引数
typedef UINT  thread_result_t; //! スレッドの戻り値

//! スレッドの属性を表す構造体
struct ThreadAttribute {
	DWORD  m_dwThreadID;
	HANDLE m_hThread;
};
typedef ThreadAttribute* thread_attr_t;

#define DECLARE_THREAD_PROC(func)  thread_result_t WINAPI func(thread_arg_t)

#define QWORD(l,h)  (__int64)(((__int64)(l) & 0x00000000FFFFFFFF) | \
							  ((__int64)(h) << 32))

#endif // TYPES_H_INC
