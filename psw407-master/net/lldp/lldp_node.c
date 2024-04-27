/*
 * Copyright (c) 2011-2016 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * @file
 * @brief LLDP nodes implementation
 */

#include "deffines.h"
#include "lldp_node.h"
#include "board.h"
#include <string.h>
#include "debug.h"

//добавление записей
void lldp_update_port(lldp_port_entry_t entry){
u8 i;

	if(!entry.valid)
		return;

	//проверяем наличие дубликатов
	//если запись уже есть, обновляем TTL
	for(i=0;i<LLDP_ENTRY_NUM;i++){
		if(lldp_port_entry[i].valid){
			if(lldp_entry_compare(&entry,&lldp_port_entry[i])==0){
				lldp_port_entry[i].ttl = entry.ttl;
				DEBUG_MSG(LLDP_DEBUG,"lldp_update_port !entry exist!  port=%d chassis_id_type=%d port_id_type=%d\r\n",
					entry.if_index,entry.chassis_id_subtype,entry.portid_subtype);

				DEBUG_MSG(LLDP_DEBUG,"%x:%x:%x:%x:%x:%x\r\n",entry.chassis_id[0],entry.chassis_id[1],
						entry.chassis_id[2],entry.chassis_id[3],entry.chassis_id[4],entry.chassis_id[5]);

				return;
			}
		}
	}

	//добавляем новую запись
	for(i=0;i<LLDP_ENTRY_NUM;i++){
		if(lldp_port_entry[i].valid!=1){

			DEBUG_MSG(LLDP_DEBUG,"%d port=%d chassis_id_type=%d port_id_type=%d\r\n",
					i,entry.if_index,entry.chassis_id_subtype,entry.portid_subtype);

			DEBUG_MSG(LLDP_DEBUG,"%x:%x:%x:%x:%x:%x\r\n",entry.chassis_id[0],entry.chassis_id[1],
								entry.chassis_id[2],entry.chassis_id[3],entry.chassis_id[4],entry.chassis_id[5]);

			lldp_port_entry[i].valid = 1;
			lldp_port_entry[i].if_index = entry.if_index;
			lldp_port_entry[i].chassis_id_subtype = entry.chassis_id_subtype;
			lldp_port_entry[i].chassis_id_len = entry.chassis_id_len;
			for(u8 j=0;j<entry.chassis_id_len&&j<LLDP_CHIDLEN;j++)
				lldp_port_entry[i].chassis_id[j] = entry.chassis_id[j];
			lldp_port_entry[i].portid_len = entry.portid_len;
			lldp_port_entry[i].portid_subtype = entry.portid_subtype;
			for(u8 j=0;j<entry.portid_len&&j<LLDP_CHIDLEN;j++)
				lldp_port_entry[i].port_id[j] = entry.port_id[j];
			for(u8 j=0;j<entry.port_descr_len&&j<LLDP_DESCR_LEN;j++)
				lldp_port_entry[i].port_descr[j] = entry.port_descr[j];
			lldp_port_entry[i].port_descr_len = entry.port_descr_len;

			for(u8 j=0;j<entry.sys_name_len&&j<LLDP_DESCR_LEN;j++)
				lldp_port_entry[i].sys_name[j] = entry.sys_name[j];
			lldp_port_entry[i].sys_name_len = entry.sys_name_len;

			for(u8 j=0;j<entry.sys_descr_len&&j<LLDP_DESCR_LEN;j++)
				lldp_port_entry[i].sys_descr[j] = entry.sys_descr[j];
			lldp_port_entry[i].sys_descr_len = entry.sys_descr_len;

			lldp_port_entry[i].sys_cap = entry.sys_cap;
			lldp_port_entry[i].sys_cap_en = entry.sys_cap_en;

			lldp_port_entry[i].ttl = entry.ttl;
			break;
		}
	}

}


void lldp_update_timer(void){
	for(u8 i=0;i<LLDP_ENTRY_NUM;i++){
		if(lldp_port_entry[i].valid){
			if(lldp_port_entry[i].ttl)
				lldp_port_entry[i].ttl--;
			else
				lldp_port_entry[i].valid = 0;
		}
	}
}


void convert_lldp_chasis_id_subtype(char *str, u8 subtype){
	switch(subtype){
		case 0: strcpy(str,"Reserved");break;
		case 1: strcpy(str,"Chassis component");break;
		case 2: strcpy(str,"Interface alias");break;
		case 3: strcpy(str,"Port component");break;
		case 4: strcpy(str,"MAC address");break;
		case 5: strcpy(str,"Network address");break;
		case 6: strcpy(str,"Interface name");break;
		case 7: strcpy(str,"Locally assigned");break;
		default:strcpy(str,"Unknown");break;
	}
}

