// $Id$

#include "bgb_manager.h"
#include "thread.h"

int
BGBuffer::init(LargeFileReader& LFReader, filesize_t offset)
{
	// ���ɓǂݍ��ݍς݂��ǂ���
	if (m_qAddress == offset) return m_nDataSize;

	m_qAddress = offset;

	m_nDataSize = LFReader.readFrom(m_qAddress, FILE_BEGIN,
									m_pDataBuf, m_nBufSize);
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

//	if (!isLoaded()) return -1;
	LargeFileReader* pLFReader;
	bool bRet = m_pLFAcceptor->tryLockReader(&pLFReader, INFINITE);
	if (!bRet) return -1;

	// offset �� nBufSize �ŃA���C�����g
//	offset = (offset / nBufSize) * nBufSize;
	offset &= ~(m_nBufSize - 1); // nBufSize == 2^n ������

	int radius = (m_nBufCount / 2) * m_nBufSize;

	// offset - radius * nBufSize ���� offset + (radius + 1) * nBufSize - 1
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
				m_rbBuffers.elementAt(i)->init(*pLFReader, start);
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
				m_rbBuffers.elementAt(i)->init(*pLFReader, start);
			}
			i++;
			start += m_nBufSize;
		}
		m_rbBuffers.setTop(new_top / m_nBufSize);
	}

	m_pLFAcceptor->releaseReader(pLFReader);

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

