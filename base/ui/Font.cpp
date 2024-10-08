/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "Font.h"

#include "Log.h"

#include <SkTypeface.h>

Font::Font(const std::string& path, int size)
     : mPath(path),
       mSize(size),
       mStyle(FONT_STYLE_PLAIN)
{
    loadFont();

}

Font::Font(const std::string& path, int size, FontStyle_e style)
     : mPath(path),
       mSize(size),
       mStyle(style)
{
    loadFont();
}

Font::Font(const Font& font)
     : mPath(font.mPath),
       mSize(font.mSize),
       mStyle(font.mStyle)
{
    if (font.mFont == NULL)
        LOGE("mFont is NULL");
    else
        mFont = font.mFont;
}

Font::~Font()
{
}

Size Font::getExtents(const std::string& text) const
{
    SkRect bounds;

    mFont->measureText(text.c_str(), strlen(text.c_str()), SkTextEncoding::kUTF8, &bounds);

    return Size(bounds.width(), bounds.height());
}


Size Font::getGlyphExtents(unsigned int c) const
{
    // TBD. IMPLMENETS HERE
    UNUSED(c);

    return Size(-1, -1);
}

void Font::stringBreak(const char* text, int offset, int maxWidth, int* lineWidth, int* length, const char** nextLine)
{
    UNUSED(text);
    UNUSED(offset);
    UNUSED(maxWidth);
    UNUSED(lineWidth);
    UNUSED(length);
    UNUSED(nextLine);

    // TBD. IMPLEMENTS HERE
}

const std::string Font::getEllipsisString(const std::string& str, int maxWidth)
{
    UNUSED(str);
    UNUSED(maxWidth);

    // TBD. IMPLEMENTS HERE

    return str;
}

std::shared_ptr<SkFont> Font::getSkFont() const
{
    return mFont;
}

bool Font::loadFont()
{
    mFont = std::make_shared<SkFont>(SkTypeface::MakeFromFile(mPath.c_str()), mSize);

    return true;
}
