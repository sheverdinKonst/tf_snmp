#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "../smtp/smtp.h"
#include "../flash/blbx.h"
#include "syslog.h"
#include "cfg_syslog.h"
#include "msg_build.h"
#include "../inc/VLAN.h"
#include "../snmp/snmp.h"
#include "board.h"
#include "settings.h"
#include "../events/events_handler.h"
#include "selftest.h"
#include "debug.h"

//#include "bb.h"

#define ULONG_MAX 4294967295

//extern uint8_t syslog_start;//задача syslog запущена
//extern  xQueueHandle xSyslogQueue;//очередь для сообщений
extern char mail_text[512+256];
//extern uip_ipaddr_t uip_hostaddr;
char descr[64];


/*расшифровываем код команды и преобразуем ее в строку
 * 1 тип (set get trap)
 * 2 группа команд
 * 3 подгруппа
 * 4 номер порта
 * 5 переменная
 * 6 значение*/

u8 make_syslog_string(u8 type,u8 group,u8 subgroup,u8 port,u8 variable,char *value){
static u16 tmp;
char tmptxt[20];
u8 send_log=0;


return 1;//my


if((uip_hostaddr[0] == 0 ) && (uip_hostaddr[1] == 0))
	return 1;

DEBUG_MSG(SYSLOG_DEBUG,"make_syslog_string\r\n");

memset(syslog_text.text,0,sizeof(syslog_text.text));
//

//смотрим разрешение работ и получаем level
if(get_syslog_param(type,group,subgroup,port,variable,value)==0)
	send_log=1;

addsysloghead(syslog_text.level,syslog_text.text);

/*вставляем код команды*/
sprintf(tmptxt,"[%d.%d.%d.%d.%d] ",type,group,subgroup,port,variable);
strcat(syslog_text.text,tmptxt);

/*расшифровка кода*/
/*type*/
switch(type){
	case 1:strcat(syslog_text.text,"set: ");break;
	case 2:strcat(syslog_text.text,"get: ");break;
	case 3:strcat(syslog_text.text,"trap: ");break;
	default:break;
}

/*command grout*/
if(type==1){
	switch(group){
		case 1:add("base_settings");break;
		case 2:add("port_settings");break;
		case 3:add("vlan_settings");break;
		case 4:add("qos_settings");break;
		case 5:add("stp_settings");break;
		case 6:add("special_function");break;
		case 7:add("diagnostic_tools");break;
		default: return 1;
	}
}

/*subgroup*/
if(type==1)
switch(group){
	case 1: switch(subgroup){
				case 1:add("device_description");
						switch(variable){
							case 1:
							add("name");
							addval(value);
							break;
							case 2:
							add("location");
							addval(value);
							break;
							case 3:
							add("company");
							addval(value);
							break;
						}
						break;
				case 2:
					add("network_settings");
					switch(variable){
						case 1:add("mac");addmac(value);break;
						case 2:add("ip");addip(value);break;
						case 3:add("mask");addip(value);break;
						case 4:add("gate");addip(value);break;
						case 5:add("dhcp");addnum(value);break;
					}
					break;
				case 3:
					add("administration");
					switch(variable){
						case 1:add("login");addval(value);break;
						case 2:add("password");addval(value);break;
					}
					break;
				case 4:
					add("syslog");
					switch(variable){
						case 1:add("state");addnum(value); break;
						case 2:add("server");addip(value); break;
					}
					break;
				case 5:
					add("sntp");
					switch(variable){
						case 1:add("state");addnum(value);break;
						case 2:add("server");addip(value);break;
						case 3:add("time_zone");addnum(value);break;
						case 4:add("period");addnum(value);break;
					}
					break;
				case 6:
					add("smtp");
					switch(variable){
						case 1:add("state");addnum(value);break;
						case 2:add("server");addip(value);break;
						case 3:add("sender");addval(value);break;
						case 4:add("reciever_1");addval(value);break;
						case 5:add("syslog2email");addnum(value);break;
						case 6:add("reciever_2");addval(value);break;
						case 7:add("reciever_3");addval(value);break;
						case 8:add("subject");addval(value);break;
					}
					break;
			}break;
	/*port settings*/
	case 2: addport(port);
			switch(variable){
				case 1:add("state");addnum(value);break;
				case 2:add("speed");addval(value);break;
				case 3:add("flow");addnum(value);break;
				case 4:add("poe");addnum(value);break;
			}break;
	case 3:
	/*vlan settings*/
		switch(subgroup){
			case 1:
				add("portbase");
				switch(variable){
					case 1:add("state"); addnum(value);
				}break;
			case 2:
				add("802_1q");
				switch(variable){
					case 1:add("VLAN_state");addnum(value);break;
					case 2:add("MVID");addnum(value);break;
					case 3:addport(port);
						add("Port_state");
						addnum(value);break;
					case 4:addport(port);
						add("DefVID");
						addnum(value);break;
					case 5:add("add_edit_vlan");
						//Read_Eeprom(Vlan1QTable+(u16)(*value)*24, &VLAN[(u16)(*value)], 24);
						add("vid");
						//tmp=settings.vlan[(u8)(*value)].VID;
						tmp = get_vlan_vid((u8)(*value));
						addnum((char *)&tmp);
						add("name");
						strcpy(tmptxt,/*settings.vlan[(u8)(*value)].VLANNAme*/get_vlan_name((u8)(*value)));
						addval(tmptxt);
						break;
					case 7:
						add("delete_vlan");
						if((u8)(*value)<MAXVlanNum){
							//Read_Eeprom(Vlan1QTable+(u16)(*value)*24, &VLAN[(u16)(*value)], 24);
							//tmp=settings.vlan[(u8)*value].VID;
							tmp = get_vlan_vid((u8)(*value));
							addnum((char *)&tmp);
						}break;
				}break;
		}break;
	case 4:
		/*qos*/
		switch(subgroup){
			case 1:add("general");
				switch(variable){
					case 1:add("state");
							addnum(value);
							break;
					case 2:
							add("shed_mechanism");
							addnum(value);
							break;
					case 3:addport(port);
							add("priority_mode");
							addnum(value);
							break;
					case 4:addport(port);
							add("default_priority");
							addnum(value);
							break;
				}break;
			case 2:add("cos");
				add("priority");
				addnum((char *)&variable);
				add("queue");
				addnum(value);
				break;
			case 3: add("tos");
				add("priority");
				addnum((char *)&variable);
				add("queue");
				addnum((char *)&variable);
				break;
			case 4:add("rate_limit");
				addport(port);
				switch(variable){
					case 1:add("rx_mode");
						addnum(value);
						break;
					case 2:add("rx_limit");
						addnum(value);
						break;
					case 3:add("tx_limit");
						addnum(value);
						break;
				}
				break;
		}
		break;
	case 5:
		/*stp*/
		switch(variable){
			case 1:add("STP_state");addnum(value);break;
			case 2:add("TX_hold_cnt");addnum(value);break;
			case 3:add("bridge_pri");addnum(value);break;
			case 4:add("bridge_max_age");addnum(value);break;
			case 5:add("bridge_hello_time"); addnum(value);break;
			case 6:add("forward_delay_time");addnum(value);break;
			case 7:addport(port);
				   add("port_state");
				   addnum(value);break;
			case 8:addport(port);
				   add("priority");
				   addnum(value);break;
			case 9:addport(port);
				   add("cost");
				   addnum(value);break;
			case 10:addport(port);
				   add("autocost");
				   addnum(value);break;
			case 11:addport(port);
				   add("edge");
				   addnum(value);break;
			case 12:addport(port);
				   add("p2p");
				   addnum(value);break;
		}
		break;
	case 6:
	/*special function*/
	switch(subgroup){
		case 1:add("comfort_start");
				switch(variable){
				case 1:
					addport(port);
					add("state");
					addnum(value);
					break;
				case 2:
					add("time");
					addnum(value);
				}
				break;
		case 2:add("autorestart");
				addport(port);
				switch(variable){
					case 1: add("mode");addnum(value); break;
					case 2: add("ip"); addip(value);break;
				}
	}break;

	case 7:
		/*diagnostic tools*/
		add("vct");
		addport(port);
		switch(variable){
			case 1:add("length");addnum(value);break;
			case 2:add("coeff"); addnum(value);break;
		}
		break;
}

/*trap*/
if(type==3){
	switch(group){
		case 1:
			add("port");
			switch(subgroup){
				case 1:add("link");
					  addport(port);
					  add("state");
					  addval(value);break;
				case 2:add("poe");
					  addport(port);
					  add("state");
					  addval(value);break;
			}break;
		case 2:add("stp");
			add("topology_change");
			addval(value);break;
		case 3:add("vlan");break;
		case 4:add("special_function");
				addport(port);
				switch(variable){
					case 1:add("link");
						addnum(value);break;
					case 2:add("ping");
						addnum(value);break;
				}break;
		case 5:add("system");
				switch(subgroup){
					case 1:add("login");
							addnum(value);break;
					case 2:add("reboot");
							//addnum(value);
							break;
					case 3:add("update");
							switch(variable){
								case 1:add("updating");
								addnum(value);break;
								case 2:add("version");
								addnum(value);break;
							}break;
					case 4:add("default");break;
					case 5:add("backup");
							switch(variable){
								case 1:add("loaded");break;
								case 2:add("saved"); break;
							}break;
					case 6:add("arp_cach");
							switch(variable){
								case 1:add("cleared");break;
							}break;
					case 7:add("start");
						   add("status");
						   //addnum(value);
							switch((u16)(*value)){
								case 5:addval("after_power_down");break;
								default:addval("after_reset");break;
							}break;
				}break;
		case 6: add("ups");
			switch(variable){
				case 1:add("low_voltage");
					addnum(value);break;
				case 2:add("bat_voltage");
					addval(value);break;
				case 3:add("power_source");
					switch((u8)*value){
						case 0:addval("VAC");break;
						case 1:addval("battery");break;
					}break;
			}break;
		case 7:	add("access_control");
			switch(variable){
				case 1:add("alarm");break;
			}
			break;
	}
}

//пишем в лог
#if BB_LOG
	if(write_log_bb(syslog_text.text,type))
		ADD_ALARM(ERROR_WRITE_BB);
#endif

if(send_log == 1){
	/*отправляем syslog*/
	if(get_syslog_state()){
		makesysloglen(syslog_text.text);
		//syslog_send_msg(syslog_text.text);
		syslog_ready();
	}

	/*добавляем к e-mail сообщению*/
	if(/*settings.smtp_sett.state*/get_smtp_state()==ENABLE){
		if((strlen(mail_text)+strlen(syslog_text.text))<(512)){
			DEBUG_MSG(SYSLOG_DEBUG,"add to text\r\n");
			strcat(mail_text,syslog_text.text);
			strcat(mail_text,"\r\n");
		}
	}
	//готовим и отправляем snmp trap
	DEBUG_MSG(SYSLOG_DEBUG,"snmp_cfg.state %d\r\n",get_snmp_state()/*settings.snmp_sett.state*/);
	if(/*settings.snmp_sett.state*/get_snmp_state()==1){
		DEBUG_MSG(SYSLOG_DEBUG,"add to snmp trap\r\n");
		snmp_string[0]=type;
		snmp_string[1]=group;
		snmp_string[2]=subgroup;
		snmp_string[3]=(port);
		snmp_string[4]=variable;
		snmp_var=strtoul(value, NULL, 10);
		if(snmp_var == ULONG_MAX)
			snmp_var=(u32)value;
		snmp_ready();
		//теперь можно отправлять snmp
	}
	return 0;
}
return 1;
}

void add(char *text){
	strcat(syslog_text.text,text);
	adddot();
}
void addval(char *text){
u16 len;
	strcat(syslog_text.text,"=\"");
	len=strlen(syslog_text.text);
	if((strlen(text)+len)>=128-5)
		strncat(syslog_text.text,text,(128-len-5));
	else
		strcat(syslog_text.text,text);
	strcat(syslog_text.text,"\"");
}
void addmac(char *text){
	char tmp[128];
	sprintf(tmp,"=\" 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X \"",text[0],text[1],text[2],text[3],text[4],text[5]);
	add(tmp);
}
void addip(char *text){
	char tmp[64];
	sprintf(tmp,"=\"%d.%d.%d.%d\"",text[0],text[1],text[2],text[3]);
	add(tmp);
}

void addnum(char *text){
char tmp[64];
	memset(tmp,0,sizeof(tmp));

	strcat(syslog_text.text,"=\"");
	/*1b*/
	if(strlen(text)<3)
		sprintf(tmp,"%d",(u8)(text[0]));
	/*2b*/
	//else if(strlen(text)<4)
	//	sprintf(tmp,"%d",((text[1]<<8) | (text[0])));
	/*4b*/
	//else
	sprintf(tmp,"%lu",(u32)((text[3]<<24) | (text[2]<<16) | (text[1]<<8) | (text[0])));

	strcat(syslog_text.text,tmp);
	strcat(syslog_text.text,"\"");

}

void adddot(void){
	strcat(syslog_text.text,".");
}

void addport(u8 port){
	switch(port){
		case 0:add("FE1");break;
		case 1:add("FE2");break;
		case 2:add("FE3");break;
		case 3:add("GE1");break;
		case 4:add("GE2");break;
	}
}

u8 get_syslog_param(u8 type,u8 group,u8 subgroup,u8 port,u8 variable,char *value){



	//get_syslog_cfg();


	/*set*/
	if((type==1)&&(get_event_base_s_st())&&(group==1)){
		syslog_text.level=get_event_base_s_level();
		return 0;
	}
	if((type==1)&&(get_event_port_s_st())&&(group==2)){
		syslog_text.level=get_event_port_s_level();
		return 0;
	}
	if((type==1)&&(get_event_vlan_s_st())&&(group==3)){
		syslog_text.level=get_event_vlan_s_level();
		return 0;
	}
	if((type==1)&&(get_event_qos_s_st())&&(group==4)){
		syslog_text.level=get_event_qos_s_level();
		return 0;
	}
	if((type==1)&&(get_event_stp_s_st())&&(group==5)){
		syslog_text.level=(get_event_stp_s_level());
		return 0;
	}
	if((type==1)&&(get_event_other_s_st())&&((group==6)||(group==7))){
		syslog_text.level=get_event_other_s_level();
		return 0;
	}
	/*trap*/
	if((type==3)&&(get_event_port_link_t_st())&&(group==1)&&(subgroup==1)){
		syslog_text.level=get_event_port_link_t_level();
		return 0;
	}
	if((type==3)&&(get_event_port_poe_t_st())&&(group==1)&&(subgroup==2)){
		syslog_text.level=get_event_port_poe_t_level();
		return 0;
	}
	if((type==3)&&(get_event_port_stp_t_st())&&(group==2)){
		syslog_text.level=get_event_port_stp_t_level();
		return 0;
	}
	if((type==3)&&(get_event_port_spec_link_t_st())&&(group==4)&&(subgroup==1)){
		syslog_text.level=get_event_port_spec_link_t_level();
		return 0;
	}
	if((type==3)&&(get_event_port_spec_ping_t_st())&&(group==4)&&(subgroup==2)){
		syslog_text.level=get_event_port_spec_ping_t_level();
		return 0;
	}

	if((type==3)&&(get_event_port_spec_speed_t_st())&&(group==4)&&(subgroup==3)){
		syslog_text.level=get_event_port_spec_speed_t_level();
		return 0;
	}

	if((type==3)&&(get_event_port_system_t_st())&&(group==5)){
		syslog_text.level=get_event_port_system_t_level();
		return 0;
	}

	//если никуда не попали, то и не посылаем сообщение
	return 1;
}



i8 if_need_send(u32 type,u8 *event_type){

	*event_type = 0;
	switch(type){

		//trap link
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
			*event_type = 1;
			if(get_event_port_link_t_st())
				return get_event_port_link_t_level();
			break;

			//trap poe
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
			*event_type = 2;
			if(get_event_port_poe_t_st())
				return get_event_port_poe_t_level();
			break;


			//trap spec link
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
			*event_type = 4;
			if(get_event_port_spec_link_t_st())
				return get_event_port_spec_link_t_level();
			break;

			//trap spec ping
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
			*event_type = 5;
			if(get_event_port_spec_ping_t_st())
				return get_event_port_spec_ping_t_level();
			break;

		//trap spec speed
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
			*event_type = 6;
			if(get_event_port_spec_speed_t_st())
				return get_event_port_spec_speed_t_level();
			break;

			//trap stp
		case EVENT_STP_REINIT:
			*event_type = 3;
			if(get_event_port_stp_t_st())
				return get_event_port_stp_t_level();
			break;

			//trap system
		case EVENT_START_PWR:
		case EVENT_START_RST:
		case EVENT_WEBINTERFACE_LOGIN:
		case EVENT_UPDATE_FIRMWARE:
		case EVENT_CLEAR_ARP:
		case EVENTS_DEFAULT:
			*event_type = 7;
			if(get_event_port_system_t_st())
				return get_event_port_system_t_level();
			break;

			//trap sensor (dry contact)
		case EVENTS_SENSOR0:
		case EVENTS_SENSOR1:
		case EVENTS_SENSOR2:
		case EVENTS_INPUT1:
		case EVENTS_INPUT2:
		case EVENTS_INPUT3:
			*event_type = 8;
			if(get_event_port_alarm_t_st())
				return get_event_port_alarm_t_level();
			break;

			//trap ups
		case EVENTS_UPS_REZERV:
		case EVENTS_UPS_LOW_VOLTAGE:
			*event_type = 9;
			if(get_event_port_ups_t_st())
				return get_event_port_ups_t_level();

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
			*event_type = 10;
			if(get_event_port_mac_t_st())
				return get_event_port_mac_t_level();
	}

	//если никуда не попали, то и не посылаем сообщение
	return -1;
}


