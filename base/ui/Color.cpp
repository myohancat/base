/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "Color.h"

Color Color::TRANSPARENT(0x00, 0x00, 0x00, 0x00);
Color Color::CLEAR(0x00, 0x00, 0x00, 0x00);
Color Color::WHITE(0xFF, 0xFF, 0xFF, 0xFF);
Color Color::BLACK(0x00, 0x00, 0x00, 0xFF);
Color Color::GRAY(0x80, 0x80, 0x80, 0xFF);
Color Color::RED(0xFF, 0x00, 0x00, 0xFF);
Color Color::GREEN(0x00, 0xFF, 0x00, 0xFF);
Color Color::BLUE(0x00, 0x00, 0xFF, 0xFF);

Color::Color()
      : mRed(0),
        mGreen(0),
        mBlue(0),
        mAlpha(0)
{
}

Color::Color(const Color& color)
          : mRed(color.getRed()),
            mGreen(color.getGreen()),
            mBlue(color.getBlue()),
            mAlpha(color.getAlpha())
{
}

Color::Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
          : mRed(red),
            mGreen(green),
            mBlue(blue),
            mAlpha(alpha)
{
}

Color::~Color()
{
}

Color& Color::operator=(const Color& color)
{
    if(this != &color)
    {
        mRed = color.getRed();
        mGreen = color.getGreen();
        mBlue = color.getBlue();
        mAlpha = color.getAlpha();
    }

    return *this;
}

bool Color::operator==(const Color& color) const
{
    return ((mRed == color.getRed()) && (mGreen == color.getGreen()) &&
            (mBlue == color.getBlue()) && (mAlpha == color.getAlpha()));
}

bool Color::operator!=(const Color& color) const
{
    return ((mRed != color.getRed()) || (mGreen != color.getGreen()) ||
            (mBlue != color.getBlue()) || (mAlpha != color.getAlpha()));
}
