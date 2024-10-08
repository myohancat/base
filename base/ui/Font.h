/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __FONT_H_
#define __FONT_H_

#include <string>
#include "Size.h"

#include <SkFont.h>

class Font
{
public:

    enum FontStyle_e
    {
        FONT_STYLE_PLAIN  = 0x00,
        FONT_STYLE_ITALIC = (0x01 << 1),
        FONT_STYLE_BOLD   = (0x01 << 2)
    };

    Font(const std::string& path, int size);
    Font(const std::string& path, int size, FontStyle_e style);
    Font(const Font& font);

    ~Font();

    Size getExtents(const std::string& text) const;

    Size getGlyphExtents(unsigned int c) const;

    void  stringBreak(const char* text, int offset, int maxWidth, int* lineWidth, int* length, const char** nextLine);

    const std::string getEllipsisString(const std::string& str, int maxWidth);

    std::shared_ptr<SkFont> getSkFont() const;

private:
    std::string              mPath;
    int                      mSize;
    FontStyle_e              mStyle;
    std::shared_ptr<SkFont>  mFont;

    bool loadFont();
};

#endif /* __FONT_H_ */
