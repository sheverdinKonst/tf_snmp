#include <stdio.h>
#include <string.h>
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "../deffines.h"
#include "board.h"
#include "settings.h"
#include "selftest.h"
#include "events_handler.h"
#include "../syslog/msg_build.h"
#include "../syslog/syslog.h"
#include "../snmp/snmp.h"
#include "blbx.h"
#include "task.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "selftest.h"
#include "../webserver/httpd-cgi.h"
#include "../stp/bridgestp.h"
#include "SMIApi.h"
#include "../snmp/snmpd/ber.h"
#include "debug.h"

static u8 dry_contact_state[NUM_ALARMS]={0,0,0};


extern xQueueHandle EventsQueue;

static char message_text[MESSAGE_LEN];
//extern char mail_text[512+256];
//static struct snmp_msg_tmp_t snmp_msg_tmp;

char port[6];
char link[6];
char wdt_state[10];
char state[10];

uip_ipaddr_t ip;
extern xQueueHandle SyslogQueue,SmtpQueue,SnmpTrapQueue;

//extern struct status_t status;

#define EMPTY_TRAP

/*if(get_snmp_state() == 1){ \
				snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC; \
				snmp_msg_tmp.spectrap[0]=0;							\
				snmp_msg_tmp.varbind[0].flag=1;						 \
				snmp_msg_tmp.varbind[0].objname[0]=0x2b;\
				snmp_msg_tmp.varbind[0].objname[1]=6;\
				snmp_msg_tmp.varbind[0].objname[2]=1;\
				snmp_msg_tmp.varbind[0].objname[3]=4;\
				snmp_msg_tmp.varbind[0].objname[4]=1;\
				snmp_msg_tmp.varbind[0].objname[5]=0x82;\
				snmp_msg_tmp.varbind[0].objname[6]=0xC8;\
				snmp_msg_tmp.varbind[0].objname[7]=0x23;\
				snmp_msg_tmp.varbind[0].objname[8]=3;\
				snmp_msg_tmp.varbind[0].objname[9]=2;\
				snmp_msg_tmp.varbind[0].objname[10]=3;\
				snmp_msg_tmp.varbind[0].objname[11]=6;\
				snmp_msg_tmp.varbind[0].len = 12;\
				snmp_msg_tmp.varbind[0].value=1;\
				snmp_msg_tmp.varbind[1].flag=0;\
			}\
*/

static u8 get_u8(u8* msg){
	return msg[7];
}


static u32 get_u32(u8* msg){
	return ((msg[4] <<24) | (msg[5] << 16) | (msg[6] << 8) | msg[7]);
}

static void get_ip(u8* msg, uip_ipaddr_t ip){
	uip_ipaddr(ip,msg[4],msg[5],msg[6],msg[7]);
}

static void get_mac(u8 *msg,u8 *mac){
	for(u8 i=0;i<6;i++)
		mac[i] = msg[4+i];
}


static void addtext(char *text){
	if((strlen(text)+strlen(message_text)) < MESSAGE_LEN)
		strcpy(message_text,text);
	else
		strncpy(message_text,text,MESSAGE_LEN-strlen(text));
}

static char *add_port(u8 port1){
	if(get_dev_type() == DEV_PSW2GPLUS){
		switch(port1){
			case 0: strcpy(port,"FE1");break;
			case 1: strcpy(port,"FE2");break;
			case 2: strcpy(port,"FE3");break;
			case 3: strcpy(port,"FE4");break;
			case 4: strcpy(port,"GE1");break;
			case 5: strcpy(port,"GE2");break;
		}
	}
	else
		sprintf(port,"#%d",port1+1);
	return port;
}

static char *add_link_state(u8 state){
	switch(state){
		case 0:strcpy(link,"Down");break;
		case 1:strcpy(link,"Up");break;
	}
	return link;
}


static char *add_mode(u8 state){
	switch(state){
		case 0:strcpy(wdt_state,"Disable");break;
		case 1:strcpy(wdt_state,"Link");break;
		case 2:strcpy(wdt_state,"Ping");break;
	}
	return wdt_state;
}

static char *add_state(u8 state1){
	switch(state1){
		case 0:strcpy(state,"Disable");break;
		case 1:strcpy(state,"Enable");break;
		case 2:strcpy(state,"Passive");break;
	}
	return state;
}

static char *add_front(u8 front){
	switch(front){
		case 1:strcpy(state,"Short");break;
		case 2:strcpy(state,"Open");break;
	}
	return state;
}

static char *add_rule(u8 rule){
	switch(rule){
		case 0:strcpy(state,"CoS only");break;
		case 1:strcpy(state,"ToS Only");break;
		case 2:strcpy(state,"CoS and ToS");break;
	}
	return state;
}

static char *add_proto(u8 proto){
	switch(proto){
		case 0:strcpy(state,"STP");break;
		case 2:strcpy(state,"RSTP");break;
	}
	return state;
}


static char *add_duplex(u8 state1){
	switch(state1){
		case 0:strcpy(state,"Auto");break;
		case 1:strcpy(state,"10M Half");break;
		case 2:strcpy(state,"10M Full");break;
		case 3:strcpy(state,"100M Half");break;
		case 4:strcpy(state,"100M Full");break;
		case 5:strcpy(state,"1000M Half");break;
		case 6:strcpy(state,"1000M Full");break;
	}
	return state;
}


/*добавление событий в очередь*/
void send_events_u32(u32 type,u32 ptr){
u8 tmp[10];
	tmp[0] = (u8)(type>>24);
	tmp[1] = (u8)(type>>16);
	tmp[2] = (u8)(type>>8);
	tmp[3] = (u8)type;
	tmp[4] = (u8)(ptr>>24);
	tmp[5] = (u8)(ptr>>16);
	tmp[6] = (u8)(ptr>>8);
	tmp[7] = (u8)ptr;
	if(EventsQueue)
		xQueueSend(EventsQueue,tmp,( portTickType ) 0 );
}

void send_events_ip(u32 type,uip_ipaddr_t ip){
u8 tmp[10];
	tmp[0] = (u8)(type>>24);
	tmp[1] = (u8)(type>>16);
	tmp[2] = (u8)(type>>8);
	tmp[3] = (u8)type;
	tmp[4] = uip_ipaddr1(ip);
	tmp[5] = uip_ipaddr2(ip);
	tmp[6] = uip_ipaddr3(ip);
	tmp[7] = uip_ipaddr4(ip);
	if(EventsQueue)
		xQueueSend(EventsQueue,tmp,( portTickType ) 0 );
}

void send_events_mac(u32 type,u8 *ptr){
u8 tmp[10];
	tmp[0] = (u8)(type>>24);
	tmp[1] = (u8)(type>>16);
	tmp[2] = (u8)(type>>8);
	tmp[3] = (u8)type;
	for(u8 i=0;i<6;i++)
		tmp[4+i] = ptr[i];
	if(EventsQueue)
		xQueueSend(EventsQueue,tmp,( portTickType ) 0 );
}

void send_events_task(void){
u8 msg[10];//сообщение из очереди
char tmp[256];
char temp[256];
u32 temp32;
u8 temp8;
i8 level;
u8 mac[10];
u32 type;
struct snmp_msg2_t snmp_msg_tmp;
bstp_bridge_stat_t  RstpStat;

	if(!EventsQueue)
		return;

	if(xQueueReceive(EventsQueue,msg,0) == pdPASS ){

		type =  msg[0]<<24 | msg[1]<<16  | msg[2]<<8 | msg[3] ;


		//printf("send_events %lu\r\n",type);



		snmp_msg_tmp.varbind[0].flag = 0;



		if(settings_is_loaded()==0)
			return;

		memset(tmp,0,sizeof(tmp));
		memset(message_text,0,sizeof(message_text));

		switch(type){
			case EVENT_LINK_P1:
			case EVENT_LINK_P1+1:
			case EVENT_LINK_P1+2:
			case EVENT_LINK_P1+3:
			case EVENT_LINK_P1+4:
			case EVENT_LINK_P1+5:
			case EVENT_LINK_P1+6:
			case EVENT_LINK_P1+7:
			case EVENT_LINK_P1+8:
			case EVENT_LINK_P1+9:
			case EVENT_LINK_P1+10:
			case EVENT_LINK_P1+11:
			case EVENT_LINK_P1+12:
			case EVENT_LINK_P1+13:
			case EVENT_LINK_P1+14:
			case EVENT_LINK_P1+15:
			case EVENT_LINK_P1+16:
			case EVENT_LINK_P1+17:
			case EVENT_LINK_P1+18:
			case EVENT_LINK_P1+19:
				//temp8 = (u8 *)ptr;
				temp8 = get_u8(msg);

				sprintf(tmp,"Port %s Link %s",add_port(type-EVENT_LINK_P1),add_link_state(temp8));
				addtext(tmp);

				if(get_snmp_state() == 1){
					if(temp8 == 1)//link up
						snmp_msg_tmp.gentrap = SNMP_GENTRAP_LINKUP;
					else
						snmp_msg_tmp.gentrap = SNMP_GENTRAP_LINKDOWN;
					snmp_msg_tmp.spectrap[0]=0;

					//ifIndex.Port = Port
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=2;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=2;
					snmp_msg_tmp.varbind[0].objname[6]=2;
					snmp_msg_tmp.varbind[0].objname[7]=1;
					snmp_msg_tmp.varbind[0].objname[8]=1;
					snmp_msg_tmp.varbind[0].objname[9]=(type - EVENT_LINK_P1 + 1);//if_num 1..6
					snmp_msg_tmp.varbind[0].len = 10;
					snmp_msg_tmp.varbind[0].value=(type - EVENT_LINK_P1 + 1);

					//ifAdminStatus.Port = state
					snmp_msg_tmp.varbind[1].flag=1;
					snmp_msg_tmp.varbind[1].objname[0]=0x2b;
					snmp_msg_tmp.varbind[1].objname[1]=6;
					snmp_msg_tmp.varbind[1].objname[2]=1;
					snmp_msg_tmp.varbind[1].objname[3]=2;
					snmp_msg_tmp.varbind[1].objname[4]=1;
					snmp_msg_tmp.varbind[1].objname[5]=2;
					snmp_msg_tmp.varbind[1].objname[6]=2;
					snmp_msg_tmp.varbind[1].objname[7]=1;
					snmp_msg_tmp.varbind[1].objname[8]=7;
					snmp_msg_tmp.varbind[1].objname[9]=(type - EVENT_LINK_P1 + 1);//if_num 1..6
					snmp_msg_tmp.varbind[1].len = 10;
					if(get_port_sett_state(type - EVENT_LINK_P1) == 1)
						snmp_msg_tmp.varbind[1].value=SNMP_UP;//up
					else
						snmp_msg_tmp.varbind[1].value=SNMP_DOWN;//down

					//ifOperStatus
					snmp_msg_tmp.varbind[2].flag=1;
					snmp_msg_tmp.varbind[2].objname[0]=0x2b;
					snmp_msg_tmp.varbind[2].objname[1]=6;
					snmp_msg_tmp.varbind[2].objname[2]=1;
					snmp_msg_tmp.varbind[2].objname[3]=2;
					snmp_msg_tmp.varbind[2].objname[4]=1;
					snmp_msg_tmp.varbind[2].objname[5]=2;
					snmp_msg_tmp.varbind[2].objname[6]=2;
					snmp_msg_tmp.varbind[2].objname[7]=1;
					snmp_msg_tmp.varbind[2].objname[8]=8;
					snmp_msg_tmp.varbind[2].objname[9]=(type - EVENT_LINK_P1 + 1);//if_num 1..6
					snmp_msg_tmp.varbind[2].len = 10;
					if(temp8 == 1)
						snmp_msg_tmp.varbind[2].value=SNMP_UP;
					else
						snmp_msg_tmp.varbind[2].value=SNMP_DOWN;
					snmp_msg_tmp.varbind[3].flag=0;
				}
				break;


			case EVENT_POE_A_P1:
			case EVENT_POE_A_P1+1:
			case EVENT_POE_A_P1+2:
			case EVENT_POE_A_P1+3:
			case EVENT_POE_A_P1+4:
			case EVENT_POE_A_P1+5:
			case EVENT_POE_A_P1+6:
			case EVENT_POE_A_P1+7:
			case EVENT_POE_A_P1+8:
			case EVENT_POE_A_P1+9:
			case EVENT_POE_A_P1+10:
			case EVENT_POE_A_P1+11:
			case EVENT_POE_A_P1+12:
			case EVENT_POE_A_P1+13:
			case EVENT_POE_A_P1+14:
			case EVENT_POE_A_P1+15:
			case EVENT_POE_A_P1+16:
			case EVENT_POE_A_P1+17:
			case EVENT_POE_A_P1+18:
			case EVENT_POE_A_P1+19:
				//temp8 = *(u8 *)ptr;
				temp8 = get_u8(msg);
				sprintf(tmp,"Port %s PoE A %s",add_port(type-EVENT_POE_A_P1),add_link_state(temp8));
				addtext(tmp);

				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=0;

					//ifIndex.Port = Port
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=2;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=2;
					snmp_msg_tmp.varbind[0].objname[6]=2;
					snmp_msg_tmp.varbind[0].objname[7]=1;
					snmp_msg_tmp.varbind[0].objname[8]=1;
					snmp_msg_tmp.varbind[0].objname[9]=(type - EVENT_POE_A_P1 + 1);//if_num 1..6
					snmp_msg_tmp.varbind[0].len = 10;
					snmp_msg_tmp.varbind[0].value=(type - EVENT_POE_A_P1 + 1);

					//ifAdminStatus.Port = state
					snmp_msg_tmp.varbind[1].flag=1;
					snmp_msg_tmp.varbind[1].objname[0]=0x2b;
					snmp_msg_tmp.varbind[1].objname[1]=6;
					snmp_msg_tmp.varbind[1].objname[2]=1;
					snmp_msg_tmp.varbind[1].objname[3]=2;
					snmp_msg_tmp.varbind[1].objname[4]=1;
					snmp_msg_tmp.varbind[1].objname[5]=105;
					snmp_msg_tmp.varbind[1].objname[6]=0;
					snmp_msg_tmp.varbind[1].objname[7]=(type - EVENT_POE_A_P1 + 1);//if_num 1..6
					snmp_msg_tmp.varbind[1].len = 8;
					if(temp8 == 1)
						snmp_msg_tmp.varbind[1].value=SNMP_UP;
					else
						snmp_msg_tmp.varbind[1].value=SNMP_DOWN;
					snmp_msg_tmp.varbind[2].flag=0;
				}
				break;

			case EVENT_POE_B_P1:
			case EVENT_POE_B_P1+1:
			case EVENT_POE_B_P1+2:
			case EVENT_POE_B_P1+3:
			case EVENT_POE_B_P1+4:
			case EVENT_POE_B_P1+5:
			case EVENT_POE_B_P1+6:
			case EVENT_POE_B_P1+7:
			case EVENT_POE_B_P1+8:
			case EVENT_POE_B_P1+9:
			case EVENT_POE_B_P1+10:
			case EVENT_POE_B_P1+11:
			case EVENT_POE_B_P1+12:
			case EVENT_POE_B_P1+13:
			case EVENT_POE_B_P1+14:
			case EVENT_POE_B_P1+15:
			case EVENT_POE_B_P1+16:
			case EVENT_POE_B_P1+17:
			case EVENT_POE_B_P1+18:
			case EVENT_POE_B_P1+19:
				//temp8 = *(u8 *)ptr;
				temp8 = get_u8(msg);
				sprintf(tmp,"Port %s PoE B %s",add_port(type-EVENT_POE_B_P1),add_link_state(temp8));
				addtext(tmp);
				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=0;
					//ifIndex.Port = Port
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=2;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=2;
					snmp_msg_tmp.varbind[0].objname[6]=2;
					snmp_msg_tmp.varbind[0].objname[7]=1;
					snmp_msg_tmp.varbind[0].objname[8]=1;
					snmp_msg_tmp.varbind[0].objname[9]=(type - EVENT_POE_B_P1 + 1);//if_num 1..6
					snmp_msg_tmp.varbind[0].len = 10;
					snmp_msg_tmp.varbind[0].value=(type - EVENT_POE_B_P1 + 1);

					//ifAdminStatus.Port = state
					snmp_msg_tmp.varbind[1].flag=1;
					snmp_msg_tmp.varbind[1].objname[0]=0x2b;
					snmp_msg_tmp.varbind[1].objname[1]=6;
					snmp_msg_tmp.varbind[1].objname[2]=1;
					snmp_msg_tmp.varbind[1].objname[3]=2;
					snmp_msg_tmp.varbind[1].objname[4]=1;
					snmp_msg_tmp.varbind[1].objname[5]=105;
					snmp_msg_tmp.varbind[1].objname[6]=0;
					snmp_msg_tmp.varbind[1].objname[7]=(type - EVENT_POE_B_P1 + 1);//if_num 1..6
					snmp_msg_tmp.varbind[1].len = 8;
					if(temp8 == 1)
						snmp_msg_tmp.varbind[1].value=SNMP_UP;
					else
						snmp_msg_tmp.varbind[1].value=SNMP_DOWN;
					snmp_msg_tmp.varbind[2].flag=0;
				}
				break;

			case EVENTS_SENSOR0:
				sprintf(tmp,"Sensor 0 (Tamper) is active!");
				addtext(tmp);
				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=16;
					snmp_msg_tmp.spectrap[1]=0;
					//sensor 0 is active
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=4;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=0x82;//
					snmp_msg_tmp.varbind[0].objname[6]=0xC8;//oid = 42019
					snmp_msg_tmp.varbind[0].objname[7]=0x23;//
					snmp_msg_tmp.varbind[0].objname[8]=3;
					snmp_msg_tmp.varbind[0].objname[9]=2;
					snmp_msg_tmp.varbind[0].objname[10]=0;
					snmp_msg_tmp.varbind[0].objname[11]=16;
					snmp_msg_tmp.varbind[0].len = 12;
					snmp_msg_tmp.varbind[0].value=1;
					snmp_msg_tmp.varbind[1].flag=0;
				}
				break;

			case EVENTS_SENSOR1:
				temp32 = get_u32(msg);
				sprintf(tmp,"Sensor 1 (Dry contact) is active! (%s)",add_front(temp32+1));
				addtext(tmp);
				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=17;
					snmp_msg_tmp.spectrap[1]=0;
					//sensor 1 is active
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=4;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=0x82;//
					snmp_msg_tmp.varbind[0].objname[6]=0xC8;//oid = 42019
					snmp_msg_tmp.varbind[0].objname[7]=0x23;//
					snmp_msg_tmp.varbind[0].objname[8]=3;
					snmp_msg_tmp.varbind[0].objname[9]=2;
					snmp_msg_tmp.varbind[0].objname[10]=0;
					snmp_msg_tmp.varbind[0].objname[11]=17;
					snmp_msg_tmp.varbind[0].len = 12;
					snmp_msg_tmp.varbind[0].value=2-temp32;
					snmp_msg_tmp.varbind[1].flag=0;
				}
				break;

			case EVENTS_SENSOR2:
				temp32 = get_u32(msg);
				sprintf(tmp,"Sensor 2 (Dry contact) is active!(%s)",add_front(temp32+1));
				addtext(tmp);
				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=18;
					snmp_msg_tmp.spectrap[1]=0;
					//sensor 2 is active
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=4;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=0x82;//
					snmp_msg_tmp.varbind[0].objname[6]=0xC8;//oid = 42019
					snmp_msg_tmp.varbind[0].objname[7]=0x23;//
					snmp_msg_tmp.varbind[0].objname[8]=3;
					snmp_msg_tmp.varbind[0].objname[9]=2;
					snmp_msg_tmp.varbind[0].objname[10]=0;
					snmp_msg_tmp.varbind[0].objname[11]=18;
					snmp_msg_tmp.varbind[0].len = 12;
					snmp_msg_tmp.varbind[0].value=2-temp32;
					snmp_msg_tmp.varbind[1].flag=0;
				}
				break;

			case EVENTS_INPUT1:
			case EVENTS_INPUT2:
			case EVENTS_INPUT3:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Input %d is active(%s)",(u8)(type-EVENTS_INPUT1+1),add_front(temp8+1));
				addtext(tmp);
				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=19+(type-EVENTS_INPUT1);
					snmp_msg_tmp.spectrap[1]=0;
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=4;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=0x82;//
					snmp_msg_tmp.varbind[0].objname[6]=0xC8;//oid = 42019
					snmp_msg_tmp.varbind[0].objname[7]=0x23;//
					snmp_msg_tmp.varbind[0].objname[8]=3;
					snmp_msg_tmp.varbind[0].objname[9]=2;
					snmp_msg_tmp.varbind[0].objname[10]=0;
					snmp_msg_tmp.varbind[0].objname[11]=19+(type-EVENTS_INPUT1);
					snmp_msg_tmp.varbind[0].len = 12;
					snmp_msg_tmp.varbind[0].value=(temp8);
					snmp_msg_tmp.varbind[1].flag=0;
				}
				break;

			case EVENT_NOREPLY_LINK_P1:
			case EVENT_NOREPLY_LINK_P1+1:
			case EVENT_NOREPLY_LINK_P1+2:
			case EVENT_NOREPLY_LINK_P1+3:
			case EVENT_NOREPLY_LINK_P1+4:
			case EVENT_NOREPLY_LINK_P1+5:
			case EVENT_NOREPLY_LINK_P1+6:
			case EVENT_NOREPLY_LINK_P1+7:
			case EVENT_NOREPLY_LINK_P1+8:
			case EVENT_NOREPLY_LINK_P1+9:
			case EVENT_NOREPLY_LINK_P1+10:
			case EVENT_NOREPLY_LINK_P1+11:
			case EVENT_NOREPLY_LINK_P1+12:
			case EVENT_NOREPLY_LINK_P1+13:
			case EVENT_NOREPLY_LINK_P1+14:
			case EVENT_NOREPLY_LINK_P1+15:
			case EVENT_NOREPLY_LINK_P1+16:
			case EVENT_NOREPLY_LINK_P1+17:
			case EVENT_NOREPLY_LINK_P1+18:
			case EVENT_NOREPLY_LINK_P1+19:
				sprintf(tmp,"Port %s Autorestart: no link",add_port(type-EVENT_NOREPLY_LINK_P1));
				addtext(tmp);
				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=2;
					snmp_msg_tmp.spectrap[1]=0;
					//ifIndex.Port = Port
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=2;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=2;
					snmp_msg_tmp.varbind[0].objname[6]=2;
					snmp_msg_tmp.varbind[0].objname[7]=1;
					snmp_msg_tmp.varbind[0].objname[8]=1;
					snmp_msg_tmp.varbind[0].objname[9]=(type - EVENT_NOREPLY_LINK_P1 + 1);//if_num 1..6
					snmp_msg_tmp.varbind[0].len = 10;
					snmp_msg_tmp.varbind[0].value=(type - EVENT_NOREPLY_LINK_P1 + 1);

					//special function: no link
					snmp_msg_tmp.varbind[1].flag=1;
					snmp_msg_tmp.varbind[1].objname[0]=0x2b;
					snmp_msg_tmp.varbind[1].objname[1]=6;
					snmp_msg_tmp.varbind[1].objname[2]=1;
					snmp_msg_tmp.varbind[1].objname[3]=4;
					snmp_msg_tmp.varbind[1].objname[4]=1;
					snmp_msg_tmp.varbind[1].objname[5]=0x82;//
					snmp_msg_tmp.varbind[1].objname[6]=0xC8;//oid = 42019
					snmp_msg_tmp.varbind[1].objname[7]=0x23;//
					snmp_msg_tmp.varbind[0].objname[8]=3;
					snmp_msg_tmp.varbind[0].objname[9]=2;
					snmp_msg_tmp.varbind[1].objname[10]=0;
					snmp_msg_tmp.varbind[1].objname[11]=2;
					snmp_msg_tmp.varbind[1].len = 12;
					snmp_msg_tmp.varbind[1].value=(type - EVENT_NOREPLY_LINK_P1+1);
					snmp_msg_tmp.varbind[2].flag=0;
				}

				break;

			case EVENT_NOREPLY_PING_P1:
			case EVENT_NOREPLY_PING_P1+1:
			case EVENT_NOREPLY_PING_P1+2:
			case EVENT_NOREPLY_PING_P1+3:
			case EVENT_NOREPLY_PING_P1+4:
			case EVENT_NOREPLY_PING_P1+5:
			case EVENT_NOREPLY_PING_P1+6:
			case EVENT_NOREPLY_PING_P1+7:
			case EVENT_NOREPLY_PING_P1+8:
			case EVENT_NOREPLY_PING_P1+9:
			case EVENT_NOREPLY_PING_P1+10:
			case EVENT_NOREPLY_PING_P1+11:
			case EVENT_NOREPLY_PING_P1+12:
			case EVENT_NOREPLY_PING_P1+13:
			case EVENT_NOREPLY_PING_P1+14:
			case EVENT_NOREPLY_PING_P1+15:
			case EVENT_NOREPLY_PING_P1+16:
			case EVENT_NOREPLY_PING_P1+17:
			case EVENT_NOREPLY_PING_P1+18:
			case EVENT_NOREPLY_PING_P1+19:
				sprintf(tmp,"Port %s Autorestart: no reply to Ping",add_port(type-EVENT_NOREPLY_PING_P1));
				addtext(tmp);
				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=3;
					snmp_msg_tmp.spectrap[1]=0;
					//ifIndex.Port = Port
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=2;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=2;
					snmp_msg_tmp.varbind[0].objname[6]=2;
					snmp_msg_tmp.varbind[0].objname[7]=1;
					snmp_msg_tmp.varbind[0].objname[8]=1;
					snmp_msg_tmp.varbind[0].objname[9]=(type - EVENT_NOREPLY_PING_P1 + 1);//if_num 1..6
					snmp_msg_tmp.varbind[0].len = 10;
					snmp_msg_tmp.varbind[0].value=(type - EVENT_NOREPLY_PING_P1 + 1);

					//special function: no ping
					snmp_msg_tmp.varbind[1].flag=1;
					snmp_msg_tmp.varbind[1].objname[0]=0x2b;
					snmp_msg_tmp.varbind[1].objname[1]=6;
					snmp_msg_tmp.varbind[1].objname[2]=1;
					snmp_msg_tmp.varbind[1].objname[3]=4;
					snmp_msg_tmp.varbind[1].objname[4]=1;
					snmp_msg_tmp.varbind[1].objname[5]=0x82;//
					snmp_msg_tmp.varbind[1].objname[6]=0xC8;//oid = 42019
					snmp_msg_tmp.varbind[1].objname[7]=0x23;//
					snmp_msg_tmp.varbind[0].objname[8]=3;
					snmp_msg_tmp.varbind[0].objname[9]=2;
					snmp_msg_tmp.varbind[1].objname[10]=0;
					snmp_msg_tmp.varbind[1].objname[11]=3;
					snmp_msg_tmp.varbind[1].len = 12;
					snmp_msg_tmp.varbind[1].value=(type - EVENT_NOREPLY_PING_P1 + 1);//if_num 1..6;
					snmp_msg_tmp.varbind[2].flag=0;
				}
				break;


			case EVENT_LOWSPEED_P1:
			case EVENT_LOWSPEED_P1+1:
			case EVENT_LOWSPEED_P1+2:
			case EVENT_LOWSPEED_P1+3:
			case EVENT_LOWSPEED_P1+4:
			case EVENT_LOWSPEED_P1+5:
			case EVENT_LOWSPEED_P1+6:
			case EVENT_LOWSPEED_P1+7:
			case EVENT_LOWSPEED_P1+8:
			case EVENT_LOWSPEED_P1+9:
			case EVENT_LOWSPEED_P1+10:
			case EVENT_LOWSPEED_P1+11:
			case EVENT_LOWSPEED_P1+12:
			case EVENT_LOWSPEED_P1+13:
			case EVENT_LOWSPEED_P1+14:
			case EVENT_LOWSPEED_P1+15:
			case EVENT_LOWSPEED_P1+16:
			case EVENT_LOWSPEED_P1+17:
			case EVENT_LOWSPEED_P1+18:
			case EVENT_LOWSPEED_P1+19:
				temp32 = get_u32(msg);
				sprintf(tmp,"Port %s Autorestart: Low Speed %dKbps",add_port(type-EVENT_LOWSPEED_P1),temp32);
				addtext(tmp);
				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=4;
					snmp_msg_tmp.spectrap[1]=0;
					//ifIndex.Port = Port
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=2;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=2;
					snmp_msg_tmp.varbind[0].objname[6]=2;
					snmp_msg_tmp.varbind[0].objname[7]=1;
					snmp_msg_tmp.varbind[0].objname[8]=1;
					snmp_msg_tmp.varbind[0].objname[9]=(type - EVENT_LOWSPEED_P1 + 1);//if_num 1..6
					snmp_msg_tmp.varbind[0].len = 10;
					snmp_msg_tmp.varbind[0].value=(type - EVENT_LOWSPEED_P1 + 1);

					//special function: no ping
					snmp_msg_tmp.varbind[1].flag=1;
					snmp_msg_tmp.varbind[1].objname[0]=0x2b;
					snmp_msg_tmp.varbind[1].objname[1]=6;
					snmp_msg_tmp.varbind[1].objname[2]=1;
					snmp_msg_tmp.varbind[1].objname[3]=4;
					snmp_msg_tmp.varbind[1].objname[4]=1;
					snmp_msg_tmp.varbind[1].objname[5]=0x82;//
					snmp_msg_tmp.varbind[1].objname[6]=0xC8;//oid = 42019
					snmp_msg_tmp.varbind[1].objname[7]=0x23;//
					snmp_msg_tmp.varbind[0].objname[8]=3;
					snmp_msg_tmp.varbind[0].objname[9]=2;
					snmp_msg_tmp.varbind[1].objname[10]=0;
					snmp_msg_tmp.varbind[1].objname[11]=4;
					snmp_msg_tmp.varbind[1].len = 12;
					snmp_msg_tmp.varbind[1].value=(type - EVENT_LOWSPEED_P1 + 1);//if_num 1..6;
					snmp_msg_tmp.varbind[2].flag=0;
				}
				break;

			case EVENT_HISPEED_P1:
			case EVENT_HISPEED_P1+1:
			case EVENT_HISPEED_P1+2:
			case EVENT_HISPEED_P1+3:
			case EVENT_HISPEED_P1+4:
			case EVENT_HISPEED_P1+5:
			case EVENT_HISPEED_P1+6:
			case EVENT_HISPEED_P1+7:
			case EVENT_HISPEED_P1+8:
			case EVENT_HISPEED_P1+9:
			case EVENT_HISPEED_P1+10:
			case EVENT_HISPEED_P1+11:
			case EVENT_HISPEED_P1+12:
			case EVENT_HISPEED_P1+13:
			case EVENT_HISPEED_P1+14:
			case EVENT_HISPEED_P1+15:
				temp32 = get_u32(msg);
				sprintf(tmp,"Port %s Autorestart: High Speed %dKbps",add_port(type-EVENT_HISPEED_P1),temp32);
				addtext(tmp);
				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=4;
					snmp_msg_tmp.spectrap[1]=0;
					//ifIndex.Port = Port
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=2;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=2;
					snmp_msg_tmp.varbind[0].objname[6]=2;
					snmp_msg_tmp.varbind[0].objname[7]=1;
					snmp_msg_tmp.varbind[0].objname[8]=1;
					snmp_msg_tmp.varbind[0].objname[9]=(type - EVENT_HISPEED_P1 + 1);//if_num 1..6
					snmp_msg_tmp.varbind[0].len = 10;
					snmp_msg_tmp.varbind[0].value=(type - EVENT_HISPEED_P1 + 1);

					//special function: no ping
					snmp_msg_tmp.varbind[1].flag=1;
					snmp_msg_tmp.varbind[1].objname[0]=0x2b;
					snmp_msg_tmp.varbind[1].objname[1]=6;
					snmp_msg_tmp.varbind[1].objname[2]=1;
					snmp_msg_tmp.varbind[1].objname[3]=4;
					snmp_msg_tmp.varbind[1].objname[4]=1;
					snmp_msg_tmp.varbind[1].objname[5]=0x82;//
					snmp_msg_tmp.varbind[1].objname[6]=0xC8;//oid = 42019
					snmp_msg_tmp.varbind[1].objname[7]=0x23;//
					snmp_msg_tmp.varbind[0].objname[8]=3;
					snmp_msg_tmp.varbind[0].objname[9]=2;
					snmp_msg_tmp.varbind[1].objname[10]=0;
					snmp_msg_tmp.varbind[1].objname[11]=4;
					snmp_msg_tmp.varbind[1].len = 12;
					snmp_msg_tmp.varbind[1].value=(type - EVENT_HISPEED_P1 + 1);//if_num 1..6;
					snmp_msg_tmp.varbind[2].flag=0;
				}
				break;




			case EVENT_START_PWR:
				sprintf(tmp,"Start after power reset");
				addtext(tmp);
				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=9;
					snmp_msg_tmp.spectrap[1]=0;
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=4;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=0x82;//
					snmp_msg_tmp.varbind[0].objname[6]=0xC8;//oid = 42019
					snmp_msg_tmp.varbind[0].objname[7]=0x23;//
					snmp_msg_tmp.varbind[0].objname[8]=3;
					snmp_msg_tmp.varbind[0].objname[9]=2;
					snmp_msg_tmp.varbind[0].objname[10]=0;
					snmp_msg_tmp.varbind[0].objname[11]=9;
					snmp_msg_tmp.varbind[0].len = 12;
					snmp_msg_tmp.varbind[0].value=1;
					snmp_msg_tmp.varbind[1].flag=0;
				}
				break;

			case EVENT_START_RST:
				sprintf(tmp,"Start after reset");
				addtext(tmp);
				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=8;
					snmp_msg_tmp.spectrap[1]=0;
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=4;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=0x82;//
					snmp_msg_tmp.varbind[0].objname[6]=0xC8;//oid = 42019
					snmp_msg_tmp.varbind[0].objname[7]=0x23;//
					snmp_msg_tmp.varbind[0].objname[8]=3;
					snmp_msg_tmp.varbind[0].objname[9]=2;
					snmp_msg_tmp.varbind[0].objname[10]=0;
					snmp_msg_tmp.varbind[0].objname[11]=8;
					snmp_msg_tmp.varbind[0].len = 12;
					snmp_msg_tmp.varbind[0].value=1;
					snmp_msg_tmp.varbind[1].flag=0;
				}
				break;

			case EVENT_WEBINTERFACE_LOGIN:
				sprintf(tmp,"Web-interface authentication: Ok");
				addtext(tmp);
				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=10;
					snmp_msg_tmp.spectrap[1]=0;
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=4;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=0x82;//
					snmp_msg_tmp.varbind[0].objname[6]=0xC8;//oid = 42019
					snmp_msg_tmp.varbind[0].objname[7]=0x23;//
					snmp_msg_tmp.varbind[0].objname[8]=3;
					snmp_msg_tmp.varbind[0].objname[9]=2;
					snmp_msg_tmp.varbind[0].objname[10]=0;
					snmp_msg_tmp.varbind[0].objname[11]=10;
					snmp_msg_tmp.varbind[0].len = 12;
					snmp_msg_tmp.varbind[0].value=1;
					snmp_msg_tmp.varbind[1].flag=0;
				}
				break;

			case EVENT_UPDATE_FIRMWARE:
				//temp32 = (*(u32 *)ptr);
				temp32 = get_u32(msg);
				sprintf(tmp,"Update firmware %x.%x.%x",(u8)(temp32>>16),(u8)(temp32>>8),(u8)(temp32));
				addtext(tmp);
				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=5;
					snmp_msg_tmp.spectrap[1]=0;
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=4;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=0x82;//
					snmp_msg_tmp.varbind[0].objname[6]=0xC8;//oid = 42019
					snmp_msg_tmp.varbind[0].objname[7]=0x23;//
					snmp_msg_tmp.varbind[0].objname[8]=3;
					snmp_msg_tmp.varbind[0].objname[9]=2;
					snmp_msg_tmp.varbind[0].objname[10]=0;
					snmp_msg_tmp.varbind[0].objname[11]=5;
					snmp_msg_tmp.varbind[0].len = 12;
					snmp_msg_tmp.varbind[0].value=temp32;
					snmp_msg_tmp.varbind[1].flag=0;
				}
				break;

			case EVENT_CLEAR_ARP:
				sprintf(tmp,"Clear ARP cash");
				addtext(tmp);
				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=12;
					snmp_msg_tmp.spectrap[1]=0;
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=4;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=0x82;//
					snmp_msg_tmp.varbind[0].objname[6]=0xC8;//oid = 42019
					snmp_msg_tmp.varbind[0].objname[7]=0x23;//
					snmp_msg_tmp.varbind[0].objname[8]=3;
					snmp_msg_tmp.varbind[0].objname[9]=2;
					snmp_msg_tmp.varbind[0].objname[10]=0;
					snmp_msg_tmp.varbind[0].objname[11]=12;
					snmp_msg_tmp.varbind[0].len = 12;
					snmp_msg_tmp.varbind[0].value=1;
					snmp_msg_tmp.varbind[1].flag=0;
				}
				break;

			case EVENT_STP_REINIT:

				bstp_get_bridge_stat(&RstpStat);
				if(RstpStat.flags & BSTP_BRIDGE_STAT_FLAG_ROOT)
					sprintf(tmp,"STP: bridge is ROOT, %lu",RstpStat.last_tc_time);
				else
					sprintf(tmp,"STP: root port %d, root bridge %02X:%02X:%02X:%02X:%02X:%02X, paht cost %lu,"
							"designated bridge %02X:%02X:%02X:%02X:%02X:%02X, %lu",
						F2L_port_conv(RstpStat.root_port)+1, RstpStat.rootaddr[0],RstpStat.rootaddr[1],RstpStat.rootaddr[2],
						RstpStat.rootaddr[3],RstpStat.rootaddr[4],RstpStat.rootaddr[5],RstpStat.root_cost,RstpStat.desgaddr[0],
						RstpStat.desgaddr[1],RstpStat.desgaddr[2],RstpStat.desgaddr[3],RstpStat.desgaddr[4],RstpStat.desgaddr[5],
						RstpStat.last_tc_time);

				addtext(tmp);
				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=1;
					snmp_msg_tmp.spectrap[1]=0;
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=4;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=0x82;//
					snmp_msg_tmp.varbind[0].objname[6]=0xC8;//oid = 42019
					snmp_msg_tmp.varbind[0].objname[7]=0x23;//
					snmp_msg_tmp.varbind[0].objname[8]=3;
					snmp_msg_tmp.varbind[0].objname[9]=2;
					snmp_msg_tmp.varbind[0].objname[10]=0;
					snmp_msg_tmp.varbind[0].objname[11]=1;
					snmp_msg_tmp.varbind[0].len = 12;
					snmp_msg_tmp.varbind[0].value=1;
					snmp_msg_tmp.varbind[1].flag=0;
				}
				break;

			case EVENT_STP_PORT_1:
			case EVENT_STP_PORT_2:
			case EVENT_STP_PORT_3:
			case EVENT_STP_PORT_4:
			case EVENT_STP_PORT_5:
			case EVENT_STP_PORT_6:
			case EVENT_STP_PORT_7:
			case EVENT_STP_PORT_8:
			case EVENT_STP_PORT_9:
			case EVENT_STP_PORT_10:
			case EVENT_STP_PORT_11:
			case EVENT_STP_PORT_12:
			case EVENT_STP_PORT_13:
			case EVENT_STP_PORT_14:
			case EVENT_STP_PORT_15:
			case EVENT_STP_PORT_16:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"STP port #%d status changed: ",(u8)(type - EVENT_STP_PORT_1 + 1));
				switch(temp8){
					case BSTP_IFSTATE_DISCARDING:
						strcat(tmp,"Discarding");
						break;
					case BSTP_IFSTATE_LEARNING:
						strcat(tmp,"Learning");
						break;
					case BSTP_IFSTATE_FORWARDING:
						strcat(tmp,"Forwarding");
						break;
				}
				addtext(tmp);
				EMPTY_TRAP
				break;

				//ups
			case EVENTS_UPS_REZERV:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				if(temp8 == 1){
					sprintf(tmp,"Power souce is battery");
					if(get_snmp_state() == 1){
						snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
						snmp_msg_tmp.spectrap[0]=14;
						snmp_msg_tmp.spectrap[1]=0;
						snmp_msg_tmp.varbind[0].flag=1;
						snmp_msg_tmp.varbind[0].objname[0]=0x2b;
						snmp_msg_tmp.varbind[0].objname[1]=6;
						snmp_msg_tmp.varbind[0].objname[2]=1;
						snmp_msg_tmp.varbind[0].objname[3]=4;
						snmp_msg_tmp.varbind[0].objname[4]=1;
						snmp_msg_tmp.varbind[0].objname[5]=0x82;//
						snmp_msg_tmp.varbind[0].objname[6]=0xC8;//oid = 42019
						snmp_msg_tmp.varbind[0].objname[7]=0x23;//
						snmp_msg_tmp.varbind[0].objname[8]=3;
						snmp_msg_tmp.varbind[0].objname[9]=2;
						snmp_msg_tmp.varbind[0].objname[10]=0;
						snmp_msg_tmp.varbind[0].objname[11]=14;
						snmp_msg_tmp.varbind[0].len = 12;
						snmp_msg_tmp.varbind[0].value=1;
						snmp_msg_tmp.varbind[1].flag=0;
					}
				}
				else{
					sprintf(tmp,"Power souce is external VAC");
					if(get_snmp_state() == 1){
						snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
						snmp_msg_tmp.spectrap[0]=15;
						snmp_msg_tmp.spectrap[1]=0;
						snmp_msg_tmp.varbind[0].flag=1;
						snmp_msg_tmp.varbind[0].objname[0]=0x2b;
						snmp_msg_tmp.varbind[0].objname[1]=6;
						snmp_msg_tmp.varbind[0].objname[2]=1;
						snmp_msg_tmp.varbind[0].objname[3]=4;
						snmp_msg_tmp.varbind[0].objname[4]=1;
						snmp_msg_tmp.varbind[0].objname[5]=0x82;//
						snmp_msg_tmp.varbind[0].objname[6]=0xC8;//oid = 42019
						snmp_msg_tmp.varbind[0].objname[7]=0x23;//
						snmp_msg_tmp.varbind[0].objname[8]=3;
						snmp_msg_tmp.varbind[0].objname[9]=2;
						snmp_msg_tmp.varbind[0].objname[10]=0;
						snmp_msg_tmp.varbind[0].objname[11]=15;
						snmp_msg_tmp.varbind[0].len = 12;
						snmp_msg_tmp.varbind[0].value=1;
						snmp_msg_tmp.varbind[1].flag=0;
					}
				}
				addtext(tmp);
				break;

			case EVENTS_UPS_LOW_VOLTAGE:
				sprintf(tmp,"Voltage on battery is low (below than 44V)");
				addtext(tmp);
				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=13;
					snmp_msg_tmp.spectrap[1]=0;
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=4;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=0x82;//
					snmp_msg_tmp.varbind[0].objname[6]=0xC8;//oid = 42019
					snmp_msg_tmp.varbind[0].objname[7]=0x23;//
					snmp_msg_tmp.varbind[0].objname[8]=3;
					snmp_msg_tmp.varbind[0].objname[9]=2;
					snmp_msg_tmp.varbind[0].objname[10]=0;
					snmp_msg_tmp.varbind[0].objname[11]=13;
					snmp_msg_tmp.varbind[0].len = 12;
					snmp_msg_tmp.varbind[0].value=1;
					snmp_msg_tmp.varbind[1].flag=0;
				}
				break;

			case EVENTS_MAC_UNALLOWED_PORT1:
			case EVENTS_MAC_UNALLOWED_PORT2:
			case EVENTS_MAC_UNALLOWED_PORT3:
			case EVENTS_MAC_UNALLOWED_PORT4:
			case EVENTS_MAC_UNALLOWED_PORT5:
			case EVENTS_MAC_UNALLOWED_PORT6:
			case EVENTS_MAC_UNALLOWED_PORT7:
			case EVENTS_MAC_UNALLOWED_PORT8:
			case EVENTS_MAC_UNALLOWED_PORT9:
			case EVENTS_MAC_UNALLOWED_PORT10:
			case EVENTS_MAC_UNALLOWED_PORT11:
			case EVENTS_MAC_UNALLOWED_PORT12:
			case EVENTS_MAC_UNALLOWED_PORT13:
			case EVENTS_MAC_UNALLOWED_PORT14:
			case EVENTS_MAC_UNALLOWED_PORT15:
			case EVENTS_MAC_UNALLOWED_PORT16:
			case EVENTS_MAC_UNALLOWED_PORT17:
			case EVENTS_MAC_UNALLOWED_PORT18:
			case EVENTS_MAC_UNALLOWED_PORT19:
			case EVENTS_MAC_UNALLOWED_PORT20:
				//mac = (u8 *)(ptr);
				get_mac(msg,mac);

				sprintf(tmp,"Unallowed MAC: %02X:%02X:%02X:%02X:%02X:%02X Port %s",mac[0],mac[1],mac[2],
						mac[3],mac[4],mac[5],add_port(type-EVENTS_MAC_UNALLOWED_PORT1));
				addtext(tmp);
				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=23;
					snmp_msg_tmp.spectrap[1]=0;
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=4;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=0x82;//
					snmp_msg_tmp.varbind[0].objname[6]=0xC8;//oid = 42019
					snmp_msg_tmp.varbind[0].objname[7]=0x23;//
					snmp_msg_tmp.varbind[0].objname[8]=3;
					snmp_msg_tmp.varbind[0].objname[9]=2;
					snmp_msg_tmp.varbind[0].objname[10]=0;
					snmp_msg_tmp.varbind[0].objname[11]=23;
					snmp_msg_tmp.varbind[0].len = 12;

					sprintf(tmp,"MAC %02X:%02X:%02X:%02X:%02X:%02X, port %d",
							mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],(u8)(type-EVENTS_MAC_UNALLOWED_PORT1+1));
					strcpy((char *)snmp_msg_tmp.varbind[0].value_str,(char *)tmp);
					snmp_msg_tmp.varbind[0].value_str_len = strlen(tmp);
					snmp_msg_tmp.varbind[0].type = BER_TYPE_OCTET_STRING;
					snmp_msg_tmp.varbind[1].flag=0;
				}
				break;

			case EVENTS_ERROR_DISABLED_PORT1:
			case EVENTS_ERROR_DISABLED_PORT2:
			case EVENTS_ERROR_DISABLED_PORT3:
			case EVENTS_ERROR_DISABLED_PORT4:
			case EVENTS_ERROR_DISABLED_PORT5:
			case EVENTS_ERROR_DISABLED_PORT6:
			case EVENTS_ERROR_DISABLED_PORT7:
			case EVENTS_ERROR_DISABLED_PORT8:
			case EVENTS_ERROR_DISABLED_PORT9:
			case EVENTS_ERROR_DISABLED_PORT10:
			case EVENTS_ERROR_DISABLED_PORT11:
			case EVENTS_ERROR_DISABLED_PORT12:
			case EVENTS_ERROR_DISABLED_PORT13:
			case EVENTS_ERROR_DISABLED_PORT14:
			case EVENTS_ERROR_DISABLED_PORT15:
			case EVENTS_ERROR_DISABLED_PORT16:
			case EVENTS_ERROR_DISABLED_PORT17:
			case EVENTS_ERROR_DISABLED_PORT18:
			case EVENTS_ERROR_DISABLED_PORT19:
			case EVENTS_ERROR_DISABLED_PORT20:
				sprintf(tmp,"Port %s Error-Disabled",add_port(type-EVENTS_ERROR_DISABLED_PORT1));
				addtext(tmp);
				if(get_snmp_state() == 1){
					snmp_msg_tmp.gentrap = SNMP_GENTRAP_ENTERPRISESPC;
					snmp_msg_tmp.spectrap[0]=24;
					snmp_msg_tmp.spectrap[1]=0;
					snmp_msg_tmp.varbind[0].flag=1;
					snmp_msg_tmp.varbind[0].objname[0]=0x2b;
					snmp_msg_tmp.varbind[0].objname[1]=6;
					snmp_msg_tmp.varbind[0].objname[2]=1;
					snmp_msg_tmp.varbind[0].objname[3]=4;
					snmp_msg_tmp.varbind[0].objname[4]=1;
					snmp_msg_tmp.varbind[0].objname[5]=0x82;//
					snmp_msg_tmp.varbind[0].objname[6]=0xC8;//oid = 42019
					snmp_msg_tmp.varbind[0].objname[7]=0x23;//
					snmp_msg_tmp.varbind[0].objname[8]=3;
					snmp_msg_tmp.varbind[0].objname[9]=2;
					snmp_msg_tmp.varbind[0].objname[10]=0;
					snmp_msg_tmp.varbind[0].objname[11]=24;
					snmp_msg_tmp.varbind[0].len = 12;
					snmp_msg_tmp.varbind[0].value=type-EVENTS_ERROR_DISABLED_PORT1+1;
					snmp_msg_tmp.varbind[1].flag=0;
				}
				break;

			case EVENTS_UP_PORT1:
			case EVENTS_UP_PORT2:
			case EVENTS_UP_PORT3:
			case EVENTS_UP_PORT4:
			case EVENTS_UP_PORT5:
			case EVENTS_UP_PORT6:
			case EVENTS_UP_PORT7:
			case EVENTS_UP_PORT8:
			case EVENTS_UP_PORT9:
			case EVENTS_UP_PORT10:
			case EVENTS_UP_PORT11:
			case EVENTS_UP_PORT12:
			case EVENTS_UP_PORT13:
			case EVENTS_UP_PORT14:
			case EVENTS_UP_PORT15:
			case EVENTS_UP_PORT16:
			case EVENTS_UP_PORT17:
			case EVENTS_UP_PORT18:
			case EVENTS_UP_PORT19:
			case EVENTS_UP_PORT20:
				sprintf(tmp,"Port %s is Up",add_port(type-EVENTS_UP_PORT1));
				addtext(tmp);
				EMPTY_TRAP
				break;




	/************************************************************************************************************/
			//группа set
			case EVENT_SET_NETIP:
				//ip[0] = *(u16 *)(ptr);
				//ip[1] = *(u16 *)(ptr+2);
				get_ip(msg,ip);
				sprintf(tmp,"Network settings edit: IP address %d.%d.%d.%d",
						uip_ipaddr1(ip),uip_ipaddr2(ip),uip_ipaddr3(ip),uip_ipaddr4(ip));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENT_SET_NETMASK:
				//ip[0] = *(u16 *)(ptr);
				//ip[1] = *(u16 *)(ptr+2);
				get_ip(msg,ip);
				sprintf(tmp,"Network settings edit: Netmask %d.%d.%d.%d",
						uip_ipaddr1(ip),uip_ipaddr2(ip),uip_ipaddr3(ip),uip_ipaddr4(ip));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENT_SET_NETGATE:
				//ip[0] = *(u16 *)(ptr);
				//ip[1] = *(u16 *)(ptr+2);
				get_ip(msg,ip);
				sprintf(tmp,"Network settings edit: Gateway IP address: %d.%d.%d.%d",
						uip_ipaddr1(&ip),uip_ipaddr2(&ip),uip_ipaddr3(&ip),uip_ipaddr4(&ip));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENT_SET_NETDNS:
				//ip[0] = *(u16 *)(ptr);
				//ip[1] = *(u16 *)(ptr+2);
				get_ip(msg,ip);
				sprintf(tmp,"Network settings edit: DNS IP address: %d.%d.%d.%d",
						uip_ipaddr1(&ip),uip_ipaddr2(&ip),uip_ipaddr3(&ip),uip_ipaddr4(&ip));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENT_SET_MAC:
				get_mac(msg,mac);
				sprintf(tmp,"Network settings edit: MAC: %02X:%02X:%02X:%02X:%02X:%02X",
						mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENT_SET_DHCPMODE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Network settings edit: DHCP Client %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENT_SET_GR_ARP:
				temp8 = get_u8(msg);
				sprintf(tmp,"Network settings edit: Gratuitous ARP %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_WDT_MODE_P1:
			case EVENTS_WDT_MODE_P1+1:
			case EVENTS_WDT_MODE_P1+2:
			case EVENTS_WDT_MODE_P1+3:
			case EVENTS_WDT_MODE_P1+4:
			case EVENTS_WDT_MODE_P1+5:
			case EVENTS_WDT_MODE_P1+6:
			case EVENTS_WDT_MODE_P1+7:
			case EVENTS_WDT_MODE_P1+8:
			case EVENTS_WDT_MODE_P1+9:
			case EVENTS_WDT_MODE_P1+10:
			case EVENTS_WDT_MODE_P1+11:
			case EVENTS_WDT_MODE_P1+12:
			case EVENTS_WDT_MODE_P1+13:
			case EVENTS_WDT_MODE_P1+14:
			case EVENTS_WDT_MODE_P1+15:
			case EVENTS_WDT_MODE_P1+16:
			case EVENTS_WDT_MODE_P1+17:
			case EVENTS_WDT_MODE_P1+18:
			case EVENTS_WDT_MODE_P1+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Autorestart settings edit: Port %s, Mode %s",add_port(type-EVENTS_WDT_MODE_P1),add_mode(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_WDT_IP_P1:
			case EVENTS_WDT_IP_P1+1:
			case EVENTS_WDT_IP_P1+2:
			case EVENTS_WDT_IP_P1+3:
			case EVENTS_WDT_IP_P1+4:
			case EVENTS_WDT_IP_P1+5:
			case EVENTS_WDT_IP_P1+6:
			case EVENTS_WDT_IP_P1+7:
			case EVENTS_WDT_IP_P1+8:
			case EVENTS_WDT_IP_P1+9:
			case EVENTS_WDT_IP_P1+10:
			case EVENTS_WDT_IP_P1+11:
			case EVENTS_WDT_IP_P1+12:
			case EVENTS_WDT_IP_P1+13:
			case EVENTS_WDT_IP_P1+14:
			case EVENTS_WDT_IP_P1+15:
			case EVENTS_WDT_IP_P1+16:
			case EVENTS_WDT_IP_P1+17:
			case EVENTS_WDT_IP_P1+18:
			case EVENTS_WDT_IP_P1+19:
				//ip[0] = *(u16 *)(ptr);
				//ip[1] = *(u16 *)(ptr+2);
				get_ip(msg,ip);
				sprintf(tmp,"Autorestart settings edit: Port %s IP address %d.%d.%d.%d",
						add_port(type-EVENTS_WDT_IP_P1),uip_ipaddr1(&ip),uip_ipaddr2(&ip),uip_ipaddr3(&ip),uip_ipaddr4(&ip));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_WDT_SPEED_DOWN_P1:
			case EVENTS_WDT_SPEED_DOWN_P1+1:
			case EVENTS_WDT_SPEED_DOWN_P1+2:
			case EVENTS_WDT_SPEED_DOWN_P1+3:
			case EVENTS_WDT_SPEED_DOWN_P1+4:
			case EVENTS_WDT_SPEED_DOWN_P1+5:
			case EVENTS_WDT_SPEED_DOWN_P1+6:
			case EVENTS_WDT_SPEED_DOWN_P1+7:
			case EVENTS_WDT_SPEED_DOWN_P1+8:
			case EVENTS_WDT_SPEED_DOWN_P1+9:
			case EVENTS_WDT_SPEED_DOWN_P1+10:
			case EVENTS_WDT_SPEED_DOWN_P1+11:
			case EVENTS_WDT_SPEED_DOWN_P1+12:
			case EVENTS_WDT_SPEED_DOWN_P1+13:
			case EVENTS_WDT_SPEED_DOWN_P1+14:
			case EVENTS_WDT_SPEED_DOWN_P1+15:
			case EVENTS_WDT_SPEED_DOWN_P1+16:
			case EVENTS_WDT_SPEED_DOWN_P1+17:
			case EVENTS_WDT_SPEED_DOWN_P1+18:
			case EVENTS_WDT_SPEED_DOWN_P1+19:
				//temp8 = (*(u8 *)ptr);
				temp32 = get_u32(msg);
				sprintf(tmp,"Autorestart settings edit: Port %s, MinSpeed %dKbps",
						add_port(type-EVENTS_WDT_SPEED_DOWN_P1),temp32);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_WDT_SPEED_UP_P1:
			case EVENTS_WDT_SPEED_UP_P1+1:
			case EVENTS_WDT_SPEED_UP_P1+2:
			case EVENTS_WDT_SPEED_UP_P1+3:
			case EVENTS_WDT_SPEED_UP_P1+4:
			case EVENTS_WDT_SPEED_UP_P1+5:
			case EVENTS_WDT_SPEED_UP_P1+6:
			case EVENTS_WDT_SPEED_UP_P1+7:
			case EVENTS_WDT_SPEED_UP_P1+8:
			case EVENTS_WDT_SPEED_UP_P1+9:
			case EVENTS_WDT_SPEED_UP_P1+10:
			case EVENTS_WDT_SPEED_UP_P1+11:
			case EVENTS_WDT_SPEED_UP_P1+12:
			case EVENTS_WDT_SPEED_UP_P1+13:
			case EVENTS_WDT_SPEED_UP_P1+14:
			case EVENTS_WDT_SPEED_UP_P1+15:
			case EVENTS_WDT_SPEED_UP_P1+16:
			case EVENTS_WDT_SPEED_UP_P1+17:
			case EVENTS_WDT_SPEED_UP_P1+18:
			case EVENTS_WDT_SPEED_UP_P1+19:
				temp32 = get_u32(msg);
				sprintf(tmp,"Autorestart settings edit: Port %s, MaxSpeed %dKbps",
						add_port(type-EVENTS_WDT_SPEED_UP_P1),temp32);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_PORT_ST_P1:
			case EVENTS_SET_PORT_ST_P1+1:
			case EVENTS_SET_PORT_ST_P1+2:
			case EVENTS_SET_PORT_ST_P1+3:
			case EVENTS_SET_PORT_ST_P1+4:
			case EVENTS_SET_PORT_ST_P1+5:
			case EVENTS_SET_PORT_ST_P1+6:
			case EVENTS_SET_PORT_ST_P1+7:
			case EVENTS_SET_PORT_ST_P1+8:
			case EVENTS_SET_PORT_ST_P1+9:
			case EVENTS_SET_PORT_ST_P1+10:
			case EVENTS_SET_PORT_ST_P1+11:
			case EVENTS_SET_PORT_ST_P1+12:
			case EVENTS_SET_PORT_ST_P1+13:
			case EVENTS_SET_PORT_ST_P1+14:
			case EVENTS_SET_PORT_ST_P1+15:
			case EVENTS_SET_PORT_ST_P1+16:
			case EVENTS_SET_PORT_ST_P1+17:
			case EVENTS_SET_PORT_ST_P1+18:
			case EVENTS_SET_PORT_ST_P1+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Port settings edit: Port %s, State %s",add_port(type-EVENTS_SET_PORT_ST_P1),add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;


			case EVENTS_SET_PORT_DPLX_P1:
			case EVENTS_SET_PORT_DPLX_P1+1:
			case EVENTS_SET_PORT_DPLX_P1+2:
			case EVENTS_SET_PORT_DPLX_P1+3:
			case EVENTS_SET_PORT_DPLX_P1+4:
			case EVENTS_SET_PORT_DPLX_P1+5:
			case EVENTS_SET_PORT_DPLX_P1+6:
			case EVENTS_SET_PORT_DPLX_P1+7:
			case EVENTS_SET_PORT_DPLX_P1+8:
			case EVENTS_SET_PORT_DPLX_P1+9:
			case EVENTS_SET_PORT_DPLX_P1+10:
			case EVENTS_SET_PORT_DPLX_P1+11:
			case EVENTS_SET_PORT_DPLX_P1+12:
			case EVENTS_SET_PORT_DPLX_P1+13:
			case EVENTS_SET_PORT_DPLX_P1+14:
			case EVENTS_SET_PORT_DPLX_P1+15:
			case EVENTS_SET_PORT_DPLX_P1+16:
			case EVENTS_SET_PORT_DPLX_P1+17:
			case EVENTS_SET_PORT_DPLX_P1+18:
			case EVENTS_SET_PORT_DPLX_P1+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Port settings edit: Port %s, Speed & Duplex %s",add_port(type-EVENTS_SET_PORT_DPLX_P1),add_duplex(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;


			case EVENTS_SET_PORT_FLOW_P1:
			case EVENTS_SET_PORT_FLOW_P1+1:
			case EVENTS_SET_PORT_FLOW_P1+2:
			case EVENTS_SET_PORT_FLOW_P1+3:
			case EVENTS_SET_PORT_FLOW_P1+4:
			case EVENTS_SET_PORT_FLOW_P1+5:
			case EVENTS_SET_PORT_FLOW_P1+6:
			case EVENTS_SET_PORT_FLOW_P1+7:
			case EVENTS_SET_PORT_FLOW_P1+8:
			case EVENTS_SET_PORT_FLOW_P1+9:
			case EVENTS_SET_PORT_FLOW_P1+10:
			case EVENTS_SET_PORT_FLOW_P1+11:
			case EVENTS_SET_PORT_FLOW_P1+12:
			case EVENTS_SET_PORT_FLOW_P1+13:
			case EVENTS_SET_PORT_FLOW_P1+14:
			case EVENTS_SET_PORT_FLOW_P1+15:
			case EVENTS_SET_PORT_FLOW_P1+16:
			case EVENTS_SET_PORT_FLOW_P1+17:
			case EVENTS_SET_PORT_FLOW_P1+18:
			case EVENTS_SET_PORT_FLOW_P1+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Port settings edit: Port %s, Flow control %s",add_port(type-EVENTS_SET_PORT_FLOW_P1),add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_PORT_SS_P1:
			case EVENTS_SET_PORT_SS_P1+1:
			case EVENTS_SET_PORT_SS_P1+2:
			case EVENTS_SET_PORT_SS_P1+3:
			case EVENTS_SET_PORT_SS_P1+4:
			case EVENTS_SET_PORT_SS_P1+5:
			case EVENTS_SET_PORT_SS_P1+6:
			case EVENTS_SET_PORT_SS_P1+7:
			case EVENTS_SET_PORT_SS_P1+8:
			case EVENTS_SET_PORT_SS_P1+9:
			case EVENTS_SET_PORT_SS_P1+10:
			case EVENTS_SET_PORT_SS_P1+11:
			case EVENTS_SET_PORT_SS_P1+12:
			case EVENTS_SET_PORT_SS_P1+13:
			case EVENTS_SET_PORT_SS_P1+14:
			case EVENTS_SET_PORT_SS_P1+15:
			case EVENTS_SET_PORT_SS_P1+16:
			case EVENTS_SET_PORT_SS_P1+17:
			case EVENTS_SET_PORT_SS_P1+18:
			case EVENTS_SET_PORT_SS_P1+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Soft start edit: Port %s, State %s",add_port(type-EVENTS_SET_PORT_SS_P1),add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_PORT_POE_P1:
			case EVENTS_SET_PORT_POE_P1+1:
			case EVENTS_SET_PORT_POE_P1+2:
			case EVENTS_SET_PORT_POE_P1+3:
			case EVENTS_SET_PORT_POE_P1+4:
			case EVENTS_SET_PORT_POE_P1+5:
			case EVENTS_SET_PORT_POE_P1+6:
			case EVENTS_SET_PORT_POE_P1+7:
			case EVENTS_SET_PORT_POE_P1+8:
			case EVENTS_SET_PORT_POE_P1+9:
			case EVENTS_SET_PORT_POE_P1+10:
			case EVENTS_SET_PORT_POE_P1+11:
			case EVENTS_SET_PORT_POE_P1+12:
			case EVENTS_SET_PORT_POE_P1+13:
			case EVENTS_SET_PORT_POE_P1+14:
			case EVENTS_SET_PORT_POE_P1+15:
			case EVENTS_SET_PORT_POE_P1+16:
			case EVENTS_SET_PORT_POE_P1+17:
			case EVENTS_SET_PORT_POE_P1+18:
			case EVENTS_SET_PORT_POE_P1+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Port settings edit: Port %s, PoE State ",add_port(type-EVENTS_SET_PORT_POE_P1));
				switch(temp8){
					case 0: strcat(tmp,"Disabled");break;
					case 1: strcat(tmp,"Auto");break;
					case 2: strcat(tmp,"A+Passive B");break;
					case 3: strcat(tmp,"UltraPoE");break;
					case 4: strcat(tmp,"Only A");break;
					case 5: strcat(tmp,"Only B");break;
				}
				addtext(tmp);
				EMPTY_TRAP
				break;


			case EVENTS_SET_PORT_POE_A_P1_LIM:
			case EVENTS_SET_PORT_POE_A_P1_LIM+1:
			case EVENTS_SET_PORT_POE_A_P1_LIM+2:
			case EVENTS_SET_PORT_POE_A_P1_LIM+3:
			case EVENTS_SET_PORT_POE_A_P1_LIM+4:
			case EVENTS_SET_PORT_POE_A_P1_LIM+5:
			case EVENTS_SET_PORT_POE_A_P1_LIM+6:
			case EVENTS_SET_PORT_POE_A_P1_LIM+7:
			case EVENTS_SET_PORT_POE_A_P1_LIM+8:
			case EVENTS_SET_PORT_POE_A_P1_LIM+9:
			case EVENTS_SET_PORT_POE_A_P1_LIM+10:
			case EVENTS_SET_PORT_POE_A_P1_LIM+11:
			case EVENTS_SET_PORT_POE_A_P1_LIM+12:
			case EVENTS_SET_PORT_POE_A_P1_LIM+13:
			case EVENTS_SET_PORT_POE_A_P1_LIM+14:
			case EVENTS_SET_PORT_POE_A_P1_LIM+15:
			case EVENTS_SET_PORT_POE_A_P1_LIM+16:
			case EVENTS_SET_PORT_POE_A_P1_LIM+17:
			case EVENTS_SET_PORT_POE_A_P1_LIM+18:
			case EVENTS_SET_PORT_POE_A_P1_LIM+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Port settings edit: Port %s, PoE A Limit %s",add_port(type-EVENTS_SET_PORT_POE_A_P1_LIM),add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_PORT_POE_B_P1_LIM:
			case EVENTS_SET_PORT_POE_B_P1_LIM+1:
			case EVENTS_SET_PORT_POE_B_P1_LIM+2:
			case EVENTS_SET_PORT_POE_B_P1_LIM+3:
			case EVENTS_SET_PORT_POE_B_P1_LIM+4:
			case EVENTS_SET_PORT_POE_B_P1_LIM+5:
			case EVENTS_SET_PORT_POE_B_P1_LIM+6:
			case EVENTS_SET_PORT_POE_B_P1_LIM+7:
			case EVENTS_SET_PORT_POE_B_P1_LIM+8:
			case EVENTS_SET_PORT_POE_B_P1_LIM+9:
			case EVENTS_SET_PORT_POE_B_P1_LIM+10:
			case EVENTS_SET_PORT_POE_B_P1_LIM+11:
			case EVENTS_SET_PORT_POE_B_P1_LIM+12:
			case EVENTS_SET_PORT_POE_B_P1_LIM+13:
			case EVENTS_SET_PORT_POE_B_P1_LIM+14:
			case EVENTS_SET_PORT_POE_B_P1_LIM+15:
			case EVENTS_SET_PORT_POE_B_P1_LIM+16:
			case EVENTS_SET_PORT_POE_B_P1_LIM+17:
			case EVENTS_SET_PORT_POE_B_P1_LIM+18:
			case EVENTS_SET_PORT_POE_B_P1_LIM+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Port settings edit: Port %s, PoE B Limit %s",add_port(type-EVENTS_SET_PORT_POE_B_P1_LIM),add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;



			case EVENTS_SFPMODE_P1:
			case EVENTS_SFPMODE_P1+1:
			case EVENTS_SFPMODE_P1+2:
			case EVENTS_SFPMODE_P1+3:
			case EVENTS_SFPMODE_P1+4:
			case EVENTS_SFPMODE_P1+5:
			case EVENTS_SFPMODE_P1+6:
			case EVENTS_SFPMODE_P1+7:
			case EVENTS_SFPMODE_P1+8:
			case EVENTS_SFPMODE_P1+9:
			case EVENTS_SFPMODE_P1+10:
			case EVENTS_SFPMODE_P1+11:
			case EVENTS_SFPMODE_P1+12:
			case EVENTS_SFPMODE_P1+13:
			case EVENTS_SFPMODE_P1+14:
			case EVENTS_SFPMODE_P1+15:
			case EVENTS_SFPMODE_P1+16:
			case EVENTS_SFPMODE_P1+17:
			case EVENTS_SFPMODE_P1+18:
			case EVENTS_SFPMODE_P1+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Port settings edit: Port %s, SFP Mode %d",add_port(type-EVENTS_SFPMODE_P1),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;





			case EVENTS_SET_LANG:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				if(temp8 == RUS)
					sprintf(tmp,"Interface settings edit: Language RUS");
				else
					sprintf(tmp,"Interface settings edit: Language ENG");
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_LOGIN:
				get_interface_users_username(0,temp);
				sprintf(tmp,"Interface settings edit: Login %s",temp);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_PASSWD:
				get_interface_users_password(0,temp);
				sprintf(tmp,"Interface settings edit: Password %s",temp);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_USER_RULE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				if(temp8==1){
					sprintf(tmp,"Interface settings edit: Rule ADMIN");
				}
				else if(temp8 == 2){
					sprintf(tmp,"Interface settings edit: Rule USER");
				}else{
					sprintf(tmp,"Interface settings edit: Rule NO RULE");
				}
				addtext(tmp);
				EMPTY_TRAP
				break;



			case EVENTS_SET_NAME:
				get_interface_name(temp);
				if(strlen((char *)temp)<DESCRIPT_LEN){
					memset(tmp,0,sizeof(tmp));
					http_url_decode((char*)temp,tmp,strlen(temp));
					for(uint8_t i=0;i<strlen(tmp);i++){
						if(tmp[i]=='+') tmp[i] = ' ';
						if(tmp[i]=='%') tmp[i]=' ';
					}
					sprintf(temp,"Interface settings edit: Device Name %s",tmp);
					addtext(temp);
					EMPTY_TRAP
				}
				break;

			case EVENTS_SET_LOCATION:
				get_interface_location(temp);
				if(strlen((char *)temp)<DESCRIPT_LEN){
					http_url_decode((char*)temp,tmp,strlen(temp));
					for(uint8_t i=0;i<strlen(tmp);i++){
						if(tmp[i]=='+') tmp[i] = ' ';
						if(tmp[i]=='%') tmp[i]=' ';
					}
					sprintf(temp,"Interface settings edit: Device Location %s",tmp);
					addtext(temp);
					EMPTY_TRAP
				}
				break;

			case EVENTS_SET_COMPANY:
				get_interface_contact(temp);
				if(strlen((char *)temp)<DESCRIPT_LEN){
					http_url_decode((char*)temp,tmp,strlen(temp));
					for(uint8_t i=0;i<strlen(tmp);i++){
						if(tmp[i]=='+') tmp[i] = ' ';
						if(tmp[i]=='%') tmp[i]=' ';
					}
					sprintf(temp,"Interface settings edit: Service Company %s",tmp);
					addtext(temp);
					EMPTY_TRAP
				}
				break;


			case EVENTS_SET_SMTP_STATE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"SMTP settings edit: State %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_SMTP_IP:
				//ip[0] = *(u16 *)(ptr);
				//ip[1] = *(u16 *)(ptr+2);
				get_ip(msg,ip);
				sprintf(tmp,"SMTP settings edit: Server IP address %d.%d.%d.%d",
					uip_ipaddr1(&ip),uip_ipaddr2(&ip),uip_ipaddr3(&ip),uip_ipaddr4(&ip));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_SMTP_TO:
				get_smtp_to(temp);
				sprintf(tmp,"SMTP settings edit: To %s",(char *)(temp));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_SMTP_TO2:
				get_smtp_to2(temp);
				sprintf(tmp,"SMTP settings edit: To2 %s",(char *)(temp));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_SMTP_TO3:
				get_smtp_to3(temp);
				sprintf(tmp,"SMTP settings edit: To3 %s",(char *)(temp));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_SMTP_FROM:
				get_smtp_from(temp);
				sprintf(tmp,"SMTP settings edit: From %s",(char *)(temp));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_SMTP_SUBJ:
				get_smtp_subj(temp);
				sprintf(tmp,"SMTP settings edit: Subject %s",(char *)(temp));
				addtext(tmp);
				EMPTY_TRAP
				break;


			case EVENTS_SET_SMTP_LOGIN:
				get_smtp_login(temp);
				sprintf(tmp,"SMTP settings edit: Login %s",(char *)(temp));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_SMTP_PASS:
				get_smtp_pass(temp);
				sprintf(tmp,"SMTP settings edit: Password %s",(char *)(temp));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_SMTP_PORT:
				temp32 = get_u32(msg);
				sprintf(tmp,"SMTP settings edit: Port %d",(u16)temp32);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_SNTP_STATE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"SNTP settings edit: State %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_SNTP_IP:
				//ip[0] = *(u16 *)(ptr);
				//ip[1] = *(u16 *)(ptr+2);
				get_ip(msg,ip);
				sprintf(tmp,"SNTP settings edit: Server IP address %d.%d.%d.%d",
					uip_ipaddr1(&ip),uip_ipaddr2(&ip),uip_ipaddr3(&ip),uip_ipaddr4(&ip));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_SNTP_TZONE:
				temp8 = get_u8(msg);
				sprintf(tmp,"SNTP settings edit: Timezone UTC %02d:0",(i8)temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_SNTP_PERIOD:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"SNTP settings edit: Synchronization period %dmin.",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SET_SYSLOG_STATE:
				temp8 = get_u8(msg);
				//temp8 = (*(u8 *)ptr);
				sprintf(tmp,"Syslog settings edit: State %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;


			case EVENTS_SET_SYSLOG_IP:
				//ip[0] = *(u16 *)(ptr);
				//ip[1] = *(u16 *)(ptr+2);
				get_ip(msg,ip);
				sprintf(tmp,"Syslog settings edit: Server IP address %d.%d.%d.%d",
					uip_ipaddr1(&ip),uip_ipaddr2(&ip),uip_ipaddr3(&ip),uip_ipaddr4(&ip));
				addtext(tmp);
				EMPTY_TRAP
				break;

	#if 0
			case EVENTS_EVENTLIST_BASE_S:
				temp8 = (*(u8 *)ptr);
				sprintf(tmp,"Event list settings edit: Base settings group, State %s",add_state(temp8));
				addtext(tmp);
				break;

			case EVENTS_EVENTLIST_PORT_S:
				temp8 = (*(u8 *)ptr);
				sprintf(tmp,"Event list settings edit: Port settings group, State %s",add_state(temp8));
				addtext(tmp);
				break;


			case EVENTS_EVENTLIST_VLAN_S:
				temp8 = (*(u8 *)ptr);
				sprintf(tmp,"Event list settings edit: VLAN settings group, State %s",add_state(temp8));
				addtext(tmp);
				break;

			case EVENTS_EVENTLIST_STP_S:
				temp8 = (*(u8 *)ptr);
				sprintf(tmp,"Event list settings edit: STP settings group, State %s",add_state(temp8));
				addtext(tmp);
				break;

			case EVENTS_EVENTLIST_QOS_S:
				temp8 = (*(u8 *)ptr);
				sprintf(tmp,"Event list settings edit: QoS settings group, State %s",add_state(temp8));
				addtext(tmp);
				break;

			case EVENTS_EVENTLIST_OTHER_S:
				temp8 = (*(u8 *)ptr);
				sprintf(tmp,"Event list settings edit: Other settings group, State %s",add_state(temp8));
				addtext(tmp);
				break;

			case EVENTS_EVENTLIST_LINK_T:
				temp8 = (*(u8 *)ptr);
				sprintf(tmp,"Event list settings edit: Link change group, State %s",add_state(temp8));
				addtext(tmp);
				break;

			case EVENTS_EVENTLIST_POE_T:
				temp8 = (*(u8 *)ptr);
				sprintf(tmp,"Event list settings edit: PoE change group, State %s",add_state(temp8));
				addtext(tmp);
				break;

			case EVENTS_EVENTLIST_STP_T:
				temp8 = (*(u8 *)ptr);
				sprintf(tmp,"Event list settings edit: STP events group, State %s",add_state(temp8));
				addtext(tmp);
				break;

			case EVENTS_EVENTLIST_SLINK_T:
				temp8 = (*(u8 *)ptr);
				sprintf(tmp,"Event list settings edit: Special Function (Link), State %s",add_state(temp8));
				addtext(tmp);
				break;

			case EVENTS_EVENTLIST_SPING_T:
				temp8 = (*(u8 *)ptr);
				sprintf(tmp,"Event list settings edit: Special Function (Ping), State %s",add_state(temp8));
				addtext(tmp);
				break;

			case EVENTS_EVENTLIST_SYSTEM_T:
				temp8 = (*(u8 *)ptr);
				sprintf(tmp,"Event list settings edit: System events group, State %s",add_state(temp8));
				addtext(tmp);
				break;

			case EVENTS_EVENTLIST_UPS_T:
				temp8 = (*(u8 *)ptr);
				sprintf(tmp,"Event list settings edit: UPS events group, State %s",add_state(temp8));
				addtext(tmp);
				break;

			case EVENTS_EVENTLIST_ALARM_T:
				temp8 = (*(u8 *)ptr);
				sprintf(tmp,"Event list settings edit: Alarm events group, State %s",add_state(temp8));
				addtext(tmp);
				break;
	#endif

			case EVENTS_EVENTLIST_BASE_S:
			case EVENTS_EVENTLIST_PORT_S:
			case EVENTS_EVENTLIST_VLAN_S:
			case EVENTS_EVENTLIST_STP_S:
			case EVENTS_EVENTLIST_QOS_S:
			case EVENTS_EVENTLIST_OTHER_S:
			case EVENTS_EVENTLIST_LINK_T:
			case EVENTS_EVENTLIST_POE_T:
			case EVENTS_EVENTLIST_STP_T:
			case EVENTS_EVENTLIST_SLINK_T:
			case EVENTS_EVENTLIST_SPING_T:
			case EVENTS_EVENTLIST_SSPEED_T:
			case EVENTS_EVENTLIST_SYSTEM_T:
			case EVENTS_EVENTLIST_UPS_T:
			case EVENTS_EVENTLIST_ALARM_T:
			case EVENTS_EVENTLIST_MAC_T:
				sprintf(tmp,"Event list settings edit, code=%lu",type);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_ALARM1_STATE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Alarm settings edit: Line 1, State %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_ALARM2_STATE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Alarm settings edit: Line 2, State %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_ALARM3_STATE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Alarm settings edit: Line 3, State %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_ALARM1_FRONT:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Alarm settings edit: Line 1, Alarm state %s",add_front(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_ALARM2_FRONT:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Alarm settings edit: Line 2, Alarm state %s",add_front(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_ALARM3_FRONT:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Alarm settings edit: Line 3, Alarm state %s",add_front(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_RATELIM_MODE_P1:
			case EVENTS_RATELIM_MODE_P1+1:
			case EVENTS_RATELIM_MODE_P1+2:
			case EVENTS_RATELIM_MODE_P1+3:
			case EVENTS_RATELIM_MODE_P1+4:
			case EVENTS_RATELIM_MODE_P1+5:
			case EVENTS_RATELIM_MODE_P1+6:
			case EVENTS_RATELIM_MODE_P1+7:
			case EVENTS_RATELIM_MODE_P1+8:
			case EVENTS_RATELIM_MODE_P1+9:
			case EVENTS_RATELIM_MODE_P1+10:
			case EVENTS_RATELIM_MODE_P1+11:
			case EVENTS_RATELIM_MODE_P1+12:
			case EVENTS_RATELIM_MODE_P1+13:
			case EVENTS_RATELIM_MODE_P1+14:
			case EVENTS_RATELIM_MODE_P1+15:
			case EVENTS_RATELIM_MODE_P1+16:
			case EVENTS_RATELIM_MODE_P1+17:
			case EVENTS_RATELIM_MODE_P1+18:
			case EVENTS_RATELIM_MODE_P1+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Rate Limit settings edit: Port %s, Limit mode %d",add_port(type-EVENTS_RATELIM_MODE_P1),(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_RATELIM_RX_P1:
			case EVENTS_RATELIM_RX_P1+1:
			case EVENTS_RATELIM_RX_P1+2:
			case EVENTS_RATELIM_RX_P1+3:
			case EVENTS_RATELIM_RX_P1+4:
			case EVENTS_RATELIM_RX_P1+5:
			case EVENTS_RATELIM_RX_P1+6:
			case EVENTS_RATELIM_RX_P1+7:
			case EVENTS_RATELIM_RX_P1+8:
			case EVENTS_RATELIM_RX_P1+9:
			case EVENTS_RATELIM_RX_P1+10:
			case EVENTS_RATELIM_RX_P1+11:
			case EVENTS_RATELIM_RX_P1+12:
			case EVENTS_RATELIM_RX_P1+13:
			case EVENTS_RATELIM_RX_P1+14:
			case EVENTS_RATELIM_RX_P1+15:
			case EVENTS_RATELIM_RX_P1+16:
			case EVENTS_RATELIM_RX_P1+17:
			case EVENTS_RATELIM_RX_P1+18:
			case EVENTS_RATELIM_RX_P1+19:
				//temp32 = (*(u32 *)ptr);
				temp32 = get_u32(msg);
				sprintf(tmp,"Rate Limit settings edit: Port %s, RX Limit %lu",add_port(type-EVENTS_RATELIM_RX_P1),(temp32));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_RATELIM_TX_P1:
			case EVENTS_RATELIM_TX_P1+1:
			case EVENTS_RATELIM_TX_P1+2:
			case EVENTS_RATELIM_TX_P1+3:
			case EVENTS_RATELIM_TX_P1+4:
			case EVENTS_RATELIM_TX_P1+5:
			case EVENTS_RATELIM_TX_P1+6:
			case EVENTS_RATELIM_TX_P1+7:
			case EVENTS_RATELIM_TX_P1+8:
			case EVENTS_RATELIM_TX_P1+9:
			case EVENTS_RATELIM_TX_P1+10:
			case EVENTS_RATELIM_TX_P1+11:
			case EVENTS_RATELIM_TX_P1+12:
			case EVENTS_RATELIM_TX_P1+13:
			case EVENTS_RATELIM_TX_P1+14:
			case EVENTS_RATELIM_TX_P1+15:
			case EVENTS_RATELIM_TX_P1+16:
			case EVENTS_RATELIM_TX_P1+17:
			case EVENTS_RATELIM_TX_P1+18:
			case EVENTS_RATELIM_TX_P1+19:
				//temp32 = (*(u32 *)ptr);
				temp32 = get_u32(msg);
				sprintf(tmp,"Rate Limit settings edit: Port %s, TX Limit %lu",add_port(type-EVENTS_RATELIM_TX_P1),(temp32));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_COS_STATE_P1:
			case EVENTS_COS_STATE_P1+1:
			case EVENTS_COS_STATE_P1+2:
			case EVENTS_COS_STATE_P1+3:
			case EVENTS_COS_STATE_P1+4:
			case EVENTS_COS_STATE_P1+5:
			case EVENTS_COS_STATE_P1+6:
			case EVENTS_COS_STATE_P1+7:
			case EVENTS_COS_STATE_P1+8:
			case EVENTS_COS_STATE_P1+9:
			case EVENTS_COS_STATE_P1+10:
			case EVENTS_COS_STATE_P1+11:
			case EVENTS_COS_STATE_P1+12:
			case EVENTS_COS_STATE_P1+13:
			case EVENTS_COS_STATE_P1+14:
			case EVENTS_COS_STATE_P1+15:
			case EVENTS_COS_STATE_P1+16:
			case EVENTS_COS_STATE_P1+17:
			case EVENTS_COS_STATE_P1+18:
			case EVENTS_COS_STATE_P1+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"QoS settings edit: Port %s, CoS State %s",add_port(type-EVENTS_COS_STATE_P1),add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;


			case EVENTS_TOS_STATE_P1:
			case EVENTS_TOS_STATE_P1+1:
			case EVENTS_TOS_STATE_P1+2:
			case EVENTS_TOS_STATE_P1+3:
			case EVENTS_TOS_STATE_P1+4:
			case EVENTS_TOS_STATE_P1+5:
			case EVENTS_TOS_STATE_P1+6:
			case EVENTS_TOS_STATE_P1+7:
			case EVENTS_TOS_STATE_P1+8:
			case EVENTS_TOS_STATE_P1+9:
			case EVENTS_TOS_STATE_P1+10:
			case EVENTS_TOS_STATE_P1+11:
			case EVENTS_TOS_STATE_P1+12:
			case EVENTS_TOS_STATE_P1+13:
			case EVENTS_TOS_STATE_P1+14:
			case EVENTS_TOS_STATE_P1+15:
			case EVENTS_TOS_STATE_P1+16:
			case EVENTS_TOS_STATE_P1+17:
			case EVENTS_TOS_STATE_P1+18:
			case EVENTS_TOS_STATE_P1+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"QoS settings edit: Port %s, ToS State %s",add_port(type-EVENTS_TOS_STATE_P1),add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_QOS_RULE_P1:
			case EVENTS_QOS_RULE_P1+1:
			case EVENTS_QOS_RULE_P1+2:
			case EVENTS_QOS_RULE_P1+3:
			case EVENTS_QOS_RULE_P1+4:
			case EVENTS_QOS_RULE_P1+5:
			case EVENTS_QOS_RULE_P1+6:
			case EVENTS_QOS_RULE_P1+7:
			case EVENTS_QOS_RULE_P1+8:
			case EVENTS_QOS_RULE_P1+9:
			case EVENTS_QOS_RULE_P1+10:
			case EVENTS_QOS_RULE_P1+11:
			case EVENTS_QOS_RULE_P1+12:
			case EVENTS_QOS_RULE_P1+13:
			case EVENTS_QOS_RULE_P1+14:
			case EVENTS_QOS_RULE_P1+15:
			case EVENTS_QOS_RULE_P1+16:
			case EVENTS_QOS_RULE_P1+17:
			case EVENTS_QOS_RULE_P1+18:
			case EVENTS_QOS_RULE_P1+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"QoS settings edit: Port %s, Rule %s",add_port(type-EVENTS_QOS_RULE_P1),add_rule(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_QOS_DEFPRI_P1:
			case EVENTS_QOS_DEFPRI_P1+1:
			case EVENTS_QOS_DEFPRI_P1+2:
			case EVENTS_QOS_DEFPRI_P1+3:
			case EVENTS_QOS_DEFPRI_P1+4:
			case EVENTS_QOS_DEFPRI_P1+5:
			case EVENTS_QOS_DEFPRI_P1+6:
			case EVENTS_QOS_DEFPRI_P1+7:
			case EVENTS_QOS_DEFPRI_P1+8:
			case EVENTS_QOS_DEFPRI_P1+9:
			case EVENTS_QOS_DEFPRI_P1+10:
			case EVENTS_QOS_DEFPRI_P1+11:
			case EVENTS_QOS_DEFPRI_P1+12:
			case EVENTS_QOS_DEFPRI_P1+13:
			case EVENTS_QOS_DEFPRI_P1+14:
			case EVENTS_QOS_DEFPRI_P1+15:
			case EVENTS_QOS_DEFPRI_P1+16:
			case EVENTS_QOS_DEFPRI_P1+17:
			case EVENTS_QOS_DEFPRI_P1+18:
			case EVENTS_QOS_DEFPRI_P1+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"QoS settings edit: Port %s, Default priority %d",add_port(type-EVENTS_QOS_DEFPRI_P1),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_QOS_STATE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"QoS settings edit: State %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_QOS_POLICY:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				if (temp8 == 0)
					sprintf(tmp,"QoS settings edit: Policy  Strict priority");
				else
					sprintf(tmp,"QoS settings edit: Policy  Weighted fair priority");
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_PB_VLAN_STATE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Port Base VLAN settings edit: State %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;


			case EVENTS_VLAN_STATE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"VLAN settings edit: State %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;



			case EVENTS_VLAN_MVID:
				//temp32 = (*(u16 *)ptr);
				temp32 = get_u32(msg);
				sprintf(tmp,"VLAN settings edit: Managment VID %lu",temp32);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_VLAN_STATE_P1:
			case EVENTS_VLAN_STATE_P1+1:
			case EVENTS_VLAN_STATE_P1+2:
			case EVENTS_VLAN_STATE_P1+3:
			case EVENTS_VLAN_STATE_P1+4:
			case EVENTS_VLAN_STATE_P1+5:
			case EVENTS_VLAN_STATE_P1+6:
			case EVENTS_VLAN_STATE_P1+7:
			case EVENTS_VLAN_STATE_P1+8:
			case EVENTS_VLAN_STATE_P1+9:
			case EVENTS_VLAN_STATE_P1+10:
			case EVENTS_VLAN_STATE_P1+11:
			case EVENTS_VLAN_STATE_P1+12:
			case EVENTS_VLAN_STATE_P1+13:
			case EVENTS_VLAN_STATE_P1+14:
			case EVENTS_VLAN_STATE_P1+15:
			case EVENTS_VLAN_STATE_P1+16:
			case EVENTS_VLAN_STATE_P1+17:
			case EVENTS_VLAN_STATE_P1+18:
			case EVENTS_VLAN_STATE_P1+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"VLAN Trunking settings edit: Port %s, State %s",add_port(type-EVENTS_VLAN_STATE_P1),add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_VLAN_TRUNK_STATE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"VLAN Trunking settings edit: State %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_VLAN_DVID_P1:
			case EVENTS_VLAN_DVID_P1+1:
			case EVENTS_VLAN_DVID_P1+2:
			case EVENTS_VLAN_DVID_P1+3:
			case EVENTS_VLAN_DVID_P1+4:
			case EVENTS_VLAN_DVID_P1+5:
			case EVENTS_VLAN_DVID_P1+6:
			case EVENTS_VLAN_DVID_P1+7:
			case EVENTS_VLAN_DVID_P1+8:
			case EVENTS_VLAN_DVID_P1+9:
			case EVENTS_VLAN_DVID_P1+10:
			case EVENTS_VLAN_DVID_P1+11:
			case EVENTS_VLAN_DVID_P1+12:
			case EVENTS_VLAN_DVID_P1+13:
			case EVENTS_VLAN_DVID_P1+14:
			case EVENTS_VLAN_DVID_P1+15:
			case EVENTS_VLAN_DVID_P1+16:
			case EVENTS_VLAN_DVID_P1+17:
			case EVENTS_VLAN_DVID_P1+18:
			case EVENTS_VLAN_DVID_P1+19:
				//temp32 = (*(u16 *)ptr);
				temp32 = get_u32(msg);
				sprintf(tmp,"VLAN settings edit: Port %s, Default VID %lu",add_port(type-EVENTS_VLAN_DVID_P1),temp32);
				addtext(tmp);
				EMPTY_TRAP
				break;


			case EVENTS_VLAN_EDIT:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"VLAN settings edit: VLAN#%d edit",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;



			case EVENTS_STP_STATE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"STP settings edit: State %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_STP_PROTO:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"STP settings edit: Protocol %s",add_proto(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_STP_PRI:
				//temp32 = (*(u16 *)ptr);
				temp32 = get_u32(msg);
				sprintf(tmp,"STP settings edit: Bridge priority %lu",temp32);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_STP_MAGE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"STP settings edit: Bridge Max Age %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_STP_HTIME:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"STP settings edit: Bridge Hello Time %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_STP_FDELAY:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"STP settings edit: Forward Delay Time %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_STP_HCNT:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"STP settings edit: Bridge Hold Count %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_STP_MDELAY:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"STP settings edit: Bridge Migrate Delay %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_STP_PORT_EN_P1:
			case EVENTS_STP_PORT_EN_P1+1:
			case EVENTS_STP_PORT_EN_P1+2:
			case EVENTS_STP_PORT_EN_P1+3:
			case EVENTS_STP_PORT_EN_P1+4:
			case EVENTS_STP_PORT_EN_P1+5:
			case EVENTS_STP_PORT_EN_P1+6:
			case EVENTS_STP_PORT_EN_P1+7:
			case EVENTS_STP_PORT_EN_P1+8:
			case EVENTS_STP_PORT_EN_P1+9:
			case EVENTS_STP_PORT_EN_P1+10:
			case EVENTS_STP_PORT_EN_P1+11:
			case EVENTS_STP_PORT_EN_P1+12:
			case EVENTS_STP_PORT_EN_P1+13:
			case EVENTS_STP_PORT_EN_P1+14:
			case EVENTS_STP_PORT_EN_P1+15:
			case EVENTS_STP_PORT_EN_P1+16:
			case EVENTS_STP_PORT_EN_P1+17:
			case EVENTS_STP_PORT_EN_P1+18:
			case EVENTS_STP_PORT_EN_P1+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"STP settings edit: Port %s, State %s",add_port(type-EVENTS_STP_PORT_EN_P1),add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_STP_PORT_ST_P1:
			case EVENTS_STP_PORT_ST_P1+1:
			case EVENTS_STP_PORT_ST_P1+2:
			case EVENTS_STP_PORT_ST_P1+3:
			case EVENTS_STP_PORT_ST_P1+4:
			case EVENTS_STP_PORT_ST_P1+5:
			case EVENTS_STP_PORT_ST_P1+6:
			case EVENTS_STP_PORT_ST_P1+7:
			case EVENTS_STP_PORT_ST_P1+8:
			case EVENTS_STP_PORT_ST_P1+9:
			case EVENTS_STP_PORT_ST_P1+10:
			case EVENTS_STP_PORT_ST_P1+11:
			case EVENTS_STP_PORT_ST_P1+12:
			case EVENTS_STP_PORT_ST_P1+13:
			case EVENTS_STP_PORT_ST_P1+14:
			case EVENTS_STP_PORT_ST_P1+15:
			case EVENTS_STP_PORT_ST_P1+16:
			case EVENTS_STP_PORT_ST_P1+17:
			case EVENTS_STP_PORT_ST_P1+18:
			case EVENTS_STP_PORT_ST_P1+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"STP settings edit: Port %s, State %s",add_port(type-EVENTS_STP_PORT_ST_P1),add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_STP_PORT_PRI_P1:
			case EVENTS_STP_PORT_PRI_P1+1:
			case EVENTS_STP_PORT_PRI_P1+2:
			case EVENTS_STP_PORT_PRI_P1+3:
			case EVENTS_STP_PORT_PRI_P1+4:
			case EVENTS_STP_PORT_PRI_P1+5:
			case EVENTS_STP_PORT_PRI_P1+6:
			case EVENTS_STP_PORT_PRI_P1+7:
			case EVENTS_STP_PORT_PRI_P1+8:
			case EVENTS_STP_PORT_PRI_P1+9:
			case EVENTS_STP_PORT_PRI_P1+10:
			case EVENTS_STP_PORT_PRI_P1+11:
			case EVENTS_STP_PORT_PRI_P1+12:
			case EVENTS_STP_PORT_PRI_P1+13:
			case EVENTS_STP_PORT_PRI_P1+14:
			case EVENTS_STP_PORT_PRI_P1+15:
			case EVENTS_STP_PORT_PRI_P1+16:
			case EVENTS_STP_PORT_PRI_P1+17:
			case EVENTS_STP_PORT_PRI_P1+18:
			case EVENTS_STP_PORT_PRI_P1+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"STP settings edit: Port %s, Priority %d",add_port(type-EVENTS_STP_PORT_PRI_P1),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_STP_PORT_COST_P1:
			case EVENTS_STP_PORT_COST_P1+1:
			case EVENTS_STP_PORT_COST_P1+2:
			case EVENTS_STP_PORT_COST_P1+3:
			case EVENTS_STP_PORT_COST_P1+4:
			case EVENTS_STP_PORT_COST_P1+5:
			case EVENTS_STP_PORT_COST_P1+6:
			case EVENTS_STP_PORT_COST_P1+7:
			case EVENTS_STP_PORT_COST_P1+8:
			case EVENTS_STP_PORT_COST_P1+9:
			case EVENTS_STP_PORT_COST_P1+10:
			case EVENTS_STP_PORT_COST_P1+11:
			case EVENTS_STP_PORT_COST_P1+12:
			case EVENTS_STP_PORT_COST_P1+13:
			case EVENTS_STP_PORT_COST_P1+14:
			case EVENTS_STP_PORT_COST_P1+15:
			case EVENTS_STP_PORT_COST_P1+16:
			case EVENTS_STP_PORT_COST_P1+17:
			case EVENTS_STP_PORT_COST_P1+18:
			case EVENTS_STP_PORT_COST_P1+19:
				//temp32 = (*(u32 *)ptr);
				temp32 = get_u32(msg);
				sprintf(tmp,"STP settings edit: Port %s, Cost %lu",add_port(type-EVENTS_STP_PORT_COST_P1),temp32);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_BPDU_FW_ST:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"STP settings edit: Forward BPDU State %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;


			case EVENTS_CALLIBRATE_KOEF1_P1:
			case EVENTS_CALLIBRATE_KOEF1_P1+1:
			case EVENTS_CALLIBRATE_KOEF1_P1+2:
			case EVENTS_CALLIBRATE_KOEF1_P1+3:
			case EVENTS_CALLIBRATE_KOEF1_P1+4:
			case EVENTS_CALLIBRATE_KOEF1_P1+5:
			case EVENTS_CALLIBRATE_KOEF1_P1+6:
			case EVENTS_CALLIBRATE_KOEF1_P1+7:
			case EVENTS_CALLIBRATE_KOEF1_P1+8:
			case EVENTS_CALLIBRATE_KOEF1_P1+9:
			case EVENTS_CALLIBRATE_KOEF1_P1+10:
			case EVENTS_CALLIBRATE_KOEF1_P1+11:
			case EVENTS_CALLIBRATE_KOEF1_P1+12:
			case EVENTS_CALLIBRATE_KOEF1_P1+13:
			case EVENTS_CALLIBRATE_KOEF1_P1+14:
			case EVENTS_CALLIBRATE_KOEF1_P1+15:
			case EVENTS_CALLIBRATE_KOEF1_P1+16:
			case EVENTS_CALLIBRATE_KOEF1_P1+17:
			case EVENTS_CALLIBRATE_KOEF1_P1+18:
			case EVENTS_CALLIBRATE_KOEF1_P1+19:
				sprintf(tmp,"Callibrate RX Line Port %s",add_port(type - EVENTS_CALLIBRATE_KOEF1_P1));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_CALLIBRATE_KOEF2_P1:
			case EVENTS_CALLIBRATE_KOEF2_P1+1:
			case EVENTS_CALLIBRATE_KOEF2_P1+2:
			case EVENTS_CALLIBRATE_KOEF2_P1+3:
			case EVENTS_CALLIBRATE_KOEF2_P1+4:
			case EVENTS_CALLIBRATE_KOEF2_P1+5:
			case EVENTS_CALLIBRATE_KOEF2_P1+6:
			case EVENTS_CALLIBRATE_KOEF2_P1+7:
			case EVENTS_CALLIBRATE_KOEF2_P1+8:
			case EVENTS_CALLIBRATE_KOEF2_P1+9:
			case EVENTS_CALLIBRATE_KOEF2_P1+10:
			case EVENTS_CALLIBRATE_KOEF2_P1+11:
			case EVENTS_CALLIBRATE_KOEF2_P1+12:
			case EVENTS_CALLIBRATE_KOEF2_P1+13:
			case EVENTS_CALLIBRATE_KOEF2_P1+14:
			case EVENTS_CALLIBRATE_KOEF2_P1+15:
			case EVENTS_CALLIBRATE_KOEF2_P1+16:
			case EVENTS_CALLIBRATE_KOEF2_P1+17:
			case EVENTS_CALLIBRATE_KOEF2_P1+18:
			case EVENTS_CALLIBRATE_KOEF2_P1+19:
				sprintf(tmp,"Callibrate TX Line Port %s",add_port(type - EVENTS_CALLIBRATE_KOEF2_P1));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SNMP_STATE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"SNMP settings edit: State %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SNMP_MODE:
				temp8 = get_u8(msg);
				sprintf(tmp,"SNMP settings edit: Mode %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;


			case EVENTS_SNMP_IP:
				//ip[0] = *(u16 *)(ptr);
				//ip[1] = *(u16 *)(ptr+2);
				get_ip(msg,ip);
				sprintf(tmp,"SNMP settings edit: Server IP address %d.%d.%d.%d",
					uip_ipaddr1(&ip),uip_ipaddr2(&ip),uip_ipaddr3(&ip),uip_ipaddr4(&ip));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SNMP_VERS:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"SNMP settings edit: Version %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SNMP3_USER1_LEVEL:
			case EVENTS_SNMP3_USER2_LEVEL:
			case EVENTS_SNMP3_USER3_LEVEL:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"SNMP settings edit: User %d,Level %d",(u8)(type-EVENTS_SNMP3_USER1_LEVEL),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SNMP3_USER1_USERNAME:
			case EVENTS_SNMP3_USER2_USERNAME:
			case EVENTS_SNMP3_USER3_USERNAME:
				get_snmp3_user_name((u8)(type-EVENTS_SNMP3_USER1_USERNAME),temp);
				sprintf(tmp,"SNMP settings edit: User %d, Name: %s",(u8)(type-EVENTS_SNMP3_USER1_USERNAME),temp);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SNMP3_USER1_AUTH_PASS:
			case EVENTS_SNMP3_USER2_AUTH_PASS:
			case EVENTS_SNMP3_USER3_AUTH_PASS:
				get_snmp3_auth_pass((u8)(type-EVENTS_SNMP3_USER1_AUTH_PASS),temp);
				sprintf(tmp,"SNMP settings edit: User %d, AuthPass: %s",(u8)(type-EVENTS_SNMP3_USER1_AUTH_PASS),temp);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SNMP3_USER1_PRIV_PASS:
			case EVENTS_SNMP3_USER2_PRIV_PASS:
			case EVENTS_SNMP3_USER3_PRIV_PASS:
				get_snmp3_priv_pass((u8)(type-EVENTS_SNMP3_USER1_PRIV_PASS),temp);
				sprintf(tmp,"SNMP settings edit: User %d, PrivPass: %s",(u8)(type-EVENTS_SNMP3_USER1_PRIV_PASS),temp);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_SS_TIME:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Soft Start settings edit: Time %dh.",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_DEFAULT:
				sprintf(tmp,"Default settings");
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_TELNET_STATE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Telnet settings edit: state %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_TELNET_ECHO:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Telnet settings edit: option echo %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;



			case EVENTS_IGMP_STATE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"IGMP settings edit: state %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_IGMP_QUERY_MODE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"IGMP settings edit: Query mode %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;


			case EVENTS_IGMP_ST_P1:
			case EVENTS_IGMP_ST_P1+1:
			case EVENTS_IGMP_ST_P1+2:
			case EVENTS_IGMP_ST_P1+3:
			case EVENTS_IGMP_ST_P1+4:
			case EVENTS_IGMP_ST_P1+5:
			case EVENTS_IGMP_ST_P1+6:
			case EVENTS_IGMP_ST_P1+7:
			case EVENTS_IGMP_ST_P1+8:
			case EVENTS_IGMP_ST_P1+9:
			case EVENTS_IGMP_ST_P1+10:
			case EVENTS_IGMP_ST_P1+11:
			case EVENTS_IGMP_ST_P1+12:
			case EVENTS_IGMP_ST_P1+13:
			case EVENTS_IGMP_ST_P1+14:
			case EVENTS_IGMP_ST_P1+15:
			case EVENTS_IGMP_ST_P1+16:
			case EVENTS_IGMP_ST_P1+17:
			case EVENTS_IGMP_ST_P1+18:
			case EVENTS_IGMP_ST_P1+19:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"IGMP settings edit: Port %s, State %d",add_port(type-EVENTS_IGMP_ST_P1),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;


			case EVENTS_IGMP_QUERY_INT:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"IGMP settings edit: Query Interval %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_IGMP_QUERY_RESP_INT:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"IGMP settings edit: Query Response Interval %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_IGMP_GR_MEMB_TIME:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"IGMP settings edit: Group Membership Time %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_IGMP_QUERIER_INT:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"IGMP settings edit: Other Querier Present Interval %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_DOWNSHIFT_STATE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"Downshifting: State %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_TFTP_STATE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"TFTP settings edit: state %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_TFTP_MODE:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"TFTP settings edit: mode %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_TFTP_PORT:
				//temp32 = (*(u8 *)ptr);
				temp32 = get_u32(msg);
				sprintf(tmp,"TFTP settings edit: port %lu",temp32);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_PLC_OUT_1:
			case EVENTS_PLC_OUT_2:
			case EVENTS_PLC_OUT_3:
			case EVENTS_PLC_OUT_4:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"PLC settings edit: out %d, state %s",(u8)(type-EVENTS_PLC_OUT_1+1),add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_PLC_ACTION_OUT1:
			case EVENTS_PLC_ACTION_OUT2:
			case EVENTS_PLC_ACTION_OUT3:
			case EVENTS_PLC_ACTION_OUT4:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"PLC settings edit: out %d, action %d",(u8)(type-EVENTS_PLC_OUT_1+1),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_PLC_STATE_IN1:
			case EVENTS_PLC_STATE_IN2:
			case EVENTS_PLC_STATE_IN3:
			case EVENTS_PLC_STATE_IN4:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"PLC settings edit: in %d, state %d",(u8)(type-EVENTS_PLC_ALSTATE_IN1+1),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_PLC_ALSTATE_IN1:
			case EVENTS_PLC_ALSTATE_IN2:
			case EVENTS_PLC_ALSTATE_IN3:
			case EVENTS_PLC_ALSTATE_IN4:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"PLC settings edit: in %d, Alarm state %d",(u8)(type-EVENTS_PLC_ALSTATE_IN1+1),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_PLC_EM_MODEL:
				//temp32 = (*(u8 *)ptr);
				temp32 = get_u32(msg);
				sprintf(tmp,"PLC settings edit: Energy Meter model %d",(u16)temp32);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_PLC_EM_RATE:
				//temp32 = (*(u8 *)ptr);
				temp32 = get_u32(msg);
				sprintf(tmp,"PLC settings edit: Energy Meter baudrate %lu",temp32*temp32*300);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_PLC_EM_PARITY:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"PLC settings edit: Energy Meter parity %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_PLC_EM_DATABITS:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"PLC settings edit: Energy Meter Data bits %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_PLC_EM_STOPBITS:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"PLC settings edit: Energy Meter Stop bits %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_PLC_EM_ID:
				get_plc_em_id(temp);
				sprintf(tmp,"PLC settings edit: Energy Meter Identification %s",temp);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_PLC_EM_PASS:
				get_plc_em_pass(temp);
				sprintf(tmp,"PLC settings edit: Energy Meter Password %s",temp);
				addtext(tmp);
				EMPTY_TRAP
				break;


			case EVENTS_UPS_DELAYED_ST:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"UPS settings edit: delayed start %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_MACFILT_ST_PORT1:
			case EVENTS_MACFILT_ST_PORT2:
			case EVENTS_MACFILT_ST_PORT3:
			case EVENTS_MACFILT_ST_PORT4:
			case EVENTS_MACFILT_ST_PORT5:
			case EVENTS_MACFILT_ST_PORT6:
			case EVENTS_MACFILT_ST_PORT7:
			case EVENTS_MACFILT_ST_PORT8:
			case EVENTS_MACFILT_ST_PORT9:
			case EVENTS_MACFILT_ST_PORT10:
			case EVENTS_MACFILT_ST_PORT11:
			case EVENTS_MACFILT_ST_PORT12:
			case EVENTS_MACFILT_ST_PORT13:
			case EVENTS_MACFILT_ST_PORT14:
			case EVENTS_MACFILT_ST_PORT15:
			case EVENTS_MACFILT_ST_PORT16:
			case EVENTS_MACFILT_ST_PORT17:
			case EVENTS_MACFILT_ST_PORT18:
			case EVENTS_MACFILT_ST_PORT19:
			case EVENTS_MACFILT_ST_PORT20:
				//temp8 = (*(u8 *)ptr);
				temp8 = get_u8(msg);
				sprintf(tmp,"MAC filtering settings edit: port %s state %d",add_port(type-EVENTS_MACFILT_ST_PORT1),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_ADD_MACFILT_PORT1:
			case EVENTS_ADD_MACFILT_PORT2:
			case EVENTS_ADD_MACFILT_PORT3:
			case EVENTS_ADD_MACFILT_PORT4:
			case EVENTS_ADD_MACFILT_PORT5:
			case EVENTS_ADD_MACFILT_PORT6:
			case EVENTS_ADD_MACFILT_PORT7:
			case EVENTS_ADD_MACFILT_PORT8:
			case EVENTS_ADD_MACFILT_PORT9:
			case EVENTS_ADD_MACFILT_PORT10:
			case EVENTS_ADD_MACFILT_PORT11:
			case EVENTS_ADD_MACFILT_PORT12:
			case EVENTS_ADD_MACFILT_PORT13:
			case EVENTS_ADD_MACFILT_PORT14:
			case EVENTS_ADD_MACFILT_PORT15:
			case EVENTS_ADD_MACFILT_PORT16:
			case EVENTS_ADD_MACFILT_PORT17:
			case EVENTS_ADD_MACFILT_PORT18:
			case EVENTS_ADD_MACFILT_PORT19:
			case EVENTS_ADD_MACFILT_PORT20:
				get_mac(msg,mac);
				sprintf(tmp,"MAC filtering settings edit: add port %s %02X:%02X:%02X:%02X:%02X:%02X",
						add_port(type-EVENTS_ADD_MACFILT_PORT1),mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_DEL_MACFILT_PORT1:
			case EVENTS_DEL_MACFILT_PORT2:
			case EVENTS_DEL_MACFILT_PORT3:
			case EVENTS_DEL_MACFILT_PORT4:
			case EVENTS_DEL_MACFILT_PORT5:
			case EVENTS_DEL_MACFILT_PORT6:
			case EVENTS_DEL_MACFILT_PORT7:
			case EVENTS_DEL_MACFILT_PORT8:
			case EVENTS_DEL_MACFILT_PORT9:
			case EVENTS_DEL_MACFILT_PORT10:
			case EVENTS_DEL_MACFILT_PORT11:
			case EVENTS_DEL_MACFILT_PORT12:
			case EVENTS_DEL_MACFILT_PORT13:
			case EVENTS_DEL_MACFILT_PORT14:
			case EVENTS_DEL_MACFILT_PORT15:
			case EVENTS_DEL_MACFILT_PORT16:
			case EVENTS_DEL_MACFILT_PORT17:
			case EVENTS_DEL_MACFILT_PORT18:
			case EVENTS_DEL_MACFILT_PORT19:
			case EVENTS_DEL_MACFILT_PORT20:
				get_mac(msg,mac);
				sprintf(tmp,"MAC filtering settings edit: delete port %s %02X:%02X:%02X:%02X:%02X:%02X",
						add_port(type-EVENTS_DEL_MACFILT_PORT1),mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
				addtext(tmp);
				EMPTY_TRAP
				break;



			case EVENTS_RATELIM_UC:
				temp8 = get_u8(msg);
				sprintf(tmp,"QoS settings edit: UC rate limit state %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_RATELIM_MC:
				temp8 = get_u8(msg);
				sprintf(tmp,"QoS settings edit: MC rate limit state %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_RATELIM_BC:
				temp8 = get_u8(msg);
				sprintf(tmp,"QoS settings edit: BC rate limit state %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_RATELIM_LIM:
				temp8 = get_u8(msg);
				sprintf(tmp,"QoS settings edit: rate limit %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_TLP1_CONN:
			case EVENTS_TLP2_CONN:
			case EVENTS_TLP3_CONN:
			case EVENTS_TLP4_CONN:
				temp8 = get_u8(msg);
				sprintf(tmp,"TLP %d connection status: %s",(u8)(type - EVENTS_TLP1_CONN),add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;


			case EVENTS_STATE_IN1:
			case EVENTS_STATE_IN2:
			case EVENTS_STATE_IN3:
				temp8 = get_u8(msg);
				sprintf(tmp,"TLP input %d state: %d",(u8)(type - EVENTS_STATE_IN1),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_REMDEV_IN1:
			case EVENTS_REMDEV_IN2:
			case EVENTS_REMDEV_IN3:
				temp8 = get_u8(msg);
				sprintf(tmp,"TLP input %d remdev: %d",(u8)(type - EVENTS_REMDEV_IN1),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_REMPORT_IN1:
			case EVENTS_REMPORT_IN2:
			case EVENTS_REMPORT_IN3:
				temp8 = get_u8(msg);
				sprintf(tmp,"TLP input %d remport: %d",(u8)(type - EVENTS_REMPORT_IN1),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_INPUT_REVERSE1:
			case EVENTS_INPUT_REVERSE2:
			case EVENTS_INPUT_REVERSE3:
				temp8 = get_u8(msg);
				sprintf(tmp,"TLP input %d inverse: %d",(u8)(type - EVENTS_INPUT_REVERSE1),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_STATE_E1:
			case EVENTS_STATE_E2:
				temp8 = get_u8(msg);
				sprintf(tmp,"TLP events %d state: %d",(u8)(type - EVENTS_STATE_E1),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_REMDEV_E1:
			case EVENTS_REMDEV_E2:
				temp8 = get_u8(msg);
				sprintf(tmp,"TLP events %d remdev: %d",(u8)(type - EVENTS_REMDEV_E1),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_REMPORT_E1:
			case EVENTS_REMPORT_E2:
				temp8 = get_u8(msg);
				sprintf(tmp,"TLP events %d remport: %d",(u8)(type - EVENTS_REMPORT_E1),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_TLP_REVERSE1:
			case EVENTS_TLP_REVERSE2:
				temp8 = get_u8(msg);
				sprintf(tmp,"TLP events %d inverse: %d",(u8)(type - EVENTS_TLP_REVERSE1),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_TLP_REMDEV1:
			case EVENTS_TLP_REMDEV2:
			case EVENTS_TLP_REMDEV3:
			case EVENTS_TLP_REMDEV4:
			case EVENTS_TLP_REMDEV5:
				temp8 = get_u8(msg);
				sprintf(tmp,"TLP dev%d valid: %d",(u8)(type - EVENTS_TLP_REMDEV1),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;


			//применение настроек из программы поиска
			case EVENT_SET_SEARCH:
				sprintf(tmp,"Settings edit from Software");
				addtext(tmp);
				EMPTY_TRAP
				break;


			case EVENT_MARVELL_FREEZE:
				sprintf(tmp,"Marvell freeze, reboot...");
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENT_BB_TEST:
				temp32 = get_u32(msg);
				sprintf(tmp,"BB test %lu",temp32);
				addtext(tmp);
				EMPTY_TRAP
				break;


			case EVENTS_LAG1_STATE:
			case EVENTS_LAG2_STATE:
			case EVENTS_LAG3_STATE:
			case EVENTS_LAG4_STATE:
			case EVENTS_LAG5_STATE:
				temp8 = get_u8(msg);
				sprintf(tmp,"LAG%d state: %s",(u8)(type - EVENTS_LAG1_STATE),add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;


			case EVENTS_LAG1_MASTER:
			case EVENTS_LAG2_MASTER:
			case EVENTS_LAG3_MASTER:
			case EVENTS_LAG4_MASTER:
			case EVENTS_LAG5_MASTER:
				temp8 = get_u8(msg);
				sprintf(tmp,"LAG%d master port: %s",(u8)(type - EVENTS_LAG1_MASTER),add_port(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_MIRROR_STATE:
				temp8 = get_u8(msg);
				sprintf(tmp,"Mirroring: state %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_MIRROR_TARGET:
				temp8 = get_u8(msg);
				sprintf(tmp,"Mirroring: Target port %s",add_port(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_MIRROR_PORT1:
			case EVENTS_MIRROR_PORT2:
			case EVENTS_MIRROR_PORT3:
			case EVENTS_MIRROR_PORT4:
			case EVENTS_MIRROR_PORT5:
			case EVENTS_MIRROR_PORT6:
			case EVENTS_MIRROR_PORT7:
			case EVENTS_MIRROR_PORT8:
			case EVENTS_MIRROR_PORT9:
			case EVENTS_MIRROR_PORT10:
			case EVENTS_MIRROR_PORT11:
			case EVENTS_MIRROR_PORT12:
			case EVENTS_MIRROR_PORT13:
			case EVENTS_MIRROR_PORT14:
			case EVENTS_MIRROR_PORT15:
			case EVENTS_MIRROR_PORT16:
				temp8 = get_u8(msg);
				sprintf(tmp,"Mirroring port %d state %d",(u8)(type - EVENTS_MIRROR_PORT1),temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_LLDP_STATE:
				temp8 = get_u8(msg);
				sprintf(tmp,"LLDP state %s",add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_LLDP_TI:
				temp8 = get_u8(msg);
				sprintf(tmp,"LLDP Transmit Interval %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_LLDP_HM:
				temp8 = get_u8(msg);
				sprintf(tmp,"LLDP Holding Multiplier %d",temp8);
				addtext(tmp);
				EMPTY_TRAP
				break;

			case EVENTS_LLDP_PORT1:
			case EVENTS_LLDP_PORT2:
			case EVENTS_LLDP_PORT3:
			case EVENTS_LLDP_PORT4:
			case EVENTS_LLDP_PORT5:
			case EVENTS_LLDP_PORT6:
			case EVENTS_LLDP_PORT7:
			case EVENTS_LLDP_PORT8:
			case EVENTS_LLDP_PORT9:
			case EVENTS_LLDP_PORT10:
			case EVENTS_LLDP_PORT11:
			case EVENTS_LLDP_PORT12:
			case EVENTS_LLDP_PORT13:
			case EVENTS_LLDP_PORT14:
			case EVENTS_LLDP_PORT15:
			case EVENTS_LLDP_PORT16:
				temp8 = get_u8(msg);
				sprintf(tmp,"LLDP port %d state %s",(u8)(type - EVENTS_LLDP_PORT1),add_state(temp8));
				addtext(tmp);
				EMPTY_TRAP
				break;





			default:
				sprintf(tmp,"unknown message: code %lu",type);
				addtext(tmp);
				break;
		}

		//смотрим разрешение работ и получаем level
		level = if_need_send(type,NULL);
		DEBUG_MSG(EVENT_CREATE_DBG,"level %d\r\n",level);

		if((level>=0)&&(level<8)){
			//send syslog
			if(uip_hostaddr[0] || uip_hostaddr[1]){
				if(get_syslog_state() == 1){
					DEBUG_MSG(EVENT_CREATE_DBG,"send syslog\r\n");
					vTaskDelay(10*MSEC);
					memset(syslog_text.text,0,sizeof(syslog_text.text));
					syslog_text.level = level;
					addsysloghead(syslog_text.level,syslog_text.text);
					strcat(syslog_text.text,message_text);
					//добавляем в очередь сообщений
					xQueueSend(SyslogQueue,&syslog_text,0);
				}
			}

			//send e-mail
			if(get_smtp_state()==ENABLE){
				DEBUG_MSG(EVENT_CREATE_DBG,"add to e-mail text\r\n");
				vTaskDelay(10*MSEC);
				//добавляем в очередь сообщений
				if(SmtpQueue)
					xQueueSend(SmtpQueue,message_text,0);
			}

			//send snmp traps
			if(get_snmp_state()==1){
				//if no var bind, no send

				if(snmp_msg_tmp.varbind[0].flag){
					if(SnmpTrapQueue){
						if(xQueueSend(SnmpTrapQueue,&snmp_msg_tmp,0)!= pdPASS){
							DEBUG_MSG(EVENT_CREATE_DBG,"error add to snmp trap\r\n");
						}
						else{
							DEBUG_MSG(EVENT_CREATE_DBG,"add to snmp trap\r\n");
						}
						vTaskDelay(10*MSEC);
					}
				}

			}
		}

		DEBUG_MSG(EVENT_CREATE_DBG,"%s\r\n",message_text);


		//пишем в лог
		#if BB_LOG
			if(write_log_bb(message_text,type)){
				ADD_ALARM(ERROR_WRITE_BB);
			}
			vTaskDelay(1*MSEC);
		#endif
	}
}

//изменение линка на порту
void if_link_changed(void){
	for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++)
		if(dev.port_stat[i].last_link != dev.port_stat[i].link){
			send_events_u32(EVENT_LINK_P1+i,(u32)dev.port_stat[i].link);
			bstp_link_change();
		}

	for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++)
		dev.port_stat[i].last_link = dev.port_stat[i].link;
}


//изменение линка на порту
void if_poe_changed(void){
	//poe a
	for(u8 i=0;i<POE_PORT_NUM;i++)
		if(dev.port_stat[i].poe_a_last != dev.port_stat[i].poe_a)
			send_events_u32(EVENT_POE_A_P1+i,(u32)dev.port_stat[i].poe_a);

	for(u8 i=0;i<POE_PORT_NUM;i++)
		dev.port_stat[i].poe_a_last = dev.port_stat[i].poe_a;

	//poe b
	for(u8 i=0;i<POE_PORT_NUM;i++)
		if(dev.port_stat[i].poe_b_last != dev.port_stat[i].poe_b)
			send_events_u32(EVENT_POE_B_P1+i,(u32)dev.port_stat[i].poe_b);

	for(u8 i=0;i<POE_PORT_NUM;i++)
		dev.port_stat[i].poe_b_last = dev.port_stat[i].poe_b;

}


void set_dry_contact_state(u8 channel,u8 state){
	if(channel > NUM_ALARMS)
		return;
	dry_contact_state[channel] = state;
}
u8 get_dry_contact_state(u8 channel){
	return dry_contact_state[channel];
}

void sensor_line_events(void){
static u8 last_state[3]={0,0,0};
static u8 init = 0;
u8 state;

if(init == 0){
	last_state[0] = get_sensor_state(0);
	last_state[1] = get_sensor_state(1);
	last_state[2] = get_sensor_state(2);
	init++;
}


	if(get_alarm_state(0)==1){
		if(get_sensor_state(0) == 0){
			vTaskDelay(50*MSEC);
			if((get_sensor_state(0) == 0)&&(get_sensor_state(0)!=last_state[0])){
				state = get_sensor_state(0);
				send_events_u32(EVENTS_SENSOR0,(u32)state);
				dev.alarm.dry_cont[0] = 1;
			}
		}
		else
			dev.alarm.dry_cont[0] = 0;
		last_state[0] = get_sensor_state(0);
	}

	if(get_alarm_state(1)==1){
		if(((get_sensor_state(1) == 0) &&(get_alarm_front(1)==1))||
				((get_sensor_state(1) == 1) &&(get_alarm_front(1)==2))
				||(get_alarm_front(1)==3)){
			vTaskDelay(50*MSEC);
			if(((get_sensor_state(1) == 0) &&(get_alarm_front(1)==1))||
					((get_sensor_state(1) == 1) &&(get_alarm_front(1)==2))
					||(get_alarm_front(1)==3)){
				if(get_sensor_state(1)!=last_state[1]){
					state = get_sensor_state(1);
					send_events_u32(EVENTS_SENSOR1,(u32)state);
					dev.alarm.dry_cont[1] = 1;
				}
			}
		}
		else
			dev.alarm.dry_cont[1] = 0;
		last_state[1] = get_sensor_state(1);
	}

	if(get_alarm_state(2)==1){
		if(((get_sensor_state(2) == 0) &&(get_alarm_front(2)==1))||
				((get_sensor_state(2) == 1) &&(get_alarm_front(2)==2))
				||(get_alarm_front(2)==3)){
			vTaskDelay(50*MSEC);
			if(((get_sensor_state(2) == 0) &&(get_alarm_front(2)==1))||
					((get_sensor_state(2) == 1) &&(get_alarm_front(2)==2))
					||(get_alarm_front(2)==3)){
				if(get_sensor_state(2)!=last_state[2]){
					state = get_sensor_state(2);
					send_events_u32(EVENTS_SENSOR2,(u32)state);
					dev.alarm.dry_cont[2] = 0;
				}
			}
		}
		else
			dev.alarm.dry_cont[2] = 0;
		last_state[2] = get_sensor_state(2);
	}
}

u8 get_event_list_num(void){
	return 10;
}

u8 get_event_list(char *text_en,char *text_ru, u8 position){
	return 0;
}


