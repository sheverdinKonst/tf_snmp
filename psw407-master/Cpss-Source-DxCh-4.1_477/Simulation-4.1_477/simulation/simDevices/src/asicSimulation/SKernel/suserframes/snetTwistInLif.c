/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetTwistInLif.c
*
* DESCRIPTION:
*      This is a external API definition for Input Logical Interface of SKernel.
*
* DEPENDENCIES:
*      None.
*
* FILE REVISION NUMBER:
*      $Revision: 12 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/suserframes/snetTwistInLif.h>
#include <asicSimulation/SKernel/suserframes/snetTwistL2.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/twistCommon/sregTwist.h>
#include <asicSimulation/SKernel/smem/smemTwist.h>
#include <asicSimulation/SLog/simLog.h>

#define IS_DST_MCAST(descrPtr) \
                ((descrPtr->dstMacPtr[0] == 0x01) && \
                 (descrPtr->dstMacPtr[1] == 0x00) && \
                 (descrPtr->dstMacPtr[2] == 0x5E) && \
                 ((descrPtr->dstMacPtr[3] & 0x80) == 0x0) && \
                 ((descrPtr->ipv4DIP & 0xf0000000) == 0xe0000000))

static void snetTwistFlowTemplateAssign
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_U32 * snetTwistInLifFind
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

static  GT_BOOL snetTwistInLifAnalize
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    GT_U32 * inlifEntryPtr
);

static void snetTwistIpSecurity
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    GT_U32 * inlifEntryPtr
);


static GT_VOID snetTwistGetInLifInfo
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    GT_U32 * inlifEntryPtr,
    OUT   IN_LIF_OBJECT_STC * inLifObjPtr
);


/*******************************************************************************
*   snetTwistInLifProcess
*
* DESCRIPTION:
*        InLif processing
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
GT_BOOL snetTwistInLifProcess
(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN  SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 * inlifEntryPtr;

    snetTwistFlowTemplateAssign(devObjPtr, descrPtr);

    inlifEntryPtr = snetTwistInLifFind(devObjPtr, descrPtr);

    snetTwistIpSecurity(devObjPtr, descrPtr, inlifEntryPtr);

    return snetTwistInLifAnalize(devObjPtr, descrPtr, inlifEntryPtr);
}

/*******************************************************************************
*   snetTwistFlowTemplateAssign
*
* DESCRIPTION:
*        Flow template assign
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
static void snetTwistFlowTemplateAssign
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 ipProtocol, ipOffset;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);

    if (descrPtr->ipHeaderPtr == NULL) {
        descrPtr->flowTemplate = SKERNEL_ETHERNET_E;
        return;
    }
    else {
        ipOffset = ((descrPtr->ipHeaderPtr[6] & 0x1F) << 8) |
                    descrPtr->ipHeaderPtr[7];
    }
    if (ipOffset > 0) {
        descrPtr->flowTemplate = SKERNEL_IP_FRAGMENT_E;
        return;
    }
    else {
        ipProtocol = descrPtr->ipHeaderPtr[9];
    }
    if (ipProtocol == 6) {
        descrPtr->flowTemplate = SKERNEL_TCP_FLOW_E;
        return;
    }
    else
    if (ipProtocol == 17) {
        descrPtr->flowTemplate = SKERNEL_UDP_FLOW_E;
        return;
    }
    else {
        descrPtr->flowTemplate = SKERNEL_IP_OTHER_E;
    }
}

/*******************************************************************************
*   snetTwistInLifFind
*
* DESCRIPTION:
*        Port to logical interface
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
static GT_U32 * snetTwistInLifFind
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistInLifFind);

    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 firstBit;
    GT_U16 isSinglnLif;
    GT_U32 inLifBaseAddr, * inLifEntryPtr;
    GT_U16 minVid, maxVid;

    PORT_TO_IN_LIF_REG(descrPtr->srcPort, &regAddress);
    firstBit = IS_SINGLE_IN_LIF(descrPtr->srcPort);
    smemRegFldGet(devObjPtr, regAddress, firstBit, 1, &fldValue);
    /* Port<port>Isnt SingleInLIF */
    isSinglnLif = (GT_U16)fldValue;

    /* Internal Input Logical Interface Table Register */
    regAddress = INTERN_INLIF_TABLE_REG + descrPtr->srcPort * 4 * 4;

    inLifEntryPtr = (GT_U32 *)smemMemGet(devObjPtr, regAddress);

    /* InLIFEntry for Sub Partitioned port */
    if (isSinglnLif != 1) {
        return inLifEntryPtr;
    }

    /* ExternalInLIFBase Address */
    /* INLIF_ENTRY_BIT_ALIGN (4) see corePrvInlif.h */
    inLifBaseAddr = (inLifEntryPtr[0] & 0x3FFFFF) << 4;
    /* PortMinVID */
    minVid = (GT_U16)((inLifEntryPtr[0] >> 28) & 0xF);
    minVid |= (GT_U16)((inLifEntryPtr[1] & 0xFF) << 4);
    /* PortMaxVID */
    maxVid = (GT_U16)((inLifEntryPtr[1] >> 8) & 0xFFF);

    /* Internal Input Logical Interface Table Register */
    if ((descrPtr->vid >= minVid) && (descrPtr->vid <= maxVid))
    {
        regAddress = STWIST_WSRAM_BASE_ADDR +
                        (inLifBaseAddr + (descrPtr->vid - minVid)*16);
    }
    else
    {
    /* get Default Input Logical Interface Register address */
        __LOG(("get Default Input Logical Interface Register address"));
        regAddress = DEFAULT_INLIF_REG;
    }

    inLifEntryPtr = (GT_U32 *)smemMemGet(devObjPtr, regAddress);

    return inLifEntryPtr;
}
/*******************************************************************************
*   snetTwistInLifAnalize
*
* DESCRIPTION:
*        Port to logical interface
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
static  GT_BOOL snetTwistInLifAnalize
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    GT_U32 * inlifEntryPtr
)
{
    DECLARE_FUNC_NAME(snetTwistInLifAnalize);

    GT_U8 isIPM = 0;
    IN_LIF_OBJECT_STC inLifObj;


    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);

        snetTwistGetInLifInfo(devObjPtr, inlifEntryPtr, &inLifObj);

        if (descrPtr->pktCmd == SKERNEL_PKT_TRAP_CPU_E) {
            descrPtr->doClassify = 0;
            descrPtr->doRout = 0;
        }
        else
        if (descrPtr->pktCmd == SKERNEL_PKT_DROP_E) {
                return GT_FALSE;
        }
        else
        /* Dis_Bridging */
            __LOG(("Dis_Bridging"));
        if (inLifObj.bridgEn & 0x01) {
            descrPtr->doRout = 0;
            descrPtr->doClassify = 0;
            descrPtr->doPcl = 0;
            descrPtr->pktCmd = SKERNEL_PKT_FORWARD_E;
            descrPtr->vlanCmd = SKERNEL_NOT_CHANGE_E;
            descrPtr->useVidx = 0;
            descrPtr->bits15_2.useVidx_0.targedDevice = descrPtr->srcDevice;
            descrPtr->bits15_2.useVidx_0.targedPort = 61;
        }
        if (descrPtr->ipHeaderPtr != NULL) {
                isIPM = IS_DST_MCAST(descrPtr);
        }

        if (isIPM == 1) {
            /* En_IPv4_MC */
            __LOG(("En_IPv4_MC"));
            descrPtr->doIpmRout = inLifObj.ipV4McEn;
        }
        else
        if (descrPtr->doRout == 1){
            /* En_IPv4 */
            __LOG(("En_IPv4"));
            descrPtr->doRout = inLifObj.ipV4En;
        }
        /* En_Classify */
        __LOG(("En_Classify"));
        if ( inLifObj.classifyEn == 0 ) {
            descrPtr->doClassify = 0;
        }
        else
        if ( inLifObj.forceClassify == 1 ) {
            descrPtr->doClassify = 1;
        }

        /* En_Pcl */
        __LOG(("En_Pcl"));
        if ( inLifObj.pclEn == 0 ) {
            descrPtr->doPcl = 0;
        }
        else
        if ( inLifObj.forcePcl == 1 ) {
            descrPtr->doPcl = 1;
        }
        /* En_MPLS */
        __LOG(("En_MPLS"));
        if ( descrPtr->mplsTopLablePtr != NULL ) {
            descrPtr->doMpls = inLifObj.mplsEn;
        }
        /* Major_Template */
        descrPtr->majorTemplate = inLifObj.majorTemplate;
        /* VR-Id The Virtual Router ID */
        descrPtr->vrId = inLifObj.vrtRoutId;
        /* InLIF# */
        descrPtr->inLifNumber = inLifObj.inLifNumber;
        /* pcl_base_addr The first address for the PCL lookup. */
        descrPtr->pclBaseAddr = inLifObj.pclBaseAddr;
        /* pcl_max_hop */
        descrPtr->pclMaxHops = inLifObj.pclMaxHop;
    /* pcl_number */
        descrPtr->pclNum = inLifObj.pclNumber;
        /* Trap_MC_Local_Scope */
        descrPtr->trapMcLocalScope = inLifObj.trapMcLclScope;
        /* Trap_Reserved_DIP */
        descrPtr->trapReservedDip = inLifObj.trapRsrvDstIp;
        /* Trap_Internal_DIP */
        descrPtr->trapIntDip = inLifObj.trapInternDstIp;

        descrPtr->pclId0 = inLifObj.pclId0;
        descrPtr->pclId1 = inLifObj.pclId1;

        return GT_TRUE;
}



/*******************************************************************************
*   snetTwistIpSecurity
*
* DESCRIPTION:
*        IP security of input LIF
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*                 inlifEntryAddr - input lif entry address.
*        inlifEntryPtr
*                   - pointer to InLif table entry
* OUTPUTS:
*
*
*******************************************************************************/
static void snetTwistIpSecurity
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    GT_U32 * inlifEntryPtr
)
{
    DECLARE_FUNC_NAME(snetTwistIpSecurity);

    IN_LIF_OBJECT_STC inLifObj;
    GT_U32 ipAddr;
    GT_U32 ipAddrMask;
    GT_U32 regAddress;
    GT_U32 fldValue;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);

    snetTwistGetInLifInfo(devObjPtr, inlifEntryPtr, &inLifObj);

    if (descrPtr->ipHeaderPtr ==  NULL) {
        return;
    }
    if (descrPtr->pktCmd == SKERNEL_PKT_DROP_E && inLifObj.bridgEn == 0) {
        return;
    }

    if (inLifObj.sipSpoofPrevent == 1) {
        ipAddrMask = ~(0xffffffff >> inLifObj.srcIpSubnetLen);
        ipAddr = descrPtr->ipv4SIP & ipAddrMask;
        if ( ipAddr != inLifObj.srcIp) {
                descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
                descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                                    TWIST_CPU_CODE_SPOOF_SIP_TRAP;
                return;
        }
    }
    if (inLifObj.trapReservSip == 1) {
        ipAddrMask = ~(0xffffffff >> 8);
        ipAddr = descrPtr->ipv4SIP & ipAddrMask;
        if (ipAddr == 10) {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                            TWIST_CPU_CODE_RESRVED_SIP_TRAP;
            return;
        }
        ipAddrMask = ~(0xffffffff >> 12);
        ipAddr = descrPtr->ipv4SIP & ipAddrMask;
        if (((ipAddr >> 24) == 172) && ((ipAddr >> 16) & 0xFF) == 16) {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                                TWIST_CPU_CODE_RESRVED_SIP_TRAP;
            return;
        }
        ipAddrMask = ~(0xffffffff >> 16);
        ipAddr = (descrPtr->ipv4SIP & ipAddrMask);
        if (((ipAddr >> 24) == 196) && ((ipAddr >> 16) & 0xFF) == 168) {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                                TWIST_CPU_CODE_RESRVED_SIP_TRAP;
            return;
        }
        ipAddrMask = 0xffffffff;
        ipAddr = (descrPtr->ipv4SIP & ipAddrMask);
        if (ipAddr == 0) {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                                TWIST_CPU_CODE_RESRVED_SIP_TRAP;
            return;
        }
    }
    if (inLifObj.trapInternalSip == 1) {
        /* Subnet Mask Size Register 0 */
        __LOG(("Subnet Mask Size Register 0"));
        regAddress = 0x02800014;
        smemRegFldGet(devObjPtr, regAddress, 16, 5, &fldValue);
        if (fldValue < 31) {
            ipAddrMask = ~(0xffffffff >> fldValue);
        }
        else {
            ipAddrMask = 0xffffffff;
        }
        regAddress = 0x0280000C;
        smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
        /* SIP<0> */
        ipAddr = fldValue;
        if ((descrPtr->ipv4SIP & ipAddrMask) == ipAddr) {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                                TWIST_CPU_CODE_INTERNAL_SIP_TRAP;
            return;
        }
        /* Subnet Mask Size Register 1 */
        __LOG(("Subnet Mask Size Register 1"));
        regAddress = 0x02800014;
        smemRegFldGet(devObjPtr, regAddress, 24, 5, &fldValue);
        if (fldValue < 31) {
            ipAddrMask = ~(0xffffffff >> fldValue);
        }
        else {
            ipAddrMask = 0xffffffff;
        }
        regAddress = 0x02800010;
        smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);
        /* SIP<1> */
        ipAddr = fldValue;
        if ((descrPtr->ipv4SIP & ipAddrMask) == ipAddr) {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                                TWIST_CPU_CODE_INTERNAL_SIP_TRAP;
            return;
        }
    }
}


