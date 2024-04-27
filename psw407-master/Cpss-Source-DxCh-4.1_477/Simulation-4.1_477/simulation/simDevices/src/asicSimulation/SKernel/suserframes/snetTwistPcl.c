/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetTwistPcl.c
*
* DESCRIPTION:
*       Policy (PCL and IP-Classifier) processing for frame
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 11 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/suserframes/snetTwistPcl.h>
#include <asicSimulation/SKernel/suserframes/snetTwistEgress.h>
#include <asicSimulation/SKernel/suserframes/snetTwistL2.h>
#include <asicSimulation/SKernel/smem/smemTwist.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/twistCommon/sregTwist.h>
#include <asicSimulation/SKernel/suserframes/snetSambaPolicy.h>
#include <asicSimulation/SLog/simLog.h>

/* the entries' size in the ft select table */
#define SELECT_PCL_TABLE_ENTRY_SIZE_BYTES           (48)
#define SNET_INVALID_PCE_IDX                        (0xffffffff)


/* number of rules in one row */
#define TWISTD_RULE_PER_ROW_NUMBER                  (8)
/* TCP header size */
#define TCP_HEADER_SIZE                             (0x14)
/* TCP header size */
#define UDP_HEADER_SIZE                             (0x10)

/* static functions */
static GT_U32 snetTwistDPclGetAddress
(
      IN GT_U32 rule,
      IN GT_U32 byte
);

static GT_U32 snetTwistPclSearch
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    IN GT_U8  searchKey[SNET_TWIST_PCL_KEY_SIZE_CNS]
);

static SNET_POLICY_ACTION_SRC_ENT snetTwistPolicyActionGet
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    IN GT_U32                           pceActionIdx,
    INOUT SNET_POLICY_ACTION_STC  *     policyActionPtr
);

static GT_VOID snetTwistPcelActionGet
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    IN GT_U32                           pceActionIdx,
    INOUT SNET_POLICY_ACTION_STC  *     policyActionPtr
);

static GT_BOOL snetTwistPolicyActionApply
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    IN SNET_POLICY_ACTION_STC     *     policyActionPtr,
    IN SNET_POLICY_ACTION_SRC_ENT       actionSrc
);

static GT_VOID snetTwistPolicyReMark
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    IN SNET_POLICY_ACTION_STC     *     policyActionPtr
);

/*******************************************************************************
*   snetTwistPclRxMirror
*
* DESCRIPTION:
*       Make RX mirroring
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*
*******************************************************************************/
GT_VOID snetTwistPclRxMirror
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistPclRxMirror);

    GT_U32 regAddress, fldValue; /* Register's address and data */
    GT_U32 targetDevice, targetPort; /*  Target device and port */

    /* Bridge Port<n> Control Register */
    BRDG_PORT0_CTRL_REG(descrPtr->srcPort, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 24, 1, &fldValue);
    /* If sniffer bit disable */
    if (fldValue != 1)
        return;

    smemRegFldGet(devObjPtr, INGR_MIRR_ANALYZER_REG, 4, 7, &fldValue);
    /* Indicates the target device number for Rx sniffed packets. */
    __LOG(("Indicates the target device number for Rx sniffed packets."));
    targetDevice = fldValue;

    smemRegFldGet(devObjPtr, INGR_MIRR_ANALYZER_REG, 11, 6, &fldValue);
    /* Indicates the target port number for Rx sniffed packets. */
    targetPort = fldValue;

    smemRegFldGet(devObjPtr, INGR_MIRR_ANALYZER_REG, 1, 3, &fldValue);
    /* Indicates the Traffic Class of mirrored packets when they are sent
    to the CPU. */
    descrPtr->trafficClass = (GT_U8)fldValue;
    /* Transmit packet to device */
    /* CPU code should be specifically defined to TX ??? */
    snetTwistTx2Device(devObjPtr, descrPtr, descrPtr->frameBuf->actualDataPtr,
                       descrPtr->byteCount,  targetPort, targetDevice,
                       TWIST_CPU_CODE_RX_SNIFF);
}
/*******************************************************************************
* gtCoreTwistDPclGetAddress
*
* DESCRIPTION:
*       Calculates the real address of specific byte in a specific rule.
*
* INPUTS:
*       rule - the rule. assume 0 <= rule
*       byte - the byte number. assume 0 <= byte <= 17
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       The hardware address.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_U32 snetTwistDPclGetAddress
(
    IN GT_U32 rule,
    IN GT_U32 byte
)
{
    GT_U32  address = 0;    /* the real address */
    GT_BOOL highOrLow;      /* high ram or low ram */

    /* bits[31:22] - the LX registers address space  = 11*/
    address =  11 << 22;

    /* bits[21:17] 01010 = 10 */
    address |= (10 << 17);

    /* bits[16] reserved */

    /* bits[15] high or low ram - in one row rules 0-3 low - bit = 0
                                             rules 4-7 high - bit = 1 */
    if((rule % TWISTD_RULE_PER_ROW_NUMBER) <= 3)
    {
        /* low */
        highOrLow = GT_FALSE;
    }
    else
    {
        /* high */
        highOrLow = GT_TRUE;
    }


    address |= (highOrLow << 15);

    /* bits[14:11] selects the RAM - byte 0-1 - RAM 0, byte 2-3 - RAM 1 ...
        (until RAM 8) */
    address |= ((byte >> 1) << 11);

    /* bits[10:4] the row in the ram */
    address |= ((rule / TWISTD_RULE_PER_ROW_NUMBER) << 4);

    /* bits[2:3] the 4 byte in one ram of even bytes: rule 0-1 4-5 - 0
                                                      rule 2-3 6-7- 1
                                          odd bytes : rule 0-1 4-5 - 2
                                                      rule 2-3 6-7- 3*/
    if((rule % (TWISTD_RULE_PER_ROW_NUMBER / 2)) <= 1)
    {
        /* rules 0,1,4,5 */

        if((byte % 2) == 0)
        {
            /* even byte */
            address |= (0 << 2);
        }
        else
        {
            /* odd byte */
            address |= (2 << 2);
        }

    }
    else
    {
        /* rules 2,3,6,7 */

        if((byte % 2) == 0)
        {
            /* even byte */
            address |= (1 << 2);
        }
        else
        {
            /* odd byte */
            address |= (3 << 2);
        }
    }

    /* bits[0:1] always 00 */

    return address;
}
/*******************************************************************************
*  snetTwistPolicyProcess
*
* DESCRIPTION:
*      Policy (PCL and IP-Classifier) processing for frame
* INPUTS:
*      deviceObjPtr     - pointer to device object.
*      descrPtr         - pointer to the frame's descriptor.
*
* OUTPUTS:
*
* RETURNS:
*      TRUE - continue frame processing, FALSE - stop frame processing
* COMMENTS:
*
*******************************************************************************/
GT_BOOL snetTwistPolicyProcess
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt

)
{
    DECLARE_FUNC_NAME(snetTwistPolicyProcess);

    GT_U8  searchKey[SNET_TWIST_PCL_KEY_SIZE_CNS]; /* PCL search key */
    GT_U32 pceActionIdx; /* PCE action index in the PCL table */
    SNET_POLICY_ACTION_STC policyAction; /* policy action entry */
    SNET_POLICY_ACTION_SRC_ENT actionSrc; /* action to apply */

    if (descrPrt->doClassify == 0 ||
        descrPrt->pktCmd == SKERNEL_PKT_TRAP_CPU_E ||
        descrPrt->pktCmd == SKERNEL_PKT_DROP_E)
    {
        return GT_TRUE;
    }
    /* Build PCL search key */
    __LOG(("Build PCL search key"));
    snetTwistPclKeyCreate(devObjPtr, descrPrt, searchKey);

    /* PCL lookup */
    __LOG(("PCL lookup"));
    pceActionIdx = snetTwistPclSearch(devObjPtr, descrPrt, searchKey);

    /* Get final action */
    __LOG(("Get final action"));
    actionSrc = snetTwistPolicyActionGet(devObjPtr, descrPrt, pceActionIdx,
                                         &policyAction);
    /* Process action */
    __LOG(("Process action"));
    return snetTwistPolicyActionApply(devObjPtr, descrPrt, &policyAction,
                                      actionSrc);
}

/*******************************************************************************
*  snetTwistPclKeyCreate
*
* DESCRIPTION:
*      Build search key from frame and templates
* INPUTS:
*      deviceObjPtr     - pointer to device object.
*      descrPtr         - pointer to the frame's descriptor.
*
* OUTPUTS:
*      searchKey[]    - PCL search key
*
* RETURNS:
*
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetTwistPclKeyCreate
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    OUT GT_U8  searchKey[SNET_TWIST_PCL_KEY_SIZE_CNS]
)
{
    SNET_TWIST_TEMPLATE_STC frameTemplate; /* flow template configuration */
    GT_U32 frameOffsetsNum, offset; /* Number of offset used in PCL search key*/
    GT_U32 l4HeaderSize;  /* Size of TCP/UDP header */

    memset(searchKey, 0, SNET_TWIST_PCL_KEY_SIZE_CNS);

    snetTwistGetFlowTemplate(devObjPtr, descrPrt, &frameTemplate);

    if (frameTemplate.includeLif == 1 ||
        frameTemplate.includeVid == 1 )
    {
        frameOffsetsNum = 14;
    }
    else
    {
        frameOffsetsNum = 16;
    }

    for(offset = 0; offset < frameOffsetsNum; offset++)
    {
        if (frameTemplate.offsetValid[offset])
        {
            if (frameTemplate.offsetType[offset] == SKERNEL_PCL_ABS_E ||
                frameTemplate.offsetType[offset] == SKERNEL_PCL_MAC_HEADER_E ||
                frameTemplate.offsetType[offset] == SKERNEL_PCL_MPLS_HEADER_E)
            {
                searchKey[offset] = *(descrPrt->dstMacPtr +
                    frameTemplate.offsetValue[offset]);
            }
            else
            if (frameTemplate.offsetType[offset] == SKERNEL_PCL_L3_HEADER_E)
            {
                if (descrPrt->ipHeaderPtr == 0)
                {
                    continue;
                }
                searchKey[offset] = *(descrPrt->ipHeaderPtr +
                    frameTemplate.offsetValue[offset]);
            }
            else
            if (frameTemplate.offsetType[offset] == SKERNEL_PCL_L4_HEADER_E)
            {
                if (descrPrt->ipHeaderPtr == 0)
                {
                    continue;
                }
                searchKey[offset] = *(descrPrt->l4HeaderPtr +
                    frameTemplate.offsetValue[offset]);
            }
            if (frameTemplate.offsetType[offset] == SKERNEL_PCL_L5_HEADER_E)
            {
                if (descrPrt->flowTemplate == SKERNEL_TCP_FLOW_E)
                {
                    l4HeaderSize = TCP_HEADER_SIZE;
                }
                else
                if (descrPrt->flowTemplate == SKERNEL_UDP_FLOW_E)
                {
                    l4HeaderSize = UDP_HEADER_SIZE;
                }
                else
                {
                    continue;
                }
                searchKey[offset] = *(descrPrt->l4HeaderPtr + l4HeaderSize +
                    frameTemplate.offsetValue[offset]);
            }
        }
    }
    if (frameTemplate.includeLif == 1)
    {
        searchKey[14] = descrPrt->inLifNumber >> 8;
        searchKey[15] = descrPrt->inLifNumber & 0xff;
    }
    else
    if (frameTemplate.includeVid == 1)
    {
        searchKey[14] = descrPrt->vid >> 8;
        searchKey[15] = descrPrt->vid & 0xff;
    }
    searchKey[16] = 1;
    if (descrPrt->doRout || (descrPrt->mplsCmd != 0))
    {
        searchKey[16] |= 0x2;
    }
    searchKey[16] |= descrPrt->macDaType << 2;
    searchKey[16] |= descrPrt->macDaLookupResult << 3;

    searchKey[17] = 1 << descrPrt->flowTemplate;
}

/*******************************************************************************
*  snetTwistPclSearch
*
* DESCRIPTION:
*      Make look up of search key in the PCL table
* INPUTS:
*      deviceObjPtr     - pointer to device object.
*      descrPtr         - pointer to the frame's descriptor.
*      searchKey[]    - PCL search key
* OUTPUTS:
*
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static GT_U32 snetTwistPclSearch
(
    IN SKERNEL_DEVICE_OBJECT      *     deviceObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    IN GT_U8  searchKey[SNET_TWIST_PCL_KEY_SIZE_CNS]
)
{
    GT_U32 firstPceIdx, lastPceIdx, pceIdx; /* Searcing index for PCL table */
    GT_U32 offset; /* PCL search key offset */
    GT_U32 regAddr, * regDataPtr, regData; /* Register's address and data */
    GT_U8  byte, mask; /* Byte pattern and mask for the rule */
    GT_U32 ruleMatch; /* 1 means that PCE matches with key */
    GT_U32 validBitMask ;

    firstPceIdx = descrPrt->pclBaseAddr * 8;
    lastPceIdx = firstPceIdx + ((descrPrt->pclMaxHops + 1) * 8);

    for (pceIdx = firstPceIdx; pceIdx < lastPceIdx; pceIdx++)
    {
        /* check tha the PCE is VALID */
        regAddr =  snetTwistDPclGetAddress(pceIdx, 0x10);
        regDataPtr = (GT_U32 *)smemMemGet(deviceObjPtr, regAddr);
        regData = *regDataPtr;
        validBitMask = ((pceIdx & 0x1) != 0) ? 0x00010000 : 1 ;
        if ((regData & validBitMask) == 0)
            continue ;

        ruleMatch = 1;
        for(offset = 0; offset < SNET_TWIST_PCL_KEY_SIZE_CNS; offset++)
        {
            regAddr =  snetTwistDPclGetAddress(pceIdx, offset);
            regDataPtr = (GT_U32 *)smemMemGet(deviceObjPtr, regAddr);
            regData = *regDataPtr;
            if((pceIdx & 0x1) == 0)
            {
                /* even rules - byte 0,1 */
                byte = (GT_U8)(regData);
                mask = (GT_U8)(regData >> 8);
            }
            else
            {
                /* odd rules - set byte 2,3 */
                byte = (GT_U8)(regData >> 16);
                mask = (GT_U8)(regData >> 24);
            }
            if ((offset + 1) < SNET_TWIST_PCL_KEY_SIZE_CNS)
            {
                /* bytes 1-17 */
                if (byte != (searchKey[offset] | mask))
                {
                    /* PCE not matches with a key*/
                    ruleMatch = 0;
                    break;
                }
            }
            else
            {
                /* byte18 - special match algorithm */
                if ((byte & searchKey[offset] & (~ mask)) == 0)
                {
                    /* PCE not matches with a key*/
                    ruleMatch = 0;
                    break;
                }
            }
        }

        /* check that all bytes are matched */
        if (ruleMatch == 1)
        {
            return pceIdx;
        }
    }

    return SNET_INVALID_PCE_IDX;
}

/*******************************************************************************
*  snetTwistGetFlowTemplate
*
* DESCRIPTION:
*      GET flow template
* INPUTS:
*      deviceObjPtr     - pointer to device object.
*      descrPtr         - pointer to the frame's descriptor.
*      frameTemplate  - pointer to flow template to be filled
* OUTPUTS:
*
*      frameTemplate  - pointer to flow template
*      searchKey[]    - PCL search key
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetTwistGetFlowTemplate
(
    IN SKERNEL_DEVICE_OBJECT      *     deviceObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    INOUT SNET_TWIST_TEMPLATE_STC *     frameTemplate
)
{
    GT_U8   majorTemplate; /* Major bit of the Generic Flow Template */
    GT_U8   flowTemplate; /* Minor Flow Template */
    GT_U8   byteData; /* 8 bits for the current profile */
    GT_U8   wordOffset; /* Flow template byte offset */
    GT_U8   bitOffset;  /* Flow template bit offset */
    GT_U32  regAddr, *regDataPtr; /* Register address and data pointre */
    GT_U32  templateIndex; /* Flow template index */
    GT_U32  selEntry; /* Selected flow data */
    GT_U32  j, offset; /* PCL search key offset  */
    GT_U32  tableCfg[20]; /* Flow template select table */
    GT_U32  tempAddress;/*address to get the info on masking of the bytes of
                        the flow*/
    GT_U32  fieldValue;

    flowTemplate = descrPrt->flowTemplate;
    majorTemplate = descrPrt->majorTemplate;
    /* calculates flow template index - bit  3 the profile id
                                        bits 0-2 the packet type */
    templateIndex = (majorTemplate << 3) | flowTemplate;

    regAddr = FLOW_TEMPLATE_CONF_REG + ((templateIndex / 4) * 4);
    /* Read config register */
    regDataPtr = smemMemGet(deviceObjPtr, regAddr);
    byteData = (GT_U8)((*regDataPtr) >> ((templateIndex % 4) * 8));

    frameTemplate->keySize = byteData & 0x1f;
    frameTemplate->includeVid = (byteData >> 7) & 0x01;
    frameTemplate->includeLif = (byteData >> 5) & 0x01;
    frameTemplate->incFlowTemp = (byteData >> 6) & 0x01;

    /* Prepare bit patterns to read - clear all bytes */
    memset(tableCfg, 0, sizeof(tableCfg));

    /* each template has its own 12 registers (48 bytes)*/
    regAddr = FLOW_TEMPLATE_SEL_TBL_REG +
              SELECT_PCL_TABLE_ENTRY_SIZE_BYTES*templateIndex;

    regDataPtr = smemMemGet(deviceObjPtr, regAddr);

    /* get the ft select table */
    for( j = 0; j < (SELECT_PCL_TABLE_ENTRY_SIZE_BYTES / 4); j++)
    {
        tableCfg[j] = *regDataPtr;
        /* jump to next register */
        regDataPtr ++;
    }
    for (offset = 0; offset < SNET_MAX_NUM_BYTES_FOR_KEY_CNS; offset++)
    {
        /* each offset takes 13 bits */
        wordOffset = (GT_U8)(((offset * 13) + ((offset / 7) * 37)) / 32);
        bitOffset = (GT_U8)(((offset * 13) + ((offset / 7) * 37)) % 32);
        if(bitOffset <= 18)
        {
            selEntry = (tableCfg[wordOffset] >> bitOffset) & 0xfff;
        }
        else
        {
            selEntry = (tableCfg[wordOffset] >> bitOffset);
            selEntry |= tableCfg[wordOffset + 1] << (32 - bitOffset);
            selEntry &= 0xfff;

        }
        frameTemplate->offsetValid[offset] = selEntry & 1;
        frameTemplate->offsetType[offset] = (selEntry >> 1) & 0x07;
        frameTemplate->offsetValue[offset] = (selEntry >> 4);
    }

    /* get the masking of all bytes */

    tempAddress = FLOW_TEMPLATE_HASH_SELECT_TABLE_REG +
        SNET_SAMBA_FLOW_TEMPLATE_HASH_SELECT_ENTRY_SIZE_IN_BYTES*templateIndex;


    /*Apply the byte mask*/
    for (offset=0; offset< SNET_MAX_NUM_BYTES_FOR_KEY_CNS;offset++)
    {
        /* 1.every byte configuration has 22 bits
           2.every 5 byte configuration has new base address that is
              4 words after the previous base address */
        if(offset!=0 && (offset%5)==0)
        {
            tempAddress += 4*sizeof(GT_U32);/* the next 4 words */
        }

        fieldValue = snetUtilGetContinuesValue(deviceObjPtr,tempAddress,22,(offset%5));

        frameTemplate->byteMask[offset] =
            (GT_U8)((fieldValue >> 14 ) & 0xff); /*8bits*/
    }

}

/*******************************************************************************
*  snetTwistPolicyActionGet
*
* DESCRIPTION:
*      Get final action
* INPUTS:
*      deviceObjPtr     - pointer to device object.
*      descrPtr         - pointer to the frame's descriptor.
*      policyActionPtr  - pointer to policy action entry
* OUTPUTS:
*
*      SNET_POLICY_ACTION_SRC_ENT
*
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static SNET_POLICY_ACTION_SRC_ENT snetTwistPolicyActionGet
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    IN GT_U32                           pceActionIdx,
    INOUT SNET_POLICY_ACTION_STC  *     policyActionPtr
)
{
    GT_U32 lookupMode; /* Look up mode */
    GT_U32 fldValue; /* Registers field value */
    GT_U8 isDone = 0; /* COS table look up result */

    smemRegFldGet(devObjPtr, TCB_CONTROL_REG, 1, 2, &fldValue);
    /* LookupMode */
    lookupMode = fldValue;

    /* Take the CC command from the default entry */
    if (lookupMode != 2)
    {
        if (pceActionIdx != SNET_INVALID_PCE_IDX)
        {
            snetTwistPcelActionGet(devObjPtr, descrPrt, pceActionIdx,
                                   policyActionPtr);
            return SNET_POLICY_ACTION_PCE_E;
        }
        else
        {
            if (lookupMode != 0)
            {
                isDone = snetTwistCosTablesLookup(devObjPtr, descrPrt,
                                                   policyActionPtr);
                if (isDone)
                {
                    return SNET_POLICY_ACTION_IP_TABLES_E;
                }
            }
        }
    }

    /* Get action from default action of template */
    snetTwisDefTemplActionGet(devObjPtr, descrPrt, policyActionPtr);

    return SNET_POLICY_ACTION_TEMPLATE_E;
}

/*******************************************************************************
*  snetTwistPcelActionGet
*
* DESCRIPTION:
*      Get final action from PCE’s action
* INPUTS:
*      deviceObjPtr     - pointer to device object.
*      descrPtr         - pointer to the frame's descriptor.
*      policyActionPtr  - pointer to policy action entry
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
static GT_VOID snetTwistPcelActionGet
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    IN GT_U32                           pceActionIdx,
    INOUT SNET_POLICY_ACTION_STC  *     policyActionPtr
)
{
    GT_U32 pceBaseAddr; /* Policy entry address */
    GT_U32 actionsAddr; /* PCL control entry address */
    GT_U32 *hwPceActionPtr; /* PCL control entry */
    GT_U32 fldValue; /* Register field value */

    smemRegFldGet(devObjPtr, PCL_CONTROL_REG, 0, 25, &fldValue);
    /* PceBaseAddr */
    pceBaseAddr = fldValue;
    /* action is in the wide SRAM */
    actionsAddr = 0x28000000 + (pceBaseAddr << 4) + pceActionIdx * 8;

    hwPceActionPtr = smemMemGet(devObjPtr, actionsAddr);
    /* PCL control entry - Word0 */
    policyActionPtr->cmd =
        (GT_U16)SMEM_U32_GET_FIELD(hwPceActionPtr[0], 0, 2);
    policyActionPtr->analyzMirror =
        SMEM_U32_GET_FIELD(hwPceActionPtr[0], 2, 1);
    policyActionPtr->cpuMirror =
        SMEM_U32_GET_FIELD(hwPceActionPtr[0], 3, 1);
    policyActionPtr->markCmd =
        (GT_U16)SMEM_U32_GET_FIELD(hwPceActionPtr[0], 5, 2);
    policyActionPtr->dscp =
        SMEM_U32_GET_FIELD(hwPceActionPtr[0], 7, 6);
    policyActionPtr->tcCosParam =
        SMEM_U32_GET_FIELD(hwPceActionPtr[0], 13, 16);
    policyActionPtr->cmdRedirect =
        (GT_U16)SMEM_U32_GET_FIELD(hwPceActionPtr[0], 29, 2);
    policyActionPtr->outLif =
        SMEM_U32_GET_FIELD(hwPceActionPtr[0], 31, 1);

    /* PCL control entry - Word1 */
    policyActionPtr->outLif |=
        SMEM_U32_GET_FIELD(hwPceActionPtr[1], 0, 19) << 1;
    policyActionPtr->outLif |=
        SMEM_U32_GET_FIELD(hwPceActionPtr[1], 19, 10) << 20;
    policyActionPtr->setLbh =
        (GT_U16)SMEM_U32_GET_FIELD(hwPceActionPtr[1], 29, 1);
    policyActionPtr->lbh =
        (GT_U16)SMEM_U32_GET_FIELD(hwPceActionPtr[1], 30, 2);
}

/*******************************************************************************
*  snetTwisDefTemplActionGet
*
* DESCRIPTION:
*      Get final action from PCE’s action
* INPUTS:
*      deviceObjPtr     - pointer to device object.
*      descrPtr         - pointer to the frame's descriptor.
*      policyActionPtr  - pointer to policy action entry
* OUTPUTS:
*
*      policyActionPtr  - pointer to policy action entry
*
*
* RETURNS:
*
* COMMENTS:
*  function depend on family type
*******************************************************************************/
GT_VOID snetTwisDefTemplActionGet
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    INOUT SNET_POLICY_ACTION_STC  *     policyActionPtr
)
{
    GT_U32 regAddr, regData, * regDataPtr; /* Register address and value */
    GT_U32  specificFamilyOffset;
    GT_U32  specificBaseAddress;

    memset(policyActionPtr,0,sizeof(SNET_POLICY_ACTION_STC));

    if(devObjPtr->deviceFamily == SKERNEL_TWIST_D_FAMILY ||
       devObjPtr->deviceFamily == SKERNEL_TIGER_FAMILY)
    {
        specificFamilyOffset = 1;
        specificBaseAddress = TWISTD_CMD_KEY_ENTRY_DEFAULT_TBL; /*0x02C80100*/
    }
    else
    {
        /* twist c and samba */
        specificFamilyOffset = 0;
        specificBaseAddress = SAMBA_CMD_KEY_ENTRY_DEFAULT_TBL;  /*0x02C80080*/
    }

    regAddr = specificBaseAddress +
        (((descrPrt->majorTemplate << 3) | descrPrt->flowTemplate) * 2) * 0x04;
    /* Word 0 */
    regDataPtr = (GT_U32 *)smemMemGet(devObjPtr, regAddr);
    regData = *regDataPtr;
    /* Cmd */
    policyActionPtr->cmd =
        SMEM_U32_GET_FIELD(regData, 0, 2);

    policyActionPtr->cpuMirror =
        SMEM_U32_GET_FIELD(regData, 3, 1);

    policyActionPtr->markCmd =
        (GT_U16)SMEM_U32_GET_FIELD(regData, 4+specificFamilyOffset, 2);
    policyActionPtr->dscp =
        SMEM_U32_GET_FIELD(regData, 6+specificFamilyOffset, 6);
    policyActionPtr->tcCosParam =
        SMEM_U32_GET_FIELD(regData, 12+specificFamilyOffset, 16);

    policyActionPtr->cmdRedirect =
        (GT_U16)SMEM_U32_GET_FIELD(regData, 28+specificFamilyOffset, 2);

    if(devObjPtr->deviceFamily == SKERNEL_TWIST_D_FAMILY ||
       devObjPtr->deviceFamily == SKERNEL_TIGER_FAMILY)
    {
        policyActionPtr->analyzMirror =
            SMEM_U32_GET_FIELD(regData, 2, 1);

        policyActionPtr->outLif =
            SMEM_U32_GET_FIELD(regData, 31, 1);

        /* Word 1 */
        regDataPtr ++ ;

        regData = *regDataPtr;
        policyActionPtr->outLif |=
            SMEM_U32_GET_FIELD(regData, 0, 29) << 1;
        policyActionPtr->setLbh =
            (GT_U16)SMEM_U32_GET_FIELD(regData, 29, 1);
        policyActionPtr->lbh =
            (GT_U16)SMEM_U32_GET_FIELD(regData, 30, 2);
    }
    else
    {
        /* the second word is very differ then on twistD */

        /* Word 1 */
        regDataPtr ++ ;
        regData = *regDataPtr;

        /* only valid when
           policyActionPtr->cmdRedirect == SKERNEL_PCL_IPV4_FRWD_E */
        policyActionPtr->outLif =
            (GT_U16)SMEM_U32_GET_FIELD(regData, 0, 20);
    }
}

