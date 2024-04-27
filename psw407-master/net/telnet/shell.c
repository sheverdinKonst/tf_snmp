 /*
 * Copyright (c) 2003, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: shell.c,v 1.1 2006/06/07 09:43:54 adam Exp $
 *
 */
#include "../deffines.h"
#if TELNET_USE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "stm32f4xx.h"
#include "telnetd.h"
#include "shell.h"
#include "../uip/uip.h"
#include "board.h"
#include "settings.h"
#include "../net/dhcp/dhcp.h"
#include "../net/stp/bridgestp.h"
#include "../net/uip/uip.h"
#include "../net/uip/uip_arp.h"
#include "stm32f4xx_rtc.h"
#include "eeprom.h"
#include "spiflash.h"
#include "../net/sntp/sntp.h"
#include "../net/syslog/syslog.h"
#include "../net/smtp/smtp.h"
#include "../net/snmp/snmpd/snmpd.h"
#include "../net/snmp/snmp.h"
#include "SMIApi.h"
#include "poe_ltc.h"
#include "task.h"
#include "../net/uip/timer.h"
#include "../net/tftp/tftpclient.h"
#include <ctype.h>
#include "../net/webserver/httpd-cgi.h"
#include "settingsfile.h"
#include "autorestart.h"
#include "debug.h"
#include "SpeedDuplex.h"
#include "FlowCtrl.h"
#include "QoS.h"
#include "VLAN.h"

#include "../plc/em.h"
#include "../plc/plc.h"
#include "../plc/plc_def.h"

//#include "appdefs.h"
//#include "rtl8201cp.h"
//#include "can_test.h"

struct ptentry {
  char *commandstr;
  void (* pfunc)(struct telnetd_state *s,char *str);
};


static u8 incorrect_autc_cnt = 0;
struct timer telnet_timeout;


u8 start_ping_flag = 0;
u8 send_ping_flag = 0;
u8 start_show_fdb = 0;
u8 start_show_vlan = 0;

u8 start_make_bak = 0;
u32 offset_make_bak = 0;
u32 backup_file_len = 0;

struct timer ping_timer;

struct tftp_proc_t tftp_proc;

#define SHELL_USERNAME "User Name>"
#define SHELL_PASSWORD "User Password>"

#define ISO_up		0x26

extern u8 dev_addr[6];
extern u32 image_version[1];
extern struct status_t status;

extern xTaskHandle xOtherCommands;
extern uint8_t OtherCommandsFlag;
extern uint8_t SendICMPPingFlag;
extern uip_ipaddr_t IPDestPing;

//extern struct command_queue_t queue;

u8 start_vct;
u8 port_vct;

char SHELL_PROMPT[32];

//char cmd_last[TELNETD_CONF_LINELEN];

static u8 parse_name(struct telnetd_state *s,char *cmd);
static u8 parse_pass(struct telnetd_state *s,char *cmd);
/*
static void http_url_decode(char *in,char *out,uint8_t mx){
  uint8_t i,j;
  char tmp[3]={0,0,0};
  for(i=0,j=0;((j<mx)&&(in[i]));i++)
	{
  	if(in[i]=='%'){
		tmp[0]=in[++i];
		tmp[1]=in[++i];
		out[j++]=(char)strtol(tmp,0,16);
	}else
		out[j++]=in[i];
	}
}
*/

void promt_print(struct telnetd_state *s){

	shell_prompt(s,SHELL_PROMPT);
	if(s->user_rule == ADMIN_RULE)
		shell_prompt(s,"#");
	else
		shell_prompt(s,">");
}


static char *ports_print(u8 *ports){
u8 i,inport=0,outport=0;
static char str[16];

	for(i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
		if(ports[i]){
			inport=i;
			break;
		}
	}
	outport = inport;
	for(i=inport;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
		if(ports[i]){
			outport=i;
		}
	}
	sprintf(str,"%d-%d",inport+1,outport+1);
	return str;
}

/*str1 - input string
 * str2 - command code string*/
static char *findcmd(struct telnetd_state *s,char *str1,char *str2){
u8 i;
char *ptr1;
u8 len;

DEBUG_MSG(TELNET_DEBUG && !s->usb,"findcmd: %s||%s||%d\r\n",str1,str2,s->tab);

	if(strcmp(str2," ")==0 && strlen(str2)==1){
		return strstr(str1,str2)+1;
	}

	//1.serch first word
	for(i=0;i<strlen(str1);i++){
		if(isalnum((int)(str1[i])))//если символ
			break;
	}
	ptr1 = str1 + i;
	//2.определяем длину слова
	len = strlen(ptr1);
	for(i=0;i<strlen(ptr1);i++){
		//если уже не символ
		if(!isalnum((int)(ptr1[i])) && ptr1[i]!='_' && ptr1[i]!='/' && ptr1[i]!='.'&& ptr1[i]!='-'){
			len = i;
			break;
		}
	}

	if(len==0)
		return NULL;

	//3. сравнение
	for(i=0;i<len;i++){
		if(ptr1[i] != str2[i])
			return NULL;
	}

	//DEBUG_MSG(TELNET_DEBUG && !s->usb,"findcmd %d %s\r\n",len,ptr1+len);

	if(s->tab){
		shell_prompt(s,"\r");
		promt_print(s);
		strcat(s->buf,&str2[len]);
		shell_prompt(s,s->buf);
		s->bufptr  += (strlen(str2)-len);
		shell_push_command(s);//add to command history
		return ptr1 + strlen(str2);
	}else
		return ptr1+len;

	return NULL;
}

static char *findmac(struct telnetd_state *s,char *str1,u8 *mac){
char tmp[3];
char *ptr,*ptr1;
u32 m1;
	for(u8 i=0; i<strlen(str1);i++){
		if(*str1 == ' ')
			str1++;
		else
			break;
	}
	ptr = str1;
	for(u8 i=0;i<6;i++){

		DEBUG_MSG(TELNET_DEBUG && !s->usb,"findmac %s\r\n",ptr);

		m1 = strtol(ptr,&ptr,16);
		if(m1==4294967295)
			return NULL;
		mac[i] = (u8)m1;

		ptr1 = strstr(ptr,":");
		if(ptr1 == NULL && i<5)
			return 0;
		ptr1++;

		DEBUG_MSG(TELNET_DEBUG && !s->usb,"findmac mac[%d]=%X\r\n",i,mac[i]);
		ptr = ptr1;
	}
	return ptr;
}

//find one word in string
static u8 findword(char *outstr, char *instr,u8 maxlen){
u8 i=0,j=0;
	//find first char

	//1.serch first word
	for(i=0;i<strlen(instr);i++){
		if(isalnum((int)(instr[i])))//если символ
			break;
	}

	//
	for(;i<strlen(instr);i++){
		if(isalnum((int)(instr[i]))){
			outstr[j++] = instr[i];
			if(j>maxlen)
				return 1;
		}
		else
			break;

	}

	outstr[j] = 0;

	if(j)
		return 1;
	else
		return 0;

	//DEBUG_MSG(TELNET_DEBUG && !s->usb,"findword:%s\r\n",outstr);
}


static u8 autocompleat(struct telnetd_state *s){

	DEBUG_MSG(TELNET_DEBUG && !s->usb,"autocompleat:%s\r\n",s->buf);

	if(s->tab)
		return 1;
	else
		return 0;
}



u8 find_ip(char *str,uip_ipaddr_t *ip){
	// atoi(str)
	char *ptr1;
	u32 tip[4];

	//DEBUG_MSG(TELNET_DEBUG && !s->usb,"find_ip %s\r\n",str);
	//1
	//ptr1 = strstr(str,".");
	//if(ptr1 == NULL)
	//	return 0;
	//ptr1-=3;
	tip[0] = strtol(str/*ptr1*/,&ptr1,10);
	if((tip[0]==4294967295))
		return 0;
	//2
	ptr1 = strstr(ptr1,".");
	if(ptr1 == NULL)
		return 0;
	tip[1] = strtol(ptr1+1,&ptr1,10);
	if((tip[1]==4294967295))
		return 0;
	//3
	ptr1 = strstr(ptr1,".");
	if(ptr1 == NULL)
		return 0;
	tip[2] = strtol(ptr1+1,&ptr1,10);
	if((tip[2]==4294967295))
		return 0;
	//4
	ptr1 = strstr(ptr1,".");
	if(ptr1 == NULL)
		return 0;
	tip[3] = strtol(ptr1+1,&ptr1,10);
	if((tip[3]==4294967295))
		return 0;
	uip_ipaddr(ip,(u8)tip[0],(u8)tip[1],(u8)tip[2],(u8)tip[3]);
	//DEBUG_MSG(TELNET_DEBUG && !s->usb,"str2ip convert %lu.%lu.%lu.%lu\r\n",tip[0],tip[1],tip[2],tip[3]);

	//modify ptr
	str = ptr1;
	return 1;
}

char *find_ip_ptr(char *str,uip_ipaddr_t *ip){
	// atoi(str)
	char *ptr1;
	u32 tip[4];

	//DEBUG_MSG(TELNET_DEBUG && !s->usb,"find_ip_ptr %s\r\n",str);

	//1
	ptr1 = strstr(str,".");
	if(ptr1 == NULL)
		return NULL;
	ptr1-=3;
	tip[0] = strtol(ptr1,&ptr1,10);
	if((tip[0]==4294967295))
		return NULL;
	//2
	ptr1 = strstr(ptr1,".");
	if(ptr1 == NULL)
		return NULL;
	tip[1] = strtol(ptr1+1,&ptr1,10);
	if((tip[1]==4294967295))
		return NULL;
	//3
	ptr1 = strstr(ptr1,".");
	if(ptr1 == NULL)
		return NULL;
	tip[2] = strtol(ptr1+1,&ptr1,10);
	if((tip[2]==4294967295))
		return NULL;
	//4
	ptr1 = strstr(ptr1,".");
	if(ptr1 == NULL)
		return NULL;
	tip[3] = strtol(ptr1+1,&ptr1,10);
	if((tip[3]==4294967295))
		return NULL;
	uip_ipaddr(ip,(u8)tip[0],(u8)tip[1],(u8)tip[2],(u8)tip[3]);
	//DEBUG_MSG(TELNET_DEBUG && !s->usb,"str2ip convert %lu.%lu.%lu.%lu\r\n",tip[0],tip[1],tip[2],tip[3]);
	return ptr1;
}

//поиск номеров портов // вида "1-2" // либо 1
static char * find_port(char *str, u8 *ports){
char *ptr;
u32 inport,outport;


	DEBUG_MSG(TELNET_DEBUG,"find_port = %s\r\n",str);

	if(strlen(str) == 0)
		return NULL;

	inport = strtol(str,NULL,10);
	//DEBUG_MSG(TELNET_DEBUG && !s->usb,"inport = %lu\r\n",inport);

	if(inport==4294967295)
		return NULL;
	ptr = strstr(str,"-");
	if(ptr == NULL){
		//значит другой формат
		if((inport<=ALL_PORT_NUM)&&(inport)){
			for(u8 i=0;i<ALL_PORT_NUM;i++){
				if((i+1)==inport)
					ports[i]=1;
				else
					ports[i]=0;
				//DEBUG_MSG(TELNET_DEBUG && !s->usb,"port[%d] = %d\r\n",i,ports[i]);
			}
			return str+1;
		}
		else{
			return NULL;
		}
	}
	//DEBUG_MSG(TELNET_DEBUG && !s->usb,"%s\r\n",ptr);

	outport = strtol(ptr+1,&ptr,10);
	//DEBUG_MSG(TELNET_DEBUG && !s->usb,"outport = %lu\r\n",outport);
	if(outport==4294967295)
		return NULL;

	if(inport>outport){
		return NULL;
	}

	if(inport>ALL_PORT_NUM){
		return NULL;
	}

	if(outport>ALL_PORT_NUM){
		return NULL;
	}

	if((outport-inport)>ALL_PORT_NUM){
		return NULL;
	}

	for(u8 i=0;i<=ALL_PORT_NUM;i++){
		if(((i+1)>=inport)&&((i+1)<=outport))
			ports[i]=1;
		else
			ports[i]=0;
		DEBUG_MSG(TELNET_DEBUG,"port[%d] = %d\r\n",i,ports[i]);
	}
	return ptr;
}

//парсинг интервалов // вида "1-20"
static char * find_interval(char *str,u32 *first,u32 *last){
char *ptr1, *ptr2, *ptr3;



	//DEBUG_MSG(TELNET_DEBUG && !s->usb,"inport_str = %s\r\n",str);

	if(strlen(str) == 0)
		return NULL;

	*first = strtol(str,&ptr1,10);
	//DEBUG_MSG(TELNET_DEBUG && !s->usb,"inport = %lu\r\n",inport);

	if(*first==4294967295)
		return NULL;


	ptr2 = strstr(ptr1,"-");
	if(ptr2 == NULL){
		*last = *first;
		return ptr1;
	}

	//проверяем, что тире без пробелов
	if((ptr2 - ptr1)>2){
		*last = *first;
		return ptr1;
	}

	//DEBUG_MSG(TELNET_DEBUG && !s->usb,"%s\r\n",ptr);

	*last = strtol(ptr2+1,&ptr3,10);

	if(*last==4294967295)
		return NULL;

	if(*first>*last){
		return NULL;
	}

	DEBUG_MSG(TELNET_DEBUG,"first %lu, last = %lu\r\n",*first,*last);

	return ptr3;
}


/*---------------------------------------------------------------------------*/
/*static void
parse(struct telnetd_state *s,register char *str, struct ptentry *t)
{
  struct ptentry *p;
  for(p = t; p->commandstr != NULL; ++p) {
    if(strncmp(p->commandstr, str, strlen(p->commandstr)) == 0)
    {
      str = str + strlen(p->commandstr) ;
      break;
    }
  }
  p->pfunc(s,str);
}*/
/*---------------------------------------------------------------------------*/
//парсинг коротких имен
static void
parse_short(struct telnetd_state *s,char *str, struct ptentry *t){
  struct ptentry *p;
  u8 len=0,len1,len2,i;

  len2 = (u8)strlen(str);
  for(i=0;i<len2;i++){
	  if(str[i]==' ')
		  break;
  }
  len2 = i;

  DEBUG_MSG(TELNET_DEBUG && !s->usb,"parse_short len2 %d, str:%s \r\n",len2,str);
  for(p = t; p->commandstr != NULL; ++p) {
	len1 = (u8)strlen(p->commandstr);
	if(len1>len2)
		len=len2;
	else
		len = len1;

	if(len){
		if(strncmp(p->commandstr, str, len) == 0){

		  //если необходимо автодополнение
	      if((s->tab)&&(len2<strlen(p->commandstr))){
			  DEBUG_MSG(TELNET_DEBUG && !s->usb,"auto compleat1 %s\r\n",&p->commandstr[len]);
			  shell_prompt(s,"\r");
			  promt_print(s);
			  strcpy(s->buf,p->commandstr);
			  s->bufptr += (strlen(p->commandstr)-len2);
			  shell_push_command(s);//add to command history
			  shell_prompt(s,s->buf);
			  return;
		  }
		  else{
			  //if((s->tab)&&(len2 == strlen(p->commandstr)))
			  //	  return;
			  //указатель на второе слово
			  str = str + len;

			  //s->bufptr += len2;
		  }
		  break;
		}
	}
  }

  p->pfunc(s,str);
}

//--------------------------------------------------------------------------
static void
help(struct telnetd_state *s,char *str)
{
  shell_output(s,"", "");
  shell_output(s,"Available commands:", "");
  shell_output(s,"------------------", "");
  if(s->user_rule==ADMIN_RULE)
	  shell_output(s,"config\t\t- configuration group","");
  shell_output(s,"show\t\t- show information group ","");
  if(s->user_rule==ADMIN_RULE)
	  shell_output(s,"download\t- download firmware, config","");
  if(s->user_rule==ADMIN_RULE)
	  shell_output(s,"upload\t\t- upload config,log","");
  shell_output(s,"ping\t\t- ping remote address","");
  shell_output(s,"cable_diag\t- Virtual Cable Tester","");
  if(s->user_rule==ADMIN_RULE)
	  shell_output(s,"save\t\t- save configuration","");
  if(s->user_rule==ADMIN_RULE)
	  shell_output(s,"reboot\t\t- reboot switch","");

  shell_output(s,"default\t\t- set default settings", "");

  shell_output(s,"help, ?\t\t- show help", "");
  shell_output(s,"exit\t\t- exit shell", "");
  shell_output(s,"", "");
}

static void save_sett(struct telnetd_state *s,char *str){
//u8 type=0,type_last=0;
	if(s->user_rule!=ADMIN_RULE){
		shell_output(s,"ERROR: Access denied","");
		return;
	}

	DEBUG_MSG(TELNET_DEBUG && !s->usb,"Save Settings\r\n");

	settings_save();

	shell_output(s,"Settings saved successfully","");

}

static void reboot_conn(struct telnetd_state *s,char *str){
	if(s->user_rule!=ADMIN_RULE){
		shell_output(s,"ERROR: Access denied","");
		return;
	}
  shell_output(s,"Rebooted....","");
  shell_output(s,"connect closed","");
  shell_quit(s,str);
  reboot(REBOOT_ALL);
}

static void default_sett(struct telnetd_state *s,char *str){
	if(s->user_rule!=ADMIN_RULE){
		shell_output(s,"ERROR: Access denied","");
		return;
	}
	shell_output(s,"Settings set to default....","");
	shell_quit(s,str);
	settings_default(DEFAULT_ALL);
}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static void unknown(struct telnetd_state *s,char *str)
{
  if(strlen(str) > 2) { //if unknown command adn not \10\13
	  help(s,str);
  }
}



//----------------------------------------------------------------------------
/**/
static void show(struct telnetd_state *s,char *str){
	char *ptr,*ptr2,*ptr1;
	char temp[256];
	char temp2[128];
	char tmp[16];
	u8 ports[PORT_NUM];
	u32 temp_u32;
	RTC_TimeTypeDef RTC_Time;
	RTC_DateTypeDef RTC_Date;
	uip_ipaddr_t ip;
	int start;
	u16 i;
	u32 Temp;
	u32 tot_pwr;
	u16 vid;
	engine_id_t eid;
	struct mac_entry_t entry;

	DEBUG_MSG(TELNET_DEBUG && !s->usb,"command show %s\r\n", str);

	//if empty line
	if (!strlen(str))
	{
		if(autocompleat(s)){
			return;
		}
		shell_output(s,"Command: show","");
		shell_output(s,"","");
		if(get_dev_type()==DEV_SWU16){
			shell_output(s,"switch\t\t\tports\t\t\tigmp_snooping","");
			shell_output(s,"stp\t\t\tsnmp\t\t\tsyslog","");
			shell_output(s,"vlan\t\t\tsntp\t\t\tsmtp","");
			shell_output(s,"firmware\t\tpacket\t\t\tfdb", "");
			shell_output(s,"arpentry\t\tmirroring\t\taggregation", "");
			shell_output(s,"mac_filtering\t\ttftp\t\t\tevents", "");
			shell_output(s,"802.1p\t\t\tscheduling_mechanism\tdscp_mapping", "");
			shell_output(s,"bandwidth_control\tconfig", "");
		}
		else{
			shell_output(s,"switch\t\t\tports\t\t\tigmp_snooping","");
			shell_output(s,"stp\t\t\tsnmp\t\t\tsyslog","");
			shell_output(s,"vlan\t\t\tsntp\t\t\tsmtp","");
			shell_output(s,"firmware\t\tpacket\t\t\tfdb", "");
			shell_output(s,"arpentry\t\tautorestart\t\tcomfortstart", "");
			shell_output(s,"dry_cont\t\ttftp\t\t\tevents", "");
			shell_output(s,"802.1p\t\t\tscheduling_mechanism\tdscp_mapping", "");
			shell_output(s,"bandwidth_control\tpoe\t\t\tconfig", "");
			shell_output(s,"mac_filtering\t\tinputs\t\t\toutputs","");
			shell_output(s,"rs485\t\t\tteleport\t\tlldp","");
		}
		shell_output(s,"", "");
		return;
	}

	ptr = findcmd(s,str, "switch");
	if(ptr != NULL){

		if(autocompleat(s)){
			return;
		}

		shell_output(s,"Command: show switch","");

		get_dev_name(temp);
		shell_output(s,"Device type:\t\t",temp);

		sprintf(temp,"%02X:%02X:%02X:%02X:%02X:%02X",
				dev_addr[0],dev_addr[1],dev_addr[2],dev_addr[3],dev_addr[4],dev_addr[5]);
		shell_output(s,"MAC Address:\t\t",temp);

		sprintf(temp,"%d.%d.%d.%d",
				uip_ipaddr1(uip_hostaddr),uip_ipaddr2(uip_hostaddr),uip_ipaddr3(uip_hostaddr),uip_ipaddr4(uip_hostaddr));
		shell_output(s,"IP Address:\t\t",temp);

		sprintf(temp,"%d.%d.%d.%d",
				uip_ipaddr1(uip_netmask),uip_ipaddr2(uip_netmask),uip_ipaddr3(uip_netmask),uip_ipaddr4(uip_netmask));
		shell_output(s,"Subnet Mask:\t\t",temp);

		sprintf(temp,"%d.%d.%d.%d",
				uip_ipaddr1(uip_draddr),uip_ipaddr2(uip_draddr),uip_ipaddr3(uip_draddr),uip_ipaddr4(uip_draddr));
		shell_output(s,"Default Gateway:\t",temp);

		sprintf(temp,"%02X.%02X.%02X",
				 (int)(image_version[0]>>16)&0xff,
				 (int)(image_version[0]>>8)&0xff,  (int)(image_version[0])&0xff);
		shell_output(s,"Firmware Version:\t",temp);

		sprintf(temp,"%02X.%02X",(int)(bootloader_version>>8)&0xff,  (int)(bootloader_version)&0xff);
		shell_output(s,"Bootloader Version:\t",temp);

	    sprintf(temp,"%d",(int)(dev_addr[4]<<8 | (uint8_t)dev_addr[5]));
		shell_output(s,"Serial Number:\t\t",temp);

		memset(temp,0,sizeof(temp));
		get_interface_name(temp2);
		http_url_decode(temp2,temp,strlen(temp2));
		for(uint8_t i=0;i<strlen(temp);i++){
			if(temp[i]=='+') temp[i] = ' ';
			if(temp[i]=='%') temp[i]=' ';
		}
		shell_output(s,"Device Description:\t",temp);

		memset(temp,0,sizeof(temp));
		get_interface_location(temp2);
		http_url_decode(temp2,temp,strlen(temp2));
		for(uint8_t i=0;i<strlen(temp);i++){
			if(temp[i]=='+') temp[i] = ' ';
			if(temp[i]=='%') temp[i]=' ';
		}
		shell_output(s,"Device Location:\t",temp);

		memset(temp,0,sizeof(temp));
		get_interface_contact(temp2);
		http_url_decode(temp,temp,strlen(temp2));
		for(uint8_t i=0;i<strlen(temp);i++){
			if(temp[i]=='+') temp[i] = ' ';
			if(temp[i]=='%') temp[i]=' ';
		}
		shell_output(s,"Device Contact:\t\t",temp);

		RTC_GetTime(RTC_Format_BIN,&RTC_Time);
		RTC_GetDate(RTC_Format_BCD,&RTC_Date);
		sprintf(temp,"%dd. %dh. %dm. %ds",(RTC_Date.RTC_Date),(RTC_Time.RTC_Hours),(RTC_Time.RTC_Minutes),(RTC_Time.RTC_Seconds));
		shell_output(s,"System Uptime:\t\t",temp);

	 	if(get_stp_state()==1){
			if(get_stp_proto()==2)
				 strcpy(temp,"RSTP");
			 else
				 strcpy(temp,"STP");
		}
		else{
			 strcpy(temp,"disable");
		}
	 	shell_output(s,"Spanning Tree:\t\t",temp);

	 	if(get_igmp_snooping_state() == 1)
	 		shell_output(s,"IGMP Snooping:\t\t","enable");
	 	else
	 		shell_output(s,"IGMP Snooping:\t\t","disable");

	 	if(get_syslog_state() == 1)
	 		shell_output(s,"Syslog:\t\t\t","enable");
	 	else
	 		shell_output(s,"Syslog:\t\t\t","disable");

	 	if(get_smtp_state() == 1)
	 		shell_output(s,"SMTP:\t\t\t","enable");
	 	else
	 		shell_output(s,"SMTP:\t\t\t","disable");


		sprintf(temp,"%d",get_vlan_sett_vlannum());
		shell_output(s,"VLAN:\t\t\t",temp);


		memset(temp,0,sizeof(temp));
		for(u8 i=0;i<COOPER_PORT_NUM;i++){
			if(get_port_sett_soft_start(i)==1){
				if(get_dev_type() == DEV_PSW2GPLUS){
					switch(i){
						case 0:strcat(temp,"FE#1 ");break;
						case 1:strcat(temp,"FE#2 ");break;
						case 2:strcat(temp,"FE#3 ");break;
						case 3:strcat(temp,"FE#4 ");break;
					}
				}else{
					sprintf(tmp,"%d ",i+1);
					strcat(temp,tmp);
				}
			}
		}
		if(strlen(temp)){
			shell_output(s,"Comfort Start:\t\t",temp);
		}

		//autorestart
		memset(temp,0,sizeof(temp));
		for(u8 i=0;i<COOPER_PORT_NUM;i++){
			if(get_port_sett_wdt(i)){
				if(get_dev_type() == DEV_PSW2GPLUS){
					switch(i){
						case 0:strcat(temp,"FE#1 ");break;
						case 1:strcat(temp,"FE#2 ");break;
						case 2:strcat(temp,"FE#3 ");break;
						case 3:strcat(temp,"FE#4 ");break;
					}
				}else{
					sprintf(tmp,"%d ",i+1);
					strcat(temp,tmp);
				}
			}
		}
		if(strlen(temp)){
			shell_output(s,"Auto Restart:\t\t",temp);
		}


		//aggregation
		if(get_lag_entries_num()){
			 shell_output(s,"Link Aggregation:\t\t","Enabled");
		}

		//mirroring
		if(get_mirror_state()){
			shell_output(s,"Port Mirroring:\t\t","Enabled");
		}

		if(get_gratuitous_arp_state()){
			shell_output(s,"Gratuitous ARP:\t\t","Enabled");
		}

		return;
	}

	ptr = findcmd(s,str,"smtp");
	if(ptr!=NULL){
		if(autocompleat(s)){
			return;
		}

		shell_output(s,"Command: show smtp","");
		if(get_smtp_state() == 1)
			shell_output(s,"SMTP State:\t\t","enable");
		else
			shell_output(s,"SMTP State:\t\t","disable");

		get_smtp_server(&ip);
		sprintf(temp,"%d.%d.%d.%d",
				uip_ipaddr1(ip),uip_ipaddr2(ip),uip_ipaddr3(ip),uip_ipaddr4(ip));
		shell_output(s,"SMTP Server IP:\t\t",temp);

		get_smtp_domain(temp);
		shell_output(s,"SMTP Server Name:\t",temp);

		temp_u32 = get_smtp_port();
		sprintf(temp,"%d",(u16)temp_u32);
		shell_output(s,"SMTP Server Port:\t",temp);

		get_smtp_from(temp);
		shell_output(s,"Mail Sender:\t\t",temp);

		get_smtp_to(temp);
		shell_output(s,"Mail Receiver 1:\t",temp);
		get_smtp_to2(temp);
		shell_output(s,"Mail Receiver 2:\t",temp);
		get_smtp_to3(temp);
		shell_output(s,"Mail Receiver 3:\t",temp);

		get_smtp_login(temp);
		shell_output(s,"SMTP Login:\t\t",temp);

		get_smtp_pass(temp);
		shell_output(s,"SMTP Password:\t\t",temp);
		return;
	}

	ptr = findcmd(s,str,"stp");
	if(ptr!=NULL){

		if(autocompleat(s)){
			return;
		}

		shell_output(s,"Command: show stp","");
		if(get_stp_state()==1)
			shell_output(s,"STP State:\t\t","enable");
		else
			shell_output(s,"STP State:\t\t","disable");

		if(get_stp_proto()==2)
			shell_output(s,"STP Version:\t\t","RSTP");
		 else
			shell_output(s,"STP Version:\t\t","STP");

		temp_u32 = get_stp_bridge_max_age();
		sprintf(temp,"%d",(u8)temp_u32);
		shell_output(s,"Max Age:\t\t",temp);

		temp_u32 = get_stp_bridge_htime();
		sprintf(temp,"%d",(u8)temp_u32);
		shell_output(s,"Hello Time:\t\t",temp);

		temp_u32 = get_stp_bridge_fdelay();
		sprintf(temp,"%d",(u8)temp_u32);
		shell_output(s,"Forward Delay:\t\t",temp);

		temp_u32 = get_stp_txholdcount();
		sprintf(temp,"%d",(u8)temp_u32);
		shell_output(s,"TX Hold Count:\t\t",temp);
		return;
	}

	ptr = findcmd(s,str,"vlan");
	if(ptr!=NULL){

		ptr2 = findcmd(s,ptr,"all");
		if(ptr2!=NULL){
			if(autocompleat(s)){
				return;
			}
			shell_output(s,"Command: show vlan all","");
			start_show_vlan = 1;
			return;
		}
		else{
			if(autocompleat(s)){
				return;
			}
			ptr2 = findcmd(s,ptr," ");
			temp_u32 = strtoul(ptr2,NULL,10);
			  if((temp_u32 > 4095)||(temp_u32 == 0)){
				  shell_output(s,"ERROR: VLAN: Incorrect VID","");
				  return;
			  }
			  else{
				  vid = temp_u32;

				  if(vlan_vid2num(vid)==-1){
					  shell_output(s,"ERROR: VLAN: Incorrect VID","");
					  return;
				  }

				  sprintf(temp,"%d",vid);
				  shell_output(s,"Command: show vlan ",temp);
				  shell_output(s,"VID:\t\t\t",temp);

				  shell_output(s,"VLAN Name:\t\t",get_vlan_name(vlan_vid2num(vid)));

				  shell_output(s,"VLAN Type:\t\t","Static");

				  memset(temp,0,sizeof(temp));
				  for(u8 j=0;j<(ALL_PORT_NUM);j++){
					  if(get_vlan_port_state(vlan_vid2num(vid),j)==3){
							sprintf(temp2,"%d ",j+1);
							strcat(temp,temp2);
					  }
				  }
				  shell_output(s,"Tagged Ports:\t\t",temp);

				  memset(temp,0,sizeof(temp));
				  for(u8 j=0;j<(ALL_PORT_NUM);j++){
					  if(get_vlan_port_state(vlan_vid2num(vid),j)==2){
						  sprintf(temp2,"%d ",j+1);
						  strcat(temp,temp2);
					  }
				  }
				  shell_output(s,"Untagged Ports:\t\t",temp);

				  memset(temp,0,sizeof(temp));
				  for(u8 j=0;j<(ALL_PORT_NUM);j++){
					  if(get_vlan_port_state(vlan_vid2num(vid),j)==0){
						  sprintf(temp2,"%d ",j+1);
						  strcat(temp,temp2);
					  }
				  }
				  shell_output(s,"Not a member Ports:\t",temp);
				  shell_output(s,"","");
				  return;

			  }
		}
	}

	ptr = findcmd(s,str,"igmp_snooping");
	if(ptr!=NULL){
		if(autocompleat(s)){
			return;
		}
		shell_output(s,"Command: show igmp_snooping","");
	 	if(get_igmp_snooping_state() == 1)
	 		shell_output(s,"IGMP Snooping State:\t\t","enable");
	 	else
	 		shell_output(s,"IGMP Snooping State:\t\t","disable");

	 	memset(temp,0,sizeof(temp));
		for(u8 j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
			if(get_igmp_port_state(j)==1){
				sprintf(temp2,"%d ",j);
				strcat(temp,temp2);
			}
		}
		shell_output(s,"IGMP Snooping Ports:\t\t",temp);
		return;
	}

	ptr = findcmd(s,str,"snmp");
	if(ptr!=NULL){
		if(autocompleat(s)){
			return;
		}
		shell_output(s,"Command: show snmp","");
	 	if(get_snmp_state() == 1)
	 		shell_output(s,"SNMP State:\t\t","enable");
	 	else
	 		shell_output(s,"SNMP State:\t\t","disable");

	 	if(get_snmp_vers() == 1)
	 		shell_output(s,"SNMP version:\t\t","1");
	 	else
	 		shell_output(s,"SNMP version:\t\t","3");

	 	get_snmp_serv(&ip);
		sprintf(temp,"%d.%d.%d.%d",
				uip_ipaddr1(ip),uip_ipaddr2(ip),uip_ipaddr3(ip),uip_ipaddr4(ip));
		shell_output(s,"SNMP Server IP:\t\t",temp);

		if(get_snmp_vers() == 1){
			get_snmp1_read_communitie(temp);
			shell_output(s,"SNMP Read community:\t",temp);

			get_snmp1_write_communitie(temp);
			shell_output(s,"SNMP Write community:\t",temp);
		}
		else{
			get_snmp3_engine_id(&eid);
			engineid_to_str(&eid,temp);
			shell_output(s,"SNMP Engine ID:\t\t",temp);

			if(get_snmp3_level(0)==0)
				shell_output(s,"Security Level:\t\t","NoAuth,NoPriv");
			else if(get_snmp3_level(0)==1)
				shell_output(s,"Security Level:\t\t","Auth,NoPriv");
			else if(get_snmp3_level(0)==2)
				shell_output(s,"Security Level:\t\t","Auth,Priv");

			get_snmp3_user_name(0,temp);
			shell_output(s,"User Name:\t\t",temp);

			get_snmp3_auth_pass(0,temp);
			shell_output(s,"Auth Password:\t\t",temp);

			get_snmp3_priv_pass(0,temp);
			shell_output(s,"Priv Password:\t\t",temp);
		}
		return;
	}

	ptr = findcmd(s,str,"ports");
	if(ptr != NULL){
		if(autocompleat(s)){
			return;
		}
		ptr = findcmd(s,ptr," ");
		if(find_port(ptr,ports) == NULL){
			for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++)
				ports[i]=1;
		}
		shell_output(s,"Command: show ports ",ptr);
		for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
			if(ports[i]){
				shell_output(s,"------------------------------","");
				sprintf(temp,"%d",i+1);
				shell_output(s,"Port ",temp);
				if(PortStateInfo(L2F_port_conv(i))==1)
					shell_output(s,"Port State:\t\t","enable");
				else if(PortStateInfo(L2F_port_conv(i))==2)
					shell_output(s,"Port State:\t\t","learning");
				else if(PortStateInfo(L2F_port_conv(i))==3)
					shell_output(s,"Port State:\t\t","blocked");
				else
					shell_output(s,"Port State:\t\t","disable");

				if(get_port_link(i) == 1){
					switch(PortSpeedInfo(L2F_port_conv(i))){
						case 1:
							strcpy(temp,"10M ");
							break;
						case 2:
							strcpy(temp,"100M ");
							break;
						case 3:
							strcpy(temp,"1000M ");
							break;
					}
					switch(PortDuplexInfo(L2F_port_conv(i))){
						case 0:
							strcat(temp,"Half Duplex");
							break;
						case 1:
							strcat(temp,"Full Duplex");
							break;
					}
				}
				else
					strcpy(temp,"---");

				shell_output(s,"Port Speed/Duplex:\t",temp);

				if(get_port_link(i) == 1)
					shell_output(s,"Port Link:\t\t","Up");
				else
					shell_output(s,"Port Link:\t\t","Down");

				if((get_port_poe_a(i) == 1)||(get_port_poe_b(i) == 1))
					shell_output(s,"Port PoE:\t\t","On");
				else
					shell_output(s,"Port PoE:\t\t","Off");
			}
		}
		return;
	}

	ptr = findcmd(s,str,"firmware");
	if(ptr != NULL){
		if(autocompleat(s)){
			return;
		}
		shell_output(s,"Command: show firmware","");
		sprintf(temp,"%02X.%02X.%02X",
				 (int)(image_version[0]>>16)&0xff,
				 (int)(image_version[0]>>8)&0xff,  (int)(image_version[0])&0xff);
		shell_output(s,"Firmware Version:\t",temp);

		sprintf(temp,"%02X.%02X",(int)(bootloader_version>>8)&0xff,  (int)(bootloader_version)&0xff);
		shell_output(s,"Bootloader Version:\t",temp);
		return;
	}


	//sntp
	ptr = findcmd(s,str,"sntp");
	if(ptr != NULL){
		if(autocompleat(s)){
			return;
		}
		shell_output(s,"Command: show sntp","");
		if(get_sntp_state()==1)
			shell_output(s,"SNTP State:\t\t","enable");
		else
			shell_output(s,"SNTP State:\t\t","disable");

		get_sntp_serv(&ip);
		sprintf(temp,"%d.%d.%d.%d",uip_ipaddr1(ip),uip_ipaddr2(ip),uip_ipaddr3(ip),uip_ipaddr4(ip));
		shell_output(s,"SNTP Server:\t\t",temp);

		sprintf(temp,"%02d:%02d:%02d",date.hour,date.min,date.sec);
		shell_output(s,"Current Time:\t\t",temp);

		sprintf(temp,"%02d/%02d/%d",date.day,date.month,(date.year+2000));
		shell_output(s,"Current Date:\t\t",temp);
		return;
	}


	//syslog
	ptr = findcmd(s,str,"syslog");
	if(ptr != NULL){
		if(autocompleat(s)){
			return;
		}
		shell_output(s,"Command: show syslog","");
		if(get_syslog_state()==1)
			shell_output(s,"Syslog State:\t\t","enable");
		else
			shell_output(s,"Syslog State:\t\t","disable");

		get_syslog_serv(&ip);
		sprintf(temp,"%d.%d.%d.%d",uip_ipaddr1(ip),uip_ipaddr2(ip),uip_ipaddr3(ip),uip_ipaddr4(ip));
		shell_output(s,"Syslog Server:\t\t",temp);
		return;
	}


	ptr = findcmd(s,str,"packet");
	if(ptr != NULL){
		if(autocompleat(s)){
			return;
		}
		ptr = findcmd(s,ptr," ");
		if(find_port(ptr,ports) == NULL){
			for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++)
				ports[i]=1;
		}
		shell_output(s,"Command: show packet ",ptr);
		for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
			if(ports[i]){
				shell_output(s,"--------------------------------------------","");
				sprintf(temp,"Port %d\t\t\tRX\t\tTX",i+1);
				shell_output(s,temp,"");

				sprintf(temp, "%lu\t\t%lu",dev.port_stat[i].rx_good,dev.port_stat[i].tx_good);
				shell_output(s,"Good bytes:\t\t",temp);

				sprintf(temp, "%d\t\t%d",InFilteredFrameCount(L2F_port_conv(i)),OutFilteredFrameCount(L2F_port_conv(i)));
				shell_output(s,"Filtered packets:\t",temp);

				sprintf(temp, "%lu\t\t%lu",dev.port_stat[i].rx_unicast,dev.port_stat[i].tx_unicast);
				shell_output(s,"Unicast packets:\t",temp);

				sprintf(temp, "%lu\t\t%lu",dev.port_stat[i].rx_broadcast,dev.port_stat[i].tx_broadcast);
				shell_output(s,"Broadcast packets:\t",temp);

				sprintf(temp, "%lu\t\t%lu",dev.port_stat[i].rx_multicast,dev.port_stat[i].tx_multicast);
				shell_output(s,"Multicast packets:\t",temp);
			}
		}
		return;
	}


	ptr = findcmd(s,str,"fdb");
	u8 page = 0;
	start = 0;
	if(ptr!=NULL){
		if(autocompleat(s)){
			return;
		}
		shell_output(s,"Command: show fdb","");
		sprintf(temp,"%lu",read_atu_agetime());
		shell_output(s," ","");
		shell_output(s,"MAC Address Aging Time: ",temp);
		shell_output(s," ","");
		shell_output(s,"N\tMAC\t\t\tPorts","");
		shell_output(s,"-----------------------------------------------------","");

		start_show_fdb=1;
	}

	ptr = findcmd(s,str,"arpentry");
	if(ptr!=NULL){
		if(autocompleat(s)){
			return;
		}
		shell_output(s,"Command: show arpentry","");
		sprintf(temp,"%d",UIP_ARP_MAXAGE);
		shell_output(s,"ARP Aging Time: ",temp);
		shell_output(s,"","");
		shell_output(s,"Interface\tIP Address\t\tMAC Address","");
		shell_output(s,"-----------------------------------------------------","");
		for(i=0;i<UIP_ARPTAB_SIZE;i++){
			if(arp_table[i].ipaddr[0]){
				shell_prompt(s,"System\t\t");
				sprintf(temp,"%d.%d.%d.%d\t\t%02X:%02X:%02X:%02X:%02X:%02X\r\n",
				(uint8_t)(arp_table[i].ipaddr[0]),
				(uint8_t)((arp_table[i].ipaddr[0])>>8),
				(uint8_t)(arp_table[i].ipaddr[1]),
				(uint8_t)((arp_table[i].ipaddr[1])>>8),
				arp_table[i].ethaddr.addr[0],
				arp_table[i].ethaddr.addr[1],
				arp_table[i].ethaddr.addr[2],
				arp_table[i].ethaddr.addr[3],
				arp_table[i].ethaddr.addr[4],
				arp_table[i].ethaddr.addr[5]);

				shell_prompt(s,temp);
			}
		}
		return;
	}

	ptr = findcmd(s,str,"autorestart");
	if(ptr!=NULL){
		if(autocompleat(s)){
			return;
		}
		shell_output(s,"Command: show autorestart","");
		temp_u32 = 0;
		for(u8 i=0;i<COOPER_PORT_NUM;i++){
			if(get_port_sett_wdt(i)){
				temp_u32++;
			}
		}
		if(temp_u32)
			shell_output(s,"Auto Restart:\t\t","Enable");
		else{
			shell_output(s,"Auto Restart:\t\t","Disable");
			return;
		}

		temp[0] = 0;
		for(u8 i=0;i<COOPER_PORT_NUM;i++){
			if(get_port_sett_wdt(i)){
				if(get_dev_type() == DEV_PSW2GPLUS){
					switch(i){
						case 0:strcat(tmp,"FE#1: ");break;
						case 1:strcat(tmp,"FE#2: ");break;
						case 2:strcat(tmp,"FE#3: ");break;
						case 3:strcat(tmp,"FE#4: ");break;
					}
				}else{
					sprintf(tmp,"Port %d: ",i+1);
					switch(get_port_sett_wdt(i)){
						case 1:
							shell_output(s,tmp,"Mode - link");
							break;
						case 2:
							get_port_sett_wdt_ip(i,&ip);
							sprintf(temp,"Mode - Ping, IP - %d.%d.%d.%d",
									uip_ipaddr1(&ip),uip_ipaddr2(&ip),uip_ipaddr3(&ip),uip_ipaddr4(&ip));
							shell_output(s,tmp,temp);
							break;
						case 3:
							sprintf(temp,"Mode - Speed, Limit - %d-%d Mbps",get_port_sett_wdt_speed_down(i),get_port_sett_wdt_speed_up(i));
							shell_output(s,tmp,temp);
							break;
					}
				}
			}
		}
		return;
	}


	ptr = findcmd(s,str,"comfortstart");
	if(ptr!=NULL){
		if(autocompleat(s)){
			return;
		}
		shell_output(s,"Command: show comfortstart","");
		temp[0] = 0;
		for(u8 i=0;i<COOPER_PORT_NUM;i++){
			if(get_port_sett_soft_start(i)==1){
				if(get_dev_type() == DEV_PSW2GPLUS){
					switch(i){
						case 0:strcat(temp,"FE#1 ");break;
						case 1:strcat(temp,"FE#2 ");break;
						case 2:strcat(temp,"FE#3 ");break;
						case 3:strcat(temp,"FE#4 ");break;
					}
				}else{
					sprintf(tmp,"%d ",i+1);
					strcat(temp,tmp);
				}
			}
		}
		if(strlen(temp)==0)
			strcat(temp,"disable");
		shell_output(s,"Comfort Start Ports:\t",temp);
		sprintf(temp,"%d Hour",get_softstart_time());
		shell_output(s,"Soft start time:\t",temp);
		return;
	}

	ptr = findcmd(s,str,"dry_cont");
	if(ptr!=NULL){
		if(autocompleat(s)){
			return;
		}
		shell_output(s,"Command: show dry_cont","");
		for(u8 i=0;i<NUM_ALARMS;i++){
			sprintf(temp,"Sensor%d. State: ",i);
			if(get_alarm_state(i))
				strcat(temp,"Enable");
			else
				strcat(temp,"Disable");
			strcat(temp,", Alarm state: ");
			if(get_alarm_front(i)==2)
				strcat(temp,"Open");
			else
				strcat(temp,"Short");
			strcat(temp,", Current state: ");
			if(get_sensor_state(i))
				strcat(temp,"Open");
			else
				strcat(temp,"Short");
			shell_output(s,temp,"");
		}
	}

	ptr = findcmd(s,str,"tftp");
	if(ptr!=NULL){
		if(autocompleat(s)){
			return;
		}
		shell_output(s,"Command: show tftp","");
		sprintf(temp,"TFTP State: ");
		if(get_tftp_state())
			strcat(temp,"Enable");
		else
			strcat(temp,"Disable");
		shell_output(s,temp,"");

		sprintf(temp,"TFTP Port: %d",get_tftp_port());
		shell_output(s,temp,"");
	}

	ptr = findcmd(s,str,"events");
	if(ptr!=NULL){
		if(autocompleat(s)){
			return;
		}
		shell_output(s,"Command: show events","");
		shell_output(s,"Event List:","");
		shell_output(s,"-----------","");

		sprintf(temp,"Port.link\t\tState ");
		if(get_event_port_link_t_st())
			strcat(temp,"Enable");
		else
			strcat(temp,"Disable");
		strcat(temp,",Level ");
		sprintf(tmp,"%d",get_event_port_link_t_level());
		strcat(temp,tmp);
		shell_output(s,temp,"");

		sprintf(temp,"Port.PoE\t\tState ");
		if(get_event_port_poe_t_st())
			strcat(temp,"Enable");
		else
			strcat(temp,"Disable");
		strcat(temp,",Level ");
		sprintf(tmp,"%d",get_event_port_poe_t_level());
		strcat(temp,tmp);
		shell_output(s,temp,"");

		sprintf(temp,"STP/RSTP\t\tState ");
		if(get_event_port_stp_t_st())
			strcat(temp,"Enable");
		else
			strcat(temp,"Disable");
		strcat(temp,",Level ");
		sprintf(tmp,"%d",get_event_port_stp_t_level());
		strcat(temp,tmp);
		shell_output(s,temp,"");

		sprintf(temp,"Autorestart.Link\tState ");
		if(get_event_port_spec_link_t_st())
			strcat(temp,"Enable");
		else
			strcat(temp,"Disable");
		strcat(temp,",Level ");
		sprintf(tmp,"%d",get_event_port_spec_link_t_level());
		strcat(temp,tmp);
		shell_output(s,temp,"");

		sprintf(temp,"Autorestart.Ping\tState ");
		if(get_event_port_spec_ping_t_st())
			strcat(temp,"Enable");
		else
			strcat(temp,"Disable");
		strcat(temp,",Level ");
		sprintf(tmp,"%d",get_event_port_spec_ping_t_level());
		strcat(temp,tmp);
		shell_output(s,temp,"");

		sprintf(temp,"Autorestart.Speed\tState ");
		if(get_event_port_spec_speed_t_st())
			strcat(temp,"Enable");
		else
			strcat(temp,"Disable");
		strcat(temp,",Level ");
		sprintf(tmp,"%d",get_event_port_spec_speed_t_level());
		strcat(temp,tmp);
		shell_output(s,temp,"");

		sprintf(temp,"System\t\t\tState ");
		if(get_event_port_system_t_st())
			strcat(temp,"Enable");
		else
			strcat(temp,"Disable");
		strcat(temp,",Level ");
		sprintf(tmp,"%d",get_event_port_system_t_level());
		strcat(temp,tmp);
		shell_output(s,temp,"");

		sprintf(temp,"UPS\t\t\tState ");
		if(get_event_port_ups_t_st())
			strcat(temp,"Enable");
		else
			strcat(temp,"Disable");
		strcat(temp,",Level ");
		sprintf(tmp,"%d",get_event_port_ups_t_level());
		strcat(temp,tmp);
		shell_output(s,temp,"");

		sprintf(temp,"Access control\t\tState ");
		if(get_event_port_alarm_t_st())
			strcat(temp,"Enable");
		else
			strcat(temp,"Disable");
		strcat(temp,",Level ");
		sprintf(tmp,"%d",get_event_port_alarm_t_level());
		strcat(temp,tmp);
		shell_output(s,temp,"");
	}


	ptr = findcmd(s,str,"802.1p");
	if(ptr!=NULL){
		if(autocompleat(s)){
			return;
		}
		ptr = findcmd(s,ptr,"state");
		if(ptr!= NULL){
			if(autocompleat(s)){
				return;
			}
			shell_output(s,"Command: show 802.1p state","");
			strcpy(temp,"QoS State: ");
			if(get_qos_state())
				strcat(temp,"Enable");
			else
				strcat(temp,"Disable");
			shell_output(s,temp,"");
		}
		else{
			ptr = findcmd(s,str,"default_priority");
			if(ptr!=NULL){
				if(autocompleat(s)){
					return;
				}
				shell_output(s,"Command: show 802.1p default_priority","");
				for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
					sprintf(temp,"Port %d: Priority %d",i+1,get_qos_port_def_pri(i));
					shell_output(s,temp,"");
				}
				return;
			}
			else{
				ptr = findcmd(s,str,"user_priority");
				if(ptr!=NULL){
					if(autocompleat(s)){
						return;
					}
					shell_output(s,"Command: show 802.1p user_priority","");
					for(u8 i=0;i<8;i++){
						sprintf(temp,"Priority %d, Queue %d",i,get_qos_cos_queue(i));
						shell_output(s,temp,"");
					}
					return;
				}
				else{
					shell_output(s,"ERROR: Command","");
					shell_output(s,"Next possible completions:","");
					shell_output(s,"state","");
					shell_output(s,"default_priority","");
					shell_output(s,"user_priority","");
					return;
				}
			}
		}
	}

	ptr = findcmd(s,str,"scheduling_mechanism");
	if(ptr!=NULL){
		if(autocompleat(s)){
			return;
		}
		shell_output(s,"Command: show scheduling_mechanism","");
		if(get_qos_policy()==0)
			shell_output(s,"Scheduling mechanism: Weighted fair priority","");
		else if(get_qos_policy()==1)
			shell_output(s,"Scheduling mechanism: Strict priority","");
		return;
	}

	ptr = findcmd(s,str,"dscp_mapping");
	if(ptr!=NULL){
		if(autocompleat(s)){
			return;
		}
		shell_output(s,"Command: show dscp_mapping","");
		for(u8 i=0;i<16;i++){
			sprintf(temp,"DSCP %d,Queue %d\tDSCP %d,Queue %d\tDSCP %d,Queue %d\tDSCP %d,Queue %d",
					i,get_qos_tos_queue(i),i+16,get_qos_tos_queue(i+16),i+32,get_qos_tos_queue(i+32),
					i+48,get_qos_tos_queue(i+48));
			shell_output(s,temp,"");
		}
		return;
	}

	ptr = findcmd(s,str,"bandwidth_control");
	if(ptr!=NULL){
		if(autocompleat(s)){
			return;
		}
		shell_output(s,"Command: show bandwidth_control","");
		for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
			sprintf(temp,"Port %d: RxLimit %lu,TxLimit %lu",i+1,get_rate_limit_rx(i),get_rate_limit_tx(i));
			shell_output(s,temp,"");
		}
		shell_output(s,"0 - no limit","");
		return;
	}


	ptr = findcmd(s,str,"poe");
	if(ptr!=NULL){
		if(autocompleat(s)){
			return;
		}
		ptr = findcmd(s,ptr," ");
		if(find_port(ptr,ports) == NULL){
			for(u8 i=0;i<ALL_PORT_NUM;i++)
				ports[i]=1;
		}
		shell_output(s,"Command: show poe ",ptr);
		for(u8 i=0;i<POE_PORT_NUM;i++){
			if(ports[i]){
				shell_output(s,"-------------------------","");
				sprintf(temp,"Port %d",i+1);
				shell_output(s,temp,"");
				if((get_port_poe_a(i) == 1)||(get_port_poe_b(i) == 1))
					shell_output(s,"State:\t\t","On");
				else
					shell_output(s,"State:\t\t","Off");

				temp[0]=0;

				if(get_port_poe_a(i)){
					strcat(temp,"PoE A ");
				}

				if(get_port_poe_b(i)){
					strcat(temp,"PoE B ");
				}
				shell_output(s,"Type:\t\t",temp);

				Temp = 0;
				if(get_port_poe_a(i)){
					Temp += get_poe_voltage(POE_A,i)*get_poe_current(POE_A,i)/1000;
				}
				if(get_port_poe_b(i)&&((i<POEB_PORT_NUM)||(get_dev_type() == DEV_PSW2GPLUS))){
					Temp += get_poe_voltage(POE_B,i)*get_poe_current(POE_B,i)/1000;
				}
				if(Temp){
					tot_pwr+=Temp;
					sprintf(temp,"%d.%d W",(int)(Temp/1000),(int)(Temp % 1000));
					addstr(temp);
				}
				shell_output(s,"Power:\t\t",temp);
			}
		}
	}


	ptr = findcmd(s,str, "config");
	if(ptr != NULL){
		if(autocompleat(s)){
			return;
		}
		if(s->user_rule!=ADMIN_RULE){
			shell_output(s,"ERROR: Access denied","");
			return;
		}

		offset_make_bak = 0;

		backup_file_len = make_bak();

		start_make_bak = 1;
		shell_output(s,"Command: show config","");
		DEBUG_MSG(TELNET_DEBUG && !s->usb,"make_bak: %lu\r\n",backup_file_len);

		/*while(1){
			memset(temp,0,sizeof(temp));
			spi_flash_read(offset,TELNETD_CONF_LINELEN,temp);
			shell_output(s,temp,"");
			offset+=TELNETD_CONF_LINELEN;
			if(offset>=backup_file_len)
				break;
		}*/
		return;
	}


	ptr = findcmd(s,str, "mac_filtering");
	if(ptr != NULL){
		if(autocompleat(s)){
			return;
		}
		ptr1 = findcmd(s,ptr, "port_state");
		if(ptr1 != NULL){
			if(autocompleat(s)){
				return;
			}
			/*ptr2= findcmd(s,ptr1," ");
			if(find_port(ptr2,ports) == NULL){
				for(u8 i=0;i<ALL_PORT_NUM;i++)
					ports[i]=1;
			}*/
			shell_output(s,"Command: show mac_filtering port_state","");
			shell_output(s,"-------------------------","");
			for(u8 i=0;i<ALL_PORT_NUM;i++){
				sprintf(temp,"Port %d",i+1);
				shell_output(s,temp,"");
				switch(get_mac_filter_state(i)){
					case PORT_NORMAL:
						shell_output(s,"State:\t\t","Normal");
						break;
					case MAC_FILT:
						shell_output(s,"State:\t\t","Secure: MAC filtration");
						break;
					case PORT_FILT:
						shell_output(s,"State:\t\t","Secure: Port shutdown");
						break;
					case PORT_FILT_TEMP:
						shell_output(s,"State:\t\t","Secure: Temporary port shutdown");
						break;
				}
			}
			return;
		}
		else{
			ptr1 = findcmd(s,ptr, "allowed");
			if(ptr1 != NULL){
				if(autocompleat(s)){
					return;
				}
				shell_output(s,"Command: show mac_filtering allowed","");
				if(get_mac_bind_num()==0){
					shell_output(s,"Empty table!","");
					return;
				}
				shell_output(s,"MAC address\t\t\tPort","");
				shell_output(s,"-------------------------","");
				for(u8 i=0;i<get_mac_bind_num();i++){
					sprintf(temp,"%02X:%02X:%02X:%02X:%02X:%02X\t\t\t%d",get_mac_bind_entry_mac(i,0),
							get_mac_bind_entry_mac(i,1),get_mac_bind_entry_mac(i,2),
							get_mac_bind_entry_mac(i,3),get_mac_bind_entry_mac(i,4),
							get_mac_bind_entry_mac(i,5),get_mac_bind_entry_port(i)+1);
					shell_output(s,temp,"");
				}
				return;
			}
			else{
				ptr1 = findcmd(s,ptr, "blocked");
				if(ptr1 != NULL){
					if(autocompleat(s)){
						return;
					}
					shell_output(s,"Command: show mac_filtering blocked","");
					shell_output(s,"MAC address\t\t\tPort","");
					shell_output(s,"-------------------------","");
					for(u8 i=0;i<MAX_BLOCKED_MAC;i++){
						if(dev.mac_blocked[i].age_time){
							sprintf(temp,"%02X:%02X:%02X:%02X:%02X:%02X\t\t\t%d",dev.mac_blocked[i].mac[0],
								dev.mac_blocked[i].mac[1],dev.mac_blocked[i].mac[2],
								dev.mac_blocked[i].mac[3],dev.mac_blocked[i].mac[4],
								dev.mac_blocked[i].mac[5],dev.mac_blocked[i].port+1);
							shell_output(s,temp,"");
						}
					}
					return;
				}
				else{
					shell_output(s,"ERROR: Command","");
					shell_output(s,"Next possible completions:","");
					shell_output(s,"port_state","");
					shell_output(s,"allowed","");
					shell_output(s,"blocked","");
					return;
				}
			}
		}
	}

	ptr = findcmd(s,str, "inputs");
	if(ptr != NULL){
		if(autocompleat(s)){
			return;
		}
		if(is_plc_connected()==0){
			shell_output(s,"ERROR: PLC board is not connected","");
			return;
		}

		shell_output(s,"Command: show inputs","");

		for(u8 i=0;i<get_plc_input_num();i++){
			sprintf(temp,"Input%d State: ",i);
			if(get_plc_in_state(i))
				strcat(temp,"Enable");
			else
				strcat(temp,"Disable");
			strcat(temp,", Alarm state: ");
			if(get_plc_in_alarm_state(i)==PLC_ON_SHORT)
				strcat(temp,"Short");
			else if(get_plc_in_alarm_state(i)==PLC_ON_OPEN)
				strcat(temp,"Open");
			else
				strcat(temp,"Any Change");
			strcat(temp,", Current state: ");
			if(dev.plc_status.in_state[i] == PLC_ON_OPEN)
				strcat(temp,"Open");
			else
				strcat(temp,"Short");
			shell_output(s,temp,"");
		}
	}

	ptr = findcmd(s,str, "outputs");
	if(ptr != NULL){
		if(autocompleat(s)){
			return;
		}

		if(is_plc_connected()==0){
			shell_output(s,"ERROR: PLC board is not connected","");
			return;
		}

		shell_output(s,"Command: show outputs","");
		for(u8 i=0;i<get_plc_output_num();i++){
			sprintf(temp,"Output%d State: ",i);
			if(get_plc_out_state(i)==PLC_ACTION_OPEN)
				strcat(temp,"Open");
			else
				strcat(temp,"Short");
			shell_output(s,temp,"");
		}
	}

	ptr = findcmd(s,str, "rs485");
	if(ptr != NULL){
		if(autocompleat(s)){
			return;
		}
		if(is_plc_connected()==0){
			shell_output(s,"ERROR: PLC board is not connected","");
			return;
		}

		ptr1 = findcmd(s,ptr, "settings");
		if(ptr1 != NULL){
			shell_output(s,"Command: show rs485 settings","");
			switch(get_plc_em_rate()){
				case 0: sprintf(temp,"300");break;
				case 1: sprintf(temp,"600");break;
				case 2: sprintf(temp,"1200");break;
				case 3: sprintf(temp,"2400");break;
				case 4: sprintf(temp,"4800");break;
				case 5: sprintf(temp,"9600");break;
				case 6: sprintf(temp,"19200");break;
			}
			shell_output(s,"Baudrate: ",temp);

			if(get_plc_em_parity()==0)
				sprintf(temp,"Disable");
			else if(get_plc_em_parity() == 1)
				sprintf(temp,"Even");
			else
				sprintf(temp,"Odd");
			shell_output(s,"Parity: ",temp);
		}
		else{
			ptr1 = findcmd(s,str, "em");
			if(ptr1 != NULL){
				//
			}
			else{
				shell_output(s,"ERROR: Command","");
				shell_output(s,"Next possible completions:","");
				shell_output(s,"settings","");
				shell_output(s,"em","");
				return;
			}
		}
	}



	//aggregation
	ptr = findcmd(s,str, "aggregation");
	if(ptr != NULL){
		if(autocompleat(s)){
			return;
		}
		if(get_lag_entries_num()){
			 shell_output(s,"Link Aggregation:\t","Enable");
			 for(u8 i=0;i<get_lag_entries_num();i++){
				 if(get_lag_state(i) && get_lag_valid(i)){
					 sprintf(temp,"ID\t\t:%d",i+1);
					 shell_output(s,temp,"");
					 sprintf(temp,"Master port\t:%d",get_lag_master_port(i));
					 shell_output(s,temp,"");
					 temp[0]=0;
					 for(u8 j=0;j<ALL_PORT_NUM;j++){
						 if(get_lag_port(i,j)){
							 sprintf(tmp,"%d ",j+1);
							 strcat(temp,tmp);
						 }
					 }
					 shell_output(s,"Ports\t:",temp);
					 shell_output(s,"","");
				 }
			 }
		}
		else{
			shell_output(s,"Link Aggregation:\t","Disable");
		}

	}


	//mirroring
	ptr = findcmd(s,str, "mirroring");
	if(ptr != NULL){
		if(autocompleat(s)){
			return;
		}
		if(get_mirror_state()){
			 shell_output(s,"Port Mirroring:\t\t","Enable");
			 sprintf(temp,"%d",get_mirror_target_port()+1);
			 shell_output(s,"Target port:\t\t",temp);
			 temp[0]=0;
			 for(u8 i=0;i<ALL_PORT_NUM;i++){
				 switch(get_mirror_port(i)){
				 	 case 1:sprintf(tmp,"%d(Rx) ",i+1); strcat(temp,tmp);break;
				 	 case 2:sprintf(tmp,"%d(Tx) ",i+1); strcat(temp,tmp);break;
				 	 case 3:sprintf(tmp,"%d(Rx+Tx) ",i+1); strcat(temp,tmp);break;
				 }
			 }
			 shell_output(s,"Ports\t:",temp);
		}
		else{
			shell_output(s,"Port Mirroring:\t\t","Disable");
		}

	}

	//remote devices list
	ptr = findcmd(s,str, "teleport");
	if(ptr != NULL){
		if(autocompleat(s)){
			return;
		}
		shell_output(s,"Command: show teleport","");
		shell_output(s,"\tIP address\tType\t\tDescription","");
		shell_output(s,"---------------------------------------------","");
		for(u8 i=0,num=0;i<MAX_REMOTE_TLP;i++){
			if(get_tlp_remdev_valid(i)){
				sprintf(temp,"%d\t",num+1);
				get_tlp_remdev_ip(i,&ip);
				sprintf(temp2,"%d.%d.%d.%d\t",uip_ipaddr1(ip),uip_ipaddr2(ip),
					uip_ipaddr3(ip),uip_ipaddr4(ip));
				strcat(temp,temp2);
				switch(get_tlp_remdev_type(i)){
					case 0:sprintf(temp2,"Teleport-1\t");break;
					case 1:sprintf(temp2,"Teleport-2\t");break;
					case 2:sprintf(temp2,"PSW\t\t");break;
				}
				strcat(temp,temp2);
				get_tlp_remdev_name(i,temp2);
				strcat(temp,temp2);
				shell_output(s,temp,"");
				num++;
			}
		}
		return;
	}


	//lldp statistics
	ptr = findcmd(s,str, "lldp");
	if(ptr != NULL){
		if(autocompleat(s)){
			return;
		}
		shell_output(s,"Command: show lldp","");
		if(get_lldp_state())strcpy(temp,"Enable");
		else strcpy(temp,"Disable");
		shell_output(s,"LLDP state:\t\t",temp);
		sprintf(temp,"%d",get_lldp_transmit_interval());
		shell_output(s,"Transmit Interval:\t",temp);
		sprintf(temp,"%d",get_lldp_hold_multiplier());
		shell_output(s,"Hold Multiplier:\t",temp);

		for(u8 i=0;i<ALL_PORT_NUM;i++){
			if(get_lldp_port_state(i))
				sprintf(temp," %d:\t\tEnable",i+1);
			else
				sprintf(temp," %d:\t\tDisable",i+1);
			shell_output(s,"Port",temp);
		}
		return;
	}

	//Stsatistics
	ptr = findcmd(s,str, "statistics");
	if(ptr != NULL){
		if(autocompleat(s)){
			return;
		}
		ptr1= findcmd(s,ptr, "cpu");
		if(ptr1 != NULL){
			if(autocompleat(s)){
				return;
			}
			shell_output(s,"Command: show statistics cpu","");
			sprintf(temp,"Min (10 sec):\t%d",get_cpu_min(CPU_STAT_10SEC));
			shell_output(s,temp,"");
			sprintf(temp,"Max (10 sec):\t%d",get_cpu_max(CPU_STAT_10SEC));
			shell_output(s,temp,"");
			sprintf(temp,"Min (5 min):\t%d",get_cpu_min(CPU_STAT_LEN));
			shell_output(s,temp,"");
			sprintf(temp,"Max (5 min):\t%d",get_cpu_max(CPU_STAT_LEN));
			shell_output(s,temp,"");

		}else{
			shell_output(s,"ERROR: Command","");
			shell_output(s,"Next possible completions:","");
			shell_output(s,"cpu","");
			return;
		}
	}
}


