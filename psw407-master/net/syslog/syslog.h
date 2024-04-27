 #ifndef SYSLOG_H
 #define SYSLOG_H
//#include <stdio.h>
//#include "stm32f10x.h"
//#include "FreeRTOS.h"
//#include "task.h"

 //#include <lwip/netif.h>
 //#include <lwip/ip_addr.h>



u8 syslog_send_flag;

#define if_syslog_ready()  syslog_send_flag == 1
#define syslog_ready()  syslog_send_flag = 1
#define syslog_empty()  syslog_send_flag = 0


typedef struct ip_addr {
  uint8_t addr[4];
}ip_addr;
/* старая структура до прошивки 0,2,0  ***
struct Syslog {
	uint8_t state;
	uint8_t servIP[4];
	//warning
	uint8_t Link_warning;//
	u8 poe_warning;
	uint8_t Freeze_link_warning;//
	uint8_t Freeze_ping_warning;//
	uint8_t No_VAC_warning;//----
	uint8_t Low_battery_level_warning;//---
	//info
	uint8_t Change_settings_info;//--
	uint8_t STP_topology_change_info;///---
	uint8_t Connect_info;///-----
	uint8_t reset;
 };
*/
//первые 3 бита - уровень важности, 4 бит- состояние




//struct syslog_cfg_t syslog_cfg;

/*
struct SYSLOG_MSG{
	uint8_t state;
	uint8_t level;
	char msg[64];
};

struct SYSLOG_MSG syslog_msg;
*/


 //int syslog_printf(const char *fmt, ...);
 //void syslog_init(SysLog *syslog_ctx, struct ip_addr addr);

 //uint8_t syslog_send(uint8_t level,const char * format, ...);
 uint8_t syslog_send(uint8_t level,char *text);
 //uint8_t syslog_send_msg(void);
 uint8_t syslog_send_msg(char *syslog_msg);
 void addsysloghead(u8 level,char *buffer);
 void makesysloglen(char *buffer);
 void set_syslog_default(void);

 void set_syslog_param_init(u8 state);
 u8 syslog_init(void);
 void syslog_appcall(void);
 void get_syslog_cfg(void);

 //defgroup syslog_module.
 #endif /* NET_SYSLOG_H */
