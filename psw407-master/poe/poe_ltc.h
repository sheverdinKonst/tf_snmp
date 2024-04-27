#ifndef POE_LTC_H_
#define POE_LTC_H_

#include "stm32f4xx.h"




#define POE_A 0
#define POE_B 1

#define ULTRAPOE_MAX_CNT	10//число повторов в запуске для ultraPoE (решение проблемы холодного старта)

#define TYPE_UNLIMIT	1
#define TYPE_NORMAL		0

#define DEV_TPS2384 1
#define DEV_LTC4271 2

#define CLSB	(float)(0.12201)
#define VLSB	(float)(5.835)

#define Adress4271 0x40 //LTC

#define Address2384       0x20 //TPS

#define Adress4271block1 0x40
#define Adress4271block2 0x42

//internal register addr
#define INTSTAT 		0x00
#define INTMASK			0x01
#define PWREVN			0x02
#define PWREVN_COR		0x03
#define DETEVN			0x04
#define DETEVN_COR		0x05
#define FLTEVN			0x06
#define FLTEVN_COR		0x07
#define TSEVN			0x08
#define TSEVN_COR		0x09
#define SUPEVN 			0x0A
#define SUPEVN_COR		0x0B
#define STATP1			0x0C
#define STATP2			0x0D
#define STATP3			0x0E
#define STATP4			0x0F
#define STATPWR			0x10
#define STSTPIN			0x11
#define OPMD			0x12
#define DISENA			0x13
#define DETENA			0x14
#define MIDSPAN			0x15
#define MCONF			0x17
#define	DETPB			0x18
#define PWRPB			0x19
#define RSTPB			0x1A
#define ID				0x1B
#define TLIM			0x1E
#define GPIOOEN			0x20
#define GPIOOUT			0x21
#define GPIOIN			0x22
#define PORTMONCFG		0x2D
#define VEELSB			0x2E
#define VEEMSB			0x2F
#define IP1LSB			0x30
#define IP1MSB			0x31
#define VP1LSB			0x32
#define VP1MSB			0x33
#define IP2LSB			0x34
#define IP2MSB			0x35
#define VP2LSB			0x36
#define VP2MSB			0x37
#define IP3LSB			0x38
#define IP3MSB			0x39
#define VP3LSB			0x3A
#define VP3MSB			0x3B
#define IP4LSB			0x3C
#define IP4MSB			0x3D
#define VP4LSB			0x3E
#define VP4MSB			0x3F
#define FIRMWARE		0x41
#define WDOG			0x42
#define DEVID			0x43
#define HPEN			0x44
#define HPMD1			0x46
#define CUT1			0x47
#define LIM1			0x48
#define HPSTAT1			0x49
#define HPMD2			0x4B
#define CUT2			0x4C
#define LIM2			0x4D
#define HPSTAT2			0x4E
#define HPMD3			0x50
#define CUT3			0x51
#define LIM3			0x52
#define HPSTAT3			0x53
#define HPMD4			0x55
#define CUT4			0x56
#define LIM4			0x57
#define HPSTAT4			0x58
#define MSDPINMODE		0x5B
#define P1_POLCURRAVG	0x61
#define P2_POLCURRAVG	0x65
#define P3_POLCURRAVG	0x69
#define P4_POLCURRAVG	0x6D
#define XIOOEN			0x70
#define XIOOUT			0x71
#define XIOIN			0x72
#define UPPROGCTL		0xEA
#define UPPROGADDLSB 	0xEB
#define UPPROGADDMSB	0xEC
#define UPPROGDATA		0xED


//interrupp mask
#define PWRENA_INT		1
#define PWRGB_INT		2
#define DIS_INT			4
#define DET_INT			8
#define CLASS_INT		16

//pushbuttons
#define INTCLR_PB		128
#define PINCLR_PB		64

//mconf
#define INTEN			0x80

i8 get_poe_id(void);
u8 poe_int_clr(void);
u8 if_poe_state_changed(u8 port);
u8 poe_port_cfg(u8 port);
u8 poe_interrupt_cfg(void);
u8 get_poe_state(u8 type,u8 port);
void set_poe_state(u8 type,uint8_t port, uint8_t state);
uint16_t get_poe_voltage(u8 type,u8 port);
uint16_t get_poe_current(u8 type,u8 port);
void PoEControlTask(void *pvParam);
void set_poe_init(u8 x);
u8 calculate_offset(u8 type,u8 port);
u8 calculate_addr_bank(u8 port);

#endif /* POE_LTC_H_   */
