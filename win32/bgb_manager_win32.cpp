// $Id$

#include "bgb_manager.h"
#include "thread.h"
#include <windows.h>
#include <assert.h>

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
			// マッチするのに十分なサイズがあるか？
			if ((readsize = pLFReader->readFrom(pos, FILE_BEGIN,
												buf + offset, readsize)) <= 0 ||
				readsize + offset < size) {
				ret = -1;
				goto _exit_thread;
			}

			// 検索
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
			// size byte を残す＆バッファの先頭に移動
			memmove(buf, buf + bufsize - size, size);
			// バッファの格納位置の調整
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

			// マッチするのに十分なサイズがあるか？
			if ((readsize = pLFReader->readFrom(pos, FILE_BEGIN,
												buf + offset, readsize)) <= 0 ||
				readsize < size) {
				ret = -1;
				goto _exit_thread;
			}

			// 検索
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
			// size byte を残す＆バッファの終端に移動
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

