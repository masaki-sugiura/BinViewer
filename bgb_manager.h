// $Id$

#ifndef BGB_MANAGER_H_INC
#define BGB_MANAGER_H_INC

#include "ringbuf.h"
#include "LF_Notify.h"
#include "LargeFileReader.h"
#include "thread.h"
#include "auto_ptr.h"

#include <assert.h>

struct BGBuffer {
	filesize_t m_qAddress;
	int m_nBufSize, m_nDataSize;
	BYTE* m_pDataBuf;
	BYTE* m_pDataBufHeader;

	BGBuffer(int bufsize)
		: m_qAddress(-1),
		  m_nBufSize(bufsize),
		  m_nDataSize(0)
	{
		m_pDataBufHeader = new BYTE[bufsize + sizeof(DWORD) - 1];
		m_pDataBuf = (BYTE*)(((DWORD)m_pDataBufHeader + sizeof(DWORD) - 1) & ~(sizeof(DWORD) - 1));
	}
	virtual ~BGBuffer()
	{
		delete [] m_pDataBufHeader;
	}

	virtual int init(LargeFileReader& LFReader, filesize_t offset);
	virtual void uninit();

private:
	BGBuffer(const BGBuffer&);
	BGBuffer& operator=(const BGBuffer&);
};

class BGB_Manager {
public:
	BGB_Manager(int bufsize, int bufcount)
		: m_nBufSize(bufsize),
		  m_nBufCount(bufcount),
		  m_qCurrentPos(-1),
		  m_pLFAcceptor(NULL),
		  m_bRBInit(false)
	{
		assert(m_nBufCount > 2);
		m_nBufCount |= 1;
	}
	virtual ~BGB_Manager() {}

	bool onLoadFile(LF_Acceptor* pLFAcceptor)
	{
		m_qCurrentPos = -1; // 最初の呼び出しで fillBGBuffer() で init() を呼ぶのに必要
		m_pLFAcceptor = pLFAcceptor;
		return isLoaded();
	}

	void onUnloadFile()
	{
		m_qCurrentPos = -1;

		// リングバッファの要素を全て無効に
		if (m_bRBInit) {
			int count = m_rbBuffers.count();
			for (int i = 0; i < count; i++) {
				m_rbBuffers.elementAt(i)->uninit();
			}
		}
	}

	bool isLoaded() const
	{
		if (!m_pLFAcceptor) return false;
		LargeFileReader* pLFReader;
		bool bRet = m_pLFAcceptor->tryLockReader(&pLFReader, INFINITE);
		if (!bRet) return false;
		m_pLFAcceptor->releaseReader(pLFReader);
		return pLFReader != NULL;
	}

	filesize_t getFileSize() const
	{
		if (!m_pLFAcceptor) return -1;
		LargeFileReader* pLFReader;
		bool bRet = m_pLFAcceptor->tryLockReader(&pLFReader, INFINITE);
		if (!bRet) return -1;
		filesize_t size = -1;
		if (pLFReader) size = pLFReader->size();
		m_pLFAcceptor->releaseReader(pLFReader);
		return size;
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

	int bufSize() const { return m_nBufSize; }
	int bufCount() const { return m_nBufCount; }

protected:
	int m_nBufSize, m_nBufCount;
	filesize_t m_qCurrentPos;
	LF_Acceptor* m_pLFAcceptor;
	RingBuffer<BGBuffer> m_rbBuffers;
	bool m_bRBInit;

	bool fillBGBuffer(filesize_t offset);

	virtual BGBuffer* createBGBufferInstance() = 0;

	BGB_Manager(const BGB_Manager&);
	BGB_Manager& operator=(const BGB_Manager&);
};

#endif
