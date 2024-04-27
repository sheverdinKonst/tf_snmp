/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetTwistTrafficCond.h
*
* DESCRIPTION:
*       Traffic condition processing for frame.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*
*******************************************************************************/
#ifndef __snetTwistTrafficCondh
#define __snetTwistTrafficCondh

#include <asicSimulation/SKernel/smain/smain.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
    structure : SNET_TWIST_TRAFFIC_CONDITION_ENTRY_STC
    See format in table 2 [traffic conditioner MAS] called TC entry format

    this is the general TC entry that covers the twistC/D and samba formats


doMetering
    0- don't perform metering
    1- perform metering on flow pointing to this entry
doCounting (not for Samba)
    0- don't perform counting
    1- perform counting on flow pointing to this entry
remarkCosByDscp
    0-don't change DSCP/EXP
    1-remark according to CL
    this bit is valid only if doMetering==1
enModifyTC (only Samba)
    0-don't enable remark TC
    1-enable remark TC
remarkDP - (not for Samba)
    0-don't remark DP
    1- remark according to CL
    this bit is valid only when remarkCosByDscp==0
remarkCosByUP
    0-don't remark UP
    1- remark according to (TC/UP,CL)->CoS
dropRed
    0- don't drop packet which are remarked red by the traffic policer
    1- Drop packet which are remarked red by the traffic policer
colorMode-
    0- the traffic conditioner works in color blind mode
    1- the traffic conditioner works in color blind aware mode
meterDuMode
    0- 1DU = 1 byte
    1- 1DU = 64 bytes
tickMode
    0- each tick is 1 core clock(6ns@166Mhz)
    1- each tick is 8 core clock(48ns@166Mhz)
policerMode-
    0- SrTCM
    1- TrTCM
enModifyExteCoS- (only Samba)
    0- the remarkCosByDscp and remarkUP are not valid (disable option to remark)
    1- the remarkCosByDscp and remarkUP are valid (enable option to remark)
counterMode- (only Samba)
    0- CDU counter 32 bits , packets counter 32 bits
    1- CDU counter only 64 bits
    2- packets counter only 64 bits
    3- same as 0
meterManagementCounters -
    which set out of 3 management counters to use when traffic policing enabled
    0- don't use TP management counters
    1- use set 0 for TP management counters
    2- use set 1 for TP management counters
    3- use set 2 for TP management counters
counterDu-
    0- 1 CDU = 1 byte
    1- 1 CDU = 16 bytes
L0-
    this is the limit of the committed bucket in tick units
    The CSB(committed burst size in ticks)=L0/I0
L1-
    this is the limit of the excess/peak bucket in tick units.
    for policeMode=SrTCM:
    the EBS(excess burst size)in data units,EBS = L1/I0
    for policeMode=TrTCM
    the PBS(peak information rate)in data units,PBS = L1/I1
I0-
    the average number of ticks between tow successive DUs[ticks/du].
    CIR(committed information rate)=1/I0
I1-
    the average number of ticks between tow successive DUs[ticks/du].
    PIR(peak information rate)=1/I1
    this field is valid only when policeMode==TrTCM

meterCounters-
    counters according to counterMode.

tet0 -
    N/A
tet1-
    N/A

conformanceCounters-
    conformance counters .number of DU passed in the TC entry.(green/yellow/red)

*/
typedef struct{
    GT_U32     doMetering       :1;
    GT_U32     doCounting       :1;
    GT_U32     remarkCosByDscp  :1;
    GT_U32     enModifyTC       :1;
    GT_U32     remarkDP         :1;
    GT_U32     remarkCosByUP    :1;
    GT_U32     dropRed          :1;   /*N/A*/
    GT_U32     colorMode        :1;   /*N/A*/
    GT_U32     meterDuMode      :1;
    GT_U32     tickMode         :1;   /*N/A*/
    GT_U32     policerMode      :1;   /*N/A*/
    GT_U32     enModifyExtCoS   :1;
    GT_U32     counterMode      :2;
    GT_U32     meterManagementCounters:2;
    GT_U32     counterDu        :1;

    GT_U64     L0;                    /*N/A*/
    GT_U64     L1;                    /*N/A*/

    GT_U16     I0;                    /*N/A*/
    GT_U16     I1;                    /*N/A*/

    struct{
        GT_U64 cdu;
        GT_U64 packets;
    }meterCounters;

    GT_U32  tet0;                     /*N/A*/
    GT_U32  tet1;                     /*N/A*/

    struct{
        GT_U64  green;
        GT_U64  yellow;
        GT_U64  red;
    }conformanceCounters;

}SNET_TWIST_TRAFFIC_CONDITION_ENTRY_STC;



/*******************************************************************************
*   snetTwistTrafficConditionProcess
*
* DESCRIPTION:
*        make traffic condition processing
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        GT_TRUE   - continue frame processing
*        GT_FALSE  - stop frame processing
*******************************************************************************/
extern GT_BOOL snetTwistTrafficConditionProcess
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

/*******************************************************************************
*   snetSambaTrafficConditionWriteGenxsReg
*
* DESCRIPTION:
*        in the samba you can set action of reset Tc entry counters and the "old"
*       values are going to the genxs global counters
* INPUTS:
*        devObjPtr - pointer to device object.
*        tcIndex   - index of tcEntry
* OUTPUTS:
*        None
* COMMENT:
*       fatal error on error
*
*******************************************************************************/
extern GT_VOID snetSambaTrafficConditionWriteGenxsReg
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    GT_U32                tcIndex,
    IN    GT_U32                resetContersFlag
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetTwistTrafficCondh */


