/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __MOUSE_LISTENER_H_
#define __MOUSE_LISTENER_H_

class IMouseListener
{
public:
    virtual ~IMouseListener() { }

    virtual bool onMouseReceived(int dx, int dy) = 0;
};

#endif /* __MOUSE_LISTENER_H_ */
