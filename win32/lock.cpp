// $Id$

#include "lock.h"
#include <windows.h>
#include <assert.h>

Lock::Lock()
{
	::InitializeCriticalSection(&m_hLock);
}

Lock::~Lock()
{
	::DeleteCriticalSection(&m_hLock);
}

void
Lock::lock()
{
	::EnterCriticalSection(&m_hLock);
}

void
Lock::release()
{
	::LeaveCriticalSection(&m_hLock);
}

