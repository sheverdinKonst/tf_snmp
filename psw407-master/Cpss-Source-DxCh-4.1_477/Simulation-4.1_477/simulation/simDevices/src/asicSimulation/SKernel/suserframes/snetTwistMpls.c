/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetTwistMpls.c
*
* DESCRIPTION:
*      This is a external API definition for MPLS Interface of SKernel.
*
* DEPENDENCIES:
*      None.
*
* FILE REVISION NUMBER:
*      $Revision: 7 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/suserframes/snetTwistMpls.h>
#include <asicSimulation/SKernel/suserframes/snetTwistL2.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/twistCommon/sregTwist.h>
#include <asicSimulation/SKernel/smem/smemTwist.h>
#include <asicSimulation/SLog/simLog.h>

#define NHLFE_SIZE_BYTE         (16)
#define MPLS_IF_SIZE_BYTE       (8)
#define WRAM_ADDR_ALLIGN_CNS    (4)

static GT_BOOL snetTwistMplsIngress
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_VOID snetTwistMplsLookUp
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_VOID snetTwistMplsTblIfEntryGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    INOUT MPLS_INTERFACE_TABLE_STC * mplsTblIfPtr
);

static GT_STATUS snetTwistIfUpdateLookUp
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    GT_U16 inLifNumber
);

static GT_VOID snetTwistMplsIlmEntryGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT   NHLF_FORWARD_ENTRY_STC * nhlfePtr
);

static GT_VOID snetTwistMplsOutContext
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_BOOL snetTwistMplsTtlGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT   GT_U8   * ttlPtr
);

static GT_VOID snetTwistMplsCmdSet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_VOID snetTwistMplsCosSet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

static GT_BOOL snetTwistMplsCounters
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
);

#define TWIST_NHLFE_FAKE(_nhlfe)                \
{                                               \
    memset(&_nhlfe, 0, sizeof(_nhlfe));       \
    _nhlfe.pktCmd = NHLFE_CMD_DROP_E;           \
    _nhlfe.passCib = 0;                         \
    _nhlfe.forceCos = 0;                        \
    _nhlfe.mngCountSet = NHLFE_MNG_COUNT_NONE;  \
    _nhlfe.decrTtl = 0;                         \
}

#define TWIST_GET_NHLFE_NEXT_HOP(ilm_ptr, _next_hop)                \
{                                                                   \
    _next_hop.mtu =                                                 \
            (GT_U32)WORD_FIELD_GET(ilm_ptr, 0, 31, 1) |             \
            (GT_U32)WORD_FIELD_GET(ilm_ptr, 1, 0, 13) << 1;         \
    _next_hop.outLifType =                                          \
            (GT_U8) WORD_FIELD_GET(ilm_ptr, 1, 13, 1);              \
    /* Output Logical Interface is Link-Layer OutLif */             \
    if (_next_hop.outLifType == 0)                                  \
    {                                                               \
        _next_hop.out_lif.link_layer.LL_OutLIF =                    \
            (GT_U32)WORD_FIELD_GET(ilm_ptr, 1, 14, 18) |            \
            (GT_U32)WORD_FIELD_GET(ilm_ptr, 2,  0, 12) << 18 ;      \
        _next_hop.out_lif.link_layer.arpPtr =                       \
            (GT_U32)WORD_FIELD_GET(ilm_ptr, 2, 12, 14);             \
    }                                                               \
    else                                                            \
    {                                                               \
        _next_hop.out_lif.tunnel_outlif.tunnelOutLifLo =            \
            (GT_U32)WORD_FIELD_GET(ilm_ptr, 1, 14, 18) |            \
            (GT_U32)WORD_FIELD_GET(ilm_ptr, 2,  0, 14) << 18;       \
        _next_hop.out_lif.tunnel_outlif.tunnelOutLifHi =            \
            (GT_U32)WORD_FIELD_GET(ilm_ptr, 2, 14, 10);             \
    }                                                               \
}
#define TWIST_GET_NHLFE_SWAP_CMD(ilm_ptr, _cmd_swap)                \
{                                                                   \
    TWIST_GET_NHLFE_NEXT_HOP(ilm_ptr,                               \
                            _cmd_swap.nextHopEntry);                \
    _cmd_swap.expSet =                                              \
        (GT_U8) WORD_FIELD_GET(ilm_ptr, 2, 31, 1);                  \
    _cmd_swap.label =                                               \
        (GT_U32)WORD_FIELD_GET(ilm_ptr, 3, 0, 20);                  \
    _cmd_swap.exp =                                                 \
        (GT_U8) WORD_FIELD_GET(ilm_ptr, 3, 20, 3);                  \
    _cmd_swap.ttl =                                                 \
        (GT_U8) WORD_FIELD_GET(ilm_ptr, 3, 23, 8);                  \
}
#define TWIST_GET_NHLFE_PUSH_CMD(ilm_ptr, _cmd_push)                \
{                                                                   \
    TWIST_GET_NHLFE_NEXT_HOP(ilm_ptr,                               \
                            _cmd_push.nextHopEntry);                \
    _cmd_push.expSet =                                              \
        (GT_U8) WORD_FIELD_GET(ilm_ptr, 2, 31, 1);                  \
    _cmd_push.label =                                               \
        (GT_U32)WORD_FIELD_GET(ilm_ptr, 3, 0, 20);                  \
    _cmd_push.exp =                                                 \
        (GT_U8) WORD_FIELD_GET(ilm_ptr, 3, 20, 3);                  \
    _cmd_push.ttl =                                                 \
        (GT_U8) WORD_FIELD_GET(ilm_ptr, 3, 23, 8);                  \
    _cmd_push.l2ce =                                                \
        (GT_U8) WORD_FIELD_GET(ilm_ptr, 3, 31, 1);                  \
}
#define TWIST_GET_NHLFE_POP(ilm_ptr, _cmd_pop)                      \
{                                                                   \
    _cmd_pop.vrId =                                                 \
        (GT_U16)WORD_FIELD_GET(ilm_ptr, 0, 31, 1) |                 \
        (GT_U16)WORD_FIELD_GET(ilm_ptr, 1, 0, 13) << 1;             \
    _cmd_pop.inLif =                                                \
        (GT_U16)WORD_FIELD_GET(ilm_ptr, 1, 13, 16);                 \
    _cmd_pop.overVrId =                                             \
        (GT_U8) WORD_FIELD_GET(ilm_ptr, 1, 29, 1);                  \
    _cmd_pop.overInLif =                                            \
        (GT_U8) WORD_FIELD_GET(ilm_ptr, 1, 30, 1);                  \
    _cmd_pop.nlp =                                                  \
        (GT_U8) WORD_FIELD_GET(ilm_ptr, 2, 26, 3);                  \
    _cmd_pop.popCpyDscpExp =                                        \
        (GT_U8) WORD_FIELD_GET(ilm_ptr, 2, 27, 1);                  \
    _cmd_pop.popCpyTtl =                                            \
        (GT_U8) WORD_FIELD_GET(ilm_ptr, 2, 28, 1);                  \
}
#define TWIST_GET_NHLFE_PENULT_POP(ilm_ptr, _cmd_pnlt)              \
{                                                                   \
    TWIST_GET_NHLFE_NEXT_HOP(ilm_ptr,                               \
                            _cmd_pnlt.nextHopEntry);                \
    _cmd_pnlt.nlp =                                                 \
        (GT_U8) WORD_FIELD_GET(ilm_ptr, 2, 26, 3);                  \
    _cmd_pnlt.popCpyDscpExp =                                       \
        (GT_U8) WORD_FIELD_GET(ilm_ptr, 2, 29, 1);                  \
    _cmd_pnlt.popCpyTtl =                                           \
        (GT_U8) WORD_FIELD_GET(ilm_ptr, 2, 30, 1);                  \
}


static MPLS_LOOKUP_INFO_STC mplsLookUpInfo;
static NHLF_FORWARD_ENTRY_STC  nhlfeEntry;

typedef enum {
    NHLFE_CMD_DROP_E,
    NHLFE_CMD_CPU_TRAP_E,
    NHLFE_CMD_SWITCH_E,
    NHLFE_CMD_TRANS_PASS
} NHLFE_CMD_STC;

typedef enum {
    NHLFE_MPLS_CMD_SWAP_E,
    NHLFE_MPLS_CMD_PUSH_E,
    NHLFE_MPLS_CMD_POP_E,
    NHLFE_MPLS_CMD_PENULT_E
} NHLFE_MPLS_CMD_STC;

typedef enum {
    NHLFE_MNG_COUNT_0,
    NHLFE_MNG_COUNT_1,
    NHLFE_MNG_COUNT_2,
    NHLFE_MNG_COUNT_NONE
} NHLFE_MNG_COUNT_E;

typedef enum {
    DONE_STATUS_DO_LOOKUP_E,
    DONE_STATUS_NO_LOOKUP_E,
    DONE_STATUS_DROP_E,
    DONE_STATUS_TTL_ZERO_E,
    DONE_STATUS_INVALID_ENT_E,
    DONE_STATUS_ILLEGAL_POP_E
} DONE_STATUS_REG_E;

#define MPLS_MAX_LOOKUP         2

/*******************************************************************************
*   snetTwistMplsProcess
*
* DESCRIPTION:
*        make MPLS processing
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
)
{
    if (!descrPtr->doMpls ||
        (descrPtr->pktCmd == SKERNEL_PKT_DROP_E ||
         descrPtr->pktCmd == SKERNEL_PKT_TRAP_CPU_E))
    {
        descrPtr->mplsDone = 0;
        return GT_TRUE;
    }

    snetTwistMplsIngress(devObjPtr, descrPtr);
    snetTwistMplsLookUp(devObjPtr, descrPtr);
    snetTwistMplsOutContext(devObjPtr, descrPtr);

    return snetTwistMplsCounters(devObjPtr, descrPtr);
}

/*******************************************************************************
*   snetTwistMplsIngress
*
* DESCRIPTION:
*        determine the Label Switched Path (LSP) in which to forward the packet
         over the MPLS domain
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        GT_TRUE   - continue frame processing
*        GT_FALSE  - stop frame processing
*******************************************************************************/
static GT_BOOL snetTwistMplsIngress
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistMplsIngress);


    memset(&mplsLookUpInfo, 0, sizeof(MPLS_LOOKUP_INFO_STC));

    /* Phase - sets the first lookup phase to be a
       Interface Entry lookup or an ILM lookup */
    if (descrPtr->flowRedirectCmd == SKERNEL_FLOW_MPLS_REDIRECT_E)
    {
        mplsLookUpInfo.phase = 1;
    }
    else
    {
        mplsLookUpInfo.phase = 0;
    }
    /* Done Status */
    __LOG(("Done Status"));
    if (descrPtr->pktCmd != SKERNEL_PKT_DROP_E    &&
        descrPtr->pktCmd != SKERNEL_PKT_TRAP_CPU_E)
    {
        if (descrPtr->flowRedirectCmd == SKERNEL_FLOW_MPLS_REDIRECT_E &&
            descrPtr->ttl == 0)
        {
            mplsLookUpInfo.doneStatus = DONE_STATUS_NO_LOOKUP_E;
        }
        else
        if (descrPtr->flowRedirectCmd == SKERNEL_FLOW_MPLS_REDIRECT_E ||
            descrPtr->doMpls == 1)
        {
            mplsLookUpInfo.doneStatus = DONE_STATUS_DO_LOOKUP_E;
        }
        else
        {
            mplsLookUpInfo.doneStatus = DONE_STATUS_NO_LOOKUP_E;
        }
    }
    else
    {
            mplsLookUpInfo.doneStatus = DONE_STATUS_NO_LOOKUP_E;
    }

    if (mplsLookUpInfo.doneStatus == DONE_STATUS_NO_LOOKUP_E)
    {
        return GT_FALSE;
    }

    mplsLookUpInfo.priority = descrPtr->trafficClass;
    mplsLookUpInfo.dp = descrPtr->dropPrecedence;
    mplsLookUpInfo.vpt = descrPtr->userPriorityTag;
    mplsLookUpInfo.inLif = descrPtr->inLifNumber;

    /* Fill label info */
    mplsLookUpInfo.labelInfo[0].label = descrPtr->mplsLabels[0].label;
    mplsLookUpInfo.labelInfo[0].exp = descrPtr->mplsLabels[0].exp;
    mplsLookUpInfo.labelInfo[0].sbit = descrPtr->mplsLabels[0].sbit;
    mplsLookUpInfo.labelInfo[0].ttl = descrPtr->mplsLabels[0].ttl;

    mplsLookUpInfo.labelInfo[1].label = descrPtr->mplsLabels[1].label;
    mplsLookUpInfo.labelInfo[1].exp = descrPtr->mplsLabels[1].exp;
    mplsLookUpInfo.labelInfo[1].sbit = descrPtr->mplsLabels[1].sbit;
    mplsLookUpInfo.labelInfo[1].ttl = descrPtr->mplsLabels[1].ttl;
    /* Lookup begins with label 0 */
    mplsLookUpInfo.labelNum = 0;

    return GT_TRUE;
}

