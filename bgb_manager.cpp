// $Id$

#include "bgb_manager.h"
#include "thread.h"

#include <assert.h>

BGBuffer::BGBuffer()
	: m_qAddress(-1),
	  m_nDataSize(0)
{
}

BGBuffer::~BGBuffer()
{
}

int
BGBuffer::init(LargeFileReader& LFReader, filesize_t offset)
{
	// ���ɓǂݍ��ݍς݂��ǂ���
	if (m_qAddress == offset) return m_nDataSize;

	m_qAddress = offset;

	m_nDataSize = LFReader.readFrom(m_qAddress, FILE_BEGIN,
									m_DataBuf, MAX_DATASIZE_PER_BUFFER);
	if (m_nDataSize >= 0) return m_nDataSize;

	// �ǂݍ��݂Ɏ��s���o�b�t�@�̖�����
	m_qAddress  = -1;
	m_nDataSize = 0;

	return -1;
}

void
BGBuffer::uninit()
{
	m_qAddress = -1;
	m_nDataSize = 0;
}


/*
	�����O�o�b�t�@�ɂ��o�b�t�@�̊Ǘ��F

					[-1: m_qCPos - BSIZE �` m_pCPos - 1]
							����					����
	[0 : m_qCPos �` m_qCPos + BSIZE - 1]		[2 : ����(�܂��͑O��̒l)]
							����					����
					[1 : m_qCPos + BSIZE �` m_pCPos + 2 * BSIZE - 1]

	�S�̃o�b�t�@�̂��� -1, 0, 1 �Ԗڂ̗v�f����ɗL���ɂ��Ă����B
	(�t�@�C���̐擪�܂��͏I�[�̏ꍇ�� -1, �܂��� 1 �Ԗڂ̗v�f�͖���)
 */

BGB_Manager::BGB_Manager(LargeFileReader* pLFReader)
	: m_pLFReader(pLFReader),
	  m_qCurrentPos(-1),
	  m_bRBInit(false),
	  m_pThread(NULL)
{
}

BGB_Manager::~BGB_Manager()
{
}

bool
BGB_Manager::loadFile(LargeFileReader* pLFReader)
{
	m_pLFReader = pLFReader;
	m_qCurrentPos = -1; // �ŏ��̌Ăяo���� fillBGBuffer() �� init() ���ĂԂ̂ɕK�v
	return isLoaded();
}

void
BGB_Manager::unloadFile()
{
	m_pLFReader = NULL;
	m_qCurrentPos = -1;

	// �����O�o�b�t�@�̗v�f��S�Ė�����
	if (m_bRBInit) {
		int count = m_rbBuffers.count();
		for (int i = 0; i < count; i++) {
			m_rbBuffers.elementAt(i)->uninit();
		}
	}
}

BGBuffer*
BGB_Manager::createBGBufferInstance()
{
	return new BGBuffer();
}

int
BGB_Manager::fillBGBuffer(filesize_t offset)
{
	assert(offset >= 0);

	if (!m_bRBInit) { // �ŏ��̌Ăяo��
		// BUFFER_NUM �̃����O�o�b�t�@�v�f��ǉ�
		for (int i = 0; i < BUFFER_NUM; i++) {
			m_rbBuffers.addElement(createBGBufferInstance(), i - 1);
		}
		m_bRBInit = true;
	}

	if (!isLoaded()) return -1;

	// offset - MAX_DATASIZE_PER_BUFFER ���� offset + 2 * MAX_DATASIZE_PER_BUFFER - 1
	// �̃f�[�^���t�@�C������ǂݏo��

	// MAX_DATASIZE_PER_BUFFER �o�C�g�ŃA���C�����g
	offset = (offset / MAX_DATASIZE_PER_BUFFER) * MAX_DATASIZE_PER_BUFFER;

	// ���݂̃o�b�t�@���X�g�ɑ}������ׂ����A
	// ���݂̃��X�g��(�S�āA�܂��͈ꕔ)�j�����ēǂݍ��ނׂ����𒲂ׂ�

	if (m_qCurrentPos == offset) {
		// ���݂Ɠ����o�b�t�@��Ԃ�
	} else if (offset == m_qCurrentPos + MAX_DATASIZE_PER_BUFFER) {
		// 0, 1 �Ԗڂ̃o�b�t�@�͍ė��p�\
		m_rbBuffers.setTop(1); // 0 -> -1, 1 -> 0
		m_rbBuffers.elementAt(1)->init(*m_pLFReader, offset + MAX_DATASIZE_PER_BUFFER);
	} else if (offset == m_qCurrentPos + 2 * MAX_DATASIZE_PER_BUFFER) {
		// 1 �Ԗڂ̃o�b�t�@�͍ė��p�\
		m_rbBuffers.setTop(2); // 1 -> -1
		m_rbBuffers.elementAt(0)->init(*m_pLFReader, offset);
		m_rbBuffers.elementAt(1)->init(*m_pLFReader, offset + MAX_DATASIZE_PER_BUFFER);
	} else if (offset == m_qCurrentPos - MAX_DATASIZE_PER_BUFFER) {
		// 0, -1 �Ԗڂ̃o�b�t�@�͍ė��p�\
		m_rbBuffers.setTop(-1); // -1 -> 0, 0 -> 1
		m_rbBuffers.elementAt(-1)->init(*m_pLFReader, offset - MAX_DATASIZE_PER_BUFFER);
	} else if (offset == m_qCurrentPos + 2 * MAX_DATASIZE_PER_BUFFER) {
		// -1 �Ԗڂ̃o�b�t�@�͍ė��p�\
		m_rbBuffers.setTop(-2); // -1 -> 1
		m_rbBuffers.elementAt(-1)->init(*m_pLFReader, offset - MAX_DATASIZE_PER_BUFFER);
		m_rbBuffers.elementAt(0)->init(*m_pLFReader, offset);
	} else {
		// �S�Ă�j�����ēǂݍ��� (m_qCurrentPos == -1 �̏ꍇ���܂�)
		m_rbBuffers.elementAt(-1)->init(*m_pLFReader, offset - MAX_DATASIZE_PER_BUFFER);
		m_rbBuffers.elementAt(0)->init(*m_pLFReader, offset);
		m_rbBuffers.elementAt(1)->init(*m_pLFReader, offset + MAX_DATASIZE_PER_BUFFER);
	}

	// �J�����g�|�W�V�����̕ύX
	m_qCurrentPos = offset;

	// �o�b�t�@�ɒ��߂��Ă���f�[�^�T�C�Y�̌v�Z
	// (�A�� 2 �ȏ�܂��� -2 �ȉ��̗v�f�͊܂߂Ȃ�)
	int totalsize = 0;
	for (int i = -1; i <= 1; i++) {
		BGBuffer* pbgb = m_rbBuffers.elementAt(i);
		if (pbgb->m_qAddress >= 0)
			totalsize += pbgb->m_nDataSize;
	}

	return totalsize;
}

// �����p�̃X���b�h�̎���

struct ThreadProcArg {
//	BGB_Manager* m_pBGB_Manager;
	LargeFileReader* m_pLFReader;
	FindCallbackArg* m_pCallbackArg;
};

class FindThread : public Thread {
public:
	FindThread(ThreadProcArg* arg, thread_attr_t attr)
		: Thread(arg, attr)
	{}

	thread_result_t thread(thread_arg_t arg);
};

