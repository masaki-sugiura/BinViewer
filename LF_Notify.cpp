// $Id$

#include "LF_Notify.h"
#include "LargeFileReader.h"

#include <assert.h>

LF_Acceptor::LF_Acceptor()
	: m_pLFNotifier(NULL)
{
}

LF_Acceptor::~LF_Acceptor()
{
	GetLock lock(m_lckData);
	if (m_pLFNotifier) {
		m_pLFNotifier->unregisterAcceptor(this);
	}
}

bool
LF_Acceptor::onRegisted(LF_Notifier* pLFNotifier)
{
	assert(!m_pLFNotifier);
	assert(pLFNotifier);

	GetLock lock(m_lckData);

	m_pLFNotifier = pLFNotifier;

	LargeFileReader* pLFReader;
	bool bRet = tryLockReader(&pLFReader, INFINITE);
	if (!bRet) return false;
	if (!pLFReader) return true;

	bRet = loadFile();

	releaseReader(pLFReader);

	return bRet;
}

void
LF_Acceptor::onUnregisted()
{
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

LF_Notifier::LF_Notifier()
	: m_pLFReader(NULL)
{
	// nothing to do.
}

LF_Notifier::~LF_Notifier()
{
	for (LFAList::iterator itr = m_lstLFAcceptor.begin();
		 itr != m_lstLFAcceptor.end();
		 ++itr) {
		(*itr)->onUnregisted();
	}
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
	GetLock lock(m_lckReader);

	m_pLFReader = pLFReader;

	for (LFAList::iterator itr = m_lstLFAcceptor.begin();
		 itr != m_lstLFAcceptor.end();
		 ++itr) {
		if (!(*itr)->loadFile()) {
			return false;
		}
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

bool
LF_Notifier::registerAcceptor(LF_Acceptor* pLFAcceptor)
{
	assert(pLFAcceptor);
	m_lstLFAcceptor.push_back(pLFAcceptor);
	bool bRet = pLFAcceptor->onRegisted(this);
	if (!bRet) {
		m_lstLFAcceptor.remove(pLFAcceptor);
	}
	return bRet;
}

void
LF_Notifier::unregisterAcceptor(LF_Acceptor* pLFAcceptor)
{
	assert(pLFAcceptor);
	pLFAcceptor->onUnregisted();
	m_lstLFAcceptor.remove(pLFAcceptor);
}

