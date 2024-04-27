//#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "stm32f4xx_spi.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "FreeRTOS.h"
//#include "bbdefines.h"
#include "spidrv.h"


#define SPI_BUFFER_SIZE (128)
static volatile u8  SpiBuffer[SPI_BUFFER_SIZE+1];  //my
static volatile u16 cp,ep,isBusy/*бит занятости*/;
#define SPI_CS_HIGH()    GPIO_SetBits(GPIOA, GPIO_Pin_4)    //my SPI_SSOutputCmd(SPI3, DISABLE)
#define SPI_CS_LOW()     GPIO_ResetBits(GPIOA, GPIO_Pin_4)  //my SPI_SSOutputCmd(SPI3, ENABLE)  


static void Spi_LowLevel_Init (void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_APB2PeriphClockCmd( RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOA, ENABLE);
  /*SPI Periph clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
  

	
    GPIO_PinAFConfig(GPIOC,GPIO_PinSource10,GPIO_AF_SPI3);
    GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_SPI3);
    GPIO_PinAFConfig(GPIOC,GPIO_PinSource12,GPIO_AF_SPI3);

   	/* Configure PC10, SPI3_SCK as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = 	GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
   	/* Configure PC11, SPI3_MISO function input pull-up */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = 	GPIO_Mode_AF;
	GPIO_Init(GPIOC, &GPIO_InitStructure);	
	
   	/* Configure PC12, SPI3_MOSI function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);	



   	/* Configure PA4, SPI3_NSS function push-pull --- soft implementation*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
}

static void SpiMasterInit(void){

  SPI_InitTypeDef  SPI_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  Spi_LowLevel_Init();
    
  /*!< Deselect the FLASH: Chip Select high */
  SPI_CS_HIGH();


  /*!< SPI configuration */
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;

  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(SPI3, &SPI_InitStructure);
  
  /*!< Enable  */
  SPI_Cmd(SPI3, ENABLE);

  // прерывания
  NVIC_InitStructure.NVIC_IRQChannel = SPI3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_KERNEL_INTERRUPT_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

  NVIC_Init( &NVIC_InitStructure );
  isBusy=0;
}

static void spi_run (u8 op, u16 len)
{
  //SpiBuffer[0]=op;
  cp=0;
  ep=len;
  while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE) == SET)
      SPI_I2S_ReceiveData(SPI3);

  isBusy=1;

  SPI_CS_LOW();

  SPI_I2S_SendData(SPI3, op);
  SPI_I2S_ITConfig(SPI3,SPI_I2S_IT_RXNE,ENABLE);
}

void SPI3_IRQHandler (void)
{
  if (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE) == SET)
     SpiBuffer[cp++] = SPI_I2S_ReceiveData(SPI3);//записать в  SpiBuffer[cp] SPI3

  if (cp>=ep){// если конец передачи(?)
     SPI_I2S_ITConfig(SPI3,SPI_I2S_IT_RXNE,DISABLE);
     SPI_CS_HIGH();
     isBusy = 0;
     //FlashEn=SpiBuffer[5]; //my
  }else{
    if (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) == SET)
      SPI_I2S_SendData(SPI3, SpiBuffer[cp]);//записать в SPI3 SpiBuffer[cp]
  }
}

void spi_drv_init(void)
{
  SpiMasterInit();                  
}

u8 spi_drv_xfer(u8 op, u8 end)// приём??//в данный момент не используется
{
  u16 data;
  SPI_CS_LOW();

  /*!< Loop while DR register in not empty */
  while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) == RESET);
  SPI_I2S_SendData(SPI3, op);
  /*!< Wait to receive a byte */
  while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE) == RESET);
  data = SPI_I2S_ReceiveData(SPI3);
 if (end)
   SPI_CS_HIGH();

  return data;
}

u16 spi_drv_cmd_maxlen(void)
{
  return (SPI_BUFFER_SIZE);
}

void spi_drv_cmd(u8 *cmd, u16 cmd_len, const u8 *buf, u16 buf_len)
{
  if (cmd_len>1)
     memcpy ((void*)&SpiBuffer,(void*)cmd,cmd_len);
  if (buf && buf_len)
     memcpy ((void*)&SpiBuffer[cmd_len],buf,buf_len);
  else
     memset ((void*)&SpiBuffer[cmd_len],0,buf_len);
  spi_run(*cmd,cmd_len+buf_len);
}

u8 spi_drv_isbusy(void)// возвращаем бит занятости
{
  return (isBusy);
}

u8 *spi_drv_result(u8 offset)// возвращаем зн. буфера
{
  return (void*)&SpiBuffer[offset];
}


void SPI_I2S_SendData(SPI_TypeDef* SPIx, uint16_t Data);
uint16_t SPI_I2S_ReceiveData(SPI_TypeDef* SPIx);
