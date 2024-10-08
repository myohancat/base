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
    COLOR_MODE_RGBA,
    COLOR_MODE_GRAYSCALE,

    MAX_COLOR_MODE
} ColorMode_e;

typedef enum
{
    PIXEL_FORMAT_RGBA,
    PIXEL_FORMAT_RGB,
    PIXEL_FORMAT_NV12,

    PIXEL_FORMAT_NOT_SUPPORT
} PixelFormat_e;

struct RawImageFrame
{
    PixelFormat_e mFmt;
    int           mWidth;
    int           mHeight;

    uint8_t*  mRawData;
};

#endif /* __RENDERER_COMMON_H_ */
