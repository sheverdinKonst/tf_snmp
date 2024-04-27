/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetTwistPcl.h
*
* DESCRIPTION:
*       Policy (PCL and IP-Classifier) processing for frame.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 5 $
*
*******************************************************************************/
#ifndef __snetTwistPclh
#define __snetTwistPclh

#include <asicSimulation/SKernel/smain/smain.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* TCP header size */
#define TCP_HEADER_SIZE                             (0x14)
/* TCP header size */
#define UDP_HEADER_SIZE                             (0x10)

typedef enum{
    SNET_POLICY_ACTION_PCE_E = 0,
    SNET_POLICY_ACTION_IP_TABLES_E,
    SNET_POLICY_ACTION_TEMPLATE_E,
    SNET_POLICY_ACTION_GFC_E
}SNET_POLICY_ACTION_SRC_ENT;

typedef enum{
    SKERNEL_PCL_COS_ENTRY_E = 0,
    SKERNEL_PCL_L2_COS_E,
    SKERNEL_PCL_DSCP_ENTRY_E,
    SKERNEL_PCL_PKT_E
}SNET_POLICY_TRAFFIC_MARK_ENT;

typedef enum{
    SKERNEL_PCL_NO_REDIRECT_E = 0,
    SKERNEL_PCL_RESERVED_E,
    SKERNEL_PCL_IPV4_FRWD_E,
    SKERNEL_PCL_OUTLIF_FRWD_E
}SNET_POLICY_REDIRECT_FLOW_ENT;

typedef enum{
    SKERNEL_PCL_ABS_E,
    SKERNEL_PCL_MAC_HEADER_E,
    SKERNEL_PCL_MPLS_HEADER_E,
    SKERNEL_PCL_L3_HEADER_E,
    SKERNEL_PCL_L4_HEADER_E,
    SKERNEL_PCL_L5_HEADER_E
}SNET_POLICY_OFFSET_TYPE_ENT;

typedef enum {
    SKERNEL_PCL_DROP_E,
    SKERNEL_PCL_PASS_NO_SEND_TC_E,
    SKERNEL_PCL_TRAP_E,
    SKERNEL_PCL_PASS_SEND_TC_E,
}SNET_POLICY_CMD_ENT;

/* Policy Control Entry Format
 *  cmd         -   0 - Drop the packet silently.
                    1 - Process the packet according to the classifier command,
                        however, do not send the packet for traffic conditioning.
                    2 - Trap the packet to the CPU.
                    3 - Process the packet according to the classifier command,
                        and send the packet for traffic conditioning

 *  analyzMirror -  0 - Do not send the packet to the Analyzer port
                    1 - Send the packet to the Analyzer port

 *  cpuMirrpor   -  0 - Do not mirror the flow to the CPU.
                    1 - Mirror the flow to the CPU.

    Traffic Marking - Valid only when Cmd is not DROP or TRAP

 *  markCmd     -   0 - Set packet.s CoS according to this entry's <DSCP> and
                        <CoS_Params> fields.
                    1 - Keep the CoS as it was received from the Layer 2 ingress
                        bridge processor.
                    2 - Set packet's CoS by mapping the entry's <DSCP> field to
                        CoS using the DSCP->CoS Table.
                    3 - Set packet.s CoS according to the incoming IPv4 DSCP or
                        MPLS EXP field and map it to CoS based on DSCP->CoS tab.

 *  dscp        -   The DSCP used for marking.

 *  tcCosParam  -   When <Cmd>=PROCESS_WITH_TRAFFIC_CONDITIONING, this is the
                    pointer to a traffic conditioning entry.
                    When <Cmd>=PROCESS_WITHOUT_TRAFFIC_CONDITIONING, then this
                    field contains the explicit CoS parameters:
                        Bits 14:13 = Drop Precedence[1:0]
                        Bits 17:15 = Traffic Class [2:0]
                        Bits 20:18 = 802.1p User Priority[0:2]
                        Bits 22:21 = Reserved (always .0.)

    Traffic Engineering - Valid only for Flows matched in the PCL table or
            Default Flow entries

 *  cmdRedirect -   Based on the lookup result, a packet can be forwarded to a
                    logical Interface.
                    0 - Do not perform traffic redirecting.
                    1 - Reserved
                    2 - Perform IPv4 Routing based on the route entry pointed
                        to by the PCR.
                    3 - Forward the packet via the Output Logical interface

 *  outLif          If <Redirect_Flow_Cmd> = FORWARD_PKT_USING_IPv4_NEXT_HOP,
                    this field contains the IPv4 next hop entry.
                    If <Redirect_Flow_Cmd> = FORWARD_PKT_USING_EXPLICIT_OUTLIF,
                    this field contains the Output Logical interface entry.


 *  setLbh          0 - Do not set the packet's LBH field
                    1 - Set the packet.s LBH field according to PCE<LBH>

 *  lbh             The Load Balancing Hash to be added to the packet that is
                    forwarded to the fabric interface.
*/

typedef struct{
    SNET_POLICY_CMD_ENT             cmd;
    GT_U32                          analyzMirror;
    GT_U32                          cpuMirror;
    SNET_POLICY_TRAFFIC_MARK_ENT    markCmd;
    GT_U32                          dscp;
    GT_U32                          tcCosParam;
    SNET_POLICY_REDIRECT_FLOW_ENT   cmdRedirect;
    GT_U32                          outLif;
    GT_U32                          setLbh;
    GT_U32                          lbh;
} SNET_POLICY_ACTION_STC;

#define SNET_TWIST_PCL_KEY_SIZE_CNS                 (18)
/* the number of bytes that are in the per template into are 20 */
#define SNET_MAX_NUM_BYTES_FOR_KEY_CNS              (20)

/* Flow Template Configuration Register
 *  keySize    -   The key size aligned to the byte maximum value is 20 bytes.
                    The key size includes the inLIF (Input Logical Interface)
                    and the flow template byte in case the <include InLIF> or
                    the <Include Flow Template> are active.

 *  incLif -        0 - The InLIF number should not be in the end of the key
                        (16 bit).
                    1 - The InLIF number should be in the end of the key (16 bit).

 *  incFlowTemp     0 - The flow template and sub-template will not be in the
                        end of the key.
                    1 - The flow template and sub-template will be in the end
                        of the key (after the LIF, if include LIF is enabled)
                        ({4h0, flow sub-template}).

 *  includeVid

 *  configTemp<n>   This field is the template configuration matching template
 *
 Flow Template Select Table Register
 *  offsetValid     0 - Byte0 is not valid.
                    1 - Byte0 offset type and offset fields are valid.
 *  offsetType      0 - The offset is from the start of packet.
                    1 - The offset is from the start of the MAC header (L2 header).
                    2 - The offset is from the start of the MPLS header.
                    3 - The offset is from the start of the L3 header. It is
                        always taken from the end of the MPLS header or the
                        L2 header or from the beginning of the packet if there
                        is no MPLS or L2.
                    4 - The offset is from the start of the L4 header,
                        regarding only the first header if a tunnel exist.
                    5 - The offset is from the end of the TCP/UDP header
                        (start L5).
                    6-7 = Reserved.
 *  offsetValue
*/
typedef struct{
    GT_U32                          keySize;
    GT_U32                          includeLif;
    GT_U32                          incFlowTemp;
    GT_U32                          includeVid;
    GT_U32                          offsetValid[SNET_MAX_NUM_BYTES_FOR_KEY_CNS];
    GT_U32                          offsetType[SNET_MAX_NUM_BYTES_FOR_KEY_CNS];
    GT_U32                          offsetValue[SNET_MAX_NUM_BYTES_FOR_KEY_CNS];
    GT_U8                           byteMask[SNET_MAX_NUM_BYTES_FOR_KEY_CNS];
}SNET_TWIST_TEMPLATE_STC;

/*******************************************************************************
*  snetTwistPolicyProcess
*
* DESCRIPTION:
*      Policy (PCL and IP-Classifier) processing for frame
* INPUTS:
*      deviceObjPtr   - pointer to device object.
*      deviceAddr     - address of XAUI device.
*
* OUTPUTS:
*
* RETURNS:
*      TRUE - continue frame processing, FALSE - stop frame processing
* COMMENTS:
*
*******************************************************************************/
GT_BOOL snetTwistPolicyProcess
(
    IN SKERNEL_DEVICE_OBJECT      *     deviceObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt
);

/*******************************************************************************
*  snetTwistGetFlowTemplate
*
* DESCRIPTION:
*      GET flow template
* INPUTS:
*      devObjPtr     - pointer to device object.
*      descrPtr         - pointer to the frame's descriptor.
*      frameTemplate  - pointer to flow template to be filled
* OUTPUTS:
*
*      frameTemplate  - pointer to flow template
*      searchKey[]    - PCL search key
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetTwistGetFlowTemplate
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    INOUT SNET_TWIST_TEMPLATE_STC *     frameTemplate
);

/*******************************************************************************
*  snetTwistPclKeyCreate
*
* DESCRIPTION:
*      Build search key from frame and templates
* INPUTS:
*      devObjPtr     - pointer to device object.
*      descrPtr         - pointer to the frame's descriptor.
*
* OUTPUTS:
*      searchKey[]    - PCL search key
*
* RETURNS:
*
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetTwistPclKeyCreate
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    OUT GT_U8  searchKey[SNET_TWIST_PCL_KEY_SIZE_CNS]
);

/*******************************************************************************
*  snetTwistCosTablesLookup
*
* DESCRIPTION:
*      Make search in the IP Classifier and get corespondent entry
* INPUTS:
*      devObjPtr     - pointer to device object.
*      descrPtr         - pointer to the frame's descriptor.
*      policyActionPtr  - pointer to policy action entry
* OUTPUTS:
*
*      policyActionPtr  - pointer to policy action entry
*
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_BOOL snetTwistCosTablesLookup
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    INOUT SNET_POLICY_ACTION_STC  *     policyActionPtr
);

/*******************************************************************************
*  snetTwisDefTemplActionGet
*
* DESCRIPTION:
*      Get final action from PCEs action
* INPUTS:
*      devObjPtr     - pointer to device object.
*      descrPtr         - pointer to the frame's descriptor.
*      policyActionPtr  - pointer to policy action entry
* OUTPUTS:
*
*      policyActionPtr  - pointer to policy action entry
*
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetTwisDefTemplActionGet
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    INOUT SNET_POLICY_ACTION_STC  *     policyActionPtr
);

/*******************************************************************************
*   snetTwistPclRxMirror
*
* DESCRIPTION:
*       Make RX mirroring
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*
*******************************************************************************/
GT_VOID snetTwistPclRxMirror
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

/*******************************************************************************
*  snetTwistPolicyMark
*
* DESCRIPTION:
*      Mark CoS atributes of frame
* INPUTS:
*      devObjPtr     - pointer to device object.
*      descrPtr         - pointer to the frame's descriptor.
*      policyActionPtr  - pointer to policy action entry
* OUTPUTS:
*
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetTwistPolicyMark
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC    *     descrPrt,
    IN SNET_POLICY_ACTION_STC     *     policyActionPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetTwistPclh */


