/******************************************************************************
*              Copyright (c) Marvell International Ltd. and its affiliates
*
* This software file (the "File") is owned and distributed by Marvell
* International Ltd. and/or its affiliates ("Marvell") under the following
* alternative licensing terms.
* If you received this File from Marvell, you may opt to use, redistribute
* and/or modify this File under the following licensing terms.
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*  -   Redistributions of source code must retain the above copyright notice,
*       this list of conditions and the following disclaimer.
*  -   Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*  -    Neither the name of Marvell nor the names of its contributors may be
*       used to endorse or promote products derived from this software without
*       specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************
* mvHwsXlgMac28nmDb.c
*
* DESCRIPTION:
*
*
* FILE REVISION NUMBER:
*       $Revision: 4 $
*
*******************************************************************************/
#include <common/configElementDb/mvCfgSeqDbIf.h>
#include <common/siliconIf/mvSiliconIf.h>
#include <common/siliconIf/siliconAddress.h>
#include <mac/xlgMac/mvHwsXlgMac28nmDb.h>

MV_CFG_SEQ hwsXlgMac28nmSeqDb[MV_MAC_XLG_LAST_SEQ];

#ifndef CO_CPU_RUN
static GT_U32 countInitCalls = 0; /* count registered instances */

static MV_OP_PARAMS mode1LaneSeqParams[] = {
    {XLGMAC_UNIT, MSM_PORT_MAC_CONTROL_REGISTER3,  0x2000,    0xE000,   0, 0},
    {XLGMAC_UNIT, MSM_PORT_MAC_CONTROL_REGISTER4,  0x210,     0xF10,    0, 0}
};

static MV_OP_PARAMS mode2LaneSeqParams[] = {
    {XLGMAC_UNIT, MSM_PORT_MAC_CONTROL_REGISTER3,  0x4000,    0xE000,   0, 0},
    {XLGMAC_UNIT, MSM_PORT_MAC_CONTROL_REGISTER4,  0x210,     0xF10,    0, 0}
};

static MV_OP_PARAMS mode4LaneSeqParams[] = {
    {XLGMAC_UNIT, MSM_PORT_MAC_CONTROL_REGISTER3,  0x6000,    0xE000,   0, 0},
    {XLGMAC_UNIT, MSM_PORT_MAC_CONTROL_REGISTER4,  0x810,     0xF10,    0, 0}
};

static MV_OP_PARAMS resetSeqParams[] = {
    {XLGMAC_UNIT, MSM_PORT_MAC_CONTROL_REGISTER0,  0x0,      (1 << 1),  0, 0},
    {XLGMAC_UNIT, MSM_PORT_MAC_CONTROL_REGISTER4, (0 << 8),  (1 << 8),  0, 0}
};

static MV_OP_PARAMS unresetSeqParams[] = {
    {XLGMAC_UNIT, MSM_PORT_MAC_CONTROL_REGISTER0, (1 << 1),  (1 << 1),  0, 0}
};

static MV_OP_PARAMS lbNormalSeqParams[] = {
    {XLGMAC_UNIT, MSM_PORT_MAC_CONTROL_REGISTER1,  0x0,      (3 << 13), 0, 0}
};

static MV_OP_PARAMS lbRx2TxSeqParams[] = {
    {XLGMAC_UNIT, MSM_PORT_MAC_CONTROL_REGISTER1, (2 << 13), (3 << 13), 0, 0}
};

static MV_OP_PARAMS lbTx2RxSeqParams[] = {
    {XLGMAC_UNIT, MSM_PORT_MAC_CONTROL_REGISTER1, (1 << 13), (3 << 13), 0, 0}
};

/* seqId to params array mapping */
static MV_SEQ_OP_PARAMS seqIdToParams[] =
{
    {unresetSeqParams,   MV_SEQ_SIZE(unresetSeqParams)},    /* XLGMAC_UNRESET_SEQ     */
    {resetSeqParams,     MV_SEQ_SIZE(resetSeqParams)},      /* XLGMAC_RESET_SEQ       */
    {mode1LaneSeqParams, MV_SEQ_SIZE(mode1LaneSeqParams)},  /* XLGMAC_MODE_1_Lane_SEQ */
    {mode2LaneSeqParams, MV_SEQ_SIZE(mode2LaneSeqParams)},  /* XLGMAC_MODE_2_Lane_SEQ */
    {mode4LaneSeqParams, MV_SEQ_SIZE(mode4LaneSeqParams)},  /* XLGMAC_MODE_4_Lane_SEQ */
    {lbNormalSeqParams,  MV_SEQ_SIZE(lbNormalSeqParams)},   /* XLGMAC_LPBK_NORMAL_SEQ */
    {lbRx2TxSeqParams,   MV_SEQ_SIZE(lbRx2TxSeqParams)},    /* XLGMAC_LPBK_RX2TX_SEQ  */
    {lbTx2RxSeqParams,   MV_SEQ_SIZE(lbTx2RxSeqParams)}     /* XLGMAC_LPBK_TX2RX_SEQ  */
};

GT_STATUS hwsXlgMac28nmSeqInit(GT_U8 devNum)
{
    MV_OP_PARAMS *updateParams;
    MV_VALIDATE_SEQ_OPER(&countInitCalls);

    /* XLG MAC Sequences create */
    CHECK_STATUS(mvAllocateAllSeqOperation(&hwsXlgMac28nmSeqDb[0], seqIdToParams, MV_MAC_XLG_LAST_SEQ));

    /* XLG MAC Sequences operation & parameters update */
    mvInitSeqOperation(&hwsXlgMac28nmSeqDb[0], seqIdToParams, MV_MAC_XLG_LAST_SEQ);

    /* XLG MAC Sequences update */
    /* Note: This code section doesn't support multiple device (such as AC3 with BobK)
             since it changes the common DB */
    if (HWS_DEV_GOP_REV(devNum) != GOP_28NM_REV1)
    {
        updateParams = (MV_OP_PARAMS*)hwsXlgMac28nmSeqDb[XLGMAC_MODE_1_Lane_SEQ].cfgSeq[1].params;
        updateParams->mask = 0x1F10;
        updateParams = (MV_OP_PARAMS*)hwsXlgMac28nmSeqDb[XLGMAC_MODE_2_Lane_SEQ].cfgSeq[1].params;
        updateParams->mask = 0x1F10;
        updateParams = (MV_OP_PARAMS*)hwsXlgMac28nmSeqDb[XLGMAC_MODE_4_Lane_SEQ].cfgSeq[1].params;
        updateParams->mask = 0x1F10;
    }

    if (HWS_DEV_GOP_REV(devNum) >= GOP_28NM_REV3)
    {
        /* for BobK (and above), TxDMA interface should work in 256bits. */
        updateParams = (MV_OP_PARAMS*)hwsXlgMac28nmSeqDb[XLGMAC_MODE_2_Lane_SEQ].cfgSeq[1].params;
        updateParams->operData = 0x810;
    }

    return GT_OK;
}

void hwsXlgMac28nmSeqFree(void)
{
    GT_U32 i;

    if (countInitCalls == 0)
    {
        return;
    }
    countInitCalls--;
    for (i = 0; (countInitCalls == 0) && (i < MV_MAC_XLG_LAST_SEQ); i++)
    {
        if (hwsXlgMac28nmSeqDb[i].cfgSeq != NULL)
        {
            hwsOsFreeFuncPtr(hwsXlgMac28nmSeqDb[i].cfgSeq);
        }
    }
}

GT_STATUS hwsXlgMac28nmSeqGet(MV_HWS_XLG_MAC_SUB_SEQ seqType, MV_CFG_ELEMENT *seqLine, GT_U32 lineNum)
{
  if ((seqType >= MV_MAC_XLG_LAST_SEQ) ||
      (hwsXlgMac28nmSeqDb[seqType].cfgSeq == NULL))
  {
      return GT_NO_SUCH;
  }

  return mvGetSeqOperation(&hwsXlgMac28nmSeqDb[seqType], seqLine, lineNum);
}

GT_STATUS hwsXlgMac28nmSeqSet(GT_BOOL firstLine, MV_HWS_XLG_MAC_SUB_SEQ seqType, MV_CFG_ELEMENT *seqLine, GT_U32 numOfOp)
{
  if (seqType >= MV_MAC_XLG_LAST_SEQ)
  {
      return GT_NO_SUCH;
  }

  return mvSetSeqOperation(firstLine, &hwsXlgMac28nmSeqDb[seqType], numOfOp, seqLine);
}
#else
GT_STATUS hwsXlgMac28nmSeqInit(GT_U8 devNum)
{
    hwsGenSeqInit(hwsXlgMac28nmSeqDb, MV_MAC_XLG_LAST_SEQ);
    return GT_OK;
}
#endif



