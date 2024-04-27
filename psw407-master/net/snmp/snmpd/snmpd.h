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

/**
 * \file
 *         Defines an SNMP agent process.
 * \author
 *         Siarhei Kuryla <kurilo@gmail.com>
 */


#ifndef __SNMPD_H__
#define __SNMPD_H__

//#include "contiki-net.h"
#include "stm32f4xx.h"
#include "snmpd-types.h"

/** \brief port listened by the SNMP agent */
// port 161 seems to be blocked on our firewall
//#define LISTEN_PORT 1610

#define LISTEN_PORT 161

uint32_t snmp_packets;



/**my def section*/

char *getIfName(u8 ifnum);
u32 getReceivedPackets(u8 ifnum);
u32 getReceivedOctets(u8 ifnum);
u32 getFailReceived(u8 ifnum);
u32 getSentPackets(u8 ifnum);
u32 getSentOctets(u8 ifnum);
u32 getSentUnicastPackets(u8 ifnum);
u32 getFailSent(u8 ifnum);

u32 get_arp_table_size(void);

u32 getInMulticastPkt(u8 ifnum);
u32 getInBroadcastPkt(u8 ifnum);
u32 getOutMulticastPkt(u8 ifnum);
u32 getOutBroadkastPkt(u8 ifnum);

/*****/


void snmpd_process(void);

void snmpd_init(void);
void snmpd_appcall(void);

/** \brief Time in seconds since the system started. */
u32t getSysUpTime();

#endif /* __SNMPD_H__ */
