/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
        used to endorse or promote products derived from this software without
        specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

/*******************************************************************************
* pssBspApis.c - bsp APIs
*
* DESCRIPTION:
*       API's supported by BSP.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 15 $
*
*******************************************************************************/

#include "vxworks.h"
#include "config.h"
#include "intLib.h"
#include "cacheLib.h"
#include "db88F6281.h"
#include "drv/pci/pciConfigLib.h"
#include "syslib.h"
#include "mvTypes.h"
#include "mvXor.h"
#include "pssBspApis.h"
#include "sysPciIntCtrl.h"
#include "vxGppIntCtrl.h"
#include "vxPexIntCtrl.h"
#include "mvBoardEnvSpec.h"
#include "mvDragonite.h"
#include "mvCtrlEnvLib.h"
#include "mvSysGbe.h"
#include "mii.h"
#include "mvEthPhy.h"
#include "mvSysHwConfig.h"
#include "bitOps.h"
#include "mvGndReg.h"
#include "mvGnd.h"

#define XCAT_INTERNAL_PP_BASE_ADDR 0xF4000000
MV_U32  XCAT_INTERNAL_PP_BASE_ADDR_extern = XCAT_INTERNAL_PP_BASE_ADDR;

typedef struct
{
    MV_U32 data;
    MV_U32 mask;
} PEX_HEADER_DATA;

PEX_HEADER_DATA pp_configHdr[16] =
{
    {0x000011ab, 0x00000000}, /* 0x00 */
    {0x00100006, 0x00000000}, /* 0x04 */
    {0x05800000, 0x00000000}, /* 0x08 */
    {0x00000008, 0x00000000}, /* 0x0C */
    {XCAT_INTERNAL_PP_BASE_ADDR | 0xC, 0x00000000}, /* 0x10 */
    {0x00000000, 0x00000000}, /* 0x14 */
    {XCAT_INTERNAL_PP_BASE_ADDR | 0xC, 0x00000000}, /* 0x18 */
    {0x00000000, 0x00000000}, /* 0x1C */
    {0x00000000, 0x00000000}, /* 0x20 */
    {0x00000000, 0x00000000}, /* 0x24 */
    {0x00000000, 0x00000000}, /* 0x28 */
    {0x11ab11ab, 0x00000000}, /* 0x2C */
    {0x00000000, 0x00000000}, /* 0x30 */
    {0x00000040, 0x00000000}, /* 0x34 */
    {0x00000000, 0x00000000}, /* 0x38 */
    {0x00000100, 0x00000000}  /* 0x3C */
};

#define HEADER_WRITE(data, offset) pp_configHdr[offset/4].data = ((pp_configHdr[offset/4].data & ~pp_configHdr[offset/4].mask) | (data & pp_configHdr[offset/4].mask))
#define HEADER_READ(offset) pp_configHdr[offset/4].data
#define DB_XCAT_INTERNAL_PP_INT_GPP_PIN    49

/* register that hold the device type*/
#define PRESTERA_DEV_ID_REG_OFFSET 0x4C
int PRESTERA_DEV_ID_REG_OFFSET_extern = PRESTERA_DEV_ID_REG_OFFSET;

MV_U32 bspReadRegisterInternal(MV_U32 address);

/*******************************************************************************
* bspReset
*
* DESCRIPTION:
*       This routine calls to reset of CPU.
*
* INPUTS:
*       none.
*
* OUTPUTS:
*       none.
*
* RETURNS:
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspReset(MV_VOID)
{
    sysToMonitor(0);
    return  MV_OK;
}

/*** DMA ***/
/*******************************************************************************
* bspDmaRead
*
* DESCRIPTION:
*       Read a memory block from a given address.
*
* INPUTS:
*       address     - The address to read from.
*       length      - Length of the memory block to read (in words).
*       burstLimit  - Number of words to be read on each burst.
*
* OUTPUTS:
*       buffer  - The read data.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*       1.  The given buffer is allways 4 bytes aligned, any further allignment
*           requirements should be handled internally by this function.
*       2.  The given buffer may be allocated from an uncached memory space, and
*           it's to the function to handle the cache flushing.
*       3.  The Prestera Driver assumes that the implementation of the DMA is
*           blocking, otherwise the Driver functionality might be damaged.
*
*******************************************************************************/
MV_STATUS bspDmaRead(IN  MV_U32   address,
                     IN  MV_U32   length,
                     IN  MV_U32   burstLimit,
                     OUT MV_U32  *buffer)
{
    MV_U32  sourceAddr;     /* The source address to begin the transfer from*/
    MV_U32  destAddr;       /* Destination address for the data to be       */
                            /* transfered.                                  */
    MV_U32    xorDescPhysAddr;
    MV_U32    dmaEngine;  /* The Dma engine to perform the burst through. */
    MV_STATUS dmaStatus;
    MV_XOR_DESC *xorDesc;
    MV_U32  numOfBursts = 0;

    if (address + length >= LOCAL_MEM_SIZE)
    {
        mvOsPrintf("%s: Trying to write out of mem.\n", __func__);
        return MV_FAIL;
    }

    /* XOR descriptor should be aligned     */
    xorDesc = mvOsIoUncachedMemalign(NULL, MV_XOR_DESC_ALIGNMENT,
                                     sizeof(MV_XOR_DESC), &xorDescPhysAddr);
    if (!xorDesc)
    {
        mvOsPrintf("%s: Alloc failed.\n", __func__);
        return MV_FAIL;
    }

    /* Set the dma function parameters.     */
    dmaEngine   = 1;
    sourceAddr  = address;
    destAddr    = (MV_U32)buffer;

    /* Wait until the Dma is Idle.          */
    while(mvXorStateGet(dmaEngine) != MV_IDLE);

    bspCacheFlush(bspCacheType_DataCache_E, buffer, sizeof(MV_U32) * length);
    bspCacheInvalidate(bspCacheType_DataCache_E, buffer, sizeof(MV_U32) * length);

    numOfBursts = length / burstLimit;

    while ( numOfBursts )
    {
        memset(xorDesc,0,sizeof(MV_XOR_DESC));
        xorDesc->srcAdd0        = (MV_U32)sourceAddr;
        xorDesc->phyDestAdd     = (MV_U32)destAddr;
        xorDesc->byteCnt        = burstLimit*4;
        xorDesc->phyNextDescPtr = 0;
        xorDesc->status         = BIT31;

        dmaStatus = mvXorTransfer(dmaEngine, MV_DMA, xorDescPhysAddr);
        if (mvXorWait(dmaEngine) != MV_OK)
        {
            mvOsPrintf("%s: mvXorWait failed.\n", __func__);
            return MV_FAIL;
        }

        if (dmaStatus != MV_OK || !(xorDesc->status & BIT30))
        {
            mvOsIoUncachedAlignedFree(xorDesc);
            mvOsPrintf("%s: mvXorTransfer failed.\n", __func__);
            return MV_FAIL;
        }

        sourceAddr += (burstLimit * 4);
        destAddr   += (burstLimit * 4);
        numOfBursts--;

        /* Wait until the Dma is Idle.          */
        while(mvXorStateGet(dmaEngine) != MV_IDLE);
    }

    mvOsIoUncachedAlignedFree(xorDesc);
    return MV_OK;
}

#ifdef INCLUDE_PCI
/*** PCI ***/

static MV_U16 gPPDevId = 0xFFFF;
static MV_U16 gPPRevision = 0;

