
#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "webserver/httpd.h"
#include "command/command.h"
#include "../uip/timer.h"

extern int ntwk_wait_and_do;
extern struct timer ntwk_timer;

extern int update_state;
#define UPDATE_STATE_IDLE     	0
#define UPDATE_STATE_UPLOADING  1
#define UPDATE_STATE_UPLOADED 	2
#define UPDATE_STATE_UPDATING 	3

extern struct timer update_timer;

void Uip_Task(void *pvParameters);

union udp_state{ 
  struct command_state command_state;
};

typedef union udp_state uip_udp_appstate_t;

union tcp_state {
  struct httpd_state httpd_state;
};

typedef union tcp_state uip_tcp_appstate_t;

void udp_appcall(void);
void dispatch_appcall(void);

void ntwk_sethaddr(uint8_t *haddr);
void ntwk_setip(void);
//void first_net_cofig(void);
//void net_test(uint16_t state);
clock_time_t clock_time( void );
void network_sett_reinit(u8 flag);
void settings_queue_processing(void);
void settings_add2queue(u8 flag);


//для добавления в очередь настройки
#define NET_SET			1

#define SQ_CAP 0//заглушка для переменных, не нуждающихся в реинициализации

#define SQ_NETWORK		1
#define SQ_SYSLOG		2
#define SQ_SNTP			3
#define SQ_STP			4
#define SQ_IGMP			5
#define SQ_SMTP			6
#define SQ_SNMP			7
#define SQ_VLAN			8
#define SQ_SS			9
#define SQ_AR			10
#define SQ_DRYCONT		11
#define SQ_USERS		12
#define SQ_TFTP			13
#define SQ_EVENTS		14
#define SQ_QOS			15
#define SQ_PORT_ALL		16
#define SQ_POE			17
#define SQ_PBVLAN		18
#define SQ_MACFILR		19
#define SQ_VTRUNK		20
#define SQ_TELEPORT		21
#define SQ_AGGREG		22
#define SQ_MIRROR		23

#define SQ_PORT_1		101
#define SQ_PORT_2		102
#define SQ_PORT_3		103
#define SQ_PORT_4		104
#define SQ_PORT_5		105
#define SQ_PORT_6		106
#define SQ_PORT_7		107
#define SQ_PORT_8		108
#define SQ_PORT_9		109
#define SQ_PORT_10		110
#define SQ_PORT_11		111
#define SQ_PORT_12		112
#define SQ_PORT_13		113
#define SQ_PORT_14		114
#define SQ_PORT_15		115
#define SQ_PORT_16		116
#define SQ_PORT_17		117
#define SQ_PORT_18		118
#define SQ_PORT_19		119
#define SQ_PORT_20		120

#define SQ_TELNET		150
#define SQ_LLDP			151

#define SQ_ERASE 		200

#endif


