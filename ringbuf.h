// $Id$

#ifndef RINGBUF_H_INC
#define RINGBUF_H_INC

#include "types.h"

//! �����O�o�b�t�@�N���X�e���v���[�g
template<class T>
class RingBuffer {
public:
	RingBuffer()
		: m_pTopElement(NULL), m_nCount(0)
	{}
	virtual ~RingBuffer();

	//! �����O�o�b�t�@�̗v�f������킷�\����
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

		U* m_pElement; //! �v�f�̒l�ւ̃|�C���^
		RingBufElement* m_prev; //! ��O�̗v�f���w���|�C���^
		RingBufElement* m_next; //! ���̗v�f���w���|�C���^
	};

	// �ȉ��̃��\�b�h�̗v�f�̈ʒu���w�肷������͐擪�� 0 �ԖڂƂ���
	//  n >= 0 : �擪����  n ��̗v�f
	//  n < 0  : �擪���� -n �O�̗v�f
	// ��\���B�A���Aabs(n) >= m_nCount �̏ꍇ��
	// abs(n) mod. m_nCount �Ƃ��ĉ��߂����B

	//! pos �Ԗڂ̗v�f��Ԃ�
	T* elementAt(int pos)
	{
		RingBufElement<T>* elem = raw_elementAt(pos);
		return elem ? elem->m_pElement : NULL;
	}

	//! val �� pos �Ԗڂ̗v�f�̑O�ɒǉ��� val �� pos �ԖڂɂȂ�悤�ɂ���
	void addElement(T* ptr, int pos = 0);

	//! pos �Ԗڂ̗v�f���폜���Apos + 1 �Ԗڂ̗v�f�� pos �Ԗڂɂ���
	void removeElement(int pos = 0);

	//! pos �Ԗڂ̗v�f���V�����擪�v�f�ɂȂ�悤�Ƀ����O�o�b�t�@����
	void setTop(int pos);

	//! �擪�̗v�f��Ԃ�
	T* top()
	{
		return elementAt(0);
	}

	//! ���݂̗v�f����Ԃ�
	int count() const { return m_nCount; }

protected:
	RingBufElement<T>* m_pTopElement; //! �擪(= 0 �Ԗ�)�̗v�f���w���|�C���^
	int m_nCount; //! �v�f��

	//! pos �Ԗڂ̗v�f���i�[���� RingBufElement �ւ̃|�C���^��Ԃ�
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
		// ���߂Ă̒ǉ�
		m_pTopElement = newElem;
		newElem->m_next = newElem->m_prev = newElem;
	} else {
		// �����̗v�f�̑O�ɒǉ�
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
