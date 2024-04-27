//#include <stdlib.h>
//#include <stdio.h>
#include <string.h>

#include "stm32f4xx_spi.h"
#include "spidrv.h"
#include "spiflash.h"
#include "../deffines.h"
#include "debug.h"


struct spi_flash sf;
#if 0
/*extern */u8 ENET_TEMP[256];
/*extern */u16  TftpSeqNum;
/*extern */uint16_t LedPeriod;
/*extern */uint16_t Firmware_len;//длина прошивки
/*extern const */unsigned long image_version[1];
#endif

u8 spi_flash_probe(void)
{
   u8 idcode[5],cmd, *s;

   spi_drv_init();

   cmd = CMD_READ_ID;
   spi_drv_cmd(&cmd,1,NULL,5);
   while (spi_drv_isbusy());
   s=spi_drv_result(1);
   memcpy (idcode,s,5);
   memset (&sf,0,sizeof(sf));
   if (spansion_probe (idcode, &sf)){
	  DEBUG_MSG(BB_DBG,"spansion_probe OK\r\n");
      return 0;
   }
   if (macronyx_probe (idcode, &sf))
   {
	   DEBUG_MSG(BB_DBG,"macronyx_probe OK\r\n");
       return 0;
   }


   DEBUG_MSG(BB_DBG,"spi_flash_probe FALSE\r\n");
   return (1);
}

i8 spi_flash_properties(u32 *size, u16 *page_size, u32 *erase_size)
{
   if (!sf.params.page_size)
      return (1);
   if (size)
      *size = sf.size;
   if (page_size)
      *page_size = sf.params.page_size;
   if (erase_size)
      *erase_size = (u32)sf.params.page_size * (sf.params.page_erasable?1:sf.params.pages_per_sector);
   return 0;
}

u8 spi_flash_manufacture(void){
	return sf.manufacture;
}


i8 spi_flash_read(u32 offset,u16 len, void *buf)
{
   if (!sf.params.page_size)
      return (1);
   return (sf.read(&sf,offset,len,buf));
}

i8 spi_flash_write(u32 offset,u16 len, void *buf)
{
   if (!sf.params.page_size)
      return (1);
   return (sf.write(&sf,offset,len,buf));
}

i8 spi_flash_erase(u32 offset,u32 len)
{
   if (!sf.params.page_size)
      return (1);
   return (sf.erase(&sf,offset,len));
}



void TFTP2Flash(u32 *buff,u32 offset,u16 len){
	if((FL_TMP_START+offset+len)<FL_TMP_END)
		spi_flash_write(FL_TMP_START+offset, len, buff);
}

//void void TFTP2Flash_Vers(uint16_t len,uint8_t  *buffer){
