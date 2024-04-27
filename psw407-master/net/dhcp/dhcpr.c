#include <string.h>
#include "stm32f4x7_eth.h"
#include "stm32f4xx_rtc.h"
#include "board.h"
#include "settings.h"
#include "../deffines.h"
#include "../uip/uip.h"
#include "../sntp/sntp.h"
#include "../smtp/smtp.h"
#include "../syslog/msg_build.h"
//#include "fifo.h"
//#include "usb.h"
#include "task.h"
#include "../dhcp/dhcpr.h"
//#include "net/dhcp/dhcps.h"
#include "../net/dhcp/dhcpc.h"
#include "../net/dhcp/dhcp.h"
#include "../net/smtp/smtp.h"

//#include "CommandProcessing.h"
//#include "inc/policy.h"
#include "../inc/SMIApi.h"
#include "debug.h"

#if DHCPR
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
#define DHCP_OPTION_82			 82
#define DHCP_OPTION_END         255

#define LEASE_FLAGS_ALLOCATED 0x01	/* Lease with an allocated address*/
#define LEASE_FLAGS_VALID 0x02		/* Contains a valid but
					   possibly outdated lease */

extern uint8_t dev_addr[6];
extern u8 MyIP[4];
u8 client_mac[6];


static struct uip_udp_conn *conn;
static struct uip_udp_conn *send_conn;
//static struct uip_udp_conn *conn_res;

static uip_ipaddr_t any_addr;
static uip_ipaddr_t bcast_addr;
static uip_ipaddr_t serv_addr;

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
  return (void *)0/*NULL*/;
}

static const uint8_t magic_cookie[4] = {99, 130, 83, 99};

