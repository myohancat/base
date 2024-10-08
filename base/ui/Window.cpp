/**
 * My simple ui framework source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "Window.h"

#include <unistd.h>
#include <algorithm>
#include "RenderService.h"
#include "Log.h"

struct WindowMessage
{
    Window* mWindow;
    int     mMessageId;
    void*   mParam;

    WindowMessage(Window* window, int messageId, void* param)
        : mWindow(window), mMessageId(messageId), mParam(param) { }
};

Window::WindowEventQueue Window::gWindowEventQ;

Window::WindowEventQueue::WindowEventQueue()
{
    setHandler(this);
}

Window::WindowEventQueue::~WindowEventQueue()
{
}

void Window::WindowEventQueue::post(int eventID, Window* window)
{
    sendEvent(eventID, (uintptr_t)window);
}

void Window::WindowEventQueue::postMessage(Window* window, int messageId, void* param)
{
    WindowMessage msg(window, messageId, param);

    sendEvent(CONTAINER_REQUEST_MESSAGE, &msg, sizeof(msg));
}

void Window::WindowEventQueue::onEventReceived(int id, void* data, int dataLen)
{
    UNUSED(dataLen);

    switch(id)
    {
        case CONTAINER_REQUEST_SHOW:
        {
            Window* window = (Window*)UINTPTR_TO_PTR(data);
            if (window != NULL)
                window->_show();
            break;
        }
        case CONTAINER_REQUEST_HIDE:
        {
            Window* window = (Window*)UINTPTR_TO_PTR(data);
            if (window != NULL)
                window->_hide(false);
            break;
        }
        case CONTAINER_REQUEST_HIDE_NO_ANI:
        {
            Window* window = (Window*)UINTPTR_TO_PTR(data);
            if (window != NULL)
                window->_hide(true);
            break;
        }
        case CONTAINER_REQUEST_FOCUS:
        {
            Window* window = (Window*)UINTPTR_TO_PTR(data);
            if (window != NULL)
                window->setFocus(true);
            break;
        }
        case CONTAINER_REQUEST_MESSAGE:
        {
            WindowMessage* msg = (WindowMessage*)data;
            
            if (msg->mWindow != NULL)
                msg->mWindow->onMessageReceived(msg->mMessageId, msg->mParam);
            break;
        }
        default:
            LOGE("Invalid Event ID : %d", id);
            break;
    }
}

Window::Window(int zOrder, int width, int height, float alpha)
          : mWindowListener(NULL),
            mRenderer(NULL),
            mZorder(zOrder),
            mAlpha(alpha),
            mBgImg(NULL),
            mBgColor(NULL)
{
    int screenWidth = RenderService::getInstance().getScreenWidth();
    int screenHeight = RenderService::getInstance().getScreenHeight();

    mX = (screenWidth - width) / 2;
    mY = (screenHeight- height) / 2;

    if(width < 0)
        mWidth = screenWidth;
    else
        mWidth = width;

    if(height < 0)
        mHeight= screenHeight;
    else
        mHeight = height;

    mFocused = false;
    mHiding  = false;
    mVisible = false;
    mUpdate = false;
}

Window::Window(int zOrder, int x, int y, int width, int height, float alpha)
          : mWindowListener(NULL),
            mRenderer(NULL),
            mZorder(zOrder),
            mAlpha(alpha),
            mX(x),
            mY(y),
            mBgImg(NULL),
            mBgColor(NULL)
{
    int screenWidth = RenderService::getInstance().getScreenWidth();
    int screenHeight = RenderService::getInstance().getScreenHeight();

    if(width < 0)
        mWidth = screenWidth;
    else
        mWidth = width;

    if(height < 0)
        mHeight= screenHeight;
    else
        mHeight = height;

    if (mX < 0)
    {
        mX = (screenWidth - mWidth) / 2;
        if (mX < 0)
            mX = 0;
    }

    if (mY < 0)
    {
        mY = (screenHeight - mHeight) / 2;
        if (mY < 0)
            mY = 0;
    }

    mFocused = false;
    mHiding  = false;
    mVisible = false;
    mUpdate  = false;
}

Window::~Window()
{
    //if (mWindowListener)
    //    mWindowListener->onDestroy(this);
    RenderService::getInstance().removeRenderer(this);

    SAFE_DELETE(mBgImg);
    SAFE_DELETE(mBgColor);
    SAFE_DELETE(mCanvas);

    SAFE_DELETE(mDefaultAniEnter);
    SAFE_DELETE(mDefaultAniExit);
}


void Window::setFocus(bool focus)
{
    if (mFocused == focus)
        return;

    onFocusChanged(focus);

    mFocused = focus;
}

// Override for focus
void Window::onFocusChanged(bool focus)
{
    // NOP
    (void) focus;
}

void Window::addWidget(IWidget* widget)
{
    if (widget == NULL)
        return;

    WidgetList::iterator it = std::find(mWidgetList.begin(), mWidgetList.end(), widget);
    if(widget == *it)
        return;

    widget->setWindow(this);
    mWidgetList.push_back(widget);
}

void Window::removeWidget(IWidget* widget)
{
    if (widget == NULL)
        return;

    for(WidgetList::iterator it = mWidgetList.begin(); it != mWidgetList.end(); it++)
    {
        if(widget == *it)
        {
            mWidgetList.erase(it);
            (*it)->setWindow(NULL);
            return;
        }
    }
}

void Window::redrawWidgets()
{
    for(WidgetList::iterator it = mWidgetList.begin(); it != mWidgetList.end(); it++)
        (*it)->paint();
}

void Window::show(bool disableAni)
{
    if (disableAni)
        gWindowEventQ.post(CONTAINER_REQUEST_SHOW_NO_ANI, this);
    else
        gWindowEventQ.post(CONTAINER_REQUEST_SHOW, this);
}

void Window::hide(bool disableAni)
{
    if (disableAni)
        gWindowEventQ.post(CONTAINER_REQUEST_HIDE_NO_ANI, this);
    else
        gWindowEventQ.post(CONTAINER_REQUEST_HIDE, this);
}

void Window::requestFocus()
{
    gWindowEventQ.post(CONTAINER_REQUEST_FOCUS, this);
}

void Window::post(const std::function<void()> &func)
{
    MainLoop::getInstance().post(func);
}

void Window::sendMessage(int id, void* param)
{
    gWindowEventQ.postMessage(this, id, param);
}

void Window::onMessageReceived(int messageId, void* param)
{
    (void)messageId;
    (void)param;
}

void Window::_show(bool disableAni)
{
    if (mVisible == true)
    {
        //LOGW("already show.. skip it !");
        return;
    }

    // Lazy Creation
    if(!getCanvas())
    {
        if(mWidth > 0 && mHeight > 0)
        {
            mCanvas = new Canvas(mWidth, mHeight);
            mCanvas->lock();
            mCanvas->clear();
            mCanvas->unlock();
        }

        RenderService::getInstance().addRenderer(this);
        if (mWindowListener)
        {
            mWindowListener->onCreate(this);
        }
    }

    Animation* ani = exitAnimation();
    if (ani)
        ani->stop();
    ani = enterAnimation();
    if (disableAni)
        ani = NULL;
    if (ani)
        ani->start();

    if(mWindowListener)
        mWindowListener->onShow(this);

#if 1
    for(WidgetList::iterator it = mWidgetList.begin(); it != mWidgetList.end(); it++)
        (*it)->paint();
#endif

    mVisible = true;
    update();
}

void Window::_hide(bool disableAni)
{
    if (mVisible == false || mHiding == true)
    {
        //LOGW("already hide.. skip it !");
        return;
    }

    Animation* ani = enterAnimation();
    if (ani)
        ani->stop();
    ani = exitAnimation();
    if (disableAni)
        ani = NULL;
    if (ani)
    {
        ani->setListener(this);
        ani->start();
    }
    if(mWindowListener)
        mWindowListener->onHide(this);

    if (!ani)
        mVisible = false;

    update();
}

void Window::setBackground(const Color& color, bool isUpdate)
{
    Canvas* canvas = getCanvas();
    if (!canvas)
        return;

    Lock lock(getCanvas());

    SAFE_DELETE(mBgImg);
    SAFE_DELETE(mBgColor);

    mBgColor = new Color(color);
    canvas->clear(*mBgColor);

    if (isUpdate)
        update();
}

void Window::setBackground(const Image& img, bool isUpdate)
{
    Canvas* canvas = getCanvas();
    if (!canvas)
        return;

    Lock lock(canvas);

    SAFE_DELETE(mBgImg);
    SAFE_DELETE(mBgColor);

    mBgImg = new Image(img.getPath());
    canvas->drawImage(mBgImg, 0, 0, NULL, BLEND_MODE_SRC);

    if (isUpdate)
        update();
}

void Window::redrawBackground(const Rectangle& rect, bool isUpdate)
{
    Canvas* canvas = getCanvas();
    if (!canvas)
        return;

    Lock lock(canvas);

    if(!rect.isValid())
        return;

    canvas->clear(rect);
    if(mBgImg)
    {
        Image imgToRedraw(*mBgImg, rect);
        canvas->drawImage(&imgToRedraw, rect.getX(), rect.getY());
    }
    else if(mBgColor)
    {
        canvas->drawRectangle(rect, *mBgColor);
    }

    if (isUpdate)
        update();
}

void Window::setZOrder(int zorder)
{
    if (zorder != mZorder)
    {
        mZorder = zorder;
        RenderService::getInstance().sortRenderer();
    }
}

int Window::getZOrder()
{
    return mZorder;
}

void Window::onSurfaceCreated(int screenWidth, int screenHeight)
{
    UNUSED(screenWidth);
    UNUSED(screenHeight);

    mRenderer = new WindowRenderer(this);
    update();
}

void Window::onDrawFrame()
{
    Lock lock(mUpdateLock);

    if (!mVisible)
    {
        mUpdate = false;
        return;
    }

    if (mUpdate)
    {
        Canvas* canvas = getCanvas(); 
        Lock lock(canvas);
        mRenderer->onDraw(canvas->getPixels());
    }
    else
        mRenderer->onDraw(NULL);

    mUpdate = false;
}

void Window::onSurfaceRemoved()
{
    SAFE_DELETE(mRenderer);
}

Animation* Window::enterAnimation()
{
    if (!mDefaultAniEnter)
        mDefaultAniEnter = new AlphaAnimation(this, 0.0f, 1.0f);

    return mDefaultAniEnter;
}

Animation* Window::exitAnimation()
{
    if (!mDefaultAniExit)
        mDefaultAniExit = new AlphaAnimation(this, 1.0f, 0.0f);

    return mDefaultAniExit;
}

void Window::onAnimationStarted()
{
    mHiding = true;
}

void Window::onAnimationEnded()
{
    mHiding = false;
    mVisible = false;
}
