// $Id$

#include "bgb_manager.h"

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

