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

#include "msg-proc-v1.h"
#include "ber.h"
#include "utils.h"
#include "logging.h"
#include "dispatcher.h"
#include "debug.h"
#include "../deffines.h"
#include "../snmp.h"
#include "board.h"
#include "settings.h"


#if ENABLE_SNMPv1

s8t prepareDataElements_v1(u8t* const input, const u16t input_len, u16t* pos, message_t* request) {
    /* decode community string */
    ptr_t community;
    u8t type;
    TRY(ber_decode_string((u8t*)input, input_len, pos, &community.ptr, &community.len));

    char comm_r[16];
    char comm_w[16];

    get_snmp1_read_communitie(comm_r);
    get_snmp1_write_communitie(comm_w);


   type =  request->pdu.request_type & 0x0f;

    if ((request->pdu.error_status == ERROR_STATUS_NO_ERROR) &&
			((((type==SNMP_GET_REQUEST)||
			(type==SNMP_GET_NEXT_REQUEST)||
			(type==SNMP_GET_RESPONSE))&&
			((strlen(comm_r)!=community.len) || strncmp(comm_r,(char *)community.ptr,community.len))) &&
			(((type==SNMP_SET_REQUEST))&&
			((strlen(comm_w)!=community.len) || strncmp(comm_w,(char *)community.ptr,community.len))))
    ){
    	 /* the protocol entity notes this failure, (possibly) generates a trap, and discards the datagram
		 and performs no further actions. */
		request->pdu.error_status = ERROR_STATUS_GEN_ERR;
		request->pdu.error_index = 0;

		DEBUG_MSG(SNMP_DEBUG,"wrong community\r\n");

		DEBUG_MSG(SNMP_DEBUG,"request type %d\r\n", (request->pdu.request_type & 0x0F));
		DEBUG_MSG(SNMP_DEBUG,"comm_r %s ,%d \r\n",comm_r,strlen(comm_r));
		DEBUG_MSG(SNMP_DEBUG,"comm_w %s ,%d \r\n",comm_w,strlen(comm_w));
		DEBUG_MSG(SNMP_DEBUG,"community %s ,%d \r\n",(char *)community.ptr,community.len);

		return 0;
	} else {
		DEBUG_MSG(SNMP_DEBUG,"authentication passed\r\n");
	}

#if 0
    /* community-based authentication scheme */
    if (request->pdu.error_status == ERROR_STATUS_NO_ERROR &&
            (strlen(COMMUNITY_STRING) != community.len ||
            strncmp(COMMUNITY_STRING, (char *)community.ptr, community.len))) {
        /* the protocol entity notes this failure, (possibly) generates a trap, and discards the datagram
         and performs no further actions. */
        request->pdu.error_status = ERROR_STATUS_GEN_ERR;
        request->pdu.error_index = 0;
        DEBUG_MSG(SNMP_DEBUG,"wrong community string (length = %d)\r\n",community.len);

        DEBUG_MSG(SNMP_DEBUG,"community = %d\r\n",strlen(COMMUNITY_STRING));
        DEBUG_MSG(SNMP_DEBUG,"community = %s\r\n",(char *)community.ptr);

        return 0;
    } else {
        DEBUG_MSG(SNMP_DEBUG,"authentication passed\r\n");
    }
#endif

    /* decode the PDU */
    s8t ret = ber_decode_pdu(input, input_len, pos, &request->pdu);
    TRY(ret);

    /* if we ran out of memory send a general error */
    if (ret == ERR_MEMORY_ALLOCATION) {
        request->pdu.error_status = ERROR_STATUS_GEN_ERR;
    } else if (ret != ERR_NO_ERROR) {
        /* if the parsing fails, it discards the datagram and performs no further actions. */
        return FAILURE;
    }
    return 0;
}


/*-----------------------------------------------------------------------------------*/
/*
 * Encode an SNMPv1 response message in BER
 */
static s8t encode_v1_response(const message_t* const message, u8t* output, u16t* output_len, const u8t* const input, u16t input_len, const u16t max_output_len, const u8 w_flag)
{
    s16t pos = max_output_len;
    char commun_r[16];
    char commun_w[16];

    get_snmp1_read_communitie(commun_r);
    get_snmp1_write_communitie(commun_w);

    ber_encode_pdu(output, &pos, input, input_len, &message->pdu, max_output_len);

    /* community string */
    if(w_flag == 1){
    	TRY(ber_encode_fixed_string(output, &pos, (u8t*)commun_w, strlen(commun_w)));
    }
    else{
    	TRY(ber_encode_fixed_string(output, &pos, (u8t*)commun_r, strlen(commun_r)));
    }

    /* version */
    TRY(ber_encode_integer(output, &pos, BER_TYPE_INTEGER, message->version));

    /* sequence header*/
    TRY(ber_encode_type_length(output, &pos, BER_TYPE_SEQUENCE, max_output_len - pos));

    *output_len = max_output_len - pos;
    if (pos > 0) {
        memmove(output, output + pos, *output_len);
    }
    return 0;
}

s8t prepareResponseMessage_v1(message_t* message, u8t* output, u16t* output_len, const u8t* const input, u16t input_len, const u16t max_output_len, u8t w_flag) {
    /* encode the response */
    if (encode_v1_response(message, output, output_len, input, input_len, max_output_len,w_flag) != ERR_NO_ERROR) {
        /* Too big message.
         * If the size of the GetResponse-PDU generated as described
         * below would exceed a local limitation, then the receiving
         * entity sends to the originator of the received message
         * the GetResponse-PDU of identical form, except that the
         * value of the error-status field is tooBig, and the value
         * of the error-index field is zero.
         */
        message->pdu.error_status = ERROR_STATUS_TOO_BIG;
        message->pdu.error_index = 0;
        if (encode_v1_response(message, output, output_len, input, input_len, max_output_len,w_flag) == -1) {
            incSilentDrops();
            return FAILURE;
        }
    }
    return 0;
}

#endif
