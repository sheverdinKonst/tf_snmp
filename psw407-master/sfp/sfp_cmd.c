#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
//#include <math.h>
#include <string.h>
#include "../deffines.h"
#include "i2c_soft.h"
#include "sfp_cmd.h"
#include "stm32f4xx_gpio.h"
#include "board.h"
#include "task.h"
#include "../net/webserver/httpd-fs.h"
#include "fw/sfp_fw.h"
#include "spiflash.h"
#include "settings.h"

GPIO_InitTypeDef GPIO_InitStructure;


void sfp_get_info(u8 port,struct sfp_state_t *sfp_state){
#if SFP_INFO
u8 i=0;
if(get_dev_type() == DEV_SWU16){
	SetI2CMode(port);
}
else{
	if(port == GE1)
		SetI2CMode(SFP1);
	else
		SetI2CMode(SFP2);
}


if(i2c_probe(0xA0)==0){
	sfp_state->state=ENABLE;
	sfp_state->identifier=read_byte_reg(0xA0,0);
	sfp_state->connector=read_byte_reg(0xA0,2);
	sfp_state->type=read_byte_reg(0xA0,6);
	sfp_state->link_len=read_byte_reg(0xA0,7) & 0xF8 ;
	sfp_state->fibre_tech=(read_byte_reg(0xA0,7) & 0x07)| (read_byte_reg(0xA0,8) & 0xF0);
	sfp_state->media=read_byte_reg(0xA0,9);
	sfp_state->speed=read_byte_reg(0xA0,10);
	sfp_state->encoding=read_byte_reg(0xA0,11);
	sfp_state->nbr = read_byte_reg(0xA0,12)*100;
	sfp_state->wavelen=(read_byte_reg(0xA0,60)<<8) | (read_byte_reg(0xA0,61));

	sfp_state->len9 = read_byte_reg(0xA0,15)*100;
	sfp_state->len50 = read_byte_reg(0xA0,16)*10;
	sfp_state->len62 = read_byte_reg(0xA0,17)*10;
	sfp_state->lenc = read_byte_reg(0xA0,18);


	for(i=0;i<16;i++){
		sfp_state->vendor[i]=(char)read_byte_reg(0xA0,20+i);
		if(sfp_state->vendor[i]==0)
			break;
	}

	for(i=0;i<3;i++){
		sfp_state->OUI[i]=(char)read_byte_reg(0xA0,37+i);
	}

	for(i=0;i<16;i++){
		sfp_state->PN[i]=(char)read_byte_reg(0xA0,40+i);
	}

	for(i=0;i<4;i++){
		sfp_state->rev[i]=(char)read_byte_reg(0xA0,56+i);
	}

	sfp_state->dm_type = read_byte_reg(0xA0,92);

	if(sfp_state->dm_type){
		sfp_state->dm_temper = (i16)((read_byte_reg(0xA2,96)<<8) | (read_byte_reg(0xA2,97)));
		sfp_state->dm_voltage = (u16)((read_byte_reg(0xA2,98)<<8) | (read_byte_reg(0xA2,99)));
		sfp_state->dm_current = (u16)((read_byte_reg(0xA2,100)<<8) | (read_byte_reg(0xA2,101)));
		//sfp_state->dm_outbias = (u16)((read_byte_reg(0xA2,98)<<8) | (read_byte_reg(0xA2,99)));
		sfp_state->dm_txpwr = (u16)((read_byte_reg(0xA2,102)<<8) | (read_byte_reg(0xA2,103)));
		sfp_state->dm_rxpwr = (u16)((read_byte_reg(0xA2,104)<<8) | (read_byte_reg(0xA2,105)));
	}
}
else{
	sfp_state->state=DISABLE;
	sfp_state->identifier=0xFF;
	sfp_state->connector=0xFF;
	sfp_state->type=0xFF;
	sfp_state->link_len=0xFF;
	sfp_state->fibre_tech=0xFF;
	sfp_state->media=0xFF;
	sfp_state->speed=0xFF;
	sfp_state->encoding=0xFF;
	sfp_state->wavelen=0xFF;
	sfp_state->dm_type = 0xFF;
	sfp_state->dm_temper = 0;
	sfp_state->dm_voltage = 0;
}
#endif
}


