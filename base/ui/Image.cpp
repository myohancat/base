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

    SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
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
    SkFILEStream fileStream(mPath.c_str());
    if (!fileStream.isValid())
    {
        LOGE("Cannot read imagefile : %s", mPath.c_str());
        return false;
    }

    int   fileLen = fileStream.getLength();
    char* buffer  = new char[fileLen];
    fileStream.read(buffer, fileLen);
    fileStream.close();

    sk_sp<SkData> data   = SkData::MakeWithoutCopy(buffer, fileLen);
    sk_sp<SkImage> image = SkImage::MakeFromEncoded(data);

    mWidth = image->width();
    mHeight = image->height();

    mBitmap = new SkBitmap();
    image->asLegacyBitmap(mBitmap, SkImage::kRO_LegacyBitmapMode);

    delete[] buffer;

    return true;
}
