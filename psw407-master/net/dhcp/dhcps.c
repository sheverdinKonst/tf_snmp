#include <stdio.h>
#include <string.h>
#include "stm32f4x7_eth.h"
#include "stm32f4xx.h"
#include "stm32f4xx_rtc.h"
#include "../deffines.h"
#include "../uip/uip.h"
#include "../sntp/sntp.h"
#include "../smtp/smtp.h"
#include "../syslog/msg_build.h"
//#include "fifo.h"
//#include "usb.h"
#include "task.h"
#include "../dhcp/dhcpr.h"
#include "../dhcp/dhcpc.h"
#include "../dhcp/dhcps.h"

#include "../smtp/smtp.h"

//#include "CommandProcessing.h"
//#include "inc/policy.h"
#include "../inc/SMIApi.h"
#include "debug.h"

#define BOOTP_BROADCAST 0x8000

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
#define DHCPINFORM    8

#define DHCP_OPTION_SUBNET_MASK   1
#define DHCP_OPTION_ROUTER        3
#define DHCP_OPTION_DNS_SERVER    6
#define DHCP_OPTION_REQ_IPADDR   50
#define DHCP_OPTION_LEASE_TIME   51
#define DHCP_OPTION_MSG_TYPE     53
#define DHCP_OPTION_SERVER_ID    54
#define DHCP_OPTION_REQ_LIST     55
#define DHCP_OPTION_END         255



#define LEASE_FLAGS_ALLOCATED 0x01	/* Lease with an allocated address*/
#define LEASE_FLAGS_VALID 0x02		/* Contains a valid but
					   possibly outdated lease */



//struct uip_udp_conn *conn;
//static dhcpcState_t *dhcpcState;



#if 0

//extern uint8_t dev_addr[6];
//extern struct uip_udp_conn *uip_udp_conn;

u8 client_mac[6];


static struct uip_udp_conn *conn;
static struct uip_udp_conn *send_conn;
static struct dhcps_client_lease *lease;

static uip_ipaddr_t any_addr;
static uip_ipaddr_t bcast_addr;

static uint8_t *find_option(uint8_t option)
{
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;
  uint8_t *optptr = &m->options[4];
  uint8_t *end = (uint8_t*)uip_appdata + uip_datalen();
  while(optptr < end && *optptr != DHCP_OPTION_END) {
    if(*optptr == option) {
      return optptr;
    }
    optptr += optptr[1] + 2;
  }
  return NULL;
}

static const uint8_t magic_cookie[4] = {99, 130, 83, 99};

static int check_cookie(void)
{
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;
  return memcmp(m->options, magic_cookie, 4) == 0;
}


/* Finds any valid lease for a given MAC address */
static struct dhcps_client_lease *lookup_lease_mac(const uint8_t *chaddr, uint8_t hlen)
{
  struct dhcps_client_lease *lease = (struct dhcps_client_lease *)&config.leases;
  struct dhcps_client_lease *end = (struct dhcps_client_lease *)&config.leases + config.num_leases;
  DEBUG_MSG(DHCP_DEBUG,"lookup_lease_mac\r\n");
  while(lease != end) {
    if (lease->flags & LEASE_FLAGS_VALID
	&& memcmp(lease->chaddr, chaddr, hlen) == 0) {
      return lease;
    }
    lease++;
  }
  return NULL;
}


static struct dhcps_client_lease *lookup_lease_ip(const uip_ipaddr_t *ip)
{
  struct dhcps_client_lease *lease = (struct dhcps_client_lease *)&config.leases;
  struct dhcps_client_lease *end = (struct dhcps_client_lease *)&config.leases + config.num_leases;
  DEBUG_MSG(DHCP_DEBUG,"lookup_lease_ip\r\n");
  while(lease != end) {
    if (uip_ipaddr_cmp(&lease->ipaddr, ip)) {
      return lease;
    }
    lease++;
  }
  return NULL;
}

static struct dhcps_client_lease *find_free_lease(void)
{
  struct dhcps_client_lease *found = NULL;
  struct dhcps_client_lease *lease = (struct dhcps_client_lease *)&config.leases;
  struct dhcps_client_lease *end = (struct dhcps_client_lease *)&config.leases + config.num_leases;
  DEBUG_MSG(DHCP_DEBUG,"find_free_lease\r\n");
  while(lease != end) {
    if (!(lease->flags & LEASE_FLAGS_VALID)) return lease;
    if (!(lease->flags & LEASE_FLAGS_ALLOCATED)) found = lease;
    lease++;
  }
  return found;
}

struct dhcps_client_lease *init_lease(struct dhcps_client_lease *lease,
	   const uint8_t *chaddr, uint8_t hlen)
{
  DEBUG_MSG(DHCP_DEBUG,"init_lease\r\n");
  if (lease) {
    memcpy(lease->chaddr, chaddr, hlen);
    lease->flags = LEASE_FLAGS_VALID;
  }
  return lease;
}


static struct dhcps_client_lease *choose_address(void)
{
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;
  struct dhcps_client_lease *lease;
  DEBUG_MSG(DHCP_DEBUG,"choose_address\r\n");
  lease = lookup_lease_mac(m->chaddr, m->hlen);
  if (lease) {
    return lease;
  }
  {
    uint8_t *opt;
    opt = find_option(DHCP_OPTION_REQ_IPADDR);
    if (opt && (lease = lookup_lease_ip((uip_ipaddr_t *)&opt[2]))
	&& !(lease->flags & LEASE_FLAGS_ALLOCATED)) {
      return init_lease(lease, m->chaddr,m->hlen);
    }
  }
  lease = find_free_lease();
  if (lease) {
    return init_lease(lease, m->chaddr,m->hlen);
  }
  return NULL;
}

static struct dhcps_client_lease *allocate_address(void)
{
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;
  struct dhcps_client_lease *lease;
  DEBUG_MSG(DHCP_DEBUG,"allocate_address\r\n");
  lease = lookup_lease_mac(m->chaddr, m->hlen);
  if (!lease) {
    uint8_t *opt;
    opt = find_option(DHCP_OPTION_REQ_IPADDR);
    if (!(opt && (lease = lookup_lease_ip((uip_ipaddr_t *)&opt[2]))
	&& !(lease->flags & LEASE_FLAGS_ALLOCATED))) {
      return NULL;
    }
  }
  lease->lease_end = clock_seconds()+config.default_lease_time;
  lease->flags |= LEASE_FLAGS_ALLOCATED;
  return lease;
}

