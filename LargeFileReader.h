// $Id$

#ifndef LARGEFILEREADER_H_INC
#define LARGEFILEREADER_H_INC

#include "types.h"
#include "lock.h"
#include "thread.h"

#include <string>
using std::string;

#include <exception>
using std::exception;

//! LargeFileReader ���������O�̊��N���X
class FileOpError : public exception {
public:
	FileOpError(const string& filename)
		: m_filename(filename)
	{}
	virtual ~FileOpError() {}

	const string& filename() const
	{
		return m_filename;
	}

	const char* what() const
	{
		return m_filename.c_str();
	}

private:
	string m_filename;
};

//! �t�@�C���I�[�v���Ɏ��s�������ɓ��������O
class FileOpenError : public FileOpError {
public:
	FileOpenError(const string& filename)
		: FileOpError(filename)
	{}
};

//! �t�@�C���V�[�N�Ɏ��s�������ɓ��������O
class FileSeekError : public FileOpError {
public:
	FileSeekError(const string& filename)
		: FileOpError(filename)
	{}
};

//! �f�[�^�̓ǂݏo���Ɏ��s�������ɓ��������O
class FileReadError : public FileOpError {
public:
	FileReadError(const string& filename)
		: FileOpError(filename)
	{}
};

//! 4GB �ȏ�̃T�C�Y�̃t�@�C���̓ǂݏo���������N���X
class LargeFileReader {
public:
	LargeFileReader(const string& filename)
		throw(FileOpenError);
	~LargeFileReader();

	/*! �w��ʒu(pos, origin) ���� size �o�C�g�̃f�[�^��ǂݏo����
		buf �Ɋi�[���� */
	int readFrom(filesize_t pos, DWORD origin, BYTE* buf, int size);

	//! �t�@�C���T�C�Y��Ԃ�
	filesize_t size() const { return m_qFileSize; }

private:
	string  m_filename; //! �t�@�C����
	FILE_HANDLE  m_hFile; //! �t�@�C���n���h��(���̂͂n�r�ˑ�)
	filesize_t m_qFileSize; //! �t�@�C���T�C�Y
	Lock m_lockAccess; //! �����A�N�Z�X�I�u�W�F�N�g

	// �R�s�[�֎~
	LargeFileReader(const LargeFileReader&);
	LargeFileReader& operator=(const LargeFileReader&);
};

//! ���������̎w��
enum FIND_DIRECTION {
	FIND_FORWARD  =  1,
	FIND_NONE     =  0,
	FIND_BACKWARD = -1
};

//! �񓯊��������I�������Ƃ��ɌĂ΂��R�[���o�b�N�֐��̈���
struct FindCallbackArg {
	BYTE* m_pData;    //! [in] �����f�[�^�ւ̃|�C���^
	int m_nBufSize;   //! [in] �����f�[�^�̃T�C�Y
	FIND_DIRECTION m_nDirection; //! [in] ��������
	filesize_t m_qOrgAddress; //! [in] ���̑I���J�n�ʒu
	int m_nOrgSelectedSize;   //! [in] ���̑I���T�C�Y
	filesize_t m_qStartAddress;   //! [in] �����J�n�ʒu
	filesize_t m_qFindAddress; //! [out] ���������A�h���X
	void (*m_pfnCallback)(FindCallbackArg*); //! [in] �����I�����ɌĂяo�����R�[���o�b�N�֐�
	void* m_pUserData; //! [in/out] ���[�U�[��`�̕ϐ�
};

//! �񓯊������X���b�h�ɓn������
struct FindThreadProcArg {
//	LargeFileReader* m_pLFReader;
	class LF_Notifier* m_pLFNotifier;
	FindCallbackArg* m_pCallbackArg;
};

//! �����p�̃X���b�h
class FindThread : public Thread {
public:
	FindThread(FindThreadProcArg* arg, thread_attr_t attr)
		: Thread(arg, attr)
	{}

	thread_result_t thread(thread_arg_t arg);
};

#endif
