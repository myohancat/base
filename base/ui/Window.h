/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __WINDOW_H_
#define __WINDOW_H_

#include <list>

#include "EventQueue.h"

#include "RenderService.h"
#include "WindowRenderer.h"
#include "Canvas.h"
#include "Color.h"
#include "Rectangle.h"
#include "Image.h"
#include "Font.h"

#include "Widget.h"
#include "Animation.h"

class Window;
class IWidget;

class IWindowListener
{
public:
    virtual ~IWindowListener() { };

    virtual void onCreate(Window* window) = 0;
    virtual void onShow(Window* window) = 0;
    virtual void onHide(Window* window) = 0;
    //virtual void onDestroy(Window* window) = 0;
};

typedef enum
{
    CONTAINER_REQUEST_CREATE,
    CONTAINER_REQUEST_SHOW,
    CONTAINER_REQUEST_SHOW_NO_ANI,
    CONTAINER_REQUEST_HIDE,
    CONTAINER_REQUEST_HIDE_NO_ANI,
    CONTAINER_REQUEST_FOCUS,

    CONTAINER_REQUEST_MESSAGE,

    MAX_CONTAINER_EVENT
} WindowEvent_e;


class Window : public IRenderable, Animation::IAnimationListener
{
public:
    Window(int zorder, int width, int height, float alpha=1.0f);
    Window(int zorder, int x, int y, int width, int height, float alpha=1.0f);
    virtual ~Window();

    void addWidget(IWidget* widget);
    void removeWidget(IWidget* widget);

    void setListener(IWindowListener* listener);

    virtual int getScreenWidth();
    virtual int getScreenHeight();

    int getX() const;
    int getY() const;
    int getWidth() const;
    int getHeight() const;

    void setPosition(int x, int y);
    void setBackground(const Color& color, bool isUpdate = false);
    void setBackground(const Image& img, bool isUpdate = false);
    void redrawBackground(const Rectangle& rect, bool isUpdate = false);

    void drawLine(const Point& p0, const Point& p1, const Color& color, float stokeWidth = 1.0f);
    void drawRectangle(const Rectangle& rect, const Color& color);
    void drawCircle(const Point& center, int radius, const Color& color);

    void drawImage(Image* image, int x = -1, int y = -1, Rectangle* res = NULL);
    void drawImageStretch(Image* img, Rectangle& dest, Rectangle* res = NULL);
    void drawText(const Font& font, const std::string& text, int x, int y, const Color& color=Color::WHITE, Rectangle* res = NULL);

    void setAlpha(float alpha);
    float getAlpha() const;

    void setScale(float scale);

    void setScaleWidth(float scale);
    void setScaleHeight(float scale);
    float getScaleWidth() const;
    float getScaleHeight() const;

    void clear();
    void clear(const Rectangle& rect);

    void redrawWidgets();

    void update();
    void update(const Rectangle& rect);

    void requestFocus();

    void post(const std::function<void()> &func);
    void sendMessage(int id, void* param = NULL);

    virtual void setFocus(bool focus);
    virtual bool isFocused() const;
    virtual void onFocusChanged(bool focus);

    virtual bool isVisible();
    virtual void show(bool disableAni = false);
    virtual void hide(bool disableAni = false);
    virtual void onMessageReceived(int id, void* param);

    void setZOrder(int zorder);
    int  getZOrder();
    void onSurfaceCreated(int screenWidth, int screenHeight);
    bool isNeedToDraw();
    void onDrawFrame();
    void onSurfaceRemoved();

    int   getDmaBufFd() const;
    void* getPixels() const;

protected:
    IWindowListener* mWindowListener = NULL;
    WindowRenderer*  mRenderer = NULL;

    int         mZorder;
    float       mAlpha;
    int         mX;
    int         mY;
    int         mWidth;
    int         mHeight;
    bool        mHiding;
    bool        mVisible;
    bool        mFocused;
    bool        mUpdate;

    Canvas*    mCanvas  = NULL;
    Image*     mBgImg   = NULL;
    Color*     mBgColor = NULL;

    float       mScaleWidth  = 1.0f;
    float       mScaleHeight = 1.0f;

    typedef std::list<IWidget*> WidgetList;
    WidgetList mWidgetList;

protected:
    Mutex   mRendererLock;
    CondVar mCondDestroy;

    Mutex   mUpdateLock;

    Canvas* getCanvas() const;

protected:
    virtual Animation* enterAnimation();
    virtual Animation* exitAnimation();

    bool mDisableAni = false;
    Animation* mDefaultAniEnter = NULL;
    Animation* mDefaultAniExit  = NULL;
    void onAnimationStarted();
    void onAnimationEnded();

public:
    class WindowEventQueue : public EventQueue, IEventHandler
    {
    public:
        WindowEventQueue();
        virtual ~WindowEventQueue();

        void post(int eventID, Window* window);
        void postMessage(Window* window, int messageId, void* param);

