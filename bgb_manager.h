// $Id$

#ifndef BGB_MANAGER_H_INC
#define BGB_MANAGER_H_INC

#include "ringbuf.h"
#include "LargeFileReader.h"
#include "thread.h"
#include "auto_ptr.h"

#define FIND_FORWARD   1
#define FIND_BACKWARD -1

struct FindCallbackArg {
	BYTE* m_pData;    // [in] 検索データへのポインタ
	int m_nBufSize;   // [in] 検索データのサイズ
	int m_nDirection; // [in] 検索方向
	filesize_t m_qOrgAddress; // [in] 元の選択開始位置
	int m_nOrgSelectedSize;   // [in] 元の選択サイズ
	filesize_t m_qStartAddress;   // [in] 検索開始位置
	void (*m_pfnCallback)(void*); // [in] 検索終了時に呼び出されるコールバック関数
	filesize_t m_qFindAddress; // [out] 見つかったアドレス
	void* m_pUserData; // [in/out] ユーザー定義の変数
};

struct BGBuffer {
	filesize_t m_qAddress;
	int m_nDataSize;
	int m_nBufSize;
	BYTE* m_DataBuf;

	BGBuffer(int bufsize);
	virtual ~BGBuffer();

	virtual int init(LargeFileReader& LFReader, filesize_t offset);
	virtual void uninit();
};

class BGB_Manager {
public:
	BGB_Manager(int bufsize, int bufcount, LargeFileReader* pLFReader);
	virtual ~BGB_Manager();

	bool loadFile(LargeFileReader* pLFReader);
	void unloadFile();
	bool isLoaded() const { return m_pLFReader != NULL; }

	filesize_t getFileSize() const
	{
		if (!isLoaded()) return -1;
		return m_pLFReader->size();
	}

	filesize_t getCurrentPosition() const
	{
		if (!isLoaded()) return -1;
		return m_qCurrentPos;
	}

	filesize_t setPosition(filesize_t offset)
	{
		if (!isLoaded()) return -1;
		fillBGBuffer(offset);
		return m_qCurrentPos;
	}

	BGBuffer* getCurrentBuffer(int offset = 0)
	{
		if (!isLoaded()) return NULL;
		BGBuffer* pbgb = m_rbBuffers.elementAt(offset);
		if (pbgb && pbgb->m_qAddress < 0) {
			return NULL;
		}
		return pbgb;
	}

	int getMinBufferIndex() const
	{
		return - (m_nBufCount / 2);
	}
	int getMaxBufferIndex() const
	{
		return m_nBufCount / 2;
	}

	BGBuffer* getBuffer(filesize_t offset)
	{
		if (!isLoaded()) return NULL;
		fillBGBuffer(offset);
		return getCurrentBuffer();
	}

	// 実装はＯＳ依存
	virtual bool findCallback(FindCallbackArg* pArg);
	virtual bool stopFind();
	virtual bool waitStopFind();
	virtual bool cleanupCallback();

protected:
	int m_nBufSize;
	int m_nBufCount;
	LargeFileReader* m_pLFReader;
	filesize_t m_qCurrentPos;
	RingBuffer<BGBuffer> m_rbBuffers;
	bool m_bRBInit;
	Auto_Ptr<Thread> m_pThread;
	Lock m_lockFindCallbackData;

	int fillBGBuffer(filesize_t offset);

	virtual BGBuffer* createBGBufferInstance();
};

#endif
