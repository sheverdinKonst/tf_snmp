#include "stm32f4x7_eth.h"
#include "../flash/spiflash.h"
#include "../deffines.h"
#include "board.h"
#include "../uip/uip.h"
#include "h/driver/gtDrvSwRegs.h"
#include "SMIApi.h"
#include "VLAN.h"
#include "SpeedDuplex.h"
#include "FlowCtrl.h"
#include "gtPortRateCtrl.h"
#include "Salsa2Regs.h"
#include "settings.h"
#include "debug.h"
#include "salsa2.h"

uint16_t SpeedConv(uint8_t Sp){
	switch(Sp){
		case 0: return 0;
		case 1: return 10;
		case 2: return 10;
		case 3: return 100;
		case 4: return 100;
		case 5: return 1000;
		case 6: return 1000;
	}
return 0;
}

uint8_t DuplexConv(uint8_t Dplx){
	switch(Dplx){
		case 0: return 0;
		case 1: return HALF;
		case 2: return FULL;
		case 3: return HALF;
		case 4: return FULL;
		case 5: return HALF;
		case 6: return FULL;
	}
return 0;
}

uint8_t ANEgConv(uint8_t ANeg){
	if(ANeg)
		return FALSE;
	else
		return TRUE;
}

/*Port config for Marvell 88E6240/6176*/
u8 PSW1G_PortConfig(port_sett_t *port_sett){
u8 hwPort;
GT_U16           u16Data,regStart;
//GT_STATUS        retVal = GT_OK;
uint8_t 		 TimeOut=0;
u16 tmp;

u8 fc_state;

	/* translate LPORT to hardware port */
	//hwPort = lport2port(port);


	if(is_fiber(port_sett->lport)){
		if(get_port_sett_state(port_sett->lport)==DISABLE){
			//link forced down
			ETH_WritePHYRegister(0x10+port_sett->fport,0x01,0x413);

			//power down
			u16Data = 1;
			hwSetPhyRegField6240(port_sett->fport, QD_PHY_CONTROL_REG,PAGE1,11,1,u16Data);

			//mac power down
			u16Data = 0;
			hwSetPhyRegField6240(port_sett->fport, QD_PHY_SPEC_CONTROL_REG,PAGE1,3,0,u16Data);

			return GT_OK;
		}
		else{
			//link auto
			ETH_WritePHYRegister(0x10+port_sett->fport,0x01,0x403);
			//power down disable
			u16Data = 0;
			hwSetPhyRegField6240(port_sett->fport, QD_PHY_CONTROL_REG,PAGE1,11,1,u16Data);

			//mac power up
			u16Data = 1;
			hwSetPhyRegField6240(port_sett->fport, QD_PHY_SPEC_CONTROL_REG,PAGE1,3,0,u16Data);


			//power down disable
			u16Data = 0;
			hwSetPhyRegField6240(port_sett->fport, QD_PHY_CONTROL_REG,PAGE0,11,1,u16Data);

			//mac power up
			u16Data = 1;
			hwSetPhyRegField6240(port_sett->fport, QD_PHY_SPEC_CONTROL_REG,PAGE0,3,0,u16Data);


			//PowerDown to 0 - normal operation - REG 0
			tmp = ETH_ReadIndirectPHYReg(port_sett->fport,PAGE0,0);
			tmp &=~0x800;
			ETH_WriteIndirectPHYReg(port_sett->fport,PAGE0,0,tmp);
			//PowerDown to 0 - normal operation - REG 16
			tmp = ETH_ReadIndirectPHYReg(port_sett->fport,0,16);
			tmp &=~0x4;
			ETH_WriteIndirectPHYReg(port_sett->fport,PAGE0,16,tmp);
			tmp = ETH_ReadIndirectPHYReg(port_sett->fport,0,16);
			tmp = ETH_ReadIndirectPHYReg(0x0F,PAGE1,0);
			tmp &=~0x800;
			ETH_WriteIndirectPHYReg(0x0F,PAGE1,0,tmp);
		}
	}
	else{
		if(get_port_sett_state(port_sett->lport)==DISABLE){
			//power down
			u16Data = 1;
			hwSetPhyRegField6240(port_sett->fport, QD_PHY_CONTROL_REG,PAGE0,11,1,u16Data);

			//mac power down
			u16Data = 0;
			hwSetPhyRegField6240(port_sett->fport, QD_PHY_SPEC_CONTROL_REG,PAGE0,3,0,u16Data);
		}
		else{
			//power down disable
			u16Data = 0;
			hwSetPhyRegField6240(port_sett->fport, QD_PHY_CONTROL_REG,PAGE0,11,1,u16Data);

			//mac power up
			u16Data = 1;
			hwSetPhyRegField6240(port_sett->fport, QD_PHY_SPEC_CONTROL_REG,PAGE0,3,0,u16Data);
		}
	}



	//todo
	/*if(get_downshifting_mode() == 1){
		//enable Downshift feature
		u16Data = 1;
		hwSetPhyRegField6240(port_sett->fport, QD_PHY_SPEC_CONTROL_REG,PAGE0,11,1,u16Data);


		//software reset
		hwSetPhyRegField6240(port_sett->fport,QD_PHY_CONTROL_REG,PAGE0,(uint8_t)15,1,1);
		while(u16Data & 0x8000){
			u16Data = ETH_ReadIndirectPHYReg(port_sett->fport,PAGE0,QD_PHY_CONTROL_REG);
		}
	}*/

	//set flow control
    DEBUG_MSG(DEBUG_QD,"phySetPause Called.\r\n");



    regStart = 10;

    /**************************************************************/
    //get flow control state
    if(port_sett->flow)
    	fc_state = GT_PHY_BOTH_PAUSE;
    else
    	fc_state = 0;


    u16Data = fc_state;


    /* Write to Phy AutoNegotiation Advertisement Register.  */
    if(hwSetPhyRegField6240(port_sett->fport,QD_PHY_AUTONEGO_AD_REG,PAGE0,(uint8_t)regStart,2,u16Data) != GT_OK)
    {
        DEBUG_MSG(DEBUG_QD,"Not able to write Phy Reg(port:%d,offset:%d).\r\n",hwPort,QD_PHY_AUTONEGO_AD_REG);
        return GT_FAIL;
    }

    /* Restart Auto Negotiation */
    if(hwSetPhyRegField6240(port_sett->fport,QD_PHY_CONTROL_REG,PAGE0,9,1,1) != GT_OK)
    {
        DEBUG_MSG(DEBUG_QD,"Not able to write Phy Reg(port:%d,offset:%d,data:%#x).\r\n",hwPort,QD_PHY_AUTONEGO_AD_REG,u16Data);
        return GT_FAIL;
    }
    /************************************************************************/
    //настройка портов

    /*ждем готовность*/
    TimeOut=100;
    while((ETH_ReadIndirectPHYReg(port_sett->fport,PAGE0, QD_PHY_CONTROL_REG) & QD_PHY_RESET) && (TimeOut>0)){
    	TimeOut--;
    }

    if(port_sett->state==ENABLE){
    	/*power down disable*///не выключаем
    	u16Data=DISABLE;
    	if(is_fiber(port_sett->lport)){
    		//hwSetPhyRegField6240(port_sett->fport,QD_PHY_CONTROL_REG,PAGE1,11,1,u16Data);
    	}
    	else{
			//hwSetPhyRegField6240(port_sett->fport,QD_PHY_CONTROL_REG,PAGE0,11,1,u16Data);

			if(port_sett->speed==0){
			//enable auto negtation
				/*disable auto neg*/
				u16Data=ENABLE;
				hwSetPhyRegField6240(port_sett->fport, QD_PHY_CONTROL_REG,PAGE0,12,1,u16Data);
				/*supoort speed:
				 * 10half
				 * 10full
				 * 100half
				 * 100full*/
				u16Data=0x0F;
				hwSetPhyRegField6240(port_sett->fport,QD_PHY_AUTONEGO_AD_REG,PAGE0,5,4,u16Data);
			}
			else{
				/*disable auto neg*/
				u16Data=DISABLE;
				hwSetPhyRegField6240(port_sett->fport, QD_PHY_CONTROL_REG,PAGE0,12,1,u16Data);
				/*speed & duplex set*/
				u16Data=ETH_ReadIndirectPHYReg(port_sett->fport,PAGE0,QD_PHY_CONTROL_REG);
				/*clear*/
				u16Data &=~(QD_PHY_RESET | QD_PHY_SPEED |QD_PHY_SPEED_MSB| QD_PHY_DUPLEX);
				/*set*/
				if(SpeedConv(port_sett->speed)==10)
					u16Data |=0;
				if(SpeedConv(port_sett->speed)==100)
					u16Data |=QD_PHY_SPEED;
				if(SpeedConv(port_sett->speed)==1000)
					u16Data |=QD_PHY_SPEED_MSB;

				if(DuplexConv(port_sett->speed)==FULL)
					u16Data |=QD_PHY_DUPLEX;
				if(DuplexConv(port_sett->speed)==HALF)//
					u16Data |=0;
				u16Data |=QD_PHY_RESET;

				ETH_WriteIndirectPHYReg(port_sett->fport,PAGE0, QD_PHY_CONTROL_REG,u16Data);
			}
    	}
    }
    /*state disable*/
    else{
    /*power down*/
    	/*u16Data=ENABLE;
    	if(is_fiber(port_sett->lport)){
    		hwSetPhyRegField6240(port_sett->fport, QD_PHY_CONTROL_REG,PAGE1,11,1,u16Data);
    		hwSetPhyRegField6240(port_sett->fport, QD_PHY_SPEC_CONTROL_REG,PAGE1,3,0,u16Data);//mac power down

    	}
    	else{
    		hwSetPhyRegField6240(port_sett->fport, QD_PHY_CONTROL_REG,PAGE0,11,1,u16Data);
    	}*/
    }


    return GT_OK;
}


uint8_t StateSpeedDuplexANegSet(uint8_t Port,uint8_t State, uint8_t Speed, uint8_t Duplex, u8 ANeg){
uint16_t data;
uint8_t TimeOut=0;

DEBUG_MSG(DEBUG_QD,"!Port %d, state %d, speed %d, duplex %d, aneg %d\r\n",Port,State,Speed,Duplex,ANeg);

/*ждем готовность*/
TimeOut=100;
while((ETH_ReadPHYRegister(Port, QD_PHY_CONTROL_REG) & QD_PHY_RESET) && (TimeOut>0)){
	TimeOut--;
}

if(State==ENABLE){
	/*power down disable*///не выключаем
	data=DISABLE;
	hwSetPhyRegField(Port, QD_PHY_CONTROL_REG,11,1,data);

	if(/*Port<=COOPER_PORTS*/1){
		switch(ANeg){
			case ENABLE:{
				/*disable auto neg*/
				data=ENABLE;
				hwSetPhyRegField(Port, QD_PHY_CONTROL_REG,12,1,data);
				/*supoort speed:
				 * 10half
				 * 10full
				 * 100half
				 * 100full*/
				data=0x0F;
				hwSetPhyRegField(Port, QD_PHY_AUTONEGO_AD_REG,5,4,data);
			}break;
			case DISABLE:{
				/*disable auto neg*/
				data=DISABLE;
				hwSetPhyRegField(Port, QD_PHY_CONTROL_REG,12,1,data);
				/*speed & duplex set*/
				data=ETH_ReadPHYRegister(Port, QD_PHY_CONTROL_REG);
				/*clear*/
				data &=~(QD_PHY_RESET | QD_PHY_SPEED | QD_PHY_DUPLEX);
				/*set*/
				if(Speed==10)
					data |=0;
				if(Speed==100)
					data |=QD_PHY_SPEED;

				if(Duplex==FULL)//
					data |=QD_PHY_DUPLEX;
				if(Duplex==HALF)//
					data |=0;
				data |=QD_PHY_RESET;

				ETH_WritePHYRegister(Port, QD_PHY_CONTROL_REG,data);
				}break;
		}
	}
}

/*state disable*/
else{
	/*power down*/
	data=ENABLE;
	hwSetPhyRegField(Port, QD_PHY_CONTROL_REG,11,1,data);
}
return 0;
}

