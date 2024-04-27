#ifndef DHCPR_H_
#define DHCPR_H_

#include "../uip/uip.h"
#include "../dhcp/dhcp.h"
#define MAX_HLEN 6



uip_ipaddr_t dhcpr_server;

#define DHCP_CONF_NETMASK 0x01
#define DHCP_CONF_DNSADDR 0x02
#define DHCP_CONF_DEFAULT_ROUTER 0x04

//#define get_dhcpr_server(addr) uip_ipaddr_copy((addr), dhcp_cfg.serverIP/*dhcpr_server*/)

void dhcpr_appcall(void);
void dhcpr_init(void);

void set_dhcpr_state(u8 val);
void set_dhcpr_server(u8 *addr);
void set_dhcpr_opt(u8 opt);
void set_dhcpr_hops(u8 hops);
u8 find_port_mac(u8 *mac);
u16 find_port_vid(u8 port);
i8 send_relay_request(struct uip_udp_conn *connection,struct dhcp_msg *m);
i8 send_relay_reply(struct uip_udp_conn *connection,struct dhcp_msg *m);
void set_conn_kostyl(void);

#endif /* DHCPR_H_ */
