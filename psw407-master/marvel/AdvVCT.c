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
#include "h/driver/gtDrvSwRegs.h"
#include "h/driver/gtHwCntl.h"
#include "AdvVCT.h"
#include "debug.h"


//extern struct status_t status;

//simple cable diagnostic

GT_STATUS simpleVctTest(GT_LPORT port){
u16 regval;
u8 phyPort;
u8 cnt;


	DEBUG_MSG(VCT_DBG,"simple Cable Test Result for Port %d\r\n",(u8)port);

	phyPort = L2F_port_conv(port);

	//restart aneg
	regval = ETH_ReadIndirectPHYReg(phyPort,PAGE0,0);
	regval |=0x200;
	ETH_WriteIndirectPHYReg(phyPort,PAGE0,0,regval);
	vTaskDelay(10*MSEC);

	//ждем завершения предыдущего тестирования
	regval = ETH_ReadIndirectPHYReg(phyPort,PAGE7,21);
	DEBUG_MSG(VCT_DBG,"R21: %X\r\n",regval);

	if(!(regval & 0x800)){
		regval |= 0x8000;//start vct/ result in meters
		ETH_WriteIndirectPHYReg(phyPort,PAGE7,21,regval);
		DEBUG_MSG(VCT_DBG,"write R21: %X\r\n",regval);



		vTaskDelay(1*MSEC);
		regval = ETH_ReadIndirectPHYReg(phyPort,PAGE7,21);
		DEBUG_MSG(VCT_DBG,"read R21: %X\r\n",regval);
		//wait if bussy
		cnt=0;
		while(regval & 0x800){
			cnt++;
			regval = ETH_ReadIndirectPHYReg(phyPort,PAGE7,21);
			DEBUG_MSG(VCT_DBG,"read R21: %X\r\n",regval);
			vTaskDelay(100*MSEC);

			if(cnt>10){
				dev.port_stat[port].rx_status = VCT_BAD;
				dev.port_stat[port].tx_status = VCT_BAD;
				return 0;
			}
		}



		//get test result
		regval = ETH_ReadIndirectPHYReg(phyPort,PAGE7,20);
		DEBUG_MSG(VCT_DBG,"Read R20: %X\r\n",regval);
		//for pair 0
		if(((regval & 0x0F) == 0)||((regval & 0x0F) == 0x09))
			dev.port_stat[port].rx_status = VCT_BAD;
		if(((regval & 0x0F) == 0x03)||((regval & 0x0F) == 0x04))
			dev.port_stat[port].rx_status = VCT_SHORT;
		if((regval & 0x0F) == 0x02)
			dev.port_stat[port].rx_status = VCT_OPEN;
		if((regval & 0x0F) == 0x01)
			dev.port_stat[port].rx_status = VCT_GOOD;

		//for pair 1
		if(((regval & 0xF0) == 0)||((regval & 0xF0) == 0x90))
			dev.port_stat[port].tx_status = VCT_BAD;
		if(((regval & 0xF0) == 0x30)||((regval & 0xF0) == 0x40))
			dev.port_stat[port].tx_status = VCT_SHORT;
		if((regval & 0xF0) == 0x20)
			dev.port_stat[port].tx_status = VCT_OPEN;
		if((regval & 0xF0) == 0x10)
			dev.port_stat[port].tx_status = VCT_GOOD;

		//read len
		if((dev.port_stat[port].rx_status == VCT_OPEN)||(dev.port_stat[port].rx_status = VCT_SHORT)){
			regval = ETH_ReadIndirectPHYReg(phyPort,PAGE7,16);
			DEBUG_MSG(VCT_DBG,"R16: %X\r\n",regval);
			dev.port_stat[port].rx_len = regval;
		}

		//read len
		if((dev.port_stat[port].tx_status == VCT_OPEN)||(dev.port_stat[port].tx_status = VCT_SHORT)){
			regval = ETH_ReadIndirectPHYReg(phyPort,PAGE7,17);
			DEBUG_MSG(VCT_DBG,"R17: %X\r\n",regval);
			dev.port_stat[port].tx_len = regval;
		}

		dev.port_stat[port].vct_compleat_ok = 1;

		DEBUG_MSG(VCT_DBG,"Result for Port %d rx:%d tx:%d\r\n",
				(u8)phyPort,dev.port_stat[port].rx_status,dev.port_stat[port].tx_status);
		DEBUG_MSG(VCT_DBG,"Result for Port %d rx_len:%d tx_len:%d\r\n",
				(u8)phyPort,dev.port_stat[port].rx_len,dev.port_stat[port].tx_len);

		DEBUG_MSG(VCT_DBG,"R21 %X\r\n",ETH_ReadIndirectPHYReg(phyPort,PAGE7,21));
		DEBUG_MSG(VCT_DBG,"R20 %X\r\n",ETH_ReadIndirectPHYReg(phyPort,PAGE7,20));
		DEBUG_MSG(VCT_DBG,"R16 %X\r\n",ETH_ReadIndirectPHYReg(phyPort,PAGE7,16));
		DEBUG_MSG(VCT_DBG,"R17 %X\r\n",ETH_ReadIndirectPHYReg(phyPort,PAGE7,17));
		DEBUG_MSG(VCT_DBG,"R18 %X\r\n",ETH_ReadIndirectPHYReg(phyPort,PAGE7,18));
		DEBUG_MSG(VCT_DBG,"R19 %X\r\n",ETH_ReadIndirectPHYReg(phyPort,PAGE7,19));
	}
	else{
		DEBUG_MSG(VCT_DBG,"VCT busy\t\n");
	}
	return GT_OK;
}




