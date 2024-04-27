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

extern struct status_t status;

char temp[64];

s8t getComfortStartTime(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getComfortStartTime\r\n");
    object->varbind.value.i_value = get_softstart_time();
    return 0;
}


s8t setComfortStartTime(mib_object_t* object, u8t* oid, u8t len, varbind_value_t value)
{
	DEBUG_MSG(SNMP_DEBUG,"setComfortStartTime: %d\r\n",(int)value.i_value);

    if(set_softstart_time(value.i_value)==0){
    	settings_save();
    	return 0;
    }
    else
        return ERROR_STATUS_BAD_VALUE;
}



/*
 */
#define comfortStartIndex           1
#define comfortStartState           2

#define comfortStartTableSize 		2

u8t comfortStartTableColumns[] = {comfortStartIndex, comfortStartState};


s8t getComfortStartEntry(mib_object_t* object, u8t* oid, u8t len)
{
    u32t oid_el1, oid_el2;
    u8t i;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    if (len != 2) {
        return -1;
    }

    DEBUG_MSG(SNMP_DEBUG,"getComfortStartEntry, el1:%lu,el2:%lu\r\n",oid_el1,oid_el2);

    switch (oid_el1) {

    	case comfortStartIndex:
			object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=COOPER_PORT_NUM){
				object->varbind.value.i_value = oid_el2;
			}else
				object->varbind.value.i_value = 0;
			break;

    	case comfortStartState:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=COOPER_PORT_NUM){
				if(get_port_sett_soft_start(oid_el2-1))
					object->varbind.value.i_value = SNMP_UP;
				else
					object->varbind.value.i_value = SNMP_DOWN;
			}else
				return -1;
			break;

        default:
            return -1;
    }
    return 0;
}


s8t setComfortStartEntry(mib_object_t* object, u8t* oid, u8t len, varbind_value_t value)
{
    u32t oid_el1, oid_el2;
    u8t i;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    DEBUG_MSG(SNMP_DEBUG,"setComfortStartEntry, el1:%lu,el2:%lu\r\n",oid_el1,oid_el2);
    DEBUG_MSG(SNMP_DEBUG,"value %lu %ld\r\n",value.u_value,value.i_value);

    if (len != 2) {
        return -1;
    }

    switch (oid_el1) {

    	case comfortStartIndex:

    		break;

		case comfortStartState:

			if(oid_el2<POE_PORT_NUM){
				if(object->varbind.value.i_value == 1)
					set_port_soft_start(oid_el2-1,ENABLE);
				else
					set_port_soft_start(oid_el2-1,DISABLE);
				settings_add2queue(SQ_SS);
				settings_save();
			}else
				return -1;
			break;

        default:
            return -1;
    }
    return 0;
}



ptr_t* getNextComfortStartEntry(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getNextComfortStartEntry\r\n");
	return handleTableNextOid2(oid, len, comfortStartTableColumns, 2, POE_PORT_NUM);
}


/*autorestart*/

#define autoReStartIndex           	1
#define autoReStartMode           	2
#define autoReStartDstIP			3
#define autoReStartSpeedDown		4
#define autoReStartSpeedUp			5

#define autoRestartTableSize 		5

u8t autoRestartTableColumns[] = {autoReStartIndex, autoReStartMode,autoReStartDstIP,autoReStartSpeedDown,autoReStartSpeedUp};

