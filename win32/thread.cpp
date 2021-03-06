/*
	$Id$
	(C) 2003, Pixela Corporation, all rights reserved.
 */

#include "thread.h"
#include <process.h>
#include <assert.h>

Thread::Thread(thread_arg_t arg, thread_attr_t attr)
	: m_arg(arg), m_attr(attr),
	  m_result(-1), m_state(TS_READY)
{
	assert(m_attr);

	GetLock lock(m_lockAttr);

	m_attr->m_dwThreadID = (DWORD)-1;
	m_attr->m_hThread = NULL;
}

Thread::~Thread()
{
	GetLock lock(m_lockAttr);
	if (m_state == TS_RUNNING || m_state == TS_SUSPENDING) {
		stop();
		join();
	}
}

bool
Thread::run()
{
	GetLock lock(m_lockAttr);

	if (m_state != TS_READY) return false;

	m_attr->m_hThread = (HANDLE)_beginthreadex(NULL, 0,
										Thread::threadProcedure,
										(void*)this, 0,
										(UINT*)&m_attr->m_dwThreadID);

	if (!m_attr->m_hThread) return false;

	m_state = TS_RUNNING;

	return true;
}

bool
Thread::stop()
{
	GetLock lock(m_lockAttr);

	switch (m_state) {
	case TS_SUSPENDING:
		if (!resume()) return false;
		// through down
	case TS_RUNNING:
		::PostThreadMessage(m_attr->m_dwThreadID, WM_QUIT, 0, 0);
		// through down
	case TS_STOPPED:
		return true;
	case TS_READY:
	default:
		return false;
	}
}

bool
Thread::resume()
{
	GetLock lock(m_lockAttr);

	if (m_state != TS_SUSPENDING) return false;
	return ::ResumeThread(m_attr->m_hThread) != 0xFFFFFFFF;
}

bool
Thread::suspend()
{
	GetLock lock(m_lockAttr);

	if (m_state != TS_RUNNING) return false;
	return ::SuspendThread(m_attr->m_hThread) != 0xFFFFFFFF;
}

bool
Thread::join()
{
	GetLock lock(m_lockAttr);

	if (m_state != TS_RUNNING && m_state != TS_STOPPED) {
		return false;
	}

	BOOL bStop = FALSE;
	while (!bStop) {
		switch (::WaitForSingleObject(m_attr->m_hThread, 1000)) {
		case WAIT_OBJECT_0:
			bStop = TRUE;
		case WAIT_TIMEOUT:
			break;
		default:
			return false;
		}
	}

	::GetExitCodeThread(m_attr->m_hThread, (DWORD*)&m_result);

	::CloseHandle(m_attr->m_hThread);

	m_attr->m_hThread = NULL;
	m_attr->m_dwThreadID = (DWORD)-1;

	m_state = TS_READY;

	return true;
}

bool
Thread::isTerminated() const
{
	MSG msg;
	return ::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) &&
		   msg.message == WM_QUIT;
}

thread_result_t
Thread::getResult()
{
	GetLock lock(m_lockAttr);

	if (m_state != TS_READY && m_state != TS_STOPPED) {
		return -1;
	}

	return m_result;
}

UINT WINAPI
Thread::threadProcedure(void* arg)
{
	Thread* This = (Thread*)arg;
	UINT ret = This->thread(This->getArg());
	This->m_state = TS_STOPPED;
	return ret;
}