/* Advanced VCT (TDR) */
GT_STATUS advVctTest(GT_LPORT port)
{
    GT_STATUS status2;
    int i, j;
    GT_ADV_VCT_MODE mode;
    GT_ADV_CABLE_STATUS advCableStatus;

    GT_ADV_VCT_MOD mod[2] = {
        GT_ADV_VCT_FIRST_PEAK,
        GT_ADV_VCT_MAX_PEAK
    };

    char modeStr[2][32] = {
        "(Adv VCT First PEAK)",
        "(Adv VCT MAX PEAK)"
    };
    DEBUG_MSG(VCT_DBG,"sample adv Cable Test Result for Port %d\r\n",(u8)port);


    for (j=0; j<2; j++)
    {
        mode.mode=mod[j];
        mode.transChanSel=GT_ADV_VCT_TCS_NO_CROSSPAIR;
        mode.sampleAvg = 0;
        mode.peakDetHyst =0;

        /*
         *    Start and get Cable Test Result
         */
        status2 = GT_OK;
        DEBUG_MSG(VCT_DBG,"sample adv Cable Test Result for Port %d\r\n",(u8)port);

        if((status2 = gvctGetAdvCableDiag(port, mode,&advCableStatus)) != GT_OK)
        {
        	DEBUG_MSG(VCT_DBG,"gvctGetAdvCableDiag return Failed\r\n");
    		dev.port_stat[F2L_port_conv(port)].rx_status = VCT_BAD;
    		dev.port_stat[F2L_port_conv(port)].tx_status = VCT_BAD;
            return status2;
        }

        DEBUG_MSG(VCT_DBG,"Cable Test Result %s for Port %i\r\n", modeStr[j], (int)port);

        for(i=0; i<GT_MDI_PAIR_NUM; i++)
        {
        	DEBUG_MSG(VCT_DBG,"MDI PAIR %i: %d %d\r\n",
     			i,advCableStatus.cableStatus[i],advCableStatus.u[i].dist2fault);
    		dev.port_stat[F2L_port_conv(port)].rx_status = VCT_BAD;
    		dev.port_stat[F2L_port_conv(port)].tx_status = VCT_BAD;
    		dev.port_stat[F2L_port_conv(port)].vct_compleat_ok = 1;
        }
    }

    return GT_OK;
}




/*******************************************************************************
* gvctGetAdvCableStatus
*
* DESCRIPTION:
*       This routine perform the advanced virtual cable test for the requested
*       port and returns the the status per MDI pair.
*
* INPUTS:
*       port - logical port number.
*       mode - advance VCT mode (either First Peak or Maximum Peak)
*
* OUTPUTS:
*       cableStatus - the port copper cable status.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       Internal Gigabit Phys in 88E6165 family and 88E6351 family devices
*        are supporting this API.
*
*******************************************************************************/
GT_STATUS gvctGetAdvCableDiag
(
    GT_LPORT        port,
    GT_ADV_VCT_MODE mode,
    GT_ADV_CABLE_STATUS *cableStatus
)
{
    GT_STATUS status;
    GT_U8 hwPort;
    GT_U16 u16Data, org0;
    GT_BOOL ppuEn;
    GT_BOOL         /*  autoOn,*/ autoNeg;
    //GT_U16            pageReg;
    //int i;

    DEBUG_MSG(VCT_DBG,"gvctGetCableDiag Called.\r\n");
    hwPort = port;

    /* check if the port is configurable */
    /*if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
    {
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }*/

    /* check if the port supports VCT */
    /*if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
    {
        DBG_INFO(("Unknown PHY device.\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }*/
    /*case DEV_E1540:*/
	/*phyInfo->anyPage = 0x0000FFFF;
	phyInfo->flag = GT_PHY_ADV_VCT_CAPABLE|GT_PHY_DTE_CAPABLE|
					GT_PHY_EX_CABLE_STATUS|
					GT_PHY_GIGABIT|
					GT_PHY_MAC_IF_LOOP|GT_PHY_LINE_LOOP|GT_PHY_EXTERNAL_LOOP|
					GT_PHY_PKT_GENERATOR;
	phyInfo->vctType = GT_PHY_ADV_VCT_TYPE2;
	phyInfo->exStatusType = GT_PHY_EX_STATUS_TYPE6;
	phyInfo->dteType = GT_PHY_DTE_TYPE4;
	phyInfo->pktGenType = GT_PHY_PKTGEN_TYPE2;
	phyInfo->macIfLoopType = GT_PHY_LOOPBACK_TYPE1;
	phyInfo->lineLoopType = 0;
	phyInfo->exLoopType = GT_PHY_EX_LB_TYPE0;
	phyInfo->pageType = GT_PHY_PAGE_WRITE_BACK;*/



    /*if (!(phyInfo.flag & GT_PHY_ADV_VCT_CAPABLE))
    {
        DBG_INFO(("Not Supported\n"));
        gtSemGive(dev,dev->phyRegsSem);
        return GT_NOT_SUPPORTED;
    }*/

    /* Need to disable PPUEn for safe. */
    if(/*gsysGetPPUEn(dev,&ppuEn) != GT_OK*/1)
    {
    	//6420 GT_NOT_SUPPORTED PPU
        ppuEn = GT_FALSE;
    }

    /*if(ppuEn != GT_FALSE)
    {
        if((status= gsysSetPPUEn(dev,GT_FALSE)) != GT_OK)
        {
            DBG_INFO(("Not able to disable PPUEn.\n"));
            gtSemGive(dev,dev->phyRegsSem);
            return status;
        }
        gtDelay(250);
    }*/

#if 1
    /*if(driverPagedAccessStart(hwPort,&pageReg) != GT_OK)
    {
        //gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }*/
#endif



#if 1
    /*
     * Check the link
     */
    u16Data = ETH_ReadIndirectPHYReg(hwPort,PAGE0,17);

    autoNeg = GT_FALSE;
    org0 = 0;
    if (!(u16Data & 0x400))
    {
    	DEBUG_MSG(VCT_DBG,"link is down\r\n");
        /* link is down, so disable auto-neg if enabled */
    	u16Data = ETH_ReadIndirectPHYReg(hwPort,PAGE0,0);

        org0 = u16Data;

        if (u16Data & 0x1000)
        {
            u16Data = 0x140;

            /* link is down, so disable auto-neg if enabled */
            ETH_WriteIndirectPHYReg(hwPort,PAGE0,0,u16Data);
            /*if((status= hwWritePagedPhyReg(hwPort,0,0,phyInfo.anyPage,u16Data)) != GT_OK)
            {
            	DEBUG_MSG(VCT_DBG,"Not able to reset the Phy.\r\n");
                return status;
            }*/

            DEBUG_MSG(VCT_DBG,"disable auto negotation\r\n");

            if((status= hwPhyReset(hwPort,0xFF)) != GT_OK)
            {
            	DEBUG_MSG(VCT_DBG,"Not able to reset the Phy.\r\n");
                return status;
            }
            autoNeg = GT_TRUE;
        }
    }

#endif

    /*switch(phyInfo.vctType)
    {
        case GT_PHY_ADV_VCT_TYPE1:
            status = getAdvCableStatus_1181(hwPort,&phyInfo,mode,cableStatus);
            break;
        case GT_PHY_ADV_VCT_TYPE2:*/
            status = getAdvCableStatus_1116(hwPort,mode,cableStatus);
            /*break;
        default:
            status = GT_FAIL;
            break;
    }*/

#if 1
    if (autoNeg)
    {
        if((status= hwPhyReset(hwPort,org0)) != GT_OK)
        {
        	DEBUG_MSG(VCT_DBG,"Not able to reset the Phy.\r\n");
            goto cableDiagCleanup;
            return status;
        }
    }
cableDiagCleanup:

    /*if(driverPagedAccessStop(hwPort,pageReg) != GT_OK)
    {
        return GT_FAIL;
    }*/

#endif
    /*if(ppuEn != GT_FALSE)
    {
        if(gsysSetPPUEn(ppuEn) != GT_OK)
        {
            DBG_INFO(("Not able to enable PPUEn.\n"));
            status = GT_FAIL;
        }
    }*/
    return status;
}


/*******************************************************************************
* hwPhyReset
*
* DESCRIPTION:
*       This function performs softreset and waits until reset completion.
*
* INPUTS:
*       portNum     - Port number to write the register for.
*       u16Data     - data should be written into Phy control register.
*                      if this value is 0xFF, normal operation occcurs (read,
*                      update, and write back.)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK on success, or
*       GT_FAIL otherwise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS hwPhyReset
(
    GT_U8        portNum,
    GT_U16        u16Data
)
{
    GT_U16 tmpData;
    GT_U32 retryCount;
    GT_BOOL    pd = GT_FALSE;

    DEBUG_MSG(VCT_DBG,"Reset bit\r\n");

    tmpData = ETH_ReadIndirectPHYReg(portNum,PAGE0,0);

    if (tmpData & 0x800)
    {
        pd = GT_TRUE;
    }

    if (u16Data != 0xFF)
    {
        tmpData = u16Data;
    }

    /* Set the desired bits to 0. */
    if (pd)
    {
        tmpData |= 0x800;
    }
    else
    {
	  if(((tmpData&0x4000)==0)||(u16Data==0xFF)) /* setting loopback do not set reset */
        tmpData |= 0x8000;
    }

    ETH_WriteIndirectPHYReg(portNum,PAGE0,0,tmpData);


    if (pd)
    {
        return GT_OK;
    }

    for (retryCount = 0x1000; retryCount > 0; retryCount--)
    {
    	tmpData = ETH_ReadIndirectPHYReg(portNum,PAGE0,0);
        if ((tmpData & 0x8000) == 0)
            break;
    }

    if (retryCount == 0)
    {
    	DEBUG_MSG(VCT_DBG,"Reset bit is not cleared\r\n");
        return GT_FAIL;
    }

    DEBUG_MSG(VCT_DBG,"Reset bit OK\r\n");
    return GT_OK;
}


/*******************************************************************************
* driverPagedAccessStart
*
* DESCRIPTION:
*       This function stores page register and Auto Reg Selection mode if needed.
*
* INPUTS:
*       hwPort     - port number where the Phy is connected
*        pageType - type of the page registers
*
* OUTPUTS:
*       autoOn    - GT_TRUE if Auto Reg Selection enabled, GT_FALSE otherwise.
*        pageReg - Page Register Data
*
* RETURNS:
*       GT_OK     - if success
*       GT_FAIL - othrwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
/*GT_STATUS driverPagedAccessStart
(
    IN    GT_U8         hwPort,
    IN    GT_U8         pageType,
    OUT    GT_BOOL         *autoOn,
    OUT    GT_U16         *pageReg
)
{
    GT_U16 data;
    GT_STATUS status;

    if((status= hwGetPhyRegField(dev,hwPort,22,0,8,pageReg)) != GT_OK)
    {
    	DEBUG_MSG(VCT_DBG,"Not able to read Phy Register.\r\n");
        return status;
    }

    return GT_OK;
}*/


/*******************************************************************************
* driverPagedAccessStop
*
* DESCRIPTION:
*       This function restores page register and Auto Reg Selection mode if needed.
*
* INPUTS:
*       hwPort     - port number where the Phy is connected
*        pageType - type of the page registers
*       autoOn     - GT_TRUE if Auto Reg Selection enabled, GT_FALSE otherwise.
*        pageReg  - Page Register Data
*
* OUTPUTS:
*        None.
*
* RETURNS:
*       GT_OK     - if success
*       GT_FAIL - othrwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
/*GT_STATUS driverPagedAccessStop
(
    GT_U8         hwPort,
    GT_U8         pageType,
    GT_BOOL         autoOn,
    GT_U16         pageReg
)
{
    GT_U16 data;
    GT_STATUS status;

    if((status= hwSetPhyRegField(dev,hwPort,22,0,8,pageReg)) != GT_OK)
    {
    	DEBUG_MSG(VCT_DBG,"Not able to write Phy Register.\n");
        return status;
    }

    return GT_OK;
}*/

GT_STATUS getAdvCableStatus_1116
(
    GT_U8           hwPort,
    GT_ADV_VCT_MODE mode,
    GT_ADV_CABLE_STATUS *cableStatus
)
{
    GT_STATUS retVal;
    GT_U16 orgPulse, u16Data;
    GT_BOOL flag, tooShort;
    GT_ADV_VCT_TRANS_CHAN_SEL crosspair;

    flag = GT_TRUE;
    crosspair = mode.transChanSel;

    /*
     * Check Adv VCT Mode
     */
    switch (mode.mode)
    {
        case GT_ADV_VCT_FIRST_PEAK:
        case GT_ADV_VCT_MAX_PEAK:
                break;

        default:
        	DEBUG_MSG(VCT_DBG,"Unknown ADV VCT Mode.\r\n");
            return GT_NOT_SUPPORTED;
    }


    u16Data = ETH_ReadIndirectPHYReg(hwPort,PAGE5,QD_REG_ADV_VCT_CONTROL_5);
    DEBUG_MSG(VCT_DBG,"Reading QD_REG_ADV_VCT_CONTROL_5 %x\r\n",u16Data);

    /*if((retVal = hwGetPagedPhyRegField(hwPort,5,QD_REG_ADV_VCT_CONTROL_5,0,13,phyInfo->anyPage,&u16Data)) != GT_OK)
    {
    	DEBUG_MSG(VCT_DBG,"Reading paged phy reg failed.\n");
        return retVal;
    }*/

    u16Data |= ((mode.mode<<6) | (mode.transChanSel<<11));
    if (mode.peakDetHyst)
    	u16Data |= (mode.peakDetHyst);
    if (mode.sampleAvg)
    	u16Data |= (mode.sampleAvg<<8) ;

    /*if((retVal = hwSetPagedPhyRegField(
                        dev,hwPort,5,QD_REG_ADV_VCT_CONTROL_5,0,13,phyInfo->anyPage,u16Data)) != GT_OK)
    {
    	DEBUG_MSG(VCT_DBG,"Writing to paged phy reg failed.\r\n");
        return retVal;
    }*/
    DEBUG_MSG(VCT_DBG,"ETH_WriteIndirectPHYReg P%d R%d D%d\r\n",PAGE5,QD_REG_ADV_VCT_CONTROL_5,u16Data);
    ETH_WriteIndirectPHYReg(hwPort,PAGE5,QD_REG_ADV_VCT_CONTROL_5,u16Data);

    if (flag)
    {
        /* save original Pulse Width */
    	orgPulse = ETH_ReadIndirectPHYReg(hwPort,PAGE5,28);
    	//orgPulse = (orgPulse>>10) & 3;
        /*if((retVal = hwGetPagedPhyRegField(dev,hwPort,5,28,10,2,phyInfo->anyPage,&orgPulse)) != GT_OK)
        {
        	DEBUG_MSG(VCT_DBG,"Reading paged phy reg failed.\r\n");
            return retVal;
        }*/

    	orgPulse = (orgPulse>>10) & 0x03;

        /* set the Pulse Width with default value */
        if (orgPulse != 0)
        {
        	//u16Data = orgPulse;
        	u16Data = ETH_ReadIndirectPHYReg(hwPort,PAGE5,28);
        	u16Data &= 0xF3FF;//0
        	ETH_WriteIndirectPHYReg(hwPort,PAGE5,28,u16Data);
        }
        tooShort=GT_FALSE;
    }

    if((retVal=runAdvCableTest_1116(hwPort,flag,crosspair,cableStatus,&tooShort)) != GT_OK)
    {
    	DEBUG_MSG(VCT_DBG,"Running advanced VCT failed.\r\n");
        return retVal;
    }

    if (flag)
    {
        if(tooShort)
        {
            /* set the Pulse Width with minimum width */
        	u16Data = ETH_ReadIndirectPHYReg(hwPort,PAGE5,28);
        	//u16Data = orgPulse;
        	u16Data |= 0xC00;
        	ETH_WriteIndirectPHYReg(hwPort,PAGE5,28,u16Data);
            /*if((retVal = hwSetPagedPhyRegField(hwPort,5,28,10,2,phyInfo->anyPage,3)) != GT_OK)
            {
            	DEBUG_MSG(VCT_DBG,"Writing to paged phy reg failed.\r\n");
                return retVal;
            }*/

        	DEBUG_MSG(VCT_DBG,"run the Adv VCT again\r\n");

            /* run the Adv VCT again */
            if((retVal=runAdvCableTest_1116(hwPort,GT_FALSE,crosspair,
                                        cableStatus,&tooShort)) != GT_OK)
            {
            	DEBUG_MSG(VCT_DBG,"Running advanced VCT failed.\r\n");
                return retVal;
            }

        }

        /* set the Pulse Width back to the original value */
        u16Data = ETH_ReadIndirectPHYReg(hwPort,PAGE5,28);
        u16Data &= ~0xC00;
        u16Data |= orgPulse<<10;
    	ETH_WriteIndirectPHYReg(hwPort,PAGE5,28,u16Data);
    }
    return GT_OK;
}


GT_STATUS runAdvCableTest_1116
(
    GT_U8           hwPort,
    GT_BOOL         mode,
    GT_ADV_VCT_TRANS_CHAN_SEL   crosspair,
    GT_ADV_CABLE_STATUS *cableStatus,
    GT_BOOL         *tooShort
)
{
    GT_STATUS retVal;
    GT_32  channel;

    DEBUG_MSG(VCT_DBG,"runAdvCableTest_1116 Called. port = %d\r\n",hwPort);

    if (crosspair!=GT_ADV_VCT_TCS_NO_CROSSPAIR)
    {
        channel = crosspair - GT_ADV_VCT_TCS_CROSSPAIR_0;
    }
    else
    {
        channel = 0;
    }

    /* Set transmit channel */
    if((retVal=runAdvCableTest_1116_set(hwPort, channel, crosspair)) != GT_OK)
    {
    	DEBUG_MSG(VCT_DBG,"Running advanced VCT failed.\r\n");
        return retVal;
    }

    /*
     * check test completion
     */
    retVal = runAdvCableTest_1116_check(hwPort);
    if (retVal != GT_OK)
    {
    	DEBUG_MSG(VCT_DBG,"Running advanced VCT failed.\r\n");
        return retVal;
    }

    /*
     * read the test result for the cross pair against selected MDI Pair
     */
    retVal = runAdvCableTest_1116_get(hwPort,crosspair,channel,cableStatus,(GT_BOOL *)tooShort);

    if(retVal != GT_OK)
    {
    	DEBUG_MSG(VCT_DBG,"Running advanced VCT get failed.\r\n");
    }
    return retVal;
}


GT_STATUS runAdvCableTest_1116_get
(
    GT_U8           hwPort,
    GT_ADV_VCT_TRANS_CHAN_SEL    crosspair,
    GT_32            channel,
    GT_ADV_CABLE_STATUS *cableStatus,
    GT_BOOL         *tooShort
)
{

    GT_U16 u16Data;
    GT_U16 crossChannelReg[GT_MDI_PAIR_NUM];
    int j;
    GT_16  dist2fault;
    GT_BOOL         mode;
    GT_BOOL         localTooShort[GT_MDI_PAIR_NUM];

    SW_VCT_REGISTER regList[GT_MDI_PAIR_NUM] = { {5,16},{5,17},{5,18},{5,19} };

    mode = GT_TRUE;

    DEBUG_MSG(VCT_DBG,"runAdvCableTest_1116_get Called.\r\n");

    u16Data = ETH_ReadIndirectPHYReg(hwPort,PAGE5,QD_REG_ADV_VCT_CONTROL_5);


    DEBUG_MSG(VCT_DBG,"Page 5 of Reg23 after test : %0#x.\r\n", u16Data);

    /*
     * read the test result for the cross pair against selected MDI Pair
     */
    for (j=0; j<GT_MDI_PAIR_NUM; j++)
    {

    	crossChannelReg[j] = ETH_ReadIndirectPHYReg(hwPort,regList[j].page,regList[j].regOffset);
        /*if((retVal = hwReadPagedPhyReg(
                                dev,hwPort,
                                regList[j].page,
                                regList[j].regOffset,
                                phyInfo->anyPage,
                                &crossChannelReg[j])) != GT_OK)
        {
        	DEBUG_MSG(VCT_DBG,"Reading from paged phy reg failed.\r\n");
            return retVal;
        }*/
    	DEBUG_MSG(VCT_DBG,"@@@@@ reg channel %d is %x \r\n", j, crossChannelReg[j]);
    }

    /*
     * analyze the test result for RX Pair
     */
    for (j=0; j<GT_MDI_PAIR_NUM; j++)
    {
        if (crosspair!=GT_ADV_VCT_TCS_NO_CROSSPAIR)
            dist2fault = analizeAdvVCTResult(j, crossChannelReg, mode&(*tooShort), cableStatus);
        else
            dist2fault = analizeAdvVCTNoCrosspairResult(j, crossChannelReg, mode&(*tooShort), cableStatus);

        localTooShort[j]=GT_FALSE;
        if((mode)&&(*tooShort==GT_FALSE))
        {
            if ((dist2fault>=0) && (dist2fault<GT_ADV_VCT_ACCEPTABLE_SHORT_CABLE))
            {
            	DEBUG_MSG(VCT_DBG,"@@@#@@@@ it is too short dist2fault %d\r\n", dist2fault);
            	DEBUG_MSG(VCT_DBG,"Distance to Fault is too Short. So, rerun after changing pulse width\r\n");
                localTooShort[j]=GT_TRUE;
            }
        }
    }

    /* check and decide if length is too short */
    for (j=0; j<GT_MDI_PAIR_NUM; j++)
    {
        if (localTooShort[j]==GT_FALSE) break;
    }

    if (j==GT_MDI_PAIR_NUM)
        *tooShort = GT_TRUE;

    return GT_OK;
}


/*******************************************************************************
* analizeAdvVCTResult
*
* DESCRIPTION:
*        This routine analize the Advanced VCT result.
*
* INPUTS:
*        channel - channel number where test was run
*        crossChannelReg - register values after the test is completed
*        mode    - use formula for normal cable case
*
* OUTPUTS:
*        cableStatus - analized test result.
*
* RETURNS:
*        -1, or distance to fault
*
* COMMENTS:
*        None.
*
*******************************************************************************/
GT_16 analizeAdvVCTNoCrosspairResult
(
    int     channel,
    GT_U16 *crossChannelReg,
    GT_BOOL isShort,
    GT_ADV_CABLE_STATUS *cableStatus
)
{
    int len;
    GT_16 dist2fault;
    GT_ADV_VCT_STATUS vctResult = GT_ADV_VCT_NORMAL;

    DEBUG_MSG(VCT_DBG,"analizeAdvVCTNoCrosspairResult Called.\r\n");
    DEBUG_MSG(VCT_DBG,"analizeAdvVCTNoCrosspairResult chan %d reg data %x\r\n", channel, crossChannelReg[channel]);

    dist2fault = -1;

    /* check if test is failed */
    if(IS_VCT_FAILED(crossChannelReg[channel]))
    {
        cableStatus->cableStatus[channel] = GT_ADV_VCT_FAIL;
        return dist2fault;
    }

    /* Check if fault detected */
    if(IS_ZERO_AMPLITUDE(crossChannelReg[channel]))
    {
        cableStatus->cableStatus[channel] = GT_ADV_VCT_NORMAL;
        return dist2fault;
    }

    /* find out test result by reading Amplitude */
    if(IS_POSITIVE_AMPLITUDE(crossChannelReg[channel]))
    {
        vctResult = GT_ADV_VCT_IMP_GREATER_THAN_115;
    }
    else
    {
        vctResult = GT_ADV_VCT_IMP_LESS_THAN_85;
    }

    /*
     * now, calculate the distance for GT_ADV_VCT_IMP_GREATER_THAN_115 and
     * GT_ADV_VCT_IMP_LESS_THAN_85
     */
    switch (vctResult)
    {
        case GT_ADV_VCT_IMP_GREATER_THAN_115:
        case GT_ADV_VCT_IMP_LESS_THAN_85:
            if(!isShort)
            {
                len = (int)GT_ADV_VCT_CALC(crossChannelReg[channel] & 0xFF);
            }
            else
            {
                len = (int)GT_ADV_VCT_CALC_SHORT(crossChannelReg[channel] & 0xFF);
            }
            DEBUG_MSG(VCT_DBG,"@@@@ no cross len %d\r\n", len);

            if (len < 0)
                len = 0;
            cableStatus->u[channel].dist2fault = (GT_16)len;
            vctResult = getDetailedAdvVCTResult(
            	                    GET_AMPLITUDE(crossChannelReg[channel]),
                                    len,
                                    vctResult);
            dist2fault = (GT_16)len;
            break;
        default:
            break;
    }

    cableStatus->cableStatus[channel] = vctResult;

    return dist2fault;
}


