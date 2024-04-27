/*
 * lldp.c
 *
 *  Created on: 5 окт. 2018 г.
 *      Author: BelyaevAlex
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "stm32f10x_lib.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "../deffines.h"
#include "stm32f4x7_eth.h"
#include "../inc/SMIApi.h"
#include "../inc/Salsa2Regs.h"
#include "../inc/h/driver/gtDrvSwRegs.h"
#include "../uip/uip.h"
#include "../uip/uip_arp.h"
#include "debug.h"
#include "../net/stp/stp_oslayer_freertos.h"
#include "lldp.h"
#include "lldp_node.h"
#include "../net/uip/timer.h"
#include "settings.h"


struct timer lldp_tim;
struct timer lldp_ti_timer;//lldp transmit interval timer
uint8_t lldp_etheraddr[6] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e };


void lldp_init(void){
	ETH_MACAddressConfig(ETH_MAC_Address2, (uint8_t *)&lldp_etheraddr[0]);
	ETH_MACAddressPerfectFilterCmd(ETH_MAC_Address2, ENABLE);
	ETH_MACAddressFilterConfig(ETH_MAC_Address2, ETH_MAC_AddressFilter_DA);


	timer_set(&lldp_tim, 1000 * MSEC  );
	timer_set(&lldp_ti_timer,  1000* MSEC * get_lldp_transmit_interval());
}

void lldp_timer_processing(void){
	if(timer_expired(&lldp_tim)){
		lldp_update_timer();
		timer_set(&lldp_tim, 1000 * MSEC);
	}
	if(timer_expired(&lldp_ti_timer)){
		lldp_output();
		timer_set(&lldp_ti_timer,  1000* MSEC * get_lldp_transmit_interval());
	}
}