/*******************************************************************************
*  snetTwistCosTablesLookup
*
* DESCRIPTION:
*      Make search in the IP Classifier and get corespondent entry
* INPUTS:
*      deviceObjPtr     - pointer to device object.
*      descrPtr         - pointer to the frame's descriptor.
*      policyActionPtr  - pointer to policy action entry
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
GT_BOOL snetTwistCosTablesLookup
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    INOUT SNET_POLICY_ACTION_STC  *     policyActionPtr
)
{
    GT_U32 regAddr, regData, * regDataPtr; /* Register address and value */
    GT_U16 destPort; /* Destination port from TCP/UDP header */
    GT_U8  protocol; /* Protocol from IP header */
    GT_U16 loopIndex, index = 0, protType; /* Indexes */
    GT_U32  specificFamilyOffset;
    GT_U32  specialCosCommandRegOffset;
    GT_U32  maxLoop;

    memset(policyActionPtr,0,sizeof(SNET_POLICY_ACTION_STC));

    if (descrPrt->ipHeaderPtr == 0)
    {
        return GT_FALSE;
    }

    regDataPtr = (GT_U32 *)smemMemGet(devObjPtr,COS_LOOK_UP_TBL);

    if (descrPrt->flowTemplate == SKERNEL_TCP_FLOW_E ||
        descrPrt->flowTemplate == SKERNEL_UDP_FLOW_E )
    {
        if(descrPrt->flowTemplate == SKERNEL_UDP_FLOW_E)
        {
            protType = 1;
            regDataPtr+=4;/* jump over 4 register */
        }
        else
        {
            protType = 0;
        }

        maxLoop = 4;

        /* Destination UDP/TCP Port */
        destPort =
            SGT_LIB_SWAP_BYTES_MAC((GT_U16)(*(GT_U16*)(descrPrt->l4HeaderPtr + 2)));

        /* Make lookup for TCP/UDP entries in the Class of Service Lookup Table */
        for (loopIndex = 0; loopIndex < maxLoop; loopIndex++,regDataPtr++)
        {
            regData = *regDataPtr;
            /* Port <portNum> */
            if (SMEM_U32_GET_FIELD(regData, 0, 16) == destPort)
            {
                index = 2 * loopIndex;
                break;
            }
            /* Port <portNum + 1> */
            else if (SMEM_U32_GET_FIELD(regData, 16, 16) == destPort)
            {
                index = 2 * loopIndex + 1;
                break;
            }
        }
    }
    else
    {
        /* Protocol 8 bit */
        protocol = descrPrt->ipHeaderPtr[9];
        protType = 2;
        regDataPtr+=8;/* jump over 8 register */
        maxLoop = 2;

        for (loopIndex = 0; loopIndex < maxLoop; loopIndex++,regDataPtr++)
        {
            regData = *regDataPtr;
            if (SMEM_U32_GET_FIELD(regData, 0, 8) == protocol)
            {
                index = 4 * loopIndex;
                break;
            }
            else if (SMEM_U32_GET_FIELD(regData, 8, 8) == protocol)
            {
                index = 4 * loopIndex + 1;
                break;
            }
            else if (SMEM_U32_GET_FIELD(regData, 16, 8) == protocol)
            {
                index = 4 * loopIndex + 2;
                break;
            }
            else if (SMEM_U32_GET_FIELD(regData, 24, 8) == protocol)
            {
                index = 4 * loopIndex + 3;
                break;
            }
        }
    }

    if (loopIndex == maxLoop)
    {
        /* not found match */
        return GT_FALSE;
    }

    if(devObjPtr->deviceFamily == SKERNEL_TWIST_D_FAMILY ||
       devObjPtr->deviceFamily == SKERNEL_TIGER_FAMILY)
    {
        specificFamilyOffset = 1;
        specialCosCommandRegOffset = 3;
    }
    else
    {
        /* twist c and samba */
        specificFamilyOffset = 0;
        specialCosCommandRegOffset = 2;
    }

    regAddr = COS_COMMAND_TBL + (protType << (3+specialCosCommandRegOffset)) +
              (index << specialCosCommandRegOffset);

    regDataPtr = (GT_U32 *)smemMemGet(devObjPtr, regAddr);
    regData = *regDataPtr;

    /* Cmd */
    policyActionPtr->cmd =
        SMEM_U32_GET_FIELD(regData, 0, 2);

    /* MarkCmd */
    policyActionPtr->markCmd =
        SMEM_U32_GET_FIELD(regData, 4+specificFamilyOffset, 2);

    /* DSCP/EXP */
    policyActionPtr->dscp =
        SMEM_U32_GET_FIELD(regData, 6+specificFamilyOffset, 6);

    /* TCPtr/CoS params */
    policyActionPtr->tcCosParam =
        SMEM_U32_GET_FIELD(regData, 12+specificFamilyOffset, 16);


    if(devObjPtr->deviceFamily == SKERNEL_TWIST_D_FAMILY ||
       devObjPtr->deviceFamily == SKERNEL_TIGER_FAMILY)
    {
        policyActionPtr->analyzMirror =
            SMEM_U32_GET_FIELD(regData, 2, 1);

        policyActionPtr->cpuMirror =
            SMEM_U32_GET_FIELD(regData, 3, 1);

        policyActionPtr->cmdRedirect =
            (GT_U16)SMEM_U32_GET_FIELD(regData, 29, 2);

        policyActionPtr->outLif =
            SMEM_U32_GET_FIELD(regData, 31, 1);

        /* Word 1 */
        regDataPtr ++ ;

        regData = *regDataPtr;
        policyActionPtr->outLif |=
            SMEM_U32_GET_FIELD(regData, 0, 29) << 1;
        policyActionPtr->setLbh =
            (GT_U16)SMEM_U32_GET_FIELD(regData, 29, 1);
        policyActionPtr->lbh =
            (GT_U16)SMEM_U32_GET_FIELD(regData, 30, 2);

    }
    else
    {
        /* the samba/twistC there is no extra info !!!*/
        policyActionPtr->cpuMirror = /* this is CPU mirror not Analyze mirror !!!*/
            SMEM_U32_GET_FIELD(regData, 3, 1);
    }

    return GT_TRUE;
}

