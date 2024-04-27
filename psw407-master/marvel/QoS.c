#include "stm32f4x7_eth.h"
#include "SMIApi.h"
#include "SpeedDuplex.h"
#include "FlowCtrl.h"
#include "FreeRTOS.h"
#include "task.h"
#include "QoS.h"
#include "VLAN.h"
#include "Salsa2Regs.h"
#include "board.h"
#include "settings.h"
#include "debug.h"

#define QD_REG_PVID            0x7
#define QD_REG_IP_PRI_BASE        0x10
#define QD_REG_IEEE_PRI            0x18
#define QD_REG_PORT_CONTROL2        0x8
#define QD_REG_PORT_CONTROL1        0x5
#define QD_REG_PORT_CONTROL        0x4

#define GLOBAL_REG_ACCESS    3

/* This macro is used to calculate the register's SMI   */
/* device address, according to the baseAddr            */
/* field in the Switch configuration struct.            */
//extern GT_U8 portToSmiMapping(GT_QD_DEV *dev, GT_U8 portNum, GT_U32 accessType);
#define CALC_SMI_DEV_ADDR(_portNum, _accessType)        \
            portToSmiMapping(_portNum, _accessType)

/* This macro calculates the mask for partial read /    */
/* write of register's data.                            */
#define CALC_MASK(fieldOffset,fieldLen,mask)        \
            if((fieldLen + fieldOffset) >= 16)      \
                mask = (0 - (1 << fieldOffset));    \
            else                                    \
                mask = (((1 << (fieldLen + fieldOffset))) - (1 << fieldOffset))


/* The following macro converts a boolean   */
/* value to a binary one (of 1 bit).        */
/* GT_FALSE --> 0                           */
/* GT_TRUE --> 1                            */
#define BOOL_2_BIT(boolVal,binVal)                                  \
            (binVal) = (((boolVal) == GT_TRUE) ? 1 : 0)

GT_STATUS miiSmiIfReadRegister
(
    IN  GT_U8        phyAddr,
    IN  GT_U8        regAddr,
    OUT GT_U16       *data
)
{
    unsigned int tmpData=0;
#ifdef GT_RMGMT_ACCESS
    if((dev->accessMode == SMI_MULTI_ADDR_MODE) &&
       (dev->fgtHwAccessMod == HW_ACCESS_MODE_SMI))
#else
    if(/*dev->accessMode == SMI_MULTI_ADDR_MODE*/0)
#endif
    {
//         if(qdMultiAddrRead(dev,(GT_U32)phyAddr,(GT_U32)regAddr,&tmpData) != GT_TRUE)
//        {
//            return GT_FAIL;
//        }
    }
    else
    {
         //if(fgtReadMii((GT_U32)phyAddr,(GT_U32)regAddr,&tmpData) != GT_TRUE)
         //ETH_WritePHYRegister(phyAddr,regAddr,tmpData);
    	tmpData=ETH_ReadPHYRegister(phyAddr,regAddr);
    }
    *data = (GT_U16)tmpData;
    return GT_OK;
}
GT_STATUS hwSetGlobalRegField
(
    IN  GT_U8    regAddr,
    IN  GT_U8    fieldOffset,
    IN  GT_U8    fieldLength,
    IN  GT_U16   data
)
{
    GT_U16 mask;
    GT_U16 tmpData;
   // GT_STATUS   retVal;
    GT_U8       phyAddr;

    phyAddr = CALC_SMI_DEV_ADDR(0, GLOBAL_REG_ACCESS);
    if (phyAddr == 0xFF)
    {
        return GT_BAD_PARAM;
    }


    //retVal =  miiSmiIfReadRegister(phyAddr,regAddr,&tmpData);
    tmpData =  ETH_ReadPHYRegister(phyAddr,regAddr);

//    if(retVal != GT_OK)
//    {
//        gtSemGive(dev,dev->multiAddrSem);
//        return retVal;
//    }

    CALC_MASK(fieldOffset,fieldLength,mask);

    /* Set the desired bits to 0.                       */
    tmpData &= ~mask;
    /* Set the given data into the above reset bits.    */
    tmpData |= ((data << fieldOffset) & mask);

    DEBUG_MSG(DEBUG_QD,"Write to global register: regAddr 0x%x, fieldOff %d, fieldLen %d, data 0x%x.\r\n",
    		regAddr,fieldOffset,fieldLength,data);
    //retVal = miiSmiIfWriteRegister(phyAddr,regAddr,tmpData);
    ETH_WritePHYRegister(phyAddr,regAddr,tmpData);
    return 0;
}


