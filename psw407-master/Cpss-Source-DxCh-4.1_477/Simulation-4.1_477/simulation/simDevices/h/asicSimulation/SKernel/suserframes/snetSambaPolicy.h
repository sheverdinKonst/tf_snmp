/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetSambaPolicy.c
*
* DESCRIPTION:
*       Policy Engine of Samba
*       calling PCL Engine and Classifier Engine
*       for applying actions on the descriptor (that will be of the frame)
*
*       implementation according to REG_SIM_POLICY_SAMBA_FRDTD document
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 7 $
*
*******************************************************************************/
#ifndef __snetSambaPolicyh
#define __snetSambaPolicyh

#include <asicSimulation/SKernel/smain/smain.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

PRAGMA_NOALIGN
#ifdef _VISUALC
#pragma pack(1)
#endif /* _VISUALC */


#define POLICY_DROP_CODE  5

#define SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS                 (20)

#define SNET_SAMBA_FLOW_TEMPLATE_HASH_SELECT_ENTRY_SIZE_IN_BYTES  (64)

#define SNET_SAMBA_INTERNAL_MEM_BASE_ADDRESS             (0x28000000)
#define SNET_SAMBA_EXTERNAL_MEM_BASE_ADDRESS             (0x38000000)

/* check if device has flow dram */
#define SNET_SAMBA_DEV_HAS_FDRAM(deviceObj)\
    (((TWIST_DEV_MEM_INFO*)(deviceObj->deviceMemory))->fdramMem.fDramSize ? \
    GT_TRUE : GT_FALSE)

/* calc memory base address in the flow dram or not */
#define SNET_SAMBA_CLASSIFIER_FLOW_BASE_ADDRESS_MAC(deviceObj)                 \
    (SNET_SAMBA_DEV_HAS_FDRAM(deviceObj) == GT_FALSE ?                         \
    SNET_SAMBA_INTERNAL_MEM_BASE_ADDRESS :                                     \
    SNET_SAMBA_EXTERNAL_MEM_BASE_ADDRESS )

/* calc num of bytes of flow key entry */
/* see the building in coreFlowInit(...) */
#define SNET_SAMBA_CLASSIFIER_FLOW_ENTRY_NUM_OF_BYTES_MAC(deviceObj,numBytesInKey) \
    (SNET_SAMBA_DEV_HAS_FDRAM(deviceObj) ?                                     \
        (numBytesInKey < 5 ? (4 * 4) :                                         \
         (numBytesInKey < 13? (6 * 4) : (8 * 4)) )                             \
        :                                                                      \
        (numBytesInKey < 5 ? (4 * 4) :                                         \
         (numBytesInKey < 13? (8 * 4) : (8 * 4) )))

/* calc base address of flow key table */
/* see the building in coreFlowInit(...) */
#define SNET_SAMBA_CLASSIFIER_FLOW_KEY_TABLE_ADDRESS(deviceObj,baseAddr_29bits)\
    (SNET_SAMBA_DEV_HAS_FDRAM(deviceObj) ?                                     \
     (baseAddr_29bits & 0x1fffffff) << 3 :                                     \
     (baseAddr_29bits & 0x0fffffff) << 4 )

/* calc size of flow key table */
/* see the building in coreFlowInit(...) */
#define SNET_SAMBA_CLASSIFIER_FLOW_KEY_TABLE_SIZE(deviceObj,baseAddr_26bits)   \
    (SNET_SAMBA_DEV_HAS_FDRAM(deviceObj) ?                                     \
     (baseAddr_26bits & 0x03ffffff) << 2 :                                     \
     (baseAddr_26bits & 0x03ffffff) << 2 )

typedef enum {
    SNET_SAMBA_POLICY_ACTION_CMD_DROP_E = 0,
    SNET_SAMBA_POLICY_ACTION_CMD_TRAP_E  = 1,
    SNET_SAMBA_POLICY_ACTION_CMD_PROCESS_E = 2
}SNET_SAMBA_POLICY_ACTION_CMD_ENT;

/*
    structure : SNET_SAMBA_POLICY_ACTION
    See format in 1.6.4.6 table 5 [3] called combined PCE format

    this is the combination of the classifier , pcl , Cos/ip , default actions .


Cmd
    0 = DROP: Drop the packet silently.
    1 = TRAP: Trap the packet to the CPU.
    2 = Process
    3 = Reserved
Mirror to Analyzer Port
    0 = Do not send the packet to the Analyzer Port.
    1 = Send the packet to the Analyzer port.
ActivateTC
    0 - Do not activate TC.
    1 - Activate TC using the TC/Cnt Ptr.
ActivateCount
    0 - Do not activate Counters.
    1 - Activate Counters using the TC/Cnt Ptr.
Mark_Cmd
    0 = mark DSCP_CoS: Set packets CoS according to this
        entries <DSCP> and <CoS_Params> fields.
    1 = Trust L2 CoS: Keep the CoS as it was received from
        the Layer 2 ingress bridge processor.
    2 = Mark DSCP: Set packets CoS by mapping the entrys
        <DSCP> field to CoS using the DSCP->CoS Table.
    3 = Trust packet DSCP: Set packets CoS according to the
        incoming IPv4 DSCP or MPLS EXP field and map it to CoS
        based on DSCP->CoS table.
DSCP
    The DSCP used for marking.
TC/Cnt_Ptr
    This field is the Tc/Ctr ptr it is copied from the same PCE
    which the Activate TC and Activate Ctr is taken from
Redirect_Flow_Cmd
    Based on the Lookup result, a packet can be forwarded to
    a Logical Interface.
    0 = NO_REDIRECTION: Do not perform traffic redirecting.
    1 = Redirect to MPLS
    2 = Redirect to IPv4
    3 = FORWARD_PKT_USING_EXPLICIT_OUTLIF: Forward
        the packet via the Output Logical Interface
OutLIF/IPv4Ptr/ MPLSPtr
    If <Redirect_Flow_Cmd> =
        FORWARD_PKT_USING_EXPLICIT_OUTLIF, this field
        contains the Output Logical Interface entry.
    Else if <Redirect_Flow_Cmd> = Redirect to MPLS, this
        field contains the MPLSPtr.
    Else if <Redirect_Flow_Cmd> = Redirect to IPv4, this
        field contains the IPv4Ptr
Set LBH
    0 = Do not set the packets LBH field
    1 = Set the packets LBH field according to PCE<LBH>
LBH
    The Load Balancing Hash to be added to the packet that is
    forwarded to the fabric interface.
Nested VLAN
    0 - Do not activate nested vlan
    1 - customer port (src tag =0)
Modify VLAN
    0 - Do not modify vlan
    1 - modify VLAN
VID
    This field is ONLY the VID and is relevant when <Modify
    VLAN> is set.
Mirror to CPU
    0 - Do not mirror the packet to the CPU
    1 - Mirror the packet to CPU
Mark DSCP/EXP
    0 - Do not mark the DSCP/EXP.
    1 - Mark DSCP.
    NOTE: Mark DSCP/EXP command have different meaning
    according to the MarkCmd see the TC MAS for more
    details.
Mark TC
    0 - Do not mark the TC (TC remains the same as the L2 TC)
    1 - Mark TC
Mark UP (.1p)
    0 - Do not mark the UP (UP remains the same as the L2 UP)
    1 - Mark UP
83 Mark DP
    0 - Do not mark the DP (DP remains the same as the L2 DP)
    1 - Mark DP
TC
    The TC CoS parameters used when the MarkCmd = Mark_DSCP_CoS
UP
    The UP CoS parameters used when the MarkCmd = Mark_DSCP_CoS
DP
    The DP CoS parameters used when the MarkCmd = Mark_DSCP_CoS
FlowID
    The FlowID is copied from PCE[74:66]
trapActionSource
    the module that the Cmd == TRAP came from it and finally in the policy
    Engine caused to trap packet to CPU
    one of SNET_POLICY_ACTION_SRC_ENT
*/
typedef struct{
    GT_U8  cmd;
    GT_U8  mirrorToAnalyzerPort; /* not supported in samba */
    GT_U8  activateTC;
    GT_U8  activateCount;
    GT_U8  mark_Cmd;
    GT_U8  dscp;
    GT_U16 tcOrCnt_Ptr;
    GT_U8  redirectFlowCmd;
    GT_U32 outlifOrIPv4_Ptr;
    GT_U8  setLBH;
    GT_U8  LBH;
    GT_U8  nestedVlan;
    GT_U8  modifyVlan;
    GT_U16 vid;
    GT_U8  mirrorToCpu;
    GT_U8  markDscp;
    GT_U8  markTc;
    GT_U8  markUp;
    GT_U8  markDp;
    GT_U8  tc;
    GT_U8  up;
    GT_U8  dp;
    GT_U16 flowId;
    SNET_POLICY_ACTION_SRC_ENT trapActionSource;
}SNET_SAMBA_POLICY_ACTION;