/*******************************************************************************
* bspPciConfigWriteReg
*
* DESCRIPTION:
*       This routine write register to the PCI configuration space.
*
* INPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*       regAddr  - Register offset in the configuration space.
*       data     - data to write.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspPciConfigWriteReg(IN  MV_U32  busNo,
                               IN  MV_U32  devSel,
                               IN  MV_U32  funcNo,
                               IN  MV_U32  regAddr,
                               IN  MV_U32  data)
{
    MV_CHIP_FEATURES *featuresP = mvChipFeaturesGet();
    MV_STATUS ret;

    /* Emulate internal PP on PEX */
    if ((0xFF == busNo) && (0xFF == devSel))
    {
        HEADER_WRITE(data, regAddr);
        return MV_OK;
    }
    /* Emulate end*/

    if (featuresP->numOfPex == 0)
    {
        return MV_FAIL;
    }

    /* call bsp write function */
    ret = pciConfigOutLong(busNo, devSel, funcNo, regAddr, data);

    /* check whether success */
    if (ret != 0)
    {
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
* bspPciConfigReadReg
*
* DESCRIPTION:
*       This routine read register from the PCI configuration space.
*
* INPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*       regAddr  - Register offset in the configuration space.
*
* OUTPUTS:
*       data     - the read data.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspPciConfigReadReg(IN  MV_U32   busNo,
                              IN  MV_U32   devSel,
                              IN  MV_U32   funcNo,
                              IN  MV_U32   regAddr,
                              OUT MV_U32  *data)
{
    MV_CHIP_FEATURES *featuresP = mvChipFeaturesGet();
    MV_STATUS ret;

    /* Emulate internal PP on PEX */
    if ((0xFF == busNo) && (0xFF == devSel))
    {
        if(0 == regAddr)
        {
            *data = 0x000011ab | (gPPDevId << 16);
        }

        else if (8 == regAddr)
        {
            *data =  HEADER_READ(regAddr) | gPPRevision;
        }
        else
        {
            *data = HEADER_READ(regAddr);
        }

        return MV_OK;
    }

    if (featuresP->numOfPex == 0)
    {
        return MV_FAIL;
    }

    /* Emulate end*/
    /* call bsp read function */
    ret = pciConfigInLong(busNo, devSel, funcNo, regAddr, (MV_U32*)data);

    /* check whether success */
    if (ret != 0)
    {
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
* bspPciFindDev
*
* DESCRIPTION:
*       This routine returns the next instance of the given device (defined by
*       vendorId & devId).
*
* INPUTS:
*       vendorId - The device vendor Id.
*       devId    - The device Id.
*       instance - The requested device instance.
*
* OUTPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspPciFindDev(IN  MV_U16   vendorId,
                        IN  MV_U16   devId,
                        IN  MV_U32   instance,
                        OUT MV_U32  *busNo,
                        OUT MV_U32  *devSel,
                        OUT MV_U32  *funcNo)
{
    MV_CHIP_FEATURES *featuresP = mvChipFeaturesGet();
    STATUS ret;

    /* Emulate internal PP on PEX */
    if(0 == instance)
    {
        /* If first time - read device ID from PP internal register space */
        if(0xFFFF == gPPDevId)
        {
            MV_U32 regValue = bspReadRegisterInternal(
            PRESTERA_DEV_ID_REG_OFFSET + XCAT_INTERNAL_PP_BASE_ADDR);

            /* get bits 4..19 */
            gPPDevId = (MV_U16) ((regValue >> 4) & 0xFFFF);
            gPPRevision = (MV_U16) (regValue & 0xF);
        }

        if(gPPDevId == devId)
        {
            *busNo  = 0xFF;
            *devSel = 0xFF;
            *funcNo = 0xFF;
            return MV_OK;
        }
    }
    else
    {
        if(gPPDevId == devId)
        {
            instance--;
        }
    }
    /* Emulate - end */

    if (featuresP->numOfPex == 0)
    {
        return MV_FAIL;
    }

    /* call bsp read function */
    ret = pciFindDevice(vendorId, devId, instance, (int*)busNo,
    (int*)devSel, (int*)funcNo);

    /* check whether success */
    if (ret != OK)
    {
        return MV_FAIL;
    }

    return MV_OK;
}
#endif /* INCLUDE_PCI */

/*******************************************************************************
* bspPciGetIntVec
*
* DESCRIPTION:
*       This routine return the PCI interrupt vector.
*
* INPUTS:
*       pciInt - PCI interrupt number.
*
* OUTPUTS:
*       intVec - PCI interrupt vector.
*
* RETURNS:
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspPciGetIntVec(IN  bspPciInt_PCI_INT    pciInt,
                          OUT void               **intVec)
{
    /* check parameters */
    if (intVec == NULL)
    {
        return MV_BAD_PARAM;
    }

    /* get the PCI interrupt vector */
    if ((pciInt == bspPciInt_PCI_INT_B) || (pciInt == bspPciInt_PCI_INT_D))
    {
        if (mvPpChipIsXCat2() == MV_FALSE)
        {
            /* The internal PP interrupt is connected to GPIO 49 of the CPU */
            *intVec = (void *)DB_XCAT_INTERNAL_PP_INT_GPP_PIN;
        }
        else
        {
            /* In xCat2 PP int is connected to bit 23 of High Cause (23+32=55)*/
            *intVec = (void *)INT_LVL_XCAT2_SWITCH;
        }
    }
    else
    {
        /* The external PP interrupt is PEX0INT of the CPU */
        *intVec = (void *)INT_LVL_PEX00_ABCD;
    }

    return MV_OK;
}

/*******************************************************************************
* bspIntConnect
*
* DESCRIPTION:
*       Connect a specified C routine to a specified interrupt vector.
*
* INPUTS:
*       vector    - interrupt vector number to attach to
*       routine   - routine to be called
*       parameter - parameter to be passed to routine
*
* OUTPUTS:
*       None
*
* RETURNS:
*       MV_OK   - on success
*       MV_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
MV_STATUS bspIntConnect(MV_U32 irqValue, MV_VOIDFUNCPTR routine, MV_U32 param)
{
    int rc = ERROR;

    if (mvPpChipIsXCat2() == MV_FALSE)
    {
        if (irqValue == DB_XCAT_INTERNAL_PP_INT_GPP_PIN)
        {
            rc = sysPciIntConnect(INT_LVL_SWITCH_GPP_INT, routine, param);
        }
        else
        {
            rc = (int)vxPexIntConnect(0, PEX_RCV_INT_A, routine, (int)param, 5);
        }
    }
    else
    {
        if (irqValue == INT_LVL_XCAT2_SWITCH)
        {
            rc = intConnect(INUM_TO_IVEC(INT_LVL_XCAT2_SWITCH), routine, param);
        }
        else
        {
            rc = (int)vxPexIntConnect(0, PEX_RCV_INT_A, routine, (int)param, 5);
        }
    }

    return (rc == OK) ? MV_OK : MV_FAIL;
}

/*******************************************************************************
* bspIntEnable
*
* DESCRIPTION:
*       Enable corresponding interrupt bits
*
* INPUTS:
*       intMask - new interrupt bits
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success
*       MV_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
MV_STATUS bspIntEnable(MV_U32 irqValue)
{
    int rc = ERROR;

    if (mvPpChipIsXCat2() == MV_FALSE)
    {
        if (irqValue == DB_XCAT_INTERNAL_PP_INT_GPP_PIN)
        {
            /* see: INT_LVL_SWITCH_GPP_INT */
            rc = vxGppIntEnable(irqValue % 32, irqValue / 32);
        }
        else
        {
            rc = vxPexIntEnable(0, PEX_RCV_INT_A);
        }
    }
    else
    {
        if (irqValue == INT_LVL_XCAT2_SWITCH)
        {
            rc = intEnable(INT_LVL_XCAT2_SWITCH);
        }
        else
        {
            rc = vxPexIntEnable(0, PEX_RCV_INT_A);
        }
    }

    return (rc == OK) ? MV_OK : MV_FAIL;
}

/*******************************************************************************
* bspIntDisable
*
* DESCRIPTION:
*       Disable corresponding interrupt bits.
*
* INPUTS:
*       intMask - new interrupt bits
*                 For xCat2: irqValue 55 means
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success
*       MV_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
MV_STATUS bspIntDisable(MV_U32 irqValue)
{
    int rc = ERROR;

    if (mvPpChipIsXCat2() == MV_FALSE)
    {
        if (irqValue == DB_XCAT_INTERNAL_PP_INT_GPP_PIN)
        {
            rc = vxGppIntDisable(irqValue % 32, irqValue / 32);
        }
        else
        {
            rc = vxPexIntDisable(0, PEX_RCV_INT_A);
        }
    }
    else
    {
        if (irqValue == INT_LVL_XCAT2_SWITCH)
        {
            rc = intDisable(INT_LVL_XCAT2_SWITCH);
        }
        else
        {
            rc = vxPexIntDisable(0, PEX_RCV_INT_A);
        }
    }

    return (rc == OK) ? MV_OK : MV_FAIL;
}

#ifdef MV_INCLUDE_DRAGONITE
/*******************************************************************************
* bspDragoniteGetIntVec
*
* DESCRIPTION:
*       This routine returns the Dragonite interrupt vector.
*
* INPUTS:
*       None
*
* OUTPUTS:
*       intVec - Dragonite interrupt vector.
*
* RETURNS:
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspDragoniteGetIntVec(MV_U32 *intVec)
{
    *intVec = INT_LVL_DRAGONITE;
    return MV_OK;
}
#endif /* MV_INCLUDE_DRAGONITE */

/*
 * Ethernet Driver
 */

/*******************************************************************************
* bspEthIntRxReadyVecGet
*
* DESCRIPTION:
*       Returns RxReady interrupt vector appropriate for current chip.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Interrupt vector.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
int bspEthIntRxReadyVecGet(void)
{
    int rxReadyIsrVec;

    if (mvPpChipIsXCat2() == MV_TRUE)
    {
        rxReadyIsrVec = INT_LVL_GBE0_RX;
    }
    else
    {
        rxReadyIsrVec = INT_LVL_GBE1_RX;
    }

    return rxReadyIsrVec;
}

/*******************************************************************************
* bspEthIntTxDoneVecGet
*
* DESCRIPTION:
*       Returns TxDone interrupt vector appropriate for current chip.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Interrupt vector.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
int bspEthIntTxDoneVecGet(void)
{
    int txDoneIsrVec;

    if (mvPpChipIsXCat2() == MV_TRUE)
    {
        txDoneIsrVec  = INT_LVL_GBE0_TX;
    }
    else
    {
        txDoneIsrVec  = INT_LVL_GBE1_TX;
    }

    return txDoneIsrVec;
}

/*******************************************************************************
* bspEthIntInit
*
* DESCRIPTION:
*       Connects all supported by MII interrupts to driver ISRs.
*
* INPUTS:
*       drvCtrlP     - pointer to the driver control structure
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       BSP ETH driver internal API.
*       This function is different for BSP/LSP.
*
*******************************************************************************/
MV_STATUS bspEthIntInit(void (*bspEthRxReadyIsrF)(void *),
                        void (*bspEthTxDoneIsrF) (void *))
{
    if (bspEthRxReadyIsrF != NULL)
    {
        if (intConnect(INUM_TO_IVEC(bspEthIntRxReadyVecGet()),
                       bspEthRxReadyIsrF, (MV_U32)0) != OK)
        {
            mvOsPrintf("%s: intConnect failed.\n", __func__);
            return MV_FAIL;
        }
    }

    if (bspEthTxDoneIsrF != NULL)
    {
        if (intConnect(INUM_TO_IVEC(bspEthIntTxDoneVecGet()),
                       bspEthTxDoneIsrF, (MV_U32)0) != OK)
        {
            mvOsPrintf("%s: intConnect failed.\n", __func__);
            return MV_FAIL;
        }
    }

    return MV_OK;
}

/*******************************************************************************
* bspEthIntEnable
*
* DESCRIPTION:
*       Enables all relevant interrupt for bspEth driver.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       BSP ETH driver internal API.
*       This function is different for BSP/LSP.
*
*******************************************************************************/
MV_VOID bspEthIntEnable(MV_BOOL rxIntEnable, MV_BOOL txIntEnable)
{
    /*
     * Enable Layer 1 interrupts (in xCat internal CPU)
     */
    if (rxIntEnable == MV_TRUE)
    {
        intEnable(bspEthIntRxReadyVecGet());
    }

    if (txIntEnable == MV_TRUE)
    {
        intEnable(bspEthIntTxDoneVecGet());
    }
}

/*******************************************************************************
* bspEthIntDisable
*
* DESCRIPTION:
*       Enables all relevant interrupt for bspEth driver.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       BSP ETH driver internal API.
*       This function is different for BSP/LSP.
*
*******************************************************************************/
MV_VOID bspEthIntDisable(MV_BOOL rxIntEnable, MV_BOOL txIntEnable)
{
    /*
     * Disable Layer 1 interrupts (in xCat internal CPU)
     */
    if (rxIntEnable == MV_TRUE)
    {
        intDisable(bspEthIntRxReadyVecGet());
    }

    if (txIntEnable == MV_TRUE)
    {
        intDisable(bspEthIntTxDoneVecGet());
    }
}

