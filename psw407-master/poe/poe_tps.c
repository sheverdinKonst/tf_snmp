/* описание функций для texas tps 2384
 *
 *
 *
 *
 */



#if 1//USE_TPS

#include "poe_tps.h"
#include "i2c_hard.h"
#include "board.h"
#include "poe_ltc.h"
#include "task.h"
#include "settings.h"

#include "stm32f4xx_gpio.h"

void poe_alt_ab_init(void){
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_9);
}

void set_poe_alt_ab(u8 state){
	//DEBUG_MSG(POE_DBG,"set_poe_alt_ab %d\r\n",state);
	if(state == 1)
		GPIO_SetBits(GPIOB, GPIO_Pin_9);
	else
		GPIO_ResetBits(GPIOB, GPIO_Pin_9);
}


//#ifndef PSW2G_PLUS
/* возвращает значение тока текущего через порт в мА */
uint16_t PoECurrentRead(uint8_t Port){
uint16_t Current=0;
//I2C_Configuration();

switch (Port){
		case 0:
			Current= (I2c_ReadByte(Address2384,0x58)<<8) | I2c_ReadByte(Address2384,0x50);
			break;

		case 1:
			Current=(I2c_ReadByte(Address2384,0x59)<<8) | I2c_ReadByte(Address2384,0x51);
			break;

		case 2:
			Current=(I2c_ReadByte(Address2384,0x5A)<<8) | I2c_ReadByte(Address2384,0x52);
			break;
		case 3:
			Current=(I2c_ReadByte(Address2384,0x5B)<<8) | I2c_ReadByte(Address2384,0x53);
			break;

	}
Current &= 0x7FFF;
Current = (u16)((float)Current*100/3641);
return Current;
}

/* возвращает значение напряжения на порту в mV */
uint16_t PoEVoltageRead(uint8_t Port){
uint16_t Voltage=0;
//uint8_t tmp[3];
	switch (Port){
	case 0:
		Voltage=(I2c_ReadByte(Address2384,0x48)<<8) | I2c_ReadByte(Address2384,0x40);
		break;
	case 1:
		Voltage=(I2c_ReadByte(Address2384,0x49)<<8) | I2c_ReadByte(Address2384,0x41);
		break;
	case 2:
		Voltage=(I2c_ReadByte(Address2384,0x4A)<<8) | I2c_ReadByte(Address2384,0x42);
		break;
	case 3:
		Voltage=(I2c_ReadByte(Address2384,0x4B)<<8) | I2c_ReadByte(Address2384,0x43);
		break;
	}
Voltage &= 0x7FFF;
Voltage = (u16)((float)Voltage*100000/35063);
return Voltage;
}

/* возвращает состояние порта PoE контроллера */
uint8_t PoEStateRead(uint8_t Port){
uint8_t State=0;
//I2C_Configuration();
switch (Port){
case 0:
	State=I2c_ReadByte(Address2384,0x28);
	break;
case 1:
	State=I2c_ReadByte(Address2384,0x29);
	break;
case 2:
	State=I2c_ReadByte(Address2384,0x2A);
	break;
case 3:
	State=I2c_ReadByte(Address2384,0x2B);
	break;
}
State &=0x07;
return State;
}

#endif
