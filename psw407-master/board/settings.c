

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
#include "../dhcp/dhcp.h"
#include "settings.h"


static u8 need_load_settings_=0;
static u8 need_save_settings_=0;
static u8 settings_is_loaded_ = 0;


extern struct settings_t settings __attribute__ ((section (".ccmram")));

static int check_str(char *str, u32 maxlen){
	if(strlen(str)>maxlen)
		return -1;
	for(u16 i=0;i<strlen(str);i++){
		if((isalnum(str[i]))||(str[i]=='.')||(str[i]==',')||
				(str[i]==':')||(str[i]=='-')||(str[i]=='/')||(str[i]=='+')||
				(str[i]=='?')||(str[i]=='=')||(str[i]=='&')||(str[i]=='!')||
				(str[i]=='@')||(str[i]=='#')||(str[i]=='$')||(str[i]=='%')||
				(str[i]=='(')||(str[i]==')')||(str[i]=='"')||(str[i]=='<')||(str[i]=='>')){
			//
		}
		else{
			return -1;
		}
	}
	return 0;
}

/**
@mainpage функции для сохренния настроек
*/

/**************************************************************/
/*прослойка для работы с элементами структуры*/

int set_net_ip(uip_ipaddr_t ip){
	if(check_ip_addr(ip)==0){
		if(!(uip_ipaddr_cmp(settings.net_sett.ip,ip))){
			send_events_ip(EVENT_SET_NETIP,ip);
			uip_ipaddr_copy(settings.net_sett.ip,ip);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else{
		return SETT_BAD;
	}
}

int set_net_mask(uip_ipaddr_t mask){
	if(uip_ipaddr4(&mask)!=0){
		if(!(uip_ipaddr_cmp(settings.net_sett.mask,mask))){
			send_events_ip(EVENT_SET_NETMASK,mask);
			uip_ipaddr_copy(settings.net_sett.mask,mask);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else{
		return SETT_BAD;
	}
}


int set_net_gate(uip_ipaddr_t gate){
	if(uip_ipaddr1(gate)!=0){
		if(!(uip_ipaddr_cmp(settings.net_sett.gate,gate))){
			send_events_ip(EVENT_SET_NETGATE,gate);
			uip_ipaddr_copy(settings.net_sett.gate,gate);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else{
		return SETT_BAD;
	}
}

int set_net_dns(uip_ipaddr_t dns){
if(uip_ipaddr1(dns)!=0){
	if(!(uip_ipaddr_cmp(settings.net_sett.dns,dns))){
		uip_ipaddr_copy(settings.net_sett.dns,dns);
		send_events_ip(EVENT_SET_NETDNS,dns);
		return SETT_DIFF;
	}
	else
		return SETT_EQUAL;
}
else{
	return SETT_BAD;
}
}

int set_net_mac(u8 *mac){
	if(mac == NULL)
		return SETT_BAD;
	if((mac[0]!=settings.net_sett.mac[0])||(mac[1]!=settings.net_sett.mac[1])||
			(mac[2]!=settings.net_sett.mac[2])||(mac[3]!=settings.net_sett.mac[3])||
			(mac[4]!=settings.net_sett.mac[4])||(mac[5]!=settings.net_sett.mac[5])){
		send_events_mac(EVENT_SET_MAC,mac);
		for(u8 i=0;i<6;i++){
			settings.net_sett.mac[i] = mac[i];
		}
		return SETT_DIFF;
	}
	else
		return SETT_EQUAL;
}


int set_net_def_mac(u8 *mac){

	if((mac[0]!=settings.net_sett.default_mac[0])||(mac[1]!=settings.net_sett.default_mac[1])||
			(mac[2]!=settings.net_sett.default_mac[2])||(mac[3]!=settings.net_sett.default_mac[3])||
			(mac[4]!=settings.net_sett.default_mac[4])||(mac[5]!=settings.net_sett.default_mac[5])){
		send_events_mac(EVENT_SET_MAC,mac);
		for(u8 i=0;i<6;i++){
			settings.net_sett.default_mac[i] = mac[i];
		}
		return SETT_DIFF;
	}
	else
		return SETT_EQUAL;
}
//dhcp settings
int set_dhcp_mode(u8 mode){
	if(mode<4){
		if(mode != settings.dhcp_sett.mode){
			send_events_u32(EVENT_SET_DHCPMODE,(u32)mode);
			settings.dhcp_sett.mode = mode;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_dhcp_server_addr(uip_ipaddr_t serv){
	if(check_ip_addr(serv)==0){
		if(!(uip_ipaddr_cmp(settings.dhcp_sett.server_addr,serv))){
			uip_ipaddr_copy(settings.dhcp_sett.server_addr,serv);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

//todo добавить генерацию собыия
int set_dhcp_hops(u8 hops){
	if((hops)&&(hops<16)){
		settings.dhcp_sett.hop_limit = hops;
		return 0;
	}
	else
		return -1;
}

//todo добавить генерацию собыия
int set_dhcp_opt82(u8 state){
	if((state==0)||(state)){
		settings.dhcp_sett.opt82 = state;
		return 0;
	}
	else
		return -1;
}


int set_gratuitous_arp_state(u8 state){
	if((state==0)||(state)){
		if(state != settings.net_sett.grat_arp){
			send_events_u32(EVENT_SET_GR_ARP,(u32)state);
			settings.net_sett.grat_arp = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}


// port settings
int set_port_state(u8 port,u8 state){
	if((port<PORT_NUM)&&((state==0)||(state==1))){
		if(state == ENABLE)
			dev.port_stat[port].error_dis = 0;
		if(state != settings.port_sett[port].state){
			send_events_u32(EVENTS_SET_PORT_ST_P1 + port,(u32)state);
			settings.port_sett[port].state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_port_speed_dplx(u8 port,u8 state){
	if((port<ALL_PORT_NUM)&&(state<=SPEED_1000_FULL)){
		if(state != settings.port_sett[port].speed_dplx){
			send_events_u32(EVENTS_SET_PORT_DPLX_P1 + port,(u32)state);
			settings.port_sett[port].speed_dplx = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_port_flow(u8 port,u8 state){
	if((port<ALL_PORT_NUM)&&((state==0)||(state==1))){
		if(state != settings.port_sett[port].flow_ctrl){
			send_events_u32(EVENTS_SET_PORT_FLOW_P1 + port,(u32)state);
			settings.port_sett[port].flow_ctrl = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}


int set_port_wdt(u8 port,u8 state){
	if((port<POE_PORT_NUM)&&(state<=WDT_SPEED)){
		if(state != settings.port_sett[port].wdt){
			send_events_u32(EVENTS_WDT_MODE_P1 + port,(u32)state);
			settings.port_sett[port].wdt = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_port_wdt_ip(u8 port,uip_ipaddr_t ip){
	if((port<POE_PORT_NUM)&&(check_ip_addr(ip)==0)){
		if(!(uip_ipaddr_cmp(settings.port_sett[port].ip_dest,ip))){
			send_events_ip(EVENTS_WDT_IP_P1 + port,ip);
			uip_ipaddr_copy(settings.port_sett[port].ip_dest,ip);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_port_wdt_speed_up(u8 port,u16 speed){
	if((port<POE_PORT_NUM)&&(speed)){
		if(settings.port_sett[port].wdt_speed_up != speed){
			send_events_u32(EVENTS_WDT_SPEED_UP_P1 + port,(u32)speed);
			settings.port_sett[port].wdt_speed_up = speed;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_port_wdt_speed_down(u8 port,u16 speed){
	if((port<POE_PORT_NUM)&&(speed)){
		if(settings.port_sett[port].wdt_speed_down != speed){
			send_events_u32(EVENTS_WDT_SPEED_DOWN_P1 + port,(u32)speed);
			settings.port_sett[port].wdt_speed_down = speed;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_port_soft_start(u8 port, u8 state){
	if((port<POE_PORT_NUM)&&((state == 0)||(state == 1))){
		if(state != settings.port_sett[port].soft_start){
			send_events_u32(EVENTS_SET_PORT_SS_P1 + port,(u32)state);
			settings.port_sett[port].soft_start = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_port_poe(u8 port,u8 state){
	if((port<POE_PORT_NUM)&&(state <7)){
		if(state != settings.port_sett[port].poe_a_set){
			send_events_u32(EVENTS_SET_PORT_POE_P1 + port,(u32)state);
			settings.port_sett[port].poe_a_set = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}


int set_port_pwr_lim_a(u8 port, u8 limit){
	if((port<POE_PORT_NUM)&&(limit<=POE_MAX_PWR)){
		if(settings.port_sett[port].poe_a_pwr_lim != limit){
			send_events_u32(EVENTS_SET_PORT_POE_A_P1_LIM + port,(u32)limit);
			settings.port_sett[port].poe_a_pwr_lim = limit;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_port_pwr_lim_b(u8 port, u8 limit){
	if((port<POE_PORT_NUM)&&(limit<=POE_MAX_PWR)){
		if(settings.port_sett[port].poe_b_pwr_lim != limit){
			send_events_u32(EVENTS_SET_PORT_POE_B_P1_LIM + port,(u32)limit);
			settings.port_sett[port].poe_b_pwr_lim = limit;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_port_sfp_mode(u8 port, u8 mode){
	if(is_fiber(port)&&((mode==0)||(mode==1)||(mode==2))){
		if(settings.port_sett[port].sfp_mode!=mode){
			send_events_u32(EVENTS_SFPMODE_P1+port,(u32)mode);
			settings.port_sett[port].sfp_mode = mode;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}


//настройки интерфейса
int set_interface_lang(u8 lang){
	if((lang == RUS)||(lang == ENG)){
		if(settings.interface_sett.lang != lang){
			send_events_u32(EVENTS_SET_LANG,(u32)lang);
			settings.interface_sett.lang = lang;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}else
		return SETT_BAD;
}


int set_interface_users_username(u8 num,char *login){
	if(num==0){
		//default users
		if(strlen(login)<=HTTPD_MAX_LEN_PASSWD){
			if(strcmp(settings.interface_sett.http_login64,login) != 0){
				send_events_u32(EVENTS_SET_LOGIN,0/*login*/);
				strcpy(settings.interface_sett.http_login64,login);
				return SETT_DIFF;
			}
			else
				return SETT_EQUAL;
		}else
			return SETT_BAD;
	}
	else{
		//additional users
		if(strlen(login)<=HTTPD_MAX_LEN_PASSWD){
			if(strcmp(settings.interface_sett.users[num-1].username,login) != 0){
				send_events_u32(EVENTS_SET_LOGIN,0/*login*/);
				strcpy(settings.interface_sett.users[num-1].username,login);
				return SETT_DIFF;
			}
			else
				return SETT_EQUAL;
		}else
			return SETT_BAD;
	}
}


int set_interface_users_password(u8 num,char *paswd){
	if(num==0){
		//default users
		if(strlen(paswd)<=HTTPD_MAX_LEN_PASSWD){
			if(strcmp(settings.interface_sett.http_passwd64,paswd) != 0){
				send_events_u32(EVENTS_SET_PASSWD,0/*paswd*/);
				strcpy(settings.interface_sett.http_passwd64,paswd);
				return SETT_DIFF;
			}
			else
				return SETT_EQUAL;
		}else
			return SETT_BAD;
	}
	else{
		//additional users
		if(strlen(paswd)<=HTTPD_MAX_LEN_PASSWD){
			if(strcmp(settings.interface_sett.users[num-1].password,paswd) != 0){
				send_events_u32(EVENTS_SET_PASSWD,0/*paswd*/);
				strcpy(settings.interface_sett.users[num-1].password,paswd);
				return SETT_DIFF;
			}
			else
				return SETT_EQUAL;
		}else
			return SETT_BAD;
	}
}

int set_interface_users_rule(u8 num,u8 rules){
	if(num==0){
		//nothing
		return SETT_BAD;
	}
	else{
		if((rules == NO_RULE)||(rules == ADMIN_RULE)||(rules == USER_RULE)){
			if(settings.interface_sett.users[num-1].rule!=rules){
				send_events_u32(EVENTS_SET_USER_RULE,(u32)rules);
				settings.interface_sett.users[num-1].rule = rules;
				return SETT_DIFF;
			}
			else
				return SETT_EQUAL;
		}
		else
			return SETT_BAD;
	}
}

void set_current_username(char *login){
	strcpy(dev.user.current_username,login);
}

//not used
int set_interface_period(u8 period){
	settings.interface_sett.refr_period = period;
	return 0;
}

int set_interface_name(char *text){
	if(strlen(text)<DESCRIPT_LEN){
		if(strcmp(settings.interface_sett.system_name,text) != 0){
			send_events_u32(EVENTS_SET_NAME,0/*text*/);
			strncpy(settings.interface_sett.system_name,text,DESCRIPT_LEN);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}else
		return SETT_BAD;
}

int set_interface_location(char *text){
	if(strlen(text)<DESCRIPT_LEN){
		if(strcmp(settings.interface_sett.system_location,text) != 0){
			send_events_u32(EVENTS_SET_LOCATION,0/*text*/);
			strncpy(settings.interface_sett.system_location,text,DESCRIPT_LEN);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}else
		return SETT_BAD;
}

int set_interface_contact(char *text){
	if(strlen(text)<DESCRIPT_LEN){
		if(strcmp(settings.interface_sett.system_contact,text) != 0){
			send_events_u32(EVENTS_SET_COMPANY,0/*text*/);
			strncpy(settings.interface_sett.system_contact,text,DESCRIPT_LEN);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}else
		return SETT_BAD;
}

int set_port_descr(u8 port, char *text){
	if(port<PORT_NUM){
		if(strlen(text)<PORT_DESCR_LEN){
			if(strcmp(settings.port_sett[port].port_descr,text) != 0){
				send_events_u32(EVENTS_SET_PORT1_DESCR+port,0/*text*/);
				strncpy(settings.port_sett[port].port_descr,text,PORT_DESCR_LEN);
				return SETT_DIFF;
			}
			else
				return SETT_EQUAL;
		}else
			return SETT_BAD;
	}
	else
		return SETT_BAD;
}

//not used
int set_ext_vendor_name_flag(u8 state){
	if((state == 0)||(state == 1)){
		///settings.interface_sett.ext_vendor_name_flag = state;
		return 0;
	}
	else
		return -1;
}

//not used
int set_ext_vendor_name(char *name){
	if(strlen(name)<DESCRIPT_LEN){
		//strncpy(settings.interface_sett.ext_vendor_name,name,DESCRIPT_LEN);
		return 0;
	}else
		return -1;
}


//настройки smtp

int set_smtp_state(u8 state){
	if((state == 0)||(state == 1)){
		if(settings.smtp_sett.state != state){
			send_events_u32(EVENTS_SET_SMTP_STATE,(u32)state);
			settings.smtp_sett.state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;

	}
	else
		return SETT_BAD;
}



int set_smtp_server(uip_ipaddr_t ip){
	if(check_ip_addr(ip)==0){
		if(!(uip_ipaddr_cmp(ip,settings.smtp_sett.server_addr))){
			send_events_ip(EVENTS_SET_SMTP_IP,ip);
			uip_ipaddr_copy(settings.smtp_sett.server_addr,ip);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_smtp_to(char *to){
	if(strlen(to)<32){
		if(strcmp(to,settings.smtp_sett.to)!=0){
			send_events_u32(EVENTS_SET_SMTP_TO,0/*to*/);
			strncpy(settings.smtp_sett.to,to,64);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_smtp_to2(char *to){
	if(strlen(to)<32){
		if(strcmp(to,settings.smtp_sett.to2)!=0){
			send_events_u32(EVENTS_SET_SMTP_TO2,0/*to*/);
			strncpy(settings.smtp_sett.to2,to,64);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_smtp_to3(char *to){
	if(strlen(to)<32){
		if(strcmp(to,settings.smtp_sett.to3)!=0){
			send_events_u32(EVENTS_SET_SMTP_TO3,0/*to*/);
			strncpy(settings.smtp_sett.to3,to,64);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}


int set_smtp_from(char *to){
	if(strlen(to)<64){
		if(strcmp(to,settings.smtp_sett.from)!=0){
			send_events_u32(EVENTS_SET_SMTP_FROM,0/*to*/);
			strncpy(settings.smtp_sett.from,to,64);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_smtp_subj(char *subj){
	if(strlen(subj)<64){
		if(strcmp(subj,settings.smtp_sett.subj)!=0){
			send_events_u32(EVENTS_SET_SMTP_SUBJ,0/*subj*/);
			strcpy(settings.smtp_sett.subj,subj);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_smtp_login(char *login){
	if(strlen(login)<32){
		if(strcmp(login,settings.smtp_sett.login)!=0){
			send_events_u32(EVENTS_SET_SMTP_LOGIN,0/*login*/);
			strncpy(settings.smtp_sett.login,login,32);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_smtp_pass(char *pass){
	if(strlen(pass)<32){
		if(strcmp(pass,settings.smtp_sett.pass)!=0){
			send_events_u32(EVENTS_SET_SMTP_PASS,0/*pass*/);
			strncpy(settings.smtp_sett.pass,pass,32);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_smtp_port(u16 port){
	if(settings.smtp_sett.port != port){
		send_events_u32(EVENTS_SET_SMTP_PORT,(u32)port);
		settings.smtp_sett.port = port;
		return SETT_DIFF;
	}
	else
		return SETT_EQUAL;
}
int set_smtp_domain(char *domain){
	if(strlen(domain)<32){
		strcpy(settings.smtp_sett.domain_name,domain);
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

//настройки sntp
int set_sntp_state(u8 state){
	if((state == 0)||(state == 1)){
		if(settings.sntp_sett.state != state){
			send_events_u32(EVENTS_SET_SNTP_STATE,(u32)state);
			settings.sntp_sett.state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_sntp_serv_name(char *name){
	if(check_str(name,MAX_SERV_NAME)==0){
		strcpy(settings.sntp_sett.serv_name,name);
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_sntp_serv(uip_ipaddr_t ip){
	if(check_ip_addr(ip)==0){
		if(!(uip_ipaddr_cmp(ip,settings.sntp_sett.addr))){
			send_events_ip(EVENTS_SET_SNTP_IP,ip);
			uip_ipaddr_copy(settings.sntp_sett.addr,ip);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_sntp_timezone(i8 timezone){
	if((timezone>=-12)&&(timezone<=13)){
		if(settings.sntp_sett.timezone != timezone){
			send_events_u32(EVENTS_SET_SNTP_TZONE,(u32)timezone);
			settings.sntp_sett.timezone = timezone;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_sntp_period(u8 period){
	if(settings.sntp_sett.period != period){
		send_events_u32(EVENTS_SET_SNTP_PERIOD,(u32)period);
		settings.sntp_sett.period = period;
		return SETT_DIFF;
	}
	else
		return SETT_EQUAL;
}

//настройка syslog
int set_syslog_state(u8 state){
	if((state == 0)||(state == 1)){
		if(settings.syslog_sett.state != state){
			send_events_u32(EVENTS_SET_SYSLOG_STATE,(u32)state);
			settings.syslog_sett.state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_syslog_serv(uip_ipaddr_t ip){
	if(check_ip_addr(ip)==0){
		if(!(uip_ipaddr_cmp(settings.syslog_sett.server_addr,ip))){
			send_events_ip(EVENTS_SET_SYSLOG_IP,ip);
			uip_ipaddr_copy(settings.syslog_sett.server_addr,ip);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

//настройки event list
int set_event_base_s(u8 state,u8 level){
	if(((state == 0)||(state == 1))&&(level<=SMASK)){
		settings.event_list.base_s = level;
		if(state == 1)
			settings.event_list.base_s |= SSTATE;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_event_port_s(u8 state,u8 level){
	if(((state == 0)||(state == 1))&&(level<=SMASK)){
		settings.event_list.port_s = level;
		if(state == 1)
			settings.event_list.port_s |= SSTATE;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_event_vlan_s(u8 state,u8 level){
	if(((state == 0)||(state == 1))&&(level<=SMASK)){
		settings.event_list.vlan_s = level;
		if(state == 1)
			settings.event_list.vlan_s |= SSTATE;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}
int set_event_stp_s(u8 state,u8 level){
	if(((state == 0)||(state == 1))&&(level<=SMASK)){
		//if(((state<<3) & level) != (settings.event_list.stp_s))
		//	send_events_u32(EVENTS_EVENTLIST_STP_S,(u32)state);
		settings.event_list.stp_s = level;
		if(state == 1)
			settings.event_list.stp_s |= SSTATE;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}
int set_event_qos_s(u8 state,u8 level){
	if(((state == 0)||(state == 1))&&(level<=SMASK)){
		//if(((state<<3) & level) != (settings.event_list.qos_s))
		//	send_events_u32(EVENTS_EVENTLIST_QOS_S,(u32)state);
		settings.event_list.qos_s = level;
		if(state == 1)
			settings.event_list.qos_s |= SSTATE;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}
int set_event_other_s(u8 state,u8 level){
	if(((state == 0)||(state == 1))&&(level<=SMASK)){
		//if(((state<<3) & level) != (settings.event_list.ohter_s))
		//	send_events_u32(EVENTS_EVENTLIST_OTHER_S,(u32)state);
		settings.event_list.ohter_s = level;
		if(state == 1)
			settings.event_list.ohter_s |= SSTATE;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_event_port_link_t(u8 state,u8 level){
	if(((state == 0)||(state == 1))&&(level<=SMASK)){
		//if(((state<<3) & level) != (settings.event_list.port_link_t))
		//	send_events_u32(EVENTS_EVENTLIST_LINK_T,(u32)state);
		settings.event_list.port_link_t = level;
		if(state == 1)
			settings.event_list.port_link_t |= SSTATE;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_event_port_poe_t(u8 state,u8 level){
	if(((state == 0)||(state == 1))&&(level<=SMASK)){
		//if(((state<<3) & level) != (settings.event_list.port_poe_t))
		//	send_events_u32(EVENTS_EVENTLIST_POE_T,(u32)state);
		settings.event_list.port_poe_t = level;
		if(state == 1)
			settings.event_list.port_poe_t |= SSTATE;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_event_stp_t(u8 state,u8 level){
	if(((state == 0)||(state == 1))&&(level<=SMASK)){
		//if(((state<<3) & level) != (settings.event_list.stp_t))
		//	send_events_u32(EVENTS_EVENTLIST_STP_T,(u32)state);
		settings.event_list.stp_t = level;
		if(state == 1)
			settings.event_list.stp_t |= SSTATE;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_event_spec_link_t(u8 state,u8 level){
	if(((state == 0)||(state == 1))&&(level<=SMASK)){
		//if(((state<<3) & level) != (settings.event_list.spec_link_t))
		//	send_events_u32(EVENTS_EVENTLIST_SLINK_T,(u32)state);
		settings.event_list.spec_link_t = level;
		if(state == 1)
			settings.event_list.spec_link_t |= SSTATE;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_event_spec_ping_t(u8 state,u8 level){
	if(((state == 0)||(state == 1))&&(level<=SMASK)){
		//if(((state<<3) & level) != (settings.event_list.spec_ping_t))
		//	send_events_u32(EVENTS_EVENTLIST_SPING_T,(u32)state);
		settings.event_list.spec_ping_t = level;
		if(state == 1)
			settings.event_list.spec_ping_t |= SSTATE;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_event_spec_speed_t(u8 state,u8 level){
	if(((state == 0)||(state == 1))&&(level<=SMASK)){
		//if(((state<<3) & level) != (settings.event_list.spec_speed_t))
		//	send_events_u32(EVENTS_EVENTLIST_SSPEED_T,(u32)state);
		settings.event_list.spec_speed_t = level;
		if(state == 1)
			settings.event_list.spec_speed_t |= SSTATE;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_event_system_t(u8 state,u8 level){
	if(((state == 0)||(state == 1))&&(level<=SMASK)){
		//if(((state<<3) & level) != (settings.event_list.system_t))
		//	send_events_u32(EVENTS_EVENTLIST_SYSTEM_T,(u32)state);
		settings.event_list.system_t = level;
		if(state == 1)
			settings.event_list.system_t |= SSTATE;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}
int set_event_ups_t(u8 state,u8 level){
	if(((state == 0)||(state == 1))&&(level<=SMASK)){
		//if(((state<<3) & level) != (settings.event_list.ups_t))
		//	send_events_u32(EVENTS_EVENTLIST_UPS_T,(u32)state);
		settings.event_list.ups_t = level;
		if(state == 1)
			settings.event_list.ups_t |= SSTATE;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_event_alarm_t(u8 state,u8 level){
	if(((state == 0)||(state == 1))&&(level<=SMASK)){
		//if(((state<<3) & level) != (settings.event_list.asc_t))
		//	send_events_u32(EVENTS_EVENTLIST_ALARM_T,(u32)state);
		settings.event_list.asc_t = level;
		if(state == 1)
			settings.event_list.asc_t |= SSTATE;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_event_mac_t(u8 state,u8 level){
	if(((state == 0)||(state == 1))&&(level<=SMASK)){
		//if(((state<<3) & level) != (settings.event_list.mac_filt_t))
		//	send_events_u32(EVENTS_EVENTLIST_MAC_T,(u32)state);
		settings.event_list.mac_filt_t = level;
		if(state == 1)
			settings.event_list.mac_filt_t |= SSTATE;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

//настройка сухих контактов
int set_alarm_state(u8 num,u8 state){
	if((num<NUM_ALARMS)&&((state == 0)||(state == 1))){
		if(state != settings.alarm[num].state){
			send_events_u32(EVENTS_ALARM1_STATE+num,(u32)state);
			settings.alarm[num].state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_alarm_front(u8 num,u8 front){
	if((num<NUM_ALARMS)&&((front == 1)||(front == 2)||(front == 3))){
		if(front != settings.alarm[num].front){
			send_events_u32(EVENTS_ALARM1_FRONT+num,(u32)front);
			settings.alarm[num].front = front;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;

	}
	else
		return SETT_BAD;
}

//настройка ограничения скорости

int set_rate_limit_mode(u8 port,u8 mode){
	if((port<ALL_PORT_NUM)&&(mode <3)){
		if(mode != settings.rate_limit[port].mode){
			send_events_u32(EVENTS_RATELIM_MODE_P1+port,(u32)mode);
			settings.rate_limit[port].mode = mode;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_uc_rate_limit(u8 state){
	if(state == ENABLE || state == DISABLE){
		if(state != settings.storm_control.uc){
			send_events_u32(EVENTS_RATELIM_UC,(u32)state);
			settings.storm_control.uc = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_mc_rate_limit(u8 state){
	if(state == ENABLE || state == DISABLE){
		if(state != settings.storm_control.mc){
			send_events_u32(EVENTS_RATELIM_MC,(u32)state);
			settings.storm_control.mc = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_bc_rate_limit(u8 state){
	if(state == ENABLE || state == DISABLE){
		if(state != settings.storm_control.bc){
			send_events_u32(EVENTS_RATELIM_BC,(u32)state);
			settings.storm_control.bc = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_bc_limit(u8 limit){
	if(limit < 100){
		if(limit != settings.storm_control.limit){
			send_events_u32(EVENTS_RATELIM_LIM,(u32)limit);
			settings.storm_control.limit = limit;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}


int set_rate_limit_rx(u8 port,u32 limit){
	if((port<ALL_PORT_NUM)&&(((limit>63)&&(limit<1024*100))||(limit == 0))){
		if(settings.rate_limit[port].rx_rate != limit){
			send_events_u32(EVENTS_RATELIM_RX_P1+port,(u32)limit);
			settings.rate_limit[port].rx_rate = limit;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_rate_limit_tx(u8 port,u32 limit){
	if((port<ALL_PORT_NUM)&&(((limit>63)&&(limit<1024*100)) || (limit==0))){
		if(settings.rate_limit[port].tx_rate != limit){
			send_events_u32(EVENTS_RATELIM_TX_P1+port,(u32)limit);
			settings.rate_limit[port].tx_rate = limit;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

//настройки qos для порта
int set_qos_port_cos_state(u8 port,u8 state){
	if((port<ALL_PORT_NUM)&&((state==0)||(state==1))){
		if(settings.qos_port_sett[port].cos_state != state){
			send_events_u32(EVENTS_COS_STATE_P1+port,(u32)state);
			settings.qos_port_sett[port].cos_state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_qos_port_tos_state(u8 port,u8 state){
	if((port<ALL_PORT_NUM)&&((state==0)||(state==1))){
		if(settings.qos_port_sett[port].tos_state != state){
			send_events_u32(EVENTS_TOS_STATE_P1+port,(u32)state);
			settings.qos_port_sett[port].tos_state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_qos_port_rule(u8 port,u8 state){
	if((port<ALL_PORT_NUM)&&((state==COS_ONLY)||(state==TOS_ONLY)||(state==TOS_AND_COS))){
		if(settings.qos_port_sett[port].rule != state){
			send_events_u32(EVENTS_QOS_RULE_P1+port,(u32)state);
			settings.qos_port_sett[port].rule = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_qos_port_def_pri(u8 port,u8 pri){
	if((port<ALL_PORT_NUM)&&(pri<8)){
		if(settings.qos_port_sett[port].def_pri != pri){
			send_events_u32(EVENTS_QOS_DEFPRI_P1+port,(u32)pri);
			settings.qos_port_sett[port].def_pri = pri;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
//глобальные настройки qos
int set_qos_state(u8 state){
	if((state == 0)||(state == 1)){
		if(settings.qos_sett.state != state){
			send_events_u32(EVENTS_QOS_STATE,(u32)state);
			settings.qos_sett.state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_qos_policy(u8 policy){
	if((policy == FIXED_PRI)||(policy==WEIGHTED_FAIR_PRI)){
		if(settings.qos_sett.policy != policy){
			send_events_u32(EVENTS_QOS_POLICY,(u32)policy);
			settings.qos_sett.policy = policy;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_qos_cos(u8 cos1,u8 queue){
	if((cos1<8)&&(queue<4)){
		//no cos event
		//	send_events_u32(EVENTS_QOS_POLICY,policy);
		settings.qos_sett.cos[cos1] = queue;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}
int set_qos_tos(u8 tos,u8 queue){
	if((tos<64)&&(queue<4)){
		//no tos event
		//	send_events_u32(EVENTS_QOS_POLICY,policy);
		settings.qos_sett.tos[tos] = queue;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

//igmp settings
int set_igmp_snooping_state(u8 state){
	if((state == 1)||(state == 0)){
		if(settings.igmp_sett.igmp_snooping_state != state){
			send_events_u32(EVENTS_IGMP_STATE,(u32)state);
			settings.igmp_sett.igmp_snooping_state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_igmp_query_mode(u8 mode){
	if((mode == 1)||(mode == 0)){
		if(settings.igmp_sett.igmp_query_mode != mode){
			send_events_u32(EVENTS_IGMP_QUERY_MODE,(u32)mode);
			settings.igmp_sett.igmp_query_mode = mode;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}


/*
 * установить поддержку igmp snooping на порту**/
int set_igmp_port_state(u8 port, u8 state){
	if((port<ALL_PORT_NUM)&&((state == 1)||(state == 0))){
		if(settings.igmp_sett.port_state[port] != state){
			send_events_u32(EVENTS_IGMP_ST_P1+port,(u32)state);
			settings.igmp_sett.port_state[port] = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_igmp_query_int(u8 time){
	if(time < 255){
		if(settings.igmp_sett.query_interval != time){
			send_events_u32(EVENTS_IGMP_QUERY_INT,(u32)time);
			settings.igmp_sett.query_interval = time;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_igmp_max_resp_time(u8 time){
	if(time<26){
		if(settings.igmp_sett.max_resp_time != time){
			send_events_u32(EVENTS_IGMP_QUERY_RESP_INT,(u32)time);
			settings.igmp_sett.max_resp_time = time;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}


int set_igmp_group_membership_time(u8 time){
	if(time){
		if(settings.igmp_sett.group_membship_time != time){
			send_events_u32(EVENTS_IGMP_GR_MEMB_TIME,(u32)time);
			settings.igmp_sett.group_membship_time = time;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}


int set_igmp_other_querier_time(u8 time){
	if(time){
		if(settings.igmp_sett.other_querier_time != time){
			send_events_u32(EVENTS_IGMP_QUERIER_INT,(u32)time);
			settings.igmp_sett.other_querier_time = time;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}




//port base vlan
int set_pb_vlan_state(u8 state){
	if((state==0)||(state==1)){
		if(settings.pb_vlan_sett.state != state){
			send_events_u32(EVENTS_PB_VLAN_STATE,(u32)state);
			settings.pb_vlan_sett.state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

//int set_pb_vlan_port(u8 port,u16 state){
//	if(state<=0x7FF){
//		settings.pb_vlan_sett.table[L2F_port_conv(port)] = state;
//		return SETT_DIFF;
//	}
//	else
//		return SETT_BAD;
//}

//управление каждым портом в отдельности
int set_pb_vlan_port(u8 rx_port,u8 tx_port,u8 state){
	if((rx_port<ALL_PORT_NUM)&&(tx_port<PORT_NUM)&&((state == 0)||(state == 1))){
		settings.pb_vlan_sett.table[rx_port][tx_port] = state;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

//групповое управление портами
int set_pb_vlan_table(struct pb_vlan_t *pb){

	for(u8 i=0;i<ALL_PORT_NUM;i++){
		for(u8 j=0;j<ALL_PORT_NUM;j++)
			settings.pb_vlan_sett.table[i][j] = pb->VLANTable[i][j];
	}

	return 0;
}

//port base vlan для SWU-16
//управление каждым портом в отдельности
int set_pb_vlan_swu_port(u8 port,u8 vid){
	if((port<ALL_PORT_NUM)&&(vid<ALL_PORT_NUM)){
		settings.pb_vlan_swu_sett.port_vid[port] = vid;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}



//настройки vlan
int set_vlan_sett_state(u8 state){
	if((state == 0)||(state == 1)){
		if(settings.vlan_sett.state != state){
			send_events_u32(EVENTS_VLAN_STATE,(u32)state);
			settings.vlan_sett.state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_vlan_trunk_state(u8 state){
	if((state == 0)||(state == 1)){
		if(settings.vlan_sett.vlan_trunk_state != state){
			send_events_u32(EVENTS_VLAN_TRUNK_STATE,(u32)state);
			settings.vlan_sett.vlan_trunk_state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_vlan_sett_mngt(u16 vid){
	if(vid<4096){
		if(settings.vlan_sett.mngvid != vid){
			send_events_u32(EVENTS_VLAN_MVID,(u32)vid);
			settings.vlan_sett.mngvid = vid;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_vlan_sett_port_state(u8 port, u8 state){
	if((port<ALL_PORT_NUM)&&(state<4)){
		if(settings.vlan_sett.port_st[port] != state){
			send_events_u32(EVENTS_VLAN_STATE_P1 + port,(u32)state);
			settings.vlan_sett.port_st[port] = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_vlan_sett_dvid(u8 port, u16 dvid){
	if((port<ALL_PORT_NUM)&&(dvid<4096	)){
		if(settings.vlan_sett.dvid[port] != dvid){
			send_events_u32(EVENTS_VLAN_DVID_P1 + port,(u32)dvid);
			settings.vlan_sett.dvid[port] = dvid;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_vlan_sett_vlannum(u16 num){
	if(num<MAXVlanNum){
		settings.vlan_sett.VLANNum = num;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int delete_vlan(u16 vid){
	int num;
	u8 state;
	u8 Ports[PORT_NUM];
	char vlan_name[128];
	u16 vid2;

	num = vlan_vid2num(vid);
	if(num == -1){
		return SETT_BAD;
	}

	//printf("delete vlan %d\r\n",num);

	for(u8 i=num;i<get_vlan_sett_vlannum()-1;i++){
		//get config
		state = get_vlan_state(i+1);
		strcpy(vlan_name,get_vlan_name(i+1));
		vid2 = get_vlan_vid(i+1);
		for(u8 j=0;j<ALL_PORT_NUM;j++){
			Ports[j] = get_vlan_port_state(i+1,j);
		}

		//set
		set_vlan_state(i,state);
		set_vlan_name(i,vlan_name);
		set_vlan_vid(i,vid2);
		for(u8 j=0;j<(ALL_PORT_NUM);j++){
			set_vlan_port(i,j,Ports[j]);
		}

	}
	num = get_vlan_sett_vlannum();
	set_vlan_sett_vlannum(--num);
	return 0;
}

//vlan`ы
int set_vlan_state(u8 num,u8 state){
	if((num<MAXVlanNum)&&((state==0)||(state==1))){
		if(settings.vlan[num].state != state){
			send_events_u32(EVENTS_VLAN_EDIT,(u32)num);
			settings.vlan[num].state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;

	}
	else
		return SETT_BAD;
}

int set_vlan_vid(u8 num,u16 vid){
	if((num<MAXVlanNum)&&(vid < 4096)){
		if(settings.vlan[num].VID != vid){
			send_events_u32(EVENTS_VLAN_EDIT,(u32)num);
			settings.vlan[num].VID = vid;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_vlan_name(u8 num, char *name){
	if((num<MAXVlanNum)&&(strlen(name)<17)){
		strncpy(settings.vlan[num].VLANNAme,name,17);
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}
int set_vlan_port(u8 num,u8 port,u8 state){
	if((num<MAXVlanNum)&&(port<ALL_PORT_NUM)&&(state<4)){
		if(settings.vlan[num].Ports[port] != state){
			send_events_u32(EVENTS_VLAN_EDIT,(u32)num);
			settings.vlan[num].Ports[port] = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

//настройки stp
int set_stp_state(u8 state){
	if((state==0)||(state==1)){
		if(settings.stp_sett.state != state){
			send_events_u32(EVENTS_STP_STATE,(u32)state);
			settings.stp_sett.state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_stp_magic(u16 magic){
	settings.stp_sett.magic = magic;
	return 0;
}

int set_stp_proto(u8 proto){
	if((proto==BSTP_PROTO_RSTP)||(proto==BSTP_PROTO_STP)){
		if(settings.stp_sett.proto != proto){
			send_events_u32(EVENTS_STP_PROTO,(u32)proto);
			settings.stp_sett.proto = proto;
			return  0;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_stp_bridge_priority(u16 priority){
	if(priority <= BSTP_DEFAULT_BRIDGE_PRIORITY){
		if(settings.stp_sett.bridge_priority != priority){
			send_events_u32(EVENTS_STP_PRI,(u32)priority);
			settings.stp_sett.bridge_priority = priority;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_stp_bridge_max_age(u8 mage){
	if((mage<=40)&&(mage>=6)){
		if(settings.stp_sett.bridge_max_age != mage){
			send_events_u32(EVENTS_STP_MAGE,(u32)mage);
			settings.stp_sett.bridge_max_age = mage;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_stp_bridge_htime(u8 htime){
	if((htime==1)||(htime==2)){
		if(settings.stp_sett.bridge_htime != htime){
			send_events_u32(EVENTS_STP_HTIME,(u32)htime);
			settings.stp_sett.bridge_htime = htime;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_stp_bridge_fdelay(u8 delay){
	if((delay>=4)&&(delay<=30)){
		if(settings.stp_sett.bridge_fdelay != delay){
			send_events_u32(EVENTS_STP_FDELAY,(u32)delay);
			settings.stp_sett.bridge_fdelay = delay;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_stp_txholdcount(u8 hcount){
	if((hcount>=1)&&(hcount<=10)){
		if(settings.stp_sett.txholdcount != hcount){
			send_events_u32(EVENTS_STP_HCNT,(u32)hcount);
			settings.stp_sett.txholdcount = hcount;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_stp_bridge_mdelay(u8 delay){
	if(settings.stp_sett.bridge_mdelay != delay){
		send_events_u32(EVENTS_STP_MDELAY,(u32)delay);
		settings.stp_sett.bridge_mdelay = delay;
		return SETT_DIFF;
	}
	else
		return SETT_EQUAL;
}

//настройки stp port
int set_stp_port_enable(u8 port,u8 en){
	if((port<ALL_PORT_NUM)&&((en == 0)||(en == 1))){
		if(settings.stp_port_sett[port].enable != en){
			send_events_u32(EVENTS_STP_PORT_EN_P1+port,(u32)en);
			settings.stp_port_sett[port].enable = en;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_stp_port_state(u8 port,u8 state){
	if((port<ALL_PORT_NUM)&&((state == 0)||(state == 1))){
		if(settings.stp_port_sett[port].state != state){
			send_events_u32(EVENTS_STP_PORT_ST_P1+port,(u32)state);
			settings.stp_port_sett[port].state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_stp_port_priority(u8 port,u8 priority){
	if((port<ALL_PORT_NUM)&&(priority <= BSTP_MAX_PORT_PRIORITY)){
		if(settings.stp_port_sett[port].priority != priority){
			send_events_u32(EVENTS_STP_PORT_PRI_P1+port,(u32)priority);
			settings.stp_port_sett[port].priority = priority;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_stp_port_cost(u8 port,u32 cost){
	if((port<ALL_PORT_NUM)&&(cost<200000000)){
		if(settings.stp_port_sett[port].path_cost != cost){
			send_events_u32(EVENTS_STP_PORT_COST_P1+port,(u32)cost);
			settings.stp_port_sett[port].path_cost = cost;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_stp_port_autocost(u8 port,u8 flag){
	if((port<ALL_PORT_NUM)&&((flag == 0)||(flag == 1))){
		if(flag == 1)
			settings.stp_port_sett[port].flags |= BSTP_PORTCFG_FLAG_ADMCOST;
		else
			settings.stp_port_sett[port].flags &= ~BSTP_PORTCFG_FLAG_ADMCOST;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_stp_port_autoedge(u8 port,u8 flag){
	if((port<ALL_PORT_NUM)&&((flag == 0)||(flag == 1))){
		if(flag == 1)
			settings.stp_port_sett[port].flags |= BSTP_PORTCFG_FLAG_AUTOEDGE;
		else
			settings.stp_port_sett[port].flags &= ~BSTP_PORTCFG_FLAG_AUTOEDGE;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_stp_port_edge(u8 port,u8 flag){
	if((port<ALL_PORT_NUM)&&((flag == 0)||(flag == 1))){
		if(flag == 1)
			settings.stp_port_sett[port].flags |= BSTP_PORTCFG_FLAG_EDGE;
		else
			settings.stp_port_sett[port].flags &= ~BSTP_PORTCFG_FLAG_EDGE;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_stp_port_autoptp(u8 port,u8 flag){
	if((port<ALL_PORT_NUM)&&((flag == 0)||(flag == 1))){
		if(flag == 1)
			settings.stp_port_sett[port].flags |= BSTP_PORTCFG_FLAG_AUTOPTP;
		else
			settings.stp_port_sett[port].flags &= ~BSTP_PORTCFG_FLAG_AUTOPTP;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_stp_port_ptp(u8 port,u8 flag){
	if((port<ALL_PORT_NUM)&&((flag == 0)||(flag == 1))){
		if(flag == 1)
			settings.stp_port_sett[port].flags |= BSTP_PORTCFG_FLAG_PTP;
		else
			settings.stp_port_sett[port].flags &= ~BSTP_PORTCFG_FLAG_PTP;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}


int set_stp_port_flags(u8 port,u8 flag){
	if(port<ALL_PORT_NUM){
		settings.stp_port_sett[port].flags = flag;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

//stp port BPDU forwarding
int set_stp_bpdu_fw(u8 state){
	if((state == 0)||(state == 1)){
		if(settings.stp_forward_bpdu.state != state){
			send_events_u32(EVENTS_BPDU_FW_ST,(u32)state);
			settings.stp_forward_bpdu.state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}


//настройки калибровки кабельного тестера
int set_callibrate_koef_1(u8 port, u16 koef){
	if(port<COOPER_PORT_NUM){
		if(settings.port_callibrate[port].koeff_1 != koef){
			send_events_u32(EVENTS_CALLIBRATE_KOEF1_P1+port,(u32)koef);
			settings.port_callibrate[port].koeff_1 = koef;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_callibrate_koef_2(u8 port, u16 koef){
	if(port<COOPER_PORT_NUM){
		if(settings.port_callibrate[port].koeff_2 != koef){
			send_events_u32(EVENTS_CALLIBRATE_KOEF2_P1+port,(u32)koef);
			settings.port_callibrate[port].koeff_2 = koef;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_callibrate_len(u8 port, u8 len){
	if(port<COOPER_PORT_NUM){
		settings.port_callibrate[port].length = len;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

//настройки snmp
int set_snmp_state(u8 state){
	if((state==0)||(state==1)){
		if(settings.snmp_sett.state != state){
			send_events_u32(EVENTS_SNMP_STATE,(u32)state);
			settings.snmp_sett.state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_snmp_mode(u8 mode){
	if((mode==SNMP_R)||(mode==SNMP_W)||(mode==SNMP_RW)){
		if(settings.snmp_sett.mode != mode){
			send_events_u32(EVENTS_SNMP_MODE,(u32)mode);
			settings.snmp_sett.mode = mode;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_snmp_serv(uip_ipaddr_t addr){
	if(check_ip_addr(addr)==0){
		if(!(uip_ipaddr_cmp(settings.snmp_sett.server_addr,addr))){
			send_events_ip(EVENTS_SNMP_IP,addr);
			uip_ipaddr_copy(settings.snmp_sett.server_addr,addr);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_snmp_vers(u8 vers){
	if(vers<4){
		if(settings.snmp_sett.version != vers){
			send_events_u32(EVENTS_SNMP_VERS,(u32)vers);
			settings.snmp_sett.version = vers;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
/*
int set_snmp_communitie(char *comm){
	if(strlen(comm)<16){
		//add later
		strcpy(settings.snmp_sett.snmp_commun[0].community,comm);
		settings.snmp_sett.snmp_commun[0].privelege = SNMP_RW;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}*/

int set_snmp1_read_communitie(char *comm){
	if(check_str(comm,16)==-1)
		return SETT_BAD;
	if(strlen(comm)<16){
		strcpy(settings.snmp_sett.snmp1_read_commun.community,comm);
		settings.snmp_sett.snmp1_read_commun.privelege = SNMP_R;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_snmp1_write_communitie(char *comm){
	if(check_str(comm,16)==-1)
		return SETT_BAD;
	if(strlen(comm)<16){
		strcpy(settings.snmp_sett.snmp1_write_commun.community,comm);
		settings.snmp_sett.snmp1_write_commun.privelege = SNMP_RW;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

//SNMP v3 part
int set_snmp3_level(u8 usernum, u8 level){
	if((usernum<NUM_USER)&&(level == NO_AUTH_NO_PRIV || level == AUTH_NO_PRIV || level == AUTH_PRIV)){
		if(settings.snmp_sett.user[usernum].level != level){
			send_events_u32(EVENTS_SNMP3_USER1_LEVEL+usernum,(u32)level);
			settings.snmp_sett.user[usernum].level = level;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}else
		return SETT_BAD;
}

int set_snmp3_user_name(u8 usernum, char *str){
	if(usernum<NUM_USER){
		if(check_str(str,64)==-1)
			return SETT_BAD;
		if(strlen(str)>64)
			return SETT_BAD;
		strcpy(settings.snmp_sett.user[usernum].user_name,str);
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_snmp3_auth_pass(u8 usernum, char *str){
	if(usernum<NUM_USER){
		if(check_str(str,64)==-1)
			return SETT_BAD;
		if(strlen(str)>64)
			return SETT_BAD;
		strcpy(settings.snmp_sett.user[usernum].auth_pass,str);
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_snmp3_priv_pass(u8 usernum, char *str){
	if(usernum<NUM_USER){
		if(check_str(str,64)==-1)
			return SETT_BAD;
		if(strlen(str)>64)
			return SETT_BAD;
		strcpy(settings.snmp_sett.user[usernum].priv_pass, str);
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_snmp3_engine_id(engine_id_t *eid){
	if(eid->len>64)
		eid->len = 0;
	for(u8 i=0;i<64;i++)
		settings.snmp_sett.engine_id.ptr[i] = eid->ptr[i];
	settings.snmp_sett.engine_id.len = eid->len;
	return SETT_DIFF;
}


int set_softstart_time(u16 time){
	if((time==1)||(time==2)){
		if(settings.sstart_time != time){
			send_events_u32(EVENTS_SS_TIME,(u32)time);
			settings.sstart_time = time;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_telnet_state(u8 state){
	if((state==0)||(state==1)){
		if(settings.telnet_sett.state != state){
			send_events_u32(EVENTS_TELNET_STATE,(u32)state);
			settings.telnet_sett.state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_telnet_echo(u8 state){
	if((state==0)||(state==1)){
		if(settings.telnet_sett.echo != state){
			send_events_u32(EVENTS_TELNET_ECHO,(u32)state);
			settings.telnet_sett.echo = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_telnet_rn(u8 mode){
	return 0;
}


//tftp
int set_tftp_state(u8 state){
	if((state==0)||(state==1)){
		if(settings.tftp_sett.state != state){
			send_events_u32(EVENTS_TFTP_STATE,(u32)state);
			settings.tftp_sett.state = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_tftp_mode(u8 mode){
	if((mode == 0)||(mode == 1)){
		if(settings.tftp_sett.mode!=mode){
			send_events_u32(EVENTS_TFTP_MODE,(u32)mode);
			settings.tftp_sett.mode = mode;
			return 0;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}
int set_tftp_port(u16 port){
	if((port)&&(port!=7753)){
		if(settings.tftp_sett.port!=port){
			send_events_u32(EVENTS_TFTP_PORT,(u32)port);
			settings.tftp_sett.port = port;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;

}

int set_downshifting_mode(u8 state){
	if((state==0)||(state==1)){
		if(settings.downshift_mode != state){
			send_events_u32(EVENTS_DOWNSHIFT_STATE,(u32)state);
			settings.downshift_mode = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}


int set_plc_out_state(u8 channel,u8 state){
	if(channel<PLC_RELAY_OUT){
		if(state == 0 || state == 1 || state == 2){
			if(settings.plc.out_state[channel] != state){
				send_events_u32(EVENTS_PLC_OUT_1+channel,(u32)state);
				settings.plc.out_state[channel] = state;
				return SETT_DIFF;
			}
			else
				return SETT_EQUAL;
		}
	}
	return SETT_BAD;
}

//состояние выхода при срабатывании правила
int set_plc_out_action(u8 channel, u8 action){
	if(channel<PLC_RELAY_OUT){
		if(action == PLC_ACTION_OPEN ||	action == PLC_ACTION_SHORT ||action == PLC_ACTION_IMPULSE){
			if(settings.plc.out_action[channel] != action){
				send_events_u32(EVENTS_PLC_ACTION_OUT1+channel,(u32)action);
				settings.plc.out_action[channel] = action;
				return SETT_DIFF;
			}
			else
				return SETT_EQUAL;
		}
		else
			return SETT_BAD;
	}
	else
		return SETT_BAD;
}

int set_plc_out_event(u8 channel, u8 event, u8 state){
	if(channel<PLC_RELAY_OUT){
		if(event<PLC_EVENTS){
			if(state == 0 || state == 1){
				settings.plc.out_event[channel][event] = state;
				return SETT_DIFF;
			}
			else
				return SETT_BAD;
		}
		else
			return SETT_BAD;
	}
	else
		return SETT_BAD;
}

int set_plc_in_state(u8 channel, u8 state){
	if(channel<PLC_INPUTS){
		if(state == 1 || state == 0 || state == 2){
			if(settings.plc.in_state[channel] != state){
				send_events_u32(EVENTS_PLC_STATE_IN1+channel,(u32)state);
				settings.plc.in_state[channel] = state;
				return SETT_DIFF;
			}
			else
				return SETT_EQUAL;
		}
		else
			return SETT_BAD;
	}
	else
		return SETT_BAD;
}

int set_plc_in_alarm_state(u8 channel, u8 state){
	if(channel<PLC_INPUTS){
		if(state == 0 || state == 1 || state == 2){
			if(settings.plc.in_alarm_state[channel] != state){
				send_events_u32(EVENTS_PLC_ALSTATE_IN1+channel,(u32)state);
				settings.plc.in_alarm_state[channel] = state;
				return SETT_DIFF;
			}
			else
				return SETT_EQUAL;

		}
		else
			return SETT_BAD;
	}
	else
		return SETT_BAD;
}

int set_plc_em_model(u16 model){
	if(settings.plc.em_model != model){
		send_events_u32(EVENTS_PLC_EM_MODEL,(u32)model);
		settings.plc.em_model = model;
		return SETT_DIFF;
	}
	else
		return SETT_EQUAL;
}

int set_plc_em_rate(u8 rate){
	if(rate<7){
		if(settings.plc.em_baudrate != rate){
			send_events_u32(EVENTS_PLC_EM_RATE,(u32)rate);
			settings.plc.em_baudrate = rate;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_plc_em_parity(u8 parity){
	if(parity == NO_PARITY || parity==EVEN_PARITY || parity==ODD_PARITY){
		if(settings.plc.em_parity != parity){
			send_events_u32(EVENTS_PLC_EM_PARITY,(u32)parity);
			settings.plc.em_parity = parity;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_plc_em_databits(u8 bits){
	if(bits>4 && bits<10){
		if(settings.plc.em_databits != bits){
			send_events_u32(EVENTS_PLC_EM_DATABITS,(u32)bits);
			settings.plc.em_databits = bits;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_plc_em_stopbits(u8 bits){
	if(bits ==1 || bits == 2){
		if(settings.plc.em_stopbits != bits){
			send_events_u32(EVENTS_PLC_EM_STOPBITS,(u32)bits);
			settings.plc.em_stopbits = bits;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_plc_em_pass(char *pass){
	if(pass != NULL && strlen(pass)<PLC_EM_MAX_PASS){
		if(strcmp(settings.plc.em_pass,pass)){
			send_events_u32(EVENTS_PLC_EM_PASS,0/*pass*/);
			strcpy(settings.plc.em_pass,pass);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;

	}
	else
		return SETT_BAD;
}

int set_plc_em_id(char *id){
	if(id != NULL && strlen(id)<PLC_EM_MAX_ID){
		if(strcmp(settings.plc.em_id,id)){
			send_events_u32(EVENTS_PLC_EM_ID,0/*id*/);
			strcpy(settings.plc.em_id,id);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;

	}
	else
		return SETT_BAD;
}



int set_mac_filter_state(u8 port,u8 state){
	if(port<ALL_PORT_NUM){
		if(state == PORT_NORMAL || state == MAC_FILT || state == PORT_FILT || state == PORT_FILT_TEMP){
			if(settings.port_sett[port].mac_filtering != state){
				send_events_u32(EVENTS_MACFILT_ST_PORT1+port,(u32)state);
				settings.port_sett[port].mac_filtering = state;
				return SETT_DIFF;
			}
			else
				return SETT_EQUAL;
		}
		else
			return SETT_BAD;
	}
	else
		return SETT_BAD;
}

int set_mac_learn_cpu(u8 state){
	if((state == ENABLE)||(state == DISABLE)){
		settings.cpu_mac_learning = state;
		return SETT_DIFF;
	}
	return SETT_BAD;
}

//установка mac
int set_mac_bind_entry_mac(u8 entry,u8 *mac){
	for(u8 i=0;i<6;i++)
		settings.mac_bind_entries[entry].mac[i] = mac[i];
	return SETT_DIFF;
}

//установка порта
int set_mac_bind_entry_port(u8 entry, u8 port){
	if(port < ALL_PORT_NUM){
		settings.mac_bind_entries[entry].port = port;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

//установка флага активности
int set_mac_bind_entry_active(u8 entry,u8 flag){
	if(flag == 1 || flag == 0){
		settings.mac_bind_entries[entry].active = flag & 0x01;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int add_mac_bind_entry(u8 *mac,u8 port){
	u8 curr_num;
	if(port<ALL_PORT_NUM){
		//проверяем, что нет такой записи
		for(u8 i=0;i<MAC_BIND_MAX_ENTRIES;i++){
			if(settings.mac_bind_entries[i].active &&
			  settings.mac_bind_entries[i].port == port &&
			  settings.mac_bind_entries[i].mac[0] == mac[0] &&
			  settings.mac_bind_entries[i].mac[1] == mac[1] &&
			  settings.mac_bind_entries[i].mac[2] == mac[2] &&
			  settings.mac_bind_entries[i].mac[3] == mac[3] &&
			  settings.mac_bind_entries[i].mac[4] == mac[4] &&
			  settings.mac_bind_entries[i].mac[5] == mac[5]){
				return SETT_EQUAL;
			}
		}

		curr_num = get_mac_bind_num();
		if(curr_num<MAC_BIND_MAX_ENTRIES){
			set_mac_bind_entry_active(curr_num,1);
			set_mac_bind_entry_mac(curr_num,mac);
			set_mac_bind_entry_port(curr_num,port);
			send_events_mac(EVENTS_ADD_MACFILT_PORT1+port,mac);
			return SETT_DIFF;
		}
		else
			return SETT_BAD;
	}
	else
		return SETT_BAD;
}

int del_mac_bind_entry(u8 entry_num){
u8 mac[6],port,curr_num;

	curr_num = get_mac_bind_num();
	if(entry_num<curr_num){
		for(u8 j=0;j<6;j++)
			mac[j] = get_mac_bind_entry_mac(entry_num,j);
		send_events_mac(EVENTS_DEL_MACFILT_PORT1+get_mac_bind_entry_port(entry_num),mac);
		set_mac_bind_entry_active(entry_num,0);
		for(u8 i=entry_num;i<(curr_num-1);i++){
			for(u8 j=0;j<6;j++)
				mac[j] = get_mac_bind_entry_mac(i+1,j);
			port = get_mac_bind_entry_port(i+1);
			set_mac_bind_entry_active(i,1);
			set_mac_bind_entry_mac(i,mac);
			set_mac_bind_entry_port(i,port);
		}
		set_mac_bind_entry_active(curr_num-1,0);
		return 0;
	}
	else
		return SETT_BAD;
}

int set_ups_delayed_start(u8 state){
	if(state != ENABLE && state != DISABLE){
		if(settings.ups.delayed_start != state){
			send_events_u32(EVENTS_UPS_DELAYED_ST,(u32)state);
			settings.ups.delayed_start = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

//Link Aggregation

int set_lag_valid(u8 index,u8 state){
	if((index<LAG_MAX_ENTRIES)&&(state == ENABLE || state == DISABLE)){
		if(settings.lag_entry[index].valid_flag != state){
			settings.lag_entry[index].valid_flag = state;

			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;

	}
	else
		return SETT_BAD;
}

int set_lag_state(u8 index,u8 state){
	if((index<LAG_MAX_ENTRIES)&&(state == ENABLE || state == DISABLE)){
		if(settings.lag_entry[index].state != state){
			settings.lag_entry[index].state = state;
			send_events_u32(EVENTS_LAG1_STATE+index,(u32)state);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_lag_master_port(u8 index, u8 port){
	if(index<LAG_MAX_ENTRIES && port < ALL_PORT_NUM){
		if(settings.lag_entry[index].master_port != port){
			settings.lag_entry[index].master_port = port;
			send_events_u32(EVENTS_LAG1_MASTER+index,(u32)port);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_lag_port(u8 index,u8 port, u8 state){
	if(index<LAG_MAX_ENTRIES && port<ALL_PORT_NUM && (state == ENABLE || state ==DISABLE)){
		if(settings.lag_entry[index].ports[port] != state){
			settings.lag_entry[index].ports[port] = state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;

	}
	else
		return SETT_BAD;
}

int del_lag_entry(u8 index){
	if(index<LAG_MAX_ENTRIES){
		settings.lag_entry[index].valid_flag = 0;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}


//Port MIrroring
int set_mirror_state(u8 state){
	if((state == ENABLE || state == DISABLE)){
		if(settings.mirror_entry.state != state){
			settings.mirror_entry.state = state;
			send_events_u32(EVENTS_MIRROR_STATE,(u32)state);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_mirror_target_port(u8 port){
	if(port < ALL_PORT_NUM){
		if(settings.mirror_entry.target_port != port){
			settings.mirror_entry.target_port = port;
			send_events_u32(EVENTS_MIRROR_TARGET,(u32)port);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;

	}
	else
		return SETT_BAD;
}

int set_mirror_port(u8 port, u8 state){
	if(port<ALL_PORT_NUM && (state < 4)){
		if(settings.mirror_entry.ports[port] != state){
			settings.mirror_entry.ports[port] = state;
			send_events_u32(EVENTS_MIRROR_PORT1+port,(u32)state);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

//teleport - inputs
int set_input_state(u8 channel, u8 state){
	if(channel<MAX_INPUT_NUM){
		if(state == 1 || state == 0){
			if(settings.input[channel].state != state){
				send_events_u32(EVENTS_STATE_IN1+channel,(u32)state);
				settings.input[channel].state = state;
				return SETT_DIFF;
			}
			else
				return SETT_EQUAL;
		}
		else
			return SETT_BAD;
	}
	else
		return SETT_BAD;
}

int set_input_name(u8 input,char *name){
	if(input < MAX_INPUT_NUM){
		strcpy(settings.input[input].name,name);
		return SETT_DIFF;
	}else
		return SETT_BAD;
}


int set_input_remdev(u8 input, u8 devnum){
	if(input < MAX_INPUT_NUM){
		if(settings.input[input].rem_dev != devnum){
			send_events_u32(EVENTS_REMDEV_IN1+input,(u32)devnum);
			settings.input[input].rem_dev= devnum;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	return SETT_BAD;
}

int set_input_remport(u8 input, u8 port){
	if(input < MAX_INPUT_NUM){
		if(settings.input[input].rem_port != port){
			send_events_u32(EVENTS_REMPORT_IN1+input,(u32)port);
			settings.input[input].rem_port = port;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_input_inverse(u8 input, u8 inv_state){
	if(input < MAX_INPUT_NUM){
		if(settings.input[input].inverse != inv_state){
			send_events_u32(EVENTS_INPUT_REVERSE1+input,(u32)inv_state);
			settings.input[input].inverse = inv_state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}


//teleport - events
int set_tlp_event_state(u8 num, u8 state){
	if(num<MAX_TLP_EVENTS_NUM){
		if(state == 1 || state == 0){
			if(settings.tlp_events[num].state != state){
				send_events_u32(EVENTS_STATE_E1+num,(u32)state);
				settings.tlp_events[num].state = state;
				return SETT_DIFF;
			}
			else
				return SETT_EQUAL;
		}
		else
			return SETT_BAD;
	}
	else
		return SETT_BAD;
}


int set_tlp_event_remdev(u8 num, u8 devnum){
	if(num < MAX_TLP_EVENTS_NUM){
		if(settings.tlp_events[num].rem_dev != devnum){
			send_events_u32(EVENTS_REMDEV_E1+num,(u32)devnum);
			settings.tlp_events[num].rem_dev= devnum;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	return SETT_BAD;
}

int set_tlp_event_remport(u8 num, u8 port){
	if(num < MAX_TLP_EVENTS_NUM){
		if(settings.tlp_events[num].rem_port != port){
			send_events_u32(EVENTS_REMPORT_E1+num,(u32)port);
			settings.tlp_events[num].rem_port = port;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_tlp_event_inverse(u8 num, u8 inv_state){
	if(num < MAX_TLP_EVENTS_NUM){
		if(settings.tlp_events[num].inverse != inv_state){
			send_events_u32(EVENTS_TLP_REVERSE1+num,(u32)inv_state);
			settings.tlp_events[num].inverse = inv_state;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

//teleport - remote devices
int set_tlp_remdev_valid(u8 num,u8 valid){
	if(num<MAX_REMOTE_TLP){
		if(settings.teleport[num].valid != valid){
			send_events_u32(EVENTS_TLP_REMDEV1+num,(u32)valid);
			settings.teleport[num].valid = valid;
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

int set_tlp_remdev_ip(u8 num,uip_ipaddr_t *ip){
	if(num<MAX_REMOTE_TLP){
		uip_ipaddr_copy(settings.teleport[num].ip,*ip);
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_tlp_remdev_mask(u8 num,uip_ipaddr_t *mask){
	if(num<MAX_REMOTE_TLP){
		uip_ipaddr_copy(settings.teleport[num].mask,*mask);
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_tlp_remdev_gate(u8 num,uip_ipaddr_t *gate){
	if(num<MAX_REMOTE_TLP){
		uip_ipaddr_copy(settings.teleport[num].gate,*gate);
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}


int set_tlp_remdev_name(u8 num,char *name){
	if(num<MAX_REMOTE_TLP){
		strcpy(settings.teleport[num].name,name);
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

int set_tlp_remdev_type(u8 num,u8 type){
	if(num<MAX_REMOTE_TLP){
		settings.teleport[num].type = type;
		return SETT_DIFF;
	}
	else
		return SETT_BAD;
}

//удалить запись
int delete_tlp_remdev(u8 num){
	for(u8 i=num;i<MAX_REMOTE_TLP-1;i++){
		settings.teleport[i].valid = settings.teleport[i+1].valid;
		settings.teleport[i].type = settings.teleport[i+1].type;
		strcpy(settings.teleport[i].name,settings.teleport[i+1].name);
		uip_ipaddr_copy(settings.teleport[i].ip,settings.teleport[i+1].ip);
		uip_ipaddr_copy(settings.teleport[i].mask,settings.teleport[i+1].mask);
		uip_ipaddr_copy(settings.teleport[i].gate,settings.teleport[i+1].gate);
	}
	settings.teleport[MAX_REMOTE_TLP-1].valid = 0;
	return 0;
}

/**
@function set_lldp_state
@param state - включение протокола LLDP
@return SETT_DIFF - настройки разные, SETT_EQUAL - настройки одинаковые, SETT_BAD - некорректные настройки
*/
int set_lldp_state(u8 state){
	if(state == 1 || state == 0){
		if(settings.lldp_settings.state != state){
			settings.lldp_settings.state = state;
			send_events_u32(EVENTS_LLDP_STATE,(u32)state);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

/**
@function set_lldp_transmit_interval
@param ti - установка Transmit Interval для протокола LLDP
@return SETT_DIFF - настройки разные, SETT_EQUAL - настройки одинаковые, SETT_BAD - некорректные настройки
*/
int set_lldp_transmit_interval(u8 ti){
	if(ti>=MIN_LLDP_TI || ti<=MAX_LLDP_TI){
		if(settings.lldp_settings.transmit_interval != ti){
			settings.lldp_settings.transmit_interval = ti;
			send_events_u32(EVENTS_LLDP_TI,(u32)ti);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}

/**
@function set_lldp_hold_multiplier
@param hm - установка Hold Multiplier для протокола LLDP
@return SETT_DIFF - настройки разные, SETT_EQUAL - настройки одинаковые, SETT_BAD - некорректные настройки
*/
int set_lldp_hold_multiplier(u8 hm){
	if(hm>=MIN_LLDP_HM || hm<=MAX_LLDP_HM){
		if(settings.lldp_settings.hold_multiplier != hm){
			settings.lldp_settings.hold_multiplier = hm;
			send_events_u32(EVENTS_LLDP_HM,(u32)hm);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;

}

int set_lldp_port_state(u8 port,u8 state){
	if(port<ALL_PORT_NUM && (state == 1 || state == 0)){
		if(settings.lldp_settings.port_state[port]!=state){
			settings.lldp_settings.port_state[port]=state;
			send_events_u32(EVENTS_LLDP_PORT1+port,(u32)state);
			return SETT_DIFF;
		}
		else
			return SETT_EQUAL;
	}
	else
		return SETT_BAD;
}





/***********************************************************************************************/
/*            методы get																	    */
/***********************************************************************************************/

void get_net_ip(uip_ipaddr_t *ip){
	if(ip == NULL)
		return;
	uip_ipaddr_copy(*ip,settings.net_sett.ip);
}

void get_net_mask(uip_ipaddr_t *ip){
	if(ip == NULL)
		return;
	uip_ipaddr_copy(*ip, settings.net_sett.mask);
}


void get_net_gate(uip_ipaddr_t *ip){
	if(ip == NULL)
		return;
	uip_ipaddr_copy(*ip, settings.net_sett.gate);
}

void get_net_dns(uip_ipaddr_t *ip){
	if(ip == NULL)
		return;
	uip_ipaddr_copy(*ip, settings.net_sett.dns);
}

void get_net_mac(u8 *mac){
	for(u8 i=0;i<6;i++)
		mac[i] = settings.net_sett.mac[i];
}

void get_net_def_mac(u8 *mac){
	for(u8 i=0;i<6;i++)
		mac[i] = settings.net_sett.default_mac[i];
}

//dhcp settings
u8 get_dhcp_mode(void){
	return settings.dhcp_sett.mode;
}

void get_dhcp_server_addr(uip_ipaddr_t *ip){
	if(ip == NULL)
		return;
	uip_ipaddr_copy(*ip,settings.dhcp_sett.server_addr);
}

u8 get_dhcp_hops(void){
	return settings.dhcp_sett.hop_limit;
}

u8 get_dhcp_opt82(void){
	return settings.dhcp_sett.opt82;
}


/*состояние dhcp relay*/
u8 get_dhcpr_state(void){
	if((settings.dhcp_sett.mode == DHCP_RELAY) || (settings.dhcp_sett.mode == DHCP_CLIENT))
		return 1;
	else
		return 0;
}

/*ограничение по hop, если 0, то нет ограничений*/
u8 get_dhcpr_hops(void){
	return settings.dhcp_sett.hop_limit;
}

/*использовать или нет option 82*/
u8 get_dhcpr_opt(void){
	return settings.dhcp_sett.opt82;
}


u8 get_gratuitous_arp_state(void){
	return settings.net_sett.grat_arp;
}

// port settings
u8 get_port_sett_state(u8 port){
	return settings.port_sett[port].state;
}

u8 get_port_sett_speed_dplx(u8 port){
	return settings.port_sett[port].speed_dplx;
}

u8 get_port_sett_flow(u8 port){
	return settings.port_sett[port].flow_ctrl;
}


u8 get_port_sett_wdt(u8 port){
	return settings.port_sett[port].wdt;
}

u16 get_port_sett_wdt_speed_down(u8 port){
	return settings.port_sett[port].wdt_speed_down;
}

u16 get_port_sett_wdt_speed_up(u8 port){
	return settings.port_sett[port].wdt_speed_up;
}

void get_port_sett_wdt_ip(u8 port,uip_ipaddr_t *ip){
	if(port<COOPER_PORT_NUM){
		if(ip == NULL)
			return;
		uip_ipaddr_copy(*ip,settings.port_sett[port].ip_dest);
	}
	else
		return;
}

u8 get_port_sett_soft_start(u8 port){
	if(port<COOPER_PORT_NUM){
		return settings.port_sett[port].soft_start;
	}
	else
		return -1;
}

u8 get_port_sett_poe(u8 port){
	if(port<COOPER_PORT_NUM){
		return settings.port_sett[port].poe_a_set;
	}
	else
		return -1;
}

u8 get_port_sett_poe_b(u8 port){
	if(port<COOPER_PORT_NUM){
		return settings.port_sett[port].poe_b_set;
	}
	else
		return -1;
}

u8 get_port_sett_pwr_lim_a(u8 port){
	if(port<COOPER_PORT_NUM){
		return settings.port_sett[port].poe_a_pwr_lim;
	}
	else
		return -1;
}

u8 get_port_sett_pwr_lim_b(u8 port){
	if(port<COOPER_PORT_NUM){
		return settings.port_sett[port].poe_b_pwr_lim;
	}
	else
		return -1;
}

u8 get_port_sett_sfp_mode(u8 port){
	if(port>=GE1){
		if((settings.port_sett[port].sfp_mode == 0)||(settings.port_sett[port].sfp_mode == 1)||(settings.port_sett[port].sfp_mode == 2))
			return settings.port_sett[port].sfp_mode;
		else
			return 0;
	}
	else
		return -1;
}


//настройки интерфейса
u8 get_interface_lang(void){
	return settings.interface_sett.lang;
}


/*void get_interface_login(char *str){
	if(strlen(settings.interface_sett.http_login64) < (HTTPD_MAX_LEN_PASSWD+1))
		strncpy(str,settings.interface_sett.http_login64,HTTPD_MAX_LEN_PASSWD+1);
}

void get_interface_passwd(char *str){
	if(strlen(settings.interface_sett.http_passwd64) < (HTTPD_MAX_LEN_PASSWD+1))
		strncpy(str,settings.interface_sett.http_passwd64,HTTPD_MAX_LEN_PASSWD+1);
}
*/


void get_interface_users_username(u8 num,char *str){
	if(num==0){
		//default users
		if(strlen(settings.interface_sett.http_login64) <= (HTTPD_MAX_LEN_PASSWD+1))
			strncpy(str,settings.interface_sett.http_login64,HTTPD_MAX_LEN_PASSWD+1);
	}
	else{
		//additional users
		if(strlen(settings.interface_sett.users[num-1].username) <= HTTPD_MAX_LEN_PASSWD)
			strcpy(str,settings.interface_sett.users[num-1].username);
	}
}



void get_interface_users_password(u8 num,char *str){
	if(num==0){
		//default users
		if(strlen(settings.interface_sett.http_passwd64) <= (HTTPD_MAX_LEN_PASSWD+1))
			strncpy(str,settings.interface_sett.http_passwd64,HTTPD_MAX_LEN_PASSWD+1);
	}
	else{
		//additional users
		if(strlen(settings.interface_sett.users[num-1].password) <= HTTPD_MAX_LEN_PASSWD)
			strcpy(str,settings.interface_sett.users[num-1].password);
	}
}




u8 get_interface_users_rule(u8 num){
	if(num==0){
		return ADMIN_RULE;
	}
	else{
		return settings.interface_sett.users[num-1].rule;
	}
}





u8 get_interface_period(void){
	return settings.interface_sett.refr_period;
}

void get_interface_name(char *str){
	strcpy(str,settings.interface_sett.system_name);
}

void get_interface_location(char *str){
	strcpy(str,settings.interface_sett.system_location);
}

void get_interface_contact(char *str){
	strcpy(str,settings.interface_sett.system_contact);
}

void get_port_descr(u8 port,char *str){
	if(port<ALL_PORT_NUM){
		strcpy(str,settings.port_sett[port].port_descr);
	}
}

u8 ext_vendor_name_flag(void){
	//return settings.interface_sett.ext_vendor_name_flag;
	return 0;
}

void get_ext_vendor_name(char *str){
	//strcpy(str,settings.interface_sett.ext_vendor_name);
}

//настройки smtp
u8 get_smtp_state(void){
	return settings.smtp_sett.state;
}

void get_smtp_server(uip_ipaddr_t *ip){
	if(ip == NULL)
		return;
	uip_ipaddr_copy(*ip,settings.smtp_sett.server_addr);
}

void get_smtp_to(char *str){
	strcpy(str,settings.smtp_sett.to);
}

void get_smtp_to2(char *str){
	strcpy(str,settings.smtp_sett.to2);
}

void get_smtp_to3(char *str){
	strcpy(str,settings.smtp_sett.to3);
}


void get_smtp_from(char *str){
	strcpy(str,settings.smtp_sett.from);
}

void get_smtp_subj(char *str){
	strcpy(str,settings.smtp_sett.subj);
}

void get_smtp_login(char *str){
	strcpy(str,settings.smtp_sett.login);
}

void get_smtp_pass(char *str){
	strcpy(str,settings.smtp_sett.pass);
}

u16 get_smtp_port(void){
	return settings.smtp_sett.port;
}
void get_smtp_domain(char *str){
	strcpy(str,settings.smtp_sett.domain_name);
}

//настройки sntp
u8 get_sntp_state(void){
	return settings.sntp_sett.state;
}

void get_sntp_serv(uip_ipaddr_t *ip){
	if(ip == NULL)
		return;
	uip_ipaddr_copy(*ip,settings.sntp_sett.addr);
}

void get_sntp_serv_name(char *name){
	if(name){
		strcpy(name,settings.sntp_sett.serv_name);
	}
}


i8 get_sntp_timezone(void){
	return settings.sntp_sett.timezone;
}

u8 get_sntp_period(void){
	return settings.sntp_sett.period;
}

//настройка syslog
u8 get_syslog_state(void){
	return settings.syslog_sett.state;
}


void get_syslog_serv(uip_ipaddr_t *ip){
	if(ip == NULL)
		return;
	uip_ipaddr_copy(*ip,settings.syslog_sett.server_addr);
}
/*u16 * get_syslog_serv(void){
	return settings.syslog_sett.server_addr;
}*/

//настройки event list
u8 get_event_base_s_st(void){
	if(settings.event_list.base_s & SSTATE)
		return 1;
	else
		return 0;
}
u8 get_event_base_s_level(void){
	return settings.event_list.base_s & SMASK;
}

u8 get_event_port_s_st(void){
	if(settings.event_list.port_s & SSTATE)
		return 1;
	else
		return 0;
}
u8 get_event_port_s_level(void){
	return settings.event_list.port_s & SMASK;
}


u8 get_event_vlan_s_st(void){
	if(settings.event_list.vlan_s & SSTATE)
		return 1;
	else
		return 0;
}
u8 get_event_vlan_s_level(void){
	return settings.event_list.vlan_s & SMASK;
}
u8 get_event_stp_s_st(void){
	if(settings.event_list.stp_s & SSTATE)
		return 1;
	else
		return 0;
}
u8 get_event_stp_s_level(void){
	return settings.event_list.stp_s & SMASK;
}

u8 get_event_qos_s_st(void){
	if(settings.event_list.qos_s & SSTATE)
		return 1;
	else
		return 0;
}
u8 get_event_qos_s_level(void){
	return settings.event_list.qos_s & SMASK;
}
u8 get_event_other_s_st(void){
	if(settings.event_list.ohter_s & SSTATE)
		return 1;
	else
		return 0;
}
u8 get_event_other_s_level(void){
	return settings.event_list.ohter_s & SMASK;
}

u8 get_event_port_link_t_st(void){
	if(settings.event_list.port_link_t & SSTATE)
		return 1;
	else
		return 0;
}
u8 get_event_port_link_t_level(void){
	return settings.event_list.port_link_t & SMASK;
}

u8 get_event_port_poe_t_st(void){
	if(settings.event_list.port_poe_t & SSTATE)
		return 1;
	else
		return 0;
}
u8 get_event_port_poe_t_level(void){
	return settings.event_list.port_poe_t & SMASK;
}

u8 get_event_port_stp_t_st(void){
	if(settings.event_list.stp_t & SSTATE)
		return 1;
	else
		return 0;
}
u8 get_event_port_stp_t_level(void){
	return settings.event_list.stp_t & SMASK;
}

u8 get_event_port_spec_link_t_st(void){
	if(settings.event_list.spec_link_t & SSTATE)
		return 1;
	else
		return 0;
}
u8 get_event_port_spec_link_t_level(void){
	return settings.event_list.spec_link_t & SMASK;
}

u8 get_event_port_spec_ping_t_st(void){
	if(settings.event_list.spec_ping_t & SSTATE)
		return 1;
	else
		return 0;
}

u8 get_event_port_spec_ping_t_level(void){
	return settings.event_list.spec_ping_t & SMASK;
}

u8 get_event_port_spec_speed_t_st(void){
	if(settings.event_list.spec_speed_t & SSTATE)
		return 1;
	else
		return 0;
}

u8 get_event_port_spec_speed_t_level(void){
	return settings.event_list.spec_speed_t & SMASK;
}

u8 get_event_port_system_t_st(void){
	if(settings.event_list.system_t & SSTATE)
		return 1;
	else
		return 0;
}

u8 get_event_port_system_t_level(void){
	return settings.event_list.system_t & SMASK;
}


u8 get_event_port_ups_t_st(void){
	if(settings.event_list.ups_t & SSTATE)
		return 1;
	else
		return 0;
}

u8 get_event_port_ups_t_level(void){
	return settings.event_list.ups_t & SMASK;
}

u8 get_event_port_alarm_t_st(void){
	if(settings.event_list.asc_t & SSTATE)
		return 1;
	else
		return 0;
}

u8 get_event_port_alarm_t_level(void){
	return settings.event_list.asc_t & SMASK;
}




u8 get_event_port_mac_t_st(void){
	if(settings.event_list.mac_filt_t & SSTATE)
		return 1;
	else
		return 0;
}
u8 get_event_port_mac_t_level(void){
	return settings.event_list.mac_filt_t & SMASK;
}

//настройка сухих контактов
u8 get_alarm_state(u8 num){
	if(num < NUM_ALARMS)
		return settings.alarm[num].state;
	else
		return 0;
}

u8 get_alarm_front(u8 num){
	if(num < NUM_ALARMS)
		return settings.alarm[num].front;
	else
		return 0;
}

//настройка ограничения скорости

u8 get_rate_limit_mode(u8 port){
	return settings.rate_limit[port].mode;
}


u8 get_uc_rate_limit(void){
	return settings.storm_control.uc;
}

u8 get_mc_rate_limit(void){
	return settings.storm_control.mc;
}

u8 get_bc_rate_limit(void){
	return settings.storm_control.bc;
}

u8 get_bc_limit(void){
	return settings.storm_control.limit;
}



u32 get_rate_limit_rx(u8 port){
	return settings.rate_limit[port].rx_rate;
}

u32 get_rate_limit_tx(u8 port){
	return settings.rate_limit[port].tx_rate;
}


//настройки qos для порта
u8 get_qos_port_cos_state(u8 port){
	return settings.qos_port_sett[port].cos_state;
}

u8 get_qos_port_tos_state(u8 port){
	return settings.qos_port_sett[port].tos_state;
}

u8 get_qos_port_rule(u8 port){
	return settings.qos_port_sett[port].rule;
}

u8 get_qos_port_def_pri(u8 port){
	return settings.qos_port_sett[port].def_pri;
}
//глобальные настройки qos
u8 get_qos_state(void){
	return settings.qos_sett.state;
}

u8 get_qos_policy(void){
	return settings.qos_sett.policy;
}

u8 get_qos_cos_queue(u8 cos1){
	return settings.qos_sett.cos[cos1];
}

u8 get_qos_tos_queue(u8 tos){
	return settings.qos_sett.tos[tos];
}

//igmp settings
u8 get_igmp_snooping_state(void){
	return 	settings.igmp_sett.igmp_snooping_state;
}

u8 get_igmp_query_mode(void){
	return settings.igmp_sett.igmp_query_mode;
}

/*
 * установить поддержку igmp snooping на порту**/
u8 get_igmp_port_state(u8 port){
	return settings.igmp_sett.port_state[port];
}

u16 get_igmp_query_int(void){
	return settings.igmp_sett.query_interval;
}

u8 get_igmp_max_resp_time(void){
	return settings.igmp_sett.max_resp_time;
}

u8 get_igmp_group_membership_time(void){
	return settings.igmp_sett.group_membship_time;
}


u16 get_igmp_other_querier_time(void){
	return settings.igmp_sett.other_querier_time;
}

/*
u8 get_igmp_group_num(void){
	return settings.igmp_sett.group_num;
}

u8 get_igmp_group_list_active(u8 num){
	return settings.igmp_group_list[num].active;
}

void get_igmp_group_list_ip(u8 num,uip_ipaddr_t *ip){
	if(ip == NULL)
		return;
	uip_ipaddr_copy(*ip,settings.igmp_group_list[num].ip);
}

u8 get_igmp_group_list_port(u8 num,u8 port_num){
	return settings.igmp_group_list[num].port_flag[port_num];
}
*/




//port base vlan
u8 get_pb_vlan_state(void){
	return settings.pb_vlan_sett.state;
}


//u16 get_pb_vlan_port(u8 port){
//
//	if(port<PORT_NUM){
//		return settings.pb_vlan_sett.table[L2F_port_conv(port)];
//	}
//	else
//		return -1;
//}

u8 get_pb_vlan_port(u8 rx_port,u8 tx_port){
	if((rx_port<ALL_PORT_NUM)&&(tx_port<ALL_PORT_NUM)){
		if(settings.pb_vlan_sett.table[rx_port][tx_port])
			return 1;
		else
			return 0;
	}
	else
		return -1;
}

void get_pb_vlan_table(struct pb_vlan_t *pb){
	for(uint8_t i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
		for(u8 j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++){
			if(settings.pb_vlan_sett.table[i][j])
				pb->VLANTable[i][j]=1;
			else
				pb->VLANTable[i][j]=0;
		}
	}
}

//port based vlan for swu-16
u8 get_pb_vlan_swu_port(u8 port){
	if(port<ALL_PORT_NUM)
		return settings.pb_vlan_swu_sett.port_vid[port];
	else
		return 0;
}


//настройки vlan
u8 get_vlan_sett_state(void){
	return 1;
	//return settings.vlan_sett.state;
}
u16 get_vlan_sett_mngt(void){
	return settings.vlan_sett.mngvid;
}

u8 get_vlan_trunk_state(void){
	return settings.vlan_sett.vlan_trunk_state;
}


u8 get_vlan_sett_port_state(u8 port){
	if(port<ALL_PORT_NUM){
		return settings.vlan_sett.port_st[port];
	}
	else
		return 0;
}

u16 get_vlan_sett_dvid(u8 port){
	if(port<ALL_PORT_NUM){
		return settings.vlan_sett.dvid[port];
	}
	else
		return 0;
}

u16 get_vlan_sett_vlannum(void){
	return settings.vlan_sett.VLANNum;
}
//vlan`ы
u8 get_vlan_state(u8 num){
	return settings.vlan[num].state;
}

//num -> vid
u16 get_vlan_vid(u8 num){
	return settings.vlan[num].VID;
}

//vid->num
int vlan_vid2num(u16 vid){
	for(u16 i = 0;i<get_vlan_sett_vlannum();i++){
		if(get_vlan_vid(i) == vid)
			return i;
	}
	return -1;
}


char *get_vlan_name(u8 num){
	return settings.vlan[num].VLANNAme;
}
u8 get_vlan_port_state(u8 num,u8 port){
	return settings.vlan[num].Ports[port];
}

//настройки stp
u8 get_stp_state(void){
	return settings.stp_sett.state;
}
u16 get_stp_magic(void){
	return settings.stp_sett.magic;
}
u8 get_stp_proto(void){
	return settings.stp_sett.proto;
}

u16 get_stp_bridge_priority(void){
	return settings.stp_sett.bridge_priority;
}
u8 get_stp_bridge_max_age(void){
	return settings.stp_sett.bridge_max_age;
}
u8 get_stp_bridge_htime(void){
	return settings.stp_sett.bridge_htime;
}
u8 get_stp_bridge_fdelay(void){
	return settings.stp_sett.bridge_fdelay;
}

u8 get_stp_bridge_mdelay(void){
	return settings.stp_sett.bridge_mdelay;
}

u8 get_stp_txholdcount(void){
	return settings.stp_sett.txholdcount;
}
//настройки stp port
u8 get_stp_port_enable(u8 port){
	if(port<ALL_PORT_NUM){
		return settings.stp_port_sett[port].enable;
	}
	else
		return 0;
}
u8 get_stp_port_state(u8 port){
	if(port<ALL_PORT_NUM){
		return settings.stp_port_sett[port].state;
	}
	else
		return 0;
}
u8 get_stp_port_priority(u8 port){
	if(port<ALL_PORT_NUM){
		return settings.stp_port_sett[port].priority;
	}
	return 0;
}

u32 get_stp_port_cost(u8 port){
	if(port<ALL_PORT_NUM){
		return settings.stp_port_sett[port].path_cost;
	}
	else
		return 0;
}

u8 get_stp_port_autocost(u8 port){
	if(settings.stp_port_sett[port].flags & BSTP_PORTCFG_FLAG_ADMCOST)
		return 1;
	else
		return 0;
}

u8 get_stp_port_autoedge(u8 port){
	if(settings.stp_port_sett[port].flags & BSTP_PORTCFG_FLAG_AUTOEDGE)
		return 1;
	else
		return 0;
}

u8 get_stp_port_edge(u8 port){
	if(settings.stp_port_sett[port].flags & BSTP_PORTCFG_FLAG_EDGE)
		return 1;
	else
		return 0;
}

u8 get_stp_port_autoptp(u8 port){
	if(settings.stp_port_sett[port].flags & BSTP_PORTCFG_FLAG_AUTOPTP)
		return 1;
	else
		return 0;
}

u8 get_stp_port_ptp(u8 port){
	if(settings.stp_port_sett[port].flags & BSTP_PORTCFG_FLAG_PTP)
		return 1;
	else
		return 0;
}

u8 get_stp_port_flags(u8 port){
	if(port<ALL_PORT_NUM){
		return settings.stp_port_sett[port].flags;
	}
	else
		return 0;
}


//stp BPDU forwarding
u8 get_stp_bpdu_fw(void){
	return settings.stp_forward_bpdu.state;
}


//настройки калибровки кабельного тестера
//пары 1-2
u16 get_callibrate_koef_1(u8 port){
	return settings.port_callibrate[port].koeff_1;
}

//пары 3-6
u16 get_callibrate_koef_2(u8 port){
	return settings.port_callibrate[port].koeff_2;
}

u8 get_callibrate_len(u8 port){
	return settings.port_callibrate[port].length;
}
//настройки snmp
u8 get_snmp_state(void){
	return settings.snmp_sett.state;
}

u8 get_snmp_mode(void){
	return settings.snmp_sett.mode;
}


void get_snmp_serv(uip_ipaddr_t *ip){
	if(ip == NULL)
		return;
	uip_ipaddr_copy(*ip,settings.snmp_sett.server_addr);
}

u8 get_snmp_vers(void){
	return settings.snmp_sett.version;
}

//void get_snmp_communitie(char *str){
//	strcpy(str,settings.snmp_sett.snmp_commun[0].community);
//}

void get_snmp1_read_communitie(char *str){
	strcpy(str,settings.snmp_sett.snmp1_read_commun.community);
}

void get_snmp1_write_communitie(char *str){
	strcpy(str,settings.snmp_sett.snmp1_write_commun.community);
}

//SNMP v3 part
int get_snmp3_level(u8 usernum){
	if(usernum<NUM_USER)
		return settings.snmp_sett.user[usernum].level;
	else
		return -1;
}

int get_snmp3_user_name(u8 usernum, char *str){
	if(usernum<NUM_USER){
		strcpy(str,settings.snmp_sett.user[usernum].user_name);
		return 0;
	}
	else
		return -1;
}

int get_snmp3_auth_pass(u8 usernum, char *str){
	if(usernum<NUM_USER){
		strcpy(str,settings.snmp_sett.user[usernum].auth_pass);
		return 0;
	}
	else
		return -1;
}

int get_snmp3_priv_pass(u8 usernum, char *str){
	if(usernum<NUM_USER){
		strcpy(str,settings.snmp_sett.user[usernum].priv_pass);
		return 0;
	}
	else
		return -1;
}

int get_snmp3_engine_id(engine_id_t *eid){
	if(eid->len>64)
		eid->len = 0;
	eid->len = settings.snmp_sett.engine_id.len;
	for(u8 i=0;i<64;i++)
		eid->ptr[i] = settings.snmp_sett.engine_id.ptr[i];
	return 0;
}



u16 get_softstart_time(void){
	return settings.sstart_time;
}

u8 get_telnet_state(void){
	return settings.telnet_sett.state;
}

u8 get_telnet_echo(void){
	return settings.telnet_sett.echo;
}

u8 get_telnet_rn(void){
	return TELNET_RN;//settings.telnet_sett.rn;
}


u8 get_downshifting_mode(void){
	return settings.downshift_mode;
}

//tftp
u8 get_tftp_state(void){
	return settings.tftp_sett.state;
}
u8 get_tftp_mode(void){
	return settings.tftp_sett.mode;
}
u16 get_tftp_port(void){
	return settings.tftp_sett.port;
}

u8 get_plc_out_state(u8 channel){
	if(channel<PLC_RELAY_OUT){
		return settings.plc.out_state[channel];
	}
	else
		return DISABLE;
}

//состояние выхода при срабатывании правила
u8 get_plc_out_action(u8 channel){
	if(channel<PLC_RELAY_OUT){
		return settings.plc.out_action[channel];
	}
	else
		return DISABLE;
}

u8 get_plc_out_event(u8 channel, u8 event){
	if(channel<PLC_RELAY_OUT){
		if(event<PLC_EVENTS){
			return settings.plc.out_event[channel][event];
		}
		else
			return 0;
	}
	else
		return 0;
}

u8 get_plc_in_state(u8 channel){
	if(channel<PLC_INPUTS){
		return settings.plc.in_state[channel];
	}
	else
		return 0;
}

u8 get_plc_in_alarm_state(u8 channel){
	if(channel<PLC_INPUTS){
		return settings.plc.in_alarm_state[channel];
	}
	else
		return 0;
}


u16 get_plc_em_model(void){
	return settings.plc.em_model;
}

u8 get_plc_em_rate(void){
	return settings.plc.em_baudrate;
}

u8 get_plc_em_parity(void){
	return settings.plc.em_parity;
}

u8 get_plc_em_databits(void){
	return settings.plc.em_databits;
}

u8 get_plc_em_stopbits(void){
	return settings.plc.em_stopbits;
}

void get_plc_em_pass(char *pass){
	if(pass != NULL){
		strcpy(pass,settings.plc.em_pass);
	}
}

void get_plc_em_id(char *id){
	if(id != NULL){
		strcpy(id,settings.plc.em_id);
	}
}




//mac filtering state
u8 get_mac_filter_state(u8 port){
	if(port<ALL_PORT_NUM)
		return settings.port_sett[port].mac_filtering;
	else
		return 0;
}



u8 get_mac_learn_cpu(void){
	return settings.cpu_mac_learning;
}

//число записей в mac bind table
u8 get_mac_bind_num(void){
u8 num = 0;
	for(u8 i=0;i<MAC_BIND_MAX_ENTRIES;i++){
		if(settings.mac_bind_entries[i].active)
			num++;
	}
	return num;
}

//получаем запись из таблицы с MAC адресом
u8 get_mac_bind_entry_mac(u8 entry,u8 position){
	if(entry<MAC_BIND_MAX_ENTRIES && position<6)
		return settings.mac_bind_entries[entry].mac[position];
	else
		return 0;
}

u8 get_mac_bind_entry_port(u8 entry){
	if(entry<MAC_BIND_MAX_ENTRIES)
		return settings.mac_bind_entries[entry].port;
	else
		return 0;
}

u8 get_mac_bind_entry_active(u8 entry){
	if(entry<MAC_BIND_MAX_ENTRIES)
		return settings.mac_bind_entries[entry].active;
	else
		return 0;
}

u8 get_ups_delayed_start(void){
	return settings.ups.delayed_start;
}


u8 get_lag_valid(u8 index){
	if(index<LAG_MAX_ENTRIES){
		return settings.lag_entry[index].valid_flag;
	}
	else
		return 0;
}

u8 get_lag_state(u8 index){
	if(index<LAG_MAX_ENTRIES){
			return settings.lag_entry[index].state;
	}
	else
		return 0;
}

u8 get_lag_master_port(u8 index){
	if(index<LAG_MAX_ENTRIES){
		return settings.lag_entry[index].master_port;
	}
	else
		return 0;
}

u8 get_lag_id(u8 index){
	return index+1;
}

u8 get_lag_port(u8 index,u8 port){
	if(index<LAG_MAX_ENTRIES && port<PORT_NUM){
		return settings.lag_entry[index].ports[port];
	}
	else
		return 0;
}

u8 get_lag_entries_num(void){
u8 num = 0;
	for(u8 i=0;i<LAG_MAX_ENTRIES;i++){
		if(settings.lag_entry[i].valid_flag)
			num++;
	}
	return num;
}



u8 get_mirror_state(void){
	return settings.mirror_entry.state;
}

u8 get_mirror_target_port(void){
	return settings.mirror_entry.target_port;
}

u8 get_mirror_port(u8 port){
	if(port<PORT_NUM){
		return settings.mirror_entry.ports[port];
	}
	else
		return 0;
}


//teleport - inputs
u8 get_input_state(u8 num){
	if(num < MAX_INPUT_NUM)
		return settings.input[num].state;
	return 0;
}

//
u8 get_input_rem_dev(u8 num){
	if(num < MAX_INPUT_NUM)
		return settings.input[num].rem_dev;
	return 0;
}

//
u8 get_input_rem_port(u8 num){
	if(num < MAX_INPUT_NUM)
		return settings.input[num].rem_port;
	return 0;
}

u8 get_input_inverse(u8 input){
	if(input < MAX_INPUT_NUM)
		return settings.input[input].inverse;
	return 0;
}

//teleport - events
u8 get_tlp_event_state(u8 num){
	if(num < MAX_TLP_EVENTS_NUM)
		return settings.tlp_events[num].state;
	return 0;
}

//
u8 get_tlp_event_rem_dev(u8 num){
	if(num < MAX_TLP_EVENTS_NUM)
		return settings.tlp_events[num].rem_dev;
	return 0;
}

//
u8 get_tlp_event_rem_port(u8 num){
	if(num < MAX_TLP_EVENTS_NUM)
		return settings.tlp_events[num].rem_port;
	return 0;
}

u8 get_tlp_event_inverse(u8 num){
	if(num < MAX_TLP_EVENTS_NUM)
		return settings.tlp_events[num].inverse;
	return 0;
}


//teleport - remote devices
u8 get_tlp_remdev_valid(u8 num){
	if(num < MAX_REMOTE_TLP)
		return settings.teleport[num].valid;
	else
		return 0;
}


void get_tlp_remdev_ip(u8 num,uip_ipaddr_t *ip){
	if(num < MAX_REMOTE_TLP)
		uip_ipaddr_copy(*ip,settings.teleport[num].ip);

}

void get_tlp_remdev_mask(u8 num,uip_ipaddr_t *mask){
	if(num < MAX_REMOTE_TLP)
		uip_ipaddr_copy(*mask,settings.teleport[num].mask);
}

void get_tlp_remdev_mask_default(uip_ipaddr_t *mask){
	get_net_mask(mask);
}

void get_tlp_remdev_gate(u8 num,uip_ipaddr_t *gate){
	if(num < MAX_REMOTE_TLP)
		uip_ipaddr_copy(*gate,settings.teleport[num].gate);
}

void get_tlp_remdev_gate_default(uip_ipaddr_t *gate){
	get_net_gate(gate);
}

void get_tlp_remdev_name(u8 num,char *name){
	if(num < MAX_REMOTE_TLP)
		strncpy(name,settings.teleport[num].name,64);
}

u8 get_tlp_remdev_type(u8 num){
	if(num < MAX_REMOTE_TLP)
		return settings.teleport[num].type;
	else
		return 0;
}


//возвращаем индекс первого свободного места
u8 get_tlp_remdev_last(void){
	for(u8 i=0;i<MAX_REMOTE_TLP;i++){
		if(settings.teleport[i].valid != 1)
			return i;
	}
	return -1;
}





//число удалённых устройств в списке
u8 get_remdev_num(void){
u8 cnt = 0;
	for(u8 i=0;i<MAX_REMOTE_TLP;i++){
		if(settings.teleport[i].valid==1)
			cnt++;
	}
	return cnt;
}


u8 get_mv_freeze_ctrl_state(void){
	/*if(settings.mv_freeze_ctrl == 0 || settings.mv_freeze_ctrl==1){
		return settings.mv_freeze_ctrl;
	}
	else*/
		return 0;
}


u8 get_lldp_state(void){
	return settings.lldp_settings.state;
}

u8 get_lldp_transmit_interval(void){
	return settings.lldp_settings.transmit_interval;
}

u8 get_lldp_port_state(u8 port){
	return settings.lldp_settings.port_state[port];
}

u8 get_lldp_hold_multiplier(void){
	return settings.lldp_settings.hold_multiplier;
}

//полная инициализация структуры настроек
//загружаем дефолтные параметры, а потом если надо, заменим их
int settings_struct_initialization(u8 flag){
	uip_ipaddr_t ip;
	char tmp[64];
	memset(&dev,0,sizeof(dev));
	uip_ipaddr(ip,0,0,0,0);
	memset(tmp,0,64);

	if(!(flag & DEFAULT_KEEP_NTWK)){

		uip_ipaddr(ip,192,168,0,1);
		settings.net_sett.ip[0]=ip[0];
		settings.net_sett.ip[1]=ip[1];

		uip_ipaddr(ip,255,255,255,0);
		settings.net_sett.mask[0]=ip[0];
		settings.net_sett.mask[1]=ip[1];

		uip_ipaddr(ip,255,255,255,255);
		settings.net_sett.gate[0]=ip[0];
		settings.net_sett.gate[1]=ip[1];

		uip_ipaddr(ip,255,255,255,255);
		settings.net_sett.dns[0]=ip[0];
		settings.net_sett.dns[1]=ip[1];

		settings.net_sett.mac[0]=settings.net_sett.default_mac[0];
		settings.net_sett.mac[1]=settings.net_sett.default_mac[1];
		settings.net_sett.mac[2]=settings.net_sett.default_mac[2];
		settings.net_sett.mac[3]=settings.net_sett.default_mac[3];
		settings.net_sett.mac[4]=settings.net_sett.default_mac[4];
		settings.net_sett.mac[5]=settings.net_sett.default_mac[5];
		settings.net_sett.grat_arp = 1;
		settings.dhcp_sett.mode = 0;
		settings.dhcp_sett.server_addr[0]=0;
		settings.dhcp_sett.server_addr[1]=0;
		settings.dhcp_sett.hop_limit = 4;
		settings.dhcp_sett.opt82 = 1;
		settings.dhcp_sett.start_ip[0]=settings.dhcp_sett.start_ip[1]=0;
		settings.dhcp_sett.end_ip[0]=settings.dhcp_sett.end_ip[1]=0;
		settings.dhcp_sett.gate[0]=settings.dhcp_sett.gate[1]=0;
		settings.dhcp_sett.mask[0]=settings.dhcp_sett.mask[1]=0;
		settings.dhcp_sett.dns[0]=settings.dhcp_sett.dns[1]=0;
		settings.dhcp_sett.lease_time = 0;
	}

	if(!(flag & DEFAULT_KEEP_PASS)){
		strncpy(settings.interface_sett.http_login64,tmp,64);
		strncpy(settings.interface_sett.http_passwd64,tmp,64);

		for(u8 i=0;i<(MAX_USERS_NUM-1);i++){
			strncpy(settings.interface_sett.users[i].username,tmp,64);
			strncpy(settings.interface_sett.users[i].password,tmp,64);
			settings.interface_sett.users[i].rule = NO_RULE;
		}
	}

	if(!(flag & DEFAULT_KEEP_STP)){
		settings.stp_sett.state = DISABLE;
		settings.stp_sett.magic = BSTP_CFG_MAGIC;
		settings.stp_sett.proto = BSTP_PROTO_RSTP;
		settings.stp_sett.bridge_priority = /*BSTP_DEFAULT_BRIDGE_PRIORITY*/32768;
		settings.stp_sett.bridge_max_age = BSTP_DEFAULT_MAX_AGE / BSTP_TICK_VAL;
		settings.stp_sett.bridge_htime = BSTP_DEFAULT_HELLO_TIME / BSTP_TICK_VAL;
		settings.stp_sett.bridge_fdelay = BSTP_DEFAULT_FORWARD_DELAY / BSTP_TICK_VAL;
		settings.stp_sett.txholdcount = BSTP_DEFAULT_HOLD_COUNT;
		settings.stp_sett.bridge_mdelay = BSTP_DEFAULT_MIGRATE_DELAY / BSTP_TICK_VAL;
		for(int i=0;i<NUM_BSTP_PORT;i++){
			if(get_dev_type()==DEV_SWU16)
				settings.stp_port_sett[i].path_cost = BSTP_DEFAULT_PATH_COST/10;
			else{
				if((i==GE1)||(i==GE2))
					settings.stp_port_sett[i].path_cost = BSTP_DEFAULT_PATH_COST/10;
				else{
					settings.stp_port_sett[i].path_cost = BSTP_DEFAULT_PATH_COST;
					//special for psw-1G
					if((i == (COOPER_PORT_NUM-1))&&(((get_dev_type()==DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)))){
						settings.stp_port_sett[i].path_cost = BSTP_DEFAULT_PATH_COST/10;
					}
				}
			}
			settings.stp_port_sett[i].enable = ENABLE;
			settings.stp_port_sett[i].state = ENABLE;
			settings.stp_port_sett[i].priority = BSTP_DEFAULT_PORT_PRIORITY;
			settings.stp_port_sett[i].flags = 0;
			set_stp_port_autocost(i,0);
			set_stp_port_autoedge(i,1);
			set_stp_port_autoptp(i,1);
		}
		settings.stp_forward_bpdu.state = ENABLE;
	}


	for(u8 i=0;i<(PORT_NUM);i++){
		settings.port_sett[i].state = 1;
		settings.port_sett[i].speed_dplx = SPEED_AUTO;
		settings.port_sett[i].flow_ctrl = DISABLE;
		settings.port_sett[i].wdt = WDT_NONE;
		settings.port_sett[i].ip_dest[0]=0;
		settings.port_sett[i].ip_dest[1]=0;
		settings.port_sett[i].wdt_speed_down=0;
		settings.port_sett[i].wdt_speed_up=0;
		settings.port_sett[i].soft_start = DISABLE;
		settings.port_sett[i].poe_a_set = POE_AUTO;
		settings.port_sett[i].poe_b_set = POE_AUTO;
		settings.port_sett[i].poe_a_pwr_lim = 0;
		settings.port_sett[i].poe_b_pwr_lim = 0;
		settings.port_sett[i].sfp_mode = 1;//auto
		settings.port_sett[i].mac_filtering = 0;
	}
	settings.interface_sett.lang = ENG;

	settings.interface_sett.refr_period = 0;
	strncpy(settings.interface_sett.system_name,tmp,DESCRIPT_LEN);
	strncpy(settings.interface_sett.system_location,tmp,DESCRIPT_LEN);
	strncpy(settings.interface_sett.system_contact,tmp,DESCRIPT_LEN);
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		strncpy(settings.port_sett[i].port_descr,tmp,PORT_DESCR_LEN);
	}

	settings.smtp_sett.state=DISABLE;
	settings.smtp_sett.aslog=1;
	settings.smtp_sett.server_addr[0]=settings.smtp_sett.server_addr[1]=0;
	strncpy(settings.smtp_sett.domain_name,tmp,32);
	strncpy(settings.smtp_sett.to,tmp,32);
	strncpy(settings.smtp_sett.to2,tmp,32);
	strncpy(settings.smtp_sett.to3,tmp,32);
	strncpy(settings.smtp_sett.from,tmp,64);
	strcpy(settings.smtp_sett.subj,"TFortis");

	memset(tmp,0,64);
	strncpy(settings.smtp_sett.login,tmp,32);
	strncpy(settings.smtp_sett.pass,tmp,32);
	settings.smtp_sett.port = 25;

	settings.snmp_sett.state = DISABLE;
	settings.snmp_sett.mode = SNMP_R;
	settings.snmp_sett.server_addr[0]=settings.snmp_sett.server_addr[1]=0;
	settings.snmp_sett.version = SNMP__VERSION_1;

	strcpy(tmp, "public");
	strncpy(settings.snmp_sett.snmp1_read_commun.community,tmp,16);
	settings.snmp_sett.snmp1_read_commun.privelege = SNMP_R;
	memset(tmp,0,64);

	strcpy(tmp, "private");
	strncpy(settings.snmp_sett.snmp1_write_commun.community,tmp,16);
	settings.snmp_sett.snmp1_write_commun.privelege = SNMP_RW;
	memset(tmp,0,64);

	settings.snmp_sett.engine_id.ptr[0] = 0;
	settings.snmp_sett.engine_id.len = 0;

	for(u8 i=0;i<NUM_USER;i++){
		settings.snmp_sett.user[i].level = NO_AUTH_NO_PRIV;
		strncpy(settings.snmp_sett.user[i].user_name,tmp,64);
		strncpy(settings.snmp_sett.user[i].auth_pass,tmp,64);
		strncpy(settings.snmp_sett.user[i].priv_pass,tmp,64);
	}

	settings.sntp_sett.state = DISABLE;
	settings.sntp_sett.addr[0]=settings.sntp_sett.addr[1]=0;
	for(u8 i=0;i<MAX_SERV_NAME;i++)
		settings.sntp_sett.serv_name[i] = 0;
	settings.sntp_sett.timezone=0;
	settings.sntp_sett.period = 10;

	settings.syslog_sett.state = DISABLE;
	settings.syslog_sett.server_addr[0]=settings.syslog_sett.server_addr[1]=0;

	settings.event_list.base_s = 4;
	settings.event_list.port_s = 4;
	settings.event_list.vlan_s = 4;
	settings.event_list.stp_s = 4;
	settings.event_list.qos_s = 4;
	settings.event_list.ohter_s = 4;
	settings.event_list.port_link_t = 12;
	settings.event_list.port_poe_t = 12;
	settings.event_list.stp_t = 4;
	settings.event_list.spec_link_t = 12;
	settings.event_list.spec_ping_t = 12;
	settings.event_list.spec_speed_t = 12;
	settings.event_list.system_t = 4;
	settings.event_list.ups_t = 12;
	settings.event_list.asc_t = 12;
	settings.event_list.mac_filt_t = 4;

	for(u8 i=0;i<NUM_ALARMS;i++){
		settings.alarm[i].state = ENABLE;
		settings.alarm[i].front = FAULING;
	}

	for(u8 i=0;i<(PORT_NUM);i++){
		settings.rate_limit[i].mode = DISABLE;
		settings.rate_limit[i].rx_rate = 0;
 		settings.rate_limit[i].tx_rate = 0;

 		settings.qos_port_sett[i].cos_state = ENABLE;
 		settings.qos_port_sett[i].tos_state = ENABLE;
 		settings.qos_port_sett[i].rule = 1;
 		settings.qos_port_sett[i].def_pri = 0;
	}

	//storm control - ограничение неизвестного трафика в SWU-16
	settings.storm_control.uc = 1;
	settings.storm_control.mc = 1;
	settings.storm_control.bc = 1;
	settings.storm_control.limit = 80;

	//qos main sett
	settings.qos_sett.policy = WEIGHTED_FAIR_PRI;
	settings.qos_sett.state = 0;
	settings.qos_sett.unicast_rl = DISABLE;
	for(u8 i=0;i<8;i++){
		settings.qos_sett.cos[0]=1;
		settings.qos_sett.cos[1]=0;
		settings.qos_sett.cos[2]=0;
		settings.qos_sett.cos[3]=1;
		settings.qos_sett.cos[4]=2;
		settings.qos_sett.cos[5]=2;
		settings.qos_sett.cos[6]=3;
		settings.qos_sett.cos[7]=3;
	}
	for(u8 j=0;j<64;j++){
		if(j<=7){
		   settings.qos_sett.tos[j]=0;
		}
		if((j>=8)&&(j<=31)){
		   settings.qos_sett.tos[j]=1;
		}
		if((j>=32)&&(j<=55)){
		   settings.qos_sett.tos[j]=2;
		}
		if((j>=56)&&(j<=63)){
		  settings.qos_sett.tos[j]=3;
		}
	}


	settings.igmp_sett.igmp_snooping_state = 0;
	settings.igmp_sett.igmp_query_mode = 1;//send query
	for(u8 i=0;i<(PORT_NUM);i++)
		settings.igmp_sett.port_state[i]=1;

	settings.igmp_sett.query_interval = 60;
	settings.igmp_sett.max_resp_time = 10;
	settings.igmp_sett.group_membship_time = 250;
	settings.igmp_sett.other_querier_time = 255;


	settings.pb_vlan_sett.state = DISABLE;
	for(u8 i=0;i<PORT_NUM;i++)
		for(u8 j=0;j<PORT_NUM;j++)
			settings.pb_vlan_sett.table[i][j] = 1;

	for(u8 i=0;i<PORT_NUM;i++)
		settings.pb_vlan_swu_sett.port_vid[i] = 1;

	settings.vlan_sett.state = ENABLE;
	settings.vlan_sett.mngvid = 1;
	settings.vlan_sett.VLANNum = 1;
	settings.vlan_sett.vlan_trunk_state = DISABLE;

	settings.vlan[0].VID = 1;
	settings.vlan[0].state = ENABLE;
	set_vlan_name(0,"default");

	for(u8 i=1;i<MAXVlanNum;i++){
		settings.vlan[i].VID = 0;
		settings.vlan[i].state = 0;
		set_vlan_name(i," ");
		for(u8 j=0;j<(ALL_PORT_NUM);j++){
			settings.vlan[i].Ports[j] = 0;
		}
	}

	for(u8 i=0;i<(PORT_NUM);i++){
		settings.vlan_sett.port_st[i] = GT_SECURE;
		settings.vlan_sett.dvid[i] = 1;
		settings.vlan[0].Ports[i] = 2;//untagged

		settings.port_callibrate[i].koeff_1 = 100;
		settings.port_callibrate[i].koeff_2 = 100;
		settings.port_callibrate[i].length = 0;
	}
	settings.sstart_time = 1;

	settings.telnet_sett.state = 1;
	settings.telnet_sett.echo = 1;

	settings.tftp_sett.state = 0;
	settings.tftp_sett.mode = 0;
	settings.tftp_sett.port = 69;

	settings.downshift_mode = 1;

	settings.plc.em_model=0;
	settings.plc.em_baudrate=5;//9600
	settings.plc.em_parity=NO_PARITY;
	settings.plc.em_databits=8;
	settings.plc.em_stopbits=1;
	strcpy(settings.plc.em_id,"www.energomera.ru");
	strcpy(settings.plc.em_pass,"777777");
	memset(settings.plc.em_pass,0,PLC_EM_MAX_PASS);

	for(u8 i=0;i<PLC_INPUTS;i++){
		settings.plc.in_state[i] = ENABLE;
	}
	for(u8 i=0;i<PLC_RELAY_OUT;i++){
		settings.plc.out_state[i] = DISABLE;
		settings.plc.out_action[i] = 0;
		for(u8 j=0;j<(PLC_EVENTS);j++)
			settings.plc.out_event[i][j] = 0;
	}

	//mac binding table
	for(u8 i=0;i<MAC_BIND_MAX_ENTRIES;i++){
		settings.mac_bind_entries[i].active = 0;
		for(u8 j=0;j<6;j++)
			settings.mac_bind_entries[i].mac[j]=0;
		settings.mac_bind_entries[i].port = 0;
	}
	//cpu addr learning
	settings.cpu_mac_learning = 1;

	//ups settings
	settings.ups.delayed_start = 1;

	//link agregation
	for(u8 i=0;i<LAG_MAX_ENTRIES;i++){
		settings.lag_entry[i].valid_flag = 0;
		settings.lag_entry[i].state = 0;
		settings.lag_entry[i].master_port = 0;
		for(u8 j=0;j<PORT_NUM;j++){
			settings.lag_entry[i].ports[j] = 0;
		}
	}

	//port mirroring
	settings.mirror_entry.state = 0;
	settings.mirror_entry.target_port = 0;
	for(u8 j=0;j<PORT_NUM;j++){
		settings.mirror_entry.ports[j] = 0;
	}

	//teleport
	for(u8 j=0;j<MAX_REMOTE_TLP;j++){
		settings.teleport[j].valid = 0;
		settings.teleport[j].type = 0;
		memset(settings.teleport[j].name,0,64);
		settings.teleport[j].ip[0]=settings.teleport[j].ip[1]=0;
		settings.teleport[j].mask[0]=settings.teleport[j].mask[1]=0;
		settings.teleport[j].gate[0]=settings.teleport[j].gate[1]=0;
	}
	for(u8 j=0;j<MAX_INPUT_NUM;j++){
		settings.input[j].state = 0;
		memset(settings.input[j].name,0,64);
		settings.input[j].rem_dev = MAX_REMOTE_TLP;
		settings.input[j].rem_port = MAX_OUTPUT_NUM;
		settings.input[j].inverse = 0;
	}
	for(u8 j=0;j<MAX_TLP_EVENTS_NUM;j++){
		settings.tlp_events[j].state = 0;
		settings.tlp_events[j].rem_dev = MAX_REMOTE_TLP;
		settings.tlp_events[j].rem_port = MAX_OUTPUT_NUM;
		settings.tlp_events[j].inverse = 0;
	}
	settings.mv_freeze_ctrl = 0;

	settings.lldp_settings.state = 0;
	settings.lldp_settings.hold_multiplier = 4;
	settings.lldp_settings.transmit_interval = 30;
	for(u8 j=0;j<ALL_PORT_NUM;j++)
		settings.lldp_settings.port_state[j] = 1;

	return 0;
}

int settings_load(void){
	need_load_settings_=1;
	return 0;
}

u8 need_load_settings(void){
	return need_load_settings_;
}

void need_load_settings_set(u8 state){
	need_load_settings_ = state;
}

int settings_save(void){
	need_save_settings_=1;
	return 0;
}

u8 need_save_settings(void){
	return need_save_settings_;
	return 0;
}

void need_save_settings_set(u8 state){
	need_save_settings_ = state;
}

void settings_default(u8 flag){
	send_events_u32(EVENTS_DEFAULT,0);
	settings_struct_initialization(flag);
	settings_save_();
}


u8 settings_is_loaded(void){
	return settings_is_loaded_;
}

void settings_loaded(u8 state){
	settings_is_loaded_ = state;
}

