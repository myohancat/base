/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __PIPE_H_
#define __PIPE_H_

#include "Types.h"
#include "Mutex.h"

class Pipe
{
public:
    Pipe();
    virtual ~Pipe();

    int  read(void* data, int len);
    int  write(const void* data, int len);

    void flush();

    int  getFD();

private:
    int  mFds[2];
    bool mEOS;
};

#endif /* __PIPE_H_ */
