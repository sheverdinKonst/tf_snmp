/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
*
* cpssDxChTmGluePfc.c
*
* DESCRIPTION:
*       Traffic Manager Glue - PFC Private APIs
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/


#include <cpss/dxCh/dxChxGen/config/private/prvCpssDxChInfo.h>
#include <cpss/dxCh/dxChxGen/tmGlue/cpssDxChTmGluePfc.h>
#include <cpss/dxCh/dxChxGen/tmGlue/private/prvCpssDxChTmGluePfc.h>
#include <cpss/dxCh/dxChxGen/tmGlue/private/prvCpssDxChTmGlue.h>
#include <cpss/extServices/cpssExtServices.h>

static GT_BOOL pfcTmGluePfcIsInitializedArr[PRV_CPSS_MAX_PP_DEVICES_CNS] = {0};

static PRV_CPSS_DXCH_TM_GLUE_PFC_DEV_INFO_STC *pfcTmGluePfcDevInfoArr[PRV_CPSS_MAX_PP_DEVICES_CNS] = {0};

/* c node must apear only one time in tc,port to c node mapping */
GT_BOOL prvPfcTmGlueCnodeMappingOnce = GT_TRUE;

/*******************************************************************************
* prvCpssDxChTmGluePfcTmDbGetTcPortByCNode
*
* DESCRIPTION:
*       get traffic class and port by cNode from SW DB.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum      - device number.
*       cNodeValue  - C node value (APPLICABLE RANGES: 0...511).
*
* OUTPUTS:
*       portPtr     - (poinetr to) source physical port number.
*       tcPtr       - (pointer to) traffic class.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number,
*                                  port or traffic class.
*       GT_OUT_OF_RANGE          - on out of range value
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvCpssDxChTmGluePfcTmDbGetTcPortByCNode
(
    IN GT_U8                devNum,
    IN GT_U32               cNodeValue,
    IN GT_PHYSICAL_PORT_NUM *portPtr,
    IN GT_U32               *tcPtr

)
{
    GT_U32  port;
    GT_U32  tc;
    GT_U32  value;
    GT_BOOL found = GT_FALSE;
    GT_STATUS rc = GT_NOT_FOUND;

    if (pfcTmGluePfcIsInitializedArr[devNum] == GT_FALSE)
    {
        return GT_NOT_INITIALIZED;
    }

    if (pfcTmGluePfcDevInfoArr[devNum] == NULL)
    {
        return GT_INIT_ERROR;
    }


    for (port = 0; port < PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_PORT_CNS && !found; port++)
    {
        for (tc = 0; tc < PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_TC_CNS; tc++)
        {
            if (pfcTmGluePfcDevInfoArr[devNum]->TcPort2CnodeSwTable[port][tc] == cNodeValue)
            {
                *portPtr = port;
                *tcPtr = tc;
                found = GT_TRUE;
                break;
            }
        }
    }

    if (found)
    {
        /* verify sync with HW*/
        rc = prvCpssDxChReadTableEntryField(devNum,
                     PRV_CPSS_DXCH_LION3_TABLE_TM_FCU_TC_PORT_TO_CNODE_PORT_MAPPING_E,
                     *portPtr,
                     PRV_CPSS_DXCH_TABLE_WORD_INDICATE_GLOBAL_BIT_CNS,
                     *tcPtr * 9, 9, &value);

        if (rc != GT_OK)
        {
            return rc;
        }

        if (value == cNodeValue)
        {
            rc = GT_OK;
        }
    }

    return rc;
}

/*******************************************************************************
* prvCpssDxChTmGluePfcTmDbGetCNodeByTcPort
*
* DESCRIPTION:
*       get cNode by traffic class and port from SW DB.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum      - device number.
*       portNum     - source physical port number.
*       tc          - traffic class (APPLICABLE RANGES: 0..7).
* OUTPUTS:
*       cNodePtr    - (pointer to) C node value.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number,
*                                  port or traffic class.
*       GT_OUT_OF_RANGE          - on out of range value
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvCpssDxChTmGluePfcTmDbGetCNodeByTcPort
(
    IN GT_U8                devNum,
    IN GT_PHYSICAL_PORT_NUM port,
    IN GT_U32               tc,
    IN GT_U32               *cNodePtr
)
{
    GT_U32  value;
    GT_STATUS rc = GT_NOT_FOUND;

    if (pfcTmGluePfcIsInitializedArr[devNum] == GT_FALSE)
    {
        return GT_NOT_INITIALIZED;
    }

    if (pfcTmGluePfcDevInfoArr[devNum] == NULL)
    {
        return GT_INIT_ERROR;
    }



    if (port >= PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_PORT_CNS || tc >= PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_TC_CNS)
    {
        return GT_OUT_OF_RANGE;
    }

     /* verify sync with HW*/
     rc = prvCpssDxChReadTableEntryField(devNum,
                     PRV_CPSS_DXCH_LION3_TABLE_TM_FCU_TC_PORT_TO_CNODE_PORT_MAPPING_E,
                     port,
                     PRV_CPSS_DXCH_TABLE_WORD_INDICATE_GLOBAL_BIT_CNS,
                     tc * 9, 9, &value);

     if (value == pfcTmGluePfcDevInfoArr[devNum]->TcPort2CnodeSwTable[port][tc])
     {
         *cNodePtr = value;
         rc = GT_OK;
     }

    return rc;
}

/*******************************************************************************
* prvCpssDxChTmGluePfcTmDbUpdateTcPortCNode
*
* DESCRIPTION:
*       update cNodes traffic class and port changes to SW DB.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum      - device number.
*       cNodeValue  - C node value (APPLICABLE RANGES: 0...511).
*
* OUTPUTS:
*       portPtr     - (poinetr to) source physical port number.
*       tcPtr       - (pointer to) traffic class.
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number,
*                                  port or traffic class.
*       GT_OUT_OF_RANGE          - on out of range value
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvCpssDxChTmGluePfcTmDbUpdateTcPortCNode
(
    IN GT_U8                                    devNum,
    IN PRV_CPSS_DXCH_TM_GLUE_PFC_CNODE_INFO_STC *cNode1Ptr,
    IN PRV_CPSS_DXCH_TM_GLUE_PFC_CNODE_INFO_STC *cNode2Ptr
)
{
    PRV_CPSS_DXCH_TM_GLUE_PFC_DEV_INFO_STC *pfcDevInfoPtr;

    if (pfcTmGluePfcIsInitializedArr[devNum] == GT_FALSE)
    {
        return GT_NOT_INITIALIZED;
    }

    if (pfcTmGluePfcDevInfoArr[devNum] == NULL)
    {
        return GT_INIT_ERROR;
    }


    CPSS_NULL_PTR_CHECK_MAC(cNode1Ptr);
    CPSS_NULL_PTR_CHECK_MAC(cNode2Ptr);

    if (cNode1Ptr->port >= PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_PORT_CNS ||
        cNode1Ptr->tc >= PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_TC_CNS ||
        cNode1Ptr->cNode >= PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_CNODE_CNS)
    {
        return GT_OUT_OF_RANGE;
    }


    if (cNode2Ptr->port >= PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_PORT_CNS ||
        cNode2Ptr->tc >= PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_TC_CNS ||
        cNode2Ptr->cNode >= PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_CNODE_CNS)
    {
        return GT_OUT_OF_RANGE;
    }

        if (cNode1Ptr->cNode == cNode2Ptr->cNode)
        {
                if (cNode1Ptr->port != cNode2Ptr->port ||
            cNode1Ptr->tc != cNode2Ptr->tc)
        {
            return GT_BAD_VALUE;
        }
        }


    pfcDevInfoPtr = pfcTmGluePfcDevInfoArr[devNum];

    pfcDevInfoPtr->
        TcPort2CnodeSwTable[cNode1Ptr->port][cNode1Ptr->tc] = cNode1Ptr->cNode;

    pfcDevInfoPtr->
        TcPort2CnodeSwTable[cNode2Ptr->port][cNode2Ptr->tc] = cNode2Ptr->cNode;

    return GT_OK;
}

/*******************************************************************************
* prvCpssDxChTmGluePfcPortMappingGet
*
* DESCRIPTION:
*       Get physical port to pfc port mapping.
*
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum      - device number.
*       portNum     - physical port number. (APPLICABLE RANGES: 0..255).
*
* OUTPUTS:
*       pfcPortNum  - (pointer to) pfc port number which is used at
*                     cpssDxChTmGluePfcTmTcPort2CNodeSet. (APPLICABLE RANGES: 0..63).
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number, or port.
*       GT_OUT_OF_RANGE          - on out of range value
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvCpssDxChTmGluePfcPortMappingGet
(
    IN GT_U8                    devNum,
    IN GT_PHYSICAL_PORT_NUM     portNum,
    OUT GT_PHYSICAL_PORT_NUM    *pfcPortNumPtr
)
{
    GT_U32  value;                  /* HW field value */
    GT_STATUS rc = GT_OK;           /* return code */

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
          CPSS_CH1_E | CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E |
          CPSS_XCAT2_E | CPSS_LION_E | CPSS_LION2_E  | CPSS_BOBCAT3_E );

    PRV_CPSS_DXCH_PP_HW_INFO_E_ARCH_SUPPORTED_CHECK_MAC(devNum);
    PRV_CPSS_DXCH_ENHANCED_PHY_PORT_OR_CPU_PORT_CHECK_MAC(devNum, portNum);
    CPSS_NULL_PTR_CHECK_MAC(pfcPortNumPtr);

        if(PRV_CPSS_SIP_5_10_CHECK_MAC(devNum))
        {
                /* Get Physical Port To PFC port mapping Entry */
                rc = prvCpssDxChReadTableEntryField(devNum,
                                         PRV_CPSS_DXCH_LION3_TABLE_TM_FCU_PORT_TO_PHYSICAL_PORT_MAPPING_E,
                                         portNum / 4,
                                         0,
                                         (portNum % 4) * 8, 6, &value);
        }
        else
        {
                value = portNum;

                if (portNum >= PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_PORT_CNS)
                {
                        rc = GT_BAD_PARAM;
                }
        }

        if(rc != GT_OK)
        {
                return rc;
        }

    *pfcPortNumPtr = value;

    return GT_OK;
}


/*******************************************************************************
* prvCpssDxChTmGluePfcInit
*
* DESCRIPTION:
*       Init TM GLUE PFC software Data Base.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum      - device number.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number.
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS prvCpssDxChTmGluePfcInit
(
    IN GT_U8 devNum
)
{
    GT_U32  port, tc;
    GT_U32  cNode = 0;
    GT_STATUS rc;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);
    PRV_CPSS_NOT_APPLICABLE_DEV_CHECK_MAC(devNum,
          CPSS_CH1_E | CPSS_CH1_DIAMOND_E | CPSS_CH2_E | CPSS_CH3_E | CPSS_XCAT_E | CPSS_XCAT3_E |
          CPSS_XCAT2_E | CPSS_LION_E | CPSS_LION2_E  | CPSS_BOBCAT3_E );



    if (prvPfcTmGlueCnodeMappingOnce == GT_FALSE)
    {
        return GT_OK;
    }

    if (pfcTmGluePfcIsInitializedArr[devNum] == GT_FALSE)
    {
        pfcTmGluePfcDevInfoArr[devNum] = (PRV_CPSS_DXCH_TM_GLUE_PFC_DEV_INFO_STC*)cpssOsMalloc(sizeof(PRV_CPSS_DXCH_TM_GLUE_PFC_DEV_INFO_STC));

        if (pfcTmGluePfcDevInfoArr[devNum] == NULL)
        {
            return GT_OUT_OF_CPU_MEM;
        }
    }

    /* initial TC port to Cnode Indexes shadow table mapping
                          TC
                  ---------------------
                    0     ...   7
            Port
            ----
             0    cNode0  ... cNode7
             .
             .
             .
            64    cNode4  ... cNode511

    */
    for (port = 0; port < PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_PORT_CNS; port++)
    {
        for (tc = 0; tc < PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_TC_CNS; tc++)
        {
            pfcTmGluePfcDevInfoArr[devNum]->TcPort2CnodeSwTable[port][tc] = (GT_U16)cNode++;
        }
    }

    if (pfcTmGluePfcIsInitializedArr[devNum] == GT_TRUE)
    {
        /* sync HW from second init */

        for (port = 0; port < PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_PORT_CNS; port++)
        {
            for (tc = 0; tc < PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_TC_CNS; tc++)
            {
                cNode = pfcTmGluePfcDevInfoArr[devNum]->TcPort2CnodeSwTable[port][tc];

                rc = prvCpssDxChWriteTableEntryField(devNum,
                             PRV_CPSS_DXCH_LION3_TABLE_TM_FCU_TC_PORT_TO_CNODE_PORT_MAPPING_E,
                             port,
                             PRV_CPSS_DXCH_TABLE_WORD_INDICATE_GLOBAL_BIT_CNS,
                             tc * 9, 9, cNode);

                if(rc != GT_OK)
                {
                    return rc;
                }
            }
        }
    }

    pfcTmGluePfcIsInitializedArr[devNum] = GT_TRUE;

    return GT_OK;
}


#define prvCpssDxChTmGluePfc_debug_def

#ifdef prvCpssDxChTmGluePfc_debug_def

#include <cpss/dxCh/dxChxGen/port/cpssDxChPortCtrl.h>

/* CpssDxChTmGluePfcPortMapping */

GT_STATUS prvCpssDxChTmGluePfcPortMappingDump
(
    IN GT_U8 devNum
)
{
    GT_STATUS rc = GT_OK;
    GT_PHYSICAL_PORT_NUM physicalPort;
    GT_PHYSICAL_PORT_NUM pfcPort;
    GT_PHYSICAL_PORT_NUM maxPort = PRV_CPSS_DXCH_MAX_PHY_PORT_NUMBER_MAC(devNum);

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);

    cpssOsPrintf("  pfcPort     physicalPort    \n");
    for (physicalPort = 0; physicalPort < maxPort; physicalPort++)
    {
        rc = cpssDxChTmGluePfcPortMappingGet(devNum, physicalPort, &pfcPort);
        if (rc == GT_OK)
        {
            cpssOsPrintf("     %d             %d\n", physicalPort, pfcPort);
        }
        else
        {
            cpssOsPrintf("failed on physicalPort: %d, rc: %d\n", physicalPort, rc);
            break;
        }
    }

    return rc;
}

GT_STATUS prvCpssDxChTmGluePfcPortMappingDumpTest
(
    IN GT_U8 devNum
)
{
    GT_STATUS rc = GT_OK;
    GT_PHYSICAL_PORT_NUM physicalPort;
    GT_PHYSICAL_PORT_NUM pfcPort;
    GT_PHYSICAL_PORT_NUM maxPort = PRV_CPSS_DXCH_MAX_PHY_PORT_NUMBER_MAC(devNum);

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);

    for (physicalPort = 0, pfcPort = 63; physicalPort < maxPort; physicalPort+=2, pfcPort--)
    {
        rc = cpssDxChTmGluePfcPortMappingSet(devNum, physicalPort, pfcPort);
        if (rc != GT_OK)
        {
            cpssOsPrintf("cpssDxChTmGluePfcPortMappingSet: physicalPort: %d, pfcPort: %d, rc: %d\n", physicalPort, pfcPort, rc);
            break;
        }
    }

    prvCpssDxChTmGluePfcPortMappingDump(devNum);

    return rc;


}

/* CpssDxChTmGluePfcTmTcPort2CNode */

GT_STATUS prvCpssDxChTmGluePfcTmTcPort2CNodeDump
(
    IN GT_U8 devNum
)
{
    GT_STATUS   rc;
    GT_U32      port;
    GT_U32      tc;
    GT_U32      cNode;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);

    cpssOsPrintf("Port  Tc  CNode\n");
    for (port = 0; port < PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_PORT_CNS; port++)
    {
        for (tc = 0; tc < PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_TC_CNS; tc++)
        {
            rc = prvCpssDxChReadTableEntryField(devNum,
                         PRV_CPSS_DXCH_LION3_TABLE_TM_FCU_TC_PORT_TO_CNODE_PORT_MAPPING_E,
                         port,
                         PRV_CPSS_DXCH_TABLE_WORD_INDICATE_GLOBAL_BIT_CNS,
                         tc * 9, 9, &cNode);

            if(rc != GT_OK)
            {
                return rc;
            }

            if ((GT_U16)cNode != pfcTmGluePfcDevInfoArr[devNum]->TcPort2CnodeSwTable[port][tc])
            {
                cpssOsPrintf(" %d        %d           HW: %d SW: %d !!!\n",
                             port,
                             tc,
                             cNode,
                             pfcTmGluePfcDevInfoArr[devNum]->TcPort2CnodeSwTable[port][tc]);
            }
            else
            {
                cpssOsPrintf(" %d        %d           %d\n", port, tc, cNode);
            }
        }
    }

    return GT_OK;
}

GT_STATUS prvCpssDxChTmGluePfcTmTcPort2CNodeTest
(
    IN GT_U8    devNum,
    IN GT_U8    startPort,
    IN GT_U8    maxPort,
    IN GT_16    startCnode
)
{
    GT_STATUS   rc;
    GT_U8       port;
    GT_U8       tc;
    GT_U32      cNode;

    PRV_CPSS_DXCH_DEV_CHECK_MAC(devNum);

    cNode = startCnode;

    for (port = startPort; port <= MIN(PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_PORT_CNS-1, maxPort); port++)
    {
        for (tc = 0; tc < PRV_CPSS_DXCH_TM_GLUE_PFC_MAX_TC_CNS && cNode < 512; tc++, cNode++)
        {
            cpssOsPrintf("Test(%d,%d,%d,%d)\n",devNum, port, tc, cNode);
            rc = cpssDxChTmGluePfcTmTcPort2CNodeSet(devNum,
                                                    port,
                                                    tc,
                                                    cNode);

            if(rc != GT_OK)
            {
                return rc;
            }
        }
    }

    rc = prvCpssDxChTmGluePfcTmTcPort2CNodeDump(devNum);

    return rc;
}


/*******************************************************************************
* prvCpssDxChTmGlueFlowControlPortSpeedGet
*
* DESCRIPTION:
*       Get port speed and calibration value.
*
* APPLICABLE DEVICES:
*       Bobcat2.
*
* NOT APPLICABLE DEVICES:
*       DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Lion3.
*
* INPUTS:
*       devNum      - device number.
*       portNum     - physical port number.
*
* OUTPUTS:
*           speedPtr    - (pointer to) port speed in MB
*       PfcTimer    - (pointer to) PfcTimer Value
*
*
* RETURNS:
*       GT_OK                    - on success.
*       GT_BAD_PARAM             - on wrong device number.
*       GT_HW_ERROR              - on hardware error.
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device.
*
* COMMENTS:
*       This table is the calibration value to be multiplied to the value in the PFC header
*           to be alligned to the number of cycles according to the port speed.
*
*******************************************************************************/
GT_STATUS prvCpssDxChTmGlueFlowControlPortSpeedGet
(
    IN  GT_U8                   devNum,
    IN  GT_PHYSICAL_PORT_NUM        portNum,
    OUT GT_U32                                *speedPtr,
        OUT        GT_U32                          *pfcTimerValue
)
{
    GT_U32 value;           /* register value */
        GT_U32 speedForCalc;
        GT_STATUS rc = GT_OK;
        CPSS_PORT_SPEED_ENT        speed = CPSS_PORT_SPEED_NA_E;
        GT_PHYSICAL_PORT_NUM pfcPortNum = 0;

        /* PFC with TM supported only for first 63 physical ports for BC2 version < B0 */
        rc = prvCpssDxChTmGluePfcPortMappingGet(devNum, portNum, &pfcPortNum);

        if (rc != GT_OK)
        {
                return rc;
        }

        rc = cpssDxChPortSpeedGet(devNum, portNum, &speed);
        if (rc != GT_OK)
        {
                speed = CPSS_PORT_SPEED_NA_E;
        }

    switch(speed)
    {
        case CPSS_PORT_SPEED_10_E:
            speedForCalc = 10;
            break;
        case CPSS_PORT_SPEED_100_E:
            speedForCalc = 100;
            break;
        case CPSS_PORT_SPEED_1000_E:
            speedForCalc = 1000;
            break;
        case CPSS_PORT_SPEED_2500_E:
            speedForCalc = 2500;
            break;
        case CPSS_PORT_SPEED_5000_E:
            speedForCalc = 5000;
            break;
        case CPSS_PORT_SPEED_10000_E:
            speedForCalc = 10000;
            break;
        case CPSS_PORT_SPEED_12000_E:
            speedForCalc = 12000;
            break;
        case CPSS_PORT_SPEED_20000_E:
            speedForCalc = 20000;
            break;
        case CPSS_PORT_SPEED_40000_E:
            speedForCalc = 40000;
            break;
        default:
            speedForCalc = 0;
    }

    /* Get Port Speed Entry - Port Speed */
    rc =  prvCpssDxChReadTableEntryField(devNum,
                 PRV_CPSS_DXCH_LION3_TABLE_TM_FCU_PORT_INGRESS_TIMERS_CONFIG_E,
                 pfcPortNum,
                 PRV_CPSS_DXCH_TABLE_WORD_INDICATE_GLOBAL_BIT_CNS,
                 0, 32, &value);

        if (rc != GT_OK)
        {
                return rc;
        }

        *speedPtr = speedForCalc;
        *pfcTimerValue = value;

        return rc;
}

GT_STATUS prvCpssDxChTmGlueFlowControlPortSpeedDump
(
    IN GT_U8        devNum,
    IN GT_U32        fromPort,
        IN GT_U32          toPort
)
{
    GT_U32 pfcTimerValue;
        GT_U32 speed;
        GT_STATUS rc = GT_OK;
        GT_U32 port;

        for (port = fromPort; port <= toPort; port++)
        {
                rc = prvCpssDxChTmGlueFlowControlPortSpeedGet(devNum, port, &speed, &pfcTimerValue);
                if (rc != GT_OK)
                {
                        return rc;
                }

                cpssOsPrintf("dev: %d, port: %02d, speed: %d[MB], pfcTimer: %d\n",
                                         devNum,
                                         port,
                                         speed,
                                         pfcTimerValue);
        }

        return rc;
}


#endif

