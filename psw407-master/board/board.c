/*
 * описание портов ввода/вывода коммутатора
 * low level definition of SW board
 *
 * generate: 23/05/2013
 *
 * обозначение типа линии:
 * LED - светодиод
 * BTN - кнопка
 * LINE - линия
 */
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_rtc.h"
#include "stm32f4xx_pwr.h"
#include "board.h"
#include "selftest.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_syscfg.h"
#include "stm32f4xx_iwdg.h"
#include "stm32f4xx_adc.h"
#include "stm32f4xx_flash.h"
#include "misc.h"
#include "poe_ltc.h"
#include "i2c_hard.h"
#include "i2c_soft.h"
#include "names.h"
#include "eeprom.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "../uip/uip.h"
#include "../stp/bridgestp.h"
#include "SMIApi.h"
#include "VLAN.h"
#include "../deffines.h"
#include "rc4.h"
#include "crc.h"
#include "spiflash.h"
#include "blbx.h"
#include "../events/events_handler.h"
#include "../snmp/snmp.h"
#include "../snmp/snmpd/snmpd-types.h"
#include "../smtp/smtp.h"
#include "../igmp/igmpv2.h"
#include "../dns/resolv.h"
#include "../tftp/tftpclient.h"
#include "../uip/timer.h"
#include "../inc/h/driver/gtDrvSwRegs.h"
#include "UPS_command.h"
#include "../webserver/httpd-cgi.h"
#include "settingsfile.h"
#include "../telnet/usb_shell.h"
#include "../command/command.h"
#include "Salsa2Regs.h"
#include "../plc/plc_def.h"
#include "../eth/stm32f4x7_eth.h"
#include "debug.h"
#include "settings.h"
#include "salsa2.h"
#include "main.h"
#include "stm32f4x7_eth.h"
#include "stm32f4x7_eth_bsp.h"




/**********************************************************************************/
/*****************************плата SW*********************************************/
/**********************************************************************************/
GPIO_InitTypeDef GPIO_InitStructure;
EXTI_InitTypeDef EXTI_InitStructure;
NVIC_InitTypeDef NVIC_InitStructure;

extern uip_ipaddr_t uip_hostaddr;

extern uint16_t need_update_flag[4];

extern const uint32_t image_version[1];

extern uint8_t dev_addr[6];

extern u8 test_processing_flag;

extern xQueueHandle UsbShellQueue;
extern xQueueHandle Salsa2Queue;



struct dev_t dev __attribute__ ((section (".ccmram")));


static u8 uip_debug_state = 0;
void uip_debug_enable(void);
void uip_debug_disable(void);



static u8 reboot_flag_=0;


u8 POE_PORT_NUM;
u8 POEB_PORT_NUM;
u8 COOPER_PORT_NUM;
u8 FIBER_PORT_NUM;
u8 CPU_PORT = 10;
u8 MV_PORT_NUM = 11;
static u8 dev_type=0;

u8 engine_id_tmp[64];

static void Reset_RTC_cnt(void);




//определение типа устройства
u8 get_dev_type(void){
	i8 poe_id = 0;
	u16 mv_id = 0;

	if(dev_type == 0){
		//get poe and marvell id`s
		mv_id = get_marvell_id();
		if(mv_id == DEV_98DX316){
			dev_type = DEV_SWU16;
			COOPER_PORT_NUM = 4;
			FIBER_PORT_NUM = 12;
			POE_PORT_NUM = 0;
			POEB_PORT_NUM = 0;
			MV_PORT_NUM = 16;
		}
		else{
			poe_id = get_poe_id();
			if(poe_id<0)
				ADD_ALARM(ERROR_POE_INIT);
		}


		if((poe_id == DEV_TPS2384)&&((mv_id == DEV_88E095)||(mv_id == DEV_88E097))){
			dev_type = DEV_PSW2G4F;
			POE_PORT_NUM = 4;
			POEB_PORT_NUM = 0;
			COOPER_PORT_NUM = 4;
			FIBER_PORT_NUM = 2;
			CPU_PORT = 10;
			MV_PORT_NUM = 11;
		}

		if((poe_id == DEV_LTC4271)&&((mv_id == DEV_88E095)||(mv_id == DEV_88E097))){
			switch(get_board_id_1()){
				case 0:
				case 4:
					dev_type = DEV_PSW2GPLUS;
					POE_PORT_NUM = 4;
					POEB_PORT_NUM = 4;
					COOPER_PORT_NUM = 4;
					FIBER_PORT_NUM = 2;
					CPU_PORT = 10;
					MV_PORT_NUM = 11;
					break;
				case 1:
				case 5:
					dev_type = DEV_PSW2G6F;
					POE_PORT_NUM = 6;
					POEB_PORT_NUM = 2;
					COOPER_PORT_NUM = 6;
					FIBER_PORT_NUM = 2;
					CPU_PORT = 10;
					MV_PORT_NUM = 11;
					break;
				case 2:
					dev_type = DEV_PSW2G2FPLUS;//v1
					POE_PORT_NUM = 1;
					POEB_PORT_NUM = 1;
					COOPER_PORT_NUM = 2;
					FIBER_PORT_NUM = 2;
					CPU_PORT = 10;
					MV_PORT_NUM = 11;
					break;
				case 3:
					dev_type = DEV_PSW2G2FPLUS;//v2
					POE_PORT_NUM = 2;
					POEB_PORT_NUM = 1;
					COOPER_PORT_NUM = 2;
					FIBER_PORT_NUM = 2;
					CPU_PORT = 10;
					MV_PORT_NUM = 11;
					break;
				case 7:
					dev_type = DEV_PSW2G8F;
					POE_PORT_NUM = 8;
					POEB_PORT_NUM = 0;
					COOPER_PORT_NUM = 8;
					FIBER_PORT_NUM = 2;
					CPU_PORT = 10;
					MV_PORT_NUM = 11;
					break;
				default:
					dev_type = DEV_PSW2GPLUS;
					POE_PORT_NUM = 4;
					POEB_PORT_NUM = 4;
					COOPER_PORT_NUM = 4;
					FIBER_PORT_NUM = 2;
					CPU_PORT = 10;
					MV_PORT_NUM = 11;
					break;
			}
		}



		if((poe_id == DEV_TPS2384)&&((mv_id == DEV_88E6176)||(mv_id == DEV_88E6240))){
			dev_type = DEV_PSW1G4F;
			POE_PORT_NUM = 4;
			POEB_PORT_NUM = 0;
			COOPER_PORT_NUM = 5;
			FIBER_PORT_NUM = 1;
			CPU_PORT = 6;
			MV_PORT_NUM = 7;
		}


	}else{
		if((dev_type == DEV_PSW1G4F)&&(is_ups_mode()==1))
			dev_type = DEV_PSW1G4FUPS;
		if((dev_type == DEV_PSW2G4F)&&(is_ups_mode()==1))
			dev_type = DEV_PSW2G4FUPS;
		if((dev_type == DEV_PSW2G2FPLUS)&&(is_ups_mode()==1))
			dev_type = DEV_PSW2G2FPLUSUPS;
	}
	return dev_type;
}

void get_dev_name(char *name){
	switch(get_dev_type()){
		case DEV_PSW2GPLUS:
			strcpy(name,"TFortis PSW-2G+");
			break;
		case DEV_PSW1G4F:
			strcpy(name,"TFortis PSW-1G4F");
			break;
		case DEV_PSW2G4F:
			strcpy(name,"TFortis PSW-2G4F");
			break;
		case DEV_PSW2G6F:
			strcpy(name,"TFortis PSW-2G6F+");
			break;
		case DEV_PSW2G8F:
			strcpy(name,"TFortis PSW-2G8F+");
			break;
		case DEV_PSW1G4FUPS:
			strcpy(name,"TFortis PSW-1G4F-UPS");
			break;
		case DEV_PSW2G4FUPS:
			strcpy(name,"TFortis PSW-2G4F-UPS");
			break;
		case DEV_PSW2G2FPLUS:
			strcpy(name,"TFortis PSW-2G2F+");
			break;
		case DEV_PSW2G2FPLUSUPS:
			strcpy(name,"TFortis PSW-2G2F-UPS+");
			break;
		case DEV_SWU16:
			strcpy(name,"TFortis SWU-16");
			break;
		case DEV_TLP1:
			strcpy(name,"TFortis Teleport-1");
			break;
		case DEV_TLP2:
			strcpy(name,"TFortis Teleport-2");
			break;
		default:
			strcpy(name,"TFortis PSW");
	}
}

//сокращенное наименование продукта
void get_dev_name_r(char *name){
	switch(get_dev_type()){

		case DEV_PSW2GPLUS:
			strcpy(name,"PSW-2G+");
			break;
		case DEV_PSW1G4F:
			strcpy(name,"PSW-1G4F");
			break;
		case DEV_PSW2G4F:
			strcpy(name,"PSW-2G4F");
			break;
		case DEV_PSW2G6F:
			strcpy(name,"PSW-2G6F+");
			break;
		case DEV_PSW2G8F:
			strcpy(name,"PSW-2G8F+");
			break;
		case DEV_PSW1G4FUPS:
			strcpy(name,"PSW-1G4F-UPS");
			break;
		case DEV_PSW2G4FUPS:
			strcpy(name,"PSW-2G4F-UPS");
			break;
		case DEV_PSW2G2FPLUS:
			strcpy(name,"PSW-2G2F+");
			break;
		case DEV_PSW2G2FPLUSUPS:
			strcpy(name,"PSW-2G2F-UPS+");
			break;
		case DEV_SWU16:
			strcpy(name,"SWU-16");
			break;
		case DEV_TLP1:
			strcpy(name,"Teleport-1");
			break;
		case DEV_TLP2:
			strcpy(name,"Teleport-2");
			break;
		default:
			strcpy(name,"PSW");
	}
}


//список типов устройств по id
u8 get_dev_type_by_num(u8 num){
	switch(num){
		case 0:
			return DEV_TLP1;
		case 1:
			return DEV_TLP2;
		case 2:
			return DEV_PSW;
	}
	return -1;
}

//список имени устройства по id
void get_dev_name_by_num(u8 num,char *name){
	switch(num){
		case 0:
			strcpy(name,"Teleport-1");
			break;
		case 1:
			strcpy(name,"Teleport-2");
			break;
		case 2:
			strcpy(name,"TFortis PSW");
			break;
	}
}

//число типов устройств
u8 get_dev_type_num(void){
	return TLP_DEV_NUM;
}


//число выходов у данного типа устройства
u8 get_dev_outputs(u8 type){
	switch(type){
		case DEV_TLP1:
			return 9;
		case DEV_TLP2:
			return 1;
		default:
			return 0;
	}
}

//число входов у данного типа устройства
u8 get_dev_inputs(u8 type){
	switch(type){
		case DEV_TLP1:
			return 3;
		case DEV_TLP2:
			return 5;
		case DEV_PSW:
			return 2;
	}
	return 0;
}


//тактирование всех линий
void RCC_Init(void){
	/* Enable GPIOs clocks */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB |
	                       RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD |
	                       RCC_AHB1Periph_GPIOE , ENABLE);

	/* Enable ETHERNET clock  */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_ETH_MAC | RCC_AHB1Periph_ETH_MAC_Tx |
	                       RCC_AHB1Periph_ETH_MAC_Rx, ENABLE);

	/* Enable I2C1 clock*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1,ENABLE);

	/* Enable SYSCFG clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
}

void Led_Init(void){
	//for all
	GPIO_InitStructure.GPIO_Pin = LED_DEFAULT_PIN_V1 | LED_CPU_PIN_V1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
	GPIO_Init(LED_CPU_GPIO_V1, &GPIO_InitStructure);

	//for psw-1g - не актуально, поправили плату
	/*GPIO_InitStructure.GPIO_Pin = LED_DEFAULT_PIN_V2 | LED_CPU_PIN_V2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
	GPIO_Init(LED_CPU_GPIO_V2, &GPIO_InitStructure);*/
}

void Btn_Init(void){
//default button
	GPIO_InitStructure.GPIO_Pin = BTN_DEFAULT_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(BTN_DEFAULT_GPIO, &GPIO_InitStructure);
}

void SW_Line_Init(void){
	GPIO_InitStructure.GPIO_Pin = LINE_SW_RESET_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(LINE_SW_RESET_GPIO, &GPIO_InitStructure);
	GPIO_SetBits(LINE_SW_RESET_GPIO, LINE_SW_RESET_PIN);

	GPIO_InitStructure.GPIO_Pin = SW_SD1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = SW_SD2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = SFP1_PRESENT;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = SFP2_PRESENT;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void Poe_Line_Init(void){
	GPIO_InitStructure.GPIO_Pin = POE_INT_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(POE_INT_GPIO, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LINE_POE_RST_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(LINE_POE_RST_GPIO, &GPIO_InitStructure);
	GPIO_SetBits(LINE_POE_RST_GPIO, LINE_POE_RST_PIN);
}

void board_id_lines_init(void){
	/*if((get_dev_type() == DEV_PSW2G4F)||(get_dev_type() == DEV_PSW2G6F)){*/
		GPIO_InitStructure.GPIO_Pin = BOARD_ID0_PIN_V1;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(BOARD_ID0_GPIO_V1, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = BOARD_ID1_PIN_V1;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(BOARD_ID1_GPIO_V1, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = BOARD_ID2_PIN_V1;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(BOARD_ID2_GPIO_V1, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = BOARD_ID3_PIN_V1;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
		GPIO_Init(BOARD_ID3_GPIO_V1, &GPIO_InitStructure);
	/*}*/

	/*if(get_dev_type() == DEV_PSW1G4F){*/
		GPIO_InitStructure.GPIO_Pin = BOARD_ID0_PIN_V2;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(BOARD_ID0_GPIO_V2, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = BOARD_ID1_PIN_V2;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(BOARD_ID1_GPIO_V2, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = BOARD_ID2_PIN_V2;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(BOARD_ID2_GPIO_V2, &GPIO_InitStructure);
	/*}*/
}

u8 get_board_id(void){
	if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
		return get_board_id_2();
	}
	else{
		return get_board_id_1();
	}
}

u8 get_board_id_2(void){
u8 ret = 0;
	//ID0
	if(GPIO_ReadInputDataBit(BOARD_ID0_GPIO_V2,BOARD_ID0_PIN_V2) == Bit_SET)
		ret |= 0<<0;
	else
		ret |= 1<<0;
	//ID1
	if(GPIO_ReadInputDataBit(BOARD_ID1_GPIO_V2,BOARD_ID1_PIN_V2) == Bit_SET)
		ret |= 0<<1;
	else
		ret |= 1<<1;
	//ID2
	if(GPIO_ReadInputDataBit(BOARD_ID2_GPIO_V2,BOARD_ID2_PIN_V2) == Bit_SET)
		ret |= 0<<2;
	else
		ret |= 1<<2;
	return ret;
}

u8 get_board_id_1(void){
u8 ret = 0;
	 //ID0
	if(GPIO_ReadInputDataBit(BOARD_ID0_GPIO_V1,BOARD_ID0_PIN_V1) == Bit_RESET)
		ret |= 0<<0;
	else
		ret |= 1<<0;
	//ID1
	if(GPIO_ReadInputDataBit(BOARD_ID1_GPIO_V1,BOARD_ID1_PIN_V1) == Bit_RESET)
		ret |= 0<<1;
	else
		ret |= 1<<1;
	//ID2
	if(GPIO_ReadInputDataBit(BOARD_ID2_GPIO_V1,BOARD_ID2_PIN_V1) == Bit_RESET)
		ret |= 0<<2;
	else
		ret |= 1<<2;

	//ID3
	if(GPIO_ReadInputDataBit(BOARD_ID3_GPIO_V1,BOARD_ID3_PIN_V1) == Bit_RESET)
		ret |= 0<<3;
	else
		ret |= 1<<3;

	return ret;
}

void Dry_Contact_line_Init(void){
	//PE6
	GPIO_InitStructure.GPIO_Pin = OPEN_SENSOR_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(OPEN_SENSOR_GPIO, &GPIO_InitStructure);

	if(get_dev_type() == DEV_PSW2GPLUS){
		GPIO_InitStructure.GPIO_Pin = SENSOR1_PIN_V1;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(SENSOR1_GPIO_V1, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = SENSOR2_PIN_V1;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(SENSOR2_GPIO_V1, &GPIO_InitStructure);
	}

	if((get_dev_type() == DEV_PSW2G4F)||(get_dev_type() == DEV_PSW2G4FUPS)||(get_dev_type() == DEV_PSW2G6F)
			||(get_dev_type() == DEV_PSW2G2FPLUS)||(get_dev_type() == DEV_PSW2G2FPLUSUPS)||(get_dev_type() == DEV_PSW2G8F)){
		GPIO_InitStructure.GPIO_Pin = SENSOR1_PIN_V2;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(SENSOR1_GPIO_V2, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = SENSOR2_PIN_V2;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(SENSOR2_GPIO_V2, &GPIO_InitStructure);
	}

	if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
		GPIO_InitStructure.GPIO_Pin = SENSOR1_PIN_V3;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(SENSOR1_GPIO_V3, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = SENSOR2_PIN_V3;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(SENSOR2_GPIO_V3, &GPIO_InitStructure);
	}

}

void psw1g_lines_init(void){
	//PD0 - managment mode
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	//PA6 - speed select
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

u8 speed_select(void){
	//return GPIO_ReadInputDataBit(GPIOD,GPIO_Pin_4);
	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_6) == Bit_SET)
		return 1;
	else
		return 0;
}

u8 man_unman_select(void){
	if(GPIO_ReadInputDataBit(GPIOD,GPIO_Pin_0) == Bit_SET)
		return 0;
	else
		return 1;
}

//настройка прерываний
void INT_Init(void){

//switch int line
	GPIO_InitStructure.GPIO_Pin = LINE_SW_INT_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(LINE_SW_GPIO, &GPIO_InitStructure);

	EXTI->IMR &= ~EXTI_IMR_MR11;
	EXTI->EMR &= ~EXTI_EMR_MR11;

	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;//RCC_APB2ENR_AFIOEN;
	// Конфигурирование PE11 для внешнего прерывания
	SYSCFG->EXTICR[2] &= ~SYSCFG_EXTICR3_EXTI11_PE;
	SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI11_PE;

	// Конфигурирование внешнего прерывания по переднему и заднему фронту
	EXTI->RTSR |= EXTI_RTSR_TR11;
	EXTI->FTSR |= EXTI_FTSR_TR11;
	// Установка линии прерывания(EXTI_Line11)
	EXTI->IMR |= EXTI_IMR_MR11;

	// Установка приоритета группы
	NVIC_SetPriority(EXTI15_10_IRQn, 15);
	// Разрешение прерывания порты 10-15
	NVIC_EnableIRQ(EXTI15_10_IRQn);


	if(get_dev_type() == DEV_SWU16){
		//switch int done line
		EXTI->IMR &= ~EXTI_IMR_MR9;
		EXTI->EMR &= ~EXTI_EMR_MR9;

		RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;//RCC_APB2ENR_AFIOEN;
		// Конфигурирование PE11 для внешнего прерывания
		SYSCFG->EXTICR[2] &= ~SYSCFG_EXTICR3_EXTI9_PE;
		SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI9_PE;

		// Конфигурирование внешнего прерывания по переднему и заднему фронту
		EXTI->RTSR |= EXTI_RTSR_TR9;
		EXTI->FTSR |= EXTI_FTSR_TR9;
		// Установка линии прерывания(EXTI_Line9)
		EXTI->IMR |= EXTI_IMR_MR9;

		// Установка приоритета группы
		NVIC_SetPriority(EXTI9_5_IRQn, 15);
		// Разрешение прерывания порты 9-5
		NVIC_EnableIRQ(EXTI9_5_IRQn);



		/************************************/
	/*настройка прерываний по нажатию кнопки default*/
		EXTI->IMR &= ~EXTI_IMR_MR1;
		EXTI->EMR &= ~EXTI_EMR_MR1;

		RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;//RCC_APB2ENR_AFIOEN;
		// Конфигурирование PE11 для внешнего прерывания
		SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI1_PE ;
		SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI1_PE ;
		// Конфигурирование внешнего прерывания по заднему фронту
		EXTI->RTSR |= EXTI_RTSR_TR1;
		EXTI->FTSR |= EXTI_FTSR_TR1;
		// Установка линии прерывания(EXTI_Line1)
		EXTI->IMR |= EXTI_IMR_MR1;

		// Установка приоритета группы
		NVIC_SetPriority(EXTI1_IRQn, 15);
		// Разрешение прерывания порт 1
		NVIC_EnableIRQ(EXTI1_IRQn);
	}
	else{

	/*прерывание по сигналу SD 1*/
		// Конфигурирование PE14 для внешнего прерывания
		SYSCFG->EXTICR[3] &= ~SYSCFG_EXTICR4_EXTI14_PD ;
		SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI14_PD ;
		// Конфигурирование внешнего прерывания по заднему фронту
		EXTI->RTSR |= EXTI_RTSR_TR14;// по переднему фронту
		EXTI->FTSR |= EXTI_FTSR_TR14;// по заднему фронту
		// Установка линии прерывания(EXTI_Line14)
		EXTI->IMR |= EXTI_IMR_MR14;


	/*прерывание по сигналу SD 2*/
		// Конфигурирование PE15 для внешнего прерывания
		SYSCFG->EXTICR[3] &= ~SYSCFG_EXTICR4_EXTI15_PD ;
		SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI15_PD ;
		// Конфигурирование внешнего прерывания по заднему фронту
		EXTI->RTSR |= EXTI_RTSR_TR15;// по переднему фронту
		EXTI->FTSR |= EXTI_FTSR_TR15;// по заднему фронту
		// Установка линии прерывания(EXTI_Line15)
		EXTI->IMR |= EXTI_IMR_MR15;





		// Установка приоритета группы
		NVIC_SetPriority(EXTI15_10_IRQn, 15);
		// Разрешение прерывания порты 10-15
		NVIC_EnableIRQ(EXTI15_10_IRQn);

		NVIC_EnableIRQ(EXTI15_10_IRQn);

//		/************************************/
//	/*настройка прерываний по нажатию кнопки default*/
//		EXTI->IMR &= ~EXTI_IMR_MR1;
//		EXTI->EMR &= ~EXTI_EMR_MR1;
//
//		RCC->APB2ENR |= (RCC_APB2ENR_SYSCFGEN | RCC_APB2ENR_AFIOEN)	;
//		// Конфигурирование PE11 для внешнего прерывания
//		//SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI1_PE ;
//		//SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI1_PE ;
//
//		SYSCFG->EXTICR[0] |= (SYSCFG_EXTICR1_EXTI1_PD | SYSCFG_EXTICR1_EXTI1_PE);//my
//
//		// Конфигурирование внешнего прерывания по заднему фронту
//		EXTI->RTSR |= EXTI_RTSR_TR1;
//		EXTI->FTSR |= EXTI_FTSR_TR1;
//		// Установка линии прерывания(EXTI_Line1)
//		EXTI->IMR |= EXTI_IMR_MR1;
//
//		// Установка приоритета группы
//		NVIC_SetPriority(EXTI1_IRQn, 15);
//		// Разрешение прерывания порт 1
//		NVIC_EnableIRQ(EXTI1_IRQn);


		//poe controller int
		SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE,EXTI_PinSource3);

		EXTI_InitStructure.EXTI_Line = EXTI_Line3;   /// poe_int на линии 3
		EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;    // прерывание (а не событие)
		EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;//EXTI_Trigger_Falling;  // срабатываем по переднему фронту импульса
		EXTI_InitStructure.EXTI_LineCmd = ENABLE;    /// вкл
		EXTI_Init(&EXTI_InitStructure);

		NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;  // указываем канал IRQ
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;  // приоритет канала ( 0 (самый приоритетный) - 15)
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 15;  // приоритет подгруппы
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
	}


}

void set_led_cpu(u8 state){
	if(state==ENABLE)
		GPIO_ResetBits(GPIOE, GPIO_Pin_0);
	else if(state == DISABLE)
		GPIO_SetBits(GPIOE, GPIO_Pin_0);
	else if(state == TOGLE_LED)
		GPIO_ToggleBits(GPIOE, GPIO_Pin_0);

}

void set_led_default(u8 state){
	if(state == ENABLE)
		GPIO_ResetBits(GPIOE, GPIO_Pin_2);
	else if(state == DISABLE)
		GPIO_SetBits(GPIOE, GPIO_Pin_2);
	else if(state == TOGLE_LED)
		GPIO_ToggleBits(GPIOE, GPIO_Pin_2);
}

//todo
void VCP_Processing(uint8_t *Buf,uint32_t Len){
u32 i,addr,data;
u8 var;
signed char AppBuff[512];
char temp[256];
struct mac_entry_t entry;
static u8 default_settings_start = 0;


bstp_bridge_stat_t bstat;
//uip_ipaddr_t ipaddr;

	if(usb_shell_state()){
		if(UsbShellQueue)
			xQueueSend(UsbShellQueue,Buf,0);
		return;
	}



	if(Len==18){
		if(Salsa2Queue)
			xQueueSend(Salsa2Queue,Buf,0);
		return;
	}


	for (i = 0; i < Len; i++)

		switch(*(Buf + i)){


			/*case '1':
				DEBUG_MSG(1,"UDP ports\r\n");
				for(u8 c = 0; c < UIP_UDP_CONNS; ++c) {
					//выводим список всех портов
					DEBUG_MSG(1,"conn: port:%d/%d %d.%d.%d.%d\r\n",
						htons(uip_udp_conns[c].rport),htons(uip_udp_conns[c].lport),
						uip_ipaddr1(uip_udp_conns[c].ripaddr),uip_ipaddr2(uip_udp_conns[c].ripaddr),
						uip_ipaddr3(uip_udp_conns[c].ripaddr),uip_ipaddr4(uip_udp_conns[c].ripaddr));
				}
				break;*/

			/*case '2':
				DEBUG_MSG(1,"STP Debug ON\r\n");
				STP_DEBUG = 1;
				break;

			case '3':
				DEBUG_MSG(1,"STP Debug OFF\r\n");
				STP_DEBUG = 0;
				break;*/

			case '1':
				test_processing_flag = 1;
				break;
			case '2':
				test_processing_flag = 2;
				break;
			case '3':
				test_processing_flag = 3;
				break;
			case '4':
				test_processing_flag = 4;
				break;
			case '5':
				test_processing_flag = 5;
				break;
			case '6':
				test_processing_flag = 6;
				break;
			case '7':
				test_processing_flag = 7;
				break;
			case '8':
				test_processing_flag = 8;
				break;
			case '9':
				test_processing_flag = 9;
				break;








			/*case '2':
				DEBUG_MSG(1,"UDP\r\n");
				for(u8 c = 0; c < UIP_UDP_CONNS; ++c) {
					//выводим список всех портов
					DEBUG_MSG(1,"conn: port:%d/%d %d.%d.%d.%d\r\n",
						htons(uip_udp_conns[c].rport),htons(uip_udp_conns[c].lport),
						uip_ipaddr1(uip_udp_conns[c].ripaddr),uip_ipaddr2(uip_udp_conns[c].ripaddr),
						uip_ipaddr3(uip_udp_conns[c].ripaddr),uip_ipaddr4(uip_udp_conns[c].ripaddr));
				}
				break;
*/


			case 'f':
#if IGMP_DEBUG
				igmp_dump_group_list();
#endif
				break;




			/**********************************************************/
			case 'c':
				usb_shell_task_start();
				break;

			case 'd':
			case 'D':
				printf("Reset ALL settings to factory default, do you want to continue?\r\n");
				default_settings_start=1;
				break;

			case 'y':
			case 'Y':
				if(default_settings_start==1){
					printf("Reset settings and reboot...\r\n");
					settings_default(0);
					reboot(REBOOT_MCU_5S);
					default_settings_start=0;
				}
				break;

			case 't':
				printf("Name       State  Priority StackSize ID\r\n");
				memset(AppBuff, 0,512);
				vTaskList(AppBuff);
				char tmpbuf[64];
				int i=0;
				for(;;){
					if((AppBuff[i] != 10) && (AppBuff[i] != 13)){
						tmpbuf[strlen(tmpbuf)] = AppBuff[i];
					}
					else{
						printf("%s\r\n",tmpbuf);
						memset(tmpbuf,0,64);
					}
					i++;
					if((i>1024)||(i>strlen((char *)AppBuff)))
						break;
				}
				printf("Heap free: %d\r\n", xPortGetFreeHeapSize());
				for(u8 i=0;i<ALARM_REC_NUM;i++){
					if(printf_alarm(i,tmpbuf)!=0){
						printf("Error: %s\r\n",tmpbuf);
						memset(tmpbuf,0,64);
					}
				}
				break;


			case 'i':
				printf("IP addr: %d:%d:%d:%d\r\n",uip_ipaddr1(uip_hostaddr),uip_ipaddr2(uip_hostaddr),
					   uip_ipaddr3(uip_hostaddr),uip_ipaddr4(uip_hostaddr));
				break;

			case 'g':
				printf("Gate: %d:%d:%d:%d\r\n",uip_ipaddr1(uip_draddr),uip_ipaddr2(uip_draddr),
					   uip_ipaddr3(uip_draddr),uip_ipaddr4(uip_draddr));
				break;

			case 'm':
				printf("Mask: %d:%d:%d:%d\r\n",uip_ipaddr1(uip_netmask),uip_ipaddr2(uip_netmask),
					   uip_ipaddr3(uip_netmask),uip_ipaddr4(uip_netmask));
				break;

			case 'M':
				printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",dev_addr[0],dev_addr[1],dev_addr[2],dev_addr[3],
						dev_addr[4],dev_addr[5]);
				break;


			case 'V':
				sampleDisplayVIDTable();
				break;

			case 'v':
				printf("Firmware version: %02X.%02X.%02X\r\n",
						 (int)(image_version[0]>>16)&0xff,
						 (int)(image_version[0]>>8)&0xff,
						 (int)(image_version[0])&0xff);
				printf("Bootloader version: %02X.%02X\r\n",(int)(bootloader_version>>8)&0xff,  (int)(bootloader_version)&0xff);
				get_dev_name((char *)AppBuff);
				printf("Device type: %s\r\n",(char *)AppBuff);
				printf("Device ID: %d\r\n",get_dev_type());
				get_cpu_id_to_str(tmpbuf);
				printf("CPU ID: %s\r\n",tmpbuf);
				printf("board ID: %d\r\n",get_board_id());
				printf("RTC value: %lu\r\n",RTC_GetCounter());
				printf("PoE Ports: %d\r\n",POE_PORT_NUM);
				printf("Cooper Ports: %d\r\n",COOPER_PORT_NUM);
				printf("Fiber(SFP) Ports: %d\r\n",FIBER_PORT_NUM);
				printf("CPU: %d\r\n",CPU_PORT);
				printf("MV: %d\r\n",MV_PORT_NUM);

				printf("USB %lX\r\n",(*(uint32_t*)0x50000440));

				printf("Global Control2 %d:\r\n",ETH_ReadPHYRegister(0x1C,0x1C));
				printf("CPU Idle %d\r\n",GetCPU_IDLE());
				break;

			case 's':
				 //bstp_dump();


				 DEBUG_MSG(1,"\r\n");

				 bstp_get_bridge_stat(&bstat);
				 if (bstat.flags&BSTP_BRIDGE_STAT_FLAG_ACTIVE) DEBUG_MSG(1,"STP active\r\n");
				 if (bstat.flags&BSTP_BRIDGE_STAT_FLAG_ROOT) DEBUG_MSG(1,"ROOT Bridge\r\n");

				 DEBUG_MSG(1,"root port: %d\r\n", bstat.root_port);

				 DEBUG_MSG(1,"\t root bridge priority: %04x\r\n", ntohs(bstat.rootpri));


				 DEBUG_MSG(1,"\t root bridge mac: ");
				 for(int i=0;i<6;i++) {
					 DEBUG_MSG(1,"%02x", bstat.rootaddr[i]);
					 if (i!=5) DEBUG_MSG(1,":");
				 }
				 DEBUG_MSG(1,"\r\n");
				 DEBUG_MSG(1,"\t path cost to root: %lu\r\n", bstat.root_cost);
				 DEBUG_MSG(1,"\t designated bridge priority: %04x\r\n", ntohs(bstat.desgpri));
				 DEBUG_MSG(1,"\t designated bridge mac: ");
				 for(int i=0;i<6;i++) {
					 DEBUG_MSG(1,"%02x", bstat.desgaddr[i]);
					 if (i!=5)DEBUG_MSG(1,":");
				 }
				 DEBUG_MSG(1,"\r\n");
				 DEBUG_MSG(1,"\t root port id: %04x\r\n", bstat.root_port_id);
				 DEBUG_MSG(1,"\t designated port id: %04x\r\n", bstat.dport_id);
				 DEBUG_MSG(1,"\t Last Topology Change: %lu\r\n", bstat.last_tc_time);
				 DEBUG_MSG(1,"\t Topology Change Count: %lu\r\n", bstat.tc_count);

				 for(int port=0;port<(ALL_PORT_NUM);port++){
				   bstp_port_stat_t pstat;
				     bstp_get_port_stat(port, &pstat);
				     if (pstat.flags.bp_active_==0) {
				    	 DEBUG_MSG(1,"Port %d not active\r\n", port);
				    	 continue;
				     } else DEBUG_MSG(1,"Port %d (active, baud = %lu):\r\n", port, pstat.if_baudrate);
				     DEBUG_MSG(1,"\t");
				     if (pstat.if_flags&IFF_UP) DEBUG_MSG(1," IfUp");
				     if (pstat.if_link_state&LINK_STATE_DOWN) DEBUG_MSG(1," LinkDown");
				     if (pstat.if_link_state&LINK_STATE_FULL_DUPLEX) DEBUG_MSG(1," FullDuplex");
				     DEBUG_MSG(1,"\r\n");
				     DEBUG_MSG(1,"\t port_id: %04x", pstat.port_id);
				     DEBUG_MSG(1,"\t priority: %d", pstat.priority);
				     DEBUG_MSG(1,"\t path_cost: %lu", pstat.path_cost);
				     DEBUG_MSG(1,"\t proto: %d", pstat.protover);
				     DEBUG_MSG(1,"\r\n");

				     DEBUG_MSG(1,"\t state: ");
				      switch(pstat.state){
				        case BSTP_IFSTATE_FORWARDING: DEBUG_MSG(1,"forwarding "); break;
				        case BSTP_IFSTATE_DISABLED:   DEBUG_MSG(1,"disabled "); break;
				        case BSTP_IFSTATE_LISTENING:  DEBUG_MSG(1,"listering"); break;
				        case BSTP_IFSTATE_LEARNING:   DEBUG_MSG(1,"learning"); break;
				        case BSTP_IFSTATE_BLOCKING:   DEBUG_MSG(1,"blocking"); break;
				        case BSTP_IFSTATE_DISCARDING: DEBUG_MSG(1,"discarding "); break;
				      }
				      DEBUG_MSG(1,"\t role: ");
				      switch(pstat.role){
				        case BSTP_ROLE_DISABLED:   DEBUG_MSG(1,"disabled"); break;
				        case BSTP_ROLE_ROOT:       DEBUG_MSG(1,"root"); break;
				        case BSTP_ROLE_DESIGNATED: DEBUG_MSG(1,"designated"); break;
				        case BSTP_ROLE_ALTERNATE:  DEBUG_MSG(1,"alternative "); break;
				        case BSTP_ROLE_BACKUP:     DEBUG_MSG(1,"backup"); break;
				      }
				      DEBUG_MSG(1,"  ");
				      if (pstat.flags.bp_ptp_link_) {DEBUG_MSG(1,"ptp "); }else {DEBUG_MSG(1,"   ");}
				      if (pstat.flags.bp_operedge_) {DEBUG_MSG(1,"edge "); }else {DEBUG_MSG(1,"    ");}
				      DEBUG_MSG(1,"\r\n");
				      DEBUG_MSG(1,"\t forward transitions: %lu\r\n", pstat.forward_transitions);

				      DEBUG_MSG(1,"\t Desgn. bridge: %04x/", bstat.desgpri);
					  for(int i=0;i<6;i++) {
						  DEBUG_MSG(1,"%02x", bstat.desgaddr[i]);
						 if (i!=5)DEBUG_MSG(1,":");
					  }
					  DEBUG_MSG(1,"\r\n");
					  DEBUG_MSG(1,"\r\n");
				 }
			 	 break;
			case 'u':
				if(uip_debug_enabled()){
					uip_debug_disable();
					printf("UIP debug STOP\r\n");
				}
				else{
					uip_debug_enable();
					printf("UIP debug START\r\n");
				}
				break;



			default:
				printf("------------------------------------------------\r\n");
				printf("Avalible commands:\r\n");
				printf("c - start console managment mode\r\n");
				printf("d - reset settings to factory default\r\n");
				printf("t - tasks stat\r\n");
				printf("i - IP address\r\n");
				printf("m - net mask\r\n");
				printf("M - MAC address\r\n");
				printf("g - gateway IP address\r\n");
				printf("v - firmware version\r\n");
				printf("V - show VID Table\r\n");
				printf("u - start UIP debug mode\r\n");

				break;
		}
}

void get_link_state_timer(void){
	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
	   (get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){

		for(u8 j=0;j<(ALL_PORT_NUM);j++){
			if(ETH_ReadPHYRegister(L2F_port_conv(j)+0x10, 0x00) & 0x800)
				dev.port_stat[j].link=1;
			else
				dev.port_stat[j].link=0;
		}
	}
	if(get_marvell_id() == DEV_98DX316){
		for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
			if(Salsa2_ReadRegField(PORT_STATUS_REG0+0x400*L2F_port_conv(i),0,1)){
				dev.port_stat[i].link=1;
			}
			else{
				dev.port_stat[i].link=0;
			}
		}
	}
}

void get_link_state_isr(void){

static u8 init=0;
static u8 run_ge1_timer;
static u8 run_ge2_timer;
static u8 sfp1_sd_last,sfp2_sd_last;
u8 sfp1_sd,sfp2_sd;
static struct timer link_ge1_timer,link_ge2_timer;
//static u8 link_temp[PORT_NUM];
//static u8 link_cnt[PORT_NUM];
u16 tmp;

if(dev.interrupts.sfp_sd_changed){
	dev.interrupts.sfp_sd_changed = 0;
	init = 0;
}

	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
	  (get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
			//если было прерывание, смотрим статусы и линки
			if ((GPIO_ReadInputDataBit(GPIOE,LINE_SW_INT_PIN)==Bit_RESET) || (init == 0) ){
				if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){
					tmp = ETH_ReadPHYRegister(0x1B, 0x00);
					if((tmp & GT_PHY_INTERRUPT)||(init==0)){//PHY Interrupt is active
						//printf("GT_PHY_INTERRUPT\r\n");
						for(u8 j=0;j<(ALL_PORT_NUM);j++){
							if(ETH_ReadPHYRegister(L2F_port_conv(j)+0x10, 0x00) & 0x800)
								dev.port_stat[j].link=1;
							else
								dev.port_stat[j].link=0;
						}
						//clear interrupts bit
						for(uint8_t j=0;j<11;j++){
							ETH_ReadPHYRegister(j,QD_PHY_INT_STATUS_REG);
						}
					}
				}
				//
				if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
					tmp = ETH_ReadPHYRegister(0x1B, 0x00);
					if((tmp & GT_DEVICE_INT)||(init==0)){//PHY Interrupt is active
						printf("GT_DEVICE_INT\r\n");
						for(u8 j=0;j<(ALL_PORT_NUM);j++){
							if(ETH_ReadPHYRegister(0x10+j, 0x00) & 0x800)
								dev.port_stat[j].link=1;
							else
								dev.port_stat[j].link=0;
						}
						for(u8 j=0;j<7;j++){
							ETH_ReadIndirectPHYReg(L2F_port_conv(j),PAGE0,QD_PHY_INT_STATUS_REG);
						}
						ETH_ReadIndirectPHYReg(0x0F, PAGE1, QD_PHY_INT_STATUS_REG);
						init = 1;
					}
				}
			}


			if((get_marvell_id() == DEV_88E095 || get_marvell_id() == DEV_88E097)){
				//timer init
				if(init == 0){
					timer_set(&link_ge1_timer, 0);
					timer_set(&link_ge2_timer, 0);
					run_ge1_timer=0;
					run_ge2_timer=0;
					sfp1_sd_last = 2;
					sfp2_sd_last = 2;
					init = 1;
				}

				sfp1_sd = GPIO_ReadInputDataBit(GPIOD,SW_SD1);
				sfp2_sd = GPIO_ReadInputDataBit(GPIOD,SW_SD2);

				//for port disable
				if((get_port_sett_state(GE1)==DISABLE)){
					return;
				}

				if((get_port_sett_state(GE2)==DISABLE)){
					return;
				}

				//SFP 1
				//if sfp sd signal changed
				if(sfp1_sd != sfp1_sd_last){
					timer_set(&link_ge1_timer, SFP_SD_TIMER * MSEC);
					run_ge1_timer=1;
					sfp1_sd_last = sfp1_sd;
					//printf("sd 1 change %d\r\n",sfp1_sd);
				}

				if((get_port_sett_sfp_mode(GE1)==2)||
				((run_ge1_timer == 1) && (timer_expired(&link_ge1_timer)) && (get_port_sett_sfp_mode(GE1)!=1))||
				((run_ge1_timer == 1) && (timer_expired(&link_ge1_timer))&&(get_port_sett_state(GE1)==DISABLE))){
					if ((sfp1_sd==1)&&(dev.port_stat[GE1].link == 0)) {
						tmp = ETH_ReadPHYRegister(0x18, 0x01);//link up port GE#1
						tmp |= 0x003E;
						ETH_WritePHYRegister(0x18, 0x01, tmp);
						printf("GE1 forced up\r\n");
						run_ge1_timer=0;
					}
					else if((sfp1_sd==0)&&(dev.port_stat[GE1].link == 1)){
						tmp = ETH_ReadPHYRegister(0x18, 0x01);//link down port GE#1
						tmp &= ~0x003E;
						tmp |= 0x000e;
						ETH_WritePHYRegister(0x18, 0x01, tmp);
						printf("GE1 forced down\r\n");
						run_ge1_timer=0;
					}

					run_ge1_timer=0;
					//printf("GE1 timer_expired\r\n");
				}

				//SFP 2
				//if sfp sd signal changed
				if(sfp2_sd != sfp2_sd_last){
					timer_set(&link_ge2_timer, SFP_SD_TIMER * MSEC);
					run_ge2_timer=1;
					sfp2_sd_last = sfp2_sd;
					//printf("sd 2 change %d\r\n",sfp2_sd);
				}

				if((get_port_sett_sfp_mode(GE2)==2)||
				((run_ge2_timer == 1) && (timer_expired(&link_ge2_timer)) && (get_port_sett_sfp_mode(GE2)!=1))||
				((run_ge2_timer == 1) && (timer_expired(&link_ge2_timer))&&(get_port_sett_state(GE2)==DISABLE))){
					if ((sfp2_sd==1)&&(dev.port_stat[GE2].link == 0)) {
						tmp = ETH_ReadPHYRegister(0x19, 0x01);//link up port GE#2
						tmp |= 0x003E;
						ETH_WritePHYRegister(0x19, 0x01, tmp);
						printf("GE2 forced up\r\n");
						run_ge1_timer=0;
					}
					else if((sfp2_sd==0)&&(dev.port_stat[GE2].link == 1)){
						tmp = ETH_ReadPHYRegister(0x19, 0x01);//link down port GE#2
						tmp &= ~0x003E;
						tmp |= 0x000e;
						ETH_WritePHYRegister(0x19, 0x01, tmp);
						printf("GE2 forced down\r\n");
						run_ge2_timer=0;
					}

					run_ge2_timer=0;
				}
		}

	}

	if(get_marvell_id() == DEV_98DX316){
		for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
			if(Salsa2_ReadRegField(PORT_STATUS_REG0+0x400*L2F_port_conv(i),0,1)){
				dev.port_stat[i].link=1;
			}
			else{
				dev.port_stat[i].link=0;
			}
		}
	}

}





u8 get_sensor_state(u8 channel){
	switch(channel){
		case 0:
			return GPIO_ReadInputDataBit(OPEN_SENSOR_GPIO,OPEN_SENSOR_PIN);
		case 1:
			if(get_dev_type()==DEV_PSW2GPLUS){
				//в старых версиях платы не было инверсии входов
				if(get_board_id()==0){
					if(GPIO_ReadInputDataBit(SENSOR1_GPIO_V1,SENSOR1_PIN_V1))
						return 1;
					else
						return 0;
				}
				//в новых есть
				else{
					if(GPIO_ReadInputDataBit(SENSOR1_GPIO_V1,SENSOR1_PIN_V1))
						return 0;
					else
						return 1;
				}
			}
			if((get_dev_type()==DEV_PSW2G4F)||(get_dev_type() == DEV_PSW2G4FUPS)||(get_dev_type()==DEV_PSW2G6F)
					||(get_dev_type()==DEV_PSW2G2FPLUS)||(get_dev_type()==DEV_PSW2G2FPLUSUPS) || (get_dev_type()==DEV_PSW2G8F)){
				if(GPIO_ReadInputDataBit(SENSOR1_GPIO_V2,SENSOR1_PIN_V2))
					return 0;
				else
					return 1;
			}
			if((get_dev_type()==DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
				if(GPIO_ReadInputDataBit(SENSOR1_GPIO_V3,SENSOR1_PIN_V3))
					return 0;
				else
					return 1;
			}
			break;
		case 2:
			if(get_dev_type()==DEV_PSW2GPLUS){
				//в старых версиях платы не было инверсии входов
				if(get_board_id()==0){
					if(GPIO_ReadInputDataBit(SENSOR2_GPIO_V1,SENSOR2_PIN_V1))
						return 1;
					else
						return 0;
				}
				else{
					//в новых есть
					if(GPIO_ReadInputDataBit(SENSOR2_GPIO_V1,SENSOR2_PIN_V1))
						return 0;
					else
						return 1;
				}
			}
			if((get_dev_type()==DEV_PSW2G4F)||(get_dev_type() == DEV_PSW2G4FUPS)||(get_dev_type()==DEV_PSW2G6F)
					||(get_dev_type()==DEV_PSW2G2FPLUS)||(get_dev_type()==DEV_PSW2G2FPLUSUPS)||(get_dev_type()==DEV_PSW2G8F)){
				if(GPIO_ReadInputDataBit(SENSOR2_GPIO_V2,SENSOR2_PIN_V2))
					return 0;
				else
					return 1;
			}
			if((get_dev_type()==DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
				if(GPIO_ReadInputDataBit(SENSOR2_GPIO_V3,SENSOR2_PIN_V3))
					return 0;
				else
					return 1;
			}
	}
	return 0;
}





void rtc_Init(void){
	int i=0;
	u32 timeout;
	RTC_TimeTypeDef RTC_Time;
	RTC_DateTypeDef RTC_Date;

    // Запускаем LSI:
    RCC->CSR |= RCC_CSR_LSION;
    // Ждём, когда он заведётся
    i=0;
    while((!(RCC->CSR & RCC_CSR_LSIRDY))&&(i<10000)) {i++;}
    // Ок, генератор на 32 кГц завёлся.


    // Включим тактирование PWR
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);


    // Разрешим доступ к управляющим регистрам энергонезависимого домена (для BKP)
    PWR->CR |= PWR_CR_DBP;


    /* Reset BKP domain if different clock source selected.*/
    //если уже настроено
    //if(RTC->TR != 0){
      /* Backup domain reset.*/
      RCC->BDCR |=  RCC_BDCR_BDRST;
      for(timeout=0;timeout<1000;timeout++){}//dumm loop
      RCC->BDCR &= ~RCC_BDCR_BDRST;
    //}



    // Выберем его как источник тактирования RTC:
    RCC->BDCR &= ~RCC_BDCR_RTCSEL; // сбросим
    RCC->BDCR |= (RCC_BDCR_RTCSEL_1); // запишем 0b10	//LSI

    // Включим тактирование RTC
    RCC->BDCR |= RCC_BDCR_RTCEN;


    // Снимем защиту от записи с регистров RTC
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;


    // Войдём в режим инициализации:
    RTC->ISR |= RTC_ISR_INIT;

        // Ждём, когда это произойдёт
    i=0;
    while((!(RTC->ISR & RTC_ISR_INITF))&&(i<10000)) {i++;}

    if(!(RTC->ISR & RTC_ISR_INITF))
       return;

	// Часы остановлены. Режим инициализации
	// Настроим предделитель для получения частоты 1 Гц.

	// LSI:
	// LSE: нужно разделить на 0x7fff (кварцы 	так точно рассчитаны на это)

	uint32_t Sync = 249;   // 15 бит
	uint32_t Async = 127;  // 7 бит

	// Сначала записываем величину для синхронного предделителя
	RTC->PRER = Sync;

	// Теперь добавим для асинхронного предделителя
	RTC->PRER = Sync | (Async << 16);

	//if(RTC->TR == 0){
		RTC_Time.RTC_Hours = RTC_Time.RTC_Minutes  = RTC_Time.RTC_Seconds = 0;
		RTC_Date.RTC_Month = RTC_Date.RTC_Date = RTC_Date.RTC_Year = 0;
		RTC_SetTime(RTC_Format_BIN,&RTC_Time);
		RTC_SetDate(RTC_Format_BCD,&RTC_Date);
	//}

    // Инициализация закончилась
    RTC->ISR &= ~RTC_ISR_INIT;

    //восстановим защиту
    RTC->WPR = 0xFF;


    RTC_GetTime(RTC_Format_BIN,&RTC_Time);
    RTC_GetDate(RTC_Format_BCD,&RTC_Date);
    // Всё, часы запустились и считают время.))
}




void set_reboot_flag(u8 flag){
	reboot_flag_ = flag;
}

u8 get_reboot_flag(void){
	return reboot_flag_;
}

void reboot(u8 flag){
	set_reboot_flag(flag);
}


void reboot_(u8 flag){

	switch(flag){
		case 0:
			return;

		case REBOOT_POE:
			if((get_dev_type() == DEV_PSW2GPLUS)||(get_dev_type() == DEV_PSW2G6F)||(get_dev_type()==DEV_PSW2G2FPLUS) ||(get_dev_type() == DEV_PSW2G6F)
					||(get_dev_type()==DEV_PSW2G2FPLUSUPS)){
				GPIO_ResetBits(LINE_POE_RST_GPIO, LINE_POE_RST_PIN);
				vTaskDelay(500*MSEC);
				GPIO_SetBits(LINE_POE_RST_GPIO, LINE_POE_RST_PIN);
			}
			else{
				GPIO_ResetBits(LINE_POE_RST_GPIO, LINE_POE_RST_PIN);
				vTaskDelay(500*MSEC);
				GPIO_SetBits(LINE_POE_RST_GPIO, LINE_POE_RST_PIN);
			}
			break;

		case REBOOT_SWITCH:
			GPIO_ResetBits(LINE_SW_RESET_GPIO, LINE_SW_RESET_PIN);
			vTaskDelay(3000*MSEC);
			GPIO_SetBits(LINE_SW_RESET_GPIO, LINE_SW_RESET_PIN);
			break;

		case REBOOT_MCU:
			NVIC_SystemReset();
			break;


		case REBOOT_ALL:
			//clear rtc count
			Reset_RTC_cnt();
			//poe reset
			//GPIO_ResetBits(LINE_POE_RST_GPIO, LINE_POE_RST_PIN);
			//sw reset
			GPIO_ResetBits(LINE_SW_RESET_GPIO, LINE_SW_RESET_PIN);
			vTaskDelay(1000*MSEC);
			//GPIO_SetBits(LINE_POE_RST_GPIO, LINE_POE_RST_PIN);
			GPIO_SetBits(LINE_SW_RESET_GPIO, LINE_SW_RESET_PIN);
			NVIC_SystemReset();
			break;

		case REBOOT_MCU_5S:
			vTaskDelay(5000*MSEC);
			NVIC_SystemReset();
			break;

		case REBOOT_MCU_SWITCH:
			GPIO_ResetBits(LINE_SW_RESET_GPIO, LINE_SW_RESET_PIN);
			vTaskDelay(50*MSEC);
			NVIC_SystemReset();
			break;

	}


}


static struct file_header{
	u32 header_crc;
	u32 file_len; // суммарный размер файла прошивки включая заголовок
	u32 file_crc;
	u32 version;
	u8	file_id[240-12];
	u32 help_addr;//смещение начала хелпа относительно начала файла прошивки
	u32 help_len;//размер области хелпа
 	u32 help_crc;
} header;

u32 get_new_fw_vers(void){
	return header.version;
}

//перед обновлением копируем часть с файлами help
/*static u8 HelpCopy(struct file_header *header){
	u32 esize;
	u32 len,offset;
	u8 buf[128];

	//мы не пишем, если help_len == 0
	if(header->help_len == 0)
		return 1;

	//erase flash
	spi_flash_properties(NULL,NULL,&esize);
	for(u8 i=0;i<((FL_FS_END-FL_FS_START)/esize);i++){
	   spi_flash_erase(FL_FS_START+esize*i,esize);
	   IWDG_ReloadCounter();
	}
	//copy data 2 flash
	offset = 0;
	while(offset < header->help_len){
		if((header->help_len - offset)>64)
			len = 64;
		else
			len = header->help_len - offset;
		spi_flash_read(FL_TMP_START + header->help_addr + offset,len,buf);
		spi_flash_write(FL_FS_START + offset, len, buf);
		IWDG_ReloadCounter();
		offset+=len;
	}
	return 0;
}
*/

/*тестирование образа прошивки ПО на предмет ошибок*/
uint32_t TestFirmware(void){

//printf("check firmware....\r\n");


#define MIN_LEN 40000  //min размер прошивки
#define MAX_LEN	983040 //max размер прошивки
#define ID_SIZE 21	   //длина строки ID для сравнения
#define OPTION_SECTOR_OFFSET 256 // первые 256 байт в файле образа для служебных целей

	//extern struct rc4_state encrypt_state;

	u8 inbuff[256];//,outbuff[256];

	u8 crypt_key[64]={'8','(',']','J','r','w','K','H','/','w','b','K','/','!','M','}',
			'n','G','[','b','k','H','h','b','<','n','p','y','I','e','A','.','z','=','H',
			'=','p','Y','0','m','d','Q','H','x','/','W','y','/','9','R','%','g','(','L','Y','F','+','K',
			'[','q','T','l','r','r'};

	const char id[ID_SIZE]={'F','O','R','T','-','T','E','L','E','C','O','M','P','S','W','2','G','P','L','U','S'};

	uint32_t crc;

	//читаем заголовок
	spi_flash_read(0,256,inbuff);
	//дешефруем
	RC4_Crypt(inbuff,256,crypt_key,64);
	memcpy(&header,inbuff,256);

	//printf("header total len %lu\r\n",header.file_len);
	//printf("header version  %lx\r\n",header.version);
	//printf("hd file crc  %lu\r\n",header.file_crc);

	//printf("real file crc  %lu\r\n",CryptCrc32(OPTION_SECTOR_OFFSET,crypt_key,header.file_len-OPTION_SECTOR_OFFSET));

	//if (header.version == image_version[0])
	//	return 1;

	//check header crc
	//sum = 0;
	//memcpy(inbuff,&header,256);
	//sum = BuffCrc(((inbuff+4)),(OPTION_SECTOR_OFFSET-4));
	//printf("real header crc %lu\r\n",sum);
	//printf("header crc %lu\r\n",header.header_crc);

	//if(sum != header.header_crc){
	//	return 2;
	//}

	//check id
	for(u8 i=0;i<ID_SIZE;i++){
		if(header.file_id[i]!=id[i]){
			return 8;
		}
	}

	//check total len
	if((header.file_len<MIN_LEN)||(header.file_len>MAX_LEN)){
		return 9;
	}

	//read & encode & check crc
	crc = CryptCrc32(OPTION_SECTOR_OFFSET,crypt_key,header.file_len-OPTION_SECTOR_OFFSET);
	if(header.file_crc != crc){
		return 10;
	}

#if FLASH_FS_USE
	//если есть хелп, то копируем его
	if(header.help_len != 0){
		//check help len
		if(header.help_len>(FL_TMP_END - FL_TMP_START - header.file_len)){
			return 10;
		}

		//check help crc
		if(header.help_crc != Crc32(FL_TMP_START + header.help_addr,header.help_len)){
			return 11;
		}

		//copy help 2 flash
		if(HelpCopy(&header)!=0){
			return 12;
		}
	}
#endif

//printf("check firmware ok\r\n");

return 0;
}






int check_ip_addr(uip_ipaddr_t addr){
	//if((uip_ipaddr1(addr)!=0)&&(uip_ipaddr4(addr)!=0)){
		return 0;
	//}
	//else
	//	return -1;
}



u8 get_board_version(void){
	return 4;
}

// группа get для статусной информации
u8 get_port_link(u8 port){
	return dev.port_stat[port].link;
}

u8 get_port_poe_a(u8 port){
	return dev.port_stat[port].poe_a;
}

u8 get_port_poe_b(u8 port){
	return dev.port_stat[port].poe_b;
}

u8 get_port_error(u8 port){
	if(port < POE_PORT_NUM){
		return dev.port_stat[port].error;
	}
	else
		return 0;
}

u8 get_all_port_error(void){
	for(u8 i=0;i<(POE_PORT_NUM);i++){
		if(get_port_error(i))
			return 1;
	}
	return 0;
}

void set_port_error(u8 port,u8 err){
	dev.port_stat[port].error = err;
}

//ret 1 если хоть один аларм активен
u8 get_all_alarms_error(void){
	for(u8 i=0;i<NUM_ALARMS;i++){
		if(dev.alarm.dry_cont[i])
			return 1;
	}
	if(dev.alarm.nopass)
		return 1;

	return 0;
}



void psw1g_fiber_speed(u16 speed){
u16 tmp;
	//printf("fiber speed %d\r\n",speed);
	if(speed == 100){
		//100
		tmp = ETH_ReadIndirectPHYReg(0x0F,1,0);
		tmp |=0x2000;
		tmp &= ~40;
		ETH_WriteIndirectPHYReg(0x0F,1,0,tmp);
	}
	else if(speed == 1000){
		//1000
		tmp = ETH_ReadIndirectPHYReg(0x0F,1,0);
		tmp &=~0x2000;
		tmp |= 40;
		ETH_WriteIndirectPHYReg(0x0F,1,0,tmp);
	}
}

void psw1g_unmanagment(u8 state){
u16 tmp;
	if(state == ENABLE){
		if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
			for(uint8_t i=0;i<7;i++){
				//PowerDown to 0 - normal operation - REG 0
				tmp = ETH_ReadIndirectPHYReg(i,0,0);
				tmp &=~0x800;
				ETH_WriteIndirectPHYReg(i,0,0,tmp);
				//PowerDown to 0 - normal operation - REG 16
				tmp = ETH_ReadIndirectPHYReg(i,0,16);
				tmp &=~0x4;
				ETH_WriteIndirectPHYReg(i,0,16,tmp);
				tmp = ETH_ReadIndirectPHYReg(i,0,16);
			}
			//PowerDown to 0 for fiber portrs
			tmp = ETH_ReadIndirectPHYReg(0x0F,1,0);
			tmp &=~0x800;
			ETH_WriteIndirectPHYReg(0x0F,1,0,tmp);

			vTaskDelay(1000*MSEC);
			for(uint8_t i=0;i<7;i++){
				stp_set_port_state(i,BSTP_IFSTATE_FORWARDING);
			}
		}
	}
}

i8 get_cpu_temper(void){
    //float V25 = 0.76;
    //float Avg_Slope = 2.5e-3;
    //float Vref = 3.3;

    //uint16_t dt = readADC1(16);
    //float V = dt/4096*Vref;
	 //return (u16)((V25 - V)/Avg_Slope + 25);

	u16 V = readADC1(16);
	//printf("Temper adc val: %d\r\n",V);

	i8 t = (V - 1037)/(3)+25;
	return t;
}

i8 get_marvell_temper(void){
u16 reg;
	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){
		return 0;
	}else{
		reg = ETH_ReadIndirectPHYReg(0,PAGE6,27);
		reg =(reg & 0x0F)-25;
		return reg;
	}
}


