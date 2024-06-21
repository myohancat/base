/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "Util.h"

#include <stdio.h>
#include <string.h>

#include "Log.h"

char* ltrim(char *s)
{
    if(!s) return s;

    while(IS_SPACE(*s)) s++;

    return s;
}

char* rtrim(char* s)
{
    char* back;

    if(!s) return s;

    back = s + strlen(s);

    while((s <= --back) && IS_SPACE(*back));
    *(back + 1)    = '\0';

    return s;
}

#define MAX_LINE_LEN     (1024)
#define MAX_SECTION_LEN  (64)
#define MAX_NAME_LEN     (64)

int parse_ini(FILE* file, fnIniCB cb, void* param)
{
    char line[MAX_LINE_LEN];

    char section[MAX_SECTION_LEN] = { 0, };

    char* start;
    char* end;
    char* name;
    char* value;

    int lineno = 0;
    int error  = 0;
    const char* errmsg = NULL;

    while (fgets(line, MAX_LINE_LEN, file) != NULL)
    {
        lineno++;

        start = line;
        start = ltrim(rtrim(start));

        if(*start == ';' || *start == '#')
            continue;

        if(*start == '[')
        {
            end = strchr(start + 1, ']');
            if(*end == ']')
            {
                *end = '\0';
                strncpy(section, start + 1, MAX_SECTION_LEN);
                section[MAX_SECTION_LEN - 1] = 0;
            }
            else if(!error)
            {
                error  = lineno;
                errmsg = "missing \']\'";
            }
        }
        else if(*start)
        {
            end = strchr(start, '=');
            if(!end) end = strchr(start, ':');

            if(end)
            {
                *end = '\0';
                name = rtrim(start);
                value = ltrim(end + 1);

                if (IS_QUOTE(*value))
                {
                    end = strchr(value + 1, *value);
                    if (!end)
                    {
                        error  = lineno;
                        errmsg = "missing \" or \'";
                        break;
                    }
                    *(++end) = '\0';
                }
                else
                {
                    // remove comment
                    end = strchr(value, '#');
                    if(!end) end = strchr(value, ';');
                    if(end) *end = '\0';
                }

                rtrim(value);
                if(cb(param, section, name, value) < 0)
                {
                    error = lineno;
                    errmsg = "handler is failed.";
                    break;
                }
            }
            else if(!error)
            {
                error = lineno;
                errmsg = "cannot find \'=:\' for value.";
                break;
            }
        }

        if(error)
        {
            LOGE("Parsing is failed ! line : %d  reason : %s", lineno, errmsg);
            break;
        }
    }

    return error;
}

#define MAX_STR_LEN (2*1024)
void parse_key_value_str(const char* str, KeyValueCB_Fn cb, void* userdat)
{
    int index = 0;
    char buf[MAX_STR_LEN];
    char* p = buf, *key = NULL, *value = NULL;

    strncpy(buf, str, MAX_STR_LEN);
    buf[MAX_STR_LEN -1] = 0;

    while(*p)
    {
        if (!key)
        {
            while (IS_SPACE(*p)) p++;
            if (*p == 0) continue;
            key = p++;
        }
        else if(!value)
        {
            while (*p && *p != '=' && !IS_SPACE(*p)) p++;
            if (*p == '=')
            {
                *p++ = 0;
                value = p;
                if(IS_QUOTE(*value))
                {
                    p++;
                    while(*p && *value != *p) p++;
                }
            }
        }

        if (IS_SPACE(*p))
        {
            *p++ = 0;
            if (cb)
                cb(index++, key, value, userdat);

            key = value = NULL;
        }
        else
            p++;
    }

    if (key)
    {
        if (cb)
            cb(index++, key, value, userdat);
    }
}
