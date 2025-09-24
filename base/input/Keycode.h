/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __KEY_CODE_
#define __KEY_CODE_

enum InputDeviceId_e
{
    INPUT_DEVICE_UNKNOWN = -1,

    INPUT_DEVICE_FOOT_PEDAL,
    INPUT_DEVICE_XIAOMI_BT_RCU,
    INPUT_DEVICE_AMLOGIC_BT_RCU,

    /* TODO. ADD HERE */

    MAX_INPUT_DEVICE /* DO NOT MODIFY */
};

enum KeyCode_e
{
    KEY_CODE_UNKNOWN      = -1,

    KEY_CODE_HOME         = 3,
    KEY_CODE_BACK         = 4, /* 이전 */

    KEY_CODE_0            = 7,
    KEY_CODE_1            = 8,
    KEY_CODE_2            = 9,
    KEY_CODE_3            = 10,
    KEY_CODE_4            = 11,
    KEY_CODE_5            = 12,
    KEY_CODE_6            = 13,
    KEY_CODE_7            = 14,
    KEY_CODE_8            = 15,
    KEY_CODE_9            = 16,

    KEY_CODE_DPAD_UP      = 19,
    KEY_CODE_DPAD_DOWN    = 20,
    KEY_CODE_DPAD_LEFT    = 21,
    KEY_CODE_DPAD_RIGHT   = 22,
    KEY_CODE_DPAD_CENTER  = 23,

    KEY_CODE_VOLUME_UP    = 24,
    KEY_CODE_VOLUME_DOWN  = 25,

    KEY_CODE_POWER        = 26,

    KEY_CODE_MIC          = 63,

    KEY_CODE_ENTER        = 66,

    KEY_CODE_MENU         = 82, /* 옵션 */

    KEY_CODE_MEDIA_PLAY   = 126,

    KEY_CODE_MEDIA_RECORD = 130,

    KEY_CODE_ZOOM_IN      = 168,
    KEY_CODE_ZOOM_OUT     = 169,

    KEY_CODE_CAMERA_ZOOM_IN    = 200,
    KEY_CODE_CAMERA_ZOOM_OUT   = 201,

    KEY_CODE_CAMERA_FOCUS_UP   = 202,
    KEY_CODE_CAMERA_FOCUS_DOWN = 203,

    KEY_CODE_CAMERA_MOVE_LEFT  = 204,
    KEY_CODE_CAMERA_MOVE_RIGHT = 205,

	KEY_CODE_PAIRING = 206,

    KEY_CODE_MOUSE_LEFT  = 272,
    KEY_CODE_MOUSE_RIGHT = 273,

    // TODO ADD KEY_CODE HERE

};

#endif /* __KEY_CODE_ */
