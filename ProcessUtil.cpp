/**
 * MetaSCOPE Service
 *
 * Author: Kyungyin.Kim < kyungyin.kim@medithinq.com >
 * Copyright (c) 2021, MedithinQ. All rights reserved.
 */
#include "ProcessUtil.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>

#include "Log.h"

namespace ProcessUtil
{

static void close_all_fds()
{
    int ii = 0;
    int flags =0;

    for(ii = 3; ii <= getdtablesize(); ii++)
    {
        if((flags = fcntl(ii, F_GETFD)) != -1)
        {
            fcntl(ii, F_SETFD, flags | FD_CLOEXEC);
        }
    }
}

static void init_signal()
{
    sigset_t set;
    sigemptyset(&set);
    sigprocmask(SIG_SETMASK, &set, NULL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
}

int get_pid(const char* process)
{
    int nPid = -1;
    char szPid[16] = {0,};
    char szCmd[128] = {0,};
    FILE *pFP = NULL;

    snprintf(szCmd, sizeof(szCmd), "pidof %s", process);
    if((pFP = ::popen(szCmd, "r")) != NULL)
    {
        if(fgets(szPid, sizeof(szPid), pFP) != NULL)
        {
            nPid = atoi(szPid);
        }

        ::pclose(pFP);
    }

    return nPid;
}

int kill(const char* process)
{
    int pid = get_pid(process);

    if(pid < 0)
    {
        LOGE("cannot found %s : pid %d\n", process, pid);
        return -1;
    }

    ::kill(pid, SIGTERM);

    return 0;
}

int kill_force(const char* process)
{
    int pid = get_pid(process);
    if(pid < 0)
    {
        LOGE("cannot found %s : pid %d\n", process, pid);
        return -1;
    }

    ::kill(pid, SIGKILL);

    return 0;
}

int system(const char* command)
{
    pid_t pid;
    int   status;

    close_all_fds();

    pid = fork();
    if(pid < (pid_t)0)
    {
        LOGE("Fork is failed !\n");
        return -1;
    }

    if(pid == (pid_t)0) // CHILD
    {
        const char* new_argv[4];
        new_argv[0] = "sh";
        new_argv[1] = "-c";
        new_argv[2] = command;
        new_argv[3] = NULL;

        init_signal();

        execve("/bin/sh", (char *const *) new_argv, __environ);
        _exit(127);
    }
    else // Parent
    {
        pid_t ret;
        do
        {
            ret = waitpid(pid, &status, 0);
        }while(ret == (pid_t)-1 && errno == EINTR);
        if(ret != pid)
        {
            LOGE("Wait PID is failed !\n");
            status = -1;
        }
    }

    return status;
}

} // namepspace ProcessUtil
