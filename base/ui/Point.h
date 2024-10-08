/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __POINT_H_
#define __POINT_H_

class Point
{
public:
    Point();
    Point(const Point& p);
    Point(int x, int y);

    int getX() const;
    int getY() const;

    void setX(int x);
    void setY(int y);
    void setPoint(int x, int y);

    Point& operator=(const Point& p);
    bool   operator==(const Point& p);
    bool   operator!=(const Point& p);

private:
    int mX;
    int mY;
};

inline int Point::getX() const
{
    return mX;
}

inline int Point::getY() const
{
    return mY;
}
#endif /* __POINT_H_ */
