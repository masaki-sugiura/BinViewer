// $Id$

#ifndef LOCK_H_INC
#define LOCK_H_INC

#include "types.h"

//! 非同期アクセスのためのロックオブジェクト
class Lock {
public:
	Lock();
	~Lock();

	void lock(); // ロックを獲得するまでブロック

	void release();

private:
	MUTEX_HANDLE m_hLock;
};

//! スコープ内でロックを獲得し続けるためのクラス
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
