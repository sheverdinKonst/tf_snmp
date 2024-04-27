/* -----------------------------------------------------------------------------
 * SNMP implementation for Contiki
 *
 * Copyright (C) 2010 Siarhei Kuryla <kurilo@gmail.com>
 *
 * This program is part of free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#include "snmpd-conf.h"
#include "keytools.h"
#include <string.h>
#include <stdlib.h>
#include "board.h"
#include "settings.h"
#include "debug.h"
#if ENABLE_SNMPv3

extern u8 dev_addr[6];
u8 defaultEngineID[] = {0x80, 0x00, 0x1f, 0x88, 0x80, 0x77, 0xd5, 0xcb, 0x77, 0x9e, 0xa0, 0xef, 0x4b};

u8t msgAuthoritativeEngineID_array[64];

ptr_t msgAuthoritativeEngineID = {msgAuthoritativeEngineID_array, 0};

u8t usmUserName[64];

u32t privacyLow = 0xA6F89012;
u32t privacyHigh = 0xF9434568;

u32t getMsgAuthoritativeEngineBoots()
{
    return 0;
}

u32t getLPrivacyParameters()
{
    privacyLow++;
    return privacyLow;
}

u32t getHPrivacyParameters()
{
    privacyHigh++;
    return privacyHigh;
}

ptr_t* getEngineID()
{
	if(msgAuthoritativeEngineID.len==0)
		setEngineID();

    return &msgAuthoritativeEngineID;
}


void convert_ptr_to_eid(engine_id_t *eid,ptr_t *eid_ptr){
	eid->len = eid_ptr->len;
	for(u8 i=0;i<eid->len;i++){
		eid->ptr[i] = eid_ptr->ptr[i];
	}
}

u8t* getUserName()
{
    return usmUserName;
}



void setUserName(void)
{
    get_snmp3_user_name(0,(char *)usmUserName);
}

void setEngineID(void)
{
	engine_id_t eid;
	get_snmp3_engine_id(&eid);

	if(eid.len == 0){
		//make eid
		eid.len = 11;

		eid.ptr[0] = 0x80;
		eid.ptr[1] = 0x00;//OID == 42019 - fort-telecom
		eid.ptr[2] = 0xa4;//
		eid.ptr[3] = 0x23;//

		eid.ptr[4] = 0x03;//mac address
		for(u8 i=0;i<6;i++){
			eid.ptr[5+i] = dev_addr[i];
		}
	}

	msgAuthoritativeEngineID.len = eid.len;
	for(u8 i=0;i<eid.len;i++){
		msgAuthoritativeEngineID.ptr[i] = eid.ptr[i];
	}
}

void setAuthPrivKul(void){
char pwd[64];
u8t key[16];
	if(get_snmp3_level(0) == AUTH_NO_PRIV || get_snmp3_level(0) == AUTH_PRIV){
		get_snmp3_auth_pass(0,pwd);
		password_to_key_md5((u8t *)pwd,strlen(pwd),getEngineID()->ptr, getEngineID()->len,key);
		setAuthKul(key);
	}

	if(get_snmp3_level(0) == AUTH_PRIV){
		get_snmp3_priv_pass(0,pwd);
		password_to_key_md5((u8t *)pwd,strlen(pwd),getEngineID()->ptr, getEngineID()->len,key);
		setPrivKul(key);
	}
}


void engineid_to_str(engine_id_t *eid,char *str){
char temp[4];
str[0] = 0;

	DEBUG_MSG(SNMP_DEBUG,"engineid_to_str:len %d\r\n", eid->len);

	if(eid->len>64)
		return;

	for(u8 i=0;i<eid->len;i++){
		sprintf(temp,"%02X",eid->ptr[i]);
		strcat(str, temp);
	}
	DEBUG_MSG(SNMP_DEBUG,"engineid_to_str: %s\r\n", str);
}

int str_to_engineid(char *str,engine_id_t *eid){
char temp[4];
eid->len = 0;
u32 tmp;
DEBUG_MSG(SNMP_DEBUG,"str_to_engineid:");
	for(u8 i=0;i<strlen(str)/2; i++){
		strncpy(temp,&str[i*2],2);
		tmp = strtoul(temp, NULL, 16);
		if(tmp>255)
			return -1;
		eid->ptr[i] = (uint8_t)tmp;
		DEBUG_MSG(SNMP_DEBUG,"%X|",eid->ptr[i]);
		eid->len++;
	}
	DEBUG_MSG(SNMP_DEBUG,"\r\n");
	return 0;
}

#endif