GT_16 analizeAdvVCTResult
(
    int     channel,
    GT_U16 *crossChannelReg,
    GT_BOOL isShort,
    GT_ADV_CABLE_STATUS *cableStatus
)
{
    int i, len;
    GT_16 dist2fault;
    GT_ADV_VCT_STATUS vctResult = GT_ADV_VCT_NORMAL;

    DEBUG_MSG(VCT_DBG,"analizeAdvVCTResult(Crosspair) chan %d reg data %x\r\n", channel, crossChannelReg[channel]);
    DEBUG_MSG(VCT_DBG,"analizeAdvVCTResult Called.\r\n");

    dist2fault = -1;

    /* check if test is failed */
    for (i=0; i<GT_MDI_PAIR_NUM; i++)
    {
        if(IS_VCT_FAILED(crossChannelReg[i]))
        {
            cableStatus->cableStatus[channel] = GT_ADV_VCT_FAIL;
            return dist2fault;
        }
    }

    /* find out test result by reading Amplitude */
    for (i=0; i<GT_MDI_PAIR_NUM; i++)
    {
        if (i == channel)
        {
            if(!IS_ZERO_AMPLITUDE(crossChannelReg[i]))
            {
                if(IS_POSITIVE_AMPLITUDE(crossChannelReg[i]))
                {
                    vctResult = GT_ADV_VCT_IMP_GREATER_THAN_115;
                }
                else
                {
                    vctResult = GT_ADV_VCT_IMP_LESS_THAN_85;
                }
            }
            continue;
        }

        if(IS_ZERO_AMPLITUDE(crossChannelReg[i]))
            continue;

        vctResult = GT_ADV_VCT_CROSS_PAIR_SHORT;
        break;
    }

    /* if it is cross pair short, check the distance for each channel */
    if(vctResult == GT_ADV_VCT_CROSS_PAIR_SHORT)
    {
        cableStatus->cableStatus[channel] = GT_ADV_VCT_CROSS_PAIR_SHORT;
        for (i=0; i<GT_MDI_PAIR_NUM; i++)
        {
            if(IS_ZERO_AMPLITUDE(crossChannelReg[i]))
            {
                cableStatus->u[channel].crossShort.channel[i] = GT_FALSE;
                cableStatus->u[channel].crossShort.dist2fault[i] = 0;
                continue;
            }

            cableStatus->u[channel].crossShort.channel[i] = GT_TRUE;
            if(!isShort)
                len = (int)GT_ADV_VCT_CALC(crossChannelReg[i] & 0xFF);
            else
                len = (int)GT_ADV_VCT_CALC_SHORT(crossChannelReg[i] & 0xFF);
            DEBUG_MSG(VCT_DBG,"@@@@ len %d\r\n", len);

            if (len < 0)
                len = 0;
            cableStatus->u[channel].crossShort.dist2fault[i] = (GT_16)len;
            dist2fault = (GT_16)len;
        }

        return dist2fault;
    }

    /*
     * now, calculate the distance for GT_ADV_VCT_IMP_GREATER_THAN_115 and
     * GT_ADV_VCT_IMP_LESS_THAN_85
     */
    switch (vctResult)
    {
        case GT_ADV_VCT_IMP_GREATER_THAN_115:
        case GT_ADV_VCT_IMP_LESS_THAN_85:
            if(isShort)
                len = (int)GT_ADV_VCT_CALC(crossChannelReg[channel] & 0xFF);
            else
                len = (int)GT_ADV_VCT_CALC_SHORT(crossChannelReg[channel] & 0xFF);
            if (len < 0)
                len = 0;
            cableStatus->u[channel].dist2fault = (GT_16)len;
            vctResult = getDetailedAdvVCTResult(
                                    GET_AMPLITUDE(crossChannelReg[channel]),
                                    len,
                                    vctResult);
            dist2fault = (GT_16)len;
            break;
        default:
            break;
    }

    cableStatus->cableStatus[channel] = vctResult;

    return dist2fault;
}


