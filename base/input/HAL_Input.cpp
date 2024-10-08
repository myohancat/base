/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "Keycode.h"

#include "InputManager.h"

#include <string.h>
#include <linux/uinput.h>

#define UNKNOWN_VERSION    (-1)
#define UNKNOWN_VENDOR     (-1)
#define UNKNOWN_PRODUCT    (-1)
#define UNKNOWN_NAME       (NULL)

typedef struct InputDeviceInfo_s
{
    int           mVersion;
    int           mVendor;
    int           mProduct;
    const char*   mName;

    int           mID;
} InputDeviceInfo_t;

typedef struct KeyMap_s
{
    int mRawKeyCode;
    int mKeyCode;
} KeyMap_t;

#ifdef __cplusplus
extern "C"
{
#endif

// [QUAD500 v3.0.6 E978] ver : 1, vendor : 290, product : 15
// [小米蓝牙语音遥控器] ver : 4132, vendor : 10007, product : 12976
static InputDeviceInfo_t INPUT_DEVICES[] =
{
    /* VERSION         VENDOR  PRODUCT   NAME                     DEVICE */
    { UNKNOWN_VERSION,   290,     15,     "QUAD500 v3.0.6 E978",    INPUT_DEVICE_FOOT_PEDAL     },
    { UNKNOWN_VERSION,   10007,   12976,  UNKNOWN_NAME,             INPUT_DEVICE_XIAOMI_BT_RCU  },
    { UNKNOWN_VERSION,   10007,   12984,  UNKNOWN_NAME,             INPUT_DEVICE_XIAOMI_BT_RCU  },
    { UNKNOWN_VERSION,   6421,    1,      UNKNOWN_NAME,             INPUT_DEVICE_AMLOGIC_BT_RCU },
    /* TODO. ADD DEVICE HERE */
};

int HAL_Input_GetDeviceID(int version, int vendor, int product, const char* name)
{
    for (int ii = 0; ii < NELEM(INPUT_DEVICES); ii++)
    {
        InputDeviceInfo_t* device = &INPUT_DEVICES[ii];

        if (((device->mVersion == UNKNOWN_VERSION) || (device->mVersion == version))
         && ((device->mVendor  == UNKNOWN_VENDOR)  || (device->mVendor  == vendor))
         && ((device->mProduct == UNKNOWN_PRODUCT) || (device->mProduct == product))
         && ((device->mName    == UNKNOWN_NAME)    || strncmp(name, device->mName, strlen(device->mName)) == 0))
            return device->mID;
    }

    return INPUT_DEVICE_UNKNOWN;
}

static KeyMap_t KEYMAP_DEFAULT[] =
{
    { KEY_BACK/* 158 */,       KEY_CODE_BACK }, /*이전*/
    { KEY_BACKSPACE /* 14 */,  KEY_CODE_BACK },
    { KEY_ESC /* 1 */,         KEY_CODE_BACK },

    { KEY_HOME /* 102 */,      KEY_CODE_HOME },
    { KEY_F1 /* 59 */,         KEY_CODE_HOME },
    { KEY_LEFTMETA /* 125 */,  KEY_CODE_HOME }, /*WINDODW*/

    { KEY_0 /* 11 */,          KEY_CODE_0 },
    { KEY_1 /* 2 */ ,          KEY_CODE_1 },
    { KEY_2 /* 3 */ ,          KEY_CODE_2 },
    { KEY_3 /* 4 */ ,          KEY_CODE_3 },
    { KEY_4 /* 5 */ ,          KEY_CODE_4 },
    { KEY_5 /* 6 */ ,          KEY_CODE_5 },
    { KEY_6 /* 7 */ ,          KEY_CODE_6 },
    { KEY_7 /* 8 */ ,          KEY_CODE_7 },
    { KEY_8 /* 9 */ ,          KEY_CODE_8 },
    { KEY_9 /* 10 */,          KEY_CODE_9 },

    { KEY_R /* 19 */,          KEY_CODE_MEDIA_RECORD },
    { KEY_P /* 25 */,          KEY_CODE_MEDIA_PLAY   },

    { KEY_UP    /* 103 */,     KEY_CODE_DPAD_UP     },
    { KEY_DOWN  /* 108 */,     KEY_CODE_DPAD_DOWN   },
    { KEY_LEFT  /* 105 */,     KEY_CODE_DPAD_LEFT   },
    { KEY_RIGHT /* 106 */,     KEY_CODE_DPAD_RIGHT  },
    { KEY_ENTER /* 28  */,     KEY_CODE_ENTER },

    { KEY_SLEEP/* 142 */,      KEY_CODE_POWER       },

    { KEY_VOLUMEUP/* 115 */,   KEY_CODE_VOLUME_UP     },
    { KEY_VOLUMEDOWN/* 114 */, KEY_CODE_VOLUME_DOWN },

    { KEY_PAGEUP/* 104 */,     KEY_CODE_VOLUME_UP     },
    { KEY_PAGEDOWN/* 109 */,   KEY_CODE_VOLUME_DOWN },

    { KEY_EQUAL/* 13 */,       KEY_CODE_VOLUME_UP },
    { KEY_MINUS/* 12 */,       KEY_CODE_VOLUME_DOWN },

    { KEY_PHONE/* 169 */,      KEY_CODE_MENU       }, /*옵션*/
    { KEY_F12/* 88 */,         KEY_CODE_MENU       }, /*옵션*/
    { KEY_COMPOSE/* 127 */,    KEY_CODE_MENU       }, /*옵션*/

    { KEY_ZOOMIN/* 0x1a2 */,   KEY_CODE_ZOOM_IN   },
    { KEY_ZOOMOUT/* 0x1a3 */,  KEY_CODE_ZOOM_OUT   },
};

static KeyMap_t KEYMAP_FOOT_PEDAL[] =
{
    { KEY_UP    /* 103 */,     KEY_CODE_DPAD_UP     },
    { KEY_DOWN  /* 108 */,     KEY_CODE_DPAD_DOWN   },
    { KEY_LEFT  /* 105 */,     KEY_CODE_DPAD_LEFT   },
    { KEY_RIGHT /* 106 */,     KEY_CODE_DPAD_RIGHT  },
    { KEY_ENTER /* 28  */,     KEY_CODE_ENTER       },
};

static KeyMap_t KEYMAP_XIAOMI_BT_RCU[] =
{
    { 116,                     KEY_CODE_POWER       },

    { KEY_UP    /* 103 */,     KEY_CODE_DPAD_UP     },
    { KEY_DOWN  /* 108 */,     KEY_CODE_DPAD_DOWN   },
    { KEY_LEFT  /* 105 */,     KEY_CODE_DPAD_LEFT   },
    { KEY_RIGHT /* 106 */,     KEY_CODE_DPAD_RIGHT  },
    { KEY_ENTER /* 28  */,     KEY_CODE_ENTER       },

    { 63,                      KEY_CODE_MIC         },

    { KEY_VOLUMEUP/* 115 */,   KEY_CODE_VOLUME_UP   },
    { KEY_VOLUMEDOWN/* 114 */, KEY_CODE_VOLUME_DOWN },

    { 102,                     KEY_CODE_HOME       },
    { KEY_BACK/* 158 */,       KEY_CODE_BACK       },
    { 127,                     KEY_CODE_MENU       },
};

static KeyMap_t KEYMAP_AMLOGIC_BT_RCU[] =
{
    { 116,                     KEY_CODE_POWER       },

    { KEY_UP    /* 103 */,     KEY_CODE_DPAD_UP     },
    { KEY_DOWN  /* 108 */,     KEY_CODE_DPAD_DOWN   },
    { KEY_LEFT  /* 105 */,     KEY_CODE_DPAD_LEFT   },
    { KEY_RIGHT /* 106 */,     KEY_CODE_DPAD_RIGHT  },
    { 353,                     KEY_CODE_ENTER       },

    { KEY_VOLUMEUP/* 115 */,   KEY_CODE_VOLUME_UP   },
    { KEY_VOLUMEDOWN/* 114 */, KEY_CODE_VOLUME_DOWN },

    { 172,                     KEY_CODE_HOME       },
    { KEY_BACK/* 158 */,       KEY_CODE_BACK       },
    { 127,                     KEY_CODE_MENU       },
};

int HAL_Input_GetKeyCode(int deviceId, int rawKeyCode)
{
    KeyMap_t* customKeyMap     = NULL;
    int       customKeyMapSize = 0;

    switch(deviceId)
    {
        case INPUT_DEVICE_FOOT_PEDAL:
            customKeyMap      = KEYMAP_FOOT_PEDAL;
            customKeyMapSize  = NELEM(KEYMAP_FOOT_PEDAL);
            break;
        case INPUT_DEVICE_XIAOMI_BT_RCU:
            customKeyMap      = KEYMAP_XIAOMI_BT_RCU;
            customKeyMapSize  = NELEM(KEYMAP_XIAOMI_BT_RCU);
            break;
        case INPUT_DEVICE_AMLOGIC_BT_RCU:
            customKeyMap      = KEYMAP_AMLOGIC_BT_RCU;
            customKeyMapSize  = NELEM(KEYMAP_AMLOGIC_BT_RCU);
            break;
        case INPUT_DEVICE_UNKNOWN:
        default:
            customKeyMap = NULL;
            customKeyMapSize = 0;
            break;
    }

    /* 1. Convert Key code by Custom Key Map Table */
    if (customKeyMap != NULL)
    {
        for (int ii = 0; ii < customKeyMapSize; ii++)
        {
            if (customKeyMap[ii].mRawKeyCode == rawKeyCode)
                return customKeyMap[ii].mKeyCode;
        }
    }

    /* 2. Convert Key code by Default Key Map Table */
    for (int ii =0; ii < NELEM(KEYMAP_DEFAULT); ii++)
    {
        if (KEYMAP_DEFAULT[ii].mRawKeyCode == rawKeyCode)
            return KEYMAP_DEFAULT[ii].mKeyCode;
    }

    return KEY_CODE_UNKNOWN;
}

#ifdef __cplusplus
} // extern "C"
#endif
