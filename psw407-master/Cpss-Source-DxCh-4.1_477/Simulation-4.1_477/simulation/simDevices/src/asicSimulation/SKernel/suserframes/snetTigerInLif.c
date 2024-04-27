/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetTigerInLif.c
*
* DESCRIPTION:
*      This is a external API definition for Input Logical Interface of SKernel.
*
* DEPENDENCIES:
*      None.
*
* FILE REVISION NUMBER:
*      $Revision: 9 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/suserframes/snetTigerInLif.h>
#include <asicSimulation/SKernel/suserframes/snetTwistL2.h>
#include <asicSimulation/SKernel/twistCommon/sregTiger.h>
#include <asicSimulation/SKernel/smem/smem.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SLog/simLog.h>


#define IS_DST_MCAST(descrPtr) \
                ((descrPtr->dstMacPtr[0] == 0x01) && \
                 (descrPtr->dstMacPtr[1] == 0x00) && \
                 (descrPtr->dstMacPtr[2] == 0x5E) && \
                 ((descrPtr->dstMacPtr[3] & 0x80) == 0x0) && \
                 ((descrPtr->ipv4DIP & 0xf0000000) == 0xe0000000))

static void snetTigerInLifAnalyzeAttributes
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    TIGER_IN_LIF_OBJECT_STC * tigerInLifObj
);

static GT_BOOL snetTigerInLifClassify
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

static void snetTigerInLifAssignment
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT   GT_U32 ** perPortInlifEntryPtr_Ptr,
    OUT   GT_U32 ** perVlanInlifEntryPtr_Ptr
);

static void snetTigerGetInLifAttributes
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    GT_U32 * perPortInlifEntry,
    IN    GT_U32 * perVlanInlifEntry,
    IN    TIGER_IN_LIF_OBJECT_STC * tigerInLifObj
);

static void snetTigerGetInLifAttributesPerPort
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    GT_U32 * perPortInlifEntry,
    IN    TIGER_IN_LIF_OBJECT_STC * tigerInLifObj
);

static void snetTigerGetInLifAttributesPerVlan
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    GT_U32 * perVlanInlifEntry,
    IN    GT_U32 * perPortInlifEntry,
    IN    TIGER_IN_LIF_OBJECT_STC * tigerInLifObj
);

static void snetTigerGetInLifAttributesPerVlanReduced
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    GT_U32 * perVlanInlifEntry,
    IN    GT_U32 * perPortInlifEntry,
    IN    TIGER_IN_LIF_OBJECT_STC * tigerInLifObj
);

static void snetTigerGetInLifAttributesHybrid
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    GT_U32 * perVlanInlifEntry,
    IN    GT_U32 * perPortInlifEntry,
    IN    TIGER_IN_LIF_OBJECT_STC * tigerInLifObj
);

static void snetTigerIpSecurity
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    TIGER_IN_LIF_OBJECT_STC * inlifEntryPtr
);

static void snetTigerCos
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    TIGER_IN_LIF_OBJECT_STC * inlifEntryPtr
);

/*******************************************************************************
*   snetTigerInLifProcess
*
* DESCRIPTION:
*        InLif processing block.
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
* RETURNS:
*        GT_BOOL - GT_TRUE(=1) for success.
* COMMENTS:
*       The function has external definition for calling outside this module.
*
*******************************************************************************/
GT_BOOL snetTigerInLifProcess
(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN  SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTigerInLifProcess);

    GT_U32 *perVlanInlifEntryPtr;
    TIGER_IN_LIF_OBJECT_STC tigerInLifObj;
    GT_U32 *perPortInlifEntryPtr;

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);

    /* classification of inlif - per port or per vlan */
    __LOG(("classification of inlif - per port or per vlan"));
    if (snetTigerInLifClassify(devObjPtr,descrPtr) == GT_FALSE)
    {
        return GT_FALSE;
    }

    /* finds the records of inlif from table */
    snetTigerInLifAssignment(devObjPtr,descrPtr,
                             &perPortInlifEntryPtr,&perVlanInlifEntryPtr);

    /* get attributes of inlif */
    snetTigerGetInLifAttributes(devObjPtr,descrPtr,
                                perPortInlifEntryPtr,perVlanInlifEntryPtr,&tigerInLifObj);

    /* analyze inlif attributes */
    snetTigerInLifAnalyzeAttributes(devObjPtr, descrPtr, &tigerInLifObj);

    /* check ip security */
    snetTigerIpSecurity(devObjPtr, descrPtr, &tigerInLifObj);

    /* check cos attributes */
    snetTigerCos(devObjPtr, descrPtr, &tigerInLifObj);

    return GT_TRUE;
}

/*******************************************************************************
*   snetTigerInLifClassify
*
* DESCRIPTION:
*        classification of input logical interface . Two option exists in
*        98ex116/98ex126/98ex136 - inlif per vlan and inlif per port.
*        The inlif per vlan can be full inlif or reduce inlif entry.
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*
* OUTPUTS:
*        descrPtr  - the field of inLifClassify is updated .
* RETURNS:
*       GT_TRUE for success , GT_FALSE for failure.
* COMMENTS:
*        update the field of inLifClassify in packet descriptor.
*
*******************************************************************************/
static GT_BOOL snetTigerInLifClassify
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTigerInLifClassify);

    GT_U32  regAddress0;
    GT_U32  regAddress1;
    GT_U32  InLifMode0Val;
    GT_U32  InLifMode1Val;
    GT_U32  InLifMode=0;
    GT_U32  srcPort;
    GT_BOOL retVal=GT_TRUE;

    srcPort = descrPtr->srcPort;
    if (srcPort > 32 )
    {
        srcPort -= 32;
        regAddress0 = SMEM_TIGER_INLIF_LOOKUPMODE_REG1 ;
        regAddress1 = SMEM_TIGER_INLIF_LOOKUPMODE_REG3 ;
    }
    else
    {
        regAddress0 = SMEM_TIGER_INLIF_LOOKUPMODE_REG0 ;
        regAddress1 = SMEM_TIGER_INLIF_LOOKUPMODE_REG2 ;
    }
    smemRegFldGet(devObjPtr, regAddress0 , srcPort, 1, &InLifMode0Val);
    smemRegFldGet(devObjPtr, regAddress1 , srcPort, 1, &InLifMode1Val);
    /* concatenate inlifmode0 and inlifmode1 */
    __LOG(("concatenate inlifmode0 and inlifmode1"));
    InLifMode = (InLifMode1Val << 1) | InLifMode0Val;

    /* setup the inlif classification type in the descriptor */
    switch (InLifMode)
    {
        case 0x0:
             descrPtr->inLifClassify = SKERNEL_TIGER_INLIF_PER_PORT_ENTRY_E ;
             break ;
        case 0x1:
             descrPtr->inLifClassify = SKERNEL_TIGER_INLIF_PER_VLAN_ENTRY_E ;
             break;
        case 0x3:
             descrPtr->inLifClassify =
                               SKERNEL_TIGER_INLIF_PER_VLAN_PORT_HYBRID_ENRTY_E;
             break;
        default:
             descrPtr->inLifClassify = SKERNEL_TIGER_INLIF_PER_UNKNOWN_E ;
             retVal = GT_FALSE;
             break;
    }

    return retVal;
}



/*******************************************************************************
* snetTigerGetInLifAttributes
*
* DESCRIPTION:
*        Get attributes of lnlif ... from the inlif entry.
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        perPortInlifEntry - per port inlif entry.
*        perVlanInlifEntry - per vlan inlif entry.
*        tigerInLifObj     - inlif object pointer.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*        The function fills the structure of tigerInLifObj.
*******************************************************************************/
static void snetTigerGetInLifAttributes
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr  ,
    IN    GT_U32 * perPortInlifEntry,
    IN    GT_U32 * perVlanInlifEntry,
    IN    TIGER_IN_LIF_OBJECT_STC * tigerInLifObj
)
{
    DECLARE_FUNC_NAME(snetTigerGetInLifAttributes);

    GT_U32 regAddress;
    SKERNEL_TIGER_INLIF_PER_VLAN_TYPE_ENT perVlanType;

    /* check if the vlan configuration is full , or reduced*/
    __LOG(("check if the vlan configuration is full , or reduced"));
    regAddress = SMEM_TIGER_INLIF_CONTROL_REG;
    smemRegFldGet(devObjPtr, regAddress , 0, 27, (void *)&perVlanType);

    memset( tigerInLifObj , 0 , sizeof(TIGER_IN_LIF_OBJECT_STC) );
    /* get the per port inlif entry */
    if (descrPtr->inLifClassify == SKERNEL_TIGER_INLIF_PER_PORT_ENTRY_E)   /*mode#0*/
    {
        snetTigerGetInLifAttributesPerPort(devObjPtr,
                                           descrPtr,
                                           perPortInlifEntry,
                                           tigerInLifObj);
    }
    else if (descrPtr->inLifClassify == SKERNEL_TIGER_INLIF_PER_VLAN_ENTRY_E) /*mode#1*/
    {
        if (perVlanType == SKERNEL_TIGER_INLIF_PER_VLAN_FULL_E)
        {
            snetTigerGetInLifAttributesPerVlan(devObjPtr,
                                               descrPtr,
                                               perVlanInlifEntry,
                                               perPortInlifEntry,
                                               tigerInLifObj);
        }
        else /*(perVlanType==INLIF_PER_VLAN_REDUCED_E)*/
        {
            snetTigerGetInLifAttributesPerVlanReduced(devObjPtr,
                                                      descrPtr,
                                                      perVlanInlifEntry,
                                                      perPortInlifEntry,
                                                      tigerInLifObj);
        }
    }
    else
    {
        snetTigerGetInLifAttributesHybrid(devObjPtr,
                                          descrPtr,
                                          perVlanInlifEntry,
                                          perPortInlifEntry,
                                          tigerInLifObj);
    }
}

