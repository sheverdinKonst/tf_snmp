#ifdef _DIAB_TOOL
volatile static void empty_func(void)
{
}
#else
#include "VxWorks.h"
#include <stdlib.h>
#include "config.h"
#include "mvCtrlEnvRegs.h"
#include "mvSysHwConfig.h"
#include "mvTypes.h"
#include "mvCommon.h"
#include "mvNflash.h"
#include "mvPrestera.h"
#include "mvPresteraPriv.h"
#include "mvDebug.h"

/******************************* twsi defintions ******************************/
#define TWSI_TEST
#ifdef TWSI_TEST
#include "mvTwsi.h"
#define TWSI_ADDR 0x50
#define TWSI_TYPE ADDR7_BIT
#define TWSI_MORE_THEN_256 MV_TRUE
#define TWSI_REPEAT 10
#define TWSI_START_ADDR 0
MV_U32 twsiTest();
#endif /* TWSI_TEST */

/******************************* Led defintions *******************************/

#define DEBUG_LED_TEST
#ifdef DEBUG_LED_TEST
void debugLedTest();
#include "mvBoardEnvLib.h"
#define LED_REPEAT 30
#endif /* DEBUG_LED_TEST */


/******************************* MMU defintions *******************************/

#define DEBUG_MMU_TEST
#ifdef DEBUG_MMU_TEST
#include "arch/arm/mmuArmLib.h"
#include "mvDramIf.h"
void mmuTest();
#endif /* DEBUG_MMU_TEST */


/******************************* SPI defintions *******************************/
#ifdef MV_INCLUDE_SPI
#define SPI_TEST
#endif /*MV_INCLUDE_SPI*/


#ifdef SPI_TEST
MV_U32 spiTest();
#include "mvSFlash.h"
MV_SFLASH_INFO G_spiFlashInfo;
#define SPI_TEST_START_ADDR 0x340000 /* disable override of the u-boot and BSP images */
#define SPI_TEST_LAST_ADDR 0x7f0000 /* disable override of the createBootrom images */
#define SPI_TEST_BYTE_NUM 4
#endif /* SPI_TEST */

/******************************* NAND defintions *******************************/
#ifdef MV_INCLUDE_NAND
#define NAND_TEST
#endif /*MV_INCLUDE_NAND*/


#ifdef NAND_TEST
MV_U32 nandTest();
#include "mvNFlash.h"
MV_NFLASH_INFO *G_nandFlashInfoP;
#define NAND_TEST_START_ADDR 0x340000 /* disable override of the u-boot and BSP images */
#define NAND_TEST_BYTE_NUM 4
#define CB_SRC_BLOCK 30
#define CB_DST_BLOCK 40

#endif /* NAND_TEST */

/******************************* XOR defintions *******************************/
#define DEBUG_XOR_TEST
#include "mvXor.h"
#include "mvSysXor.h"

MV_U32 copyChunks[] = {1, 2, 4, 8, 32, 64, 128, 256, 512,
    _1KB, _2KB, _4KB, _8KB, _10KB, _16KB, _32KB, _64KB,
    _128KB, _256KB, _512KB, _1MB, _2MB, _4MB, _8MB};
#define COPY_CHUNK_NUM      (sizeof(copyChunks) / sizeof(MV_U32))
#define copyTotals          copyChunks
#define COPY_TOTAL_NUM      COPY_CHUNK_NUM

XorBurstLimit xorBurstLimits[] = { XorBurstLimit_32bytes,
                                   XorBurstLimit_64bytes,
                                   XorBurstLimit_128bytes };
#define XOR_BURST_LIMITS_LEN (3)

#ifdef DMV_INCLUDE_CESA
#include "mvSysCesa.h"
#endif /*DMV_INCLUDE_CESA*/
#ifdef DEBUG_XOR_TEST
void xorTest();
#define XOR_ARRAY_NBYTES         0x100000 /* 1M */
#define XOR_ARRAY_NBYTES_DRAM    0x100000 /* 1M */
#define XOR_ARRAY_NBYTES_SMALL   0x20 /* 32 bytes */

#endif /* DEBUG_XOR_TEST */

/******************************* SMI defintions *******************************/

#define DEBUG_SMI_TEST
#ifdef DEBUG_SMI_TEST
#include "mvEthPhy.h"
void smiTest();

#endif /* DEBUG_SMI_TEST */

/******************************** main test function **************************/
MV_U32 kwTest() {

#ifdef TWSI_TEST
        twsiTest();
#endif /* TWSI_TEST */

#ifdef DEBUG_LED_TEST
        debugLedTest();
#endif /* DEBUG_LED_TEST */

#ifdef SPI_TEST
        spiTest();
#endif SPI_TEST

#ifdef NAND_TEST
        nandTest();
#endif NAND_TEST

#ifdef DEBUG_MMU_TEST
        mmuTest();
#endif /* DEBUG_MMU_TEST */

#ifdef DEBUG_SMI_TEST
        smiTest();
#endif /* DEBUG_SMI_TEST */

#ifdef XOR_TEST
        xorTest;
#endif /* XOR_TEST */

        return MV_OK;
}

void debugLedTest()
{
        MV_U32 led_val = 0;
        MV_U32 i;

        for (i = 0; i < LED_REPEAT; i++)
        {
                led_val = (led_val % (1 << mvBoardDebugLedNumGet(mvBoardIdGet())));
                mvBoardDebugLed(led_val);
                mvOsDelay(100);
                led_val++;
        }
}

MV_U32 twsiTest()
{
    MV_TWSI_SLAVE twsiSlave;
    MV_U8 writeRegVal = 0,readRegVal = 0, oldVal;
    MV_U32 i;

    twsiSlave.slaveAddr.address = TWSI_ADDR;
    twsiSlave.slaveAddr.type = TWSI_TYPE;
    twsiSlave.validOffset = MV_TRUE;
    twsiSlave.moreThen256 = TWSI_MORE_THEN_256;

    for (i = TWSI_START_ADDR; i < TWSI_REPEAT; i++)
    {
        twsiSlave.offset = i;
        writeRegVal = (mvOsRand() % 256);

        /* save current value */
        if (MV_OK != mvTwsiRead (MV_BOARD_DIMM_I2C_CHANNEL,&twsiSlave, &oldVal, 1))
        {
            mvOsPrintf("TWSI: Read fail offset %d\n",twsiSlave.offset);
            return MV_ERROR;
        }

        if( MV_OK != mvTwsiWrite (MV_BOARD_DIMM_I2C_CHANNEL,&twsiSlave, &writeRegVal, 1) )
        {
            mvOsPrintf("TWSI: Write fail offset %d\n",twsiSlave.offset);
            return MV_ERROR;
        }

        mvOsDelay(10);

        if( MV_OK != mvTwsiRead (MV_BOARD_DIMM_I2C_CHANNEL,&twsiSlave, &readRegVal, 1) )
        {
            mvOsPrintf("TWSI: Read fail offset %d\n",twsiSlave.offset);
            return MV_ERROR;
        }

        if (readRegVal != writeRegVal)
        {
            mvOsPrintf("TWSI: compare fail offset %d write value 0x%x read value 0x%x\n",
                       twsiSlave.offset,writeRegVal,readRegVal);
            return MV_ERROR;
        }

        /*  return to old value */
        if( MV_OK != mvTwsiWrite (MV_BOARD_DIMM_I2C_CHANNEL,&twsiSlave, &oldVal, 1) )
        {
            mvOsPrintf("TWSI: Write fail offset %d\n",twsiSlave.offset);
            return MV_ERROR;
        }
        mvOsDelay(10);
    }

    mvOsPrintf("TWSI: test pass\n");
    return MV_OK;
}