/*******************************************************************************
*   snetTwistMplsLookUp
*
* DESCRIPTION:
*        Resolving the NHLFE for a given packet sent to the MPLS engine
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
*        GT_TRUE   - continue frame processing
*        GT_FALSE  - stop frame processing
*******************************************************************************/
static GT_VOID snetTwistMplsLookUp
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistMplsLookUp);

    GT_U32 lbIdx, baseAddr;
    MPLS_LABELINFO_STC * mplsLabelPtr;
    GT_U32 fldValue;

    if (mplsLookUpInfo.doneStatus != DONE_STATUS_DO_LOOKUP_E)
        return;


    lbIdx = mplsLookUpInfo.labelNum;
    mplsLabelPtr = &mplsLookUpInfo.labelInfo[lbIdx];

    if (mplsLookUpInfo.phase == 0)
    {
        /* For reserved labels do not lookup label interface table */
        __LOG(("For reserved labels do not lookup label interface table"));
        if (mplsLabelPtr->label <= 15)
        {
            smemRegFldGet(devObjPtr, RSRV_LABEL_BASE_ADDR_REG, 0, 21,
                          &fldValue);
            baseAddr = 0x28000000 + (fldValue << WRAM_ADDR_ALLIGN_CNS);
            /* label + BaseAddress */
            mplsLookUpInfo.lookUpAddress =
                baseAddr + mplsLabelPtr->label * NHLFE_SIZE_BYTE;
        }
        else
        {
            /* Update lookup address */
            __LOG(("Update lookup address"));
            snetTwistIfUpdateLookUp(devObjPtr, descrPtr,
                descrPtr->inLifNumber);
        }
        mplsLookUpInfo.phase = 1;
    }

    /* Get NHLFE entry */
    __LOG(("Get NHLFE entry"));
    snetTwistMplsIlmEntryGet(devObjPtr, descrPtr, &nhlfeEntry);

    if (nhlfeEntry.pktCmd == NHLFE_CMD_SWITCH_E &&
        nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_POP_E &&
        nhlfeEntry.mplsInfo.cmd_pop.nlp == NHLFE_NLP_MPLS_E)
    {
        if (mplsLabelPtr->sbit)
        {
            mplsLookUpInfo.doneStatus = DONE_STATUS_ILLEGAL_POP_E;
            return;
        }

        if (mplsLookUpInfo.labelNum < MPLS_MAX_LOOKUP)
        {
            /* Update lookup address */
            snetTwistIfUpdateLookUp(devObjPtr, descrPtr,
                nhlfeEntry.mplsInfo.cmd_pop.inLif);

            mplsLookUpInfo.labelNum = mplsLookUpInfo.labelNum + 1;
            mplsLookUpInfo.phase = 0;
            mplsLabelPtr->sbit = mplsLookUpInfo.labelInfo[lbIdx + 1].sbit;
            /* If a second or more lookups are done
            - [68:67] Mg Counter */
            mplsLookUpInfo.labelInfo[0].label &= ~(0x3 << 18);
            mplsLookUpInfo.labelInfo[0].label |= nhlfeEntry.mngCountSet << 18;
            if (nhlfeEntry.mplsInfo.cmd_pop.popCpyTtl == 0)
            {
                GT_U8  ttl_nxt;
                ttl_nxt = mplsLookUpInfo.labelInfo[lbIdx + 1].ttl;
                if (ttl_nxt != 0)
                {
                    mplsLookUpInfo.labelInfo[0].ttl =
                        ttl_nxt - nhlfeEntry.decrTtl;
                }
                else
                {
                    mplsLookUpInfo.labelInfo[0].ttl = 0;
                }
            }
            else
            {
                GT_U8  ttl_cur;
                ttl_cur = mplsLabelPtr->ttl;
                if (mplsLabelPtr->ttl)
                {
                    mplsLookUpInfo.labelInfo[0].ttl =
                        ttl_cur - nhlfeEntry.decrTtl;
                }
            }
            if (nhlfeEntry.mplsInfo.cmd_pop.popCpyDscpExp == 0)
            {
                mplsLabelPtr->exp = mplsLookUpInfo.labelInfo[lbIdx + 1].exp;
                mplsLookUpInfo.remarkCos = 1;
            }
            if (nhlfeEntry.mplsInfo.cmd_pop.overVrId == 1)
            {
                /* If a second or more lookups are done [50] Set VR-ID */
                mplsLookUpInfo.labelInfo[0].label |= 1;
                /* If a second or more lookups are done [66:51] - VR-ID */
                mplsLookUpInfo.labelInfo[0].label &= ~(0xFFFF << 1);
                mplsLookUpInfo.labelInfo[0].label |=
                    nhlfeEntry.mplsInfo.cmd_pop.vrId << 1;
            }
            if (nhlfeEntry.mplsInfo.cmd_pop.overInLif)
            {
                mplsLookUpInfo.inLif = nhlfeEntry.mplsInfo.cmd_pop.inLif;
            }
            snetTwistMplsLookUp(devObjPtr, descrPtr);
        }
        /* Default PenUltimate Command Registers */
        if (mplsLookUpInfo.labelNum == MPLS_MAX_LOOKUP)
        {
            smemRegGet(devObjPtr, DFLT_PENULT_CMD_0_ADDR_REG, &fldValue);
            nhlfeEntry.mplsInfo.defCmdEntry[0] = fldValue;

            smemRegGet(devObjPtr, DFLT_PENULT_CMD_1_ADDR_REG, &fldValue);
            nhlfeEntry.mplsInfo.defCmdEntry[1] = fldValue;
            nhlfeEntry.mplsCmd = NHLFE_MPLS_CMD_PENULT_E;
            return;
        }
    }
    else
    if (nhlfeEntry.pktCmd == NHLFE_CMD_SWITCH_E)
    {
        if (nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_PENULT_E &&
            nhlfeEntry.mplsInfo.cmd_pop.nlp == NHLFE_NLP_MPLS_E &&
            mplsLabelPtr->sbit)
        {
            mplsLookUpInfo.doneStatus = DONE_STATUS_ILLEGAL_POP_E;
            return;
        }
        else
        if ((nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_PENULT_E ||
            nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_POP_E) &&
            nhlfeEntry.mplsInfo.cmd_pop.nlp != NHLFE_NLP_MPLS_E &&
            mplsLabelPtr->sbit == 0)
        {
            mplsLookUpInfo.doneStatus = DONE_STATUS_ILLEGAL_POP_E;
            return;
        }
        else
        if (nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_POP_E &&
            nhlfeEntry.mplsInfo.cmd_pop.nlp != NHLFE_NLP_MPLS_E)
        {
            if (nhlfeEntry.mplsInfo.cmd_pop.overVrId == 1)
            {
                /* Set bit 0 (Set VR-ID) */
                mplsLookUpInfo.labelInfo[0].label |= 1;
                /* Set bits 1-16 (VR-ID) */
                mplsLookUpInfo.labelInfo[0].label &= ~(0xFFFF << 1);
                mplsLookUpInfo.labelInfo[0].label |=
                    nhlfeEntry.mplsInfo.cmd_pop.vrId << 1;
            }
            if (nhlfeEntry.mplsInfo.cmd_pop.overInLif)
            {
                mplsLookUpInfo.inLif = nhlfeEntry.mplsInfo.cmd_pop.inLif;
            }
        }
        /* CoS assignment */
        if (nhlfeEntry.forceCos == 1)
        {
            mplsLookUpInfo.priority = nhlfeEntry.trafficClass;
            mplsLookUpInfo.dp = nhlfeEntry.dp;
            mplsLookUpInfo.vpt = nhlfeEntry.userPriority;
        }
        /* Do EPI */
        if (nhlfeEntry.passCib == 1)
        {
            mplsLookUpInfo.doEpi = 1;
        }
    }

    return;
}