/*******************************************************************************
* snetTigerGetInLifAttributesPerPort
*
* DESCRIPTION:
*        get attributes of inlif from the per port entry.
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        perPortInlifEntry - per port inlif entry.
*        tigerInLifObj     - inlif object pointer.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void snetTigerGetInLifAttributesPerPort
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr     ,
    IN    GT_U32 * perPortInlifEntry,
    IN    TIGER_IN_LIF_OBJECT_STC * tigerInLifObj
)
{
    GT_U32 * lifEntryPtr;

    /* word 0 */
    lifEntryPtr = perPortInlifEntry;
    /* Disable/enable Bridge layer 2 */
    tigerInLifObj->bridgEn          = (GT_U8)(*lifEntryPtr & 0x01);
    /* Disable/enable IPv4 UC */
    tigerInLifObj->ipV4En           = (GT_U8)((*lifEntryPtr >> 3) & 0x01);
    /* Disable/enable IPv4 MC */
    tigerInLifObj->ipV4McEn         = (GT_U8)((*lifEntryPtr >> 4) & 0x01);
    /* Disable/enable IPv6 UC */
    tigerInLifObj->ipV6En           = (GT_U8)((*lifEntryPtr >> 5) & 0x01);
    /* Disable/enable IPv6 MC */
    tigerInLifObj->ipV6McEn         = (GT_U8)((*lifEntryPtr >> 6) & 0x01);
    /* PCL enable */
    tigerInLifObj->pclEn            = (GT_U8)((*lifEntryPtr >> 7) & 0x01);
    /* PCL Profile */
    tigerInLifObj->pclProfile       = (GT_U8)((*lifEntryPtr >> 8) & 0x10);
    /* Force classify */
    tigerInLifObj->forcePcl         = (GT_U8)((*lifEntryPtr >> 12) & 0x01);
    /* mirror to analyser port */
    tigerInLifObj->mirrorToAnalyser = (GT_U8)((*lifEntryPtr >> 13) & 0x01);
    /* Enable/disable icmp for ipv6 */
    tigerInLifObj->ipv6IcmpEn       = (GT_U8)((*lifEntryPtr >> 14) & 0x01);
    /*  site ID for ipv6 packets */
    tigerInLifObj->ipv6Site         = (GT_U8)((*lifEntryPtr >> 15) & 0x01);
    /* Virtual Router ID */
    tigerInLifObj->vrtRoutId        = (GT_U16)((*lifEntryPtr >> 16) & 0xFFFF);

    /* word 1 */
    lifEntryPtr++;
    /* InLif number */
    tigerInLifObj->inLifNumber        = (GT_U16)((*lifEntryPtr) & 0xFFFF);
    /* SIP spoofing prevention enable */
    tigerInLifObj->sipSpoofPrevent    = (GT_U8)((*lifEntryPtr >> 16) & 0x01);
    /* enable/disable reserved SIP trap */
    tigerInLifObj->trapReservSip      = (GT_U8)((*lifEntryPtr >> 17) & 0x01);
    /* enable/disable restricted SIP trap */
    tigerInLifObj->trapRestrictedSip  = (GT_U8)((*lifEntryPtr >> 18) & 0x01);
    /* enable/disable IPv4 Multicast Local Scope */
    tigerInLifObj->ipv4McLclScopeEn   = (GT_U8)((*lifEntryPtr >> 20) & 0x01);
    /* enable/disable reserved DIP */
    tigerInLifObj->reservedDstIpv4En  = (GT_U8)((*lifEntryPtr >> 21) & 0x01);
    /* enable/disable restricted DIP */
    tigerInLifObj->restrictedDipEn    = (GT_U8)((*lifEntryPtr >> 22) & 0x01);
    /* PCL ID0 */
    tigerInLifObj->pclId0             = (GT_U8)((*lifEntryPtr >> 23) & 0x01);
    /* PCL ID1 */
    tigerInLifObj->pclId1             = (GT_U8)((*lifEntryPtr >> 24) & 0x01);
    /* enable/disable long pcl lookup0 */
    tigerInLifObj->pclLongLookupEn0   = (GT_U8)((*lifEntryPtr >> 25) & 0x01);
    /* IP subnet length */
    tigerInLifObj->srcIpSubnetLen     = (GT_U8)((*lifEntryPtr >> 26) & 0x1F);

    /* word 2 */
    lifEntryPtr++;
    /* Source_IP */
    tigerInLifObj->srcIp =  (*lifEntryPtr & 0xFF) << 24 |
                            ((*lifEntryPtr >> 8) & 0xFF) << 16 |
                            ((*lifEntryPtr >> 16) & 0xFF) << 8 |
                            ((*lifEntryPtr >> 24) & 0xFF);

    /* word 3 */
    lifEntryPtr++;
    /* pclId0 */
    tigerInLifObj->pclId0       |= (GT_U8)((*lifEntryPtr & 0x3f) << 1);
    /* pclId1 */
    tigerInLifObj->pclId1       |= (GT_U8)(((*lifEntryPtr >> 6) & 0x3f) << 1);
    /* enable igmp detect */
    tigerInLifObj->igmpDetectEn  = (GT_U8)((*lifEntryPtr >> 12) & 0x1);
    /* mirror to cpu */
    tigerInLifObj->mirrorToCpu   = (GT_U8)((*lifEntryPtr >> 13) & 0x1);
    /* default next hop */
    tigerInLifObj->defaultNextHop= (GT_U8)((*lifEntryPtr >> 14) & 0xf);
    /* enable/disable ipv6 management */
    tigerInLifObj->ipv6MgmtEn    = (GT_U8)((*lifEntryPtr >> 18) & 0x1);
    /* enable/disable ipv4 management */
    tigerInLifObj->ipv4MgmtEn    = (GT_U8)((*lifEntryPtr >> 19) & 0x1);
    /* enable/disable ripv1 broadcast mirroring */
    tigerInLifObj->ripV1En       = (GT_U8)((*lifEntryPtr >> 20) & 0x1);
    /* mark port l3 cos */
    tigerInLifObj->markPortL3Cos = (GT_U8)((*lifEntryPtr >> 21) & 0x1);
    /* dscp */
    tigerInLifObj->dscp          = (GT_U8)((*lifEntryPtr >> 22) & 0x3f);
    /* mark user priority */
    tigerInLifObj->markPortL3Cos = (GT_U8)((*lifEntryPtr >> 28) & 0x1);
    /* user priority */
    tigerInLifObj->UP            = (GT_U8)((*lifEntryPtr >> 29) & 0x7);
}

