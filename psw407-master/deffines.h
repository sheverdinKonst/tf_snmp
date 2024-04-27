#ifndef DEFFINES_H_
#define DEFFINES_H_

#include "stm32f4xx.h"
#include "FreeRTOSConfig.h"
#include <stdio.h>

u8 uip_debug_enabled(void);

#define date_of_fv __DATE__ //дата выпуска прошивки

#define DEV_PSW2G       	1
#define DEV_PSW2GUPS    	2
#define DEV_PSW2GPLUS   	3
#define DEV_PSW1G4F     	4
#define DEV_PSW2G4F			5
#define DEV_PSW2G6F			6
#define DEV_PSW2G8F			7
#define DEV_PSW1G4FUPS		8
#define DEV_PSW2G4FUPS		9
#define DEV_PSW2G2FPLUS		10
#define DEV_PSW2G2FPLUSUPS	11
#define DEV_SWU16			200

#define DEV_PSW1        	100
#define DEV_PSW145      	101
#define DEV_PSW11       	102
#define DEV_FC1         	103
#define DEV_FCG1        	104
#define DEV_FSE2G       	105
#define DEV_FSE2C       	106
#define DEV_FSE4B       	107
#define DEV_FD1H        	108

#define DEV_PSW				100//тип объединяет все PSW
#define DEV_TLP1			101
#define DEV_TLP2			102

//#define POEB_PORT_NUM	2//число портов с управлением PoE И у PSW2G6F


#define SALSA2_PHY_ADDR		0x10//PHY адрес микросхемы


/*********  debbug printf section ***********/
#define SMTP_DEBUG			0
#define SNMP_DEBUG 			0
#define SNTP_DEBUG 			0
#define SYSLOG_DEBUG 		0
#define IGMP_DEBUG 			0
#define IGMPV3_DEBUG		0
#define PSWS_DEBUG			0//отладка поиска устройств программой
#define BRIDGESTP_DEBUG 	0
#define STP_DEBUG 			0// отладочные сообщения для STP
//#define UIP_DEBUG 		0
#define DEBUG_QD 			0//marvell debug meaasge
#define ETH_DBG				0//ethernet debug
#define UPS_DEBUG 			0
#define PRINTF_DEB			1//1 - buildin printf -> usb com
#define DNS_DEBUG			0
#define TELNET_DEBUG		1
#define AUTORESTART_DBG		0
#define SETTINGS_DBG		0//настройка
#define BB_DBG				0//black box
#define EVENT_CREATE_DBG	0//event handler debug
#define POE_DBG				0//PoE processing
#define DHCP_DEBUG			0//DHCP client debug
#define VCT_DBG 			0//cable tester debug
#define AUTH_DBG			0//basic and digest web auth debug
#define RATE_LIM_DBG		0///rate limit debug
#define TFTP_DEBUG			0//TFTP Debug
#define LLDP_DEBUG			0//LLDP DEbug
#define RS485_DEBUG			0//RS485 debug
#define I2CSOFT_DEBUG		0//soft i2c
#define SSL_DEBUG			0//SSL Server Debug
#define TELEPORT_DEBUG 		0//teleport debug
#define DOT1X_DEBUG			0//IEEE 802.1X debug
#define MAC_FILTR_DEBUG		0//отладка защиты по маку
/************************************************/

#define UIP__HARD_DEBUG		0
#define UIP_DEBUG 			(UIP__HARD_DEBUG || uip_debug_enabled())


/******       включение функций *********/
#define BB_LOG   			1//включаем запись логов в черный ящик
#define USE_STP  			1
#define DSA_TAG_ENABLE  	1//dsa теги для stp
#define WATCHDOG_ENABLE 	1 // включаем вочдог
#define USE_FULL_ASSERT
#define DHCPR 				0//поддержка DHCP relay
#define LWIP_IGMP 			1//поддержка IGMP
#define PING_CONF 			1
#define SNMP_MANAGE			1//поддержка управления по SNMP
#define SFP_INFO 			1//получение по i2c информации о sfp
#define FLASH_FS_USE 		0//файлы help лежат на флешке
#define HELP_BKP_USE 		0//опция/ храним данные справки в 2-х местах
#define PPU_DIS 			1//отключить PPU
#define SAVE_SETT			1//сохранение настроек в файл
#define TELNET_USE			1//использование telnet
#define SSL_USE				0//ssl server
#define PSW1G_SUPPORT		1//включение поддержки psw1g в этой прошивке
#define POE_LIMIT 			0//включение поддержки ограничения мощности
#define IGMPV3_USE			0//IGMP V3
#define LLDP_USE			1//включение LLDP
#define LOOP_DET_USE		0//Loopback Detection
#define DOT1X_USE			0//IEEE 802.1X use


