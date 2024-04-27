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
*       $Revision: 17 $
*
*******************************************************************************/

#include "mvTypes.h"
#include "mvXor.h"
#include "pssBspApis.h"
#include "mvBoardEnvSpec.h"
#include "mvDragonite.h"
#include "mvCtrlEnvLib.h"
#include "mvSysGbe.h"
#include "mii.h"
#include "mvEthPhy.h"
#include "mvSysHwConfig.h"
#include "mvGndReg.h"
#include "mvPrestera.h"
#include "mvGnd.h"
#include "mvGndOsIf.h"
#include "mvGndHwIf.h"
#include "mvGenSyncPool.h"
#include "mvNetDrvCommon.h"
#include "bitOps.h"
#include "mvGenBuffPool.h"
#include "mvTwsiDrvCtrl.h"

#define  SMI_WRITE_ADDRESS_MSB_REGISTER   (0x00)
#define  SMI_WRITE_ADDRESS_LSB_REGISTER   (0x01)
#define  SMI_WRITE_DATA_MSB_REGISTER      (0x02)
#define  SMI_WRITE_DATA_LSB_REGISTER      (0x03)

#define  SMI_READ_ADDRESS_MSB_REGISTER    (0x04)
#define  SMI_READ_ADDRESS_LSB_REGISTER    (0x05)
#define  SMI_READ_DATA_MSB_REGISTER       (0x06)
#define  SMI_READ_DATA_LSB_REGISTER       (0x07)

#define  SMI_STATUS_REGISTER              (0x1f)

#define SMI_STATUS_WRITE_DONE             (0x02)
#define SMI_STATUS_READ_READY             (0x01)

/* #define SMI_WAIT_FOR_STATUS_DONE */
#define SMI_TIMEOUT_COUNTER  1000

#define STUB_FAIL printf("stub function %s returning MV_NOT_SUPPORTED\n", \
                         __FUNCTION__);  return MV_NOT_SUPPORTED

/*******************************************************************************
 * Externs
 */

   void hsuRebootBootRom(void);

/*** reset ***/
/*******************************************************************************
* bspResetInit
*
* DESCRIPTION:
*       This routine calls in init to do system init config for reset.
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
MV_STATUS bspResetInit(MV_VOID)
{
    return MV_OK;
}

/*** cache ***/
/*******************************************************************************
* bspCacheFlush
*
* DESCRIPTION:
*       Flush to RAM content of cache
*
* INPUTS:
*       type        - type of cache memory data/intraction
*       address_PTR - starting address of memory block to flush
*       size        - size of memory block (in bytes)
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
MV_STATUS bspCacheFlush(bspCacheType_ENT cacheType, void *addr, size_t size)
{
    switch (cacheType)
    {
    case bspCacheType_InstructionCache_E:
        return MV_BAD_PARAM; /* only data cache supported */

    case bspCacheType_DataCache_E:
        break;

    default:
        return MV_BAD_PARAM;
    }

    mvOsCacheFlush(NULL, addr, size);
    return MV_OK;
}

/*******************************************************************************
* bspCacheInvalidate
*
* DESCRIPTION:
*       Invalidate current content of cache
*
* INPUTS:
*       type        - type of cache memory data/intraction
*       address_PTR - starting address of memory block to flush
*       size        - size of memory block
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
MV_STATUS bspCacheInvalidate(bspCacheType_ENT cacheType, void *address_PTR,
                                                         size_t size)
{
    switch (cacheType)
    {
    case bspCacheType_InstructionCache_E:
        return MV_BAD_PARAM; /* only data cache supported */

    case bspCacheType_DataCache_E:
        break;

    default:
        return MV_BAD_PARAM;
    }

    mvOsCacheInvalidate(NULL, address_PTR, size);
    return MV_OK;
}

/*** DMA ***/
/*******************************************************************************
* bspDmaWrite
*
* DESCRIPTION:
*       Write a given buffer to the given address using the Dma.
*
* INPUTS:
*       address     - The destination address to write to.
*       buffer      - The buffer to be written.
*       length      - Length of buffer in words.
*       burstLimit  - Number of words to be written on each burst.
*
* OUTPUTS:
*       None.
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
MV_STATUS bspDmaWrite(IN  MV_U32  toAddr,
                      IN  MV_U32 *fromBuff,
                      IN  MV_U32  wordsNum,
                      IN  MV_U32  burstLimit)
{
    return bspDmaRead((MV_U32)fromBuff, wordsNum, burstLimit, (MV_U32 *)toAddr);
}

#ifdef INCLUDE_PCI
/*** PCI ***/
/*******************************************************************************
 * bspReadRegisterInternal
 */
MV_U32 bspReadRegisterInternal(MV_U32 address)
{
    /* Endianess. */
#if defined(MV_CPU_BE)

    /* need to swap the bytes */
    MV_U8   *bytesPtr;
    MV_U32  registerValue = *((MV_U32*)address);

    bytesPtr = (MV_U8*)&registerValue;

    return ((MV_U32)(bytesPtr[3] << 24)) |
           ((MV_U32)(bytesPtr[2] << 16)) |
           ((MV_U32)(bytesPtr[1] << 8 )) |
           ((MV_U32)(bytesPtr[0]      )) ;

#elif defined(MV_CPU_LE)

    /* direct access - no swap needed */
    return *((MV_U32*)address);

#endif  /* MV_CPU_BE */
}

/*******************************************************************************
* bspPciGetIntMask
*
* DESCRIPTION:
*       This routine return the PCI interrupt vector.
*
* INPUTS:
*       pciInt - PCI interrupt number.
*
* OUTPUTS:
*       intMask - PCI interrupt mask.
*
* RETURNS:
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       PCI interrupt mask should be used for interrupt disable/enable.
*
*******************************************************************************/
MV_STATUS bspPciGetIntMask(IN  bspPciInt_PCI_INT   pciInt,
                           OUT MV_U32             *intMask)
{
    MV_CHIP_FEATURES *featuresP = mvChipFeaturesGet();
    void        *intVec;

    /*
     * In case application tries to connect to interrupt from 2nd device
     * and PEX is not present in the chip, failure indication is returned.
     */
    if (featuresP->numOfPex == 0 && pciInt == INT_LVL_PEX0_INTA)
    {
        return MV_FAIL;
    }

    /* check parameters */
    if (intMask == NULL)
    {
        return MV_BAD_PARAM;
    }

    /* get the PCI interrupt vector */
    bspPciGetIntVec(pciInt, &intVec);

    *intMask = (MV_U32)intVec;

    return MV_OK;
}

/*******************************************************************************
* bspPciEnableCombinedAccess
*
* DESCRIPTION:
*       This function enables / disables the Pci writes / reads combining
*       feature.
*       Some system controllers support combining memory writes / reads. When a
*       long burst write / read is required and combining is enabled, the master
*       combines consecutive write / read transactions, if possible, and
*       performs one burst on the Pci instead of two. (see comments)
*
* INPUTS:
*       enWrCombine - MV_TRUE enables write requests combining.
*       enRdCombine - MV_TRUE enables read requests combining.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on sucess,
*       MV_NOT_SUPPORTED    - if the controller does not support this feature,
*       MV_FAIL             - otherwise.
*
* COMMENTS:
*       1.  Example for combined write scenario:
*           The controller is required to write a 32-bit data to address 0x8000,
*           while this transaction is still in progress, a request for a write
*           operation to address 0x8004 arrives, in this case the two writes are
*           combined into a single burst of 8-bytes.
*
*******************************************************************************/
MV_STATUS bspPciEnableCombinedAccess(IN  MV_BOOL     enWrCombine,
                                     IN  MV_BOOL     enRdCombine)
{
    /*
     * For CPSS PEX of the local device is simulated anyway
     * (CPSS is not aware of XBAR), ==> no need to test the presence
     * of physical PEX controller.
     *
     * MV_CHIP_FEATURES *featuresP = mvChipFeaturesGet();
     *
     * if (featuresP->numOfPex == 0)
     * {
     *     return MV_OK;
     * }
     */

    if ((enWrCombine == MV_TRUE) || (enRdCombine == MV_TRUE))
    {
        return MV_NOT_SUPPORTED;
    }

    return MV_OK;
}
#endif /* INCLUDE_PCI */

/*** TWSI ***/
/*******************************************************************************
* bspTwsiInitDriver
*
* DESCRIPTION:
*       Init the TWSI interface
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspTwsiInitDriver(MV_VOID)
{
    return mvDirectTwsiInitDriver();
}

/*******************************************************************************
* bspTwsiWaitNotBusy
*
* DESCRIPTION:
*       Wait for TWSI interface not BUSY
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspTwsiWaitNotBusy(MV_VOID)
{
    return mvDirectTwsiWaitNotBusy();
}

/*******************************************************************************
* bspTwsiMasterReadTrans
*
* DESCRIPTION:
*       do TWSI interface Transaction
*
* INPUTS:
*    devId - I2c slave ID
*    pData - Pointer to array of chars (address / data)
*    len   - pData array size (in chars).
*    stop  - Indicates if stop bit is needed.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspTwsiMasterReadTrans(
    MV_U8    devId,  /* I2C slave ID                                */
    MV_U8   *pData,  /* Pointer to array of chars (address / data)  */
    MV_U8    len,    /* pData array size (in chars).                */
    MV_BOOL  stop    /* Indicates if stop bit is needed in the end  */
)
{
    return mvDirectTwsiMasterReadTrans(devId, pData, len, stop);
}

/*******************************************************************************
* bspTwsiMasterWriteTrans
*
* DESCRIPTION:
*       do TWSI interface Transaction
*
* INPUTS:
*    devId - I2c slave ID
*    pData - Pointer to array of chars (address / data)
*    len   - pData array size (in chars).
*    stop  - Indicates if stop bit is needed.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspTwsiMasterWriteTrans(
    MV_U8    devId,   /* I2c slave ID                                */
    MV_U8   *pData,   /* Pointer to array of chars (address / data)  */
    MV_U8    len,     /* pData array size (in chars).                */
    MV_BOOL  stop     /* Indicates if stop bit is needed in the end  */
)
{
    return mvDirectTwsiMasterWriteTrans(devId, pData, len, stop);
}

/*
  service routines
  On Xcat with smi connection to prestera only one device is connected and
  it's on smi address 0x0. Since we may be invoked from other functions that
  assume that first phy is on 0x10 we adjust it here.
*/

#define FIX_XCAT_SMI_ADDRESS(x)  if (x >=0x10) x-= 0x10

