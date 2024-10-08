/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "Rectangle.h"

#ifndef _MIN
#define _MIN(x, y)   (x > y)?(y):(x)
#endif

#ifndef _MAX
#define _MAX(x, y)   (x > y)?(x):(y)
#endif

Rectangle::Rectangle()
          : mX(0),
            mY(0),
            mWidth(0),
            mHeight(0)
{
}


Rectangle::Rectangle(int x, int y, int width, int height)
          : mX(x),
            mY(y),
            mWidth(width),
            mHeight(height)
{
}

Rectangle::Rectangle(int x, int y, const Size& size)
          : mX(x),
            mY(y),
            mWidth(size.getWidth()),
            mHeight(size.getHeight())
{
}

Rectangle::Rectangle(const Point& point, const Size& size)
          : mX(point.getX()),
            mY(point.getY()),
            mWidth(size.getWidth()),
            mHeight(size.getHeight())
{
}


Rectangle::Rectangle(const Rectangle& rect)
          : mX(rect.getX()),
            mY(rect.getY()),
            mWidth(rect.getWidth()),
            mHeight(rect.getHeight())
{
}


Rectangle::~Rectangle()
{
}


bool Rectangle::contains(const Point& point) const
{
    return contains(point.getX(), point.getY());
}


bool Rectangle::contains(int x, int y) const
{
    return ((x >= mX && x < (mX + mWidth)) &&
            (y >= mY && y < (mY + mHeight)));
}


bool Rectangle::contains(const Rectangle& rect) const
{
    return ((rect.getX() >= mX) && (rect.getX() + rect.getWidth() < mX + mWidth) &&
            (rect.getY() >= mY) && (rect.getY() + rect.getHeight() < mY + mHeight));
}


bool Rectangle::intersects(const Rectangle& rect) const
{
    int iLeft = _MAX(left(), rect.left());
    int iTop  = _MAX(top(), rect.top());

    int iRight  = _MIN(right(), rect.right());
    int iBottom = _MIN(bottom(), rect.bottom());

    if (iRight > iLeft && iBottom > iTop)
        return true;

    return false;
}


Rectangle Rectangle::intersection(const Rectangle& rect) const
{
    int iLeft = _MAX(left(), rect.left());
    int iTop  = _MAX(top(), rect.top());

    int iRight  = _MIN(right(), rect.right());
    int iBottom = _MIN(bottom(), rect.bottom());

    if (iRight > iLeft && iBottom > iTop)
        return Rectangle(iLeft, iTop, iRight - iLeft, iBottom - iTop);

    return Rectangle(0, 0, 0, 0);
}

#define MIN(x, y) ((x>y)?y:x)
#define MAX(x, y) ((x>y)?x:y)

void Rectangle::united(const Rectangle& rect)
{
    if(!rect.isValid())
        return;

    if(!isValid())
    {
        setRectangle(rect);
        return;
    }

    int x1 = MIN(mX, rect.getX());
    int y1 = MIN(mY, rect.getY());

    int x2 = MAX(mX + mWidth, rect.getX() + rect.getWidth());
    int y2 = MAX(mY + mHeight, rect.getY() + rect.getHeight());

    mX = x1;
    mY = y1;
    mWidth = x2 - x1;
    mHeight = y2 - y1;
}

Rectangle& Rectangle::operator=(const Rectangle &rect)
{
    if(this != &rect)
    {
        mX = rect.mX;
        mY = rect.mY;
        mWidth = rect.mWidth;
        mHeight = rect.mHeight;
    }

    return *this;
}

bool Rectangle::operator==(const Rectangle &rect)
{
    return (mX == rect.mX) && (mY == rect.mY) && (mWidth == rect.mWidth) && (mHeight == rect.mHeight);
}

bool Rectangle::operator!=(const Rectangle &rect)
{
    return !(*this == rect);
}

