// $Id$

#ifndef LF_NOTIFY_H_INC
#define LF_NOTIFY_H_INC

#include "auto_ptr.h"
#include "lock.h"
#include <list>
using std::list;

class LargeFileReader;

class LF_Notifier;

class LF_Acceptor {
public:
	LF_Acceptor();
	virtual ~LF_Acceptor();

	bool loadFile(LargeFileReader* pLFReader);
	void unloadFile();

	LargeFileReader* getReader() const { return m_pLFReader; }

protected:
	virtual bool onLoadFile() = 0;
	virtual void onUnloadFile() = 0;

private:
	Lock m_lckData;
	LF_Notifier* m_pLFNotifier;
	LargeFileReader* m_pLFReader;

	bool onRegisted(LF_Notifier* pLFNotifier);
	void onUnregisted();

	friend LF_Notifier;
};

class LF_Notifier {
public:
	LF_Notifier();
	~LF_Notifier();

	LargeFileReader* getReader() const { return m_pLFReader; }

	bool loadFile(LargeFileReader* pLFReader);
	void unloadFile();

	bool registerAcceptor(LF_Acceptor* pLFAcceptor);
	void unregisterAcceptor(LF_Acceptor* pLFAcceptor);

private:
	LargeFileReader* m_pLFReader;
	typedef list<LF_Acceptor*> LFAList;
	LFAList m_lstLFAcceptor;
};

#endif
