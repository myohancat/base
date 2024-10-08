/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __UTIL_H_
#define __UTIL_H_

#include "Types.h"

#ifdef __cplusplus
extern "C"
{
#endif

char* ltrim(char *s);
char* rtrim(char* s);

#define trim(s)        rtrim(ltrim(s))

/**
 * Ini Parser 
 * example :
 *      [section]
 *      name=value
 */
typedef int (*fnIniCB)(void* param, const char* section, const char* name, const char* value);
int parse_ini(FILE* file, fnIniCB cb, void* param);

/**
 * Key-Value Parser 
 * example :
 *      key1 key2=value2 key3='value3' key4="value4"
 */
typedef void (*KeyValueCB_Fn)(int index, const char* key, const char* value, void* userdat);
void parse_key_value_str(const char* str, KeyValueCB_Fn cb, void* userdat);

#ifdef __cplusplus
}
#endif

#endif /*__UTIL_H_*/