/*******************************************************************************
*   snetTwistMplsTblIfEntryGet
*
* DESCRIPTION:
*        Fetching the MPLS table interface entry
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*
* OUTPUTS:
*        mplsTblIfPtr - pointer to MPLS table interface entry
*
*******************************************************************************/
static GT_VOID snetTwistMplsTblIfEntryGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    INOUT MPLS_INTERFACE_TABLE_STC * mplsTblIfPtr
)
{
    DECLARE_FUNC_NAME(snetTwistMplsTblIfEntryGet);

    GT_U32 memAddr, baseAddr, fldValue;
    GT_U32 * labelIfEntryPtr;


    smemRegFldGet(devObjPtr, MPLS_IF_TABL_BASE_ADDR_REG, 0, 21, &fldValue);
    /* InterfaceTable BaseAddress */
    __LOG(("InterfaceTable BaseAddress"));
    baseAddr = 0x28000000 + (fldValue << WRAM_ADDR_ALLIGN_CNS);
    memAddr = baseAddr + mplsTblIfPtr->inLifNumber * MPLS_IF_SIZE_BYTE;

    /* If '1' the interface entry reside in the MSB bits of the data [127:64] */
    if (memAddr & 0x01)
    {
        memAddr += 0x8;
    }
    /* MPLS Interface Entry */
    labelIfEntryPtr = smemMemGet(devObjPtr, memAddr);

    mplsTblIfPtr->valid =
        (GT_U8)WORD_FIELD_GET(labelIfEntryPtr, 0, 0, 1);
    /* The minimal label to accept on this interface */
    mplsTblIfPtr->minLabel =
        (GT_U8)WORD_FIELD_GET(labelIfEntryPtr, 0, 4, 20);
    /* Valid values are from 0 to 12 */
    mplsTblIfPtr->size0 =
        (GT_U8)WORD_FIELD_GET(labelIfEntryPtr, 0, 24, 4);
    /* The label space size is Size1 << Size 0  */
    mplsTblIfPtr->size1 =
        (GT_U8)(WORD_FIELD_GET(labelIfEntryPtr, 0, 28, 4)<<4) |
        (GT_U8) WORD_FIELD_GET(labelIfEntryPtr, 1, 0, 4);
    /* The base address points to the entry having the Min_Label */
    mplsTblIfPtr->baseAddress =
        (GT_U32)WORD_FIELD_GET(labelIfEntryPtr, 1, 4, 23);
}

/*******************************************************************************
*   snetTwistMplsIlmEntryGet
*
* DESCRIPTION:
*        Fetching the ILM table entry
*
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*
* OUTPUTS:
*        nhlfePtr - pointer Next-Hop Label Forwarding Entry
*
*******************************************************************************/
static GT_VOID snetTwistMplsIlmEntryGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT   NHLF_FORWARD_ENTRY_STC * nhlfePtr
)
{
    GT_U32 * ilmPtr;

    /* Fetch ILM table entry */
    ilmPtr = smemMemGet(devObjPtr, mplsLookUpInfo.lookUpAddress);
    /* word 0 */
    nhlfePtr->pktCmd =
        (GT_U8) WORD_FIELD_GET(ilmPtr, 0, 0, 2);
    nhlfePtr->passCib =
        (GT_U8) WORD_FIELD_GET(ilmPtr, 0, 2, 1);
    nhlfePtr->forceCos =
        (GT_U8) WORD_FIELD_GET(ilmPtr, 0, 3, 1);
    nhlfePtr->trafficClass =
        (GT_U8) WORD_FIELD_GET(ilmPtr, 0, 4, 3);
    nhlfePtr->dp =
        (GT_U8) WORD_FIELD_GET(ilmPtr, 0, 7, 2);
    nhlfePtr->userPriority =
        (GT_U8) WORD_FIELD_GET(ilmPtr, 0, 9, 3);
    nhlfePtr->mngCountSet =
        (GT_U8) WORD_FIELD_GET(ilmPtr, 0, 12, 2);
    nhlfePtr->decrTtl =
        (GT_U8) WORD_FIELD_GET(ilmPtr, 0, 14, 1);
    nhlfePtr->mplsCmd =
        (GT_U8) WORD_FIELD_GET(ilmPtr, 0, 15, 2);

    switch (nhlfePtr->mplsCmd)
    {
    case NHLFE_MPLS_CMD_SWAP_E:
        TWIST_GET_NHLFE_SWAP_CMD(ilmPtr, nhlfePtr->mplsInfo.cmd_swap);
    break;
    case NHLFE_MPLS_CMD_PUSH_E:
        TWIST_GET_NHLFE_PUSH_CMD(ilmPtr, nhlfePtr->mplsInfo.cmd_push);
    break;
    case NHLFE_MPLS_CMD_POP_E:
        TWIST_GET_NHLFE_POP(ilmPtr, nhlfePtr->mplsInfo.cmd_pop);
    break;
    case NHLFE_MPLS_CMD_PENULT_E:
        TWIST_GET_NHLFE_PENULT_POP(ilmPtr, nhlfePtr->mplsInfo.cmd_pnlt_pop);
    break;
    }
}

