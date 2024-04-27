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
 * @brief LLDP packet generation implementation
 */
#include "stm32f4xx.h"
#include "lldp_node.h"
#include <string.h>
#include "../net/uip/uip.h"
#include "../net/uip/uip_arp.h"
#include "lldp.h"
#include "board.h"
#include "task.h"
#include "debug.h"
#include "settings.h"

extern uint8_t lldp_etheraddr[6];
extern u8 dev_addr[6];

static u8 oid[10]  = {0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0xc8, 0x23, 0x03, 0x02};

void lldp_output(void){

	for(u8 i=0;i<ALL_PORT_NUM;i++){
		if(get_lldp_state() && get_lldp_port_state(i)){
			lldp_send_port(i);
		}
	}
}

int lldp_send_port(int port){
  struct uip_eth_hdr *eh;
	lldp_main_t  lm;
	lldp_intf_t * n;
	int shutdown=1;
	u8 *t0;
	u16 len=100;
	u8 buf[1000];

	//taskENTER_CRITICAL();
    eh = (struct uip_eth_hdr *)buf;

    //make Ethernet header
    memcpy(eh->src.addr, dev_addr, 6);
    memcpy(eh->dest.addr, lldp_etheraddr, 6);
    eh->type = HTONS(LLDP_ETHTYPE);
#if DSA_TAG_ENABLE
	//FROM_CPU DSA Tag - они одинаковы для всех чипов
	eh->dsa_tag = HTONL(0x40000000|(L2F_port_conv(port)<<19)|(7<<13));
#endif
	/* add TLVs */
	lm.msg_tx_interval = get_lldp_transmit_interval();
	lm.msg_tx_hold = get_lldp_hold_multiplier();
	shutdown = 0;
	len = lldp_add_tlvs (&lm, port, buf+sizeof(struct uip_eth_hdr), shutdown, n);

    ETH_HandleTxPkt(buf, len+sizeof(struct uip_eth_hdr));
    //taskEXIT_CRITICAL();

    //DEBUG_MSG(LLDP_DEBUG,"lldp_send_port=%d len = %d\r\n",port,len+sizeof(struct uip_eth_hdr));

    return 0;
}

#if 1


static u16 lldp_build_mgmt_addr_tlv (u8 *t0p, u8 subtype, u8 addr_len, uip_ipaddr_t addr,
			  u32 if_index, u8 oid_len, u8 *oid)
{
  lldp_tlv_t *t = (lldp_tlv_t *) t0p;

  lldp_tlv_set_code (t, LLDP_TLV_NAME (mgmt_addr));
  t->v[0] = addr_len + 1;	/* address string length */
  t->v[1] = subtype;		/* address subtype */
  memcpy (&(t->v[2]), addr, addr_len);	/* address */
  t->v[addr_len + 2] = 2;	/* interface numbering subtype: ifIndex */
  t->v[addr_len + 3] = (if_index >> 24) & 0xFF;	/* interface number */
  t->v[addr_len + 4] = (if_index >> 16) & 0xFF;
  t->v[addr_len + 5] = (if_index >> 8) & 0xFF;
  t->v[addr_len + 6] = (if_index >> 0) & 0xFF;
  t->v[addr_len + 7] = oid_len;	/* OID string length */

  if (oid_len > 0)
    memcpy ((u8 *) & (t->v[addr_len + 8]), oid, oid_len);

  lldp_tlv_set_length (t, addr_len + oid_len + 8);
  return sizeof(struct lldp_tlv_head) + addr_len + oid_len + 8;
}

static u16
lldp_add_chassis_id (u8 hw_port, u8 *t0p)
{
  lldp_chassis_id_tlv_t *t = (lldp_chassis_id_tlv_t *) t0p;

  lldp_tlv_set_code ((lldp_tlv_t *) t, LLDP_TLV_NAME (chassis_id));
  t->subtype = LLDP_CHASS_ID_SUBTYPE_NAME (mac_addr);

  const size_t addr_len = 6;
  memcpy (&t->id, dev_addr, addr_len);
  const size_t len = 1+addr_len;
  lldp_tlv_set_length ((lldp_tlv_t *) t, len);
  return sizeof(struct lldp_tlv_head) + len;
}

static u16 lldp_add_port_id (u8 hw_port, u8 *t0p)
{
char temp[10];
u8 name_len;

  lldp_port_id_tlv_t *t = (lldp_port_id_tlv_t *) t0p;

  lldp_tlv_set_code ((lldp_tlv_t *) t, LLDP_TLV_NAME (port_id));
  t->subtype = LLDP_PORT_ID_SUBTYPE_NAME (local);
  sprintf(temp,"1/%d",hw_port+1);
  name_len = strlen(temp);
  memcpy (&t->id, temp, name_len);
  const size_t len = 1+name_len;
  lldp_tlv_set_length ((lldp_tlv_t *) t, len);
  return sizeof(struct lldp_tlv_head) + len;
}

static u16 lldp_add_ttl (const lldp_main_t * lm, u8 *t0p, int shutdown)
{
  lldp_ttl_tlv_t *t = (lldp_ttl_tlv_t *) t0p;
  lldp_tlv_set_code ((lldp_tlv_t *) t, LLDP_TLV_NAME (ttl));
  if (shutdown)
    {
      t->ttl = 0;
    }
  else{
      if ((size_t) lm->msg_tx_interval * lm->msg_tx_hold + 1 > (1 << 16) - 1)
	{
	  t->ttl = htons ((1 << 16) - 1);
	}
      else
	{
	  t->ttl = htons (lm->msg_tx_hold * lm->msg_tx_interval + 1);
	}
  }
  const size_t len = 2;
  lldp_tlv_set_length ((lldp_tlv_t *) t, len);
  return sizeof(struct lldp_tlv_head) + len;
}

static u16 lldp_add_port_desc (u8 hw_port, u8 *t0p)
{

  u8 len;
  char temp[64];
  char temp1[64];

  get_port_descr(hw_port,temp);
  if(strlen(temp)==0){
	  get_dev_name(temp);
	  sprintf(temp1," Port %d",hw_port+1);
	  strcat(temp,temp1);
  }
  len = strlen(temp);



  lldp_tlv_t *t = (lldp_tlv_t *) t0p;
  lldp_tlv_set_code (t, LLDP_TLV_NAME (port_desc));
  lldp_tlv_set_length (t, len);

  memcpy (t->v, temp, len);
  return sizeof(struct lldp_tlv_head) + len;

}

static u16 lldp_add_sys_name (u8 *t0p)
{
  u8 len;
  char system_name[DESCRIPT_LEN];
  get_interface_name(system_name);
  len = strlen(system_name);

  lldp_tlv_t *t = (lldp_tlv_t *) t0p;
  lldp_tlv_set_code (t, LLDP_TLV_NAME (sys_name));
  lldp_tlv_set_length (t, len);
  memcpy (t->v, system_name, len);
  return sizeof(struct lldp_tlv_head) + len;
}

static u16 lldp_add_sys_descr (u8 *t0p)
{
  u8 len;
  char name[64];
  get_dev_name(name);
  len = strlen(name);

  lldp_tlv_t *t = (lldp_tlv_t *) t0p;
  lldp_tlv_set_code (t, LLDP_TLV_NAME (sys_desc));
  lldp_tlv_set_length (t, len);
  memcpy (t->v, name, len);
  return sizeof(struct lldp_tlv_head) + len;
}


//system capability
static u16 lldp_add_sys_cap (u8 *t0p)
{
  u8 len;

  lldp_cap_tlv_t *t = (lldp_cap_tlv_t *) t0p;
  lldp_tlv_set_code ((lldp_tlv_t *)t, LLDP_TLV_NAME (sys_caps));

  t->capabilities = htons(0x06);
  t->enabled_capabilities =htons(0x06);

  len = 4;
  lldp_tlv_set_length ((lldp_tlv_t *) t, len);
  return sizeof(struct lldp_tlv_head) + len;
}

static u16 lldp_add_mgmt_addr (u8 hw_port, u8 *t0p)
{
  u8 len_ip4,oid_len;
  len_ip4 = 4;

  //u8 oid[64];
  uip_ipaddr_t ip;
  oid_len = 10;


  if (len_ip4){
	  get_net_ip(ip);
      return lldp_build_mgmt_addr_tlv (t0p, 1,	/* address subtype: Ipv4 */
				len_ip4,	/* address string lenth */
				ip,	/* address */
				hw_port,	/* if index */
				oid_len,	/* OID length */
				oid);	/* OID */
  }
  else
	  return 0;


}

static u16 lldp_add_pdu_end (u8 *t0p)
{
  lldp_tlv_t *t = (lldp_tlv_t *)  t0p;
  lldp_tlv_set_code (t, LLDP_TLV_NAME (pdu_end));
  lldp_tlv_set_length (t, 0);
  return sizeof(struct lldp_tlv_head);
}

