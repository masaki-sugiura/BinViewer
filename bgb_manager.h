// $Id$

#ifndef BGB_MANAGER_H_INC
#define BGB_MANAGER_H_INC

#include "ringbuf.h"
#include "LargeFileReader.h"
#include "thread.h"
#include "auto_ptr.h"

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

protected:
	int m_nBufSize;
	int m_nBufCount;
	LargeFileReader* m_pLFReader;
	filesize_t m_qCurrentPos;
	RingBuffer<BGBuffer> m_rbBuffers;
	bool m_bRBInit;

	int fillBGBuffer(filesize_t offset);

	virtual BGBuffer* createBGBufferInstance();
};

#endif