/*******************************************************************************
*   snetTwistMplsOutContext
*
* DESCRIPTION:
*        The outgoing context assembly block
*
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*
* OUTPUTS:
*
*******************************************************************************/
static GT_VOID snetTwistMplsOutContext
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistMplsOutContext);

    GT_BOOL badTtl;
    GT_U8  ttl;

    snetTwistMplsCmdSet(devObjPtr, descrPtr);
    if (nhlfeEntry.pktCmd == NHLFE_CMD_DROP_E ||
        nhlfeEntry.pktCmd == NHLFE_CMD_CPU_TRAP_E)
    {
        descrPtr->doHa = 0;
        return;
    }

    /* Do IPV4 */
    __LOG(("Do IPV4"));
    if (mplsLookUpInfo.doneStatus == DONE_STATUS_DO_LOOKUP_E &&
       nhlfeEntry.pktCmd == NHLFE_CMD_SWITCH_E &&
       nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_POP_E &&
       nhlfeEntry.mplsInfo.cmd_pop.nlp == NHLFE_NLP_IPV4_E)
    {
        descrPtr->doRout = 1;
    }

    /* Done MPLS */
    __LOG(("Done MPLS"));
    if (mplsLookUpInfo.doneStatus == DONE_STATUS_DO_LOOKUP_E &&
        nhlfeEntry.pktCmd == NHLFE_CMD_TRANS_PASS)
    {
        descrPtr->mplsDone = 0;
    }
    else
    {
        descrPtr->mplsDone = descrPtr->doMpls;
    }

    if (descrPtr->mplsDone)
    {
        descrPtr->doHa = 1;
    }

    /* CoS and DSCP Assignments */
    __LOG(("CoS and DSCP Assignments"));
    if (mplsLookUpInfo.doneStatus == DONE_STATUS_DO_LOOKUP_E)
    {
        if (descrPtr->pktCmd == SKERNEL_PKT_TRAP_CPU_E ||
            nhlfeEntry.forceCos)
        {
            snetTwistMplsCosSet(devObjPtr, descrPtr);
        }
    }
    if ((mplsLookUpInfo.doneStatus == DONE_STATUS_DO_LOOKUP_E) &&
        (nhlfeEntry.pktCmd == NHLFE_CMD_SWITCH_E) &&
        ((nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_POP_E) ||
         (nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_PENULT_E) ||
         (nhlfeEntry.mplsInfo.cmd_pop.popCpyDscpExp == 0)))
    {
        descrPtr->modifyDscpOrExp = 0;
    }
    else
    {
        descrPtr->modifyDscpOrExp = 1;
    }
    /* EXP */
    __LOG(("EXP"));
    if (mplsLookUpInfo.doneStatus == DONE_STATUS_DO_LOOKUP_E &&
        nhlfeEntry.pktCmd == NHLFE_CMD_SWITCH_E &&
       (nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_PUSH_E ||
        nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_SWAP_E) &&
        nhlfeEntry.mplsInfo.cmd_push.expSet == 1)
    {
        descrPtr->exp = nhlfeEntry.mplsInfo.cmd_push.expSet;
    }
    else
    {
        descrPtr->exp = mplsLookUpInfo.labelInfo[0].exp;
    }
    /* Copy TTL */
    __LOG(("Copy TTL"));
    if (mplsLookUpInfo.doneStatus == DONE_STATUS_DO_LOOKUP_E &&
        nhlfeEntry.pktCmd == NHLFE_CMD_SWITCH_E &&
       (nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_POP_E ||
        nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_PENULT_E) &&
        nhlfeEntry.mplsInfo.cmd_pop.popCpyTtl == 0)
    {
        descrPtr->copyTtl = 0;
    }
    else
    {
        descrPtr->copyTtl = 1;
    }
    /* TTL */
    __LOG(("TTL"));
    badTtl = snetTwistMplsTtlGet(devObjPtr, descrPtr, &ttl);
    if (badTtl == GT_FALSE)
    {
        descrPtr->ttl = ttl;
    }

    if (mplsLookUpInfo.doneStatus == DONE_STATUS_DO_LOOKUP_E &&
        nhlfeEntry.pktCmd == NHLFE_CMD_SWITCH_E)
    {
        if (nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_POP_E ||
           nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_PENULT_E)
        {
            if (nhlfeEntry.mplsInfo.cmd_pop.popCpyTtl)
            {
                descrPtr->decTtl = nhlfeEntry.decrTtl;
            }
            else
            {
                descrPtr->decTtl = 0;
            }
        }
        else
        {
            descrPtr->decTtl = 0;
        }
    }
    else
    {
        descrPtr->decTtl = 0;
    }
    /* InLif, VR-Id */
    if (mplsLookUpInfo.doneStatus == DONE_STATUS_DO_LOOKUP_E &&
        nhlfeEntry.pktCmd == NHLFE_CMD_SWITCH_E &&
        nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_POP_E)
    {
        descrPtr->inLifNumber = mplsLookUpInfo.inLif;
        descrPtr->setVrId =
            (GT_U8)mplsLookUpInfo.labelInfo[0].label & 0x1;
        descrPtr->vrId =
            (GT_U16)(mplsLookUpInfo.labelInfo[0].label >> 1) & 0xFFFF;
    }

    /* OutLIF Assignment */
    if (mplsLookUpInfo.doneStatus == DONE_STATUS_DO_LOOKUP_E &&
        nhlfeEntry.pktCmd == NHLFE_CMD_SWITCH_E)
    {
        if (nhlfeEntry.mplsCmd != NHLFE_MPLS_CMD_POP_E)
        {
            MPLS_NEXT_HOP_UNT nextHopEntry =
            nhlfeEntry.mplsInfo.cmd_push.nextHopEntry;

            descrPtr->LLL_outLif = nextHopEntry.out_lif.link_layer.LL_OutLIF;
            descrPtr->arpPointer = nextHopEntry.out_lif.link_layer.arpPtr;
        }
    }
    /* MPLS Cmd */
    if (mplsLookUpInfo.doneStatus == DONE_STATUS_DO_LOOKUP_E &&
        nhlfeEntry.pktCmd == NHLFE_CMD_SWITCH_E)
    {
        if (mplsLookUpInfo.labelNum == 1 &&
           (nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_POP_E ||
            nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_PENULT_E))
        {
            descrPtr->mplsCmd = SKERNEL_MPLS_CMD_POP2_LABLE_E;
        }
        else
        if (mplsLookUpInfo.labelNum == 1 &&
            nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_SWAP_E)
        {
            descrPtr->mplsCmd = SKERNEL_MPLS_CMD_POP_SWAP_E;
        }
        else
        if (nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_POP_E ||
            nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_PENULT_E)
        {
            descrPtr->mplsCmd = SKERNEL_MPLS_CMD_POP1_LABLE_E;
        }
        else
        {
            descrPtr->mplsCmd = (nhlfeEntry.mplsCmd + 1);
        }
    }
    else
    {
        descrPtr->mplsCmd = SKERNEL_MPLS_CMD_NONE_E;
    }
    /* The label to push or swap if MPLS_Command = Push or Swap */
    if (descrPtr->mplsCmd == SKERNEL_MPLS_CMD_SWAP_E ||
        descrPtr->mplsCmd == SKERNEL_MPLS_CMD_PUSH1_LABLE_E)
    {
        descrPtr->label = nhlfeEntry.mplsInfo.cmd_swap.label;
    }
    /* NLP */
    if (nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_POP_E)
    {
        descrPtr->nlpAboveMpls = nhlfeEntry.mplsInfo.cmd_pop.nlp;
    }
    /* L2CE */
    if (nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_PUSH_E)
    {
        descrPtr->l2ce = nhlfeEntry.mplsInfo.cmd_push.l2ce;
    }

    if (nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_PUSH_E)
    {
        descrPtr->byteCount += 4;
    }
    else
    if (nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_PENULT_E &&
        mplsLookUpInfo.labelNum == 0)
    {
        descrPtr->byteCount -= 4;
    }
    else
    if (nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_PENULT_E &&
        mplsLookUpInfo.labelNum == 1)
    {
        descrPtr->byteCount -= 8;
    }
    else
    if (nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_SWAP_E &&
        mplsLookUpInfo.labelNum == 1)
    {
        descrPtr->byteCount -= 4;
    }

    if (mplsLookUpInfo.doneStatus == DONE_STATUS_DO_LOOKUP_E &&
        nhlfeEntry.pktCmd == NHLFE_CMD_SWITCH_E)
    {
        if (nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_PUSH_E ||
            nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_SWAP_E ||
            nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_PENULT_E)
        {
            if (descrPtr->byteCount >
                nhlfeEntry.mplsInfo.cmd_push.nextHopEntry.mtu)
            {
                descrPtr->mplsMtuExceed = 1;
            }
            else
            {
                descrPtr->mplsMtuExceed = 0;
            }
        }
        else
        {
            descrPtr->mplsMtuExceed = 0;
        }
    }
    else
    {
        descrPtr->mplsMtuExceed = 0;
    }
}

