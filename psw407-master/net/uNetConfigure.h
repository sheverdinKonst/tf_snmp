#ifndef __UNETCONFIGURE_H__
#define __UNETCONFIGURE_H__

#include <stdio.h>
#include "stm32f4x7_eth.h"

#include "../deffines.h"
       
#define UIP_CONF_BUFFER_SIZE    ETH_MAX_PACKET_SIZE

#define UIP_CONF_LLH_LEN 14+4

#define UIP_CONF_UDP_CONNS 32

// enable uip split hack - to circumvent slow-down due to delayed ACK algorithm
// (will send each tcp packet in two halves)
#define UIP_SPLIT_HACK          0
// enable empty packet - to circumvent slow-down due to delayed ACK algorithm
// (will send each tcp packet in two halves) - alternative to UIP_SPLIT_HACK
#define UIP_EMPTY_PACKET_HACK   1

#define ETHERNET

// which mechanism to use for protothreads
//#define LC_CONF_INCLUDE "lc-addrlabels.h" // using special GCC feature (uses sligtly less program memory)
#define LC_CONF_INCLUDE "lc-switch.h" // using switch statements (standard)


#endif //__UNETCONFIGURE_H__
