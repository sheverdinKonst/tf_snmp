/*
 * SpeedDuplex.h
 *
 *  Created on: 06.07.2012
 *      Author:
 */


#ifndef SPEEDDUPLEX_H_
#define SPEEDDUPLEX_H_

typedef struct{
	u8 lport;
	u8 fport;
	u8 state;
	u8 speed;
	u8 duplex;
	u8 flow;
}port_sett_t;


uint8_t StateSpeedDuplexANegSet(uint8_t Port,uint8_t State, uint8_t Speed, uint8_t Duplex, u8 ANeg);
uint16_t SpeedConv(uint8_t Sp);
uint8_t DuplexConv(uint8_t Dplx);
uint8_t ANEgConv(uint8_t ANeg);
uint8_t RateLimitConfig(void);
void SWU_PortConfig(port_sett_t *port_sett);

void get_port_config(u8 lport,port_sett_t *port_sett);
void switch_port_config(port_sett_t *port_sett);

//struct Rate rate[5];

#endif /* SPEEDDUPLEX_H_ */