/*******************************************************************************
*   snetTwistMplsTtlGet
*
* DESCRIPTION:
*        TTL validity check
*
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*
* OUTPUTS:
*        ttlPtr - TTL value pointer
*
*******************************************************************************/
static GT_BOOL snetTwistMplsTtlGet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    OUT   GT_U8   * ttlPtr
)
{
    *ttlPtr = 0;

    if (mplsLookUpInfo.doneStatus == DONE_STATUS_TTL_ZERO_E)
    {
        return GT_TRUE;
    }
    else
    if (mplsLookUpInfo.doneStatus == DONE_STATUS_DO_LOOKUP_E &&
        nhlfeEntry.pktCmd == NHLFE_CMD_SWITCH_E)
    {
        if (nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_PUSH_E ||
            nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_SWAP_E)
        {
            if (nhlfeEntry.mplsInfo.cmd_swap.ttl == 0)
            {
                if (mplsLookUpInfo.labelInfo[0].ttl == 1 &&
                    nhlfeEntry.decrTtl == 1)
                {
                    return GT_TRUE;
                }
                else
                {
                    *ttlPtr = mplsLookUpInfo.labelInfo[0].ttl -
                              nhlfeEntry.decrTtl;
                    return GT_FALSE;
                }
            }
            else
            {
                *ttlPtr = nhlfeEntry.decrTtl;
                return GT_FALSE;
            }
        }
        else
        if (nhlfeEntry.mplsCmd == NHLFE_MPLS_CMD_PENULT_E)
        {
            if (mplsLookUpInfo.labelInfo[0].ttl == 1 &&
                nhlfeEntry.decrTtl == 1)
            {
                return GT_TRUE;
            }
            else
            {
                *ttlPtr = mplsLookUpInfo.labelInfo[0].ttl -
                          nhlfeEntry.decrTtl;

                return GT_FALSE;
            }
        }
        else
        {
            if (mplsLookUpInfo.labelInfo[0].ttl == 1 &&
                nhlfeEntry.decrTtl == 1 &&
                nhlfeEntry.mplsInfo.cmd_pop.popCpyTtl == 1)
            {
                return GT_TRUE;
            }
            else
            {
                *ttlPtr = mplsLookUpInfo.labelInfo[0].ttl -
                          nhlfeEntry.decrTtl;

                return GT_FALSE;
            }
        }
    }
    else
    if (mplsLookUpInfo.doneStatus == DONE_STATUS_NO_LOOKUP_E &&
        nhlfeEntry.pktCmd == NHLFE_CMD_TRANS_PASS &&
        descrPtr->flowRedirectCmd == SKERNEL_FLOW_MPLS_REDIRECT_E)
    {
        *ttlPtr = mplsLookUpInfo.labelInfo[0].ttl;
        return GT_FALSE;
    }
    else
    {
        *ttlPtr = mplsLookUpInfo.labelInfo[0].ttl;
        return GT_TRUE;
    }
}

