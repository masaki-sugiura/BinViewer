// $Id$

#include "thread.h"
#include <windows.h>
#include <process.h>
#include <assert.h>

typedef UINT (WINAPI *thread_func_t)(void* arg);

Thread::Thread(thread_arg_t arg, thread_attr_t attr)
	: m_arg(arg), m_attr(attr),
	  m_result(-1), m_state(TS_READY)
{
	assert(m_attr != NULL);

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
	::CloseHandle(m_attr->m_hThread);
}

bool
Thread::run()
{
	GetLock lock(m_lockAttr);

	if (m_state != TS_READY) return false;

	m_attr->m_hThread = (HANDLE)_beginthreadex(NULL, 0,
									   (thread_func_t)threadProcedure,
									   (void*)this, 0,
									   (UINT*)&m_attr->m_dwThreadID);

	if (m_attr->m_hThread == NULL) return false;

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

	if (m_state != TS_RUNNING ||
		::WaitForSingleObject(m_attr->m_hThread, INFINITE)
		 != WAIT_OBJECT_0) return false;

	::GetExitCodeThread(m_attr->m_hThread, (DWORD*)&m_result);

	m_state = TS_STOPPED;

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

	if (m_state != TS_STOPPED) return (thread_result_t)-1;

	return m_result;
}

UINT WINAPI
Thread::threadProcedure(void* arg)
{
	Thread* This = (Thread*)arg;
	return This->thread(This->getArg());
}