/*******************************************************************************
*   snetTwistGetInLifInfo
*
* DESCRIPTION:
*        Fill IN_LIF_OBJECT_STC structure
* INPUTS:
*        devObjPtr - pointer to device object.
*        inlifEntryPtr  - pointer InLif table entry.
* OUTPUTS:
*        inLifObjPtr - pointer to IN_LIF_OBJECT_STC
*
*******************************************************************************/
static GT_VOID snetTwistGetInLifInfo
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    GT_U32 * inlifEntryPtr,
    OUT   IN_LIF_OBJECT_STC * inLifObjPtr
)
{
    GT_U32 * lifEntryPtr;

    ASSERT_PTR(inLifObjPtr);
    /* word 0 */
    lifEntryPtr = inlifEntryPtr;
    /* Dis_Bridging */
    inLifObjPtr->bridgEn = (GT_U8)(*lifEntryPtr & 0x01);
    /* En_MPLS */
    inLifObjPtr->mplsEn = (GT_U8)((*lifEntryPtr >> 1) & 0x01);
    /* En_MPLS_MC */
    inLifObjPtr->mplsMcEn = (GT_U8)((*lifEntryPtr >> 2) & 0x01);
    /* En_MPLS_MC */
    inLifObjPtr->ipV4En = (GT_U8)((*lifEntryPtr >> 3) & 0x01);
    /* En_IPv4_MC */
    inLifObjPtr->ipV4McEn = (GT_U8)((*lifEntryPtr >> 4) & 0x01);
    /* En_IPv6 */
    inLifObjPtr->ipV6En = (GT_U8)((*lifEntryPtr >> 5) & 0x01);
    /* En_IPv6_MC */
    inLifObjPtr->ipV6McEn = (GT_U8)((*lifEntryPtr >> 6) & 0x01);
    /* En_Classify */
    inLifObjPtr->classifyEn = (GT_U8)((*lifEntryPtr >> 7) & 0x01);
    /* Major_Template */
    inLifObjPtr->majorTemplate = (GT_U8)((*lifEntryPtr >> 8) & 0x01);
    /* Force_Classify */
    inLifObjPtr->forceClassify = (GT_U8)((*lifEntryPtr >> 10) & 0x01);
    /* En_CIB */
    inLifObjPtr->cibEn = (GT_U8)((*lifEntryPtr >> 11) & 0x01);
    /* En_CIB */
    inLifObjPtr->cibEn = (GT_U8)((*lifEntryPtr >> 11) & 0x01);
    /* Force_CIB */
    inLifObjPtr->forceCib = (GT_U8)((*lifEntryPtr >> 12) & 0x01);
    /* Do_NAT */
    inLifObjPtr->doNat = (GT_U8)((*lifEntryPtr >> 13) & 0x01);
    /* pclEn */
    inLifObjPtr->pclEn = (GT_U8)((*lifEntryPtr >> 14) & 0x01);
    /* forcePcl */
    inLifObjPtr->forcePcl = (GT_U8)((*lifEntryPtr >> 15) & 0x01);
    /* VR-Id The Virtual Router ID */
    inLifObjPtr->vrtRoutId = (GT_U16)((*lifEntryPtr >> 16) & 0x1FFF);

    /* word 1 */
    lifEntryPtr++;

    /* InLIF# */
    inLifObjPtr->inLifNumber = (GT_U16)(*lifEntryPtr & 0xFFFF);
    /* Prevent_SIP_Spoofing */
    inLifObjPtr->sipSpoofPrevent = (GT_U8)((*lifEntryPtr >> 16) & 0x01);
    /* Trap_Reserved_SIP */
    inLifObjPtr->trapReservSip = (GT_U8)((*lifEntryPtr >> 17) & 0x01);
    /* Trap_Internal_SIP */
    inLifObjPtr->trapInternalSip = (GT_U8)((*lifEntryPtr >> 18) & 0x01);
    /* Trap_MC_Local_Scope */
    inLifObjPtr->trapMcLclScope = (GT_U8)((*lifEntryPtr >> 20) & 0x01);
    /* Trap_Reserved_DIP */
    inLifObjPtr->trapRsrvDstIp = (GT_U8)((*lifEntryPtr >> 21) & 0x01);
    /* Trap_Internal_DIP */
    inLifObjPtr->trapInternDstIp = (GT_U8)((*lifEntryPtr >> 22) & 0x01);
    /* Source_IP_Subnet_Len */
    inLifObjPtr->srcIpSubnetLen = (GT_U16)((*lifEntryPtr >> 26) & 0x3F);

    /* word 2 */
    lifEntryPtr++;
    /* Source_IP */
    inLifObjPtr->srcIp = (*lifEntryPtr & 0xFF) << 24 |
                         ((*lifEntryPtr >> 8) & 0xFF) << 16 |
                         ((*lifEntryPtr >> 16) & 0xFF) << 8 |
                         ((*lifEntryPtr >> 24) & 0xFF);
    /* word 3 */
    lifEntryPtr++;
    /********************/
    /* only TwistD uses */
    /********************/

    /* pcl_base_addr */
    inLifObjPtr->pclBaseAddr = (GT_U16)(*lifEntryPtr & 0x3FF);
    /* pcl_max_hop */
    inLifObjPtr->pclMaxHop = (GT_U16)((*lifEntryPtr >> 10) & 0x3FF);
    /* pcl_number */
    inLifObjPtr->pclNumber = (GT_U8)((*lifEntryPtr >> 20) & 0x0F);

    /*******************/
    /* only Samba uses */
    /*******************/
    /* pclId0 */
    inLifObjPtr->pclId0 = (GT_U8)(*lifEntryPtr & 0x3f);
    /* pclId1 */
    inLifObjPtr->pclId1 = (GT_U8)((*lifEntryPtr>>6) & 0x3f);

}