thread_result_t
FindThread::thread(thread_arg_t arg)
{
	ThreadProcArg* pThreadArg = (ThreadProcArg*)arg;
	LargeFileReader* pLFReader = pThreadArg->m_pLFReader;
	FindCallbackArg* pCallbackArg = pThreadArg->m_pCallbackArg;
	filesize_t pos = pCallbackArg->m_qStartAddress;
	const BYTE* data = pCallbackArg->m_pData;
	int size = pCallbackArg->m_nBufSize;

	int bufsize = MAX_DATASIZE_PER_BUFFER
			* ((MAX_DATASIZE_PER_BUFFER + size - 1) / MAX_DATASIZE_PER_BUFFER);

	BYTE* buf = new BYTE[bufsize];

	int readsize = bufsize, offset = 0;
	bool match = true;

	DWORD ret = 0;

	if (pCallbackArg->m_nDirection == FIND_FORWARD) {
		while (!isTerminated()) {
			// �}�b�`����̂ɏ\���ȃT�C�Y�����邩�H
			if ((readsize = pLFReader->readFrom(pos, FILE_BEGIN,
												buf + offset, readsize)) <= 0 ||
				readsize + offset < size) {
				ret = -1;
				goto _exit_thread;
			}

			// ����
			int searchsize = readsize + offset - size;
			for (int i = 0; i < searchsize; i++) {
				match = true;
				for (int j = 0; j < size; j++) {
					if (buf[i + j] != data[j]) {
						match = false;
						break;
					}
				}
				if (match) {
					pCallbackArg->m_qFindAddress = pos + i - offset;
					goto _exit_thread;
				}
			}
			pos += readsize;
			// size byte ���c�����o�b�t�@�̐擪�Ɉړ�
			memmove(buf, buf + bufsize - size, size);
			// �o�b�t�@�̊i�[�ʒu�̒���
			offset = size;
			readsize = bufsize - offset;
		}
	} else {
		if (pos - bufsize < 0) {
			if (!pos) {
				ret = -1;
				goto _exit_thread;
			}
			readsize = (int)pos;
		}
		while (!isTerminated()) {
			pos -= readsize;
			if (pos < 0) {
				readsize -= (int)-pos;
				pos = 0;
			}

			// �}�b�`����̂ɏ\���ȃT�C�Y�����邩�H
			if ((readsize = pLFReader->readFrom(pos, FILE_BEGIN,
												buf + offset, readsize)) <= 0 ||
				readsize < size) {
				ret = -1;
				goto _exit_thread;
			}

			// ����
			int searchsize = readsize + offset - size;
			for (int i = searchsize; i > 0; i--) {
				match = true;
				for (int j = 0; j < size; j++) {
					if (buf[i + j] != data[j]) {
						match = false;
						break;
					}
				}
				if (match) {
					pCallbackArg->m_qFindAddress = pos + i - offset;
					goto _exit_thread;
				}
			}
			// size byte ���c�����o�b�t�@�̏I�[�Ɉړ�
			memmove(buf + bufsize - size, buf, size);
			offset = size;
			readsize = bufsize - offset;
		}
	}

	assert(ret == -1);

_exit_thread:
	delete [] buf;

	(*pCallbackArg->m_pfnCallback)(pCallbackArg);

	return ret;
}

bool
BGB_Manager::findCallback(FindCallbackArg* pArg)
{
	if (m_pThread.ptr()) return false;

	GetLock lock(m_lockFindCallbackData);

	ThreadProcArg* pThreadArg = new ThreadProcArg;
//	pThreadArg->m_pBGB_Manager = this;
	pThreadArg->m_pLFReader = m_pLFReader;
	pThreadArg->m_pCallbackArg = pArg;

	m_pThread = new FindThread(pThreadArg, new ThreadAttribute);
	if (m_pThread->run()) return true;

	thread_attr_t attr = m_pThread->getAttribute();
	m_pThread = NULL;
	delete attr;
	delete pThreadArg;

	return false;
}

bool
BGB_Manager::stopFind()
{
	if (!m_pThread.ptr()) return false;

	GetLock lock(m_lockFindCallbackData);

	return m_pThread->join();
}

bool
BGB_Manager::cleanupCallback()
{
	if (!m_pThread.ptr()) return false;

	GetLock lock(m_lockFindCallbackData);

	ThreadProcArg* parg = (ThreadProcArg*)m_pThread->getArg();
	thread_attr_t attr = m_pThread->getAttribute();
	m_pThread = NULL;
	delete parg;
	delete attr;

	return true;
}

