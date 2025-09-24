/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "EEPROM.h"

#include <unistd.h>
#include "Log.h"

EEPROM::EEPROM(int i2cNum, uint8_t slaveAddr, int totalSize, int pageSize)
       : mSlaveAddr(slaveAddr),
         mTotalSize(totalSize),
         mPageSize(pageSize)
{
    mI2C = I2C::open(i2cNum);
}

EEPROM::~EEPROM()
{
    mI2C.reset();
}

int EEPROM::getSize()
{
    return mTotalSize;
}

bool EEPROM::writeByte(uint16_t memAddr, uint8_t val)
{
    if (memAddr > mTotalSize - 1)
    {
        LOGE("Invalid address : 0x%04x", memAddr);
        return false;
    }

    if (mI2C->write(mSlaveAddr, memAddr, &val, 1) == 0)
        return true;

    return false;
}

bool EEPROM::readByte(uint16_t memAddr, uint8_t* val)
{
    if (memAddr > mTotalSize - 1)
    {
        LOGE("Invalid address : 0x%04x", memAddr);
        return false;
    }

    if (mI2C->read(mSlaveAddr, memAddr, val, 1) == 0)
        return true;

    return false;
}

bool EEPROM::writeBytes(uint16_t memAddr, void* buf, int len)
{
    int wtotal = 0;
    int wsize  = 0;

    if (memAddr + len > mTotalSize)
    {
        LOGE("data is too big to write");
        return false;
    }

    int startSize = ((memAddr / mPageSize + 1) * mPageSize) - memAddr;
    int pageCnt = ((len - 1) / mPageSize) + 1;

    for (int ii = 0; ii < pageCnt; ii++)
    {
        if (ii == 0)
            wsize = (len > startSize) ? startSize : len;
        else
            wsize = (len > mPageSize) ? mPageSize : len;

        if (mI2C->write(mSlaveAddr, (uint16_t)(memAddr + wtotal), (uint8_t*)buf + wtotal, wsize))
            break;

        usleep(10*1000);
        wtotal += wsize;
        len -= wsize;
    }

    if (len == 0)
        return true;

    return false;
}

bool EEPROM::readBytes(uint16_t memAddr, void* buf, int len)
{
    int rtotal = 0;

    if (memAddr + len > mTotalSize)
    {
        LOGE("data is too big to read");
        return false;
    }

    while(len > 0)
    {
        int rsize = (len > mPageSize)? mPageSize: len;

        if (mI2C->read(mSlaveAddr, (uint16_t)(memAddr + rtotal), (uint8_t*)buf + rtotal, rsize))
            break;

        rtotal += rsize;
        len -= rsize;
    }

    if (len == 0)
        return true;

    return false;
}


