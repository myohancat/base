/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __EEPROM_H_
#define __EEPROM_H_

#include "I2C.h"

class EEPROM
{
public:
    EEPROM();
    ~EEPROM();

    int getSize();

    bool writeByte(uint16_t memAddr, uint8_t val);
    bool readByte(uint16_t memAddr, uint8_t* val);

    bool writeBytes(uint16_t memAddr, void* buf, int len);
    bool readBytes(uint16_t memAddr, void* buf, int len);

protected:
    std::shared_ptr<I2C> mI2C;
};


#endif /* __EEPROM_H_ */