static MV_STATUS INLINE ethPhyRegRead(MV_U32 phyAddr, MV_U32 regOffs, MV_U16 *data)
{
    return mvEthPhyRegRead(phyAddr, regOffs, data);
}

static INLINE MV_STATUS ethPhyRegWrite(MV_U32 phyAddr, MV_U32 regOffs, MV_U16 data)
{
    return mvEthPhyRegWrite(phyAddr, regOffs, data);
}

static INLINE MV_STATUS smiReadReg(MV_U32 devSlvId, MV_U32  regAddr, MV_U32 *value)
{
    /* perform direct smi read */
    MV_STATUS ret;
    MV_U16    temp1;

    FIX_XCAT_SMI_ADDRESS(devSlvId);

    ret = ethPhyRegRead(devSlvId, regAddr, &temp1);
    *value = temp1;
    return (MV_OK == ret)? MV_OK : MV_FAIL;
}

static INLINE MV_STATUS smiWriteReg(MV_U32 devSlvId, MV_U32 regAddr, MV_U32 value)
{
    /* Perform direct smi write reg */
    MV_STATUS ret;

    FIX_XCAT_SMI_ADDRESS(devSlvId);

    ret = ethPhyRegWrite(devSlvId, regAddr, value);
    return (MV_OK == ret)? MV_OK : MV_FAIL;
}

static INLINE void smiWaitForStatus(MV_U32 devSlvId)
{
#ifdef SMI_WAIT_FOR_STATUS_DONE
    MV_U32 stat;
    unsigned int timeOut;
    int rc;

    /* wait for write done */
    timeOut = SMI_TIMEOUT_COUNTER;
    do
    {
        rc = smiReadReg(devSlvId, SMI_STATUS_REGISTER,&stat);
        if (rc != MV_OK)
            return;
        if (--timeOut < 1)
            return;
    } while ((stat & SMI_STATUS_WRITE_DONE) == 0);
#endif
}

/*******************************************************************************
* bspSmiInitDriver
*
* DESCRIPTION:
*       Init the SMI interface
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       smiAccessMode - direct/indirect mode
*
* RETURNS:
*       MV_OK               - on success
*       MV_FAIL   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspSmiInitDriver(bspSmiAccessMode_ENT *smiAccessMode)
{
  /* Set SMI access speed to be faster than default */
  MV_REG_WRITE(ETH_PHY_SMI_ACCEL_REG, 1<<ETH_PHY_SMI_ACCEL_8_OFFS);
  *smiAccessMode = bspSmiAccessMode_inDirect_E;
  return MV_OK;
}

/*******************************************************************************
* bspSmiReadReg
*
* DESCRIPTION:
*       Reads a register from SMI slave.
*
* INPUTS:
*       devSlvId - Slave Device ID
*      actSmiAddr - actual smi addr to use (relevant for SX PPs)
*       regAddr - Register address to read from.
*
* OUTPUTS:
*       valuePtr     - Data read from register.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspSmiReadReg(IN  MV_U32  devSlvId,
                        IN  MV_U32  actSmiAddr,
                        IN  MV_U32  regAddr,
                        OUT MV_U32 *valuePtr)
{
    static int first_time=1;
    if (first_time)
    {
        bspSmiAccessMode_ENT  smiAccessMode;
        first_time = 0;
        bspSmiInitDriver(&smiAccessMode);
    }

    return  smiReadReg(devSlvId, regAddr, valuePtr);
}

/*******************************************************************************
* bspSmiWriteReg
*
* DESCRIPTION:
*       Writes a register to an SMI slave.
*
* INPUTS:
*       devSlvId - Slave Device ID
*      actSmiAddr - actual smi addr to use (relevant for SX PPs)
*       regAddr - Register address to read from.
*       value   - data to be written.
*
* OUTPUTS:
*        None,
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspSmiWriteReg(IN MV_U32 devSlvId,
                         IN MV_U32 actSmiAddr,
                         IN MV_U32 regAddr,
                         IN MV_U32 value)
{
  return smiWriteReg(devSlvId, regAddr, value);
}

/*** Dragonite Driver ***/
#ifdef MV_INCLUDE_DRAGONITE
/*******************************************************************************
* bspDragoniteSWDownload
*
* DESCRIPTION:
*       Download new version of Dragonite firmware to Dragonite MCU
*
* INPUTS:
*       sourcePtr - Ptr to mem where new version of Dragonite firmware resides.
*       size      - size of firmware to download to ITCM.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspDragoniteSWDownload(const MV_VOID *src, MV_U32 size)
{
    MV_CHIP_FEATURES *featuresP = mvChipFeaturesGet();

    if (featuresP->numOfDragonite == 0)
    {
        return MV_FAIL;
    }

    return mvDragoniteSWDownload(src, size);
}

/*******************************************************************************
* bspDragoniteEnableSet
*
* DESCRIPTION: Enable/Disable Dragonite module
*
* INPUTS:
*       enable MV_TRUE  Dragonite MCU starts to work
*              MV_FALSE Dragonite MCU stops function
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       call after SW download
*
*******************************************************************************/
MV_STATUS bspDragoniteEnableSet(MV_BOOL enableFlag)
{
    MV_CHIP_FEATURES *featuresP = mvChipFeaturesGet();

    if (featuresP->numOfDragonite == 0)
    {
        return MV_FAIL;
    }

    return mvDragoniteEnableSet(enableFlag);
}

/*******************************************************************************
* bspDragoniteInit
*
* DESCRIPTION: Initialize Dragonite module
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       Application will call this before firmware download
*
*******************************************************************************/
MV_STATUS bspDragoniteInit(MV_VOID)
{
    MV_CHIP_FEATURES *featuresP = mvChipFeaturesGet();

    if (featuresP->numOfDragonite == 0)
    {
        return MV_FAIL;
    }

    return mvDragoniteInit();
}

/*******************************************************************************
* bspDragoniteSharedMemWrite
*
* DESCRIPTION:
*       Write a given buffer to the given offset in shared memory of Dragonite
*       microcontroller.
*
* INPUTS:
*       offset  - Offset from beginning of shared memory
*       buffer  - The buffer to be written.
*       length  - Length of buffer in bytes.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success.
*       MV_BAD_PARAM - out-of-boundary memory access
*       MV_FAIL - otherwise.
*
* COMMENTS:
*       Only DTCM is reachable
*
*******************************************************************************/
MV_STATUS bspDragoniteSharedMemWrite(MV_U32          offset,
                                     const MV_VOID  *buffer,
                                     MV_U32          length)
{
    MV_CHIP_FEATURES *featuresP = mvChipFeaturesGet();

    if (featuresP->numOfDragonite == 0)
    {
        return MV_FAIL;
    }

    return mvDragoniteSharedMemWrite(offset, buffer, length);
}

/*******************************************************************************
* bspDragoniteSharedMemRead
*
* DESCRIPTION:
*       Read a memory block from a given offset in shared memory of Dragonite
*        microcontroller.
*
* INPUTS:
*       offset  - Offset from beginning of shared memory
*       length  - Length of the memory block to read (in bytes).
*
* OUTPUTS:
*       buff  - The read data.
*
* RETURNS:
*       MV_OK   - on success.
*       MV_BAD_PARAM - out-of-boundary memory access.
*       MV_FAIL - otherwise.
*
* COMMENTS:
*       Only DTCM is reachanble
*
*******************************************************************************/
MV_STATUS bspDragoniteSharedMemRead(MV_U32 offset, MV_VOID *buff, MV_U32 length)
{
    MV_CHIP_FEATURES *featuresP = mvChipFeaturesGet();

    if (featuresP->numOfDragonite == 0)
    {
        return MV_FAIL;
    }

    return mvDragoniteSharedMemRead(offset, length, buff);
}

/*******************************************************************************
* bspDragoniteSharedMemoryBaseAddrGet
*
* DESCRIPTION:
*       Get start address of DTCM
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       dtcmPtr - Pointer to beginning of DTCM where communication structures
*                 must be placed
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspDragoniteSharedMemoryBaseAddrGet(MV_U32 *sharedMemP)
{
    MV_CHIP_FEATURES *featuresP = mvChipFeaturesGet();

    if (featuresP->numOfDragonite == 0)
    {
        return MV_FAIL;
    }

    return mvDragoniteSharedMemBaseAddrGet(sharedMemP);
}

/*******************************************************************************
* bspDragoniteFwCrcCheck
*
* DESCRIPTION:
*       This routine executes Dragonite firmware checksum test
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspDragoniteFwCrcCheck(MV_VOID)
{
    MV_CHIP_FEATURES *featuresP = mvChipFeaturesGet();

    if (featuresP->numOfDragonite == 0)
    {
        return MV_FAIL;
    }

    return mvDragoniteFwCrcCheck();
}
#endif /* MV_INCLUDE_DRAGONITE */


/*** Ethernet Driver ***/
/*
 * Defines
 */
#define BSP_ETH_MAX_BUF_PER_PKT            10
#define BSP_ETH_SYNC_TX_POLL_TIMEOUT       50
#define BSP_ETH_DEF_RX_Q                   MII_DEF_RXQ
#define BSP_ETH_DEF_TX_Q                   MII_DEF_TXQ
#define BSP_ETH_RX_BUFF_SIZE_DEFAULT       MII_RX_BUFF_SIZE_DEFAULT
#define BSP_ETH_MRU_DEFAULT                MII_MRU_DEFAULT

/*
 * Take care of mapping for RX packets.
 */
static MV_BOOL G_needToCfgCpuCodeToRxQMap = MV_FALSE;

/* Default: every CPU_Code is mapped to RX Queue 0. */
static MV_U32  G_dsaCpuCode[NUM_OF_SWITCH_CPU_CODES] = {0};

static MV_BOOL G_bspEthIsToDel2PrepBytes = MV_FALSE;

/*
 * Ethernet driver control structure.
 */