/*******************************************************************************
*   snetTwistMplsCmdSet
*
* DESCRIPTION:
*        Set packet output command
*
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*
* OUTPUTS:
*
*
*******************************************************************************/
static GT_VOID snetTwistMplsCmdSet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetTwistMplsCmdSet);

    GT_BOOL badTtl;
    GT_U8  ttl;
    GT_U32  fldValue;

    badTtl = snetTwistMplsTtlGet(devObjPtr, descrPtr, &ttl);

    /* Command set to regular */
    __LOG(("Command set to regular"));
    if (mplsLookUpInfo.doneStatus == DONE_STATUS_NO_LOOKUP_E &&
        badTtl == GT_TRUE)
    {
        descrPtr->pktCmd = SKERNEL_PKT_FORWARD_E;
    }
    else
    if (mplsLookUpInfo.doneStatus == DONE_STATUS_NO_LOOKUP_E &&
        nhlfeEntry.pktCmd == NHLFE_CMD_SWITCH_E)
    {
        descrPtr->pktCmd = SKERNEL_PKT_FORWARD_E;
    }
    if (descrPtr->pktCmd == SKERNEL_PKT_FORWARD_E)
    {
        return;
    }

    /* Command set to drop */
    __LOG(("Command set to drop"));
    if (badTtl == GT_TRUE)
    {
        smemRegFldGet(devObjPtr, MPLS_CTRL_REG, 4, 1, &fldValue);
        /* TrapTTL */
        if (fldValue == 0)
        {
            descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
        }
    }
    else
    if (mplsLookUpInfo.doneStatus == DONE_STATUS_INVALID_ENT_E)
    {
        smemRegFldGet(devObjPtr, MPLS_CTRL_REG, 6, 1, &fldValue);
        /* TrapInvalid NhlfeEntry */
        if (fldValue == 0)
        {
            descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
        }
    }
    else
    if (mplsLookUpInfo.doneStatus == DONE_STATUS_ILLEGAL_POP_E)
    {
        smemRegFldGet(devObjPtr, MPLS_CTRL_REG, 5, 1, &fldValue);
        /* TrapIllegalPop */
        if (fldValue == 0)
        {
            descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
        }
    }
    else
    if (mplsLookUpInfo.doneStatus == DONE_STATUS_DO_LOOKUP_E &&
        nhlfeEntry.pktCmd == NHLFE_CMD_DROP_E)
    {
        descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
    }

    if (descrPtr->pktCmd == SKERNEL_PKT_DROP_E)
    {
        return;
    }

    /* Command set to trap */
    __LOG(("Command set to trap"));
    if (descrPtr->pktCmd == SKERNEL_PKT_TRAP_CPU_E)
    {
        descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
    }
    else
    if (badTtl == GT_TRUE)
    {
        smemRegFldGet(devObjPtr, MPLS_CTRL_REG, 4, 1, &fldValue);
        /* TrapTTL */
        if (fldValue)
        {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                TWIST_CPU_MPLS_NULL_TTL;
        }
    }
    else
    if (mplsLookUpInfo.doneStatus == DONE_STATUS_INVALID_ENT_E)
    {
        smemRegFldGet(devObjPtr, MPLS_CTRL_REG, 6, 1, &fldValue);
        /* TrapInvalid NhlfeEntry */
        if (fldValue)
        {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                TWIST_CPU_MPLS_INVALID_IF_ENTRY;
        }
    }
    else
    if (mplsLookUpInfo.doneStatus == DONE_STATUS_ILLEGAL_POP_E)
    {
        smemRegFldGet(devObjPtr, MPLS_CTRL_REG, 5, 1, &fldValue);
        /* TrapIllegalPop */
        if (fldValue)
        {
            descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
                TWIST_CPU_MPLS_ILLEGAL_POP;
        }
    }
    else
    if (mplsLookUpInfo.doneStatus == DONE_STATUS_DO_LOOKUP_E &&
        nhlfeEntry.pktCmd == NHLFE_CMD_CPU_TRAP_E)
    {
        descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
        descrPtr->bits15_2.cmd_trap2cpu.cpuCode =
            TWIST_CPU_MPLS_NHLFE_CMD_TRAP;
    }
}

/*******************************************************************************
*   snetTwistMplsCosSet
*
* DESCRIPTION:
*        Cos and DSCP assignment
*
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*
* OUTPUTS:
*
*
*******************************************************************************/
static GT_VOID snetTwistMplsCosSet
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 regAddr, fldValue;

    if (descrPtr->pktCmd == SKERNEL_PKT_TRAP_CPU_E)
    {
        GT_U16 cpuCode = descrPtr->bits15_2.cmd_trap2cpu.cpuCode;

        if (cpuCode == TWIST_CPU_MPLS_NULL_TTL ||
            cpuCode == TWIST_CPU_MPLS_INVALID_IF_ENTRY ||
            cpuCode == TWIST_CPU_MPLS_ILLEGAL_POP)
        {
            smemRegFldGet(devObjPtr, MPLS_CTRL_REG, 1, 3, &fldValue);
            /* TrapPriority */
            descrPtr->trafficClass = (GT_U8)fldValue;

        }
        else
        if (cpuCode == TWIST_CPU_MPLS_NHLFE_CMD_TRAP)
        {
            descrPtr->trafficClass = nhlfeEntry.trafficClass;
        }
    }
    else
    /* Remark the CoS according to EXP2Cos*/
    if (mplsLookUpInfo.remarkCos == 1)
    {
        GT_U32 entryOffset;
        GT_U32 lbIdx = mplsLookUpInfo.labelNum;
        regAddr = EXP_TO_COS_TABLE0_REG +
            0x4 * (mplsLookUpInfo.labelInfo[lbIdx].exp / 2);
        entryOffset =
            (mplsLookUpInfo.labelInfo[lbIdx].exp % 2) ? 16 : 0;
        /* EXP to COS Mapping */
        smemRegFldGet(devObjPtr, regAddr, entryOffset, 3, &fldValue);
        /* Priority */
        descrPtr->trafficClass = (GT_U8)fldValue;
        /* DP */
        smemRegFldGet(devObjPtr, regAddr, entryOffset + 3, 2, &fldValue);
        descrPtr->dropPrecedence = (GT_U8)fldValue;
        /* VPT */
        smemRegFldGet(devObjPtr, regAddr, entryOffset + 5, 3, &fldValue);
        descrPtr->userPriorityTag = (GT_U8)fldValue;
        /* DSCP */
        smemRegFldGet(devObjPtr, regAddr, entryOffset + 8, 6, &fldValue);
        descrPtr->dscp = (GT_U8)fldValue;
    }
    else
    {
        descrPtr->trafficClass = mplsLookUpInfo.priority;
        descrPtr->dropPrecedence = mplsLookUpInfo.dp;
        descrPtr->userPriorityTag = mplsLookUpInfo.vpt;
    }
}

