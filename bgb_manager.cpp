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
	// 既に読み込み済みかどうか
	if (m_qAddress == offset) return m_nDataSize;

	m_qAddress = offset;

	m_nDataSize = LFReader.readFrom(m_qAddress, FILE_BEGIN,
									m_DataBuf, m_nBufSize);
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
	2k + 1 個のリングバッファによるバッファの管理：

	[-k: m_qCPos - k * BSIZE 〜 m_pCPos - (k - 1) * BSIZE - 1]
					↓↑
				[..........]
					↓↑
	[-2: m_qCPos - 2 * BSIZE 〜 m_pCPos - BSIZE - 1]
					↓↑
	[-1: m_qCPos - BSIZE 〜 m_pCPos - 1]
					↓↑
	[0 : m_qCPos 〜 m_qCPos + BSIZE - 1]
					↓↑
	[1 : m_qCPos + BSIZE 〜 m_pCPos + 2 * BSIZE - 1]
					↓↑
				[..........]
					↓↑
	[k : m_qCPos + k * BSIZE 〜 m_pCPos + (k + 1) * BSIZE - 1]

	(一般に [j : m_qCPos + j * BSIZE 〜 m_pCPos + (j + 1) * BSIZE - 1] (-k <= j <= k))

	1) m_qCPos -> m_qCPos + m (0 < m <= k) の場合
	[-k: m_qCPos + (m - k) * BSIZE 〜 m_pCPos + (m - k + 1) * BSIZE - 1]
					↓↑
				[..........]
					↓↑
	[-2: m_qCPos + (m - 2) * BSIZE 〜 m_pCPos + (m - 1) * BSIZE - 1]
					↓↑
	[-1: m_qCPos + (m - 1) * BSIZE 〜 m_pCPos + m * BSIZE - 1]
					↓↑
	[0 : m_qCPos + m * BSIZE 〜 m_qCPos + (m + 1) * BSIZE - 1]
					↓↑
	[1 : m_qCPos + (m + 1) * BSIZE 〜 m_pCPos + (m + 2) * BSIZE - 1]
					↓↑
				[..........]
					↓↑
	[k - m : m_qCPos + k * BSIZE 〜 m_pCPos + (k + 1) * BSIZE - 1]
					↓↑
	[k - m + 1 : m_qCPos + (k + 1) * BSIZE 〜 m_pCPos + (k + 2) * BSIZE - 1] *
					↓↑
				[..........] *
					↓↑
	[k : m_qCPos + (m + k) * BSIZE 〜 m_pCPos + (m + k + 1) * BSIZE - 1] *

	2) m_qCPos -> m_qCPos - m (0 < m <= k) の場合
	[-k: m_qCPos - (m + k) * BSIZE 〜 m_pCPos - (m + k - 1) * BSIZE - 1] *
					↓↑
				[..........] *
					↓↑
	[- k + m - 1: m_qCPos - (k + 1) * BSIZE 〜 m_pCPos - (k + 2) * BSIZE - 1] *
					↓↑
	[- k + m: m_qCPos - k * BSIZE 〜 m_pCPos - (k - 1) * BSIZE - 1]
					↓↑
				[..........]
					↓↑
	[-1: m_qCPos - (m + 1) * BSIZE 〜 m_pCPos - m * BSIZE - 1]
					↓↑
	[0 : m_qCPos - m * BSIZE 〜 m_qCPos + (m + 1) * BSIZE - 1]
					↓↑
	[1 : m_qCPos - (m - 1) * BSIZE 〜 m_pCPos - (m - 2) * BSIZE - 1]
					↓↑
				[..........]
					↓↑
	[m : m_qCPos + 0 * BSIZE 〜 m_pCPos + 1 * BSIZE - 1]
					↓↑
				[..........]
					↓↑
	[k : m_qCPos + (k - m) * BSIZE 〜 m_pCPos + (k - m + 1) * BSIZE - 1]
 */

BGB_Manager::BGB_Manager(int bufsize, int bufcount, LargeFileReader* pLFReader)
	: m_nBufSize(bufsize),
	  m_nBufCount(bufcount),
	  m_pLFReader(pLFReader),
	  m_qCurrentPos(-1),
	  m_bRBInit(false)
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
	return new BGBuffer(m_nBufSize);
}

int
BGB_Manager::fillBGBuffer(filesize_t offset)
{
	assert(offset >= 0);

	if (!m_bRBInit) { // 最初の呼び出し
		// m_nBufCount 個のリングバッファ要素を追加
		for (int i = 0; i < m_nBufCount; i++) {
			m_rbBuffers.addElement(createBGBufferInstance(), i);
		}
		m_bRBInit = true;
	}

	if (!isLoaded()) return -1;

	// offset を m_nBufSize でアライメント
//	offset = (offset / m_nBufSize) * m_nBufSize;
	offset &= ~(m_nBufSize - 1); // m_nBufSize == 2^n を仮定

	int radius = (m_nBufCount / 2) * m_nBufSize;

	// offset - radius * m_nBufSize から offset + (radius + 1) * m_nBufSize - 1
	// のデータをファイルから読み出す

	// 全てを新規に読み込むべきか、
	// 現在のリストを一部破棄して読み込むべきかを調べる
	if (m_qCurrentPos < 0 ||
		offset >= m_qCurrentPos + radius + m_nBufSize ||
		offset <  m_qCurrentPos - radius) {
		// 全く新規に読み込み
		m_qCurrentPos = offset;
		int i = - radius / m_nBufSize;
		filesize_t start, end;
		start = m_qCurrentPos - radius;
		end = start + m_nBufCount * m_nBufSize;
		while (start < end) {
			if (start >= 0) { // 終端を越えて描画するためファイル終端を判定してはいけない！！
				m_rbBuffers.elementAt(i)->init(*m_pLFReader, start);
			}
			i++;
			start += m_nBufSize;
		}
	} else if (offset != m_qCurrentPos) {
		// offset のブロックは既に読み込まれている
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

	// カレントポジションの変更
	m_qCurrentPos = offset;

	// バッファに貯められているデータサイズの計算
	int totalsize = 0;
	for (int i = 0; i < m_nBufCount; i++) {
		BGBuffer* pbgb = m_rbBuffers.elementAt(i);
		if (pbgb->m_qAddress >= 0)
			totalsize += pbgb->m_nDataSize;
	}

	return totalsize;
}

