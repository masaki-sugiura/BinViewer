// $Id$

#ifndef LARGEFILEREADER_H_INC
#define LARGEFILEREADER_H_INC

#include "types.h"
#include "lock.h"

#include <string>
using std::string;

#include <exception>
using std::exception;

// #include "sync_que.h"

// LargeFileReader が投げる例外の基底クラス
class FileOpError : public exception {
public:
	FileOpError(const string& filename)
		: m_filename(filename)
	{}
	virtual ~FileOpError() {}

	const string& filename() const
	{
		return m_filename;
	}

	const char* what() const
	{
		return m_filename.c_str();
	}

private:
	string m_filename;
};

// ファイルオープンに失敗した時に投げられる例外
class FileOpenError : public FileOpError {
public:
	FileOpenError(const string& filename)
		: FileOpError(filename)
	{}
};

// ファイルシークに失敗した時に投げられる例外
class FileSeekError : public FileOpError {
public:
	FileSeekError(const string& filename)
		: FileOpError(filename)
	{}
};

// データの読み出しに失敗した時に投げられる例外
class FileReadError : public FileOpError {
public:
	FileReadError(const string& filename)
		: FileOpError(filename)
	{}
};

#if 0
struct ReadFileRequest {
	DWORD   m_queID;
	HANDLE  m_hFile;
	__int64 m_pos;
	DWORD  m_size;
	LPBYTE m_buf;
};
#endif

// 4GB 以上のサイズのファイルの読み出しを扱うクラス
class LargeFileReader {
public:
	LargeFileReader(const string& filename)
		throw(FileOpenError);
	~LargeFileReader();

	int readFrom(filesize_t pos, DWORD origin, BYTE* buf, int size);

	filesize_t size() const { return m_qFileSize; }

private:
	string  m_filename;
	FILE_HANDLE  m_hFile;
//	FILE_HANDLE  m_hThread;
	filesize_t m_qFileSize;
	Lock m_lockAccess;
//	SynchronizedQueue<ReadFileRequest> m_ReadReqQueue;

	// コピー禁止
	LargeFileReader(const LargeFileReader&);
	LargeFileReader& operator=(const LargeFileReader&);
};

#endif
