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
#include "debug.h"
#include "ctrls_deffines.h"


s8t getDot1qVlanVersionNumber(mib_object_t* object, u8t* oid, u8t len)
{
    object->varbind.value.u_value = 1;
    return 0;
}

s8t getDot1qMaxVlanId(mib_object_t* object, u8t* oid, u8t len)
{
    object->varbind.value.u_value = 4094;
    return 0;
}


s8t getDot1qMaxSupportedVlans(mib_object_t* object, u8t* oid, u8t len)
{
    object->varbind.value.u_value = MAXVlanNum;
    return 0;
}

s8t getDot1qNumVlans(mib_object_t* object, u8t* oid, u8t len)
{
    object->varbind.value.u_value = get_vlan_sett_vlannum();
    return 0;
}


s8t getDot1qGvrpStatus(mib_object_t* object, u8t* oid, u8t len)
{
    object->varbind.value.i_value = 2;//disabled
    return 0;
}

//vlan table edit / info
s8t getDot1qVlanStaticName(mib_object_t* object, u8t* oid, u8t len)
{
    u32t oid_el1, oid_el2;
    u8t i;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    DEBUG_MSG(SNMP_DEBUG,"getDot1qVlanStaticName %lu %lu\r\n",oid_el1,oid_el2);

    object->varbind.value.p_value.ptr = (u8t*)"System Description2";
    object->varbind.value.p_value.len = 19;
    return 0;
}





s8t setDot1qVlanStaticName(mib_object_t* object, u8t* oid, u8t len, varbind_value_t value)
{



    object->varbind.value.p_value.ptr = (u8t*)"System Description2";
    object->varbind.value.p_value.len = 19;
    return 0;
}

#define dot1qVlanStaticName 			1
#define dot1qVlanStaticEgressPorts 		2
#define dot1qVlanForbiddenEgressPorts 	3
#define dot1qVlanStaticUntaggedPorts	4
#define dot1qVlanStaticRowStatus		5


s8t getDot1qVlanStaticTable(mib_object_t* object, u8t* oid, u8t len)
{
    u32t oid_el1, oid_el2;    u8t i;
    static u32t vid = 0;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    //если первый или не найден, берем первый vid
    if((vlan_vid2num(oid_el2) == -1)||((vlan_vid2num(oid_el2) == 0)&&(vid == 0))){
    	vid = get_vlan_vid(0); // default vlan
    }
    else{
    	//get next vid
    	i = vlan_vid2num(oid_el2)+1;
    	vid = get_vlan_vid(i);
    }

    //модифицируем oid
    ber_encode_oid_item(vid,oid+i);

    if(vlan_vid2num(vid) == -1){
    	vid = 0;
    	return -1;
    }

    DEBUG_MSG(SNMP_DEBUG,"%d: getDot1qVlanStaticTable %lu %lu %d vid = %lu\r\n",vlan_vid2num(vid),oid_el1,oid_el2, i,vid);

    if (len != 2) {
        return -1;
    }

    switch (oid_el1) {
        case dot1qVlanStaticName:
            object->varbind.value_type = BER_TYPE_OCTET_STRING;
            //if(oid_el2<=/*get_vlan_sett_vlannum()*/4094){

            //if this is last+1
            if(vlan_vid2num(vid) == -1){
            	vid = 0;
            	return -1;
            }

            	object->varbind.value.p_value.ptr = (u8t*)get_vlan_name(vlan_vid2num(vid));
            	object->varbind.value.p_value.len = strlen(get_vlan_name(vlan_vid2num(vid)));
            //}
            //else
            //	return -1;
            break;

        case dot1qVlanStaticEgressPorts:
            object->varbind.value_type = BER_TYPE_OCTET_STRING;
            if(oid_el2<=get_vlan_sett_vlannum()){
            	object->varbind.value.u_value = getInMulticastPkt(oid_el2-1);
            }else
            	return -1;
            break;

        case dot1qVlanForbiddenEgressPorts:
            object->varbind.value_type = BER_TYPE_OCTET_STRING;
            if(oid_el2<=get_vlan_sett_vlannum()){
            //	object->varbind.value.u_value = getInMulticastPkt(oid_el2-1);
            }else
            	return -1;
            break;

        case dot1qVlanStaticUntaggedPorts:
            object->varbind.value_type = BER_TYPE_OCTET_STRING;
            if(oid_el2<=get_vlan_sett_vlannum()){
            //	object->varbind.value.u_value = getInMulticastPkt(oid_el2-1);
            }else
            	return -1;
            break;

        case dot1qVlanStaticRowStatus:
            object->varbind.value_type = BER_TYPE_INTEGER;
            if(oid_el2<=get_vlan_sett_vlannum()){
            	if(get_vlan_state(oid_el2-1) == 1)
            		object->varbind.value.i_value = 1;//enabled
            	else
            		object->varbind.value.i_value = 2;//disabled
            }else
            	return -1;
            break;

        default:
            return -1;
    }
    return 0;
}


u8t Dot1qVlanStaticTableColumns[] = {dot1qVlanStaticName, dot1qVlanStaticEgressPorts,
		dot1qVlanForbiddenEgressPorts, dot1qVlanStaticUntaggedPorts, dot1qVlanStaticRowStatus};

ptr_t* getNextDot1qVlanStaticTable(mib_object_t* object, u8t* oid, u8t len)
{
	return handleTableNextOid2(oid, len, Dot1qVlanStaticTableColumns, 5, get_vlan_sett_vlannum());
}


s8t setDot1qVlanStaticTable(mib_object_t* object, u8t* oid, u8t len, varbind_value_t value)
{
    u32t oid_el1, oid_el2;
    u8t i;
    i = ber_decode_oid_item(oid, len, &oid_el1);
    i = ber_decode_oid_item(oid + i, len - i, &oid_el2);

    DEBUG_MSG(SNMP_DEBUG,"setDot1qVlanStaticTable\r\n");

    DEBUG_MSG(SNMP_DEBUG,"value %lu %ld\r\n",value.u_value,value.i_value);

    if (len != 2) {
        return -1;
    }

    switch (oid_el1) {
        case dot1qVlanStaticName:

            object->varbind.value_type = BER_TYPE_OCTET_STRING;
            //if(oid_el2<=get_vlan_sett_vlannum()){
				object->varbind.value.p_value.ptr = (u8t*)get_vlan_name(oid_el2-1);
				object->varbind.value.p_value.len = strlen(get_vlan_name(oid_el2-1));
            //}else
            //    return -1;
            break;

        case dot1qVlanStaticEgressPorts:
            object->varbind.value_type = BER_TYPE_OCTET_STRING;
            if(oid_el2<=get_vlan_sett_vlannum()){
            //	object->varbind.value.u_value = getInMulticastPkt(oid_el2-1);
            }else
            	return -1;
            break;

        case dot1qVlanForbiddenEgressPorts:
            object->varbind.value_type = BER_TYPE_OCTET_STRING;
            if(oid_el2<=get_vlan_sett_vlannum()){
            //	object->varbind.value.u_value = getInMulticastPkt(oid_el2-1);
            }else
            	return -1;
            break;

        case dot1qVlanStaticUntaggedPorts:
            object->varbind.value_type = BER_TYPE_OCTET_STRING;
            if(oid_el2<=get_vlan_sett_vlannum()){
            //	object->varbind.value.u_value = getInMulticastPkt(oid_el2-1);
            }else
            	return -1;
            break;

        case dot1qVlanStaticRowStatus:
            object->varbind.value_type = BER_TYPE_INTEGER;
            if(oid_el2<=get_vlan_sett_vlannum()){
            	if(get_vlan_state(oid_el2) == 1)
            		object->varbind.value.i_value = 1;
            	else
            		object->varbind.value.i_value = 2;
            }else
            	return -1;
            break;

        default:
            return -1;
    }
    return 0;
}


