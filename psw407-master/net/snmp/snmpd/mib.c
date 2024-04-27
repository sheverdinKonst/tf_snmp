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
#include <string.h>
#include <stdlib.h>

#include "../deffines.h"
#include "debug.h"
#include "mib.h"
#include "ber.h"
#include "utils.h"
#include "logging.h"

#include "FreeRTOS.h"
#include "snmpd-types.h"



#if MIB_SIZE > 0
static mib_object_t mib[MIB_SIZE] __attribute__ ((section (".ccmram")));
static u16t mib_length;

#else
static mib_object_t *mib_head = 0, *mib_tail = 0;
#endif





/*-----------------------------------------------------------------------------------*/
/*
 * Adds an object to the MIB.
 * TODO: sort while adding an object
 */
void mib_add(mib_object_t* object) {
    #if !MIB_SIZE
    if (!mib_head) {
        mib_head = object;
        mib_tail = object;
        object->next_ptr = 0;
    } else {
        mib_tail->next_ptr = object;
        object->next_ptr = 0;
        mib_tail = object;
    }
    #else
    mib_length++;
    #endif
}

/*-----------------------------------------------------------------------------------*/
/*
 * Adds a scalar to the MIB.
 */
s8t add_scalar(ptr_t* oid, u8t flags, u8t value_type, const void* const value, get_value_t gfp, set_value_t svfp)
{
    #if !MIB_SIZE
    mib_object_t* object = mib_object_create();
    memset(object, 0, sizeof(mib_object_t));
    CHECK_PTR(object);
    #else
    mib_object_t* object = &mib[mib_length];
    #endif
    /* set oid functions */
    object->get_fnc_ptr = gfp;
    object->set_fnc_ptr = svfp;
    #if ENABLE_MIB_TABLE
    object->get_next_oid_fnc_ptr = 0;
    #endif

    object->attrs = flags;
    
    /* set initial value if it's not NULL */
    if (value) {
        switch (value_type) {
            case BER_TYPE_IPADDRESS:
            case BER_TYPE_OCTET_STRING:
                // we allocate memory for a string, because it's released afterwards,
                // when we set a new value
                object->varbind.value.p_value.len = strlen((char*)value);
                object->varbind.value.p_value.ptr = (u8t*)value;
                break;

            case BER_TYPE_INTEGER:
                object->varbind.value.i_value = *((s32t*)value);
                break;

            case BER_TYPE_COUNTER:
            case BER_TYPE_TIME_TICKS:
            case BER_TYPE_GAUGE:
                object->varbind.value.u_value = *((u32t*)value);
                break;

            case BER_TYPE_OPAQUE:
            case BER_TYPE_OID:
                // we allocate memory for a string, because it's released afterwards,
                // when a new value is set
                object->varbind.value.p_value.len = ((ptr_t*)value)->len;
                object->varbind.value.p_value.ptr = ((ptr_t*)value)->ptr;
                break;

            default:
                break;
        }
    }
    
    object->varbind.oid_ptr = oid;
    object->varbind.value_type = value_type;


    //for debug
    print_oid(oid);

    mib_add(object);

    return ERR_NO_ERROR;
}

#if ENABLE_MIB_TABLE
/*-----------------------------------------------------------------------------------*/
/*
 * Adds a table to the MIB.
 */
s8t add_table(ptr_t* oid_prefix, get_value_t  gfp, get_next_oid_t gnofp, set_value_t svfp)
{
    if (!gfp || !gnofp) {
        return -1;
    }
    #if !MIB_SIZE
    mib_object_t* object = mib_object_create();
    CHECK_PTR(object);
    #else
    mib_object_t* object = &mib[mib_length];
    #endif

    object->varbind.oid_ptr = oid_prefix;

    //for debug
    print_oid(oid_prefix);

    /* set getter functions */
    object->get_fnc_ptr = gfp;
    /* set next oid function */
    object->get_next_oid_fnc_ptr = gnofp;
    if (svfp) {
        /* set set value function */
        object->set_fnc_ptr = svfp;
        object->attrs &= ~FLAG_ACCESS_READONLY;
        DEBUG_MSG(SNMP_DEBUG,"read-write\r\n");
    } else {
        object->attrs |= FLAG_ACCESS_READONLY;
        DEBUG_MSG(SNMP_DEBUG,"read-only\r\n");
    }

    /* mark the entry in the MIB as a table */
    object->varbind.value_type = BER_TYPE_NULL;

    mib_add(object);



    return ERR_NO_ERROR;
}

#define GET_NEXT_OID_PTR ptr->get_next_oid_fnc_ptr
#else
#define GET_NEXT_OID_PTR 0
#endif

mib_object_t* ptr;

#if MIB_SIZE
static u16t mib_cur_index;
#endif

void mib_iterator_init()
{
    #if !MIB_SIZE
    ptr = mib_head;
    #else
    mib_cur_index = 0;
    ptr = &mib[0];
    #endif    
}

u8t mib_iterator_is_end()
{
    #if !MIB_SIZE
    return ptr == 0;
    #else
    return mib_cur_index >= mib_length;
    #endif
}

void mib_iterator_next() {
    #if !MIB_SIZE
    ptr = ptr->next_ptr;
    #else
    mib_cur_index++;
    ptr = &mib[mib_cur_index];
    #endif
}


#if CONTIKI_TARGET_AVR_RAVEN
#include <avr/pgmspace.h>
#else
#endif


/*-----------------------------------------------------------------------------------*/
/*
 * Find an object in the MIB corresponding to the oid in the snmp-get request.
 */
mib_object_t* mib_get(varbind_t* req)
{
    u16t oid_len=0;
    
    DEBUG_MSG(SNMP_DEBUG,"mib_get\r\n");
    print_oid(req->oid_ptr);

    mib_iterator_init();
    while (!mib_iterator_is_end()) {

    	//print_oid(ptr->varbind.oid_ptr);
        oid_len = oid_length(ptr->varbind.oid_ptr);
        if (!GET_NEXT_OID_PTR) {
            // scalar
            if (oid_len == req->oid_ptr->len && !oid_cmp(req->oid_ptr, ptr->varbind.oid_ptr)) {
                break;
            }
            /* check noSuchInstance */
            if (oid_len - 1 <= req->oid_ptr->len + 1 && !oid_cmpn(req->oid_ptr, ptr->varbind.oid_ptr, oid_len - 1)) {
                req->value_type = BER_TYPE_NO_SUCH_INSTANCE;
                DEBUG_MSG(SNMP_DEBUG,"noSuchInstance\r\n");
                return 0;
            }
        }
        if (GET_NEXT_OID_PTR && oid_len < req->oid_ptr->len &&
                !oid_cmp(req->oid_ptr, ptr->varbind.oid_ptr)) {
            // tabular
            break;
        }
        mib_iterator_next();
    }

    if (mib_iterator_is_end()) {
        req->value_type = BER_TYPE_NO_SUCH_OBJECT;
        DEBUG_MSG(SNMP_DEBUG,"noSuchObject\r\n");
        return 0;
    }

    if (ptr->get_fnc_ptr) {
        if ((ptr->get_fnc_ptr)(ptr, &req->oid_ptr->ptr[oid_len], req->oid_ptr->len - oid_len) == -1) {
            req->value_type = BER_TYPE_NO_SUCH_INSTANCE;
            DEBUG_MSG(SNMP_DEBUG,"noSuchInstance - get value function\r\n");
            return 0;
        }
    }

    /* copy the value */
    memcpy(&req->value, &ptr->varbind.value, sizeof(varbind_value_t));
    req->value_type = ptr->varbind.value_type;

    DEBUG_MSG(SNMP_DEBUG,"mib_get_end\r\n");
    return ptr;
}

/*-----------------------------------------------------------------------------------*/
/*
 * Find an object in the MIB that is the lexicographical successor of the given one.
 */
mib_object_t* mib_get_next(varbind_t* req)
{
    int cmp;
    u16t oid_len=0;
    
    DEBUG_MSG(SNMP_DEBUG,"mib_get_next\r\n");
    print_oid(req->oid_ptr);


    mib_iterator_init();
    while (!mib_iterator_is_end()) {
        // find the object
    	DEBUG_MSG(SNMP_DEBUG,"#");
    	print_oid(ptr->varbind.oid_ptr);


        cmp = oid_cmp(req->oid_ptr, ptr->varbind.oid_ptr);
        oid_len = oid_length(ptr->varbind.oid_ptr);

        DEBUG_MSG(SNMP_DEBUG,"cmp %d,oid_len1 %d,oid_len2 %d\r\n",cmp,oid_len,req->oid_ptr->len);


        if (!GET_NEXT_OID_PTR) {
            // handle a scalar object
            if (cmp < 0 || (cmp == 0 && req->oid_ptr->len < oid_len)) {
                #if ENABLE_MIB_TABLE || ENABLE_PROGMEM
                    oid_copy(req->oid_ptr, ptr->varbind.oid_ptr, 0);
                    CHECK_PTR_U(req->oid_ptr->ptr);
                #else
                    //req->oid_ptr.len = ptr->varbind.oid_ptr.len;
                    //req->oid_ptr.ptr = ptr->varbind.oid_ptr.ptr;
                #endif
                break;
            }
        } else {
            #if ENABLE_MIB_TABLE
            /* handle a tabular object */
            if (cmp < 0 || cmp == 0) {
            	DEBUG_MSG(SNMP_DEBUG,"handle a tabular object\r\n");
                ptr_t* table_oid_ptr;
                if ((table_oid_ptr = (ptr->get_next_oid_fnc_ptr)(ptr, (cmp < 0 ? 0 : &req->oid_ptr->ptr[oid_len]),
                        cmp < 0 ? 0 : req->oid_ptr->len - oid_len)) != 0) {
                    /* copy the mib object's oid */
                    oid_copy(req->oid_ptr, ptr->varbind.oid_ptr, oid_len + table_oid_ptr->len);
                    CHECK_PTR_U(req->oid_ptr->ptr);
                    memcpy(&req->oid_ptr->ptr[oid_len], table_oid_ptr->ptr, table_oid_ptr->len);
                    req->oid_ptr->len += table_oid_ptr->len;

                    //free(table_oid_ptr->ptr);
                    vPortFree(table_oid_ptr->ptr);
                    oid_free(table_oid_ptr);
                    break;                    
                }
            }
            #endif
        }

        mib_iterator_next();
    }

    if (mib_iterator_is_end()) {
        req->value_type = BER_TYPE_END_OF_MIB;
        DEBUG_MSG(SNMP_DEBUG,"mib does not contain next object\r\n");
        return 0;
    }

    if (ptr->get_fnc_ptr) {
        if ((ptr->get_fnc_ptr)(ptr, &req->oid_ptr->ptr[oid_len],
                                    req->oid_ptr->len - oid_len) == -1) {
        	DEBUG_MSG(SNMP_DEBUG,"can not get the value of the object\r\n");
            return 0;
        }
    }

    /* copy the value */
    memcpy(&req->value, &ptr->varbind.value, sizeof(varbind_value_t));
    req->value_type = ptr->varbind.value_type;


    DEBUG_MSG(SNMP_DEBUG,"mib_get_next_end\r\n");

    return ptr;
}

