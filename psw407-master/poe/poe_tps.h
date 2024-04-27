#ifndef __POE_TPS_H
#define __POE_TPS_H

#if 1//USE_TPS
#include <stdint.h>
#include "stm32f4xx.h"

void poe_alt_ab_init(void);
void set_poe_alt_ab(u8 state);

uint16_t PoECurrentRead(uint8_t Port);
uint16_t PoEVoltageRead(uint8_t Port);
uint8_t PoEStateRead(uint8_t Port);

#endif
#endif
