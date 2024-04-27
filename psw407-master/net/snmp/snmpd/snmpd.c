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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//#include "contiki.h"
//#include "contiki-net.h"

#include "board.h"
#include "settings.h"
#include "snmpd.h"
#include "snmpd-conf.h"
#include "dispatcher.h"
#include "mib-init.h"
#include "logging.h"
#include "keytools.h"

#include "stm32f4xx_rtc.h"

#include "../uip/uip.h"
#include "../uip/uipopt.h"
#include "../clock-arch.h"
#include "../dhcp/dhcpc.h"

#include "task.h"
#include "FreeRTOS.h"

#include "SMIApi.h"

extern struct status_t status;

#include "../deffines.h"
#include "debug.h"

#define UDP_IP_BUF   ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

/* UDP connection */
static struct uip_udp_conn *udpconn;





/**/
/**my def section*/

char *getIfName(u8 ifnum){
	static char ifname[5];
	if(get_dev_type() == DEV_PSW2GPLUS){
		switch(ifnum){
			case 0:strcpy(ifname,"FE#1");break;
			case 1:strcpy(ifname,"FE#2");break;
			case 2:strcpy(ifname,"FE#3");break;
			case 3:strcpy(ifname,"FE#4");break;
			case 4:strcpy(ifname,"GE#1");break;
			case 5:strcpy(ifname,"GE#2");break;
		}
	}
	else{
		sprintf(ifname,"Port #%d",ifnum+1);
	}
	return ifname;
}

u32 getReceivedPackets(u8 ifnum){
	//return dev.port_stat[ifnum].rx_good;
	return 0;
}

u32 getReceivedOctets(u8 ifnum){
	return dev.port_stat[ifnum].rx_good;
}

u32 getFailReceived(u8 ifnum){
	return 0;
}

u32 getSentPackets(u8 ifnum){
	return 0;
}


u32 getSentOctets(u8 ifnum){
	return dev.port_stat[ifnum].tx_good;
}

u32 getSentUnicastPackets(u8 ifnum){
	if(ifnum<PORT_NUM)
		return dev.port_stat[ifnum].tx_unicast;
	return 0;
}

u32 getFailSent(u8 ifnum){
	return 0;
}



u32 get_arp_table_size(void){
	return UIP_ARPTAB_SIZE;
}


u32 getInMulticastPkt(u8 ifnum){
	return read_stats_cnt(ifnum,RX_MULTICAST);
}

u32 getOutMulticastPkt(u8 ifnum){
	return read_stats_cnt(ifnum,TX_MULTICAST);
}

u32 getOutBroadkastPkt(u8 ifnum){
	return read_stats_cnt(ifnum,TX_BROADCAST);
}

u32 getInBroadcastPkt(u8 ifnum){
	return read_stats_cnt(ifnum,RX_BROADCAST);
}

/*****/



#if CONTIKI_TARGET_AVR_RAVEN
extern unsigned long seconds;
#else
clock_time_t systemStartTime;
#endif

u32t getSysUpTime()
{
    #if CONTIKI_TARGET_AVR_RAVEN
        return clock_seconds();
	  //return seconds * 100;
    #else
        //return (clock_time() - systemStartTime)/ 10;
        return RTC_GetCounter()*100;// - systemStartTime)*100;
    #endif
}

#if CHECK_STACK_SIZE
int max = 0;
u32t* marker;
#endif

/*-----------------------------------------------------------------------------------*/
/*
 * UDP handler.
 */