/*******************************************************************************
* gcosSetPortDefaultTc
*
* DESCRIPTION:
*       Sets the default traffic class for a specific port.
*
* INPUTS:
*       port      - logical port number
*       trafClass - default traffic class of a port.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       Fast Ethernet switch family supports 2 bits (0 ~ 3) while Gigabit Switch
*        family supports 3 bits (0 ~ 7)
*
* GalTis:
*
*******************************************************************************/
GT_STATUS
gcosSetPortDefaultTc
(
    IN GT_LPORT   port,
    IN GT_U8      trafClass
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           hwPort;         /* the physical port number     */

    DEBUG_MSG(DEBUG_QD,"gcosSetPortDefaultTc Called.\r\n");
    /* translate LPORT to hardware port */
    hwPort = lport2port(port);

    /* check if device supports this feature */
    //if((retVal = IS_VALID_API_CALL(dev,hwPort, DEV_QoS)) != GT_OK )
    //  return retVal;

    /* Only Gigabit Switch supports this status. */
   // if ((IS_IN_DEV_GROUP(dev,DEV_GIGABIT_SWITCH)) ||
   //     (IS_IN_DEV_GROUP(dev,DEV_ENHANCED_FE_SWITCH)) ||
   //	(IS_IN_DEV_GROUP(dev,DEV_FE_AVB_FAMILY)))
   // {
        /* Set the default port pri.  */
        retVal = hwSetPortRegField(hwPort,QD_REG_PVID,13,3,trafClass);

        //printf("gcosSetPortDefaultTc: port %d, class %d\r\n",hwPort,trafClass);
    //}
    //else
    //{
        /* Set the default port pri.  */
    //    retVal = hwSetPortRegField(hwPort,QD_REG_PVID,14,2,trafClass);
    //}

    if(retVal != GT_OK)
    {
    	DEBUG_MSG(DEBUG_QD,"Failed.\r\n");
    }
    else
    {
    	DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    }
    return retVal;
}
/*******************************************************************************
* gcosSetDscp2Tc
*
* DESCRIPTION:
*       This routine sets the traffic class assigned for a specific
*       IPv4 Dscp.
*
* INPUTS:
*       dscp    - the IPv4 frame dscp to map.
*       trClass - the Traffic Class the received frame is assigned.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None.
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gcosSetDscp2Tc
(
    IN GT_U8      dscp,
    IN GT_U8      trClass
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           bitOffset;      /* the bit offset in the reg    */
    GT_U8           regOffset;      /* the reg offset in the IP tbl */

    DEBUG_MSG(DEBUG_QD,"gcosSetDscp2Tc Called.\r\n");
    /* check if device supports this feature */
    //if(!IS_IN_DEV_GROUP(dev,DEV_QoS))
    //    return GT_NOT_SUPPORTED;

    /* calc the bit offset */
    bitOffset = (((dscp & 0x3f) % 8) * 2);
    regOffset = ((dscp & 0x3f) / 8);
    /* Set the traffic class for the IP dscp.  */
    retVal = hwSetGlobalRegField((GT_U8)(QD_REG_IP_PRI_BASE+regOffset),
                                 bitOffset, 2, trClass);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(DEBUG_QD,"Failed.\r\n");
    }
    else
    {
    	DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    }

    return retVal;
}

/*******************************************************************************
* gcosSetUserPrio2Tc
*
* DESCRIPTION:
*       Sets the traffic class number for a specific 802.1p user priority.
*
* INPUTS:
*       userPrior - user priority of a port.
*       trClass   - the Traffic Class the received frame is assigned.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None.
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gcosSetUserPrio2Tc
(
    IN GT_U8      userPrior,
    IN GT_U8      trClass
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           bitOffset;      /* the bit offset in the reg    */

    DEBUG_MSG(DEBUG_QD,"gcosSetUserPrio2Tc Called.\r\n");
    /* check if device supports this feature */
    //if(!IS_IN_DEV_GROUP(dev,DEV_QoS))
    //    return GT_NOT_SUPPORTED;

    /* calc the bit offset */
    bitOffset = ((userPrior & 0x7) * 2);
    /* Set the traffic class for the VPT.  */
    retVal = hwSetGlobalRegField(QD_REG_IEEE_PRI, bitOffset,2,trClass);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(DEBUG_QD,"Failed.\r\n");
    }
    else
    {
    	DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    }

    return retVal;
}



/*******************************************************************************
* gqosIpPrioMapEn
*
* DESCRIPTION:
*       This routine enables the IP priority mapping.
*
* INPUTS:
*       port - the logical port number.
*       en   - GT_TRUE to Enable, GT_FALSE for otherwise.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gqosIpPrioMapEn
(
    IN GT_LPORT   port,
    IN GT_BOOL    en
)
{
    GT_U16          data;           /* Used to poll the SWReset bit */
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           hwPort;         /* the physical port number     */

    DEBUG_MSG(DEBUG_QD,"gqosIpPrioMapEn Called.\r\n");
    /* translate bool to binary */
    BOOL_2_BIT(en, data);
    /* translate LPORT to hardware port */
    hwPort = lport2port(port);

    /* check if device supports this feature */
    //if((retVal = IS_VALID_API_CALL(dev,hwPort, DEV_QoS)) != GT_OK )
    //  return retVal;

    /* Set the useIp.  */
    retVal = hwSetPortRegField(hwPort, QD_REG_PORT_CONTROL,5,1,data);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(DEBUG_QD,"Failed.\r\n");
    }
    else
    {
    	DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    }

    return retVal;
}




GT_STATUS gqosUserPrioMapEn
(
    IN GT_LPORT   port,
    IN GT_BOOL    en
)
{
    GT_U16          data;           /* Used to poll the SWReset bit */
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           hwPort;         /* the physical port number     */

    DEBUG_MSG(DEBUG_QD,"gqosUserPrioMapEn Called.\r\n");
    /* translate bool to binary */
    BOOL_2_BIT(en, data);
    /* translate LPORT to hardware port */
    hwPort = lport2port(port);

    /* check if device supports this feature */
    //if((retVal = IS_VALID_API_CALL(dev,hwPort, DEV_QoS)) != GT_OK )
    //  return retVal;

    /* Set the useTag.  */
    retVal = hwSetPortRegField(hwPort, QD_REG_PORT_CONTROL,4,1,data);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(DEBUG_QD,"Failed.\r\n");
    }
    else
    {
    	DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    }

    return retVal;
}


/*******************************************************************************
* gqosSetPrioMapRule
*
* DESCRIPTION:
*       This routine sets priority mapping rule.
*        If the current frame is both IEEE 802.3ac tagged and an IPv4 or IPv6,
*        and UserPrioMap (for IEEE 802.3ac) and IPPrioMap (for IP frame) are
*        enabled, then priority selection is made based on this setup.
*        If PrioMapRule is set to GT_TRUE, UserPrioMap is used.
*        If PrioMapRule is reset to GT_FALSE, IPPrioMap is used.
*
* INPUTS:
*       port - the logical port number.
*       mode - GT_TRUE for user prio rule, GT_FALSE for otherwise.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gqosSetPrioMapRule
(
    IN GT_LPORT   port,
    IN GT_BOOL    mode
)
{
    GT_U16          data;           /* temporary data buffer */
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           hwPort;         /* the physical port number     */

    DEBUG_MSG(DEBUG_QD,"gqosSetPrioMapRule Called.\r\n");
    /* translate bool to binary */
    BOOL_2_BIT(mode, data);
    /* translate LPORT to hardware port */
    hwPort = lport2port(port);

    /* check if device supports this feature */
    //if((retVal = IS_VALID_API_CALL(dev,hwPort, DEV_QoS)) != GT_OK )
    //  return retVal;

    /* Set the TagIfBoth.  */
    retVal = hwSetPortRegField(hwPort,QD_REG_PORT_CONTROL,6,1,data);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(DEBUG_QD,"Failed.\r\n");
    }
    else
    {
    	DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    }
    return retVal;
}



/**************************************************************************************v
*
 *    sampleQoS will enable using both IEEE 802.3ac Tag and IPv4/IPv6 Traffic
 *    Class field and IEEE 802.3ac has a higher priority than IPv4/IPv6.
 *    The following is the QoS mapping programmed by sampleQos:
 *
 *    1) IEEE 802.3ac Tag (Priority 0 ~ 7, 3 bits)
 *        Priority 1~3 is using QuarterDeck Queue 0.
 *        Priority 0,4 is using QuarterDeck Queue 1.
 *        Priority 6,7 is using QuarterDeck Queue 2.
 *        Priority 5 is using QuarterDeck Queue 3.
 *    2) IPv4/IPv6 (Priority 0 ~ 63, 6 bits)
 *        Priority 0~7 is using QuaterDeck Queue 0.
 *        Priority 8~31 is using QuaterDeck Queue 1.
 *        Priority 32~55 is using QuaterDeck Queue 2.
 *        Priority 56~63 is using QuaterDeck Queue 3.
 *
 *    3) Each port's default priority is set to 1.
**************************************************************************************/
#if 0
GT_STATUS sampleQos(void)
{
    GT_STATUS status;
    GT_U8 priority;
    GT_LPORT port;

    for(port=0; port<11; port++)
    {
       /*
         *  Use IEEE Tag
         */
        if((status = gqosUserPrioMapEn(port,GT_TRUE)) != GT_OK)
        {
            MSG_PRINT(("gqosUserPrioMapEn return Failed\r\n"));
            return status;
        }

        /*
         *  Use IPv4/IPv6 priority fields (use IP)
         */
        if((status = gqosIpPrioMapEn(port,GT_TRUE)) != GT_OK)
        {
            MSG_PRINT(("gqosIpPrioMapEn return Failed\r\n"));
            return status;
        }

        /*
         *  IEEE Tag has higher priority than IP priority fields
         */
        if((status = gqosSetPrioMapRule(port,GT_TRUE)) != GT_OK)
        {
            MSG_PRINT(("gqosSetPrioMapRule return Failed\r\n"));
            return status;
        }
        MSG_PRINT(("sampleQos port %d Ok\r\n", port));
    }

    /*COS*/

    /*
     *    IEEE 802.3ac Tag (Priority 0 ~ 7, 3 bits)
     *    Priority 1~3 is using QuarterDeck Queue 0.
     *    Priority 0,4 is using QuarterDeck Queue 1.
     *    Priority 6,7 is using QuarterDeck Queue 2.
     *    Priority 5 is using QuarterDeck Queue 3.
     */

    /*    Priority 0 is using QuarterDeck Queue 1. */
    if((status = gcosSetUserPrio2Tc(0,1)) != GT_OK)
    {
        MSG_PRINT(("gcosSetUserPrio2Tc returned fail.\r\n"));
        return status;
    }

    /*    Priority 1 is using QuarterDeck Queue 0. */
    if((status = gcosSetUserPrio2Tc(1,0)) != GT_OK)
    {
        MSG_PRINT(("gcosSetUserPrio2Tc returned fail.\=r\n"));
        return status;
    }

    /*    Priority 2 is using QuarterDeck Queue 0. */
    if((status = gcosSetUserPrio2Tc(2,0)) != GT_OK)
    {
        MSG_PRINT(("gcosSetUserPrio2Tc returned fail.\r\n"));
        return status;
    }

    /*    Priority 3 is using QuarterDeck Queue 0. */
    if((status = gcosSetUserPrio2Tc(3,0)) != GT_OK)
    {
        MSG_PRINT(("gcosSetUserPrio2Tc returned fail.\r\n"));
        return status;
    }

    /*    Priority 4 is using QuarterDeck Queue 1. */
    if((status = gcosSetUserPrio2Tc(4,1)) != GT_OK)
    {
        MSG_PRINT(("gcosSetUserPrio2Tc returned fail.\r\n"));
        return status;
    }

    /*    Priority 5 is using QuarterDeck Queue 3. */
    if((status = gcosSetUserPrio2Tc(5,3)) != GT_OK)
    {
        MSG_PRINT(("gcosSetUserPrio2Tc returned fail.\r\n"));
        return status;
    }

    /*    Priority 6 is using QuarterDeck Queue 2. */
    if((status = gcosSetUserPrio2Tc(6,2)) != GT_OK)
    {
        MSG_PRINT(("gcosSetUserPrio2Tc returned fail.\r\n"));
        return status;
    }

    /*    Priority 7 is using QuarterDeck Queue 2. */
    if((status = gcosSetUserPrio2Tc(7,2)) != GT_OK)
    {
        MSG_PRINT(("gcosSetUserPrio2Tc returned fail.\r\n"));
        return status;
    }


    /*TOS*/

    /*
     *    IPv4/IPv6 (Priority 0 ~ 63, 6 bits)
     *    Priority 0~7 is using QuaterDeck Queue 0.
     *    Priority 8~31 is using QuaterDeck Queue 1.
     *    Priority 32~55 is using QuaterDeck Queue 2.
     *    Priority 56~63 is using QuaterDeck Queue 3.
    */

    /*    Priority 0~7 is using QuaterDeck Queue 0. */
    for(priority=0; priority<8; priority++)
    {
        if((status = gcosSetDscp2Tc(priority,0)) != GT_OK)
        {
            MSG_PRINT(("gcosSetDscp2Tc returned fail.\r\n"));
            return status;
        }
    }

    /*    Priority 8~31 is using QuaterDeck Queue 1. */
    for(priority=8; priority<32; priority++)
    {
        if((status = gcosSetDscp2Tc(priority,1)) != GT_OK)
        {
            MSG_PRINT(("gcosSetDscp2Tc returned fail.\r\n"));
            return status;
        }
    }

    /*    Priority 32~55 is using QuaterDeck Queue 2. */
    for(priority=32; priority<56; priority++)
    {
        if((status = gcosSetDscp2Tc(priority,2)) != GT_OK)
        {
            MSG_PRINT(("gcosSetDscp2Tc returned fail.\r\n"));
            return status;
        }
    }

    /*    Priority 56~63 is using QuaterDeck Queue 3. */
    for(priority=56; priority<64; priority++)
    {
        if((status = gcosSetDscp2Tc(priority,3)) != GT_OK)
        {
            MSG_PRINT(("gcosSetDscp2Tc returned fail.\r\n"));
            return status;
        }
    }

    /*
     * Each port's default priority is set to 1.
    */
    for(port=0; port<11; port++)
    {
        if((status = gcosSetPortDefaultTc(port,1)) != GT_OK)
        {
            MSG_PRINT(("gcosSetDscp2Tc returned fail.\r\n"));
            return status;
        }
    }

    return GT_OK;
}
#endif

