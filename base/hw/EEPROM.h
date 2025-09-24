/**
 * My Base Code
 * c wrapper class for developing embedded system.
 *
 * author: Kyungyin.Kim < myohancat@naver.com >
 */
#ifndef __EEPROM_H_
#define __EEPROM_H_

#include "I2C.h"

#define DEF_EEPROM_I2C_NUM    (3)
#define DEF_EEPROM_SLAVE_ADDR (0x50)
#define DEF_EEPROM_SIZE       (64*1024)
#define DEF_EEPROM_PAGE_SIZE  (128)

class EEPROM
{
public:
    EEPROM(int i2cNum = DEF_EEPROM_I2C_NUM, uint8_t slaveAddr = DEF_EEPROM_SLAVE_ADDR,
           int totalSize = DEF_EEPROM_SIZE, int pageSize = DEF_EEPROM_PAGE_SIZE);

    ~EEPROM();

    int getSize();

    bool writeByte(uint16_t memAddr, uint8_t val);
    bool readByte(uint16_t memAddr, uint8_t* val);

    bool writeBytes(uint16_t memAddr, void* buf, int len);
    bool readBytes(uint16_t memAddr, void* buf, int len);

protected:
    std::shared_ptr<I2C> mI2C;

    uint8_t mSlaveAddr;
    int     mTotalSize;
    int     mPageSize;
};


#endif /* __EEPROM_H_ */
