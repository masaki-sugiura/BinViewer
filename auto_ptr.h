// $Id$

#ifndef	AUTO_PTR_H_INC
#define	AUTO_PTR_H_INC

//! 配列でないオブジェクトへのポインタを扱うスマートポインタ
template<class T>
class Auto_Ptr {
public:
	//! 生のポインタを受け取った場合のコンストラクタ
	Auto_Ptr(T* ptr)
		:	m_ptr(ptr),
			m_bOwner(true) // ポインタが指すオブジェクトを削除する責任がある
	{}
	//! コピーコンストラクタ
	Auto_Ptr(Auto_Ptr<T>& aptr)
		:	m_ptr(aptr.m_ptr),
			m_bOwner(aptr.m_bOwner)	// オーナーフラグの継承
	{
		aptr.m_bOwner = false;	//	代入元の Auto_Ptr オブジェクトは
								//	オーナーではなくなる
	}
	~Auto_Ptr()
	{
		//	自分がオーナーであればオブジェクトを削除
		if (m_bOwner) delete m_ptr;
	}

	Auto_Ptr<T>& operator=(Auto_Ptr<T>& aptr)
	{
		//	自分がオーナーであればオブジェクトを削除
		if (m_bOwner) delete m_ptr;
		m_ptr = aptr.m_ptr;
		m_bOwner = aptr.m_bOwner;	//	オーナーフラグの継承
		aptr.m_bOwner = false;	//	aptr はオーナーではなくなる
		return *this;
	}
	Auto_Ptr<T>& operator=(T* ptr)
	{
		if (m_bOwner) delete m_ptr;
		m_ptr = ptr;
		m_bOwner = true;
		return *this;
	}

	T& operator*() { return *m_ptr; }
	const T& operator*() const { return *m_ptr; }
	T* operator->() { return m_ptr; }
	const T* operator->() const { return m_ptr; }
	T* ptr() const { return m_ptr; }

private:
	T* m_ptr;		//!	生のポインタ
	bool m_bOwner;	//!	オーナーフラグ
};

#endif
