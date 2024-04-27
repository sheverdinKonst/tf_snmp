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
 * @brief LLDP packet parsing implementation
 */
#include "stm32f4xx.h"
#include "stddef.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lldp_node.h"
#include "lldp_protocol.h"
#include "uip/uip.h"
#include "debug.h"
#include <string.h>
#include "lldp.h"
#include "../net/stp/stp_oslayer_freertos.h"
#include "settings.h"

//#include <vlibmemory/api.h>

typedef struct
{
  u32 hw_if_index;
  u8 chassis_id_len;
  u8 chassis_id_subtype;
  u8 portid_len;
  u8 portid_subtype;
  u16 ttl;
  u8 data[0];			/* this contains both chassis id (chassis_id_len bytes) and port
				   id (portid_len bytes) */
} lldp_intf_update_t;



static void
lldp_rpc_update_peer (u32 hw_if_index, const u8 * chid, u8 chid_len,
		      u8 chid_subtype, const u8 * portid,
		      u8 portid_len, u8 portid_subtype, u16 ttl)
{
  lldp_port_entry_t entry;
  entry.valid = 1;
  entry.if_index = hw_if_index;
  entry.chassis_id_len = chid_len;
  entry.chassis_id_subtype = chid_subtype;
  entry.ttl = ttl;
  entry.portid_len = portid_len;
  entry.portid_subtype = portid_subtype;
  if(chid_len<=LLDP_CHIDLEN)
	  memcpy (entry.chassis_id, chid, chid_len);
  else
	  entry.valid = 0;
  if(portid_len<=LLDP_CHIDLEN)
	  memcpy (entry.port_id, portid, portid_len);
  else
	  entry.valid = 0;

  DEBUG_MSG(LLDP_DEBUG,"chid len=%d %x:%x:%x:%x:%x:%x\r\n",chid_len,chid[0],chid[1],
		  chid[2],chid[3],chid[4],chid[5]);

  lldp_update_port(entry);
}

lldp_tlv_code_t
lldp_tlv_get_code (const lldp_tlv_t * tlv)
{
  return tlv->head.byte1 >> 1;
}

void
lldp_tlv_set_code (lldp_tlv_t * tlv, lldp_tlv_code_t code)
{
  tlv->head.byte1 = (tlv->head.byte1 & 1) + (code << 1);
}

u16
lldp_tlv_get_length (const lldp_tlv_t * tlv)
{
  return (((u16) (tlv->head.byte1 & 1)) << 8) + tlv->head.byte2;
}

void
lldp_tlv_set_length (lldp_tlv_t * tlv, u16 length)
{
  tlv->head.byte2 = length & ((1 << 8) - 1);
  if (length > (1 << 8) - 1)
    {
      tlv->head.byte1 |= 1;
    }
  else
    {
      tlv->head.byte1 &= (1 << 8) - 2;
    }
}

lldp_main_t lldp_main;