typedef struct _bspEthDrvCtrl
{
    BSP_RX_ISR_CALLBACK_FUNCPTR         rxReadyIsrCb;
    BSP_TX_DONE_ISR_CALLBACK_FUNCPTR    txDoneIsrCb;
    BSP_RX_CALLBACK_FUNCPTR             userRxReadyCb;
    BSP_TX_COMPLETE_CALLBACK_FUNCPTR    userTxDoneCb;

    MV_ULONG                            rxTaskId;
    MV_U32                              rxReadySemId;
    MV_U32                              rxPathSemId;
    MV_ULONG                            txTaskId;
    MV_U32                              txDoneSemId;
    MV_U32                              txPathSemId;

    GEN_SYNC_POOL                      *rxPktInfoPoolP;
    GEN_SYNC_POOL                      *txPktInfoPoolP;

    MV_U8                             **txBuffs;
    MV_U8                             **rxBuffs;
    MV_U32                             *rxBuffsLen;

    MV_BOOL                             isDsrMode;
    MV_BOOL                             isTxSyncMode;
    MV_GND_HW_FUNCS                    *hwP;

    bspEthNetPortType_ENT               muxPorts[MV_PP_NUM_OF_PORTS];

} BSP_ETH_DRV_CTRL;

/*
 * Struct needed to init Generic Network Driver (GND).
 */
static MV_GND_INIT        G_gndInit;
static MV_SWITCH_GEN_INIT G_swGenInit;

static MV_U32      G_rxDescNumPerQ[MV_NET_NUM_OF_RX_Q] = {0};
static MV_U32      G_txDescNumPerQ[MV_NET_NUM_OF_TX_Q] = {0};
static MV_U32      G_maxFragsInPkt =
        BSP_ETH_MAX_BUF_PER_PKT + MV_ETH_EXTRA_FRAGS_NUM;
static MV_BOOL     G_bspEthIsTxSyncMode = MV_TRUE;

/*
 * Forward declarations.
 */
extern MV_SWITCH_GEN_HOOK_FWD_RX_PKT       switchFwdRxPktToOs;
extern MV_SWITCH_GEN_HOOK_DRV_OS_PKT_FREE  switchDrvOsPktFree;

/*
 * Declarations
 */
static MV_VOID     bspEthRxPathLock   (MV_VOID);
static MV_VOID     bspEthRxPathUnlock (MV_VOID);
static MV_VOID     bspEthTxPathLock   (MV_VOID);
static MV_VOID     bspEthTxPathUnlock (MV_VOID);

static MV_VOID     bspEthRxReadyIsrCb (MV_VOID);
static MV_VOID     bspEthTxDoneIsrCb  (MV_VOID);
static void        bspEthRxReadyIsrGen(MV_VOID);
static void        bspEthTxDoneIsrGen (MV_VOID);
static void        bspEthRxReadyIsr   (MV_VOID);
static void        bspEthTxDoneIsr    (MV_VOID);

static MV_U32      bspEthRxReadyTask(void *arglist);
static MV_U32      bspEthTxDoneTask (void *arglist);
static MV_STATUS   bspEthCreateRxTask(MV_VOID);
static MV_STATUS   bspEthCreateTxTask(MV_VOID);

MV_STATUS          bspEthUserTxDone(MV_GND_PKT_INFO *pktInfoP);
MV_STATUS          bspEthFwdRxPkt(MV_GND_PKT_INFO *pktInfoP, MV_U32 rxQ);

/*
 * OS dependent declarations.
 */
MV_STATUS bspEthIntInit(void (*bspEthRxReadyIsrF)(void),
                        void (*bspEthTxDoneIsrF) (void));
MV_VOID   bspEthIntEnable (MV_BOOL rxIntEnable, MV_BOOL txIntEnable);
MV_VOID   bspEthIntDisable(MV_BOOL rxIntEnable, MV_BOOL txIntEnable);

/*
 * Globals
 */
static MV_U8                G_bspEthDefaultMacAddr[MV_MAC_ADDR_SIZE] =
                                        {0x00, 0x45, 0x78, 0x14, 0x59, 0x00};
static MV_BOOL              G_bspEthIsDrvDsrMode = MV_TRUE;
static BSP_ETH_DRV_CTRL    *G_bspEthDrvCtrlP     = NULL;
static MV_BOOL              G_bspEthIsInited     = MV_FALSE;

MV_GND_OS_FUNCS bspEthGndOsIf =
{
    /* .mvGndOsIsrCbRxReadyF       = */ bspEthRxReadyIsrCb,
    /* .mvGndOsIsrCbTxDoneF        = */ bspEthTxDoneIsrCb,
    /* .mvGndOsRxPathLockF         = */ bspEthRxPathLock,
    /* .mvGndOsRxPathUnlockF       = */ bspEthRxPathUnlock,
    /* .mvGndOsTxPathLockF         = */ bspEthTxPathLock,
    /* .mvGndOsTxPathUnlockF       = */ bspEthTxPathUnlock,
    /* .mvGndOsUserTxDoneF         = */ bspEthUserTxDone,
    /* .mvGndOsFwdRxPktF           = */ bspEthFwdRxPkt
};

MV_BOOL standalone_network_device = MV_TRUE;

/*
 * CREATE FILE FOR BSP ETH DRIVER !!!
 * CREATE FILE FOR BSP ETH DRIVER !!!
 * CREATE FILE FOR BSP ETH DRIVER !!!
 * CREATE FILE FOR BSP ETH DRIVER !!!
 * CREATE FILE FOR BSP ETH DRIVER !!!
 * CREATE FILE FOR BSP ETH DRIVER !!!
 * CREATE FILE FOR BSP ETH DRIVER !!!
 * CREATE FILE FOR BSP ETH DRIVER !!!
 * CREATE FILE FOR BSP ETH DRIVER !!!
 * CREATE FILE FOR BSP ETH DRIVER !!!
 * CREATE FILE FOR BSP ETH DRIVER !!!
 * CREATE FILE FOR BSP ETH DRIVER !!!
 * CREATE FILE FOR BSP ETH DRIVER !!!
 * CREATE FILE FOR BSP ETH DRIVER !!!
 * CREATE FILE FOR BSP ETH DRIVER !!!
 * CREATE FILE FOR BSP ETH DRIVER !!!
 */

/*******************************************************************************
 * bspEthDisableDsrMode
 */
MV_VOID bspEthDisableDsrMode(MV_VOID)
{
    G_bspEthIsDrvDsrMode = MV_FALSE;
}

/*******************************************************************************
 * bspEthRegisterRxIsrCb
 */
MV_VOID bspEthRegisterRxIsrCb(BSP_RX_ISR_CALLBACK_FUNCPTR rxReadyIsrCb)
{
    G_bspEthDrvCtrlP->rxReadyIsrCb = rxReadyIsrCb;
}

/*******************************************************************************
 * bspEthRegisterTxDoneIsrCb
 */
MV_VOID bspEthRegisterTxDoneIsrCb(BSP_TX_DONE_ISR_CALLBACK_FUNCPTR txDoneIsrCb)
{
    G_bspEthDrvCtrlP->txDoneIsrCb = txDoneIsrCb;
}

/*******************************************************************************
 * bspEthRxReadyQGet
 */
MV_U32 bspEthRxReadyQGet(MV_VOID)
{
    return miiRxReadyQGet();
}

/*******************************************************************************
 * bspEthTxDoneQGet
 */
MV_U32 bspEthTxDoneQGet(MV_VOID)
{
    return miiTxDoneQGet();
}

/*******************************************************************************
 * bspBitMaskFfs
 */
MV_U32 bspBitMaskFfs(MV_U32 bitMask)
{
    return mvBitMaskFfs(bitMask);
}

/*******************************************************************************
 * bspEthIsInited
 */
MV_BOOL bspEthIsInited(MV_VOID)
{
    return G_bspEthIsInited;
}

/*******************************************************************************
 * bspEthInit
 */
MV_VOID bspEthInit(MV_U8 port)
{
    BSP_ETH_DRV_CTRL *drvCtrlP;
    MV_U32            maxFragsInPkt = G_maxFragsInPkt;
    MV_GND_HW_FUNCS  *hwP           = NULL;
    MV_CHIP_FEATURES *featuresP          = mvChipFeaturesGet();

    static MV_BOOL funcAlreadyRun = MV_FALSE;
    if (funcAlreadyRun == MV_TRUE)
    {
        return;
    }
    funcAlreadyRun = MV_TRUE;
    G_bspEthIsInited = MV_TRUE;

    standalone_network_device = MV_FALSE;

    /*
     * Allocate and init the device structure.
     */
    drvCtrlP = (BSP_ETH_DRV_CTRL *)mvOsCalloc(1, sizeof (BSP_ETH_DRV_CTRL));
    if (drvCtrlP == NULL)
    {
        mvOsPrintf("%s: Alloc failed.\n", __func__);
        return;
    }
    G_bspEthDrvCtrlP = drvCtrlP;
    drvCtrlP->isDsrMode    = G_bspEthIsDrvDsrMode;
    drvCtrlP->isTxSyncMode = G_bspEthIsTxSyncMode;

    /*
     * If in DSR mode (old approach), then RX/TX tasks should be created.
     * TX task is created later.
     */
    if (drvCtrlP->isDsrMode == MV_TRUE)
    {
        /*
         * 'if' is needed, because LSP uses fake tasks and sems in kernel mode.
         */
        if (switchDrvGenIsInited() == MV_FALSE)
        {
            /*
             * Create RX task for processing RX_READY queues.
             */
            if (bspEthCreateRxTask() != MV_OK)
            {
                mvOsPrintf("%s: bspEthCreateRxTask failed.\n", __func__);
                return;
            }

            /*
             * Create semaphores for RX path.
             */
            if (mvOsSemCreate("rxPathSemId",
                              1, /* init sem value is empty  */
                              1, /* 1 ==> create binary sem */
                              &drvCtrlP->rxPathSemId) != MV_OK)
            {
                mvOsPrintf("%s: mvOsSemCreate failed.\n", __func__);
                return;
            }

            /*
             * Create semaphores for TX path.
             */
            if (mvOsSemCreate("txPathSemId",
                              1, /* init sem value is empty  */
                              1, /* 1 ==> create binary sem */
                              &drvCtrlP->txPathSemId) != MV_OK)
            {
                mvOsPrintf("%s: mvOsSemCreate failed.\n", __func__);
                return;
            }
        }

        /*
         * Prepare stuff for DSR mode.
         */
        drvCtrlP->txBuffs = (MV_U8 **)mvOsCalloc(maxFragsInPkt, sizeof(MV_U8 *));
        if (drvCtrlP->txBuffs == NULL)
        {
            mvOsPrintf("%s: mvOsCalloc failed.\n", __func__);
            return;
        }

        drvCtrlP->rxBuffs = (MV_U8 **)mvOsCalloc(maxFragsInPkt, sizeof(MV_U8 *));
        if (drvCtrlP->rxBuffs == NULL)
        {
            mvOsPrintf("%s: mvOsCalloc failed.\n", __func__);
            return;
        }

        drvCtrlP->rxBuffsLen = (MV_U32 *)mvOsCalloc(maxFragsInPkt, sizeof(MV_U32));
        if (drvCtrlP->rxBuffsLen == NULL)
        {
            mvOsPrintf("%s: mvOsCalloc failed.\n", __func__);
            return;
        }
    }
    else
    {
        /*
         * Upper layer (CPSS) explicitly requests to provide it with
         * received packet by calling appropriate API of BSP ETH driver,
         * ==> no callback is needed.
         */
    }

    if (switchDrvGenIsInited() == MV_FALSE)
    {
        mvOsMemset(&G_gndInit,   0, sizeof(MV_GND_INIT));
        mvOsMemset(&G_swGenInit, 0, sizeof(MV_SWITCH_GEN_INIT));

        /*
         * Register hardware interface.
         */
        hwP = mvGndHwIfGet(MV_GND_HW_IF_MII);
        if (hwP == NULL)
        {
            mvOsPrintf("%s: mvGndHwIfGet failed.\n", __func__);
            return;
        }
        G_gndInit.hwP = hwP;

        /*
         * Register upper layer interface.
         */
        G_gndInit.osP = &bspEthGndOsIf;

        /*
         * Set max number of RX/TX fragments in packet.
         */
        G_gndInit.maxFragsInPkt = G_maxFragsInPkt;

        G_gndInit.gbeIndex = featuresP->miiGbeIndex;
    }

    mvOsPrintf("Ethernet port #%d initialized.\n", port);
}

/*******************************************************************************
 * bspEthRxPathLock
 */
static MV_VOID bspEthRxPathLock(MV_VOID)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;

    if (mvOsSemWait(drvCtrlP->rxPathSemId, MV_OS_WAIT_FOREVER) != MV_OK)
    {
        mvOsPrintf("%s: mvOsSemWait failed.\n", __func__);
    }
}

/*******************************************************************************
 * bspEthRxPathUnlock
 */
static MV_VOID bspEthRxPathUnlock(MV_VOID)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;

    if (mvOsSemSignal(drvCtrlP->rxPathSemId) != MV_OK)
    {
        mvOsPrintf("%s: mvOsSemSignal failed.\n", __func__);
    }
}

/*******************************************************************************
 * bspEthTxPathLock
 */
static MV_VOID bspEthTxPathLock(MV_VOID)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;

    if (mvOsSemWait(drvCtrlP->txPathSemId, MV_OS_WAIT_FOREVER) != MV_OK)
    {
        mvOsPrintf("%s: mvOsSemWait failed.\n", __func__);
    }
}

/*******************************************************************************
 * bspEthTxPathUnlock
 */
static MV_VOID bspEthTxPathUnlock(MV_VOID)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;

    if (mvOsSemSignal(drvCtrlP->txPathSemId) != MV_OK)
    {
        mvOsPrintf("%s: mvOsSemSignal failed.\n", __func__);
    }
}

/*******************************************************************************
 * bspEthFwdRxPkt
 */
MV_STATUS bspEthFwdRxPkt(MV_GND_PKT_INFO *pktInfoP, MV_U32 rxQ)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;
    MV_U32 i;
	MV_GND_PKT_INFO *tmpPktInfoP;

    tmpPktInfoP = pktInfoP;
	for (i = 0; i < pktInfoP->numFrags; i++)
    {
        drvCtrlP->rxBuffs[i]    = tmpPktInfoP->pFrags->bufVirtPtr;
        drvCtrlP->rxBuffsLen[i] = tmpPktInfoP->pFrags->dataSize;

		if (genSyncPoolPut(drvCtrlP->rxPktInfoPoolP, tmpPktInfoP) != MV_OK)
		{
			mvOsPrintf("%s: genSyncPoolPut failed.\n", __func__);
			return MV_FAIL;
		}

        tmpPktInfoP = tmpPktInfoP->nextP;
    }
    /*
     * CPSS: do not remove CRC for RGMII-1, because MII is emulation of SDMA
     * and in SDMA case CPSS expects that length includes 4 bytes of CRC.
     */
    drvCtrlP->rxBuffsLen[pktInfoP->numFrags - 1] += 4  /* 4 bytes of CRC */;


    /*
     * The upper layer _has_ to copy RX buffers.
     */
    if (drvCtrlP->userRxReadyCb(drvCtrlP->rxBuffs,
                                drvCtrlP->rxBuffsLen,
                                pktInfoP->numFrags,
                                rxQ) != MV_OK)
    {
        mvOsPrintf("%s: userRxReadyCb failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * bspEthRxReadyIsrCb
 */
static MV_VOID bspEthRxReadyIsrCb(MV_VOID)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;

    if (mvOsSemSignal(drvCtrlP->rxReadySemId) != MV_OK)
    {
        /* mvOsPrintf("%s: mvOsSemSignal failed.\n", __func__); */
    }
}

/*******************************************************************************
 * bspEthTxDoneIsrCb
 */
static MV_VOID bspEthTxDoneIsrCb(MV_VOID)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;

    if (mvOsSemSignal(drvCtrlP->txDoneSemId) != MV_OK)
    {
        /* mvOsPrintf("%s: mvOsSemSignal failed.\n", __func__); */
    }
}

/*******************************************************************************
* bspEthRxReadyIsr
*
* DESCRIPTION:
*       RX ISR for MII mode.
*       For portability it only calls to generic RX ISR.
*
* INPUTS:
*       drvCtrlP  - Pointer to the driver control structure.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Nothing.
*
* COMMENTS:
*       BSP-Internal function, should not be exposed to CPSS.
*       This function is different for BSP and LSP.
*
*******************************************************************************/
static void bspEthRxReadyIsr(MV_VOID)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;

    if (drvCtrlP->isDsrMode == MV_TRUE)
    {
        /*
         * Call the registered generic ISR (old approach).
         * Generic ISR will forward (to upper layer, e.g. CPSS)
         * all the received packets to preregistered callback.
         */
        mvGndRxReadyIsr();
    }
    else
    {
        /*
         * Call the registered ISR callback (new approach).
         */
        bspEthRxReadyIsrGen();
    }
}

/*******************************************************************************
* bspEthTxDoneIsr
*
* DESCRIPTION:
*       TX_DONE ISR for MII mode. Calls to generic TX_DONE ISR.
*       For portability it only calls to generic TX_DONE ISR.
*
* INPUTS:
*       drvCtrlP  - Pointer to the driver control structure.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Nothing.
*
* COMMENTS:
*       BSP-Internal function, should not be exposed to CPSS.
*       This function is different for BSP and LSP.
*
*******************************************************************************/
static void bspEthTxDoneIsr(MV_VOID)
{
    BSP_ETH_DRV_CTRL *drvCtrlP;

    drvCtrlP = G_bspEthDrvCtrlP;

    if (drvCtrlP->isDsrMode == MV_TRUE)
    {
        /*
         * Call the registered generic ISR (old approach).
         * Generic ISR will forward (to upper layer, e.g. CPSS)
         * all the transmitted packets to preregistered callback.
         */
        mvGndTxDoneIsr();
    }
    else
    {
        /*
         * Call the registered ISR callback (new approach).
         */
        bspEthTxDoneIsrGen();
    }
}

/*******************************************************************************
* bspEthRxReadyIsrGen
*
* DESCRIPTION:
*       Generic RX ISR for MII mode.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Nothing.
*
* COMMENTS:
*       BSP-Internal function, should not be exposed to CPSS.
*       This function should be the same for BSP and LSP.
*       BSP/LSP actual interrupt should call to this function.
*
*******************************************************************************/
static void bspEthRxReadyIsrGen(MV_VOID)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;
    MV_U32            rxReadyQBitMask;

    /* Read cause register */
    rxReadyQBitMask = miiRxReadyQGet();

    /*
     * The ISR callback should mask and acknowledge interrupts.
     */
    drvCtrlP->rxReadyIsrCb(rxReadyQBitMask);
}

/*******************************************************************************
* bspEthTxDoneIsrGen
*
* DESCRIPTION:
*       Generic TX_DONE ISR for MII mode.
*
* INPUTS:
*       drvCtrlP  - Pointer to the driver control structure.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Nothing.
*
* COMMENTS:
*       BSP-Internal function, should not be exposed to CPSS.
*       This function should be the same for BSP and LSP.
*       BSP/LSP actual interrupt should call to this function.
*
*******************************************************************************/
static void bspEthTxDoneIsrGen(MV_VOID)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;
    MV_U32            txDoneQBitMask;

    /* Read cause register */
    txDoneQBitMask = miiTxDoneQGet();

    /*
     * The ISR callback should mask and acknowledge interrupts.
     */
    drvCtrlP->txDoneIsrCb(txDoneQBitMask);
}

/*******************************************************************************
 * bspEthPortRxInit
 */
MV_STATUS bspEthPortRxInit(MV_U32     rxBuffsBulkSize,
                           MV_U8     *rxBuffsBulkP,
                           MV_U32     rxBuffSize,
                           MV_U32    *actualRxBuffsNumP,
                           MV_U32     rxBuffHdrOffset,
                           MV_U32     numOfRxQueues,
                           MV_U32     rxQbufPercent[])
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;
    MV_U32            totalBuffNum = 0;
    MV_U32            percentSum   = 0, i;

    totalBuffNum = genBuffPoolCalcBuffNum(rxBuffsBulkSize,
                                          rxBuffsBulkP,
                                          rxBuffSize,
                                          rxBuffHdrOffset,
                                          miiBuffAlignGet());

    *actualRxBuffsNumP = totalBuffNum;

    /*
     * Distribute buffers among the queues according to percentage
     */
    for (i = 0; i < MV_ETH_RX_Q_NUM; i++)
    {
        G_rxDescNumPerQ[i] = (totalBuffNum * rxQbufPercent[i]) / 100;

        percentSum += rxQbufPercent[i];
        if (percentSum > 100)
        {
            mvOsPrintf("%s: Total percents > 100.\n", __func__);
            return MV_FAIL;
        }
    }

    G_gndInit.rx.rxBuffsBulkSize   = rxBuffsBulkSize;
    G_gndInit.rx.rxBuffsBulkP      = rxBuffsBulkP;
    G_gndInit.rx.rxBuffSize        = rxBuffSize;
    G_gndInit.rx.actualRxBuffsNumP = NULL;
    G_gndInit.rx.rxBuffHdrOffset   = rxBuffHdrOffset;
    G_gndInit.rx.rxDescNumPerQ     = G_rxDescNumPerQ;
    G_gndInit.rx.rxDescNumTotal    = totalBuffNum;
    G_gndInit.rx.numOfRxQueues     = MV_NET_NUM_OF_RX_Q;
    G_gndInit.rx.isToDel2PrepBytes = G_bspEthIsToDel2PrepBytes;
    mvOsMemcpy(G_gndInit.rx.macAddr, G_bspEthDefaultMacAddr, MV_MAC_ADDR_SIZE);
    G_gndInit.rx.mruBytes = 1522 + 8 /* DSA tag */;

    if (drvCtrlP->isDsrMode == MV_TRUE)
    {
        /*
         * For DSR mode (old approach) - create pool to hold pktInfos when
         * upper layer (CPSS) expects buffers only.
         */
        drvCtrlP->rxPktInfoPoolP = genSyncPoolCreate(totalBuffNum);
        if (drvCtrlP->rxPktInfoPoolP == NULL)
        {
            mvOsPrintf("%s: genSyncPoolCreate failed.\n", __func__);
            return MV_FAIL;
        }
    }

    return MV_OK;
}

