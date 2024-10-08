/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __TOUCH_LISTENER_H_
#define __TOUCH_LISTENER_H_ 

enum TouchState_e
{
    TOUCH_ON,
    TOUCH_OFF,
    
    TOUCH_STATE_UNKNOWN /* DO NOT MODIFY */
};
    
class ITouchListener
{
public:
    virtual ~ITouchListener() { }

    virtual bool onTouched(int state, int posX, int posY) = 0;
};

#endif /* __TOUCH_LISTENER_H_ */
