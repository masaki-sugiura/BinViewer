// $Id$

#include <queue>

using std::queue;

template<class T>
class SynchronizedQueue {
public:
	SynchronizedQueue()
	{
		m_mtxLock = ::CreateMutex(NULL, FALSE, NULL);
		m_evtPushed = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	}
	~SynchronizedQueue()
	{
		::CloseHandle(m_evtPushed);
		::CloseHandle(m_mtxLock);
	}

	T pop()
	{
		this->lock();
		while (m_queue.empty()) {
			this->release();
			this->waitPushed();
			this->lock();
		}
		T ret = m_queue.top();
		m_queue.pop();
		this->release();
		return ret;
	}

	void push(const T& val)
	{
		this->lock();
		m_queue.push(val);
		this->notifyPushed();
		this->release();
	}

	queue<T>::size_type size() const
	{
		return m_queue.size();
	}

private:
	queue<T> m_queue;
	HANDLE   m_mtxLock;
	HANDLE   m_evtPushed;

	void lock()
	{
		::WaitForSingleObject(m_mtxLock, INFINITE);
	}

	void release()
	{
		::ReleaseMutex(m_mtxLock);
	}

	void notifyPushed()
	{
		::PulseEvent(m_evtPushed);
	}

	void waitPushed()
	{
		::WaitForSingleObject(m_evtPushed, INFINITE);
	}
};

