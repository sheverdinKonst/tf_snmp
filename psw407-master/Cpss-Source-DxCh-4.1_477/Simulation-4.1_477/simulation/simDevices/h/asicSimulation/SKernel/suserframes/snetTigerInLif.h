/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetTigerInLif.h
*
* DESCRIPTION:
*       This is a external API definition for Snet InLif module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*
*******************************************************************************/
#ifndef __snetTigerInLifh
#define __snetTigerInLifh


#include <asicSimulation/SKernel/smain/smain.h>
#include <common/Utils/FrameInfo/sframeInfoAddr.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Typedef: struct SKERNEL_DEVICE_OBJECT
 *
 * Description:
 *      Describe a Input Logical Interface in the simulation.
 *
 * Fields:
 *      bridgEn         : Enable/disable Ethernet bridging on this InLif
 *      ipV4En          : Enable/disable IPv4 unicast Routing on this InLif.
 *      ipV4McEn        : Enable/disable IPv4 multicast Routing on this InLif.
 *      ipV6En          : Enable/disable IPv6 unicast Routing on this InLif.
 *      ipV6McEn        : Enable/disable IPv6 multicast Routing on this InLif.
 *      classifyEn:     : Enable/disable  multifield classification on this InLif.
 *      pclEn           : Enable/disable  activation of pcl on this InLif.
 *
 *      forcePcl        : Automatically trigger pcl for packets received on this
 *                        interface.
 *                        Meaningful only when <En_Pcl> is set.
 *      InLifNumber     : Input Logical Interface Number
 *      sipSpoofPrevent : Prevent Source IP Spoofing by trapping to the CPU.
 *      trapReservSip   : Whether to trap packet received on this interface
 *                        with Source IP in the following subnets:
 *                         10/8
 *                         172.16/12
 *                         196.168/16
 *                         0/32
 *
 *      trapRestrictedSip : IP packets that have a source IP matching a globally
 *                          configured restricted SIP subnets trap to cpu.
 *
 *      ipv4McLclScopeEn  :  IP Multicast packet with Group ID matchinf the
 *                           MC local scope are handled by router engine.
 *
 *      reservedDstIpv4En : Whether to trap packets received on this interface
 *                          with Destination IP in the following subnets:
 *                          10/8
 *                          172.16/12
 *                          196.168/16
 *                          0/32
 *
 *      restrictedDipEn : Trap IP packets that have a Destination IP used by
 *                        IP addresses in the internal network
 *
 *      srcIpSubnetLen  : The prefix length of the subnet presented by
 *                         <Source_IP> field.
 *
 *      srcIp           : The Source IP of this interface.
 *
 *      pclId0          : PCL ID used for first lookup (as part of the key)-
 *      pclId1          : PCL ID used for second lookup(as part of the key)-
 *      pclProfile      : PCL profile used to select the template for the
 *                        USER DEFINED fields extraction
 *
 *      mirrorToAnalyser: Set the promiscuous mode for packet inspection on an
 *                        IDS device or a port analyzer
 *      ipv6IcmpEn      : packet is handled according to the command associated
 *                        with the ICMPv6 type
 *      ipv6Site        : determine whether the InLif reside on an external or
 *                      : internal site
 *      pclLongLookupEn0: Enable the first PCL lookup to be a long rule (48KB)
 *      pclLongLookupEn1: Enable the second PCL lookup to be a long rule (48KB)
 *
 *      igmpDetectEn    : Enable the detection of igmp
 *
 *      ipv4MgmtEn      : Enable IpV4 management
 *      ipv6MgmtEn      : Enable IpV6 management
 *
 *      ripV1En         : RIPv1 and IPv4 over MAC broadcast .
 *      markPortL3Cos   : used to set the DSCP for IPv4 packets or TC for IPv6
 *      dscp            : the dscp value
 *      markUP          : used to set the UP for all packets associated with inlif
 *      UP              : relevant only if markPortL3Cos is set.
 */

typedef struct {
    GT_U8               bridgEn;
    GT_U8               ipV4En;
    GT_U8               ipV4McEn;
    GT_U8               ipV6En;
    GT_U8               ipV6McEn;
    GT_U8               pclEn;
    GT_U8               forcePcl;
    GT_U16              vrtRoutId;
    GT_U16              inLifNumber;
    GT_U8               sipSpoofPrevent;
    GT_U8               trapReservSip;
    GT_U8               trapRestrictedSip;
    GT_U8               ipv4McLclScopeEn;
    GT_U8               reservedDstIpv4En;
    GT_U8               restrictedDipEn;
    GT_U16              srcIpSubnetLen;
    GT_U32              srcIp;
    GT_U8               pclId0;
    GT_U8               pclId1;
    GT_U8               pclProfile;
    GT_U8               mirrorToAnalyser;
    GT_U8               ipv6IcmpEn;
    GT_U8               ipv6Site;
    GT_U8               pclLongLookupEn0;
    GT_U8               pclLongLookupEn1;
    GT_U8               igmpDetectEn;
    GT_U8               mirrorToCpu;
    GT_U8               defaultNextHop;
    GT_U8               ipv6MgmtEn;
    GT_U8               ipv4MgmtEn;
    GT_U8               ripV1En;
    GT_U8               markPortL3Cos;
    GT_U8               dscp;
    GT_U8               markUP;
    GT_U8               UP;
} TIGER_IN_LIF_OBJECT_STC;

/*******************************************************************************
*   snetTwistInLifProcess
*
* DESCRIPTION:
*        InLif processing
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
GT_BOOL snetTigerInLifProcess
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetTwistInLifh */
