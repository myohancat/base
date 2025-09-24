/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __PAGE_H_
#define __PAGE_H_

#include "Window.h"
#include "InputManager.h"

#define ZORDER_UI 500

class Page : public Window, IWindowListener, IKeyListener
{
public:
    Page(float alpha = 1.0f);
    Page(int zOrder, float alpha = 1.0f);

    Page(int width, int height, float alpha = 1.0f);
    Page(int posX, int posY, int width, int height, float alpha = 1.0f);

    Page(int zOrder, int width, int height, float alpha = 1.0f);
    Page(int zOrder, int posX, int posY, int width, int height, float alpha = 1.0f);

    virtual ~Page();

    virtual bool onProcessKey(int keyCode, int state);

    virtual bool onKeyPressed(int keyCode);
    virtual bool onKeyReleased(int keyCode);
    virtual bool onKeyRepeated(int keyCode);

protected:
    virtual void onCreate();
    virtual void onShow();
    virtual void onHide();
    virtual void onDestroy();

    void onCreate(Window* window);
    void onShow(Window* window);
    void onHide(Window* window);

private:
    bool onKeyReceived(int keyCode, int state);

private:
    int mLastKeyCode;
};

#endif // __PAGE_H_