/*
    structure : SNET_SAMBA_CLASSIFIER_ACTION
    See format in 5.2 Table 6 (page 13) [6] called
    Generic Flow Classifier Policy Action Entry (GFC_PAE)
*/
typedef struct{
/* word 0 */
    union{
        GT_U32  word0;

        struct{
            GT_U32  cmd:2;
            GT_U32  reserved1:1;
            GT_U32  mirrorToCpu:1;
            GT_U32  reserved2:1;
            GT_U32  mark_Cmd:2;
            GT_U32  dscp:6;
            GT_U32  tc_PtrOrCosParam:16;
            GT_U32  redirectFlowCmd:2;
            GT_U32  outlifOrIPv4_Ptr_lsb:1;
        }fields;
    }word0;
/* word 1 */
    union{
        GT_U32  word1;

    /*[87:51] if Policy Control Register<Enable Multiplexed CoS > is SET*/
        struct{
            GT_U32  outlifOrIPv4_Ptr_msb:19;
            GT_U32  reserved:13;
        }multiplexedCos;
    /*[87:51] if Policy Control Register<Enable Multiplexed CoS > is not SET and
        GFC_PAE<Flow Redirect Cmd> = OutLIF*/
        struct{
            GT_U32  outlifOrIPv4_Ptr_msb:19;
            GT_U32  reserved:13;
        }noMultiplexedCosRedirectOutLif;

    /*[87:51] if Policy Control Register<Enable Multiplexed CoS > is not SET and
       GFC_PAE<Flow Redirect Cmd> != OutLIF*/
        struct{
            GT_U32  outlifOrIPv4_Ptr_msb:19;
            GT_U32  reserved:5;
            GT_U32  cosTc:3;
            GT_U32  cosDp:2;
            GT_U32  cosUp:3;
        }noMultiplexedCosRedirectNoOutLif;

    }word1;

/* word 2 */
    union{
        GT_U32  word2;

    /*[87:51] if Policy Control Register<Enable Multiplexed CoS > is SET*/
        struct{
            GT_U32  reserved1:1;
            GT_U32  linkLayerOutLif:10;
            GT_U32  reserved2:21;
        }multiplexedCos;

    /*[87:51] if Policy Control Register<Enable Multiplexed CoS > is not SET and
        GFC_PAE<Flow Redirect Cmd> = OutLIF*/
        struct{
            GT_U32  reserved1:1;
            GT_U32  linkLayerOutLif:10;
            GT_U32  markUp:1;
            GT_U32  markDp:1;
            GT_U32  cosTc:3;
            GT_U32  cosDp:2;
            GT_U32  cosUp:3;
            GT_U32  markDscp:1;
            GT_U32  markTc:1;
            GT_U32  reserved2:9;
        }noMultiplexedCosRedirectOutLif;

    /*[87:51] if Policy Control Register<Enable Multiplexed CoS > is not SET and
       GFC_PAE<Flow Redirect Cmd> != OutLIF*/
        struct{
            GT_U32  markDscp:1;
            GT_U32  markTc:1;
            GT_U32  markUp:1;
            GT_U32  markDp:1;
            GT_U32  reserved1:28;
        }noMultiplexedCosRedirectNoOutLif;

    }word2;

}SNET_SAMBA_CLASSIFIER_ACTION;

/*
    structure : SNET_SAMBA_PCE_ACTION
    See table 3 in 1.6.4.2 [3] called Pce Format
*/
typedef struct{
/* word 0 */
    GT_U32  cmd:2;
    GT_U32  mirrorToAnalyzerPort:1; /* not supported in samba */
    GT_U32  activateTC:1;
    GT_U32  activateCount:1;
    GT_U32  mark_Cmd:2;
    GT_U32  dscp:6;
    GT_U32  tcOrCnt_Ptr:16;
    GT_U32  redirectFlowCmd:2;
    GT_U32  outlifOrIPv4_Ptr_lsb:1;
/* word 1 */
    GT_U32  outlifOrIPv4_Ptr_msb:29;
    GT_U32  setLBH:1;
    GT_U32  LBH:2;
/* word 2 */
    union{
        GT_U32  word2;

        struct{
            GT_U32  nestedVlan:1;
            GT_U32  modifyVlan:1;
            GT_U32  vidOrFlowId:8;
            GT_U32  cosTc:3;
            GT_U32  cosUp:3;
            GT_U32  cosDp:2;
            GT_U32  age:1;
            GT_U32  mirrorToCpu:1;
            GT_U32  markDscp:1;
            GT_U32  markTc:1;
            GT_U32  markUp:1;
            GT_U32  markDp:1;
            GT_U32  reserved2:8;
        }markCmdDscpCos;

        struct{
            GT_U32  nestedVlan:1;
            GT_U32  modifyVlan:1;
            GT_U32  vid:12;
            GT_U32  reserved1:4;
            GT_U32  age:1;
            GT_U32  mirrorToCpu:1;
            GT_U32  markDscp:1;
            GT_U32  markTc:1;
            GT_U32  markUp:1;
            GT_U32  markDp:1;
            GT_U32  reserved2:8;
        }markCmdNotDscpCos_modifyVlan;

        union{
            GT_U32  nestedVlan:1;
            GT_U32  modifyVlan:1;
            GT_U32  flowId:9;
            GT_U32  reserved1:7;
            GT_U32  age:1;
            GT_U32  mirrorToCpu:1;
            GT_U32  markDscp:1;
            GT_U32  markTc:1;
            GT_U32  markUp:1;
            GT_U32  markDp:1;
            GT_U32  reserved2:8;
        }markCmdNotDscpCos_noModifyVlan;
    }word2;
}SNET_SAMBA_PCE_ACTION;

/*
    structure : SNET_SAMBA_FLOW_TEMPLATE_HASH
    See format registers Table 555 0x02C28000-0x02C2803C [4]
*/
enum{
    SNET_POLICY_FLOW_XOR_HASH = 0,
    SNET_POLICY_FLOW_CRC_HASH = 1
};

/*  structure : SNET_SAMBA_FLOW_TEMPLATE_HASH
    See Table 555 registers 0x02C28000-0x02C2803C [4]*/
typedef struct{
    GT_U32 hashType:2;
    GT_U32 adWidth:5;
    GT_U32 csh:3;
    GT_U32 baseAddress:22;
}SNET_SAMBA_FLOW_TEMPLATE_HASH;

typedef struct{
/* word 0 */
    GT_U32 valid:1;
    GT_U32 col:1;
    GT_U32 parity:1;/* not used in simulation */
    GT_U32 addressLsb:21;
    GT_U32 Offset0_or_addressMsb:8;
/* word 1 */
    GT_U32 colRange0:2;
    GT_U32 offset1:8;
    GT_U32 colRange1:2;
    GT_U32 offset2:8;
    GT_U32 colRange2:2;
    GT_U32 offset3:8;
    GT_U32 colRange3:2;
}SNET_SAMBA_CLASSIFIER_HASH_ENTRY;

typedef struct{
/* word 0 */
    GT_U32 valid:1;
    GT_U32 col:1;
    GT_U32 addressLsb:22;
    GT_U32 Offset0_or_addressMsb:8;
/* word 1 */
    GT_U32 colRange0:2;
    GT_U32 offset1:8;
    GT_U32 colRange1:2;
    GT_U32 offset2:8;
    GT_U32 colRange2:2;
    GT_U32 offset3:8;
    GT_U32 colRange3:2;
}SNET_SAMBA_CLASSIFIER_VL_TRIE_ENTRY;

/*
    structure : SNET_SAMBA_BYTE_17
See structure from Table 2 in 1.6.1.3 [3]  called key attribute Byte17
*/

typedef union{
    struct{
        GT_U8  route:1;
        GT_U8  foundDa:1;
        /*Bits 7:2 when PCLCtrlReg<EnableMacDATypeAtt> = 1*/
        GT_U8  macDaType:2;
        GT_U8  pclId:4;
    }nacDATypeAttEn;

    struct{
        GT_U8  route:1;
        GT_U8  foundDa:1;
        /*Bits 7:2 when PCLCtrlReg<EnableMacDATypeAtt> = 0*/
        GT_U8  pclId:6;
    }nacDATypeAttDis;

}SNET_SAMBA_BYTE_17;

/* structure : SNET_SAMBA_ACTION2LOOKUP
  See table 461 [4] called PCL Action to Lookup Mapping Register : 0x028001E0 */
typedef struct{
    GT_U32 cmdMap:2;
    GT_U32 mirrorToAnalyzerPortMap:2; /* not supported in samba */
    GT_U32 activateTCAndContMap:2;
    GT_U32 mark_CmdMap:2;
    GT_U32 redirectFlowCmdMap:2;
    GT_U32 setLBHMap:2;
    GT_U32 nestedVlanMap:2;
    GT_U32 modifyVlanMap:2;
    GT_U32 flowIdMap:2;
    GT_U32 mirrorToCpuMap:2;
    GT_U32 reserved:12;
}SNET_SAMBA_ACTION2LOOKUP;

/*  structure : SNET_SAMBA_POLICY_FINAL_SETTING
    fields from Table 469 [4] called Policy Final Action Setting
    Table Register 0x028001E4*/
typedef struct{
    GT_U32 cmd:3;
    GT_U32 mirrorToAnalyzerPort:2;  /* not supported in samba */
    GT_U32 reserved1:1;
    GT_U32 activateTC:2;
    GT_U32 mark_Cmd:2;
    GT_U32 redirectFlowCmd:2;
    GT_U32 setLBH:2;                /* not supported in samba */
    GT_U32 nestedVlan:2;            /* not supported in samba */
    GT_U32 modifyVlan:2;            /* not supported in samba */
    GT_U32 flowId:2;                /* not supported in samba */
    GT_U32 mirrorToCpu:2;
    GT_U32 reserved:10;
}SNET_SAMBA_POLICY_FINAL_SETTING;

PRAGMA_ALIGN
#ifdef _VISUALC
#pragma pack()
#endif /* _VISUALC */


/*******************************************************************************
*   snetSambaPolicyEngine
*
* DESCRIPTION:
*        do the policy from the classifier Engine and the pcl Engine.
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
extern void snetSambaPolicyEngine
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

/*******************************************************************************
*   snetSambaPolicyDefaultActionsSet
*
* DESCRIPTION:
*       the function set default values to the input pointer
*
* INPUTS:
*
* OUTPUTS:
*       policyActionPtr - pointer to a structure that hold the actions in
*                       the policy format
*
* RETURNS:
*
* COMMENTS:  from 1.6 [5] (reset Value)
*
*
*******************************************************************************/
extern void snetSambaPolicyDefaultActionsSet(
    OUT SNET_SAMBA_POLICY_ACTION *policyActionPtr
);


/*******************************************************************************
*   snetSambaClassificationCheck
*
* DESCRIPTION:
*        check if need to enter the classifier
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*  Return: GT_TRUE if need to enter the classifier
*
*COMMENTS: Logic From 12.2  [1]
*******************************************************************************/
extern GT_BOOL snetSambaClassificationCheck
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

/*******************************************************************************
*   snetSambaClassificationEngine
*
* DESCRIPTION:
*        the function do the classifier engine to get an action info
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        policyActionPtr - hold reset values of this info
* OUTPUTS:
*        policyActionPtr - pointer to where to put the actions info
*        classifierFoundPtr - pointer to return if the gfc/ip found entry or
*                             the default entry was taken .
*
*******************************************************************************/
extern void snetSambaClassificationEngine
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    INOUT SNET_SAMBA_POLICY_ACTION *policyActionPtr,
    OUT   GT_BOOL *classifierFoundPtr
);

/*******************************************************************************
*   snetSambaClassificationKeyCreate
*
* DESCRIPTION:
*        create key for generic flow classification
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        searchKeyPtr - pointer to the search key .
*
* COMMENT:
*        from 12.5.1.1 [1]
*******************************************************************************/
extern GT_VOID snetSambaClassificationKeyCreate
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT   GT_U8                    searchKeyPtr[SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS]
);

/*******************************************************************************
*   snetSambaPclCheck
*
* DESCRIPTION:
*        check if need to enter the Pcl
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*  Return: GT_TRUE if need to enter the Pcl
*
*COMMENTS: Logic From 13.3.1  [1]
*******************************************************************************/
extern GT_BOOL snetSambaPclCheck
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);