/*******************************************************************************
* twsiWriteEprom
*
* DESCRIPTION:
*       None.
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
MV_U32 twsiWriteEprom()
{
        MV_TWSI_SLAVE twsiSlave;
        MV_U32 writeRegArr[8] = {0xd0010004,0x3311,0xd0020154,0x200,0xd0020148,0x1,0xd002014c,0x1c00};
        MV_U8 writeRegVal;
        MV_U32 i;

        twsiSlave.slaveAddr.address = TWSI_ADDR;
        twsiSlave.slaveAddr.type = TWSI_TYPE;
        twsiSlave.validOffset = MV_TRUE;
        twsiSlave.moreThen256 = TWSI_MORE_THEN_256;

        for (i = TWSI_START_ADDR; i < (8 * 4); i++)
        {
                twsiSlave.offset = i;
                writeRegVal = (writeRegArr[i / 4] >> ((3 - (i % 4)) * 8)) & 0xff;

                if( MV_OK != mvTwsiWrite (MV_BOARD_DIMM_I2C_CHANNEL,&twsiSlave, &writeRegVal, 1) )
                {
                        mvOsPrintf("TWSI: Write fail offset %d\n",twsiSlave.offset);
                        return MV_ERROR;
                }

                mvOsDelay(10);
        }
        return MV_OK;
}

/*******************************************************************************
* twsiReadEprom
*
* DESCRIPTION:
*       None.
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
MV_U32 twsiReadEprom()
{
        MV_TWSI_SLAVE twsiSlave;
        MV_U32 regVal = 0;
        MV_U8 readRegVal = 0;
        MV_U32 i;

        twsiSlave.slaveAddr.address = TWSI_ADDR;
        twsiSlave.slaveAddr.type = TWSI_TYPE;
        twsiSlave.validOffset = MV_TRUE;
        twsiSlave.moreThen256 = TWSI_MORE_THEN_256;

        for (i = TWSI_START_ADDR; i < (8 * 4); i++)
        {
                twsiSlave.offset = i;

           if( MV_OK != mvTwsiRead (MV_BOARD_DIMM_I2C_CHANNEL, &twsiSlave, &readRegVal, 1) )
                {
                        mvOsPrintf("TWSI: Read fail offset %d\n",twsiSlave.offset);
                        return MV_ERROR;

                }

                regVal |= readRegVal << ((3 - (i % 4)) * 8);

                if ((i % 4) == 3)
                {
                        mvOsPrintf("\n%d 0x%08x",(i / 4),regVal);
                        regVal = 0;
                }

        }
        mvOsPrintf("TWSI: test pass\n");
        return MV_OK;
}


#ifdef SPI_TEST

MV_U32 spiTest() {
        MV_U32 sectorSize = G_spiFlashInfo.sectorSize;
        MV_U32 i,j;
        MV_U8 writeVal[SPI_TEST_BYTE_NUM];
        MV_U8 readVal[SPI_TEST_BYTE_NUM];

        if (mvSFlashWpRegionSet(&G_spiFlashInfo,MV_WP_NONE) != MV_OK)
        {
        mvOsPrintf("%s: mvSFlashWpRegionSet failed.\n", __func__);
                return ERROR;
        }

    mvOsPrintf("SPI: test started.\n");

   for (i = SPI_TEST_START_ADDR / G_spiFlashInfo.sectorSize;
                i < ((SPI_TEST_LAST_ADDR / G_spiFlashInfo.sectorSize) - 1) ; i++)
        {
        mvOsPrintf(".");

                /* erase the sector */
                if(mvSFlashSectorErase(&G_spiFlashInfo ,i) != MV_OK)
                {
                        mvOsPrintf("SPI: test Failed to erase Flash sector %d\n",i);
                        continue;
                }

                for (j = 0; j <SPI_TEST_BYTE_NUM; j++)
                {
                        writeVal[j] = mvOsRand() % 256;
                }

                if(mvSFlashBlockWr (&G_spiFlashInfo, (i * sectorSize) , writeVal, SPI_TEST_BYTE_NUM) != MV_OK)
                {
                        mvOsPrintf("SPI: test Failed to write to Flash\n");
                        return ERROR;
                }
                if(mvSFlashBlockRd (&G_spiFlashInfo, (i * sectorSize) , readVal, SPI_TEST_BYTE_NUM) != MV_OK)
                {
                        mvOsPrintf("SPI: test Failed to read to Flash\n");
                        return ERROR;
                }
                for (j = 0; j < SPI_TEST_BYTE_NUM; j++)
                {
                        if (readVal[j] != writeVal[j])
                        {
                                mvOsPrintf("SPI: compare fail sector %d byte %d write value 0x%x read value 0x%x\n",
                                                   i,j,writeVal[j],readVal[j]);
                                return ERROR;
                        }
                }
        }
    mvOsPrintf("\nSPI: test pass\n");
        return MV_OK;

}
#endif /* SPI_TEST */



#ifdef NAND_TEST

MV_U32 nandTest() {
        MV_U32 i,j, block,offset, lastBlock;
        MV_U8 writeVal[NAND_TEST_BYTE_NUM];
        MV_U8 readVal[NAND_TEST_BYTE_NUM];
        MV_U8 readCbVal[NAND_TEST_BYTE_NUM];
        MV_U32 srcBlockOffset, dstBlockOffset;

        lastBlock = G_nandFlashInfoP->pNflashStruct->blockNum + 1;
        /* found the relevant block number */
    for (block = 0; block < lastBlock; block++)
        {
                offset = mvNflashBlkOffsGet(G_nandFlashInfoP,block);

                if ((NAND_TEST_START_ADDR >= offset) &&
                        (NAND_TEST_START_ADDR < (offset +
                                                           (G_nandFlashInfoP->pNflashStruct->pagesPerBlk *
                                                                G_nandFlashInfoP->pNflashStruct->pageDataSize))))
                {
                        /* found the block */
                        mvOsPrintf("start from block %d\n",block);
                        break;
                }
        }

        if (block == lastBlock)
        {
                mvOsPrintf("Error - offset not in valid block");
                return ERROR;
        }

   for (i = block; i < lastBlock; i += 20)
        {
           mvOsPrintf(" %d",i);
           offset = mvNflashBlkOffsGet(G_nandFlashInfoP,i);

                /* erase the sector */
           if(mvNflashBlockErase(G_nandFlashInfoP ,i) != MV_OK)
                {
                        mvOsPrintf("NAND test Failed to erase Flash boot sector\n");
                        return ERROR;
                }

                for (j = 0; j <NAND_TEST_BYTE_NUM; j++)
                {
                        writeVal[j] = mvOsRand() % 256;
                }

        if (mvNflashPageProg (G_nandFlashInfoP, offset , NAND_TEST_BYTE_NUM,
                                                   writeVal, MV_FALSE) != MV_OK)
                {
                        mvOsPrintf("NAND: test Failed to write to Flash\n");
                        return ERROR;
                }
                if((mvNflashPageRead(G_nandFlashInfoP,offset,NAND_TEST_BYTE_NUM,readVal)
       != NAND_TEST_BYTE_NUM))
                {
                        mvOsPrintf("NAND: test Failed to read to Flash\n");
                        return ERROR;
                }

                for (j = 0; j < NAND_TEST_BYTE_NUM; j++)
                {
                        if (readVal[j] != writeVal[j])
                        {
                mvOsPrintf("NAND: compare fail block %d byte %d "
                           "write value 0x%x read value 0x%x\n",
                                                   i,j,writeVal[j],readVal[j]);
                                return ERROR;
                        }
                }

                /* write to spare */
                for (j = 0; j <NAND_TEST_BYTE_NUM; j++)
                {
                        writeVal[j] = mvOsRand() % 256;
                }

        if (mvNflashPageProg (G_nandFlashInfoP, offset, NAND_TEST_BYTE_NUM,
                                                     writeVal,MV_TRUE) != MV_OK)
                {
                        mvOsPrintf("NAND: test Failed to write to spare Flash\n");
                        return ERROR;
                }

                if((mvNflashLSpareFRead(G_nandFlashInfoP,offset,NAND_TEST_BYTE_NUM,readVal)
       != NAND_TEST_BYTE_NUM))
                {
                        mvOsPrintf("NAND: test Failed to read to spare Flash\n");
                        return ERROR;
                }

                for (j = 0; j < NAND_TEST_BYTE_NUM; j++)
                {
                        if (readVal[j] != writeVal[j])
                        {
                                mvOsPrintf("NAND: compare fail spare block %d byte %d write value 0x%x read value 0x%x\n",
                                                   i,j,writeVal[j],readVal[j]);
                                return ERROR;
                        }
                }
        }

   /* run copy back test */
   srcBlockOffset = mvNflashBlkOffsGet(G_nandFlashInfoP,20 + block);
   dstBlockOffset = mvNflashBlkOffsGet(G_nandFlashInfoP,(20 * 2) + block);

   if(mvNflashBlockErase(G_nandFlashInfoP ,(20 * 2) + block) != MV_OK)
        {
                mvOsPrintf("NAND test Failed to erase Flash boot sector\n");
                return ERROR;
        }

   if(mvNflashCpBackProg(G_nandFlashInfoP,srcBlockOffset,dstBlockOffset,
                                                 MV_FALSE, MV_FALSE) != MV_OK)
   {
           mvOsPrintf("NAND: test Failed to call mvNflashCpBackProg\n");
           return ERROR;

   }
   if((mvNflashPageRead(G_nandFlashInfoP,srcBlockOffset,NAND_TEST_BYTE_NUM,readVal)
  != NAND_TEST_BYTE_NUM))
   {
           mvOsPrintf("NAND: test Failed to read to Flash\n");
           return ERROR;
   }
   if((mvNflashPageRead(G_nandFlashInfoP,dstBlockOffset,NAND_TEST_BYTE_NUM,readCbVal)
  != NAND_TEST_BYTE_NUM))
   {
           mvOsPrintf("NAND: test Failed to read to Flash\n");
           return ERROR;
   }

   for (j = 0; j < NAND_TEST_BYTE_NUM; j++)
   {
           if (readVal[j] != readCbVal[j])
           {
                   mvOsPrintf("NAND: compare fail compare Copy Back\n");
                   return ERROR;
           }
   }

        mvOsPrintf("\nNAND: test pass\n");
        return MV_OK;
}
#endif /* NAND_TEST */

void mmuTest()
{
    MV_U32 sdramCsSize;
    MV_U32 dramBaseAddr;
    MV_U32 i;
    MV_U32 data;
    MV_U32 realSize;

    /* Access DRAM memory */
    for(i = 0; i < MV_DRAM_MAX_CS; i++)
    {
        sdramCsSize = mvDramIfBankSizeGet(i);
        if(sdramCsSize != 0)
        {
            realSize = ROUND_UP(sdramCsSize, PAGE_SIZE);
            dramBaseAddr = mvDramIfBankBaseGet(i);
            mvOsPrintf("\nCS %d size 0x%x base 0x%x \n",
                       i,sdramCsSize,dramBaseAddr);
            data = MV_MEMIO32_READ(dramBaseAddr);
            data = MV_MEMIO32_READ(dramBaseAddr + (realSize - 4));
        }
    }
    mvOsPrintf("DRAM access pass\n");

#ifdef MV_INCLUDE_CESA
    /* access SRAM memory */
    data = MV_MEMIO32_READ(CRYPT_ENG_BASE);
    data = MV_MEMIO32_READ(CRYPT_ENG_BASE + (CRYPT_ENG_SIZE - 4));
    mvOsPrintf("SRAM access pass\n");
#endif

    /* access to PEX memory */
    data = MV_MEMIO32_READ(PEX0_MEM_BASE);
    data = MV_MEMIO32_READ(PEX0_MEM_BASE + (PEX0_MEM_SIZE - 4));
    mvOsPrintf("PEX access pass\n");

    /* access internal registers */
    data = MV_MEMIO32_READ(INTER_REGS_BASE);
    data = MV_MEMIO32_READ(INTER_REGS_BASE + (INTER_REGS_SIZE - 4));
    mvOsPrintf("Internal access pass\n");

#ifdef MV_INCLUDE_SAGE
    {
    MV_U32 devId = 0, vendorId = 0, dev;
    if(mvBoardIdGet() != DB_88F6281A_BP_ID)
    {
        for (dev = 0; dev < mvSwitchGetDevicesNum(); dev++) {
            CHK_STS_VOID(mvSwitchReadReg(dev, PRESTERA_VENDOR_ID_REG, &vendorId));
            CHK_STS_VOID(mvSwitchReadReg(dev, PRESTERA_DEV_ID_REG,    &devId));
            mvOsPrintf("Prestera access through %s pass: "
                       "vendor id = 0x%X, dev id = 0x%X\n",
                       dev == 0 ? "SAGE" : "PEX", vendorId, devId);
        }
    }
    }
#endif
}

#define XOR_TIMEOUT 0x8000000
#define XOR_CAUSE_DONE_MASK(chan) ((BIT0|BIT1) << (chan * 16) )

void xor_waiton_eng(int chan)
{
    int timeout = 0;

    while(!(MV_REG_READ(XOR_CAUSE_REG(XOR_UNIT(chan))) & XOR_CAUSE_DONE_MASK(XOR_CHAN(chan))))
    {
        if(timeout > XOR_TIMEOUT)
            goto timeout;
        timeout++;
    }

    timeout = 0;
    while(mvXorStateGet(chan) != MV_IDLE)
    {
        if(timeout > XOR_TIMEOUT)
            goto timeout;
        timeout++;
    }
    /* Clear int */
    MV_REG_WRITE(XOR_CAUSE_REG(XOR_UNIT(chan)), ~(XOR_CAUSE_DONE_MASK(XOR_CHAN(chan))));

timeout:
    if(timeout > XOR_TIMEOUT)
    {
        mvOsPrintf("ERR: XOR eng got timedout!!\n");

    }
    return;
}

void xorDram2DramTest()
{
    MV_U32 chan, i, physAddr;
    MV_XOR_DESC *xorDesc;
    MV_U32 *to, *from;

    mvOsPrintf("XOR DRAM to DRAM: test started.\n");

    for(chan = 0; chan < MV_XOR_MAX_CHAN; chan++)
    {
        xorDesc = mvOsIoUncachedMemalign(NULL, MV_XOR_DESC_ALIGNMENT,
                                         sizeof(MV_XOR_DESC), &physAddr);
        if (!xorDesc)
        {
            mvOsPrintf("\n%s: mvOsIoUncachedMemalign failed.\n", __func__);
            return;
        }

        if (mvXorStateGet(chan) != MV_IDLE)
            mvOsPrintf("ERR: %s XOR chan %d is not idle", __func__, chan);

        to   = (MV_U32 *)mvOsIoUncachedMalloc(NULL, XOR_ARRAY_NBYTES_DRAM, NULL, NULL);
        from = (MV_U32 *)mvOsIoUncachedMalloc(NULL, XOR_ARRAY_NBYTES_DRAM, NULL, NULL);

        if (!from || !to)
        {
            mvOsPrintf("%s: mem alloc failed.\n", __func__);
            return;
        }

        for (i = 0; i < XOR_ARRAY_NBYTES_DRAM / 4; i++)
        {
            from[i] = i + 1;
            to  [i] = 0xFFFFFFFF;
        }

        memset(xorDesc, 0, sizeof(MV_XOR_DESC));
        xorDesc->srcAdd0        = (MV_U32)from;
        xorDesc->phyDestAdd     = (MV_U32)to;
        xorDesc->byteCnt        = XOR_ARRAY_NBYTES_DRAM;
        xorDesc->phyNextDescPtr = 0;
        xorDesc->status         = BIT31;
    #ifdef CONFIG_MV88F6281
        mvOsBridgeReorderWA();
    #endif

        if (mvXorTransfer(chan, MV_DMA, physAddr) != MV_OK)
        {
            mvOsPrintf("%s: DMA copy operation on channel %d failed!\n",
                       __func__, chan);
            mvOsIoUncachedFree(NULL, 0, 0, to,      NULL);
            mvOsIoUncachedFree(NULL, 0, 0, from,    NULL);
            mvOsIoUncachedFree(NULL, 0, 0, xorDesc, NULL);
            return;
        }

        xor_waiton_eng(chan);
        if (!(xorDesc->status & BIT30))
        {
            mvOsPrintf("%s: DMA copy operation status is wrong on channel %d!\n",
                       __func__, chan);
            mvOsIoUncachedFree(NULL, 0, 0, to,      NULL);
            mvOsIoUncachedFree(NULL, 0, 0, from,    NULL);
            mvOsIoUncachedAlignedFree(xorDesc);
            return;
        }

        for (i = 0; i < XOR_ARRAY_NBYTES_DRAM / 4; i++)
            if (to[i] != i + 1)
                mvOsPrintf("XOR Fail memcmp channel %d offset 0x%x value 0x%x\n",
                           chan, i, to[i]);

        mvOsIoUncachedFree(NULL, 0, 0, to,      NULL);
        mvOsIoUncachedFree(NULL, 0, 0, from,    NULL);
        mvOsIoUncachedAlignedFree(xorDesc);
    }

    mvOsPrintf("XOR DRAM to DRAM: test pass.\n");
}

void xorDram2DramSrcBurstLimitTest(XorBurstLimit srcBurstLimit)
{
    MV_U32 chan, i, physAddr;
    MV_XOR_DESC *xorDesc;
    MV_U32 *to, *from;

    mvOsPrintf("XOR DRAM to DRAM: test started: srcBurtsLimit = %d.\n",
                srcBurstLimit);

    for(chan = 0; chan < MV_XOR_MAX_CHAN; chan++)
    {
        xorSetSrcBurstLimit(chan, srcBurstLimit);

        xorDesc = mvOsIoUncachedMemalign(NULL, MV_XOR_DESC_ALIGNMENT,
                                         sizeof(MV_XOR_DESC), &physAddr);
        if (!xorDesc)
        {
            mvOsPrintf("\n%s: mvOsIoUncachedMemalign failed.\n", __func__);
            return;
        }

        if (mvXorStateGet(chan) != MV_IDLE)
            mvOsPrintf("ERR: %s XOR chan %d is not idle", __func__, chan);

        to   = (MV_U32 *)mvOsIoUncachedMalloc(NULL, XOR_ARRAY_NBYTES_DRAM, NULL, NULL);
        from = (MV_U32 *)mvOsIoUncachedMalloc(NULL, XOR_ARRAY_NBYTES_DRAM, NULL, NULL);

        if (!from || !to)
        {
            mvOsPrintf("%s: mem alloc failed.\n", __func__);
            return;
        }

        for (i = 0; i < XOR_ARRAY_NBYTES_DRAM / 4; i++)
        {
            from[i] = i + 1;
            to  [i] = 0xFFFFFFFF;
        }

        memset(xorDesc, 0, sizeof(MV_XOR_DESC));
        xorDesc->srcAdd0        = (MV_U32)from;
        xorDesc->phyDestAdd     = (MV_U32)to;
        xorDesc->byteCnt        = XOR_ARRAY_NBYTES_DRAM;
        xorDesc->phyNextDescPtr = 0;
        xorDesc->status         = BIT31;
    #ifdef CONFIG_MV88F6281
        mvOsBridgeReorderWA();
    #endif

        if (mvXorTransfer(chan, MV_DMA, physAddr) != MV_OK)
        {
            mvOsPrintf("%s: DMA copy operation on channel %d failed!\n",
                       __func__, chan);
            mvOsIoUncachedFree(NULL, 0, 0, to,      NULL);
            mvOsIoUncachedFree(NULL, 0, 0, from,    NULL);
            mvOsIoUncachedFree(NULL, 0, 0, xorDesc, NULL);
            return;
        }

        xor_waiton_eng(chan);
        if (!(xorDesc->status & BIT30))
        {
            mvOsPrintf("%s: DMA copy operation status is wrong on channel %d!\n",
                       __func__, chan);
            mvOsIoUncachedFree(NULL, 0, 0, to,      NULL);
            mvOsIoUncachedFree(NULL, 0, 0, from,    NULL);
            mvOsIoUncachedAlignedFree(xorDesc);
            return;
        }

        for (i = 0; i < XOR_ARRAY_NBYTES_DRAM / 4; i++)
            if (to[i] != i + 1)
                mvOsPrintf("XOR Fail memcmp channel %d offset 0x%x value 0x%x\n",
                           chan, i, to[i]);

        mvOsIoUncachedFree(NULL, 0, 0, to,      NULL);
        mvOsIoUncachedFree(NULL, 0, 0, from,    NULL);
        mvOsIoUncachedAlignedFree(xorDesc);
    }

    mvOsPrintf("XOR DRAM to DRAM: test pass..\n");
}

