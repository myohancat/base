/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "Page.h"

#include "Log.h"

Page::Page(float alpha)
     : Window(ZORDER_UI, 0, 0, -1, -1, alpha)
{
    setListener(this);
}

Page::Page(int width, int height, float alpha)
     : Window(ZORDER_UI, -1, -1, width, height, alpha)
{
    setListener(this);
}

Page::Page(int posX, int posY, int width, int height, float alpha)
     : Window(ZORDER_UI, posX, posY, width, height, alpha)
{
    setListener(this);
}

Page::Page(int zOrder, float alpha)
     : Window(zOrder, 0, 0, -1, -1, alpha)
{
    setListener(this);
}

Page::Page(int zOrder, int width, int height, float alpha)
     : Window(zOrder, -1, -1, width, height, alpha)
{
    setListener(this);
}

Page::Page(int zOrder, int posX, int posY, int width, int height, float alpha)
     : Window(zOrder, posX, posY, width, height, alpha)
{
    setListener(this);
}

Page::~Page()
{
    onDestroy();
}

void Page::onCreate(Window* window)
{
    (void)(window);

    onCreate();
}

void Page::onShow(Window* window)
{
    (void)(window);

    mFocused = true;

    onShow();

    mLastKeyCode = -1;
    InputManager::getInstance().addKeyListener(this);
}

void Page::onHide(Window* window)
{
    (void)(window);

    InputManager::getInstance().removeKeyListener(this);
    mLastKeyCode = -1;

    onHide();

    mFocused = false;
}

void Page::onCreate()
{
}

void Page::onShow()
{
}

void Page::onHide()
{
}

void Page::onDestroy()
{
}

bool Page::onKeyPressed(int keyCode)
{
    (void)keyCode;

    /* TODO Please override this */

    return false;
}

bool Page::onKeyReleased(int keyCode)
{
    (void)keyCode;

    /* TODO Please override this */

    return false;
}

bool Page::onKeyReceived(int keyCode, int state)
{
    if (!isFocused())
        return false;

    #if 1 /* W/A Drop Invalid Up KeyCode */
    if(state == KEY_PRESSED)
        mLastKeyCode = keyCode;
    else if (state == KEY_RELEASED && mLastKeyCode != keyCode)
        return false;
    #endif

    for(WidgetList::iterator it = mWidgetList.begin(); it != mWidgetList.end(); it++)
    {
        if ((*it)->isFocused() && (*it)->onKeyReceived(keyCode, state) == true)
            return true;
    }

    if(state == KEY_PRESSED)
        return onKeyPressed(keyCode);
    else if(state == KEY_RELEASED)
        return onKeyReleased(keyCode);
#if 0
    else if(state == KEY_REPEATED)
        onKeyRepeated(keyCode);
#endif
    return false;
}

