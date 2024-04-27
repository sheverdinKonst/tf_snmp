#include "../deffines.h"
#include <stdio.h>
#include <string.h>
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "debug.h"

void mii_init(void){
	GPIO_InitTypeDef  GPIO_InitStructure;

	//MDC - out
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//MDIO - out
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static void mdc(u8 state){
	if(state)
		GPIO_SetBits(GPIOC, GPIO_Pin_1);
	else
		GPIO_ResetBits(GPIOC, GPIO_Pin_1);
}


static void mdio(u8 state){
	if(state)
		GPIO_SetBits(GPIOA, GPIO_Pin_2);
	else
		GPIO_ResetBits(GPIOA, GPIO_Pin_2);
}

static void mdio_tristate(void){
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static void mdio_active(void){
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static u8 mdio_read(void){
	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_2))
		return 1;
	else
		return 0;
}

static void delay(u32 time){
	vTaskDelay(time*MSEC);
}


//set data lines if mdc in higth
static void send_bit(u8 bit){
	mdc(1);
	if(bit)
		mdio(1);
	else
		mdio(0);
	delay(5);
	mdc(0);
	delay(5);
}

//set data lines if mdc in higth
static u8 read_bit(void){
u8 ret = 0;
	mdc(1);
	delay(5);
	mdc(0);
	ret = mdio_read();
	delay(5);
	return ret;
}


u16 mii_read(u8 phyaddr,u8 reg){
	u16 data=0;
	u8 addr=0;
	mdio(0);
	mdc(0);
	mdio_active();
	DEBUG_MSG(DEBUG_QD,"send preamble\r\n");
	//send preabble // 32 bits
	for(u8 i=0;i<32;i++){
		send_bit(1);
	}
	DEBUG_MSG(DEBUG_QD,"send start\r\n");
	//send start
	send_bit(0);
	send_bit(1);

	//send opcode // READ
	send_bit(1);
	send_bit(0);

	//phy addr
	addr = phyaddr;
	for(u8 i=0;i<5;i++){
		if(addr & 0x10)
			send_bit(1);
		else
			send_bit(0);
		addr = addr << 1;
	}

	//reg addr
	addr = reg;
	for(u8 i=0;i<5;i++){
		if(addr & 0x10)
			send_bit(1);
		else
			send_bit(0);
		addr = addr << 1;
	}

	//ta
	mdio_tristate();
	if(read_bit()!=0){
		DEBUG_MSG(DEBUG_QD,"no\r\n");
		//return;
	}
	if(read_bit()!=0){
		DEBUG_MSG(DEBUG_QD,"no ta\r\n");
		return -1;
	}

	for(u8 j = 0; j < 16; j++){
		data <<= 1;
		if (read_bit()==Bit_RESET) {data &=0xFFFE;} else data |= 1;
	}
	DEBUG_MSG(DEBUG_QD,"read addr 0x%x reg 0x%x = %X\r\n",phyaddr,reg,data);
	return 0;
}

u8 mii_write(u8 phyaddr,u8 reg,u16 data){
	u8 addr=0;
	u16 rawdata;
	mdio(0);
	mdc(0);
	mdio_active();
	DEBUG_MSG(DEBUG_QD,"send preamble\r\n");
	//send preabble // 32 bits
	for(u8 i=0;i<32;i++){
		send_bit(1);
	}
	DEBUG_MSG(DEBUG_QD,"send start\r\n");
	//send start
	send_bit(0);
	send_bit(1);

	//send opcode // write
	send_bit(0);
	send_bit(1);

	//phy addr
	addr = phyaddr;
	for(u8 i=0;i<5;i++){
		if(addr & 0x10)
			send_bit(1);
		else
			send_bit(0);
		addr = addr << 1;
	}
	//reg addr
	addr = reg;
	for(u8 i=0;i<5;i++){
		if(addr & 0x10)
			send_bit(1);
		else
			send_bit(0);
		addr = addr << 1;
	}
	//ta
	send_bit(1);
	send_bit(0);

	//send data
	rawdata = data;
	for(u8 j = 0; j < 16; j++){
		if(rawdata & 0x800)
			send_bit(1);
		else
			send_bit(0);
		rawdata <<= 1;
	}
	mdio_tristate();
	DEBUG_MSG(DEBUG_QD,"write addr 0x%x reg 0x%x = %X\r\n",phyaddr,reg,data);
	return 0;
}

