// $Id$

#include "LargeFileReader.h"
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