/*******************************************************************************
*   snetSambaPCLEngine
*
* DESCRIPTION:
*        the function do the pcl engine to get an action info
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        policyActionPtr - hold reset values of this info
* OUTPUTS:
*        policyActionPtr - pointer to where to put the actions info
*        pclFoundFirstPtr - pointer to return if the first pcl found entry or
*                             the default entry was taken .
*        pclFoundSecondPtr - pointer to return if the second pcl found entry or
*                             the default entry was taken .
*
*COMMENTS: Logic From 1.2 [3]
*******************************************************************************/
extern void snetSambaPCLEngine
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    INOUT SNET_SAMBA_POLICY_ACTION *policyActionPtr,
    OUT   GT_BOOL *pclFoundFirstPtr,
    OUT   GT_BOOL *pclFoundSecondPtr
);


/*******************************************************************************
*   snetUtilGetContinuesValue
*
* DESCRIPTION:
*        the function get value of next continues fields .
* INPUTS:
*        deviceObj - pointer to device object.
*        startAddress  - need to be register address %4 == 0.
*        sizeofValue - how many bits this continues field is
*        indexOfField - what is the index of the field
* OUTPUTS:
*   none
*
* RETURN : the value of fields
*
*COMMENTS:
*******************************************************************************/
extern GT_U32 snetUtilGetContinuesValue(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 startAddress ,
    IN GT_U32 sizeofValue,
    IN GT_U32 indexOfField
);

/*******************************************************************************
*   snetSambaGfcFlowHashAddrGet
*
* DESCRIPTION:
*        function calculate hashAdrress from the hash parameters of the template .
*        the code is basically taken from the function : coreFlowHash
*        of PSS CORE
*
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        searchKeyPtr - pointer to the search key
*        policyActionPtr - hold reset values of this info
*        templIndex - index of template (major <<3 |minor)
* OUTPUTS:
*        policyActionPtr - pointer to where to put the actions info
*
* RETURNS:
*       hashAddr from where to get hash entry .
*
* COMMENT:
*
*******************************************************************************/
extern GT_U32 snetSambaGfcFlowHashAddrGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    SNET_SAMBA_FLOW_TEMPLATE_HASH  *hashInfoPtr,
    OUT   GT_U8                    searchKeyPtr[SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS],
    IN    GT_U32                   templIndex
);

/*******************************************************************************
*   snetSambaGfcFlowVlTriSearch
*
* DESCRIPTION:
*        The function get the hashEntryInfoPtr where there is collision and
*        look for the next entry address until no collitions or entry no valid
*        if found valid entry it set validVlTrieEntryPtrPtr to point to the entry
*         in the vl-trie
*
* INPUTS:
*        deviceObj - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        searchKeyPtr - pointer to the search key
*        policyActionPtr - hold reset values of this info
*        hashEntryInfoPtr - the pointer to the first entry after the hashing
*                           algorithm found that the entry has collision .
*
* OUTPUTS:
*        validVlTrieEntryPtr_Ptr - pointer to pointer pointer to the entry in
*                                  the vl-trie that holds the info to the floe entry
*
* RETURNS:
*       GT_TRUE - if entry found in the Vl-tri
*       GT_FALSE - otherwise
* COMMENT:
*       logic taken from 5.3 [2] called Search VL-trie algorithm.
*       And 5.8.4 [2] called Decode Entry Algorithm
*       non recursive implementation
*
*******************************************************************************/
extern GT_BOOL snetSambaGfcFlowVlTriSearch
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    SNET_SAMBA_CLASSIFIER_HASH_ENTRY *hashEntryInfoPtr,
    OUT   GT_U8                    searchKeyPtr[SNET_SAMBA_CLASSIFIER_KEY_SIZE_CNS],
    OUT   SNET_SAMBA_CLASSIFIER_VL_TRIE_ENTRY **validVlTrieEntryPtr_Ptr
);

/*******************************************************************************
* snetSambeGfcDiffBitVal
*
* DESCRIPTION:
*       Compute value of key on diff bits.
*
* INPUTS:
*   col - start offset.
*   len - number of bits.
*   pattern - flow pattern.
*
* OUTPUTS:
*   val - computed value.
*
* RETURNS:
*   val - computed value.
*
* COMMENTS:
*       the function is COPIED from the coreDiffBitVal(...) implementation
*******************************************************************************/
extern GT_U8 snetSambeGfcDiffBitVal
(
    GT_U8 col,
    GT_U8 len,
    GT_U8 pattern[]
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetSambaPolicyh */


