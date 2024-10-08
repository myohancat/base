/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __SIZE_H_
#define __SIZE_H_

class Size
{
public:
    Size();

    Size(const Size& size);

    Size(int width, int height);

    ~Size();

    int getWidth() const;

    int getHeight() const;

    void setWidth(int width);

    void setHeight(int height);

    Size&
    operator=(const Size &size);

    bool
    operator==(const Size &size) const;

    bool
    operator!=(const Size &size) const;

private:
    int mWidth;

    int mHeight;
};

inline int Size::getWidth() const
{
    return mWidth;
}

inline int Size::getHeight() const
{
    return mHeight;
}

#endif /* __SIZE_H_ */
