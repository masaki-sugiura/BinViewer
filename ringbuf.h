// $Id$

#ifndef RINGBUF_H_INC
#define RINGBUF_H_INC

#include "types.h"

//! リングバッファクラステンプレート
template<class T>
class RingBuffer {
public:
	RingBuffer()
		: m_pTopElement(NULL), m_nCount(0)
	{}
	virtual ~RingBuffer();

	//! リングバッファの要素をあらわす構造体
	template<class U>
	struct RingBufElement {
		RingBufElement(U* ptr)
			: m_pElement(ptr),
			  m_prev(NULL), m_next(NULL)
		{}
		~RingBufElement()
		{
			delete m_pElement;
		}

		U* m_pElement; //! 要素の値へのポインタ
		RingBufElement* m_prev; //! 一つ前の要素を指すポインタ
		RingBufElement* m_next; //! 一つ後の要素を指すポインタ
	};

	// 以下のメソッドの要素の位置を指定する引数は先頭を 0 番目として
	//  n >= 0 : 先頭から  n 個後の要素
	//  n < 0  : 先頭から -n 個前の要素
	// を表す。但し、abs(n) >= m_nCount の場合は
	// abs(n) mod. m_nCount として解釈される。

	//! pos 番目の要素を返す
	T* elementAt(int pos)
	{
		RingBufElement<T>* elem = raw_elementAt(pos);
		return elem ? elem->m_pElement : NULL;
	}

	//! val を pos 番目の要素の前に追加し val が pos 番目になるようにする
	void addElement(T* ptr, int pos = 0);

	//! pos 番目の要素を削除し、pos + 1 番目の要素を pos 番目にする
	void removeElement(int pos = 0);

	//! pos 番目の要素が新しい先頭要素になるようにリングバッファを回す
	void setTop(int pos);

	//! 先頭の要素を返す
	T* top()
	{
		return elementAt(0);
	}

	//! 現在の要素数を返す
	int count() const { return m_nCount; }

protected:
	RingBufElement<T>* m_pTopElement; //! 先頭(= 0 番目)の要素を指すポインタ
	int m_nCount; //! 要素数

	//! pos 番目の要素を格納する RingBufElement へのポインタを返す
	RingBufElement<T>* raw_elementAt(int pos);
};

template<class T>
RingBuffer<T>::~RingBuffer()
{
	RingBufElement<T> *elem = m_pTopElement, *next;
	while (m_nCount-- > 0) {
		next = elem->m_next;
		delete elem;
		elem = next;
	}
}

template<class T>
RingBuffer<T>::RingBufElement<T>*
RingBuffer<T>::raw_elementAt(int pos)
{
	if (!m_nCount) return NULL;
	RingBufElement<T>* elem = m_pTopElement;
	if (pos >= 0) {
		pos %= m_nCount;
		while (pos-- > 0) {
			elem = elem->m_next;
		}
	} else {
		pos = - pos % m_nCount;
		while (pos-- > 0) {
			elem = elem->m_prev;
		}
	}
	return elem;
}

template<class T>
void
RingBuffer<T>::addElement(T* ptr, int pos)
{
	RingBufElement<T>* newElem = new RingBufElement<T>(ptr);
	if (!m_nCount) {
		// 初めての追加
		m_pTopElement = newElem;
		newElem->m_next = newElem->m_prev = newElem;
	} else {
		// 既存の要素の前に追加
		RingBufElement<T>* elem = raw_elementAt(pos); // throw exception
		RingBufElement<T>* next = elem->m_next;
		next->m_prev = newElem;
		elem->m_next = newElem;
		newElem->m_prev = elem;
		newElem->m_next = next;
	}
	m_nCount++;
}

template<class T>
void
RingBuffer<T>::removeElement(int pos)
{
	RingBufElement<T>* elem = raw_elementAt(pos);
	if (!elem) return;
	RingBufElement<T> *next = elem->m_next, *prev = elem->m_prev;
	delete elem;
	if (--m_nCount == 0) {
		m_pTopElement = NULL;
	} else {
		next->m_prev = prev;
		prev->m_next = next;
		if (m_pTopElement == elem)
			m_pTopElement = next;
	}
}

template<class T>
void
RingBuffer<T>::setTop(int pos)
{
	RingBufElement<T>* elem = raw_elementAt(pos);
	if (!elem) return;
	m_pTopElement = elem;
}

#endif
