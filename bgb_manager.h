// $Id$

#ifndef BGB_MANAGER_H_INC
#define BGB_MANAGER_H_INC

#include "ringbuf.h"
#include "LargeFileReader.h"
#include "thread.h"
#include "auto_ptr.h"

#define MAX_DATASIZE_PER_BUFFER  1024 // 1KB

#define BUFFER_NUM  4

#define FIND_FORWARD   1
#define FIND_BACKWARD -1

struct FindCallbackArg {
	BYTE* m_pData;    // [in] �����f�[�^�ւ̃|�C���^
	int m_nBufSize;   // [in] �����f�[�^�̃T�C�Y
	int m_nDirection; // [in] ��������
	filesize_t m_qOrgAddress; // [in] ���̑I���J�n�ʒu
	int m_nOrgSelectedSize;   // [in] ���̑I���T�C�Y
	filesize_t m_qStartAddress;   // [in] �����J�n�ʒu
	void (*m_pfnCallback)(void*); // [in] �����I�����ɌĂяo�����R�[���o�b�N�֐�
	filesize_t m_qFindAddress; // [out] ���������A�h���X
};

struct BGBuffer {
	filesize_t m_qAddress;
	int m_nDataSize;
	BYTE m_DataBuf[MAX_DATASIZE_PER_BUFFER];

	BGBuffer();
	virtual ~BGBuffer();

	virtual int init(LargeFileReader& LFReader, filesize_t offset);
	virtual void uninit();
};

class BGB_Manager {
public:
	BGB_Manager(LargeFileReader* pLFReader = NULL);
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
		return - ((BUFFER_NUM - 1) / 2);
	}
	int getMaxBufferIndex() const
	{
		return (BUFFER_NUM - 1) / 2;
	}

	BGBuffer* getBuffer(filesize_t offset)
	{
		if (!isLoaded()) return NULL;
		fillBGBuffer(offset);
		return getCurrentBuffer();
	}

	// �����͂n�r�ˑ�
	virtual bool findCallback(FindCallbackArg* pArg);
	virtual bool stopFind();
	virtual bool cleanupCallback();

protected:
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
