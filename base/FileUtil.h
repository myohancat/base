/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __FILE_UTIL_H_
#define __FILE_UTIL_H_

#include "Types.h"
#include <sys/types.h>

namespace FileUtil
{

int filesize(const char* file, size_t* psize);

typedef void (*CopyCB_fn)(void* param, const char* file, size_t copied);
int copy(const char* src, const char* dst, CopyCB_fn fnCB, void* param);

int mkdir(const char* path, mode_t mode=0644);

} // namespace FileUtil

#endif /* __FILE_UTIL_H_ */
