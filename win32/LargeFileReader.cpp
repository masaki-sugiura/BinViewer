// $Id$

#include "LargeFileReader.h"
#include "LF_Notify.h"
#include "lock.h"

// �t�@�C���|�C���^�̈ړ�
// 64bit �������߂�l���b�p�[�֐�
//
//  hFile  : �t�@�C�� HANDLE (SetFilePointer() �̑�P�����Ɠ���)
//  pos    : �V�[�N�ʒu
//  origin : �V�[�N�ʒu�̎w�� (SetFilePointer() �̑�S�����Ɠ���)
//
//  return : �V�����t�@�C���|�C���^�̈ʒu�B�V�[�N�Ɏ��s������ -1 ��Ԃ��B
//
// ��pos ���t�@�C���T�C�Y�ȏ�ł� -1 ��Ԃ��Ȃ����Ƃɒ���
//
static inline filesize_t
SetFilePointer64(FILE_HANDLE hFile, filesize_t pos, DWORD origin)
{
	long sizehigh = (long)(pos >> 32);

#ifdef _DEBUG
	if (pos == 0x0000000080000000) {
		::OutputDebugString("");
	}
#endif

	long sizelow = ::SetFilePointer(hFile, (long)pos, &sizehigh, origin);
	if (sizelow == 0xFFFFFFFF && ::GetLastError() != NO_ERROR)
		sizehigh = -1;
	return QWORD(sizelow, sizehigh);
}

#if 0
static DWORD WINAPI
ReadFileProc(LPVOID lpThreadArg)
{
	SynchronizedQueue<ReadFileRequest>*
		pReadReqQueue = (SynchronizedQueue<ReadFileRequest>*)lpThreadArg;

	MSG msg;
	while (1) {
		ReadFileRequest req = pReadReqQueue->pop();
		if (req.m_size < 0) break;
		SetFilePointer64(req.m_hFile, req.m_pos, FILE_BEGIN);
		if (req.m_pos == -1) {
		} else {
			DWORD read_size;
			if (!::ReadFile(req.m_hFile, req.m_buf, req.m_size, &read_size, NULL)) {
				;
			}
		}
	};

}
#endif

// LargeFileReader �N���X�R���X�g���N�^
//
//  filename : �t�@�C����
//  ini_pos  : �t�@�C���|�C���^�̏����ʒu
//
//  throw : ���炩�̗��R�Ńt�@�C���I�[�v���Ɏ��s�����ꍇ
//
LargeFileReader::LargeFileReader(const string& filename)
	throw(FileOpenError, FileSeekError)
	: m_filename(filename)
{
	m_hFile = ::CreateFile(m_filename.c_str(),
						   GENERIC_READ, FILE_SHARE_READ, NULL,
						   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE)
		throw FileOpenError(m_filename);

	DWORD sizelow, sizehigh;
	sizelow = ::GetFileSize(m_hFile, &sizehigh);
	m_qFileSize = QWORD(sizelow, sizehigh);
}

// LargeFileReader �N���X�f�X�g���N�^
LargeFileReader::~LargeFileReader()
{
	GetLock lock(m_lockAccess);
	::CloseHandle(m_hFile);
}

// �t�@�C���|�C���^�̎w��ʒu����f�[�^��ǂݏo��
//
//  pos    : �t�@�C���|�C���^�̈ʒu
//  origin : ���_�̎w��(FILE_BEGIN, FILE_CURRENT, FILE_END)
//  buf    : �ǂݏo�����f�[�^���i�[����o�b�t�@
//  size   : �ǂݏo���o�C�g��
//
//  return : ���ۂɓǂݏo�����o�C�g���B�ǂݏo���Ɏ��s������ -1 ��Ԃ��B
//
int
LargeFileReader::readFrom(filesize_t pos, DWORD origin, BYTE* buf, int size)
{
	GetLock lock(m_lockAccess);

	DWORD readsize;
	if (SetFilePointer64(m_hFile, pos, origin) < 0 ||
		!::ReadFile(m_hFile, buf, (DWORD)size, &readsize, NULL))
		return -1;
	return (int)readsize;
}

// �����p�̃X���b�h�̎���
thread_result_t
FindThread::thread(thread_arg_t arg)
{
	FindThreadProcArg* pThreadArg = (FindThreadProcArg*)arg;
//	LargeFileReader* pLFReader = pThreadArg->m_pLFReader;
	LF_Notifier* pLFNotifier = pThreadArg->m_pLFNotifier;
	FindCallbackArg* pCallbackArg = pThreadArg->m_pCallbackArg;

	filesize_t pos = pCallbackArg->m_qStartAddress;
	const BYTE* data = pCallbackArg->m_pData;
	int size = pCallbackArg->m_nBufSize;
	const int blocksize = 1024;

	int bufsize = blocksize * ((blocksize + size - 1) / blocksize);

	bool bRet;
	LargeFileReader* pOrgReader;
	{
		AutoLockReader<LF_Notifier> alReaderOrg(pLFNotifier, INFINITE, &bRet);
		if (!bRet) {
			if (pCallbackArg)
				(*pCallbackArg->m_pfnCallback)(pCallbackArg);
			return -1;
		}
		pOrgReader = alReaderOrg;
	}

	BYTE* buf = new BYTE[bufsize];

	int readsize = bufsize, offset = 0;
	bool match = true;

	DWORD ret = 0;

	if (pCallbackArg->m_nDirection == FIND_FORWARD) {
		while (!isTerminated()) {
			AutoLockReader<LF_Notifier> alReader(pLFNotifier, INFINITE, &bRet);
			if (!bRet || alReader != pOrgReader) {
				if (pCallbackArg)
					(*pCallbackArg->m_pfnCallback)(pCallbackArg);
				goto _exit_thread;
			}

			// �}�b�`����̂ɏ\���ȃT�C�Y�����邩�H
			if ((readsize = alReader->readFrom(pos, FILE_BEGIN,
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

			AutoLockReader<LF_Notifier> alReader(pLFNotifier, INFINITE, &bRet);
			if (!bRet || alReader != pOrgReader) {
				if (pCallbackArg)
					(*pCallbackArg->m_pfnCallback)(pCallbackArg);
				goto _exit_thread;
			}

			// �}�b�`����̂ɏ\���ȃT�C�Y�����邩�H
			if ((readsize = alReader->readFrom(pos, FILE_BEGIN,
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

