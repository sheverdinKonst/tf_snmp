#ifndef DHCP_H_
#define DHCP_H_

#define DHCP_DISABLED 	0
#define DHCP_CLIENT 	1
#define DHCP_RELAY		2
#define DHCP_SERVER		3


struct dhcp_msg {
  uint8_t op, htype, hlen, hops;
  uint8_t xid[4];
  uint16_t secs, flags;
  uint8_t ciaddr[4];
  uint8_t yiaddr[4];
  uint8_t siaddr[4];
  uint8_t giaddr[4];
  uint8_t chaddr[16];
#ifndef UIP_CONF_DHCP_LIGHT
  uint8_t sname[64];
  uint8_t file[128];
#endif
  uint8_t options[312];
};




//struct dhcp_cfg_t dhcp_cfg;

u8 get_dhcp_cfg(void);
u8 set_dhcp_cfg(void);


//u8 get_dhcp_mode(void);
void dhcp_appcall(void);
void set_dhcp_default(void);


#endif /* DHCP_H_ */
