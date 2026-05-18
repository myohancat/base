/**
 * My simple base code
 * for developing embedded system.
 *
 * author: Kyungin.Kim < myohancat@naver.com >
 */
#include "Log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/time.h>

static LOG_LEVEL_e  gLogLevel = LOG_LEVEL_TRACE;
static bool gLogWithTime = true;

#if 1 /* Enable Mutual logs */
static pthread_mutex_t gLogMutex = PTHREAD_MUTEX_INITIALIZER;

#define LOCK_LOG_OUT()    pthread_mutex_lock(&gLogMutex)
#define UNLOCK_LOG_OUT()  pthread_mutex_unlock(&gLogMutex)
#else
#define LOCK_LOG_OUT()    do { } while(0)
#define UNLOCK_LOG_OUT()  do { } while(0)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

const char* simplify_function(char* buf, const char* func)
{
    char* begin, *end, *p;

    strncpy(buf, func, MAX_FUNCTION_SIZE-1);
    buf[MAX_FUNCTION_SIZE-1] = 0;

    end = strrchr(buf, '(');
    if (end) *end = 0;

    begin = NULL;
    p = buf;
    while((p = strchr(p, ' '))) { begin = p; p++; }
    if (begin) begin++;
    else begin = buf;

    return begin;
}

static FILE* output_device(void)
{
    // TODO IMPLEMETNS HERE
    return stdout;
}

static const char *cur_time_str(char *buf)
{
    struct timeval tv;
    struct tm t_buf;
    gettimeofday(&tv, 0);
    time_t curtime = tv.tv_sec;

    struct tm *t = localtime_r(&curtime, &t_buf);

    snprintf(buf, 32, "[%02d:%02d:%02d.%03ld] ", t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec / 1000);
    return buf;
}

void LOG_SetLevel(LOG_LEVEL_e eLevel)
{
    gLogLevel = eLevel;
}

LOG_LEVEL_e LOG_GetLevel(void)
{
    return gLogLevel;
}

void LOG_Print(int priority, const char* color, const char* fmt, ...)
{
    va_list ap;
    FILE* fp;

    if(priority > gLogLevel)
        return;

    LOCK_LOG_OUT();

    fp = output_device();

    if(color)
        fputs(color, fp);

    if(gLogWithTime)
    {
        char timestr[32];
        fputs(cur_time_str(timestr), fp);
    }

    va_start(ap, fmt);
    vfprintf(fp, fmt, ap);
    va_end(ap);

    if(color)
        fputs(ANSI_COLOR_RESET, fp);

    fflush(fp);

    UNLOCK_LOG_OUT();
}

#define ISPRINTABLE(c)  (((c)>=32 && (c)<=126))

void LOG_Dump(int priority, const void* ptr, int size)
{
    FILE* fp;
    char buffer[128];
    int ii, n;
    int offset = 0;
    const unsigned char* data = (const unsigned char*)ptr;

    if(priority > gLogLevel)
        return;

    if (!ptr || size == 0)
        return;

    fp = output_device();

    LOCK_LOG_OUT();

    while (offset < size)
    {
        char* p = buffer;
        int remain = size - offset;

        n = sprintf(p, "0x%04x  ", offset);
        p += n;

        if (remain > 16)
            remain = 16;

        for (ii = 0; ii < 16; ii++)
        {
            if (ii == 8)
                *p++ = ' ';

            if (offset + ii < size)
                n = snprintf(p, "%02x ", data[offset + ii]);
            else
                n = sprintf(p, "   ");

            p += n;
        }

        *p++ = ' ';

        for (ii = 0; ii < remain; ii++)
        {
            if (ISPRINTABLE(data[offset + ii]))
                *p++ = (char)data[offset + ii];
            else
                *p++ = '.';
        }
        *p++ = '\n';
        *p = '\0';

        offset += 16;
        fputs(buffer, fp);
    }

    fflush(fp);
    UNLOCK_LOG_OUT();
}

#ifdef __cplusplus
}
#endif
