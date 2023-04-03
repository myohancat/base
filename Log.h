/**
 * My simple event loop source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __LOG_H_
#define __LOG_H_

#include "Types.h"

#include <stdio.h>
#include <string.h>

#define BASENAME(str)       (strrchr(str, '/') ? strrchr(str, '/') + 1 : str)
#define __BASE_FILE_NAME__   BASENAME(__FILE__)

#define ANSI_COLOR_NONE    NULL
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

typedef enum
{
    LOG_LEVEL_NONE,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE,

    MAX_LOG_LEVEL
}LOG_LEVEL_e;

#ifdef __cplusplus
extern "C"
{
#endif

void         LOG_SetLevel(LOG_LEVEL_e eLevel);
LOG_LEVEL_e  LOG_GetLevel();

void LOG_Print(int priority, const char* color, const char* fmt, ...);
void LOG_Dump(int priority, const void* ptr, int size);

#define MAX_FUNCION_SIZE    (1024)
const char* simplify_function(char* buf, const char* func);

#define LOGT(fmt, args...)      do { \
                                         LOG_Print(LOG_LEVEL_TRACE, ANSI_COLOR_NONE, fmt, ##args); \
                                     } while(0)

#define LOGD(fmt, args...)      do { \
                                         LOG_Print(LOG_LEVEL_DEBUG, ANSI_COLOR_NONE, fmt, ##args); \
                                     } while(0)

#define LOGI(fmt, args...)       do { \
                                         char tmp[MAX_FUNCION_SIZE]; \
                                         const char* func = simplify_function(tmp, __PRETTY_FUNCTION__); \
                                         LOG_Print(LOG_LEVEL_INFO, ANSI_COLOR_NONE, "[%s:%d] %s() " fmt, __BASE_FILE_NAME__, __LINE__, func, ##args); \
                                     } while(0)

#define LOGW(fmt, args...)      do { \
                                         char tmp[MAX_FUNCION_SIZE]; \
                                         const char* func = simplify_function(tmp, __PRETTY_FUNCTION__); \
                                         LOG_Print(LOG_LEVEL_WARN, ANSI_COLOR_YELLOW, "[%s:%d] %s() " fmt, __BASE_FILE_NAME__, __LINE__, func, ##args); \
                                     } while(0)

#define LOGE(fmt, args...)      do { \
                                         char tmp[MAX_FUNCION_SIZE]; \
                                         const char* func = simplify_function(tmp, __PRETTY_FUNCTION__); \
                                         LOG_Print(LOG_LEVEL_ERROR, ANSI_COLOR_RED, "[%s:%d] %s() " fmt, __BASE_FILE_NAME__, __LINE__, func, ##args); \
                                     } while(0)

#ifdef __cplusplus
class AutoFunctionTrace
{
public:
    AutoFunctionTrace(const char* filename, int line, const char* func)
    {
        char tmp[MAX_FUNCION_SIZE];

        snprintf(mStackTrace, MAX_FUNCION_SIZE-1, "[%s:%d] %s", filename, line, simplify_function(tmp, func));

        LOGT("------ %s() Enter ------\n", mStackTrace);
    }

    ~AutoFunctionTrace()
    {
        LOGT("------ %s() Leave ------\n", mStackTrace);
    }

private:
    char mStackTrace[MAX_FUNCION_SIZE];
};

#define __TRACE__   AutoFunctionTrace __function_trace__(__BASE_FILE_NAME__, __LINE__, __PRETTY_FUNCTION__);
#else
#define __TRACE__   LOGT("----- [%s:%d] %s() -----\n", __BASE_FILE_NAME__, __LINE__, __PRETTY_FUNCTION__);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __LOG_H_ */
