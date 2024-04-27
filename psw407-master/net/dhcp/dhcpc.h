/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * @(#)$Id: dhcpc.h,v 1.4 2007/03/16 12:16:16 bg- Exp $
 */
#ifndef __DHCPC_H__
#define __DHCPC_H__

//#include "process.h"
#include "../net/uip/pt.h"
//#include "usb.h"
//#include "fifo.h"

#define DHCPS_SERVER_PORT  67
#define DHCPS_CLIENT_PORT  68

/**
 * Convert 16-bit quantity from host byte order to network byte order.
 *
 * This macro is primarily used for converting constants from host
 * byte order to network byte order. For converting variables to
 * network byte order, use the uip_htons() function instead.
 *
 * \hideinitializer
 */
#ifndef UIP_HTONS
#   if UIP_BYTE_ORDER == UIP_BIG_ENDIAN
#      define UIP_HTONS(n) (n)
#      define UIP_HTONL(n) (n)
#   else /* UIP_BYTE_ORDER == UIP_BIG_ENDIAN */
#      define UIP_HTONS(n) (uint16_t)((((uint16_t) (n)) << 8) | (((uint16_t) (n)) >> 8))
#      define UIP_HTONL(n) (((uint32_t)UIP_HTONS(n) << 16) | UIP_HTONS((uint32_t)(n) >> 16))
#   endif /* UIP_BYTE_ORDER == UIP_BIG_ENDIAN */
#else
#error "UIP_HTONS already defined!"
#endif /* UIP_HTONS */

/**
 * Convert a 16-bit quantity from host byte order to network byte order.
 *
 * This function is primarily used for converting variables from host
 * byte order to network byte order. For converting constants to
 * network byte order, use the UIP_HTONS() macro instead.
 */
#ifndef uip_htons
uint16_t uip_htons(uint16_t val);
#endif /* uip_htons */

#ifndef uip_ntohs
#define uip_ntohs uip_htons
#endif

#ifndef uip_htonl
uint32_t uip_htonl(uint32_t val);
#endif /* uip_htonl */
#ifndef uip_ntohl
#define uip_ntohl uip_htonl
#endif

#define DHCPC_SERVER_PORT  67
#define DHCPC_CLIENT_PORT  68

typedef struct dhcpcState_s
{
  struct pt pt;
  char state;
  struct uip_udp_conn *conn;
  struct timer timer;
  u32 ticks;
  const void *mac_addr;
  int mac_len;

  u8_t serverid [4];
  u16_t lease_time [2];
  u16_t ipaddr [2];
  u16_t netmask [2];
  u16_t dnsaddr [2];
  u16_t default_router [2];
  u16_t sntpaddr [2];
  u32 timeoffset;
}
dhcpcState_t;

void dhcp_client_config(void);

void dhcpc_init(const void *mac_addr, int mac_len);
void dhcpc_request(void);
void send_request(void);
void send_discover(void);
//void create_msg(struct dhcp_msg *m);
void dhcpc_appcall(void);
u8 get_dhcpc_state(void);
void set_dhcpc_state(u16 state);
void set_dhcpc_cfg_default(void);
//void DHCPC_start_task(void);
void dhcpc_configured (const dhcpcState_t *s);
u32 get_lease_time(void);
/* Mandatory callbacks provided by the user. */



#endif /* __DHCPC_H__ */
