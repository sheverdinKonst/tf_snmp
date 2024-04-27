#include <string.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "stm32f4xx_adc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "selftest.h"
#include "board.h"
#include "settings.h"
#include "blbx.h"
#include "../deffines.h"
#include "debug.h"

extern u8 init_ok;
struct alarm_list_t alarm_list[ALARM_REC_NUM];

/*вывод сообщений об хардварных авариях */
/*пример */

void add_alarm(u8 alarm_code,u8 state){
char text[128];
	//del
	if(state == 0){
		for(u8 i=0;i<ALARM_REC_NUM;i++){
			if(alarm_list[i].alarm_code == alarm_code){
				alarm_list[i].alarm_code = 0;
				return;
			}
		}
	}
	//add
	if(state == 1){
		init_ok = 0;
		for(u8 i=0;i<ALARM_REC_NUM;i++){
			//ищем свободное место
			if(!alarm_list[i].alarm_code){
				alarm_list[i].alarm_code = alarm_code;
				//пишем в лог
				printf_alarm(alarm_code,text);
				DEBUG_MSG(PRINTF_DEB,"ERROR: %d\r\n",alarm_code);
				//write_log_bb(text,7);
				return;
			}
		}
	}
}


int printf_alarm(u8 num,char *text){
	if(alarm_list[num].alarm_code == 0)
		return 0;

	switch(alarm_list[num].alarm_code){
		case 0:strcpy(text,"No errors");break;
		case ERROR_WRITE_BB: strcpy(text,"Error write in BB log");break;
		case ERROR_POE_INIT: strcpy(text,"Error PoE Identifire");break;
		case DEFAULT_TAST_NOSTART:strcpy(text,"Deafult Task not started");break;
		case ERROR_INIT_BB:strcpy(text,"Error init BB");break;
		case ERROR_I2C:strcpy(text,"Error read I2C");break;
		case ERROR_CREATE_IGMP_QUEUE:strcpy(text,"Error create IGMP queue");break;
		case ERROR_CREATE_IGMP_TASK:strcpy(text,"Error create IGMP task");break;
		case ERROR_CREATE_SMTP_QUEUE:strcpy(text,"Error create SMTP queue");break;
		case ERROR_CREATE_SMTP_TASK:strcpy(text,"Error create SMTP task");break;
		case ERROR_CREATE_SNMP_QUEUE:strcpy(text,"Error create SNMP queue");break;
		case ERROR_CREATE_SNTP_TASK:strcpy(text,"Error create SNTP task");break;
		case ERROR_CREATE_STP_QUEUE:strcpy(text,"Error create STP/RSTP queue");break;
		case ERROR_CREATE_STP_NOSEND:strcpy(text,"Error send STP queue");break;
		case ERROR_CREATE_SYSLOG_QUEUE:strcpy(text,"Error create SYSLOG Queue");break;
		case ERROR_CREATE_AUTORESTART_TASK:strcpy(text,"Error create Autorestart task");break;
		case ERROR_CREATE_POE_TASK:strcpy(text,"Error create PoEControl task");break;
		case ERROR_CREATE_SS_TASK:strcpy(text,"Error create SoftStart task");break;
		case ERROR_CREATE_POECAM_TASK:strcpy(text,"Error create PoECamCtrl task");break;
		case ERROR_CREATE_OTHER_TASK:strcpy(text,"Error create OtherCommands task");break;
		case ERROR_CREATE_UIP_TASK:strcpy(text,"Error create UIP task");break;
		case ERROR_MARVEL_START: strcpy(text,"Error Marvell starting");break;
		case ERROR_CREATE_SHELL_TASK:strcpy(text,"Error create usb_shell task");break;
		case ERROR_MARVEL_PHY0: strcpy(text,"Error PHY0");break;
		case ERROR_MARVEL_PHY1: strcpy(text,"Error PHY1");break;
		case ERROR_MARVEL_PHY2: strcpy(text,"Error PHY2");break;
		case ERROR_MARVEL_PHY3: strcpy(text,"Error PHY3");break;
		case ERROR_MARVEL_FREEZE: strcpy(text,"Error Marvell freeze");break;
		default:sprintf(text,"error: %d",alarm_list[num].alarm_code);
	}
	return alarm_list[num].alarm_code;
}

u8 get_alarm_num(void){
u8 temp=0;
	for(u8 i=0;i<ALARM_REC_NUM;i++){
		if(alarm_list[i].alarm_code){
			temp++;
		}
	}
	return temp;
}


//для измерения напряжения в контрольных точках

//напряжение в контрольных точках

//PSW2G+
//PC0 - 1.2 // ADC12_IN10
//PA6 - 1.5	// ADC12_IN6
//PA5 - 2.5 // ADC12_IN5

//PSW2G4F - PSW2G6F+
//PA6 - 1.2 // ADC12_IN5
//PA5 - 1.5	// ADC12_IN6
//PC0 - 2.5 // ADC12_IN10

//PSW1G
//PA5 - 1.0	// ADC12_IN5
//PC0 - 1.8 // ADC12_IN10

//SWU16
//PC0 - 1.8//IN10
//PA5 - 1.0//IN5
//PA6 - 3.3 - sfp//IN6
void ADC_test_init(void){

GPIO_InitTypeDef GPIO_InitStructure;
ADC_InitTypeDef  ADC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	//PC0
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);


	//PA5
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	if((get_dev_type()!= DEV_PSW1G4F)&&(get_dev_type() != DEV_PSW1G4FUPS)){
		//PA6
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
	}



	ADC_StructInit(&ADC_InitStructure);

	ADC_Init(ADC1,&ADC_InitStructure);

	ADC_DiscModeChannelCountConfig(ADC1,1);
	ADC_DiscModeCmd(ADC1,ENABLE);

	/* Enable ADC1 */
	ADC_Cmd(ADC1, ENABLE);
}



u16 readADC1(u8 channel){
	if(channel == 16)
		ADC_TempSensorVrefintCmd(ENABLE);

	ADC_RegularChannelConfig(ADC1,channel,1,ADC_SampleTime_480Cycles);//установить номер канала
	ADC_SoftwareStartConv(ADC1);//запустить преобразование в регулярном канале
	while(!ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC)){}//ждать окончания имерения
	return ADC_GetConversionValue(ADC1);//прочитать результат измерения
}

#define VOLTAGE_1V0_MIN		1116
#define VOLTAGE_1V0_MAX		1364

#define VOLTAGE_1V2_MIN		1340
#define VOLTAGE_1V2_MAX		1637

#define VOLTAGE_1V5_MIN		1674
#define VOLTAGE_1V5_MAX		2047

#define VOLTAGE_1V8_MIN		2009
#define VOLTAGE_1V8_MAX		2456

#define VOLTAGE_2V5_MIN 	2791
#define VOLTAGE_2V5_MAX 	3412

#define VOLTAGE_3V3_MIN		3685
#define VOLTAGE_3V3_MAX		4504

//проверка напряжения, отклонение в 10 % считаем нормальным
#define CHECK_VOLTAGE(x,min,max) ((x<min) || (x>max))

/*запуск самотесттирования*/
u8 start_selftest(void){
//проверяем уровни напряжений
	if(get_dev_type()==DEV_PSW2GPLUS || get_dev_type()==DEV_PSW2G2FPLUSUPS || get_dev_type()==DEV_PSW2G2FPLUS){
		if(CHECK_VOLTAGE(readADC1(10),VOLTAGE_1V2_MIN,VOLTAGE_1V2_MAX))
			return 1;
		if(CHECK_VOLTAGE(readADC1(6),VOLTAGE_1V5_MIN,VOLTAGE_1V5_MAX))
			return 1;
		if(CHECK_VOLTAGE(readADC1(5),VOLTAGE_2V5_MIN,VOLTAGE_2V5_MAX))
			return 1;
	}
	if((get_dev_type()==DEV_PSW2G4F)||(get_dev_type() == DEV_PSW2G4FUPS)||(get_dev_type()==DEV_PSW2G6F)||(get_dev_type() == DEV_PSW2G6F)){
		if(CHECK_VOLTAGE(readADC1(6),VOLTAGE_1V2_MIN,VOLTAGE_1V2_MAX))
			return 1;
		if(CHECK_VOLTAGE(readADC1(5),VOLTAGE_1V5_MIN,VOLTAGE_1V5_MAX))
			return 1;
		if(CHECK_VOLTAGE(readADC1(10),VOLTAGE_2V5_MIN,VOLTAGE_2V5_MAX))
			return 1;
	}
	if((get_dev_type()==DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS) || (get_dev_type()==DEV_SWU16)){
		if(CHECK_VOLTAGE(readADC1(5),VOLTAGE_1V0_MIN,VOLTAGE_1V0_MAX))
			return 1;
		if(CHECK_VOLTAGE(readADC1(10),VOLTAGE_1V8_MIN,VOLTAGE_1V8_MAX))
			return 1;
	}
	if(get_dev_type()==DEV_SWU16){
		if(CHECK_VOLTAGE((readADC1(6)*2),VOLTAGE_3V3_MIN,VOLTAGE_3V3_MAX))
			return 1;
	}


	return 0;
}


