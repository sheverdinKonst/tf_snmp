/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smainSwitchFabric.c
*
* DESCRIPTION:
*       This is a external API definition for sSwitchFabrics module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 7 $
*
*******************************************************************************/
#ifndef __smainSwitchFabrich
#define __smainSwitchFabrich

#include <asicSimulation/SKernel/smain/smain.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* maximum number of devices in dune simulated system */
#define SMAIN_SWITCH_FABRIC_MAX_NUM_OF_DEVICES_CNS      0xFF

/* maximum number of links in dune simulated system */
#define SMAIN_SWITCH_FABRIC_MAX_NUM_OF_LINKS_CNS        0xFF

/* 1KB */
#define SMAIN_SWITCH_FABRIC_1KB                         0x400

/* Reachability time interval */
#define SMAIN_SWITCH_FABRIC_RM_TIME_INTERVAL_CNS        0x40

/* arbitrary definition for time interval */
#define SMAIN_SWITCH_FABRIC_CHIP_CLOCK_CNS             0xc8 * \
                                                        SMAIN_SWITCH_FABRIC_RM_TIME_INTERVAL_CNS
typedef enum
{
    SKERNEL_UPLINK_FRAME_TYPE_NONE_E,    /* no uplink exists                  */
    SKERNEL_UPLINK_FRAME_PP_TYPE_E,      /* data type is frame                */
    SKERNEL_UPLINK_FRAME_FA_TYPE_E,      /* data type is FA           */
    SKERNEL_UPLINK_FRAME_DUNE_RM_E       /* dune reachability message */
}SKERNEL_UPLINK_FRAME_TYPE_ENT;

/*
 * Typedef: struct SKERNEL_UPLINK_DESCR_STC
 *
 * Description:
 *      Describe a descriptor of frame.
 *
 * Fields:
 *      frameBuf         : Buffer's id.
 *      byteCount        : Length of the frame L2 included.
 *      frameType        : Type of the frame one of SKERNEL_FRAME_TYPE
 *      srcTrunkId       : Trunk ID of the source port.
 *                       : 0 = The packet was received from a port that is
 *                       :     not a trunk member.
 *                       : 1 through 63 = The port is member of a trunk group
 *                       :
 *      srcPort          : Source port number on which the packet was received.
 *                       : Port 63 is reserved for packets sent from the CPU.
 *                       : Port 62 is reserved for packets sent by the Customer
 *                          Interface Bus (CIB).
 *                       :
 *      uplink           : 0 = The packet was allocated a buffer from the port
 *                       :     buffers pool.
 *                       : 1 = The packet was allocated a buffer from the
 *                       :     Fabric/Stacking Adapter buffers pool.
 *                       :
 *                       :
 *      vid              : The VLAN-ID to which the packet is classified.
 *                       : This VLAN-ID is used for the Egress filtering and
 *                       : this is the VID used if the packet is trans-mitted
 *                       : with an 802.1Q tag.
 *                       :
 *      trafficClass     : The packet's traffic class.
 *                       :
 *      pktCmd           : 0 = Forward packet to destination/s according
 *                       :      to the data below
 *                       : 1 = Drop packet
 *                       : 2 = Trap to CPU
 *                       : 3 = Reserved
 *                       :
 *                       :
 *      srcDevice        : The packet's source device number.
 *                       :
 *      ingressFport     : ingress fabric port
 *      srcData          : source port.
 *      partnerDeviceID  : in the case of Dune System , this is the ID of
 *                         packet processor connected to the FAP.
 * Comments:
 */
typedef struct
{
    SBUF_BUF_ID                 frameBuf;
    GT_U16                      byteCount;
    GT_U16                      frameType;
    GT_U16                      srcTrunkId;
    GT_U32                      srcPort;
    GT_U32                      uplink;
    GT_U16                      vid;
    GT_U8                       trafficClass;
    GT_U8                       pktCmd;
    GT_U8                       srcDevice;
    GT_U32                      ingressFport;
    GT_U32                      srcData;
    GT_U32                      srcVlanTagged;
    GT_U32                      partnerDeviceID;
}SKERNEL_UPLINK_PP_SOURCE_DESCR_STC;

