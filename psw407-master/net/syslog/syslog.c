#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "stm32f4xx_rtc.h"
#include "../deffines.h"
#include "../uip/uipopt.h"
#include "../uip/uip.h"
#include "../uip/uip_arp.h"
#include "../syslog/msg_build.h"
#include "../smtp/smtp.h"
#include "syslog.h"
#include "cfg_syslog.h"
#include "../sntp/sntp.h"
#include "../webserver/httpd-cgi.h"
#include "board.h"
#include "settings.h"
#include "names.h"
#include "../events/events_handler.h"
#include "selftest.h"
#include "debug.h"


 extern uint8_t dev_addr[6];
 extern uint16_t IDENTIFICATION;
 extern uint8_t MyIP[4];
 extern u8_t uip_buf[UIP_BUFSIZE + 8];
 extern uint16_t uip_len;
 extern uint8_t AppBuff[256];


 u8 send_syslog_flag=0;
 struct syslog_text_t syslog_txt;
 uint16_t syslog_len=0;

 struct uip_udp_conn *syslog_conn;

 struct timer  syslog_arp_timer;

 u8 syslog_param_init=0;

extern char mail_text[512+256];
extern char descr[64];

xQueueHandle SyslogQueue;//очередь для syslog сообщений


uint8_t syslog_send_msg(char *syslog_msg){
	DEBUG_MSG(SYSLOG_DEBUG,"syslog_send_msg\r\n");

	makesysloglen(syslog_msg);

#if DSA_TAG_ENABLE
		for (u8 t=0;t<syslog_len;t++)
			uip_buf[42+4+t]=syslog_msg[t];
#else
		for (u8 t=0;t<syslog_len;t++)
			uip_buf[42+t]=syslog_msg[t];
#endif
	uip_udp_send (syslog_len);
	return 0;
}

void addsysloghead(u8 level,char *buffer){
char temp[64];
uint8_t facility,severity;
//va_list args;

facility=16;//local 0
severity=0;//all

	/*поле HEADER*/
	sprintf(buffer,"<%d> ",(u8)(facility*8+level));

	/*поле TIMESTAMP*/
	if((/*settings.sntp_sett.state*/get_sntp_state()==ENABLE)&&(date.year)){
		switch(date.month){
			case 1:sprintf(temp,"Jan");break;
			case 2:sprintf(temp,"Feb");break;
			case 3:sprintf(temp,"Mar");break;
			case 4:sprintf(temp,"Apr");break;
			case 5:sprintf(temp,"May");break;
			case 6:sprintf(temp,"Jun");break;
			case 7:sprintf(temp,"Jul");break;
			case 8:sprintf(temp,"Aug");break;
			case 9:sprintf(temp,"Sep");break;
			case 10:sprintf(temp,"Oct");break;
			case 11:sprintf(temp,"Nov");break;
			case 12:sprintf(temp,"Dec");break;
		}
		strcat(buffer,temp);
		strcat(buffer," ");
		if(date.day<10)
			strcat(buffer," ");
		sprintf(temp,"%d",date.day);
		strcat(buffer,temp);
		strcat(buffer," ");
		sprintf(temp,"%02d:%02d:%02d",date.hour,date.min,date.sec);
		strcat(buffer,temp);
		strcat(buffer," ");
	}else
		//if sntp not run
	{
		sprintf(temp,"<%lu>",RTC_GetCounter());
		strcat(buffer,temp);
	}


	/*поле HOSTNAME*/
	sprintf(temp,"%d.%d.%d.%d ",
	(u8)uip_hostaddr[0],(u8)(uip_hostaddr[0]>>8),(u8)uip_hostaddr[1],(u8)(uip_hostaddr[1]>>8));
	strcat(buffer,temp);

	/*поле device descr*/
	if((descr[0]!=0)&&(descr[0]!=255)&&(descr[0]!=0x0a)&&(descr[0]!=0x0c)){
		strcat(buffer,"name=[");
		strcat(buffer,descr);
		strcat(buffer,"]");
	}
}

void makesysloglen(char *buffer){
syslog_len=strlen(buffer);
if(syslog_len%2){//делаем длину четной
	strcat(buffer," ");
	syslog_len++;
}
}



u8 syslog_init(void){
	uip_ipaddr_t addr;

	if(get_syslog_state() == ENABLE){
		  if(SyslogQueue == NULL){
			 //создаём очередь для отправки сообщений
			 //глубина очереди MSG_QUEUE_LEN
			 SyslogQueue = xQueueCreate(MSG_QUEUE_LEN,SYSLOG_MAX_LEN);
			 if(SyslogQueue == NULL){
				ADD_ALARM(ERROR_CREATE_SYSLOG_QUEUE);
				return 1;
			 }
		  }

		  get_syslog_serv(&addr);
		  if (syslog_conn){
		    uip_udp_remove (syslog_conn);
		    DEBUG_MSG(SYSLOG_DEBUG,"remove udp conn snmp\r\n");
		  }
		  if ((syslog_conn = uip_udp_new (&addr, HTONS (SYSLOG_PORT))))
		  {
			  uip_udp_bind (syslog_conn, HTONS (SYSLOG_PORT));
			  DEBUG_MSG(SYSLOG_DEBUG,"snmp conn created OK\r\n");
		  }
		  syslog_param_init = 1;
		  timer_set(&syslog_arp_timer,5000*MSEC);
		  return 0;

	}
	return 1;
}



void syslog_appcall(void){

  if(uip_hostaddr[0] == 0 && uip_hostaddr[1]==0)
	  return;

	if(syslog_param_init == 0){
		syslog_init();
	}
	//ecли в очереди что-то есть
	if(uxQueueMessagesWaiting(SyslogQueue)){
		//и запись в arp есть
		if(uip_arp_out_check(syslog_conn) == 0){
			if(xQueueReceive(SyslogQueue,&syslog_text2,0) == pdPASS ){
				DEBUG_MSG(SYSLOG_DEBUG,"syslog send msg\r\n");
				syslog_send_msg(syslog_text2.text);
			}
		}else{
			//send arp
			if(timer_expired(&syslog_arp_timer)){
				uip_udp_send(uip_len);
				DEBUG_MSG(SYSLOG_DEBUG,"syslog not send, arp msg\r\n");
				timer_reset(&syslog_arp_timer);
			}
		}
	}
}


void set_syslog_param_init(u8 state){
	syslog_param_init = state;
}
