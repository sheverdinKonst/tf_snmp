/*
 * eapol.c
 *
 *  Created on: 28 февр. 2018 г.
 *      Author: BelyaevAlex
 */
#include "board.h"
#include "deffines.h"
#include "debug.h"
#include "uip/uip.h"
#include "uip/uip_arp.h"

void eapol_input(){
struct uip_eth_hdr *xHeader;
uint16_t type;
	DEBUG_MSG(DOT1X_DEBUG,"eapol_input=%d\r\n", uip_len);


	xHeader = (struct uip_eth_hdr *) &uip_buf[ 0 ];
	type = xHeader->type;
}
