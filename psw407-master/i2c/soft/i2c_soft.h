#ifndef __INC_I2C_H
#define __INC_I2C_H

#include "stm32f4xx.h"

#define SFP1 101
#define SFP2 102
#define IRP	 103
#define PLC	 104
#define SFP_NONE 0xFF

#define IRP_ADDR 0x02
#define PLC_ADDR 0x04

#define SFP_ADDR 0x50



void I2C_SCL_ACTIVE(void);
void I2C_SCL_TRISTATE(void);

void I2C_SCL(uint8_t x);
void i2c_init (void);
int i2c_probe(unsigned char _addr);
int  i2c_read(unsigned char chip, unsigned char _addr, int alen, unsigned char *buffer, int len);
int  i2c_write(unsigned char chip, unsigned int _addr, int alen, unsigned char *buffer, int len);
unsigned char i2c_reg_read(unsigned char i2c_addr, unsigned char reg);
u8 i2c_reg_write(unsigned char i2c_addr, unsigned char reg, unsigned char *val);
void SetI2CMode(uint8_t Mode);
u8 GetI2CMode(void);
uint8_t read_I2C(uint8_t chip);
uint8_t write_I2C(uint8_t chip,uint8_t data);
int write_byte(unsigned char data);
uint8_t read_byte_reg(uint8_t chip,uint8_t reg);
float read_float_reg(uint8_t chip,uint8_t reg);
uint8_t write_float_reg(uint8_t chip,uint8_t reg,float data);
uint8_t write_byte_reg(uint8_t chip,uint8_t reg,uint8_t data);
int i2c_buf_read(unsigned char i2c_addr, unsigned char reg, unsigned char *val, u8 len);
void i2c_int_config(void);



#endif //__INC_I2C_H
