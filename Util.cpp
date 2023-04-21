/**
 * My simple event loop source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "Util.h"

#include <stdio.h>
#include <string.h>

#define ISSPACE(c)     ((c)==' ' || (c)=='\f' || (c)=='\n' || (c)=='\r' || (c)=='\t' || (c)=='\v')

char* ltrim(char *s)
{
    if(!s) return s;

    while(ISSPACE(*s)) s++;

    return s;
}

char* rtrim(char* s)
{
    char* back;

    if(!s) return s;

    back = s + strlen(s);

    while((s <= --back) && ISSPACE(*back));
    *(back + 1)    = '\0';

    return s;
}
