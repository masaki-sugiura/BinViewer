// $Id$

#ifndef THREAD_H_INC
#define THREAD_H_INC

#include "types.h"
#include "lock.h"

typedef enum {
	TS_READY,      // まだスタートしていない
	TS_RUNNING,    // 実行中
	TS_SUSPENDING, // サスペンドされている
	TS_STOPPED     // 実行が終了している
} THREAD_STATE;


class Thread {
public:
	Thread(thread_arg_t arg, thread_attr_t attr);
	virtual ~Thread();

	// スレッドを開始
	bool run();

	// スレッド本体
	// arg : コンストラクタで渡された arg (のコピー)
	// ※関数の中で定期的に isTerminated() を呼び出し、
	//   true が返ってきた場合は速やかに関数を抜けること。
	virtual thread_result_t thread(thread_arg_t arg) = 0;

	// スレッドを一時停止(サスペンド)
	bool suspend();
	// サスペンドされたスレッドを再開
	bool resume();

	// スレッドを停止
	// ※実際に停止するのを待つには join() を呼び出すこと。
	bool stop();

	// スレッドの停止を待つ
	// ※stop() を呼ばないと永久にブロックするので注意。
	bool join();

	thread_attr_t& getAttribute()
	{
		return m_attr;
	}

	thread_arg_t& getArg()
	{
		return m_arg;
	}

	// thread() の戻り値を返す
	// ※スレッドが完全停止する前(＝ join() を呼び出す前)に
	//   呼び出した場合は (thread_result_t)-1 を返す。
	thread_result_t getResult();

protected:
	// thread() を終了させるべきかどうかを返す
	bool isTerminated() const;

private:
	thread_arg_t    m_arg;
	thread_attr_t   m_attr;
	thread_result_t m_result;
	THREAD_STATE    m_state;
	Lock m_lockAttr;
};

#endif