uint8_t qos_set(void){
uint8_t port,i,status;
	if(get_qos_state()==1){
		DEBUG_MSG(DEBUG_QD,"qos start\r\n");

		/*set scheduling mechanism*/
		if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){
			if(get_qos_policy()==FIXED_PRI){
				hwSetGlobalRegField(0x04,11,1,FIXED_PRI);
			}
			else{
				hwSetGlobalRegField(0x04,11,1,WEIGHTED_FAIR_PRI);
			}
		}

		/*применяем параметры*/
		for(i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
			port=L2F_port_conv(i);
			/*cos state*/
			if((status = gqosUserPrioMapEn(port,get_qos_port_cos_state(i))) != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"gqosUserPrioMapEn return Failed\r\n");
				return status;
			}
			/*tos state*/
			/*
			 *  Use IPv4/IPv6 priority fields (use IP)
			 */
			if((status = gqosIpPrioMapEn(port,get_qos_port_tos_state(i))) != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"gqosIpPrioMapEn return Failed\r\n");
				return status;
			}
			/*
			 *  IEEE Tag has higher priority than IP priority fields
			 */
			if((status = gqosSetPrioMapRule(port,get_qos_port_rule(i))) != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"gqosSetPrioMapRule return Failed\r\n");
				return status;
			}
			/*set default cos priority*/
			if((status = gcosSetPortDefaultTc(port,get_qos_port_def_pri(i))) != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"gcosSetDscp2Tc returned fail.\r\n");
				return status;
			}
		}

		/*cos priority set*/
		for(i=0;i<8;i++){
			if((status = gcosSetUserPrio2Tc(i,get_qos_cos_queue(i))) != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"gcosSetUserPrio2Tc returned fail.\r\n");
				return status;
			}
		}
		/*set tos priority*/
		for(i=0;i<64;i++){
			if((status = gcosSetDscp2Tc(i,get_qos_tos_queue(i))) != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"gcosSetDscp2Tc returned fail.\r\n");
				return status;
			}
		}
		return 0;
	}
	else
		return 0;

	return 0;
}

uint8_t   SWU_qos_set(void){
u8 port;
u8 priority;
u8 i;
	DEBUG_MSG(DEBUG_QD,"SWU_qos_set\r\n");


	if(get_qos_policy()==FIXED_PRI){
		//strict
		Salsa2_WriteRegField(PROFILE0_WRR_SP_CFG_REG,0,0,4);
	}
	else{
		//WRR
		Salsa2_WriteRegField(PROFILE0_WRR_SP_CFG_REG,0,0x0F,4);
	}

	for(i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
		port=L2F_port_conv(i);

		//set default user priority
		priority = get_qos_port_def_pri(port);
		Salsa2_WriteRegField(BRIDGE_PORT0_CTRL_REG+port*0x1000,8,(priority & 0x01),1);
		Salsa2_WriteRegField(BRIDGE_PORT0_CTRL_REG+port*0x1000,12,(priority>>1 & 0x01),1);
		Salsa2_WriteRegField(BRIDGE_PORT0_CTRL_REG+port*0x1000,19,(priority>>2 & 0x01),1);


		//set WRR profile 0 for port
		Salsa2_WriteRegField(PORT0_TRANSMIT_CFG_REG+port*0x1000,8,0,1);
	}

	/*set tos priority*/
	for(i=0;i<64;i++){
		if(i%2 == 0)
			Salsa2_WriteRegField(DSCP_TO_COS_TABLE+(i/2)*0x04,2,get_qos_tos_queue(i),2);
		else
			Salsa2_WriteRegField(DSCP_TO_COS_TABLE+(i/2)*0x04,10,get_qos_tos_queue(i),2);
	}

	return GT_OK;
}
