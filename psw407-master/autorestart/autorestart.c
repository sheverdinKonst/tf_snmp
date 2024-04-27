/*
 * autorestart.c
 *
 *  Created on: 21.01.2014
 *      Author: Alex
 */
#include "string.h"
#include "stm32f4xx.h"
#include "FreeRtos.h"
#include "task.h"
#include "../net/uip/uip.h"
#include "autorestart.h"
#include "board.h"
#include "settings.h"
#include "../net/syslog/msg_build.h"
#include "../net/smtp/smtp.h"
#include "poe_ltc.h"
#include "../net/icmp/icmp.h"
#include "../events/events_handler.h"
#include "debug.h"

xTaskHandle xOtherCommands, xPoE1,xPoE2,xPoE3,xPoE4;



uint8_t OtherCommandsFlag=0;
uint8_t SendICMPPingFlag;
u8 autorestart_init=0;
u8 PingFlag;
u8 RunTaskCAM=0;
uip_ipaddr_t IPDestPing;

static u8 PoE1Run,PoE2Run,PoE3Run,PoE4Run;

//extern uint8_t SS;
extern uip_ipaddr_t uip_hostaddr, uip_dest_ping_addr;
extern u16 LedPeriod;

//extern struct status_t status;

/*контроль зависания камер*/
void PoECamControl(void *pvParameters){
    uint8_t Time[PORT_NUM];
    memset(Time,0,sizeof(Time));

    u32 tmp;
    RunTaskCAM=1;
    uip_ipaddr_t ip;
    u8 i;

    //clear reset counters
    for(i=0;i<POE_PORT_NUM;i++){
    	dev.port_stat[i].rst_cnt = 0;
    }


    vTaskDelay(10000*MSEC);

  for (;;){
	  vTaskDelay(1000*MSEC);
  	  if(((uip_hostaddr[0]!=0)&&(uip_hostaddr[1]!=0))){
			for(i=0;i<POE_PORT_NUM;i++){
				//for port fe1-4
				if(dev.port_stat[i].ss_process==0){
					  switch(get_port_sett_wdt(i)){
						  case 0: vTaskDelay(5000*MSEC); // ничего не делаем
						  	  	  dev.port_stat[i].rst_cnt = 0;
						  	  	  set_port_error(i,0);
						  	  	  break;
						  case 1://link // max delay 5-10 sec
							  	//если есть линк или нет PoE, то останавливаем
								if((get_port_link(i))||(get_port_poe_a(i)==0)) {
									set_port_error(i,0);
									dev.port_stat[i].rst_cnt = 0;
									if(NO_PORT_ERROR)
										LedPeriod=1000;
										Time[i]=0;
								}
								else{
									DEBUG_MSG(AUTORESTART_DBG,"No link FE%d\r\n",i+1);
									Time[i]++;
									vTaskDelay(5000*MSEC);
									if(Time[i]>=6){
										if(dev.port_stat[i].rst_cnt<MAX_RST_CNT){
											set_poe_state(POE_A,i,DISABLE);
											set_poe_state(POE_B,i,DISABLE);
											vTaskDelay(10000*MSEC);
											set_poe_state(POE_A,i,ENABLE);
											set_poe_state(POE_B,i,ENABLE);
											set_poe_init(0);
											dev.port_stat[i].rst_cnt++;
											send_events_u32(EVENT_NOREPLY_LINK_P1+i,0);
										}
										set_port_error(i,1);
										LedPeriod=100;
										Time[i]=0;
									}
								}
								break;

						  case 2://ping // max delay 25 sec
								dev.port_stat[i].dest_ping=0;
								get_port_sett_wdt_ip(i,&ip);
								if(RemotePing(ip)==2){
									//ICMP not send
									dev.port_stat[i].dest_ping=1;
									RemotePing(ip);
								}
								vTaskDelay(10000*MSEC);

								//если отвечает на пинг или нет poe
								if((dev.port_stat[i].dest_ping)||(get_port_poe_a(i)==0 && get_port_poe_b(i)==0)) {
									Time[i]=0;
									set_port_error(i,0);
									if(NO_PORT_ERROR)
										LedPeriod=1000;
									dev.port_stat[i].rst_cnt = 0;
									DEBUG_MSG(AUTORESTART_DBG,"Reply ping FE%d OK\r\n",i+1);
								}
								else{
									Time[i]++;
									if(Time[i]>=6){
										DEBUG_MSG(AUTORESTART_DBG,"No reply ping FE%d\r\n",i+1);
										/*poe reset*/
										if(dev.port_stat[i].rst_cnt<MAX_RST_CNT){
											set_poe_state(POE_A,i,DISABLE);
											set_poe_state(POE_B,i,DISABLE);
											vTaskDelay(10000*MSEC);
											set_poe_init(0);
											send_events_u32(EVENT_NOREPLY_PING_P1+i,0);
											dev.port_stat[i].rst_cnt++;
										}
										set_port_error(i,2);
										LedPeriod=100;
										Time[i]=0;
									}
									dev.port_stat[i].dest_ping=0;
								}
								break;
						  case 3: //speed
							  if((get_port_poe_a(i)||get_port_poe_b(i))&&
								  ((get_port_sett_wdt_speed_down(i)&&((dev.port_stat[i].rx_speed/128) < (get_port_sett_wdt_speed_down(i))))||
								     (get_port_sett_wdt_speed_up(i)&&((dev.port_stat[i].rx_speed/128) > (get_port_sett_wdt_speed_up(i)))))){

									  Time[i]++;
									  if(Time[i]>=6){
										  DEBUG_MSG(AUTORESTART_DBG,"Incorrect speed on port FE%d\r\n",i+1);
										  /*poe reset*/
										  if(dev.port_stat[i].rst_cnt<MAX_RST_CNT){
											  if(get_port_sett_wdt_speed_down(i)&&((dev.port_stat[i].rx_speed/128) < (get_port_sett_wdt_speed_down(i)))){
												  send_events_u32(EVENT_LOWSPEED_P1+i,dev.port_stat[i].rx_speed/128);
											  	  set_port_error(i,3);
											  }
											  else if(get_port_sett_wdt_speed_up(i)&&((dev.port_stat[i].rx_speed/128) > (get_port_sett_wdt_speed_up(i)))){
												  send_events_u32(EVENTS_WDT_SPEED_UP_P1+i,dev.port_stat[i].rx_speed/128);
												  set_port_error(i,4);
											  }

											  set_poe_state(POE_A,i,DISABLE);
											  set_poe_state(POE_B,i,DISABLE);
											  set_port_state(i,DISABLE);
											  settings_add2queue(SQ_PORT_1+i);
											  vTaskDelay(10000*MSEC);
											  set_poe_init(0);
											  set_port_state(i,ENABLE);
											  settings_add2queue(SQ_PORT_1+i);

											  dev.port_stat[i].rst_cnt++;
										  }
										  //set_port_error(i,3);
										  LedPeriod=100;
										  Time[i]=0;
									  }

							  }
							  else{
								  //norm
								  Time[i]=0;
								  dev.port_stat[i].rst_cnt = 0;
								  set_port_error(i,0);
								  if(NO_PORT_ERROR)
									 LedPeriod=1000;
							  }
							  vTaskDelay(10000*MSEC);
							  break;
					  }

				}
		}
	    if(NO_PORT_ERROR)
			LedPeriod=1000;
  	  }
  }
  vTaskDelete( NULL);
}


/*отправка ICMP пакета в сеть*/
u8 RemotePing(uip_ipaddr_t IP){
	if(PingFlag!=0)
		return 1;

	PingFlag=1;

	DEBUG_MSG(AUTORESTART_DBG,"RemotePing IP %d.%d.%d.%d\r\n",uip_ipaddr1(IP),uip_ipaddr2(IP),uip_ipaddr3(IP),uip_ipaddr4(IP));

	/*
#ifdef DSA_TAG_ENABLE
	len=78;
#else
	len=74;
#endif
	*/

	if ((IP[0]==0)||(IP[1]==0)){
		PingFlag=0;
		//xSemaphoreGive(PingSemaphore);
		DEBUG_MSG(AUTORESTART_DBG,"null IP addr\r\n");
		return 1;
	}
	if((uip_hostaddr[0]==IP[0])&&(uip_hostaddr[1]==IP[1])){
		DEBUG_MSG(AUTORESTART_DBG,"MyIP == IP addr\r\n");
		PingFlag=0;
		return 2;
	}

	uip_ipaddr_copy(&uip_dest_ping_addr, IP);

	set_icmp_need_send(1);

PingFlag=0;
return 1;
}



void OtherCommands(void *arg){
static uint8_t cnt;
uint32_t TimeOut=0;
vTaskSuspend(NULL);
  while (1)
  {
	switch(OtherCommandsFlag){
		case 0:
			break;

		case POE_DISABLE_FE1:
		case POE_DISABLE_FE2:
		case POE_DISABLE_FE3:
		case POE_DISABLE_FE4:
		case POE_DISABLE_FE5:
		case POE_DISABLE_FE6:
		case POE_DISABLE_FE7:
		case POE_DISABLE_FE8:
			set_poe_state(POE_A,OtherCommandsFlag-1,DISABLE);
			vTaskDelay(10000*MSEC);
			//power on if need
			set_poe_state(POE_A,OtherCommandsFlag-1,get_port_sett_poe(OtherCommandsFlag-1));
			DEBUG_MSG(AUTORESTART_DBG,"reboot port FE%d\r\n",OtherCommandsFlag);
			OtherCommandsFlag = 0;
			break;


		case PING_PROCESSING:{
			cnt=0;
			SendICMPPingFlag=0;
			do{
				if(PingFlag){
					TimeOut=200;
					while(PingFlag && TimeOut)
						TimeOut--;
				}else{
					RemotePing(IPDestPing);
				}
				vTaskDelay(1000*MSEC);
				cnt++;
				vTaskDelay(1000*MSEC);
			}while((SendICMPPingFlag<4)&&(cnt<=6));
			cnt=0;

		break;
		}



	}
	OtherCommandsFlag=0;

	vTaskSuspend(NULL);

  }
}

/*перезагрузка камеры на порту FE1*/
void PoE1(void *arg){
PoE1Run=1;
	set_poe_state(POE_A,0,DISABLE);
	vTaskDelay(10000*MSEC);
	set_poe_state(POE_A,0,ENABLE);
PoE1Run=0;
	vTaskDelete( NULL);
  while(1);
}
/*перезагрузка камеры на порту FE2*/
void PoE2(void *arg){
PoE2Run=1;
	set_poe_state(POE_A,1,DISABLE);
	vTaskDelay(10000*MSEC);
	set_poe_state(POE_A,1,ENABLE);
PoE2Run=0;
	vTaskDelete( NULL);
  while(1);
}
/*перезагрузка камеры на порту FE3*/
void PoE3(void *arg){
PoE3Run=1;
	set_poe_state(POE_A,2,DISABLE);
	vTaskDelay(10000*MSEC);
	set_poe_state(POE_A,2,ENABLE);
PoE3Run=0;
	vTaskDelete( NULL);
  while(1);
}
/*перезагрузка камеры на порту FE4*/
void PoE4(void *arg){
PoE4Run=1;
	set_poe_state(POE_A,3,DISABLE);
	vTaskDelay(10000*MSEC);
	set_poe_state(POE_A,3,ENABLE);
PoE4Run=0;
	vTaskDelete( NULL);
  while(1);
}

