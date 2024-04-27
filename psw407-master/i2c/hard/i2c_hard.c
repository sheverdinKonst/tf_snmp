#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stm32f4xx_i2c.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "../deffines.h"
#include "i2c_hard.h"
#include "selftest.h"

#define ReadAddress            0x21
#define WriteAddress         0x20
#define GetHeading            0x00// registr addres
#define ClockSpeed              5000

#define I2C_SPEED               30000
#define I2C_SLAVE_ADDRESS7      0xA0

void I2C_Configuration(void)
{
           I2C_InitTypeDef  I2C_InitStructure;
           GPIO_InitTypeDef  GPIO_InitStructure;

           // use in RCC_Init();
           RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1,ENABLE);
           //alternate function
           RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG , ENABLE);//

           RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);


           /* Configure I2C1 pins: PB6->SCL and PB7->SDA */
           GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7;
           GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
           GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
           GPIO_InitStructure.GPIO_OType=GPIO_OType_OD;
           GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;           // enable pull up resistors
           GPIO_Init(GPIOB, &GPIO_InitStructure);
           /* Configure alternate functions */
           GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);
           GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_I2C1);


           I2C_DeInit(I2C1);
           I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
           I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9;
           I2C_InitStructure.I2C_OwnAddress1 = 1;
           I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
           I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
           I2C_InitStructure.I2C_ClockSpeed = 1000;  /* 1kHz */

           I2C_Init(I2C1, &I2C_InitStructure);
           I2C_Cmd(I2C1, ENABLE);
           I2C_AcknowledgeConfig(I2C1, ENABLE);

}
/********************************************************************************************
 *                          I2C READ Byte
 *******************************************************************************************/
uint8_t I2c_ReadByte(uint8_t BusAddr,uint8_t RegAddr)
{
  uint32_t Timeout;
  uint8_t RegValue = 0;



  /* ждем освобождения шины */
  Timeout = I2C_TIMEOUT;
  while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
  {
    if((Timeout--) == 0) {RegValue = -1; return RegValue;}
  }

  /* посылаем СТАРТ */
  I2C_GenerateSTART(I2C1, ENABLE);

  /*!< Test on EV5 and clear it (cleared by reading SR1 then writing to DR) */
  //Timeout = I2C_FLAG_TIMEOUT;
  Timeout = I2C_TIMEOUT;
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if((Timeout--) == 0) {RegValue = -2; goto i2c_stop;}
  }

  /*!< Send EEPROM address for write */
  I2C_Send7bitAddress(I2C1, BusAddr, I2C_Direction_Transmitter);

  /*!< Test on EV6 and clear it */
  Timeout = I2C_TIMEOUT;
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))/*!< EV6 */
  {
    if((Timeout--) == 0) { RegValue = -3; goto i2c_stop;}
  }
  I2C_SendData(I2C1, RegAddr);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)); /*!< EV8 */

 /*-------------------------------- Reception Phase --------------------------*/
  I2C_GenerateSTART(I2C1, ENABLE);
  while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));  /*!< EV5 */

  I2C_Send7bitAddress(I2C1, BusAddr/*ReadAddress*/, I2C_Direction_Receiver);

  /*!< Test on EV7 and clear it */

  Timeout = I2C_TIMEOUT;
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED))
  {
    if((Timeout--) == 0) { RegValue = -4; goto i2c_stop; }
  }

  RegValue = I2C_ReceiveData(I2C1);
  if (I2C_CheckEvent(I2C1, I2C_EVENT_SLAVE_ACK_FAILURE)){
	  RegValue = -5;
  }

  I2C_AcknowledgeConfig(I2C1, DISABLE);



i2c_stop:

  I2C_GenerateSTOP(I2C1, ENABLE);

  if((RegValue == -1)||(RegValue == -2)||(RegValue == -3)||(RegValue == -4)||(RegValue == -5))
	  ADD_ALARM(ERROR_I2C);

  return (RegValue);

}
/**********************************************************************************
 * 					I2C WRITE BYTE DATA
 ********************************************************************************/
