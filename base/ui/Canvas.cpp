/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "Canvas.h"

#include "RenderService.h"
#include "Log.h"

#include <SkRect.h>
#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkTextBlob.h>

#include "EGLHelper.h"
#include <sys/mman.h>
#include <linux/dma-buf.h>
#include <sys/ioctl.h>

Canvas::Canvas()
       : mWidth(-1),
         mHeight(-1)
{
    mWidth = RenderService::getInstance().getScreenWidth();
    mHeight = RenderService::getInstance().getScreenHeight();

    init();
}

Canvas::Canvas(int width, int height)
       : mWidth(width),
         mHeight(height)
{
    init();
}

Canvas::~Canvas()
{
    deinit();
}

void Canvas::drawText(const Font& font, const std::string& text, int x, int y, const Color& color, Rectangle* res)
{
    if (!mCanvas)
        return;

    std::shared_ptr<SkFont> skFont = font.getSkFont();
    SkRect bounds;
    SkPaint paint;
    SkColor skColor = SkColorSetARGB(color.getAlpha(), color.getRed(), color.getGreen(), color.getBlue());
    paint.setColor(skColor);
    paint.setAntiAlias(true);
    paint.setDither(true);

    skFont->measureText(text.c_str(), strlen(text.c_str()), SkTextEncoding::kUTF8, &bounds);
    if (x < 0)
        x = (mWidth - bounds.width()) /2;
    if (y < 0)
        y = (mHeight - bounds.height()) /2;

    /* Move to font cornor */
    int realX = x - (int)bounds.x();
    int realY = y - (int)bounds.y();

    mCanvas->drawTextBlob(SkTextBlob::MakeFromText(text.c_str(), strlen(text.c_str()), *skFont, SkTextEncoding::kUTF8), realX, realY, paint);

    if (res)
        res->setRectangle(x, y, bounds.width(), bounds.height());
}

void Canvas::drawImage(Image* img, int x, int y, Rectangle* res, BlendMode_e eMode)
{
    SkPaint paint;
    paint.setAlpha(0xFF);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setBlendMode(convBlendMode(eMode));
    //paint.setAntiAlias(true);
    //paint.setDither(true);

    if (x < 0)
        x = (mWidth - img->getWidth()) /2;
    if (y < 0)
        y = (mHeight - img->getHeight()) /2;

    SkSamplingOptions sampling;
    SkRect skRect = SkRect::MakeXYWH(x, y, img->getWidth(), img->getHeight());
    mCanvas->drawImageRect(img->getBitmap().asImage(), skRect, sampling, &paint);
    if (res)
        res->setRectangle(x, y, img->getSize());
}

void Canvas::drawImageStretch(Image* img, Rectangle& dest, Rectangle* res, BlendMode_e eMode)
{
    SkPaint paint;
    paint.setAlpha(0xFF);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setBlendMode(convBlendMode(eMode));
    paint.setAntiAlias(true);
    paint.setDither(true);

    SkSamplingOptions sampling;
    SkRect skRect = SkRect::MakeXYWH(dest.getX(), dest.getY(), dest.getWidth(), dest.getHeight());
    mCanvas->drawImageRect(img->getBitmap().asImage(), skRect, sampling, &paint);
    if (res)
        res->setRectangle(dest);
}

void Canvas::drawRectangle(const Rectangle& rect, const Color& color, BlendMode_e eMode)
{
    if (!mCanvas)
        return;

    SkPaint paint;
    SkColor skColor = SkColorSetARGB(color.getAlpha(), color.getRed(), color.getGreen(), color.getBlue());

    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(skColor);
    paint.setBlendMode(convBlendMode(eMode));

    int x = rect.getX();
    int y = rect.getY();

    if (x < 0)
        x = (mWidth - rect.getWidth()) /2;
    if (y < 0)
        y = (mHeight - rect.getHeight()) /2;

    SkRect skRect = SkRect::MakeXYWH(x, y, rect.getWidth(), rect.getHeight());

    mCanvas->drawRect(skRect, paint);
    mCanvas->restore();
}

void Canvas::drawCircle(const Point& center, int radius, const Color& color, BlendMode_e eMode)
{
    if (!mCanvas)
        return;

    SkPaint paint;
    SkColor skColor = SkColorSetARGB(color.getAlpha(), color.getRed(), color.getGreen(), color.getBlue());

    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(skColor);
    paint.setBlendMode(convBlendMode(eMode));

    mCanvas->drawCircle(center.getX(), center.getY(), radius, paint);
    mCanvas->restore();
}

void Canvas::drawLine(const Point& p0, const Point& p1, const Color& color, float strokeWidth, BlendMode_e eMode)
{
    if (!mCanvas)
        return;

    SkPaint paint;
    SkColor skColor = SkColorSetARGB(color.getAlpha(), color.getRed(), color.getGreen(), color.getBlue());

    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(strokeWidth);
    paint.setColor(skColor);
    paint.setBlendMode(convBlendMode(eMode));

    mCanvas->drawLine(p0.getX(), p0.getY(), p1.getX(), p1.getY(), paint);
    mCanvas->restore();
}

void Canvas::clear(const Color& color)
{
    if (!mCanvas)
        return;

    SkColor skColor = SkColorSetARGB(color.getAlpha(), color.getRed(), color.getGreen(), color.getBlue());
    mCanvas->clear(0x00);
    mCanvas->drawColor(skColor);
    mCanvas->restore();
}

void Canvas::clear(const Rectangle& rect, const Color& color)
{
    if (!mCanvas)
        return;

    drawRectangle(rect, color, BLEND_MODE_CLEAR);
}

SkBlendMode Canvas::convBlendMode(BlendMode_e eMode)
{
    switch(eMode)
    {
        case BLEND_MODE_CLEAR:    return SkBlendMode::kClear;
        case BLEND_MODE_SRC:      return SkBlendMode::kSrc;
        case BLEND_MODE_DST:      return SkBlendMode::kDst;
        case BLEND_MODE_SRCOVER:  return SkBlendMode::kSrcOver;;
        case BLEND_MODE_DSTOVER:  return SkBlendMode::kDstOver;;
        case BLEND_MODE_SRC_IN:   return SkBlendMode::kSrcIn;;
        case BLEND_MODE_DST_IN:   return SkBlendMode::kDstIn;
        case BLEND_MODE_SRC_OUT:  return SkBlendMode::kSrcOut;
        case BLEND_MODE_DST_OUT:  return SkBlendMode::kDstOut;
        case BLEND_MODE_SRC_ATOP: return SkBlendMode::kSrcATop;
        case BLEND_MODE_DST_ATOP: return SkBlendMode::kDstATop;
        case BLEND_MODE_XOR:      return SkBlendMode::kXor;
        default: return SkBlendMode::kSrc; // DEFAULT VALUE
    }
}

bool Canvas::init()
{
    std::shared_ptr<SkBitmap> bitmap = std::make_shared<SkBitmap>();
    bitmap->setInfo(SkImageInfo::MakeN32(mWidth, mHeight, kPremul_SkAlphaType));
    bitmap->allocPixels();

    std::shared_ptr<SkCanvas> canvas = std::make_shared<SkCanvas>(*bitmap);
    canvas->clear(0x00);
    canvas->restore();

    mBitmap = bitmap;
    mCanvas = canvas;

    return true;
}


void Canvas::deinit()
{
    mCanvas.reset();
    mBitmap.reset();
}

void* Canvas::getPixels()
{
    if(!mBitmap)
        return NULL;

    return mBitmap->getPixels();
}
