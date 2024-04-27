//#include <stdio.h>
#include <string.h>

#include "stm32f4xx_spi.h"
#include "spidrv.h"
#include "spiflash.h"
#include "../deffines.h"
#include "debug.h"
#include "s25fl128p.h"


#define CMD_S25FL_WREN			0x06 //Write Enable
#define CMD_S25FL_WRDI			0x04 //Write Disable
#define CMD_S25FL_RDSR			0x05 //Read Status Register
#define CMD_S25FL_SE			0xD8 //Sector Erase
#define CMD_S25FL_BE			0x60 //Bulk Erase
#define S25FL_IDCODE0			0x01
/* status register bits */
#define S25FL_STATUS_WIP		(1 << 0)//Write in Progress
#define S25FL_STATUS_WEL		(1 << 0)

#define PROGMEM


static const struct spi_flash_prop PROGMEM spansion_spi_flash_table[] = {
	{
		.idcode1		= 0x20,
		.idcode2		= 0x18,
		.idcode3		= 0x03,
		.idcode4		= 0x00,
		.l2_page_size	= 8,
		.page_size		= 256,
		.pages_per_sector	= 1024,
		.num_sectors		= 64
	},
	{
		.idcode1		= 0x20,
		.idcode2		= 0x18,
		.idcode3		= 0x03,
		.idcode4		= 0x01,
		.l2_page_size	= 8,
		.page_size		= 256,
		.pages_per_sector	= 256,
		.num_sectors		= 256
	},

	{
		.idcode1		= 0x02,
		.idcode2		= 0x15,
		.idcode3		= 0x00,
		.idcode4		= 0x00,
		.l2_page_size	= 8,
		.page_size		= 256,
		.pages_per_sector	= 256,
		.num_sectors		= 64
	},

	{
		.idcode1		= 0x02,
		.idcode2		= 0x16,
		.idcode3		= 0x00,
		.idcode4		= 0x00,
		.l2_page_size	= 8,
		.page_size		= 256,
		.pages_per_sector	= 256,
		.num_sectors		= 128
	},

	{
		.idcode1		= 0x20,
		.idcode2		= 0x18,
		.idcode3		= 0x4d,
		.idcode4		= 0x01,
		.l2_page_size	= 8,
		.page_size		= 256,
		.pages_per_sector	= 256,
		.num_sectors		= 256
	},
};

#define MX25L_IDCODE0			0xC2

static const struct spi_flash_prop PROGMEM macronyx_spi_flash_table[] =
{
    {
        .idcode1		= 0x20,
        .idcode2		= 0x18,
        .idcode3		= 0x00,
        .idcode4		= 0x00,
        .l2_page_size	= 8,
        .page_size		= 256,
        .pages_per_sector	= 256,
        .num_sectors		= 256
    },

};


void spansion_address (u8 *cmd, u16 page_addr, u8 byte_addr, struct spi_flash_params *params)
{
        cmd[1]=page_addr >> 8;
        cmd[2]=page_addr & 0xff;
        cmd[3]=byte_addr;
}

i8 spansion_read(struct spi_flash *flash, u32 offset, u16 len, void *buf){


        u8 cmd[4];
        u16 page_addr,byte_addr,l,chunk;

        page_addr = offset / flash->params.page_size;
        byte_addr = offset % flash->params.page_size;

        spi_drv_xfer(CMD_S25FL_RDSR,0);

		while ((spi_drv_xfer(0,0)&(S25FL_STATUS_WIP|S25FL_STATUS_WEL)));
		spi_drv_xfer(0,1);
        cmd[0]=CMD_READ_ARRAY;
        for (l=0;l<len;l+=chunk){
           chunk = min(len-l,spi_drv_cmd_maxlen()-4);
		   spansion_address(cmd,page_addr, byte_addr, &flash->params);
		   spi_drv_cmd(cmd,4,NULL,chunk);
		   while (spi_drv_isbusy());
		   memcpy ((u8*)buf+l,spi_drv_result(4),chunk);
		   byte_addr += chunk;
		   if (byte_addr >= flash->params.page_size){
			  byte_addr -= flash->params.page_size;
			  page_addr++;
		   }
        }
       //u16 *tmp;
       //tmp=(u16 *)buf;
       //DEBUG_MSG(BB_DBG,"read: %lu buf[0]=0x%X\r\n",offset,*tmp);
	return 0;
}

i8 spansion_write(struct spi_flash *flash, u32 offset, u16 len, void *buf)
{
        u8 cmd[4];
        u16 page_addr, byte_addr, l, chunk;

        page_addr = offset / flash->params.page_size;
        byte_addr = offset % flash->params.page_size;

        spi_drv_xfer(CMD_S25FL_RDSR,0);
        while ((spi_drv_xfer(0,0)&(S25FL_STATUS_WIP|S25FL_STATUS_WEL)));
        spi_drv_xfer(0,1);

        for (l=0; l<len; l+=chunk){

            spi_drv_xfer(CMD_S25FL_WREN,1);

            chunk = min (min (len - l, flash->params.page_size - byte_addr),spi_drv_cmd_maxlen()-4);
            cmd[0]=CMD_WRITE_PAGE;
            spansion_address(cmd, page_addr, byte_addr, &flash->params);
            spi_drv_cmd(cmd,4,((u8*)buf)+l,chunk);
            while (spi_drv_isbusy());

            byte_addr += chunk;
            if (byte_addr == flash->params.page_size){
               byte_addr = 0;
               page_addr++;
            }

            spi_drv_xfer(CMD_S25FL_RDSR,0);
            while ((spi_drv_xfer(0,0)&(S25FL_STATUS_WIP|S25FL_STATUS_WEL)));
            spi_drv_xfer(0,1);
        }
        //u16 *tmp;
        //tmp=(u16 *)buf;
        //DEBUG_MSG(BB_DBG,"write: %lu buf[0]=0x%X\r\n",offset,*tmp);
	return 0;
}

i8 spansion_erase(struct spi_flash *flash, u32 offset, u32 len)
{
        u8 cmd[4];
        u16 l;
        u32 addr,size;

        size = (u32)flash->params.page_size * flash->params.pages_per_sector;

        if ((offset % size) || (len % size)){
        	DEBUG_MSG(BB_DBG,"erase offset %lu len %lu\r\n",offset,len);
        	return -1;
        }

        addr = offset / flash->params.page_size;
        len /= size;

        spi_drv_xfer(CMD_S25FL_RDSR,0);
        while ((spi_drv_xfer(0,0)&(S25FL_STATUS_WIP|S25FL_STATUS_WEL)));
        spi_drv_xfer(0,1);

        for (l=0; l<len; l++){

            spi_drv_xfer(CMD_S25FL_WREN,1);

            cmd[0] = CMD_S25FL_SE;
            spansion_address(cmd, addr, 0, &flash->params);
            spi_drv_cmd(cmd,4,NULL,0);
            while (spi_drv_isbusy());

            addr+=flash->params.pages_per_sector;

            spi_drv_xfer(CMD_S25FL_RDSR,0);
            while ((spi_drv_xfer(0,0)&(S25FL_STATUS_WIP|S25FL_STATUS_WEL)));
            spi_drv_xfer(0,1);

        }

	return 0;
}

struct spi_flash *spansion_probe (u8 *idcode, struct spi_flash *sf){
u8 i;
struct spi_flash_prop prop;

     DEBUG_MSG(BB_DBG,"spansion_probe %x-%x-%x-%x-%x\r\n",
      		idcode[0],idcode[1],idcode[2],idcode[3],idcode[4]);


     if (idcode[0] != S25FL_IDCODE0){
        return NULL;
    }



	for (i = 0; i < ARRAY_SIZE(spansion_spi_flash_table); i++) {
		memcpy(&prop,&spansion_spi_flash_table[i],sizeof(struct spi_flash_prop));
		if (prop.idcode1 == idcode[1]&&
		    prop.idcode2 == idcode[2]&&
                    (prop.idcode1 == 0x02||
                     (prop.idcode1 == 0x20 &&
		      prop.idcode3 == idcode[3]&&
		      prop.idcode4 == idcode[4])))
		   break;
	}

	if (i == ARRAY_SIZE(spansion_spi_flash_table)){
		return NULL;
	}

    sf->params.l2_page_size = prop.l2_page_size;
	sf->params.page_erasable = 0;
	sf->params.page_size = prop.page_size;
	sf->params.pages_per_sector = prop.pages_per_sector;
	sf->params.num_sectors = prop.num_sectors;

	sf->size = (u32)prop.page_size * prop.pages_per_sector * prop.num_sectors;
	sf->read = spansion_read;
	sf->write = spansion_write;
	sf->erase = spansion_erase;
	sf->manufacture = SPANSION_FLASH;
	return (sf);
}

struct spi_flash *macronyx_probe (u8 *idcode, struct spi_flash *sf)
{
    u8 i;
    struct spi_flash_prop prop;

    if (idcode[0] != MX25L_IDCODE0)
    {
        return NULL;
    }

    for (i = 0; i < ARRAY_SIZE(macronyx_spi_flash_table); i++)
    {
        memcpy(&prop,&macronyx_spi_flash_table[i],sizeof(struct spi_flash_prop));
        if (prop.idcode1 == idcode[1]&&
            prop.idcode2 == idcode[2])
            break;
    }

    if (i == ARRAY_SIZE(macronyx_spi_flash_table))
    {
        return NULL;
    }

    sf->params.l2_page_size = prop.l2_page_size;
    sf->params.page_erasable = 0;
    sf->params.page_size = prop.page_size;
    sf->params.pages_per_sector = prop.pages_per_sector;
    sf->params.num_sectors = prop.num_sectors;

    sf->size = (u32)prop.page_size * prop.pages_per_sector * prop.num_sectors;
    sf->read = spansion_read;
    sf->write = spansion_write;
    sf->erase = spansion_erase;
    sf->manufacture = MACRONYX_FLASH;

    return (sf);
}