/*******************************************************************************
* snetTigerGetInLifAttributesPerVlan
*
* DESCRIPTION:
*        get attributes of inlif from the per vlan entry.
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        perVlanInlifEntry - per port inlif entry.
*        tigerInLifObj     - inlif object pointer.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void snetTigerGetInLifAttributesPerVlan
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    GT_U32 * perVlanInlifEntry,
    IN    GT_U32 * perPortInlifEntry,
    IN    TIGER_IN_LIF_OBJECT_STC * tigerInLifObj
)
{
    GT_U32 * lifEntryPtr;
    GT_U32 * lifEntryPerPortPtr;

    /* word 0 */
    lifEntryPtr = perVlanInlifEntry;
    lifEntryPerPortPtr = perPortInlifEntry;
    /* Disable/enable Bridge layer 2 */
    tigerInLifObj->bridgEn          = (GT_U8)(*lifEntryPtr & 0x01);
    /* Disable/enable IPv4 UC */
    tigerInLifObj->ipV4En           = (GT_U8)((*lifEntryPtr >> 3) & 0x01);
    /* Disable/enable IPv4 MC */
    tigerInLifObj->ipV4McEn         = (GT_U8)((*lifEntryPtr >> 4) & 0x01);
    /* Disable/enable IPv6 UC */
    tigerInLifObj->ipV6En           = (GT_U8)((*lifEntryPtr >> 5) & 0x01);
    /* Disable/enable IPv6 MC */
    tigerInLifObj->ipV6McEn         = (GT_U8)((*lifEntryPtr >> 6) & 0x01);
    /* PCL enable */
    tigerInLifObj->pclEn            = (GT_U8)((*lifEntryPtr >> 7) & 0x01);
    /* PCL Profile */
    tigerInLifObj->pclProfile       = (GT_U8)((*lifEntryPtr >> 8) & 0x10);
    /* Force classify */
    tigerInLifObj->forcePcl         = (GT_U8)((*lifEntryPtr >> 12) & 0x01);
    /* mirror to analyser port */
    tigerInLifObj->mirrorToAnalyser = (GT_U8)((*lifEntryPtr >> 13) & 0x01);
    /* Enable/disable icmp for ipv6 */
    tigerInLifObj->ipv6IcmpEn       = (GT_U8)((*lifEntryPtr >> 14) & 0x01);
    /*  site ID for ipv6 packets */
    tigerInLifObj->ipv6Site         = (GT_U8)((*lifEntryPtr >> 15) & 0x01);
    /* Virtual Router ID */
    tigerInLifObj->vrtRoutId        = (GT_U16)((*lifEntryPtr >> 16) & 0xFFFF);

    /* word 1 */
    lifEntryPtr++;
    lifEntryPerPortPtr++;
    /* InLif number */
    tigerInLifObj->inLifNumber        = (GT_U16)((*lifEntryPtr) & 0xFFFF);
    /* SIP spoofing prevention enable */
    tigerInLifObj->sipSpoofPrevent    = (GT_U8)((*lifEntryPtr >> 16) & 0x01);
    /* enable/disable reserved SIP trap */
    tigerInLifObj->trapReservSip      = (GT_U8)((*lifEntryPtr >> 17) & 0x01);
    /* enable/disable restricted SIP trap */
    tigerInLifObj->trapRestrictedSip  = (GT_U8)((*lifEntryPtr >> 18) & 0x01);
    /* enable/disable IPv4 Multicast Local Scope */
    tigerInLifObj->ipv4McLclScopeEn   = (GT_U8)((*lifEntryPtr >> 20) & 0x01);
    /* enable/disable reserved DIP */
    tigerInLifObj->reservedDstIpv4En  = (GT_U8)((*lifEntryPtr >> 21) & 0x01);
    /* enable/disable restricted DIP */
    tigerInLifObj->restrictedDipEn    = (GT_U8)((*lifEntryPtr >> 22) & 0x01);
    /* PCL ID0 */
    tigerInLifObj->pclId0             = (GT_U8)((*lifEntryPtr >> 23) & 0x01);
    /* PCL ID1 */
    tigerInLifObj->pclId1             = (GT_U8)((*lifEntryPtr >> 24) & 0x01);
    /* enable/disable long pcl lookup0 */
    tigerInLifObj->pclLongLookupEn0   = (GT_U8)((*lifEntryPtr >> 25) & 0x01);
    /* IP subnet length */
    tigerInLifObj->srcIpSubnetLen     = (GT_U8)((*lifEntryPtr >> 26) & 0x1F);

    /* word 2 */
    lifEntryPtr++;
    lifEntryPerPortPtr++;
    /* Source_IP */
    tigerInLifObj->srcIp =  (*lifEntryPtr & 0xFF) << 24 |
                            ((*lifEntryPtr >> 8) & 0xFF) << 16 |
                            ((*lifEntryPtr >> 16) & 0xFF) << 8 |
                            ((*lifEntryPtr >> 24) & 0xFF);

    /* word 3 */
    lifEntryPtr++;
    lifEntryPerPortPtr++;
    /* pclId0 */
    tigerInLifObj->pclId0 |= (GT_U8)((*lifEntryPtr & 0x3f) << 1);
    /* pclId1 */
    tigerInLifObj->pclId1 |= (GT_U8)(((*lifEntryPtr >> 6) & 0x3f) << 1);
    /* enable igmp detect */
    tigerInLifObj->igmpDetectEn  = (GT_U8)((*lifEntryPtr >> 12) & 0x1);
    /* mirror to cpu */
    tigerInLifObj->mirrorToCpu   = (GT_U8)((*lifEntryPtr >> 13) & 0x1);
    /* default next hop */
    tigerInLifObj->defaultNextHop= (GT_U8)((*lifEntryPtr >> 14) & 0xf);
    /* enable/disable ipv6 management */
    tigerInLifObj->ipv6MgmtEn    = (GT_U8)((*lifEntryPtr >> 18) & 0x1);
    /* enable/disable ipv4 management */
    tigerInLifObj->ipv4MgmtEn    = (GT_U8)((*lifEntryPtr >> 19) & 0x1);
    /* enable/disable ripv1 broadcast mirroring */
    tigerInLifObj->ripV1En       = (GT_U8)((*lifEntryPtr >> 20) & 0x1);

    /* COS - dscp */
    tigerInLifObj->dscp          = (GT_U8)((*lifEntryPerPortPtr >> 22) & 0x3f);
    /* mark user priority */
    tigerInLifObj->markPortL3Cos = (GT_U8)((*lifEntryPerPortPtr >> 28) & 0x1);
    /* user priority */
    tigerInLifObj->UP            = (GT_U8)((*lifEntryPerPortPtr >> 29) & 0x7);

}