/*******************************************************************************
*  snetTwistPolicyActionApply
*
* DESCRIPTION:
*      Apply actions
* INPUTS:
*      deviceObjPtr     - pointer to device object.
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
static GT_BOOL snetTwistPolicyActionApply
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    IN SNET_POLICY_ACTION_STC     *     policyActionPtr,
    IN SNET_POLICY_ACTION_SRC_ENT       actionSrc
)
{
    if (policyActionPtr->cmd == SKERNEL_PCL_DROP_E)
    {
        descrPrt->pktCmd = SKERNEL_PKT_DROP_E;

        return GT_TRUE;
    }
    else
    if (policyActionPtr->cmd == SKERNEL_PCL_TRAP_E)
    {
        if (actionSrc == SNET_POLICY_ACTION_PCE_E)
        {
            descrPrt->bits15_2.cmd_trap2cpu.cpuCode =
                TWIST_CPU_TCB_KEY_ENTRY;
        }
        else
        if (actionSrc == SNET_POLICY_ACTION_TEMPLATE_E)
        {
            descrPrt->bits15_2.cmd_trap2cpu.cpuCode =
                TWIST_CPU_TCB_TRAP_DEFAULT;
        }
        if (actionSrc == SNET_POLICY_ACTION_IP_TABLES_E)
        {
            descrPrt->bits15_2.cmd_trap2cpu.cpuCode =
                TWIST_CPU_TCB_TRAP_COS;
        }
        descrPrt->pktCmd = SKERNEL_PKT_TRAP_CPU_E;

        return GT_TRUE;
    }
    else
    {
        if (policyActionPtr->analyzMirror == 1)
        {
            snetTwistPclRxMirror(devObjPtr, descrPrt);
        }
        if (policyActionPtr->cpuMirror == 1)
        {
            snetTwistTx2Cpu(devObjPtr, descrPrt, TWIST_CPU_TCB_CPU_MIRROR_FIN);
        }
        if (policyActionPtr->cmdRedirect == SKERNEL_PCL_IPV4_FRWD_E)
        {
            descrPrt->policySwitchType = POLICY_ROUTE_E;
            descrPrt->nextLookupPtr = policyActionPtr->outLif;
        }
        else
        if (policyActionPtr->cmdRedirect == SKERNEL_PCL_OUTLIF_FRWD_E)
        {
            descrPrt->policySwitchType = POLICY_SWITCH_E;
            descrPrt->LLL_outLif = policyActionPtr->outLif;
        }

    }
    snetTwistPolicyMark(devObjPtr, descrPrt, policyActionPtr);

    snetTwistPolicyReMark(devObjPtr, descrPrt, policyActionPtr);

    return GT_TRUE;
}

/*******************************************************************************
*  snetTwistPolicyMark
*
* DESCRIPTION:
*      Mark CoS atributes of frame
* INPUTS:
*      deviceObjPtr     - pointer to device object.
*      descrPtr         - pointer to the frame's descriptor.
*      policyActionPtr  - pointer to policy action entry
* OUTPUTS:
*
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetTwistPolicyMark
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    IN SNET_POLICY_ACTION_STC     *     policyActionPtr
)
{
    GT_U32 dscp; /* Use for remarking the IP */
    GT_U32 regAddr, fldValue, tcbRegVal; /* Register's address and data */

    smemRegFldGet(devObjPtr, TCB_CONTROL_REG, 8, 1,&tcbRegVal);

    if (tcbRegVal)
    {
        descrPrt->modifyDscpOrExp = GT_TRUE;
    }

    if (policyActionPtr->markCmd == SKERNEL_PCL_L2_COS_E)
    {
        return;
    }

    if (policyActionPtr->markCmd == SKERNEL_PCL_COS_ENTRY_E)
    {
        if (policyActionPtr->cmd != SKERNEL_PCL_PASS_SEND_TC_E)
        {
            descrPrt->dscp = (GT_U8)policyActionPtr->dscp;

            descrPrt->dropPrecedence =
                (GT_U8)SMEM_U32_GET_FIELD(policyActionPtr->tcCosParam, 0, 2);
            descrPrt->trafficClass =
                (GT_U8)SMEM_U32_GET_FIELD(policyActionPtr->tcCosParam, 2, 3);
            descrPrt->userPriorityTag =
                (GT_U8)SMEM_U32_GET_FIELD(policyActionPtr->tcCosParam, 5, 3);
        }
        return;
    }
    if (policyActionPtr->markCmd == SKERNEL_PCL_DSCP_ENTRY_E)
    {
        dscp = policyActionPtr->dscp;
    }
    else
    {
        dscp = descrPrt->dscp;
    }

    regAddr = COS_MARK_REMARK_REG + (dscp << 2);

    smemRegFldGet(devObjPtr, regAddr, 6, 2, &fldValue);
    /* Drop precedence. */
    descrPrt->dropPrecedence = (GT_U8)fldValue;

    smemRegFldGet(devObjPtr, regAddr, 8, 3, &fldValue);
    /* TrafficClass */
    descrPrt->trafficClass = (GT_U8)fldValue;

    smemRegFldGet(devObjPtr, regAddr, 11, 3, &fldValue);
    /* VPT */
    descrPrt->userPriorityTag = (GT_U8)fldValue;

    descrPrt->dscp = (GT_U8)dscp;
}
/*******************************************************************************
*  snetTwistPolicyReMark
*
* DESCRIPTION:
*      Re-Mark CoS atributes of frame
* INPUTS:
*      deviceObjPtr     - pointer to device object.
*      descrPtr         - pointer to the frame's descriptor.
*      policyActionPtr  - pointer to policy action entry
* OUTPUTS:
*
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID snetTwistPolicyReMark
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    IN SNET_POLICY_ACTION_STC     *     policyActionPtr
)
{
    return;
}