s8t getAutoReStartEntry(mib_object_t* object, u8t* oid, u8t len)
{
    u32t oid_el1, oid_el2;
    u8t i;
    uip_ipaddr_t ip;
    u8 ip_addr[5];
    u8 add_oid[16];
    u8 add_len=0;

    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    if (len != 2) {
        return -1;
    }

    DEBUG_MSG(SNMP_DEBUG,"getAutoReStartEntry, el1:%lu,el2:%lu\r\n",oid_el1,oid_el2);

    switch (oid_el1) {

    	case autoReStartIndex:
			object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=COOPER_PORT_NUM){
				object->varbind.value.i_value = oid_el2;
			}else
				object->varbind.value.i_value = 0;
			break;

    	case autoReStartMode:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=COOPER_PORT_NUM){
				object->varbind.value.i_value = get_port_sett_wdt(oid_el2-1);
			}else
				return -1;
			break;

        case autoReStartDstIP:
        	 get_port_sett_wdt_ip(oid_el2-1,&ip);

             object->varbind.value_type = BER_TYPE_OCTET_STRING; //BER_TYPE_OCTET_STRING;//BER_TYPE_IPADDRESS;

         	 sprintf(temp,"%d.%d.%d.%d",uip_ipaddr1(ip),uip_ipaddr2(ip),uip_ipaddr3(ip),uip_ipaddr4(ip));
         	 object->varbind.value_type = BER_TYPE_OCTET_STRING;
             object->varbind.value.p_value.ptr = (u8t*)temp;
             object->varbind.value.p_value.len = strlen(temp);

             /*
             if(oid_el2==1){
 				ip_addr[0] = uip_ipaddr1(ip);
 				ip_addr[1] = uip_ipaddr2(ip);
 				ip_addr[2] = uip_ipaddr3(ip);
 				ip_addr[3] = uip_ipaddr4(ip);

 				add_oid[0] = 0x01;
 				add_len = 1;
 				for(u8 l=0;l<4;l++){
 					if(ip_addr[l]>128){
 						add_oid[add_len] = 0x81;
 						add_oid[add_len+1] = ip_addr[l]-128;
 						add_len+=2;
 					}
 					else{
 						add_oid[add_len] = ip_addr[l];
 						add_len++;
 					}
 				}

 				for(u8 i=0;i<add_len;i++)
 					object->varbind.oid_ptr->ptr[object->varbind.oid_ptr->len + i] = add_oid[i];
 				object->varbind.oid_ptr->len += (add_len);

 				print_oid(object->varbind.oid_ptr);

 				object->varbind.value.p_value.ptr = (u8 *)ip_addr;
 				object->varbind.value.p_value.len = 4;

 				DEBUG_MSG(SNMP_DEBUG,"ipAdEntAddr len:%d %d.%d.%d.%d.%d.%d.%d.%d\r\n",add_len,add_oid[0],add_oid[1],add_oid[2],
 						add_oid[3],add_oid[4],add_oid[5],add_oid[6],add_oid[7]);
             }
             else
             	return -1;
             	*/
             break;



    	case autoReStartSpeedDown:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=COOPER_PORT_NUM){
				object->varbind.value.i_value = get_port_sett_wdt_speed_down(oid_el2-1);
			}else
				return -1;
			break;

    	case autoReStartSpeedUp:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=COOPER_PORT_NUM){
				object->varbind.value.i_value = get_port_sett_wdt_speed_up(oid_el2-1);
			}else
				return -1;
			break;


        default:
            return -1;
    }
    return 0;
}


ptr_t* getNextAutoReStartEntry(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getNextAutoReStartEntry\r\n");
	return handleTableNextOid2(oid, len, autoRestartTableColumns, autoRestartTableSize, POE_PORT_NUM);
}


//port PoE Settings

#define portPoeIndex           1
#define portPoeState           2

#define portPoeTableSize 		2

u8t portPoeTableColumns[] = {portPoeIndex, portPoeState};



s8t getPortPoeEntry(mib_object_t* object, u8t* oid, u8t len)
{
    u32t oid_el1, oid_el2;
    u8t i;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    if (len != 2) {
        return -1;
    }

    DEBUG_MSG(SNMP_DEBUG,"getPortPoeEntry, el1:%lu,el2:%lu\r\n",oid_el1,oid_el2);

    switch (oid_el1) {

    	case portPoeIndex:
			object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=COOPER_PORT_NUM){
				object->varbind.value.i_value = oid_el2;
			}else
				object->varbind.value.i_value = 0;
			break;

    	case portPoeState:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=COOPER_PORT_NUM){
				if(get_port_sett_poe(oid_el2-1)||get_port_sett_poe_b(oid_el2-1))
					object->varbind.value.i_value = SNMP_UP;
				else
					object->varbind.value.i_value = SNMP_DOWN;
			}else
				return -1;
			break;

        default:
            return -1;
    }
    return 0;
}



s8t setPortPoeEntry(mib_object_t* object, u8t* oid, u8t len, varbind_value_t value)
{
    u32t oid_el1, oid_el2;
    u8t i;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    DEBUG_MSG(SNMP_DEBUG,"setPortPoeEntry, el1:%lu,el2:%lu\r\n",oid_el1,oid_el2);
    DEBUG_MSG(SNMP_DEBUG,"value %lu %ld\r\n",value.u_value,value.i_value);

    if (len != 2) {
        return -1;
    }

    switch (oid_el1) {

		case portPoeState:

			if(oid_el2<=POE_PORT_NUM){
				if(value.i_value == 1){
					set_port_poe(oid_el2-1,POE_AUTO);
				}
				else{
					set_port_poe(oid_el2-1,POE_DISABLE);
				}
				set_poe_init(0);
				settings_save();
			}else
				return -1;
			break;

        default:
            return -1;
    }
    return 0;
}


/*
ptr_t* getNextPortPoeEntry(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getNextPortPoeEntry\r\n");
	return handleTableNextOid2(oid, len, portPoeTableColumns, portPoeTableSize, POE_PORT_NUM);
}*/

ptr_t* getNextPortPoeEntry(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getNextPortPoeEntry\r\n");
	return handleTableNextOid2(oid, len, portPoeTableColumns, 2, POE_PORT_NUM);
}






///Status Group /////////////////////////////////////////////////////////////////////////////
//ups
s8t getUpsModeAvalible(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getUpsModeAvalible\r\n");
	if(is_ups_mode())
		object->varbind.value.i_value = 1;//true
	else
		object->varbind.value.i_value = 2;//false
    return 0;
}

s8t getUpsPwrSource(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getUpsPwrSource\r\n");
	if(is_ups_rezerv() && is_ups_mode())
		object->varbind.value.i_value = 1;//battery
	else
		object->varbind.value.i_value = 2;//ac
    return 0;
}

s8t getUpsBatteryVoltage(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getUpsBatteryVoltage\r\n");
	if(is_ups_mode()){
		object->varbind.value.i_value = get_akb_voltage()/10;
	}
	else
		object->varbind.value.i_value = 0;
    return 0;
}

//inputs
#define inputIndex     	1
#define inputType      	2
#define inputState     	3
#define inputAlarm     	4

#define inputStatusTableSize 	4

u8t inputStatusTableColumns[] = {inputIndex, inputType, inputState,inputAlarm};


s8t getInputStatusEntry(mib_object_t* object, u8t* oid, u8t len)
{
    u32t oid_el1, oid_el2;
    u8t i;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    if (len != 2) {
        return -1;
    }

    DEBUG_MSG(SNMP_DEBUG,"getInputStatusEntry, el1:%lu,el2:%lu\r\n",oid_el1,oid_el2);

    switch (oid_el1) {

    	case inputIndex:
			object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=(NUM_ALARMS+get_plc_input_num())){
				object->varbind.value.i_value = oid_el2;
			}else
				object->varbind.value.i_value = 0;
			break;

    	case inputType:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=(NUM_ALARMS+get_plc_input_num())){
				if(oid_el2<=(NUM_ALARMS)){
					object->varbind.value.i_value = 1;//build in
				}else{
					object->varbind.value.i_value = 2;//option
				}
			}else

				return -1;
			break;

    	case inputState:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=(NUM_ALARMS+get_plc_input_num())){
				if(oid_el2<=(NUM_ALARMS)){
					if(get_sensor_state(oid_el2-1))
						object->varbind.value.i_value = 1;//open
					else
						object->varbind.value.i_value = 2;//short
				}else{
					if(dev.plc_status.in_state[oid_el2-NUM_ALARMS-1])
						object->varbind.value.i_value = 1;//open
					else
						object->varbind.value.i_value = 2;//short
				}
			}else
				return -1;
			break;

    	case inputAlarm:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=(NUM_ALARMS+get_plc_input_num())){
				if(oid_el2<=(NUM_ALARMS)){
					if(dev.alarm.dry_cont[oid_el2-1])
						object->varbind.value.i_value = 1;//true
					else
						object->varbind.value.i_value = 2;//false
				}else{
					if(dev.plc_status.in_state[oid_el2-NUM_ALARMS-1] == get_plc_in_alarm_state(oid_el2-NUM_ALARMS-1))
						object->varbind.value.i_value = 1;//true
					else
						object->varbind.value.i_value = 2;//false
				}
			}else
				return -1;
			break;


        default:
            return -1;
    }
    return 0;
}

ptr_t* getNextInputStatusEntry(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getNextInputStatusEntry\r\n");
	return handleTableNextOid2(oid, len, inputStatusTableColumns, inputStatusTableSize, NUM_ALARMS+get_plc_input_num());
}

