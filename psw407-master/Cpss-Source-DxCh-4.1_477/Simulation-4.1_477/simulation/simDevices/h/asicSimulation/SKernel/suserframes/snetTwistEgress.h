/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetTwistL2.h
*
* DESCRIPTION:
*       This is a external API definition for SMem module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 10 $
*
*******************************************************************************/
#ifndef __snetTwistEgressh
#define __snetTwistEgressh

#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/smain/smainSwitchFabric.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    GT_U8 bottomEncap;
    GT_U16 vlanId;
    GT_U8 daTag;
    GT_U8 useVidx;
    union {
        GT_U16 vidx;
        struct {
            GT_U8 trgIsTrunk;
            GT_U8 trgPort;
            GT_U8 trgDev;
        } port;
        struct {
            GT_U8 trgIsTrunk;
            GT_U8 trgTrunkId;
            GT_U16 trgTrunkHush;
        }trunk;
    } target;
} LLL_OUTLIF_STC;

typedef struct {
    GT_U32 copyExp;
} MPLS_OUTLIF_STC;

/* Multicast Link List */
typedef struct {
    struct {
        GT_U32 last;
        GT_U32 outLifType;
        GT_U32 outLif[2];
        union {
            LLL_OUTLIF_STC lll;
            MPLS_OUTLIF_STC mpls;
        } out_lif;
        GT_U32 ttlThres;
        GT_U32 excludeSrcVlan;
    } first_mll;
    struct {
        GT_U32 last;
        GT_U32 outLifType;
        GT_U32 outLif[2];
        union {
            LLL_OUTLIF_STC lll;
            MPLS_OUTLIF_STC mpls;
        } out_lif;
        GT_U32 ttlThres;
        GT_U32 excludeSrcVlan;
        GT_U32 nextPtr;
    } second_mll;
} SNET_MLL_STC;

#define RX_DESC_CPU_OWN     (0)

/* Sets the Own bit field of the rx descriptor */
/* Returns / Sets the Own bit field of the rx descriptor.           */
#define RX_DESC_GET_OWN_BIT(rxDesc) (((rxDesc)->word1) >> 31)

/* Return the buffer size field from the second word of an Rx desc. */
/* Make sure to set the lower 3 bits to 0.                          */
#define RX_DESC_GET_BUFF_SIZE_FIELD(rxDesc)             \
            (((((rxDesc)->word2) >> 3) & 0x7FF) << 3)

#define RX_DESC_SET_OWN_BIT(rxDesc, val)                     \
            (SMEM_U32_SET_FIELD((rxDesc)->word1,31,1,val))

/* Return the byte count field from the second word of an Rx desc.  */
/* Make sure to set the lower 3 bits to 0.                          */
#define RX_DESC_SET_BYTE_COUNT_FIELD(rxDesc, val)        \
            (SMEM_U32_SET_FIELD((rxDesc)->word2,16,14,val))


/*
 * Typedef: struct STRUCT_RX_DESC
 *
 * Description: Includes the PP Rx descriptor fields, to be used for handling
 *              received packets.
 *
 * Fields:
 *      word1           - First word of the Rx Descriptor.
 *      word2           - Second word of the Rx Descriptor.
 *      buffPointer     - The physical data-buffer address.
 *      nextDescPointer - The physical address of the next Rx descriptor.
 *
 */
typedef struct _rxDesc
{
    volatile GT_U32         word1;
    volatile GT_U32         word2;

    volatile GT_U32         buffPointer;
    volatile GT_U32         nextDescPointer;

}STRUCT_RX_DESC;

/*
 * Typedef: struct TIGER_VM_PORT_EGRESS_STC
 *
 * Description: Egress Virtual Port Configuration for Tiger.
 *
 * Fields:
 *      actualPort   - The actual transmit port of 98EX126 mapped for VM port
 *      dxPort       - The DX device transmit port mapped for VM port
 *      dxPortTagged - The DX device target port is VLAN-tagged
 *      addDsaTag    - The target VM port is mapped to cascade or non-cascade port
 *
 */
typedef struct {
    GT_U32 actualPort;
    GT_U32 dxPort;          
    GT_U32 dxPortTagged;
    GT_U32 addDsaTag; 
} TIGER_VM_PORT_EGRESS_STC;

/*******************************************************************************
*   snetTwistEgress
*
* DESCRIPTION:
*        Egress frame processing dispatcher.
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        descrPtr  - pointer to the frame's descriptor.
*
*******************************************************************************/
GT_VOID snetTwistEgress
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_DESCR_STC * descrPtr
);

/*******************************************************************************
*   snetTwistTx2Cpu
*
* DESCRIPTION:
*         Transfer frame to CPU by DMA
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
GT_BOOL snetTwistTx2Cpu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32 cpuCode
);

/*******************************************************************************
*   snetTwistTx2Device
*
* DESCRIPTION:
*         Tx mirroring processing
* INPUTS:
*        devObjPtr   -  pointer to device object.
*        descrPtr  -    pointer to the frame's descriptor.
*        frameDataPtr - frame data pointer
*        frameDataLength  - frame data length
*        txPort - TX port
*
*******************************************************************************/
GT_VOID snetTwistTx2Device
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U8_PTR frameDataPtr,
    IN GT_U32 frameDataLength,
    IN GT_U32 targetPort,
    IN GT_U32 targetDevice,
    IN GT_U32 cpuCode
);

GT_VOID sstackTwistToUplink
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
);

/*******************************************************************************
*   snetTwistLxOutBlock
*
* DESCRIPTION:
*        Form descriptor according to previous processing
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
GT_VOID  snetTwistLxOutBlock
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
);

/*******************************************************************************
*   snetTwistEqBlock
*
* DESCRIPTION:
*        Send descriptor to different egress  units
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
GT_VOID  snetTwistEqBlock
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr
);

/*******************************************************************************
*   snetTwistCpuTxFrameProcess
*
* DESCRIPTION:
*       Process frame sent from CPU accordingly to rules of TWISTD
*
* INPUTS:
*       deviceObj_PTR   - pointer to device object.
*       descr_PTR       - pointer to the frame's descriptor.
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
GT_VOID snetTwistCpuTxFrameProcess
(
    IN SKERNEL_DEVICE_OBJECT      *deviceObj_PTR,
    IN SKERNEL_FRAME_DESCR_STC    *descr_PTR
);

/*******************************************************************************
*   snetTwistPortEgressCount
*
* DESCRIPTION:
*       do port egress counting
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descr_PTR       - pointer to the frame's descriptor.
*       egressPort      - port that packet is transmitted to (can be CPU port)
*       egressCounter   - type of counting to perform
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID snetTwistPortEgressCount
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN GT_U32  egressPort,
    IN SKERNEL_EGRESS_COUNTER_ENT egressCounter
);

/*******************************************************************************
*   snetTwistProcessFrameFromUpLink
*
* DESCRIPTION:
*       Process frame from UPLINK
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       uplinkDesc      - pointer to the frame's descriptor.
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID snetTwistProcessFrameFromUpLink
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_UPLINK_DESC_STC * uplinkDesc
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetTwistEgressh */


