// $Id$

#ifndef BGB_MANAGER_H_INC
#define BGB_MANAGER_H_INC

#include "ringbuf.h"
#include "LargeFileReader.h"
#include "thread.h"
#include "auto_ptr.h"

#include <assert.h>

template<int nBufSize>
struct BGBuffer {
	filesize_t m_qAddress;
	int m_nDataSize;
	BYTE m_DataBuf[nBufSize];

	BGBuffer()
		: m_qAddress(-1),
		  m_nDataSize(0)
	{}
	virtual ~BGBuffer() {}

	virtual int init(LargeFileReader& LFReader, filesize_t offset);
	virtual void uninit();

private:
	BGBuffer(const BGBuffer&);
	BGBuffer& operator=(const BGBuffer&);
};

template<int nBufSize>
class BGB_Manager {
public:
	BGB_Manager(int bufcount, LargeFileReader* pLFReader)
		: m_nBufCount(bufcount),
		  m_pLFReader(pLFReader),
		  m_qCurrentPos(-1),
		  m_bRBInit(false)
	{
		assert(m_nBufCount > 2);
		m_nBufCount |= 1;
	}
	virtual ~BGB_Manager() {}

	bool loadFile(LargeFileReader* pLFReader)
	{
		m_pLFReader = pLFReader;
		m_qCurrentPos = -1; // 最初の呼び出しで fillBGBuffer() で init() を呼ぶのに必要
		return isLoaded();
	}
	void unloadFile()
	{
		m_pLFReader = NULL;
		m_qCurrentPos = -1;

		// リングバッファの要素を全て無効に
		if (m_bRBInit) {
			int count = m_rbBuffers.count();
			for (int i = 0; i < count; i++) {
				m_rbBuffers.elementAt(i)->uninit();
			}
		}
	}

	bool isLoaded() const { return m_pLFReader != NULL; }
	LargeFileReader* getReader() { return m_pLFReader; }

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

	BGBuffer<nBufSize>* getCurrentBuffer(int offset = 0)
	{
		if (!isLoaded()) return NULL;
		BGBuffer<nBufSize>* pbgb = m_rbBuffers.elementAt(offset);
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

	BGBuffer<nBufSize>* getBuffer(filesize_t offset)
	{
		if (!isLoaded()) return NULL;
		fillBGBuffer(offset);
		return getCurrentBuffer();
	}

protected:
	int m_nBufCount;
	LargeFileReader* m_pLFReader;
	filesize_t m_qCurrentPos;
	RingBuffer< BGBuffer<nBufSize> > m_rbBuffers;
	bool m_bRBInit;

	int fillBGBuffer(filesize_t offset);

	virtual BGBuffer<nBufSize>* createBGBufferInstance()
	{
		return new BGBuffer<nBufSize>();
	}

	BGB_Manager(const BGB_Manager<nBufSize>&);
	BGB_Manager<nBufSize>& operator=(const BGB_Manager<nBufSize>&);
};

#include "bgb_manager.cpp"

#endif