//fw version
s8t getFwVersion(mib_object_t* object, u8t* oid, u8t len)
{
	extern const uint32_t image_version[1];

	DEBUG_MSG(SNMP_DEBUG,"getFwVersion\r\n");
	sprintf(temp,"%02X.%02X.%02X",
			 (int)(image_version[0]>>16)&0xff,
			 (int)(image_version[0]>>8)&0xff,  (int)(image_version[0])&0xff);
	object->varbind.value_type = BER_TYPE_OCTET_STRING;
    object->varbind.value.p_value.ptr = (u8t*)temp;
    object->varbind.value.p_value.len = strlen(temp);
    return 0;
}

//energy meter
s8t getEmConnectionStatus(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getEmConnectionStatus\r\n");
	if(is_plc_connected())
		object->varbind.value.i_value = 1;//true
	else
		object->varbind.value.i_value = 2;//false
    return 0;
}

s8t getEmResultTotal(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getEmResultTotal\r\n");
	if(is_plc_connected()){
		get_em_total(temp);
		object->varbind.value_type = BER_TYPE_OCTET_STRING;
		object->varbind.value.p_value.ptr = (u8t*)temp;
	    object->varbind.value.p_value.len = strlen(temp);
	}
	else
		return -1;
    return 0;
}

s8t getEmResultT1(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getEmResultT1\r\n");
	if(is_plc_connected()){
		get_em_t1(temp);
		object->varbind.value_type = BER_TYPE_OCTET_STRING;
		object->varbind.value.p_value.ptr = (u8t*)temp;
		object->varbind.value.p_value.len = strlen(temp);
	}
	else
		return -1;
	return 0;
}

s8t getEmResultT2(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getEmResultT2\r\n");
	if(is_plc_connected()){
		get_em_t2(temp);
		object->varbind.value_type = BER_TYPE_OCTET_STRING;
		object->varbind.value.p_value.ptr = (u8t*)temp;
		object->varbind.value.p_value.len = strlen(temp);
	}
	else
		return -1;
	return 0;
}

s8t getEmResultT3(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getEmResultT3\r\n");
	if(is_plc_connected()){
		get_em_t3(temp);
		object->varbind.value_type = BER_TYPE_OCTET_STRING;
		object->varbind.value.p_value.ptr = (u8t*)temp;
		object->varbind.value.p_value.len = strlen(temp);
	}
	else
		return -1;
	return 0;
}

s8t getEmResultT4(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getEmResultT4\r\n");
	if(is_plc_connected()){
		get_em_t4(temp);
		object->varbind.value_type = BER_TYPE_OCTET_STRING;
		object->varbind.value.p_value.ptr = (u8t*)temp;
		object->varbind.value.p_value.len = strlen(temp);
	}
	else
		return -1;
	return 0;
}

s8t getEmPollingInterval(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getEmPollingInterval\r\n");
	if(is_plc_connected()){
		object->varbind.value.i_value = PLC_POLLING_INTERVAL;
	}
	else
		return -1;
    return 0;
}

//poe status

#define portPoeStatusIndex           1
#define portPoeStatusState           2
#define portPoeStatusPower           3

#define portPoeStatusTableSize 		 3

u8t portPoeStatusTableColumns[] = {portPoeStatusIndex, portPoeStatusState, portPoeStatusPower};


s8t getPortPoeStatusEntry(mib_object_t* object, u8t* oid, u8t len)
{
    u32t oid_el1, oid_el2;
    u8t i;
    uint32_t Temp;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    if (len != 2) {
        return -1;
    }

    DEBUG_MSG(SNMP_DEBUG,"getPortPoeStatusEntry, el1:%lu,el2:%lu\r\n",oid_el1,oid_el2);

    switch (oid_el1) {

    	case portPoeStatusIndex:
			object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=COOPER_PORT_NUM)
				object->varbind.value.i_value = oid_el2;
			else
				object->varbind.value.i_value = 0;
			break;

    	case portPoeStatusState:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=POE_PORT_NUM){
				if((get_port_poe_a(oid_el2-1)||get_port_poe_b(oid_el2-1)))
					object->varbind.value.i_value = SNMP_UP;
				else
					object->varbind.value.i_value = SNMP_DOWN;
			}else
				return -1;
			break;

    	case portPoeStatusPower:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=POE_PORT_NUM){
				if((get_port_poe_a(oid_el2-1))||((get_port_poe_b(oid_el2-1))&&(((oid_el2-1)<POEB_PORT_NUM)||(get_dev_type() == DEV_PSW2GPLUS)))){
					Temp = 0;
					if(get_port_poe_a(oid_el2-1)){
						Temp += get_poe_voltage(POE_A,oid_el2-1)*get_poe_current(POE_A,oid_el2-1)/1000;
					}
					if(get_port_poe_b(oid_el2-1)&&((i<POEB_PORT_NUM)||(get_dev_type() == DEV_PSW2GPLUS))){
						Temp += get_poe_voltage(POE_B,oid_el2-1)*get_poe_current(POE_B,oid_el2-1)/1000;
					}
					object->varbind.value.i_value = Temp;
				}
				else
					object->varbind.value.i_value = 0;
			}else
				return -1;
			break;
        default:
            return -1;
    }
    return 0;
}

ptr_t* getNextPortPoeStatusEntry(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getNextPortPoeStatusEntry\r\n");
	return handleTableNextOid2(oid, len, portPoeStatusTableColumns, 3, POE_PORT_NUM);
}

//autorestart status
#define arPortIndex           		1
#define arPortStatus          		2

#define autoRestartErrorsTableSize 	2

u8t autoRestartErrorsTableColumns[] = {arPortIndex, arPortStatus};


s8t getAutoRestartErrorsEntry(mib_object_t* object, u8t* oid, u8t len)
{
    u32t oid_el1, oid_el2;
    u8t i;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    if (len != 2) {
        return -1;
    }

    DEBUG_MSG(SNMP_DEBUG,"getAutoRestartErrorsEntry, el1:%lu,el2:%lu\r\n",oid_el1,oid_el2);

    switch (oid_el1) {

    	case arPortIndex:
			object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=POE_PORT_NUM)
				object->varbind.value.i_value = oid_el2;
			else
				object->varbind.value.i_value = 0;
			break;

    	case arPortStatus:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=POE_PORT_NUM){
				switch(get_port_error(oid_el2-1)){
					case 0:object->varbind.value.i_value = 1;//normal
						break;
					case 1:object->varbind.value.i_value = 2;//noLink
						break;
					case 2:object->varbind.value.i_value = 3;//no ping
						break;
					case 3:object->varbind.value.i_value = 4;//low speed
						break;
					case 4:object->varbind.value.i_value = 5;//high speed
						break;
					default:object->varbind.value.i_value = 0;
				}
			}else
				return -1;
			break;


        default:
            return -1;
    }
    return 0;
}

ptr_t* getNextAutoRestartErrorsEntry(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getNextAutoRestartErrorsEntry\r\n");
	return handleTableNextOid2(oid, len, autoRestartErrorsTableColumns, autoRestartErrorsTableSize, POE_PORT_NUM);
}


//comfort start status
#define csPortIndex           			1
#define csPortStatus          			2

#define comfortStartStatusTableSize 	3

u8t comfortStartStatusTableColumns[] = {csPortIndex, csPortStatus};


s8t getComfortStartStatusEntry(mib_object_t* object, u8t* oid, u8t len)
{
    u32t oid_el1, oid_el2;
    u8t i;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    if (len != 2) {
        return -1;
    }

    DEBUG_MSG(SNMP_DEBUG,"getComfortStartStatusEntry, el1:%lu,el2:%lu\r\n",oid_el1,oid_el2);

    switch (oid_el1) {

    	case csPortIndex:
			object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=POE_PORT_NUM)
				object->varbind.value.i_value = oid_el2;
			else
				object->varbind.value.i_value = 0;
			break;

    	case csPortStatus:
            object->varbind.value_type = BER_TYPE_INTEGER;
			if(oid_el2<=POE_PORT_NUM){
				if(dev.port_stat[oid_el2-1].ss_process)
					object->varbind.value.i_value = 2;//processing
				else
					object->varbind.value.i_value = 1;//normal
			}else
				return -1;
			break;


        default:
            return -1;
    }
    return 0;
}

ptr_t* getNextComfortStartStatusEntry(mib_object_t* object, u8t* oid, u8t len)
{
	DEBUG_MSG(SNMP_DEBUG,"getNextComfortStartStatusEntry\r\n");
	return handleTableNextOid2(oid, len, comfortStartStatusTableColumns, comfortStartStatusTableSize, POE_PORT_NUM);
}
