// $Id$

#include "LargeFileReader.h"
#include "lock.h"

// ファイルポインタの移動
// 64bit 引数＆戻り値ラッパー関数
//
//  hFile  : ファイル HANDLE (SetFilePointer() の第１引数と同じ)
//  pos    : シーク位置
//  origin : シーク位置の指定 (SetFilePointer() の第４引数と同じ)
//
//  return : 新しいファイルポインタの位置。シークに失敗したら -1 を返す。
//
// ※pos がファイルサイズ以上でも -1 を返さないことに注意
//
static inline filesize_t
SetFilePointer64(FILE_HANDLE hFile, filesize_t pos, DWORD origin)
{
	long sizehigh = (long)(pos >> 32);

#ifdef _DEBUG
	if (pos == 0x0000000080000000) {
		::OutputDebugString("");
	}
#endif

	long sizelow = ::SetFilePointer(hFile, (long)pos, &sizehigh, origin);
	if (sizelow == 0xFFFFFFFF && ::GetLastError() != NO_ERROR)
		sizehigh = -1;
	return QWORD(sizelow, sizehigh);
}

#if 0
static DWORD WINAPI
ReadFileProc(LPVOID lpThreadArg)
{
	SynchronizedQueue<ReadFileRequest>*
		pReadReqQueue = (SynchronizedQueue<ReadFileRequest>*)lpThreadArg;

	MSG msg;
	while (1) {
		ReadFileRequest req = pReadReqQueue->pop();
		if (req.m_size < 0) break;
		SetFilePointer64(req.m_hFile, req.m_pos, FILE_BEGIN);
		if (req.m_pos == -1) {
		} else {
			DWORD read_size;
			if (!::ReadFile(req.m_hFile, req.m_buf, req.m_size, &read_size, NULL)) {
				;
			}
		}
	};

}
#endif

// LargeFileReader クラスコンストラクタ
//
//  filename : ファイル名
//  ini_pos  : ファイルポインタの初期位置
//
//  throw : 何らかの理由でファイルオープンに失敗した場合
//
LargeFileReader::LargeFileReader(const string& filename)
	throw(FileOpenError, FileSeekError)
	: m_filename(filename)
{
	m_hFile = ::CreateFile(m_filename.c_str(),
						   GENERIC_READ, FILE_SHARE_READ, NULL,
						   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE)
		throw FileOpenError(m_filename);

	DWORD sizelow, sizehigh;
	sizelow = ::GetFileSize(m_hFile, &sizehigh);
	m_qFileSize = QWORD(sizelow, sizehigh);
}

// LargeFileReader クラスデストラクタ
LargeFileReader::~LargeFileReader()
{
	GetLock lock(m_lockAccess);
	::CloseHandle(m_hFile);
}

// ファイルポインタの指定位置からデータを読み出す
//
//  pos    : ファイルポインタの位置
//  origin : 原点の指定(FILE_BEGIN, FILE_CURRENT, FILE_END)
//  buf    : 読み出したデータを格納するバッファ
//  size   : 読み出すバイト数
//
//  return : 実際に読み出したバイト数。読み出しに失敗したら -1 を返す。
//
int
LargeFileReader::readFrom(filesize_t pos, DWORD origin, BYTE* buf, int size)
{
	GetLock lock(m_lockAccess);

	DWORD readsize;
	if (SetFilePointer64(m_hFile, pos, origin) < 0 ||
		!::ReadFile(m_hFile, buf, (DWORD)size, &readsize, NULL))
		return -1;
	return (int)readsize;
}

