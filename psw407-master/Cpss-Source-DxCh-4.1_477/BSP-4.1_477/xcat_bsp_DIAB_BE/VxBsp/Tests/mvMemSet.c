/*******************************************************************************
*                Copyright 2005, MARVELL SEMICONDUCTOR, LTD.                   *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.                      *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
*                                                                              *
* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, *
* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL    *
* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.  *
* (MJKK), MARVELL ISRAEL LTD. (MSIL).                                          *
*******************************************************************************/
/*******************************************************************************
* mvMemset.c - Marvell implementation for memset ANSI library
*
* DESCRIPTION:
*       This file includes implementation for memset using the Marvell
*       device IDMA engines.
*
* DEPENDENCIES:
*       Marvell IDMA HW library.
*
*******************************************************************************/
#ifdef _DIAB_TOOL
volatile static void empty_func(void)
{
}
#else
/* includes */

#include "vxWorks.h"
#include "tickLib.h"
#include "mvOs.h"
#include "mvXor.h"


/* defines */
#define MV_IDMA_MAX_SIZE    (0xFFFFF)   /* 16MB - 1byte */
                       
/* locals */    

void * mvMemset(void *pBuff, int c, size_t size,MV_ULONG addr,MV_XOR_DESC *xorDesc);

extern void xor_waiton_eng(int chan);



int sysClkRateGet (void);

void cacheTest(int size, int incVal) {
    void *pBuff;
    char *pBuffTmp, *pBuffBase;
    int tick, time;
	int c = 0x33;
	int i;


    /* Allocate a buffer */
    pBuff = mvOsMalloc(size);
    
    if (pBuff == NULL)
	{
        mvOsPrintf("Fail to allocate buffer!\n");
        return;
    }

	tickSet(0);
	memset(pBuff, c, size);

	for (i = 0; i < 10000; i++)
	{
		pBuffBase = (char*)pBuff;

		for(pBuffTmp = pBuffBase; pBuffTmp < (pBuffBase + size) ; pBuffTmp += incVal)
		{
			if(*pBuffTmp != (char)c)
			{
				mvOsPrintf("ERROR at address 0x%x 0x%x\n",(UINT32)pBuffTmp,*pBuffTmp);
				free(pBuff);
				return;
			}
		}

	}

	tick = tickGet();

	mvOsPrintf("Done\n");

	time = tick * (1000 / sysClkRateGet());

	mvOsPrintf("Test took %d ms.\n", time);
	free(pBuff);
}

void l1CacheTest() {
	cacheTest(0x4000,1);
}

void mrc_p15_1_c15_c1_0()
{
    int regVal = 0;
    __asm volatile ("mrc p15, 1, %0, c15, c1, 0" : "=r" (regVal));
    mvOsPrintf("mcr returned 0x%08X;\n", regVal);
    if (regVal & BIT_28)
        mvOsPrintf("L2 cache is enabled\n");
    else
        mvOsPrintf("L2 cache is DISABLED\n");
}

void mrc_p15_0_c1_c0_0()
{
    int regVal = 0;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (regVal));
    mvOsPrintf("mcr returned 0x%08X;\n", regVal);

    if (regVal & BIT_2)
        mvOsPrintf("L1 DATA cache is enabled\n");
    else
        mvOsPrintf("L1 DATA cache is DISABLED\n");

    if (regVal & BIT_0)
        mvOsPrintf("MMU is enabled\n");
    else
        mvOsPrintf("MMU is DISABLED\n");
}

void checkCaches()
{
    mrc_p15_1_c15_c1_0();
    mrc_p15_0_c1_c0_0();
}

void l2CacheTest() {
    int size = 0x40000; /* 256 KB */
    void *pBuff;
    char *pBuffTmp, *pBuffBase;
    int tick, time;
    int c = 0x33;
    int i,j;


    /* Allocate a buffer */
    pBuff = mvOsMalloc(size);

    if (pBuff == NULL)
    {
        mvOsPrintf("Fail to allocate buffer!\n");
        return;
    }

    tickSet(0);
    memset(pBuff, c, size);

    for (i = 0; i < 10000; i++)
    {
        pBuffBase = (char*)pBuff;

        for (j = 0; j < 10000; j++)
        {
            for (pBuffTmp = pBuffBase + j * 0x20; pBuffTmp < pBuffBase + size ; pBuffTmp += 0x4000)
            {
                if (*pBuffTmp != (char)c)
                {
                    mvOsPrintf("ERROR at address 0x%x 0x%x\n",(UINT32)pBuffTmp,*pBuffTmp);
                    free(pBuff);
                    return;
                }
            }
        }
    }

    tick = tickGet();

    mvOsPrintf("Done\n");

    time = tick * (1000 / sysClkRateGet());

    mvOsPrintf("Test took %d ms.\n", time);
    free(pBuff);
}