static struct dhcps_client_lease *release_address(void){
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;
  struct dhcps_client_lease *lease;
  DEBUG_MSG(DHCP_DEBUG,"release_address\r\n");
  lease = lookup_lease_mac(m->chaddr, m->hlen);
  if (!lease) {
    return NULL;
  }
  lease->flags &= ~LEASE_FLAGS_ALLOCATED;
  return lease;
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
static uint8_t *add_server_id(uint8_t *optptr)
{
  *optptr++ = DHCP_OPTION_SERVER_ID;
  *optptr++ = 4;
  memcpy(optptr, &uip_hostaddr, 4);
  return optptr + 4;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_lease_time(uint8_t *optptr)
{
  uint32_t lt;
  *optptr++ = DHCP_OPTION_LEASE_TIME;
  *optptr++ = 4;
  lt = UIP_HTONL(config.default_lease_time);
  memcpy(optptr, &lt, 4);
  return optptr + 4;
}

/*---------------------------------------------------------------------------*/
static uint8_t *add_end(uint8_t *optptr)
{
  *optptr++ = DHCP_OPTION_END;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_config(uint8_t *optptr)
{
  if (config.flags & DHCP_CONF_NETMASK) {
    *optptr++ = DHCP_OPTION_SUBNET_MASK;
    *optptr++ = 4;
    memcpy(optptr, config.netmask, 4);
    optptr += 4;
  }
  if (config.flags & DHCP_CONF_DNSADDR) {
    *optptr++ = DHCP_OPTION_DNS_SERVER;
    *optptr++ = 4;
    memcpy(optptr, config.dnsaddr, 4);
    optptr += 4;
  }
  if (config.flags & DHCP_CONF_DEFAULT_ROUTER) {
    *optptr++ = DHCP_OPTION_ROUTER;
    *optptr++ = 4;
    memcpy(optptr, config.default_router, 4);
    optptr += 4;
  }
  return optptr;
}
/*---------------------------------------------------------------------------*/
static void create_msg(struct dhcp_msg *m)
{
  m->op = DHCP_REPLY;
  /* m->htype = DHCP_HTYPE_ETHERNET; */
/*   m->hlen = DHCP_HLEN_ETHERNET; */
/*   memcpy(m->chaddr, &uip_ethaddr,DHCP_HLEN_ETHERNET); */
  m->hops = 0;
  m->secs = 0;
  memcpy(m->siaddr, &uip_hostaddr, 4);
  m->sname[0] = '\0';
  m->file[0] = '\0';
  memcpy(m->options, magic_cookie, sizeof(magic_cookie));
}
/*---------------------------------------------------------------------------*/

void send_offer(struct uip_udp_conn *conn, struct dhcps_client_lease *lease){
  uint8_t *end;
  //u16 len;
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;

  create_msg(m);
  memcpy(&m->yiaddr, &lease->ipaddr,4);

  end = add_msg_type(&m->options[4], DHCPOFFER);
  end = add_server_id(end);
  end = add_lease_time(end);
  end = add_config(end);
  end = add_end(end);
  uip_ipaddr_copy(&conn->ripaddr, &bcast_addr);

  uip_send(uip_appdata, (int)(end - (uint8_t *)uip_appdata));
}

static void send_ack(struct uip_udp_conn *conn, struct dhcps_client_lease *lease)
{
  uint8_t *end;
  //u16 len;
  uip_ipaddr_t ciaddr;
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;

  create_msg(m);
  memcpy(&m->yiaddr, &lease->ipaddr,4);

  end = add_msg_type(&m->options[4], DHCPACK);
  end = add_server_id(end);
  end = add_lease_time(end);
  end = add_config(end);
  end = add_end(end);
  memcpy(&ciaddr, &lease->ipaddr,4);
  uip_ipaddr_copy(&conn->ripaddr, &bcast_addr);
  uip_send(uip_appdata, (int)(end - (uint8_t *)uip_appdata));
  DEBUG_MSG(DHCP_DEBUG,"ACK\r\n");
}
void send_nack(struct uip_udp_conn *conn)
{
  uint8_t *end;
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;

  create_msg(m);
  memset(&m->yiaddr, 0, 4);

  end = add_msg_type(&m->options[4], DHCPNAK);
  end = add_server_id(end);
  end = add_end(end);

  uip_ipaddr_copy(&conn->ripaddr, &bcast_addr);

  uip_send(uip_appdata, (int)(end - (uint8_t *)uip_appdata));
  DEBUG_MSG(DHCP_DEBUG,"NACK\r\n");
}




void dhcps_appcall(void){
if(/*ev == tcpip_event*/1) {

	if (uip_newdata()) {

#ifdef DHCP_DEBUG
		DEBUG_MSG(DHCP_DEBUG,"DHCP 2 server \r\n");
#endif

		struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;
		//struct uip_udpip_hdr *header = (struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN];
		//copy clien t mac address
		for(u8 i=0;i<6;i++){
			client_mac[i]=uip_buf[6+i];
		}

		DEBUG_MSG(DHCP_DEBUG,"%x:%x:%x:%x:%x:%x\r\n",
				client_mac[0],client_mac[1],client_mac[2],client_mac[3],client_mac[4],client_mac[5]);

		DEBUG_MSG(DHCP_DEBUG,"find_port_mac port %d\r\n",find_port_mac(client_mac));

		if (m->op == DHCP_REQUEST && check_cookie() && m->hlen <= MAX_HLEN) {

			uint8_t *opt = find_option(DHCP_OPTION_MSG_TYPE);
			DEBUG_MSG(DHCP_DEBUG,"recieve request + check cookie\r\n");
			if (opt) {
				uint8_t mtype = opt[2];
				if (opt[2] == DHCPDISCOVER) {
					DEBUG_MSG(DHCP_DEBUG,"DHCP Discover\r\n");

					lease = choose_address();

					if (lease) {
						lease->lease_end = clock_seconds()+config.default_lease_time;
						//tcpip_poll_udp(send_conn);

						//process_post(&tcpip_process, UDP_POLL, send_conn);
						DEBUG_MSG(DHCP_DEBUG,"wait ...\r\n");
						//PROCESS_WAIT_EVENT_UNTIL(uip_poll());
						send_offer(conn,lease);
						DEBUG_MSG(DHCP_DEBUG,"send_offer\r\n");

					}
				 }
				else {
						uint8_t *opt = find_option(DHCP_OPTION_SERVER_ID);
						if (!opt || uip_ipaddr_cmp((uip_ipaddr_t*)&opt[2], &uip_hostaddr)) {
							if (mtype == DHCPREQUEST) {
								DEBUG_MSG(DHCP_DEBUG,"Request\r\n");
								lease = allocate_address();
								//tcpip_poll_udp(send_conn);
						 		//PROCESS_WAIT_EVENT_UNTIL(uip_poll());
								if (!lease) {
									send_nack(send_conn);
								} else {
									send_ack(send_conn,lease);
								}
							} else if (mtype == DHCPRELEASE) {
								DEBUG_MSG(DHCP_DEBUG,"Release\r\n");
								release_address();
							} else if (mtype ==  DHCPDECLINE) {
								DEBUG_MSG(DHCP_DEBUG,"Decline\r\n");
							} else if (mtype == DHCPINFORM) {
								DEBUG_MSG(DHCP_DEBUG,"Inform\r\n");
							}
						}
				}
			}
		}
	}
} else if (uip_poll()) {

}

}



void dhcps_init(void){
	uip_ipaddr_t router,mask;


	    config.default_lease_time=1;
	    uip_ipaddr(&router, 192,168,0,81);
	    config.default_router[0]=router[0];
	    config.default_router[1]=router[1];
	    config.dnsaddr[0]=router[0];
	    config.dnsaddr[1]=router[1];
	    uip_ipaddr(&mask, 255,255,255,0);
	    config.netmask[0]=mask[0];
	    config.netmask[1]=mask[1];
	    config.num_leases=2;
	    config.leases[0].flags=LEASE_FLAGS_VALID;
	    config.leases[1].flags=LEASE_FLAGS_VALID;



	  DEBUG_MSG(DHCP_DEBUG,"DHCP server starting\r\n");
	  uip_ipaddr(&any_addr, 0,0,0,0);
	  uip_ipaddr(&bcast_addr, 255,255,255,255);


	  if ((send_conn = uip_udp_new (&bcast_addr, HTONS (DHCPC_CLIENT_PORT))))
	  {
	      uip_udp_bind (send_conn, HTONS (DHCPC_SERVER_PORT));
	  }

	  if ((conn = uip_udp_new (&any_addr, HTONS (DHCPC_CLIENT_PORT))))
	  {
	      uip_udp_bind (conn, HTONS (DHCPC_SERVER_PORT));
	  }

}






#endif
