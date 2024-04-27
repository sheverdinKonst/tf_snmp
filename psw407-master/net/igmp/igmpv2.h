/*
 * Copyright (c) 2002 CITEL Technologies Ltd.
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
 * 3. Neither the name of CITEL Technologies Ltd nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY CITEL TECHNOLOGIES AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL CITEL TECHNOLOGIES OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 *
 * This file is a contribution to the lwIP TCP/IP stack.
 * The Swedish Institute of Computer Science and Adam Dunkels
 * are specifically granted permission to redistribute this
 * source code.
*/

#ifndef __IGMP_H__
#define __IGMP_H__

//#include "opt.h"
//#include "ip_addr.h"
//#include "netif.h"
//#include "pbuf.h"

#if LWIP_IGMP /* don't build if not configured for use in lwipopts.h */


#ifdef __cplusplus
extern "C" {
#endif

#ifdef IGMP_DEBUG
	#define IGMP_STATS 1// статистика
#else

#endif




/* IGMP timer */
#define IGMP_TMR_INTERVAL              100 /* Milliseconds */
#define IGMP_V1_DELAYING_MEMBER_TMR   (1000/IGMP_TMR_INTERVAL)
#define IGMP_JOIN_DELAYING_MEMBER_TMR (500 /IGMP_TMR_INTERVAL)

/* MAC Filter Actions, these are passed to a netif's
 * igmp_mac_filter callback function. */
#define IGMP_DEL_MAC_FILTER            0
#define IGMP_ADD_MAC_FILTER            1


#define STATS_INC(x) {} ++x
#define STATS_DEC(x) {} --x

#define IPADDR_ANY NULL

#define uip_ipaddr_isany(addr1) ((addr1) == IPADDR_ANY)

#define ip_addr_ismulticast(addr1) (uip_ipaddr4(addr1) == 0xe0)

#if IGMP_STATS
#define IGMP_STATS_INC(x) STATS_INC(x)
#define IGMP_STATS_DISPLAY() stats_display_igmp()
#else
#define IGMP_STATS_INC(x)
#define IGMP_STATS_DISPLAY()
#endif


/**
 * igmp group structure - there is
 * a list of groups for each interface
 * these should really be linked from the interface, but
 * if we keep them separate we will not affect the lwip original code
 * too much
 * 
 * There will be a group for the all systems group address but this 
 * will not run the state machine as it is used to kick off reports
 * from all the other groups
 */


struct stats_igmp {
  u32 xmit;             /* Transmitted packets. */
  u32 recv;             /* Received packets. */
  u32 drop;             /* Dropped packets. */
  u32 chkerr;           /* Checksum error. */
  u32 lenerr;           /* Invalid length error. */
  u32 memerr;           /* Out of memory error. */
  u32 proterr;          /* Protocol error. */
  u32 rx_v1;            /* Received v1 frames. */
  u32 rx_group;         /* Received group-specific queries. */
  u32 rx_general;       /* Received general queries. */
  u32 rx_report;        /* Received reports. */
  u32 rx_leave;         /* Received leaves. */
  u32 tx_join;          /* Sent joins. */
  u32 tx_leave;         /* Sent leaves. */
  u32 tx_report;        /* Sent reports. */
  u32 tx_general;       /* Sent general query. */
  u32 tx_query;         /* Sent specific query. */
};


#define IGMP_NOQUERIER				0
#define IGMP_QUERIER				1

//общее для всех групп
#define IGMP_IDLE					0	//ничего не делать
#define IGMP_START_WAIT_QI			1	//запуск таймера Query interval
#define IGMP_WAIT_QI				2	//ждем таймер Query interval
#define IGMP_SEND_GEN_QUERY	   		3	//послать general query
#define IGMP_START_WAIT_REPORTS_MRT	4	//старт ожидания reports на general request / max response time
#define IGMP_WAIT_REPORTS_MRT  		5	//ожидание reports на general request / max response time
#define IGMP_WAIT_QPI				6	//ожидание querier present interval

//для каждой конкретной групппы отдельно
#define IGMP_LEAVE					7	//принят запрос на удаление из группы
#define IGMP_SEND_GSPEC_QUERY	   	8	//послать group specific query
#define IGMP_WAIT_REPORTS_GS		9	//ожидание reports на group specific qurey





/*struct  definition*/
struct igmp_group_t {
  u8			   ports[PORT_NUM];//port
  //u16 			   ports_mask;
  uip_ipaddr_t     group_addr;/** multicast address */
  u8 			   reports[PORT_NUM];//ответы report
  u8 			   leaves[PORT_NUM]; // запросы leave

  u8 			   state;//текущее состояние группы
  struct timer	   timer;//таймер

};

struct igmp_t {
	u8 				state_querier;//1 - если qurerier
	u8 				state;//текущее состояние
	struct timer 	timer_qi;//таймер посылки gen query
	struct timer 	timer_mrt;//таймер ожидания max response time
	struct timer 	timer_qpi;//таймер query present interval

};

struct igmp_msg {
	u8_t           igmp_msgtype;
 	u8_t           igmp_maxresp;
 	u16_t          igmp_checksum;
 	uip_ipaddr_t   igmp_group_address;
} PACK_STRUCT_STRUCT;

struct igmp_v3_msg_t {
	u8_t           igmp_msgtype;
 	u8_t           igmp_maxresp;
 	u16_t          igmp_checksum;
 	uip_ipaddr_t   igmp_group_address;
 	u8_t		   rsv:4;
 	u8_t 		   s_flag:1;
 	u8_t 		   qrv:3;
 	u8_t		   qqic;
 	u16_t 		   numsource;
 	uip_ipaddr_t   *sa_address;//pointer to sa array
};



//typedef err_t u8;

/*  Prototypes */
void   igmp_init(void);
i8  igmp_start(u8 netif);
i8  igmp_stop(u8 netif);
void   igmp_report_groups(u8 netif);
//struct igmp_group_t *igmp_lookfor_group(u8  ifp, uip_ipaddr_t addr);
int 	igmp_lookfor_group(u8  ifp, uip_ipaddr_t addr);
void   igmp_input(u8 inp, uip_ipaddr_t dest);
//i8 igmp_joingroup(uip_ipaddr_t ifaddr, uip_ipaddr_t groupaddr);
//i8  igmp_leavegroup(uip_ipaddr_t ifaddr, uip_ipaddr_t groupaddr);
void   igmp_tmr(void);
void igmp_dump_group_list(void);

u8 get_igmp_groupsnum(void);
void get_igmp_group(u8 num, uip_ipaddr_t addr);
u8 get_igmp_group_port(u8 num,u8 port);


void   igmp_init(void);
void add_igmp_port(u8 port);
void igmp_appcall(void);

void igmp_task_start(void);

u8 get_igmp_querier(void);
void stats_display_igmp(void);
#ifdef __cplusplus
}
#endif

#endif /* LWIP_IGMP */

#endif /* __LWIP_IGMP_H__ */
