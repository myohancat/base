/**
 * My utils source code
 *
 * Author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "vkey.h"

#include <ctype.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <string.h>
#include <unistd.h>

#define TIMEOUT     (200) // 200ms
#define CRLF        "\r\n"
#define ASCII_ESC   '\x1B'

#define INIT_VKEY(sVKey)    do{                                 \
                                sVKey.mCode  = VKEY_CODE_UNKNOWN; \
                                sVKey.mValue = ' ';             \
                            }while(0)

typedef struct KeyBind_s
{
    VKeyCode_e    mCode;
    const char*   mString;
}KeyBind_t;

static const KeyBind_t CONV_TABLE_ASCII[] =
{
    { VKEY_CODE_BACKSPACE,       "\b"   },
    { VKEY_CODE_CARRIAGE_RETURN, "\r"   },
    { VKEY_CODE_RETURN,          "\n"   },
    { VKEY_CODE_TAB,             "\t"   },
    { VKEY_CODE_NULL,            "\x00" }, 
    { VKEY_CODE_BEGIN_OF_LINE,   "\x01" },
    { VKEY_CODE_ARROW_LEFT,      "\x02" },
    { VKEY_CODE_CANCEL_TEXT,     "\x03" },
    { VKEY_CODE_END_OF_LINE,     "\x04" },
    { VKEY_CODE_ARROW_RIGHT,     "\x06" },
    { VKEY_CODE_BACKSPACE,       "\x08" },
    { VKEY_CODE_END_OF_LINE,     "\x05" },
    { VKEY_CODE_CLEAR_EOL,       "\x0B" },
    { VKEY_CODE_ARROW_DOWN,      "\x0E" },
    { VKEY_CODE_ARROW_UP,        "\x10" },
    { VKEY_CODE_CLEAR_LINE,      "\x15" },
    { VKEY_CODE_BACKSPACE,       "\x7F" },
};

static const KeyBind_t CONV_TABLE_ESC_SEQ[] =
{
    { VKEY_CODE_BEGIN_OF_LINE, "\x1B[1~"   },
    { VKEY_CODE_INSERT,        "\x1B[2~"   },
    { VKEY_CODE_DELETE,        "\x1B[3~"   },
    { VKEY_CODE_END_OF_LINE,   "\x1B[4~"   },
    { VKEY_CODE_PAGE_UP,       "\x1B[5~"   },
    { VKEY_CODE_PAGE_DOWN,     "\x1B[6~"   },
    { VKEY_CODE_BEGIN_OF_LINE, "\x1B[7~"   },
    { VKEY_CODE_END_OF_LINE,   "\x1B[8~"   },
    { VKEY_CODE_CTRL_LEFT,     "\x1B[1;5D" },
    { VKEY_CODE_CTRL_RIGHT,    "\x1B[1;5C" },
    { VKEY_CODE_ARROW_UP,      "\x1B[A"    },
    { VKEY_CODE_ARROW_DOWN,    "\x1B[B"    },
    { VKEY_CODE_ARROW_RIGHT,   "\x1B[C"    },
    { VKEY_CODE_ARROW_LEFT,    "\x1B[D"    },
    { VKEY_CODE_END_OF_LINE,   "\x1B[F"    },
    { VKEY_CODE_BEGIN_OF_LINE, "\x1B[H"    },
    { VKEY_CODE_F1,            "\x1B[[A"   },
    { VKEY_CODE_F2,            "\x1B[[B"   },
    { VKEY_CODE_F3,            "\x1B[[C"   },
    { VKEY_CODE_F4,            "\x1B[[D"   },
    { VKEY_CODE_F5,            "\x1B[[E"   },
    { VKEY_CODE_F1,            "\x1BOP"    },
    { VKEY_CODE_F2,            "\x1BOQ"    },
    { VKEY_CODE_F3,            "\x1BOR"    },
    { VKEY_CODE_F4,            "\x1BOS"    },
    { VKEY_CODE_F5,            "\x1B[15~"  },
    { VKEY_CODE_F6,            "\x1B[17~"  },
    { VKEY_CODE_F7,            "\x1B[18~"  },
    { VKEY_CODE_F8,            "\x1B[19~"  },
    { VKEY_CODE_F9,            "\x1B[20~"  },
    { VKEY_CODE_F10,           "\x1B[21~"  },
    { VKEY_CODE_F11,           "\x1B[23~"  },
    { VKEY_CODE_F12,           "\x1B[24~"  },
};

static int _ReadKey(int nFd, char* pKey)
{
    unsigned int    nReadChar = -1;
    struct pollfd   sPollFd;
    int ret;

    sPollFd.fd = nFd;
    sPollFd.events = POLLPRI|POLLIN|POLLERR|POLLHUP|POLLNVAL|POLLRDHUP;
    sPollFd.revents = 0;

    ret = poll(&sPollFd, 1, TIMEOUT);
    if (ret > 0)
    {
        if(sPollFd.revents & (POLLRDHUP | POLLERR | POLLHUP | POLLNVAL))
            return -1;

        if (sPollFd.revents & POLLIN)
        {
            nReadChar = read(nFd, pKey, 1);
            if(nReadChar < 1)
                return -1;
        }
    }
    else if (ret == 0)
    {
        // TIMEOUT
        return 0;
    }

    return 1;
}

static int _GetVKeyFromASCII(VKey_t* pVKey, char cIn)
{
    unsigned int ii;

    for(ii = 0; ii < sizeof(CONV_TABLE_ASCII)/sizeof(KeyBind_t); ii++)
    {
        if(CONV_TABLE_ASCII[ii].mString[0] == cIn)
        {
            pVKey->mCode  = CONV_TABLE_ASCII[ii].mCode;
            pVKey->mValue = ' ';
            return 1;
        }
    }

    return 0;
}

static int _GetVKeyFromEscSeq(int nFd, VKey_t* pVKey)
{
    unsigned char abEscSeqBuf[8];
    char          cIn;
    int           nIdx = 0;
    int           ret;

    abEscSeqBuf[nIdx++] = ASCII_ESC;
    abEscSeqBuf[nIdx]   = '\0';

    do
    {
        ret = _ReadKey(nFd, &cIn);
        if (ret < 0)
            return -1;

        /* TIMEOUT */
        if (ret == 0)
        {
            pVKey->mCode = VKEY_CODE_ESC;
            pVKey->mValue = 0;
            return 1;
        }

    }while(cIn == ASCII_ESC);

    if(cIn != '[' && cIn != 'O')
    {
        pVKey->mCode  = VKEY_CODE_CHAR;
        pVKey->mValue = cIn;

        return 1;
    }

    abEscSeqBuf[nIdx++] = cIn;
    abEscSeqBuf[nIdx]   = '\0';

    while(nIdx < 7)
    {
        unsigned int ii;
        ret = _ReadKey(nFd, &cIn);
        if (ret <= 0)
            return ret;

        abEscSeqBuf[nIdx++] = cIn;
        abEscSeqBuf[nIdx]   = '\0';

#if 0
        for(ii = 0; ii < (unsigned int)nIdx; ii++)
            printf("0x%02x, ", abEscSeqBuf[ii]);
        printf("\n");
#endif

        for(ii = 0; ii < sizeof(CONV_TABLE_ESC_SEQ)/sizeof(KeyBind_t); ii++)
        {
            if(!strcmp((char *)abEscSeqBuf, (char *)CONV_TABLE_ESC_SEQ[ii].mString))
            {
                pVKey->mCode  = CONV_TABLE_ESC_SEQ[ii].mCode;
                pVKey->mValue = ' ';
                return 1;
            }
        }
    }

    INIT_VKEY((*pVKey));

    return 0;
}

int ReadVKey(int nFd, VKey_t* pVKey)
{
    char      cIn;
    int       ret;

    ret = _ReadKey(nFd, &cIn);
    if(ret <= 0)
        return ret;

    INIT_VKEY((*pVKey));

    if(isprint(cIn))
    {
        pVKey->mCode  = VKEY_CODE_CHAR;
        pVKey->mValue = cIn;
    }
    else if(cIn != ASCII_ESC)
    {
        ret = _GetVKeyFromASCII(pVKey, cIn);
        if (ret <= 0)
            return ret;
    }
    else
    {
        ret = _GetVKeyFromEscSeq(nFd, pVKey);
        if (ret <= 0)
            return ret;
    }

    return 1;
}

