// $Id$

#ifndef LOCK_H_INC
#define LOCK_H_INC

#include "types.h"

//! �񓯊��A�N�Z�X�̂��߂̃��b�N�I�u�W�F�N�g
class Lock {
public:
	Lock();
	~Lock();

	void lock(); // ���b�N���l������܂Ńu���b�N

	void release();

private:
	MUTEX_HANDLE m_hLock;
};

//! �X�R�[�v���Ń��b�N���l���������邽�߂̃N���X
class GetLock {
public:
	GetLock(Lock& lock)
		: m_lock(lock)
	{
		m_lock.lock();
	}
	~GetLock()
	{
		m_lock.release();
	}

private:
	Lock& m_lock;
};

#endif // LOCK_H_INC
