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
#include "dhcp.h"
#include "dhcpc.h"
#include "board.h"
#include "names.h"
#include "settings.h"


u8 get_dhcp_cfg(void){
	if((get_dhcp_mode()>DHCP_RELAY) || ((get_dhcp_mode() == 255))){
		set_dhcp_default();
		return 1;
	}
	return 0;
}

u8 set_dhcp_cfg(void){
	if(get_dhcp_mode()>DHCP_RELAY){
		set_dhcp_default();
		return 1;
	}else{
		return 0;
	}
}

void set_dhcp_default(void){
	uip_ipaddr_t ip;
	uip_ipaddr(ip,0,0,0,0);
	set_dhcp_mode(0);
	set_dhcp_hops(4);
	set_dhcp_server_addr(ip);
	set_dhcp_opt82(1);
	set_dhcp_cfg();
}



void dhcp_appcall(void)
{
	switch(get_dhcp_mode()){
		case DHCP_DISABLED:
			break;
		case DHCP_CLIENT:
			if (uip_udp_conn->lport==HTONS(DHCPS_CLIENT_PORT))
				dhcpc_appcall();
			break;
		/*case DHCP_SERVER:
			if (uip_udp_conn->lport==HTONS(DHCPS_SERVER_PORT))
				dhcps_appcall();
			break
		*/
#if DHCPR
		case DHCP_RELAY:
			dhcpr_appcall();
			break;
#endif
	}
}