static int check_cookie(void)
{
  struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;
  return memcmp(m->options, magic_cookie, 4) == 0;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_end(uint8_t *optptr)
{
  *optptr++ = DHCP_OPTION_END;
  return optptr;
}
/*---------------------------------------------------------------------------*/
static uint8_t *add_opt_82(uint8_t *optptr, u8 port, u16 vid){
	*optptr++ = 82;
	*optptr++ = 18;//option len
	*optptr++ = 0x01;//
	//circuit ID
	*optptr++ = 0x06;//prefix
	*optptr++ = 0x00;//
	*optptr++ = 0x04;//len

	if(vid > 0xFF){
		*optptr++ = (u8)(vid>>8);
		*optptr++ = (u8)vid;
	}else{
		*optptr++ = 0x00;//vlah Hbyte
		*optptr++ = vid;//vlan Lbytу
	}

	*optptr++ = 0x00;
	*optptr++ = port;//switch port

	//Agent Remote ID = my mac
	*optptr++ = 0x02;//
	*optptr++ = 0x08;
	*optptr++ = 0x00;
	*optptr++ = 06;//dev addr len
    memcpy(optptr, dev_addr, 6);
    optptr += 6;
    return optptr;
}
/*---------------------------------------------------------------------------*/
/*static uint8_t *del_opt_82(uint8_t *optptr,u8 len){
u8 *next;
u8 *end;
    u8 k=(int)(find_option(DHCP_OPTION_END) - optptr);
	len = optptr[1]+2;
	//next = (uint8_t*)uip_appdata + optptr + len;
	//end = ;
	//for(u8 i=k;i<(uip_datalen()-len);i++){
		//uip_appdata[i]=uip_appdata[i+len];
	//}
	return optptr;
}
*/





void set_conn_kostyl(void){
	/*set conn to recieve*/

	if(conn){
		uip_ipaddr_copy(conn->ripaddr, &any_addr);
		conn->rport=HTONS(DHCPC_CLIENT_PORT);
		conn->lport=HTONS(DHCPC_SERVER_PORT);
	}

	if(send_conn){
		uip_ipaddr_copy(send_conn->ripaddr, &serv_addr);
		send_conn->rport=HTONS(DHCPC_SERVER_PORT);
		send_conn->lport=HTONS(DHCPC_SERVER_PORT);
	}

}

void dhcpr_appcall(void){
uint8_t *opt;
uint8_t mtype;

set_conn_kostyl();

	if (uip_newdata()) {

		struct dhcp_msg *m = (struct dhcp_msg *)uip_appdata;

		if (m->op == DHCP_REQUEST && check_cookie() && m->hlen <= MAX_HLEN) {

			opt = find_option(DHCP_OPTION_MSG_TYPE);
			DEBUG_MSG(DHCP_DEBUG,"Request\r\n");
			if (opt) {
				mtype = opt[2];
				switch(mtype){
					case DHCPDISCOVER: DEBUG_MSG(DHCP_DEBUG,"DHCP Discover\r\n");break;
					case DHCPREQUEST:  DEBUG_MSG(DHCP_DEBUG,"DHCP Request\r\n");break;
					case DHCPRELEASE:  DEBUG_MSG(DHCP_DEBUG,"DHCP Release\r\n");break;
					case DHCPDECLINE:  DEBUG_MSG(DHCP_DEBUG,"Decline\r\n"); break;
					case DHCPINFORM:   DEBUG_MSG(DHCP_DEBUG,"Inform\r\n"); break;
					default: return;
				}
				send_relay_request(conn,m);
			}
		}
		else if(m->op == DHCP_REPLY && check_cookie() && m->hlen <= MAX_HLEN){
			DEBUG_MSG(DHCP_DEBUG,"Reply\r\n");
			opt = find_option(DHCP_OPTION_MSG_TYPE);
			if (opt) {
				mtype = opt[2];
				switch(mtype){
					case DHCPOFFER: DEBUG_MSG(DHCP_DEBUG,"DHCP Offer\r\n"); break;
					case DHCPACK: DEBUG_MSG(DHCP_DEBUG,"DHCP ACK\r\n"); break;
					case DHCPNAK: DEBUG_MSG(DHCP_DEBUG,"DHCP NAK\r\n"); break;
					default: return;
				}
				send_relay_reply(send_conn,m);
			}
			else
				DEBUG_MSG(DHCP_DEBUG,"no opt 53\r\n");
		}
		uip_flags &= ~UIP_NEWDATA;
	}
}




void dhcpr_init(void){
u8 port;
uip_ipaddr_t ip;

	//если не сконфигурировано
	if(get_dhcp_mode() != DHCP_RELAY){
		for(u8 i=0;i<ALL_PORT_NUM;i++){
			port=i;
//			gprtSetPolicy(port,POLICY_TYPE_OPT82,FRAME_POLICY_NONE);
		}
		return;
	}

	 /*check if this hardware supported*/
	 if(get_marvell_id() != DEV_88E097){
		 DEBUG_MSG(DHCP_DEBUG,"DHCP option 82 no support");
		 return;
	 }else{
		 /*set switch to dhcp relay mode*/
//		 gsysSetCPUDest(CPU_PORT);
		 for(u8 i=0;i<ALL_PORT_NUM;i++){
			 port=i;
			 if(port!=CPU_PORT){
//				 gprtSetPolicy(port,POLICY_TYPE_OPT82,FRAME_POLICY_TRAP);
			 }
			 else{
//				 gprtSetPolicy(port,POLICY_TYPE_OPT82,FRAME_POLICY_NONE);
			 }

		 }
	 }


	  DEBUG_MSG(DHCP_DEBUG,"DHCP server starting\r\n");
	  uip_ipaddr(&any_addr, 0,0,0,0);
	  uip_ipaddr(&bcast_addr, 255,255,255,255);
	  get_dhcp_server_addr(&ip);
	  uip_ipaddr_copy(&serv_addr,&ip);


	  if ((send_conn = uip_udp_new (&serv_addr, HTONS (DHCPC_SERVER_PORT))))
	  {
	      uip_udp_bind (send_conn, HTONS (DHCPC_SERVER_PORT));
	  }

	  if ((conn = uip_udp_new (&any_addr, HTONS (DHCPC_CLIENT_PORT))))
	  {
	      uip_udp_bind (conn, HTONS (DHCPC_SERVER_PORT));
	  }
}

/*функция определяет с какого порта пришел пакет с данным mac*/
u8 find_port_mac(u8 *mac){
int start;
uint16_t port;
uint16_t vid;
u8 mac1[6];
u8 port_=0xff;
struct mac_entry_t entry;

start = 1;


	 while(read_atu(0, start, &entry)==0){
		 DEBUG_MSG(DHCP_DEBUG,"find mac %x:%x:%x:%x:%x:%x\r\n",entry.mac[1],entry.mac[0],entry.mac[3],entry.mac[2],entry.mac[5],entry.mac[4]);
		 if((entry.mac[0]==mac[1])&&(entry.mac[1]==mac[0])&&(entry.mac[2]==mac[3])&&(entry.mac[3]==mac[2])&&(entry.mac[4]==mac[5])&&(entry.mac[5]==mac[4])){
			 if(get_dev_type() == DEV_PSW2GPLUS || get_dev_type() == DEV_PSW2G4F ||get_dev_type() == DEV_PSW2G4FUPS){
				 if(entry.port_vect & 1)
					 port = 0;
				 if(entry.port_vect & 4)
					 port = 1;
				 if(entry.port_vect & 16)
					 port = 2;
				 if(entry.port_vect & 64)
					 port = 3;
				 if(entry.port_vect & 256)
					 port = 4;
				 if(entry.port_vect & 512)
					 port = 5;
				 //if(entry.port_vect & 1024)
				 //	 addstr("CPU ");
			}else if(get_dev_type() == DEV_PSW2G6F){
				 if(entry.port_vect & 1)
					 port = 0;
				 if(entry.port_vect & 4)
					 port = 1;
				 if(entry.port_vect & 16)
					 port = 2;
				 if(entry.port_vect & 32)
					 port = 3;
				 if(entry.port_vect & 64)
					 port = 4;
				 if(entry.port_vect & 128)
					 port = 5;
				 if(entry.port_vect & 256)
					 port = 6;
				 if(entry.port_vect & 512)
					 port = 7;

			 }
			 else if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
				 if(entry.port_vect & 1)
					 port = 0;
				 if(entry.port_vect & 2)
					 port = 1;
				 if(entry.port_vect & 4)
					 port = 2;
				 if(entry.port_vect & 8)
					 port = 3;
				 if(entry.port_vect & 16)
					 port = 4;
				 if(entry.port_vect & 32)
					 port = 5;

			 }
			 else if((get_dev_type() == DEV_PSW2G2FPLUS)||(get_dev_type() == DEV_PSW2G2FPLUSUPS)){
				 if(entry.port_vect & 16)
					 port = 0;
				 if(entry.port_vect & 64)
					 port = 1;
				 if(entry.port_vect & 256)
					 port = 2;
				 if(entry.port_vect & 512)
					 port = 3;
			 }
		 }
		 start = 0;
	 }
	 return port_;
}

/*функция определяет с какого vlan пришел пакет*/
u16 find_port_vid(u8 port){
	return 1;
}

//client to server: add option 82
i8 send_relay_request(struct uip_udp_conn *connection,struct dhcp_msg *m){
uint8_t *end;
u8 port=0;
u8 client_mac[6];
u16 rport,lport,vid;

	for(u8 i=0;i<6;i++){
		client_mac[i]=uip_buf[6+i];
	}

	port = find_port_mac(client_mac);
	if(port == 0xFF)
		return -1;

	vid=find_port_vid(port);
	if((vid == 0)||(vid >4094))
		return -1;

	DEBUG_MSG(DHCP_DEBUG,"find_port_mac port %d\r\n",port);

	DEBUG_MSG(DHCP_DEBUG,"send_relay_request\r\n");

	if((get_dhcpr_hops()>(m->hops)) || (get_dhcpr_hops()==0))
		m->hops++;
	else
		return -1;

	m->giaddr[0]=MyIP[0];
	m->giaddr[1]=MyIP[1];
	m->giaddr[2]=MyIP[2];
	m->giaddr[3]=MyIP[3];

	end = (uint8_t*)uip_appdata + uip_datalen();

	//если разрешено использование опции
	//если не найдена опция 82, то вставляем свою, если есть, то игнорируем и передаем как есть
	if(get_dhcpr_opt()==1){
		if(find_option(DHCP_OPTION_82)==NULL){
			end = add_opt_82((end-1), port,vid);
			end = add_end(end);
		}
	}
	uip_ipaddr_copy(&connection->ripaddr, &serv_addr);
	lport=connection->lport;
	rport=connection->rport;

	connection->lport=rport;
	connection->rport=lport;


	uip_send(uip_appdata, (int)(end - (uint8_t *)uip_appdata));

	DEBUG_MSG(DHCP_DEBUG,"sended len=%d\r\n",(int)(end - (uint8_t *)uip_appdata));
return 0;
}

//server to client: delete option 82
i8 send_relay_reply(struct uip_udp_conn *connection,struct dhcp_msg *m){
uint8_t *end;
//u8 opt82_len;
//u16 lport,rport;
DEBUG_MSG(DHCP_DEBUG,"send_relay_reply\r\n");

	if((get_dhcpr_hops()>(m->hops)) || (get_dhcpr_hops()==0))
		m->hops++;
	else
		return -1;

	/*remove relay agent ip*/
	m->giaddr[0]=0;
	m->giaddr[1]=0;
	m->giaddr[2]=0;
	m->giaddr[3]=0;

	//если опция 82 найдена, то удаляем ее в любом случае

	end=find_option(DHCP_OPTION_82);

	if(end != NULL){
		//end = del_opt_82(end,opt82_len);
		//end=find_option(DHCP_OPTION_END);
		end = add_end(end);
		DEBUG_MSG(DHCP_DEBUG,"opt82 val end %d\r\n",(int)(*end));
	}else{
		end = (uint8_t*)uip_appdata + uip_datalen();
	}


    //broadcast sa
	for(u8 i=0;i<6;i++)
		uip_buf[i]=0xFF;

	//broadcast ip
	uip_ipaddr_copy(&connection->ripaddr, &bcast_addr);

	connection->lport=HTONS(DHCPC_SERVER_PORT)/*rport*/;
	connection->rport=HTONS(DHCPC_CLIENT_PORT)/*lport*/;

	uip_send(uip_appdata, (int)(end - (uint8_t *)uip_appdata));
	DEBUG_MSG(DHCP_DEBUG,"send_relay_reply end len %d\r\n",(int)(end - (uint8_t *)uip_appdata));
	return 0;
}
#endif
