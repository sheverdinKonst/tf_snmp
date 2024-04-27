#include "stm32f4x7_eth.h"
//#include "flash/spiflash.h"
//#include "eeprom/eeprom.h"
#include "board.h"
#include "settings.h"
//#include "eth_LWIP.h"
//#include "i2c/i2c.h"
#include "SMIApi.h"
#include "SpeedDuplex.h"
#include "FlowCtrl.h"
//#include "task.h"
//#include "fifo.h"
//#include "usb.h"

//#include <SwitchAPI/Include/msApiDefs.h>
//#include "SwitchAPI/Include/h/msApi/msApiInternal.h"
//#include <SwitchAPI/Include/msApiPrototype.h>
#include "h/driver/gtDrvSwRegs.h"
#include "h/driver/gtHwCntl.h"
#include "VLAN.h"
#include "debug.h"

/* The following macro converts a boolean   */
/* value to a binary one (of 1 bit).        */
/* GT_FALSE --> 0                           */
/* GT_TRUE --> 1                            */
#define BOOL_2_BIT(boolVal,binVal)                                  \
            (binVal) = (((boolVal) == GT_TRUE) ? 1 : 0)

//GT_STATUS driverFindPhyInformation
//(
//    IN    GT_U8         hwPort,
//    OUT    GT_PHY_INFO     *phyInfo
//)
//{
//    GT_U32 phyId;
//    GT_U16 data;
//
//    phyId = phyInfo->phyId;
//
//    switch (phyId & PHY_MODEL_MASK)
//    {
//        case DEV_E3082:
//        case DEV_MELODY:
//                phyInfo->anyPage = 0xFFFFFFFF;
//                phyInfo->flag = GT_PHY_VCT_CAPABLE|GT_PHY_DTE_CAPABLE|
//                                GT_PHY_MAC_IF_LOOP|GT_PHY_EXTERNAL_LOOP|
//                                GT_PHY_COPPER;
//                phyInfo->vctType = GT_PHY_VCT_TYPE1;
//                phyInfo->exStatusType = 0;
//                if ((phyId & PHY_REV_MASK) < 9)
//                    phyInfo->dteType = GT_PHY_DTE_TYPE1;    /* need workaround */
//                else
//                    phyInfo->dteType = GT_PHY_DTE_TYPE5;
//
//                phyInfo->pktGenType = 0;
//                phyInfo->macIfLoopType = GT_PHY_LOOPBACK_TYPE1;
//                phyInfo->lineLoopType = 0;
//                phyInfo->exLoopType = GT_PHY_EX_LB_TYPE0;
//                phyInfo->pageType = GT_PHY_NO_PAGE;
//                break;
//
//        case DEV_E104X:
//                phyInfo->anyPage = 0xFFFFFFFF;
//                phyInfo->flag = GT_PHY_VCT_CAPABLE|GT_PHY_GIGABIT|
//                                GT_PHY_MAC_IF_LOOP|GT_PHY_EXTERNAL_LOOP;
//
//                phyInfo->dteType = 0;
//                if ((phyId & PHY_REV_MASK) < 3)
//                    phyInfo->flag &= ~GT_PHY_VCT_CAPABLE; /* VCT is not supported */
//                else if ((phyId & PHY_REV_MASK) == 3)
//                    phyInfo->vctType = GT_PHY_VCT_TYPE3;    /* Need workaround */
//                else
//                    phyInfo->vctType = GT_PHY_VCT_TYPE2;
//                phyInfo->exStatusType = 0;
//
//                phyInfo->pktGenType = 0;
//                phyInfo->macIfLoopType = GT_PHY_LOOPBACK_TYPE1;
//                phyInfo->lineLoopType = 0;
//                phyInfo->exLoopType = GT_PHY_EX_LB_TYPE0;
//                phyInfo->pageType = GT_PHY_NO_PAGE;
//
//                break;
//
//        case DEV_E1111:
//                phyInfo->anyPage = 0xFFF1FE0C;
//                phyInfo->flag = GT_PHY_VCT_CAPABLE|GT_PHY_DTE_CAPABLE|
//                                GT_PHY_EX_CABLE_STATUS|
//                                GT_PHY_MAC_IF_LOOP|GT_PHY_LINE_LOOP|GT_PHY_EXTERNAL_LOOP|
//                                GT_PHY_GIGABIT|GT_PHY_RESTRICTED_PAGE;
//
//                phyInfo->vctType = GT_PHY_VCT_TYPE2;
//                phyInfo->exStatusType = GT_PHY_EX_STATUS_TYPE1;
//                if ((phyId & PHY_REV_MASK) < 2)
//                    phyInfo->dteType = GT_PHY_DTE_TYPE3;    /* Need workaround */
//                else
//                    phyInfo->dteType = GT_PHY_DTE_TYPE2;
//
//                phyInfo->pktGenType = GT_PHY_PKTGEN_TYPE1;
//                phyInfo->macIfLoopType = GT_PHY_LOOPBACK_TYPE1;
//                phyInfo->lineLoopType = 0;
//                phyInfo->exLoopType = GT_PHY_EX_LB_TYPE0;
//                phyInfo->pageType = GT_PHY_PAGE_DIS_AUTO1;
//                break;
//
//        case DEV_E1112:
//                phyInfo->anyPage = 0x1BC0780C;
//                phyInfo->flag = GT_PHY_VCT_CAPABLE|GT_PHY_DTE_CAPABLE|
//                                GT_PHY_EX_CABLE_STATUS|
//                                GT_PHY_GIGABIT|GT_PHY_RESTRICTED_PAGE|
//                                GT_PHY_MAC_IF_LOOP|GT_PHY_LINE_LOOP|GT_PHY_EXTERNAL_LOOP|
//                                GT_PHY_PKT_GENERATOR;
//
//                phyInfo->vctType = GT_PHY_VCT_TYPE4;
//                phyInfo->exStatusType = GT_PHY_EX_STATUS_TYPE2;
//                phyInfo->dteType = GT_PHY_DTE_TYPE4;
//
//                phyInfo->pktGenType = GT_PHY_PKTGEN_TYPE2;
//                phyInfo->macIfLoopType = GT_PHY_LOOPBACK_TYPE1;
//                phyInfo->lineLoopType = 0;
//                phyInfo->exLoopType = GT_PHY_EX_LB_TYPE0;
//                phyInfo->pageType = GT_PHY_PAGE_DIS_AUTO2;
//                break;
//
//        case DEV_E114X:
//                phyInfo->anyPage = 0x2FF1FE0C;
//                phyInfo->flag = GT_PHY_VCT_CAPABLE|GT_PHY_DTE_CAPABLE|
//                                GT_PHY_EX_CABLE_STATUS|
//                                GT_PHY_MAC_IF_LOOP|GT_PHY_LINE_LOOP|GT_PHY_EXTERNAL_LOOP|
//                                GT_PHY_GIGABIT|GT_PHY_RESTRICTED_PAGE;
//
//                phyInfo->vctType = GT_PHY_VCT_TYPE2;
//                phyInfo->exStatusType = GT_PHY_EX_STATUS_TYPE1;
//                if ((phyId & PHY_REV_MASK) < 4)
//                    phyInfo->dteType = GT_PHY_DTE_TYPE3;    /* Need workaround */
//                else
//                    phyInfo->dteType = GT_PHY_DTE_TYPE2;
//
//                phyInfo->pktGenType = GT_PHY_PKTGEN_TYPE1;
//                phyInfo->macIfLoopType = GT_PHY_LOOPBACK_TYPE1;
//                phyInfo->lineLoopType = 0;
//                phyInfo->exLoopType = GT_PHY_EX_LB_TYPE0;
//                phyInfo->pageType = GT_PHY_PAGE_DIS_AUTO1;
//
//                break;
//
//        case DEV_E1149:
//                phyInfo->anyPage = 0x2040FFFF;
//                phyInfo->flag = GT_PHY_VCT_CAPABLE|GT_PHY_DTE_CAPABLE|
//                                GT_PHY_EX_CABLE_STATUS|
//                                GT_PHY_GIGABIT|
//                                GT_PHY_MAC_IF_LOOP|GT_PHY_LINE_LOOP|GT_PHY_EXTERNAL_LOOP|
//                                GT_PHY_PKT_GENERATOR;
//                phyInfo->vctType = GT_PHY_VCT_TYPE4;
//                phyInfo->exStatusType = GT_PHY_EX_STATUS_TYPE3;
//                phyInfo->dteType = GT_PHY_DTE_TYPE4;
//                phyInfo->pktGenType = GT_PHY_PKTGEN_TYPE2;
//                phyInfo->macIfLoopType = GT_PHY_LOOPBACK_TYPE1;
//                phyInfo->lineLoopType = 0;
//                phyInfo->exLoopType = GT_PHY_EX_LB_TYPE0;
//                phyInfo->pageType = GT_PHY_PAGE_WRITE_BACK;
//                break;
//
//        case DEV_G15LV:
//                if (dev->devName1 &DEV_88E6108)
//                {
//                    phyInfo->anyPage = 0x2040FFFF;
//                    phyInfo->flag = GT_PHY_VCT_CAPABLE|GT_PHY_DTE_CAPABLE|
//                                    GT_PHY_EX_CABLE_STATUS|
//                                    GT_PHY_GIGABIT|
//                                    GT_PHY_MAC_IF_LOOP|GT_PHY_LINE_LOOP|GT_PHY_EXTERNAL_LOOP|
//                                    GT_PHY_PKT_GENERATOR;
//                    phyInfo->vctType = GT_PHY_VCT_TYPE4;
//                    phyInfo->exStatusType = GT_PHY_EX_STATUS_TYPE3;
//                    phyInfo->dteType = GT_PHY_DTE_TYPE4;
//                    phyInfo->pktGenType = GT_PHY_PKTGEN_TYPE2;
//                    phyInfo->macIfLoopType = GT_PHY_LOOPBACK_TYPE1;
//                    phyInfo->lineLoopType = 0;
//                    phyInfo->exLoopType = GT_PHY_EX_LB_TYPE0;
//                    phyInfo->pageType = GT_PHY_PAGE_WRITE_BACK;
//                }
//                else /* 88E6165 family */
//                {
//                    phyInfo->anyPage = 0x2040FFFF;
//                    phyInfo->flag = GT_PHY_ADV_VCT_CAPABLE|GT_PHY_DTE_CAPABLE|
//                                    GT_PHY_EX_CABLE_STATUS|
//                                    GT_PHY_GIGABIT|
//                                    GT_PHY_MAC_IF_LOOP|GT_PHY_LINE_LOOP|GT_PHY_EXTERNAL_LOOP|
//                                    GT_PHY_PKT_GENERATOR;
//                    phyInfo->vctType = GT_PHY_ADV_VCT_TYPE2;
//                    phyInfo->exStatusType = GT_PHY_EX_STATUS_TYPE6;
//                    phyInfo->dteType = GT_PHY_DTE_TYPE4;
//                    phyInfo->pktGenType = GT_PHY_PKTGEN_TYPE2;
//                    phyInfo->macIfLoopType = GT_PHY_LOOPBACK_TYPE1;
//                    phyInfo->lineLoopType = 0;
//                    phyInfo->exLoopType = GT_PHY_EX_LB_TYPE0;
//                    phyInfo->pageType = GT_PHY_PAGE_WRITE_BACK;
//                }
//                break;
//
//        case DEV_EC010:
//                phyInfo->anyPage = 0x2040780C;
//                phyInfo->flag = GT_PHY_VCT_CAPABLE|GT_PHY_DTE_CAPABLE|
//                                GT_PHY_EX_CABLE_STATUS|
//                                GT_PHY_GIGABIT|GT_PHY_RESTRICTED_PAGE|
//                                GT_PHY_MAC_IF_LOOP|GT_PHY_LINE_LOOP|GT_PHY_EXTERNAL_LOOP;
//                phyInfo->vctType = GT_PHY_VCT_TYPE2;
//                phyInfo->exStatusType = 0;
//                phyInfo->dteType = GT_PHY_DTE_TYPE3;    /* Need workaround */
//                phyInfo->pktGenType = 0;
//                phyInfo->macIfLoopType = GT_PHY_LOOPBACK_TYPE1;
//                phyInfo->lineLoopType = 0;
//                phyInfo->exLoopType = GT_PHY_EX_LB_TYPE0;
//                phyInfo->pageType = GT_PHY_PAGE_WRITE_BACK;
//                break;
//
//        case DEV_S15LV:
//                phyInfo->anyPage = 0xFFFFFFFF;
//                phyInfo->flag = GT_PHY_SERDES_CORE|GT_PHY_GIGABIT|
//                                GT_PHY_MAC_IF_LOOP|GT_PHY_LINE_LOOP|GT_PHY_EXTERNAL_LOOP|
//                                GT_PHY_PKT_GENERATOR;
//                phyInfo->vctType = 0;
//                phyInfo->exStatusType = 0;
//                phyInfo->dteType = 0;
//                phyInfo->pktGenType = GT_PHY_PKTGEN_TYPE3;
//                phyInfo->macIfLoopType = GT_PHY_LOOPBACK_TYPE3;
//                phyInfo->lineLoopType = GT_PHY_LINE_LB_TYPE4;
//                phyInfo->exLoopType = 0;
//                phyInfo->pageType = GT_PHY_NO_PAGE;
//                break;
//
//        case DEV_G65G:
//                phyInfo->anyPage = 0x2040FFFF;
//                phyInfo->flag = GT_PHY_ADV_VCT_CAPABLE|GT_PHY_DTE_CAPABLE|
//                                GT_PHY_EX_CABLE_STATUS|
//                                GT_PHY_GIGABIT|
//                                GT_PHY_MAC_IF_LOOP|GT_PHY_LINE_LOOP|GT_PHY_EXTERNAL_LOOP|
//                                GT_PHY_PKT_GENERATOR;
//                phyInfo->vctType = GT_PHY_ADV_VCT_TYPE2;
//                phyInfo->exStatusType = GT_PHY_EX_STATUS_TYPE6;
//                phyInfo->dteType = GT_PHY_DTE_TYPE4;
//                phyInfo->pktGenType = GT_PHY_PKTGEN_TYPE2;
//                phyInfo->macIfLoopType = GT_PHY_LOOPBACK_TYPE1;
//                phyInfo->lineLoopType = 0;
//                phyInfo->exLoopType = GT_PHY_EX_LB_TYPE0;
//                phyInfo->pageType = GT_PHY_PAGE_WRITE_BACK;
//                break;
//        default:
//            return GT_FAIL;
//    }
//
//    if (phyInfo->flag & GT_PHY_GIGABIT)
//    {
//        if(hwGetPhyRegField(dev,hwPort,15,12,4,&data) != GT_OK)
//        {
//            DBG_INFO(("Not able to read Phy Reg(port:%d,offset:%d).\n",hwPort,15));
//               return GT_FAIL;
//        }
//
//        if(data & QD_GIGPHY_1000X_CAP)
//            phyInfo->flag |= GT_PHY_FIBER;
//
//        if(data & QD_GIGPHY_1000T_CAP)
//        {
//            phyInfo->flag |= GT_PHY_COPPER;
//        }
//        else
//        {
//            phyInfo->flag &= ~(GT_PHY_VCT_CAPABLE|GT_PHY_EX_CABLE_STATUS|GT_PHY_DTE_CAPABLE|GT_PHY_ADV_VCT_CAPABLE);
//        }
//    }
//
//    return GT_OK;
//}


GT_STATUS hwSetPhyRegField
(
    IN  GT_U8    portNum,
    IN  GT_U8    regAddr,
    IN  GT_U8    fieldOffset,
    IN  GT_U8    fieldLength,
    IN  GT_U16   data
)
{
    GT_U16 mask;
    GT_U16 tmpData;
    GT_STATUS   retVal;


    //retVal = coreReadPhyReg(dev, portNum, regAddr, &tmpData);
    tmpData=ETH_ReadPHYRegister(portNum,regAddr);

    CALC_MASK(fieldOffset,fieldLength,mask);

    /* Set the desired bits to 0.                       */
    tmpData &= ~mask;
    /* Set the given data into the above reset bits.    */
    tmpData |= ((data << fieldOffset) & mask);

    DEBUG_MSG(DEBUG_QD,"Write to phy(%d) register: regAddr 0x%x, ",
              portNum,regAddr);
    DEBUG_MSG(DEBUG_QD,"fieldOff %d, fieldLen %d, data 0x%x.\r\n",fieldOffset,
              fieldLength,data);

    //retVal = coreWritePhyReg(dev, portNum, regAddr, tmpData);
    retVal=ETH_WritePHYRegister(portNum,regAddr,tmpData);

//    gtSemGive(dev,dev->multiAddrSem);
    return retVal;
}

GT_STATUS hwSetPhyRegField6240
(
    IN  GT_U8    portNum,
    IN  GT_U8    regAddr,
    IN  GT_U8    pageNum,
    IN  GT_U8    fieldOffset,
    IN  GT_U8    fieldLength,
    IN  GT_U16   data
)
{
    GT_U16 mask;
    GT_U16 tmpData;


    //retVal = coreReadPhyReg(dev, portNum, regAddr, &tmpData);
    tmpData=ETH_ReadIndirectPHYReg(portNum,pageNum,regAddr);

    CALC_MASK(fieldOffset,fieldLength,mask);
    /* Set the desired bits to 0.                       */
    tmpData &= ~mask;
    /* Set the given data into the above reset bits.    */
    tmpData |= ((data << fieldOffset) & mask);

    DEBUG_MSG(DEBUG_QD,"Write to phy(%d) register: regAddr 0x%x, ",
              portNum,regAddr);
    DEBUG_MSG(DEBUG_QD,"fieldOff %d, fieldLen %d, data 0x%x.\r\n",fieldOffset,
              fieldLength,data);

    ETH_WriteIndirectPHYReg(portNum,pageNum,regAddr,tmpData);

    return GT_OK;
}


GT_STATUS hwGetGlobalRegField
(
    IN  GT_U8    regAddr,
    IN  GT_U8    fieldOffset,
    IN  GT_U8    fieldLength,
    OUT GT_U16   *data
)
{
    GT_U16 mask;            /* Bits mask to be read */
    GT_U16 tmpData;
    GT_U8       phyAddr;

    phyAddr = portToSmiMapping(0, GLOBAL_REG_ACCESS);
    if (phyAddr == 0xFF)
    {
        return GT_BAD_PARAM;
    }

    tmpData = ETH_ReadPHYRegister(phyAddr,regAddr);


    CALC_MASK(fieldOffset,fieldLength,mask);
    tmpData = (tmpData & mask) >> fieldOffset;
    *data = tmpData;
    DEBUG_MSG(DEBUG_QD,"Read from global register: regAddr 0x%x, fOff %d, fLen %d, data 0x%x.\r\n",
    		regAddr,fieldOffset,fieldLength,*data);

    return GT_OK;
}

uint8_t portToSmiMapping
(
    uint8_t    portNum,
    uint32_t    accessType
)
{
    GT_U8 smiAddr;

    if(/*IS_IN_DEV_GROUP(dev,DEV_8PORT_SWITCH)*/1)
    {
        switch(accessType)
        {
            case PHY_ACCESS:
                    //if (dev->validPhyVec & (1<<portNum))
                        smiAddr = PHY_REGS_START_ADDR_8PORT + portNum;
                    //else
                    //    smiAddr = 0xFF;
                    break;
            case PORT_ACCESS:
                    //if (dev->validPortVec & (1<<portNum))
                        smiAddr = PORT_REGS_START_ADDR_8PORT + portNum;
                    //else
                    //    smiAddr = 0xFF;
                    break;
            case GLOBAL_REG_ACCESS:
                    smiAddr = GLOBAL_REGS_START_ADDR_8PORT;
                    break;
            case GLOBAL3_REG_ACCESS:
                    smiAddr = GLOBAL_REGS_START_ADDR_8PORT + 2;
                    break;
            default:
                    smiAddr = GLOBAL_REGS_START_ADDR_8PORT + 1;
                    break;
        }
    }
    return smiAddr;
}

GT_STATUS hwSetPortRegField
(
    IN  GT_U8    portNum,
    IN  GT_U8    regAddr,
    IN  GT_U8    fieldOffset,
    IN  GT_U8    fieldLength,
    IN  GT_U16   data
)
{
    GT_U16 mask;
    GT_U16 tmpData;
    //GT_STATUS   retVal;
    GT_U8       phyAddr=0;

    phyAddr = portToSmiMapping(portNum, PORT_ACCESS);

    tmpData =  ETH_ReadPHYRegister(phyAddr,regAddr);

    CALC_MASK(fieldOffset,fieldLength,mask);

    /* Set the desired bits to 0.                       */
    tmpData &= ~mask;
    /* Set the given data into the above reset bits.    */
    tmpData |= ((data << fieldOffset) & mask);
    DEBUG_MSG(DEBUG_QD,"Write to port(%d) register: regAddr 0x%x, fieldOff %d, fieldLen %d, data 0x%x.  \r\n",
    		portNum,regAddr,fieldOffset,
              fieldLength,data);

    ETH_WritePHYRegister(phyAddr,regAddr,tmpData);

    return GT_OK;
}

GT_STATUS hwSetPortRegField6240
(
    IN  GT_U8    portNum,
    IN  GT_U8    regAddr,
    IN  GT_U8    pageNum,
    IN  GT_U8    fieldOffset,
    IN  GT_U8    fieldLength,
    IN  GT_U16   data
)
{
    GT_U16 mask;
    GT_U16 tmpData;
    //GT_STATUS   retVal;
    GT_U8       phyAddr=0;

    phyAddr = portToSmiMapping(portNum, PORT_ACCESS);


    tmpData=ETH_ReadIndirectPHYReg(phyAddr,pageNum,regAddr);

    CALC_MASK(fieldOffset,fieldLength,mask);

    /* Set the desired bits to 0.                       */
    tmpData &= ~mask;
    /* Set the given data into the above reset bits.    */
    tmpData |= ((data << fieldOffset) & mask);
    DEBUG_MSG(DEBUG_QD,"Write to port(%d) register: regAddr 0x%x, fieldOff %d, fieldLen %d, data 0x%x.  \r\n",
    		portNum,regAddr,fieldOffset,
              fieldLength,data);

    ETH_WriteIndirectPHYReg(phyAddr,pageNum,regAddr,tmpData);

    return GT_OK;
}

GT_STATUS hwGetPortRegField
(
    IN  GT_U8    portNum,
    IN  GT_U8    regAddr,
    IN  GT_U8    fieldOffset,
    IN  GT_U8    fieldLength,
    OUT GT_U16   *data
)
{
    GT_U16 mask;            /* Bits mask to be read */
    GT_U16 tmpData;
    GT_U8       phyAddr;

    phyAddr = portToSmiMapping(portNum, PORT_ACCESS);
    if (phyAddr == 0xFF)
    {
       return GT_BAD_PARAM;
    }

    //retVal =  miiSmiIfReadRegister(dev,phyAddr,regAddr,&tmpData);
    tmpData=ETH_ReadPHYRegister(phyAddr,regAddr);

    CALC_MASK(fieldOffset,fieldLength,mask);

    tmpData = (tmpData & mask) >> fieldOffset;
    *data = tmpData;
    DEBUG_MSG(DEBUG_QD,"Read from port(%d) register: regAddr 0x%x, fOff %d, fLen %d, data 0x%x.\r\n",
    		portNum,regAddr,fieldOffset,fieldLength,*data);

    return GT_OK;
}

GT_STATUS gprtSetPause(IN GT_LPORT  port,IN GT_PHY_PAUSE_MODE state){
    GT_U8           hwPort;         /* the physical port number     */
    GT_U16             u16Data,regStart;
    GT_STATUS        retVal = 0;//GT_OK;

#ifdef GT_USE_MAD
	if (dev->use_mad==GT_TRUE)
		return gprtSetPause_mad(dev, port, state);
#endif

    DEBUG_MSG(DEBUG_QD,"phySetPause Called.\r\n");

    /* translate LPORT to hardware port */
    //hwPort = GT_LPORT_2_PHY(port);
    hwPort = lport2port(port);

    /* check if the port is configurable */
//    if((phyInfo.phyId=GT_GET_PHY_ID(dev,hwPort)) == GT_INVALID_PHY)
//    {
//        gtSemGive(dev,dev->phyRegsSem);
//        return GT_NOT_SUPPORTED;
//    }

    regStart = 10;

    if(state & GT_PHY_ASYMMETRIC_PAUSE)
    {
//        if(driverFindPhyInformation(dev,hwPort,&phyInfo) != GT_OK)
//        {
//            DBG_INFO(("Unknown PHY device.\n"));
//            gtSemGive(dev,dev->phyRegsSem);
//            return GT_FAIL;
//        }

//        if (!(phyInfo.flag & GT_PHY_GIGABIT))
//        {
//            DBG_INFO(("Not Supported\n"));
//            gtSemGive(dev,dev->phyRegsSem);
//            return GT_BAD_PARAM;
//        }

//        if(!(phyInfo.flag & GT_PHY_COPPER))
//        {
//            regStart = 7;
//        }


    	if(port>8){
    		regStart = 7;
    	}
    }

    u16Data = state;

    /* Write to Phy AutoNegotiation Advertisement Register.  */
    if((retVal=hwSetPhyRegField(hwPort,QD_PHY_AUTONEGO_AD_REG,(uint8_t)regStart,2,u16Data)) != 1)
    {
        DEBUG_MSG(DEBUG_QD,"Not able to write Phy Reg(port:%d,offset:%d).\r\n",hwPort,QD_PHY_AUTONEGO_AD_REG);
        ///gtSemGive(dev,dev->phyRegsSem);
        return GT_FAIL;
    }

    /* Restart Auto Negotiation */
    if((retVal=hwSetPhyRegField(hwPort,QD_PHY_CONTROL_REG,9,1,1)) != 1)
    {
        DEBUG_MSG(DEBUG_QD,"Not able to write Phy Reg(port:%d,offset:%d,data:%#x).\r\n",hwPort,QD_PHY_AUTONEGO_AD_REG,u16Data);
        return GT_FAIL;
    }

   return 0;//retVal;
}


GT_STATUS gprtPortRestartAutoNeg(IN GT_LPORT  port)
{
    GT_STATUS       retVal;
    GT_U8           hwPort;         /* the physical port number     */
    GT_U16             u16Data;

#ifdef GT_USE_MAD
	if (dev->use_mad==GT_TRUE)
		return gprtPortRestartAutoNeg_mad(dev, port);
#endif

    DEBUG_MSG(DEBUG_QD,"gprtPortRestartAutoNeg Called.\r\n");

    /* translate LPORT to hardware port */
    hwPort = lport2port(port);

    u16Data=ETH_ReadPHYRegister(hwPort,QD_PHY_CONTROL_REG);

    u16Data &= (QD_PHY_DUPLEX | QD_PHY_SPEED);
    u16Data |= (QD_PHY_RESTART_AUTONEGO | QD_PHY_AUTONEGO);

    DEBUG_MSG(DEBUG_QD,"Write to phy(%d) register(QD_PHY_CONTROL_REG): regAddr 0x%x, data %#x",
              hwPort,QD_PHY_CONTROL_REG,u16Data);

    /* Write to Phy Control Register.  */
    retVal = ETH_WritePHYRegister(hwPort,QD_PHY_CONTROL_REG,u16Data);

    DEBUG_MSG(DEBUG_QD,"OK.\r\n");

    return retVal;
}

GT_STATUS gstpGetPortState
(
    IN  GT_LPORT           port,
    OUT GT_PORT_STP_STATE  *state
)
{
    GT_U8           phyPort;        /* Physical port                */
    GT_U16          data;           /* Data read from register.     */
    GT_STATUS       retVal;         /* Functions return value.      */

    DEBUG_MSG(DEBUG_QD,"gstpGetPortState Called.\r\n");

    phyPort = lport2port(port);

    /* Get the port state bits.             */
    retVal = hwGetPortRegField(phyPort, QD_REG_PORT_CONTROL,0,2,&data);
    if(retVal != GT_OK)
    {
        DEBUG_MSG(DEBUG_QD,"Failed.\r\n");
        return retVal;
    }

    *state = data & 0x3;
    DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    return GT_OK;
}


GT_STATUS gstpSetPortState
(
    IN GT_LPORT           port,
    IN GT_PORT_STP_STATE  state
)
{
    GT_U8           phyPort;        /* Physical port                */
    GT_U16          data;           /* Data to write to register.   */
    //GT_STATUS       retVal;         /* Functions return value.      */

    DEBUG_MSG(DEBUG_QD,"gstpSetPortState Called.\r\n");

    phyPort = lport2port(port);
    data    = state;

    /* Set the port state bits.             */
    hwSetPortRegField(phyPort, QD_REG_PORT_CONTROL,0,2,data);

    DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    return GT_OK;
}


GT_STATUS gprtSetForceFc
(
    IN GT_LPORT   port,
    IN GT_BOOL    force
)
{
    GT_U16          data;
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           hwPort;         /* the physical port number     */
    GT_PORT_STP_STATE  state;

    DEBUG_MSG(DEBUG_QD,"gprtSetForceFc Called.\r\n");

    /* translate LPORT to hardware port */
    hwPort = lport2port(port);

    DEBUG_MSG(DEBUG_QD,"Lport2Port return value=%d \r\n",hwPort);

    /* check if device allows to force a flowcontrol disabled */

	if(force)
		data = 3;
	else
		data = 0;

	retVal = hwSetPortRegField(hwPort, QD_REG_PCS_CONTROL,6,2,data);
	if(retVal != GT_OK)
	{
		DEBUG_MSG(DEBUG_QD,"Failed.\r\n");
	}
	else
	{
		DEBUG_MSG(DEBUG_QD,"OK.\r\n");
	}
	return retVal;

    /* Port should be disabled before Set Force Flow Control bit */
    retVal = gstpGetPortState(port, &state);
    if(retVal != 1)
    {
        DEBUG_MSG(DEBUG_QD,"gstpGetPortState failed.\r\n");
        return retVal;
    }

    retVal = gstpSetPortState(port, GT_PORT_DISABLE);
    if(retVal != GT_OK)
    {
        DEBUG_MSG(DEBUG_QD,"gstpSetPortState failed.\r\n");
        return retVal;
    }

    /* translate BOOL to binary */
    BOOL_2_BIT(force, data);

    /* Set the force flow control bit.  */
    retVal = hwSetPortRegField(hwPort, QD_REG_PORT_CONTROL,15,1,data);
    if(retVal != 0)
    {
        DEBUG_MSG(DEBUG_QD,"Failed.\r\n");
    }
    else
    {
        DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    }

    /* Restore original stp state. */
    if(gstpSetPortState(port, state) != GT_OK)
    {
        DEBUG_MSG(DEBUG_QD,"gstpSetPortState failed.\r\n");
        return GT_FAIL;
    }

    return retVal;
}

GT_STATUS PortConfig(port_sett_t *port_sett){
GT_STATUS status=0;

//printf("port config %d\r\n",port_sett->lport);

	/*настройка SFP портов*/
	if(is_fiber(port_sett->lport)){
		if(port_sett->state == DISABLE){
			ETH_WritePHYRegister(0x10+L2F_port_conv(port_sett->lport),0x01,0x413);
		}
		else{
			ETH_WritePHYRegister(0x10+L2F_port_conv(port_sett->lport),0x01,0x403);
		}
	}

    DEBUG_MSG(DEBUG_QD,"port %d, state %d, flow %d, speed %d\r\n",(u8)
    		port_sett->lport,
    		port_sett->state,
    		port_sett->flow,
    		port_sett->speed);

    //if noauto

    if((port_sett->flow==ENABLE)||(port_sett->flow==DISABLE)){
		//
		//    Program Phy's Pause bit in AutoNegotiation Advertisement Register.
		//
		if((status = gprtSetPause(port_sett->fport,port_sett->flow)) != GT_OK)
		{
			DEBUG_MSG(DEBUG_QD,"gprtSetPause return Failed\r\n");
			return status;
		}
    }


    /*
     *    Restart AutoNegotiation of the given Port's phy
     */
    DEBUG_MSG(DEBUG_QD,"Restart Aneg & Speed &Duplex set Called.\r\n");

    StateSpeedDuplexANegSet(port_sett->fport,port_sett->state,SpeedConv(port_sett->speed),DuplexConv(port_sett->speed),ANEgConv(port_sett->speed));

    //if noauto
    if((port_sett->flow==ENABLE)||(port_sett->flow==DISABLE)){
		if((status = gprtSetForceFc(port_sett->fport,port_sett->flow)) != GT_OK)
		{
			DEBUG_MSG(DEBUG_QD,"gprtSetForceFc return Failed\r\n");
			return status;
		}
	}

	return GT_OK;

}