u16 lldp_add_tlvs (lldp_main_t * lm, u8 hw_port, u8 *t0p,int shutdown, lldp_intf_t * n)
{
u16 len;
  len = lldp_add_chassis_id (hw_port, t0p);
  len += lldp_add_port_id (hw_port,t0p+len);
  len += lldp_add_ttl (lm, t0p+len, shutdown);
  len += lldp_add_port_desc (hw_port, t0p+len);
  len += lldp_add_sys_name (t0p+len);
  len += lldp_add_sys_descr (t0p+len);
  len += lldp_add_sys_cap (t0p+len);
  len += lldp_add_mgmt_addr (hw_port, t0p+len);
  len += lldp_add_pdu_end (t0p+len);
  return len;
}

/*
 * send a lldp pkt on an ethernet interface
 */
void
lldp_send_ethernet (lldp_main_t * lm, lldp_intf_t * n, int shutdown)
{
#if 0
  u32 *to_next;
  ethernet_header_t *h0;
  vnet_hw_interface_t *hw;
  u32 bi0;
  vlib_buffer_t *b0;
  u8 *t0;
  vlib_frame_t *f;
  vlib_main_t *vm = lm->vlib_main;
  vnet_main_t *vnm = lm->vnet_main;

  /*
   * see lldp_template_init() to understand what's already painted
   * into the buffer by the packet template mechanism
   */
  h0 = vlib_packet_template_get_packet (vm, &lm->packet_template, &bi0);

  if (!h0)
    return;

  /* Add the interface's ethernet source address */
  hw = vnet_get_hw_interface (vnm, n->hw_if_index);

  clib_memcpy (h0->src_address, hw->hw_address, vec_len (hw->hw_address));

  u8 *data = ((u8 *) h0) + sizeof (*h0);
  t0 = data;

  /* add TLVs */
  lldp_add_tlvs (lm, hw, &t0, shutdown, n);

  /* Set the outbound packet length */
  b0 = vlib_get_buffer (vm, bi0);
  b0->current_length = sizeof (*h0) + t0 - data;

  /* And the outbound interface */
  vnet_buffer (b0)->sw_if_index[VLIB_TX] = hw->sw_if_index;

  /* And output the packet on the correct interface */
  f = vlib_get_frame_to_node (vm, hw->output_node_index);
  to_next = vlib_frame_vector_args (f);
  to_next[0] = bi0;
  f->n_vectors = 1;

  vlib_put_frame_to_node (vm, hw->output_node_index, f);
  n->last_sent = vlib_time_now (vm);
#endif
}

void
lldp_delete_intf (lldp_main_t * lm, lldp_intf_t * n)
{
#if 0
  if (n)
    {
      lldp_unschedule_intf (lm, n);
      hash_unset (lm->intf_by_hw_if_index, n->hw_if_index);
      vec_free (n->chassis_id);
      vec_free (n->port_id);
      vec_free (n->port_desc);
      vec_free (n->mgmt_ip4);
      vec_free (n->mgmt_ip6);
      vec_free (n->mgmt_oid);
      pool_put (lm->intfs, n);
    }
#endif
}

#if 0
void lldp_template_init (/*vlib_main_t * vm*/)
{
  lldp_main_t *lm = &lldp_main;

  /* Create the ethernet lldp packet template */
  {
    ethernet_header_t h;

    memset (&h, 0, sizeof (h));

    /*
     * Send to 01:80:C2:00:00:0E - propagation constrained to a single
     * physical link - stopped by all type of bridge
     */
    h.dst_address[0] = 0x01;
    h.dst_address[1] = 0x80;
    h.dst_address[2] = 0xC2;
    /* h.dst_address[3] = 0x00; (memset) */
    /* h.dst_address[4] = 0x00; (memset) */
    h.dst_address[5] = 0x0E;

    /* leave src address blank (fill in at send time) */

    h.type = htons (ETHERNET_TYPE_802_1_LLDP);

    vlib_packet_template_init (vm, &lm->packet_template,
			       /* data */ &h, sizeof (h),
			       /* alloc chunk size */ 8, "lldp-ethernet");
  }

  return 0;
}
#endif

//VLIB_INIT_FUNCTION (lldp_template_init);

/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "gnu")
 * End:
 */
#endif
