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
*       Policy Engine of TwistC
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*
*******************************************************************************/
#ifndef __snetTwistClassifierh
#define __snetTwistClassifierh

#include <asicSimulation/SKernel/smain/smain.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

PRAGMA_NOALIGN
#ifdef _VISUALC
#pragma pack(1)
#endif /* _VISUALC */

/*
    structure : SNET_TWIST_CLASSIFIER_ACTION
    See format in 6.5 Tables 11-12 (key entry format and classifier command)
    (page 48) in Lx Unit classifier Mas
    or in 12.5.6 and 14.2 Tables 29,32 (generic flow entry and classifier command)
    in Prestera Mx architectural specification
*/
typedef struct{
/* word 0 */
    union{
        GT_U32  word0;

        struct{
            GT_U32  cmd:2;
            GT_U32  do_Epi:1;
            GT_U32  mirrorToCpu:1;
            GT_U32  do_Nat:1;
            GT_U32  mark_Cmd:2;
            GT_U32  dscp:6;
            GT_U32  tc_PtrOrCosParam:16;
            GT_U32  redirectFlowCmd:2;
            GT_U32  outlif0To19OrIPv4_Ptr_lsb:1;
        }fields;
    }word0;
/* word 1 */
    union{
        GT_U32  word1;

        /*if CC<Flow Redirect Cmd> = FlowSwitching*/
        struct{
            GT_U32  outlif0To19OrIPv4_Ptr_msb:19;
            GT_U32  NAT_Parity:1;
            GT_U32  NAT_Cmd:4;
            GT_U32  key_byte_mask:8;
        }flowSwitching;

        /*if CC<Flow Redirect Cmd> != FlowSwitching*/
        struct{
            GT_U32  outlif0To19OrIPv4_Ptr_msb:19;
            GT_U32  NAT_Parity:1;
            GT_U32  NAT_Cmd:4;
            GT_U32  user_DefinedOrkeyByteMask_lsb:8;
        }noFlowSwitching;

    }word1;

/* word 2 */
    union{
        GT_U32  word2;

        /*if CC<Flow Redirect Cmd> = FlowSwitching*/
        struct{
            GT_U32  reserved1:1;
            GT_U32  outLif20To41:22;
            GT_U32  outLifType:1;
            GT_U32  reserved2:8;
        }flowSwitching;

        /*if CC<Flow Redirect Cmd> != FlowSwitching*/
        struct{
            GT_U32  user_DefinedOrkeyByteMask_msb:8;
            GT_U32  NAT_Port:16;
            GT_U32  reserved:8;
        }noFlowSwitching;

    }word2;

} SNET_TWIST_CLASSIFIER_ACTION;

typedef struct{
    SNET_POLICY_CMD_ENT             cmd;
    GT_U32                          doEPI;                    /* not used now */
    GT_U32                          cpuMirror;
    GT_U32                          doNAT;                    /* not used now */
    SNET_POLICY_TRAFFIC_MARK_ENT    markCmd;
    GT_U32                          dscp;
    GT_U32                          tcPtr;
    GT_U8                           tc;
    GT_U8                           up;
    GT_U8                           dp;
    SKERNEL_FLOW_REDIRECT_ENT       cmdRedirect;
    GT_U32                          outLif0To19OrIPv4orMpls_Ptr;
    GT_U32                          outLif20To41;
    GT_U32                          natParity;                /* not used now */
    GT_U32                          natCmd;                   /* not used now */
    GT_U32                          keyByteMask;
    GT_U32                          outLifType;
    GT_U32                          natPort;                  /* not used now */
} SNET_TWIST_CLASSIFIER_FINAL_ACTION;

/*******************************************************************************
*   snetTwistCClassificationEngine
*
* DESCRIPTION:
*        the function do the classifier engine to get an action info
* INPUTS:
*        deviceObj - pointer to device object.
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
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetSambaPolicyh */


