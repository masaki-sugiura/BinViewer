// $Id$

#include "LF_Notify.h"
#include "LargeFileReader.h"

#include <assert.h>

LF_Acceptor::LF_Acceptor()
	: m_pLFNotifier(NULL),
	  m_pLFReader(NULL)
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

	LargeFileReader* pLFReader = pLFNotifier->getReader();
	if (!pLFReader) return true;

	return loadFile(pLFReader);
}

void
LF_Acceptor::onUnregisted()
{
	GetLock lock(m_lckData);
	if (m_pLFReader) unloadFile();
	m_pLFNotifier = NULL;
}

bool
LF_Acceptor::loadFile(LargeFileReader* pLFReader)
{
	GetLock lock(m_lckData);
	m_pLFReader = pLFReader;
	return onLoadFile();
}

void
LF_Acceptor::unloadFile()
{
	GetLock lock(m_lckData);
	onUnloadFile();
	m_pLFReader = NULL;
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
LF_Notifier::loadFile(LargeFileReader* pLFReader)
{
	m_pLFReader = pLFReader;

	for (LFAList::iterator itr = m_lstLFAcceptor.begin();
		 itr != m_lstLFAcceptor.end();
		 ++itr) {
		if (!(*itr)->loadFile(pLFReader)) {
			return false;
		}
	}

	return true;
}

void
LF_Notifier::unloadFile()
{
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