static int
lldp_packet_scan (u8 hw_if_index, const lldp_tlv_t * pkt)
{
  const lldp_tlv_t *tlv = pkt;
  lldp_port_entry_t entry;

  #define TLV_VIOLATES_PKT_BOUNDARY(pkt, tlv) 0

  /* first tlv is always chassis id, followed by port id and ttl tlvs */
  if (TLV_VIOLATES_PKT_BOUNDARY (pkt, tlv) ||
      LLDP_TLV_NAME (chassis_id) != lldp_tlv_get_code (tlv))
    {
      DEBUG_MSG(LLDP_DEBUG,"lldp_packet_scan LLDP_ERROR_BAD_TLV: lldp_tlv_get_code\r\n");
	  return LLDP_ERROR_BAD_TLV;
    }

  u16 l = lldp_tlv_get_length (tlv);

  if (l < sizeof (u8) +
      LLDP_MIN_CHASS_ID_LEN ||
      l > sizeof (u8) +
      LLDP_MAX_CHASS_ID_LEN)
    {
	  DEBUG_MSG(LLDP_DEBUG,"lldp_packet_scan LLDP_ERROR_BAD_TLV LLDP_MAX_CHASS_ID_LEN\r\n");
      return LLDP_ERROR_BAD_TLV;
    }

  u8 chid_subtype = ((lldp_chassis_id_tlv_t *) tlv)->subtype;
  u8 *chid = ((lldp_chassis_id_tlv_t *) tlv)->id;
  u8 chid_len = l - sizeof (/*lldp_chassis_id_tlv_t.subtype*/u8);

  tlv = (lldp_tlv_t *) ((u8 *) tlv + sizeof(/*lldp_tlv_t.head*/struct lldp_tlv_head) + l);

  if (TLV_VIOLATES_PKT_BOUNDARY (pkt, tlv) ||
      LLDP_TLV_NAME (port_id) != lldp_tlv_get_code (tlv))
    {
	  DEBUG_MSG(LLDP_DEBUG,"lldp_packet_scan LLDP_ERROR_BAD_TLV lldp_tlv_get_code\r\n");
	  return LLDP_ERROR_BAD_TLV;
    }
  l = lldp_tlv_get_length (tlv);
  if (l < sizeof (/*lldp_port_id_tlv_t.subtype*/u8) +
      LLDP_MIN_PORT_ID_LEN ||
      l > sizeof (/*lldp_chassis_id_tlv_t.subtype*/u8) +
      LLDP_MAX_PORT_ID_LEN)
    {
	  DEBUG_MSG(LLDP_DEBUG,"lldp_packet_scan LLDP_ERROR_BAD_TLV LLDP_MAX_PORT_ID_LEN\r\n");
	  return LLDP_ERROR_BAD_TLV;
    }

  u8 portid_subtype = ((lldp_port_id_tlv_t *) tlv)->subtype;
  u8 *portid = ((lldp_port_id_tlv_t *) tlv)->id;
  u8 portid_len = l - sizeof (/*lldp_port_id_tlv_t.subtype*/u8);



  tlv = (lldp_tlv_t *) ((u8 *) tlv + sizeof (/*lldp_tlv_t.head*/struct lldp_tlv_head) + l);

  if (TLV_VIOLATES_PKT_BOUNDARY (pkt, tlv) ||
      LLDP_TLV_NAME (ttl) != lldp_tlv_get_code (tlv))
    {
	  DEBUG_MSG(LLDP_DEBUG,"lldp_packet_scan LLDP_ERROR_BAD_TLV\r\n");
	  return LLDP_ERROR_BAD_TLV;
    }
  l = lldp_tlv_get_length (tlv);

  u16 ttl = ntohs (((lldp_ttl_tlv_t *) tlv)->ttl);
  tlv = (lldp_tlv_t *) ((u8 *) tlv + sizeof (struct lldp_tlv_head) + l);

  while (LLDP_TLV_NAME (pdu_end) != lldp_tlv_get_code (tlv)){
	  l = lldp_tlv_get_length (tlv);
	  DEBUG_MSG(LLDP_DEBUG,"lldp_packet_scan code = %d, len=%d\r\n",lldp_tlv_get_code (tlv),l);
	  switch (lldp_tlv_get_code (tlv))	{
		case LLDP_TLV_NAME (port_desc):
		    DEBUG_MSG(LLDP_DEBUG,"len = %d\r\n",l);
		 	if(l){
		 		if(l<LLDP_DESCR_LEN){
		 			strncpy (entry.port_descr, ((lldp_port_descr_tlv_t *) tlv)->descr, l);
		 			entry.port_descr_len = l;
		 			entry.port_descr[entry.port_descr_len]=0;
		 		}
		 		else{
		 			strncpy (entry.port_descr, ((lldp_port_descr_tlv_t *) tlv)->descr, LLDP_DESCR_LEN);
		 			entry.port_descr_len = LLDP_DESCR_LEN;
		 			entry.port_descr[LLDP_DESCR_LEN-1]=0;
		 		}
		 		DEBUG_MSG(LLDP_DEBUG,"lldp_packet_scan PORT DESCRIPTION %d %s\r\n",entry.port_descr_len,entry.port_descr);
		 	}
		 	else
		 		entry.port_descr_len = 0;

			break;

		case LLDP_TLV_NAME (sys_name):
			if(l){
			    if(l<LLDP_DESCR_LEN){
				    strncpy (entry.sys_name,  ((lldp_port_descr_tlv_t *) tlv)->descr, l);
				    entry.sys_name_len = l;
				    entry.sys_name[entry.sys_name_len]=0;
			    }
			    else{
				    strncpy (entry.sys_name,  ((lldp_port_descr_tlv_t *) tlv)->descr, LLDP_DESCR_LEN);
				    entry.sys_name_len = LLDP_DESCR_LEN;
				    entry.sys_name[LLDP_DESCR_LEN-1]=0;
			    }
			    DEBUG_MSG(LLDP_DEBUG,"lldp_packet_scan SYSTEM NAME %d %s\r\n",entry.sys_name_len,entry.sys_name);
		    }
			else
				entry.sys_name_len = 0;
			break;

		case LLDP_TLV_NAME (sys_desc):

			if(l){
				if(l<LLDP_DESCR_LEN){
					strncpy (entry.sys_descr,((lldp_port_descr_tlv_t *) tlv)->descr, l);
					entry.sys_descr_len = l;
					entry.sys_descr[entry.sys_descr_len] = 0;
				}
				else{
					strncpy (entry.sys_descr,((lldp_port_descr_tlv_t *) tlv)->descr, LLDP_DESCR_LEN);
					entry.sys_descr_len = LLDP_DESCR_LEN;
					entry.sys_descr[LLDP_DESCR_LEN-1] = 0;
				}
				DEBUG_MSG(LLDP_DEBUG,"lldp_packet_scan SYSTEM DESCRIPTION  %d %s\r\n",entry.sys_descr_len,entry.sys_descr);
			}
			else
				entry.sys_descr_len = 0;
			break;

		case LLDP_TLV_NAME (sys_caps):
			entry.sys_cap = ntohs(((lldp_cap_tlv_t *) tlv)->capabilities);
			entry.sys_cap_en = ntohs(((lldp_cap_tlv_t *) tlv)->enabled_capabilities);
			DEBUG_MSG(LLDP_DEBUG,"lldp_packet_scan SYSTEM CAPABILITYS %d %d \r\n",entry.sys_cap,entry.sys_cap_en);
			break;

		default:
			DEBUG_MSG(LLDP_DEBUG,"lldp_packet_scan LLDP_ERROR_BAD_TLV\r\n");
			return LLDP_ERROR_BAD_TLV;
	  }
	  tlv = (lldp_tlv_t *) ((u8 *) tlv + sizeof (struct lldp_tlv_head)+lldp_tlv_get_length (tlv));

  }



  /* last tlv is pdu_end */
  if (TLV_VIOLATES_PKT_BOUNDARY (pkt, tlv) ||
      LLDP_TLV_NAME (pdu_end) != lldp_tlv_get_code (tlv) ||
      0 != lldp_tlv_get_length (tlv))
    {
	  DEBUG_MSG(LLDP_DEBUG,"lldp_packet_scan LLDP_ERROR_BAD_TLV\r\n");
	  return LLDP_ERROR_BAD_TLV;
    }


  entry.valid = 1;
  entry.if_index = hw_if_index;
  entry.chassis_id_len = chid_len;
  entry.chassis_id_subtype = chid_subtype;
  entry.ttl = ttl;
  entry.portid_len = portid_len;
  entry.portid_subtype = portid_subtype;
  if(chid_len<=LLDP_CHIDLEN)
	  memcpy (entry.chassis_id, chid, chid_len);
  else
	  entry.valid = 0;
  if(portid_len<=LLDP_CHIDLEN)
	  memcpy (entry.port_id, portid, portid_len);
  else
	  memcpy (entry.port_id, portid, LLDP_CHIDLEN);

  DEBUG_MSG(LLDP_DEBUG,"chid len=%d %x:%x:%x:%x:%x:%x\r\n",chid_len,chid[0],chid[1],
		  chid[2],chid[3],chid[4],chid[5]);

  lldp_update_port(entry);

  return LLDP_ERROR_NONE;
}