/*-----------------------------------------------------------------------------------*/
/*
 * Set the value for an object in the MIB.
 */
s8t mib_set(mib_object_t* object, varbind_t* req)
{
    s8t ret;
    
    DEBUG_MSG(SNMP_DEBUG,"mib_set\r\n");
    print_oid(req->oid_ptr);

    if (object->set_fnc_ptr) {
        if ((ret = (object->set_fnc_ptr)(object,
                &req->oid_ptr->ptr[object->varbind.oid_ptr->len],
                req->oid_ptr->len - object->varbind.oid_ptr->len, req->value)) != ERROR_STATUS_NO_ERROR) {
        	DEBUG_MSG(SNMP_DEBUG,"can not set the value of the object\r\n");
            return ret;
        }
    } else {        
        switch (req->value_type) {
            case BER_TYPE_IPADDRESS:
            case BER_TYPE_OID:
            case BER_TYPE_OCTET_STRING:
                if ((object->attrs & FLAG_SET_VALUE) && object->varbind.value.p_value.ptr) {
                    //free(object->varbind.value.p_value.ptr);
                    vPortFree(object->varbind.value.p_value.ptr);
                }
                object->varbind.value.p_value.len = req->value.p_value.len;
                //object->varbind.value.p_value.ptr = (u8t*)malloc(req->value.p_value.len);
                object->varbind.value.p_value.ptr = (u8t*)pvPortMalloc(req->value.p_value.len);
                CHECK_PTR(object->varbind.value.p_value.ptr);
                memcpy(object->varbind.value.p_value.ptr, req->value.p_value.ptr, object->varbind.value.p_value.len);
                object->attrs |= FLAG_SET_VALUE;
                break;

            case BER_TYPE_INTEGER:
                object->varbind.value.i_value = req->value.i_value;
                break;

            case BER_TYPE_COUNTER:
            case BER_TYPE_TIME_TICKS:
            case BER_TYPE_GAUGE:
                object->varbind.value.u_value = req->value.u_value;
                break;

            default:
                return ERROR_STATUS_BAD_VALUE;
        }
    }

    return 0;
}


/*functuions for statistics*/

//print OID string
void print_oid(ptr_t* oid_prefix){
#if SNMP_DEBUG
	for(u8 i=0;i<oid_prefix->len;i++){
		DEBUG_MSG(SNMP_DEBUG,"%x.",oid_prefix->ptr[i]);
	}
	DEBUG_MSG(SNMP_DEBUG,"\r\n");
#endif
}

//prints OID string to file
void printf_oid(char *str,ptr_t* oid_prefix){
char temp[64];
u8 i;
str[0] = 0;

	print_oid(oid_prefix);

	if(oid_prefix->ptr[0]==0x28){
		sprintf(str,".1.0.8802");
		for(i=3;i<oid_prefix->len;i++){
			sprintf(temp,".%d",oid_prefix->ptr[i]);
			strcat(str,temp);
		}
	}
	else{
		for(i=0;i<oid_prefix->len;i++){
			if(i==0 && oid_prefix->ptr[0] == 0x2b){
				sprintf(temp,".1.3.");
			}
			else{
				if(oid_prefix->ptr[i] & 0x80)
					continue;
				if((oid_prefix->ptr[i-1] & 0x80) && (oid_prefix->ptr[i] == 35))
					sprintf(temp,"42019.");
				else
					sprintf(temp,"%d.",oid_prefix->ptr[i]);
			}
			strcat(str,temp);
		}
	}
	str[strlen(str)] = 0;
}

int get_mib_object(mib_object_t *object,int num){

	DEBUG_MSG(SNMP_DEBUG,"get_mib_object %d\r\n",num);

	/*if (num>=mib_length) {
		return -1;
	}

	memcpy(object,&mib[num],sizeof(mib_object_t));
*/
	return 0;
}