//only for 88e6095
uint8_t RateLimitConfig(void){
uint8_t port,returnP=0;

	for(uint8_t i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
		port = L2F_port_conv(i);
		/*if(get_rate_limit_mode(i)>3){
			set_rate_limit_mode(i,0);
			returnP=1;
		}*/
		/*устанавливаем режим ограничения*/
		//grcSetLimitMode(port,(GT_RATE_LIMIT_MODE)(get_rate_limit_mode(i)));
		grcSetLimitMode(port,GT_LIMT_ALL);


		/*ограничения по приоритетам для входящего трафика*/
		/*одинаковые ограничения скорости для разных приоритетов*/
		grcSetPri3Rate(port,GT_FALSE);
		grcSetPri2Rate(port,GT_FALSE);
		grcSetPri1Rate(port,GT_FALSE);

		grcSetPri0Rate(port,get_rate_limit_rx(i));

		//grcSetEgressRate(port,(GT_EGRESS_RATE)(250000/rate[i].TXrate));
		hwSetPortRegField(port,QD_REG_EGRESS_RATE_CTRL,0,12,(GT_U16)(GT_GET_RATE_LIMIT2(get_rate_limit_tx(i))));

		/*устанавливаем биты Count IFG, Count Pre в 0*/
		hwSetPortRegField(port,QD_REG_EGRESS_RATE_CTRL,12,2,0);
	}

return returnP;
}

void SWU_PortConfig(port_sett_t *port_sett){
u16 tmp;
	if(port_sett->state==ENABLE){
		//port enable
		Salsa2_WriteRegField(PORT_MAC_CTRL_REG_P0+0x400*port_sett->fport,0,ENABLE,1);

		//power up - fiber Phy
		if(is_fiber(port_sett->lport)){
			Salsa2_WritePhyReg(port_sett->lport,Salsa2_get_phyAddr(port_sett->lport),22,PAGE1);
			tmp = Salsa2_ReadPhyReg(port_sett->lport,Salsa2_get_phyAddr(port_sett->lport),0);
			tmp &=~(1<<11);
			Salsa2_WritePhyReg(port_sett->lport,Salsa2_get_phyAddr(port_sett->lport),0,tmp);
		}
		else{
			//cooper phy
			Salsa2_WritePhyReg(port_sett->lport,Salsa2_get_phyAddr(port_sett->lport),22,PAGE0);
			tmp = Salsa2_ReadPhyReg(port_sett->lport,Salsa2_get_phyAddr(port_sett->lport),0);
			tmp &=~(1<<11);
			Salsa2_WritePhyReg(port_sett->lport,Salsa2_get_phyAddr(port_sett->lport),0,tmp);
		}

		//set sgmii mode - PortType
		//Salsa2_WriteRegField(PORT_MAC_CTRL_REG_P0+0x400*hw_port,1,0x00,1);

		//enable PortBaseVlan support
		//Salsa2_WriteRegField(BRIDGE_PORT0_CTRL_REG+0x1000*port_sett->fport,18,0x01,1);


		//enable SecureVlan support
		//Salsa2_WriteRegField(BRIDGE_PORT0_CTRL_REG+0x1000*hw_port,9,0x01,1);



		//only for cooper ports
		if(port_sett->fport>FIBER_PORT_NUM){
			//autoneggotation
			if(port_sett->speed == 0){
				//enable autoneggotation
				Salsa2_WriteRegField(PORT_ANEG_CONFIG_REG+0x400*port_sett->fport,13,0x01,1);
				//anable speed aneg
				Salsa2_WriteRegField(PORT_ANEG_CONFIG_REG+0x400*port_sett->fport,7,0x01,1);
			}
			else if(SpeedConv(port_sett->speed)==10){
				//disable autoneggotation
				Salsa2_WriteRegField(PORT_ANEG_CONFIG_REG+0x400*port_sett->fport,13,0x01,1);
				//set speed
				Salsa2_WriteRegField(PORT_ANEG_CONFIG_REG+0x400*port_sett->fport,5,0x00,2);
				//set duplex
				if(DuplexConv(port_sett->speed)==FULL)
					Salsa2_WriteRegField(PORT_ANEG_CONFIG_REG+0x400*port_sett->fport,12,0x01,2);
				if(DuplexConv(port_sett->speed)==HALF)//
					Salsa2_WriteRegField(PORT_ANEG_CONFIG_REG+0x400*port_sett->fport,12,0x00,2);
			}
			else if(SpeedConv(port_sett->speed)==100){
				//set speed
				Salsa2_WriteRegField(PORT_ANEG_CONFIG_REG+0x400*port_sett->fport,5,0x01,2);
				//set duplex
				if(DuplexConv(port_sett->speed)==FULL)
					Salsa2_WriteRegField(PORT_ANEG_CONFIG_REG+0x400*port_sett->fport,12,0x01,2);
				if(DuplexConv(port_sett->speed)==HALF)//
					Salsa2_WriteRegField(PORT_ANEG_CONFIG_REG+0x400*port_sett->fport,12,0x00,2);
			}
			else if(SpeedConv(port_sett->speed)==1000){
				Salsa2_WriteRegField(PORT_ANEG_CONFIG_REG+0x400*port_sett->fport,5,0x02,2);
			}
		}


		//set flow control
		if(port_sett->flow){
			//enable flow control aneg
			Salsa2_WriteRegField(PORT_ANEG_CONFIG_REG+0x400*port_sett->fport,9,0x01,1);
		}
		else{
			//disable flow control aneg
			Salsa2_WriteRegField(PORT_ANEG_CONFIG_REG+0x400*port_sett->fport,9,0x00,1);
			//disable fc
			Salsa2_WriteRegField(PORT_ANEG_CONFIG_REG+0x400*port_sett->fport,8,0x00,1);
		}

	}
	else{
		//port disable - Salsa
		Salsa2_WriteRegField(PORT_MAC_CTRL_REG_P0+0x400*port_sett->fport,0,DISABLE,1);
		//power down - Phy
		if(is_fiber(port_sett->lport)){
			Salsa2_WritePhyReg(port_sett->lport,Salsa2_get_phyAddr(port_sett->lport),22,PAGE1);
			tmp = Salsa2_ReadPhyReg(port_sett->lport,Salsa2_get_phyAddr(port_sett->lport),0);
			tmp |= 1<<11;
			Salsa2_WritePhyReg(port_sett->lport,Salsa2_get_phyAddr(port_sett->lport),0,tmp);
		}
		else{
			Salsa2_WritePhyReg(port_sett->lport,Salsa2_get_phyAddr(port_sett->lport),22,PAGE0);
			tmp = Salsa2_ReadPhyReg(port_sett->lport,Salsa2_get_phyAddr(port_sett->lport),0);
			tmp |= 1<<11;
			Salsa2_WritePhyReg(port_sett->lport,Salsa2_get_phyAddr(port_sett->lport),0,tmp);
		}
	}

}

void get_port_config(u8 lport,port_sett_t *port_sett){
	port_sett->lport = lport;
	port_sett->fport = L2F_port_conv(lport);
	port_sett->state = get_port_sett_state(lport);
	port_sett->speed = get_port_sett_speed_dplx(lport);
	port_sett->flow	= get_port_sett_flow(lport);
}

//настройка порта
void switch_port_config(port_sett_t *port_sett){
	//printf("switch_port_config port %d, state %d\r\n",port_sett->lport, port_sett->state);

	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){
		PortConfig(port_sett);
	}
	else if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
		PSW1G_PortConfig(port_sett);
	}
	else if	(get_marvell_id() == DEV_98DX316){
		SWU_PortConfig(port_sett);
	}
}
