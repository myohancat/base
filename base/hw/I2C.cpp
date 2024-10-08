/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#include "I2C.h"

#include <string.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "Log.h"

#define MAX_BUFFER_SIZE (4096)
#define DEVICE_PREFIX "/dev/i2c-"

static std::map<int, std::shared_ptr<I2C>> gCacheDevs;

std::shared_ptr<I2C> I2C::open(int busNum)
{
    if (gCacheDevs.find(busNum) != gCacheDevs.end())
        return gCacheDevs[busNum];

    std::shared_ptr<I2C> i2c;
    I2C* dev = new I2C(busNum);
    if (dev->isOpened())
    {
        i2c.reset(dev);
        gCacheDevs[busNum] = i2c;
    }

    return i2c;
}

I2C::I2C(int busNum)
{
    char path[MAX_PATH_LEN];
    snprintf(path, MAX_PATH_LEN, "%s%d", DEVICE_PREFIX, busNum);

    mFD = ::open(path, O_RDWR);
    if (mFD < 0)
    {
        LOGE("Cannot open i2c device : %s", path);
        mFD = -1;
    }

    mPath = path;
}

I2C::~I2C()
{
    if (mFD != -1)
        close(mFD);
}

int I2C::read(uint8_t slaveAddr, uint8_t *data, uint16_t len)
{
    struct i2c_rdwr_ioctl_data ioctlData;
    struct i2c_msg msg;

    Lock lock(mLock);

    msg.addr = slaveAddr;
    msg.flags = I2C_M_RD;
    msg.len = len;
    msg.buf = data;

    ioctlData.msgs = &msg;
    ioctlData.nmsgs = 1;

    if (ioctl(mFD, I2C_RDWR, &ioctlData) < 0)
    {
        LOGE("Cannot readslave : 0x%02x error %d : %s", slaveAddr, errno, strerror(errno));
        return -1;
    }

    return 0;
}


int I2C::read(uint8_t slaveAddr, uint8_t regAddr, uint8_t *data, uint16_t len)
{
    struct i2c_rdwr_ioctl_data ioctlData;
    struct i2c_msg msg[2];

    Lock lock(mLock);
    msg[0].addr = slaveAddr;
    msg[0].flags = 0;
    msg[0].len = 1;
    msg[0].buf = &regAddr;

    msg[1].addr = slaveAddr;
    msg[1].flags = I2C_M_RD;
    msg[1].len = len;
    msg[1].buf = data;
    ioctlData.msgs = msg;
    ioctlData.nmsgs = 2;


    if (ioctl(mFD, I2C_RDWR, &ioctlData) < 0)
    {
        LOGE("Cannot read slave : 0x%02x, reg : 0x%02x error %d : %s", slaveAddr, regAddr, errno, strerror(errno));
        return -1;
    }

    return 0;
}

int I2C::read(uint8_t slaveAddr, uint16_t regAddr, uint8_t *data, uint16_t len)
{
    struct i2c_rdwr_ioctl_data ioctlData;
    struct i2c_msg msg[2];

    char buffer[2];

    Lock lock(mLock);

    buffer[0] = (regAddr >> 8 ) & 0xFF;
    buffer[1] = (regAddr ) & 0xFF;

    msg[0].addr = slaveAddr;
    msg[0].flags = 0;
    msg[0].len = 2;
    msg[0].buf = (uint8_t*)buffer;

    msg[1].addr = slaveAddr;
    msg[1].flags = I2C_M_RD;
    msg[1].len = len;
    msg[1].buf = data;
    ioctlData.msgs = msg;
    ioctlData.nmsgs = 2;

    if (ioctl(mFD, I2C_RDWR, &ioctlData) < 0)
    {
        LOGE("Cannot read slave : 0x%02x, reg : 0x%04x error %d : %s", slaveAddr, regAddr, errno, strerror(errno));
        return -1;
    }

    return 0;
}

int I2C::write(uint8_t slaveAddr, const uint8_t *data, uint16_t len)
{
    struct i2c_rdwr_ioctl_data ioctlData;
    struct i2c_msg msg;

    Lock lock(mLock);

    msg.addr = slaveAddr;
    msg.flags = 0;
    msg.len = len;
    msg.buf = (uint8_t*)data;

    ioctlData.msgs = &msg;
    ioctlData.nmsgs = 1;

    if (ioctl(mFD, I2C_RDWR, &ioctlData) < 0)
    {
        LOGE("Cannot write slave : 0x%02x error %d : %s", slaveAddr, errno, strerror(errno));
        return -1;
    }

    return 0;
}

int I2C::write(uint8_t slaveAddr, uint8_t regAddr, uint8_t *data, uint16_t len)
{
    struct i2c_rdwr_ioctl_data ioctlData;
    struct i2c_msg msg[1];
    char buffer[MAX_BUFFER_SIZE];

    Lock lock(mLock);

    buffer[0] = regAddr;
    memcpy(&buffer[1], data, len);

    msg[0].addr = slaveAddr;
    msg[0].flags = 0;
    msg[0].len = 1 + len;
    msg[0].buf = (uint8_t*)buffer;
    ioctlData.msgs = msg;
    ioctlData.nmsgs = 1;

    if (ioctl(mFD, I2C_RDWR, &ioctlData) < 0)
    {
        LOGE("Cannot write slave : 0x%02x, reg : 0x%02x error %d : %s", slaveAddr, regAddr, errno, strerror(errno));
        return -1;
    }

    return 0;
}


int I2C::write(uint8_t slaveAddr, uint16_t regAddr, uint8_t *data, uint16_t len)
{
    struct i2c_rdwr_ioctl_data ioctlData;
    struct i2c_msg msg[1];
    char buffer[MAX_BUFFER_SIZE];

    Lock lock(mLock);

    buffer[0] = (regAddr >> 8) & 0xFF;
    buffer[1] = (regAddr) & 0xFF;
    memcpy(&buffer[2], data, len);

    msg[0].addr = slaveAddr;
    msg[0].flags = 0;
    msg[0].len = 2 + len;
    msg[0].buf = (uint8_t*)buffer;
    ioctlData.msgs = msg;
    ioctlData.nmsgs = 1;

    if (ioctl(mFD, I2C_RDWR, &ioctlData) < 0)
    {
        LOGE("Cannot write slave : 0x%02x, reg : 0x%04x error %d : %s", slaveAddr, regAddr, errno, strerror(errno));
        return -1;
    }

    return 0;
}

