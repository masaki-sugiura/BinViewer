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
	// 既に読み込み済みかどうか
	if (m_qAddress == offset) return m_nDataSize;

	m_qAddress = offset;

	m_nDataSize = LFReader.readFrom(m_qAddress, FILE_BEGIN,
									m_DataBuf, MAX_DATASIZE_PER_BUFFER);
	if (m_nDataSize >= 0) return m_nDataSize;

	// 読み込みに失敗→バッファの無効化
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
	リングバッファによるバッファの管理：

					[-1: m_qCPos - BSIZE ～ m_pCPos - 1]
							↓↑					↓↑
	[0 : m_qCPos ～ m_qCPos + BSIZE - 1]		[2 : 無効(または前回の値)]
							↓↑					↓↑
					[1 : m_qCPos + BSIZE ～ m_pCPos + 2 * BSIZE - 1]

	４個のバッファのうち -1, 0, 1 番目の要素を常に有効にしておく。
	(ファイルの先頭または終端の場合は -1, または 1 番目の要素は無効)
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
	m_qCurrentPos = -1; // 最初の呼び出しで fillBGBuffer() で init() を呼ぶのに必要
	return isLoaded();
}

void
BGB_Manager::unloadFile()
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

BGBuffer*
BGB_Manager::createBGBufferInstance()
{
	return new BGBuffer();
}

int
BGB_Manager::fillBGBuffer(filesize_t offset)
{
	assert(offset >= 0);

	if (!m_bRBInit) { // 最初の呼び出し
		// BUFFER_NUM 個のリングバッファ要素を追加
		for (int i = 0; i < BUFFER_NUM; i++) {
			m_rbBuffers.addElement(createBGBufferInstance(), i - 1);
		}
		m_bRBInit = true;
	}

	if (!isLoaded()) return -1;

	// offset - MAX_DATASIZE_PER_BUFFER から offset + 2 * MAX_DATASIZE_PER_BUFFER - 1
	// のデータをファイルから読み出す

	// MAX_DATASIZE_PER_BUFFER バイトでアライメント
	offset = (offset / MAX_DATASIZE_PER_BUFFER) * MAX_DATASIZE_PER_BUFFER;

	// 現在のバッファリストに挿入するべきか、
	// 現在のリストを(全て、または一部)破棄して読み込むべきかを調べる

	if (m_qCurrentPos == offset) {
		// 現在と同じバッファを返す
	} else if (offset == m_qCurrentPos + MAX_DATASIZE_PER_BUFFER) {
		// 0, 1 番目のバッファは再利用可能
		m_rbBuffers.setTop(1); // 0 -> -1, 1 -> 0
		m_rbBuffers.elementAt(1)->init(*m_pLFReader, offset + MAX_DATASIZE_PER_BUFFER);
	} else if (offset == m_qCurrentPos + 2 * MAX_DATASIZE_PER_BUFFER) {
		// 1 番目のバッファは再利用可能
		m_rbBuffers.setTop(2); // 1 -> -1
		m_rbBuffers.elementAt(0)->init(*m_pLFReader, offset);
		m_rbBuffers.elementAt(1)->init(*m_pLFReader, offset + MAX_DATASIZE_PER_BUFFER);
	} else if (offset == m_qCurrentPos - MAX_DATASIZE_PER_BUFFER) {
		// 0, -1 番目のバッファは再利用可能
		m_rbBuffers.setTop(-1); // -1 -> 0, 0 -> 1
		m_rbBuffers.elementAt(-1)->init(*m_pLFReader, offset - MAX_DATASIZE_PER_BUFFER);
	} else if (offset == m_qCurrentPos + 2 * MAX_DATASIZE_PER_BUFFER) {
		// -1 番目のバッファは再利用可能
		m_rbBuffers.setTop(-2); // -1 -> 1
		m_rbBuffers.elementAt(-1)->init(*m_pLFReader, offset - MAX_DATASIZE_PER_BUFFER);
		m_rbBuffers.elementAt(0)->init(*m_pLFReader, offset);
	} else {
		// 全てを破棄して読み込み (m_qCurrentPos == -1 の場合も含む)
		m_rbBuffers.elementAt(-1)->init(*m_pLFReader, offset - MAX_DATASIZE_PER_BUFFER);
		m_rbBuffers.elementAt(0)->init(*m_pLFReader, offset);
		m_rbBuffers.elementAt(1)->init(*m_pLFReader, offset + MAX_DATASIZE_PER_BUFFER);
	}

	// カレントポジションの変更
	m_qCurrentPos = offset;

	// バッファに貯められているデータサイズの計算
	// (但し 2 以上または -2 以下の要素は含めない)
	int totalsize = 0;
	for (int i = -1; i <= 1; i++) {
		BGBuffer* pbgb = m_rbBuffers.elementAt(i);
		if (pbgb->m_qAddress >= 0)
			totalsize += pbgb->m_nDataSize;
	}

	return totalsize;
}

// 検索用のスレッドの実装

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

