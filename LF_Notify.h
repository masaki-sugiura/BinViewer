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

	filesize_t getCursorPos() const { return m_qCursorPos; }
	void setCursorPos(filesize_t newpos);

	bool registerAcceptor(LF_Acceptor* pLFAcceptor);
	void unregisterAcceptor(LF_Acceptor* pLFAcceptor);

private:
	LargeFileReader* m_pLFReader;
	filesize_t m_qCursorPos;
	typedef list<LF_Acceptor*> LFAList;
	LFAList m_lstLFAcceptor;
	Lock m_lckReader, m_lckCursor;
};

class LF_Acceptor {
public:
	LF_Acceptor();
	virtual ~LF_Acceptor();

//	LargeFileReader* getReader() const { return m_pLFReader; }
	bool tryLockReader(LargeFileReader** ppLFReader, DWORD dwWaitTime)
	{
		GetLock lock(m_lckData);
		if (!m_pLFNotifier) return false;
		return m_pLFNotifier->tryLockReader(ppLFReader, dwWaitTime);
	}
	void releaseReader(LargeFileReader* pLFReader)
	{
		GetLock lock(m_lckData);
		if (m_pLFNotifier) {
			m_pLFNotifier->releaseReader(pLFReader);
		}
	}

	void setNotifier(LF_Notifier* pLFNotifier);
	bool registTo(LF_Notifier& lfNotifier)
	{
		return lfNotifier.registerAcceptor(this);
	}
	void unregist()
	{
		GetLock lock(m_lckData);
		if (m_pLFNotifier) {
			m_pLFNotifier->unregisterAcceptor(this);
		}
	}

	virtual bool onLoadFile() = 0;
	virtual void onUnloadFile() = 0;

	virtual void onSetCursorPos(filesize_t pos) = 0;

	friend class LF_Notifier;

protected:
	LF_Notifier* m_pLFNotifier;

private:
	Lock m_lckData;

	bool loadFile();
	void unloadFile();
	void setCursorPos(filesize_t pos);
};

template<class T>
class AutoLockReader {
public:
	AutoLockReader(T* pLFProvideReader, DWORD dwWaitTime, bool* pResult)
		: m_pLFProvideReader(pLFProvideReader),
		  m_pLFReader(NULL)
	{
		if (!pLFProvideReader) {
			if (pResult) *pResult = false;
			return;
		}
		bool bRet = pLFProvideReader->tryLockReader(&m_pLFReader, dwWaitTime);
		if (pResult) *pResult = bRet;
	}

	~AutoLockReader()
	{
		if (m_pLFProvideReader && m_pLFReader) {
			m_pLFProvideReader->releaseReader(m_pLFReader);
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
	T* m_pLFProvideReader;
	LargeFileReader* m_pLFReader;
};

class RegisterAcceptorError {};

#endif