/*******************************************************************************
 * bspEthPortTxInit
 */
MV_STATUS bspEthPortTxInit(MV_U32 txDescTotal)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;
    GEN_SYNC_POOL    *poolP;
    MV_U32            totalBuffNum  = 0;
    MV_U32            txDescPerQNum;
    MV_U32            i;
    MV_U32            maxFragsInPkt = G_maxFragsInPkt;

    /*
     * Calculate the number of TX descriptors per TX queue.
     */
    txDescPerQNum = txDescTotal / MV_ETH_TX_Q_NUM;
    for (i = 0; i < MV_ETH_TX_Q_NUM; i++)
    {
        G_txDescNumPerQ[i] = txDescPerQNum;
        totalBuffNum      += txDescPerQNum;
    }

    G_gndInit.tx.txDescNumPerQ     = G_txDescNumPerQ;
    G_gndInit.tx.txDescNumTotal    = totalBuffNum;
    G_gndInit.tx.numOfTxQueues     = MV_ETH_TX_Q_NUM;
    G_gndInit.tx.isTxSyncMode      = drvCtrlP->isTxSyncMode;
    G_gndInit.tx.pollTimoutUSec    = MV_GND_POLL_TIMOUT_USEC_DEFAULT;
    G_gndInit.tx.maxPollTimes      = MV_GND_POLL_COUNT_MAX_DEFAULT;

    if (drvCtrlP->isDsrMode == MV_TRUE)
    {
        /*
         * For DSR mode (old approach) - create pool to hold pktInfos when
         * upper layer (CPSS) sends to driver only buffers.
         */
        poolP = mvGndTxPktInfoPoolCreate(totalBuffNum, maxFragsInPkt);
        if (poolP == NULL)
        {
            mvOsPrintf("%s: mvGndTxPktInfoPoolCreate failed.\n", __func__);
            return MV_FAIL;
        }
        drvCtrlP->txPktInfoPoolP = poolP;
    }

    return MV_OK;
}

/*******************************************************************************
* bspEthRxReadyTask
*
* DESCRIPTION:
*       This is the main funtion of RX task, which is needed in DSR mode
*       (old approach). The task processes received by MII packets.
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
*       None.
*
*******************************************************************************/
static MV_U32 bspEthRxReadyTask(void *arglist)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;

    if (drvCtrlP->rxReadySemId == 0)
    {
        mvOsPrintf("%s: rxReadySemId doesn't exist.\n", __func__);
        return MV_FAIL;
    }

    if (drvCtrlP->userRxReadyCb == NULL)
    {
        mvOsPrintf("%s: userRxReadyCb is not configured.\n", __func__);
        return MV_FAIL;
    }

    while (MV_TRUE)
    {
        if (mvOsSemWait(drvCtrlP->rxReadySemId, MV_OS_WAIT_FOREVER) != MV_OK)
        {
            mvOsPrintf("%s: mvOsSemWait failed.\n", __func__);
            return MV_FAIL;
        }

        if (switchGenRxJob() != MV_OK)
        {
            mvOsPrintf("%s: switchGenRxJob failed.\n", __func__);
            return MV_FAIL;
        }
    }

    /* This function should never return. */
#ifndef _DIAB_TOOL
    return MV_OK; /* make the compiler happy */
#endif
}

/*******************************************************************************
* bspEthTxDoneTask
*
* DESCRIPTION:
*       This is the main funtion of TX task, which is needed in DSR mode
*       (old approach). The task processes transmitted by MII packets.
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
*       None.
*
*******************************************************************************/
static MV_U32 bspEthTxDoneTask(void *arglist)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;

    if (drvCtrlP->txDoneSemId == 0)
    {
        mvOsPrintf("%s: txDoneSemId doesn't exist.\n", __func__);
        return MV_FAIL;
    }

    if (drvCtrlP->userTxDoneCb == NULL)
    {
        mvOsPrintf("%s: userTxDoneCb is not configured.\n", __func__);
        return MV_FAIL;
    }

    while (MV_TRUE)
    {
        if (mvOsSemWait(drvCtrlP->txDoneSemId, MV_OS_WAIT_FOREVER) != MV_OK)
        {
            mvOsPrintf("%s: mvOsSemWait failed.\n", __func__);
            return MV_FAIL;
        }

        if (switchGenTxJob() != MV_OK)
        {
            mvOsPrintf("%s: switchGenRxJob failed.\n", __func__);
            return MV_FAIL;
        }
    }

    /* This function should never return. */
#ifndef _DIAB_TOOL
    return MV_OK; /* make the compiler happy */
#endif
}

/*******************************************************************************
* bspEthCreateRxTask
*
* DESCRIPTION:
*       Creates RX task for driver DSR mode (old approach).
*       Each tasks processes RX_READY queues and calls for each
*       frame the registered callback.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static MV_STATUS bspEthCreateRxTask(MV_VOID)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;
    MV_STATUS         status;

    /*
     * Create RX_READY task (DSR mode - old approach).
     * Check it was not created previously.
     */
    if (drvCtrlP->rxTaskId == 0)
    {
        status = mvOsTaskCreate("BSP_rx",
                                50                               /* priority    */,
                                0x2000                           /* stackSize   */,
                                bspEthRxReadyTask                /* entry point */,
                                NULL                             /* arg list    */,
                                &drvCtrlP->rxTaskId);
        if (status != MV_OK)
        {
            mvOsPrintf("%s: Could not spawn Rx task.\n", __func__);
            return MV_FAIL;
        }
    }

    /*
     * Create semaphores for RX_READY task (DSR mode - old approach).
     * Check it was not created previously.
     */
    if (drvCtrlP->rxReadySemId == 0)
    {
        if (mvOsSemCreate("rxReadyTaskSem",
                          0, /* init sem value is busy  */
                          1, /* 1 ==> create binary sem */
                          &drvCtrlP->rxReadySemId) != MV_OK)
        {
            mvOsPrintf("%s: mvOsSemCreate failed.\n", __func__);
            return MV_FAIL;
        }
    }

    return MV_OK;
}

/*******************************************************************************
* bspEthCreateTxTask
*
* DESCRIPTION:
*       Creates TX task for driver DSR mode (old approach).
*       Each tasks processes TX_DONE queues and calls for each
*       frame the registered callback.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static MV_STATUS bspEthCreateTxTask(MV_VOID)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;
    MV_STATUS         status;

    /*
     * Create TX_DONE task (DSR mode - old approach).
     * Check it was not created previously.
     */
    if (drvCtrlP->txTaskId == 0)
    {
        status = mvOsTaskCreate("BSP_tx",
                                50                           /* priority    */,
                                0x2000                       /* stackSize   */,
                                bspEthTxDoneTask             /* entry point */,
                                NULL                         /* arg list    */,
                                &drvCtrlP->txTaskId);
        if (status != MV_OK)
        {
            mvOsPrintf("%s: Could not spawn Tx task.\n", __func__);
            return MV_FAIL;
        }
    }

    /*
     * Create semaphores for TX_DONE task (DSR mode - old approach).
     * Check it was not created previously.
     */
    if (drvCtrlP->txDoneSemId == 0)
    {
        if (mvOsSemCreate("txDoneTaskSem",
                          0, /* init sem value is busy  */
                          1, /* 1 ==> create binary sem */
                          &drvCtrlP->txDoneSemId) != MV_OK)
        {
            mvOsPrintf("%s: mvOsSemCreate failed.\n", __func__);
            return MV_FAIL;
        }
    }

    return MV_OK;
}

/*******************************************************************************
 * bspEthPortEnable
 */
MV_STATUS bspEthPortEnable(MV_VOID)
{
    BSP_ETH_DRV_CTRL     *drvCtrlP = G_bspEthDrvCtrlP;
    MV_CHIP_FEATURES     *featuresP     = mvChipFeaturesGet();
    MV_SW_GEN_RX_HOOKS    rxHooks;
    MV_SW_GEN_TX_HOOKS    txHooks;
    MV_U32                hookId;
    MV_U32                cpuCode;

    if (switchDrvGenIsInited() == MV_TRUE)
    {
        /*
         * Switch Generic Network layer is in operational mode already.
         */
        return MV_OK;
    }

    if (drvCtrlP->isTxSyncMode == MV_TRUE)
    {
        mvOsPrintf("Switch: MII Syncronous Mode is used.\n");
    }
    else
    {
        mvOsPrintf("Switch: MII Asyncronous Mode is used.\n");
    }

    /*
     * Init and configure Generic Network Driver.
     */
    G_swGenInit.isMiiMode       = MV_TRUE;
    G_swGenInit.isTxSyncMode    = drvCtrlP->isTxSyncMode;
    G_swGenInit.gndInitP        = &G_gndInit;
    G_swGenInit.gbeDefaultIndex = featuresP->miiGbeIndex;

    if (switchDrvGenInit(&G_swGenInit) != MV_OK)
    {
        mvOsPrintf("%s: switchDrvGenInit failed.\n", __func__);
        return MV_FAIL;
    }

    /*
     * Register RX callbacks.
     */
    if (switchDrvGenHookSetCalcFwdRxPktHookId(
              (MV_SWITCH_GEN_HOOK_CALC_PKT_ID)mvSwitchGetPortNumFromExtDsa)
                                                                  != MV_OK)
    {
        mvOsPrintf("%s: switchDrvGenHookSetCalcFwdRxPktHookId failed.\n", __func__);
        return MV_FAIL;
    }

    if (switchGenHooksInitRx(MV_PP_NUM_OF_PORTS) != MV_OK)
    {
        mvOsPrintf("%s: switchGenHooksInitRx failed.\n", __func__);
        return MV_FAIL;
    }

    rxHooks.fwdRxReadyPktHookF = (MV_SWITCH_GEN_HOOK_FWD_RX_PKT)bspEthFwdRxPkt;
    rxHooks.hdrAltBeforeFwdRxPktHookF = NULL;
    rxHooks.hdrAltBeforeFwdRxFreePktHookF = NULL;

    for (hookId = 0; hookId < MV_PP_NUM_OF_PORTS; hookId++)
    {
        if (switchGenHooksFillRx(&rxHooks, hookId) != MV_OK)
        {
            mvOsPrintf("%s: switchGenHooksFillRx failed.\n", __func__);
            return MV_FAIL;
        }
    }

    /*
     * Register TX callbacks.
     */
    if (switchGenHooksInitTx(bspEthNetPortType_numOfTypes) != MV_OK)
    {
        mvOsPrintf("%s: switchGenHooksInitTx failed.\n", __func__);
        return MV_FAIL;
    }

    txHooks.hdrAltBeforeFwdTxDonePktHookF = NULL;
    txHooks.txDonePktFreeF  = (MV_SWITCH_GEN_HOOK_DRV_OS_PKT_FREE)bspEthUserTxDone;
    txHooks.hdrAltBeforeTxF = NULL;

    if (switchGenHooksFillTx(&txHooks, MV_NET_OWN_CPSS) != MV_OK)
    {
        mvOsPrintf("%s: switchGenHooksFillTx failed.\n", __func__);
        return MV_FAIL;
    }

    /*
     * Enable Generic Network Driver (GND).
     */
    if (mvGndEnable(BSP_ETH_DEF_RX_Q,
                    BSP_ETH_RX_BUFF_SIZE_DEFAULT,
                    BSP_ETH_MRU_DEFAULT) != MV_OK)
    {
        mvOsPrintf("%s: mvGndEnable failed.\n", __func__);
        return MV_FAIL;
    }

    /*
     *
     *
     *
     * VERIFY WE REALLY NEED IT!!!
     *
     *
     *
     */
    MV_REG_BIT_SET(ETH_PORT_CONFIG_REG(featuresP->miiGbeIndex),
                   ETH_UNICAST_PROMISCUOUS_MODE_MASK);

    /*
     * Init params that were configured before bspEthPortEnable.
     */
    if (G_needToCfgCpuCodeToRxQMap == MV_TRUE)
    {
        miiRxQMapSet(MV_ETH_RX_CPU_CODE_MAP);
        for (cpuCode = 0; cpuCode < NUM_OF_SWITCH_CPU_CODES; cpuCode++)
        {
            miiCpuCodeToRxQMap(cpuCode, G_dsaCpuCode[cpuCode]);
        }
    }

    /*
     * Init and Enable interrupts (OS dependent).
     * This should be done after the creation of RX/TX tasks (if any).
     */
    if (drvCtrlP->isTxSyncMode == MV_FALSE)
    {
        if (bspEthIntInit(bspEthRxReadyIsr, bspEthTxDoneIsr) != MV_OK)
        {
            mvOsPrintf("%s: bspEthIntInit failed.\n", __func__);
            return MV_FAIL;
        }
        bspEthIntEnable(MV_TRUE /* RX */, MV_TRUE  /* TX */);
        mvGndIntEnable (MV_TRUE /* RX */, MV_TRUE  /* TX */);
    }
    else
    {
        if (bspEthIntInit(bspEthRxReadyIsr, NULL /* TX */) != MV_OK)
        {
            mvOsPrintf("%s: bspEthIntInit failed.\n", __func__);
            return MV_FAIL;
        }
        bspEthIntEnable(MV_TRUE /* RX */, MV_FALSE /* TX */);
        mvGndIntEnable (MV_TRUE /* RX */, MV_FALSE /* TX */);
    }

    /*
     * RX Queue mapping should be configued before RX interrupts are enabled.
     */
#if USE_BSP_API_MII_RX_MAP
    miiRxQMapSet(MV_ETH_RX_CPU_CODE_MAP);
    miiAllCpuCodesToRxQMap(BSP_ETH_DEF_RX_Q);
#endif

    return MV_OK;
}

/*******************************************************************************
 * bspEthPortDisable
 */
