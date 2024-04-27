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
 * @(#)$Id: dhcpc.c,v 1.9 2010/10/19 18:29:04 adamdunkels Exp $
 */

//#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include "stm32f4x7_eth.h"
#include "stm32f4xx_rtc.h"
#include "../deffines.h"
#include "../uip/uip.h"
#include "../sntp/sntp.h"
#include "../smtp/smtp.h"
#include "../syslog/msg_build.h"
#include "task.h"
#include "board.h"
#include "settings.h"
#include "dhcp.h"
#include "dhcpc.h"
#include "../snmp/snmp.h"
#include "../snmp/snmpd/snmpd.h"
#include "../snmp/snmpd/mib-init.h"
#include "../psw_search/search.h"
#include "../dns/resolv.h"
#include "../telnet/telnetd.h"
#include "../syslog/syslog.h"
#include "../tftp/tftpserver.h"
#include "debug.h"



#define STATE_INITIAL         0
#define STATE_SENDING         1
#define STATE_OFFER_RECEIVED  2
#define STATE_CONFIG_RECEIVED 3

#define uip_ipaddr_to_quad(a) (a)[0],(a)[1],(a)[2],(a)[3]

dhcpcState_t *dhcpcState;


#define CLOCK_SECOND		( 100*200  )

//#define UIP_CONF_DHCP_LIGHT//урезанная версия DHCP

#define BOOTP_BROADCAST 0x8000
#define BOOTP_UNICAST 0x0000


#define DHCP_REQUEST        1
#define DHCP_REPLY          2
#define DHCP_HTYPE_ETHERNET 1
#define DHCP_HLEN_ETHERNET  6
#define DHCP_MSG_LEN      236

#define DHCPDISCOVER  1
#define DHCPOFFER     2
#define DHCPREQUEST   3
#define DHCPDECLINE   4
#define DHCPACK       5
#define DHCPNAK       6
#define DHCPRELEASE   7

#define DHCP_OPTION_SUBNET_MASK  1
#define DHCP_OPTION_ROUTER       3
#define DHCP_OPTION_DNS_SERVER   6
#define DHCP_OPTION_NTP_SERVER   42
#define DHCP_OPTION_REQ_IPADDR   50
#define DHCP_OPTION_LEASE_TIME   51
#define DHCP_OPTION_MSG_TYPE     53
#define DHCP_OPTION_SERVER_ID    54
#define DHCP_OPTION_REQ_LIST     55
#define DHCP_OPTION_CLIENT_ID    61
#define DHCP_OPTION_HOST_NAME    12
#define DHCP_OPTION_OPT60	     60
#define DHCP_OPTION_END          255

const char dhcp_vendor[12]={'F','o','r','t','-','T','e','l','e','c','o','m'};

extern uint8_t dev_addr[6];
extern uint16_t IDENTIFICATION;
extern uint8_t MyIP[4];
//extern u16_t ipaddr[2],netmask[2],gateway[2],dnsserver[2];


struct uip_udp_conn *conn;

uint32_t xid;
uint32_t Now;
static const uint8_t magic_cookie[4] = {99, 130, 83, 99};

static u8 dhcp_request_repetition=0;

#define MAX_REQUEST 20


void dhcp_client_config(void){
	xid=(dev_addr[3]<<24) | (dev_addr[2]<<16) | (dev_addr[1]<<8) | dev_addr[0];
	xid+=IDENTIFICATION+xTaskGetTickCount();
	IDENTIFICATION++;
}


/*---------------------------------------------------------------------------*/
static uint8_t *add_msg_type(uint8_t *optptr, uint8_t type)
{
  *optptr++ = DHCP_OPTION_MSG_TYPE;
  *optptr++ = 1;
  *optptr++ = type;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_client_id(uint8_t *optptr, uint8_t id)
{
  *optptr++ = DHCP_OPTION_CLIENT_ID;
  *optptr++ = 6;
  memcpy(optptr, dev_addr, 6);
  return optptr + 6;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_server_id(uint8_t *optptr)
{
  *optptr++ = DHCP_OPTION_SERVER_ID;
  *optptr++ = 4;
  memcpy(optptr, dhcpcState->serverid, 4);
  return optptr + 4;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_vendor_class_id(uint8_t *optptr)
{
  *optptr++ = DHCP_OPTION_OPT60;
  *optptr++ = 12;
  memcpy(optptr, dhcp_vendor, 12);
  return optptr + 12;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_req_ipaddr(uint8_t *optptr)
{
  *optptr++ = DHCP_OPTION_REQ_IPADDR;
  *optptr++ = 4;
  memcpy(optptr, dhcpcState->ipaddr, 4);
  return optptr + 4;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_req_options(uint8_t *optptr)
{
  *optptr++ = DHCP_OPTION_REQ_LIST;
  *optptr++ = 4;
  *optptr++ = DHCP_OPTION_SUBNET_MASK;
  *optptr++ = DHCP_OPTION_ROUTER;
  *optptr++ = DHCP_OPTION_DNS_SERVER;
  *optptr++ = DHCP_OPTION_NTP_SERVER;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_end(uint8_t *optptr)
{
  *optptr++ = DHCP_OPTION_END;
  return optptr;
}
/*---------------------------------------------------------------------------*/
void create_msg(struct dhcp_msg *m){
//u8 IP[4]={0,0,0,0};
  m->op = DHCP_REQUEST;
  m->htype = DHCP_HTYPE_ETHERNET;
  m->hlen = 6;
  m->hops = 0;
  memcpy(m->xid, &xid, sizeof(m->xid));
  m->secs = 0;
  m->flags = UIP_HTONS(BOOTP_BROADCAST); /*  Broadcast bit. */

  if((uip_hostaddr[0]!=0)&&(uip_hostaddr[1]!=255)&&(uip_hostaddr[1]!=0)&&(uip_hostaddr[1]!=255))
	  memcpy (m->ciaddr, &uip_hostaddr, sizeof (m->ciaddr));
  else
	  memset (m->ciaddr, 0, sizeof (m->ciaddr));

  memset(m->yiaddr, 0, sizeof(m->yiaddr));
  memset(m->siaddr, 0, sizeof(m->siaddr));
  memset(m->giaddr, 0, sizeof(m->giaddr));
  memcpy(m->chaddr, dhcpcState->mac_addr, dhcpcState->mac_len);
  memset(&m->chaddr[dhcpcState->mac_len], 0, sizeof(m->chaddr) - dhcpcState->mac_len);
#ifndef UIP_CONF_DHCP_LIGHT
  memset(m->sname, 0, sizeof(m->sname));
  memset(m->file, 0, sizeof(m->file));
#endif
  memcpy(m->options, magic_cookie, sizeof(magic_cookie));
}
/*---------------------------------------------------------------------------*/
void send_discover(void)
{
  uint8_t *end;
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;
  create_msg(m);
  end = add_msg_type(&m->options[4], DHCPDISCOVER);
  end = add_client_id(end, 7);
  end = add_vendor_class_id(end);
  end = add_req_options(end);
  end = add_end(end);
  uip_send(uip_appdata, (int)(end - (uint8_t *)uip_appdata));
}

/*---------------------------------------------------------------------------*/
void send_request(void)
{
  uint8_t *end;
  struct dhcp_msg *m=(struct dhcp_msg *)uip_appdata;
  create_msg(m);

  end = add_msg_type(&m->options[4], DHCPREQUEST);
  end = add_server_id(end);
  end = add_vendor_class_id(end);
  end = add_req_ipaddr(end);
  end = add_end(end);
  
  uip_send(uip_appdata, (int)(end - (uint8_t *)uip_appdata));
}
/*---------------------------------------------------------------------------*/
static uint8_t parse_options(uint8_t *optptr, int len)
{
  uint8_t *end = optptr + len;
  uint8_t type = 0;
  DEBUG_MSG(DHCP_DEBUG,"start parse_options\r\n");
  while(optptr < end) {
    switch(*optptr) {
    case DHCP_OPTION_SUBNET_MASK:
      memcpy(dhcpcState->netmask, optptr + 2, 4);
      DEBUG_MSG(DHCP_DEBUG,"optptr DHCP_OPTION_SUBNET_MASK\r\n");
      break;
    case DHCP_OPTION_ROUTER:
      memcpy(dhcpcState->default_router, optptr + 2, 4);
      DEBUG_MSG(DHCP_DEBUG,"optptr DHCP_OPTION_ROUTER\r\n");
      break;
    case DHCP_OPTION_DNS_SERVER:
      memcpy(dhcpcState->dnsaddr, optptr + 2, 4);
      DEBUG_MSG(DHCP_DEBUG,"optptr DHCP_OPTION_DNS_SERVER\r\n");
      break;
    case DHCP_OPTION_NTP_SERVER:
      memcpy(dhcpcState->sntpaddr, optptr + 2, 4);
      DEBUG_MSG(DHCP_DEBUG,"optptr DHCP_OPTION_NTP_SERVER\r\n");
      break;
    case DHCP_OPTION_MSG_TYPE:
      type = *(optptr + 2);
      DEBUG_MSG(DHCP_DEBUG,"optptr DHCP_OPTION_MSG_TYPE\r\n");
      break;
    case DHCP_OPTION_SERVER_ID:
      memcpy(dhcpcState->serverid, optptr + 2, 4);
      DEBUG_MSG(DHCP_DEBUG,"optptr DHCP_OPTION_SERVER_ID\r\n");
      break;
    case DHCP_OPTION_LEASE_TIME:
      memcpy(dhcpcState->lease_time, optptr + 2, 4);
      DEBUG_MSG(DHCP_DEBUG,"optptr DHCP_OPTION_LEASE_TIME\r\n");
      break;
    case DHCP_OPTION_END:
      DEBUG_MSG(DHCP_DEBUG,"optptr DHCP_OPTION_END\r\n");
      return type;
    }

    optptr += optptr[1] + 2;
  }
  return type;
}
/*---------------------------------------------------------------------------*/
static uint8_t parse_msg(void)
{
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;
  DEBUG_MSG(DHCP_DEBUG,"start parse_msg\r\n");
  if(m->op == DHCP_REPLY &&
     memcmp(m->xid, &xid, sizeof(xid)) == 0 &&
     memcmp(m->chaddr, dhcpcState->mac_addr, dhcpcState->mac_len) == 0) {
    memcpy(dhcpcState->ipaddr, m->yiaddr, 4);
    return parse_options(&m->options[4], uip_datalen());
  }
  return 0;
}

/*---------------------------------------------------------------------------*/
PT_THREAD(handle_dhcp(void)) {
static uip_ipaddr_t temp_ip = {0,0};
static uip_ipaddr_t temp_mask = {0,0};
static uip_ipaddr_t temp_dr = {0,0};
static uip_ipaddr_t temp_lease_time = {0,0};
static uip_ipaddr_t temp_dns = {0,0};
static uip_ipaddr_t temp_sntp = {0,0};

      PT_BEGIN(&dhcpcState->pt);
      DEBUG_MSG(DHCP_DEBUG,"start handle_dhcp. s->state=%d\r\n",dhcpcState->state);
      dhcpcState->state = STATE_SENDING;
      dhcpcState->ticks = CLOCK_SECOND;

      do{
		  do {
			send_discover();
			DEBUG_MSG(DHCP_DEBUG,"send_discover\r\n");
			timer_set(&dhcpcState->timer, dhcpcState->ticks);
			DEBUG_MSG(DHCP_DEBUG,"send_discover timer\r\n");
			PT_WAIT_UNTIL(&dhcpcState->pt, uip_newdata() || timer_expired(&dhcpcState->timer));
			if(timer_expired(&dhcpcState->timer)){
				dhcp_request_repetition++;
				DEBUG_MSG(DHCP_DEBUG,"timer_expired\r\n");
				uip_udp_remove(conn);
				dhcpc_init(dev_addr,6);
				dhcp_client_config();
				dhcpc_request();
				//если не пришло не одного ответа то через MAX_REQUEST попыток
				//делаем вывод, что в сети нет dhcp сервера
				if(dhcp_request_repetition>MAX_REQUEST){
					dhcpcState->ipaddr [0] = dhcpcState->ipaddr [1] = 0;
					dhcp_request_repetition=0;
					goto dhcpcf;
				}
				return 1;
			}
			dhcp_request_repetition=0;
			DEBUG_MSG(DHCP_DEBUG,"send_discover TIMER\r\n");

			if(uip_newdata() && (parse_msg() == DHCPOFFER)) {
			  uip_flags &= ~UIP_NEWDATA;
			  dhcpcState->state = STATE_OFFER_RECEIVED;
			  DEBUG_MSG(DHCP_DEBUG,"DHCPOFFER + IP %d.%d..\r\n",(u8)(dhcpcState->ipaddr[0]),(u8)(dhcpcState->ipaddr[0]>>8));
			  if((dhcpcState->ipaddr[0]!=0)&&(dhcpcState->ipaddr[0]!=255)&&
			  		  (dhcpcState->ipaddr[1]!=0)&&(dhcpcState->ipaddr[1]!=255)){
				  uip_ipaddr_copy(&temp_ip,&dhcpcState->ipaddr);
				  uip_ipaddr_copy(&temp_mask,&dhcpcState->netmask);
				  uip_ipaddr_copy(&temp_dr,&dhcpcState->default_router);
				  uip_ipaddr_copy(&temp_dns,&dhcpcState->dnsaddr);
				  uip_ipaddr_copy(&temp_sntp,&dhcpcState->sntpaddr);
				  uip_ipaddr_copy(&temp_lease_time,&dhcpcState->lease_time);//да это не ip)) но по размерности подходит
			  }
			  break;
			}

			uip_flags &= ~UIP_NEWDATA;

			//если в течении 60 секунд не получили адрес, грузим дефолтный
			if(dhcpcState->ticks < CLOCK_SECOND * 60) {
				dhcpcState->ticks *= 2;
				//dhcpcState->ticks ++;;
			}
			else
			{
				dhcpcState->ipaddr [0] = dhcpcState->ipaddr [1] = 0;
			  goto dhcpcf;
			}

		  } while(dhcpcState->state != STATE_OFFER_RECEIVED);

		  dhcpcState->ticks = CLOCK_SECOND;

		  do {
			send_request();
			DEBUG_MSG(DHCP_DEBUG,"send request\r\n");
			timer_set(&dhcpcState->timer, dhcpcState->ticks);

			PT_WAIT_UNTIL(&dhcpcState->pt, uip_newdata() || timer_expired(&dhcpcState->timer));

			//если получили ack применяем настройки
			if(uip_newdata() && parse_msg() == DHCPACK) {
			  DEBUG_MSG(DHCP_DEBUG,"ACK\r\n");
			  uip_flags &= ~UIP_NEWDATA;
			  dhcpcState->state = STATE_CONFIG_RECEIVED;
			  break;
			}

			// NAK // применяем старые настройки
			if(uip_newdata() && parse_msg() == DHCPNAK) {
			  DEBUG_MSG(DHCP_DEBUG,"NAK\r\n");
			  uip_flags &= ~UIP_NEWDATA;
			  //если получили адрес
			  if((temp_ip[0]!=0)&&(temp_ip[1]!=0)){
				  dhcpcState->state = STATE_CONFIG_RECEIVED;
				  uip_ipaddr_copy(&dhcpcState->ipaddr,&temp_ip);
				  uip_ipaddr_copy(&dhcpcState->netmask,&temp_mask);
				  uip_ipaddr_copy(&dhcpcState->default_router,&temp_dr);
				  uip_ipaddr_copy(&dhcpcState->dnsaddr,&temp_dns);
				  uip_ipaddr_copy(&dhcpcState->sntpaddr,&temp_sntp);
				  uip_ipaddr_copy(&dhcpcState->lease_time,&temp_lease_time);
			  }else
				  dhcpcState->state = STATE_INITIAL;
			  break;
			}

			uip_flags &= ~UIP_NEWDATA;

			if(dhcpcState->ticks <= CLOCK_SECOND * 10) {
				dhcpcState->ticks += CLOCK_SECOND;
			} else {
			  PT_RESTART(&dhcpcState->pt);
			}
		  } while(dhcpcState->state != STATE_CONFIG_RECEIVED);

		#if 1
		  DEBUG_MSG(DHCP_DEBUG,"Got IP address %d.%d.%d.%d\r\n",
			 uip_ipaddr1(dhcpcState->ipaddr), uip_ipaddr2(dhcpcState->ipaddr),
			 uip_ipaddr3(dhcpcState->ipaddr), uip_ipaddr4(dhcpcState->ipaddr));
		  DEBUG_MSG(DHCP_DEBUG,"Got netmask %d.%d.%d.%d\r\n",
			 uip_ipaddr1(dhcpcState->netmask), uip_ipaddr2(dhcpcState->netmask),
			 uip_ipaddr3(dhcpcState->netmask), uip_ipaddr4(dhcpcState->netmask));
		  DEBUG_MSG(DHCP_DEBUG,"Got DNS server %d.%d.%d.%d\r\n",
			 uip_ipaddr1(dhcpcState->dnsaddr), uip_ipaddr2(dhcpcState->dnsaddr),
			 uip_ipaddr3(dhcpcState->dnsaddr), uip_ipaddr4(dhcpcState->dnsaddr));

		  DEBUG_MSG(DHCP_DEBUG,"Got default router %d.%d.%d.%d\r\n",
			 uip_ipaddr1(dhcpcState->default_router), uip_ipaddr2(dhcpcState->default_router),
			 uip_ipaddr3(dhcpcState->default_router), uip_ipaddr4(dhcpcState->default_router));

		  DEBUG_MSG(DHCP_DEBUG,"Got SNTP server %d.%d.%d.%d\r\n",
			 uip_ipaddr1(dhcpcState->sntpaddr), uip_ipaddr2(dhcpcState->sntpaddr),
			 uip_ipaddr3(dhcpcState->sntpaddr), uip_ipaddr4(dhcpcState->sntpaddr));


		  DEBUG_MSG(DHCP_DEBUG,"Lease expires in %ld seconds\r\n",
			 ntohs(dhcpcState->lease_time[0])*65536ul + ntohs(dhcpcState->lease_time[1]));
		  Now=RTC_GetCounter();
		#endif

	dhcpcf:
			dhcpc_configured (dhcpcState);
		   /*
		   * PT_END restarts the thread so we do this instead. Eventually we
		   * should reacquire expired leases here.
		   */
		  DEBUG_MSG(DHCP_DEBUG,"end\r\n");


		  //ждем пока закончится время и начнем сначала
		  PT_WAIT_UNTIL(&dhcpcState->pt, /*get_lease_time()==0*/0);

		  DEBUG_MSG(DHCP_DEBUG,"re init\r\n");
		  /*re init*/
	      dhcp_client_config();//new xid
	      dhcpc_init(dev_addr,6);

      }while(get_dhcp_mode()==DHCP_CLIENT);

	  uip_udp_remove (dhcpcState->conn);
      PT_END(&dhcpcState->pt);
}

void dhcpc_init(const void *mac_addr, int mac_len)
{
  uip_ipaddr_t addr;
  uip_ipaddr (addr, 255,255,255,255);

  if (dhcpcState && dhcpcState->conn){
    uip_udp_remove (dhcpcState->conn);
    DEBUG_MSG(DHCP_DEBUG,"remove udp conn dhcp\r\n");
  }
  if ((conn = uip_udp_new (&addr, HTONS (DHCPC_SERVER_PORT))))
  {
	  dhcpcState = (dhcpcState_t *) &conn->appstate;
	  dhcpcState->conn = conn;
	  dhcpcState->mac_addr = mac_addr;
	  dhcpcState->mac_len  = mac_len;
	  dhcpcState->state = STATE_INITIAL;
      uip_udp_bind (dhcpcState->conn, HTONS (DHCPC_CLIENT_PORT));
  }
  PT_INIT(&dhcpcState->pt);

}

/*---------------------------------------------------------------------------*/
void dhcpc_appcall(void)
{
//static u8 first_start=0;
   //if((uip_newdata())||(first_start==0)){
	//DEBUG_MSG(DHCP_DEBUG,"dhcpс appcall");
	   handle_dhcp();
	 //  first_start=1;
   //}
}
/*---------------------------------------------------------------------------*/
void dhcpc_request(void)
{
  u16_t ipaddr[2];

  if(dhcpcState->state == STATE_INITIAL) {
	uip_ipaddr(ipaddr, 0,0,0,0);
	uip_sethostaddr(ipaddr);
  }
}


void dhcpc_configured (const dhcpcState_t *s)
{
static u8 init=0;
extern uip_ipaddr_t uip_sntpaddr;

  //SS=0;

  if (!s->ipaddr [0] && !s->ipaddr [1])
  {
		printf("DHCP server not found\r\n");
		ntwk_setip();
		uip_ipaddr(uip_hostaddr,192,168,0,1);
		memcpy (MyIP, uip_hostaddr, 4);
  }
  else
  {
		uip_sethostaddr (s->ipaddr);
		memcpy (MyIP, uip_hostaddr, 4);
		uip_setnetmask (s->netmask);
		uip_setdraddr (s->default_router);
		uip_setdnsaddr(s->dnsaddr);
		uip_ipaddr_copy(uip_sntpaddr,s->sntpaddr);
		//uip_settimeoffset (&s->timeoffset);
  }

	if(init == 0){
		init=1;

    	httpd_init();
    	command_init();
    	snmp_trap_init();
    	syslog_init();
    	dns_resolv_init();//dns
       	SNTP_config();//старт задачи NTP
    	psw_search_init();//поиск устройств
    	smtp_config();//email port config
    	e_mail_init();//email task create
    	mib_init();
    	snmpd_init();
    	telnetd_init();
#if TFTP_SERVER
    	tftps_init();
#endif
		DEBUG_MSG(DHCP_DEBUG,"configuration compleat\r\n");
	}
}

/*возвращает время, оставшееся до следующего запроса адреса*/
u32 get_lease_time(void){
u32 ret;
	if(Now){
		if((ntohs(dhcpcState->lease_time[0])*65536ul + ntohs(dhcpcState->lease_time[1])) > (RTC_GetCounter()-Now)){
			ret=(ntohs(dhcpcState->lease_time[0])*65536ul + ntohs(dhcpcState->lease_time[1]))-(RTC_GetCounter()-Now);
				if(ret)
					return ret;
				else
					return 0;
		}
		else
			return 0;
	}
	else
		return 0;
}
