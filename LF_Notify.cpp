// $Id$

#include "LF_Notify.h"
#include "LargeFileReader.h"

#include <assert.h>

LF_Acceptor::LF_Acceptor(LF_Notifier& lfNotifier)
	: m_lfNotifier(lfNotifier)
{
	GetLock lock(m_lckData);

	if (!m_lfNotifier.registerAcceptor(this)) {
		throw RegisterAcceptorError();
	}

	LargeFileReader* pLFReader;
	tryLockReader(&pLFReader, INFINITE);

	if (pLFReader) {
		bool bRet = loadFile();
	}

	releaseReader(pLFReader);
}

LF_Acceptor::~LF_Acceptor()
{
	GetLock lock(m_lckData);

	LargeFileReader* pLFReader;
	tryLockReader(&pLFReader, INFINITE);

	if (pLFReader) {
		unloadFile();
	}

	m_lfNotifier.unregisterAcceptor(this);
}

bool
LF_Acceptor::loadFile()
{
	GetLock lock(m_lckData);
	return onLoadFile();
}

void
LF_Acceptor::unloadFile()
{
	GetLock lock(m_lckData);
	onUnloadFile();
}

void
LF_Acceptor::setCursorPos(filesize_t pos)
{
	GetLock lock(m_lckData);
	onSetCursorPos(pos);
}

LF_Notifier::LF_Notifier()
	: m_pLFReader(NULL),
	  m_qCursorPos(-1)
{
	// nothing to do.
}

LF_Notifier::~LF_Notifier()
{
}

bool
LF_Notifier::tryLockReader(LargeFileReader** ppLFReader, DWORD dwWaitTime)
{
	if (!ppLFReader) return false;
	m_lckReader.lock();
	*ppLFReader = m_pLFReader;
	return true;
}

void
LF_Notifier::releaseReader(LargeFileReader* pLFReader)
{
	if (pLFReader == m_pLFReader) {
		m_lckReader.release();
	}
}

bool
LF_Notifier::loadFile(LargeFileReader* pLFReader)
{
	assert(pLFReader);

	GetLock lockReader(m_lckReader);
	GetLock lockCursor(m_lckCursor);

	m_pLFReader = pLFReader;
	m_qCursorPos = 0;

	for (LFAList::iterator itr = m_lstLFAcceptor.begin();
		 itr != m_lstLFAcceptor.end();
		 ++itr) {
		if (!(*itr)->loadFile()) {
			return false;
		}
		(*itr)->setCursorPos(0);
	}

	return true;
}

void
LF_Notifier::unloadFile()
{
	GetLock lock(m_lckReader);

	for (LFAList::iterator itr = m_lstLFAcceptor.begin();
		 itr != m_lstLFAcceptor.end();
		 ++itr) {
		(*itr)->unloadFile();
	}

	m_pLFReader = NULL;
}

void
LF_Notifier::setCursorPos(filesize_t newpos)
{
	assert(newpos >= 0);

	GetLock lock(m_lckCursor);

	m_qCursorPos = newpos;

	for (LFAList::iterator itr = m_lstLFAcceptor.begin();
		 itr != m_lstLFAcceptor.end();
		 ++itr) {
		(*itr)->setCursorPos(newpos);
	}
}

bool
LF_Notifier::registerAcceptor(LF_Acceptor* pLFAcceptor)
{
	assert(pLFAcceptor);
	m_lstLFAcceptor.push_back(pLFAcceptor);
	return true;
}

void
LF_Notifier::unregisterAcceptor(LF_Acceptor* pLFAcceptor)
{
	assert(pLFAcceptor);
	m_lstLFAcceptor.remove(pLFAcceptor);
}

