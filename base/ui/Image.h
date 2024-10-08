/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __IMAGE_H_
#define __IMAGE_H_

#include <string>

#include "Rectangle.h"
#include "Size.h"

#include <SkImage.h>

class Image
{
public:
    Image(const std::string& path, bool preload = false);
    Image(const std::string& path, int width, int height, bool preload = false);
    Image(Image& img);
    Image(Image& img, const Rectangle& rect); // Load Sub image
    Image(uint8_t* ptr, int width, int height); // direct load from memory

    ~Image();

    int getWidth();
    int getHeight();

    Size getSize();

    std::string getPath() const;

    SkBitmap& getBitmap();
    void*     getPixels();

    void flush();

private:
    SkBitmap*        mBitmap;

    std::string      mPath;

    int              mWidth;
    int              mHeight;

    bool loadImage();
};

inline std::string Image::getPath() const
{
    return mPath;
}

#endif /* __IMAGE_H_ */
