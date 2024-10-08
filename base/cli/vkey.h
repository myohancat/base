/**
 * My utils source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __VKEY_H_
#define __VKEY_H_

typedef enum
{
    VKEY_CODE_NULL,
    VKEY_CODE_ESC,
    VKEY_CODE_CHAR,
    VKEY_CODE_BACKSPACE,
    VKEY_CODE_CARRIAGE_RETURN,
    VKEY_CODE_RETURN,
    VKEY_CODE_END_OF_LINE,
    VKEY_CODE_BEGIN_OF_LINE,
    VKEY_CODE_CANCEL_TEXT,
    VKEY_CODE_ARROW_UP,
    VKEY_CODE_ARROW_DOWN,
    VKEY_CODE_ARROW_LEFT,
    VKEY_CODE_ARROW_RIGHT,
    VKEY_CODE_PAGE_UP,
    VKEY_CODE_PAGE_DOWN,
    VKEY_CODE_INSERT,
    VKEY_CODE_DELETE,
    VKEY_CODE_TAB,
    VKEY_CODE_CLEAR_LINE,
    VKEY_CODE_CLEAR_EOL,
    VKEY_CODE_CTRL_LEFT,
    VKEY_CODE_CTRL_RIGHT,
    VKEY_CODE_F1,
    VKEY_CODE_F2,
    VKEY_CODE_F3,
    VKEY_CODE_F4,
    VKEY_CODE_F5,
    VKEY_CODE_F6,
    VKEY_CODE_F7,
    VKEY_CODE_F8,
    VKEY_CODE_F9,
    VKEY_CODE_F10,
    VKEY_CODE_F11,
    VKEY_CODE_F12,

    VKEY_CODE_UNKNOWN,
} VKeyCode_e;

typedef struct VKey_s
{
    VKeyCode_e  mCode;
    char        mValue;
}VKey_t;

/**
  * RETURN VALUE
  *   1  : vkey received
  *   0  : timeout
  *   -1 : error
  */
int ReadVKey(int nFd, VKey_t* pVKey);

#endif /* __VKEY_H_ */