#ifdef MV_INCLUDE_SPI
void xorSpi2DramTest()
{
    MV_U32 chan, i, physAddr, data, to_init_val = 0xAABBEE44;
    MV_XOR_DESC *xorDesc;
    MV_U32 *to;

    mvOsPrintf("XOR SPI to DRAM: test started.\n");

    for(chan = 0; chan < MV_XOR_MAX_CHAN; chan++)
    {
        /* set src burst limit to 32 */
        MV_U32 temp = MV_REG_READ(XOR_CONFIG_REG(XOR_UNIT(chan),XOR_CHAN(chan)));
        temp &= ~XEXCR_SRC_BURST_LIMIT_MASK;
        temp |= 2 << XEXCR_SRC_BURST_LIMIT_OFFS;

        MV_REG_WRITE(XOR_CONFIG_REG(XOR_UNIT(chan),XOR_CHAN(chan)),temp);

        xorDesc = mvOsIoUncachedMemalign(NULL, MV_XOR_DESC_ALIGNMENT,
                                         sizeof(MV_XOR_DESC), &physAddr);

        if (xorDesc == NULL)
        {
            mvOsPrintf("%s: Etrror malloc failed.\n", __func__);
            return;
        }


        if(mvXorStateGet(chan) != MV_IDLE)
            mvOsPrintf("ERR: %s XOR chan %d is not idle", __func__, chan);

        to = (MV_U32 *)mvOsIoUncachedMalloc(NULL, XOR_ARRAY_NBYTES, NULL, NULL);

        if (to == NULL)
        {
            mvOsPrintf("XOR Fail allocate memory \n");
            return;
        }

        for (i=0; i < (XOR_ARRAY_NBYTES / 4); i++)
        {
            to[i] = to_init_val;
        }

        memset(xorDesc,0,sizeof(MV_XOR_DESC));
        xorDesc->srcAdd0 = (SPI_CS_BASE + NV_BOOT_OFFSET);
        xorDesc->phyDestAdd = (MV_U32)to;
        xorDesc->byteCnt = XOR_ARRAY_NBYTES;
        xorDesc->phyNextDescPtr = 0;
        xorDesc->status = BIT31;
    #ifdef CONFIG_MV88F6281
        mvOsBridgeReorderWA();
    #endif
        if( mvXorTransfer(chan, MV_DMA, physAddr) != MV_OK)
        {
            mvOsPrintf("%s: DMA copy operation on channel %d failed!\n", __func__, chan);
            mvOsIoUncachedFree(NULL, 0, 0, to,      NULL);
            mvOsIoUncachedAlignedFree(xorDesc);
            return;
        }
        xor_waiton_eng(chan);

        if (!(xorDesc->status & BIT30)) {
            mvOsPrintf("%s: DMA copy operation status is wrong on channel %d!\n", __func__, chan);
            mvOsIoUncachedFree(NULL, 0, 0, to,      NULL);
            mvOsIoUncachedAlignedFree(xorDesc);
            return;
        }

        for (i=0; i < (XOR_ARRAY_NBYTES / 4); i++)
        {
            if (mvSFlashBlockRd(&G_spiFlashInfo, NV_BOOT_OFFSET + (i * 4),(MV_U8*)&data,4) != MV_OK)
            {
                mvOsPrintf("\n failed read flashBootSector");
                return;
            }

            if (to[i] != data)
            {
                mvOsPrintf("XOR Fail compare memory channel %d offset 0x%x "
                           "value 0x%x origValue 0x%x\n", chan, i, to[i], data);
                break;
            }
        }

        mvOsIoUncachedFree(NULL, 0, 0, to,      NULL);
        mvOsIoUncachedAlignedFree(xorDesc);
    }

    mvOsPrintf("XOR SPI to DRAM: test pass.\n");
}

void xorSetSrcBurtLimit(MV_U32 chan, XorBurstLimit srcBurstLimit)
{
    MV_U32 temp = MV_REG_READ(XOR_CONFIG_REG(XOR_UNIT(chan), XOR_CHAN(chan)));
    temp &= ~XEXCR_SRC_BURST_LIMIT_MASK;
    temp |= srcBurstLimit << XEXCR_SRC_BURST_LIMIT_OFFS;
    MV_REG_WRITE(XOR_CONFIG_REG(XOR_UNIT(chan),XOR_CHAN(chan)), temp);
}

void xorSpi2DramSmartTest(XorBurstLimit srcBurstLimit, MV_U32 copyChunk,
                          MV_U32 copyTotal, MV_U32 *from, MV_U32 *to,
                          MV_U32 toInitVal, MV_BOOL memCmp)
{
    MV_U32 chan, i, physAddr, data ,nTimes;
    MV_XOR_DESC *xorDesc;

    for(chan = 0; chan < MV_XOR_MAX_CHAN; chan++) {
        xorSetSrcBurtLimit(chan, srcBurstLimit);

        xorDesc = mvOsIoUncachedMemalign(NULL, MV_XOR_DESC_ALIGNMENT,
                                         sizeof(MV_XOR_DESC), &physAddr);
        if (!xorDesc) {
            mvOsPrintf("%s: malloc failed.", __func__);
            goto testExit;
        }
        for (i = 0; i < copyTotal / 4; i++)
            to[i] = toInitVal;

        for (nTimes = 0; nTimes < copyTotal; nTimes += copyChunk) {
            if(mvXorStateGet(chan) != MV_IDLE)
                mvOsPrintf("ERR: %s XOR chan %d is not idle", __func__, chan);

            memset(xorDesc, 0, sizeof(MV_XOR_DESC));
            xorDesc->srcAdd0        = (MV_U32) &from[nTimes / 4];
            xorDesc->phyDestAdd     = (MV_U32) &to[nTimes / 4];
            xorDesc->byteCnt        = copyChunk;
            xorDesc->phyNextDescPtr = 0;
            xorDesc->status         = BIT31;
        #ifdef CONFIG_MV88F6281
            mvOsBridgeReorderWA();
        #endif
            if (mvXorTransfer(chan, MV_DMA, physAddr) != MV_OK) {
                mvOsPrintf("%s: DMA copy operation on channel %d failed!\n",
                           __func__, chan);
                goto testOut;
            }
            xor_waiton_eng(chan);

            if ((xorDesc->status & BIT30) == 0) {
                mvOsPrintf("%s: DMA copy status is wrong on channel %d!\n",
                           __func__, chan);
                goto testOut;
            }
            if (memCmp)
                for (i = 0; i < copyChunk / 4; i++) {
                    if (mvSFlashBlockRd(&G_spiFlashInfo, (MV_U32)&from[i], (MV_U8*)&data, 4) != MV_OK) {
                        mvOsPrintf("%s: flash block read failed.\n", __func__);
                        goto testOut;
                    }
                    if (to[i] != data) {
                        mvOsPrintf("%s: Memcmp failed.\n", __func__);
                        goto testOut;
                    }
                }
        }
    }
testOut:
    mvOsIoUncachedAlignedFree(xorDesc);
    mvOsPrintf("XOR: test pass: srcBurstLimit = %3d bytes, copyChunk = 0x%X bytes, "
               "copyTotal = 0x%X bytes\n",
               2 << (srcBurstLimit + 2), /* limit bits to bytes */
               copyChunk, copyTotal);
testExit:
}

void xorSpi2DramSmartSimpleTest(XorBurstLimit srcBurstLimit, MV_U32 copyChunk,
                                MV_U32 copyTotal)
{
    MV_U32 *from = (MV_U32 *)SPI_CS_BASE /* + NV_BOOT_OFFSET */, *to;
    MV_U32  burst_i, dramInitVal = 0xDD993301;
    MV_BOOL memCmpFlag = MV_TRUE;

    to = (MV_U32 *)mvOsIoUncachedMalloc(NULL, _8MB, NULL, NULL);
    if (!to) {
        mvOsPrintf("%s: mem alloc failed.\n", __func__);
        return;
    }

    for (burst_i = 0; burst_i < XOR_BURST_LIMITS_LEN; burst_i++)
        xorSpi2DramSmartTest(srcBurstLimit, copyChunk, copyTotal,
                             from, to, dramInitVal, memCmpFlag);

    mvOsIoUncachedFree(NULL, 0, 0, to, NULL);
}

void xorSpi2DramSrcBurstLimitTest(XorBurstLimit srcBurstLimit)
{
    MV_U32 *from = (MV_U32 *)SPI_CS_BASE /* + NV_BOOT_OFFSET */, *to;
    MV_U32  chunk_i, total_i, dramInitVal = 0xDD993301;
    MV_BOOL memCmpFlag = MV_TRUE;

    to = (MV_U32 *)mvOsIoUncachedMalloc(NULL, _8MB, NULL, NULL);
    if (!to) {
        mvOsPrintf("%s: mem alloc failed.\n", __func__);
        return;
    }

    /*
     * total copy size > chunk copy size, ==> total_i begins by chunk_i
     */
    for (chunk_i = 0; chunk_i < COPY_CHUNK_NUM; chunk_i++) {
        for (total_i = chunk_i; total_i < COPY_TOTAL_NUM; total_i++) {
            xorSpi2DramSmartTest(srcBurstLimit,
                                 copyChunks[chunk_i],
                                 copyTotals[total_i],
                                 from, to, dramInitVal,
                                 memCmpFlag);
        }
    }

    mvOsIoUncachedFree(NULL, 0, 0, to, NULL);
}

void xorSpi2DramSrcBurstLimitTotalTest()
{
    MV_U32 burst_i;
    for (burst_i = 0; burst_i < XOR_BURST_LIMITS_LEN; burst_i++)
        xorSpi2DramSrcBurstLimitTest(xorBurstLimits[burst_i]);
}

void xorSpi2DramTotalTest()
{
    MV_U32 *from = (MV_U32 *)SPI_CS_BASE /* + NV_BOOT_OFFSET */, *to;
    MV_U32  chunk_i, total_i, burst_i, dramInitVal = 0xDD993301;
    MV_BOOL memCmpFlag = MV_TRUE;

    to = (MV_U32 *)mvOsIoUncachedMalloc(NULL, _8MB, NULL, NULL);
    if (!to) {
        mvOsPrintf("%s: mem alloc failed.\n", __func__);
        return;
    }

    /*
     * total copy size > chunk copy size, ==> total_i begins by chunk_i
     */
    for (chunk_i = 0; chunk_i < COPY_CHUNK_NUM; chunk_i++) {
        for (total_i = chunk_i; total_i < COPY_TOTAL_NUM; total_i++) {
            for (burst_i = 0; burst_i < XOR_BURST_LIMITS_LEN; burst_i++) {
                xorSpi2DramSmartTest(xorBurstLimits[burst_i],
                                     copyChunks[chunk_i],
                                     copyTotals[total_i],
                                     from, to, dramInitVal,
                                     memCmpFlag);
            }
        }
    }

    mvOsIoUncachedFree(NULL, 0, 0, to, NULL);
}

