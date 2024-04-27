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
#include "board.h"
#include "SMIApi.h"
#include "VLAN.h"
#include "../deffines.h"
#include "names.h"
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

#include  "debug.h"



//1 - символ
//0 - не символ
static u8 check_ch(char c){
	if((isalnum(c))||(c=='.')||(c==',')||
			(c==':')||(c=='-')||(c=='/')||(c=='+')||
			(c=='?')||(c=='=')||(c=='&')||(c=='!')||
			(c=='@')||(c=='#')||(c=='$')||(c=='%')||
			(c=='(')||(c==')')||(c=='"')||(c=='<')||(c=='>')){
		return 1;
	}
	else{
		return 0;
	}

}

void printf_arr(u8 type,u8 *buff,u32 size){
char temp[16];
char printf_buff[128];

	//printf("printf_arr\r\n");
	//печатаем по 20 символов в строке
	for(u8 i=0;i<(size/10);i++){
		printf_buff[0] = 0;
		for(u8 j=0;j<10;j++){
			if(check_ch(buff[i*10+j]) && type == TYPE_CHAR)
				sprintf(temp,"%02X[%c] ",buff[i*10+j],buff[i*10+j]);
			else
				sprintf(temp,"%02X ",buff[i*10+j]);
			strcat(printf_buff,temp);
		}
		printf("%s\r\n",printf_buff);
	}
	//для остатка
	printf_buff[0] = 0;
	for(u8 i=0;i<size%10;i++){
		if(check_ch(buff[i]) && type == TYPE_CHAR)
			sprintf(temp,"%02X[%c] ",buff[(size/10)*10+i],buff[(size/10)*10+i]);
		else
			sprintf(temp,"%02X ",buff[(size/10)*10+i]);
		strcat(printf_buff,temp);
	}
	printf("%s\r\n",printf_buff);
}
