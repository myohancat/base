/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "EEPROM.h"

#include <unistd.h>
#include "Log.h"

#define I2C_BUS         (6)
#define I2C_SLAVE_ADDR  (0x50)

#if 1 
#define EEPROM_SIZE     (64 * 1024)
#define MAX_PAGE_SIZE   (32)
#else
#define EEPROM_SIZE     (256 * 1024)
#define MAX_PAGE_SIZE   (64)
#endif

EEPROM::EEPROM()
{
    mI2C = I2C::open(I2C_BUS);
}

EEPROM::~EEPROM()
{
    mI2C.reset();
}

int EEPROM::getSize()
{
    return EEPROM_SIZE;
}

bool EEPROM::writeByte(uint16_t memAddr, uint8_t val)
{
    if (memAddr > EEPROM_SIZE - 1)
    {
        LOGE("Invalid address : 0x%04x", memAddr);
        return false;
    }

    if (mI2C->write(I2C_SLAVE_ADDR, memAddr, &val, 1) == 0)
        return true;

    return false;
}

bool EEPROM::readByte(uint16_t memAddr, uint8_t* val)
{
    if (memAddr > EEPROM_SIZE - 1)
    {
        LOGE("Invalid address : 0x%04x", memAddr);
        return false;
    }

    if (mI2C->read(I2C_SLAVE_ADDR, memAddr, val, 1) == 0)
        return true;

    return false;
}

bool EEPROM::writeBytes(uint16_t memAddr, void* buf, int len)
{
    int wtotal = 0;
    int wsize  = 0;
    
    if (memAddr + len > EEPROM_SIZE)
    {
        LOGE("data is too big to write");
        return false;
    }

    int startSize = ((memAddr / MAX_PAGE_SIZE + 1) * MAX_PAGE_SIZE) - memAddr;
    int pageCnt = ((len - 1) / MAX_PAGE_SIZE) + 1;

    for (int ii = 0; ii < pageCnt; ii++)
    {
        if (ii == 0)
            wsize = (len > startSize) ? startSize : len;
        else
            wsize = (len > MAX_PAGE_SIZE) ? MAX_PAGE_SIZE : len;

        if (mI2C->write(I2C_SLAVE_ADDR, (uint16_t)(memAddr + wtotal), (uint8_t*)buf + wtotal, wsize))
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

    if (memAddr + len > EEPROM_SIZE)
    {
        LOGE("data is too big to read");
        return false;
    }

    while(len > 0)
    {
        int rsize = (len > MAX_PAGE_SIZE)? MAX_PAGE_SIZE: len;

        if (mI2C->read(I2C_SLAVE_ADDR, (uint16_t)(memAddr + rtotal), (uint8_t*)buf + rtotal, rsize))
            break;

        rtotal += rsize;
        len -= rsize;
    }

    if (len == 0)
        return true;

    return false;
}