int8_t I2c_WriteByteData(uint8_t BusAddr,uint8_t RegAddr, uint8_t Data)
{
  uint32_t Timeout;
  int8_t res = 0;

  /* ждем освобождения шины */
  Timeout = I2C_TIMEOUT;
  while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
  {
    if((Timeout--) == 0) return -1;
  }

  /* посылаем СТАРТ */
  I2C_GenerateSTART(I2C1, ENABLE);

  /*!< Test on EV5 and clear it (cleared by reading SR1 then writing to DR) */
  Timeout = I2C_TIMEOUT;
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if((Timeout--) == 0) {res = -2; goto i2c_stop;}
  }

  /*!< Send EEPROM address for write */
  I2C_Send7bitAddress(I2C1, BusAddr/*WriteAddress*/, I2C_Direction_Transmitter);

  /*!< Test on EV6 and clear it */
  Timeout = I2C_TIMEOUT;
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    if((Timeout--) == 0) { res = -3; goto i2c_stop;}
  }

  /*!< Send the RESET command */
  I2C_SendData(I2C1, RegAddr);

  /*!< Test on EV8_2 and clear it */
  Timeout = I2C_TIMEOUT;
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    if((Timeout--) == 0) { res = -4; goto i2c_stop;}
  }

  if (I2C_CheckEvent(I2C1, I2C_EVENT_SLAVE_ACK_FAILURE)){
     res = -5; goto i2c_stop;

  }

  /*!< Send the RESET command */
  I2C_SendData(I2C1, Data);

  /*!< Test on EV8_2 and clear it */
  Timeout = I2C_TIMEOUT;
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    if((Timeout--) == 0) { res = -6; goto i2c_stop;}
  }

  if (I2C_CheckEvent(I2C1, I2C_EVENT_SLAVE_ACK_FAILURE)){
     res = -7;
  }

i2c_stop:

  I2C_GenerateSTOP(I2C1, ENABLE);
  vTaskDelay(5*MSEC);// нужна задержка, иначе глюки
  return (res);

}
/**********************************************************************/
void I2c_suspend(uint8_t ch)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  /*!< GPIO configuration */  
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;

  /*!< Configure SCL&SDA  */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void I2c_resume(uint8_t ch)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  /*!< GPIO configuration */  
  //GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  //GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType=GPIO_OType_OD;

  /*!< Configure SCL */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /*!< Configure SDA */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_Init(GPIOB, &GPIO_InitStructure); 
}

int8_t I2c_WriteByte(uint8_t ch, uint8_t address, uint8_t byte)
{
  uint32_t Timeout;
  int8_t res = 0;

  /* ждем освобождения шины */
  Timeout = I2C_TIMEOUT;
  while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
  {
    if((Timeout--) == 0) return -1;
  }
  
  /* посылаем СТАРТ */
  I2C_GenerateSTART(I2C1, ENABLE);
  
  /*!< Test on EV5 and clear it (cleared by reading SR1 then writing to DR) */
  Timeout = I2C_TIMEOUT;
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if((Timeout--) == 0) {res = -2; goto i2c_stop;}
  }
  
  /*!< Send EEPROM address for write */
  I2C_Send7bitAddress(I2C1, address, I2C_Direction_Transmitter);

  /*!< Test on EV6 and clear it */
  Timeout = I2C_TIMEOUT;
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    if((Timeout--) == 0) { res = -3; goto i2c_stop;}
  } 

  /*!< Send the RESET command */
  I2C_SendData(I2C1, byte);

  /*!< Test on EV8_2 and clear it */
  Timeout = I2C_TIMEOUT;
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    if((Timeout--) == 0) { res = -4; goto i2c_stop;}
  } 

  if (I2C_CheckEvent(I2C1, I2C_EVENT_SLAVE_ACK_FAILURE)){
     res = -5;
  }

i2c_stop:

  I2C_GenerateSTOP(I2C1, ENABLE);

  return (res);

}