void xorSpi2DramLimitedTest(MV_U32 maxCopyTotal)
{
    MV_U32 *from = (MV_U32 *)SPI_CS_BASE /* + NV_BOOT_OFFSET */, *to;
    MV_U32  chunk_i, total_i, burst_i, dramInitVal = 0xDD993301;
    MV_BOOL memCmpFlag = MV_TRUE;

    to = (MV_U32 *)mvOsIoUncachedMalloc(NULL, _8MB, NULL, NULL);
    if (!to) {
        mvOsPrintf("%s: mem alloc failed.\n", __func__);
        return;
    }

    /*
     * total copy size > chunk copy size, ==> total_i begins by chunk_i
     */
    for (chunk_i = 0; chunk_i < COPY_CHUNK_NUM; chunk_i++) {
        for (total_i = chunk_i; total_i < COPY_TOTAL_NUM; total_i++) {
            if (copyTotals[total_i] > maxCopyTotal)
                continue;
            for (burst_i = 0; burst_i < XOR_BURST_LIMITS_LEN; burst_i++) {
                xorSpi2DramSmartTest(xorBurstLimits[burst_i],
                                     copyChunks[chunk_i],
                                     copyTotals[total_i],
                                     from, to, dramInitVal,
                                     memCmpFlag);
            }
        }
    }

    mvOsIoUncachedFree(NULL, 0, 0, to, NULL);
}

void xorSpi2DramEternalTest()
{
    MV_U32 *from = (MV_U32 *)(SPI_CS_BASE + NV_BOOT_OFFSET), *to;
    MV_U32 copyTotal = _1MB;
    MV_U32 copyChunk = _1MB;
    MV_U32 dramInitVal = 0xDD993301;
    MV_BOOL memCmpFlag = MV_TRUE;

    to = (MV_U32 *)mvOsIoUncachedMalloc(NULL, copyTotal, NULL, NULL);
    if (!to) {
        mvOsPrintf("%s: mem alloc failed.\n", __func__);
        return;
    }

    while (1) {
        mvOsSleep(100);
        xorSpi2DramSmartTest(XorBurstLimit_32bytes, copyChunk, copyTotal,
                             from, to, dramInitVal, memCmpFlag);
    }

    while (1) {
        mvOsSleep(100);
        xorSpi2DramSmartTest(XorBurstLimit_64bytes, copyChunk, copyTotal,
                             from, to, dramInitVal, memCmpFlag);
    }

    while (1) {
        mvOsSleep(100);
        xorSpi2DramSmartTest(XorBurstLimit_128bytes, copyChunk, copyTotal,
                             from, to, dramInitVal, memCmpFlag);
    }

    mvOsIoUncachedFree(NULL, 0, 0, to, NULL);
}

void xorSpi2DramEternalSrcBurstLimitTest(XorBurstLimit srcBurstLimit)
{
    MV_U32 *from = (MV_U32 *)(SPI_CS_BASE + NV_BOOT_OFFSET), *to;
    MV_U32 copyTotal = _1MB;
    MV_U32 copyChunk = _1MB;
    MV_U32 dramInitVal = 0xDD993301;
    MV_BOOL memCmpFlag = MV_TRUE;

    to = (MV_U32 *)mvOsIoUncachedMalloc(NULL, copyTotal, NULL, NULL);
    if (!to) {
        mvOsPrintf("%s: mem alloc failed.\n", __func__);
        return;
    }

    while (1) {
        mvOsSleep(100);
        xorSpi2DramSmartTest(srcBurstLimit, copyChunk, copyTotal,
                             from, to, dramInitVal, memCmpFlag);
    }

    mvOsIoUncachedFree(NULL, 0, 0, to, NULL);
}

#endif /* MV_INCLUDE_SPI */

#ifdef DMV_INCLUDE_CESA
void xorSramDramTest()
{
        MV_U32 chan;
        MV_U32 i;
        MV_ULONG addr;
        MV_XOR_DESC *xorDesc;
        MV_U32 *to, *from;
        MV_U32 sizeReg, baseReg;


        /* set CESA windows */
        baseReg = SDRAM_CS0_BASE;
    sizeReg = MV_TRUE |
                (((SDRAM_CS0 << MV_CESA_TDMA_WIN_TARGET_OFFSET) & MV_CESA_TDMA_WIN_TARGET_MASK) |
               ((0xe   << MV_CESA_TDMA_WIN_ATTR_OFFSET)   & MV_CESA_TDMA_WIN_ATTR_MASK)   |
               ((SDRAM_CS0_SIZE << MV_CESA_TDMA_WIN_SIZE_OFFSET) & MV_CESA_TDMA_WIN_SIZE_MASK));

    MV_REG_WRITE( MV_CESA_TDMA_WIN_CTRL_REG(0), sizeReg);
    MV_REG_WRITE( MV_CESA_TDMA_BASE_ADDR_REG(0), baseReg);


        baseReg = SDRAM_CS1_BASE;
        sizeReg = MV_TRUE |
        (((SDRAM_CS1 << MV_CESA_TDMA_WIN_TARGET_OFFSET) & MV_CESA_TDMA_WIN_TARGET_MASK) |
                   ((0xd   << MV_CESA_TDMA_WIN_ATTR_OFFSET)   & MV_CESA_TDMA_WIN_ATTR_MASK)   |
                   ((SDRAM_CS1_SIZE << MV_CESA_TDMA_WIN_SIZE_OFFSET) & MV_CESA_TDMA_WIN_SIZE_MASK));

    MV_REG_WRITE( MV_CESA_TDMA_WIN_CTRL_REG(1), sizeReg);
    MV_REG_WRITE( MV_CESA_TDMA_BASE_ADDR_REG(1), baseReg);



        for(chan = 0; chan < MV_XOR_MAX_CHAN; chan++)
    {
                xorDesc = mvOsIoUncachedMemalign(NULL, MV_XOR_DESC_ALIGNMENT,
                                         sizeof(MV_XOR_DESC), &addr);

                if (xorDesc == NULL)
                {
                        mvOsPrintf("\nEtrror malloc fail\n");
                }

                if(mvXorStateGet(chan) != MV_IDLE) {
                        mvOsPrintf("ERR: %s XOR chan %d is not idle",__FUNCTION__, chan);
                }

                to   = (MV_U32 *)mvOsIoUncachedMalloc(NULL,XOR_ARRAY_NBYTES,NULL,NULL);
                from = (MV_U32 *)mvOsIoUncachedMalloc(NULL,XOR_ARRAY_NBYTES,NULL,NULL);

                if ((from == NULL) || (to == NULL))
                {
                        mvOsPrintf("XOR Fail allocate memory \n");
                        return;
                }

        for (i=0; i < (XOR_ARRAY_NBYTES / 4); i++)
                {
                        to[i] = i;
                        from[i] = 0xFFFFFFFF;
                }

                memset(xorDesc,0,sizeof(MV_XOR_DESC));
                xorDesc->srcAdd0 = (MV_U32)from;
                xorDesc->phyDestAdd = CRYPT_ENG_BASE;
                xorDesc->byteCnt = XOR_ARRAY_NBYTES;
                xorDesc->phyNextDescPtr = 0;
                xorDesc->status = BIT31;
        #ifdef CONFIG_MV88F6281
                mvOsBridgeReorderWA();
        #endif


                if( mvXorTransfer(chan, MV_DMA, addr) != MV_OK)
                {
                        mvOsPrintf("%s: DMA copy operation on channel %d failed!\n", __func__, chan);
                        mvOsIoUncachedFree(NULL,0,0,to);
                        mvOsIoUncachedFree(NULL,0,0,from);
                        mvOsIoUncachedAlignedFree(xorDesc);
                        return;
                }


            xor_waiton_eng(chan);

                if (!(xorDesc->status & BIT30)) {
                        mvOsPrintf("DMA copy operation status is wrong on channel %d!\n", chan);
                        mvOsIoUncachedFree(NULL,0,0,to);
                        mvOsIoUncachedFree(NULL,0,0,from);
                        mvOsIoUncachedAlignedFree(xorDesc);
                        return;

                }


                for (i=0; i < (XOR_ARRAY_NBYTES / 4); i++)
                {
                        if (MV_MEMIO32_READ(CRYPT_ENG_BASE + (i * 4)) != i)
                        {
                                mvOsPrintf("XOR Fail compare memory channel %d offset 0x%x value 0x%x\n",
                                                   chan,i, MV_MEMIO32_READ(CRYPT_ENG_BASE + (i * 4)));
                        }
                }

                mvOsIoUncachedFree(NULL,0,0,to);
                mvOsIoUncachedFree(NULL,0,0,from);
                mvOsIoUncachedAlignedFree(xorDesc);

    }

        mvOsPrintf("XOR: test pass\n");
}
#endif /*DMV_INCLUDE_CESA*/

