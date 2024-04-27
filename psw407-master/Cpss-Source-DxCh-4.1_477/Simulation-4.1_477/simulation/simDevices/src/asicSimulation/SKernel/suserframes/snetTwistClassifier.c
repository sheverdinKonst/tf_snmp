/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetTwistClassifier.c
*
* DESCRIPTION:
*       classifier Engine of TwistC
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 5 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/suserframes/snetTwistPcl.h>
#include <asicSimulation/SKernel/suserframes/snetTwistEgress.h>
#include <asicSimulation/SKernel/suserframes/snetTwistL2.h>
#include <asicSimulation/SKernel/smem/smemTwist.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/twistCommon/sregTwist.h>
#include <asicSimulation/SKernel/suserframes/snetSambaPolicy.h>
#include <asicSimulation/SKernel/suserframes/snetTwistClassifier.h>
#include <asicSimulation/SLog/simLog.h>

static void snetTwistCClassificationConvertTwistFormat2TwistCFormat(
    IN    SNET_POLICY_ACTION_STC              *twistPolicyActionPtr,
    INOUT SNET_TWIST_CLASSIFIER_FINAL_ACTION  *twistCPolicyActionPtr
);

static void snetTwistCClassificationConvertTwistCFormat2TwistFormat(
    INOUT    SNET_POLICY_ACTION_STC             * twistPolicyActionPtr,
    IN       SNET_TWIST_CLASSIFIER_FINAL_ACTION * twistCPolicyActionPtr
);

static GT_BOOL snetTwistCIpFlowGet
(
    IN    SKERNEL_DEVICE_OBJECT               * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC             * descrPtr,
    INOUT SNET_TWIST_CLASSIFIER_FINAL_ACTION  * policyActionPtr
);

static void snetTwistCDefaultTemplateFlowGet
(
    IN    SKERNEL_DEVICE_OBJECT               * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC             * descrPtr,
    INOUT SNET_TWIST_CLASSIFIER_FINAL_ACTION  * policyActionPtr
);

static void snetTwistCClassificationSearch
(
    IN    SKERNEL_DEVICE_OBJECT              * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC            * descrPtr,
    OUT   GT_U8                searchKeyPtr[SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS],
    INOUT SNET_TWIST_CLASSIFIER_FINAL_ACTION * policyActionPtr,
    OUT   GT_BOOL                            * classifierFoundPtr,
    OUT   SNET_POLICY_ACTION_SRC_ENT           actionSrc
);

static GT_BOOL snetTwistCGfcSearch
(
    IN    SKERNEL_DEVICE_OBJECT               * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC             * descrPtr,
    OUT   GT_U8                searchKeyPtr[SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS],
    INOUT SNET_TWIST_CLASSIFIER_FINAL_ACTION  * policyActionPtr
);

static GT_BOOL snetTwistCClassifierConvertToPolicy
(
    IN  SKERNEL_DEVICE_OBJECT              * devObjPtr,
    IN  GT_U32                               keyTableAddr,
    OUT SNET_TWIST_CLASSIFIER_FINAL_ACTION * policyActionPtr,
    IN  GT_U8                              * searchKeyPtr,
    IN  GT_U32                               maxGroupIndex,
    IN  GT_U32                               flowEntrySize
);

static GT_BOOL snetTwistCClassificationActionApply
(
    IN SKERNEL_DEVICE_OBJECT                  * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC                * descrPrt,
    IN SNET_TWIST_CLASSIFIER_FINAL_ACTION     * policyActionPtr,
    IN SNET_POLICY_ACTION_SRC_ENT               actionSrc
);



static void snetTwistCClassifierDefaultActionsSet(
    OUT SNET_TWIST_CLASSIFIER_FINAL_ACTION *policyActionPtr
);


/*******************************************************************************
*   snetTwistCClassificationEngine
*
* DESCRIPTION:
*        the function do the classifier engine to get an action info
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        classifierPolicyActionPtr - hold reset values of this info
* OUTPUTS:
*        classifierPolicyActionPtr - pointer to where to put the actions info
*        classifierFoundPtr - pointer to return if the gfc/ip found entry or
*                             the default entry was taken .
*
*******************************************************************************/
GT_BOOL snetTwistCClassificationEngine
(
    IN    SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistCClassificationEngine);

    GT_U8              searchKeyArray[SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS];
    SNET_TWIST_CLASSIFIER_FINAL_ACTION policyAction; /* policy action entry */
    SNET_POLICY_ACTION_SRC_ENT         actionSrc;    /* action to apply */
    GT_BOOL                            classifierFoundPtr;

    actionSrc = 0;

    snetTwistCClassifierDefaultActionsSet(&policyAction);

    if (snetSambaClassificationCheck(devObjPtr, descrPtr) == GT_FALSE)
    {
        /* no classification cmd */
        __LOG(("no classification cmd"));
        return GT_TRUE;
    }

    /* Build Classifier search key */
    snetSambaClassificationKeyCreate(devObjPtr, descrPtr, searchKeyArray);

    snetTwistCClassificationSearch(devObjPtr, descrPtr, searchKeyArray,
                                &policyAction, &classifierFoundPtr, actionSrc);

    /* Process action */
    __LOG(("Process action"));
    return snetTwistCClassificationActionApply(devObjPtr, descrPtr, &policyAction,
                                      actionSrc);

}

/*******************************************************************************
*   snetTwistCClassificationConvertTwistFormat2TwistCFormat
*
* DESCRIPTION:
*        convert policy stc from twist to samba
*
* INPUTS:
*        twistPolicyActionPtr - the values as twist simulation holds it
*        twistCPolicyActionPtr - hold reset values of this info
* OUTPUTS:
*        twistCPolicyActionPtr - the info in the TwistC simulation format
*
* RETURNS:
*
*
* COMMENT:
*
*******************************************************************************/
static void snetTwistCClassificationConvertTwistFormat2TwistCFormat(
    IN    SNET_POLICY_ACTION_STC             * twistPolicyActionPtr,
    INOUT SNET_TWIST_CLASSIFIER_FINAL_ACTION * twistCPolicyActionPtr
)
{
    twistCPolicyActionPtr->cmd = twistPolicyActionPtr->cmd;
    twistCPolicyActionPtr->cpuMirror = twistPolicyActionPtr->cpuMirror;
    twistCPolicyActionPtr->markCmd = twistPolicyActionPtr->markCmd;
    twistCPolicyActionPtr->dscp = twistPolicyActionPtr->dscp;

    /* mapping GFC Activate TC/ Cnt to policy */
    if (twistPolicyActionPtr->cmd ==
            SKERNEL_PCL_PASS_SEND_TC_E)/* pass and tc */
    {
        twistCPolicyActionPtr->tcPtr = twistPolicyActionPtr->tcCosParam;
    }
    else
    {
        twistCPolicyActionPtr->dp =
            (GT_U8)(twistPolicyActionPtr->tcCosParam) & 0x3 ;
        twistCPolicyActionPtr->tc =
            (GT_U8)(twistPolicyActionPtr->tcCosParam >> 2) & 0x7 ;
        twistCPolicyActionPtr->up =
            (GT_U8)(twistPolicyActionPtr->tcCosParam >> 5) & 0x7 ;
    }
}

/*******************************************************************************
*   snetTwistCClassificationConvertTwistCFormat2TwistFormat
*
* DESCRIPTION:
*        convert policy stc from twist to samba
*
* INPUTS:
*        twistPolicyActionPtr - the values as twist simulation holds it
*        twistCPolicyActionPtr - hold reset values of this info
* OUTPUTS:
*        twistPolicyActionPtr - the info in the twistD pcl simulation format
*
* RETURNS:
*
*
* COMMENT:
*
*******************************************************************************/
static void snetTwistCClassificationConvertTwistCFormat2TwistFormat(
    INOUT    SNET_POLICY_ACTION_STC             * twistPolicyActionPtr,
    IN       SNET_TWIST_CLASSIFIER_FINAL_ACTION * twistCPolicyActionPtr
)
{
    twistPolicyActionPtr->cmd = twistCPolicyActionPtr->cmd;
    twistPolicyActionPtr->cpuMirror = twistCPolicyActionPtr->cpuMirror;
    twistPolicyActionPtr->markCmd = twistCPolicyActionPtr->markCmd;
    twistPolicyActionPtr->dscp = twistCPolicyActionPtr->dscp;

    if (twistPolicyActionPtr->cmd ==
            SKERNEL_PCL_PASS_SEND_TC_E)/* pass and tc */
    {
        twistPolicyActionPtr->tcCosParam = twistCPolicyActionPtr->tcPtr;
    }
    else
    {
        twistPolicyActionPtr->tcCosParam =
            (twistCPolicyActionPtr->dp & 0x3) |
            ((twistCPolicyActionPtr->tc & 0x7) << 2) |
            ((twistCPolicyActionPtr->up & 0x7) << 5);
    }
}

/*******************************************************************************
*   snetTwistCIpFlowGet
*
* DESCRIPTION:
*        search in CoS for classification actions , fill the info in policyActionPtr
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        policyActionPtr - hold reset values of this info
* OUTPUTS:
*        policyActionPtr - pointer to where to put the actions info
*
* RETURNS:
*       GT_TRUE - if entry found in the Cos
*       GT_FALSE - otherwise
* COMMENT:
*       use twist function
*******************************************************************************/
static GT_BOOL snetTwistCIpFlowGet
(
    IN    SKERNEL_DEVICE_OBJECT               * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC             * descrPtr,
    INOUT SNET_TWIST_CLASSIFIER_FINAL_ACTION  * policyActionPtr
)
{
    DECLARE_FUNC_NAME(snetTwistCIpFlowGet);

    SNET_POLICY_ACTION_STC twistPolicyActionInfo;

    if(GT_FALSE == snetTwistCosTablesLookup(devObjPtr,descrPtr,
                    &twistPolicyActionInfo))
    {
        return GT_FALSE;
    }

    /* convert SNET_POLICY_ACTION_STC to SNET_TWIST_CLASSIFIER_FINAL_ACTION */
    __LOG(("convert SNET_POLICY_ACTION_STC to SNET_TWIST_CLASSIFIER_FINAL_ACTION"));
    snetTwistCClassificationConvertTwistFormat2TwistCFormat(&twistPolicyActionInfo,
        policyActionPtr);

    return GT_TRUE;
}



/*******************************************************************************
*   snetTwistCDefaultTemplateFlowGet
*
* DESCRIPTION:
*        get template default classification actions ,
*        fill the info in policyActionPtr
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        policyActionPtr - hold reset values of this info
* OUTPUTS:
*        policyActionPtr - pointer to where to put the actions info
*
* RETURNS:
*       GT_TRUE - if entry found in the Cos
*       GT_FALSE - otherwise
* COMMENT:
*       use twist function
*******************************************************************************/
static void snetTwistCDefaultTemplateFlowGet
(
    IN    SKERNEL_DEVICE_OBJECT               * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC             * descrPtr,
    INOUT SNET_TWIST_CLASSIFIER_FINAL_ACTION  * policyActionPtr
)
{
    DECLARE_FUNC_NAME(snetTwistCDefaultTemplateFlowGet);

    SNET_POLICY_ACTION_STC twistPolicyActionInfo;

    snetTwisDefTemplActionGet(devObjPtr,descrPtr,&twistPolicyActionInfo);

    /* convert SNET_POLICY_ACTION_STC to SNET_TWIST_CLASSIFIER_FINAL_ACTION */
    __LOG(("convert SNET_POLICY_ACTION_STC to SNET_TWIST_CLASSIFIER_FINAL_ACTION"));
    snetTwistCClassificationConvertTwistFormat2TwistCFormat(&twistPolicyActionInfo,
        policyActionPtr);

    return ;
}

/*******************************************************************************
*   snetTwistCClassificationSearch
*
* DESCRIPTION:
*        search for classification actions , fill the info in policyAction
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        searchKeyPtr - pointer to the search key
*        policyActionPtr - hold reset values of this info
* OUTPUTS:
*        policyActionPtr - pointer to where to put the actions info
*        classifierFoundPtr - pointer to return if the gfc/ip found entry or
*                             the default entry was taken .
*
* COMMENT:
*        from 12.4 [1]
*        similar to snetSambaClassificationSearch().
*******************************************************************************/
static void snetTwistCClassificationSearch
(
    IN    SKERNEL_DEVICE_OBJECT              * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC            * descrPtr,
    OUT   GT_U8                searchKeyPtr[SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS],
    INOUT SNET_TWIST_CLASSIFIER_FINAL_ACTION * policyActionPtr,
    OUT   GT_BOOL                            * classifierFoundPtr,
    OUT   SNET_POLICY_ACTION_SRC_ENT           actionSrc
)
{
    GT_BOOL doIp = GT_FALSE; /* need to do IP classification */
    GT_BOOL doClassifier = GT_FALSE;/* need to do GFC */
    GT_BOOL entryFound = GT_FALSE;
    GT_U32 lookUpMode;

    smemRegFldGet(devObjPtr, TCB_CONTROL_REG, 1, 2,&lookUpMode);

    if( lookUpMode == 0)
    {
        doClassifier = GT_TRUE;
    }
    else if( lookUpMode == 1)
    {
        doClassifier = GT_TRUE;
        doIp = GT_TRUE;
    }

    if(doClassifier)
    {
        entryFound = snetTwistCGfcSearch(devObjPtr, descrPtr,searchKeyPtr,
                                        policyActionPtr);
        actionSrc = SNET_POLICY_ACTION_GFC_E;
    }

    if(entryFound == GT_FALSE && doIp)
    {
        entryFound = snetTwistCIpFlowGet(devObjPtr, descrPtr,policyActionPtr);
        actionSrc = SNET_POLICY_ACTION_IP_TABLES_E;
    }

    *classifierFoundPtr = entryFound;

    if(entryFound == GT_FALSE)
    {
        snetTwistCDefaultTemplateFlowGet(devObjPtr, descrPtr,policyActionPtr);
        actionSrc = SNET_POLICY_ACTION_TEMPLATE_E;
    }

    return ;
}

/*******************************************************************************
*   snetTwistCGfcSearch
*
* DESCRIPTION:
*        search in GFC for classification actions , fill the info in policyActionPtr
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        searchKeyPtr - pointer to the search key
*        policyActionPtr - hold reset values of this info
* OUTPUTS:
*        policyActionPtr - pointer to where to put the actions info
*
* RETURNS:
*       GT_TRUE - if entry found in the GFC
*       GT_FALSE - otherwise
* COMMENT:
*       similar to snetSambaGfcSearch().
*
*******************************************************************************/
static GT_BOOL snetTwistCGfcSearch
(
    IN    SKERNEL_DEVICE_OBJECT              * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC            * descrPtr,
    OUT   GT_U8                searchKeyPtr[SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS],
    INOUT SNET_TWIST_CLASSIFIER_FINAL_ACTION * policyActionPtr
)
{
    DECLARE_FUNC_NAME(snetTwistCGfcSearch);

    GT_U32  tempIndex; /* index of the template.*/
    GT_U32  BaseAddress ; /* This field shows the base address to be added to the
                            final hash value (after csh and AdWidth) in 128
                            entries resolution.*/
    GT_U32  hashAddr ; /*the address in the hash table where the hash entry
                        should exist.*/
    GT_U32  keyTableAddr ; /*the address where the actions exist*/
    GT_U8   offset;
    GT_U8   range;
    GT_U32  groupIndex;/* the index of the vl-tri in the array of leaf */
    GT_U32  flowEntrySize;/* the size in bytes of the flow entry */
    GT_U32  regValue;/* value of register*/
    GT_U32  byteNum ;
    GT_U32  fieldValue;
    GT_U32  keyTableSize;/* the key table size in bytes */
    GT_BOOL finalMatch;/* final matching in the flow table*/

    SNET_SAMBA_FLOW_TEMPLATE_HASH  hashInfo;/*see registers 0x02C28000-0x02C2803C*/
    SNET_SAMBA_CLASSIFIER_HASH_ENTRY *hashEntryInfoPtr;/* table info from 4.7 [2] */
    SNET_SAMBA_CLASSIFIER_VL_TRIE_ENTRY *validVlTrieEntryPtr;/* table info from 4.7 [2]*/

    tempIndex = (descrPtr->majorTemplate <<3) | descrPtr->flowTemplate;

    smemRegGet(devObjPtr, FLOW_TEMPLATE_HASH_CONFIGURATION_REG+tempIndex*4,
                (void *)&hashInfo);

    /* get base address for the hashAdresses of that template */
    __LOG(("get base address for the hashAdresses of that template"));
    BaseAddress = hashInfo.baseAddress<<10 ;/*22 bits for base address*/

    /*Compute Hash value*/
    hashAddr = snetSambaGfcFlowHashAddrGet(devObjPtr, descrPtr,
                                           &hashInfo , searchKeyPtr , tempIndex);

    hashAddr += BaseAddress;

    /* base address for the flow dram or the internal registers memory */
    hashAddr |= SNET_SAMBA_CLASSIFIER_FLOW_BASE_ADDRESS_MAC(devObjPtr);

    hashEntryInfoPtr = smemMemGet(devObjPtr,hashAddr);

    if (hashEntryInfoPtr->valid == 0)
    {
        return GT_FALSE ;
    }

    if(hashEntryInfoPtr->col == 1)
    {
        /* the entry has collisions */
        __LOG(("the entry has collisions"));
        if(GT_FALSE == snetSambaGfcFlowVlTriSearch(devObjPtr, descrPtr,
                                            hashEntryInfoPtr,searchKeyPtr,
                                            &validVlTrieEntryPtr))
        {
            return GT_FALSE;
        }

        /* the entry is valid with no collisions */
/*        keyTableAddr = (validVlTrieEntryPtr->addressLsb>>1);*/
        keyTableAddr = ((validVlTrieEntryPtr->addressLsb>>1) |
          ((validVlTrieEntryPtr->Offset0_or_addressMsb << 21)))<<3;

        /* the offset to the group 0 in the key that cause a collision in the
        hash function */
        offset = (GT_U8)validVlTrieEntryPtr->offset1;
        range = (GT_U8)validVlTrieEntryPtr->colRange1 + 1 ;
    }
    else
    {
        /* the entry is valid with no collisions */
        __LOG(("the entry is valid with no collisions"));
        keyTableAddr = (hashEntryInfoPtr->addressLsb |
                        (hashEntryInfoPtr->Offset0_or_addressMsb <<21)) << 3;

        /* the offset to the group 0 in the key that cause a collision in the
        hash function */
        offset = (GT_U8)hashEntryInfoPtr->offset1;
        range = (GT_U8)hashEntryInfoPtr->colRange1 + 1 ;
    }



    if( offset == 0xFF )
    {
        groupIndex = 0;
    }
    else
    {
        if((offset+range)>=(SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS*8))
        {
            skernelFatalError(" snetSambaGfcFlowVlTriSearch: offset[%d] + range[%d]"\
                              " out of max size [%d]",
                              offset,
                              range,
                              (SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS*8));
        }
        groupIndex = snetSambeGfcDiffBitVal(offset,range,searchKeyPtr);
    }

    /* every template has 8 bits */
    fieldValue = snetUtilGetContinuesValue(devObjPtr,
            FLOW_TEMPLATE_CONF_REG,8,tempIndex);

    /* get the size of the search key size */
    byteNum = (fieldValue >> 0) & 0x1f ;/* 5 bits */

    /* get the size of the flow entry */
    flowEntrySize =
        SNET_SAMBA_CLASSIFIER_FLOW_ENTRY_NUM_OF_BYTES_MAC(devObjPtr,byteNum);

    /*keyTableAddr += groupIndex * flowEntrySize;*/

    /* check validation with the size of the flow table size */
    smemRegGet(devObjPtr, SAMBA_KEY_TABLE_SIZE_REG,&regValue);

    keyTableSize = SNET_SAMBA_CLASSIFIER_FLOW_KEY_TABLE_SIZE(devObjPtr,regValue);

    /* it seems that keyTableAddr > keyTableSize */
    keyTableAddr %= keyTableSize;

    if( keyTableSize < (keyTableAddr + flowEntrySize))
    {
        /* the index is out the flow table range */
        skernelFatalError(" snetSambaGfcSearch: the address is out the flow table range"\
        " (regValue[0x%8.8x] & 0x03fffffc) < (keyTableAddr[0x%8.8x] + flowEntrySize[0x%8.8x])" ,
        regValue,keyTableAddr,flowEntrySize);
    }

    /* get the base address of the flow table*/
    smemRegGet(devObjPtr, SAMBA_KEY_TABLE_BASE_ADDRESS_REG,&regValue);


    keyTableAddr +=
        SNET_SAMBA_CLASSIFIER_FLOW_KEY_TABLE_ADDRESS(devObjPtr,regValue);
            /* 29 bits -- but in 4 bytes resolution (1 word )*/

    /* base address for the flow dram or the internal registers memory */
    keyTableAddr |= SNET_SAMBA_CLASSIFIER_FLOW_BASE_ADDRESS_MAC(devObjPtr);

    /* FILL policyActionPtr from the flow entry info in address KeyTableAddr  */
    finalMatch = snetTwistCClassifierConvertToPolicy(devObjPtr,keyTableAddr,
                                        policyActionPtr,
                                        searchKeyPtr,
                                        groupIndex,
                                        flowEntrySize);

    return finalMatch;
}

/*******************************************************************************
*   snetTwistCClassifierConvertToPolicy
*
* DESCRIPTION:
*        the function convert info form address keyTableAddr to the format of
*        policy into the policyActionPtr
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        keyTableAddr  -- address where the flow entry exist
*        policyActionPtr - hold reset values of this info
*        searchKeyPtr - pointer to the search key
*        maxGroupIndex - the number of flows to check for match in flow tale
*        flowEntrySize - the size of flow entry
* OUTPUTS:
*        policyActionPtr - pointer to where to put the actions info
*
* RETURNS:
*        GT_TRUE -  if flow entry matched
*        GT_FALSE - if flow entry not matched
* COMMENT:
*       similar to snetSambaClassifierConvertToPolicy().
*
*******************************************************************************/
static GT_BOOL snetTwistCClassifierConvertToPolicy
(
    IN  SKERNEL_DEVICE_OBJECT               * devObjPtr,
    IN  GT_U32                                keyTableAddr,
    OUT SNET_TWIST_CLASSIFIER_FINAL_ACTION  * policyActionPtr,
    IN  GT_U8                               * searchKeyPtr,
    IN  GT_U32                                maxGroupIndex,
    IN  GT_U32                                flowEntrySize
)
{
    SNET_TWIST_CLASSIFIER_ACTION classifierActions;
    GT_U8   *memoryPtr;
    GT_BOOL flowPattentMatched = GT_FALSE;

    memoryPtr = smemMemGet(devObjPtr,keyTableAddr);

    /* find the matching pattern */
    while(GT_TRUE)
    {
        /* check that the entry is valid */
        if(memoryPtr[0]&0x01)
        {
            if(0==memcmp(memoryPtr+12,searchKeyPtr,(flowEntrySize-12)))
            {
                flowPattentMatched = GT_TRUE ;
                break;
            }
        }

        /* decrement the num of searches */
        if(0==maxGroupIndex)
        {
            /* no more search to do */
            break;
        }

        maxGroupIndex--;
        memoryPtr+=flowEntrySize ;
    }

    if(flowPattentMatched == GT_FALSE)
    {
        return GT_FALSE;
    }

    memoryPtr ++; /* the actions start at the second byte of the flow entry */
    /*SET classifierActions ACCORDING to memory in keyTableAddr*/
    memcpy(&classifierActions,memoryPtr,sizeof(classifierActions));

    policyActionPtr->cmd = classifierActions.word0.fields.cmd;

    /* mapping GFC Activate TC/ Cnt to policy */
    if (classifierActions.word0.fields.cmd ==
            SKERNEL_PCL_PASS_SEND_TC_E)/* pass and tc */
    {
        policyActionPtr->tcPtr =
            (GT_U32)classifierActions.word0.fields.tc_PtrOrCosParam;
    }
    else
    {
        policyActionPtr->dp =
            (GT_U8)(classifierActions.word0.fields.tc_PtrOrCosParam) & 0x3;

        policyActionPtr->tc =
            (GT_U8)(classifierActions.word0.fields.tc_PtrOrCosParam >> 2) & 0x7;

        policyActionPtr->up =
            (GT_U8)(classifierActions.word0.fields.tc_PtrOrCosParam >> 5) & 0x7;
    }

    policyActionPtr->dscp=
        (GT_U32)classifierActions.word0.fields.dscp;
    policyActionPtr->markCmd =
        (GT_U32)classifierActions.word0.fields.mark_Cmd;
    policyActionPtr->cmdRedirect =
        (GT_U32)classifierActions.word0.fields.redirectFlowCmd;
    policyActionPtr->cpuMirror =
        (GT_U32)classifierActions.word0.fields.mirrorToCpu;

    policyActionPtr->outLif0To19OrIPv4orMpls_Ptr =
        (classifierActions.word1.flowSwitching.outlif0To19OrIPv4_Ptr_msb<<1) |
        classifierActions.word0.fields.outlif0To19OrIPv4_Ptr_lsb;

    if (policyActionPtr->cmdRedirect == (SKERNEL_FLOW_REDIRECT_ENT)SKERNEL_PCL_OUTLIF_FRWD_E)
    {
        policyActionPtr->keyByteMask =
            (GT_U32)classifierActions.word1.flowSwitching.key_byte_mask;

        policyActionPtr->outLif20To41 =
            (GT_U32)classifierActions.word2.flowSwitching.outLif20To41;

        policyActionPtr->outLifType =
            (GT_U32)classifierActions.word2.flowSwitching.outLifType;
    }
    else
    {
        policyActionPtr->keyByteMask =
            (classifierActions.word2.noFlowSwitching.user_DefinedOrkeyByteMask_msb<<8) |
            classifierActions.word1.noFlowSwitching.user_DefinedOrkeyByteMask_lsb;
    }

    return GT_TRUE;
}

/*******************************************************************************
*  snetTwistCClassificationActionApply
*
* DESCRIPTION:
*      Apply actions
* INPUTS:
*      devObjPtr     - pointer to device object.
*      descrPtr         - pointer to the frame's descriptor.
*      policyActionPtr  - pointer to policy action entry
*      actionSrc        - action to apply
* OUTPUTS:
*
*      policyActionPtr  - pointer to policy action entry
*
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static GT_BOOL snetTwistCClassificationActionApply
(
    IN SKERNEL_DEVICE_OBJECT                  * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC                * descrPtr,
    IN SNET_TWIST_CLASSIFIER_FINAL_ACTION     * policyActionPtr,
    IN SNET_POLICY_ACTION_SRC_ENT               actionSrc
)
{
    DECLARE_FUNC_NAME(snetTwistCClassificationActionApply);

    SNET_POLICY_ACTION_STC twistPolicyAction;

    if (policyActionPtr->cmd == SKERNEL_PCL_DROP_E)
    {
        /* instead of calling static snetTwistDropPacket(...) */
        __LOG(("instead of calling static snetTwistDropPacket(...)"));
        descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
        descrPtr->useVidx = 0;
        descrPtr->bits15_2.useVidx_1.vidx = (GT_U16)POLICY_DROP_CODE;
        return GT_TRUE;
    }
    else
    if (policyActionPtr->cmd == SKERNEL_PCL_TRAP_E)
    {
        if (actionSrc == SNET_POLICY_ACTION_GFC_E)
        {
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                TWIST_CPU_TCB_KEY_ENTRY;
        }
        else
        if (actionSrc == SNET_POLICY_ACTION_TEMPLATE_E)
        {
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                TWIST_CPU_TCB_TRAP_DEFAULT;
        }
        if (actionSrc == SNET_POLICY_ACTION_IP_TABLES_E)
        {
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                TWIST_CPU_TCB_TRAP_COS;
        }
        descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
        descrPtr->useVidx = 0;

        return GT_TRUE;
    }
    else
    {
        if (policyActionPtr->cpuMirror == 1)
        {
            descrPtr->pktCmd = SKERNEL_PKT_MIRROR_CPU_E;
            snetTwistTx2Cpu(devObjPtr, descrPtr, TWIST_CPU_TCB_CPU_MIRROR_FIN);
        }
        descrPtr->pktCmd = SKERNEL_PKT_FORWARD_E;
        if (policyActionPtr->cmdRedirect == SKERNEL_FLOW_IPV4_REDIRECT_E)
        {
            descrPtr->policySwitchType = POLICY_ROUTE_E;
            descrPtr->nextLookupPtr = policyActionPtr->outLif0To19OrIPv4orMpls_Ptr;
        }
        else
        if (policyActionPtr->cmdRedirect == SKERNEL_FLOW_OUTLIF_REDIRECT_E)
        {
            descrPtr->policySwitchType = POLICY_SWITCH_E;
            if (policyActionPtr->outLifType == 0) /* LL OutLif */
            {
                descrPtr->outLifType = 0;
                descrPtr->LLL_outLif = policyActionPtr->outLif0To19OrIPv4orMpls_Ptr;
            }
            else if (policyActionPtr->outLifType == 1) /* Tunnel OutLif */
            {
                descrPtr->outLifType = 1;
                descrPtr->Tunnel_outLif0To31 =
                        (GT_U32)(((policyActionPtr->outLif20To41) << 20) |
                        policyActionPtr->outLif0To19OrIPv4orMpls_Ptr);
                descrPtr->Tunnel_outLif32To41 =
                        (GT_U16)((policyActionPtr->outLif20To41 & 0x3FFFFF) >> 12);
            }
        }
        else
        if (policyActionPtr->cmdRedirect == SKERNEL_FLOW_MPLS_REDIRECT_E)
        {
            descrPtr->mplsPtr = policyActionPtr->outLif0To19OrIPv4orMpls_Ptr;
            descrPtr->policySwitchType = POLICY_SWITCH_E; /* ??? */
        }

    }

    snetTwistCClassificationConvertTwistCFormat2TwistFormat(
                                            &twistPolicyAction, policyActionPtr);

    snetTwistPolicyMark(devObjPtr, descrPtr, &twistPolicyAction);

    return GT_TRUE;
}

/*******************************************************************************
*   snetTwistCPolicyDefaultActionsSet
*
* DESCRIPTION:
*       the function set default values to the input pointer
*
* INPUTS:
*
* OUTPUTS:
*       policyActionPtr - pointer to a structure that hold the actions in
*                       the policy format
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void snetTwistCClassifierDefaultActionsSet(
    OUT SNET_TWIST_CLASSIFIER_FINAL_ACTION *policyActionPtr
)
{
    memset(policyActionPtr,0,sizeof(SNET_TWIST_CLASSIFIER_FINAL_ACTION));

    policyActionPtr->cmd =  SKERNEL_PCL_PASS_NO_SEND_TC_E;
    policyActionPtr->markCmd = SKERNEL_PCL_L2_COS_E ;

    return;
}


