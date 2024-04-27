/*
 * lldp_mib.c
 *
 *  Created on: 26 окт. 2018 г.
 *      Author: BelyaevAlex
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "board.h"
#include "settings.h"
#include "../deffines.h"
#include "../snmpd/mib-init.h"
#include "../snmpd/ber.h"
#include "../snmpd/utils.h"
#include "../snmpd/logging.h"
#include "../snmpd/dispatcher.h"
#include "../snmpd/snmpd.h"
#include "../snmp.h"
#include "../poe/poe_ltc.h"
#include "ctrls_deffines.h"
#include "UPS_command.h"
#include "../plc/em.h"
#include "../plc/plc.h"
#include "debug.h"

#include "../lldp/lldp_node.h"

char temp[128];

//#define lldpRemTimeMark           1
//#define lldpRemLocalPortNum       2
//#define lldpRemIndex              3
#define lldpRemChassisIdSubtype   4
#define lldpRemChassisId          5
#define lldpRemPortIdSubtype      6
#define lldpRemPortId		      7
#define lldpRemPortDesc        	  8
#define lldpRemSysName        	  9
#define lldpRemSysDesc            10
#define lldpRemSysCapSupported    11
#define lldpRemSysCapEnabled      12

s8t getLldpRemEntry(mib_object_t* object, u8t* oid, u8t len)
{
	u32t oid_el1, oid_el2;
	u8t i;
	i = ber_decode_oid_item(oid, len, &oid_el1);
	i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

	DEBUG_MSG(SNMP_DEBUG,"getLldpRemEntry, el1:%lu,el2:%lu\r\n",oid_el1,oid_el2);

	if (len != 2) {
		return -1;
	}

	switch (oid_el1) {
		case lldpRemChassisIdSubtype:
			object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2 <= get_lldp_entry_num()){
				object->varbind.value.i_value = lldp_port_entry[oid_el2-1].chassis_id_subtype;
			}
			else
				return -1;
			break;

		case lldpRemChassisId:
			object->varbind.value_type = BER_TYPE_OCTET_STRING;
			if(oid_el2 <= get_lldp_entry_num()){
				object->varbind.value.p_value.ptr = (u8t*)lldp_port_entry[oid_el2-1].chassis_id;
				object->varbind.value.p_value.len = lldp_port_entry[oid_el2-1].chassis_id_len;
			}
			else
				return -1;
			break;


		case lldpRemPortIdSubtype:
			object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2 <= get_lldp_entry_num()){
				object->varbind.value.i_value = lldp_port_entry[oid_el2-1].portid_subtype;
			}
			else
				return -1;
			break;


		case lldpRemPortId:
			object->varbind.value_type = BER_TYPE_OCTET_STRING;
			if(oid_el2 <= get_lldp_entry_num()){
				object->varbind.value.p_value.ptr = (u8t*)lldp_port_entry[oid_el2-1].port_id;
				object->varbind.value.p_value.len = lldp_port_entry[oid_el2-1].portid_len;;
			}
			else
				return -1;
			break;

		case lldpRemPortDesc:
			object->varbind.value_type = BER_TYPE_OCTET_STRING;
			if(oid_el2 <= get_lldp_entry_num()){
				object->varbind.value.p_value.ptr = (u8t*)lldp_port_entry[oid_el2-1].port_descr;
				object->varbind.value.p_value.len = lldp_port_entry[oid_el2-1].port_descr_len;
			}
			else
				return -1;
			break;

		case lldpRemSysName:
			object->varbind.value_type = BER_TYPE_OCTET_STRING;
			if(oid_el2 <= get_lldp_entry_num()){
				object->varbind.value.p_value.ptr = (u8t*)lldp_port_entry[oid_el2-1].sys_name;;
				object->varbind.value.p_value.len = lldp_port_entry[oid_el2-1].sys_name_len;
			}
			else
				return -1;
			break;

		case lldpRemSysDesc:
			object->varbind.value_type = BER_TYPE_OCTET_STRING;
			if(oid_el2 <= get_lldp_entry_num()){
				object->varbind.value.p_value.ptr = (u8t*)lldp_port_entry[oid_el2-1].sys_descr;
				object->varbind.value.p_value.len = lldp_port_entry[oid_el2-1].sys_descr_len;
			}
			else
				return -1;
			break;

		case lldpRemSysCapSupported:
			object->varbind.value_type = BER_TYPE_OCTET_STRING;
			if(oid_el2 <=  get_lldp_entry_num()){
				temp[0] = (lldp_port_entry[oid_el2-1].sys_cap)<<4;
				object->varbind.value.p_value.ptr = (u8t*)temp;
				object->varbind.value.p_value.len = 1;
			}
			else
				return -1;
			break;


		case lldpRemSysCapEnabled:
			object->varbind.value_type = BER_TYPE_OCTET_STRING;
			if(oid_el2 <= get_lldp_entry_num()){
				temp[0] = (lldp_port_entry[oid_el2-1].sys_cap_en<<4);
				object->varbind.value.p_value.ptr = (u8t*)temp;
				object->varbind.value.p_value.len = 1;
			}
			else
				return -1;
			break;


		default:
			return -1;
	}
	return 0;
}


u8t lldpTableColumns[] = {/*lldpRemTimeMark, lldpRemLocalPortNum, lldpRemIndex, */lldpRemChassisIdSubtype,
		lldpRemChassisId, lldpRemPortIdSubtype, lldpRemPortId, lldpRemPortDesc,lldpRemSysName, lldpRemSysDesc,
		lldpRemSysCapSupported, lldpRemSysCapEnabled};
u8 lldpTableSize;


ptr_t* getNextLldpRemEntry(mib_object_t* object, u8t* oid, u8t len)
{
	lldpTableSize = get_lldp_entry_num();
	DEBUG_MSG(SNMP_DEBUG,"getNextLldpRemEntry \r\n");
    return handleTableNextOid2(oid, len, lldpTableColumns, 9, lldpTableSize);
}


s8t getLldpMessageTxInterval(mib_object_t* object, u8t* oid, u8t len){
	DEBUG_MSG(SNMP_DEBUG,"getLldpMessageTxInterval\r\n");
	object->varbind.value.i_value = get_lldp_transmit_interval();
	return 0;
}

s8t getLldpTxHoldMultiplier(mib_object_t* object, u8t* oid, u8t len){
	DEBUG_MSG(SNMP_DEBUG,"getLldpTxHoldMultiplier\r\n");
	object->varbind.value.i_value = get_lldp_hold_multiplier();
	return 0;
}

s8t getLldLocChassisIdSubtype(mib_object_t* object, u8t* oid, u8t len){
	DEBUG_MSG(SNMP_DEBUG,"getLldLocChassisIdSubtype\r\n");
	object->varbind.value.i_value = LLDP_CHASS_ID_SUBTYPE_NAME (mac_addr);
	return 0;
}

s8t getLldLocChassisId(mib_object_t* object, u8t* oid, u8t len){
	extern u8 dev_addr[6];
	DEBUG_MSG(SNMP_DEBUG,"getLldLocChassisId\r\n");
	object->varbind.value_type = BER_TYPE_OCTET_STRING;
    object->varbind.value.p_value.ptr = (u8t*)dev_addr;
    object->varbind.value.p_value.len = 6;
    return 0;
}

s8t getLldLocSysName(mib_object_t* object, u8t* oid, u8t len){
	DEBUG_MSG(SNMP_DEBUG,"getLldLocSysName\r\n");
	u8 l;
	get_interface_name(temp);
	l = strlen(temp);
	object->varbind.value_type = BER_TYPE_OCTET_STRING;
    object->varbind.value.p_value.ptr = (u8t*)temp;
    object->varbind.value.p_value.len = l;
    return 0;
}

s8t getLldLocSysDesc(mib_object_t* object, u8t* oid, u8t len){
u8 l;
	DEBUG_MSG(SNMP_DEBUG,"getLldLocSysDesc\r\n");
	get_dev_name(temp);
	l = strlen(temp);
	object->varbind.value_type = BER_TYPE_OCTET_STRING;
    object->varbind.value.p_value.ptr = (u8t*)temp;
    object->varbind.value.p_value.len = l;
    return 0;
}

s8t getLldLocSysCapSupported(mib_object_t* object, u8t* oid, u8t len){

	DEBUG_MSG(SNMP_DEBUG,"getLldLocSysCapSupported\r\n");
	object->varbind.value_type = BER_TYPE_OCTET_STRING;
	temp[0] = 0x60;
	object->varbind.value.p_value.ptr = (u8t*)temp;
	object->varbind.value.p_value.len = 1;
	return 0;
}

s8t getLldLocSysCapEnabled(mib_object_t* object, u8t* oid, u8t len){

	DEBUG_MSG(SNMP_DEBUG,"getLldLocSysCapEnabled\r\n");
	temp[0] = 0x60;
	object->varbind.value.p_value.ptr = (u8t*)temp;
	object->varbind.value.p_value.len = 1;
	return 0;
}

#define lldpPortConfigPortNum				1
#define lldpPortConfigAdminStatus 			2
#define lldpPortConfigNotificationEnable	3
#define lldpPortConfigTLVsTxEnable			4
u8t lldpPortConfigTableColumns[] ={lldpPortConfigPortNum,lldpPortConfigAdminStatus,
							  lldpPortConfigNotificationEnable,lldpPortConfigTLVsTxEnable};
u8t lldpPortConfigTableSize;
s8t getLldpPortConfigEntry(mib_object_t* object, u8t* oid, u8t len){
	u32t oid_el1, oid_el2;
	u8t i;
	static char name[1];
	i = ber_decode_oid_item(oid, len, &oid_el1);
	i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

	DEBUG_MSG(SNMP_DEBUG,"getLldpPortConfigEntry, el1:%lu,el2:%lu\r\n",oid_el1,oid_el2);

	if (len != 2) {
		return -1;
	}

	switch (oid_el1) {
		case lldpPortConfigPortNum:
			object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=ALL_PORT_NUM){
				object->varbind.value.i_value = oid_el2;
			}else
				return -1;
			break;
		case lldpPortConfigAdminStatus:
			object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=ALL_PORT_NUM){
				if(get_lldp_port_state(oid_el2-1))
					object->varbind.value.i_value = 3;
				else
					object->varbind.value.i_value = SNMP_DOWN;
			}else
				return -1;
			break;
		case lldpPortConfigNotificationEnable:
			object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=ALL_PORT_NUM){
				//todo пока уведомления рассылаются, если LLDP включен на порту
				if(get_lldp_port_state(oid_el2-1))
					object->varbind.value.i_value = SNMP_UP;
				else
					object->varbind.value.i_value = SNMP_DOWN;
			}else
				return -1;
			break;
		case lldpPortConfigTLVsTxEnable:
			temp[0] = 0xF0;
			object->varbind.value_type = BER_TYPE_OCTET_STRING;
			if(oid_el2<=ALL_PORT_NUM){
				object->varbind.value.p_value.ptr = (u8t*)temp;
				object->varbind.value.p_value.len = 1;
			}else
				return -1;
			break;
        default:
            return -1;


	}
	return 0;

}

ptr_t* getNextLldpPortConfigEntry(mib_object_t* object, u8t* oid, u8t len){
	lldpPortConfigTableSize = ALL_PORT_NUM;
	DEBUG_MSG(SNMP_DEBUG,"getNextLldpPortConfigEntry \r\n");
	return handleTableNextOid2(oid, len, lldpPortConfigTableColumns, 4, lldpPortConfigTableSize);
}

#define lldpLocPortNum 			1
#define lldpLocPortIdSubtype	2
#define lldpLocPortId			3
#define lldpLocPortDesc			4
u8t lldpLocPortTableColumns[] ={lldpLocPortNum,lldpLocPortIdSubtype,lldpLocPortId,lldpLocPortDesc};
u8t lldpLocPortTableSize;
s8t getLldpLocPortEntry(mib_object_t* object, u8t* oid, u8t len){

	u32t oid_el1, oid_el2;
	u8t i,l;
	char temp1[128];

	i = ber_decode_oid_item(oid, len, &oid_el1);
	i = ber_decode_oid_item(oid + i, len - i, &oid_el2);


	DEBUG_MSG(SNMP_DEBUG,"getLldpLocPortEntry, el1:%lu,el2:%lu\r\n",oid_el1,oid_el2);

	if (len != 2) {
		return -1;
	}

	switch (oid_el1) {
		case lldpLocPortNum:
			object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=ALL_PORT_NUM){
				object->varbind.value.i_value = oid_el2;
			}else
				return -1;
			break;
		case lldpLocPortIdSubtype:
			object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=ALL_PORT_NUM){
				object->varbind.value.i_value = LLDP_PORT_ID_SUBTYPE_NAME (local);
			}else
				return -1;
			break;

		case lldpLocPortId:
			object->varbind.value_type = BER_TYPE_OCTET_STRING;
			if(oid_el2<=ALL_PORT_NUM){
				sprintf(temp,"Port %d",oid_el2);
				l = strlen(temp);
				object->varbind.value.p_value.ptr = (u8t*)temp;
				object->varbind.value.p_value.len = l;
			}else
				return -1;
			break;

		case lldpLocPortDesc:
			object->varbind.value_type = BER_TYPE_OCTET_STRING;
			if(oid_el2<=ALL_PORT_NUM){
				get_dev_name(temp);
				sprintf(temp1," Port %d",oid_el2);
				strcat(temp,temp1);
				l = strlen(temp);
				object->varbind.value.p_value.ptr = (u8t*)temp;
				object->varbind.value.p_value.len = l;
			}else
				return -1;
			break;
        default:
            return -1;
	}
	return 0;
}
ptr_t* getNextLldpLocPortEntry(mib_object_t* object, u8t* oid, u8t len){
	lldpLocPortTableSize = ALL_PORT_NUM;
	DEBUG_MSG(SNMP_DEBUG,"getNextLldpLocPortEntry\r\n");
	return handleTableNextOid2(oid, len, lldpLocPortTableColumns, 4, lldpLocPortTableSize);
}

