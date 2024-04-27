

#include "../uip/uip.h"
#include "icmp.h"

//extern uip_ipaddr_t uip_hostaddr, uip_draddr, uip_netmask,uip_dest_ping_addr;
u8 icmp_need_send_;


void ping_init(void){
	ICMPBUF->type = ICMP_ECHO;
}


u8 icmp_need_send(void){
	return icmp_need_send_;
}

void set_icmp_need_send(u8 mode){
	icmp_need_send_ = mode;
}