MV_STATUS bspEthPortDisable(MV_VOID)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;

    if (switchDrvGenIsInited() == MV_TRUE)
    {
        /*
         * BSP ETH Driver works through Switch Generic Network layer only.
         */
        return MV_OK;
    }

    if (drvCtrlP->isTxSyncMode == MV_TRUE)
    {
        bspEthIntDisable(MV_TRUE /* RX */, MV_FALSE /* TX */);
        mvGndIntDisable (MV_TRUE /* RX */, MV_FALSE /* TX */);
    }
    else
    {
        bspEthIntDisable(MV_TRUE /* RX */, MV_TRUE  /* TX */);
        mvGndIntDisable (MV_TRUE /* RX */, MV_TRUE  /* TX */);
    }

    /* Stop RX, TX and disable the Ethernet port */
    if (miiPortDisable() != MV_OK)
    {
        mvOsPrintf("%s: miiPortDisable failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * bspEthRxBuffSizeGet
 */
MV_U32 bspEthRxBuffSizeGet(MV_VOID)
{
    return mvGndBuffSizeGet();
}

/*******************************************************************************
 * bspEthMruGet
 */
BSP_ETH_MRU bspEthMruGet(MV_VOID)
{
    MV_ETH_MRU mru = miiMruGet();

    if (mru == MV_ETH_MRU_ILLEGAL)
    {
        mvOsPrintf("%s: miiMruGet failed.\n", __func__);
    }

    return (BSP_ETH_MRU)mru;
}

/*******************************************************************************
 * bspEthMruSet
 */
MV_STATUS bspEthMruSet(BSP_ETH_MRU mru)
{
    if (miiMruSet(mru) != MV_OK)
    {
        mvOsPrintf("%s: miiMruSet failed.\n", __func__);
    }

    return MV_OK;
}

/*******************************************************************************
 * bspEthPortTxModeSet
 */
MV_STATUS bspEthPortTxModeSet(bspEthTxMode_ENT txMode)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;
    MV_BOOL           txSyncFlag;

    if (txMode == bspEthTxMode_synch_E)
    {
        txSyncFlag = MV_TRUE;
    }
    else if (txMode == bspEthTxMode_asynch_E)
    {
        txSyncFlag = MV_FALSE;
        /*
         * Create TX task for processing TX_DONE queues.
         */
        if (switchDrvGenIsInited() == MV_FALSE)
        {
            if (bspEthCreateTxTask() != MV_OK)
            {
                mvOsPrintf("%s: bspEthCreateTxTask failed.\n", __func__);
                return MV_FAIL;
            }
        }
    }
    else
    {
        mvOsPrintf("%s: Wrong TX mode (%d).\n", __func__, txMode);
        return MV_FAIL;
    }

    /*
     * Execute or store configuration.
     */
    if (mvGndIsInited() == MV_TRUE)
    {
        if (txSyncFlag == MV_TRUE)
        {
            mvGndIsTxSyncModeSet(MV_GND_POLL_TIMOUT_USEC_DEFAULT,
                                 MV_GND_POLL_COUNT_MAX_DEFAULT);

            bspEthIntDisable(MV_FALSE /* RX */, MV_TRUE  /* TX */);
            mvGndIntDisable (MV_FALSE /* RX */, MV_TRUE  /* TX */);
        }
        else
        {
            mvGndIsTxSyncModeUnset();

            bspEthIntEnable(MV_TRUE /* RX */, MV_TRUE  /* TX */);
            mvGndIntEnable (MV_TRUE /* RX */, MV_TRUE  /* TX */);
        }
    }
    else
    {
        drvCtrlP->isTxSyncMode       = txSyncFlag;
        G_gndInit.tx.isTxSyncMode    = txSyncFlag;
        G_gndInit.tx.pollTimoutUSec  = MV_GND_POLL_TIMOUT_USEC_DEFAULT;
        G_gndInit.tx.maxPollTimes    = MV_GND_POLL_COUNT_MAX_DEFAULT;
    }

    return MV_OK;
}

/*******************************************************************************
 * bspEthPortTx
 */
MV_STATUS bspEthPortTx(MV_U8 *buffs[], MV_U32 buffsLen[], MV_U32 buffNum)
{
    if (bspEthPortTxQueue(buffs, buffsLen, buffNum, BSP_ETH_DEF_TX_Q) != MV_OK)
    {
        mvOsPrintf("%s: mvGndSendBuffs failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * bspSendPkt
 */
MV_STATUS bspSendPkt(MV_GND_PKT_INFO *pktInfoP, MV_U32 txQ)
{
    if (mvGndSendPkt(pktInfoP, txQ) != MV_OK)
    {
        mvOsPrintf("%s: mvGndSendPkt failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * bspEthBuildPktWrapper
 */
MV_GND_PKT_INFO *bspEthBuildPktWrapper(MV_U8 *buffs[],
                                       MV_U32 buffsLen[],
                                       MV_U32 buffNum)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;
    MV_GND_PKT_INFO  *pktInfoP;
    MV_GND_BUF_INFO  *bufInfoP;
    MV_U32            i;

    if (buffNum > G_maxFragsInPkt)
    {
        mvOsPrintf("%s: buffNum(%d) > G_maxFragsInPkt(%d).\n",
                   __func__, buffNum, G_maxFragsInPkt);
        return NULL;
    }

    /*
     * Allocate packet wrapper (analagous to mBlk, skb etc.).
     */
    pktInfoP = (MV_GND_PKT_INFO *)genSyncPoolGet(drvCtrlP->txPktInfoPoolP);
    if (pktInfoP == NULL)
    {
        mvOsPrintf("%s: genSyncPoolGet failed.\n", __func__);
        return NULL;
    }

    /*
     * Fill packet wrapper.
     */
    bufInfoP = pktInfoP->pFrags;
    for (i = 0; i < buffNum; i++)
    {
        bufInfoP[i].bufVirtPtr   = buffs[i];
    #ifdef ETH_DESCR_IN_HIGH_MEM
        bufInfoP[i].bufVirtPtr  = (MV_U8* )bspPhys2Virt((MV_U32)(buffs[i]));
        bufInfoP[i].bufPhysAddr  = buffs[i];
    #else
        bufInfoP[i].bufPhysAddr  = mvOsIoVirtToPhy(NULL, buffs[i]);
    #endif
        bufInfoP[i].dataSize     = buffsLen[i];

        pktInfoP->pktSize       += buffsLen[i];
    }
    pktInfoP->numFrags = buffNum;
    pktInfoP->status   = 0;

    return pktInfoP;
}

/*******************************************************************************
 * bspEthPortTxQueue
 */
MV_STATUS bspEthPortTxQueue(MV_U8 *buffs[],
                            MV_U32 buffsLen[],
                            MV_U32 buffNum,
                            MV_U32 txQ)
{
    MV_GND_PKT_INFO  *pktInfoP;
    MV_GND_PKT_INFO  *pktWrapperP;

    if (buffNum > mvGndMaxFragsInPktGet())
    {
        mvOsPrintf("%s: Too many fragments (%d).\n", __func__, buffNum);
        return MV_FAIL;
    }

    /*
     * Alloc and fill packet wrapper (analagous to mBlk, skb etc.).
     */
    pktWrapperP = bspEthBuildPktWrapper(buffs, buffsLen, buffNum);
    if (pktWrapperP == NULL)
    {
        mvOsPrintf("%s: bspEthBuildPktWrapper failed.\n", __func__);
        return MV_FAIL;
    }

    if (switchDrvGenIsInited() == MV_TRUE)
    {
        /*
         * Alloc pktInfo for TX purposes.
         */
        pktInfoP = switchGenTxPktInfoGet();
        if (pktInfoP == NULL)
        {
            mvOsPrintf("%s: switchGenTxPktInfoGet failed.\n", __func__);
            return MV_FAIL;
        }

        /*
         * Prepare pktInfo to be freed in TX_DONE operation.
         */
        if (pktInfoToPktInfo(pktWrapperP /* from */, pktInfoP /* to */) != MV_OK)
        {
            mvOsPrintf("%s: pktInfoToPktInfo failed.\n", __func__);
            return MV_FAIL;
        }
        pktInfoP->ownerId = MV_NET_OWN_CPSS;

        /*
         * Actual send.
         */
        if (switchGenSendPkt(pktInfoP, txQ) != MV_OK)
        {
            mvOsPrintf("%s: switchGenSendPkt failed.\n", __func__);
            return MV_FAIL;
        }
    }
    else
    {
        if (mvGndSendPkt(pktWrapperP, txQ) != MV_OK)
        {
            mvOsPrintf("%s: mvGndSendPkt failed.\n", __func__);
            return MV_FAIL;
        }
    }

    return MV_OK;
}

/*******************************************************************************
 * bspGetPkt
 */
MV_GND_PKT_INFO *bspGetPkt(MV_U32 rxQ)
{
    return mvGndGetRxPkt(rxQ);
}

/*******************************************************************************
 * bspGetTxDonePkt
 */
MV_GND_PKT_INFO *bspGetTxDonePkt(MV_U32 txQ)
{
    MV_GND_HW_FUNCS  *hwP      = G_bspEthDrvCtrlP->hwP;

    return (MV_GND_PKT_INFO *)hwP->mvGndHwGetPktTxDoneF(txQ);
}

/*******************************************************************************
 * bspEthRxPktFree
 */
MV_STATUS bspEthRxPktFree(MV_GND_PKT_INFO *pktInfoP, MV_U32 rxQ)
{
    if (mvGndFreeRxPkt(pktInfoP, rxQ) != MV_OK)
    {
        mvOsPrintf("%s: mvGndFreeRxPkt failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * bspEthRxPacketFree
 */
MV_STATUS bspEthRxPacketFree(MV_U8 *buffs[], MV_U32 buffNum, MV_U32 rxQ)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;
    MV_GND_PKT_INFO  *pktInfoP;
    MV_GND_BUF_INFO  *bufInfoP;
    MV_U32            i;

    for (i = 0; i < buffNum; i++)
    {
        pktInfoP = (MV_GND_PKT_INFO *)genSyncPoolGet(drvCtrlP->rxPktInfoPoolP);
        if (pktInfoP == NULL)
        {
            mvOsPrintf("%s: genSyncPoolGet failed.\n", __func__);
            return MV_FAIL;
        }

        bufInfoP              = pktInfoP->pFrags;
        pktInfoP->pktSize     = bufInfoP->bufSize;

        bufInfoP->bufVirtPtr  = buffs[i];
    #ifdef ETH_DESCR_IN_HIGH_MEM
        bufInfoP->bufPhysAddr = mvOsIoAddrToPhy(NULL, buffs[i]);
    #else
        bufInfoP->bufPhysAddr = mvOsIoVirtToPhy(NULL, buffs[i]);
    #endif
        bufInfoP->dataSize    = bufInfoP->bufSize;

        if (mvGndFreeRxPkt(pktInfoP, rxQ) != MV_OK)
        {
            mvOsPrintf("%s: mvGndFreeRxPkt failed.\n", __func__);
            return MV_FAIL;
        }
    }

    return MV_OK;
}

/*******************************************************************************
 * bspEthUserTxDone
 *
 * Note:
 *     Should be used for the new approach (not DSR mode).
 */
MV_STATUS bspEthUserTxDone(MV_GND_PKT_INFO *pktInfoP)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;
    MV_GND_BUF_INFO  *bufInfoP;
    MV_U32            numFrags = pktInfoP->numFrags;
    MV_U32            i;

    /*
     * Prepare array of pointers to transmitted buffers (global array!)
     * in order to deliver them to upper layer (e.g. CPSS ).
     */
    bufInfoP = pktInfoP->pFrags;
    for (i = 0; i < numFrags; i++)
    {
        drvCtrlP->txBuffs[i] = bufInfoP[i].bufVirtPtr;
    }

    if (genSyncPoolPut(drvCtrlP->txPktInfoPoolP, pktInfoP) != MV_OK)
    {
        mvOsPrintf("%s: genSyncPoolPut failed.\n", __func__);
        return MV_FAIL;
    }

    /*
     * 'txBuffs' must be copied by the callback.
     */
    if (drvCtrlP->userTxDoneCb(drvCtrlP->txBuffs, numFrags) != MV_OK)
    {
        mvOsPrintf("%s: userTxDoneCb failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * bspEthFreeTxPkt
 *
 * Note:
 *     Should be used for the new approach (not DSR mode).
 */
MV_STATUS bspEthFreeTxPkt(MV_GND_PKT_INFO *pktInfoP)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;

    if (genSyncPoolPut(drvCtrlP->txPktInfoPoolP, pktInfoP) != MV_OK)
    {
        mvOsPrintf("%s: genSyncPoolPut failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * bspMiiTxDoneIntMask
 */
MV_VOID bspMiiTxDoneIntMask(MV_U32 txQBitMask)
{
    miiTxDoneIntMask(txQBitMask);
}

/*******************************************************************************
 * bspMiiTxDoneIntUnmask
 */
MV_VOID bspMiiTxDoneIntUnmask(MV_U32 txQBitMask)
{
    miiTxDoneIntUnmask(txQBitMask);
}

/*******************************************************************************
 * bspMiiRxReadyIntMask
 */
MV_VOID bspMiiRxReadyIntMask(MV_U32 rxQBitMask)
{
    miiRxReadyIntMask(rxQBitMask);
}

/*******************************************************************************
 * bspMiiRxReadyIntUnmask
 */
MV_VOID bspMiiRxReadyIntUnmask(MV_U32 rxQBitMask)
{
    miiRxReadyIntUnmask(rxQBitMask);
}

/*******************************************************************************
 * bspMiiRxReadyIntAck
 */
MV_VOID bspMiiRxReadyIntAck(MV_U32 rxQBitMask)
{
    miiRxReadyIntAck(rxQBitMask);
}

/*******************************************************************************
 * bspMiiTxDoneIntAck
 */
MV_VOID bspMiiTxDoneIntAck(MV_U32 txQBitMask)
{
    miiTxDoneIntAck(txQBitMask);
}

/*******************************************************************************
 * bspEthInputHookAdd
 */
MV_STATUS bspEthInputHookAdd(BSP_RX_CALLBACK_FUNCPTR userRxReadyCb)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;

    drvCtrlP->userRxReadyCb = userRxReadyCb;

    return MV_OK;
}

/*******************************************************************************
 * bspEthTxCompleteHookAdd
 */
MV_STATUS bspEthTxCompleteHookAdd(BSP_TX_COMPLETE_CALLBACK_FUNCPTR userTxDoneCb)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;

    drvCtrlP->userTxDoneCb = userTxDoneCb;

    return MV_OK;
}

/*******************************************************************************
 * bspEthCpuCodeToQueue
 */
MV_STATUS bspEthCpuCodeToQueue(MV_U32 dsaCpuCode, MV_U32 rxQueue)
{
    MV_ETH_RX_MAPPING map = MV_ETH_RX_DEFAULT_MAP;

    if (mvGndIsInited() == MV_TRUE)
    {
        /* Verify that GbE RX queueing policy uses CPU_Code. */
        map = miiRxQMapGet();
        if (map != MV_ETH_RX_CPU_CODE_MAP)
        {
            miiRxQMapSet(MV_ETH_RX_CPU_CODE_MAP);
        }
        miiCpuCodeToRxQMap(dsaCpuCode, rxQueue);
    }
    else
    {
        /*
         * Remember params to use on init.
         */
        G_needToCfgCpuCodeToRxQMap = MV_TRUE;
        G_dsaCpuCode[dsaCpuCode] = rxQueue;
    }

    return MV_OK;
}

/*******************************************************************************
 * bspEthPrePendTwoBytesHeaderSet
 */
MV_STATUS bspEthPrePendTwoBytesHeaderSet(MV_BOOL enable)
{
    if (bspEthIsInited() == MV_TRUE)
    {
        if (mvGndIsInited() == MV_TRUE)
        {
            if (enable == MV_TRUE)
            {
                mvGndDel2PrepBytesModeSet(MV_FALSE);
            }
            else
            {
                mvGndDel2PrepBytesModeSet(MV_TRUE);
            }
        }
        else
        {
            /*
             * Remember params to use on init.
             */
            if (enable == MV_TRUE)
            {
                G_gndInit.rx.isToDel2PrepBytes = MV_FALSE;
            }
            else
            {
                G_gndInit.rx.isToDel2PrepBytes = MV_TRUE;
            }
        }
    }
    else
    {
        if (enable == MV_TRUE)
        {
            G_bspEthIsToDel2PrepBytes = MV_FALSE;
        }
        else
        {
            G_bspEthIsToDel2PrepBytes = MV_TRUE;
        }
    }

    return MV_OK;
}

/*******************************************************************************
 * bspEthPrePendTwoBytesHeaderGet
 */
MV_STATUS bspEthPrePendTwoBytesHeaderGet(MV_BOOL *pEnable)
{
    *pEnable = mvGndDel2PrepBytesModeGet();

    return MV_OK;
}

/*******************************************************************************
 * bspEthMuxEnable
 */
MV_STATUS bspEthMuxEnable(MV_BOOL enable)
{
    /*
     * Do nothing.
     * The MUX is buit-in in the SwitchGen software layer.
     * Only appropriate RX and TX callbacks should be initialized.
     */

    return MV_OK;
}

/*******************************************************************************
 * bspEthMuxSet
 */
MV_STATUS bspEthMuxSet(MV_U32 portNum, bspEthNetPortType_ENT portType)
{
    BSP_ETH_DRV_CTRL     *drvCtrlP = G_bspEthDrvCtrlP;
    MV_SW_GEN_RX_HOOKS    rxHooks;
    MV_SW_GEN_TX_HOOKS    txHooks;

    if (portNum >= MV_PP_NUM_OF_PORTS)
    {
        mvOsPrintf("%s: Wrong params (portNum = %d).\n", __func__, portNum);
        return MV_FAIL;
    }

    if (bspEthIsInited() == MV_FALSE)
    {
        mvOsPrintf("%s: BSP ETH driver is not initialized.\n", __func__);
        return MV_FAIL;
    }

    if (switchDrvGenIsInited() == MV_FALSE)
    {
        mvOsPrintf("%s: SwitchGen is not initialized.\n", __func__);
        return MV_FAIL;
    }

    if (portType != bspEthNetPortType_cpssOwned_E                &&
        portType != bspEthNetPortType_osOwned_RxRemoveTxAddDsa_E &&
        portType != bspEthNetPortType_osOwned_RxLeaveTxNoAddDsa_E)
    {
        mvOsPrintf("%s: Wrong portType(%d).\n", __func__, portType);
        return MV_FAIL;
    }

    /*
     * Just remember the configuration to use when needed.
     */
    drvCtrlP->muxPorts[portNum] = portType;

    /*
     * Open RX path for MUX.
     */
    if (switchDrvGenHookSetCalcFwdRxPktHookId(
                                    mvSwitchGetPortNumFromExtDsa) != MV_OK)
    {
        mvOsPrintf("%s: switchDrvGenHookSetCalcFwdRxPktHookId failed.\n",
                   __func__);
        return MV_FAIL;
    }

    /*
     * Register RX callbacks.
     */
    if (portType == bspEthNetPortType_cpssOwned_E)
    {
        rxHooks.fwdRxReadyPktHookF =
                (MV_SWITCH_GEN_HOOK_FWD_RX_PKT)bspEthFwdRxPkt;
        rxHooks.hdrAltBeforeFwdRxPktHookF =
                NULL;
        rxHooks.hdrAltBeforeFwdRxFreePktHookF =
                NULL;
    }
    else if (portType == bspEthNetPortType_osOwned_RxRemoveTxAddDsa_E)
    {
        rxHooks.fwdRxReadyPktHookF =
                (MV_SWITCH_GEN_HOOK_FWD_RX_PKT)switchFwdRxPktToOs;
        rxHooks.hdrAltBeforeFwdRxPktHookF =
                (MV_SWITCH_GEN_HOOK_HDR_ALT_RX)mvSwitchExtractExtDsa;
        rxHooks.hdrAltBeforeFwdRxFreePktHookF =
                (MV_SWITCH_GEN_HOOK_HDR_ALT_RX_FREE)mvSwitchInjectExtDsa;
    }
    else if (portType == bspEthNetPortType_osOwned_RxLeaveTxNoAddDsa_E)
    {
        rxHooks.fwdRxReadyPktHookF =
                (MV_SWITCH_GEN_HOOK_FWD_RX_PKT)switchFwdRxPktToOs;
        rxHooks.hdrAltBeforeFwdRxPktHookF =
                NULL;
        rxHooks.hdrAltBeforeFwdRxFreePktHookF =
                NULL;
    }

    if (switchGenHooksFillRx(&rxHooks, portNum) != MV_OK)
    {
        mvOsPrintf("%s: switchGenHooksFillRx failed.\n", __func__);
        return MV_FAIL;
    }

    /*
     * Register TX callbacks.
     */

    if (portType == bspEthNetPortType_cpssOwned_E)
    {
        txHooks.hdrAltBeforeFwdTxDonePktHookF =
                NULL;
        txHooks.hdrAltBeforeTxF =
                NULL;
        txHooks.txDonePktFreeF  =
                (MV_SWITCH_GEN_HOOK_DRV_OS_PKT_FREE)bspEthUserTxDone;
    }
    else if (portType == bspEthNetPortType_osOwned_RxRemoveTxAddDsa_E)
    {
        txHooks.hdrAltBeforeFwdTxDonePktHookF =
                (MV_SWITCH_GEN_HOOK_HDR_ALT_TX)mvSwitchRemoveExtDsa;
        txHooks.hdrAltBeforeTxF =
                (MV_SWITCH_GEN_HOOK_HDR_ALT_TX)mvSwitchInjectExtDsa;
        txHooks.txDonePktFreeF  =
                (MV_SWITCH_GEN_HOOK_DRV_OS_PKT_FREE)switchDrvOsPktFree;
    }
    else if (portType == bspEthNetPortType_osOwned_RxLeaveTxNoAddDsa_E)
    {
        txHooks.hdrAltBeforeFwdTxDonePktHookF =
                NULL;
        txHooks.hdrAltBeforeTxF =
                NULL;
        txHooks.txDonePktFreeF  =
                (MV_SWITCH_GEN_HOOK_DRV_OS_PKT_FREE)switchDrvOsPktFree;
    }

    if (switchGenHooksFillTx(&txHooks, portType) != MV_OK)
    {
        mvOsPrintf("%s: switchGenHooksFillTx failed.\n", __func__);
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
 * bspEthMuxGet
 */
bspEthNetPortType_ENT bspEthMuxGet(MV_U32 portNum,
                                   bspEthNetPortType_ENT *portTypeP)
{
    BSP_ETH_DRV_CTRL *drvCtrlP = G_bspEthDrvCtrlP;

    if (portNum >= MV_PP_NUM_OF_PORTS)
    {
        mvOsPrintf("%s: Wrong porn number (%d).\n", __func__, portNum);
        return MV_FAIL;
    }

    if (bspEthIsInited() == MV_FALSE)
    {
        mvOsPrintf("%s: BSP ETH driver is not initialized.\n", __func__);
        return MV_FAIL;
    }

    if (switchDrvGenIsInited() == MV_FALSE)
    {
        mvOsPrintf("%s: SwitchGen is not initialized.\n", __func__);
        return MV_FAIL;
    }

    *portTypeP = drvCtrlP->muxPorts[portNum];

    return MV_OK;
}


/*******************************************************************************
* bspHsuMalloc
*
* DESCRIPTION:
*       Allocate a free area for HSU usage.
*
* INPUTS:
*       size_t bytes - number of bytes to allocate
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       pointer to allocated data per success
*       NULL - per failure to allocate space
*
* COMMENTS:
*       None
*
*******************************************************************************/
void          *hsuBase = (void *)0x2000000;
unsigned long  hsuLen  = 0x1000000;

void *bspHsuMalloc(IN size_t bytes)
{
  return hsuBase;  
}

/*******************************************************************************
 * bspCacheDmaMalloc
 *
 * DESCRIPTION:
 *       Allocate a cache free area for DMA devices.
 *
 * INPUTS:
 *       size_t bytes - number of bytes to allocate
 *
 * OUTPUTS:
 *       None.
 *
 * RETURNS:
 *       virtual address of area
 *       NULL - per failure to allocate space
 *
 * COMMENTS:
 *       None
 *
 *******************************************************************************/

void *dmaBase = NULL;
void *dmaTop;
void *dmaFree;
int  dmaLen;

void *bspCacheDmaMalloc(IN size_t bytes_in)
{
  void *ptr;
  size_t bytes = (bytes_in + 3) &  0xfffffffc; /* align */

  if (!dmaBase) /* first time ? */
  {
    dmaBase = (void *)((unsigned long)hsuBase + 0x800000); /* hsu area + 8MB */
    dmaFree = dmaBase;
    dmaTop = (void *)((unsigned long)hsuBase + hsuLen);
    dmaLen = 0x800000;
  }

  ptr = dmaFree;
  dmaFree = (void *)((unsigned long)dmaFree + bytes);
  if (ptr > dmaTop)    
  {
    mvOsPrintf("%s: dma memory exhausted.\n", __func__);
    return NULL;
  }

#if 0
  printf(">>> bspCacheDmaMalloc: size_in=0x%08x, size=0x%08x, "
         " ret_ptr=0x%08x\n", (int)bytes_in, (int)bytes, (int)ptr);
#endif

  return ptr;
}

/*******************************************************************************
 * bspCacheDmaFree
 *
 * DESCRIPTION:
 *       free a cache free area back to pool.
 *
 * INPUTS:
 *       size_t bytes - number of bytes to allocate
 *
 * OUTPUTS:
 *       None.
 *
 * RETURNS:
 *       GT_OK   - on success
 *       GT_FAIL - on error
 *
 * COMMENTS:
 *       None
 *
 *******************************************************************************/
MV_STATUS bspCacheDmaFree(void * pBuf)
{
  STUB_FAIL;
}

/*******************************************************************************
 * bspHsuFree
*
* DESCRIPTION:
*       free a hsu area back to pool.
*
* INPUTS:
*       size_t bytes - number of bytes to allocate
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
MV_STATUS bspHsuFree(void * pBuf)
{
  STUB_FAIL;
}

/*******************************************************************************
* bspWarmRestart
*
* DESCRIPTION:
*       This routine performs warm restart.
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

MV_STATUS bspWarmRestart
(
    MV_VOID
)
{
  hsuRebootBootRom();

  while(1); /* no return */
#ifndef _DIAB_TOOL
  return MV_OK; /* make the compiler happy */
#endif
}