/*******************************************************************************
* getDetailedAdvVCTResult
*
* DESCRIPTION:
*        This routine differenciate Open/Short from Impedance mismatch.
*
* INPUTS:
*        amp - amplitude
*        len - distance to fault
*        vctResult - test result
*                    (Impedance mismatch, either > 115 ohms, or < 85 ohms)
*
* OUTPUTS:
*
* RETURNS:
*       GT_ADV_VCT_STATUS
*
* COMMENTS:
*       This routine assumes test result is not normal nor cross pair short.
*
*******************************************************************************/
GT_ADV_VCT_STATUS getDetailedAdvVCTResult
(
    GT_U32  amp,
    GT_U32  len,
    GT_ADV_VCT_STATUS result
)
{
    GT_ADV_VCT_STATUS vctResult;
    GT_BOOL    update = GT_FALSE;

    DEBUG_MSG(VCT_DBG,"getDetailedAdvVCTResult Called.\r\n");

    if (/*devType == GT_PHY_ADV_VCT_TYPE2*/1)
    {
        if(len < 10)
        {
            if(amp > 54)  /* 90 x 0.6 */
                update = GT_TRUE;
        }
        else if(len < 50)
        {
            if(amp > 42) /* 70 x 0.6 */
                update = GT_TRUE;
        }
        else if(len < 110)
        {
            if(amp > 30)  /* 50 x 0.6 */
                update = GT_TRUE;
        }
        else if(len < 140)
        {
            if(amp > 24)  /* 40 x 0.6 */
                update = GT_TRUE;
        }
        else
        {
            if(amp > 18) /* 30 x 0.6 */
                update = GT_TRUE;
        }
    }
    else
    {
        if(len < 10)
        {
            if(amp > 90)
                update = GT_TRUE;
        }
        else if(len < 50)
        {
            if(amp > 70)
                update = GT_TRUE;
        }
        else if(len < 110)
        {
            if(amp > 50)
                update = GT_TRUE;
        }
        else if(len < 140)
        {
            if(amp > 40)
                update = GT_TRUE;
        }
        else
        {
            if(amp > 30)
                update = GT_TRUE;
        }
    }


    switch (result)
    {
        case GT_ADV_VCT_IMP_GREATER_THAN_115:
                if(update)
                    vctResult = GT_ADV_VCT_OPEN;
                else
                    vctResult = result;
                break;
        case GT_ADV_VCT_IMP_LESS_THAN_85:
                if(update)
                    vctResult = GT_ADV_VCT_SHORT;
                else
                    vctResult = result;
                break;
        default:
                vctResult = result;
                break;
    }

    return vctResult;
}


GT_STATUS runAdvCableTest_1116_check
(
	GT_U8           hwPort
)
{

    GT_U16 u16Data;
    DEBUG_MSG(VCT_DBG,"wait..\r\n");
    /*
     * loop until test completion and result is valid
     */
    do {
    	u16Data = ETH_ReadIndirectPHYReg(hwPort,PAGE5,QD_REG_ADV_VCT_CONTROL_5);

    } while (u16Data & 0x8000);
    DEBUG_MSG(VCT_DBG,"wait..\r\n");
    return GT_OK;
}



GT_STATUS runAdvCableTest_1116_set
(
	GT_U8           hwPort,
	GT_32           channel,
    GT_ADV_VCT_TRANS_CHAN_SEL        crosspair

)
{
	u16 u16Data;


    DEBUG_MSG(VCT_DBG,"runAdvCableTest_1116_set Called.\r\n");

    /*
     * start Advanced Virtual Cable Tester
     */

    u16Data = ETH_ReadIndirectPHYReg(hwPort,PAGE5,QD_REG_ADV_VCT_CONTROL_5);
    u16Data |= 0x8000;
    ETH_WriteIndirectPHYReg(hwPort,PAGE5,QD_REG_ADV_VCT_CONTROL_5,u16Data);

    DEBUG_MSG(VCT_DBG,"Writing to paged phy reg data: %X\r\n",u16Data);
    return GT_OK;
}

/*******************************************************************************
* driverPagedAccessStart
*
* DESCRIPTION:
*       This function stores page register and Auto Reg Selection mode if needed.
*
* INPUTS:
*       hwPort     - port number where the Phy is connected
*        pageType - type of the page registers
*
* OUTPUTS:
*       autoOn    - GT_TRUE if Auto Reg Selection enabled, GT_FALSE otherwise.
*        pageReg - Page Register Data
*
* RETURNS:
*       GT_OK     - if success
*       GT_FAIL - othrwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS driverPagedAccessStart
(
    GT_U8         hwPort,
    GT_U16         *pageReg
)
{
    *pageReg = (ETH_ReadIndirectPHYReg(hwPort,PAGE0,22) & 0xFF);

    return GT_OK;
}


/*******************************************************************************
* driverPagedAccessStop
*
* DESCRIPTION:
*       This function restores page register and Auto Reg Selection mode if needed.
*
* INPUTS:
*       hwPort     - port number where the Phy is connected
*        pageType - type of the page registers
*       autoOn     - GT_TRUE if Auto Reg Selection enabled, GT_FALSE otherwise.
*        pageReg  - Page Register Data
*
* OUTPUTS:
*        None.
*
* RETURNS:
*       GT_OK     - if success
*       GT_FAIL - othrwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS driverPagedAccessStop
(
    GT_U8         hwPort,
    GT_U16         pageReg
)
{

    hwSetPhyRegField6240(hwPort,22,PAGE0,0,8,pageReg);
    return GT_OK;
}