static void udp_handler(void)
{   
    snmp_packets++;
    //u8t response[MAX_BUF_SIZE];
    u8 *response  = uip_appdata;

    u16t resp_len;
    #if CHECK_STACK_SIZE
    memset(response, 0, sizeof(response));
    #endif

    #if DEBUG && CONTIKI_TARGET_AVR_RAVEN
    u8t request[MAX_BUF_SIZE];
    u16t req_len;
    #endif /* DEBUG && CONTIKI_TARGET_AVR_RAVEN */

    if (/*ev == tcpip_event && */uip_newdata()) {//my

        #if INFO
            uip_ipaddr_t ripaddr;
            u16_t rport;
            uip_ipaddr_copy(&ripaddr, &UDP_IP_BUF->srcipaddr);
            rport = UDP_IP_BUF->srcport;
        #else
            uip_ipaddr_copy(&udpconn->ripaddr, &UDP_IP_BUF->srcipaddr);
            udpconn->rport = UDP_IP_BUF->srcport;
        #endif

        #if DEBUG && CONTIKI_TARGET_AVR_RAVEN
        req_len = uip_datalen();
        memcpy(request, uip_appdata, req_len);
        if (dispatch(request, &req_len, response, resp_len, MAX_BUF_SIZE) != ERR_NO_ERROR) {
            udpconn->rport = 0;
            memset(&udpconn->ripaddr, 0, sizeof(udpconn->ripaddr));
            return;
        }
        #else

        if (dispatch((u8_t*)uip_appdata, uip_datalen(), response, &resp_len, MAX_BUF_SIZE) != ERR_NO_ERROR) {
            udpconn->rport = 0;
            memset(&udpconn->ripaddr, 0, sizeof(udpconn->ripaddr));
            DEBUG_MSG(SNMP_DEBUG,"dispatch error\r\n");
            return;
        }

        #endif /* DEBUG && CONTIKI_TARGET_AVR_RAVEN */

        #if CHECK_STACK_SIZE
        u32t *p = marker - 1;
        u16t i = 0;
        while (*p != 0xAAAAAAAA || *(p - 1) != 0xAAAAAAAA || *(p - 2) != 0xAAAAAAAA) {
            i+=4;
            p--;
        }
        if (i > max) {
            max = i;
        }
        snmp_info(" %d", max);
        #endif


        #if INFO
            uip_ipaddr_copy(&udpconn->ripaddr, &ripaddr);
            udpconn->rport = rport;
        #endif

        
        //    response,
        uip_udp_send(resp_len);

        memset(&udpconn->ripaddr, 0, sizeof(udpconn->ripaddr));
        udpconn->rport = 0;
    }
}
/*-----------------------------------------------------------------------------------*/

#include "md5.h"

/*-----------------------------------------------------------------------------------*/
/*
 *  Entry point of the SNMP server.
 */
//PROCESS_THREAD(snmpd_process, ev, data) {

void snmpd_process(/*process_event_t ev,process_data_t data*/void){
	//PROCESS_BEGIN();

	snmp_packets = 0;

        #ifndef CONTIKI_TARGET_AVR_RAVEN
        systemStartTime = RTC_GetCounter()*100;//clock_time();
        #endif

        #if CHECK_STACK_SIZE
        u16t i = 0;
        u32t pointer;
        u32t* p = &pointer;
        for (i = 0; i < 1000; i++) {
            *p = 0xAAAAAAAA;
            p--;
        }
        marker = &pointer;
        #endif

	//udpconn = udp_new(NULL, UIP_HTONS(0), NULL);
	//udp_bind(udpconn, UIP_HTONS(LISTEN_PORT));

      udpconn = uip_udp_new(NULL,UIP_HTONS(0));
      if(udpconn != NULL)
    	  uip_udp_bind(udpconn, UIP_HTONS(LISTEN_PORT));


        /* init MIB */
        if (mib_init() != -1) {
            while(1) {
                 udp_handler();
            }
        } else {
        	DEBUG_MSG(SNMP_DEBUG,"error occurs while initializing the MIB\n");
        }
	//PROCESS_END();
    vTaskDelete(NULL);
}

void snmpd_init(void){

	if(get_snmp_state() == 0)
		return;
	snmp_packets = 0;
	systemStartTime = clock_time();
	if(udpconn){
		uip_udp_remove (udpconn);
	}

    udpconn = uip_udp_new(NULL,UIP_HTONS(0));
    if(udpconn != NULL)
  	  uip_udp_bind(udpconn, UIP_HTONS(LISTEN_PORT));

    /* init MIB */
    //
    //инициализация MIB происходит только один раз
    /*if (mib_init() == -1) {
    	DEBUG_MSG(SNMP_DEBUG,"error occurs while initializing the MIB\n");
        return;
    }*/

    //SNMPv3
    if(get_snmp_vers()==3){
    	setUserName();
    	setEngineID();
    	setAuthPrivKul();
    }

    DEBUG_MSG(SNMP_DEBUG,"snmpd init ok\r\n");


}

void snmpd_appcall(void){
	if(uip_hostaddr[0] == 0 && uip_hostaddr[1]==0)
		return;
	udp_handler();
}
