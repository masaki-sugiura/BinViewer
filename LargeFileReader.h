// $Id$

#ifndef LARGEFILEREADER_H_INC
#define LARGEFILEREADER_H_INC

#include "types.h"
#include "lock.h"
#include "thread.h"

#include <string>
using std::string;

#include <exception>
using std::exception;

//! LargeFileReader が投げる例外の基底クラス
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

//! ファイルオープンに失敗した時に投げられる例外
class FileOpenError : public FileOpError {
public:
	FileOpenError(const string& filename)
		: FileOpError(filename)
	{}
};

//! ファイルシークに失敗した時に投げられる例外
class FileSeekError : public FileOpError {
public:
	FileSeekError(const string& filename)
		: FileOpError(filename)
	{}
};

//! データの読み出しに失敗した時に投げられる例外
class FileReadError : public FileOpError {
public:
	FileReadError(const string& filename)
		: FileOpError(filename)
	{}
};

//! 4GB 以上のサイズのファイルの読み出しを扱うクラス
class LargeFileReader {
public:
	LargeFileReader(const string& filename)
		throw(FileOpenError);
	~LargeFileReader();

	/*! 指定位置(pos, origin) から size バイトのデータを読み出して
		buf に格納する */
	int readFrom(filesize_t pos, DWORD origin, BYTE* buf, int size);

	//! ファイルサイズを返す
	filesize_t size() const { return m_qFileSize; }

private:
	string  m_filename; //! ファイル名
	FILE_HANDLE  m_hFile; //! ファイルハンドル(実体はＯＳ依存)
	filesize_t m_qFileSize; //! ファイルサイズ
	Lock m_lockAccess; //! 同期アクセスオブジェクト

	// コピー禁止
	LargeFileReader(const LargeFileReader&);
	LargeFileReader& operator=(const LargeFileReader&);
};

//! 検索方向の指定
enum FIND_DIRECTION {
	FIND_FORWARD  =  1,
	FIND_NONE     =  0,
	FIND_BACKWARD = -1
};

//! 非同期検索が終了したときに呼ばれるコールバック関数の引数
struct FindCallbackArg {
	BYTE* m_pData;    //! [in] 検索データへのポインタ
	int m_nBufSize;   //! [in] 検索データのサイズ
	FIND_DIRECTION m_nDirection; //! [in] 検索方向
	filesize_t m_qOrgAddress; //! [in] 元の選択開始位置
	int m_nOrgSelectedSize;   //! [in] 元の選択サイズ
	filesize_t m_qStartAddress;   //! [in] 検索開始位置
	filesize_t m_qFindAddress; //! [out] 見つかったアドレス
	void (*m_pfnCallback)(FindCallbackArg*); //! [in] 検索終了時に呼び出されるコールバック関数
	void* m_pUserData; //! [in/out] ユーザー定義の変数
};

//! 非同期検索スレッドに渡す引数
struct FindThreadProcArg {
//	LargeFileReader* m_pLFReader;
	class LF_Acceptor* m_pLFAcceptor;
	FindCallbackArg* m_pCallbackArg;
};

//! 検索用のスレッド
class FindThread : public Thread {
public:
	FindThread(FindThreadProcArg* arg, thread_attr_t attr)
		: Thread(arg, attr)
	{}

	thread_result_t thread(thread_arg_t arg);
};

#endif
