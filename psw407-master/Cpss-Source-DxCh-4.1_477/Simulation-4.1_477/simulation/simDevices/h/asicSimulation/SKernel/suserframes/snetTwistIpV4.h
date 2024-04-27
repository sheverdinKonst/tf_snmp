/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetTwistIpV4.h
*
* DESCRIPTION:
*       This is a external API definition for Snet IpV4 module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 5 $
*
*******************************************************************************/
#ifndef __snetTwistIpV4h
#define __snetTwistIpV4h


#include <asicSimulation/SKernel/smain/smain.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Typedef: union IPMC_GROUP_ENTRY_STC
 *
 * Description:
 *      Describe a multicast group entry (Stg2 - MC address calculation).
 *
 * Fields:
*          nextMcPtr  : pointer to the base of a mc group table.
 *        minimumPtr  : Minimum pointer for next level mc group in 
 *                      64 entries resolution.
 *        maximumPtr  : Maximum Pointer for next level mc group in 
 *                      64 entries resolution.
 *     nextHopeRoute  : pointer to the Next Hop Mc Route Entry
 *         lastIndex  : The index to the fifth 1 starting from the LSB. 
 *        ecpQosSize  : Number of bits to be taken as least significant bits 
 *                      from ECP hash or InTOS.(0 = one bit, 1 = two bits ..)
 *     nextHopMethod  : Next Hop Route Method
 *       mcGroupType  : Multi cast group type
*/    
typedef union {
    struct {
        GT_U32 nextMcPtr;
        GT_U8 minimumPtr;
        GT_U8 maximumPtr;
    } level1_2;
    union {
        struct {
            GT_U32 nextMcPtr;
        } regular_bkt;
        struct {
            GT_U32 nextMcPtr;
            GT_U8 lastIdx;
        } compress_bkt;
        struct {
            GT_U32 nextHopeRoute;
            GT_U8 ecpQosSize;
            GT_U8 nextHopMethod;
            GT_U8 mcGroupType;
        } next_hope;
    } level3;
} IPMC_GROUP_ENTRY_UNT;

/*
 * Typedef: union SNET_LPM_NEXT_POINTER_UNT
 *
 * Fields:
 *         nextPtr    : address of the next bucket
 *         lastIndex  : The index to the fifth 1 starting from the LSB
 *     nextHopeRoute  : pointer to the Next Hop Mc Route Entry
 *        ecpQosSize  : Number of bits to be taken as least significant
 *                      bits from ECP hash or InTOS (0 = one bit, 1 =
 *                      two bits ...).
 *                      from ECP hash or InTOS.(0 = one bit, 1 = two bits ..)
 *     nextHopMethod  : 0 - pointer to an ECP entry
 *                      1 - pointer to a QOS route entry
 *                      2 - pointer to a regular route entry
 *                      3 - reserved
 *       bucketType   : 0 - Next pointer is a pointer to another Regular
 *                          Bucket
 *                      1 - Next pointer is a pointer to a compressed
 *                          Bucket with one row
 *                      2 - Next pointer is a pointer to a compressed
 *                          Bucket with two rows
 *                      3 - Current pointer is a pointer to a Next Hop
 *                          Route Entry
*/    
typedef union {
        struct {
            GT_U32 nextPtr;
            GT_U16 bucketType; 
        } bucket_0_1;
        struct {
            GT_U32 nextPtr;
            GT_U8 lastIdx;
            GT_U16 bucketType; 
        } bucket_2;
        struct {
            GT_U32 nextPtr;
            GT_U32 ecpQosSize;
            GT_U32 nextHopMethod;
            GT_U16 bucketType; 
        } bucket_3;
} SNET_LPM_NEXT_POINTER_UNT;

