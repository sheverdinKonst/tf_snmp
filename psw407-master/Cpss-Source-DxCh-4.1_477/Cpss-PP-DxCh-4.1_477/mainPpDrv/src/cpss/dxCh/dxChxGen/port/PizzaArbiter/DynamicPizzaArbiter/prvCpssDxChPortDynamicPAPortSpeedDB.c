/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvCpssDxChPortDynamicPAPortSpeedDB.c
*
* DESCRIPTION:
*       bobcat2 and higher dynamic (algorithmic) pizza arbiter Port Speed DB
*
* FILE REVISION NUMBER:
*       $Revision: 4 $
*******************************************************************************/
#include <cpss/dxCh/dxChxGen/config/private/prvCpssDxChInfo.h>
#include <cpss/dxCh/dxChxGen/port/private/prvCpssDxChPortCtrl.h>
#include <cpss/dxCh/dxChxGen/port/cpssDxChPortCtrl.h>
#include <cpss/dxCh/dxChxGen/port/PizzaArbiter/DynamicPizzaArbiter/prvCpssDxChPortDynamicPAPortSpeedDB.h>

   
   

GT_STATUS prvCpssDxChPortDynamicPAPortSpeedDBInit
(
    IN    GT_U8    devNum
)
{
    GT_STATUS rc;
    GT_PHYSICAL_PORT_NUM i;
    PRV_CPSS_DXCH_PORT_PA_PORT_DB_STC *portDBPtr;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);


    portDBPtr = &PRV_CPSS_PP_MAC(devNum)->paPortDB;

    portDBPtr->numInitPorts = 0;
    for (i = 0 ; i < PRV_CPSS_DXCH_PORT_NUM_CNS; i++)
    {
        portDBPtr->prv_portDB[i].physicalPortNum = i;
        portDBPtr->prv_portDB[i].isInitilized = GT_FALSE;
        portDBPtr->prv_portDB[i].portSpeedInMBit        = 0;
        portDBPtr->prv_portDB[i].ilknChannelSpeedInMBit = 0;
    }

    rc = prvCpssDxChPortDynamicPAIlknSpeedResolutionSet(devNum,40,0);/* default resolution, 0 bw, the IF is still not initialized */
    if (rc != GT_OK)
    {
        return rc;
    }
    return GT_OK;
}

GT_STATUS prvCpssDxChPortDynamicPAPortSpeedDBSet
(
    IN    GT_U8                       devNum,
    IN    GT_PHYSICAL_PORT_NUM        portNum,
    IN    GT_U32                      speedInMBit
)
{
    PRV_CPSS_DXCH_PORT_PA_PORT_DB_STC *portDBPtr;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);


    portDBPtr = &PRV_CPSS_PP_MAC(devNum)->paPortDB;
    if (portNum >= PRV_CPSS_DXCH_PORT_NUM_CNS)
    {
        return GT_OUT_OF_RANGE;
    }
    if (speedInMBit > 0)
    {
        portDBPtr->prv_portDB[portNum].isInitilized = GT_TRUE;
        portDBPtr->prv_portDB[portNum].portSpeedInMBit  = speedInMBit;
        portDBPtr->numInitPorts ++;
    }
    else
    {
        portDBPtr->prv_portDB[portNum].isInitilized = GT_FALSE;
        portDBPtr->prv_portDB[portNum].portSpeedInMBit  = 0;
        portDBPtr->numInitPorts --;
    }

    return GT_OK;
}

GT_STATUS prvCpssDxChPortDynamicPAPortSpeedDBGet
(
    IN    GT_U8                      devNum,
    IN    GT_PHYSICAL_PORT_NUM       portNum,
    OUT   GT_BOOL                   *isInitilizedPtr,
    OUT   GT_U32                    *speedInMBitPtr
)
{
    PRV_CPSS_DXCH_PORT_PA_PORT_DB_STC *portDBPtr;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);

    CPSS_NULL_PTR_CHECK_MAC(isInitilizedPtr);
    CPSS_NULL_PTR_CHECK_MAC(speedInMBitPtr);

    portDBPtr = &PRV_CPSS_PP_MAC(devNum)->paPortDB;

    if (portNum >= PRV_CPSS_DXCH_PORT_NUM_CNS)
    {
        return GT_BAD_PARAM;
    }
    
    *isInitilizedPtr = portDBPtr->prv_portDB[portNum].isInitilized;;
    *speedInMBitPtr  = portDBPtr->prv_portDB[portNum].portSpeedInMBit;
    return GT_OK;
}


GT_STATUS prvCpssDxChPortDynamicPAIlknChannelSpeedDBSet
(
    IN    GT_U8                       devNum,
    IN    GT_PHYSICAL_PORT_NUM        portNum,
    IN    GT_U32                      speedInMBit
)
{
    PRV_CPSS_DXCH_PORT_PA_PORT_DB_STC *portDBPtr;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);


    portDBPtr = &PRV_CPSS_PP_MAC(devNum)->paPortDB;
    if (portNum >= PRV_CPSS_DXCH_PORT_NUM_CNS)
    {
        return GT_OUT_OF_RANGE;
    }
    if (speedInMBit > 0)
    {
        portDBPtr->prv_portDB[portNum].isInitilized           = GT_TRUE;
        portDBPtr->prv_portDB[portNum].ilknChannelSpeedInMBit = speedInMBit;
        portDBPtr->numInitPorts ++;
    }
    else
    {
        portDBPtr->prv_portDB[portNum].isInitilized           = GT_FALSE;
        portDBPtr->prv_portDB[portNum].ilknChannelSpeedInMBit = 0;
        portDBPtr->numInitPorts --;
    }

    return GT_OK;
}

GT_STATUS prvCpssDxChPortDynamicPAIlknChannelSpeedDBGet
(
    IN    GT_U8                      devNum,
    IN    GT_PHYSICAL_PORT_NUM       portNum,
    OUT   GT_BOOL                   *isInitilizedPtr,
    OUT   GT_U32                    *speedInMBitPtr
)
{
    PRV_CPSS_DXCH_PORT_PA_PORT_DB_STC *portDBPtr;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);

    CPSS_NULL_PTR_CHECK_MAC(isInitilizedPtr);
    CPSS_NULL_PTR_CHECK_MAC(speedInMBitPtr);

    portDBPtr = &PRV_CPSS_PP_MAC(devNum)->paPortDB;

    if (portNum >= PRV_CPSS_DXCH_PORT_NUM_CNS)
    {
        return GT_BAD_PARAM;
    }
    
    *isInitilizedPtr = portDBPtr->prv_portDB[portNum].isInitilized;;
    *speedInMBitPtr  = portDBPtr->prv_portDB[portNum].ilknChannelSpeedInMBit;
    return GT_OK;
}


GT_STATUS prvCpssDxChPortDynamicPAIlknSpeedResolutionSet
(
    IN    GT_U8                       devNum,
    IN    GT_U32                      speedResulutionMBps,
    IN    GT_U32                      ilknIFnaxBWMbps
)
{
    PRV_CPSS_DXCH_PORT_PA_PORT_DB_STC *portDBPtr;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);


    portDBPtr = &PRV_CPSS_PP_MAC(devNum)->paPortDB;
    portDBPtr->ilknSpeedResolutionMbps = speedResulutionMBps; /* default resolution */
    portDBPtr->ilknIFnaxBWMbps         = ilknIFnaxBWMbps;

    return GT_OK;
}


GT_STATUS prvCpssDxChPortDynamicPAIlknSpeedResolutionGet
(
    IN    GT_U8                       devNum,
    OUT   GT_U32                     *speedResulutionMBpsPtr,
    OUT   GT_U32                     *ilknIFnaxBWMbpsPtr
)
{
    PRV_CPSS_DXCH_PORT_PA_PORT_DB_STC *portDBPtr;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);

    CPSS_NULL_PTR_CHECK_MAC(speedResulutionMBpsPtr);
    CPSS_NULL_PTR_CHECK_MAC(ilknIFnaxBWMbpsPtr);

    portDBPtr = &PRV_CPSS_PP_MAC(devNum)->paPortDB;
    *speedResulutionMBpsPtr = portDBPtr->ilknSpeedResolutionMbps;
    *ilknIFnaxBWMbpsPtr     = portDBPtr->ilknIFnaxBWMbps;
    return GT_OK;
}
