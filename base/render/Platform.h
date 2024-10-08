/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __PLATFORM_H_
#define __PLATFORM_H_

#include "Types.h"
#include <EGL/egl.h>
#include <EGL/eglplatform.h>

class IPlatform
{
public:
    virtual ~IPlatform() { }

    virtual NativeDisplayType getDisplay()    = 0;
    virtual NativeWindowType  getWindow()     = 0;

    /* For eglGetPlatformDisplayEXT */
    virtual EGLDisplay        getEGLDisplay() { return NULL; };

    virtual int getScreenWidth()  = 0;
    virtual int getScreenHeight() = 0;
};

#endif /* __PLATFORM_H_ */