/*
 * Typedef: union SNET_UC_NEXTHOP_STC
 *
 * Description:
 *      IPV4 UC Next-Hop Entry
 *
 * Fields:
 *      mtu             MTU size
 *
 *      outLifType      0 - Output Logical Interface is Link-Layer OutLIF.
 *                      1 - Output Logical Interface is Tunnel.
 *
 *      tunnelOutLif    When <OutLIFType> = 1, the Tunnel-Layer 
 *                      Output Logical Interface.
 *                            
 *      LL_OutLIF       Pointer to Link-Layer Output Interface entry
 *                        
 *      arpPtr          ARP entry pointer.
 *                                                              
 */ 
typedef struct {
    GT_U32 mtu;
    GT_U8 outLifType;
    union {
        struct {
            GT_U32 tunnelOutLif;
        } tunnel_outlif;
        struct {
            GT_U32 LL_OutLIF; 
            GT_U32 arpPtr;
        }link_layer; 
    } out_lif;
} SNET_UC_NEXTHOP_STC;

/*
 * Typedef: struct MPLS_LABLE_STC
 *
 * Description:
 *      IPv4 Unicast Route Entry
 *
 * Fields:
 *  expSet        - 0 - Set outgoing EXP field in the MPLS label according to
 *                      the EXP CoS assigned to the packet by previous 
 *                      operations before this lookup.
 *                  
 *                  1 - Set EXP field in the MPLS Label Stack according to this 
 *                      entry.s <EXP> field  
 *      
 *  label         - Outgoing MPLS label to swap on top of the incoming label    
 *    
 *  exp             If <EXP Setting>=0, reserved. 
 *                  If <EXP Setting>=1, the EXP field to set in the MPLS label.  
 *    
 *  ttl             0 - if <Dec_TTL>=0, set outgoing TTL field in the MPLS 
 *                      label to incoming TTL. 
 *                      If <Dec_TTL>=1, set outgoing TTL field in the MPLS label 
 *                      to incoming TTL-1.
 *                  >0 - The outgoing TTL is set to this value.
*/ 
typedef struct {
    GT_U8 expSet;
    GT_U32 label;
    GT_U8 exp;
    GT_U8 ttl;
}MPLS_LABLE_STC;

