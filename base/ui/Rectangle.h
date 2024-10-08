/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __RECTANGLE_H_
#define __RECTANGLE_H_

#include "Point.h"
#include "Size.h"


class Rectangle
{
public:
    Rectangle();
    Rectangle(int x, int y, int width, int height);
    Rectangle(int x, int y, const Size& size);
    Rectangle(const Point& topLeft, const Size& size);
    Rectangle(const Rectangle& rect);

    ~Rectangle();

    int       left() const;
    int       right() const;
    int       top() const;
    int       bottom() const;

    int       getX() const;
    int       getY() const;
    int       getWidth() const;
    int       getHeight() const;

    bool      isValid() const;

    void      setX(int x);
    void      setY(int y);
    void      setWidth(int w);
    void      setHeight(int h);

    void      setPos(int x, int y);
    void      setPos(const Point& p);
    void      setSize(int w, int h);
    void      setSize(const Size& s);
    void      setRectangle(int x, int y, int w, int h);
    void      setRectangle(int x, int y, const Size& s);
    void      setRectangle(const Rectangle& rect);

    bool      contains(const Point& point) const;
    bool      contains(int x, int y) const;
    bool      contains(const Rectangle& rect) const;

    bool      intersects(const Rectangle& rect) const;
    Rectangle intersection(const Rectangle& rect) const;

    void      united(const Rectangle& rect);

    Rectangle& operator=(const Rectangle &rect);
    bool       operator==(const Rectangle &rect);
    bool       operator!=(const Rectangle &rect);

private:
    int mX;
    int mY;
    int mWidth;
    int mHeight;
};

inline int Rectangle::left() const
{
    return mX;
}

inline int Rectangle::right() const
{
    return mX + mWidth;
}

inline int Rectangle::top() const
{
    return mY;
}

inline int Rectangle::bottom() const
{
    return mY + mHeight;
}

inline int Rectangle::getX() const
{
    return mX;
}

inline int Rectangle::getY() const
{
    return mY;
}

inline int Rectangle::getWidth() const
{
    return mWidth;
}

inline int Rectangle::getHeight() const
{
    return mHeight;
}

inline bool Rectangle::isValid() const
{
    if(mWidth > 0 && mHeight > 0)
        return true;

    return false;
}

inline void Rectangle::setX(int x)
{
    mX = x;
}

inline void Rectangle::setY(int y)
{
    mY = y;
}

inline void Rectangle::setWidth(int w)
{
    mWidth = w;
}

inline void Rectangle::setHeight(int h)
{
    mHeight = h;
}

inline void Rectangle::setPos(int x, int y)
{
    mX = x;
    mY = y;
}

inline void Rectangle::setPos(const Point& p)
{
    mX = p.getX();
    mY = p.getY();
}

inline void Rectangle::setSize(int w, int h)
{
    mWidth = w;
    mHeight = h;
}

inline void Rectangle::setSize(const Size& s)
{
    mWidth = s.getWidth();
    mHeight = s.getHeight();
}

inline void Rectangle::setRectangle(int x, int y, int w, int h)
{
    mX = x;
    mY = y;
    mWidth = w;
    mHeight = h;
}

inline void Rectangle::setRectangle(int x, int y, const Size& size)
{
    mX = x;
    mY = y;
    mWidth = size.getWidth();
    mHeight = size.getHeight();
}

inline void Rectangle::setRectangle(const Rectangle& rect)
{
    mX = rect.getX();
    mY = rect.getY();
    mWidth = rect.getWidth();
    mHeight = rect.getHeight();
}


#endif // __RECTANGLE_H_