/*******************************************************************************
*   snetTwistMplsCounters
*
* DESCRIPTION:
*        Updates MPLS counters
*
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*
* OUTPUTS:
*
*
*******************************************************************************/
static GT_BOOL snetTwistMplsCounters
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr
)
{
    GT_U32 regAddress;
    GT_U32 fldValue;
    GT_U32 offset;


    if (descrPtr->mplsDone == 0)
    {
        return GT_FALSE;
    }

    offset = nhlfeEntry.mngCountSet * 0x80;

    switch (descrPtr->pktCmd)
    {
    case SKERNEL_PKT_FORWARD_E:
        regAddress = MPLS_MNG_RX_PKT_CNT_0_REG + offset;
        smemRegGet(devObjPtr, regAddress, &fldValue);
        /* MPLSRx Packet Counter */
        smemRegSet(devObjPtr, regAddress, ++fldValue);

        regAddress = MPLS_MNG_RX_OCT_LOW_0_REG + offset;
        smemRegGet(devObjPtr, regAddress, &fldValue);
        /* MPLSRxOctet CounterLo */
        fldValue+=descrPtr->byteCount;
        smemRegSet(devObjPtr, regAddress, fldValue);

        smemRegGet(devObjPtr, MPLS_GLB_RCV_PKT_REG, &fldValue);
        /* RxPacket */
        smemRegSet(devObjPtr, MPLS_GLB_RCV_PKT_REG, ++fldValue);

        smemRegGet(devObjPtr, MPLS_GLB_RCV_OCT_LOW_REG, &fldValue);
        /* RXOctetLo */
        fldValue+=descrPtr->byteCount;
        smemRegSet(devObjPtr, MPLS_GLB_RCV_OCT_LOW_REG, fldValue);


        break;
    case SKERNEL_PKT_DROP_E:
        regAddress = MPLS_MNG_DSCRD_PKT_0_REG + offset;
        smemRegGet(devObjPtr, regAddress, &fldValue);
        /* MPLS Dropped Packet */
        smemRegSet(devObjPtr, regAddress, ++fldValue);

        regAddress = MPLS_MNG_DSCRD_OCT_LOW_0_REG + offset;
        smemRegGet(devObjPtr, regAddress, &fldValue);
        /* MPLS Dropped OctetsLo */
        fldValue+=descrPtr->byteCount;
        smemRegSet(devObjPtr, regAddress, fldValue);

        smemRegGet(devObjPtr, MPLS_GLB_DROP_PKT_REG, &fldValue);
        /* Discarded Packets */
        smemRegSet(devObjPtr, MPLS_GLB_DROP_PKT_REG, ++fldValue);

        smemRegGet(devObjPtr, MPLS_GLB_DROP_OCT_LOW_REG, &fldValue);
        /* Discarded OctetsLo */
        fldValue+=descrPtr->byteCount;
        smemRegSet(devObjPtr, MPLS_GLB_DROP_OCT_LOW_REG, fldValue);
        break;
    case SKERNEL_PKT_TRAP_CPU_E:
        regAddress = MPLS_MNG_TRAP_PKT_0_REG + offset;
        smemRegGet(devObjPtr, regAddress, &fldValue);
        /* MPLS Trapped Packet */
        smemRegSet(devObjPtr, regAddress, ++fldValue);

        regAddress = MPLS_MNG_TRAP_OCT_LOW_0_REG + offset;
        smemRegGet(devObjPtr, regAddress, &fldValue);
        /* MPLS Trapped OctetsLo */
        fldValue+=descrPtr->byteCount;
        smemRegSet(devObjPtr, regAddress, ++fldValue);
        break;
    case SKERNEL_PKT_MIRROR_CPU_E:
        break;
    }

    return GT_TRUE;
}

/*******************************************************************************
*   snetTwistIfUpdateLookUp
*
* DESCRIPTION:
*        Updates lookup address according to MPLS_INTERFACE_TABLE_STC
*
*
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
*        inLifNumber - inLif number
* OUTPUTS:
*
*
*******************************************************************************/
static GT_STATUS snetTwistIfUpdateLookUp
(
    IN    SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN    SKERNEL_FRAME_DESCR_STC * descrPtr,
    IN    GT_U16 inLifNumber
)
{
    DECLARE_FUNC_NAME(snetTwistIfUpdateLookUp);

    GT_U32  minLabel, maxLabel;
    GT_U32  baseAddr;
    GT_U8   lbIdx;

    MPLS_INTERFACE_TABLE_STC  mplsIfTblEntry;
    MPLS_LABELINFO_STC * mplsLabelPtr;

    memset(&mplsIfTblEntry, 0, sizeof(MPLS_INTERFACE_TABLE_STC));

    lbIdx = mplsLookUpInfo.labelNum;
    mplsLabelPtr = &mplsLookUpInfo.labelInfo[lbIdx];

    mplsIfTblEntry.inLifNumber = inLifNumber;
    /* Get label interface entry */
    __LOG(("Get label interface entry"));
    snetTwistMplsTblIfEntryGet(devObjPtr, descrPtr, &mplsIfTblEntry);

    minLabel = mplsIfTblEntry.minLabel;
    maxLabel = mplsIfTblEntry.minLabel +
              (mplsIfTblEntry.size1 << mplsIfTblEntry.size0);

    if (!mplsIfTblEntry.valid ||
        mplsLabelPtr->label < minLabel ||
        mplsLabelPtr->label > maxLabel)
    {
        mplsLookUpInfo.doneStatus = DONE_STATUS_INVALID_ENT_E;
        TWIST_NHLFE_FAKE(nhlfeEntry);
        return GT_OUT_OF_RANGE;
    }
    else
    if (mplsLabelPtr->ttl == 0)
    {
        mplsLookUpInfo.doneStatus = DONE_STATUS_TTL_ZERO_E;
        TWIST_NHLFE_FAKE(nhlfeEntry);
        return GT_BAD_VALUE;
    }
    baseAddr = 0x28000000 +
        (mplsIfTblEntry.baseAddress << WRAM_ADDR_ALLIGN_CNS);

    mplsLookUpInfo.lookUpAddress =
        baseAddr + (mplsLabelPtr->label - minLabel) * NHLFE_SIZE_BYTE;

    return GT_OK;
}