void convert_lldp_chasis_id(char *str, u8 subtype, u8*id){
	if(subtype == LLDP_CHASS_ID_SUBTYPE_NAME (mac_addr)){
		sprintf(str,"%02X:%02X:%02X:%02X:%02X:%02X",id[0],id[1],id[2],id[3],id[4],id[5]);
	}
	else if(subtype == LLDP_CHASS_ID_SUBTYPE_NAME(net_addr)){
		sprintf(str,"%02d.%02d.%02d.%02d",id[0],id[1],id[2],id[3]);
	}
	else if(subtype == LLDP_CHASS_ID_SUBTYPE_NAME(intf_name)){
		strcpy(str,id);
	}
	else{
		strcpy(str,"---");
	}
}

void convert_lldp_port_id_subtype(char *str, u8 subtype){
	switch(subtype){
		case 0: strcpy(str,"Reserved");break;
		case 1: strcpy(str,"Interface alias");break;
  	  	case 2: strcpy(str,"Port component");break;
  	  	case 3: strcpy(str,"MAC address");break;
  	  	case 4: strcpy(str,"Network address");break;
  	    case 5: strcpy(str,"Interface name");break;
  	  	case 6: strcpy(str,"Agent circuit ID");break;
  	  	case 7: strcpy(str,"Locally assigned");break;
  	  	default:strcpy(str,"Reserved");
	}
}

/**
@function convert_lldp_port_id
Функция форматирует вывод
@param str - указатель на возвращаемую строку
@param subtype - подтип port id
@param id - указател на port_id
 */
void convert_lldp_port_id(char *str, u8 subtype, u8*id){
	if(subtype == LLDP_PORT_ID_SUBTYPE_NAME (mac_addr)){
		sprintf(str,"%02X:%02X:%02X:%02X:%02X:%02X",id[0],id[1],id[2],id[3],id[4],id[5]);
	}
	else if(subtype == LLDP_PORT_ID_SUBTYPE_NAME(net_addr)){
		sprintf(str,"%02d.%02d.%02d.%02d",id[0],id[1],id[2],id[3]);
	}
	else if(subtype == LLDP_PORT_ID_SUBTYPE_NAME(intf_name)){
		strcpy(str,id);
	}
	else if(subtype == LLDP_PORT_ID_SUBTYPE_NAME(local)){
		strcpy(str,id);
	}
	else{
		strcpy(str,"---");
	}
}

/**
@function convert_lldp_cap
Функция форматирует вывод system capabilities
@param str - указатель на возвращаемую строку
@param cap - capabilities
@param lang - языкинтерфейса
 */
void convert_lldp_cap(char *str,u8 cap,u8 lang){

	str[0]=0;
	if((1<<LLDP_CAP_OTHER)&cap)
		if(lang==ENG)strcat(str,"Other");else strcat(str,"Другие");
	if((1<<LLDP_CAP_REPEATER)&cap)
		if(lang==ENG)strcat(str,"Repeater ");else strcat(str,"Повторитель ");
	if((1<<LLDP_CAP_BRIDGE)&cap)
		if(lang==ENG)strcat(str,"Bridge ");else strcat(str,"Мост ");
	if((1<<LLDP_CAP_WLAN_AP)&cap)
		if(lang==ENG)strcat(str,"WLAN Access Point ");else strcat(str,"Беспроводная точка доступа ");
	if((1<<LLDP_CAP_ROUTER)&cap)
		if(lang==ENG)strcat(str,"Router ");else strcat(str,"Маршрутизатор ");
	if((1<<LLDP_CAP_TELEPHONE)&cap)
		if(lang==ENG)strcat(str,"Telephone ");else strcat(str,"Телефон ");
	if((1<<LLDP_CAP_DOCSIS)&cap)
		if(lang==ENG)strcat(str,"DOCSIS Cable Device ");else strcat(str,"Устройство DOCSIS ");
	if((1<<LLDP_CAP_STATION)&cap)
		if(lang==ENG)strcat(str,"Station ");else strcat(str,"Станция ");
}

/**
 @function lldp_entry_compare
 Функция сравнивает записи LLDP
 @param entry1 - указатель на первую запись
 @param entry2 - указатель на вторую запись
 @return 1 - записи различны, 0 - записи идентичны
 */
u8 lldp_entry_compare(lldp_port_entry_t *entry1,lldp_port_entry_t *entry2){
	if(entry1->if_index == entry2->if_index &&
			entry1->chassis_id_subtype == entry2->chassis_id_subtype &&
			entry1->chassis_id_len == entry2->chassis_id_len &&
			entry1->portid_subtype == entry2->portid_subtype &&
			entry1->portid_len == entry2->portid_len){
		for(u8 i=0;i<entry1->chassis_id_len;i++){
			if(entry1->chassis_id[i]!=entry2->chassis_id[i])
				return 1;
		}
		for(u8 i=0;i<entry1->portid_len;i++){
			if(entry1->port_id[i]!=entry2->port_id[i])
				return 1;
		}
		return 0;
	}
	else
		return 1;
}

/**
 @function get_lldp_entry_num
 Функция подсчитывает число записей LLDP
 @return число валидных записей
 */
u16 get_lldp_entry_num(void){
	u8 num = 0;
	for(u8 i=0;i<LLDP_ENTRY_NUM;i++){
		if(lldp_port_entry[i].valid){
			num++;
		}
	}
	return num;
}

