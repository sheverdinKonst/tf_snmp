
#ifndef INC_I2C_H
#define INC_I2C_H

#include <stdint.h>

#define I2C_CH1  0
#define I2C_CH2  1
#define I2C1_addr 0x21
#define DS2484_ADDR 0x30

#define I2C_TIMEOUT        ((uint32_t)100000)

void I2C_Configuration(void);
int8_t I2c_WriteByteData(uint8_t BusAddr,uint8_t RegAddr, uint8_t Data);
uint8_t I2c_ReadByte(uint8_t BusAddr,uint8_t RegAddr);


#endif
