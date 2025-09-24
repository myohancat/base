/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __WIDGET_H_
#define __WIDGET_H_

#include "Rectangle.h"
#include "Window.h"
#include "InputManager.h"

class Window;

enum Align_e
{
    ALIGN_CENTER,
    ALIGN_LEFT,
    ALIGN_RIGHT
};

enum KeyCapability_e
{
    KEY_CAPA_NONE       = 0x00,

    KEY_CAPA_ENTER      = 0x01,
    KEY_CAPA_NUM        = 0x02,
    KEY_CAPA_LEFT_RIGHT = 0x04,

    MAX_KEY_CAPA        = 0xFF
};

class IWidget : public IKeyListener
{
public:
    virtual ~IWidget() { }

    virtual void setWindow(Window* window) = 0;

    virtual void setBackground(const Color& color) = 0;

    virtual void setPos(int x, int y) = 0;
    virtual void setSize(int width, int height) = 0;
    virtual void setRectangle(int x, int y, int width, int height) = 0;

    virtual int  getX() = 0;
    virtual int  getY() = 0;
    virtual int  getWidth() = 0;
    virtual int  getHeight() = 0;
    virtual Rectangle getRectangle() = 0;

    virtual bool contains(int posX, int posY) = 0;

    virtual void setFocus(bool focus) = 0;
    virtual bool isFocused() = 0;

    virtual void setEnable(bool enable) = 0;
    virtual bool isEnabled() = 0;

    virtual void paint() = 0;
    virtual void paint(const Rectangle& rect) = 0;

    virtual int  getKeyCapability()
    {
        return MAX_KEY_CAPA;
    }

    virtual bool onKeyReceived(int keyCode, int state)
    {
        UNUSED(keyCode);
        UNUSED(state);

        return false;
    }

    virtual bool onTouched(int state, int posX, int posY)
    {
        UNUSED(state);
        UNUSED(posX);
        UNUSED(posY);

        return false;
    }
};

#endif /* __WIDGET_H_ */
