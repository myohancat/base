/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __KEY_LISTENER_H_
#define __KEY_LISTENER_H_

enum KeyState_e
{
    KEY_RELEASED,
    KEY_PRESSED,
    KEY_REPEATED,

    KEY_STATE_UNKNOWN /* DO NOT MODIFY */
};

class IKeyListener
{
public:
    virtual ~IKeyListener() { }

    virtual bool onKeyReceived(int keyCode, int state) = 0;
};

class IRawKeyListener
{
public:
    virtual ~IRawKeyListener() { }

    virtual void onRawKeyReceived(int deviceId, int code, int value) = 0;
};


#endif /* __KEY_LISTENER_H_ */
