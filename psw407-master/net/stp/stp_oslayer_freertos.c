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

#include "bstp.h"
#include "board.h"
#include "selftest.h"
#include "settings.h"
#include "debug.h"
#ifdef USE_STP

void bstp_main_task(void *pvParameters);

static xSemaphoreHandle bstp_sem;
static xQueueHandle bstp_queue;

extern const u_int8_t bstp_etheraddr[6];

static void sw_rstp_config(void){
	uint16_t tmp;
	switch(get_marvell_id()){
		case DEV_88E6176:
		case DEV_88E6240:
			//set cpu port
			//CPU  destination port
			tmp = ETH_ReadPHYRegister(GlobalRegister,0x1A);
			tmp&=~0xF0;
			tmp|=CPU_PORT<<4;
			ETH_WritePHYRegister(GlobalRegister,0x1A,tmp);
			//no brake
		case DEV_88E095:
			for(int i=0;i<MV_PORT_NUM;i++){
				  //BPDU to CPU
				  //Set CPU Port
				  tmp = ETH_ReadPHYRegister(0x10+i,0x08);
				  tmp&=~0x0f;
				  tmp|=CPU_PORT;
				  ETH_WritePHYRegister(0x10+i, 0x08, tmp);
			}
			break;

		case DEV_88E096:
		case DEV_88E097:
			/*only in 6096(7)*/
			tmp = ETH_ReadPHYRegister(GlobalRegister,0x1A);
			tmp&=~0xF0;
			tmp|=0xA0;
			ETH_WritePHYRegister(GlobalRegister,0x1A,tmp);
			break;

		case DEV_98DX316:

			break;
	}


	if(get_marvell_id()!=DEV_98DX316){
		//Enable MGMT
		tmp = ETH_ReadPHYRegister(Global2Register,0x03);
		tmp|=0x01;
		ETH_WritePHYRegister(Global2Register, 0x03, tmp);

		tmp = ETH_ReadPHYRegister(Global2Register,0x05);
		tmp|=(	1<<3)|0x07;//set Rsvd2CPU + MGMT Priority
		ETH_WritePHYRegister(Global2Register, 0x05, tmp);
	}
}

void sw_rstp_deconfig(void){
	uint16_t tmp;

	switch(get_marvell_id()){
		case DEV_88E095:
		case DEV_88E6176:
		case DEV_88E6240:
			/*only in 6095*/
			for(int i=0;i<MV_PORT_NUM;i++){
				 //BPDU to CPU
				 //delete cpu port
				 tmp = ETH_ReadPHYRegister(0x10+i,0x08);
				 tmp&=~0x0f;
				 ETH_WritePHYRegister(0x10+i, 0x08, tmp);
			}
			break;

		case DEV_88E096:
		case DEV_88E097:
			/*only in 6096(7)*/
			tmp = ETH_ReadPHYRegister(GlobalRegister,0x1A);
			tmp&=~0xF0;
			ETH_WritePHYRegister(GlobalRegister,0x1A,tmp);
			break;
	}
	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
		(get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
		//Disable MGMT
		tmp = ETH_ReadPHYRegister(Global2Register,0x03);
		tmp&=~0x01;
		ETH_WritePHYRegister(Global2Register, 0x03, tmp);
		tmp = ETH_ReadPHYRegister(Global2Register,0x05);
		tmp&=~((1<<3)|0x07);
		ETH_WritePHYRegister(Global2Register, 0x05, tmp);
	}
}

void bstp_task_start(void){
  xTaskHandle handle;
  uint16_t tmp;

   DEBUG_MSG(STP_DEBUG,"bstp first config\r\n");

   vSemaphoreCreateBinary(bstp_sem);
   bstp_queue = xQueueCreate((ALL_PORT_NUM), sizeof(bstp_event_t));
   if (bstp_queue==NULL)
      return;

   bstp_create();

   if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
   		(get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
	  //enable cpu port
	   tmp = ETH_ReadPHYRegister(0x10+CPU_PORT,0x04);
	   tmp|= (1<<10) //enable igmp
			 |(1<<8) //enamble dsa tags
			 | 0x03; //port state - forwarding
	   ETH_WritePHYRegister(0x10+CPU_PORT, 0x04, tmp);
   }

   sw_rstp_config();


	ETH_MACAddressConfig(ETH_MAC_Address1, (uint8_t *)&bstp_etheraddr[0]);
	ETH_MACAddressPerfectFilterCmd(ETH_MAC_Address1, ENABLE);
	ETH_MACAddressFilterConfig(ETH_MAC_Address1, ETH_MAC_AddressFilter_DA);

	#if BRIDGESTP_DEBUG
		xTaskCreate( bstp_main_task, (void*)"STP_D", (128*15), NULL, STP_PRIORITY, &handle );
	#else
		xTaskCreate( bstp_main_task, (void*)"STP", 128*15, NULL, STP_PRIORITY, &handle );
	#endif

}


void get_haddr(uint8_t *mac){
	extern uint8_t dev_addr[6];
	memcpy(mac, dev_addr, 6);
}

void stp_set_port_state(int port, int state){
u32 val;
uint16_t tmp;
u32 offset;



	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
			(get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
		  switch (state) {
			case BSTP_IFSTATE_DISABLED:
				val = 0x00;
				break;
			case BSTP_IFSTATE_DISCARDING:
				val = 0x01;
				break;
			case BSTP_IFSTATE_LEARNING:
				val = 0x02;
				break;
			case BSTP_IFSTATE_FORWARDING:
				val = 0x03;
				break;
			default: val = 1;
		  }


		  tmp = ETH_ReadPHYRegister(0x10+port,0x04);
		  tmp&=~0x03;
		  tmp|=val;
		  ETH_WritePHYRegister(0x10+port, 0x04, tmp);

		//for PSW-1G
		  //поднятие линка происходит при настройке порта
		  if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
				//PowerDown to 0 - normal operation - REG 0
				tmp = ETH_ReadIndirectPHYReg(port,PAGE0,0);
				tmp &=~0x800;
				ETH_WriteIndirectPHYReg(port,PAGE0,0,tmp);
				//PowerDown to 0 - normal operation - REG 16
				tmp = ETH_ReadIndirectPHYReg(port,0,16);
				tmp &=~0x4;
				ETH_WriteIndirectPHYReg(port,PAGE0,16,tmp);
				tmp = ETH_ReadIndirectPHYReg(port,0,16);

				//PowerDown to 0 for fiber portrs
				if(port == (MV_PORT_NUM-1)){
					tmp = ETH_ReadIndirectPHYReg(0x0F,PAGE1,0);
					tmp &=~0x800;
					ETH_WriteIndirectPHYReg(0x0F,PAGE1,0,tmp);
				}
		  }
	}

	if(get_marvell_id() == DEV_98DX316){
		switch(state){
			case BSTP_IFSTATE_DISABLED:
				val = 0x00;
				break;

			case BSTP_IFSTATE_DISCARDING:
				val = 0x01;
				break;

			case BSTP_IFSTATE_LEARNING:
				val = 0x02;
				break;

			case BSTP_IFSTATE_FORWARDING:
				val = 0x03;
				break;

			default: val = 1;
		}
		if(port<16){
			offset = port*2;
			Salsa2_WriteRegField(SPAN_STATE_GROUP0_W0,offset,val,2);
		}
		else{
			offset = (port-16)*2;
			Salsa2_WriteRegField(SPAN_STATE_GROUP0_W1,offset,val,2);
		}
	}
}

int stp_send(int port, uint8_t *buf, int len){
  struct uip_eth_hdr *eh;
	taskENTER_CRITICAL();
    eh = (struct uip_eth_hdr *)buf;
#if DSA_TAG_ENABLE

	//FROM_CPU DSA Tag - они одинаковы для всех чипов
	eh->dsa_tag = HTONL(0x40000000|(port<<19)|(7<<13));

#endif
    ETH_HandleTxPkt(buf, len);
    taskEXIT_CRITICAL();
    return 0;
}

int stp_ethhdr2port(uint8_t *ptr){
  struct uip_eth_hdr *eh;
   eh = (struct uip_eth_hdr *)ptr;
#if DSA_TAG_ENABLE
   return DSA_TAG_PORT(eh->dsa_tag);
#else
   return 0;
#endif
}

int bstp_sem_take(void){
	xSemaphoreTake( bstp_sem, (portTickType)portMAX_DELAY);
	return 0;
}

int bstp_sem_free(void){
	xSemaphoreGive( bstp_sem);
	return 0;
}

int bstp_send_event(bstp_event_t *event){
	if (bstp_queue==NULL){
		ADD_ALARM(ERROR_CREATE_STP_QUEUE);
		return -1;
	}
	if( xQueueSend(bstp_queue, (void *)event, (portTickType)portMAX_DELAY)!=pdPASS){
		ADD_ALARM(ERROR_CREATE_STP_NOSEND);
		return -1;
	}
	return 0;
}

int bstp_send_event_i(bstp_event_t *event){
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	if (bstp_queue==NULL) return xHigherPriorityTaskWoken;
	xQueueSendFromISR( bstp_queue, (void *)event, &xHigherPriorityTaskWoken );
	return xHigherPriorityTaskWoken;
}

int bstp_recv_event(bstp_event_t *event, unsigned long tout){
	if(xQueueReceive(bstp_queue, event, (portTickType)tout)==pdPASS) return 0;
	return -1;
}



unsigned long bstp_getticks(void){
  return xTaskGetTickCount();
}

static portTickType xTaskGetDeltaTickCount(portTickType TickCount){
	portTickType Now;
	Now = xTaskGetTickCount();
	if (Now >= TickCount)
		return (Now-TickCount);
	else
	   return (portMAX_DELAY-TickCount+Now);
}

unsigned long bstp_getdeltatick(unsigned long ticks){
	portTickType xTaskGetDeltaTickCount(portTickType TickCount);
    return xTaskGetDeltaTickCount(ticks);
}

void flush_db(int port){

	DEBUG_MSG(STP_DEBUG,"flush_db\r\n");

	smi_flush_all();
	/*
	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
			(get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
		smi_flush_all();
	}
	else if(get_marvell_id() == DEV_98DX316){
		//Salsa2_WriteRegField(PORT0_TRANSMIT_CFG_REG+0x1000*L2F_port_conv(port),12,1,1);
		//set bit
		for(u8 i=0;i<ALL_PORT_NUM;i++)
			Salsa2_WriteRegField(PORT0_TRANSMIT_CFG_REG+0x1000*L2F_port_conv(i),12,1,1);
		Salsa2_WriteRegField(PORT0_TRANSMIT_CFG_REG+0x1F000,12,1,1);//Cpu port
		//clear bit
		vTaskDelay(10*MSEC);
		for(u8 i=0;i<ALL_PORT_NUM;i++)
			Salsa2_WriteRegField(PORT0_TRANSMIT_CFG_REG+0x1000*L2F_port_conv(i),12,0,1);
		Salsa2_WriteRegField(PORT0_TRANSMIT_CFG_REG+0x1F000,12,0,1);//Cpu port

	}*/

}


void stp_if_poll(int port, int *link, int *duplex, uint32_t *baudrate){
uint16_t tmp;
	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
		(get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
		tmp = ETH_ReadPHYRegister(port+0x10, 0x00);
		switch ((tmp>>8)&0x03){
		  case 0: *baudrate =   10*1000; break;
		  case 1: *baudrate =  100*1000; break;
		  case 2: *baudrate = 1000*1000; break;
		  default: *baudrate = 0;
		}
		*link = tmp&(1<<11);
		*duplex = tmp&(1<<10);
	}

	else if(get_marvell_id() == DEV_98DX316){
		tmp = Salsa2_ReadReg(PORT_STATUS_REG0+0x400*L2F_port_conv(port));
		//link
		if(tmp & 0x01)
			*link = 1;
		else
			*link = 0;
		//speed
		switch((tmp>>1) & 0x03){
			case 0: *baudrate =   10*1000; break;
			case 2: *baudrate =   100*1000; break;
			case 1: *baudrate =   1000*1000; break;
			default: *baudrate = 0;
		}
		//duplex
		if(tmp & 0x08)
			*duplex = 1;
		else
			*duplex = 0;
	}
}


#endif /*USE_STP*/

