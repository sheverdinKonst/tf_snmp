/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvCpssDxChPortIfModeCfgResource.c
*
* DESCRIPTION:
*       CPSS implementation for port resource configuration.
*
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/
#include <cpss/extServices/private/prvCpssBindFunc.h>
#include <cpss/dxCh/dxChxGen/config/private/prvCpssDxChInfo.h>
#include <cpss/dxCh/dxChxGen/cpssHwInit/private/prvCpssDxChHwInit.h>
#include <cpss/dxCh/dxChxGen/port/cpssDxChPortCtrl.h>
#include <cpss/dxCh/dxChxGen/port/cpssDxChPortInterlaken.h>
#include <cpss/dxCh/dxChxGen/port/PortMapping/prvCpssDxChPortMappingShadowDB.h>
#include <cpss/dxCh/dxChxGen/port/private/prvCpssDxChPortCtrl.h>
#include <cpss/dxCh/dxChxGen/port/private/prvCpssDxChPortIfModeCfgBcat2Resource.h>
#include <cpss/dxCh/dxChxGen/port/private/prvCpssDxChPortIfModeCfgBobKResource.h>
#include <cpss/dxCh/dxChxGen/port/private/prvCpssDxChPortIfModeCfgResource.h>


#define INVALID_PORT_CNS  0xFFFF

GT_STATUS prvCpssDxChPortMappingCPUPortGet
(
     IN  GT_U8   devNum,
    OUT  GT_PHYSICAL_PORT_NUM *cpuPortNumPtr
)
{
    GT_STATUS rc;
    GT_U32 maxPortNum;
    GT_PHYSICAL_PORT_NUM portNum;
    PRV_CPSS_DXCH_PORT_MAP_SHADOW_STC *portMapShadowPtr;


    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);

    CPSS_NULL_PTR_CHECK_MAC(cpuPortNumPtr);
    rc = prvCpssDxChPortPhysicalPortMapShadowDBPossiblePortNumGet(devNum,&maxPortNum);
    if (rc != GT_OK)
    {
        return rc;
    }
    for (portNum = 0 ; portNum < maxPortNum; portNum++)
    {
        /* find local physical port number of CPU port */
        rc =  prvCpssDxChPortPhysicalPortMapShadowDBGet(devNum,portNum, /*OUT*/&portMapShadowPtr);
        if (rc != GT_OK)
        {
            return rc;
        }
        if (GT_TRUE == portMapShadowPtr->valid)
        {
            if (portMapShadowPtr->portMap.mappingType == CPSS_DXCH_PORT_MAPPING_TYPE_CPU_SDMA_E)
            {
                *cpuPortNumPtr = portNum;
                return GT_OK;
            }
        }
    }
    *cpuPortNumPtr = INVALID_PORT_CNS;
    return GT_OK;
}



 /*  
//    +------------------------------------------------------------------------------------------------------------------------------+
//    |                             Port resources                                                                                   |
//    +----+------+----------+-------+--------------+--------------------+-----+-----------------------------+-----------+-----------+
//    |    |      |          |       |              |                    |RXDMA|      TXDMA SCDMA            | TX-FIFO   |Eth-TX-FIFO|
//    |    |      |          |       |              |                    |-----|-----+-----+-----+-----------|-----+-----|-----+-----|
//    | #  | Port | map type | Speed |    IF        | rxdma txq txdma tm |  IF | TxQ |Burst|Burst| TX-FIFO   | Out | Pay | Out | Pay |
//    |    |      |          |       |              |                    |Width|Descr| All | Full|-----+-----|Going| Load|Going| Load|
//    |    |      |          |       |              |                    |     |     | Most|Thrsh| Hdr | Pay | Bus |Thrsh| Bus |Thrsh|
//    |    |      |          |       |              |                    |     |     | Full|     |Thrsh|Load |Width|     |Width|     |
//    |    |      |          |       |              |                    |     |     |Thrsh|     |     |Thrsh|     |     |     |     |
//    +----+------+----------+-------+--------------+--------------------+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//    | 53 |   63 | CPU-SDMA | 1G    | ------------ |    72  63    72 -1 | 64b |   2 |   2 |   2 |   2 |   4 |  1B |   2 |  -- |  -- |  As 1G
//    | 53 |   63 | CPU-SDMA | 1G    | ------------ |    72  63    72 -1 | 64b |   2 |   0 |   0 |   2 |   4 |  8B |   1 |  -- |  -- | Default
//    +----+------+----------+-------+--------------+--------------------+-----+-----------------------------+-----------+-----------+

CPU  TxFIFO out-going-bus (DMA 72) shall be configured as 8byte

Regarding the TxDMA “Burst Almost Full Thr” and “Burst Full Thr” the configuration is irrelevant if we aren’t enabling the feature 
(/Cider/EBU/BobK/BobK {Current}/Switching Core/TXDMA/Units/<TXDMA1_IP> TxDMA IP Units/TxDMA Per SCDMA Configurations/SCDMA %p Configurations Reg 1/ Enable Burst Limit SCDMA %p). 


*/

/*******************************************************************************
* prvCpssDxChPortResourcesInit
*
* DESCRIPTION:
*       Initialize data structure for port resource allocation
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum;
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Lion2.
*
* INPUTS:
*       devNum      - physical device number
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK             - on success
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChPortResourcesInit
(
    IN  GT_U8   devNum
)
{
    GT_STATUS rc = GT_OK; /*return code*/
    GT_PHYSICAL_PORT_NUM cpuPortNum;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum, CPSS_CH1_E | CPSS_CH1_DIAMOND_E
                                            | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E
                                            | CPSS_LION_E | CPSS_LION2_E | CPSS_XCAT2_E);

    rc = prvCpssDxChPortMappingCPUPortGet(devNum,/*OUT*/&cpuPortNum);
    if (rc != GT_OK)
    {
        return rc;
    }

    if(PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_BOBCAT2_E)
    {
        if(PRV_CPSS_PP_MAC(devNum)->devSubFamily == CPSS_PP_SUB_FAMILY_NONE_E)
        {
            rc = prvCpssDxChBcat2PortResourcesInit(devNum);
            if (rc != GT_OK)
            {
                return rc;
            }
            if (cpuPortNum != INVALID_PORT_CNS)
            {
                rc = prvCpssDxChBcat2PortResourcesConfig(devNum,cpuPortNum,CPSS_PORT_INTERFACE_MODE_NA_E,CPSS_PORT_SPEED_1000_E,GT_TRUE);
                if (rc != GT_OK)
                {
                    return rc;
                }
            }
        }
        else if(PRV_CPSS_PP_MAC(devNum)->devSubFamily == CPSS_PP_SUB_FAMILY_BOBCAT2_BOBK_E)
        {
            rc = prvCpssDxChCaelumPortResourcesInit(devNum);
            if (rc != GT_OK)
            {
                return rc;
            }
            if (cpuPortNum != INVALID_PORT_CNS)
            {
                rc = prvCpssDxChCaelumPortResourcesConfig(devNum,cpuPortNum,CPSS_PORT_INTERFACE_MODE_NA_E,CPSS_PORT_SPEED_1000_E);
                if (rc != GT_OK)
                {
                    return rc;
                }
            }
        }
        else
        {
            return GT_NOT_APPLICABLE_DEVICE;
        }
    }
    /*-----------------------------------------------------------------*
     * currently BC3 is configuyred as BC2                             *
     *-----------------------------------------------------------------*/
    if(PRV_CPSS_PP_MAC(devNum)->devFamily == CPSS_PP_FAMILY_DXCH_BOBCAT3_E)
    {
        rc = prvCpssDxChBcat2PortResourcesInit(devNum);
        if (rc != GT_OK)
        {
            return rc;
        }
        if (cpuPortNum != INVALID_PORT_CNS)
        {
            rc = prvCpssDxChBcat2PortResourcesConfig(devNum,cpuPortNum,CPSS_PORT_INTERFACE_MODE_NA_E,CPSS_PORT_SPEED_1000_E,GT_TRUE);
            if (rc != GT_OK)
            {
                return rc;
            }
        }
    }

    return GT_OK;
}


GT_BOOL g_doNotCheckCredits = GT_FALSE;

/*--------------------------------------------------------------------
 *
 *--------------------------------------------------------------------
 */
GT_STATUS prvCpssDxChPortBcat2CreditsCheckSet(GT_BOOL val)
{
    switch (val)
    {
        case GT_FALSE:
            g_doNotCheckCredits = GT_TRUE;
        break;
        default:
        {
            g_doNotCheckCredits = GT_FALSE;
        }
    }
    cpssOsPrintf("\nCredits control = %d\n",!g_doNotCheckCredits);
    return GT_OK;
}

/*******************************************************************************
* prvCpssDxChPortResourcesConfigDbInit
*
* DESCRIPTION:
*       Resource data structure initialization
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum;
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Lion2.
*
* INPUTS:
*       devNum              - physical device number
*       maxDescCredits      - physical device number
*       maxHeaderCredits    - physical device number
*       maxPayloadCredits   - physical device number
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK             - on success
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChPortResourcesConfigDbInit
(
    IN  GT_U8   devNum,
    IN GT_U32   dpIndex,
    IN  GT_U32  maxDescCredits,
    IN  GT_U32  maxHeaderCredits,
    IN  GT_U32  maxPayloadCredits
)
{
    if(PRV_CPSS_DXCH_PP_MAC(devNum)->hwInfo.multiDataPath.supportMultiDataPath)
    {
        if(dpIndex >= PRV_CPSS_DXCH_PP_MAC(devNum)->hwInfo.multiDataPath.maxDp)
        {
            return GT_BAD_PARAM;
        }
    }

    PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.groupResorcesStatus.usedDescCredits[dpIndex]         = 0;
    PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.groupResorcesStatus.maxDescCredits[dpIndex]          = maxDescCredits;
    PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.groupResorcesStatus.usedHeaderCredits[dpIndex]       = 0;
    PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.groupResorcesStatus.maxHeaderCredits[dpIndex]        = maxHeaderCredits;
    PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.groupResorcesStatus.usedPayloadCredits[dpIndex]      = 0;
    PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.groupResorcesStatus.maxPayloadCredits[dpIndex]       = maxPayloadCredits;
    PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.groupResorcesStatus.trafficManagerCumBWMbps          = 0;
    PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.groupResorcesStatus.coreOverallSpeedSummary[dpIndex] = 0;

    return GT_OK;
}


/*******************************************************************************
* prvCpssDxChPortResourcesConfigDbAvailabilityCheck
*
* DESCRIPTION:
*       Resource data structure limitations checks
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum;
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Lion2.
*
* INPUTS:
*       devNum              - physical device number
*       maxDescCredits      - physical device number
*       maxHeaderCredits    - physical device number
*       maxPayloadCredits   - physical device number
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK             - on success
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChPortResourcesConfigDbAvailabilityCheck
(
    IN  GT_U8   devNum,
    IN GT_U32   dpIndex,
    IN  GT_U32  txQDescrCredits,
    IN  GT_U32  txFifoHeaderCredits,
    IN  GT_U32  txFifoPayloadCredits
)
{
    struct groupResorcesStatus_STC *groupResorcesStatusPtr;

    groupResorcesStatusPtr = &PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.groupResorcesStatus;

    if (g_doNotCheckCredits == GT_FALSE)  /* check credits */
    {
        if (txQDescrCredits + groupResorcesStatusPtr->usedDescCredits[dpIndex] > groupResorcesStatusPtr->maxDescCredits[dpIndex])
        {
            cpssOsPrintf("\n--> ERROR : TXQ credit: number (%d) of credit exceeds limit (%d)\n",
                                      groupResorcesStatusPtr->usedDescCredits[dpIndex] + txQDescrCredits,
                                      groupResorcesStatusPtr->maxDescCredits[dpIndex]);

            return GT_BAD_STATE;
        }
        if (txFifoHeaderCredits + groupResorcesStatusPtr->usedHeaderCredits[dpIndex] > groupResorcesStatusPtr->maxHeaderCredits[dpIndex])
        {
            cpssOsPrintf("\n--> ERROR : TxFIFO payload credits: number (%d) of credit exceeds limit (%d)\n",
                                    groupResorcesStatusPtr->usedHeaderCredits[dpIndex] + txFifoHeaderCredits,
                                    groupResorcesStatusPtr->maxHeaderCredits[dpIndex]);

            return GT_BAD_STATE;
        }
        if (txFifoPayloadCredits + groupResorcesStatusPtr->usedPayloadCredits[dpIndex] > groupResorcesStatusPtr->maxPayloadCredits[dpIndex])
        {
            cpssOsPrintf("\n--> ERROR : TxFIFO headers credits: number (%d) of credit exceeds limit (%d)\n",
                                    groupResorcesStatusPtr->usedHeaderCredits[dpIndex] + txFifoPayloadCredits,
                                    groupResorcesStatusPtr->maxPayloadCredits[dpIndex]);

            return GT_BAD_STATE;
        }
    }
    return GT_OK;
}


/*******************************************************************************
* prvCpssDxChPortResourcesConfigDbAdd
*
* DESCRIPTION:
*       Resource data structure DB add operation.
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum;
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Lion2.
*
* INPUTS:
*       devNum              - physical device number
*       maxDescCredits      - physical device number
*       maxHeaderCredits    - physical device number
*       maxPayloadCredits   - physical device number
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK             - on success
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChPortResourcesConfigDbAdd
(
    IN  GT_U8   devNum,    
    IN GT_U32   dpIndex,
    IN  GT_U32  txQDescrCredits,
    IN  GT_U32  txFifoHeaderCredits,
    IN  GT_U32  txFifoPayloadCredits
)
{
    PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.groupResorcesStatus.usedDescCredits[dpIndex]    += txQDescrCredits;
    PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.groupResorcesStatus.usedHeaderCredits[dpIndex]  += txFifoHeaderCredits;
    PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.groupResorcesStatus.usedPayloadCredits[dpIndex] += txFifoPayloadCredits;

    return GT_OK;
}


/*******************************************************************************
* prvCpssDxChPortResourcesConfigDbDelete
*
* DESCRIPTION:
*       Resource data structure DB delete operation.
*
* APPLICABLE DEVICES:
*       Bobcat2; Caelum;
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; Lion; xCat2; Lion2.
*
* INPUTS:
*       devNum              - physical device number
*       maxDescCredits      - physical device number
*       maxHeaderCredits    - physical device number
*       maxPayloadCredits   - physical device number
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK             - on success
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChPortResourcesConfigDbDelete
(
    IN  GT_U8   devNum,    
    IN GT_U32   dpIndex,
    IN  GT_U32  txQDescrCredits,
    IN  GT_U32  txFifoHeaderCredits,
    IN  GT_U32  txFifoPayloadCredits
)
{
    PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.groupResorcesStatus.usedDescCredits[dpIndex]    -= txQDescrCredits;
    PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.groupResorcesStatus.usedHeaderCredits[dpIndex]  -= txFifoHeaderCredits;
    PRV_CPSS_DXCH_PP_MAC(devNum)->portGroupsExtraInfo.groupResorcesStatus.usedPayloadCredits[dpIndex] -= txFifoPayloadCredits;
    
    return GT_OK;
}


