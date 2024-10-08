/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __CONDITION_VARIABLE_H_
#define __CONDITION_VARIABLE_H_

#include "Types.h"
#include "Mutex.h"

#include <pthread.h>

class CondVar
{
public:
    CondVar();
    ~CondVar();

    void wait(Mutex& mutex);

    /**
     * @param mutex
     * @param timeout : milliseconds
     * @return true  : condition variable wakeup by signal
     *         false : timeout or error
     */
    bool wait(Mutex& mutex, int timeout);

    void signal();
    void broadcast();

private:
    CondVar(const CondVar&) = delete;
    CondVar& operator= (const CondVar&) = delete;

private:
    pthread_cond_t mId;

};


#endif /* __CONDITION_VARIABLE_H_ */
