// $Id$

#ifndef THREAD_H_INC
#define THREAD_H_INC

#include "types.h"
#include "lock.h"

typedef enum {
	TS_READY,      // �܂��X�^�[�g���Ă��Ȃ�
	TS_RUNNING,    // ���s��
	TS_SUSPENDING, // �T�X�y���h����Ă���
	TS_STOPPED     // ���s���I�����Ă���
} THREAD_STATE;


class Thread {
public:
	Thread(thread_arg_t arg, thread_attr_t attr);
	virtual ~Thread();

	// �X���b�h���J�n
	bool run();

	// �X���b�h�{��
	// arg : �R���X�g���N�^�œn���ꂽ arg (�̃R�s�[)
	// ���֐��̒��Œ���I�� isTerminated() ���Ăяo���A
	//   true ���Ԃ��Ă����ꍇ�͑��₩�Ɋ֐��𔲂��邱�ƁB
	virtual thread_result_t thread(thread_arg_t arg) = 0;

	// �X���b�h���ꎞ��~(�T�X�y���h)
	bool suspend();
	// �T�X�y���h���ꂽ�X���b�h���ĊJ
	bool resume();

	// �X���b�h���~
	// �����ۂɒ�~����̂�҂ɂ� join() ���Ăяo�����ƁB
	bool stop();

	// �X���b�h�̒�~��҂�
	// ��stop() ���Ă΂Ȃ��Ɖi�v�Ƀu���b�N����̂Œ��ӁB
	bool join();

	thread_attr_t& getAttribute()
	{
		return m_attr;
	}

	thread_arg_t& getArg()
	{
		return m_arg;
	}

	// thread() �̖߂�l��Ԃ�
	// ���X���b�h�����S��~����O(�� join() ���Ăяo���O)��
	//   �Ăяo�����ꍇ�� (thread_result_t)-1 ��Ԃ��B
	thread_result_t getResult();

protected:
	// thread() ���I��������ׂ����ǂ�����Ԃ�
	bool isTerminated() const;

private:
	thread_arg_t    m_arg;
	thread_attr_t   m_attr;
	thread_result_t m_result;
	THREAD_STATE    m_state;
	Lock m_lockAttr;

	static DECLARE_THREAD_PROC(threadProcedure);
};

#endif
