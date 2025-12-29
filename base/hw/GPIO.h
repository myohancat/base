/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __GPIO_H_
#define __GPIO_H_

#include "Types.h"

#include <string>

typedef enum {
    GPIO_DIR_IN,
    GPIO_DIR_OUT,

    GPIO_DIR_UNKNOWN
}GPIO_Dir_e;

typedef enum {
    GPIO_EDGE_NONE,
    GPIO_EDGE_RISING,
    GPIO_EDGE_FALLING,
    GPIO_EDGE_BOTH,

    GPIO_EDGE_UNKNOWN
}GPIO_Edge_e;

class GPIO
{
public:
    static GPIO* open(int num);
    static GPIO* open(int num, const std::string ioname);
    virtual ~GPIO() { }

    GPIO_Dir_e   getOutDir();
    void         setOutDir(GPIO_Dir_e dir);

    GPIO_Edge_e  getEdge();
    void         setEdge(GPIO_Edge_e  edge);

    bool         isActiveLow();
    void         setActiveLow(bool activeLow);

    bool         getValue();
    void         setValue(bool value);

    const std::string& getPath() { return mPath; }

    int          getNumber() { return mNum; }

protected:
    GPIO(int num);
    GPIO(int num, const std::string ioname);

protected:
    static bool _export(int num);
    static bool _export(std::string ioname);
    static bool _exist(int num);
    static bool _exist(std::string ioname);

private:
    int         mNum;
    std::string mPath;
};


#endif //__GPIO_H_