lldp_intf_t *
lldp_get_intf (lldp_main_t * lm, u32 hw_if_index)
{
  /*uword *p = hash_get (lm->intf_by_hw_if_index, hw_if_index);

  if (p)
    {
      return pool_elt_at_index (lm->intfs, p[0]);
    }
    */
  return NULL;
}

lldp_intf_t *
lldp_create_intf (lldp_main_t * lm, u32 hw_if_index)
{
#if 0
  uword *p;
  lldp_intf_t *n;
  //p = hash_get (lm->intf_by_hw_if_index, hw_if_index);

  if (p == 0)
    {
      pool_get (lm->intfs, n);
      memset (n, 0, sizeof (*n));
      n->hw_if_index = hw_if_index;
      hash_set (lm->intf_by_hw_if_index, n->hw_if_index, n - lm->intfs);
    }
  else
    {
      n = pool_elt_at_index (lm->intfs, p[0]);
    }
  return n;
#endif
}

/*
 * lldp input routine
 */
lldp_error_t lldp_input (u8 port, u8 *buff){

  lldp_main_t *lm = &lldp_main;
  lldp_error_t e;

  if(get_lldp_state() == 0)
	  return e;


  DEBUG_MSG(LLDP_DEBUG,"lldp_input port=%d\r\n",port);

  /* Actually scan the packet */
  e = lldp_packet_scan (port, (const lldp_tlv_t *)buff);

  return e;
}


void lldp_rawinput(uint8_t *ptr, int size){
  struct ether_header *eh;
  int port = -1;
  int i;
  eh = (struct ether_header *)ptr;
  if(eh->ether_type == HTONS(LLDP_ETHTYPE)){
	  port = F2L_port_conv(stp_ethhdr2port(ptr));
	  lldp_input(port,(ptr+sizeof(struct ether_header)));
  }
}


/*
 * setup function
 */
/*static
//clib_error_t *
void
lldp_init ()
{
  //clib_error_t *error;
  lldp_main_t *lm = &lldp_main;

  //if ((error = vlib_call_init_function (vm, lldp_template_init)))
  //  return error;

  //lm->vlib_main = vm;
  //lm->vnet_main = vnet_get_main ();
  lm->msg_tx_hold = 4;		// default value per IEEE 802.1AB-2009
  lm->msg_tx_interval = 30;	// default value per IEEE 802.1AB-2009
  return;
}
*/

//VLIB_INIT_FUNCTION (lldp_init);

/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "gnu")
 * End:
 */

