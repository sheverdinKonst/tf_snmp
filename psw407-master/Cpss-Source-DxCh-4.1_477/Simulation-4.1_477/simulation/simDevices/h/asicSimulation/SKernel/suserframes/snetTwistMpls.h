/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetTwistMpls.h
*
* DESCRIPTION:
*       This is a external API definition for Snet MPLS module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*
*******************************************************************************/
#ifndef __snetTwistMplsh
#define __snetTwistMplsh

#include <asicSimulation/SKernel/smain/smain.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Typedef: union MPLS_NEXT_HOP_UNT
 *
 * Description:
 *      MPLS Next-Hop Entry
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
            GT_U32 tunnelOutLifLo;
            GT_U32 tunnelOutLifHi;
        } tunnel_outlif;
        struct {
            GT_U32 LL_OutLIF; 
            GT_U32 arpPtr;
        }link_layer; 
    } out_lif;
} MPLS_NEXT_HOP_UNT;

/*
 * Typedef: union MPLS_COMMAND_UNT
 *
 * Description:
 *      Describe a MPLS command Data Structure.
 *
 * Fields:
 *
 *  defCmdEntry   - Default PenUltimate Command
 *    
 *  nextHopEntry  - The Next-Hop information.  
 *    
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
 *    
 *  l2ce            This is set when the packet is entering a Layer 2 Circuit 
 *                  Emulation Tunnel. If set and the packet is an Ethernet 
 *                  frame, the packet.s CRC is removed.
 *    
 *  nlp             Next layer protocol: This field is used to set the 
 *                  appropriate outgoing encapsulation:
 *                  0 - IPv4
 *                  1 - IPv6
 *                  2 - MPLS
 *                  3 - User_Defined_Protocol_0
 *                  4 - User_Defined_Protocol_1
 *                  5 - Reserved
 *                  6 - L2CE, Ethernet: An Ethernet CRC must be generated.
 *                  7 - L2CE, Non-Ethernet  
 *    
 *  popCpyDscpExp   Valid only when NLP = IPv4, IPv6 or MPLS.
 *                  0 - DSCP field of the inner IP header or the EXP field in 
 *                      the next layer MPLS label remains as is.
 *                  1 - DSCP field of the inner IP header or the EXP field in 
 *                      the next layer MPLS label will
 *                      be taken from the DSCP/EXP field associated with the 
 *                      packet CoS.    
 *    
 *  popCpyTtl       1 - When popping an MPLS label, the TTL from the popped 
 *                      label is copied to the inner header, which may be IP 
 *                      or MPLS.
 *                  0 - The TTL field of the popped label is discarded and the 
 *                      TTL/HopCount field of the inner header is maintained.
 *                      NOTE: The incoming TTL is still checked, to make sure 
 *                      that it is greater than 0.  
 *
 *  vrId            If <override_VR-ID>=1, this value is the packet.s new 
 *                  Virtual Router ID. This may be needed when popping the last 
 *                  label in the stack and the <NLP>=IPv4. 
 *                  If <override_VR-ID>=0, reserved.
 *
 *  inLif           If <override_InLIF>=1, this value is the packet's new 
 *                  InLIF index. This may be needed when: popping the last 
 *                  label in the stack and NLP=IPv4, so the IP Multicast can
 *                  use this value for checking RPF.popping an MPLS label, 
 *                  and the next layer (NLP) is also MPLS, this field will be 
 *                  used to fetch a Label Space Entry and access the ILM.
 *
 *  overVrId        0 - Do not override the packet's VR-ID
 *                  1 - Override the packet's VR-ID according to this 
 *                      entry's <VR-ID>
 *
 *  overInLif       0 - Do not override the packet's InLIF index
 *                  1 - Override the packet's InLIF index according to this 
 *                      entry's <InLIF#>
 *
 
*/    

typedef struct {
    MPLS_NEXT_HOP_UNT nextHopEntry;
    GT_U8 expSet;
    GT_U32 label;
    GT_U8 exp;
    GT_U8 ttl;
}CMD_SWAP_STC;

typedef struct {
    MPLS_NEXT_HOP_UNT nextHopEntry;
    GT_U8 expSet;
    GT_U32 label;
    GT_U8 exp;
    GT_U8 ttl;
    GT_U8 l2ce;
}CMD_PUSH_STC;

typedef struct {
    MPLS_NEXT_HOP_UNT nextHopEntry;
    GT_U8 nlp;
    GT_U8 popCpyDscpExp;
    GT_U8 popCpyTtl;
}CMD_PNLT_POP_STC;
        
typedef struct {
    GT_U16 vrId;
    GT_U16 inLif;
    GT_U8 overVrId;
    GT_U8 overInLif;
    GT_U8 nlp;
    GT_U8 popCpyDscpExp;
    GT_U8 popCpyTtl;
}CMD_POP_STC;

typedef union {
    GT_U32 defCmdEntry[2];
    CMD_SWAP_STC        cmd_swap;
    CMD_PUSH_STC        cmd_push;
    CMD_PNLT_POP_STC    cmd_pnlt_pop;
    CMD_POP_STC         cmd_pop;
} MPLS_COMMAND_UNT;

/*
 * Typedef: struct NHLF_FORWARD_ENTRY_STC
 *
 * Description:
 *      Describe a Next-Hop Label Forwarding Entry
 *
 * Fields:
 *  pktCmd  -       0 - Drop
 *                  1 - Trap to CPU
 *                  2 - MPLS Switch according to <MPLS_Cmd>
 *                  3 - Pass transparently
 *   
 *  passCib  -      Pass to CIB (reserved)
 *
 *  forceCos -      0 - The CoS of the packet is not changed by this NHLFE. 
 *                      It remains as set by all previous operations prior to 
 *                      this lookup. (When multiple Pops are performed, this
 *                      includes CoS assigned, possibly by previous Pops.)
 *                  1 - The CoS of the packet is set according to this entry.s 
 *                      <Traffic Class>, <DP>, <User Priority> settings   
 *
 *  trafficClass    If (<Force_COS>=1) this is the traffic class assigned to 
 *                  the packet
 *   
 *  dp              If (<Force_COS>=0) this field has no meaning
 *                  If (<Force_COS>=1) this is the Drop-Precedence 
 *                  assigned to the packet.   
 *   
 *  userPriority    If (<Force_COS>=0) this field has no meaning
 *                  If (<Force_COS>=1) this is the 802.1p User Priority assigned 
 *                  to the packet.
 *
 *  mngCountSet     Maintains counters for this NHLFE entry in management set:
 *                  0 - Set 0
 *                  1 - Set 1
 *                  2 - Set 2
 *                  3 - Do not maintain any counter set for this entry.
 *
 *  decrTtl         1 - Decrement the TTL of packets matching this MPLS entry.
 *                  0 - Do not decrement the TTL of packets matching this MPLS 
 *                      entry.
 *  
 *  mplsCmd         This field is valid only when <CMD>=MPLS Switch
 *                  0 - Swap MPLS label.
 *                  1 - Push one MPLS Label.
 *                  2 - Pop one label.
 *                  3 - Penultimate Hop pop (PHP).
 *
 */    
typedef struct 
{
    GT_U8 pktCmd;
    GT_U8 passCib;
    GT_U8 forceCos;
    GT_U8 trafficClass;
    GT_U8 dp;
    GT_U8 userPriority;
    GT_U8 mngCountSet;
    GT_U8 decrTtl;
    GT_U8 mplsCmd;
    /* MPLS commands */
    MPLS_COMMAND_UNT mplsInfo;
} NHLF_FORWARD_ENTRY_STC;


/*
 * Typedef: struct MPLS_LOOKUP_INFO_STC
 *
 * Description:
 *      Describe a Next-Hop Label Forwarding Entry
 *
 * Fields:
 *  priority        -   Priority of the context
 *
 *  dp              -   Drop precedence of the context
 *
 *  vpt             -   VPT of the context
 *
 *  inLif           -   The InLIF of the incoming context
 *
 *  remarkCos       -   0 - No remarking is needed
 *                      1 - remark the CoS according to EXP2Cos
 *
 *  doEpi           -   Set according to the InNHLFE<Pass to EPI> bit
 *   
 *  lookUpAddress   -   address for the first lookup 
 *
 *  labelInfo       -   label info
 *
 *  Status registers
 *
 *  phase           -   0 - search MPLS Interface table.
 *                      1 - search ILM table 
 *
 *  pendIssue       -   0 - packet is not waiting for the issue block 
 *                      1 - Waiting for the Issue block to issue a req
 *
 *  labelNum            0 - working on label 0
 *                      1 - working on label 1
 *
 *  doneStatus      -   0 - do lookup   
 *                      1 - no need for lookup                       
 *                      2 - drop                       
 *                      3 - TTL-0
 *                      4 - invalid entry(valid=0,label>max,<min)
 *                      5 - illegal pop
*/ 
typedef struct {
    GT_U8               priority;
    GT_U8               dp;
    GT_U8               vpt;
    GT_U16              inLif;
    GT_U8               remarkCos;
    GT_U8               doEpi;
    GT_U32              lookUpAddress;
    MPLS_LABELINFO_STC  labelInfo[2];
    GT_U8               phase;
    GT_U8               labelNum;
    GT_U8               doneStatus;
} MPLS_LOOKUP_INFO_STC;

/*
 * Typedef: struct MPLS_INTERFACE_TABLE_STC
 *
 * Description:
 *      MPLS Interface Table
 *
 * Fields:
 *  inLifNumber -   inLif index
 *  valid       -   This interface is valid and up
 *  minLabel    -   The minimal label to accept on this interface
 *  size0       -   valid values are from 0 to 12
 *  size1       -   The label space size is Size1 << Size 0 (<< - is shift left)
 *  baseAddress -   The base address. The base address points to the entry 
 *                  having the Min_Label
 *
*/ 
typedef struct {
    GT_U16  inLifNumber;
    GT_U8   valid;
    GT_U32  minLabel;
    GT_U8   size0;
    GT_U8   size1;
    GT_U32  baseAddress;
} MPLS_INTERFACE_TABLE_STC;


/*******************************************************************************
*   snetTwistMplsProcess
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
GT_BOOL snetTwistMplsProcess
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetTwistMplsh */

