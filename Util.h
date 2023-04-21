/**
 * My simple event loop source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __UTIL_H_
#define __UTIL_H_

#ifdef __cplusplus
extern "C"
{
#endif

char* ltrim(char *s);
char* rtrim(char* s);

#define trim(s)        rtrim(ltrim(s))

#ifdef __cplusplus
}
#endif

#endif /*__UTIL_H_*/