//не актуально, смотрим на перемычку ID0
#define PSW2GPLUS_BOOT_VERS	0x0100 // у PSW-2G+ - такая версия бутлоадера
								   // все что старше, считаем PSW-2G6F+

//выбор типа аутентификации для веб интерфейса
#define AUTH_DIGEST			1
#define AUTH_BASIC			0

#define TFTP_SERVER			0//включение TFTP сервера
/****************************************/






/****************************************/
// макросы, описывающие характеристики железа

//порты

#define MAX_PORT_NUM		16
#define PORT_NUM			MAX_PORT_NUM //(COOPER_PORT_NUM + FIBER_PORT_NUM)


extern u8 POE_PORT_NUM;
extern u8 POEB_PORT_NUM;
extern u8 COOPER_PORT_NUM;
extern u8 FIBER_PORT_NUM;
extern u8 CPU_PORT;
extern u8 MV_PORT_NUM;

#define ALL_PORT_NUM (COOPER_PORT_NUM + FIBER_PORT_NUM)


/****************************************/

#define RUS					1
#define ENG					0

#define NEED_UPDATE_YES		0xA1
#define NEED_UPDATE_NO		0xA0

#define NEED_DEFAULT_YES	0xB1
#define NEED_DEFAULT_NO		0xD0

#define MSEC (configTICK_RATE_HZ/1000)

//таймер перевода порта с sfp в форсированное состояние
#define SFP_SD_TIMER		5000 // 5sec

#define MAX_STR_LEN 		1000//размер "довеска" к размеру фрейма

#define HTTP_UPDATE_TIMER	240000//4мин


//TCP/IP Stack:
#define UIP 				1
#define LWIP				2


#define IP_STACK UIP

#ifndef IP_STACK
	#define IP_STACK	UIP
#endif


//статусы перезагрузки
#define LOW_POWER_RESET		1
#define WWDG_RESET 			2
#define IWDG_RESET			3
#define SOFT_RESET			4
#define POWER_RESET			5
#define BTN_RESET			6

//типы дефолтов
#define DEFAULT_ALL			0
#define DEFAULT_KEEP_NTWK	1
#define DEFAULT_KEEP_PASS	2
#define DEFAULT_KEEP_STP	4

#define ALL_PORT_ERROR (get_all_port_error()) // error хоть на одном из портов

#define NO_PORT_ERROR (get_all_port_error() == 0)


/*******************************/
/*    Switch Controller        */
/*******************************/
// Virtual Cable Tester
#define LinkStateMask 0x400
#define VCTCompleatMask 0x8000
// loop
#define ED_MDI_DisableMask 0xBFCF
#define AddresLearningDisMask 0xF7FF
#define DisAnegMask 0xEFFF
#define ForceSpeedFDMask 0x2100
#define VCTCompleatMask 0x8000
#define LoopModeMask 0x4000
//interrupt
#define PHYIntMask 0x0002
#define GlobalRegisters 0x1B
//counters
#define NoFlushAllMAsk 0x8FFF
#define NoFlushAllMAsk2 0x5000

/******************************/
/*Reset
*******************************/
#define LowPowerResetMask 0x80000000
#define WWDGResetMask 0x40000000//Window watchdog reset flag
#define IWDGResetMask 0x20000000// Independent watchdog reset flag
#define SoftResetMask 0x10000000
#define PowerResetMask 0x8000000// hard reset//power off
#define PinNRSTResetMask 0x4000000//сброс по кнопке


#define firmvare_addr 0 //адрес обновления

#define HTTPD_MAX_LEN_PASSWD 20

#define UPS_CAPACITY 2200*48*60*60//емкость АКБ в мBт*сек
//#define PSW_POWER 9000//потребление PSW в мВт ()
#define RIP_EFFECTIVITY 85//эффективность преобразователя в плате IPR

#define IRP_ADDR 0x02


#endif /* DEFFINES_H_ */