    private:
        void onEventReceived(int id, void* data, int dataLen);
    };

    static WindowEventQueue gWindowEventQ;

private:
    void _show(bool disableAni = false);
    void _hide(bool disableAni = false);
};

inline Canvas* Window::getCanvas() const
{
    return mCanvas;
}

inline void Window::setListener(IWindowListener* listener)
{
    mWindowListener = listener;
}

inline int Window::getScreenWidth()
{
    return 1920;
    //return RenderService::getInstance().getScreenWidth();
}

inline int Window::getScreenHeight()
{
    return 1080;
    //return RenderService::getInstance().getScreenHeight();
}

inline int Window::getX() const
{
    return mX;
}

inline int Window::getY() const
{
    return mY;
}

inline int Window::getWidth() const
{
    return mWidth;
}

inline int Window::getHeight() const
{
    return mHeight;
}

inline void Window::drawText(const Font& font, const std::string& text, int x, int y, const Color& color, Rectangle* res)
{
    Canvas* canvas = getCanvas();
    if(!canvas)
        return;

    Lock lock(canvas);
    canvas->drawText(font, text, x, y, color, res);
}

inline void Window::drawImage(Image* image, int x, int y, Rectangle* res)
{
    Canvas* canvas = getCanvas();
    if(!canvas)
        return;

    Lock lock(canvas);
    canvas->drawImage(image, x, y, res);
}

inline void Window::drawImageStretch(Image* image, Rectangle& dest, Rectangle* res)
{
    Canvas* canvas = getCanvas();
    if(!canvas)
        return;

    Lock lock(canvas);
    canvas->drawImageStretch(image, dest, res);
}

#if 0
inline void Window::drawImageStretchFixed(Image* image, Rectangle& dest, Rectangle* res)
{
    Canvas* canvas = getCanvas();
    if(!canvas)
        return;

    Lock lock(canvas);
    canvas->drawImageStretchFixed(image, dest, res);
}
#endif

inline void Window::drawRectangle(const Rectangle& rect, const Color& color)
{
    Canvas* canvas = getCanvas();
    if(!canvas)
        return;

    Lock lock(canvas);
    canvas->drawRectangle(rect, color);
}

inline void Window::drawLine(const Point& p0, const Point& p1, const Color& color, float stokeWidth)
{
    Canvas* canvas = getCanvas();
    if(!canvas)
        return;

    Lock lock(canvas);
    canvas->drawLine(p0, p1, color, stokeWidth);
}

inline void Window::drawCircle(const Point& center, int radius, const Color& color)
{
    Canvas* canvas = getCanvas();
    if(!canvas)
        return;

    Lock lock(canvas);
    canvas->drawCircle(center, radius, color);
}

inline void Window::clear()
{
    Canvas* canvas = getCanvas();
    if(!canvas)
        return;

    Lock lock(canvas);
    canvas->clear();
}

inline void Window::clear(const Rectangle& rect)
{
    Canvas* canvas = getCanvas();
    if(!canvas)
        return;

    Lock lock(canvas);
    canvas->clear(rect);
}

inline bool Window::isNeedToDraw()
{
    Lock lock(mUpdateLock);
    return mUpdate;
}

inline void Window::update()
{
    Lock lock(mUpdateLock);
    mUpdate = true;
    if (RenderService::getInstance().getRenderMode() == RENDER_MODE_WHEN_DIRTY)
        RenderService::getInstance().update();
}

inline void Window::update(const Rectangle& rect)
{
    Lock lock(mUpdateLock);

    UNUSED(rect);

    mUpdate = true;
    if (RenderService::getInstance().getRenderMode() == RENDER_MODE_WHEN_DIRTY)
        RenderService::getInstance().update();
}

inline bool Window::isFocused() const
{
    return mVisible && !mHiding && mFocused;
}

inline int Window::getDmaBufFd() const
{
    Canvas* canvas = getCanvas();
    if(!canvas)
        return -1;

    return canvas->getDmaBufFd();
}

inline void* Window::getPixels() const
{
    Canvas* canvas = getCanvas();
    if(!canvas)
        return NULL;

    return canvas->getPixels();
}

inline bool Window::isVisible()
{
    return mVisible;
}

inline void Window::setPosition(int x, int y)
{
    mX = x;
    mY = y;
}

inline void Window::setAlpha(float alpha)
{
    mAlpha = alpha;
}

inline float Window::getAlpha() const
{
    return mAlpha;
}

inline void Window::setScale(float scale)
{
    mScaleWidth = mScaleHeight = scale;
}

inline void Window::setScaleWidth(float scale)
{
    mScaleWidth = scale;
}

inline void Window::setScaleHeight(float scale)
{
    mScaleHeight = scale;
}

inline float Window::getScaleWidth() const
{
    return mScaleWidth;
}

inline float Window::getScaleHeight() const
{
    return mScaleHeight;
}
#endif // __WINDOW_H_
