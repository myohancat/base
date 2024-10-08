/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __COLOR_H_
#define __COLOR_H_

#include "Types.h"

class Color
{
public:
    static Color TRANSPARENT;
    static Color CLEAR;
    static Color WHITE;
    static Color BLACK;
    static Color GRAY;
    static Color RED;
    static Color GREEN;
    static Color BLUE;

    Color();

    Color(const Color& color);

    Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255);

    ~Color();

    uint8_t getRed() const;
    uint8_t getGreen() const;
    uint8_t getBlue() const;
    uint8_t getAlpha() const;

    void setColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255);

    Color& operator=(const Color& color);
    bool   operator==(const Color& color) const;
    bool   operator!=(const Color& color) const;

private:
    uint8_t mRed;
    uint8_t mGreen;
    uint8_t mBlue;
    uint8_t mAlpha;
};

inline uint8_t Color::getRed() const
{
    return mRed;
}

inline uint8_t Color::getGreen() const
{
    return mGreen;
}

inline uint8_t Color::getBlue() const
{
    return mBlue;
}

inline uint8_t Color::getAlpha() const
{
    return mAlpha;
}

inline void Color::setColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    mRed    = red;
    mGreen  = green;
    mBlue   = blue;
    mAlpha  = alpha;
}

#endif /* __COLOR_H_ */
