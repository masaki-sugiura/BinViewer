// $Id$

#ifndef LF_NOTIFY_H_INC
#define LF_NOTIFY_H_INC

#include "auto_ptr.h"
#include "lock.h"
#include <list>
using std::list;

class LargeFileReader;

class LF_Acceptor;

class LF_Notifier {
public:
	LF_Notifier();
	~LF_Notifier();

//	LargeFileReader* getReader() const { return m_pLFReader; }
	bool tryLockReader(LargeFileReader** ppLFReader, DWORD dwWaitTime);
	void releaseReader(LargeFileReader* pLFReader);

	bool loadFile(LargeFileReader* pLFReader);
	void unloadFile();

	bool registerAcceptor(LF_Acceptor* pLFAcceptor);
	void unregisterAcceptor(LF_Acceptor* pLFAcceptor);

private:
	LargeFileReader* m_pLFReader;
	typedef list<LF_Acceptor*> LFAList;
	LFAList m_lstLFAcceptor;
	Lock m_lckReader;
};

class LF_Acceptor {
public:
	LF_Acceptor(LF_Notifier& lfNotifier);
	virtual ~LF_Acceptor();

//	LargeFileReader* getReader() const { return m_pLFReader; }
	bool tryLockReader(LargeFileReader** ppLFReader, DWORD dwWaitTime)
	{
		return m_lfNotifier.tryLockReader(ppLFReader, dwWaitTime);
	}
	void releaseReader(LargeFileReader* pLFReader)
	{
		m_lfNotifier.releaseReader(pLFReader);
	}

	virtual bool onLoadFile() = 0;
	virtual void onUnloadFile() = 0;

private:
	Lock m_lckData;
	LF_Notifier& m_lfNotifier;

	bool loadFile();
	void unloadFile();

	friend LF_Notifier;
};

class AutoLockReader {
public:
	AutoLockReader(LF_Acceptor* pLFAcceptor, DWORD dwWaitTime, bool* pResult)
		: m_pLFAcceptor(pLFAcceptor),
		  m_pLFReader(NULL)
	{
		if (!pLFAcceptor) {
			if (pResult) *pResult = false;
			return;
		}
		bool bRet = pLFAcceptor->tryLockReader(&m_pLFReader, dwWaitTime);
		if (pResult) *pResult = bRet;
	}

	~AutoLockReader()
	{
		if (m_pLFAcceptor && m_pLFReader) {
			m_pLFAcceptor->releaseReader(m_pLFReader);
		}
	}

	operator LargeFileReader*()
	{
		return m_pLFReader;
	}

	operator const LargeFileReader*() const
	{
		return m_pLFReader;
	}

	LargeFileReader* operator->()
	{
		return m_pLFReader;
	}

	const LargeFileReader* operator->() const
	{
		return m_pLFReader;
	}

private:
	LF_Acceptor* m_pLFAcceptor;
	LargeFileReader* m_pLFReader;
};

class RegisterAcceptorError {};

#endif