u16 get_poe_temper(void){
u16 temper;
	if(get_poe_id() == DEV_LTC4271){
		return 0;
	}
	else{
		/*temper= (I2c_ReadByte(Address2384,0x68)<<8) | I2c_ReadByte(Address2384,0x60);
		if(temper & 0x8000)
			return temper & 0x7FFF;*/

	}
	return 0;
}

static void Reset_RTC_cnt(void){
RTC_TimeTypeDef RTC_Time;
RTC_DateTypeDef RTC_Date;

	RTC_Time.RTC_Hours = 0;
	RTC_Time.RTC_Minutes = 0;
	RTC_Time.RTC_Seconds = 0;

	RTC_Date.RTC_Year = 0;
	RTC_Date.RTC_Date = 0;
	RTC_Date.RTC_Month = 0;

	RTC_SetTime(RTC_Format_BIN,&RTC_Time);
	RTC_SetDate(RTC_Format_BCD,&RTC_Date);
}

u8 get_current_user_rule(void){
	return dev.user.current_user_rule;;
}

void set_current_user_rule(u8 rule){
	dev.user.current_user_rule = rule;
}




/*get 96 byte device ID as string*/
void get_cpu_id_to_str(char *str){
u32 Data[3];
	  portENTER_CRITICAL();
	  memcpy(Data, (const void*)(0x1FFF7A10), 12);
	  portEXIT_CRITICAL();
	  sprintf(str,"%lx%lx%lx",Data[2],Data[1],Data[0]);
}