/*******************************************************************************
* snetTigerGetInLifAttributesHybrid
*
* DESCRIPTION:
*        get attributes of inlif from the per vlan entry.
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        perVlanInlifEntry - per port inlif entry.
*        tigerInLifObj     - inlif object pointer.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void snetTigerGetInLifAttributesHybrid
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    GT_U32 * perVlanInlifEntry,
    IN    GT_U32 * perPortInlifEntry,
    IN    TIGER_IN_LIF_OBJECT_STC * tigerInLifObj
)
{
    GT_U32 * lifEntryPerVlanPtr;
    GT_U32 * lifEntryPerPortPtr;

    /* word 0 */
    lifEntryPerVlanPtr = perVlanInlifEntry;
    lifEntryPerPortPtr = perPortInlifEntry;
    /* Disable/enable Bridge layer 2 */
    tigerInLifObj->bridgEn          = (GT_U8)(*lifEntryPerVlanPtr & 0x01);
    /* Disable/enable IPv4 UC */
    tigerInLifObj->ipV4En           = (GT_U8)((*lifEntryPerVlanPtr >> 3) & 0x01);
    /* Disable/enable IPv4 MC */
    tigerInLifObj->ipV4McEn         = (GT_U8)((*lifEntryPerVlanPtr >> 4) & 0x01);
    /* Disable/enable IPv6 UC */
    tigerInLifObj->ipV6En           = (GT_U8)((*lifEntryPerVlanPtr >> 5) & 0x01);
    /* Disable/enable IPv6 MC */
    tigerInLifObj->ipV6McEn         = (GT_U8)((*lifEntryPerVlanPtr >> 6) & 0x01);
    /* PCL enable */
    tigerInLifObj->pclEn            = (GT_U8)((*lifEntryPerPortPtr >> 7) & 0x01);
    /* PCL Profile */
    tigerInLifObj->pclProfile       = (GT_U8)((*lifEntryPerPortPtr >> 8) & 0x10);
    /* Force classify */
    tigerInLifObj->forcePcl         = (GT_U8)((*lifEntryPerPortPtr >> 12) & 0x01);
    /* mirror to analyser port */
    tigerInLifObj->mirrorToAnalyser = (GT_U8)((*lifEntryPerVlanPtr >> 13) & 0x01);
    /* Enable/disable icmp for ipv6 */
    tigerInLifObj->ipv6IcmpEn       = (GT_U8)((*lifEntryPerVlanPtr >> 14) & 0x01);
    /*  site ID for ipv6 packets */
    tigerInLifObj->ipv6Site         = (GT_U8)((*lifEntryPerVlanPtr >> 15) & 0x01);

    /* word 1 */
    lifEntryPerVlanPtr++;
    lifEntryPerPortPtr++;
    /* InLif number */
    tigerInLifObj->inLifNumber        = (GT_U16)((*lifEntryPerPortPtr) & 0xFFFF);
    /* SIP spoofing prevention enable */
    tigerInLifObj->sipSpoofPrevent    = (GT_U8)((*lifEntryPerVlanPtr >> 16) & 0x01);
    /* enable/disable reserved SIP trap */
    tigerInLifObj->trapReservSip      = (GT_U8)((*lifEntryPerVlanPtr >> 17) & 0x01);
    /* enable/disable restricted SIP trap */
    tigerInLifObj->trapRestrictedSip  = (GT_U8)((*lifEntryPerVlanPtr >> 18) & 0x01);
    /* enable/disable IPv4 Multicast Local Scope */
    tigerInLifObj->ipv4McLclScopeEn   = (GT_U8)((*lifEntryPerVlanPtr >> 20) & 0x01);
    /* enable/disable reserved DIP */
    tigerInLifObj->reservedDstIpv4En  = (GT_U8)((*lifEntryPerVlanPtr >> 21) & 0x01);
    /* enable/disable restricted DIP */
    tigerInLifObj->restrictedDipEn    = (GT_U8)((*lifEntryPerVlanPtr     >> 22) & 0x01);
    /* PCL ID0 */
    tigerInLifObj->pclId0             = (GT_U8)((*lifEntryPerPortPtr >> 23) & 0x01);
    /* PCL ID1 */
    tigerInLifObj->pclId1             = (GT_U8)((*lifEntryPerPortPtr >> 24) & 0x01);
    /* enable/disable long pcl lookup0 */
    tigerInLifObj->pclLongLookupEn0   = (GT_U8)((*lifEntryPerPortPtr >> 25) & 0x01);

    /* word 2 */
    lifEntryPerVlanPtr++;
    lifEntryPerPortPtr++;

    /* word 3 */
    lifEntryPerVlanPtr++;
    lifEntryPerPortPtr++;
    /* pclId0 */
    tigerInLifObj->pclId0       |= (GT_U8)((*lifEntryPerPortPtr & 0x3f) << 1);
    /* pclId1 */
    tigerInLifObj->pclId1       |= (GT_U8)(((*lifEntryPerPortPtr >> 6) & 0x3f) << 1);
    /* enable igmp detect */
    tigerInLifObj->igmpDetectEn  = (GT_U8)((*lifEntryPerVlanPtr >> 12) & 0x1);
    /* mirror to cpu */
    tigerInLifObj->mirrorToCpu   = (GT_U8)((*lifEntryPerVlanPtr >> 13) & 0x1);
    /* default next hop */
    tigerInLifObj->defaultNextHop= (GT_U8)((*lifEntryPerVlanPtr >> 14) & 0xf);
    /* enable/disable ipv6 management */
    tigerInLifObj->ipv6MgmtEn    = (GT_U8)((*lifEntryPerVlanPtr >> 18) & 0x1);
    /* enable/disable ipv4 management */
    tigerInLifObj->ipv4MgmtEn    = (GT_U8)((*lifEntryPerVlanPtr >> 19) & 0x1);
    /* enable/disable ripv1 broadcast mirroring */
    tigerInLifObj->ripV1En       = (GT_U8)((*lifEntryPerVlanPtr >> 20) & 0x1);
    /* mark port l3 cos */
    tigerInLifObj->markPortL3Cos = (GT_U8)((*lifEntryPerPortPtr >> 21) & 0x1);
    /* dscp */
    tigerInLifObj->dscp          = (GT_U8)((*lifEntryPerPortPtr >> 22) & 0x3f);
    /* mark user priority */
    tigerInLifObj->markPortL3Cos = (GT_U8)((*lifEntryPerPortPtr >> 28) & 0x1);
    /* user priority */
    tigerInLifObj->UP            = (GT_U8)((*lifEntryPerPortPtr >> 29) & 0x7);
}

/*******************************************************************************
* snetTigerGetInLifAttributesPerVlanReduced
*
* DESCRIPTION:
*        Port to logical interface
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        perVlanInlifEntry - per port inlif entry.
*        perPortInlifEntry = per port inlif entry.
*        tigerInLifObj     - inlif object pointer.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void snetTigerGetInLifAttributesPerVlanReduced
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    GT_U32 * perVlanInlifEntry,
    IN    GT_U32 * perPortInlifEntry,
    IN    TIGER_IN_LIF_OBJECT_STC * tigerInLifObj
)
{
    GT_U32 * lifEntryPtr;
    GT_U32 * lifEntryPerPortPtr;

    /* word 0 for per reduced vlan*/
    lifEntryPtr         = perVlanInlifEntry;
    /* Disable/enable Bridge layer 2 */
    tigerInLifObj->bridgEn          = (GT_U8)(*lifEntryPtr & 0x01);
    /* Disable/enable IPv4 UC */
    tigerInLifObj->ipV4En           = (GT_U8)((*lifEntryPtr >> 3) & 0x01);
    /* Disable/enable IPv4 MC */
    tigerInLifObj->ipV4McEn         = (GT_U8)((*lifEntryPtr >> 4) & 0x01);
    /* Disable/enable IPv6 UC */
    tigerInLifObj->ipV6En           = (GT_U8)((*lifEntryPtr >> 5) & 0x01);
    /* Disable/enable IPv6 MC */
    tigerInLifObj->ipV6McEn         = (GT_U8)((*lifEntryPtr >> 6) & 0x01);
    /* PCL enable */
    tigerInLifObj->pclEn            = (GT_U8)((*lifEntryPtr >> 7) & 0x01);
    /* PCL Profile */
    tigerInLifObj->pclProfile       = (GT_U8)((*lifEntryPtr >> 8) & 0x10);
    /* Force classify */
    tigerInLifObj->forcePcl         = (GT_U8)((*lifEntryPtr >> 12) & 0x01);
    /* mirror to analyser port */
    tigerInLifObj->mirrorToAnalyser = (GT_U8)((*lifEntryPtr >> 13) & 0x01);
    /* Enable/disable icmp for ipv6 */
    tigerInLifObj->ipv6IcmpEn       = (GT_U8)((*lifEntryPtr >> 14) & 0x01);
    /*  site ID for ipv6 packets */
    tigerInLifObj->ipv6Site         = (GT_U8)((*lifEntryPtr >> 15) & 0x01);
    /* pclId0 */
    tigerInLifObj->pclId0           = (GT_U8)(((*lifEntryPtr >> 16) & 0x3f) << 1);
    /* pclId1 */
    tigerInLifObj->pclId1           = (GT_U8)(((*lifEntryPtr >> 22) & 0x3f) << 1);
    /* enable igmp detect */
    tigerInLifObj->igmpDetectEn     = (GT_U8)((*lifEntryPtr >> 28) & 0x1);
    /* mirror to cpu */
    tigerInLifObj->mirrorToCpu      = (GT_U8)((*lifEntryPtr >> 29) & 0x1);
    /* default next hop */
    tigerInLifObj->defaultNextHop   = (GT_U8)((*lifEntryPtr >> 30) & 0x2);


    /* word 1 */
    lifEntryPtr++;
    /* default next hop */
    tigerInLifObj->defaultNextHop   |= (GT_U8)((*lifEntryPtr & 0x1) << 3);
    /* enable/disable ipv6 management */
    tigerInLifObj->ipv6MgmtEn        = (GT_U8)((*lifEntryPtr >> 1) & 0x1);
    /* enable/disable ipv4 management */
    tigerInLifObj->ipv4MgmtEn        = (GT_U8)((*lifEntryPtr >> 2) & 0x1);
    /* enable/disable ripv1 broadcast mirroring */
    tigerInLifObj->ripV1En           = (GT_U8)((*lifEntryPtr >> 3) & 0x1);
    /* pclId0 */
    tigerInLifObj->pclId0            |= (GT_U8)((*lifEntryPtr >> 4) & 0x1);
    /* pclId1 */
    tigerInLifObj->pclId1            |= (GT_U8)((*lifEntryPtr >> 5) & 0x1);
    /* pcl lookup 0  */
    tigerInLifObj->pclLongLookupEn0  |= (GT_U8)((*lifEntryPtr >> 6) & 0x1);
    /* pcl lookup 1  */
    tigerInLifObj->pclLongLookupEn1  |= (GT_U8)((*lifEntryPtr >> 7) & 0x1);

    /* word 0 for per port entry */
    lifEntryPerPortPtr  = perPortInlifEntry;
    /* word 1 for per port entry */
    lifEntryPerPortPtr++;
    /* InLif number */
    tigerInLifObj->inLifNumber       = (GT_U16)((*lifEntryPerPortPtr) & 0xFFFF);
    /* SIP spoofing prevention enable */
    tigerInLifObj->sipSpoofPrevent    = (GT_U8)((*lifEntryPerPortPtr >> 16) & 0x01);
    /* enable/disable reserved SIP trap */
    tigerInLifObj->trapReservSip      = (GT_U8)((*lifEntryPerPortPtr >> 17) & 0x01);
    /* enable/disable restricted SIP trap */
    tigerInLifObj->trapRestrictedSip  = (GT_U8)((*lifEntryPerPortPtr >> 18) & 0x01);
    /* enable/disable IPv4 Multicast Local Scope */
    tigerInLifObj->ipv4McLclScopeEn   = (GT_U8)((*lifEntryPerPortPtr >> 20) & 0x01);
    /* enable/disable reserved DIP */
    tigerInLifObj->reservedDstIpv4En  = (GT_U8)((*lifEntryPerPortPtr >> 21) & 0x01);
    /* enable/disable restricted DIP */
    tigerInLifObj->restrictedDipEn    = (GT_U8)((*lifEntryPerPortPtr >> 22) & 0x01);
}


/*******************************************************************************
*   snetTigerInLifAssignment
*
* DESCRIPTION:
*        input logical interface to table entry
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        perPortInlifEntryPtr_Ptr - pointer to per port inlif entry .
*        perVlanInlifEntryPtr_Ptr - pointer to per vlan inlif entry .
*
*******************************************************************************/
static void snetTigerInLifAssignment
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT   GT_U32 ** perPortInlifEntryPtr_Ptr,
    OUT   GT_U32 ** perVlanInlifEntryPtr_Ptr
)
{
    DECLARE_FUNC_NAME(snetTigerInLifAssignment);

    GT_U32 regAddress;
    GT_U32 vlanLineOffsetTbl;
    GT_U32 perVlanInLifTblOffSet;
    SKERNEL_TIGER_INLIF_PER_VLAN_TYPE_ENT perVlanType;
    GT_U32 modeVidOffSet;
    GT_U32 perVlanOffsetTbl;

    /* Set init values */
    *perPortInlifEntryPtr_Ptr = 0;
    *perVlanInlifEntryPtr_Ptr = 0;

    /***  PER PORT ASSIGNMENT                                      ***/
    /* input logical interface Table Register for per port assignment*/
    regAddress = SMEM_TIGER_PER_PORT_INLIF_TABLE_OFFSET +
                        descrPtr->srcPort * 4 * 4;

    *perPortInlifEntryPtr_Ptr = smemMemGet(devObjPtr, regAddress);



    if(descrPtr->inLifClassify != SKERNEL_TIGER_INLIF_PER_VLAN_ENTRY_E &&
       descrPtr->inLifClassify != SKERNEL_TIGER_INLIF_PER_VLAN_PORT_HYBRID_ENRTY_E)
    {
        return;
    }

        /***  PER VLAN ASSIGNMENT                                      ***/
    /* input logical interface Table Register for per vlan assignment*/
    regAddress = SMEM_TIGER_INLIF_CONTROL_REG    ;
    smemRegFldGet(devObjPtr, regAddress , 0, 27, (void *)&perVlanType);


    if (perVlanType == SKERNEL_TIGER_INLIF_PER_VLAN_FULL_E)
    {
        /* find the index of the vlan entry in memory */
        __LOG(("find the index of the vlan entry in memory"));
        regAddress = SMEM_TIGER_PER_VLAN_INLIF_MAPPING_TABLE_OFFSET +
                        4 * (descrPtr->vid /2) ;
        modeVidOffSet = (descrPtr->vid % 2)*12 + 1*((descrPtr->vid % 2)==0 ? 0 : 1);
        smemRegFldGet(devObjPtr, regAddress , modeVidOffSet, 12, &vlanLineOffsetTbl);


        /* find the start of the vlan table in the control memory offset */
        regAddress = SMEM_TIGER_PER_VLAN_INLIF_TABLE_OFFSET_REG ;
        smemRegFldGet(devObjPtr, regAddress , 0, 13, &perVlanInLifTblOffSet);
        perVlanOffsetTbl = vlanLineOffsetTbl + (perVlanInLifTblOffSet * 16);


        /* add the start of the control memory register */
        perVlanOffsetTbl = perVlanOffsetTbl + SMEM_TIGER_CONTROL_MEMORY_OFFSET_REG;
    }
    else /* inlif per vlan reduced mode */
    {
        /* find the index of the vlan entry in memory */
        __LOG(("find the index of the vlan entry in memory"));
        regAddress = SMEM_TIGER_PER_VLAN_INLIF_MAPPING_TABLE_OFFSET +
                        4 * (descrPtr->vid /2) ;
        modeVidOffSet = (descrPtr->vid % 2)*12 + 1*((descrPtr->vid % 2)==0 ? 0 : 1);
        smemRegFldGet(devObjPtr, regAddress , 0, 10, &perVlanOffsetTbl);


        /* find the start of the vlan table in the control memory offset */
        regAddress = SMEM_TIGER_PER_VLAN_INLIF_TABLE_OFFSET_REG ;
        smemRegFldGet(devObjPtr, regAddress , 0, 12, &perVlanInLifTblOffSet);
        perVlanOffsetTbl = perVlanOffsetTbl + (perVlanInLifTblOffSet * 16);


        /* add the start of the control memory register */
        perVlanOffsetTbl = perVlanOffsetTbl + SMEM_TIGER_CONTROL_MEMORY_OFFSET_REG;
    }

    *perVlanInlifEntryPtr_Ptr = smemMemGet(devObjPtr, perVlanOffsetTbl);
}

/*******************************************************************************
*   snetTigerInLifAnalyzeAttributes
*
* DESCRIPTION:
*         analyze the attributes of the inlif entry .
* INPUTS:
*        devObjPtr      - pointer to device object.
*        descrPtr       - pointer to the frame's descriptor.
*        tigerInLifObj  - inlif object pointer.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*         i have made some changes in the frame descriptor without confirm
*         that this is the tiger descriptor . MSIL didn't supply any further
*         details for their tiger frame descriptor.
*******************************************************************************/
static void snetTigerInLifAnalyzeAttributes
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    TIGER_IN_LIF_OBJECT_STC * tigerInLifObj
)
{
    DECLARE_FUNC_NAME(snetTigerInLifAnalyzeAttributes);

    GT_U8 isIPM = 0;

    if (descrPtr->pktCmd == SKERNEL_PKT_DROP_E)
    {
      return ;
    }
    if (tigerInLifObj->bridgEn & 0x01)
    {
        descrPtr->doRout = 0;
        descrPtr->doClassify = 0;
        descrPtr->doPcl = 0;
        descrPtr->pktCmd = SKERNEL_PKT_FORWARD_E;
        descrPtr->vlanCmd = SKERNEL_NOT_CHANGE_E;
        descrPtr->useVidx = 0;
        descrPtr->bits15_2.useVidx_0.targedDevice = descrPtr->srcDevice;
        descrPtr->bits15_2.useVidx_0.targedPort = 61;
    }
    if (descrPtr->ipHeaderPtr != NULL)
    {
        isIPM = IS_DST_MCAST(descrPtr);
    }
    if (isIPM == 1)
    {
        /* En_IPv4_MC */
        __LOG(("En_IPv4_MC"));
        descrPtr->doIpmRout = tigerInLifObj->ipV4McEn | tigerInLifObj->ipV6McEn;
    }
    else
    if (descrPtr->doRout == 1)
    {
        /* En_IPv4_IPV6 */
        __LOG(("En_IPv4_IPV6"));
        descrPtr->doRout = tigerInLifObj->ipV4En | tigerInLifObj->ipV6En ;
    }

    /* En_Pcl */
    __LOG(("En_Pcl"));
    if (( tigerInLifObj->forcePcl != 0 ) || ( tigerInLifObj->pclEn != 0 ))
    {
        descrPtr->doPcl = 1;
        descrPtr->doClassify = 1;
    }
    else
    {
        descrPtr->doPcl = 0;
        descrPtr->doClassify = 0;
    }
    /* VR-Id The Virtual Router ID */
    descrPtr->vrId = tigerInLifObj->vrtRoutId;
    /* InLIF# */
    descrPtr->inLifNumber = tigerInLifObj->inLifNumber ;
    /* PCL_ID_#0 */
    descrPtr->pclId0 = tigerInLifObj->pclId0;
    /* PCL_ID_#1 */
    descrPtr->pclId1 = tigerInLifObj->pclId1;
    /* Trap_MC_Local_Scope */
    descrPtr->trapMcLocalScope = tigerInLifObj->ipv4McLclScopeEn;
    /* Trap_Reserved_SIP - tiger only */
    descrPtr->trapReservedSip  = tigerInLifObj->trapReservSip ;
    /* Trap_Resetricted_SIP - tiger only */
    descrPtr->trapRestrictedSip= tigerInLifObj->trapRestrictedSip ;
    /* Enable_MC_Local_Scope - tiger only */
    descrPtr->mcLocalScopeEn   = tigerInLifObj->ipv4McLclScopeEn;
    /* Enable_RESERVED_DIP - tiger only */
    descrPtr->enReservedDip    = tigerInLifObj->reservedDstIpv4En;
    /* Enable_Resetricted_SIP - tiger only */
    descrPtr->enRestrictedDip  = tigerInLifObj->restrictedDipEn;
    /* PCL_PROFILE - tiger only */
    descrPtr->pclProfile       = tigerInLifObj->pclProfile;
    /* DO_Mirror - tiger only */
    descrPtr->rxSniffed        = tigerInLifObj->mirrorToAnalyser ;
    /* IPV6_Site - tiger only */
    descrPtr->ipV6site         = tigerInLifObj->ipv6Site ;
    /* IPV6_ICMP - tiger only */
    descrPtr->ipv6Icmp         = tigerInLifObj->ipv6IcmpEn;
    /* PCL_Long_Lkup#0 - tiger only */
    descrPtr->pclLongLkup0     = tigerInLifObj->pclLongLookupEn0 ;
    /* PCL_Long_Lkup#1 - tiger only */
    descrPtr->pclLongLkup1     = tigerInLifObj->pclLongLookupEn1 ;
    /* MC_Lcl_Scpe_EN - tiger only */
    descrPtr->mcLocalScopeEn  =  tigerInLifObj->ipv4McLclScopeEn ;
    /* Igmp_Enable - tiger only */
    descrPtr->igmpDetect      =  tigerInLifObj->igmpDetectEn ;
    /* Mirror_CPU - tiger only */
    descrPtr->doMirrorToCpu   =  tigerInLifObj->mirrorToCpu ;
    /* Def_Next_HOP - tiger only */
    descrPtr->defNextHop      =  tigerInLifObj->defaultNextHop ;
    /* IPv4_MGMT - tiger only */
    descrPtr->ipV4Mgmt        =  tigerInLifObj->ipv4MgmtEn ;
    /* IPv6_MGMT - tiger only */
    descrPtr->ipV6Mgmt        =  tigerInLifObj->ipv6MgmtEn ;
    /* RipV1 - tiger only */
    descrPtr->ipv4Ripv1       =  tigerInLifObj->ripV1En  ;
    /***********************************************************
     *** the following fields are handled in the ip security ***
     *** function and in the class of service maintenance    ***
     *** sipSpoofPrevent ,                                   ***
     *** trapReservSip ,                                     ***
     *** trapRestrictedSip                                   ***
     *** markPortL3Cos ,                                     ***
     *** dscp ,                                              ***
     *** UP .                                                ***
     ***********************************************************/

    return ;
}