/*
 * Typedef: struct UC_ROUT_ENTRY_STC and UC_ROUT_ENTRY_STC
 *
 * Description:
 *      IPv4 Unicast/Multicast Route Entry
 *
 * Fields:
 *      cmd             : 0 - Pass transparently
 *                        1 - Route
 *                        2 - Trap to CPU
 *                        3 - Drop
 *      forceCos        : When this bit is set, the COS of the packet is taken 
 *                        from the route entry <Prirority> ,<DP>, <VPT>.
 *      priority        : If (<Force_COS> = 1) this is the priority of the 
 *                        outgoing packet Else Reserved     
 *      dp              : If (<Force_COS> = 1) this is the DP of the 
 *                        outgoing packet Else Reserved
 *      vpt             : If (<Force_COS> = 1) this is the VPT of the 
 *                        outgoing packet Else Reserved     
 *      ipMngCntSet     : 0 - IP Multicast Management Counter Set 0
 *                        1 - IP Multicast Management Counter Set 1
 *                        2- IP Multicast Management Counter Set 2
 *                        3 - Theres no IP Multicast Management Counter 
 *                        Set associated with this Route Entry and will not 
 *                        be counted.      
 *      enableDec       : 0 -The TTL of Packets with this route entry will 
 *                        not be decremented
 *                        1 -The TTL of Packets with this route entry can be 
 *                        decremented     
 *      l2Ce            : The route entry is an L2CE multicast route entry
 *      age             : This bit is set to 1 by the Packet Processor every 
 *                        time a match is found on the route entry.
 *                        This feature is activated only if 
 *                        IPv4CtrlReg<EnAgeRefresh> is set.    
 *      mtu             :     
 *      rpfCmd          : Indicates the action to be done on packets which do not 
 *                        pass the Check_RPF.
 *                        0 - Trap packet to CPU
 *                        1 - Packet is dropped
 *                        Note: this field is valid only when Check_RPF = 1
 *      checkRpf        : Check whether this packet was received on the RPF 
 *                        Logical Interface or not.
 *                        0 - Don't check
 *                        1 - Check
 *      rpfInLif        : Input Logical Interface or VID for 
 *                        Reverse Path Forwarding, according to the
 *                        ipv4_ctrl_reg [3] = Inlif/VID_RFP_Check
 *                        Note: InLIF is 16 bits and VID is 12 bits
 *      
 *      mll             : Multicast Linked List pointer.
 *                        When MLL = 0, then the Multicast will be bridged 
 *                        to the ports of the VLAN that was received on.  
 *      devVidx         :
 *      nextHop         : The next hop, valid when <CMD>=Router see     
 *      greTunnel       : 1 - Packet can be an end of a GRE_IPv4.
 *                        0 - Packet can not be an end of a tunnel     
 *      ipTunnel        : 1 - Packet can be an end of a IPv4 tunnel.
 *                        0 - Packet can not be an end of a tunnel     
 *      tunCpyDscp      : 1 - Copy DSCP to inner header
 *                        0 - maintain original DSCP of inner header    
 *      tunCpyTtl       : When set and terminating a tunnel, the TTL field 
 *                        is copied from the external IPv4 header to the 
 *                        tunneled IPv4,IPv6 or MPLS headers.    
 *      pushMpls        : 0 - do not push MPLS Label
 *                        1 - push MPLS label, as defined in this entry         
 *      mplsExpSettings : 0 - Preserve incoming EXP
 *                        1 - Set EXP field in the MPLS Label Stack 
 *                        according to <EXP> field
 *      mplsLabel       : New MPLS label to push onto the label stack
 *      mplsExp         : If <EXP Setting> = 1: the EXP field to set in the 
 *                        MPLS Label If <EXP Setting> = 0: reserved          
 *      mplsTtl         : If <TTL> = 0 TTL is copied from the Ip header
 *                        If <TTL> !=0 set the TTL to the value in this field. 
*/ 
typedef struct {
    /* Word 0 common structure for UC and MC entry */
    GT_U8 cmd;        
    GT_U8 forceCos;
    GT_U8 priority;
    GT_U8 dp;
    GT_U8 vpt;
    GT_U8 ipMngCntSet;
    GT_U8 enableDecTtl;
    GT_U8 l2Ce;
    GT_U8 age;
    GT_U32 mtu;
    SNET_UC_NEXTHOP_STC ucNextHopEntry;
    GT_U8  greTunnelEnd;
    GT_U8  ipTunnelEnd;
    GT_U8  tunnelCpyDscp;
    GT_U8  tunnelCpyTtl;
    GT_U8  pushMpls;
    MPLS_LABLE_STC mplsInfo;
} UC_ROUT_ENTRY_STC;

typedef struct {
    /* Word 0 common structure for UC and MC entry */
    GT_U8 cmd;        
    GT_U8 forceCos;
    GT_U8 priority;
    GT_U8 dp;
    GT_U8 vpt;
    GT_U8 ipMngCntSet;
    GT_U8 enableDecTtl;
    GT_U8 l2Ce;
    GT_U8 age;
    GT_U32 mtu;
    GT_U8 rpfCmd;
    GT_U8 checkRpf;
    GT_U16 rpfInLif;
    GT_U16 mll;
    GT_U16 devVidx;
    GT_U8  pushMpls;
    MPLS_LABLE_STC mplsInfo;
} MC_ROUT_ENTRY_STC;
/*******************************************************************************
*   ipV4CheckSumCalc
*
* DESCRIPTION:
*        Perform ones-complement sum , and ones-complement on the final sum-word.
*        The function can be used to make checksum for various protocols.
* INPUTS:
*        msgPtr - pointer to IP header.
*        msgSize  - IP header length.
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
    IN GT_U8 *pMsg,
    IN GT_U16 lMsg
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
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetTwistIpV4h */