/*return 1 if cooper port*/
u8 is_cooper(u8 port){
	if(get_dev_type()==DEV_SWU16){
		if(port>=FIBER_PORT_NUM && port<ALL_PORT_NUM)
			return 1;
		else
			return 0;
	}
	else{
		if(port<COOPER_PORT_NUM)
			return 1;
		else
			return 0;
	}
	return 0;
}

u8 is_fiber(u8 port){
	if(get_dev_type()==DEV_SWU16){
		if(port<FIBER_PORT_NUM)
			return 1;
		else
			return 0;
	}
	else{
		if(port>=COOPER_PORT_NUM && port<ALL_PORT_NUM)
			return 1;
		else
			return 0;
	}
	return 0;
}

u8 is_gigabit(u8 port){
	if(get_dev_type() == DEV_SWU16){
		return 1;
	}else if(get_dev_type() == DEV_PSW1G4F || get_dev_type() == DEV_PSW1G4FUPS){
		if(port == 4 || port== 5)
			return 1;
		else
			return 0;
	}else{
		return is_fiber(port);
	}
	return 0;
}


void swu_io_config(void){

	//DEBUG_MSG(PRINTF_DEB,"swu_io_config\r\n");

	//switch interrupt done line
	//DEV_INIT_DONE
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	//EN_DEV line
	/*GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);*/

	//EN_DEV line
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	//sfp addr
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

}



