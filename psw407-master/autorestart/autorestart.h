/*
 * autorestart.h
 *
 *  Created on: 21.01.2014
 *      Author: Tsepelev
 */

#ifndef AUTORESTART_H_
#define AUTORESTART_H_

//OtherCommand Task
#define POE_DISABLE_FE1 1
#define POE_DISABLE_FE2 2
#define POE_DISABLE_FE3 3
#define POE_DISABLE_FE4 4
#define POE_DISABLE_FE5 5
#define POE_DISABLE_FE6 6
#define POE_DISABLE_FE7 7
#define POE_DISABLE_FE8 8

#define PING_PROCESSING 10

void set_autorestart_init(u8 state);
void PoECamControl(void *pvParameters);
void PoE1(void *arg);
void PoE2(void *arg);
void PoE3(void *arg);
void PoE4(void *arg);
void OtherCommands(void *arg);
u8 RemotePing(uip_ipaddr_t IP);



#endif /* AUTORESTART_H_ */
