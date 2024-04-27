/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetTwistInLif.h
*
* DESCRIPTION:
*       This is a external API definition for Snet InLif module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 5 $
*
*******************************************************************************/
#ifndef __snetTwistInLifh
#define __snetTwistInLifh


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
 *      mplsEn          : Enable/disable MPLS switching on this InLif.
 *      mplsMcEn:       : Enable/disable multicast MPLS switching on this InLif.
 *      ipV4En          : Enable/disable IPv4 Routing on this InLif.
 *      ipV4McEn        : Enable/disable IPv4 multicast Routing on this InLif.
 *      ipV6En          : Enable/disable IPv6 Routing on this InLif.
 *      ipV6McEn        : Enable/disable IPv6 multicast Routing on this InLif.
 *      classifyEn:     : Enable/disable  multifield classification on this InLif.
 *      majorTemplate   : Major bit of the Generic Flow Template. Determines
 *                        the criteria for performing flow classification on
 *                        InLif
 *
 *      forceClassify   : Automatically trigger flow classification for packets
 *                        received on this interface.
 *                        Meaningful only when <En_Classify> is set.
 *
 *      cibEn:          : Enable sending the packet to the CIB if Ethernet
 *                        bridging ingress engine asked for that or <Force_EPI>
 *                        is set.
 *
 *      forceCib:       : Force all packets to go to the CIB interface when
 *                         received on this InLIF
 *
 *      doNat           : When the bit is set the NAT will be executed on
 *                        his packet
 *
 *      pclEn           : Enable/disable  pcl engine on this InLif.
 *
 *      forcePcl        : Automatically trigger pcl for packets received on this
 *                        interface.
 *                        Meaningful only when <En_Pcl> is set.
 *
 *      vrtRoutId       : The Virtual Router ID to assign to packets received
 *                        on this interface.
 *                        When Virtual Routing is used, this field will set
 *                        which IP forwarding table to use for forwarding this
 *                        packet.
 *
 *      InLifNumber     : Input Logical Interface Number
 *      sipSpoofPrevent : Prevent Source IP Spoofing by trapping to the CPU.
 *      trapReservSip   : Whether to trap packet received on this interface
 *                        with Source IP in the following subnets:
 *                         10/8
 *                         172.16/12
 *                         196.168/16
 *                         0/32
 *
 *      trapInternalSip : Trap IP packets that have a source IP used by IP
 *                         addresses in the internal network
 *
 *      trapMcLclScope  : Trap IP Multicast packet with Group ID from 239/24
 *      trapRsrvDstIp   : Whether to trap packets received on this interface
 *                        with Destination IP in the following subnets:
 *                         10/8
 *                         172.16/12
 *                         196.168/16
 *                         0/32
 *
 *      trapInternDstIp : Trap IP packets that have a Destination IP used by
 *                        IP addresses in the internal network
 *
 *      srcIpSubnetLen  : The prefix length of the subnet presented by
 *                         <Source_IP> field.
 *
 *      srcIp           : The Source IP of this interface.
 *      pciBaseAddr     : The first address for the PCL lookup.
 *      pciMaxHop       : Maximum number of lookups in the PCL.
 *      pclNumber       : Added to the key in the PCL (for virtual PCLS).
 *      pclId0          : PCL ID used for first lookup (as part of the key)-Samba
 *      pclId1          : PCL ID used for second lookup(as part of the key)-Samba
 */

typedef struct {
    GT_U8               bridgEn;
    GT_U8               mplsEn;
    GT_U8               mplsMcEn;
    GT_U8               ipV4En;
    GT_U8               ipV4McEn;
    GT_U8               ipV6En;
    GT_U8               ipV6McEn;
    GT_U8               classifyEn;
    GT_U8               majorTemplate;
    GT_U8               forceClassify;
    GT_U8               cibEn;
    GT_U8               forceCib;
    GT_U8               doNat;
    GT_U8               pclEn;         /* Samba only */
    GT_U8               forcePcl;      /* Samba only */
    GT_U16              vrtRoutId;
    GT_U16              inLifNumber;
    GT_U8               sipSpoofPrevent;
    GT_U8               trapReservSip;
    GT_U8               trapInternalSip;
    GT_U8               trapMcLclScope;
    GT_U8               trapRsrvDstIp;
    GT_U8               trapInternDstIp;
    GT_U16              srcIpSubnetLen;
    GT_U32              srcIp;
    GT_U16              pclBaseAddr;   /* twistD only */
    GT_U16              pclMaxHop;     /* twistD only */
    GT_U16              pclNumber;     /* twistD only */
    GT_U8               pclId0;        /* Samba only */
    GT_U8               pclId1;        /* Samba only */
} IN_LIF_OBJECT_STC;

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
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetTwistInLifh */
