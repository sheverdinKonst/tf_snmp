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
 * @brief LLDP global declarations
 */
#ifndef __included_lldp_node_h__
#define __included_lldp_node_h__
/*
#include <vlib/vlib.h>
#include <vlib/unix/unix.h>
#include <vnet/snap/snap.h>
#include <vppinfra/format.h>
#include <vppinfra/hash.h>
*/
#include "lldp_protocol.h"
#include <stdarg.h>
#include "deffines.h"
#include "board.h"


#define LLDP_DESCR_LEN	36//описание
typedef struct{
	u8 valid;
	u8 if_index;
	u8 chassis_id_len;
	u8 chassis_id_subtype;
	u8 portid_len;
	u8 portid_subtype;
	u16 ttl;
	u8 chassis_id[LLDP_CHIDLEN];
	u8 port_id[LLDP_CHIDLEN];
	u8 port_descr[LLDP_DESCR_LEN];
	u8 port_descr_len;
	u8 sys_name[LLDP_DESCR_LEN];
	u8 sys_name_len;
	u8 sys_descr[LLDP_DESCR_LEN];
	u8 sys_descr_len;
	u16 sys_cap;
	u16 sys_cap_en;
}lldp_port_entry_t;

lldp_port_entry_t lldp_port_entry[LLDP_ENTRY_NUM];

void lldp_update_port(lldp_port_entry_t entry);
void lldp_update_timer(void);//вызывать раз в 1 секунду

void convert_lldp_chasis_id_subtype(char *str, u8 subtype);
void convert_lldp_chasis_id(char *str, u8 subtype, u8*id);
void convert_lldp_port_id_subtype(char *str, u8 subtype);
void convert_lldp_port_id(char *str, u8 subtype, u8*id);
void convert_lldp_cap(char *str,u8 cap,u8 lang);
u8 lldp_entry_compare(lldp_port_entry_t *entry1,lldp_port_entry_t *entry2);
u16 get_lldp_entry_num(void);
#if 1

//todo
typedef u32 uword;
typedef u32 f64;



typedef struct lldp_intf
{
  /* hw interface index */
  u32 hw_if_index;

  /* Timers */
  f64 last_heard;
  f64 last_sent;

  /* Info received from peer */
  u8 *chassis_id;
  u8 *port_id;
  u16 ttl;
  lldp_port_id_subtype_t port_id_subtype;
  lldp_chassis_id_subtype_t chassis_id_subtype;

  /* Local info */
  u8 *port_desc;

  /* management ipv4 address */
  u8 *mgmt_ip4;

  /* management ipv6 address */
  u8 *mgmt_ip6;

  /* management object identifier */
  u8 *mgmt_oid;
} lldp_intf_t;



typedef struct
{
  /* pool of lldp-enabled interface context data */
  lldp_intf_t *intfs;

  /* rapidly find an interface by vlib hw interface index */
  uword *intf_by_hw_if_index;

  /* Background process node index */
  u32 lldp_process_node_index;

  /* interface idxs (into intfs pool) in the order of timing out */
  u32 *intfs_timeouts;

  /* index of the interface which will time out next */
  u32 intfs_timeouts_idx;

  /* packet template for sending out packets */
  //vlib_packet_template_t packet_template;

  /* convenience variables */
  //vlib_main_t *vlib_main;
  //vnet_main_t *vnet_main;

  /* system name advertised over LLDP (default is none) */
  u8 *sys_name;

  /* IEEE Std 802.1AB-2009:
   * 9.2.5.6 msgTxHold
   * This variable is used, as a multiplier of msgTxInterval, to determine the
   * value of txTTL that is carried in LLDP frames transmitted by the LLDP
   * agent. The recommended default value of msgTxHold is 4; this value can
   * be changed by management to any value in the range 1 through 100.
   */
  u8 msg_tx_hold;

  /* IEEE Std 802.1AB-2009:
   * 9.2.5.7 msgTxInterval
   * This variable defines the time interval in timer ticks between
   * transmissions during normal transmission periods (i.e., txFast is zero).
   * The recommended default value for msgTxInterval is 30 s; this value can
   * be changed by management to any value in the range 1 through 3600.
   */
  u16 msg_tx_interval;
} lldp_main_t;

#define LLDP_MIN_TX_HOLD (1)
#define LLDP_MAX_TX_HOLD (100)
#define LLDP_MIN_TX_INTERVAL (1)
#define LLDP_MAX_TX_INTERVAL (3600)

extern lldp_main_t lldp_main;

/* Packet counters */
#define foreach_lldp_error(F)                     \
    F(NONE, "good lldp packets (processed)")      \
    F(CACHE_HIT, "good lldp packets (cache hit)") \
    F(BAD_TLV, "lldp packets with bad TLVs")      \
    F(DISABLED, "lldp packets received on disabled interfaces")

typedef enum
{
#define F(sym, str) LLDP_ERROR_##sym,
  foreach_lldp_error (F)
#undef F
    LLDP_N_ERROR,
} lldp_error_t;

/* lldp packet trace capture */
typedef struct
{
  u32 len;
  u8 data[400];
} lldp_input_trace_t;

enum
{
  LLDP_EVENT_RESCHEDULE = 1,
} lldp_process_event_t;

lldp_intf_t *lldp_get_intf (lldp_main_t * lm, u32 hw_if_index);
lldp_intf_t *lldp_create_intf (lldp_main_t * lm, u32 hw_if_index);
void lldp_delete_intf (lldp_main_t * lm, lldp_intf_t * n);
lldp_error_t lldp_input (u8  port,u8 *buff);
u8 *lldp_input_format_trace (u8 * s, va_list * args);
void lldp_send_ethernet (lldp_main_t * lm, lldp_intf_t * n, int shutdown);
void lldp_schedule_intf (lldp_main_t * lm, lldp_intf_t * n);
void lldp_unschedule_intf (lldp_main_t * lm, lldp_intf_t * n);
u16 lldp_add_tlvs (lldp_main_t * lm, u8 hw_port, u8 *t0p,int shutdown, lldp_intf_t * n);
int lldp_send_port(int port);
#endif /* __included_lldp_node_h__ */

/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "gnu")
 * End:
 */
#endif