//---------------------------------------------------------------------------
static void config(struct telnetd_state *s,char *str){
//char *ptr;
char temp[64];
uip_ipaddr_t ip,ip2;
u32 temp_u32;
u8 temp_u8,mac[6];
u8 ports[PORT_NUM];
u16 vid;
char username[20],password[20];
engine_id_t eid;
u8 remport,remdev;
u8 port_in,port_out;


char *ptr1,*ptr2,*ptr3,*ptr4,*ptr5,*ptr6,*ptr7, *ptr8;//указатели для разной вложенности


if(s->user_rule!=ADMIN_RULE){
	shell_output(s,"ERROR: Access denied","");
	return;
}

DEBUG_MSG(TELNET_DEBUG && !s->usb,"command config %s||%s||%d\r\n",str,s->buf,s->bufptr);

  //ip addr
  ptr1 = findcmd(s,str,"ipif");
    if(ptr1 !=NULL){
  	  ptr2 = findcmd(s,ptr1,"System");
  	  if(ptr2 != NULL){
  	  	  ptr3 = findcmd(s,ptr2,"ipaddress");
  	  	  if(ptr3 != NULL){
  	  		  if(autocompleat(s)){
  	  			return;
  	  		  }
  	  		  if(find_ip(ptr3,&ip) == 1){
  	  			  shell_output(s,"Command: config ipif System ipaddress",ptr3);
  	  			  shell_output(s,"Success.","");
  	  			  set_net_ip(ip);
  	  			  settings_add2queue(SQ_NETWORK);
  	  			  return;
  	  		  }
  	  		  else{
  	  			  shell_output(s,"ERROR: incorrect IP address","");
  	  			  return;
  	  		  }
  	  	  }else{
  	  	  	  ptr3 = findcmd(s,ptr2,"gateway");
  	  	  	  if(ptr3 != NULL){
  	    		  if(autocompleat(s)){
  	    			return;
  	    		  }
  	  	  		  if(find_ip(ptr3,&ip) == 1){
  	  	  			  shell_output(s,"Command: config ipif System gateway",ptr3);
  	  	  			  shell_output(s,"Success.","");
  	  	  			  set_net_gate(ip);
  	  	  			  settings_add2queue(SQ_NETWORK);
  	  	  			  return;
  	  	  		  }
  	  	  		  else{
  	  	  			  shell_output(s,"ERROR: incorrect Gateway","");
  	  	  			  return;
  	  	  		  }
  	  	  	  }
  	  	  	  else{
  	    	  	  ptr3 = findcmd(s,ptr2,"netmask");
  	    	  	  if(ptr3 != NULL){
  	    	  		  if(autocompleat(s)){
  	    	  			return;
  	    	  		  }
  	    	  		  if(find_ip(ptr3,&ip) == 1){
  	    	  			  shell_output(s,"Command: config ipif System netmask",ptr3);
  	    	  			  shell_output(s,"Success.","");
  	    	  			  set_net_mask(ip);
  	    	  			  settings_add2queue(SQ_NETWORK);
  	    	  			  return;
  	    	  		  }
  	    	  		  else{
  	    	  			  shell_output(s,"ERROR: incorrect Netmask","");
  	    	  			  return;
  	    	  		  }
  	    	  	  }
  	    	  	  else{
  	  	    	  	  ptr3 = findcmd(s,ptr2,"dhcp");
  	  	    	  	  if(ptr3 != NULL){
		  	    	  	  ptr4 = findcmd(s,str,"enable");
  	  	    	  		  if(ptr4 != NULL){
							  if(autocompleat(s)){
								return;
							  }
  	  	    	  			  shell_output(s,"Command: config ipif System dhcp enable","");
  	  	    	  			  shell_output(s,"Success.","");
  	  	    	  			  set_dhcp_mode(DHCP_CLIENT);
  	  	    	  			  settings_add2queue(SQ_NETWORK);
  	  	    	  			  return;
  	  	    	  		  }
  	  	    	  		  else{
  	  	    	  			  ptr4 = findcmd(s,ptr3,"disable");
  	  	    	  			  if(ptr4 != NULL){
  								  if(autocompleat(s)){
  									return;
  								  }
  	  	    	  				  shell_output(s,"Command: config ipif System dhcp disable","");
  	  	    	  				  shell_output(s,"Success.","");
  	  	    	  				  set_dhcp_mode(DHCP_DISABLED);
  	  	    	  				  settings_add2queue(SQ_NETWORK);
  	  	    	  				  return;
  	  	    	  			  }
  	  	    	  			  else{
								  if(autocompleat(s)){
									  return;
								  }
  	  	    	  				  shell_output(s,"ERROR: Command","");
  	  	    	  				  shell_output(s,"Next possible completions:","");
  	  							  shell_output(s,"enable","");
  	  							  shell_output(s,"disable","");
  	  							  return;
  	  	    	  			  }
  	  	    	  		  }
  	  	    	  	  }
  	  	    	  	  else{
  	  	    	  		  ptr3 = findcmd(s,ptr2,"dns");
  	  	    	  		  if(ptr3!=NULL){
							  if(autocompleat(s)){
								return;
							  }
  	  	    	  			  if(find_ip(ptr3,&ip) == 1){
  	  	    	  				  shell_output(s,"Command: config ipif System dns",ptr3);
  	  	    	  				  shell_output(s,"Success.","");
								  set_net_dns(ip);
								  settings_add2queue(SQ_NETWORK);
								  return;
							  }
							  else{
								  shell_output(s,"ERROR: incorrect IP address","");
								  return;
							  }
  	  	    	  		  }else {
  	  	    	  			  ptr3 = findcmd(s,ptr2,"grat_arp");
  	  	    	  		  	  if(ptr3!=NULL){
								  if(autocompleat(s)){
									return;
								  }
								  if(ptr3 != NULL){
									  ptr4 = findcmd(s,str,"enable");
									  if(ptr4 != NULL){
										  if(autocompleat(s)){
											return;
										  }
										  shell_output(s,"Command: config ipif System grat_arp enable","");
										  shell_output(s,"Success.","");
										  set_gratuitous_arp_state(ENABLE);
										  settings_add2queue(SQ_NETWORK);
										  return;
									  }
									  else{
										  ptr4 = findcmd(s,ptr3,"disable");
										  if(ptr4 != NULL){
											  if(autocompleat(s)){
												return;
											  }
											  shell_output(s,"Command: config ipif System grat_arp disable","");
											  shell_output(s,"Success.","");
											  set_gratuitous_arp_state(DISABLE);
											  settings_add2queue(SQ_NETWORK);
											  return;
										 }
										 else{
											 if(autocompleat(s)){
												return;
											 }
											 shell_output(s,"ERROR: Command","");
											 shell_output(s,"Next possible completions:","");
											 shell_output(s,"enable","");
											 shell_output(s,"disable","");
											 return;
										}
									}
  	  	    	  		  	  }
  	  	    	  		  	  else{
								  if(autocompleat(s)){
									  return;
								  }
								  shell_output(s,"ERROR: Command","");
								  shell_output(s,"Next possible completions:","");
								  shell_output(s,"ipaddress","");
								  shell_output(s,"gateway","");
								  shell_output(s,"netmask","");
								  shell_output(s,"dhcp","");
								  shell_output(s,"dns","");
								  shell_output(s,"grat_arp","");
								  return;
  	  	    	  		  	  }
  	  	    	  		  }
  	  	    	  	  }
  	    	  	  }
  	  	  	  }
  	  	  }

  	  	  }
  	  }
  	  else{
		  if(autocompleat(s)){
			  return;
		  }
  		  shell_output(s,"ERROR: Command","");
  		  shell_output(s,"Next possible completions:","");
  		  shell_output(s,"System","");
  		  return;
  	  }
    }


  //config syslog
  ptr1 = findcmd(s,str,"syslog");
  if(ptr1 !=NULL){
	  ptr2 = findcmd(s,ptr1,"host");
	  if(ptr2 != NULL){
		  if(autocompleat(s)){
			return;
		  }
		  if(find_ip(ptr2,&ip) == 1){
			  shell_output(s,"Command: config syslog host",ptr2);
			  shell_output(s,"Success.","");
			  set_syslog_serv(ip);
			  settings_add2queue(SQ_SYSLOG);
			  return;
		  }
		  else{
			  shell_output(s,"ERROR: incorrect IP address","");
			  return;
		  }
	  }
	  else{
		  ptr2 = findcmd(s,ptr1,"state");
		  if(ptr2 != NULL){
			  ptr3 = findcmd(s,ptr2,"enable");
			  if(ptr3 != NULL){
				  if(autocompleat(s)){
					return;
				  }
				  shell_output(s,"Command: config syslog state enable","");
				  shell_output(s,"Success.","");
				  set_syslog_state(ENABLE);
				  settings_add2queue(SQ_SYSLOG);
				  return;
			  }
			  else{
				  ptr3 = findcmd(s,ptr2,"disable");
				  if(ptr3 != NULL){
					  if(autocompleat(s)){
						return;
					  }
					  shell_output(s,"Command: config syslog state disable","");
					  shell_output(s,"Success.","");
					  set_syslog_state(DISABLE);
					  settings_add2queue(SQ_SYSLOG);
					  return;
				  }
				  else{
					  if(autocompleat(s)){
						  return;
					  }
					  shell_output(s,"ERROR: Command","");
					  shell_output(s,"Next possible completions:","");
					  shell_output(s,"enable","");
					  shell_output(s,"disable","");
					  return;
				  }
			  }
		  }
		  else{
			  if(autocompleat(s)){
				  return;
			  }
			  shell_output(s,"ERROR: Command","");
			  shell_output(s,"Next possible completions:","");
			  shell_output(s,"state","");
			  shell_output(s,"host <ip>","");
			  return;
		  }
	  }
  }

  //config sntp
  ptr1 = findcmd(s,str,"sntp");
  if(ptr1 !=NULL){
	  ptr2 = findcmd(s,ptr1,"primary");
	  if(ptr2 != NULL){
		  if(autocompleat(s)){
			return;
		  }
		  if(find_ip(ptr2,&ip) == 1){
			  shell_output(s,"Command: config sntp primary",ptr2);
			  shell_output(s,"Success.","");
			  set_sntp_serv(ip);
			  settings_add2queue(SQ_SNTP);
			  return;
		  }
		  else{
			  shell_output(s,"ERROR: incorrect IP address","");
			  return;
		  }
	  }
	  else{
		  ptr2 = findcmd(s,ptr1,"state");
		  if(ptr2 != NULL){
    	  	  ptr3 = findcmd(s,ptr2,"enable");
    	  	  if(ptr3 != NULL){
				  if(autocompleat(s)){
					return;
				  }
    	  		  shell_output(s,"Command: config sntp state enable","");
    	  		  shell_output(s,"Success.","");
    	  		  set_sntp_state(ENABLE);
    	  		  settings_add2queue(SQ_SNTP);
    	  		  return;
    	  	  }
    	  	  else{
    	  		  ptr3 = findcmd(s,ptr2,"disable");
    	  		  if(ptr3 != NULL){
					  if(autocompleat(s)){
						return;
					  }
    	  			  shell_output(s,"Command: config sntp state disable","");
    	  			  shell_output(s,"Success.","");
    	  			  set_sntp_state(DISABLE);
    	  			  settings_add2queue(SQ_SNTP);
    	  			  return;
    	  		  }
    	  		  else{
					  if(autocompleat(s)){
						  return;
					  }
    	  			  shell_output(s,"ERROR: Command","");
    	  			  shell_output(s,"Next possible completions:","");
					  shell_output(s,"enable","");
					  shell_output(s,"disable","");
					  return;
    	  		  }
    	  	  }
		  }
		  else{
			  ptr2 = findcmd(s,ptr1,"timezone");
			  if(ptr2 != NULL){
				  if(autocompleat(s)){
					return;
				  }
				  ptr3 = findcmd(s,ptr2," ");
				  temp_u8 = (u8)strtol(ptr3,&ptr4,10);
				  if(set_sntp_timezone((i8)temp_u8) == 0){
					  shell_output(s,"Command: config sntp timezone ",ptr3);
					  shell_output(s,"Success.","");
					  settings_add2queue(SQ_SNTP);
					  return;
				  }else{
					  shell_output(s,"ERROR: Timezone: ","Incorrect Value");
					  return;
				  }
			  }
			  else{
				  if(autocompleat(s)){
					  return;
				  }
				  shell_output(s,"ERROR: Command","");
				  shell_output(s,"Next possible completions:","");
				  shell_output(s,"state","");
				  shell_output(s,"primary","");
				  shell_output(s,"timezone","");
				  return;
			  }

		  }
	  }
  }


  //set_stp_state
  //config stp
  if(/*get_dev_type()!=DEV_PSW1G4F*/1){
	  ptr1 = findcmd(s,str,"stp");
	  if(ptr1 !=NULL){
		  ptr2 = findcmd(s,ptr1,"state");
		  if(ptr2 != NULL){
			  if(findcmd(s,ptr2,"enable") != NULL){
				  if(autocompleat(s)){
					return;
				  }
				  shell_output(s,"Command: config stp state enable","");
				  shell_output(s,"Success.","");
				  set_stp_state(ENABLE);
				  settings_add2queue(SQ_STP);
				  return;
			  }else if(findcmd(s,ptr2,"disable")){
				  if(autocompleat(s)){
					return;
				  }
				  shell_output(s,"Command: config stp state disable","");
				  shell_output(s,"Success.","");
				  set_stp_state(DISABLE);
				  settings_add2queue(SQ_STP);
				  return;
			  }else{
				  if(autocompleat(s)){
					return;
				  }
				  shell_output(s,"ERROR: Command","");
				  shell_output(s,"Next possible completions:","");
				  shell_output(s,"enable","");
				  shell_output(s,"disable","");
				  return;
			  }
		  }
		  else{
			  ptr2 = findcmd(s,ptr1,"priority");
			  if(ptr2 != NULL){
				  if(autocompleat(s)){
					return;
				  }
				  ptr3 = findcmd(s,ptr2," ");
				  temp_u32 = strtol(ptr3,&ptr4,10);
				  if(set_stp_bridge_priority((u16)temp_u32) == 0){
					  shell_output(s,"Command: config stp priority ",ptr3);
					  shell_output(s,"Success.","");
					  settings_add2queue(SQ_STP);
					  return;
				  }else{
					  shell_output(s,"ERROR: STP Bridge Priority: ","Incorrect Value");
					  return;
				  }
			  }
			  else{
				  ptr2 = findcmd(s,ptr1,"hellotime");
				  if(ptr2 != NULL){
					  if(autocompleat(s)){
						return;
					  }
					  ptr3 = findcmd(s,ptr2," ");
					  temp_u32 = strtol(ptr3,&ptr4,10);
					  if(set_stp_bridge_htime((u8)temp_u32) == 0){
						  shell_output(s,"Command: config stp hellotime ",ptr3);
						  shell_output(s,"Success.","");
						  settings_add2queue(SQ_STP);
						  return;
					  }else{
						  shell_output(s,"ERROR: STP Hello Time: ","Incorrect Value");
						  return;
					  }
				  }
				  else
				  {
					  ptr2 = findcmd(s,ptr1,"version");
					  if(ptr2 != NULL){
						  ptr3 = findcmd(s,ptr2,"rstp");
						  if(ptr3 != NULL){
							  if(autocompleat(s)){
								return;
							  }
							  shell_output(s,"Command: config stp version rstp","");
							  shell_output(s,"Success.","");
							  set_stp_proto(BSTP_PROTO_RSTP);
							  settings_add2queue(SQ_STP);
							  return;
						  }
						  else
						  {
							  ptr3 = findcmd(s,ptr2,"stp");
							  if(ptr3 != NULL){
								  if(autocompleat(s)){
									return;
								  }
								  shell_output(s,"Command: config stp version stp","");
								  shell_output(s,"Success.","");
								  set_stp_proto(BSTP_PROTO_STP);
								  settings_add2queue(SQ_STP);
								  return;
							  }
							  else{
								  if(autocompleat(s)){
									return;
								  }
								  shell_output(s,"ERROR: Command","");
								  shell_output(s,"Next possible completions:","");
								  shell_output(s,"stp","");
								  shell_output(s,"rstp","");
								  return;
							  }
						  }
					  }
					  else{
						  ptr2 = findcmd(s,ptr1,"txholdcount");
						  if(ptr2 != NULL){
							  if(autocompleat(s)){
								return;
							  }
							  ptr3 = findcmd(s,ptr2," ");
							  temp_u32 = strtol(ptr3,&ptr4,10);
							  if(set_stp_txholdcount((u8)temp_u32) == 0){
								  shell_output(s,"Command: config stp txholdcount ",ptr3);
								  shell_output(s,"Success.","");
								  settings_add2queue(SQ_STP);
								  return;
							  }else{
								  shell_output(s,"ERROR: STP TX Hold Count: ","Incorrect Value");
								  return;
							  }
						  }
						  else
						  {
							  ptr2 = findcmd(s,ptr1,"maxage");
							  if(ptr2 != NULL){
								  if(autocompleat(s)){
									return;
								  }
								  ptr3 = findcmd(s,ptr2," ");
								  temp_u32 = strtol(ptr3,&ptr4,10);
								  if(set_stp_bridge_max_age((u8)temp_u32) == 0){
									  shell_output(s,"Command: config stp maxage ",ptr3);
									  shell_output(s,"Success.","");
									  settings_add2queue(SQ_STP);
									  return;
								  }else{
									  shell_output(s,"ERROR: STP Bridge Max Age: ","Incorrect Value");
									  return;
								  }
							  }
							  else
							  {
								  ptr2 = findcmd(s,ptr1,"forwarddelay");
								  if(ptr2 != NULL){
									  if(autocompleat(s)){
										return;
									  }
									  ptr3 = findcmd(s,ptr2," ");
									  temp_u32 = strtol(ptr3,&ptr4,10);
									  if(set_stp_bridge_fdelay((u8)temp_u32) == 0){
										  shell_output(s,"Command: config stp forvarddelay ",ptr3);
										  shell_output(s,"Success.","");
										  settings_add2queue(SQ_STP);
										  return;
									  }else{
										  shell_output(s,"ERROR: STP Forward Delay Time: ","Incorrect Value");
										  return;
									  }
								  }
								  else{
									  ptr2 = findcmd(s,ptr1,"forward_bpdu");
									  if(ptr2!=NULL){
										  ptr3=findcmd(s,ptr2,"state");
										  if(ptr3!=NULL){
											  ptr4=findcmd(s,ptr3,"enable");
											  if(ptr4!=NULL){
												  if(autocompleat(s)){
													  return;
												  }
												  shell_output(s,"Command: config stp forward_bpdu state enable","");
												  shell_output(s,"Success.","");
												  set_stp_bpdu_fw(ENABLE);
												  settings_add2queue(SQ_STP);
												  return;
											  }
											  else{
												  ptr4=findcmd(s,ptr3,"disable");
												  if(ptr4!=NULL){
													  if(autocompleat(s)){
														  return;
													  }
													  shell_output(s,"Command: config stp forward_bpdu state disable","");
													  shell_output(s,"Success.","");
													  set_stp_bpdu_fw(DISABLE);
													  settings_add2queue(SQ_STP);
													  return;
												  }
												  else{
													  if(autocompleat(s)){
														  return;
													  }
													  shell_output(s,"ERROR: Command","");
													  shell_output(s,"Next possible completions:","");
													  shell_output(s,"enable","");
													  shell_output(s,"disable","");
													  return;
												  }
											  }
										  }
										  else{
											  if(autocompleat(s)){
												return;
											  }
											  shell_output(s,"ERROR: Command","");
											  shell_output(s,"Next possible completions:","");
											  shell_output(s,"state","");
											  return;
										  }
									  }
									  else{
										  if(autocompleat(s)){
											return;
										  }
										  shell_output(s,"ERROR: Command","");
										  shell_output(s,"Next possible completions:","");
										  shell_output(s,"state","");
										  shell_output(s,"version","");
										  shell_output(s,"priority","");
										  shell_output(s,"hellotime","");
										  shell_output(s,"txholdcount","");
										  shell_output(s,"maxage","");
										  shell_output(s,"forwarddelay","");
										  shell_output(s,"forward_bpdu","");
										  return;
									  }
								  }
							  }
						  }
					  }
				  }

			  }
		  }
	  }
  }


  ptr1 = findcmd(s,str,"igmp_snooping");
  if(ptr1 != NULL){
	  ptr2 = findcmd(s,ptr1,"portlist");
	  if(ptr2 != NULL){
		  if(autocompleat(s)){
			return;
		  }
		  ptr3 = find_port(ptr2,ports);
		  if(ptr3 == NULL){
		 	 shell_output(s,"ERROR: Incorrect port list","");
		 	 return;
		  }
		  else{
			  for(u8 i = 0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
				  if(ports[i]==1)
					  set_igmp_port_state(i,ENABLE);
				  else
					  set_igmp_port_state(i,DISABLE);
			  }
			  shell_output(s,"Command: config igmp_snooping portlist ",ports_print(ports));
			  shell_output(s,"Success.","");
			  settings_add2queue(SQ_IGMP);
			  return;
		  }
	  }else
	  {
		  ptr2 = findcmd(s,ptr1,"state");
		  if(ptr2 != NULL){
			  ptr3 = findcmd(s,ptr2,"enable");
			  if(ptr3 != NULL){
				  if(autocompleat(s)){
					return;
				  }
				  shell_output(s,"Command: config igmp_snooping state enable","");
				  shell_output(s,"Success.","");
				  set_igmp_snooping_state(ENABLE);
				  settings_add2queue(SQ_IGMP);
				  return;
			  }
			  else{
				  ptr3 = findcmd(s,ptr2,"disable");
				  if(ptr3 != NULL){
					  if(autocompleat(s)){
						return;
					  }
					  shell_output(s,"Command: config igmp_snooping state disable","");
					  shell_output(s,"Success.","");
					  set_igmp_snooping_state(DISABLE);
					  settings_add2queue(SQ_IGMP);
					  return;
				  }
				  else{
					  if(autocompleat(s)){
						  return;
					  }
					  shell_output(s,"ERROR: Command","");
					  shell_output(s,"Next possible completions:","");
					  shell_output(s,"enable","");
					  shell_output(s,"disable","");
					  return;
				  }
			  }
		  }
		  else{
			  ptr2 = findcmd(s,ptr1,"query_interval");
			  if(ptr2 != NULL){
				  if(autocompleat(s)){
					return;
				  }
				  ptr3 = findcmd(s,ptr2," ");
				  if(ptr3 != NULL){
					  temp_u32 = strtol(ptr3,&ptr4,10);
					  if(set_igmp_query_int((u8)temp_u32) == 0){
						  shell_output(s,"Command: config igmp_snooping query_interval ",ptr3);
						  shell_output(s,"Success.","");
						  settings_add2queue(SQ_IGMP);
						  return;
					  }else{
						  shell_output(s,"ERROR: IGMP Snooping config: Query interval ","Incorrect Value");
						  return;
					  }
				  }
				  else{
					  shell_output(s,"ERROR: IGMP Snooping config: Query interval ","Incorrect Value");
					  return;
				  }
			  }
			  else
			  {
				  ptr2 = findcmd(s,ptr1,"query_response_interval");
				  if(ptr2 != NULL){
					  if(autocompleat(s)){
						return;
					  }
					  ptr3 = findcmd(s,ptr2," ");
					  if(ptr3!=NULL){
						  temp_u32 = strtol(ptr3,&ptr4,10);
						  if(set_igmp_max_resp_time((u8)temp_u32) == 0){
							  shell_output(s,"Command: config igmp_snooping query_response_interval ",ptr3);
							  shell_output(s,"Success.","");
							  settings_add2queue(SQ_IGMP);
							  return;
						  }else{
							  shell_output(s,"ERROR: IGMP Snooping config: Query response interval ","Incorrect Value");
							  return;
						  }
					  }
					  else{
						  shell_output(s,"ERROR: IGMP Snooping config: Query response interval ","Incorrect Value");
						  return;
					  }
				  }
				  else{
					  ptr2 = findcmd(s,ptr1,"group_membership_time");
					  if(ptr2 != NULL){
						  if(autocompleat(s)){
							return;
						  }
						  ptr3 = findcmd(s,ptr2," ");
						  if(ptr3!=NULL){
							  temp_u32 = strtol(ptr3,&ptr4,10);
							  if(set_igmp_group_membership_time((u8)temp_u32) == 0){
								  shell_output(s,"Command: config igmp_snooping group_membership_time ",ptr3);
								  shell_output(s,"Success.","");
								  settings_add2queue(SQ_IGMP);
								  return;
							  }else{
								  shell_output(s,"ERROR: IGMP Snooping config: Group Membership Time ","Incorrect Value");
								  return;
							  }
						  }
						  else{
							  shell_output(s,"ERROR: IGMP Snooping config: Group Membership Time ","Incorrect Value");
							  return;
						  }
					  }
					  else{
						  ptr2 = findcmd(s,ptr1,"other_querier_present_int");
						  if(ptr2 != NULL){
							  if(autocompleat(s)){
								return;
							  }
							  ptr3 = findcmd(s,ptr2," ");
							  if(ptr3!=NULL){
								  temp_u32 = strtol(ptr3,&ptr4,10);
								  if(set_igmp_other_querier_time((u8)temp_u32) == 0){
									  shell_output(s,"Command: config igmp_snooping other_querier_present_int ",ptr3);
									  shell_output(s,"Success.","");
									  settings_add2queue(SQ_IGMP);
									  return;
								  }else{
									  shell_output(s,"ERROR: IGMP Snooping config: Other Querier Present Interval ","Incorrect Value");
									  return;
								  }
							  }
							  else{
								  shell_output(s,"ERROR: IGMP Snooping config: Other Querier Present Interval ","Incorrect Value");
								  return;
							  }
						  }
						  else{
							  ptr2 = findcmd(s,ptr1,"send_query");
							  if(ptr2 != NULL){
								  ptr3 = findcmd(s,ptr2,"enable");
								  if(ptr3 != NULL){
									  if(autocompleat(s)){
										return;
									  }
									  shell_output(s,"Command: config igmp_snooping send_query enable","");
									  shell_output(s,"Success.","");
									  set_igmp_query_mode(ENABLE);
									  settings_add2queue(SQ_IGMP);
									  return;
								  }
								  else{
									  ptr3 = findcmd(s,ptr2,"disable");
									  if(ptr3 != NULL){
										  if(autocompleat(s)){
											return;
										  }
										  shell_output(s,"Command: config igmp_snooping send_query disable","");
										  shell_output(s,"Success.","");
										  set_igmp_query_mode(DISABLE);
										  settings_add2queue(SQ_IGMP);
										  return;
									  }
									  else{
										  if(autocompleat(s)){
											  return;
										  }
										  shell_output(s,"ERROR: Command","");
										  shell_output(s,"Next possible completions:","");
										  shell_output(s,"enable","");
										  shell_output(s,"disable","");
										  return;
									  }
								  }
							  }
							  else{
								  shell_output(s,"ERROR: Command","");
							  	  shell_output(s,"Next possible completions:","");
								  shell_output(s,"state","");
								  shell_output(s,"portlist","");
								  shell_output(s,"query_interval","");
								  shell_output(s,"query_response_interval","");
								  shell_output(s,"group_membership_time","");
								  shell_output(s,"other_querier_present_int","");
								  shell_output(s,"send_query","");
								  return;
							  }
						  }
					  }
				  }
			  }
		  }
	  }
  }

  ptr1 = findcmd(s,str,"smtp");
  if(ptr1 != NULL){
	  ptr2 = findcmd(s,ptr1,"server");
	  if(ptr2 != NULL){
		  if(autocompleat(s)){
			return;
		  }
		  if(find_ip(ptr2,&ip) == 1){
			  shell_output(s,"Command: config smtp server ",ptr2);
			  shell_output(s,"Success.","");
			  set_smtp_server(ip);
			  settings_add2queue(SQ_SMTP);
			  return;
		  }
		  else{
			  ptr2 = findcmd(s,ptr1,"server_addr");
			  if(autocompleat(s)){
				return;
			  }
			  ptr3 = findcmd(s,ptr2," ");
			  shell_output(s,"Command: config smtp server_addr ",ptr3);
			  shell_output(s,"Success.","");
			  set_smtp_domain(ptr3);
			  settings_add2queue(SQ_SMTP);
			  return;
		  }
	  }
	  else{
		  ptr2 = findcmd(s,ptr1,"server_port");
		  if(ptr2 != NULL){
			  if(autocompleat(s)){
				return;
			  }
			  ptr3 = findcmd(s,ptr2," ");
			  temp_u32 = strtoul(ptr3,NULL,10);
			  if(set_smtp_port((u16)temp_u32)==0){
				  shell_output(s,"Command: config smtp server_port ",ptr3);
				  shell_output(s,"Success.","");
				  settings_add2queue(SQ_SMTP);
				  return;
			  }
			  else{
				  shell_output(s,"ERROR: SMTP config: server port"," Incorrect value");
				  return;
			  }
		  }
		  else{
			  ptr2 = findcmd(s,ptr1,"mail_sender");
			  if(ptr2 != NULL){
				 if(autocompleat(s)){
					return;
				 }
				 ptr3 = findcmd(s,ptr2," ");
				 shell_output(s,"Command: config smtp mail_sender ",ptr3);
				 shell_output(s,"Success.","");
				 set_smtp_from(ptr3);
				 settings_add2queue(SQ_SMTP);
				 return;
			  }
			  else{
				  ptr2 = findcmd(s,ptr1,"mail_receiver1");
				  if(ptr2 != NULL){
					 if(autocompleat(s)){
						return;
					 }
					 ptr3 = findcmd(s,ptr2," ");
					 set_smtp_to(ptr3);
					 shell_output(s,"Command: config smtp mail_receiver1 ",ptr3);
					 shell_output(s,"Success.","");
					 settings_add2queue(SQ_SMTP);
					 return;
				  }
				  else
				  {
					  ptr2 = findcmd(s,ptr1,"mail_receiver2");
					  if(ptr2 != NULL){
						 ptr3 = findcmd(s,ptr2," ");
						 set_smtp_to2(ptr3);
						 shell_output(s,"Command: config smtp mail_receiver2 ",ptr3);
						 shell_output(s,"Success.","");
						 settings_add2queue(SQ_SMTP);
						 return;
					  }
					  else{
						  ptr2 = findcmd(s,ptr1,"mail_receiver3");
						  if(ptr2 != NULL){
							 ptr3 = findcmd(s,ptr2," ");
							 set_smtp_to3(ptr3);
							 shell_output(s,"Command: config smtp mail_receiver3 ",ptr3);
							 shell_output(s,"Success.","");
							 settings_add2queue(SQ_SMTP);
							 return;
						  }
						  else{
							  ptr2 = findcmd(s,ptr1,"login");
							  if(ptr2 != NULL){
								 if(autocompleat(s)){
									return;
								 }
								 ptr3 = findcmd(s,ptr2," ");
								 set_smtp_login(ptr3);
								 shell_output(s,"Command: config smtp login ",ptr3);
								 shell_output(s,"Success.","");
								 settings_add2queue(SQ_SMTP);
								 return;
							  }
							  else{
								  ptr2 = findcmd(s,ptr1,"password");
								  if(ptr2 != NULL){
									 if(autocompleat(s)){
										return;
									 }
									 ptr3 = findcmd(s,ptr2," ");
									 set_smtp_pass(ptr3);
									 shell_output(s,"Command: config smtp password ",ptr3);
									 shell_output(s,"Success.","");
									 settings_add2queue(SQ_SMTP);
									 return;
								  }
								  else{
									  ptr2 = findcmd(s,ptr1,"state");
									  if(ptr2 != NULL){
										 ptr3 = findcmd(s,ptr2,"enable");
										 if(ptr3 != NULL){
											 if(autocompleat(s)){
												return;
											 }
											 shell_output(s,"Command: config smtp state enable","");
											 shell_output(s,"Success.","");
											 set_smtp_state(ENABLE);
											 settings_add2queue(SQ_SMTP);
											 return;
										 }
										 else{
											 ptr3 = findcmd(s,ptr2,"disable");
											 if(ptr3 != NULL){
												 if(autocompleat(s)){
													return;
												 }
												 shell_output(s,"Command: config smtp state disable","");
												 shell_output(s,"Success.","");
												 set_smtp_state(DISABLE);
												 settings_add2queue(SQ_SMTP);
												 return;
											 }
											 else{
												  if(autocompleat(s)){
													  return;
												  }
												  shell_output(s,"ERROR: Command","");
												  shell_output(s,"Next possible completions:","");
												  shell_output(s,"state","");
												  shell_output(s,"disable","");
												  return;
											 }
										 }

									  }
									  else{
										  if(autocompleat(s)){
											  return;
										  }
										  shell_output(s,"ERROR: Command","");
										  shell_output(s,"Next possible completions:","");
										  shell_output(s,"state","");
										  shell_output(s,"server","");
										  shell_output(s,"server_port","");
										  shell_output(s,"mail_sender","");
										  shell_output(s,"mail_receiver1","");
										  shell_output(s,"mail_receiver2","");
										  shell_output(s,"mail_receiver3","");
										  shell_output(s,"login","");
										  shell_output(s,"password","");
										  return;
									  }
								  }
							  }
						  }
					  }
				  }
			  }
		  }
	  }
  }

  //snmp
  ptr1 = findcmd(s,str,"snmp");
  if(ptr1!=NULL){
	  ptr2 = findcmd(s,ptr1,"state");
	  if(ptr2 != NULL){
		 ptr3 = findcmd(s,ptr2,"enable");
		 if(ptr3 != NULL){
			 if(autocompleat(s)){
				return;
			 }
			 shell_output(s,"Command: config snmp state enable","");
			 shell_output(s,"Success.","");
			 set_snmp_state(ENABLE);
			 settings_add2queue(SQ_SNMP);
			 return;
		 }
		 else{
			 ptr3 = findcmd(s,ptr2,"disable");
			 if(ptr3 != NULL){
				 if(autocompleat(s)){
					return;
				 }
				 shell_output(s,"Command: config snmp state disable","");
				 shell_output(s,"Success.","");
				 set_snmp_state(DISABLE);
				 settings_add2queue(SQ_SNMP);
				 return;
			 }
			 else{
				  if(autocompleat(s)){
					  return;
				  }
				  shell_output(s,"ERROR: Command","");
				  shell_output(s,"Next possible completions:","");
				  shell_output(s,"state","");
				  shell_output(s,"disable","");
				  return;
			 }
		 }
	  }
	  else{
		  	ptr2 = findcmd(s,ptr1,"host");
		  	if(ptr2 != NULL){
				if(autocompleat(s)){
					return;
				}
		  		if(find_ip(ptr2,&ip) == 1){
		  			shell_output(s,"Command: config snmp host ",ptr2);
		  			shell_output(s,"Success.","");
		  			set_snmp_serv(ip);
		  			settings_add2queue(SQ_SNMP);
					return;
				}else{
					shell_output(s,"ERROR: incorrect IP address","");
					return;
				}
		  	}
		  	else
		  	{
		  		ptr2 = findcmd(s,ptr1,"read_community");
		  		if(ptr2!= NULL){
					if(autocompleat(s)){
						return;
					}
		  			ptr3 = findcmd(s,ptr2," ");
		  			set_snmp1_read_communitie(ptr3);
		  			shell_output(s,"Command: config snmp read_community ",ptr3);
		  			shell_output(s,"Success.","");
		  			settings_add2queue(SQ_SNMP);
		  			return;
		  		}
		  		else{
			  		ptr2 = findcmd(s,ptr1,"write_community");
			  		if(ptr2!= NULL){
						if(autocompleat(s)){
							return;
						}
			  			ptr3 = findcmd(s,ptr2," ");
			  			set_snmp1_write_communitie(ptr3);
			  			shell_output(s,"Command: config snmp write_community ",ptr3);
			  			shell_output(s,"Success.","");
			  			settings_add2queue(SQ_SNMP);
			  			return;
			  		}
			  		else{
			  			ptr2 = findcmd(s,ptr1,"level");
			  			if(ptr2!=NULL){
						    if(autocompleat(s)){
								return;
							}
							ptr3 = findcmd(s,ptr2," ");
							temp_u32 = strtoul(ptr3,NULL,10);
							if(temp_u32 >2){
								shell_output(s,"ERROR: SNMP: Incorrect Security Level","");
								return;
							}
							if(temp_u32 == 0)
								set_snmp3_level(0,NO_AUTH_NO_PRIV);
							if (temp_u32 == 1)
								set_snmp3_level(0,AUTH_NO_PRIV);
							if (temp_u32 == 2)
								set_snmp3_level(0,AUTH_PRIV);
							shell_output(s,"Command: config snmp level ",ptr3);
							shell_output(s,"Success.","");
							settings_add2queue(SQ_SNMP);
							return;
			  			}else{
			  				ptr2 = findcmd(s,ptr1,"engine_id");
							if(ptr2!=NULL){
								if(autocompleat(s)){
									return;
								}
								ptr3 = findcmd(s,ptr2," ");
								if(str_to_engineid(ptr3,&eid) == -1){
									shell_output(s,"ERROR: SNMP: Incorrect Engine ID format","");
									return;
								}
								set_snmp3_engine_id(&eid);
								shell_output(s,"Command: config snmp engine_id ",ptr3);
								shell_output(s,"Success.","");
								settings_add2queue(SQ_SNMP);
								return;
							}
							else{
								ptr2 = findcmd(s,ptr1,"user_name");
								if(ptr2!=NULL){
									if(autocompleat(s)){
										return;
									}
									ptr3 = findcmd(s,ptr2," ");
									set_snmp3_user_name(0,ptr3);
									shell_output(s,"Command: config snmp user_name ",ptr3);
									shell_output(s,"Success.","");
									settings_add2queue(SQ_SNMP);
									return;
								}
								else{
									ptr2 = findcmd(s,ptr1,"auth_pass");
									if(ptr2!=NULL){
										if(autocompleat(s)){
											return;
										}
										ptr3 = findcmd(s,ptr2," ");
										set_snmp3_auth_pass(0,ptr3);
										shell_output(s,"Command: config snmp auth_pass ",ptr3);
										shell_output(s,"Success.","");
										settings_add2queue(SQ_SNMP);
										return;
									}
									else{
										ptr2 = findcmd(s,ptr1,"priv_pass");
										if(ptr2!=NULL){
											if(autocompleat(s)){
												return;
											}
											ptr3 = findcmd(s,ptr2," ");
											set_snmp3_priv_pass(0,ptr3);
											shell_output(s,"Command: config snmp priv_pass ",ptr3);
											shell_output(s,"Success.","");
											settings_add2queue(SQ_SNMP);
											return;
										}
										else{
											ptr2 = findcmd(s,ptr1,"version");
											if(ptr2!=NULL){
												if(autocompleat(s)){
													return;
												}
												ptr3 = findcmd(s,ptr2," ");
												temp_u32 = strtoul(ptr3,NULL,10);
												if((temp_u32 != 1)&&(temp_u32 != 3)){
													shell_output(s,"ERROR: SNMP: Incorrect Version","");
													return;
												}
												set_snmp_vers((u8)temp_u32);
												shell_output(s,"Command: config snmp version ",ptr3);
												shell_output(s,"Success.","");
												settings_add2queue(SQ_SNMP);
												return;
											}
											else{
												if(autocompleat(s)){
												  return;
												}
												shell_output(s,"ERROR: Command","");
												shell_output(s,"Next possible completions:","");
												shell_output(s,"state","");
												shell_output(s,"version","");
												shell_output(s,"host","");
												shell_output(s,"read_community","");
												shell_output(s,"write_community","");
												shell_output(s,"level","");
												shell_output(s,"engine_id","");
												shell_output(s,"user_name","");
												shell_output(s,"auth_pass","");
												shell_output(s,"priv_pass","");
												return;
											}
										}
									}
								}
							}


			  			}
			  		}
		  		}
		  	}

	  }

  }



  //vlan
  u32 first_vid,last_vid;
  ptr1 = findcmd(s,str,"vlan");
  if(ptr1 != NULL){
	  ptr2 = findcmd(s,ptr1,"vlanid");
	  if(ptr2 != NULL){
		  if(autocompleat(s)){
			  return;
		  }
		  ptr3 = findcmd(s,ptr2," ");
		  if(ptr3==NULL){
			  shell_output(s,"ERROR: Incorrect command","");
			  return;
		  }


		  ptr4 = find_interval(ptr3, &first_vid,&last_vid);
		  if(ptr4 == NULL){
			  //first_vid = last_vid= strtol(ptr3,&ptr3,10);
			  //if(first_vid>4095 || first_vid == 0){
				  shell_output(s,"ERROR: VLAN: Incorrect VID","");
				  shell_output(s,"Next possible completions: VID[1..4094] or VID interval[1..4094]-[1..4094]","");
				  return;
			  //}
		  }
		  else{
			  if(first_vid>4095 || last_vid>4095){
				  shell_output(s,"ERROR: VLAN: Incorrect VID","");
				  return;
			  }

			  ptr5 = findcmd(s,ptr4,"add");
			  if(ptr5 != NULL){
				  if(autocompleat(s)){
					return;
				  }
				  ptr6 = findcmd(s,ptr5,"untagged");
				  if(ptr6 != NULL){
					  if(autocompleat(s)){
						return;
					  }
					  ptr7 = findcmd(s,ptr5," ");
					  ptr8 = find_port(ptr7,ports);
					  if(ptr8 != NULL){
						  	for(u16 i=first_vid;i<=last_vid;i++){
						  		temp_u32 = vlan_vid2num(i);
						  		if(vlan_vid2num(i)==-1){
									//create new vlan
									temp_u32 = get_vlan_sett_vlannum();
									set_vlan_sett_vlannum(++temp_u32);
									temp_u32--;
									set_vlan_state(temp_u32,ENABLE);
									set_vlan_vid(temp_u32,i);
						  		}
								//addport
								for(u8 i=0;i<ALL_PORT_NUM;i++){
									if(ports[i] == 1){
										set_vlan_port(temp_u32,i,2);//untagged
									}
								}
						  	}
						  	if(first_vid==last_vid)
						  		sprintf(temp,"%d add untagged %s",first_vid,ports_print(ports));
						  	else
						  		sprintf(temp,"%d-%d add untagged %s",first_vid,last_vid,ports_print(ports));
							shell_output(s,"Command: config vlan vlanid ",temp);
							shell_output(s,"Success.","");
							settings_add2queue(SQ_VLAN);
							return;

					  }
					  else{
						  shell_output(s,"ERROR: Incorrect portlist","");
						  return;
					  }
				  }
				  ptr6 = findcmd(s,ptr5,"tagged");
				  if(ptr6 != NULL){
					  if(autocompleat(s)){
						return;
					  }
					  ptr7 = findcmd(s,ptr6," ");
					  ptr8 = find_port(ptr7,ports);
					  if(ptr8 != NULL){
						  for(u16 j=first_vid;j<=last_vid;j++){
							  temp_u32 = vlan_vid2num(j);
							  if(vlan_vid2num(j)==-1){
									//create new vlan
									temp_u32 = get_vlan_sett_vlannum();
									set_vlan_sett_vlannum(++temp_u32);
									temp_u32--;
									set_vlan_state(temp_u32,ENABLE);
									set_vlan_vid(temp_u32,j);
							  }
							  //addport
							  for(u8 i=0;i<ALL_PORT_NUM;i++){
								  if(ports[i] == 1){
										set_vlan_port(temp_u32,i,3);//tagged
								  }
							  }
						  }
						  if(first_vid==last_vid)
							  sprintf(temp,"%d add tagged %s",first_vid,ports_print(ports));
						  else
							  sprintf(temp,"%d-%d add tagged %s",first_vid,last_vid,ports_print(ports));
						  shell_output(s,"Command: config vlan vlanid ",temp);
						  shell_output(s,"Success.","");
						  settings_add2queue(SQ_VLAN);
						  return;
					  }
					  else{
						  shell_output(s,"ERROR: incorrect portlist","");
						  return;
					  }
				  }
				  ptr6 = findcmd(s,ptr5,"not_memb");
				  if(ptr6 != NULL){
					  if(autocompleat(s)){
						return;
					  }
					  ptr7 = findcmd(s,ptr5," ");
					  ptr8 = find_port(ptr6,ports);
					  if(ptr8 != NULL){
						    for(u16 j=first_vid;j<=last_vid;j++){
						    	temp_u32 = vlan_vid2num(j);
						    	if(vlan_vid2num(j)==-1){
									//create new vlan
									temp_u32 = get_vlan_sett_vlannum();
									set_vlan_sett_vlannum(++temp_u32);
									temp_u32--;
									set_vlan_state(temp_u32,ENABLE);
									set_vlan_vid(temp_u32,vid);
						    	}
								//addport
								for(u8 i=0;i<ALL_PORT_NUM;i++){
									if(ports[i] == 1){
										set_vlan_port(temp_u32,i,0);//not a member
									}
								}
						    }
						    if(first_vid==last_vid)
						    	sprintf(temp,"%d add not_memb %s",first_vid,ports_print(ports));
						    else
						    	sprintf(temp,"%d-%d add not_memb %s",first_vid,last_vid,ports_print(ports));
							shell_output(s,"Command: config vlan vlanid ",temp);
							shell_output(s,"Success.","");
							settings_add2queue(SQ_VLAN);
							return;
					  }
					  else{
						  shell_output(s,"ERROR: Incorrect portlist","");
						  return;
					  }
				  }
				  else{
					  if(autocompleat(s)){
						  return;
					  }
					  shell_output(s,"ERROR: Command","");
					  shell_output(s,"Next possible completions:","");
					  shell_output(s,"untagged","");
					  shell_output(s,"tagged","");
					  shell_output(s,"not_memb","");
					  return;
				  }
			  }else{
				  ptr5 = findcmd(s,ptr4,"delete");
				  if(ptr5 != NULL){
					  if(autocompleat(s)){
						return;
					  }
					  ptr6 = findcmd(s,ptr5,"untagged");
					  if(ptr6 != NULL){
						  if(autocompleat(s)){
							return;
						  }
						  ptr7 = findcmd(s,ptr5," ");
						  ptr8 = find_port(ptr6,ports);
						  if(ptr8 != NULL){
							  for(u16 j=first_vid;j<=last_vid;j++){
								temp_u32 = vlan_vid2num(j);
							  	if(vlan_vid2num(j)==-1){
							  		sprintf(temp,"ERROR: VID %d doesn't exist",j);
							  		shell_output(s,temp,"");
							  		return;
							  	}
							  	//dell port
								for(u8 i=0;i<ALL_PORT_NUM;i++){
									if(ports[i] == 1){
										if(get_vlan_port_state(temp_u32,i)==2){
											set_vlan_port(temp_u32,i,0);//not a member
										}
										else{
									  		sprintf(temp,"ERROR: port %d doesn't exist in VLAN %d",i+1,j);
									  		shell_output(s,temp,"");
									  		return;
										}

									}
								}
							  }
							  if(first_vid == last_vid)
									sprintf(temp,"%d delete untagged %s",first_vid,ports_print(ports));
							  else
									sprintf(temp,"%d-%d delete untagged %s",first_vid,last_vid,ports_print(ports));
								shell_output(s,"Command: config vlan vlanid ",temp);
								shell_output(s,"Success.","");
								settings_add2queue(SQ_VLAN);
								return;
						  }
						  else{
							  shell_output(s,"ERROR: Incorrect portlist","");
							  return;
						  }
					  }
					  else{
						  ptr6 = findcmd(s,ptr5,"tagged");
						  if(ptr6 != NULL){
							  if(autocompleat(s)){
								 return;
							  }
							  ptr7 = findcmd(s,ptr6," ");
							  ptr8 = find_port(ptr7,ports);
							  if(ptr8 != NULL){
								  for(u16 j=first_vid;j<=last_vid;j++){
								  	  temp_u32 = vlan_vid2num(j);
								  	  if(vlan_vid2num(j)==-1){
								  		 sprintf(temp,"ERROR: VID %d doesn't exist",j);
								  		 shell_output(s,temp,"");
								  		 return;
								  	  }
								  	  //dell port
									  for(u8 i=0;i<ALL_PORT_NUM;i++){
										 if(ports[i] == 1){
											 if(get_vlan_port_state(temp_u32,i)==3){
												 set_vlan_port(temp_u32,i,0);//not a member
											 }
											 else{
											  	 sprintf(temp,"ERROR: port %d doesn't exist in VLAN %d",i+1,j);
											  	 shell_output(s,temp,"");
											  	 return;
											 }
										 }
									  }
								  }
								  if(first_vid == last_vid)
									  sprintf(temp,"%d delete tagged %s",first_vid,ports_print(ports));
								  else
									  sprintf(temp,"%d-%d delete tagged %s",last_vid,ports_print(ports));
								  shell_output(s,"Command: config vlan vlanid ",temp);
								  shell_output(s,"Success.","");
								  settings_add2queue(SQ_VLAN);
								  return;
							  }
							  else{
								  shell_output(s,"ERROR: incorrect portlist","");
								  return;
							  }
						  }
						  else{
							  ptr6 = findcmd(s,ptr5,"vlan");
							  if(ptr6 != NULL){
								  if(autocompleat(s)){
									 return;
								  }
								  for(u16 j=first_vid;j<=last_vid;j++){
									  delete_vlan(j);
								  }
								  settings_add2queue(SQ_VLAN);
								  if(first_vid == last_vid)
									  sprintf(temp,"%d delete vlan",first_vid);
								  else
									  sprintf(temp,"%d-%d delete vlan",first_vid,last_vid);
								  shell_output(s,"Command: config vlan vlanid ",temp);
								  shell_output(s,"Success.","");

								  return;
							  }
							  else{
								  if(autocompleat(s)){
									  return;
								  }
								  shell_output(s,"ERROR: Command","");
								  shell_output(s,"Next possible completions:","");
								  shell_output(s,"untagged","");
								  shell_output(s,"tagged","");
								  shell_output(s,"vlan","");
								  return;
							  }
						  }
					  }
				  }
				  else{
					  ptr5 = findcmd(s,ptr4,"name");
					  if(ptr5 != NULL){
						  if(autocompleat(s)){
							 return;
						  }
						  ptr6 = findcmd(s,ptr5," ");
						  for(u16 j=first_vid;j<=last_vid;j++){
							  if(vlan_vid2num(j)==-1){
								  sprintf(temp,"ERROR: VID %d not found",j);
								  shell_output(s,temp,"");
								  return;
							  }else{
								  set_vlan_name(vlan_vid2num(j),ptr5);
							  }
						  }
						  if(first_vid == last_vid)
							  sprintf(temp,"%d name %s",first_vid,ptr5);
						  else
							  sprintf(temp,"%d-%d name %s",first_vid,last_vid,ptr5);
						  shell_output(s,"Command: config vlan vlanid ",temp);
						  shell_output(s,"Success.","");
						  settings_add2queue(SQ_VLAN);
						  return;
					  }
					  else{
						  if(autocompleat(s)){
							  return;
						  }
						  shell_output(s,"ERROR: Command","");
						  shell_output(s,"Next possible completions:","");
						  shell_output(s,"add","");
						  shell_output(s,"delete","");
						  shell_output(s,"name","");
						  return;
					  }
				  }
			  }
		  }
	  }
	  else
	  {
		  ptr2 = findcmd(s,ptr1,"mngt_vlan");
		  if(ptr2!=NULL){
			  if(autocompleat(s)){
				return;
			  }
			  ptr3 = findcmd(s,ptr2," ");
			  temp_u32 = strtoul(ptr3,NULL,10);
			  if((temp_u32 > 4095)||(temp_u32 == 0)){
				  shell_output(s,"ERROR: VLAN: Incorrect VID","");
				  return;
			  }
			  else
				  vid = temp_u32;

			  sprintf(temp,"%d",vid);
			  shell_output(s,"Command: config vlan mngt_vlan ",temp);
			  shell_output(s,"Success.","");
			  set_vlan_sett_mngt(vid);
			  settings_add2queue(SQ_VLAN);
			  return;
		  }
		  else{
			  ptr2 = findcmd(s,ptr1,"vlan_trunking");
			  if(ptr2!=NULL){
				  ptr3 = findcmd(s,ptr2,"state");
				  if(ptr3!=NULL){
					  ptr4 = findcmd(s,ptr3,"enable");
					  if(ptr4!=NULL){
						  if(autocompleat(s)){
							return;
						  }
						  shell_output(s,"Command: config vlan vlan_trunking state enable","");
						  shell_output(s,"Success.","");
						  set_vlan_trunk_state(ENABLE);
						  settings_add2queue(SQ_VLAN);
						  return;
					  }
					  else{
						  ptr4 = findcmd(s,ptr3,"disable");
						  if(ptr4!=NULL){
							  if(autocompleat(s)){
								return;
							  }
							  shell_output(s,"Command: config vlan vlan_trunking state disable","");
							  shell_output(s,"Success.","");
							  set_vlan_trunk_state(DISABLE);
							  settings_add2queue(SQ_VLAN);
							  return;
						  }
						  else{
							  if(autocompleat(s)){
								  return;
							  }
							  shell_output(s,"ERROR: Command","");
							  shell_output(s,"Next possible completions:","");
							  shell_output(s,"enable","");
							  shell_output(s,"disable","");
							  return;
						  }
					  }
				  }
				  else{
					  ptr3 = findcmd(s,ptr2,"ports");
					  if(ptr3!=NULL){
						 ptr4 = find_port(ptr3,ports);
						 if(ptr4 != NULL){
							ptr5 = findcmd(s,ptr4,"state");
							if(ptr5!=NULL){
								ptr6 = findcmd(s,ptr5,"enable");
								if(ptr6!=NULL){
									if(autocompleat(s)){
									  return;
									}
									//add port
									for(u8 i=0;i<ALL_PORT_NUM;i++){
										if(ports[i] == 1){
											set_vlan_sett_port_state(i,GT_FALLBACK);//enable trunk
										}
									}
									sprintf(temp,"%s state enable",ports_print(ports));
									shell_output(s,"Command: config vlan vlan_trunking ports ",temp);
									shell_output(s,"Success.","");
									settings_add2queue(SQ_VLAN);
									return;
								}
								else{
									ptr6 = findcmd(s,ptr5,"disable");
									if(ptr6!=NULL){
										if(autocompleat(s)){
										  return;
										}
										//add port
										for(u8 i=0;i<ALL_PORT_NUM;i++){
											if(ports[i] == 1){
												set_vlan_sett_port_state(i,GT_SECURE);//disable trunk
											}
										}
										sprintf(temp,"%s state disable",ports_print(ports));
										shell_output(s,"Command: config vlan vlan_trunking ports ",temp);
										shell_output(s,"Success.","");
										settings_add2queue(SQ_VLAN);
										return;
									}
									else{
										if(autocompleat(s)){
										  return;
										}
										shell_output(s,"ERROR: Command","");
										shell_output(s,"Next possible completions:","");
										shell_output(s,"enable","");
										shell_output(s,"disable","");
										return;
									}
								}
							}
							else{
								if(autocompleat(s)){
								  return;
								}
								shell_output(s,"ERROR: Command","");
								shell_output(s,"Next possible completions:","");
								shell_output(s,"state","");
								return;
							}
						 }
						 else{
							 shell_output(s,"ERROR: Incorrect portlist","");
							 return;
						 }
					  }else{
						  if(autocompleat(s)){
							  return;
						  }
						  shell_output(s,"ERROR: Command","");
						  shell_output(s,"Next possible completions:","");
						  shell_output(s,"state","");
						  shell_output(s,"ports","");
						  return;
					  }
				  }

			  }
			  else{
				  if(autocompleat(s)){
					  return;
				  }
			  	  shell_output(s,"ERROR: Command","");
				  shell_output(s,"Next possible completions:","");
				  shell_output(s,"vlanid <vid>","");
				  shell_output(s,"mngt_vlan <vid>","");
				  shell_output(s,"vlan_trunking","");
				  return;
			  }
		  }
	  }
  }

  ptr1 = findcmd(s,str,"comfortstart");
  if(ptr1 != NULL){
	  ptr2 = findcmd(s,ptr1,"sstime");
	  if(ptr2 != NULL){
		  if(autocompleat(s)){
			return;
		  }
		  ptr3 = findcmd(s,ptr2," ");
		  temp_u32 = strtoul(ptr3,NULL,10);
		  if(set_softstart_time((u16)temp_u32) == 0){
			  shell_output(s,"Command: config comfortstart sstime ",ptr3);
			  shell_output(s,"Success.","");
			  settings_add2queue(SQ_SS);
			  return;
		  }
		  else{
			  shell_output(s,"ERROR: Comfort Start time ","Incorrect value");
			  return;
		  }
	  }
	  else{
		  ptr2 = findcmd(s,ptr1,"portlist");
		  if(ptr2 != NULL){
			  if(autocompleat(s)){
				return;
			  }
			  ptr3 = findcmd(s,ptr2," ");
			  ptr4 = find_port(ptr3,ports);
			  if(ptr4 != NULL){
				  for(u8 i=0;i<(COOPER_PORT_NUM);i++){
					  if((ports[i] == 0)||(ports[i] == 1))
						  set_port_soft_start(i,ports[i]);
				  }
				  shell_output(s,"Command: config comfortstart portlist ",ports_print(ports));
				  shell_output(s,"Success.","");
				  settings_add2queue(SQ_SS);
			  	  return;
			  }
			  else{
				  shell_output(s,"ERROR: Comfort Start: ","Incorrect portlist");
				  return;
			  }
		  }
		  else{
			  if(autocompleat(s)){
				  return;
			  }
			  shell_output(s,"ERROR: Command","");
			  shell_output(s,"Next possible completions:","");
			  shell_output(s,"sstime","");
			  shell_output(s,"portlist","");
			  return;
		  }
	  }
  }

  ptr1 = findcmd(s,str,"autorestart");
  if(ptr1 != NULL){

	  ptr2 = findcmd(s,ptr1,"ports");
	  if(ptr2 != NULL){
		  if(autocompleat(s)){
			return;
		  }
		  ptr4 = find_port(ptr2,ports);
		  if(ptr4 != NULL){
			  ptr5 = findcmd(s,ptr4,"state");
			  if(ptr5 != NULL){
				  if(autocompleat(s)){
					return;
				  }
				  ptr6 = findcmd(s,ptr5,"disable");
				  if(ptr6 != NULL){
					  if(autocompleat(s)){
						return;
					  }
					  sprintf(temp,"%s state disable",ports_print(ports));
					  shell_output(s,"Command: config autorestart ports ",temp);
					  shell_output(s,"Success.","");
					  for(u8 i=0;i<COOPER_PORT_NUM;i++){
						  if(ports[i] == 1){
							  set_port_wdt(i,WDT_NONE);
						  }
					  }
					  settings_add2queue(SQ_AR);
					  return;
				  }
				  else{
					  ptr6 = findcmd(s,ptr5,"link");
					  if(ptr6 != NULL){
						  if(autocompleat(s)){
							return;
						  }
						  sprintf(temp,"%s state link",ports_print(ports));
						  shell_output(s,"Command: config autorestart ports ",temp);
						  shell_output(s,"Success.","");
						  for(u8 i=0;i<COOPER_PORT_NUM;i++){
							  if(ports[i] == 1){
								  set_port_wdt(i,WDT_LINK);
							  }
						  }
						  settings_add2queue(SQ_AR);
						  return;
					  }
					  else{
						  ptr6 = findcmd(s,ptr5,"ping");
						  if(ptr6 != NULL){
							  if(autocompleat(s)){
								return;
							  }
							  sprintf(temp,"%s state ping ",ports_print(ports));
							  shell_output(s,"Command: config autorestart ports ",temp);
							  shell_output(s,"Success.","");
							  for(u8 i=0;i<COOPER_PORT_NUM;i++){
								  if(ports[i] == 1){
									  set_port_wdt(i,WDT_PING);
								  }
							  }
							  settings_add2queue(SQ_AR);
							  return;
						  }
						  else{
							  ptr6 = findcmd(s,str,"speed");
							  if(ptr6 != NULL){
								  if(autocompleat(s)){
									return;
								  }
								  sprintf(temp,"%s state speed",ports_print(ports));
								  shell_output(s,"Command: config autorestart ports ",temp);
								  shell_output(s,"Success.","");
								  for(u8 i=0;i<COOPER_PORT_NUM;i++){
									  if(ports[i] == 1){
										  set_port_wdt(i,WDT_SPEED);
									  }
								  }
								  settings_add2queue(SQ_AR);
								  return;

							  }
							  else{
								  if(autocompleat(s)){
									  return;
								  }
								  shell_output(s,"ERROR: Command","");
								  shell_output(s,"Next possible completions:","");
								  shell_output(s,"disable","");
								  shell_output(s,"link","");
								  shell_output(s,"ping","");
								  shell_output(s,"speed","");
								  return;
							  }
						  }
					  }
				  }
			  }
			  else{
				  	ptr5 = findcmd(s,ptr4,"host");
				  	if(ptr5 != NULL){
						if(autocompleat(s)){
							return;
						}
				  		if(find_ip(ptr5,&ip) == 1){
				  			for(u8 i=0;i<COOPER_PORT_NUM;i++){
				  				if(ports[i] == 1)
				  					set_port_wdt_ip(i,ip);
				  			}
							sprintf(temp,"%s host %d.%d.%d.%d",
									ports_print(ports),uip_ipaddr1(&ip),uip_ipaddr2(&ip),uip_ipaddr3(&ip),uip_ipaddr4(&ip));
							shell_output(s,"Command: config autorestart ports ",temp);
							shell_output(s,"Success.","");
							settings_add2queue(SQ_AR);
							return;
						}else{
							shell_output(s,"ERROR: incorrect IP address","");
							return;
						}
				  	}
				  	else{
				  		ptr5 = findcmd(s,ptr4,"min_speed");
				  		if(ptr5 != NULL){
				  		  if(autocompleat(s)){
				  			  return;
						  }
				  		  ptr6 = findcmd(s,ptr5," ");
				  		  temp_u32 = strtoul(ptr6,NULL,10);
				  		  for(u8 i=0;i<COOPER_PORT_NUM;i++){
				  			  if(ports[i] == 1){
				  				if(set_port_wdt_speed_down(i,(u16)temp_u32)==0){
									sprintf(temp,"%s min_speed %d",
											ports_print(ports),(u16)temp_u32);
									shell_output(s,"Command: config autorestart ports ",temp);
									shell_output(s,"Success.","");
									settings_add2queue(SQ_AR);
				  					return;
				  				}
				  				else{
				  					shell_output(s,"ERROR: incorrect speed (Must be 1-102400 Kbps)","");
				  					return;
				  				}
				  			  }
				  		  }
				  		}
				  		else
				  	  	{
					  		ptr5 = findcmd(s,ptr4,"max_speed");
					  		if(ptr5 != NULL){
					  		  if(autocompleat(s)){
					  			  return;
							  }
					  		  ptr6 = findcmd(s,ptr5," ");
					  		  temp_u32 = strtoul(ptr6,NULL,10);
					  		  for(u8 i=0;i<COOPER_PORT_NUM;i++){
					  			  if(ports[i] == 1){
					  				if(set_port_wdt_speed_up(i,(u16)temp_u32)==0){
										sprintf(temp,"%s max_speed %d",
												ports_print(ports),(u16)temp_u32);
										shell_output(s,"Command: config autorestart ports ",temp);
										shell_output(s,"Success.","");
										settings_add2queue(SQ_AR);
					  					return;
					  				}
					  				else{
					  					shell_output(s,"ERROR: incorrect speed (Must be 1-102400 Kbps)","");
					  					return;
					  				}
					  			  }
					  		  }
					  		}
					  		else{
								  if(autocompleat(s)){
									  return;
								  }
								  shell_output(s,"ERROR: Command","");
								  shell_output(s,"Next possible completions:","");
								  shell_output(s,"state","");
								  shell_output(s,"host","");
								  shell_output(s,"min_speed","");
								  return;
					  		}
				  	  	}
				  	}
			  }

		  }
		  else{
			  shell_output(s,"ERROR: Autorestart: ","Incorrect portlist");
			  return;
		  }
	  }
	  else{
		  if(autocompleat(s)){
			  return;
		  }
		  shell_output(s,"ERROR: Command","");
		  shell_output(s,"Next possible completions:","");
		  shell_output(s,"ports","");
		  return;
	  }
  }

  ptr1 = findcmd(s,str,"dry_cont");
  if(ptr1 != NULL){
	  ptr2 = findcmd(s,ptr1," ");
	  if(ptr2!=NULL){
		  temp_u32 = strtoul(ptr2,&ptr3,10);
			  if(temp_u32<NUM_ALARMS){
				  ptr4 = findcmd(s,ptr3,"state");
				  if(ptr4 != NULL){
					  ptr5 = findcmd(s,ptr4,"enable");
					  if(ptr5 != NULL){
						  if(autocompleat(s)){
							return;
						  }
						  sprintf(temp,"%d state enable",(u8)temp_u32);
						  shell_output(s,"Command: config dry_cont ",temp);
						  set_alarm_state(temp_u32,ENABLE);
						  shell_output(s,"Success.","");
						  settings_add2queue(SQ_DRYCONT);
						  return;
					  }
					  else{
						  ptr5 = findcmd(s,ptr4,"disable");
						  if(ptr5 != NULL){
							  if(autocompleat(s)){
								return;
							  }
							  sprintf(temp,"%d state disable",(u8)temp_u32);
							  shell_output(s,"Command: config dry_cont ",temp);
							  set_alarm_state(temp_u32,DISABLE);
							  shell_output(s,"Success.","");
							  settings_add2queue(SQ_DRYCONT);
							  return;
						  }
						  else{
							  if(autocompleat(s)){
								  return;
							  }
							  shell_output(s,"ERROR: Command","");
							  shell_output(s,"Next possible completions:","");
							  shell_output(s,"enable","");
							  shell_output(s,"disable","");
							  return;
						  }
					  }
				  }
				  else{
					  ptr4 = findcmd(s,ptr3,"alarm_level");
					  if(ptr4 != NULL){
						  ptr5 = findcmd(s,ptr4,"open");
						  if(ptr5 != NULL){
							  if(autocompleat(s)){
								  return;
							  }
							  sprintf(temp,"%d alarm_level open",(u8)temp_u32);
							  shell_output(s,"Command: config dry_cont ",temp);
							  set_alarm_front(temp_u32,2);
							  shell_output(s,"Success.","");
							  settings_add2queue(SQ_DRYCONT);
							  return;

						  }
						  else{
							  ptr5 = findcmd(s,ptr4,"connected");
							  if(ptr5 != NULL){
								  if(autocompleat(s)){
									  return;
								  }
								  sprintf(temp,"%d alarm_level connected",(u8)temp_u32);
								  shell_output(s,"Command: config dry_cont ",temp);
								  set_alarm_front(temp_u32,1);
								  shell_output(s,"Success.","");
								  settings_add2queue(SQ_DRYCONT);
								  return;
							  }
							  else{
								  if(autocompleat(s)){
									  return;
								  }
								  shell_output(s,"ERROR: Command","");
								  shell_output(s,"Next possible completions:","");
								  shell_output(s,"connected","");
								  shell_output(s,"open","");
								  return;
							  }
						  }
					  }
					  else{
						  if(autocompleat(s)){
							  return;
						  }
						  shell_output(s,"ERROR: Command","");
						  shell_output(s,"Next possible completions:","");
						  shell_output(s,"state","");
						  shell_output(s,"alarm_level","");
						  return;
					  }
				  }
			  }
			  else{
				  if(autocompleat(s)){
					  return;
				  }
				  shell_output(s,"ERROR: Command","");
				  shell_output(s,"Next possible completions:","");
				  shell_output(s,"Dry contact number: <0-2>","");
				  return;
			  }
  	  }
	  else{
		  if(autocompleat(s)){
			  return;
		  }
		  shell_output(s,"ERROR: Command","");
		  shell_output(s,"Next possible completions:","");
		  shell_output(s,"Dry contact number: <0-2>","");
		  return;
	  }
  }


  ptr1 = findcmd(s,str,"user_accounts");
  if(ptr1 != NULL){
	  ptr2 = findcmd(s,ptr1,"add");
	  if(ptr2 != NULL){
		  if(autocompleat(s)){
			  return;
		  }

		  ptr3 = findcmd(s,ptr2," ");
		  if(findword(username,ptr3,20)==0){
			  shell_output(s,"ERROR: Incorrect Username:","");
			  return;
		  }

		  ptr4 = findcmd(s,ptr3," ");
		  if(findword(password,ptr4,20)==0){
			  shell_output(s,"ERROR: Incorrect Password:","");
			  return;
		  }

		  ptr5 = findcmd(s,ptr4," ");

		  ptr6 = findcmd(s,ptr5,"admin_rule");
		  if(ptr6!=NULL){
			  for(u8 i=0;i<MAX_USERS_NUM;i++){
				  //если пользователь уже существует
				  get_interface_users_username(i,temp);
				  if(strcmp(username,temp) == 0){
					  set_interface_users_password(i,password);
					  set_interface_users_rule(i,ADMIN_RULE);
					  sprintf(temp,"%s %s admin_rule",username,password);
					  shell_output(s,"Command: config user_accounts add ",temp);
					  shell_output(s,"Success.","");
					  settings_add2queue(SQ_USERS);
					  return;
				  }
			  }
			  //если пользователя не существует
			  //ищем место под запись
			  for(u8 i=0;i<MAX_USERS_NUM;i++){
				  get_interface_users_username(i,temp);
				  if(strlen(temp)==0){
					  set_interface_users_username(i,username);
					  set_interface_users_password(i,password);
					  set_interface_users_rule(i,ADMIN_RULE);
					  sprintf(temp,"%s %s admin_rule",username,password);
					  shell_output(s,"Command: config user_accounts add ",temp);
					  shell_output(s,"Success.","");
					  settings_add2queue(SQ_USERS);
					  return;
				  }
			  }
		  }
		  else{
			  ptr6 = findcmd(s,ptr5,"user_rule");
			  if(ptr6!=NULL){
				  for(u8 i=0;i<MAX_USERS_NUM;i++){
					  //если пользователь уже существует
					  get_interface_users_username(i,temp);
					  if(strcmp(username,temp) == 0){
						  set_interface_users_password(i,password);
						  set_interface_users_rule(i,USER_RULE);
						  sprintf(temp,"%s %s user_rule",username,password);
						  shell_output(s,"Command: config user_accounts add ",temp);
						  shell_output(s,"Success.","");
						  settings_add2queue(SQ_USERS);
						  return;
					  }
				  }
				  //если пользователя не существует
				  //ищем место под запись
				  for(u8 i=0;i<MAX_USERS_NUM;i++){
					  get_interface_users_username(i,temp);
					  if(strlen(temp)==0){
						  set_interface_users_username(i,username);
						  set_interface_users_password(i,password);
						  set_interface_users_rule(i,USER_RULE);
						  sprintf(temp,"%s %s user_rule",username,password);
						  shell_output(s,"Command: config user_accounts add ",temp);
						  shell_output(s,"Success.","");
						  settings_add2queue(SQ_USERS);
						  return;
					  }
				  }
			  }
			  else{
				  shell_output(s,"ERROR: Command","");
				  shell_output(s,"Next possible completions:","");
				  shell_output(s,"admin_rule","");
				  shell_output(s,"user_rule","");
				  return;
			  }
		  }
	  }
	  else{
		  ptr2 = findcmd(s,ptr1,"delete");
		  if(ptr2 != NULL){
			  if(autocompleat(s)){
				  return;
			  }
			  ptr3 = findcmd(s,ptr2," ");
			  if(findword(username,ptr3,20)==0){
				  shell_output(s,"ERROR: Incorrect Username:","");
				  return;
			  }
			  for(u8 i=0;i<MAX_USERS_NUM;i++){
				  //если пользователь существует
				  get_interface_users_username(i,temp);
				  if(strcmp(username,temp) == 0){
					  set_interface_users_username(i,"");
					  set_interface_users_password(i,"");
					  set_interface_users_rule(i,NO_RULE);
					  shell_output(s,"Command: config user_accounts delete ",username);
					  shell_output(s,"Success.","");
					  settings_add2queue(SQ_USERS);
					  return;
				  }
			  }
			  shell_output(s,"ERROR: Username is not found","");
			  return;
		  }
		  else{
			  if(autocompleat(s)){
				  return;
			  }
			  shell_output(s,"ERROR: Command","");
			  shell_output(s,"Next possible completions:","");
			  shell_output(s,"add","");
			  shell_output(s,"delete","");
			  return;
		  }
	  }
  }

  ptr1 = findcmd(s,str,"tftp");
   if(ptr1 != NULL){
 	  ptr2 = findcmd(s,ptr1," ");
 	  if(ptr2 != NULL){
		  ptr3 = findcmd(s,ptr2,"state");
		  if(ptr3 != NULL){
			  ptr4 = findcmd(s,ptr3,"enable");
			  if(ptr4 != NULL){
				  if(autocompleat(s)){
					  return;
				  }
				  shell_output(s,"Command: config tftp state enable","");
				  shell_output(s,"Success.","");
				  set_tftp_state(ENABLE);
				  settings_add2queue(SQ_TFTP);
				  return;
			  }
			  else{
				  ptr4 = findcmd(s,ptr3,"disable");
				  if(ptr4 != NULL){
					  if(autocompleat(s)){
						  return;
					  }
					  shell_output(s,"Command: config tftp state disable","");
					  shell_output(s,"Success.","");
					  set_tftp_state(DISABLE);
					  settings_add2queue(SQ_TFTP);
					  return;
				  }
				  else{
					  if(autocompleat(s)){
						  return;
					  }
					  shell_output(s,"ERROR: Command","");
					  shell_output(s,"Next possible completions:","");
					  shell_output(s,"enable","");
					  shell_output(s,"disable","");
					  return;
				  }
			  }
		  }
		  else{
			  ptr3 = findcmd(s,ptr2,"port");
			  if(ptr3!= NULL){
				  if(autocompleat(s)){
					  return;
				  }
				  ptr4 = findcmd(s,ptr3," ");
				  ptr4++;
				  temp_u32 = strtoul(ptr4,NULL,10);
				  if(set_tftp_port((u16)temp_u32)==0){
					  shell_output(s,"Command: config tftp port ",ptr4);
					  shell_output(s,"Success.","");
					  settings_add2queue(SQ_TFTP);
					  return;
				  }
				  else{
					  shell_output(s,"ERROR: Command","");
					  shell_output(s,"Next possible completions:","");
					  shell_output(s,"TFTP port <num>","");
					  return;
				  }
			  }
			  else{
				  if(autocompleat(s)){
					  return;
				  }
				  shell_output(s,"ERROR: Command","");
				  shell_output(s,"Next possible completions:","");
				  shell_output(s,"state","");
				  shell_output(s,"port","");
				  return;
			  }
		  }
 	  }
	  else{
		  if(autocompleat(s)){
			  return;
		  }
		  shell_output(s,"ERROR: Command","");
		  shell_output(s,"Next possible completions:","");
		  shell_output(s,"state","");
		  shell_output(s,"port","");
		  return;
	  }
   }

   ptr1 = findcmd(s,str,"events");
   if(ptr1 != NULL){
	   //port link
	   ptr2 = findcmd(s,ptr1,"port_link");
	   if(ptr2!=NULL){
		   ptr3 = findcmd(s,ptr2,"state");
		   if(ptr3!=NULL){
			   if(autocompleat(s)){
				   return;
			   }
			   ptr4 = findcmd(s,ptr3,"enable");
			   if(ptr4!=NULL){
				   ptr5 = findcmd(s,ptr4,"level");
				   if(ptr5!=NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   ptr6 = findcmd(s,ptr5," ");
					   temp_u32 = strtoul(ptr6,NULL,10);
					   if(set_event_port_link_t(ENABLE,(u8)temp_u32)==0){
						   shell_output(s,"Command: config events port_link state enable level ",ptr6);
						   shell_output(s,"Success.","");
						   settings_add2queue(SQ_EVENTS);
						   return;
					   }
					   else{
						   shell_output(s,"ERROR: Event level <0-7>","");
						   return;
					   }
				   }
				   else{
					   if(autocompleat(s)){
						  return;
					   }
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"level","");
					   return;
				   }
			   }
			   else{
				   ptr4 = findcmd(s,ptr3,"disable");
				   if(ptr4!=NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   shell_output(s,"Command: config events port_link state disable","");
					   set_event_port_link_t(DISABLE,0);
					   shell_output(s,"Success.","");
					   settings_add2queue(SQ_EVENTS);
					   return;
				   }
				   else{
					   if(autocompleat(s)){
						  return;
					   }
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"enable","");
					   shell_output(s,"disable","");
					   return;
				   }
			   }
		   }
		   else{
			   if(autocompleat(s)){
				  return;
			   }
			   shell_output(s,"ERROR: Command","");
			   shell_output(s,"Next possible completions:","");
			   shell_output(s,"state","");
			   return;
		   }
	   }
	   //port poe
	   ptr2 = findcmd(s,ptr1,"port_poe");
	   if(ptr2!=NULL){
		   ptr3 = findcmd(s,ptr2,"state");
		   if(ptr3!=NULL){
			   ptr4 = findcmd(s,ptr3,"enable");
			   if(ptr4!=NULL){
				   ptr5 = findcmd(s,ptr4,"level");
				   if(ptr5!=NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   ptr6 = findcmd(s,ptr5," ");
					   temp_u32 = strtoul(ptr6,NULL,10);
					   if(set_event_port_poe_t(ENABLE,(u8)temp_u32)==0){
						   shell_output(s,"Command: config events port_poe state enable level ",ptr6);
						   shell_output(s,"Success.","");
						   settings_add2queue(SQ_EVENTS);
						   return;
					   }
					   else{
						   shell_output(s,"ERROR: Event level <0-7>","");
						   return;
					   }
				   }
				   else{
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"level","");
					   return;
				   }
			   }
			   else{
				   ptr4 = findcmd(s,ptr3,"disable");
				   if(ptr4!=NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   shell_output(s,"Command: config events port_poe state disable","");
					   set_event_port_poe_t(DISABLE,0);
					   shell_output(s,"Success.","");
					   settings_add2queue(SQ_EVENTS);
					   return;
				   }
				   else{
				       if(autocompleat(s)){
						  return;
					   }
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"enable","");
					   shell_output(s,"disable","");
					   return;
				   }
			   }
		   }
		   else{
			   if(autocompleat(s)){
				  return;
			   }
			   shell_output(s,"ERROR: Command","");
			   shell_output(s,"Next possible completions:","");
			   shell_output(s,"state","");
			   return;
		   }
	   }//
	   //rstp
	   ptr2 = findcmd(s,ptr1,"stp");
	   if(ptr2!=NULL){
		   ptr3 = findcmd(s,ptr2,"state");
		   if(ptr3!=NULL){
			   ptr4 = findcmd(s,ptr3,"enable");
			   if(ptr4!=NULL){
				   if(autocompleat(s)){
					   return;
				   }
				   ptr5 = findcmd(s,ptr4,"level");
				   if(ptr5!=NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   ptr6 = findcmd(s,ptr5," ");
					   temp_u32 = strtoul(ptr6,NULL,10);
					   if(set_event_stp_t(ENABLE,(u8)temp_u32)==0){
						   shell_output(s,"Command: config events stp state enable level ",ptr6);
						   shell_output(s,"Success.","");
						   settings_add2queue(SQ_EVENTS);
						   return;
					   }
					   else{
						   shell_output(s,"ERROR: Event level <0-7>","");
						   return;
					   }
				   }
				   else{
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"level","");
					   return;
				   }
			   }
			   else{
				   ptr4 = findcmd(s,ptr3,"disable");
				   if(ptr4!=NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   shell_output(s,"Command: config events stp state disable","");
					   set_event_stp_t(DISABLE,0);
					   shell_output(s,"Success.","");
					   settings_add2queue(SQ_EVENTS);
					   return;
				   }
				   else{
					   if(autocompleat(s)){
						  return;
					   }
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"enable","");
					   shell_output(s,"disable","");
					   return;
				   }
			   }
		   }
		   else{
			   if(autocompleat(s)){
				  return;
			   }
			   shell_output(s,"ERROR: Command","");
			   shell_output(s,"Next possible completions:","");
			   shell_output(s,"state","");
			   return;
		   }
	   }//

	   //autorestart link
	   ptr2 = findcmd(s,ptr1,"ar_link");
	   if(ptr2!=NULL){
		   ptr3 = findcmd(s,ptr2,"state");
		   if(ptr3!=NULL){
			   ptr4 = findcmd(s,ptr3,"enable");
			   if(ptr4!=NULL){
				   ptr5 = findcmd(s,ptr4,"level");
				   if(ptr5!=NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   ptr6 = findcmd(s,ptr5," ");
					   temp_u32 = strtoul(ptr6,NULL,10);
					   if(set_event_spec_link_t(ENABLE,(u8)temp_u32)==0){
						   shell_output(s,"Command: config events ar_link state enable level ",ptr6);
						   shell_output(s,"Success.","");
						   settings_add2queue(SQ_EVENTS);
						   return;
					   }
					   else{
						   shell_output(s,"ERROR: Event level <0-7>","");
						   return;
					   }
				   }
				   else{
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"level","");
					   return;
				   }
			   }
			   else{
				   ptr4 = findcmd(s,ptr3,"disable");
				   if(ptr4!=NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   shell_output(s,"Command: config events ar_link state disable","");
					   set_event_spec_link_t(DISABLE,0);
					   shell_output(s,"Success.","");
					   settings_add2queue(SQ_EVENTS);
					   return;
				   }
				   else{
					   if(autocompleat(s)){
						  return;
					   }
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"enable","");
					   shell_output(s,"disable","");
					   return;
				   }
			   }
		   }
		   else{
			   if(autocompleat(s)){
				  return;
			   }
			   shell_output(s,"ERROR: Command","");
			   shell_output(s,"Next possible completions:","");
			   shell_output(s,"state","");
			   return;
		   }
	   }//
	   //autorestart ping
	   ptr2 = findcmd(s,ptr1,"ar_ping");
	   if(ptr2!=NULL){
		   ptr3 = findcmd(s,ptr2,"state");
		   if(ptr3!=NULL){
			   ptr4 = findcmd(s,ptr3,"enable");
			   if(ptr4!=NULL){
				   ptr5 = findcmd(s,ptr4,"level");
				   if(ptr5!=NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   ptr6 = findcmd(s,ptr5," ");
					   temp_u32 = strtoul(ptr6,NULL,10);
					   if(set_event_spec_ping_t(ENABLE,(u8)temp_u32)==0){
						   shell_output(s,"Command: config events ar_ping state enable level ",ptr6);
						   shell_output(s,"Success.","");
						   settings_add2queue(SQ_EVENTS);
						   return;
					   }
					   else{
						   shell_output(s,"ERROR: Event level <0-7>","");
						   return;
					   }
				   }
				   else{
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"level","");
					   return;
				   }
			   }
			   else{
				   ptr4 = findcmd(s,ptr3,"disable");
				   if(ptr4!=NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   shell_output(s,"Command: config events ar_ping state disable","");
					   set_event_spec_ping_t(DISABLE,0);
					   shell_output(s,"Success.","");
					   settings_add2queue(SQ_EVENTS);
					   return;
				   }
				   else{
					   if(autocompleat(s)){
						  return;
					   }
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"enable","");
					   shell_output(s,"disable","");
					   return;
				   }
			   }
		   }
		   else{
			   if(autocompleat(s)){
				  return;
			   }
			   shell_output(s,"ERROR: Command","");
			   shell_output(s,"Next possible completions:","");
			   shell_output(s,"state","");
			   return;
		   }
	   }//

	   //ar speed
	   ptr2 = findcmd(s,ptr1,"ar_speed");
	   if(ptr2!=NULL){
		   ptr3 = findcmd(s,ptr2,"state");
		   if(ptr3!=NULL){
			   ptr4 = findcmd(s,ptr3,"enable");
			   if(ptr4!=NULL){
				   ptr5 = findcmd(s,ptr4,"level");
				   if(ptr5!=NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   ptr6 = findcmd(s,ptr5," ");
					   temp_u32 = strtoul(ptr6,NULL,10);
					   if(set_event_spec_speed_t(ENABLE,(u8)temp_u32)==0){
						   shell_output(s,"Command: config events ar_speed state enable level ",ptr6);
						   shell_output(s,"Success.","");
						   settings_add2queue(SQ_EVENTS);
						   return;
					   }
					   else{
						   shell_output(s,"ERROR: Event level <0-7>","");
						   return;
					   }
				   }
				   else{
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"level","");
					   return;
				   }
			   }
			   else{
				   ptr4 = findcmd(s,ptr3,"disable");
				   if(ptr4!=NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   shell_output(s,"Command: config events ar_speed state disable","");
					   set_event_spec_speed_t(DISABLE,0);
					   shell_output(s,"Success.","");
					   settings_add2queue(SQ_EVENTS);
					   return;
				   }
				   else{
					   if(autocompleat(s)){
						  return;
					   }
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"enable","");
					   shell_output(s,"disable","");
					   return;
				   }
			   }
		   }
		   else{
			   if(autocompleat(s)){
				  return;
			   }
			   shell_output(s,"ERROR: Command","");
			   shell_output(s,"Next possible completions:","");
			   shell_output(s,"state","");
			   return;
		   }
	   }//
	   //system
	   ptr2 = findcmd(s,ptr1,"system");
	   if(ptr2!=NULL){
		   ptr3 = findcmd(s,ptr2,"state");
		   if(ptr3!=NULL){
			   ptr4 = findcmd(s,ptr3,"enable");
			   if(ptr4!=NULL){
				   ptr5 = findcmd(s,ptr4,"level");
				   if(ptr5!=NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   ptr6 = findcmd(s,ptr5," ");
					   temp_u32 = strtoul(ptr6,NULL,10);
					   if(set_event_system_t(ENABLE,(u8)temp_u32)==0){
						   shell_output(s,"Command: config events system state enable level ",ptr6);
						   shell_output(s,"Success.","");
						   settings_add2queue(SQ_EVENTS);
						   return;
					   }
					   else{
						   shell_output(s,"ERROR: Event level <0-7>","");
						   return;
					   }
				   }
				   else{
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"level","");
					   return;
				   }
			   }
			   else{
				   ptr4 = findcmd(s,ptr3,"disable");
				   if(ptr4!=NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   shell_output(s,"Command: config events system state disable","");
					   set_event_system_t(DISABLE,0);
					   shell_output(s,"Success.","");
					   settings_add2queue(SQ_EVENTS);
					   return;
				   }
				   else{
					   if(autocompleat(s)){
						  return;
					   }
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"enable","");
					   shell_output(s,"disable","");
					   return;
				   }
			   }
		   }
		   else{
			   if(autocompleat(s)){
				  return;
			   }
			   shell_output(s,"ERROR: Command","");
			   shell_output(s,"Next possible completions:","");
			   shell_output(s,"state","");
			   return;
		   }
	   }//
	   //ups
	   ptr2 = findcmd(s,ptr1,"ups");
	   if(ptr2!=NULL){
		   ptr3 = findcmd(s,ptr2,"state");
		   if(ptr3!=NULL){
			   ptr4 = findcmd(s,ptr3,"enable");
			   if(ptr4!=NULL){
				   ptr5 = findcmd(s,ptr4,"level");
				   if(ptr5!=NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   ptr6 = findcmd(s,ptr5," ");
					   temp_u32 = strtoul(ptr6,NULL,10);
					   if(set_event_ups_t(ENABLE,(u8)temp_u32)==0){
						   shell_output(s,"Command: config events ups state enable level ",ptr6);
						   shell_output(s,"Success.","");
						   settings_add2queue(SQ_EVENTS);
						   return;
					   }
					   else{
						   shell_output(s,"ERROR: Event level <0-7>","");
						   return;
					   }
				   }
				   else{
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"level","");
					   return;
				   }
			   }
			   else{
				   ptr4 = findcmd(s,ptr3,"disable");
				   if(ptr4!=NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   shell_output(s,"ERROR: Command","");
					   set_event_ups_t(DISABLE,0);
					   shell_output(s,"Success.","");
					   settings_add2queue(SQ_EVENTS);
					   return;
				   }
				   else{
					   if(autocompleat(s)){
						  return;
					   }
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"enable","");
					   shell_output(s,"disable","");
					   return;
				   }
			   }
		   }
		   else{
			   if(autocompleat(s)){
				  return;
			   }
			   shell_output(s,"ERROR: Command","");
			   shell_output(s,"Next possible completions:","");
			   shell_output(s,"state","");
			   return;
		   }
	   }//
	   //dry_cont
	   ptr2 = findcmd(s,ptr1,"dry_cont");
	   if(ptr2!=NULL){
		   ptr3 = findcmd(s,ptr2,"state");
		   if(ptr3!=NULL){
			   ptr4 = findcmd(s,ptr3,"enable");
			   if(ptr4!=NULL){
				   ptr5 = findcmd(s,ptr4,"level");
				   if(ptr5!=NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   ptr6 = findcmd(s,ptr5," ");
					   temp_u32 = strtoul(ptr6,NULL,10);
					   if(set_event_alarm_t(ENABLE,(u8)temp_u32)==0){
						   shell_output(s,"Command: config events dry_cont state enable level ",ptr6);
						   shell_output(s,"Success.","");
						   settings_add2queue(SQ_EVENTS);
						   return;
					   }
					   else{
						   shell_output(s,"ERROR: Event level <0-7>","");
						   return;
					   }
				   }
				   else{
					   if(autocompleat(s)){
						  return;
					   }
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"level","");
					   return;
				   }
			   }
			   else{
				   ptr4 = findcmd(s,ptr3,"disable");
				   if(ptr4!=NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   shell_output(s,"Command: config events dry_cont state disable","");
					   set_event_alarm_t(DISABLE,0);
					   shell_output(s,"Success.","");
					   settings_add2queue(SQ_EVENTS);
					   return;
				   }
				   else{
					   if(autocompleat(s)){
						  return;
					   }
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"enable","");
					   shell_output(s,"disable","");
					   return;
				   }
			   }
		   }
		   else{
			   if(autocompleat(s)){
				  return;
			   }
			   shell_output(s,"ERROR: Command","");
			   shell_output(s,"Next possible completions:","");
			   shell_output(s,"state","");
			   return;
		   }
	   }//

	   if(autocompleat(s)){
		  return;
	   }
	   //если не попали
	   shell_output(s,"ERROR: Command","");
	   shell_output(s,"Next possible completions:","");
	   shell_output(s,"port_link","");
	   shell_output(s,"port_poe","");
	   shell_output(s,"stp","");
	   shell_output(s,"ar_link","");
	   shell_output(s,"ar_ping","");
	   shell_output(s,"ar_speed","");
	   shell_output(s,"system","");
	   shell_output(s,"ups","");
	   shell_output(s,"dry_cont","");
	   return;

   }


   //QoS group
   ptr1 = findcmd(s,str,"802.1p");
   if(ptr1 != NULL){
	   ptr2 = findcmd(s,ptr1,"state");
	   if(ptr2 != NULL){
		   ptr3 = findcmd(s,ptr2,"enable");
		   if(ptr3!=NULL){
			   if(autocompleat(s)){
				   return;
			   }
			   shell_output(s,"Command: config 802.1p state enable","");
			   set_qos_state(ENABLE);
			   shell_output(s,"Success.","");
			   settings_add2queue(SQ_QOS);
			   return;
		   }
		   else{
			   ptr3 = findcmd(s,ptr2,"disable");
			   if(ptr3!=NULL){
				   if(autocompleat(s)){
					   return;
				   }
				   shell_output(s,"Command: config 802.1p state disable","");
				   set_qos_state(DISABLE);
				   shell_output(s,"Success.","");
				   settings_add2queue(SQ_QOS);
				   return;
			   }
			   else{
				   if(autocompleat(s)){
					  return;
				   }
				   shell_output(s,"ERROR: Command","");
				   shell_output(s,"Next possible completions:","");
				   shell_output(s,"enable","");
				   shell_output(s,"disable","");
				   return;
			   }
		   }
	   }
	   else{
		   ptr2 = findcmd(s,ptr1,"default_priority");
		   if(ptr2!=NULL){
			   ptr3 = findcmd(s,ptr2,"ports");
			   if(ptr3 != NULL){
				  if(autocompleat(s)){
					   return;
				  }
			 	  ptr4 = findcmd(s,ptr3," ");
			 	  if(ptr4 == NULL){
			 		  shell_output(s,"ERROR: Incorrect port list","");
			 		  return;
			 	  }
			 	  else{
			 		  ptr5 = find_port(ptr4,ports);
			 		  if(ptr5 == NULL){
			 			  shell_output(s,"ERROR: Incorrect port list","");
			 			  return;
			 		  }
			 		  else{
			 			 ptr6 = findcmd(s,ptr5," ");
			 			 if(ptr6 != NULL){
			 				temp_u32 = strtoul(ptr6,NULL,10);
			 				if(temp_u32<8){
								for(u8 i=0;i<ALL_PORT_NUM;i++){
									if(ports[i] == 1)
										set_qos_port_def_pri(i,(u8)temp_u32);
								}
								sprintf(temp,"%s %d",ports_print(ports),(u8)temp_u32);
								shell_output(s,"Command: config 802.1p default_priority ports ",temp);
								shell_output(s,"Success.","");
					 			settings_add2queue(SQ_QOS);
					 			return;
			 				}
			 				else{
								shell_output(s,"ERROR: Command","");
							    shell_output(s,"Next possible completions:","");
							    shell_output(s,"Priority <0-7>","");
							    return;
			 				}
			 			 }
			 			 else{
			 				shell_output(s,"ERROR: Command","");
						    shell_output(s,"Next possible completions:","");
						    shell_output(s,"<priority 0-7>","");
						    return;
			 			 }
			 		  }
			 	  }
			   }
			   else{
				   if(autocompleat(s)){
					  return;
				   }
				   shell_output(s,"ERROR: Command","");
				   shell_output(s,"Next possible completions:","");
				   shell_output(s,"ports","");
				   return;
			   }
		   }
		   else{
			   ptr2 = findcmd(s,ptr1,"user_priority");
			   if(ptr2!=NULL){
				   if(autocompleat(s)){
					   return;
				   }
				   ptr3 = findcmd(s,ptr2," ");
				   if(ptr3 != NULL){
					   u8 priority = (u8)strtoul(ptr3,NULL,10);
					   if(priority<8){
						   ptr4 = findcmd(s,ptr3," ");
						   if(ptr4!= NULL){
							   u8 queue = (u8)strtoul(ptr4,NULL,10);
							   if(queue<4){
								   sprintf(temp,"%d %d",priority,queue);
								   shell_output(s,"Command: config 802.1p user_priority ",temp);
								   set_qos_cos(priority,queue);
								   shell_output(s,"Success.","");
						 		   settings_add2queue(SQ_QOS);
						 		   return;
							   }
							   else{
								   shell_output(s,"ERROR: Command","");
								   shell_output(s,"Next possible completions:","");
								   shell_output(s,"<queue 0-3>","");
								   return;
							   }
						   }else{
							   shell_output(s,"ERROR: Command","");
							   shell_output(s,"Next possible completions:","");
							   shell_output(s,"<queue 0-3>","");
							   return;
						   }
					   }
					   else{
						   shell_output(s,"ERROR: Command","");
						   shell_output(s,"Next possible completions:","");
						   shell_output(s,"<priority 0-7>","");
						   return;
					   }
				   }
				   else{
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"<priority 0-7>","");
					   return;
				   }
			   }
			   else{
				   if(autocompleat(s)){
					  return;
				   }
				   shell_output(s,"ERROR: Command","");
				   shell_output(s,"Next possible completions:","");
				   shell_output(s,"state","");
				   shell_output(s,"default_priority","");
				   shell_output(s,"user_priority","");
				   return;
			   }
		   }
	   }
   }

   //rate limit
   ptr1 = findcmd(s,str,"bandwidth_control");
   if(ptr1 != NULL){
	  ptr2 = findcmd(s,ptr1," ");
	  if(ptr2 == NULL){
		  shell_output(s,"ERROR: Incorrect port list","");
		  return;
	  }
	  else{
		  ptr3 = find_port(ptr2,ports);
		  if(ptr3 == NULL){
			  shell_output(s,"ERROR: Incorrect port list","");
			  return;
		  }else{
			  ptr4 = findcmd(s,ptr3,"rx_rate");
			  if(ptr4!=NULL){
				  if(autocompleat(s)){
					   return;
				  }
				  ptr5 = findcmd(s,ptr4," ");
				  if(ptr5!=NULL){
					  temp_u32 = strtoul(ptr5,NULL,10);

					  sprintf(temp,"%s rx_rate %lu",ports_print(ports),temp_u32);
					  shell_output(s,"Command: config bandwidth_control ",temp);

					  for(u8 i=0;i<ALL_PORT_NUM;i++){
						  if(ports[i] == 1){
							  if(set_rate_limit_rx(i,temp_u32)==0){

							  }
							  else{
								  shell_output(s,"ERROR: Incorrect rate value","");
								  return;
							  }
						  }
					  }
					  shell_output(s,"Success.","");
					  settings_add2queue(SQ_QOS);
					  return;
				  }else{
					  shell_output(s,"ERROR: Incorrect rate value","");
					  return;
				  }
			  }
			  else{
				  ptr4 = findcmd(s,ptr3,"tx_rate");
				  if(ptr4 != NULL ){
					  if(autocompleat(s)){
						   return;
					  }
					  ptr5 = findcmd(s,ptr4," ");
					  if(ptr5!=NULL){
					  	  temp_u32 = strtoul(ptr5,NULL,10);

						  sprintf(temp,"%s tx_rate %lu",ports_print(ports),temp_u32);
						  shell_output(s,"Command: config bandwidth_control ",temp);

					  	  for(u8 i=0;i<ALL_PORT_NUM;i++){
					  		  if(ports[i] == 1){
					  			 if(set_rate_limit_tx(i,temp_u32)==0){
								    //sprintf(temp,"TX Rate limit config: port %d, rate %lu",i+1,temp_u32);
								    //shell_output(s,temp,"");
					  			 }
					  			 else{
								    shell_output(s,"ERROR: Incorrect rate value","");
								    return;
					  			 }
					  		  }
					  	  }
					  	  shell_output(s,"Success.","");
						  settings_add2queue(SQ_QOS);
						  return;
					  }
					  else{
						  shell_output(s,"ERROR: Incorrect rate value","");
						  return;
					  }
				  }
				  else{
					   if(autocompleat(s)){
						  return;
					   }
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"rx_rate","");
					   shell_output(s,"tx_rate","");
					   return;
				  }
			  }
		  }
	  }

   }

   //sheduling mechanism
   ptr1 = findcmd(s,str,"scheduling_mechanism");
   if(ptr1!= NULL){
	   ptr2 = findcmd(s,ptr1,"strict");
	   if(ptr2!= NULL){
		   if(autocompleat(s)){
			   return;
		   }
		   shell_output(s,"Command: config scheduling_mechanism strict","");
		   set_qos_policy(FIXED_PRI);
		   shell_output(s,"Success.","");
		   settings_add2queue(SQ_QOS);
		   return;
	   }
	   else{
		   ptr2 = findcmd(s,ptr1,"weight_fair");
		   if(ptr2!= NULL){
			   if(autocompleat(s)){
				   return;
			   }
			   shell_output(s,"Command: config scheduling_mechanism weight_fair","");
			   set_qos_policy(WEIGHTED_FAIR_PRI);
			   shell_output(s,"Success.","");
			   settings_add2queue(SQ_QOS);
			   return;
		   }
		   else{
			   if(autocompleat(s)){
				  return;
			   }
			   shell_output(s,"ERROR: Command","");
			   shell_output(s,"Next possible completions:","");
			   shell_output(s,"strict","");
			   shell_output(s,"weight_fair","");
			   return;
		   }
	   }
   }

   ptr1 = findcmd(s,str,"dscp_mapping");
   if(ptr1 != NULL){
	   ptr2 = findcmd(s,ptr1,"dscp_value");
	   if(ptr2!=NULL){
		   if(autocompleat(s)){
			   return;
		   }
		   ptr3 = findcmd(s,ptr2," ");
		   if(ptr3!=NULL){
			   u8 dscp_val = (u8)strtoul(ptr3,&ptr4,10);
			   if(dscp_val<64){
				   ptr5 = findcmd(s,ptr4,"queue");
				   if(ptr5!=NULL){
					   ptr6 = findcmd(s,ptr5," ");
					   if(ptr6!=NULL){
						   u8 queue_dscp = (u8)strtoul(ptr6,NULL,10);
						   if(queue_dscp<4){
							   sprintf(temp,"%d queue %d",dscp_val,queue_dscp);
							   shell_output(s,"Command: config dscp_mapping dscp_value ",temp);
							   set_qos_tos(dscp_val,queue_dscp);
							   shell_output(s,"Success.","");
							   settings_add2queue(SQ_QOS);
							   return;
						   }
						   else{
							   shell_output(s,"ERROR: Command","");
							   shell_output(s,"Next possible completions:","");
							   shell_output(s,"<queue 0-3>","");
							   return;
						   }
					   }else{
						   shell_output(s,"ERROR: Command","");
						   shell_output(s,"Next possible completions:","");
						   shell_output(s,"<queue 0-3>","");
						   return;
					   }

				   }
				   else{
					   shell_output(s,"ERROR: Command","");
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"queue","");
					   return;
				   }
			   }
			   else{
				   shell_output(s,"ERROR: Command","");
				   shell_output(s,"Next possible completions:","");
				   shell_output(s,"<dscp_value 0-63>","");
				   return;
			   }
		   }
		   else{
			   shell_output(s,"ERROR: Command","");
			   shell_output(s,"Next possible completions:","");
			   shell_output(s,"<dscp_value 0-63>","");
			   return;
		   }
	   }
	   else{
		   if(autocompleat(s)){
			  return;
		   }
		   shell_output(s,"ERROR: Command","");
		   shell_output(s,"Next possible completions:","");
		   shell_output(s,"dscp_value","");
		   return;
	   }
   }



   //port settings
   ptr1 = findcmd(s,str,"ports");
   if(ptr1 != NULL){
 	  ptr2= findcmd(s,ptr1," ");
 	  if(ptr2 == NULL){
		  if(autocompleat(s)){
			   return;
		  }
 		  shell_output(s,"ERROR: Incorrect port list","");
 		  return;
 	  }
 	  else{
 		  ptr3 = find_port(ptr2,ports);
 		  if(ptr3 == NULL){
			  if(autocompleat(s)){
				   return;
			  }
 			  shell_output(s,"ERROR: Incorrect port list","");
 			  return;
 		  }
 		  else{
 			  ptr4 = findcmd(s,ptr3,"speed");
 			  if(ptr4 != NULL){
 				  ptr5 = findcmd(s,ptr4,"auto");
 				  if(ptr5 != NULL){
 					  if(autocompleat(s)){
 						   return;
 					  }
 					  sprintf(temp,"%s speed auto",ports_print(ports));
 					  shell_output(s,"Command: config ports ",temp);
 					  for(u8 i=0;i<ALL_PORT_NUM;i++){
 						  if(ports[i] == 1)
 							  set_port_speed_dplx(i,SPEED_AUTO);
 					  }
 					  shell_output(s,"Success.","");
 					  settings_add2queue(SQ_PORT_ALL);
 					  return;
 				  }
 				  else{
 					  ptr5 = findcmd(s,ptr4,"100_full");
 					  if(ptr5 != NULL){
 						  if(autocompleat(s)){
 							   return;
 						  }
 	 					  sprintf(temp,"%s speed 100_full",ports_print(ports));
 	 					  shell_output(s,"Command: config ports ",temp);
 						  for(u8 i=0;i<ALL_PORT_NUM;i++){
 							  if(ports[i] == 1)
 								  set_port_speed_dplx(i,SPEED_100_FULL);
 						  }
 						  shell_output(s,"Ports speed: ","100 Full Duplex");
 						  settings_add2queue(SQ_PORT_ALL);
 						  return;
 					  }
 					  else{
 						  ptr5 = findcmd(s,ptr4,"100_half");
 						  if(ptr5 != NULL){
 							  if(autocompleat(s)){
 								   return;
 							  }
 	 	 					  sprintf(temp,"%s speed 100_half",ports_print(ports));
 	 	 					  shell_output(s,"Command: config ports ",temp);
 							  for(u8 i=0;i<PORT_NUM;i++){
 								  if(ports[i] == 1)
 									  set_port_speed_dplx(i,SPEED_100_HALF);
 							  }
 							  shell_output(s,"Success.","");
 							  settings_add2queue(SQ_PORT_ALL);
 							  return;
 						  }
 						  else{
 							  ptr5 = findcmd(s,ptr4,"10_full");
 							  if(ptr5 != NULL){
 								  if(autocompleat(s)){
 									   return;
 								  }
 	 	 	 					  sprintf(temp,"%s speed 10_full",ports_print(ports));
 	 	 	 					  shell_output(s,"Command: config ports ",temp);
 								  for(u8 i=0;i<ALL_PORT_NUM;i++){
 									  if(ports[i] == 1)
 										  set_port_speed_dplx(i,SPEED_10_FULL);
 								  }
 								  shell_output(s,"Success.","");
 								  settings_add2queue(SQ_PORT_ALL);
 								  return;
 							  }
 							  else
 							  {
 								  ptr5 = findcmd(s,ptr4,"10_half");
 								  if(ptr5 != NULL){
 									  if(autocompleat(s)){
 										   return;
 									  }
 	 	 	 	 					  sprintf(temp,"%s speed 10_half",ports_print(ports));
 	 	 	 	 					  shell_output(s,"Command: config ports ",temp);
 									  for(u8 i=0;i<ALL_PORT_NUM;i++){
 										  if(ports[i] == 1)
 											  set_port_speed_dplx(i,SPEED_10_HALF);
 									  }
 									  shell_output(s,"Success.","");
 									  settings_add2queue(SQ_PORT_ALL);
 									  return;
 								  }
 								  else
 								  {
 	 								  ptr5 = findcmd(s,ptr4,"1000_full");
 	 								  if(ptr5 != NULL){
 	 									  if(autocompleat(s)){
 	 										   return;
 	 									  }
 	 	 	 	 	 					  sprintf(temp,"%s speed 1000_full",ports_print(ports));
 	 	 	 	 	 					  shell_output(s,"Command: config ports ",temp);
 	 									  for(u8 i=0;i<ALL_PORT_NUM;i++){
 	 										  if(ports[i] == 1)
 	 											  set_port_speed_dplx(i,SPEED_1000_FULL);
 	 									  }
 	 									  shell_output(s,"Success.","");
 	 									  settings_add2queue(SQ_PORT_ALL);
 	 									  return;
 	 								  }
 	 								  else{
 	 									   if(autocompleat(s)){
 	 										  return;
 	 									   }
										  shell_output(s,"ERROR: Command","");
										  shell_output(s,"Next possible completions:","");
										  shell_output(s,"auto","");
										  shell_output(s,"100_full","");
										  shell_output(s,"100_half","");
										  shell_output(s,"10_full","");
										  shell_output(s,"10_half","");
										  return;
 	 								  }
 								  }
 							  }
 						  }
 					  }

 				  }
 			  }
 			  else{
 				  ptr4 = findcmd(s,ptr3,"flow_control");
 				  if(ptr4 != NULL){
 					  ptr5 = findcmd(s,ptr4,"enable");
 					  if(ptr5 != NULL){
 						  if(autocompleat(s)){
 							 return;
 						  }
	 	 				  sprintf(temp,"%s flow_control enable",ports_print(ports));
	 	 				  shell_output(s,"Command: config ports ",temp);
 						  for(u8 i=0;i<ALL_PORT_NUM;i++){
 							  if(ports[i] == 1)
 								  set_port_flow(i,ENABLE);
 						  }
 						  shell_output(s,"Success.","");
 						  settings_add2queue(SQ_PORT_ALL);
 						  return;
 					  }else{
 						  ptr5 = findcmd(s,ptr4,"disable");
 						  if(ptr5 != NULL){
 							  if(autocompleat(s)){
 								 return;
 							  }
 		 	 				  sprintf(temp,"%s flow_control disable",ports_print(ports));
 		 	 				  shell_output(s,"Command: config ports ",temp);
 							  for(u8 i=0;i<ALL_PORT_NUM;i++){
 								  if(ports[i] == 1)
 									  set_port_flow(i,DISABLE);
 							  }
 							  shell_output(s,"Success.","");
 							  settings_add2queue(SQ_PORT_ALL);
 							  return;
 						  }
 						  else{
 							  if(autocompleat(s)){
 								  return;
 							  }
 							  shell_output(s,"ERROR: Command","");
 							  shell_output(s,"Next possible completions:","");
 							  shell_output(s,"enable","");
 							  shell_output(s,"disable","");
 							  return;
 						  }
 					  }
 				  }
 				  else{
 					  ptr4 = findcmd(s,ptr3,"state");
 					  if(ptr4 != NULL){
  						  ptr5 = findcmd(s,ptr4,"enable");
 						  if(ptr5 != NULL){
 							  if(autocompleat(s)){
 								 return;
 							  }
 		 	 				  sprintf(temp,"%s state enable",ports_print(ports));
 		 	 				  shell_output(s,"Command: config ports ",temp);
 							  for(u8 i=0;i<ALL_PORT_NUM;i++){
 								  if(ports[i] == 1)
 									  set_port_state(i,ENABLE);
 							  }
 							  shell_output(s,"Success.","");
 							  settings_add2queue(SQ_PORT_ALL);
 							  return;
 						  }else{
 							  ptr5 = findcmd(s,ptr4,"disable");
 							  if(ptr5 != NULL){
 								  if(autocompleat(s)){
 									 return;
 								  }
 	 		 	 				  sprintf(temp,"%s state disable",ports_print(ports));
 	 		 	 				  shell_output(s,"Command: config ports ",temp);
 								  for(u8 i=0;i<ALL_PORT_NUM;i++){
 									  if(ports[i] == 1)
 										  set_port_state(i,DISABLE);
 								  }
 								  shell_output(s,"Success.","");
 								  settings_add2queue(SQ_PORT_ALL);
 								  return;
 							  }
 							  else{
 								  if(autocompleat(s)){
 									  return;
 								  }
 								  shell_output(s,"ERROR: Command","");
 								  shell_output(s,"Next possible completions:","");
 								  shell_output(s,"enable","");
 								  shell_output(s,"disable","");
 								  return;
 							  }
 						  }
 					  }
 					  else
 					  {
 						  ptr4 = findcmd(s,ptr3,"poe");
 						  if(ptr4 != NULL){
 							  ptr5 = findcmd(s,ptr4,"auto");
 							  if(ptr5 != NULL){
 								  if(autocompleat(s)){
 									 return;
 								  }
 	 		 	 				  sprintf(temp,"%s poe auto",ports_print(ports));
 	 		 	 				  shell_output(s,"Command: config ports ",temp);
 								  for(u8 i=0;i<ALL_PORT_NUM;i++){
 									  if(ports[i] == 1){
 										  set_port_poe(i,POE_AUTO);
 										  set_poe_init(0);//reinit poe if need
 									  }
 								  }
 								  shell_output(s,"Success.","");
 								  settings_add2queue(SQ_POE);
 								  return;
 							  }else{
 								  ptr5 = findcmd(s,ptr4,"disable");
 								  if(ptr5 != NULL){
 									  if(autocompleat(s)){
 										 return;
 									  }
 	 	 		 	 				  sprintf(temp,"%s poe disable",ports_print(ports));
 	 	 		 	 				  shell_output(s,"Command: config ports ",temp);
 									  for(u8 i=0;i<ALL_PORT_NUM;i++){
 										  if(ports[i] == 1){
 											  set_port_poe(i,DISABLE);
 											  set_poe_init(0);//reinit poe if need
 										  }
 									  }
 									  shell_output(s,"Success.","");
 									  settings_add2queue(SQ_POE);
 									  return;
 								  }
 								  else{
 	 								  ptr5 = findcmd(s,ptr4,"passive");
 	 								  if(ptr5 != NULL){
 	 									  if(autocompleat(s)){
 	 										 return;
 	 									  }
 	 	 	 		 	 				  sprintf(temp,"%s poe passive",ports_print(ports));
 	 	 	 		 	 				  shell_output(s,"Command: config ports ",temp);
 	 									  for(u8 i=0;i<ALL_PORT_NUM;i++){
 	 										  if(ports[i] == 1){
 	 											  set_port_poe(i,POE_MANUAL_EN);
 	 											  set_poe_init(0);//reinit poe if need
 	 										  }
 	 									  }
 	 									  shell_output(s,"Success.","");
 	 									  settings_add2queue(SQ_POE);
 	 									  return;
 	 								  }
 	 								  else{
 	 	 								  ptr5 = findcmd(s,ptr4,"ultrapoe");
 	 	 								  if(ptr5 != NULL){
 	 	 									  if(autocompleat(s)){
 	 	 										 return;
 	 	 									  }
 	 	 	 	 		 	 				  sprintf(temp,"%s poe ultrapoe",ports_print(ports));
 	 	 	 	 		 	 				  shell_output(s,"Command: config ports ",temp);
 	 	 									  for(u8 i=0;i<ALL_PORT_NUM;i++){
 	 	 										  if(ports[i] == 1){
 	 	 											  set_port_poe(i,POE_ULTRAPOE);

 	 	 										  }
 	 	 									  }
 	 	 									  set_poe_init(0);//reinit poe if need
 	 	 									  shell_output(s,"Success.","");
 	 	 									  settings_add2queue(SQ_POE);
 	 	 									  return;
 	 	 								  }
 	 	 								  else{
 	 	 	 								  ptr5 = findcmd(s,ptr4,"onlya");
 	 	 	 								  if(ptr5 != NULL){
 	 	 	 									  if(autocompleat(s)){
 	 	 	 										 return;
 	 	 	 									  }
 	 	 	 	 	 		 	 				  sprintf(temp,"%s poe onlya",ports_print(ports));
 	 	 	 	 	 		 	 				  shell_output(s,"Command: config ports ",temp);
 	 	 	 									  for(u8 i=0;i<ALL_PORT_NUM;i++){
 	 	 	 										  if(ports[i] == 1){
 	 	 	 											  set_port_poe(i,POE_ONLY_A);
 	 	 	 											  set_poe_init(0);//reinit poe if need
 	 	 	 										  }
 	 	 	 									  }
 	 	 	 									  shell_output(s,"Success.","");
 	 	 	 									  settings_add2queue(SQ_POE);
 	 	 	 									  return;
 	 	 	 								  }
 	 	 	 								  else{
 	 	 	 	 								  ptr5 = findcmd(s,ptr4,"onlyb");
 	 	 	 	 								  if(ptr5 != NULL){
 	 	 	 	 									  if(autocompleat(s)){
 	 	 	 	 										 return;
 	 	 	 	 									  }
 	 	 	 	 	 	 		 	 				  sprintf(temp,"%s poe onlyb",ports_print(ports));
 	 	 	 	 	 	 		 	 				  shell_output(s,"Command: config ports ",temp);
 	 	 	 	 									  for(u8 i=0;i<ALL_PORT_NUM;i++){
 	 	 	 	 										  if(ports[i] == 1){
 	 	 	 	 											  set_port_poe(i,POE_ONLY_B);
 	 	 	 	 											  set_poe_init(0);//reinit poe if need
 	 	 	 	 										  }
 	 	 	 	 									  }
 	 	 	 	 									  shell_output(s,"Success.","");
 	 	 	 	 									  settings_add2queue(SQ_POE);
 	 	 	 	 									  return;
 	 	 	 	 								  }
 	 	 	 	 								  else{
 	 	 	 	 									  if(autocompleat(s)){
														  return;
													  }
													  shell_output(s,"ERROR: Command","");
													  shell_output(s,"Next possible completions:","");
													  shell_output(s,"disable","");
													  shell_output(s,"auto","");
													  if(get_dev_type()==DEV_PSW2GPLUS ||
														 get_dev_type() == DEV_PSW2G2FPLUS ||
														 get_dev_type() == DEV_PSW2G2FPLUSUPS ||
														 get_dev_type() == DEV_PSW2G6F){
														 shell_output(s,"passive","");
														 shell_output(s,"ultrapoe","");
														 shell_output(s,"onlya","");
														 shell_output(s,"onlyb","");
													  }
													  return;
 	 	 	 	 								  }
 	 	 	 								  }
 	 	 								  }
 	 								  }
 								  }
 							  }
 						  }
 						  else{
 							  ptr4 = findcmd(s,ptr3,"sfp_mode");
 							  if(ptr4!=NULL){
 								 ptr5 = findcmd(s,ptr4,"auto");
								 if(ptr5 != NULL){
									 if(autocompleat(s)){
										 return;
									 }
	 	 		 	 				 sprintf(temp,"%s sfp_mode auto",ports_print(ports));
	 	 		 	 				 shell_output(s,"Command: config ports ",temp);
									 for(u8 i=0;i<ALL_PORT_NUM;i++){
										 if(ports[i] == 1){
											 if(i>=GE1){
												 set_port_sfp_mode(i,1);
												 settings_add2queue(SQ_PORT_ALL);
											 }
											 else{
												 shell_output(s,"ERROR: Only for SFP ports!","");
												 return;
											 }
										 }
									 }
									 shell_output(s,"Success.","");
									 return;
 								 }
								 else{
									 ptr5 = findcmd(s,ptr4,"forced");
									 if(ptr5!=NULL){
										 if(autocompleat(s)){
											return;
										 }
		 	 		 	 				 sprintf(temp,"%s sfp_mode forced",ports_print(ports));
		 	 		 	 				 shell_output(s,"Command: config ports ",temp);
										 for(u8 i=0;i<ALL_PORT_NUM;i++){
											 if(ports[i] == 1){
												 if(i>=GE1){
													 set_port_sfp_mode(i,0);
													 //shell_output(s,"Ports SFP Mode: ","auto");
												 }
												 else{
													 shell_output(s,"ERROR: Only for SFP ports!","");
													 return;
												 }
											 }
										 }
										 shell_output(s,"Success.","");
										 settings_add2queue(SQ_PORT_ALL);
										 return;
									 }
									 else{
										   if(autocompleat(s)){
											  return;
										   }
										  shell_output(s,"ERROR: Command","");
		 								  shell_output(s,"Next possible completions:","");
										  shell_output(s,"forced","");
										  shell_output(s,"auto","");
										  return;
									 }
								 }

 							  }
 							  else{
 								   if(autocompleat(s)){
 									  return;
 								   }
 								  shell_output(s,"ERROR: Command","");
 								  shell_output(s,"Next possible completions:","");
								  shell_output(s,"state","");
								  shell_output(s,"speed","");
								  shell_output(s,"flow_control","");
								  shell_output(s,"poe","");
								  return;
 							  }
 						  }
 					  }
 				  }
 			  }
 		  }
 	  }
   }


   //device description
   ptr1 = findcmd(s,str,"description");
   if(ptr1 != NULL){
 	  ptr2 = findcmd(s,ptr1," ");
 	  if(ptr2!=NULL){
		  ptr3 = findcmd(s,ptr2,"name");
		  if(ptr3!=NULL){
			  if(autocompleat(s)){
				 return;
			  }
			  ptr4 = findcmd(s,ptr3," ");
			  if(ptr4!=NULL){
				  shell_output(s,"Command: config description name ",ptr4);
				  set_interface_name(ptr4);
				  shell_output(s,"Success.","");
				  return;
			  }
			  else{
		 		  shell_output(s,"ERROR: incorrect Device Name","");
		 		  return;
			  }
		  }
		  else{
			  ptr3 = findcmd(s,ptr2,"location");
			  if(ptr3!=NULL){
				  if(autocompleat(s)){
					 return;
				  }
				  ptr4 = findcmd(s,ptr3," ");
				  if(ptr4!=NULL){
					  shell_output(s,"Command: config description location ",ptr4);
					  set_interface_location(ptr4);
					  shell_output(s,"Success.","");
					  return;
				  }
				  else{
			 		  shell_output(s,"ERROR: incorrect Device Location name","");
			 		  return;
				  }
			  }
			  else{
				  ptr3 = findcmd(s,ptr2,"company");
				  if(ptr3!=NULL){
					  if(autocompleat(s)){
						 return;
					  }
					  ptr4 = findcmd(s,ptr3," ");
					  if(ptr4!=NULL){
						  shell_output(s,"Command: config description company ",ptr4);
						  set_interface_contact(ptr4);
						  shell_output(s,"Success.","");
						  return;
					  }
					  else{
				 		  shell_output(s,"ERROR: incorrect Service Company name","");
				 		  return;
					  }
				  }
				  else{
					  if(autocompleat(s)){
						  return;
					  }
					  shell_output(s,"ERROR: Command","");
			 		  shell_output(s,"Next possible completions:","");
			 		  shell_output(s,"name","");
			 		  shell_output(s,"location","");
			 		  shell_output(s,"company","");
			 		  //TODO: Добавить команду настройки Port Description
			 		  return;
				  }
			  }
		  }
 	  }
 	  else{
		  if(autocompleat(s)){
			  return;
		  }
 		  shell_output(s,"ERROR: Command","");
 		  shell_output(s,"Next possible completions:","");
 		  shell_output(s,"name","");
 		  shell_output(s,"location","");
 		  shell_output(s,"company","");
 		  return;
 	  }
   }





   //mac address filtering
   ptr1 = findcmd(s,str,"mac_filtering");
   if(ptr1 != NULL){
	   if(autocompleat(s)){
		   return;
	   }
	   ptr2 = findcmd(s,ptr1," ");
	   if(ptr2!=NULL){
		  ptr3 = findcmd(s,ptr2,"port_state");
		  if(ptr3!=NULL){
			  if(autocompleat(s)){
				 return;
			  }
			  ptr4 = findcmd(s,ptr3," ");
			  if(ptr4 == NULL){
				  shell_output(s,"ERROR: Incorrect port list","");
				  return;
			  }
			  ptr5 = find_port(ptr4,ports);
			  if(ptr5 == NULL){
				  shell_output(s,"ERROR: Incorrect port list","");
				  return;
			  }
			  else{
				  ptr6 = findcmd(s,ptr5,"normal");
				  if(ptr6 != NULL){
					  if(autocompleat(s)){
						 return;
					  }
					  sprintf(temp,"%s port_state normal",ports_print(ports));
					  shell_output(s,"Command: config mac_filtering ",temp);
				  	  for(u8 i = 0;i<(ALL_PORT_NUM);i++){
						  if(ports[i]==1){
							  set_mac_filter_state(i,PORT_NORMAL);
							  return;
						  }
					  }
				  	  shell_output(s,"Success.","");
				  	  settings_add2queue(SQ_PORT_ALL);
				  	  return;
				  }
				  else{
					  ptr6 = findcmd(s,ptr5,"mac");
					  if(ptr6 != NULL){
						  if(autocompleat(s)){
							 return;
						  }
						  sprintf(temp,"%s port_state mac",ports_print(ports));
						  shell_output(s,"Command: config mac_filtering ",temp);
					  	  for(u8 i = 0;i<(ALL_PORT_NUM);i++){
							  if(ports[i]==1){
								  set_mac_filter_state(i,MAC_FILT);
								  return;
							  }
						  }
					  	  shell_output(s,"Success.","");
					  	  settings_add2queue(SQ_PORT_ALL);
					  	  return;
					  }
					  else{
						  ptr6 = findcmd(s,ptr5,"port");
						  if(ptr6 != NULL){
							  if(autocompleat(s)){
								 return;
							  }
							  sprintf(temp,"%s port_state port",ports_print(ports));
							  shell_output(s,"Command: config mac_filtering ",temp);
						  	  for(u8 i = 0;i<(ALL_PORT_NUM);i++){
								  if(ports[i]==1){
									  set_mac_filter_state(i,PORT_FILT);
									  return;
								  }
							  }
						  	  shell_output(s,"Success.","");
						  	  settings_add2queue(SQ_PORT_ALL);
						  	  return;
						  }
						  else{
							  ptr6 = findcmd(s,ptr5,"port_temp");
							  if(ptr6 != NULL){
								  if(autocompleat(s)){
									 return;
								  }
								  sprintf(temp,"%s port_state port_temp",ports_print(ports));
								  shell_output(s,"Command: config mac_filtering ",temp);
							  	  for(u8 i = 0;i<(ALL_PORT_NUM);i++){
									  if(ports[i]==1){
										  set_mac_filter_state(i,PORT_FILT_TEMP);
										  return;
									  }
								  }
							  	  shell_output(s,"Success.","");
							  	  settings_add2queue(SQ_PORT_ALL);
							  	  return;
							  }
							  else{
								  shell_output(s,"Next possible completions:","");
								 						  shell_output(s,"normal","");
								 						  shell_output(s,"mac","");
								 						  shell_output(s,"port","");
								 						  shell_output(s,"port_temp","");
								 						  return;
							  }
						  }
					  }
				  }
			  }
		  }
		  else{
			  ptr3 = findcmd(s,ptr2,"add");
			  if(ptr3!=NULL){
				  if(autocompleat(s)){
					 return;
				  }
				  ptr4 = findcmd(s,ptr3," ");
				  if(ptr4 == NULL){
					  shell_output(s,"ERROR: Incorrect port list","");
					  return;
				  }
				  ptr5 = find_port(ptr4,ports);
				  if(ptr5 == NULL){
					  shell_output(s,"ERROR: Incorrect port list","");
					  return;
				  }
				  ptr6 =  findcmd(s,ptr5," ");
				  if(ptr6 == NULL){
					  shell_output(s,"ERROR: Incorrect MAC address","");
					  return;
				  }
				  ptr7 = findmac(s,ptr6,mac);
				  if(ptr7 != NULL){
					  for(u8 i = 0;i<(ALL_PORT_NUM);i++){
						 if(ports[i]==1){
							sprintf(temp,"%d",i+1);
							add_mac_bind_entry(mac,i);
						 }
					  }
					  sprintf(temp,"%s %02X:%02X:%02X:%02X:%02X:%02X",ports_print(ports),mac[0],mac[1],mac[2],
							  mac[3],mac[4],mac[5]);
					  shell_output(s,"Command: config mac_filtering add ",temp);
					  settings_add2queue(SQ_PORT_ALL);
					  return;
				  }
				  else{
					  shell_output(s,"ERROR: Incorrect MAC address","");
					  return;
				  }

			  }
			  else{
				  ptr3 = findcmd(s,ptr2,"del");
				  if(ptr3 != NULL){
					  if(autocompleat(s)){
						 return;
					  }
					  ptr4 = findcmd(s,ptr3," ");
					  if(ptr4 == NULL){
						  shell_output(s,"ERROR: Incorrect port list","");
						  return;
					  }
					  ptr5 = find_port(ptr4,ports);
					  if(ptr5 == NULL){
						  shell_output(s,"ERROR: Incorrect port list","");
						  return;
					  }
					  ptr6 =  findcmd(s,ptr5," ");
					  if(ptr6 == NULL){
						  shell_output(s,"ERROR: Incorrect MAC address","");
						  return;
					  }

					  ptr7 = findmac(s,ptr6,mac);
					  if(ptr7 != NULL){
						  for(u8 i = 0;i<(ALL_PORT_NUM);i++){
							 if(ports[i]==1){
								sprintf(temp,"%d",i+1);
								//find entry
								for(u8 j=0;j<get_mac_bind_num();j++){
									if(get_mac_bind_entry_port(j)==i &&
									   get_mac_bind_entry_mac(j,0) == mac[0] &&
									   get_mac_bind_entry_mac(j,1) == mac[1] &&
									   get_mac_bind_entry_mac(j,2) == mac[2] &&
									   get_mac_bind_entry_mac(j,3) == mac[3] &&
									   get_mac_bind_entry_mac(j,4) == mac[4] &&
									   get_mac_bind_entry_mac(j,5) == mac[5]){
										  del_mac_bind_entry(j);
										  sprintf(temp,"%d %02X:%02X:%02X:%02X:%02X:%02X",i+1,mac[0],mac[1],mac[2],
										 	mac[3],mac[4],mac[5]);
										  shell_output(s,"Command: config mac_filtering del ",temp);
										  settings_add2queue(SQ_PORT_ALL);
										  return;
									}
								}
							 }
						  }
						  shell_output(s,"ERROR: Entry not found","");
						  return;
					  }
					  else{
						  shell_output(s,"ERROR: Incorrect MAC address","");
						  return;
					  }

				  }
				  else{
					  shell_output(s,"Next possible completions:","");
					  shell_output(s,"port_state","");
					  shell_output(s,"add","");
					  shell_output(s,"del","");
					  return;
				  }
			  }
		  }
	   }
   }

   //PLC - inputs
   ptr1 = findcmd(s,str,"inputs");
   if(ptr1 != NULL){
	   if(autocompleat(s)){
	      return;
	   }
	   ptr2 = findcmd(s,ptr1," ");
	   temp_u32 = strtoul(ptr2,&ptr2,10);
	   if(temp_u32<=(PLC_INPUTS+1) && temp_u32){
		   ptr3 = findcmd(s,ptr2,"state");
		   if(ptr3 != NULL){
			   if(autocompleat(s)){
				   return;
			   }
			   ptr4 = findcmd(s,ptr3,"enable");
			   if(ptr4 != NULL){
			       if(autocompleat(s)){
			    	   return;
				   }
				   set_plc_in_state((u8)temp_u32,ENABLE);
				   sprintf(temp,"%d state enable",(u8)temp_u32);
				   shell_output(s,"Command: config inputs ",temp);
				   shell_output(s,"Success.","");
				   return;
			   }
			   else{
				   ptr4 = findcmd(s,ptr3,"disable");
				   if(ptr4 != NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   set_plc_in_state((u8)temp_u32,DISABLE);
					   sprintf(temp,"%d state disable",(u8)temp_u32);
					   shell_output(s,"Command: config inputs ",temp);
					   shell_output(s,"Success.","");
					   return;
				   }
				   else{
					  shell_output(s,"Next possible completions:","");
					  shell_output(s,"enable","");
					  shell_output(s,"disable","");
					  return;
				   }
			   }
		   }
		   else{
			   ptr3 = findcmd(s,ptr2,"alarm_level");
			   if(ptr3 != NULL){
				  if(autocompleat(s)){
					 return;
				  }
				  ptr4 = findcmd(s,ptr3,"open");
				  if(ptr4 != NULL){
					  if(autocompleat(s)){
						 return;
					  }
					  set_alarm_front((u8)temp_u32,2);
					  sprintf(temp,"%d alarm_level open",(u8)temp_u32);
					  shell_output(s,"Command: config inputs ",temp);
					  shell_output(s,"Success.","");
					  return;
				  }
				  else{
					  ptr4 = findcmd(s,ptr3,"short");
					  if(ptr4 != NULL){
						  if(autocompleat(s)){
							 return;
						  }
						  set_alarm_front((u8)temp_u32,1);
						  sprintf(temp,"%d alarm_level short",(u8)temp_u32);
						  shell_output(s,"Command: config inputs ",temp);
						  shell_output(s,"Success.","");
						  return;
					  }
					  else{
						  ptr4 = findcmd(s,ptr3,"any");
						  if(ptr4 != NULL){
							  if(autocompleat(s)){
								 return;
							  }
							  set_alarm_front((u8)temp_u32,3);
							  sprintf(temp,"%d alarm_level any",(u8)temp_u32);
							  shell_output(s,"Command: config inputs ",temp);
							  shell_output(s,"Success.","");
							  return;
						  }
						  else{
							  shell_output(s,"Next possible completions:","");
							  shell_output(s,"open","");
							  shell_output(s,"short","");
							  shell_output(s,"any","");
							  return;
						  }
					  }
				  }
			   }
			   else{
				  shell_output(s,"Next possible completions:","");
				  shell_output(s,"state","");
				  shell_output(s,"alarm_level","");
				  return;
			   }
		   }
	   }
	   else{
		   shell_output(s,"ERROR: Input number <1-3>","");
		   return;
	   }
   }

   //PLC - outputs
   ptr1 = findcmd(s,str,"outputs");
   if(ptr1 != NULL){
	   if(autocompleat(s)){
	      return;
	   }
	   ptr2 = findcmd(s,ptr1," ");
	   temp_u32 = strtoul(ptr2,&ptr2,10);
	   if(temp_u32<=(PLC_RELAY_OUT) && temp_u32){
		   ptr3 = findcmd(s,ptr2,"state");
		   if(ptr3 != NULL){
			   if(autocompleat(s)){
				   return;
			   }
			   ptr4 = findcmd(s,ptr3,"short");
			   if(ptr4 != NULL){
				   if(autocompleat(s)){
					   return;
				   }
				   set_plc_out_state((u8)temp_u32,0);
				   sprintf(temp,"%d state short",(u8)temp_u32);
				   shell_output(s,"Command: config outputs ",temp);
				   shell_output(s,"Success.","");
				   return;
			   }
			   else{
				   ptr4 = findcmd(s,ptr3,"open");
				   if(ptr4 != NULL){
					   if(autocompleat(s)){
						   return;
					   }
					   set_plc_out_state((u8)temp_u32,1);
					   sprintf(temp,"%d state open",(u8)temp_u32);
					   shell_output(s,"Command: config outputs ",temp);
					   shell_output(s,"Success.","");
					   return;
				   }
				   else{
					   ptr4 = findcmd(s,ptr3,"logic");
					   if(ptr4 != NULL){
						   if(autocompleat(s)){
							   return;
						   }
						   set_plc_out_state((u8)temp_u32,2);
						   sprintf(temp,"%d state logic",(u8)temp_u32);
						   shell_output(s,"Command: config outputs ",temp);
						   shell_output(s,"Success.","");
						   return;
					   }
					   else{
						   shell_output(s,"Next possible completions:","");
						   shell_output(s,"short","");
						   shell_output(s,"open","");
						   shell_output(s,"logic","");
						   return;
					   }
				   }

			   }

		   }
		   else{
			   ptr3 = findcmd(s,ptr2,"events");
			   if(ptr3 != NULL){
				   if(autocompleat(s)){
					  return;
				   }
				   for(u8 i=0;i<5;i++)
					   set_plc_out_event((u8)temp_u32,i,0);
				   //sensor1
				   ptr4 = findcmd(s,ptr3,"sensor1");
				   if(ptr4 != NULL){
					   set_plc_out_event((u8)temp_u32,0,1);
				   }
				   //sensor2
				   ptr4 = findcmd(s,ptr3,"sensor2");
				   if(ptr4 != NULL){
					   set_plc_out_event((u8)temp_u32,1,1);
				   }
				   //input 1
				   ptr4 = findcmd(s,ptr3,"input1");
				   if(ptr4 != NULL){
					   set_plc_out_event((u8)temp_u32,2,1);
				   }
				   //input2
				   ptr4 = findcmd(s,ptr3,"input2");
				   if(ptr4 != NULL){
					   set_plc_out_event((u8)temp_u32,3,1);
				   }
				   //input3
				   ptr4 = findcmd(s,ptr3,"input3");
				   if(ptr4 != NULL){
					   set_plc_out_event((u8)temp_u32,4,1);
				   }

				   sprintf(temp,"%d event",(u8)temp_u32);
				   if(get_plc_out_event(temp_u32,0))
					   strcat(temp," sensor1");
				   if(get_plc_out_event(temp_u32,1))
					   strcat(temp," sensor2");
				   if(get_plc_out_event(temp_u32,2))
					   strcat(temp," input1");
				   if(get_plc_out_event(temp_u32,3))
					   strcat(temp," input2");
				   if(get_plc_out_event(temp_u32,4))
					   strcat(temp," input3");
				   shell_output(s,"Command: config outputs ",temp);
				   shell_output(s,"Success.","");
				   return;
			   }
			   else{
				   ptr3 = findcmd(s,ptr2,"action");
				   if(ptr3 != NULL){
					   if(autocompleat(s)){
					 	 return;
					   }
					   ptr4 = findcmd(s,ptr3,"short");
					   if(ptr4 != NULL){
						  if(autocompleat(s)){
							 return;
						  }
						  set_plc_out_action((u8)temp_u32,0);
						  sprintf(temp,"%d action short",(u8)temp_u32);
						  shell_output(s,"Command: config outputs ",temp);
						  shell_output(s,"Success.","");
						  return;
					   }
					   else{
						   ptr4 = findcmd(s,ptr3,"open");
						   if(ptr4 != NULL){
							  if(autocompleat(s)){
								 return;
							  }
							  set_plc_out_action((u8)temp_u32,1);
							  sprintf(temp,"%d action open",(u8)temp_u32);
							  shell_output(s,"Command: config outputs ",temp);
							  shell_output(s,"Success.","");
							  return;
						   }
						   else{
							   ptr4 = findcmd(s,ptr3,"impulse");
							   if(ptr4 != NULL){
								  if(autocompleat(s)){
									 return;
								  }
								  set_plc_out_action((u8)temp_u32,2);
								  sprintf(temp,"%d action impulse",(u8)temp_u32);
								  shell_output(s,"Command: config outputs ",temp);
								  shell_output(s,"Success.","");
								  return;
							   }
							   else{
								   shell_output(s,"Next possible completions:","");
								   shell_output(s,"short","");
								   shell_output(s,"open","");
								   shell_output(s,"impulse","");
								   return;
							   }
						   }
					   }
				   }
				   else{
					   shell_output(s,"Next possible completions:","");
					   shell_output(s,"state","");
					   shell_output(s,"events","");
					   shell_output(s,"action","");
					   return;
				   }
			   }
		   }
	   }
	   else{
		   shell_output(s,"ERROR: Output number <1-2>","");
		   return;
	   }
   }

    //PLC - rs-485
    ptr1 = findcmd(s,str,"rs485");
    if(ptr1 != NULL){
		if(autocompleat(s)){
			return;
		}
		ptr2 = findcmd(s,ptr1,"baudrate");
		if(ptr2 != NULL){
			if(autocompleat(s)){
				return;
			}
			ptr2 = findcmd(s,ptr1," ");
			temp_u32 = strtoul(ptr2,NULL,10);
			if((temp_u32 % 300) == 0){
				switch(temp_u32){
					case 300: set_plc_em_rate(BR_300);break;
					case 600: set_plc_em_rate(BR_600);break;
					case 1200: set_plc_em_rate(BR_1200);break;
					case 2400: set_plc_em_rate(BR_2400);break;
					case 4800: set_plc_em_rate(BR_4800);break;
					case 9600: set_plc_em_rate(BR_9600);break;
					case 19200: set_plc_em_rate(BR_19200);break;
					default:
						shell_output(s,"ERROR: baudrate <300, 600, 1200, 2400, 4800, 9600, 19200>","");
						return;
				}
				sprintf(temp,"%lu",temp_u32);
				shell_output(s,"Command: config rs485 baudrate",temp);
				shell_output(s,"Success.","");
				return;
			}
			else{
				shell_output(s,"ERROR: baudrate <300, 600, 1200, 2400, 4800, 9600, 19200>","");
				return;
			}

		}
		else{
			ptr2 = findcmd(s,ptr1,"parity");
			if(ptr2 != NULL){
				if(autocompleat(s)){
					return;
				}
				ptr3 = findcmd(s,ptr2,"disable");
				if(ptr3 != NULL){
					if(autocompleat(s)){
						return;
					}
					set_plc_em_parity(NO_PARITY);
					shell_output(s,"Command: config rs485 parity disable","");
					shell_output(s,"Success.","");
					return;
				}
				else{
					ptr3 = findcmd(s,ptr2,"even");
					if(ptr3 != NULL){
						if(autocompleat(s)){
							return;
						}
						set_plc_em_parity(EVEN_PARITY);
						shell_output(s,"Command: config rs485 parity even","");
						shell_output(s,"Success.","");
						return;
					}
					else{
						ptr3 = findcmd(s,ptr2,"odd");
						if(ptr3 != NULL){
							if(autocompleat(s)){
								return;
							}
							set_plc_em_parity(ODD_PARITY);
							shell_output(s,"Command: config rs485 parity odd","");
							shell_output(s,"Success.","");
							return;
						}
						else{
						   shell_output(s,"Next possible completions:","");
						   shell_output(s,"disable","");
						   shell_output(s,"even","");
						   shell_output(s,"odd","");
						   return;
						}
					}
				}
			}
			else{
				ptr2 = findcmd(s,ptr1,"databits");
				if(ptr2 != NULL){
					if(autocompleat(s)){
						return;
					}
					ptr2 = findcmd(s,ptr1," ");
					temp_u32 = strtoul(ptr2,NULL,10);
					if(temp_u32 >= 5 && temp_u32 <=9){
						set_plc_em_databits((u8)temp_u32);
						sprintf(temp,"%d",(u8)temp_u32);
						shell_output(s,"Command: config rs485 databits ",temp);
						shell_output(s,"Success.","");
						return;
					}
					else{
						shell_output(s,"ERROR: databits <5 - 9>","");
						return;
					}
				}
				else{
					ptr2 = findcmd(s,ptr1,"stopbits");
					if(ptr2!= NULL){
						if(autocompleat(s)){
							return;
						}
						ptr2 = findcmd(s,ptr1," ");
						temp_u32 = strtoul(ptr2,NULL,10);
						if(temp_u32 == 1 || temp_u32 == 2){
							set_plc_em_stopbits((u8)temp_u32);
							sprintf(temp,"%d",(u8)temp_u32);
							shell_output(s,"Command: config rs485 stopbits ",temp);
							shell_output(s,"Success.","");
							return;
						}
						else{
							shell_output(s,"ERROR: stopbits <1, 2>","");
							return;
						}
					}
					else{
						ptr2 = findcmd(s,ptr1,"model");
						if(ptr2 != NULL){
							if(autocompleat(s)){
								return;
							}
							ptr3 = findcmd(s,ptr2," ");
							  if(ptr3!=NULL){
								  ptr2 = findcmd(s,ptr1," ");
								  temp_u32 = strtoul(ptr2,NULL,10);
								  switch(temp_u32){
								  	  case 1:set_plc_em_model(1);break;
								  	  default:
								  		  shell_output(s,"ERROR: incorrect model","");
								  		  return;
								  }
								  get_plc_em_model_name_list((u8)temp_u32,temp);
								  shell_output(s,"Command: config rs485 model ",temp);
								  shell_output(s,"Success.","");
								  return;
							  }
							  else{
						 		  shell_output(s,"ERROR: incorrect model","");
						 		  return;
							  }
							sprintf(temp,"%d",(u8)temp_u32);
							shell_output(s,"Command: config rs485 model ",temp);
							shell_output(s,"Success.","");
						}
						else{
							ptr2 = findcmd(s,ptr1,"identification");
							if(ptr2 != NULL){
								if(autocompleat(s)){
									return;
								}
								ptr3 = findcmd(s,ptr2," ");
								if(ptr3!=NULL){
									set_plc_em_id(ptr3);
									shell_output(s,"Command: config rs485 identification ",ptr3);
									shell_output(s,"Success.","");
									return;
								}
								else{
									shell_output(s,"ERROR: incorrect identification","");
									return;
								}
							}
							else{
								ptr2 = findcmd(s,ptr1,"password");
								if(ptr2 != NULL){
									if(autocompleat(s)){
										return;
									}
									ptr3 = findcmd(s,ptr2," ");
									if(ptr3!=NULL){
										set_plc_em_pass(ptr3);
										shell_output(s,"Command: config rs485 password ",ptr3);
										shell_output(s,"Success.","");
										return;
									}
									else{
										shell_output(s,"ERROR: incorrect password","");
										return;
									}
								}
								else{
									shell_output(s,"Next possible completions:","");
									shell_output(s,"baudrate","");
									shell_output(s,"parity","");
									shell_output(s,"databits","");
									shell_output(s,"stopbits","");
									shell_output(s,"model","");
									shell_output(s,"identification","");
									shell_output(s,"password","");
									return;
								}
							}
						}
					}
				}
			}
		}

    }

    //aggregation
    ptr1 = findcmd(s,str,"aggregation");
    if(ptr1 != NULL){
    	if(autocompleat(s)){
    		return;
    	}
    	ptr2 = findcmd(s,ptr1,"trunk");
    	if(ptr2 != NULL){
        	if(autocompleat(s)){
        		return;
        	}
	    	ptr3 = findcmd(s,ptr2," ");
	    	if(ptr3 != NULL){
	    		temp_u32 = strtoul(ptr3,&ptr3,10);
	    		if(temp_u32 && temp_u32 <= (LAG_MAX_ENTRIES)){

	    			for(u8 i=0;i<LAG_MAX_ENTRIES;i++){
	    				if(get_lag_valid(i) && (get_lag_id(i)==temp_u32)){
	    					break;
	    				}
	    				else{
	    					if(i==(LAG_MAX_ENTRIES-1)){
	    						shell_output(s,"ERROR: trunkID is not found","");
	    						return;
	    					}

	    				}
	    			}

	    			ptr4 = findcmd(s,ptr3,"state");
	    			if(ptr4!=NULL){
	    	        	if(autocompleat(s)){
	    	        		return;
	    	        	}
	    	        	ptr5 = findcmd(s,ptr4,"enable");
	    	        	if(ptr5!=NULL){
		    	        	if(autocompleat(s)){
		    	        		return;
		    	        	}
		 	    			set_lag_state(temp_u32-1,ENABLE);
		 	    			sprintf(temp,"%d state enable",(u8)(temp_u32));
		 	    			shell_output(s,"Command: config aggregation trunk ",temp);
		    	    		shell_output(s,"Success.","");
		    	    		settings_add2queue(SQ_AGGREG);
		    	    		return;
	    	        	}
	    	        	else{
	    	        		ptr5 = findcmd(s,ptr4,"disable");
	    	        		if(ptr5!=NULL){
								if(autocompleat(s)){
									return;
								}
			 	    			set_lag_state(temp_u32-1,DISABLE);
			 	    			sprintf(temp,"%d state disable",(u8)(temp_u32));
			 	    			shell_output(s,"Command: config aggregation trunk ",temp);
			    	    		shell_output(s,"Success.","");
			    	    		settings_add2queue(SQ_AGGREG);
			    	    		return;
							}
							else{
								shell_output(s,"Next possible completions:","");
								shell_output(s,"enable","");
								shell_output(s,"disable","");
								return;
							}
	    	        	}
	    			}
	    			else{
	    				ptr4 = findcmd(s,ptr3,"master");
	    				if(ptr4!=NULL){
		    	        	if(autocompleat(s)){
		    	        		return;
		    	        	}
		    	        	ptr5 = findcmd(s,ptr4," ");
		    	        	ptr6 = find_port(ptr5,ports);
		    	        	if(ptr6 != NULL){
		    	        		for(u8 i=0;i<(ALL_PORT_NUM);i++){
		    	        			if(ports[i] == 1){
		    	        				set_lag_master_port(temp_u32-1,i);
    	    							sprintf(temp,"%d master %s",(u8)(temp_u32-1),ports_print(ports));
    				 	    			shell_output(s,"Command: config aggregation trunk ",temp);
		    	        				shell_output(s,"Success.","");
		    	        				settings_add2queue(SQ_AGGREG);
		    	        				return;
		    	        			}
		    	        		}
		    	        	}
		    	        	else{
		    	        		shell_output(s,"ERROR: Incorrect master port","");
							  	return;
		    	        	}
	    				}
	    				else{
	    					ptr4 = findcmd(s,ptr3,"ports");
	    					if(ptr4!=NULL){
								if(autocompleat(s)){
									return;
								}
								ptr5 = findcmd(s,ptr4," ");
								ptr6 = find_port(ptr5,ports);
								if(ptr6 != NULL){
									for(u8 i=0;i<(ALL_PORT_NUM);i++){
										if(ports[i] == 1){
											set_lag_port(temp_u32-1,i,ENABLE);
										}
									}
									sprintf(temp,"%d ports %s",(u8)(temp_u32-1),ports_print(ports));
									shell_output(s,"Command: config aggregation trunk ",temp);
									shell_output(s,"Success.","");
									settings_add2queue(SQ_AGGREG);
									return;
								}
								else{
									shell_output(s,"ERROR: Incorrect ports list","");
									return;
								}
							}
	    					else{
	    						shell_output(s,"Next possible completions:","");
								shell_output(s,"state","");
								shell_output(s,"master","");
								shell_output(s,"ports","");
								return;
	    					}
	    				}
	    			}

	    		}
	    		else{
	    			shell_output(s,"ERROR: incorrect trunkID, <1-5> ","");
	    			return;
	    		}
	    	}
	    	else{
				shell_output(s,"ERROR: incorrect trunkID, <1-5> ","");
				return;
	    	}
    	}
    	else{
    		ptr2 = findcmd(s,ptr1,"add");
    		if(ptr2 != NULL){
    	    	if(autocompleat(s)){
    	    		return;
    	    	}
    	    	ptr3 = findcmd(s,ptr2," ");
    	    	if(ptr3 != NULL){
    	    		temp_u32 = strtoul(ptr3,NULL,10);
    	    		if(temp_u32 && temp_u32 <= (LAG_MAX_ENTRIES)){
    	    			set_lag_valid(temp_u32-1,ENABLE);
    	    			shell_output(s,"Command: config aggregation add ",ptr3);
    	    			shell_output(s,"Success.","");
    	    			settings_add2queue(SQ_AGGREG);
    	    			return;
    	    		}
    	    		else{
    	    			shell_output(s,"ERROR: incorrect trunkID, <1-5> ","");
    	    			return;
    	    		}
    	    	}
    	    	else{
					shell_output(s,"ERROR: incorrect trunkID, <1-5> ","");
					return;
    	    	}

    		}
    		else{
    			ptr2 = findcmd(s,ptr1,"del");
    			if(ptr2 != NULL){
    			    if(autocompleat(s)){
    			    	return;
    			    }
        	    	ptr3 = findcmd(s,ptr2," ");
        	    	if(ptr3 != NULL){
        	    		temp_u32 = strtoul(ptr3,NULL,10);
        	    		if(temp_u32 && temp_u32 <= (LAG_MAX_ENTRIES)){
        	    			set_lag_valid(temp_u32-1,DISABLE);
        	    			shell_output(s,"Command: config aggregation del ",ptr3);
        	    			shell_output(s,"Success.","");
        	    			settings_add2queue(SQ_AGGREG);
        	    			return;
        	    		}
        	    		else{
        	    			shell_output(s,"ERROR: incorrect trunkID, <1-5> ","");
        	    			return;
        	    		}
        	    	}
        	    	else{
    					shell_output(s,"ERROR: incorrect trunkID, <1-5> ","");
    					return;
        	    	}


    			}
    			else{
    				shell_output(s,"Next possible completions:","");
					shell_output(s,"add","");
					shell_output(s,"del","");
					shell_output(s,"trunk","");
					return;
    			}
    		}
    	}
    }


    //mirroring
    ptr1 = findcmd(s,str,"mirroring");
    if(ptr1 != NULL){
    	if(autocompleat(s)){
       		return;
       	}
    	ptr2 = findcmd(s,ptr1,"state");
    	if(ptr2 != NULL){
    		if(autocompleat(s)){
				return;
			}
    		ptr3 = findcmd(s,ptr2,"enable");
    		if(ptr3 != NULL){
        		if(autocompleat(s)){
    				return;
    			}
        		set_mirror_state(ENABLE);
        		shell_output(s,"Command: config mirroring state enable","");
				shell_output(s,"Success.","");
				settings_add2queue(SQ_MIRROR);
				return;
    		}
    		else{
    			ptr3 = findcmd(s,ptr2,"disable");
				if(ptr3 != NULL){
					if(autocompleat(s)){
						return;
					}
	        		set_mirror_state(DISABLE);
	        		shell_output(s,"Command: config mirroring state disable","");
					shell_output(s,"Success.","");
					settings_add2queue(SQ_MIRROR);
					return;
				}
				else{
					shell_output(s,"Next possible completions:","");
					shell_output(s,"enable","");
					shell_output(s,"disable","");
					return;
				}
    		}
    	}
    	else{
        	ptr2 = findcmd(s,ptr1,"target");
        	if(ptr2 != NULL){
        		if(autocompleat(s)){
					return;
				}
        		ptr3 = findcmd(s,ptr2," ");
				ptr4 = find_port(ptr3,ports);
				if(ptr4 != NULL){
					for(u8 i=0;i<(ALL_PORT_NUM);i++){
						if(ports[i] == 1){
							set_mirror_target_port(i);
							shell_output(s,"Command: config mirroring target ",ports_print(ports));
							shell_output(s,"Success.","");
							settings_add2queue(SQ_MIRROR);
							return;
						}
					}
				}
				else{
					shell_output(s,"ERROR: Incorrect target port","");
					return;
				}
        	}
        	else{
        		ptr2 = findcmd(s,ptr1,"ports");
				if(ptr2 != NULL){
					if(autocompleat(s)){
						return;
					}
					ptr3 = findcmd(s,ptr2," ");
					ptr4 = find_port(ptr3,ports);
					if(ptr4 != NULL){
						ptr5 = findcmd(s,ptr4,"type");
						if(ptr5 != NULL){
							if(autocompleat(s)){
								return;
							}
							ptr6 = findcmd(s,ptr5,"normal");
							if(ptr6 != NULL){
								if(autocompleat(s)){
									return;
								}
								for(u8 i=0;i<(ALL_PORT_NUM);i++){
									if(ports[i] == 1){
										set_mirror_port(i,0);
									}
								}
								sprintf(temp,"%s type normal",ports_print(ports));
								shell_output(s,"Command: config mirroring ports ",temp);
								shell_output(s,"Success.","");
								settings_add2queue(SQ_MIRROR);
								return;
							}
							else{
								ptr6 = findcmd(s,ptr5,"rx");
								if(ptr6 != NULL){
									if(autocompleat(s)){
										return;
									}
									for(u8 i=0;i<(ALL_PORT_NUM);i++){
										if(ports[i] == 1){
											set_mirror_port(i,1);
										}
									}
									sprintf(temp,"%s type rx",ports_print(ports));
									shell_output(s,"Command: config mirroring ports ",temp);
									shell_output(s,"Success.","");
									settings_add2queue(SQ_MIRROR);
									return;
								}
								else{
									ptr6 = findcmd(s,ptr5,"tx");
									if(ptr6 != NULL){
										if(autocompleat(s)){
											return;
										}
										for(u8 i=0;i<(ALL_PORT_NUM);i++){
											if(ports[i] == 1){
												set_mirror_port(i,2);
											}
										}
										sprintf(temp,"%s type tx",ports_print(ports));
										shell_output(s,"Command: config mirroring ports ",temp);
										shell_output(s,"Success.","");
										settings_add2queue(SQ_MIRROR);
										return;
									}
									else{
										ptr6 = findcmd(s,ptr5,"both");
										if(ptr6 != NULL){
											if(autocompleat(s)){
												return;
											}
											for(u8 i=0;i<(ALL_PORT_NUM);i++){
												if(ports[i] == 1){
													set_mirror_port(i,3);
												}
											}
											sprintf(temp,"%s type both",ports_print(ports));
											shell_output(s,"Command: config mirroring ports ",temp);
											shell_output(s,"Success.","");
											settings_add2queue(SQ_MIRROR);
											return;
										}
										else{
											shell_output(s,"Next possible completions:","");
											shell_output(s,"normal","");
											shell_output(s,"rx","");
											shell_output(s,"tx","");
											shell_output(s,"both","");
											return;
										}
									}
								}
							}
						}
						else{
							shell_output(s,"Next possible completions:","");
							shell_output(s,"type","");
							return;
						}


						for(u8 i=0;i<(ALL_PORT_NUM);i++){
							if(ports[i] == 1){
								set_mirror_target_port(i);
								shell_output(s,"Command: config mirroring target ",ports_print(ports));
								shell_output(s,"Success.","");
								settings_add2queue(SQ_MIRROR);
								return;
							}
						}
					}
					else{
						shell_output(s,"ERROR: Incorrect ports list","");
						return;
					}
				}
				else{
					shell_output(s,"Next possible completions:","");
					shell_output(s,"state","");
					shell_output(s,"target","");
					shell_output(s,"ports","");
					return;
				}
        	}
    	}
    }



    //teleport
	ptr1 = findcmd(s,str,"teleport");
	if(ptr1 != NULL){
		if(autocompleat(s)){
		  return;
		}
		ptr2 = findcmd(s,ptr1,"remdev");
		if(ptr2 != NULL){
			if(autocompleat(s)){
				return;
			}

			ptr3 = findcmd(s,ptr2,"add");
			if(ptr3 != NULL){
				if(autocompleat(s)){
				  return;
				}
				if(find_ip(ptr3,&ip) == 1){
					remdev = MAX_REMOTE_TLP;
					for(u8 i=0;i<get_remdev_num();i++){
						get_tlp_remdev_ip(i,&ip2);
						if(uip_ipaddr_cmp(ip,ip2)){
							remdev = i;
						}
					}
					if(remdev == MAX_REMOTE_TLP){
						shell_output(s,"Command: config teleport remdev add ",ptr3);
						shell_output(s,"Success.","");
						remdev = get_remdev_num();
						set_tlp_remdev_valid(remdev,1);
						set_tlp_remdev_ip(remdev,&ip);
						settings_add2queue(SQ_TELEPORT);
						return;
					}
					else{
						shell_output(s,"ERROR: IP address is already used","");
						return;
					}
				}
				else{
					shell_output(s,"ERROR: incorrect IP address","");
					return;
				}
			}
			else{
				ptr3 = findcmd(s,ptr2,"del");
				if(ptr3 != NULL){
					if(autocompleat(s)){
					  return;
					}
					if(find_ip(ptr3,&ip) == 1){
						remdev = MAX_REMOTE_TLP;
						for(u8 i=0;i<get_remdev_num();i++){
							get_tlp_remdev_ip(i,&ip2);
							if(uip_ipaddr_cmp(ip,ip2)){
								remdev = i;
							}
						}
						if(remdev != MAX_REMOTE_TLP && remdev<MAX_REMOTE_TLP){
							shell_output(s,"Command: config teleport remdev del ",ptr3);
							shell_output(s,"Success.","");
							delete_tlp_remdev(remdev);
							settings_add2queue(SQ_TELEPORT);
							return;
						}
						else{
							shell_output(s,"ERROR: IP address not found","");
							return;
						}
					}
					else{
						shell_output(s,"ERROR: incorrect IP address","");
						return;
					}
				}
				else{
					shell_output(s,"Next possible completions:","");
					shell_output(s,"add","");
					shell_output(s,"del","");
					return;
				}
			}
		}
		else{
			ptr2 = findcmd(s,ptr1,"input");
			if(ptr2 != NULL){
				if(autocompleat(s)){
					return;
				}
				ptr3 = findcmd(s,ptr2," ");
				temp_u32 = strtoul(ptr3,&ptr3,10);
				if(temp_u32<=(MAX_INPUT_NUM) && temp_u32){
					ptr4 = findcmd(s,ptr3,"state");
					if(ptr4 != NULL){
						if(autocompleat(s)){
						   return;
						}
						ptr5 = findcmd(s,ptr4,"enable");
						if(ptr5 != NULL){
							if(autocompleat(s)){
							   return;
							}
							set_input_state((u8)(temp_u32-1),1);
							sprintf(temp,"%d state enable",(u8)temp_u32);
							shell_output(s,"Command: config teleport input ",temp);
							shell_output(s,"Success.","");
							settings_add2queue(SQ_TELEPORT);
							return;
						}
						else{
							ptr5 = findcmd(s,ptr4,"disable");
							if(ptr5 != NULL){
								if(autocompleat(s)){
									return;
								}
								set_input_state((u8)(temp_u32-1),0);
								sprintf(temp,"%d state disable",(u8)temp_u32);
								shell_output(s,"Command: config teleport input ",temp);
								shell_output(s,"Success.","");
								settings_add2queue(SQ_TELEPORT);
								return;
							}
							else{
								shell_output(s,"Next possible completions:","");
								shell_output(s,"enable","");
								shell_output(s,"disable","");
								return;
							}
					    }
					}
					else{
						ptr4 = findcmd(s,ptr3,"remdev");
						if(ptr4 != NULL){
							if(autocompleat(s)){
								 return;
							}
							if(find_ip(ptr4,&ip) == 1){
								remdev = MAX_REMOTE_TLP;
								for(u8 i=0;i<get_remdev_num();i++){
									get_tlp_remdev_ip(i,&ip2);
									if(uip_ipaddr_cmp(ip,ip2)){
										remdev = i;
									}
								}
								if(remdev != MAX_REMOTE_TLP && remdev < MAX_REMOTE_TLP){
									sprintf(temp,"Command: config teleport input %d remdev",(u8)temp_u32);
									shell_output(s,temp,ptr3);
									shell_output(s,"Success.","");
									set_input_remdev((u8)temp_u32-1,remdev);
									settings_add2queue(SQ_TELEPORT);
									return;
								}
								else{
									shell_output(s,"ERROR: IP address not found","");
									return;
								}
							}
							else{
								shell_output(s,"ERROR: incorrect IP address","");
								return;
							}
						}
						else{
							ptr4 = findcmd(s,ptr3,"remport");
							if(ptr4 != NULL){
								if(autocompleat(s)){
									 return;
								}
								remport = strtoul(ptr4,&ptr4,10);
								if(remport< MAX_OUTPUT_NUM && remport){
									sprintf(temp,"Command: config teleport input %d remport %d",(u8)temp_u32,remport);
									shell_output(s,temp,"");
									shell_output(s,"Success.","");
									set_input_remport((u8)temp_u32-1,remport-1);
									settings_add2queue(SQ_TELEPORT);
									return;
								}
								else{
									shell_output(s,"ERROR: incorrect output num <1-9>","");
									return;
								}
							}
							else{
								ptr4 = findcmd(s,ptr4,"inverse");
								if(ptr4 != NULL){
									if(autocompleat(s)){
										return;
									}
									ptr5 = findcmd(s,ptr4,"enable");
									if(ptr5 != NULL){
										if(autocompleat(s)){
										   return;
										}
										set_input_inverse((u8)temp_u32-1,ENABLE);
										sprintf(temp,"%d inverse enable",(u8)temp_u32);
										shell_output(s,"Command: config teleport input ",temp);
										shell_output(s,"Success.","");
										settings_add2queue(SQ_TELEPORT);
										return;
									}
									else{
										ptr5 = findcmd(s,ptr4,"disable");
										if(ptr5 != NULL){
										    if(autocompleat(s)){
											    return;
										    }
										    set_input_inverse((u8)temp_u32-1,DISABLE);
											sprintf(temp,"%d inverse disable",(u8)temp_u32);
											shell_output(s,"Command: config teleport input ",temp);
											shell_output(s,"Success.","");
											settings_add2queue(SQ_TELEPORT);
											return;
										}
										else{
										   shell_output(s,"Next possible completions:","");
										   shell_output(s,"enable","");
										   shell_output(s,"disable","");
										   return;
										}
									}
								 }
								 else{
									  shell_output(s,"Next possible completions:","");
									  shell_output(s,"state","");
									  shell_output(s,"remdev","");
									  shell_output(s,"remport","");
									  shell_output(s,"inverse","");
									  return;
								 }
							  }
						  }
					  }
				   }
				   else{
					   shell_output(s,"ERROR: Input number <1-3>","");
					   return;
				   }
			}
			else{
				ptr2 = findcmd(s,ptr1,"event");
				if(ptr2 != NULL){
					if(autocompleat(s)){
						return;
					}
					ptr3 = findcmd(s,ptr2," ");
					temp_u32 = strtoul(ptr3,&ptr3,10);
					if(temp_u32<=(MAX_TLP_EVENTS_NUM) && temp_u32){
						ptr4 = findcmd(s,ptr2,"state");
						if(ptr4 != NULL){
							if(autocompleat(s)){
							   return;
							}
							ptr5 = findcmd(s,ptr4,"enable");
							if(ptr5 != NULL){
								if(autocompleat(s)){
								   return;
								}
								set_tlp_event_state((u8)(temp_u32-1),1);
								sprintf(temp,"%d state enable",(u8)temp_u32);
								shell_output(s,"Command: config teleport event ",temp);
								shell_output(s,"Success.","");
								settings_add2queue(SQ_TELEPORT);
								return;
							}
							else{
								ptr5 = findcmd(s,ptr4,"disable");
								if(ptr5 != NULL){
									if(autocompleat(s)){
										return;
									}
									set_tlp_event_state((u8)(temp_u32-1),0);
									sprintf(temp,"%d state disable",(u8)temp_u32);
									shell_output(s,"Command: config teleport event ",temp);
									shell_output(s,"Success.","");
									settings_add2queue(SQ_TELEPORT);
									return;
								}
								else{
									shell_output(s,"Next possible completions:","");
									shell_output(s,"enable","");
									shell_output(s,"disable","");
									return;
								}
						    }
						}
						else{
							ptr4 = findcmd(s,ptr4,"remdev");
							if(ptr4 != NULL){
								if(autocompleat(s)){
									 return;
								}
								if(find_ip(ptr4,&ip) == 1){
									remdev = MAX_REMOTE_TLP;
									for(u8 i=0;i<get_remdev_num();i++){
										get_tlp_remdev_ip(i,&ip2);
										if(uip_ipaddr_cmp(ip,ip2)){
											remdev = i;
										}
									}
									if(remdev != MAX_REMOTE_TLP && remdev < MAX_REMOTE_TLP){
										sprintf(temp,"Command: config teleport event %d remdev",(u8)temp_u32);
										shell_output(s,temp,ptr4);
										shell_output(s,"Success.","");
										set_tlp_event_remdev((u8)temp_u32-1,remdev);
										settings_add2queue(SQ_TELEPORT);
										return;
									}
									else{
										shell_output(s,"ERROR: IP address not found","");
										return;
									}
								}
								else{
									shell_output(s,"ERROR: incorrect IP address","");
									return;
								}
							}
							else{
								ptr4 = findcmd(s,ptr3,"remport");
								if(ptr4 != NULL){
									if(autocompleat(s)){
										 return;
									}
									remport = strtoul(ptr4,&ptr4,10);
									if(remport< MAX_OUTPUT_NUM && remport){
										sprintf(temp,"Command: config teleport event %d remport %d",(u8)temp_u32,remport);
										shell_output(s,temp,"");
										shell_output(s,"Success.","");
										set_tlp_event_remport((u8)temp_u32-1,remport-1);
										settings_add2queue(SQ_TELEPORT);
										return;
									}
									else{
										shell_output(s,"ERROR: incorrect output num <1-9>","");
										return;
									}
								}
								else{
									ptr4 = findcmd(s,ptr4,"inverse");
									if(ptr4 != NULL){
										if(autocompleat(s)){
											return;
										}
										ptr5 = findcmd(s,ptr4,"enable");
										if(ptr5 != NULL){
											if(autocompleat(s)){
											   return;
											}
											set_tlp_event_inverse((u8)temp_u32-1,ENABLE);
											sprintf(temp,"%d inverse enable",(u8)temp_u32);
											shell_output(s,"Command: config teleport event ",temp);
											shell_output(s,"Success.","");
											settings_add2queue(SQ_TELEPORT);
											return;
										}
										else{
											ptr5 = findcmd(s,ptr4,"disable");
											if(ptr5 != NULL){
											    if(autocompleat(s)){
												    return;
											    }
											    set_tlp_event_inverse((u8)temp_u32-1,DISABLE);
												sprintf(temp,"%d inverse disable",(u8)temp_u32);
												shell_output(s,"Command: config teleport event ",temp);
												shell_output(s,"Success.","");
												settings_add2queue(SQ_TELEPORT);
												return;
											}
											else{
											   shell_output(s,"Next possible completions:","");
											   shell_output(s,"enable","");
											   shell_output(s,"disable","");
											   return;
											}
										}
									 }
									 else{
										  shell_output(s,"Next possible completions:","");
										  shell_output(s,"state","");
										  shell_output(s,"remdev","");
										  shell_output(s,"remport","");
										  shell_output(s,"inverse","");
										  return;
									 }
								  }
							  }
						  }
					   }
					   else{
						   shell_output(s,"ERROR: Event number: 1 - UPS events, 2 - Autorestart events","");
						   return;
					   }



				}
				else{
					shell_output(s,"Next possible completions:","");
					shell_output(s,"remdev","");
					shell_output(s,"input","");
					shell_output(s,"event","");
					return;
				}
			}
		}

	}


	//todo доделать обработчик команд настройки LLDP
	//lldp
	ptr1 = findcmd(s,str,"lldp");
	if(ptr1 != NULL){
		if(autocompleat(s)){
		   return;
		}
		ptr2 = findcmd(s,ptr1,"state");
		if(ptr2 != NULL){
			if(autocompleat(s)){
				return;
			}
			ptr3 = findcmd(s,ptr2,"enable");
			if(ptr3 != NULL){
				if(autocompleat(s)){
				   return;
				}
				shell_output(s,"Command: config lldp state enable","");
				shell_output(s,"Success.","");
				set_lldp_state(ENABLE);
				settings_add2queue(SQ_LLDP);
				return;
			}else{
				ptr3 = findcmd(s,ptr2,"disable");
				if(ptr3 != NULL){
					if(autocompleat(s)){
					   return;
					}
					shell_output(s,"Command: config lldp state disable","");
					shell_output(s,"Success.","");
					set_lldp_state(DISABLE);
					settings_add2queue(SQ_LLDP);
					return;
				}
				else{
					shell_output(s,"Next possible completions:","");
				    shell_output(s,"enable","");
				    shell_output(s,"disable","");
				    return;
				}
			}
		}
		else{
			ptr2 = findcmd(s,ptr1,"transmit_interval");
			if(ptr2 != NULL){
				if(autocompleat(s)){
				   return;
				}
				temp_u32 = strtoul(ptr2,&ptr2,10);
				if(temp_u32>=MIN_LLDP_TI && temp_u32<=MAX_LLDP_TI){
					sprintf(temp,"Command: config lldp transmit_interval %d",(u8)temp_u32);
					shell_output(s,temp,"");
					shell_output(s,"Success.","");
					set_lldp_transmit_interval((u8)temp_u32);
					settings_add2queue(SQ_LLDP);
					return;
				}
				else{
					shell_output(s,"ERROR: incorrect transmit interval <5-120>","");
					return;
				}
			}
			else{
				ptr2 = findcmd(s,ptr1,"hold_multiplier");
				if(ptr2 != NULL){
					if(autocompleat(s)){
					   return;
					}
					temp_u32 = strtoul(ptr2,&ptr2,10);
					if(temp_u32>=MIN_LLDP_HM && temp_u32<=MAX_LLDP_HM){
						sprintf(temp,"Command: config lldp hold_multiplier %d",(u8)temp_u32);
						shell_output(s,temp,"");
						shell_output(s,"Success.","");
						set_lldp_hold_multiplier((u8)temp_u32);
						settings_add2queue(SQ_LLDP);
						return;
					}
					else{
						shell_output(s,"ERROR: incorrect hold multiplier <1-10>","");
						return;
					}
				}
				else{
					ptr2 = findcmd(s,ptr1,"port");
					if(ptr2 != NULL){
						if(autocompleat(s)){
						   return;
						}
						ptr3 = find_port(ptr2,ports);
						if(ptr3 == NULL){
							shell_output(s,"ERROR: Incorrect port list","");
							return;
						}
						else{
							ptr3 = findcmd(s,ptr3," ");

							ptr4 = findcmd(s,ptr3,"state");
							if(ptr4 != NULL){
								if(autocompleat(s)){
								   return;
								}
								ptr5 = findcmd(s,ptr4,"enable");
								if(ptr5 != NULL){
									if(autocompleat(s)){
									   return;
									}
									port_in = 0;
									for(u8 i=0;i<ALL_PORT_NUM;i++){
										if(ports[i]==1){
											if(port_in==0)
												port_in=i+1;
											port_out=i+1;
											set_lldp_port_state(i,DISABLE);
										}
									}
									sprintf(temp,"Command: config lldp port %d-%d state enable",port_in,port_out);
									shell_output(s,temp,"");
									shell_output(s,"Success.","");
									settings_add2queue(SQ_LLDP);
									return;
								}
								else{
									ptr5 = findcmd(s,ptr4,"disable");
									if(ptr5 != NULL){
										if(autocompleat(s)){
										   return;
										}
										port_in = 0;
										for(u8 i=0;i<ALL_PORT_NUM;i++){
											if(ports[i]==1){
												if(port_in==0)
													port_in=i+1;
												port_out=i+1;
												set_lldp_port_state(i,DISABLE);
											}
										}
										sprintf(temp,"Command: config lldp port %d-%d state disable",port_in,port_out);
										shell_output(s,temp,"");
										shell_output(s,"Success.","");
										settings_add2queue(SQ_LLDP);
										return;
									}
									else{
										shell_output(s,"Next possible completions:","");
										shell_output(s,"enable","");
										shell_output(s,"disable","");
										return;
									}
								}
							}
							else{
								shell_output(s,"Next possible completions:","");
								shell_output(s,"state","");
								return;
							}
						}

					}
					else{
						shell_output(s,"Next possible completions:","");
						shell_output(s,"state","");
						shell_output(s,"transmit_interval","");
						shell_output(s,"hold_multiplier","");
						shell_output(s,"port","");
						return;
					}
				}
			}
		}
	}


	if(get_telnet_rn() == TELNET_RN)
		shell_output(s,"\r\nCommand: config","");
	else
		shell_output(s,"\rCommand: config","");
	shell_output(s,"","");
	if(get_dev_type() == DEV_SWU16){
		shell_output(s,"ipif\t\t\tports\t\t\tigmp_snooping","");
		shell_output(s,"stp\t\t\tsnmp\t\t\tsyslog","");
		shell_output(s,"vlan\t\t\tsntp\t\t\tsmtp","");
		shell_output(s,"users_account\t\ttftp\t\t\tevents", "");
		shell_output(s,"802.1p\t\t\tscheduling_mechanism\tdscp_mapping", "");
		shell_output(s,"bandwidth_control\tdescription\t\tmac_filtering","");
		shell_output(s,"aggregation\t\tmirroring","");
	}
	else{
		shell_output(s,"ipif\t\t\tports\t\t\tigmp_snooping","");
		shell_output(s,"stp\t\t\tsnmp\t\t\tsyslog","");
		shell_output(s,"vlan\t\t\tsntp\t\t\tsmtp","");
		shell_output(s,"autorestart\t\tcomfortstart\t\tdry_cont", "");
		shell_output(s,"users_account\t\ttftp\t\t\tevents", "");
		shell_output(s,"802.1p\t\t\tscheduling_mechanism\tdscp_mapping", "");
		shell_output(s,"bandwidth_control\tdescription\t\tmac_filtering","");
		shell_output(s,"inputs\t\t\toutputs\t\t\trs485","");
		shell_output(s,"teleport\t\tlldp\t\t","");
	}

	return;
}

static u8 checkandcopy_filename(char *inptr,char *outptr){
	//DEBUG_MSG(TELNET_DEBUG && !s->usb,"checkandcopy_filename: %s\r\n",inptr);
	if(strlen(inptr)!=0 && strlen(inptr)<64){
		strcpy(outptr,inptr);
		for(u8 i=0;i<strlen(outptr);i++){
			if(isalnum((int)(outptr[i])) || outptr[i]==' '|| outptr[i]=='_' || outptr[i]=='/' || outptr[i]=='.'|| outptr[i]=='-'){

			}
			else
				return 1;
		}
		return 0;
	}
	return 1;
}

static void tftp_start_updating(char *FileName,uip_ipaddr_t *ip){
	send_rrq(FileName,ip);
	tftp_proc.start = 1;
	tftp_proc.opcode = TFTP_DOWNLOADING;
	tftp_proc.wait_time = 0;
	nosave();
}

static void tftp_start_backup_dl(char *FileName,uip_ipaddr_t *ip){
	send_rrq(FileName,ip);
	tftp_proc.start = 1;
	tftp_proc.opcode = TFTP_DOWNLOADING;
	tftp_proc.wait_time = 0;
	tosave();
}

static void tftp_start_backup_ul(char *FileName,uip_ipaddr_t *ip){
u32 len;
	len = make_bak();
	send_wrq(FileName,ip,len);
	tftp_proc.start = 1;
	tftp_proc.opcode = TFTP_UPLOADING;
	tftp_proc.wait_time = 0;
}

static void tftp_start_log_ul(char *FileName,uip_ipaddr_t *ip){
u32 len;
	len = make_log();
	send_wrq(FileName,ip,len);
	tftp_proc.start = 1;
	tftp_proc.opcode = TFTP_UPLOADING;
	tftp_proc.wait_time = 0;

}



//---------------------------------------------------------------------------

static void download(struct telnetd_state *s,char *str){
uip_ipaddr_t ip;
char FileName[64];
char *ptr1,*ptr2,*ptr3;
	  //if empty line

if(s->user_rule!=ADMIN_RULE){
	shell_output(s,"ERROR: Access denied","");
	return;
}

	  if (!strlen(str))
	  {
		  shell_output(s,"Command: download","");
		  shell_output(s,"","");
		  shell_output(s,"firmware_fromTFTP\t\t\tcfg_fromTFTP", "");
		  return ;
	  }

	  //download FW from TFTP server
	  ptr1 = findcmd(s,str,"firmware_fromTFTP");
	  if(ptr1 !=NULL){
		 if(autocompleat(s)){
			 return;
		 }
		 ptr2 = find_ip_ptr(ptr1,&ip);
		 if(ptr2 !=NULL){
			 ptr3 = findcmd(s,ptr2," ");
			 if(ptr3!=NULL){
				 if(get_tftp_state() == ENABLE){
					 if(checkandcopy_filename(ptr3,FileName)==0){
						 tftp_start_updating(FileName,&ip);
						 shell_output(s,"Download file:",FileName);
						 return;
					 }
					 else{
						 shell_output(s,"ERROR: Incorrect filename","");
						 return;
					 }
				 }
				 else{
					 shell_output(s,"ERROR: Impossible, TFTP is disabled","");
					 return;
				 }
			 }
			 else{
				 shell_output(s,"ERROR: Command","");
				 shell_output(s,"Next possible completions:","");
				 shell_output(s,"<path_filename 64>","");
				 return;
			 }
		 }
		 else{
			 shell_output(s,"ERROR: incorrect IP address","");
 	  		 return;
		 }
	  }
	  else{
		  ptr1 = findcmd(s,str,"cfg_fromTFTP");
		  if(ptr1 !=NULL){
			 if(autocompleat(s)){
				return;
			 }
			 ptr2 = find_ip_ptr(ptr1,&ip);
			 if(ptr2 !=NULL){
				 ptr3 = findcmd(s,ptr2," ");
				 if(ptr3!=NULL){
					 if(get_tftp_state() == ENABLE){
						 if(checkandcopy_filename(ptr3,FileName)==0){
							 tftp_start_backup_dl(FileName,&ip);
							 shell_output(s,"Download file:",FileName);
							 return;
						 }
						 else{
							 shell_output(s,"ERROR: Incorrect filename","");
							 return;
						 }
					 }
					 else{
						 shell_output(s,"ERROR: Impossible, TFTP is disabled","");
						 return;
					 }
				 }
				 else{
					 shell_output(s,"ERROR: Command","");
					 shell_output(s,"Next possible completions:","");
					 shell_output(s,"<path_filename 64>","");
					 return;
				 }
			 }
			 else{
				 shell_output(s,"ERROR: incorrect IP address","");
	 	  		 return;
			 }
		  }
		  else{
			  shell_output(s,"ERROR: Command","");
	  		  shell_output(s,"Next possible completions:","");
			  shell_output(s,"firmware_fromTFTP","");
			  shell_output(s,"cfg_fromTFTP","");
			  return;
		  }
	  }
}

static void upload(struct telnetd_state *s,char *str){
uip_ipaddr_t ip;
char FileName[64];
char *ptr1,*ptr2,*ptr3;

if(s->user_rule!=ADMIN_RULE){
	shell_output(s,"ERROR: Access denied","");
	return;
}

	  //if empty line
	  if (!strlen(str))
	  {
		  shell_output(s,"Command: upload","");
		  shell_output(s,"","");
		  shell_output(s,"cfg_toTFTP\t\t\tlog_toTFTP", "");
		  return ;
	  }

	  //upload config to TFTP server
	  ptr1 = findcmd(s,str,"cfg_toTFTP");
	  if(ptr1 !=NULL){
		 if(autocompleat(s)){
			 return;
		 }
		 ptr2 = find_ip_ptr(ptr1,&ip);
		 if(ptr2 !=NULL){
			 ptr3 = findcmd(s,ptr2," ");
			 if(ptr3!=NULL){
				 if(get_tftp_state() == ENABLE){
					 if(checkandcopy_filename(ptr3,FileName)==0){
						 tftp_start_backup_ul(FileName,&ip);
						 shell_output(s,"Upload file:",FileName);
						 return;
					 }
					 else{
						 shell_output(s,"ERROR: Incorrect filename","");
						 return;
					 }
				 }
				 else{
					 shell_output(s,"ERROR: Impossible, TFTP is disabled","");
					 return;
				 }
			 }
			 else{
				 shell_output(s,"ERROR: Command","");
				 shell_output(s,"Next possible completions:","");
				 shell_output(s,"<path_filename 64>","");
				 return;
			 }
		 }
		 else{
			 shell_output(s,"ERROR: incorrect IP address","");
			 return;
		 }
	  }
	  else{
		  //upload log to TFTP server
		  ptr1 = findcmd(s,str,"log_toTFTP");
		  if(ptr1 !=NULL){
			 if(autocompleat(s)){
				 return;
			 }
			 ptr2 = find_ip_ptr(ptr1,&ip);
			 if(ptr2 !=NULL){
				 ptr3 = findcmd(s,ptr2," ");
				 if(ptr3!=NULL){
					 if(get_tftp_state() == ENABLE){
						 if(checkandcopy_filename(ptr3,FileName)==0){
							 tftp_start_log_ul(FileName,&ip);
							 shell_output(s,"Upload file:",FileName);
							 return;
						 }
						 else{
							 shell_output(s,"ERROR: Incorrect filename","");
							 return;
						 }
					 }
					 else{
						 shell_output(s,"ERROR: Impossible, TFTP is disabled","");
						 return;
					 }
				 }
				 else{
					 shell_output(s,"ERROR: Command","");
					 shell_output(s,"Next possible completions:","");
					 shell_output(s,"<path_filename 64>","");
					 return;
				 }
			 }
			 else{
				 shell_output(s,"ERROR: incorrect IP address","");
				 return;
			 }
		  }
		  else{
			  shell_output(s,"ERROR: Command","");
			  shell_output(s,"Next possible completions:","");
			  shell_output(s,"cfg_toTFTP","");
			  shell_output(s,"log_toTFTP","");
			  return;
		  }
	  }
}

static void ping_start(uip_ipaddr_t IP){
	if(OtherCommandsFlag==0){
	  uip_ipaddr_copy(IPDestPing,IP);
	  OtherCommandsFlag=PING_PROCESSING;
	  SendICMPPingFlag = 0;
	  vTaskResume(xOtherCommands);
	  start_ping_flag = 1;
	  send_ping_flag = 0;
	  timer_set(&ping_timer, 10000*MSEC);
	}
}

static void ping(struct telnetd_state *s,char *str){
uip_ipaddr_t ip;
char *ptr;
char temp[64];

	ptr = findcmd(s,str," ");
	if(ptr != NULL){
		if(find_ip(ptr,&ip) == 1){
			sprintf(temp,"Ping %d.%d.%d.%d with 32 bytes of data",
		  		  uip_ipaddr1(&ip),uip_ipaddr2(&ip),uip_ipaddr3(&ip),uip_ipaddr4(&ip));
		  	shell_output(s,temp,"");
		  	ping_start(ip);
		  	return;
		}
		else{
			shell_output(s,"ERROR: incorrect IP address","");
			return;
		}
	}
	else{
		shell_output(s,"ERROR: incorrect IP address","");
		return;
	}

}

static void start_cable_test(u8 port){
	set_cable_test(VCT_TEST,port,0);
	start_vct = 1;
	port_vct = port;
}