/*******************************************************************************
*   snetTigerIpSecurity
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
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void snetTigerIpSecurity
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    TIGER_IN_LIF_OBJECT_STC * inlifEntryPtr
)
{
    DECLARE_FUNC_NAME(snetTigerIpSecurity);

    GT_U32 ipAddrMask;
    GT_U32 ipSIP;
    GT_U32 ipSIPSubnet;
    GT_U32 ipAddr;
    GT_U32 regAddress;
    GT_U32 fldValue;

    if (descrPtr->ipHeaderPtr ==  NULL)
    {
        return;
    }
    if (descrPtr->pktCmd == SKERNEL_PKT_DROP_E && inlifEntryPtr->bridgEn == 0)
    {
        return;
    }

    if (inlifEntryPtr->sipSpoofPrevent == 1)
    {
        ipAddrMask = ~(0xffffffff >> inlifEntryPtr->srcIpSubnetLen);
        ipSIPSubnet = inlifEntryPtr->srcIp & ipAddrMask;
        ipSIP = descrPtr->ipv4SIP &  ipAddrMask;
        if (ipSIP != ipSIPSubnet)
        {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode = TWIST_CPU_CODE_SPOOF_SIP_TRAP;
            return;
        }
    }

    if (inlifEntryPtr->trapReservSip == 1)
    {
        ipAddrMask = ~(0xffffffff >> 8);
        ipAddr = descrPtr->ipv4SIP & ipAddrMask;
        if (ipAddr == 10)
        {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                                            TWIST_CPU_CODE_RESRVED_SIP_TRAP;
            return;
        }

        ipAddrMask = ~(0xffffffff >> 12);
        ipAddr = descrPtr->ipv4SIP & ipAddrMask;
        if (((ipAddr >> 24) == 172) && (((ipAddr >> 16) & 0x0F) == 16))
        {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                                            TWIST_CPU_CODE_RESRVED_SIP_TRAP;
            return;
        }

        ipAddrMask = ~(0xffffffff >> 16);
        ipAddr = descrPtr->ipv4SIP & ipAddrMask;
        if (((ipAddr >> 24) == 196) && (((ipAddr >> 16) & 0x0F) == 168))
        {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                                            TWIST_CPU_CODE_RESRVED_SIP_TRAP;
            return;
        }

        ipAddrMask = 0xffffffff;
        ipAddr = descrPtr->ipv4SIP & ipAddrMask;
        if (ipAddr == 0)
        {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                                            TWIST_CPU_CODE_RESRVED_SIP_TRAP;
            return;
        }
    }

    if (inlifEntryPtr->trapRestrictedSip == 1)
    {
        /* Subnet Mask Size Register 0 */
        __LOG(("Subnet Mask Size Register 0"));
        regAddress = 0x02800014;
        smemRegFldGet(devObjPtr, regAddress, 16, 5, &fldValue);
        if (fldValue < 31)
        {
            ipAddrMask = ~(0xffffffff >> fldValue);
        }
        else
        {
            ipAddrMask = 0xffffffff;
        }

        regAddress = 0x0280000C;
        smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);     /* SIP<0> */
        ipAddr = fldValue;
        if ((descrPtr->ipv4SIP & ipAddrMask) == ipAddr)
        {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                                           TWIST_CPU_CODE_INTERNAL_SIP_TRAP;
            return;
        }

        /* Subnet Mask Size Register 1 */
        regAddress = 0x02800014;
        smemRegFldGet(devObjPtr, regAddress, 24, 5, &fldValue);
        if (fldValue < 31)
        {
            ipAddrMask = ~(0xffffffff >> fldValue);
        }
        else
        {
            ipAddrMask = 0xffffffff;
        }

        regAddress = 0x02800010;
        smemRegFldGet(devObjPtr, regAddress, 0, 32, &fldValue);     /* SIP<1> */
        ipAddr = fldValue;
        if ((descrPtr->ipv4SIP & ipAddrMask) == ipAddr)
        {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                                            TWIST_CPU_CODE_INTERNAL_SIP_TRAP;
            return;
        }
    }
}

/*******************************************************************************
*   snetTigerCos
*
* DESCRIPTION:
*        COS of input LIF
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*                 inlifEntryAddr - input lif entry address.
*        inlifEntryPtr
*                   - pointer to InLif table entry
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void snetTigerCos
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    TIGER_IN_LIF_OBJECT_STC * inlifEntryPtr
)
{
    if (inlifEntryPtr->markPortL3Cos  == 1)
    {
        if (descrPtr->frameType == SKERNEL_FRAME_TYPE_IPV4_E)
        {
             descrPtr->dscp = inlifEntryPtr->dscp ;
        }
        else if (descrPtr->frameType == SKERNEL_FRAME_TYPE_IPV6_E)
        {
             descrPtr->trafficClass = inlifEntryPtr->dscp;
        }
        if (inlifEntryPtr->markUP)
        {
            descrPtr->userPriorityTag = inlifEntryPtr->UP;
        }
    }
}
