/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetTwistIpV4.c
*
* DESCRIPTION:
*      This is a external API definition for IpV4 Interface of SKernel.
*
* DEPENDENCIES:
*      None.
*
* FILE REVISION NUMBER:
*      $Revision: 23 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/suserframes/snetTwistIpV4.h>
#include <asicSimulation/SKernel/suserframes/snetTwistL2.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/twistCommon/sregTwist.h>
#include <asicSimulation/SKernel/smem/smemTwist.h>
#include <asicSimulation/SLog/simLog.h>

static void snetTwistIpv4InFilter
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

static void snetTwistIpv4Stage2
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

static void snetTwistIpv4UcastStage2
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

static void snetTwistIpv4DefaultIpm
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

static void snetTwistIpv4Lpm
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_U32 snetTwistIpv4Lpm4IpmInit
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_U32 snetTwistIpv4NextPointGet
(
    IN GT_U32 bitVector,
    IN GT_U32 bitVectorAddr,
    IN IPV4_LPM_BUCKET_TYPE_ENT bucketType,
    IN GT_U8 ipOctet,
    IN GT_U32 bvIdx
);

static GT_U32 snetTwistLpmNextSearch
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    SNET_LPM_NEXT_POINTER_UNT lpmNextEntry,
    IN    GT_U8 ipOctet,
    IN    IPV4_LPM_BUCKET_TYPE_ENT bucketType
);

static void snetTwistIpv4Egress
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_BOOL snetTwistIpv4Counters
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

static void snetTwistIpMcGroupGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    GT_U32 nextLookupAddr,
    IN    GT_U32 level,
    OUT   IPMC_GROUP_ENTRY_UNT * ipMcEntryPtr
);

static GT_U8 snetTwistLpmNextPtrGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    GT_U32 nextPtrAddr,
    OUT   SNET_LPM_NEXT_POINTER_UNT * lpmNextPtr
);

static GT_VOID snetTwistUcRoutEntryGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    GT_U32 ucEntryAddr,
    OUT   UC_ROUT_ENTRY_STC * ucRoutePtr
);

static GT_VOID snetTwistMcRoutEntryGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    GT_U32 mcEntryAddr,
    OUT   MC_ROUT_ENTRY_STC * mcRoutePtr
);

/*******************************************************************************
*   snetTwistIPv4Process
*
* DESCRIPTION:
*        make IPv4 processing
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        GT_TRUE   - continue frame processing
*        GT_FALSE  - stop frame processing
*******************************************************************************/
GT_BOOL snetTwistIPv4Process
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);


    if ((!descrPtr->doRout && !descrPtr->doIpmRout)   ||
         (descrPtr->pktCmd == SKERNEL_PKT_DROP_E)     ||
         (descrPtr->pktCmd == SKERNEL_PKT_TRAP_CPU_E) ||
         (descrPtr->ipHeaderPtr == NULL) )
    {
        return GT_TRUE;
    }

    snetTwistIpv4InFilter(devObjPtr, descrPtr);
    snetTwistIpv4Stage2(devObjPtr, descrPtr);
    snetTwistIpv4Lpm(devObjPtr, descrPtr);
    snetTwistIpv4Egress(devObjPtr, descrPtr);

    return snetTwistIpv4Counters (devObjPtr, descrPtr);
}

/*******************************************************************************
*   snetTwistIpv4InFilter
*
* DESCRIPTION:
*        Make ingress processing in the Ipv4 routing block
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
static void snetTwistIpv4InFilter
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistIpv4InFilter);

    GT_U32 ipCheckSum;
    GT_U8  ttl;
    GT_U32 pktIpCheckSum;
    GT_U32 isIpOptions;
    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 subNetMask, internDip;
    GT_U32 protocol4Hash;

    DSCP_TO_COS_REG(descrPtr->dscp, &regAddress);
    smemRegFldGet(devObjPtr, regAddress, 0, 3, &fldValue);
    /* Priority of the packet */
    descrPtr->trafficClass = (GT_U8)fldValue;
    smemRegFldGet(devObjPtr, regAddress, 3, 2, &fldValue);
    /* The drop precedence. */
    descrPtr->dropPrecedence = (GT_U8)fldValue;
    smemRegFldGet(devObjPtr, regAddress, 5, 3, &fldValue);
    /* The VLAN Priority Tag field */
    descrPtr->userPriorityTag = (GT_U8)fldValue;

    if (descrPtr->trapMcLocalScope == 1)
    {
        if ((descrPtr->ipv4DIP & 0xFF000000) == (239 << 24))
        {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode = TWIST_CPU_CODE_LOCAL_SCOPE;
            descrPtr->doIpmRout = 0;
            return;
        }
    }
    if (descrPtr->trapReservedDip == 1)
    {
        if ((descrPtr->ipv4DIP & 0xFF000000) == (10 << 24) ||
            (descrPtr->ipv4DIP & 0xFFC00000) == (172 << 24 | 16 << 16) ||
            (descrPtr->ipv4DIP & 0xFFFF0000) == (196 << 24 | 168 << 16) ||
             descrPtr->ipv4DIP == 0)
        {

            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode = TWIST_CPU_CODE_RESRVED_DA;
            descrPtr->doIpmRout = 0;
            return;
        }
    }
    if (descrPtr->trapIntDip == 1)
    {
        smemRegFldGet(devObjPtr, SUB_NET_MASK_SIZE_REG, 0, 5, &fldValue);
        /* DIP0SubnetSize */
        subNetMask = fldValue;

        INTERN_DIP_REG(0, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
        /* Internal DIP address */
        internDip = fldValue;
        if ((descrPtr->ipv4DIP & ~(0xFFFFFFFF >> subNetMask)) == internDip)
        {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode = TWIST_CPU_CODE_INTERNAL_DA;
            descrPtr->doIpmRout = 0;
            return;
        }
        smemRegFldGet(devObjPtr, SUB_NET_MASK_SIZE_REG, 8, 5, &fldValue);
        /* DIP1SubnetSize */
        subNetMask = fldValue;

        INTERN_DIP_REG(1, &regAddress);
        smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
        /* Internal DIP address */
        internDip = fldValue;
        if ((descrPtr->ipv4DIP & ~(0xFFFFFFFF >> subNetMask)) == internDip)
        {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode = TWIST_CPU_CODE_INTERNAL_DA;
            descrPtr->doIpmRout = 0;
            return;
        }
    }
    /* Get packet checksum from IP header */
    __LOG(("Get packet checksum from IP header"));
    pktIpCheckSum = descrPtr->ipHeaderPtr[10] << 8 | descrPtr->ipHeaderPtr[11];
    descrPtr->ipHeaderPtr[10] = 0;
    descrPtr->ipHeaderPtr[11] = 0;
    /* Calculate IP packet checksum */
    __LOG(("Calculate IP packet checksum"));
    ipCheckSum = ipV4CheckSumCalc(descrPtr->ipHeaderPtr,
                                  (GT_U16)
                                  ((descrPtr->ipHeaderPtr[0] & 0xF) * 4));
    /* restore checksum */
    descrPtr->ipHeaderPtr[10] = (GT_U8)(pktIpCheckSum >> 8);
    descrPtr->ipHeaderPtr[11] = (GT_U8)pktIpCheckSum;
    /* Time To Live */
    ttl = descrPtr->ipHeaderPtr[8];
    descrPtr->ttl = ttl;

    isIpOptions = descrPtr->ipHeaderPtr[0] & 0x0F;
    if (ipCheckSum != pktIpCheckSum)
    {
        smemRegFldGet(devObjPtr, IPV4_CONTROL_REG, 1, 1, &fldValue);
        /* DropBad Chksum */
        if (fldValue == 1)
        {
            descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
            descrPtr->doRout = descrPtr->doIpmRout = 0;
            return;
        }
        descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
        descrPtr->bits15_2.cmd_trap2cpu.cpuCode = TWIST_CPU_CODE_BAD_CHECKSUM;
        descrPtr->doRout = descrPtr->doIpmRout = 0;
    }
    else
    if (ttl == 0)
    {
        smemRegFldGet(devObjPtr, IPV4_CONTROL_REG, 0, 1, &fldValue);
        /* TrapTTL */
        __LOG(("TrapTTL"));
        if (fldValue == 0)
        {
           descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
           descrPtr->doRout = descrPtr->doIpmRout = 0;
           return;
        }
        else
        {
           descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
           descrPtr->bits15_2.cmd_trap2cpu.cpuCode = TWIST_CPU_CODE_TRAP_TTL;
           descrPtr->doRout = descrPtr->doIpmRout = 0;
           return;
        }
    }
    if (isIpOptions == 1)
    {
        smemRegFldGet(devObjPtr, IPV4_CONTROL_REG, 2, 1, &fldValue);
        /* TrapOptions */
        if (fldValue != 0)
        {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                                TWIST_CPU_CODE_OPTION_FAILED;
            descrPtr->doRout = descrPtr->doIpmRout = 0;
            return;
        }
    }
    smemRegFldGet(devObjPtr, IPV4_CONTROL_REG, 8, 1, &fldValue);
    /* UseProtocol ForECP */
    if (fldValue >= 1)
    {
        protocol4Hash = 1 << descrPtr->ipHeaderPtr[9];
    }
    else
    {
        protocol4Hash = 0x0000;
    }
    descrPtr->ecmpHash = (GT_U16)
        ((GT_U16)descrPtr->ipv4DIP ^ descrPtr->ipv4DIP >> 16 ^
         (GT_U16)descrPtr->ipv4SIP ^ descrPtr->ipv4SIP >> 16 ^
         protocol4Hash);

    return;
}

/*******************************************************************************
*   snetTwistIpv4Stage2
*
* DESCRIPTION:
*        Make IPM groups processing and provide data for LPM engine
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
static void snetTwistIpv4Stage2
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistIpv4Stage2);

    GT_U32 nextLookupAddr;
    GT_U32 vrBaseAddr;
    IPMC_GROUP_ENTRY_UNT groupEntry;
    GT_U32 dipFld;
    GT_U32 sipFld;
    GT_U32 invalid = 0;
    GT_U32 fldValue;

    if (descrPtr->doIpmRout != 1)
    {
        snetTwistIpv4UcastStage2(devObjPtr, descrPtr);
        return;
    }
    smemRegFldGet(devObjPtr, VIRTUAL_ROUT_TABLE_REG, 0, 24, &fldValue);
    /* BaseAddress */
    __LOG(("BaseAddress"));
    vrBaseAddr = fldValue;
    /* The first level search. Bits [27:20] taken from the DIP */
    dipFld = (descrPtr->ipv4DIP >> 20) & 0xFF;

    nextLookupAddr = 256 + descrPtr->vrId * 512 + dipFld + vrBaseAddr;
    /* Multicast group entry */
    snetTwistIpMcGroupGet(devObjPtr, nextLookupAddr, 1, &groupEntry);
    dipFld = (descrPtr->ipv4DIP >> 16) & 0x0F;
    if (groupEntry.level1_2.nextMcPtr == 0 ||
        dipFld < groupEntry.level1_2.minimumPtr ||
        dipFld > groupEntry.level1_2.maximumPtr)
    {
        invalid = 1;
    }
    if (invalid != 1)
    {
        /* Second level is accessed DIP[19:10]+pointer from the first level */
        dipFld = (descrPtr->ipv4DIP >> 10) & 0x3FF;
        nextLookupAddr = groupEntry.level1_2.nextMcPtr + dipFld -
                        (groupEntry.level1_2.minimumPtr << 6);
        /* Multicast group entry */
        snetTwistIpMcGroupGet(devObjPtr, nextLookupAddr, 2, &groupEntry);
        dipFld = (descrPtr->ipv4DIP >> 6) & 0x0F;
        if (groupEntry.level1_2.nextMcPtr == 0 ||
            dipFld < groupEntry.level1_2.minimumPtr ||
            dipFld > groupEntry.level1_2.maximumPtr)
        {
            invalid = 1;
        }
        if (invalid != 1)
        {
            /* Third level is accessed DIP[9:0]+pointer form the second level.*/
            dipFld = descrPtr->ipv4DIP & 0x3FF;
            nextLookupAddr = groupEntry.level1_2.nextMcPtr + dipFld -
                            (groupEntry.level1_2.minimumPtr << 6);

            /* Multicast group entry */
            snetTwistIpMcGroupGet(devObjPtr, nextLookupAddr, 3, &groupEntry);
            if (groupEntry.level3.regular_bkt.nextMcPtr != 0)
            {
                if (groupEntry.level3.next_hope.mcGroupType ==
                                        IPV4_LPM_NEXT_HOP_BUCKET_E)
                {
                    descrPtr->nextHopPtr =
                        groupEntry.level3.next_hope.nextHopeRoute;
                    descrPtr->nextHopEcpQosSize =
                        (GT_U8)groupEntry.level3.next_hope.ecpQosSize;
                    descrPtr->nextHopRoutMethod =
                        (GT_U8)groupEntry.level3.next_hope.nextHopMethod;
                    descrPtr->isGIpm = 1;
                    descrPtr->doIpv4Lpm = 0;
                    descrPtr->ipm = 1;
                    return;
                }
            }

        }
    }
    if (invalid == 1 || groupEntry.level3.regular_bkt.nextMcPtr == 0)
    {
        /* Terminate search and fetch Default Mc Group Route Entry */
        snetTwistIpv4DefaultIpm(devObjPtr, descrPtr);
        return;
    }

    descrPtr->lpmKey = descrPtr->ipv4SIP;
    descrPtr->rootBucketType = groupEntry.level3.next_hope.mcGroupType;
    descrPtr->isGIpm = 1;
    descrPtr->doIpv4Lpm = 1;
    descrPtr->ipm = 1;
    if (groupEntry.level3.next_hope.mcGroupType == IPV4_LPM_REGULAR_BUCKET_E)
    {
        /* Direct pointer to a Route Entry */
        sipFld = (descrPtr->ipv4SIP >> 24) & 0xFF;
        descrPtr->nextLookupPtr =
            groupEntry.level3.regular_bkt.nextMcPtr + sipFld / 24;
        descrPtr->bvIdx = (GT_U8)sipFld % 24;
    }
    else
    {
        /* Pointer to a root bucket of LPM tree */
        if (groupEntry.level3.next_hope.mcGroupType ==
                            IPV4_LPM_COMPRESS_1_BUCKET_E)
        {
            descrPtr->nextLookupPtr = groupEntry.level3.regular_bkt.nextMcPtr;
            descrPtr->bvIdx = 0;
        }
        else
        {
            sipFld = (descrPtr->ipv4SIP >> 24) & 0xFF;
            if (groupEntry.level3.compress_bkt.lastIdx > sipFld)
            {
                descrPtr->nextLookupPtr =
                    groupEntry.level3.compress_bkt.nextMcPtr;
                descrPtr->bvIdx = 0;
            }
            else
            {
                descrPtr->nextLookupPtr =
                    groupEntry.level3.compress_bkt.nextMcPtr + 1;
                descrPtr->bvIdx = 1;
            }
        }
    }
}

/*******************************************************************************
*   snetTwistIpv4UcastStage2
*
* DESCRIPTION:
*        Provides unicast IPv4 data for LPM engine
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
static void snetTwistIpv4UcastStage2
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 vrBaseAddr;
    GT_U32 dipFld;
    GT_U32 fldValue;

    smemRegFldGet(devObjPtr, VIRTUAL_ROUT_TABLE_REG, 0, 24, &fldValue);
    /* BaseAddress */
    vrBaseAddr = fldValue;
    /* The first level search. Bits [31:24] taken from the DIP */
    dipFld = (descrPtr->ipv4DIP >> 24) & 0xFF;

    descrPtr->nextLookupPtr = descrPtr->vrId * 512 + dipFld + vrBaseAddr;

    descrPtr->lpmKey = descrPtr->ipv4DIP;
    descrPtr->rootBucketType = 0;
    descrPtr->doIpv4Lpm = 1;
    return;
}

/*******************************************************************************
*   snetTwistIpv4DefaultIpm
*
* DESCRIPTION:
*        Purpose: get next Hop data for default IPM routing
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
static void snetTwistIpv4DefaultIpm
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 defaultIpmBaseAddr;
    GT_U32 fldValue;

    smemRegFldGet(devObjPtr, DEFAULT_MC_TABLE_REG, 0, 22, &fldValue);
    /* BaseAddress */
    defaultIpmBaseAddr = fldValue;
    descrPtr->nextHopPtr = defaultIpmBaseAddr + descrPtr->vrId;
    descrPtr->nextHopRoutMethod = IPV4_LPM_REGULAR_ROUT_METHOD_E;
    descrPtr->isDefaultIpm = 1;
    descrPtr->ipm = 1;
    descrPtr->doIpv4Lpm = 0;
}
/*******************************************************************************
*   snetTwistIpv4Lpm
*
* DESCRIPTION:
*        LPM search engine
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
static void snetTwistIpv4Lpm
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistIpv4Lpm);

    GT_U32 nextPointerAddr;
    SNET_LPM_NEXT_POINTER_UNT lpmNextEntry;
    GT_U32 ipOffset = 0;
    GT_U32 searchLevel = 0;
    GT_U8 bucketType;

    if (descrPtr->doIpv4Lpm != 1)
    {
        return;
    }
    if (descrPtr->doIpmRout == 0)
    {
        nextPointerAddr = descrPtr->nextLookupPtr;
    }
    else
    {
        nextPointerAddr = snetTwistIpv4Lpm4IpmInit(devObjPtr, descrPtr);
    }
    /* First LPM Prefix Entry Get */
    __LOG(("First LPM Prefix Entry Get"));
    bucketType =
        snetTwistLpmNextPtrGet(devObjPtr, nextPointerAddr, &lpmNextEntry);

    while (bucketType != IPV4_LPM_NEXT_HOP_BUCKET_E)
    {
        if (++searchLevel < 4)
        {
            switch (searchLevel)
            {
            case 1:
                ipOffset = 0xFF & (descrPtr->lpmKey >> 16);
                break;
            case 2:
                ipOffset = 0xFF & (descrPtr->lpmKey >> 8);
                break;
            case 3:
                ipOffset = 0xFF & (descrPtr->lpmKey);
                break;
            }
            /* Next LPM Prefix Entry Address */
            nextPointerAddr = snetTwistLpmNextSearch(devObjPtr, descrPtr,
                    lpmNextEntry, (GT_U8)ipOffset, bucketType);
            /* Next LPM Prefix Entry Get */
            bucketType = snetTwistLpmNextPtrGet(devObjPtr, nextPointerAddr,
                    &lpmNextEntry);
        }
        else
        {
            descrPtr->lpmResult = IPV4_LPM_RESULT_NOT_FOUND_E;
            break;
        }
    }

    if (bucketType == IPV4_LPM_NEXT_HOP_BUCKET_E)
    {
        descrPtr->nextHopPtr = lpmNextEntry.bucket_3.nextPtr;
        descrPtr->nextHopRoutMethod = lpmNextEntry.bucket_3.nextHopMethod;
        descrPtr->nextHopEcpQosSize = (GT_U8)lpmNextEntry.bucket_3.ecpQosSize;
        descrPtr->lpmResult = IPV4_LPM_RESULT_FOUND_E;
    }

    return;
}

