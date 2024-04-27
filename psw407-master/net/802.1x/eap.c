/*
 * eap.c
 *
 *  Created on: 28 февр. 2018 г.
 *      Author: BelyaevAlex
 */
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "../deffines.h"
#include "stm32f4x7_eth.h"

#include <string.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"
#include "board.h"
#include "settings.h"
#include "SMIApi.h"
#include "../igmp/igmp_mv.h"
#include "h/driver/gtDrvSwRegs.h"
#include "VLAN.h"
#include "FlowCtrl.h"
#include "SpeedDuplex.h"
#include "QoS.h"
#include "AdvVCT.h"
#include "debug.h"

#include "h/driver/gtDrvSwRegs.h"
#include "h/driver/gtHwCntl.h"
#include "gtPortRateCtrl.h"

#include "../stp/bridgestp.h"
#include "salsa2.h"
#include "../uip/timer.h"
#include "eapol.h"

//зарезервированный MAC адрес для EAPOL пакетов
const u8 eapol_ethaddr[6] = {0x01,0x80,0xC2,0x00,0x00,0x03};


void ieee802_1x_init(void){
	DEBUG_MSG(DOT1X_DEBUG,"ieee802_1x_init\r\n");
	eap_config();
}

void eap_config(void){
u16 tmp;
	DEBUG_MSG(DOT1X_DEBUG,"eap_config\r\n");

	//фильтр в процессоре
	ETH_MACAddressConfig(ETH_MAC_Address2, (uint8_t *)&eapol_ethaddr[0]);
	ETH_MACAddressPerfectFilterCmd(ETH_MAC_Address2, ENABLE);
	ETH_MACAddressFilterConfig(ETH_MAC_Address2, ETH_MAC_AddressFilter_DA);

	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){
		//config rsvr2cpu enable vector
		tmp = ETH_ReadPHYRegister(0x1C,0x03);
		tmp |= 1<<3;
		ETH_WritePHYRegister(0x1C,0x03,tmp);

		//enable rsvd2cpu
		tmp = ETH_ReadPHYRegister(0x1C,0x05);
		tmp |= 1<<3;
		ETH_WritePHYRegister(0x1C,0x05,tmp);

		//config cpu port
		for(u8 i=0;i<ALL_PORT_NUM;i++){
			tmp = ETH_ReadPHYRegister(0x10+i,0x08);
			tmp &=~0x0F;
			tmp |= CPU_PORT;
			ETH_WritePHYRegister(0x10+i,0x08,tmp);

			tmp = ETH_ReadPHYRegister(0x10+i,0x04);
			tmp|= (1<<10) //enable igmp
				 |(1<<8); //enamble dsa tags
		    ETH_WritePHYRegister(0x10+i, 0x04, tmp);
		}


	}

}
