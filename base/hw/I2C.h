/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __I2C_H_
#define __I2C_H_

#include "Mutex.h"

#include <memory>
#include <map>
#include <string>

class I2C
{
public:
    static std::shared_ptr<I2C> open(int busNum);
    virtual ~I2C();

    int read(uint8_t slaveAddr, uint8_t* data, uint16_t len);
    int read(uint8_t slaveAddr, uint8_t regAddr, uint8_t* data, uint16_t len);
    int read(uint8_t slaveAddr, uint16_t regAddr, uint8_t* data, uint16_t len);

    int write(uint8_t slaveAddr, const uint8_t *data, uint16_t len);
    int write(uint8_t slaveAddr, uint8_t regAddr, uint8_t *data, uint16_t len);
    int write(uint8_t slaveAddr, uint16_t regAddr, uint8_t *data, uint16_t len);

    int readByte(uint8_t slaveAddr, uint8_t  regAddr, uint8_t* data);
    int readByte(uint8_t slaveAddr, uint16_t regAddr, uint8_t* data);
    int readWord(uint8_t slaveAddr, uint8_t  regAddr, uint16_t* data);
    int readWord(uint8_t slaveAddr, uint16_t regAddr, uint16_t* data);

    int writeByte(uint8_t slaveAddr, uint8_t  regAddr, uint8_t data);
    int writeByte(uint8_t slaveAddr, uint16_t regAddr, uint8_t data);
    int writeWord(uint8_t slaveAddr, uint8_t  regAddr, uint16_t data);
    int writeWord(uint8_t slaveAddr, uint16_t regAddr, uint16_t data);

private:
    I2C(int busNum);

private:
    bool isOpened() const;

private:
    Mutex       mLock;
    std::string mPath;
    int         mFD;
};

inline bool I2C::isOpened() const { return mFD != -1; }

inline int I2C::readByte(uint8_t slaveAddr, uint8_t  regAddr, uint8_t* data)
{
    return read(slaveAddr, regAddr, data, 1);
}

inline int I2C::readByte(uint8_t slaveAddr, uint16_t regAddr, uint8_t* data)
{
    return read(slaveAddr, regAddr, data, 1);
}

inline int I2C::readWord(uint8_t slaveAddr, uint8_t  regAddr, uint16_t* data)
{
    uint8_t v[2];
    int ret;

    if ((ret = read(slaveAddr, regAddr, v, 2)))
        return ret;

    *data = (v[0] << 8) | v[1];
    return ret;
}

inline int I2C::readWord(uint8_t slaveAddr, uint16_t regAddr, uint16_t* data)
{
    uint8_t v[2];
    int ret;

    if ((ret = read(slaveAddr, regAddr, v, 2)))
        return ret;

    *data = (v[0] << 8) | v[1];
    return ret;
}

inline int I2C::writeByte(uint8_t slaveAddr, uint8_t  regAddr, uint8_t data)
{
    return write(slaveAddr, regAddr, &data, 1);
}

inline int I2C::writeByte(uint8_t slaveAddr, uint16_t regAddr, uint8_t data)
{
    return write(slaveAddr, regAddr, &data, 1);
}

inline int I2C::writeWord(uint8_t slaveAddr, uint8_t  regAddr, uint16_t data)
{
    uint8_t v[2];

    v[0] = (data >> 8) & 0xFF;
    v[1] = data & 0xFF;

    return write(slaveAddr, regAddr, v, 2);
}

inline int I2C::writeWord(uint8_t slaveAddr, uint16_t regAddr, uint16_t data)
{
    uint8_t v[2];

    v[0] = (data >> 8) & 0xFF;
    v[1] = data & 0xFF;

    return write(slaveAddr, regAddr, v, 2);
}

#endif /* __I2C_H_ */
