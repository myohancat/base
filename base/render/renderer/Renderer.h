/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __RENDERER_COMMON_H_
#define __RENDERER_COMMON_H_

typedef enum
{
    PIXEL_FORMAT_RGBA,
    PIXEL_FORMAT_RGB,
    PIXEL_FORMAT_NV12,

    PIXEL_FORMAT_NOT_SUPPORT
} PixelFormat_e;

typedef enum
{
    COLOR_MODE_RGBA,
    COLOR_MODE_GRAYSCALE,

    MAX_COLOR_MODE
} ColorMode_e;

typedef enum
{
    MEM_TYPE_EGL,
    MEM_TYPE_SYS_MEM,

    MAX_MEM_TYPE
} MemoryType_e;

struct ImageFrame
{
    PixelFormat_e mFmt;
    int           mWidth;
    int           mHeight;

    MemoryType_e  mMemType;
    union
    {
        uint8_t*    mRawData;
        EGLImageKHR mEglImg;
    };
};

class IRenderer
{
public:
    virtual ~IRenderer() { }

    virtual void setColorMode(ColorMode_e eMode) = 0;
    virtual void setView(int x, int y, int width, int height) = 0;
    virtual void setCrop(int x, int y, int width, int height) = 0;
    virtual void setAlpha(float alpha) = 0;
    virtual void setMVP(float* mvp) = 0;

    virtual void draw(ImageFrame* image) = 0;
};

#endif /* __RENDERER_COMMON_H_ */
