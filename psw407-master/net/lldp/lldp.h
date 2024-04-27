/*
 * lldp.h
 *
 *  Created on: 5 окт. 2018 г.
 *      Author: BelyaevAlex
 */

#ifndef NET_LLDP_LLDP_H_
#define NET_LLDP_LLDP_H_


#define LLDP_ETHTYPE 0x88cc


void lldp_init(void);
void lldp_timer_processing(void);
void lldp_rawinput(uint8_t *ptr, int size);
void lldp_output(void);

#endif /* NET_LLDP_LLDP_H_ */
