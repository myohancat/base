/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "FileUtil.h"
#include "Log.h"

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/stat.h>

namespace FileUtil
{

int filesize(const char* file, size_t* psize)
{
    struct stat st;
    if(stat(file, &st) == -1)
    {
        LOGE("Cannot get stat %s : errno(%d) - %s", file, errno, strerror(errno));
        return -1;
    }

    if (S_ISDIR(st.st_mode))
    {
        int    ret = 0;
        size_t size = 0;
        DIR*   dir;
        struct dirent* de;
        char   path[MAX_PATH_LEN];

        dir = opendir(file);
        if(!dir)
        {
            LOGE("opendir failed : %s", file);
            return -1;
        }

        while((de = readdir(dir)))
        {
            size_t sub_size = 0;

            if(de->d_name[0] == '.')
            {
                if(de->d_name[1] == '\0' || (de->d_name[1] == '.' && de->d_name[2] == '\0'))
                    continue;
            }

            sprintf(path, "%s/%s", file, de->d_name);
            if(stat(path, &st) == -1)
                continue;

            ret = filesize(path, &sub_size);
            if (ret)
                break;

            size += sub_size;
        }
        
        if (ret == 0)
            *psize = size;

        closedir(dir);
        return ret;
    }

    FILE* fp = fopen(file, "rb");
    if (!fp)
        return -1;

    fseek(fp, 0, SEEK_END);
    *psize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fclose(fp);

    return 0;
}

int copy(const char* src, const char* dst, CopyCB_fn fnCB, void* param)
{
    struct stat st;

    if(stat(src, &st) == -1)
    {
        LOGE("Cannot get stat %s : errno(%d) - %s", src, errno, strerror(errno));
        return -1;
    }

    if (S_ISDIR(st.st_mode)) /* Directory Copy */
    {
        DIR*   dir;
        struct dirent* de;
        char   sub_src[MAX_PATH_LEN];
        char   sub_dst[MAX_PATH_LEN];

        dir = opendir(src);
        if(!dir)
        {
            LOGE("opendir failed : %s", src);
            return -1;
        }

        FileUtil::mkdir(dst, st.st_mode);
        while((de = readdir(dir)))
        {
            if(de->d_name[0] == '.')
            {
                if(de->d_name[1] == '\0' || (de->d_name[1] == '.' && de->d_name[2] == '\0'))
                    continue;
            }

            sprintf(sub_src, "%s/%s", src, de->d_name);
            if (stat(sub_src, &st) == -1)
                continue;

            sprintf(sub_dst, "%s/%s", dst, de->d_name);

            LOGD("Copy file %s -> %s", sub_src, sub_dst);
            copy(sub_src, sub_dst, fnCB, param);
        }

        closedir(dir);
    }
    else
    {
        char  buffer[1024];
        FILE* fpSrc = NULL;
        FILE* fpDst = NULL;
        size_t readSize, writeSize;

        fpSrc = fopen(src, "rb");
        if (!fpSrc)
        {
            LOGE("Cannot open file %s", src);
            return -1;
        }
        fpDst = fopen(dst, "wb");
        if (!fpDst)
        {
            LOGE("Cannot open file %s", dst);
            fclose(fpSrc);
            return -1;
        }

        const char* p = strrchr(src, '/');
        if(p) p++;

        while((readSize = fread(buffer, 1, sizeof(buffer), fpSrc)) > 0)
        {
            writeSize = fwrite(buffer, 1, readSize, fpDst);
            if (fnCB)
                fnCB(param, p, writeSize);
        }

        stat(src, &st);
        chmod(dst, st.st_mode);

        fclose(fpSrc);

        fflush(fpDst);
        fsync(fileno(fpDst));
        fclose(fpDst);
    }

    return 0;
}

int mkdir(const char* path, mode_t mode)
{
    char buf[4*1024];
    int len = 0;
    int ret = -1;
    const char* p = path;

    if(path == NULL || strlen(path) == 0)
        return ret;

    while((p = strchr(p, '/')) != NULL)
    {
        len = p - path;
        p++;
        path = p;

        if (len == 0)
            continue;

        strncpy(buf, path, len);
        buf[len] = '\0';

        if ((ret = ::mkdir(buf, mode)) == -1)
        {
            if (errno != EEXIST)
            {
                LOGE("mkdir error : %d, %s", errno, strerror(errno));
                return ret;
            }
        }
    }

    return ::mkdir(path, mode);
}

} // namespace FileUtil
