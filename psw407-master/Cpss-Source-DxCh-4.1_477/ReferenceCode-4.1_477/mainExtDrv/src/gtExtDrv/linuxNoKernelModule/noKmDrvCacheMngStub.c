/*******************************************************************************
*                Copyright 2015, MARVELL SEMICONDUCTOR, LTD.                   *
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
********************************************************************************
* noKmDrvCacheMngStub.c
*
* DESCRIPTION:
*       Stubs for cache management functions
*       Emulation using malloc()
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*
*******************************************************************************/
#define _GNU_SOURCE
#include <gtExtDrv/drivers/gtCacheMng.h>

/*******************************************************************************
* extDrvMgmtCacheFlush
*
* DESCRIPTION:
*       Flush to RAM content of cache
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
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS extDrvMgmtCacheFlush
(
    IN GT_MGMT_CACHE_TYPE_ENT   type, 
    IN void                     *address_PTR, 
    IN size_t                   size
)
{
    (void) type;
    (void) address_PTR;
    (void) size;
    return GT_OK;
}


/*******************************************************************************
* extDrvMgmtCacheInvalidate
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
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS extDrvMgmtCacheInvalidate 
(
    IN GT_MGMT_CACHE_TYPE_ENT   type, 
    IN void                     *address_PTR, 
    IN size_t                   size
)
{
    (void) type;
    (void) address_PTR;
    (void) size;
    return GT_OK;
}

#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
static void*  dmaPtr = NULL;
static GT_U32 dmaLen = 2*1024*1024;

#ifndef MAP_32BIT
#define MAP_32BIT 0x40
#endif

/*******************************************************************************
* check_dma
*
* DESCRIPTION:
*       Check if DMA block already allocated/mapped to userspace
*       If no then allocate/map
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None
*
*******************************************************************************/
static void check_dma(void)
{
    if (dmaPtr != NULL)
        return;
    dmaPtr = mmap(NULL, dmaLen,
        PROT_READ | PROT_WRITE,
        MAP_32BIT | MAP_ANONYMOUS | MAP_PRIVATE,
        0, 0);
}
/*******************************************************************************
* extDrvGetDmaBase
*
* DESCRIPTION:
*       Get the base address of the DMA area need for Virt2Phys and Phys2Virt
*           translaton
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       dmaBase     - he base address of the DMA area.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS extDrvGetDmaBase
(
	OUT GT_UINTPTR * dmaBase
)
{
    /* TODO */
    /*  *dmaBase = (GT_UINTPTR)base; */
    check_dma();
    *dmaBase = (GT_UINTPTR)dmaPtr;
    return GT_OK;
}

/*******************************************************************************
* extDrvGetDmaVirtBase
*
* DESCRIPTION:
*       Get the base address of the DMA area in userspace
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       dmaVirtBase     - the base address of the DMA area in userspace.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS extDrvGetDmaVirtBase
(
	OUT GT_UINTPTR *dmaVirtBase
)
{
    /* TODO */
    /* *dmaVirtBase = LINUX_VMA_DMABASE; */
    check_dma();
    *dmaVirtBase = (GT_UINTPTR)dmaPtr;
    return GT_OK;
}

/*******************************************************************************
* extDrvGetDmaSize
*
* DESCRIPTION:
*       Get the size of the DMA area
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       dmaSize     - The size of the DMA area.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*
*******************************************************************************/
extern GT_STATUS extDrvGetDmaSize
(
	OUT GT_U32 * dmaSize
)
{
    /* TODO */
    *dmaSize = dmaLen;
    return GT_OK;
}
