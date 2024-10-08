/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "Size.h"

Size::Size()
     : mWidth(-1),
       mHeight(-1)
{
}


Size::Size(const Size& size)
     : mWidth(size.mWidth),
       mHeight(size.mHeight)
{
}


Size::Size(int width, int height)
     : mWidth(width),
       mHeight(height)
{
}


Size::~Size()
{
}


void Size::setWidth(int width)
{
    mWidth = width;
}


void Size::setHeight(int height)
{
    mHeight = height;
}


Size& Size::operator=(const Size& size)
{
    if(this != &size)
    {
        mWidth = size.mWidth;
        mHeight = size.mHeight;
    }

    return *this;
}


bool Size::operator==(const Size& size) const
{
    return (mWidth == size.mWidth) && (mHeight == size.mHeight);
}


bool Size::operator!=(const Size& size) const
{
    return (mWidth != size.mWidth) || (mHeight != size.mHeight);
}
