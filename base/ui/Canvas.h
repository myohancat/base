/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __CANVAS_H_
#define __CANVAS_H_

#include <memory>

#include "Mutex.h"
#include "Point.h"
#include "Size.h"
#include "Color.h"
#include "Image.h"
#include "Font.h"

#include <SkBitmap.h>
#include <SkCanvas.h>

enum BlendMode_e
{
    BLEND_MODE_CLEAR,
    BLEND_MODE_SRC,
    BLEND_MODE_DST,
    BLEND_MODE_SRCOVER,
    BLEND_MODE_DSTOVER,
    BLEND_MODE_SRC_IN,
    BLEND_MODE_DST_IN,
    BLEND_MODE_SRC_OUT,
    BLEND_MODE_DST_OUT,
    BLEND_MODE_SRC_ATOP,
    BLEND_MODE_DST_ATOP,
    BLEND_MODE_XOR,

    MAX_BLEND_MODE
};

class Canvas : public ILockable
{
public:
    Canvas();
    Canvas(int width, int height);
    Canvas(int x, int y, int width, int height);

    virtual ~Canvas();

    void   drawText(const Font& font, const std::string& text, int x, int y, const Color& color=Color::WHITE, Rectangle* res = NULL);

    void   drawImage(Image* image, int x = -1, int y = -1, Rectangle* res = NULL, BlendMode_e eMode = BLEND_MODE_SRCOVER);
    void   drawImageStretch(Image* img, Rectangle& dest, Rectangle* res = NULL, BlendMode_e eMode = BLEND_MODE_SRCOVER);
    void   drawImageStretchFixed(Image* img, Rectangle& dest, Rectangle* res = NULL, BlendMode_e eMode = BLEND_MODE_SRCOVER);

    void   drawLine(const Point& p0, const Point& p1, const Color& color, float strokeWidth = 1.0f, BlendMode_e eMode = BLEND_MODE_SRC);
    void   drawRectangle(const Rectangle& rect, const Color& color, BlendMode_e eMode = BLEND_MODE_SRC);
    void   drawCircle(const Point& center, int radius, const Color& color, BlendMode_e eMode = BLEND_MODE_SRC);

    void   clear(const Color& color=Color::CLEAR);
    void   clear(const Rectangle& rect, const Color& color=Color::CLEAR);

    int    getWidth() const;
    int    getHeight() const;

    Size   getSize() const;

    void*  getPixels();

    void   lock();
    void   unlock();

protected:
    bool  init();
    void  deinit();

    void onDraw();
    void onDestroy();

    SkBlendMode convBlendMode(BlendMode_e eMode);

protected:
    std::shared_ptr<SkCanvas>  mCanvas;
    std::shared_ptr<SkBitmap>  mBitmap;

    RecursiveMutex mLock;

    int   mWidth;
    int   mHeight;
    int   mSize;
};

inline int Canvas::getWidth() const
{
    return mWidth;
}

inline int Canvas::getHeight() const
{
    return mHeight;
}

inline Size Canvas::getSize() const
{
    return Size(mWidth, mHeight);
}

inline void Canvas::lock()
{
    mLock.lock();
}

inline void Canvas::unlock()
{
    mLock.unlock();
}

#endif /* __CANVAS_H_ */