void xorPex2DramTest(int numOfBytes)
{
    MV_U32 chan, i, physAddr;
    MV_XOR_DESC *xorDesc;
    MV_U32 *to, *from;

    for (chan = 0; chan < MV_XOR_MAX_CHAN; chan++)
    {
        xorDesc = mvOsIoUncachedMemalign(NULL, MV_XOR_DESC_ALIGNMENT,
                                         sizeof(MV_XOR_DESC), &physAddr);
        if (!xorDesc)
        {
            mvOsPrintf("\n%s: mvOsIoUncachedMemalign failed.\n", __func__);
            return;
        }

        if (mvXorStateGet(chan) != MV_IDLE)
            mvOsPrintf("%s: XOR chan %d is not idle", __func__, chan);

        to   = (MV_U32 *)mvOsIoUncachedMalloc(NULL, XOR_ARRAY_NBYTES, NULL, NULL);
        from = (MV_U32 *)mvOsIoUncachedMalloc(NULL, XOR_ARRAY_NBYTES, NULL, NULL);

        if (!from || !to)
        {
            mvOsPrintf("%s: mem alloc failed.\n", __func__);
            return;
        }

        for (i = 0; i < XOR_ARRAY_NBYTES / 4; i++)
        {
            from[i] = i + 1;
            to  [i] = 0xFFFFFFFF;
        }

        memset(xorDesc,0,sizeof(MV_XOR_DESC));
        xorDesc->srcAdd0        = PEX0_MEM_BASE;
        xorDesc->phyDestAdd     = (MV_U32)from;
        xorDesc->byteCnt        = numOfBytes;
        xorDesc->phyNextDescPtr = 0;
        xorDesc->status         = BIT31;
    #ifdef CONFIG_MV88F6281
        mvOsBridgeReorderWA();
    #endif
        if (mvXorTransfer(chan, MV_DMA, physAddr) != MV_OK)
        {
            mvOsPrintf("%s: DMA copy operation on channel %d failed!\n",
                       __func__, chan);
            mvOsIoUncachedFree(NULL, 0, 0, to,   NULL);
            mvOsIoUncachedFree(NULL, 0, 0, from, NULL);
            mvOsIoUncachedAlignedFree(xorDesc);
            return;
        }
        xor_waiton_eng(chan);
        if (!(xorDesc->status & BIT30)) {
            mvOsPrintf("%s: DMA copy operation status is wrong on channel %d!\n",
                       __func__, chan);
            mvOsIoUncachedFree(NULL, 0, 0, to,   NULL);
            mvOsIoUncachedFree(NULL, 0, 0, from, NULL);
            mvOsIoUncachedAlignedFree(xorDesc);
            return;
        }

        for (i = 0; i < XOR_ARRAY_NBYTES / 4; i++)
        {
            if (MV_MEMIO32_READ(PEX0_MEM_BASE + (i * 4)) != i)
            {
                mvOsPrintf("XOR Fail compare memory channel %d offset 0x%x value 0x%x\n",
                           chan, i, MV_MEMIO32_READ(PEX0_MEM_BASE + (i * 4)));
                return;
            }
        }

        mvOsIoUncachedFree(NULL, 0, 0, to,   NULL);
        mvOsIoUncachedFree(NULL, 0, 0, from, NULL);
        mvOsIoUncachedAlignedFree(xorDesc);
    }

    mvOsPrintf("XOR: test pass\n");
}

void xorTest()
{
#ifdef MV_INCLUDE_SPI
    xorSpi2DramTest();
#endif
    xorDram2DramTest();
    xorPex2DramTest(XOR_ARRAY_NBYTES);
}

void smiTest()
{
    MV_U16 reg;

    mvEthPhyRegRead(mvBoardPhyAddrGet(0), 2, &reg);
    if (reg != 0x141)
        mvOsPrintf("\nError marvell ID read 0x%x instead 0x141\n", reg);

    mvEthPhyRegRead(mvBoardPhyAddrGet(0), 3, &reg);

    reg  = MV_16BIT_LE(reg);
    reg &= MV_16BIT_LE(0xFFF0); /* disregard revision number */
    if (reg != MV_16BIT_LE(0x0CC0))
        mvOsPrintf("%s: Error: vendor ID read 0x%04X instead 0x0CC0.\n",
                   __func__, reg);

    mvOsPrintf("SMI: test pass\n");
}

#ifdef INCLUDE_AUXCLK
int    sysAuxClkRateGet(void);
STATUS sysAuxClkConnect(FUNCPTR isrRouting, int isrArg);
STATUS sysAuxClkRateSet(int ticksPerSecond);
void   sysAuxClkEnable(void);
void   sysAuxClkDisable(void);

static int sysAuxClkCount;

/*
 * prints dot twice a second
 */
void usrAuxClkTest()
{
    int rate = sysAuxClkRateGet() / 2; /* twice a second */

    if (sysAuxClkCount % rate == 0)
    {
        mvDebugPrint("."); /* prints right to UART */
    }
    sysAuxClkCount++;
}

void sysAuxClkTest()
{
    sysAuxClkCount = 0;

    mvOsPrintf("Auxiliary Clock: test started.\n");

    sysAuxClkConnect((FUNCPTR)usrAuxClkTest, 0);
    sysAuxClkRateSet(SYS_CLK_RATE);
    sysAuxClkEnable();

    mvOsDelay(10000);
    sysAuxClkDisable();

    mvOsPrintf("\nAuxiliary Clock: test pass.\n");
}
#endif /* INCLUDE_AUXCLK */

#if defined(MV_SYS_TIMER_THROUGH_GPIO) && defined(INCLUDE_AUXCLK)
MV_VOID mvbridgeIntFiqCntsPrint(MV_VOID);
MV_VOID mvbridgeIntFiqCntsInit (MV_VOID);
MV_VOID mvSwitchTimerToFiqIsr  (MV_VOID);
MV_VOID mvSwitchTimerToIrqIsr  (MV_VOID);

MV_BOOL mvFiqTimersTest()
{
    mvOsPrintf("FIQ Timers: test started.\n");

    /*
     * IRQ timer test
     */
    mvOsPrintf("Both SoC timers use IRQ interrupt (default mode).\n");
    mvbridgeIntFiqCntsPrint();

    mvOsPrintf("Running Auxiliary Clock test.\n");
    mvbridgeIntFiqCntsInit();
    sysAuxClkTest();
    mvbridgeIntFiqCntsPrint();

    mvbridgeIntFiqCntsInit();
    mvOsPrintf("One second delay.\n");
    mvOsDelay(1000);
    mvbridgeIntFiqCntsPrint();

    mvbridgeIntFiqCntsInit();
    mvOsPrintf("Three seconds delay.\n");
    mvOsDelay(3000);
    mvbridgeIntFiqCntsPrint();

    /*
     * FIQ timer test
     */
    mvOsPrintf("Switching both timer to FIQ!\n");
    mvSwitchTimerToFiqIsr();

    mvbridgeIntFiqCntsInit();
    mvOsPrintf("One second delay.\n");
    mvOsDelay(1000);
    mvbridgeIntFiqCntsPrint();

    mvbridgeIntFiqCntsInit();
    mvOsPrintf("Three seconds delay.\n");
    mvOsDelay(3000);
    mvbridgeIntFiqCntsPrint();

    mvOsPrintf("Running Auxiliary Clock test.\n");
    mvbridgeIntFiqCntsInit();
    sysAuxClkTest();
    mvbridgeIntFiqCntsPrint();

    /*
     * IRQ timer test - back to IRQ (default mode).
     */
    mvSwitchTimerToIrqIsr();

    mvOsPrintf("Both SoC timers use IRQ interrupt - again.\n");
    mvbridgeIntFiqCntsPrint();

    mvbridgeIntFiqCntsInit();
    mvOsPrintf("One second delay.\n");
    mvOsDelay(1000);
    mvbridgeIntFiqCntsPrint();

    mvbridgeIntFiqCntsInit();
    mvOsPrintf("Three seconds delay.\n");
    mvOsDelay(3000);
    mvbridgeIntFiqCntsPrint();

    mvOsPrintf("Running Auxiliary Clock test.\n");
    mvbridgeIntFiqCntsInit();
    sysAuxClkTest();
    mvbridgeIntFiqCntsPrint();

    mvOsPrintf("\nFIQ Timers: test pass.\n");
    return MV_OK;
}
#endif /* MV_SYS_TIMER_THROUGH_GPIO && INCLUDE_AUXCLK*/

void basicTests()
{
    smiTest();
#ifdef MV_INCLUDE_SPI
    xorSpi2DramTest();
    spiTest();
#endif
#ifdef MV_INCLUDE_NAND
    nandTest();
#endif
#ifdef INCLUDE_AUXCLK
    sysAuxClkTest();
#endif
    xorDram2DramTest();
    mmuTest();
    twsiTest();
}

/*******************************************************************************
* mvAdvancedTests
*
* DESCRIPTION:
*       None.
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
void mvAdvancedTests()
{
    basicTests();
#if defined(MV_SYS_TIMER_THROUGH_GPIO) && defined(INCLUDE_AUXCLK)
    mvFiqTimersTest();
#endif

#ifdef MV_INCLUDE_SPI
    xorSpi2DramLimitedTest(65536);
#endif
}

void read_CP15_regR1()
{
    MV_U32 regVal = 0;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (regVal));
    mvOsPrintf("%s: regVal = 0x%08X.\n", __func__, regVal);
}

void read_CP15_extraFeatures()
{
    MV_U32 regVal = 0;
    __asm volatile ("mrc p15, 1, %0, c15, c1, 0" : "=r" (regVal));
    mvOsPrintf("%s: regVal = 0x%08X.\n", __func__, regVal);
}

void enableAlignmentFaultChecking_CP15_regR1()
{
    MV_U32 regVal = 0;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (regVal));
    mvOsPrintf("%s: regVal = 0x%08X.\n", __func__, regVal);

    regVal |= 0x2;
    __asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r" (regVal));

    __asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (regVal));
    mvOsPrintf("%s: regVal = 0x%08X.\n", __func__, regVal);
}

void enableAlignmentFaultChecking_CP15_regR1_silent()
{
    MV_U32 regVal = 0;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (regVal));

    regVal |= 0x2;
    __asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r" (regVal));

    __asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (regVal));
}

void disableAlignmentFaultChecking_CP15_regR1()
{
    MV_U32 regVal = 0;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (regVal));
    mvOsPrintf("%s: regVal = 0x%08X.\n", __func__, regVal);

    regVal &= ~0x2;
    __asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r" (regVal));

    __asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (regVal));
    mvOsPrintf("%s: regVal = 0x%08X.\n", __func__, regVal);
}

/*
 * reads cache details from Cache Type Register: R0 (CP15.0.R0.C0.1)
 */
