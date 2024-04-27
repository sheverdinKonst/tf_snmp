#ifndef DHCPS_H_
#define DHCPS_H_

#define MAX_HLEN 6

struct dhcps_client_lease
{
  uint8_t chaddr[MAX_HLEN];
  uip_ipaddr_t ipaddr;
  uint32_t lease_end;
  uint8_t flags;
};

struct dhcps_config
{
  uint32_t default_lease_time;
  uip_ipaddr_t netmask;
  uip_ipaddr_t dnsaddr;
  uip_ipaddr_t default_router;
  struct dhcps_client_lease leases[3];
  uint8_t flags;
  uint8_t num_leases;
};

struct dhcps_config config;



uip_ipaddr_t dhcpr_server;

#define DHCP_CONF_NETMASK 0x01
#define DHCP_CONF_DNSADDR 0x02
#define DHCP_CONF_DEFAULT_ROUTER 0x04

#define DHCP_INIT_LEASE(addr0, addr1, addr2, addr3) \
{{0},{addr0, addr1, addr2, addr3},0,0}

#define clock_seconds() RTC_GetCounter()

#define get_dhcpr_server(addr) uip_ipaddr_copy((addr), dhcpr_server)

void dhcps_appcall(void);
void dhcps_init(void);
u8 get_dhcpr_state(void);
u8 get_dhcpr_hops(void);
u8 get_dhcpr_opt(void);
void set_dhcpr_state(u8 val);
void set_dhcpr_server(u8 *addr);
void set_dhcpr_opt(u8 opt);
void set_dhcpr_hops(u8 hops);
//u8 find_port_mac(u8 *mac);



#endif /* DHCPS_H_ */
