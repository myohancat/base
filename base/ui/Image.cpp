/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "Image.h"

#include "Rectangle.h"
#include "Log.h"

#include <SkStream.h>
#include <SkImage.h>
#include <SkBitmap.h>

Image::Image(const std::string& path, bool preload)
      : mBitmap(NULL),
        mPath(path),
        mWidth(-1),
        mHeight(-1)
{
    if(preload)
        loadImage();
}

Image::Image(const std::string& path, int width, int height, bool preload)
      : mBitmap(NULL),
        mPath(path),
        mWidth(width),
        mHeight(height)
{
    if(preload)
        loadImage();
}

Image::Image(Image& img)
      : mBitmap(NULL),
        mPath(img.mPath),
        mWidth(img.getWidth()),
        mHeight(img.getHeight())
{
    mBitmap = new SkBitmap(*img.mBitmap);
}

Image::Image(Image& img, const Rectangle& rect)
      : mPath(""),
        mWidth(rect.getWidth()),
        mHeight(rect.getHeight())
{
    SkIRect skRect = SkIRect::MakeXYWH(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    mBitmap = new SkBitmap();
    if (!img.mBitmap->extractSubset(mBitmap, skRect))
        LOGE("Cannot make subset bitmap");

    mWidth = mBitmap->width();
    mHeight = mBitmap->height();
}

Image::Image(uint8_t* ptr, int width, int height)
{
    mBitmap = new SkBitmap();

    SkImageInfo info = SkImageInfo::MakeN32(width, height, kPremul_SkAlphaType);
    size_t rowBytes = info.minRowBytes();
    mBitmap->setInfo(info, rowBytes);
    mBitmap->setPixels(ptr);
}

Image::~Image()
{
    flush();
    delete mBitmap;
}

int Image::getWidth()
{
    if(!mBitmap)
        loadImage();

    return mWidth;
}


int Image::getHeight()
{
    if(!mBitmap)
        loadImage();

    return mHeight;
}


Size Image::getSize()
{
    if(!mBitmap)
        loadImage();

    return Size(mWidth, mHeight);
}

void Image::flush()
{
#if 0 /* [XXX] TBD. IMPLMENTS HERE */
    if(mDFBSurface != NULL)
    {
        mDFBSurface->Release(mDFBSurface);
        mDFBSurface = NULL;
    }
#endif
}

SkBitmap& Image::getBitmap()
{
    if (!mBitmap)
        loadImage();

    return *mBitmap;
}

void* Image::getPixels()
{
    if (!mBitmap)
        loadImage();

    return mBitmap->getPixels();
}

bool Image::loadImage()
{
    sk_sp<SkData> data = SkData::MakeFromFileName(mPath.c_str());
    if (!data)
    {
        LOGE("Cannot read imagefile : %s", mPath.c_str());
        return false;
    }

    sk_sp<SkImage> encoded = SkImage::MakeFromEncoded(data);
    if (!encoded)
    {
        LOGE("Decode fail : %s", mPath.c_str());
        return false;
    }

    if (mWidth  == -1) mWidth  = encoded->width();
    if (mHeight == -1) mHeight = encoded->height();

    SkImageInfo info = SkImageInfo::MakeN32(mWidth, mHeight, kPremul_SkAlphaType);
    mBitmap = new SkBitmap();
    if (!mBitmap->tryAllocPixels(info))
    {
        LOGE("alloc fail");
        return false;
    }

    if (mWidth == encoded->width() && mHeight == encoded->height())
    {
        if (!encoded->readPixels(mBitmap->pixmap(), 0, 0))
        {
            LOGE("readPixels fail");
            return false;
        }
    }
    else
    {
        SkBitmap tmp;
        tmp.allocPixels(SkImageInfo::MakeN32(encoded->width(), encoded->height(), kPremul_SkAlphaType));
        encoded->readPixels(tmp.pixmap(), 0, 0);

        SkSamplingOptions samp(SkFilterMode::kNearest, SkMipmapMode::kNone);
        tmp.pixmap().scalePixels(mBitmap->pixmap(), samp);
    }
    mBitmap->setImmutable();

    return true;
}