/*******************************************************************************
* mvMemsetTest - Test routine for memset
*
* DESCRIPTION:
*       This function can run both system memset and Marvell's mvMemset 
*       according to a given size, filling it with the pattern "33'.
*       Selecting either memset is done according to a given parameter.
*       The function allocates a cached buffer and calls memset.
*       The function measures the time spent in the memset and displays the 
*       bandwidth calculated.
*
* INPUT:
*       size     - Size of buffer to set.
*       boolFlag - '0' -> Run Marvell mvMemset. '1' -> Run system memset.
*
* RETURN:
*       None
*
*******************************************************************************/
void mvMemsetTest(size_t size/*, int boolFlag*/)
{
    void *pBuff;
    char *pBuffTmp, *pBuffBase;
    int tick, time;
    int c = 0x33;
	MV_XOR_DESC *xorDesc;
	int i;


    /* Allocate a buffer */
    pBuff = mvOsMalloc(size);
    
    if (pBuff != NULL)
    {
        mvOsPrintf("Allocate a buffer. Pointer at 0x%x\n", (UINT32)pBuff);
    }
    else
    {
        mvOsPrintf("Fail to allocate buffer!\n");
        return;
    }

    xorDesc = mvOsIoUncachedMemalign(NULL, MV_XOR_DESC_ALIGNMENT,
                                     sizeof(MV_XOR_DESC), NULL);
    if (!xorDesc)
    {
        mvOsPrintf("\nEtrror malloc fail\n");
        return;
    }

    /* Set the buffer with a known pattern */
    mvOsPrintf("Set the buffer with a known pattern...");
    memset(pBuff, 0xFF, size);
    mvOsPrintf("Done\n");

   /* if (boolFlag)
    {*/
        mvOsPrintf("Calling the System memset routine....");
        tickSet(0);
		for (i = 0; i < 10000; i++)
		{
			memset(pBuff, c, size);

    /*}
    else
    {
        mvOsPrintf("Calling the Marvell memset routine....");
        tickSet(0);
        mvMemset(pBuff, c, size,addr, xorDesc);
    }*/

	    /* Make sure memory was set correctly */
    pBuffBase = (char*)pBuff;

    for(pBuffTmp = pBuffBase; pBuffTmp < (pBuffBase + size) ; pBuffTmp++)
    {
        if(*pBuffTmp != (char)c)
         {
            mvOsPrintf("ERROR at address 0x%x 0x%x\n",(UINT32)pBuffTmp,*pBuffTmp);
            free(pBuff);
            return;
        }
    }


		}
    tick = tickGet();
    
    mvOsPrintf("Done\n");

    time = tick * (1000 / sysClkRateGet());
    
    mvOsPrintf("Marvell memset took %d ms.\n", time);

    mvOsPrintf("Tesing buffer pattern...");
    
    
    mvOsPrintf("Done\n");

    mvOsPrintf("Bandwidth (MB/sec) = %f\n",size/(time*1000.0));

    free(pBuff);

    return;
}


/* Note: This function assumes Physical address = Virtual address       */
/* This function fragments the user block into 64MB block size          */
/* In order to gain performance, channel competion is tested directly   */
/* in the cause register                                                */
int mvMemsetCnt =0;
/*******************************************************************************
* mvMemset - Marvell memset routine
*
* DESCRIPTION:
*       This function implements memset using the Marvell device IDMA unit.
*       It has the same function declaration as the system memset.
*       The function will use the system memset to set initial small section 
*       with the pattern and then use it as source for the IDMA engines. 
*       All available IDMA engines are used for the memset purpose.
*
* INPUT:
*       pBuff - block of memory
*       c     - character to store
*       size  - size of memory
* RETURN:
*       Block of memeory pointer.
*
*******************************************************************************/
void * mvMemset(void *pBuff, int c, size_t size,MV_ULONG addr,MV_XOR_DESC *xorDesc)
{
    int chan;
    size_t         idmaSize = MV_IDMA_MAX_SIZE;
    size_t coveredSize = 0;     /* Size that already been DMAed */
	

    /* Set the first 128 bytes using memset. This buffer will be used   */
    /* as source for the IDMA engines                                   */
    memset(pBuff, c, 128); 
    
    /* Flushes and invalidates the data cache so any further            */
    /* access to the buffer will go directly to the DRAM.               */
    /* It is faster to flush all D cache that invalidate the buffer     */
    /* in case of large buffers                                         */
    cacheClear(DATA_CACHE, pBuff, ENTIRE_CACHE);
    
    
    /* Each of the IDMA channels can process up to 16MB.                 */
    /* We have 4 channels, thus total of 64MB max capacity per iteration */
    
    while ((int)size > 0)
    {
        if ((int)size - (MV_XOR_MAX_CHAN * MV_IDMA_MAX_SIZE) >= 0)
        {
            /* Each engine can handle its maximum byte count */
            idmaSize = MV_IDMA_MAX_SIZE;
        }
        else
        {
            idmaSize = size / 4;
        }

        /* Activate each channel    */
		for(chan = 0; chan < MV_XOR_MAX_CHAN; chan++)
		{
			mvMemsetCnt++;
			if(mvXorStateGet(chan) != MV_IDLE) {
				mvOsPrintf("ERR: %s XOR chan %d is not idle",__FUNCTION__, chan);
			}

			memset(xorDesc,0,sizeof(MV_XOR_DESC));
			xorDesc->srcAdd0 = (MV_U32)pBuff;
			xorDesc->phyDestAdd = (MV_U32)pBuff + coveredSize;
			xorDesc->byteCnt = idmaSize;
			xorDesc->phyNextDescPtr = 0;
			xorDesc->status = BIT31;
		
			if( mvXorTransfer(chan, MV_DMA, addr) != MV_OK)
			{
				mvOsPrintf("%s: DMA copy operation on channel %d failed!\n", __func__, chan);
				return pBuff;
			}

			xor_waiton_eng(chan);

			if (!(xorDesc->status & BIT30)) {
				mvOsPrintf("%s: DMA copy operation status is wrong on channel %d!\n", __func__, chan);
				return pBuff;		
			}

        
			/* Accumulate the memory area size already DMAd */
			coveredSize += idmaSize;
			mvOsPrintf("\ncoveredSize 0x%x idmaSize 0x%x size 0x%x\n",coveredSize, idmaSize, size);
        }

        size -= idmaSize * 4;
    }

    return pBuff;
}
#endif /* _DIAB_TOOL */