void sfp_set_addr(u8 addr){
	if(get_dev_type()==DEV_SWU16){
		//0
		if (addr & 0x01)
			GPIO_SetBits(GPIOD, GPIO_Pin_0);
		else
			GPIO_ResetBits(GPIOD, GPIO_Pin_0);
		//1
		if (addr & 0x02)
			GPIO_SetBits(GPIOD, GPIO_Pin_1);
		else
			GPIO_ResetBits(GPIOD, GPIO_Pin_1);
		//2
		if (addr & 0x04)
			GPIO_SetBits(GPIOD, GPIO_Pin_2);
		else
			GPIO_ResetBits(GPIOD, GPIO_Pin_2);
		//3
		if (addr & 0x08)
			GPIO_SetBits(GPIOD, GPIO_Pin_3);
		else
			GPIO_ResetBits(GPIOD, GPIO_Pin_3);
		vTaskDelay(10*MSEC);
	}
}

u8 get_sfp_present(u8 port){
	if(get_dev_type()==DEV_SWU16){
		sfp_set_addr(port);
		if (GPIO_ReadInputDataBit(GPIOD,SFP1_PRESENT)==Bit_RESET)
			return 1;
		else
			return 0;
	}
	else{
		//psw only
		if(port == GE1){
			if (GPIO_ReadInputDataBit(GPIOD,SFP1_PRESENT)==Bit_RESET)
				return 1;
			else
				return 0;
		} else if (port == GE2){
			if (GPIO_ReadInputDataBit(GPIOD,SFP2_PRESENT)==Bit_RESET)
				return 1;
			else
				return 0;
		} else
			return -1;
	}
}


//Los / SD
u8 get_sfp_los(u8 port){
	if(get_dev_type()==DEV_SWU16){
		sfp_set_addr(port);
		return GPIO_ReadInputDataBit(GPIOD,SFP_LOS_SWU);
	}
	else{
	//psw only
		if(port == GE1){
			return GPIO_ReadInputDataBit(GPIOD,SW_SD1);
		} else if (port == GE2){
			return GPIO_ReadInputDataBit(GPIOD,SW_SD2);
		}
	}
	return 0;
}


void sfp_set_line_init(void){
	//SFP_SET line - SWU16 only
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	//R/W select - для SWU
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOD, GPIO_Pin_9);
}

u8 get_sfp_set(void){
	if (GPIO_ReadInputDataBit(GPIOD,GPIO_Pin_12)==Bit_RESET)
		return 0;
	else
		return 1;
}

void sfp_set_write(u8 state){
	if(state)
		GPIO_SetBits(GPIOD, GPIO_Pin_9);
	else
		GPIO_ResetBits(GPIOD, GPIO_Pin_9);
}

void sfp_los_line_init(void){
	//SFP_LOS line - SWU16 only
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
}





//перепрошивка eeprom модулей
u8  sfp_ee_write(u8 addr,u8 *buff){
	SetI2CMode(addr);

	if(i2c_probe(0xA0)){
		return 1;
	}

	for(u8 i=0;i<128;i++){
		//printf("%d:%02X\r\n",i,buff[i]);
		if(write_byte_reg(0xA0,i,buff[i])){
			return 1;
		}
	}
	return 0;
}

u8 sfp_reprog(u8 port,u8 index){
	//printf("sfp_reprog %d %d\r\n",port,index);
	struct httpd_fsdata_file_noconst *f;
	u8 i;
	  for(f = (struct httpd_fsdata_file_noconst *)SFP_FW_FS_ROOT,i=0;
			  f != NULL;
		  	  f = (struct httpd_fsdata_file_noconst *)f->next,i++) {

		  //printf("sfp %d, %s\r\n",f->len,f->name);

		  if(i==index){
			  if(sfp_ee_write(port,(u8 *)f->data))
				 return 1;
			  else
				 return 0;
		  }
	  }
	  return 1;
}

//обновление SFP загруженным файлом
u8 sfp_reprog_file(u8 addr){
u8 tmp[128];
	spi_flash_read(0,128,tmp);
    if(sfp_ee_write(addr,tmp))
	    return 1;
    else
	    return 0;
}

u8 get_sfp_fw_num(void){
	return SFP_FW_FS_NUMFILES;
}

//получить имя файла прошивки
u8 get_sfp_fw_name(u8 index,char *name){
	struct httpd_fsdata_file_noconst *f;
	u8 i;
	for(f = (struct httpd_fsdata_file_noconst *)SFP_FW_FS_ROOT,i=0;
		  f != NULL;
		  f = (struct httpd_fsdata_file_noconst *)f->next,i++) {
	  if(i==index){
		  strcpy(name,f->name);
		  return 0;
	  }
	}
	return 1;
}