static void cable_diag(struct telnetd_state *s,char *str){
char *ptr1,*ptr2,*ptr3;
u8 ports[PORT_NUM];
char temp[64];

	ptr1 = findcmd(s,str,"ports");
	if(ptr1!=NULL){
		if(autocompleat(s)){
			 return;
		}
		ptr2 = findcmd(s,ptr1," ");
		ptr3 = find_port(ptr2,ports);
		if(ptr3 == NULL){
			shell_output(s,"ERROR: Incorrect port list","");
			return;
		}
		else{
			for(u8 i = 0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
				if(i<COOPER_PORT_NUM){
					if(ports[i]==1){
						sprintf(temp,"%d",i+1);
						shell_output(s,"Start cable diagnostic on port ",temp);
						start_cable_test(i);
						return;
					}
				}else{
					shell_output(s,"ERROR: Avalible for FastEthernet ports only","");
					return;
				}
			}
		}
	}
	else{
		shell_output(s,"ERROR: Command","");
		shell_output(s,"Next possible completions:","");
		shell_output(s,"ports","");
		return;
	}
}

//---------------------------------------------------------------------------
static struct ptentry parsetab_root[] =
  {
   {"config",config},
   {"show",show},
   {"download",download},
   {"upload",upload},
   {"ping",ping},
   {"cable_diag",cable_diag},
   {"help", help},
   {"exit", shell_quit},
   {"save",save_sett},
   {"reboot",reboot_conn},
   {"default",default_sett},
   {"?", help},
   {NULL, unknown}
  };
/*---------------------------------------------------------------------------*/
void
shell_init(struct telnetd_state *s)
{
	memset(SHELL_PROMPT,0,sizeof(SHELL_PROMPT));
	timer_set(&telnet_timeout,0);
	DEBUG_MSG(TELNET_DEBUG && !s->usb,"shell_init()\r\n");
	s->entered_pass = 0;
	s->entered_name = 0;
	s->active = 0;
	s->rport = 0;
	s->usb = 0;
	s->telnetd_comm_mem.linenum = 0;
	s->user_rule = NO_RULE;
	for(u8 i=0;i<TELNETD_MEM_SIZE;i++)
		memset(s->telnetd_comm_mem.line[i],0,TELNETD_CONF_LINELEN);
	s->bufptr=0;
	if(get_telnet_echo())
		s->echo_flag = 1;
	else
		s->echo_flag = 0;
	memset(s->buf,0,TELNETD_CONF_LINELEN);
	get_dev_name_r(SHELL_PROMPT);
}

/*---------------------------------------------------------------------------*/
void
shell_start(struct telnetd_state *s)
{
	  //shell start
	  DEBUG_MSG(TELNET_DEBUG && !s->usb,"shell_start()\r\n");
	  shell_output(s,"                      TFortis - Industrial Switch","");
	  //shell_output(s,"                          Command Line Interface","");
	  shell_output(s,"          Copyright(C) 2018 \"Fort-Telecom\" Ltd. All rights reserved.","");
	  //shell_output(s,"Type '?' or \"help\" and return for help", "");

	  //показываем ввод пароля
	  shell_prompt(s,SHELL_USERNAME);
}
/*---------------------------------------------------------------------------*/
void
shell_input(struct telnetd_state *s)
{
char username[64];

  if((incorrect_autc_cnt>TELNET_MAX_AUTH_FAIL)&&(timer_expired(&telnet_timeout))){
	  timer_set(&telnet_timeout, TELNET_FAIL_TIMEOUT*MSEC*1000);
	  incorrect_autc_cnt = 0;
  }

  if((timer_expired(&telnet_timeout))&&(incorrect_autc_cnt<=TELNET_MAX_AUTH_FAIL)){

	  DEBUG_MSG(TELNET_DEBUG && !s->usb,"shell_input() %s\r\n",s->buf);
	  if(s->entered_name == 0){
		  parse_name(s,s->buf);
		  s->entered_name = 1;
		  shell_prompt(s,SHELL_PASSWORD);
		  return;
	  }
	  else if(s->entered_pass == 0){
		  if(parse_name(s,s->username)==0){
			  incorrect_autc_cnt++;
			  s->entered_name = 0;
			  if(get_telnet_rn() == TELNET_RN)
				  shell_prompt(s,"ERROR: Incorrect Username or Password\r\n");
			  else
				  shell_prompt(s,"ERROR: Incorrect Username or Password\r");
			  shell_prompt(s,SHELL_USERNAME);
			  return;
		  }

		  if(parse_pass(s,s->buf)){
			  s->entered_pass = 1;
			  incorrect_autc_cnt = 0;
			  //set user rule - default
			  s->user_rule = get_interface_users_rule(0);
			  //set user rule
			  for(u8 i=0;i<MAX_USERS_NUM;i++){
				  if((get_interface_users_rule(i)==ADMIN_RULE)||(get_interface_users_rule(i)==USER_RULE)){
					  get_interface_users_username(i,username);
					  if(strcmp(username,s->username) == 0){
						  s->user_rule = get_interface_users_rule(i);
						  break;
					  }
				  }
			  }

			  //shell_prompt(s,SHELL_PROMPT);
			  promt_print(s);
		  }
		  else{
			  s->entered_name = 0;
			  s->entered_pass = 0;
			  incorrect_autc_cnt++;
			  if(get_telnet_rn() == TELNET_RN)
				  shell_prompt(s,"ERROR: Incorrect Username or Password\r\n");
			  else
				  shell_prompt(s,"ERROR: Incorrect Username or Password\r");
			  shell_prompt(s,SHELL_USERNAME);
		  }
	  }
	  else{
		  parse_short(s,s->buf, parsetab_root);
		  if(s->tab){
			  //s->tab = 0;
		  }
		  else if(start_vct || start_make_bak || start_ping_flag || start_show_fdb || start_show_vlan){
			  //
		  }
		  else
			  promt_print(s);
	  }
  }
}
/*---------------------------------------------------------------------------*/

static u8 parse_pass(struct telnetd_state *s,char *cmd){
char pass[64];
	get_interface_users_password(0,pass);
	if(pass[0]==0)
		return 1;

	if(strlen(cmd)>2){
		for(u8 i=0;i<MAX_USERS_NUM;i++){
			get_interface_users_password(i,pass);
			if(strncmp(pass,cmd,strlen(pass))==0){
				return 1;
			}
		}
		return 0;
	}
	else{
		return 0;
	}
}

static u8 parse_name(struct telnetd_state *s,char *cmd){
	char name[64];
	get_interface_users_username(0,name);
	if(name[0]==0)
		return 1;

	if(strlen(cmd)>2){
		for(u8 i=0;i<MAX_USERS_NUM;i++){
			get_interface_users_username(i,name);
			if(strncmp(name,cmd,strlen(name))==0){
				strcpy(s->username,cmd);
				return 1;
			}
		}
		strcpy(s->username,cmd);
		return 0;
	}
	else{
		strcpy(s->username,cmd);
		return 0;
	}
}

void show_fdb(struct telnetd_state *s){
static u8 start=1;
static u32 i=0,k=0;
struct mac_entry_t entry;
char temp[64];


	if(get_dev_type() == DEV_SWU16){
		if(k<SALSA2_FDB_MAX){
			//for(u32 k=0;k<SALSA2_FDB_MAX;k++){
			if(get_salsa2_fdb_entry(k,&entry)==1){
				 sprintf(temp,"%d\t%02X:%02X:%02X:%02X:%02X:%02X\t%lu\r\n",i+1,(uint8_t)entry.mac[0],(uint8_t)entry.mac[1],
						 (uint8_t)entry.mac[2],(uint8_t)entry.mac[3],
						 (uint8_t)entry.mac[4],(uint8_t)entry.mac[5],
						 entry.port_vect+1);
				 shell_prompt(s,temp);
				 i++;
			}
			k++;
		}
		else{
			shell_output(s," ","");
			sprintf(temp,"%d",i);
			shell_output(s,"Total Entries: ",temp);
			shell_prompt(s,"\r\n");
			shell_prompt(s,SHELL_PROMPT);
			start_show_fdb = 0;
			k=0;
			i=0;
		}

	}
	else{
		if(read_atu(0, start, &entry)==0){
		//while(read_atu(0, start, &entry)==0){
			 sprintf(temp,"%d\t%02X:%02X:%02X:%02X:%02X:%02X\t",i+1,(uint8_t)entry.mac[1],(uint8_t)entry.mac[0],
					 (uint8_t)entry.mac[3],(uint8_t)entry.mac[2],
					 (uint8_t)entry.mac[5],(uint8_t)entry.mac[4]);
			 shell_prompt(s,temp);
			 if(get_dev_type() == DEV_PSW2GPLUS){
					 if(entry.port_vect & 1)
						 shell_prompt(s,"FE#1 ");
					 if(entry.port_vect & 4)
						 shell_prompt(s,"FE#2 ");
					 if(entry.port_vect & 16)
						 shell_prompt(s,"FE#3 ");
					 if(entry.port_vect & 64)
						 shell_prompt(s,"FE#4 ");
					 if(entry.port_vect & 256)
						 shell_prompt(s,"GE#1 ");
					 if(entry.port_vect & 512)
						 shell_prompt(s,"GE#2 ");
					 if(entry.port_vect & 1024)
						 shell_prompt(s,"CPU ");
			} else if((get_dev_type() == DEV_PSW2G4F)||(get_dev_type() == DEV_PSW2G4FUPS)){
					 if(entry.port_vect & 1)
						 shell_prompt(s,"1 ");
					 if(entry.port_vect & 4)
						 shell_prompt(s,"2 ");
					 if(entry.port_vect & 16)
						 shell_prompt(s,"3 ");
					 if(entry.port_vect & 64)
						 shell_prompt(s,"4 ");
					 if(entry.port_vect & 256)
						 shell_prompt(s,"5 ");
					 if(entry.port_vect & 512)
						 shell_prompt(s,"6 ");
					 if(entry.port_vect & 1024)
						 shell_prompt(s,"CPU ");
			}else if(get_dev_type() == DEV_PSW2G6F){
					 if(entry.port_vect & 1)
						 shell_prompt(s,"1 ");
					 if(entry.port_vect & 4)
						 shell_prompt(s,"2 ");
					 if(entry.port_vect & 16)
						 shell_prompt(s,"3 ");
					 if(entry.port_vect & 32)
						 shell_prompt(s,"4 ");
					 if(entry.port_vect & 64)
						 shell_prompt(s,"5 ");
					 if(entry.port_vect & 128)
						 shell_prompt(s,"6 ");
					 if(entry.port_vect & 256)
						 shell_prompt(s,"7 ");
					 if(entry.port_vect & 512)
						 shell_prompt(s,"8 ");
					 if(entry.port_vect & 1024)
						 shell_prompt(s,"CPU ");
			}else if(get_dev_type() == DEV_PSW2G8F){
					 if(entry.port_vect & 1)
						 shell_prompt(s,"1 ");
					 if(entry.port_vect & 2)
						 shell_prompt(s,"2 ");
					 if(entry.port_vect & 4)
						 shell_prompt(s,"3 ");
					 if(entry.port_vect & 8)
						 shell_prompt(s,"4 ");
					 if(entry.port_vect & 16)
						 shell_prompt(s,"5 ");
					 if(entry.port_vect & 32)
						 shell_prompt(s,"6 ");
					 if(entry.port_vect & 64)
						 shell_prompt(s,"7 ");
					 if(entry.port_vect & 128)
						 shell_prompt(s,"8 ");
					 if(entry.port_vect & 256)
						 shell_prompt(s,"9 ");
					 if(entry.port_vect & 512)
						 shell_prompt(s,"10 ");
					 if(entry.port_vect & 1024)
						 shell_prompt(s,"CPU ");
			} else if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
					 if(entry.port_vect & 1)
						 shell_prompt(s,"1 ");
					 if(entry.port_vect & 2)
						 shell_prompt(s,"2 ");
					 if(entry.port_vect & 4)
						 shell_prompt(s,"3 ");
					 if(entry.port_vect & 8)
						 shell_prompt(s,"4 ");
					 if(entry.port_vect & 16)
						 shell_prompt(s,"5 ");
					 if(entry.port_vect & 32)
						 shell_prompt(s,"6 ");
					 if(entry.port_vect & 64)
						 shell_prompt(s,"CPU ");
			} else if((get_dev_type() == DEV_PSW2G2FPLUS)||(get_dev_type() == DEV_PSW2G2FPLUSUPS)){
					 if(entry.port_vect & 16)
						 shell_prompt(s,"1 ");
					 if(entry.port_vect & 64)
						 shell_prompt(s,"2 ");
					 if(entry.port_vect & 256)
						 shell_prompt(s,"3 ");
					 if(entry.port_vect & 512)
						 shell_prompt(s,"4 ");
					 if(entry.port_vect & 1024)
						 shell_prompt(s,"CPU ");
			}

			start = 0;
			shell_prompt(s,"\r\n");
			i++;
		}
		else{
			shell_output(s," ","");
			sprintf(temp,"%d",i);
			shell_output(s,"Total Entries: ",temp);
			shell_prompt(s,"\r\n");
			shell_prompt(s,SHELL_PROMPT);
			start_show_fdb = 0;
			i=0;
			k=0;
			start = 1;
		}
	}
	//shell_output(s," ","");
	//sprintf(temp,"%d",i);
	//shell_output(s,"Total Entries: ",temp);

}


void show_vlan_all(struct telnetd_state *s){
char temp[64];
char temp2[64];
static u16 i=0;

	//for(u8 i=0;i<get_vlan_sett_vlannum();i++){
	if(i<get_vlan_sett_vlannum()){

		sprintf(temp,"%d",(u16)get_vlan_vid(i));
		shell_output(s,"VID:\t\t\t",temp);

		shell_output(s,"VLAN Name:\t\t",get_vlan_name(i));

		shell_output(s,"VLAN Type:\t\t","Static");

		memset(temp,0,sizeof(temp));
		for(u8 j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
			if(get_vlan_port_state(i,j)==3){
				sprintf(temp2,"%d ",j+1);
				strcat(temp,temp2);
			}
		}
		shell_output(s,"Tagged Ports:\t\t",temp);

		memset(temp,0,sizeof(temp));
		for(u8 j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
			if(get_vlan_port_state(i,j)==2){
				sprintf(temp2,"%d ",j+1);
				strcat(temp,temp2);
			}
		}
		shell_output(s,"Untagged Ports:\t\t",temp);

		memset(temp,0,sizeof(temp));
		for(u8 j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
			if(get_vlan_port_state(i,j)==0){
				sprintf(temp2,"%d ",j+1);
				strcat(temp,temp2);
			}
		}
		shell_output(s,"Not a member Ports:\t",temp);
		shell_output(s,"","");
		i++;
	}
	else{
		start_show_vlan = 0;
		i=0;
		shell_prompt(s,"\r\n");
		shell_prompt(s,SHELL_PROMPT);
	}
}

#endif


