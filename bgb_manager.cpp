// $Id$

#include "bgb_manager.h"
#include "thread.h"

#include <assert.h>

BGBuffer::BGBuffer(int bufsize)
	: m_qAddress(-1),
	  m_nDataSize(0),
	  m_nBufSize(bufsize)
{
	assert(bufsize > 0);
	m_DataBuf = new BYTE[bufsize];
}

BGBuffer::~BGBuffer()
{
	delete [] m_DataBuf;
}

int
BGBuffer::init(LargeFileReader& LFReader, filesize_t offset)
{
	// ���ɓǂݍ��ݍς݂��ǂ���
	if (m_qAddress == offset) return m_nDataSize;

	m_qAddress = offset;

	m_nDataSize = LFReader.readFrom(m_qAddress, FILE_BEGIN,
									m_DataBuf, m_nBufSize);
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
	2k + 1 �̃����O�o�b�t�@�ɂ��o�b�t�@�̊Ǘ��F

	[-k: m_qCPos - k * BSIZE �` m_pCPos - (k - 1) * BSIZE - 1]
					����
				[..........]
					����
	[-2: m_qCPos - 2 * BSIZE �` m_pCPos - BSIZE - 1]
					����
	[-1: m_qCPos - BSIZE �` m_pCPos - 1]
					����
	[0 : m_qCPos �` m_qCPos + BSIZE - 1]
					����
	[1 : m_qCPos + BSIZE �` m_pCPos + 2 * BSIZE - 1]
					����
				[..........]
					����
	[k : m_qCPos + k * BSIZE �` m_pCPos + (k + 1) * BSIZE - 1]

	(��ʂ� [j : m_qCPos + j * BSIZE �` m_pCPos + (j + 1) * BSIZE - 1] (-k <= j <= k))

	1) m_qCPos -> m_qCPos + m (0 < m <= k) �̏ꍇ
	[-k: m_qCPos + (m - k) * BSIZE �` m_pCPos + (m - k + 1) * BSIZE - 1]
					����
				[..........]
					����
	[-2: m_qCPos + (m - 2) * BSIZE �` m_pCPos + (m - 1) * BSIZE - 1]
					����
	[-1: m_qCPos + (m - 1) * BSIZE �` m_pCPos + m * BSIZE - 1]
					����
	[0 : m_qCPos + m * BSIZE �` m_qCPos + (m + 1) * BSIZE - 1]
					����
	[1 : m_qCPos + (m + 1) * BSIZE �` m_pCPos + (m + 2) * BSIZE - 1]
					����
				[..........]
					����
	[k - m : m_qCPos + k * BSIZE �` m_pCPos + (k + 1) * BSIZE - 1]
					����
	[k - m + 1 : m_qCPos + (k + 1) * BSIZE �` m_pCPos + (k + 2) * BSIZE - 1] *
					����
				[..........] *
					����
	[k : m_qCPos + (m + k) * BSIZE �` m_pCPos + (m + k + 1) * BSIZE - 1] *

	2) m_qCPos -> m_qCPos - m (0 < m <= k) �̏ꍇ
	[-k: m_qCPos - (m + k) * BSIZE �` m_pCPos - (m + k - 1) * BSIZE - 1] *
					����
				[..........] *
					����
	[- k + m - 1: m_qCPos - (k + 1) * BSIZE �` m_pCPos - (k + 2) * BSIZE - 1] *
					����
	[- k + m: m_qCPos - k * BSIZE �` m_pCPos - (k - 1) * BSIZE - 1]
					����
				[..........]
					����
	[-1: m_qCPos - (m + 1) * BSIZE �` m_pCPos - m * BSIZE - 1]
					����
	[0 : m_qCPos - m * BSIZE �` m_qCPos + (m + 1) * BSIZE - 1]
					����
	[1 : m_qCPos - (m - 1) * BSIZE �` m_pCPos - (m - 2) * BSIZE - 1]
					����
				[..........]
					����
	[m : m_qCPos + 0 * BSIZE �` m_pCPos + 1 * BSIZE - 1]
					����
				[..........]
					����
	[k : m_qCPos + (k - m) * BSIZE �` m_pCPos + (k - m + 1) * BSIZE - 1]
 */

BGB_Manager::BGB_Manager(int bufsize, int bufcount, LargeFileReader* pLFReader)
	: m_nBufSize(bufsize),
	  m_nBufCount(bufcount),
	  m_pLFReader(pLFReader),
	  m_qCurrentPos(-1),
	  m_bRBInit(false),
	  m_pThread(NULL)
{
	assert(m_nBufSize > 0 && m_nBufCount > 2);
	m_nBufCount |= 1;
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
	return new BGBuffer(m_nBufSize);
}

int
BGB_Manager::fillBGBuffer(filesize_t offset)
{
	assert(offset >= 0);

	if (!m_bRBInit) { // �ŏ��̌Ăяo��
		// m_nBufCount �̃����O�o�b�t�@�v�f��ǉ�
		for (int i = 0; i < m_nBufCount; i++) {
			m_rbBuffers.addElement(createBGBufferInstance(), i);
		}
		m_bRBInit = true;
	}

	if (!isLoaded()) return -1;

	// offset �� m_nBufSize �ŃA���C�����g
//	offset = (offset / m_nBufSize) * m_nBufSize;
	offset &= ~(m_nBufSize - 1); // m_nBufSize == 2^n ������

	int radius = (m_nBufCount / 2) * m_nBufSize;

	// offset - radius * m_nBufSize ���� offset + (radius + 1) * m_nBufSize - 1
	// �̃f�[�^���t�@�C������ǂݏo��

	// �S�Ă�V�K�ɓǂݍ��ނׂ����A
	// ���݂̃��X�g���ꕔ�j�����ēǂݍ��ނׂ����𒲂ׂ�
	if (m_qCurrentPos < 0 ||
		offset >= m_qCurrentPos + radius + m_nBufSize ||
		offset <  m_qCurrentPos - radius) {
		// �S���V�K�ɓǂݍ���
		m_qCurrentPos = offset;
		int i = - radius / m_nBufSize;
		filesize_t start, end;
		start = m_qCurrentPos - radius;
		end = start + m_nBufCount * m_nBufSize;
		while (start < end) {
			if (start >= 0) { // �I�[���z���ĕ`�悷�邽�߃t�@�C���I�[�𔻒肵�Ă͂����Ȃ��I�I
				m_rbBuffers.elementAt(i)->init(*m_pLFReader, start);
			}
			i++;
			start += m_nBufSize;
		}
	} else if (offset != m_qCurrentPos) {
		// offset �̃u���b�N�͊��ɓǂݍ��܂�Ă���
		int new_top = (int)(offset - m_qCurrentPos), i;
		filesize_t start, end;
		if (new_top > 0) {
			// case 1)
			i = radius / m_nBufSize + 1;
			start = m_qCurrentPos + radius + m_nBufSize;
			end = start + new_top;
		} else {
			// case 2)
			i = (- radius + new_top) / m_nBufSize;
			end = m_qCurrentPos - radius;
			start = end + new_top;
		}
		while (start < end) {
			if (start >= 0) {
				m_rbBuffers.elementAt(i)->init(*m_pLFReader, start);
			}
			i++;
			start += m_nBufSize;
		}
		m_rbBuffers.setTop(new_top / m_nBufSize);
	}

	// �J�����g�|�W�V�����̕ύX
	m_qCurrentPos = offset;

	// �o�b�t�@�ɒ��߂��Ă���f�[�^�T�C�Y�̌v�Z
	int totalsize = 0;
	for (int i = 0; i < m_nBufCount; i++) {
		BGBuffer* pbgb = m_rbBuffers.elementAt(i);
		if (pbgb->m_qAddress >= 0)
			totalsize += pbgb->m_nDataSize;
	}

	return totalsize;
}

// �����p�̃X���b�h�̎���

struct ThreadProcArg {
//	BGB_Manager* m_pBGB_Manager;
	int m_nBufSizePerBlock;
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
	int blocksize = pThreadArg->m_nBufSizePerBlock,
		size = pCallbackArg->m_nBufSize;

	int bufsize = blocksize * ((blocksize + size - 1) / blocksize);

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

//	assert(ret == -1);

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
	pThreadArg->m_nBufSizePerBlock = m_nBufSize;
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

	return m_pThread->stop();
}

bool
BGB_Manager::waitStopFind()
{
	if (!m_pThread.ptr()) return false;

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

