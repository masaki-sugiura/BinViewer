// $Id$

#ifndef	AUTO_PTR_H_INC
#define	AUTO_PTR_H_INC

//! �z��łȂ��I�u�W�F�N�g�ւ̃|�C���^�������X�}�[�g�|�C���^
template<class T>
class Auto_Ptr {
public:
	//! ���̃|�C���^���󂯎�����ꍇ�̃R���X�g���N�^
	Auto_Ptr(T* ptr)
		:	m_ptr(ptr),
			m_bOwner(true) // �|�C���^���w���I�u�W�F�N�g���폜����ӔC������
	{}
	//! �R�s�[�R���X�g���N�^
	Auto_Ptr(Auto_Ptr<T>& aptr)
		:	m_ptr(aptr.m_ptr),
			m_bOwner(aptr.m_bOwner)	// �I�[�i�[�t���O�̌p��
	{
		aptr.m_bOwner = false;	//	������� Auto_Ptr �I�u�W�F�N�g��
								//	�I�[�i�[�ł͂Ȃ��Ȃ�
	}
	~Auto_Ptr()
	{
		//	�������I�[�i�[�ł���΃I�u�W�F�N�g���폜
		if (m_bOwner) delete m_ptr;
	}

	Auto_Ptr<T>& operator=(Auto_Ptr<T>& aptr)
	{
		//	�������I�[�i�[�ł���΃I�u�W�F�N�g���폜
		if (m_bOwner) delete m_ptr;
		m_ptr = aptr.m_ptr;
		m_bOwner = aptr.m_bOwner;	//	�I�[�i�[�t���O�̌p��
		aptr.m_bOwner = false;	//	aptr �̓I�[�i�[�ł͂Ȃ��Ȃ�
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
	T* m_ptr;		//!	���̃|�C���^
	bool m_bOwner;	//!	�I�[�i�[�t���O
};

#endif