void getCacheDetails_Control_Reg_R1()
{
    MV_U32 bits, regVal = 0;
    __asm volatile ("mrc p15, 0, %0, c0, c0, 1" : "=r" (regVal));
    mvOsPrintf("mcr returned 0x%08X;\n", regVal);

    /*
     * I/D-Cache line length
     */
    if (regVal & BIT_1)
        mvOsPrintf("ICache Line Length = 8 words (32 bytes) per line.\n");
    else
        mvOsPrintf("ICache Line Length = UNKNOWN.\n");

    if (regVal & BIT_13)
        mvOsPrintf("DCache Line Length = 8 words (32 bytes) per line.\n");
    else
        mvOsPrintf("DCache Line Length = UNKNOWN.\n");

    /*
     * ICache associativity
     */
    bits = (regVal & (BIT_3 | BIT_4 | BIT_5)) >> 3;
    if (bits > 2)
        mvOsPrintf("ICache Set Associativity is UNKNOWN.\n");
    else if (bits == 0)
        mvOsPrintf("ICache Set Associativity = Direct mapped.\n");
    else
        mvOsPrintf("ICache Set Associativity = %d-way.\n", 2 << (bits - 1));

    /*
     * DCache associativity
     */
    bits = (regVal & (BIT_15 | BIT_16 | BIT_17)) >> 15;
    if (bits > 2)
        mvOsPrintf("DCache Set Associativity is UNKNOWN.\n");
    else if (bits == 0)
        mvOsPrintf("DCache Set Associativity = Direct mapped.\n");
    else
        mvOsPrintf("DCache Set Associativity = %d-way.\n", 2 << (bits - 1));

    /*
     * I/D-Cache size
     */
    bits = (regVal & (BIT_6 | BIT_7 | BIT_8 | BIT_9)) >> 6;
    mvOsPrintf("ICache Size = %d KB\n", 2 << (bits - 1));
    bits = (regVal & (BIT_18 | BIT_19 | BIT_20 | BIT_21)) >> 18;
    mvOsPrintf("DCache Size = %d KB\n", 2 << (bits - 1));

    /*
     * Unified or separated cache
     */
    if (regVal & BIT_24)
        mvOsPrintf("Cache Split = The ICache and DCache are separate.\n");
    else
        mvOsPrintf("Cache Split = Cache is unified.\n");
}

MV_U32 G_unaligned_addr_read_4_bytes_with_1_byte_align  = 0x701;
MV_U32 G_unaligned_addr_read_4_bytes_with_2_byte_align  = 0x702;
MV_U32 G_unaligned_addr_read_4_bytes_with_3_byte_align  = 0x703;
MV_U32 G_unaligned_addr_write_4_bytes_with_1_byte_align = 0x701;
MV_U32 G_unaligned_addr_write_4_bytes_with_2_byte_align = 0x702;
MV_U32 G_unaligned_addr_write_4_bytes_with_3_byte_align = 0x703;

MV_U32 alignment_read_4_bytes_with_1_byte_align_test()
{
    MV_U32 *ptr = (MV_U32 *)G_unaligned_addr_read_4_bytes_with_1_byte_align;
    MV_U32  val;

    val = *ptr;

    mvOsPrintf("alignment_read_4_bytes_with_1_byte_align_test: (*0x%08X) is 0x%08X.\n", ptr, val);
    return val;
}

MV_U32 alignment_read_4_bytes_with_2_byte_align_test()
{
    MV_U32 *ptr = (MV_U32 *)G_unaligned_addr_read_4_bytes_with_2_byte_align;
    MV_U32  val;

    val = *ptr;

    mvOsPrintf("alignment_read_4_bytes_with_1_byte_align_test: (*0x%08X) is 0x%08X.\n", ptr, val);
    return val;
}

MV_U32 alignment_read_4_bytes_with_3_byte_align_test()
{
    MV_U32 *ptr = (MV_U32 *)G_unaligned_addr_read_4_bytes_with_3_byte_align;
    MV_U32  val;

    val = *ptr;

    mvOsPrintf("alignment_read_4_bytes_with_3_byte_align_test: (*0x%08X) is 0x%08X.\n", ptr, val);
    return val;
}

MV_U32 alignment_write_4_bytes_with_1_byte_align_test()
{
    MV_U32 *ptr = (MV_U32 *)G_unaligned_addr_write_4_bytes_with_1_byte_align;
    MV_U32  val = 0xdeadc0de;

    *ptr = 0xdeadc0de;

    mvOsPrintf("alignment_write_4_bytes_with_1_byte_align_test: (*0x%08X) is 0x%08X.\n", ptr, val);
    return val;
}

MV_U32 alignment_write_4_bytes_with_2_byte_align_test()
{
    MV_U32 *ptr = (MV_U32 *)G_unaligned_addr_write_4_bytes_with_2_byte_align;
    MV_U32  val = 0xdeadc0de;

    *ptr = 0xdeadc0de;

    mvOsPrintf("alignment_write_4_bytes_with_1_byte_align_test: (*0x%08X) is 0x%08X.\n", ptr, val);
    return val;
}

MV_U32 alignment_write_4_bytes_with_3_byte_align_test()
{
    MV_U32 *ptr = (MV_U32 *)G_unaligned_addr_write_4_bytes_with_3_byte_align;
    MV_U32  val = 0xdeadc0de;

    *ptr = 0xdeadc0de;

    mvOsPrintf("alignment_write_4_bytes_with_3_byte_align_test: (*0x%08X) is 0x%08X.\n", ptr, val);
    return val;
}