//CPU statistics
void cpu_stat_processing(void){
static u8 init = 0;
	if(init == 0){
		for(u16 j=0;j<CPU_STAT_LEN;j++)
			dev.cpu_stat.cpu_level[j] = 0;
		init = 1;
	}

	for(u16 j=0;j<(CPU_STAT_LEN-1);j++){
		dev.cpu_stat.cpu_level[j] = dev.cpu_stat.cpu_level[j+1];
	}
	dev.cpu_stat.cpu_level[CPU_STAT_LEN-1] = GetCPU_IDLE();
}

u8 get_cpu_max(u16 period){
u8 max = 0;
	if(period <= CPU_STAT_LEN){
		for(u16 j=CPU_STAT_LEN-1;j>(CPU_STAT_LEN-period);j--){
			if(j < CPU_STAT_LEN){
				if(dev.cpu_stat.cpu_level[j]>max)
					max = dev.cpu_stat.cpu_level[j];
			}
		}
	}
	return max;
}

u8 get_cpu_min(u16 period){
u8 min = 100;
	if(period <= CPU_STAT_LEN){
		for(u16 j=CPU_STAT_LEN-1;j>(CPU_STAT_LEN-period);j--){
			if(j < CPU_STAT_LEN){
				if(dev.cpu_stat.cpu_level[j] < min)
					min = dev.cpu_stat.cpu_level[j];
			}
		}
	}
	return min;
}


void uip_debug_enable(void){
	uip_debug_state = 1;
}
void uip_debug_disable(void){
	uip_debug_state = 0;
}

u8 uip_debug_enabled(){
	return uip_debug_state;
}