/*******************************************************************************
*   snetTwistIpv4Lpm4IpmInit
*
* DESCRIPTION:
*        init IPM LPM search by get next pointer address for given bit vector
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
* RETURN:
*        GT_U32 - address of next pointer structure of LPM bucket
*
*******************************************************************************/
static GT_U32 snetTwistIpv4Lpm4IpmInit
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 nextBucketAddr;
    GT_U32 bitVectorAddr;
    GT_U32 bitVector;

    bitVectorAddr = descrPtr->nextLookupPtr;
    bitVector = smemTwistNsramRead(devObjPtr, TWIST_NSRAM_E, bitVectorAddr);

    nextBucketAddr = snetTwistIpv4NextPointGet(bitVector, bitVectorAddr,
                                               descrPtr->rootBucketType,
                                               (GT_U8)(descrPtr->lpmKey >> 24),
                                               descrPtr->bvIdx);
    return nextBucketAddr;
}

/*******************************************************************************
*   snetTwistIpv4NextPointGet
*
* DESCRIPTION:
*        Calculate LPM buckets next pointer structure address for bit vector
* INPUTS:
*        bitVector      - vector of bits used
*        bitVectorAddr  - address of bits vector
*        bucketType     - bucket type
*        ipOctet        -
*        bvIdx          -
* OUTPUTS:
*
*
*******************************************************************************/
static GT_U32 snetTwistIpv4NextPointGet
(
    IN    GT_U32 bitVector,
    IN    GT_U32 bitVectorAddr,
    IN    IPV4_LPM_BUCKET_TYPE_ENT bucketType,
    IN    GT_U8 ipOctet,
    IN    GT_U32 bvIdx
)
{
    GT_U32 prevCount = 0;
    GT_U32 counter = 0;
    GT_U32 lineShift = 0;
    GT_U8 currentByteVal = 0;

    if (bucketType == IPV4_LPM_REGULAR_BUCKET_E)
    {
        GT_U16 i;
        prevCount = bitVector >> 24;
        bvIdx = ipOctet%24;
        /* the prevCount already include the line shift */
        lineShift = 0;/*12 - ipOctet / 24;*/

        /* loop on bits include the bvIdx index too */
        for (i = 0; i <= bvIdx; i++)
        {
            counter += (bitVector & 0x01);
            bitVector >>= 1;
        }
    }
    else
    {
        if (bucketType == IPV4_LPM_COMPRESS_1_BUCKET_E)
        {
            prevCount = 1;
            lineShift  = 0;
        }
        else
        if (bucketType == IPV4_LPM_COMPRESS_2_BUCKET_E)
        {
            prevCount = ( bvIdx == 0) ? 2 : 6;
            lineShift = 0;
        }
        for (counter = 0; counter < 4; counter++)
        {
            currentByteVal = (GT_U8)((bitVector >> (8 * counter)) & 0xFF);
            if (currentByteVal > ipOctet ||
                currentByteVal == 0)
            {
                break;
            }
        }
    }
    return (bitVectorAddr + lineShift + prevCount + counter);
}

/*******************************************************************************
*   snetTwistLpmNextSearch
*
* DESCRIPTION:
*        Calc LPM buckets next pointer address from previous next pointer data
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        nextPointer - next pointer
*        ipOctet  -
*        bucketType - bucket type
* OUTPUTS:
*
*
*******************************************************************************/
static GT_U32 snetTwistLpmNextSearch
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    SNET_LPM_NEXT_POINTER_UNT lpmNextEntry,
    IN    GT_U8 ipOctet,
    IN    IPV4_LPM_BUCKET_TYPE_ENT bucketType
)
{
    GT_U32 bitVectorLine = 0;
    GT_U32 bitVectorAddr;
    GT_U32 bitVector;
    GT_U32 nextPointerAddr;

    if (bucketType == IPV4_LPM_COMPRESS_2_BUCKET_E)
    {
        if (ipOctet < lpmNextEntry.bucket_2.lastIdx)
        {
            bitVectorLine = 0;
        }
        else
        {
            bitVectorLine = 1;
        }
        bitVectorAddr = lpmNextEntry.bucket_2.nextPtr + bitVectorLine;
    }
    else
    {
        if (bucketType == IPV4_LPM_REGULAR_BUCKET_E)
        {
            bitVectorLine = ipOctet / 24;
        }
        else
        if (bucketType == IPV4_LPM_COMPRESS_1_BUCKET_E)
        {
            bitVectorLine = 0;
        }
        bitVectorAddr = lpmNextEntry.bucket_0_1.nextPtr + bitVectorLine;
    }

    bitVector = smemTwistNsramRead(devObjPtr, TWIST_NSRAM_E, bitVectorAddr);

    nextPointerAddr = snetTwistIpv4NextPointGet(bitVector, bitVectorAddr,
                                                bucketType, ipOctet,
                                                bitVectorLine);
    return nextPointerAddr;
}
/*******************************************************************************
*   snetTwistIpv4Egress
*
* DESCRIPTION:
*        Get rout entry and form ipV4 decision
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
static void snetTwistIpv4Egress
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistIpv4Egress);

    GT_U32 routEntryAddr;
    GT_U32 tos;
    GT_U32 routEntryIndex = 0;
    GT_U32  * routEntryPtr;
    GT_U8 routEntryCmd, routForceCos;
    GT_U8 age;
    UC_ROUT_ENTRY_STC ucRoutEntry;
    MC_ROUT_ENTRY_STC mcRoutEntry;
    GT_U32 useInlifForRPF;
    GT_U32 fldValue;
    GT_U8 rpfCheckFail = 0;

    if (descrPtr->pktCmd == SKERNEL_PKT_DROP_E    ||
        descrPtr->pktCmd == SKERNEL_PKT_TRAP_CPU_E)
    {
        descrPtr->ipv4Done = 1;
        return;
    }

    if (descrPtr->nextHopRoutMethod == IPV4_LPM_ECMP_ROUT_METHOD_E)
    {
        GT_U32 onesBits = 0;
        GT_U8 i;

        routEntryIndex = ~(0xffffffff << (descrPtr->nextHopEcpQosSize + 1));
        routEntryIndex &= descrPtr->ecmpHash;
        for (i = 0; i < descrPtr->nextHopEcpQosSize + 1; i++)
        {
            onesBits += routEntryIndex & 0x01;
            routEntryIndex >>= 1;
        }
        routEntryIndex = onesBits;
    }
    else
    if (descrPtr->nextHopRoutMethod == IPV4_LPM_QOS_ROUT_METHOD_E)
    {
        tos =  descrPtr->ipHeaderPtr[1];
        routEntryIndex =
            ~(0xffffffff << (descrPtr->nextHopEcpQosSize + 1)) & tos;
    }
    else
    if (descrPtr->nextHopRoutMethod == IPV4_LPM_REGULAR_ROUT_METHOD_E)
    {
        routEntryIndex = 0;
    }
    routEntryAddr = 0x28000000
         + ((descrPtr->nextHopPtr + routEntryIndex) << 4);
    /* Get rout entry pointer word 0 */
    routEntryPtr = (GT_U32 *)smemMemGet(devObjPtr, routEntryAddr);
    /* Cmd */
    routEntryCmd = (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 0, 2);

    if (routEntryCmd == IPV4_CMD_DROP_E)
    {
        descrPtr->ipv4Done = 1;
        return;
    }
    if (routEntryCmd == IPV4_CMD_TRANSPARENT_E)
    {
        return;
    }
    if (routEntryCmd == IPV4_CMD_TRAP_E)
    {
        descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
        descrPtr->bits15_2.cmd_trap2cpu.cpuCode = TWIST_CPU_CODE_PACKET_TRAPPED;
        descrPtr->ipv4Done = 1;
        return;
    }
    if (routEntryCmd == IPV4_CMD_ROUT_E)
    {
        descrPtr->ttlDecrement =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 14, 1);
    }

    /* Management Counter set to update when packet hits this Route Entry */
    __LOG(("Management Counter set to update when packet hits this Route Entry"));
    descrPtr->ipv4CounterSet =
        (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 12, 2);

    routForceCos =
        (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 3, 1);
    /* COS of the packet is taken from the route entry */
    __LOG(("COS of the packet is taken from the route entry"));
    if (routForceCos)
    {
        descrPtr->trafficClass =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 4, 3);
        descrPtr->dropPrecedence =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 7, 2);
        descrPtr->userPriorityTag =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 9, 2);
    }

    age = 0;
    if (devObjPtr->deviceFamily != SKERNEL_TWIST_C_FAMILY)
    {
        smemRegFldGet(devObjPtr, IPV4_CONTROL_REG, 13, 1, &fldValue);
        /* EnAgeRefresh */
        if (fldValue == 1)
        {
            age = 1;
        }
    }

    if (descrPtr->doIpmRout == 0)
    {
        snetTwistUcRoutEntryGet(devObjPtr, routEntryAddr, &ucRoutEntry);
        if (age == 1)
        {
            ucRoutEntry.age = 1;
        }
        descrPtr->outLifType = ucRoutEntry.ucNextHopEntry.outLifType;
        descrPtr->LLL_outLif =
            ucRoutEntry.ucNextHopEntry.out_lif.link_layer.LL_OutLIF;
        descrPtr->arpPointer =
            ucRoutEntry.ucNextHopEntry.out_lif.link_layer.arpPtr;
        descrPtr->ipv4Done = 1;
    }
    else
    {
        snetTwistMcRoutEntryGet(devObjPtr, routEntryAddr, &mcRoutEntry);
        if (age == 1)
        {
            ucRoutEntry.age = 1;
        }
        if (mcRoutEntry.checkRpf == 1)
        {
            /* this bit relevant for twistd only */
            smemRegFldGet(devObjPtr, IPV4_CONTROL_REG, 3, 1, &fldValue);
            /* UseInlifForRPF */
            useInlifForRPF = fldValue;
            if (useInlifForRPF == 0 &&
                mcRoutEntry.rpfInLif != descrPtr->vid)
            {
                rpfCheckFail = 1;
            }
            else
            if (useInlifForRPF == 1 &&
                mcRoutEntry.rpfInLif != descrPtr->inLifNumber)
            {
                rpfCheckFail = 1;
            }
            else
            {
                rpfCheckFail = 0;
            }

            if (rpfCheckFail == 1)
            {
                if (mcRoutEntry.rpfCmd == 1)
                {
                    descrPtr->ipv4Done = 0;
                    return;
                }
                else
                {
                    descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
                    descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                                            TWIST_CPU_CODE_RPF_NOT_PASSED;
                    descrPtr->ipv4Done = 1;
                    return;
                }
            }
        }
        if (mcRoutEntry.checkRpf == 0 || rpfCheckFail == 0)
        {
            descrPtr->outLifType = 2;
            descrPtr->mll = mcRoutEntry.mll;
            descrPtr->deviceVidx = mcRoutEntry.devVidx;
            descrPtr->ipv4Done = 1;/* ipv4 mc route decision is done */
        }
    }
}

/*******************************************************************************
*   snetTwistIpv4Counters
*
* DESCRIPTION:
*        Update ipV4 counters
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
static GT_BOOL snetTwistIpv4Counters
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 regAddress;
    GT_U32 fldValue;

    if (descrPtr->ipv4Done == 0)
    {
        return GT_FALSE;
    }

    switch (descrPtr->pktCmd)
    {
    case SKERNEL_PKT_FORWARD_E:
        regAddress = IPV4_GLBL_CNT0_REG;
        smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
        /* number of packets received by the IPv4 Engine */
        smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);

        regAddress = IPV4_GLBL_CNT1_REG;
        smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
        /* number of octets Least Significant Bits received
           by the IPv4 Engine */
        smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);

        regAddress = IPV4_GLBL_CNT2_REG;
        smemRegFldGet(devObjPtr, regAddress, 0, 16, &fldValue);
        /* number of octets Most Significant Bits received
           by the IPv4 Engine */
        smemRegFldSet(devObjPtr, regAddress, 0, 16, ++fldValue);
        break;
    case SKERNEL_PKT_DROP_E:
        regAddress = IPV4_GLBL_DSCRD_CNT_PKT_REG;
        smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
        /* the number of packets discarded by the route entry. */
        smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);

        regAddress = IPV4_GLBL_DSCRD_OCT_LOW_REG;
        smemRegFldGet(devObjPtr, regAddress, 0, 16, &fldValue);
        /* number of octets Most Significant Bits discarded by the route entry. */
        smemRegFldSet(devObjPtr, regAddress, 0, 16, ++fldValue);

        smemRegFldGet(devObjPtr, regAddress, 25, 7, &fldValue);
        /* number of octets Least Significant Bits discarded by the route entry. */
        smemRegFldSet(devObjPtr, regAddress, 25, 7, ++fldValue);

        regAddress = IPV4_GLBL_DSCRD_OCT_HI_REG;
        smemRegFldGet(devObjPtr, regAddress, 0, 16, &fldValue);
        /* number of octets Most Significant Bits discarded by the route entry. */
        smemRegFldSet(devObjPtr, regAddress, 0, 16, ++fldValue);
        break;
    case SKERNEL_PKT_TRAP_CPU_E:

        break;
    case SKERNEL_PKT_MIRROR_CPU_E:

        break;
    }

    if (descrPtr->ipv4CounterSet < 3)
    {
        GT_U32 offset;
        offset = descrPtr->ipv4CounterSet * 0x60;

        switch (descrPtr->pktCmd)
        {
        case SKERNEL_PKT_FORWARD_E:
            regAddress = IPV4_GLBL_RCV_PCKT_CNT_REG + offset;
            smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
            /* number of contexts passed in the set */
            smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);

            regAddress = IPV4_GLBL_RCV_OCT_LOW_CNT_REG + offset;
            smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
            /* number of contexts passed in the set 32 Least Significant Bits*/
            smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);

            regAddress = IPV4_GLBL_RCV_OCT_HI_CNT_REG + offset;
            smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
            /* number of contexts passed in the set 32 Most Significant Bits*/
            smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);

            break;
        case SKERNEL_PKT_DROP_E:
            regAddress = IPV4_GLBL_DSCRD_CNT_REG + offset;
            smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
            /* number of discarded packets */
            smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);

            regAddress = IPV4_GLBL_DSCRD_OCT_CNT_REG + offset;
            smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
            /* number of discarded octets */
            smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);

            break;
        case SKERNEL_PKT_TRAP_CPU_E:
            regAddress = IPV4_GLBL_TRAP_CNT_REG + offset;
            smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
            /* The number of trapped packets */
            smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);

            break;
        case SKERNEL_PKT_MIRROR_CPU_E:
            break;
        }
        if (descrPtr->doIpmRout == 1)
        {
            regAddress = IPV4_GLBL_MCAST_CNT_REG + offset;
            smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
            /* IP Multicast packets routed by the IPv4 engine. */
            smemRegFldSet(devObjPtr, regAddress, 0, 32, ++fldValue);
        }
    }

    return GT_TRUE;
}

/*******************************************************************************
*   ipV4CheckSumCalc
*
* DESCRIPTION:
*        Perform ones-complement sum , and ones-complement on the final sum-word.
*        The function can be used to make checksum for various protocols.
* INPUTS:
*        bytesPtr - pointer to IP header.
*        numBytes - IP header length.
* OUTPUTS:
*
* COMMENTS:
*        1. If there's a field CHECKSUM within the input-buffer
*           it supposed to be zero before calling this function.
*
*        2. The input buffer is supposed to be in network byte-order.
*******************************************************************************/
GT_U32 ipV4CheckSumCalc
(
    IN GT_U8 *bytesPtr,
    IN GT_U16 numBytes
)
{
    GT_U32 vResult;
    GT_U32 sum;
    GT_U32 byteIndex;

    sum = 0;

    /* add up all of the 16 bit quantities */
    for (byteIndex = 0 ; byteIndex < numBytes ;  byteIndex += 2)
    {
        sum += (bytesPtr[byteIndex   + 0] << 8) |
                bytesPtr[byteIndex   + 1] ;
    }

    if (numBytes & 1)
    {
        /* an odd number of bytes */
        sum += (bytesPtr[byteIndex   + 0] << 8);
    }

    /* sum together the two 16 bits sections*/
    vResult = (GT_U16)(sum >> 16) + (GT_U16)sum;

    /* vResult can be bigger then a word (example : 0003 +
    *  fffe = 0001 0001), so sum its two words again.
    */
    if (vResult & 0x10000)
    {
        vResult -= 0x0ffff;
    }

    return (GT_U16)(~vResult);
}

/*******************************************************************************
*   snetTwistIpMcGroupGet
*
* DESCRIPTION:
*        Get Mc group entry
*
* INPUTS:
*        devObjPtr - pointer to device object
*        nextLookupAddr - Mc group entry address in NSRAM
*        level - the level of the mc trie search
* OUTPUTS:
*       ipMcEntryPtr -  pointer to  OUT   IPMC_GROUP_ENTRY_UNT
*
*******************************************************************************/
static void snetTwistIpMcGroupGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    GT_U32 nextLookupAddr,
    IN    GT_U32 level,
    OUT   IPMC_GROUP_ENTRY_UNT * ipMcEntryPtr
)
{
    GT_U32 mcGrpEntryPtr;
    GT_U8 mcGroupType;

    /* Fetch Multicast group Entry */
    mcGrpEntryPtr = smemTwistNsramRead(devObjPtr, TWIST_NSRAM_E, nextLookupAddr);

    /* deal with levels 1,2 of the search */
    if ((level == 1)|| (level == 2))
    {

            ipMcEntryPtr->level1_2.nextMcPtr =
                    (GT_U32) SMEM_U32_GET_FIELD(mcGrpEntryPtr, 0, 24);
            ipMcEntryPtr->level1_2.minimumPtr =
                    (GT_U8) SMEM_U32_GET_FIELD(mcGrpEntryPtr, 24, 4);
            ipMcEntryPtr->level1_2.maximumPtr =
                    (GT_U8) SMEM_U32_GET_FIELD(mcGrpEntryPtr, 28, 4);
    }
    else
    {
        /* deal with level 3 of the search */

        mcGroupType = (GT_U8) SMEM_U32_GET_FIELD(mcGrpEntryPtr, 30, 2);
        if (mcGroupType == 0)
        {
                    ipMcEntryPtr->level3.regular_bkt.nextMcPtr =
                (GT_U32) SMEM_U32_GET_FIELD(mcGrpEntryPtr, 0, 24);
        }
        else
        if ((mcGroupType == 2) || (mcGroupType == 1))
        {
            ipMcEntryPtr->level3.compress_bkt.nextMcPtr =
                (GT_U32) SMEM_U32_GET_FIELD(mcGrpEntryPtr, 0, 22);
            ipMcEntryPtr->level3.compress_bkt.lastIdx =
                (GT_U8) SMEM_U32_GET_FIELD(mcGrpEntryPtr, 22, 8);
        }
        if (mcGroupType == 3)
        {
            ipMcEntryPtr->level3.next_hope.nextHopeRoute =
                (GT_U32) SMEM_U32_GET_FIELD(mcGrpEntryPtr, 0, 22);
            ipMcEntryPtr->level3.next_hope.ecpQosSize =
                (GT_U8) SMEM_U32_GET_FIELD(mcGrpEntryPtr, 24, 4);
            ipMcEntryPtr->level3.next_hope.nextHopMethod =
                (GT_U8) SMEM_U32_GET_FIELD(mcGrpEntryPtr, 28, 2);
        }

        ipMcEntryPtr->level3.next_hope.mcGroupType = mcGroupType;
    }
}

/*******************************************************************************
*   snetTwistLpmNextPtrGet
*
* DESCRIPTION:
*        Get LPM next pointer
*
* INPUTS:
*        devObjPtr - pointer to device object
*        nextPtrAddr - LPM next pointer entry address in NSRAM
* OUTPUTS:
*       lpmNextPtr -  pointer to  OUT   SNET_LPM_NEXT_POINTER_UNT
*
*******************************************************************************/
static GT_U8 snetTwistLpmNextPtrGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    GT_U32 nextPtrAddr,
    OUT   SNET_LPM_NEXT_POINTER_UNT * lpmNextPtr
)
{
    GT_U32 lpmNextEntry;
    GT_U8 bucketType;

    /* Fetch LPM next pointer entry */
    lpmNextEntry = smemTwistNsramRead(devObjPtr, TWIST_NSRAM_E, nextPtrAddr);

    bucketType = (GT_U8) SMEM_U32_GET_FIELD(lpmNextEntry, 30, 2);
    if (bucketType == IPV4_LPM_REGULAR_BUCKET_E ||
        bucketType == IPV4_LPM_COMPRESS_1_BUCKET_E)
    {
        lpmNextPtr->bucket_0_1.nextPtr =
            (GT_U32)SMEM_U32_GET_FIELD(lpmNextEntry, 0, 24);
        lpmNextPtr->bucket_0_1.bucketType =
            (GT_U8) SMEM_U32_GET_FIELD(lpmNextEntry, 30, 2);;
    }
    else
    if (bucketType == IPV4_LPM_COMPRESS_2_BUCKET_E)
    {
        lpmNextPtr->bucket_2.nextPtr =
            (GT_U32)SMEM_U32_GET_FIELD(lpmNextEntry, 0, 22);
        lpmNextPtr->bucket_2.lastIdx =
            (GT_U8) SMEM_U32_GET_FIELD(lpmNextEntry, 22, 8);
        lpmNextPtr->bucket_2.bucketType =
            (GT_U8) SMEM_U32_GET_FIELD(lpmNextEntry, 30, 2);
    }
    else
    if (bucketType == IPV4_LPM_NEXT_HOP_BUCKET_E)
    {
        lpmNextPtr->bucket_3.nextPtr =
            (GT_U32)SMEM_U32_GET_FIELD(lpmNextEntry, 0, 22);
        lpmNextPtr->bucket_3.ecpQosSize =
            (GT_U8) SMEM_U32_GET_FIELD(lpmNextEntry, 24, 4);
        lpmNextPtr->bucket_3.nextHopMethod =
            (GT_U8) SMEM_U32_GET_FIELD(lpmNextEntry, 28, 2);
        lpmNextPtr->bucket_3.bucketType =
            (GT_U8) SMEM_U32_GET_FIELD(lpmNextEntry, 30, 2);
    }

    return bucketType;
}

/*******************************************************************************
*   snetTwistUcRoutEntryGet
*
* DESCRIPTION:
*        Get Unicast Route Entry
*
* INPUTS:
*        devObjPtr - pointer to device object
*        ucEntryAddr - unicast route entry address in SRAM
* OUTPUTS:
*       ucRoutePtr -  pointer to  OUT   UC_ROUT_ENTRY_STC
*
*******************************************************************************/
static GT_VOID snetTwistUcRoutEntryGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    GT_U32 ucEntryAddr,
    OUT   UC_ROUT_ENTRY_STC * ucRoutePtr
)
{
    GT_U32 * routEntryPtr;

    memset(ucRoutePtr, 0, sizeof(UC_ROUT_ENTRY_STC));
    /* Get rout entry pointer word 0 */
    routEntryPtr = (GT_U32 *)smemMemGet(devObjPtr, ucEntryAddr);
    ucRoutePtr->cmd =
        (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 0, 2);
    ucRoutePtr->forceCos =
        (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 3, 1);
    if (ucRoutePtr->forceCos)
    {
        ucRoutePtr->priority =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 4, 3);
        ucRoutePtr->dp =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 7, 2);
        ucRoutePtr->vpt =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 9, 3);
    }
    ucRoutePtr->ipMngCntSet =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 12, 2);
    ucRoutePtr->enableDecTtl =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 14, 1);
    ucRoutePtr->age =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 16, 1);
    ucRoutePtr->mtu =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 31, 1) |
    /* Word 1 */
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 1, 0, 13) << 1;

    ucRoutePtr->ucNextHopEntry.outLifType =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 1, 13, 1);
    if (ucRoutePtr->ucNextHopEntry.outLifType == 0)
    {
        ucRoutePtr->ucNextHopEntry.out_lif.link_layer.LL_OutLIF =
            (GT_U32) WORD_FIELD_GET(routEntryPtr, 1, 14, 18) |
    /* Word 2 */
            (GT_U32) WORD_FIELD_GET(routEntryPtr, 2, 0, 12) << 18;
        ucRoutePtr->ucNextHopEntry.out_lif.link_layer.arpPtr =
            (GT_U32) WORD_FIELD_GET(routEntryPtr, 2, 12, 14);
    }
    else
    {
        ucRoutePtr->ucNextHopEntry.out_lif.tunnel_outlif.tunnelOutLif =
            (GT_U32) WORD_FIELD_GET(routEntryPtr, 1, 14, 18) |
    /* Word 2 */
            (GT_U32) WORD_FIELD_GET(routEntryPtr, 2, 0, 14) << 18;
    }
    /* Word 2 */
    ucRoutePtr->greTunnelEnd =
        (GT_U8) WORD_FIELD_GET(routEntryPtr, 2, 27, 1);
    ucRoutePtr->ipTunnelEnd =
        (GT_U8) WORD_FIELD_GET(routEntryPtr, 2, 28, 1);
    ucRoutePtr->tunnelCpyDscp =
        (GT_U8) WORD_FIELD_GET(routEntryPtr, 2, 29, 1);
    ucRoutePtr->pushMpls =
        (GT_U8) WORD_FIELD_GET(routEntryPtr, 2, 30, 1);

    if (ucRoutePtr->pushMpls == 1)
    {
        ucRoutePtr->mplsInfo.expSet =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 2, 31, 1);
    /* Word 3 */
        ucRoutePtr->mplsInfo.label =
            (GT_U32)WORD_FIELD_GET(routEntryPtr, 3, 0, 1) |
            (GT_U32)WORD_FIELD_GET(routEntryPtr, 3, 1, 19) << 1;
        ucRoutePtr->mplsInfo.exp =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 3, 20, 3);
        ucRoutePtr->mplsInfo.ttl =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 3, 23, 8);
    }
}

/*******************************************************************************
*   snetTwistMcRoutEntryGet
*
* DESCRIPTION:
*        Get Multicast Route Entry
*
* INPUTS:
*        devObjPtr - pointer to device object
*        mcEntryAddr - unicast route entry address in SRAM
* OUTPUTS:
*       mcRoutePtr -  pointer to  OUT   MC_ROUT_ENTRY_STC
*
*******************************************************************************/
static GT_VOID snetTwistMcRoutEntryGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    GT_U32 mcEntryAddr,
    OUT   MC_ROUT_ENTRY_STC * mcRoutePtr
)
{
    GT_U32 * routEntryPtr;

    memset(mcRoutePtr, 0, sizeof(MC_ROUT_ENTRY_STC));

    /* Get rout entry pointer word 0 */
    routEntryPtr = (GT_U32 *)smemMemGet(devObjPtr, mcEntryAddr);
    mcRoutePtr->cmd =
        (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 0, 2);
    mcRoutePtr->forceCos =
        (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 3, 1);
    if (mcRoutePtr->forceCos)
    {
        mcRoutePtr->priority =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 4, 3);
        mcRoutePtr->dp =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 7, 2);
        mcRoutePtr->vpt =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 9, 3);
    }
    mcRoutePtr->ipMngCntSet =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 12, 2);
    mcRoutePtr->enableDecTtl =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 14, 1);
    mcRoutePtr->l2Ce =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 15, 1);
    mcRoutePtr->age =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 16, 1);
    mcRoutePtr->mtu =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 0, 31, 1) |
    /* Word 1 */
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 1, 0, 13) << 1;
    mcRoutePtr->rpfCmd =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 1, 14, 1);
    mcRoutePtr->checkRpf =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 1, 15, 1);
    mcRoutePtr->rpfInLif =
            (GT_U16)WORD_FIELD_GET(routEntryPtr, 1, 16, 16);
    /* Word 2 */
    mcRoutePtr->mll =
            (GT_U16)WORD_FIELD_GET(routEntryPtr, 2, 0, 16);
    mcRoutePtr->devVidx =
            (GT_U16)WORD_FIELD_GET(routEntryPtr, 2, 16, 14);

    mcRoutePtr->pushMpls =
        (GT_U8) WORD_FIELD_GET(routEntryPtr, 2, 30, 1);
    if (mcRoutePtr->pushMpls == 1)
    {
        mcRoutePtr->mplsInfo.expSet =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 2, 31, 1);
    /* Word 3 */
        mcRoutePtr->mplsInfo.label =
            (GT_U32)WORD_FIELD_GET(routEntryPtr, 3, 0, 1) |
            (GT_U32)WORD_FIELD_GET(routEntryPtr, 3, 1, 19) << 1;
        mcRoutePtr->mplsInfo.exp =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 3, 20, 3);
        mcRoutePtr->mplsInfo.ttl =
            (GT_U8) WORD_FIELD_GET(routEntryPtr, 3, 23, 8);
    }
}