MV_U32 unaligned_test(int dummy0, int dummy1, int dummy2, int dummy3)
{
    dummy0 = 0x01020304;
    dummy1 = 0x05060708;
    dummy2 = 0x090a0b0c;
    dummy3 = 0x11223344;

    /**************************************************************************/
    /* Load and Store Word Immediate offset, pre-index and post-index         */
    /**************************************************************************/

    /*
     * Load Word Immediate offset
     */

    /* 1-byte alignment */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("sub     r0, r0, #1");
    __asm volatile("ldr     r3, [r0]");

    /* 2-byte alignment */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("sub     r0, r0, #2");
    __asm volatile("ldr     r3, [r0]");

    /* 3-byte alignment */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("sub     r0, r0, #3");
    __asm volatile("ldr     r3, [r0]");

    /* 4-byte alignment (no exception) */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("sub     r0, r0, #4");
    __asm volatile("ldr     r3, [r0]");

    /* 1-byte alignment */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldr     r3, [r0, #-1]");

    /* 2-byte alignment */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldr     r3, [r0, #-2]");

    /* 3-byte alignment */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldr     r3, [r0, #-3]");

    /* 4-byte alignment (no exception) */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldr     r3, [r0, #-4]");

    /*
     * Load Word Immediate pre-index
     */

    /* 1-byte alignment */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldr     r3, [r0, #-1]!");

    /*
     * Store Word Immediate pre-index
     */

    /* 2-byte alignment */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("str     r3, [r0, #-2]!");

    /*
     * Load Word Immediate post-index
     */

    /* 1-byte alignment */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldr     r3, [r0], #-1");

    /*
     * Store Word Immediate post-index
     */

    /* 2-byte alignment */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("str     r3, [r0], #-2");

    /**************************************************************************/
    /* Load and Store Word Register offset, pre-index and post-index          */
    /**************************************************************************/

    /*
     * Load Word Register offset
     */

    /* 1-byte alignment */
    __asm volatile("mov     r1, #1"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldr     r3, [r0, -r1]");

    /*
     * Store Word Register offset
     */

    /* 3-byte alignment */
    __asm volatile("mov     r1, #3"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("str     r3, [r0, +r1]");

    /*
     * Load Word Register pre-index
     */

    /* 1-byte alignment */
    __asm volatile("mov     r1, #1"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldr     r3, [r0, -r1]!");

    /*
     * Store Word Register pre-index
     */

    /* 3-byte alignment */
    __asm volatile("mov     r1, #3"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("str     r3, [r0, +r1]!");

    /*
     * Load Word Register post-index
     */

    /* 1-byte alignment */
    __asm volatile("mov     r1, #1"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldr     r3, [r0], +r1");

    /*
     * Store Word Register post-index
     */

    /* 3-byte alignment */
    __asm volatile("mov     r1, #3"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("str     r3, [r0], -r1");

    /**************************************************************************/
    /* Load and Store Word Scaled register offset, pre-index and post-index   */
    /**************************************************************************/

    /*
     * Load Word Scaled register offset
     */

    /* 2-byte alignment */
    __asm volatile("mov     r1, #1"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldr     r3, [r0, -r1, lsl #1]");

    /* 2-byte alignment */
    __asm volatile("mov     r1, #1"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldr     r3, [r0, +r1, lsl #1]");

    /* 1-byte alignment */
    __asm volatile("mov     r1, #3"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldr     r3, [r0, -r1, lsr #1]");

    /* 3-byte alignment */
    __asm volatile("mov     r1, #6"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldr     r3, [r0, +r1, lsr #1]");

    /* 1-byte alignment */
    __asm volatile("mov     r1, #3"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldr     r3, [r0, -r1, asr #1]");

    /* 3-byte alignment */
    __asm volatile("mov     r1, #-6"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldr     r3, [r0, +r1, asr #1]");

    /*
     * Store Word Scaled register offset
     */

    /* 3-byte alignment */
    __asm volatile("mov     r1, #6"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("str     r3, [r0, -r1, lsr #1]");

    /* 3-byte alignment */
    __asm volatile("mov     r1, #6"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("str     r3, [r0, -r1, lsr #1]");

    /* 1-byte alignment */
    __asm volatile("mov     r1, #3"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("str     r3, [r0, +r1, lsr #1]");

    /* 3-byte alignment */
    __asm volatile("mov     r1, #6"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("str     r3, [r0, +r1, lsr #1]");

    /* 1-byte alignment */
    __asm volatile("mov     r1, #3"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("str     r3, [r0, -r1, asr #1]");

    /* 3-byte alignment */
    __asm volatile("mov     r1, #-6"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("str     r3, [r0, +r1, asr #1]");

    /*
     * Load and Store Word Scaled register pre-index
     */

    /* 1-byte alignment */
    __asm volatile("mov     r1, #2"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldr     r3, [r0, -r1, ror #1]!");

    /*
     * Load and Store Word Scaled register post-index
     */

    /* 3-byte alignment */
    __asm volatile("mov     r1, #7"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldr     r3, [r0], -r1, rrx");

    /* 2-byte alignment */
    __asm volatile("mov     r1, #5"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("str     r3, [r0], +r1, ror #1");

    /**************************************************************************/
    /* Miscellaneous Load and Store Word immediate, pre-index and post-index  */
    /**************************************************************************/

    /*
     * Miscellaneous Loads and Stores Immediate offset
     */

    /* 1-byte alignment LDRH */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("sub     r0, r0, #1");
    __asm volatile("ldrh    r3, [r0]");

    /* 1-byte alignment LDRSH */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("sub     r0, r0, #1");
    __asm volatile("ldrsh   r3, [r0]");

    /* 1-byte alignment STRH (STRSH does not exist) */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("sub     r0, r0, #1");
    __asm volatile("strh    r3, [r0]");

    /*
     * Miscellaneous Loads and Stores Register offset
     */

    /* 1-byte alignment LDRH */
    __asm volatile("mov     r1, #1"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldrh    r3, [r0, -r1]");

    /* 1-byte alignment LDRSH */
    __asm volatile("mov     r1, #1"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldrsh   r3, [r0, -r1]");

    /* 1-byte alignment STRH (STRSH does not exist) */
    __asm volatile("mov     r1, #1"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("strh    r3, [r0, +r1]");

    /*
     * Miscellaneous Loads and Stores Immediate pre-indexed
     */

    /* 1-byte alignment LDRH */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldrh    r3, [r0, #-2]!");

    /* 1-byte alignment LDRSH */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldrsh   r3, [r0, #-1]!");

    /* 1-byte alignment STRH (STRSH does not exist) */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("strh    r3, [r0, #-1]!");

    /*
     * Miscellaneous Loads and Stores Register pre-indexed
     */

    /* 1-byte alignment LDRH */
    __asm volatile("mov     r1, #1"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldrh    r3, [r0, -r1]!");

    /* 1-byte alignment LDRSH */
    __asm volatile("mov     r1, #1"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldrsh   r3, [r0, -r1]!");

    /* 1-byte alignment STRH (STRSH does not exist) */
    __asm volatile("mov     r1, #1"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("strh    r3, [r0, +r1]!");

    /*
     * Miscellaneous Loads and Stores Immediate post-indexed
     */

    /* 1-byte alignment LDRH */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldrh    r3, [r0], #-1");

    /* 1-byte alignment LDRSH */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("ldrsh   r3, [r0], #-1");

    /* 1-byte alignment STRH (STRSH does not exist) */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("strh    r3, [r0], #-1");

    /*
     * Miscellaneous Loads and Stores Register post-indexed
     */

    /* 1-byte alignment LDRH */
    __asm volatile("mov     r1, #1"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("sub     r0, r0, #1");
    __asm volatile("ldrh    r3, [r0], -r1");

    /* 1-byte alignment LDRSH */
    __asm volatile("mov     r1, #1"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("sub     r0, r0, #1");
    __asm volatile("ldrsh   r3, [r0], -r1");

    /* 1-byte alignment STRH (STRSH does not exist) */
    __asm volatile("mov     r1, #1"); /* prepare Rm reg */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("sub     r0, r0, #1");
    __asm volatile("strh    r3, [r0], +r1");

    /**************************************************************************/
    /* Load and Store Multiple - Increment/Decrement Before/After             */
    /**************************************************************************/

    /*
     * Load and Store Multiple Increment after
     */

    /* 1-byte alignment LDMIA */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("sub     r0, r0, #1");
    __asm volatile("ldmia   r0, {r2, r3}");

    /* 2-byte alignment LDMIA */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("sub     r0, r0, #2");
    __asm volatile("ldmia   r0!, {r2, r3, r4}");

    /* 3-byte alignment LDMIB */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("sub     r0, r0, #3");
    __asm volatile("ldmib   r0, {r2, r3, r4}");

    /* 2-byte alignment LDMIB */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("sub     r0, r0, #2");
    __asm volatile("ldmib   r0!, {r2, r3, r4}");

    /* 2-byte alignment LDMDA */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("sub     r0, r0, #2");
    __asm volatile("ldmda   r0, {r2, r3, r4}");

    /* 1-byte alignment LDMDA */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("sub     r0, r0, #1");
    __asm volatile("ldmda   r0!, {r2, r3}");

    /* 1-byte alignment LDMDB */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("sub     r0, r0, #1");
    __asm volatile("ldmdb   r0!, {r2, r3}");

    /* 2-byte alignment LDMDB */
    __asm volatile("mov     r0, r11");
    __asm volatile("sub     r0, r0, #20");
    __asm volatile("sub     r0, r0, #2");
    __asm volatile("ldmdb   r0!, {r2, r3}");

    return MV_OK;
}

void mv_sheeva_event_select_read_cp15_0_r9_c13_1(void)
{
    MV_U32 regVal = 0;

    __asm ("mrc    p15, 0, %0, c9, c13, 1" : "=r" (regVal));
    mvOsPrintf("Event Select (before) 0x%08X.\n", regVal);
}

void mv_sheeva_event_select_set_cp15_0_r9_c13_1(MV_U32 newVal)
{
    MV_U32 regVal;

    __asm ("mrc    p15, 0, %0, c9, c13, 1" : "=r" (regVal));
    mvOsPrintf("Event Select (before) 0x%08X.\n", regVal);

    mvOsPrintf("Writing value 0x%08X.\n", newVal);
    __asm ("mcr    p15, 0, %0, c9, c13, 1" : "=r" (newVal));

    __asm ("mrc    p15, 0, %0, c9, c13, 1" : "=r" (regVal));
    mvOsPrintf("Event Select (after)  0x%08X.\n", regVal);
}

/*******************************************************************************
 * L2 Performance Counter0 Control Register
 */
__inline MV_U32 L2_PERF_READ_COUNTER_CNTRL0(void)
{
    MV_U32 regVal = 0;
    __asm__ __volatile__("mrc p15,6,%0,c15,c12,0" : "=r" (regVal));
    return regVal;
}

__inline void L2_PERF_WRITE_COUNTER_CNTRL0(MV_U32 newVal)
{
    __asm__ __volatile__("mcr p15,6,%0,c15,c12,0" :: "r" (newVal));
}

/*******************************************************************************
 * L2 Performance Counter0 Low Register
 */
__inline MV_U32 L2_PERF_READ_COUNTER0_LOW(void)
{
    MV_U32 regVal = 0;
    __asm__ __volatile__("mrc p15,6,%0,c15,c13,0" : "=r" (regVal));
    return regVal;
}

__inline void L2_PERF_WRITE_COUNTER0_LOW(MV_U32 newVal)
{
    __asm__ __volatile__("mcr p15,6,%0,c15,c13,0" :: "r" (newVal));
}

/*******************************************************************************
 * L2 Performance Counter0 High Register
 */
__inline MV_U32 L2_PERF_READ_COUNTER0_HIGH(void)
{
    MV_U32 regVal = 0;
    __asm__ __volatile__("mrc p15,6,%0,c15,c13,1" : "=r" (regVal));
    return regVal;
}

__inline void L2_PERF_WRITE_COUNTER0_HIGH(MV_U32 newVal)
{
    __asm__ __volatile__("mcr p15,6,%0,c15,c13,1" :: "r" (newVal));
}

/*******************************************************************************
 * L2 Performance Counters Test
 */
void mv_uniL2Cache_perf_cnt0_reg_set(MV_U32 newVal)
{
    MV_U32 regVal;

    regVal = L2_PERF_READ_COUNTER_CNTRL0();
    mvOsPrintf("Perfomance Counter0 Control Reg (before) = 0x%08X.\n", regVal);

    mvOsPrintf("Writing value 0x%08X.\n", newVal);
    L2_PERF_WRITE_COUNTER_CNTRL0(newVal);

    regVal = L2_PERF_READ_COUNTER_CNTRL0();
    mvOsPrintf("Perfomance Counter0 Reg (after)  = 0x%08X.\n", regVal);
}

void mv_uniL2Cache_perf_test(MV_U32 newCnt0CtrlVal)
{
    L2_PERF_WRITE_COUNTER0_LOW(0);
    L2_PERF_WRITE_COUNTER0_HIGH(0);

    mv_uniL2Cache_perf_cnt0_reg_set(newCnt0CtrlVal);
}

void unaligned_test_caller()
{
    unaligned_test(0x10, 0x20, 0x30, 0x40);
}

MV_32 eternal_print_loop()
{
    MV_U32 i;

    for (i = 0; ; i++)
    {
        mvOsPrintf("iteration # = %d;\n", i);
    }

    return 0;
}
#endif /* _DIAB_TOOL */

