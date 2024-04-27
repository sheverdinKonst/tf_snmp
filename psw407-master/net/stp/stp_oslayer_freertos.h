#ifndef __STP_OSLAYER_FREERTOS_H__
#define __STP_OSLAYER_FREERTOS_H__

#include "../uip/uip.h"
#include "../uip/uip_arp.h"
#include "stm32f4x7_eth.h"
//#include "usb.h"
#include "bridgestp.h"
#include <stdio.h>
#include  "../deffines.h"

#define htonl HTONL
#define ntohl HTONL

#define ETHER_ADDR_LEN 6

struct ether_header {
  uint8_t  ether_dhost[ETHER_ADDR_LEN];
  uint8_t  ether_shost[ETHER_ADDR_LEN];
  uint32_t tag;
  uint16_t ether_type;
} __attribute__ ((packed, aligned (1)));


void sw_rstp_deconfig(void);

#if BRIDGESTP_DEBUG
extern unsigned long deb_ticks;
extern int deb_src;
extern int deb_tick;
#define DEB_PRINTF(fmt, arg...) printf("bstp %d (%lu/%lu): ", deb_src, bstp_getticks(), bstp_getdeltatick(deb_ticks)); \
                                deb_ticks = bstp_getticks(); \
                                printf("" fmt, ##arg); printf("\r")
#define DPRINTF(fmt, arg...)    {deb_tick = 0; DEB_PRINTF(fmt, ##arg);}
#else
#define DPRINTF(fmt, arg...) {}
#endif

#if STP_DEBUG
	#define PRINTF(fmt, arg...)	printf(fmt, ##arg)//xUsb_Print(fmt, ##arg)
#else
	#define PRINTF(fmt, arg...) {}
#endif

#endif