/*
 * Typedef: struct SKERNEL_UPLINK_PP_VIDX_UNT
 *
 * Description:
 *      Describe vidx parameter
 *
 * Fields:
 *      vidx         : The vidx group.When <Use_VIDX> = 1 .
 *      target port  : unicast target port.
 *      targetDevice : unicast target device.
 *      lbh          : load balancing hash mode.
 * Comments:
 */
typedef  union
{
    GT_U32              vidx;
    GT_U32              reserved1;
    GT_U32              reserved2;
    struct
    {
      GT_U32            targetPort;
      GT_U32            targetDevice;
      GT_U32            lbh;
    }targetInfo;
}SKERNEL_UPLINK_PP_VIDX_UNT;


/*
 *      macDaType        : data type of destination MAC address (unicast or Mc).
 *      useVidx          : Indicates whether this packet should be forwarded as
 *                       : a Unicast packet or as a Multi-destina-tion (Multicast)
 *                       : packet and whether it should be forwarded to the
 *                       : Multicast group pointed to by <VIDX>.
 *                       : 0 = The packet is Unicast.
 *                       : 1 = Use Vidx for forwarding the packet
 *                       :     (Multi-destination packet).
 *                       :
 *      dest             : destination packet information .
 *      source           : source packet information .

*/
typedef struct
{
    GT_MAC_TYPE                         macDaType;
    GT_BOOL                             useVidx;
    GT_U8                               llt;
    SKERNEL_UPLINK_PP_VIDX_UNT          dest;
    SKERNEL_UPLINK_PP_SOURCE_DESCR_STC  source;
}SKERNEL_UPLINK_FRAME_PP_DESCR_STC;

/*
 * Typedef: struct SKERNEL_UPLINK_DESC_STC
 *
 * Description:
 *      Describe a descriptor of frame.
 *
 * Fields:
 *      type    - frame or fdb message
 *      frame   - frame descriptor
 *      fdb     - fdb message
 *
 * Comments:
 */
typedef struct
{
    SKERNEL_UPLINK_FRAME_TYPE_ENT       type;
    struct
    {
      SKERNEL_UPLINK_FRAME_PP_DESCR_STC      PpPacket;
    }data;
}SKERNEL_UPLINK_DESC_STC;

/*******************************************************************************
*   smainSwitchFabricFrame2UplinkSend
*
* DESCRIPTION:
*        tramsmit frame to uplink
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr    -  pointer to the uplink descriptor.
*
* OUTPUT:
*
* RETURNS:
*       GT_OK   - successful
*       GT_FAIL - failure
*
* COMMENTS:
*        the function is used by the fa device and by the pp device objects.
*
*******************************************************************************/
GT_STATUS smainSwitchFabricFrame2UplinkSend
(
    IN SKERNEL_DEVICE_OBJECT    * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC  * descrPtr
);


/*******************************************************************************
* smainSwitchFabricSkernelFaTask
*
* DESCRIPTION:
*        fabric switch main device task.
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*      The function is called by smainSwitchFabricInit function.
*
*******************************************************************************/
void smainSwitchFabricSkernelFaTask
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

/*******************************************************************************
*   smainSwitchFabricInit
*
* DESCRIPTION:
*    Initiate the smain switch fabric module .
*
* INPUTS:
*       deviceObjPtr -  pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smainSwitchFabricInit
(
    SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

/*******************************************************************************
*   smainSwitchFabricDevObjGetByUplinkId
*
* DESCRIPTION:
*       Find target device, connected to the UpLink interface
*
* INPUTS:
*       upLinkId            -  uplink ID
*       destDeviceObjPtr    -  pointer to destination device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smainSwitchFabricDevObjGetByUplinkId
(
    IN    GT_U32                    upLinkId,
    OUT   SKERNEL_DEVICE_OBJECT     **destDeviceObjPtr
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smainSwitchFabrich */




