/**
 * My simple event loop source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __MSG_QUEUE_H_
#define __MSG_QUEUE_H_

#include <deque>

#include "Mutex.h"
#include "CondVar.h"

#define DEF_QUEUE_SIZE  10

struct Msg
{
    int   what;
    int   arg;
    void* obj;

    Msg() : what(0) { }
    Msg(int _what) : what(_what) { }
    Msg(int _what, int _arg) : what(_what), arg(_arg) { }
    Msg(int _what, void* _obj) : what(_what), obj(_obj) { }
    Msg(int _what, int _arg, void* _obj) : what(_what), arg(_arg), obj(_obj) { }
};

#define MSGQ_RET_SUCCESS  0
#define MSGQ_RET_TIMEOUT  1
#define MSGQ_RET_EMPTY   -1
#define MSGQ_RET_FULL    -1

class MsgQ
{
public:
    MsgQ();
    MsgQ(size_t capacity);
    ~MsgQ();

    int send(const Msg& msg, int timeoutMs = 0);
    int recv(Msg& msg, int timeoutMs = -1);
    int peek(Msg& msg, int timeoutMs = -1);

    void remove(int what);

    void flush();
    void setEOS(bool eos);

private:
    std::deque<Msg> mQueue;

    size_t  mCapacity;
    bool    mEOS;

    Mutex   mLock;
    CondVar mCvFull;
    CondVar mCvEmpty;
};

#endif /* __MSG_QUEUE_H_ */
