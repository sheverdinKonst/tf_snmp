/*******************************************************************************
*                Copyright 2004, MARVELL SEMICONDUCTOR, LTD.                   *
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

#ifndef MV_OS_VXW_H
#define MV_OS_VXW_H

/*
 * VxWorks
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <cacheLib.h>
#include <taskLib.h>
#include <intLib.h>
#include "mvSysHwConfig.h"

#include "mvTypes.h"
#include "mvCommon.h"

#define CACHE_DLINESIZE 32

#if defined(MV_MIPS)
#   include "mvVxwMips.h"
#elif defined (MV_PPC)
#   include "mvVxwPpc.h"
#elif defined (MV_ARM)
#   include "VxWorks/ArchARM/mvVxwArm.h"
#else
#   error "CPU type not selected"
#endif    

#define MV_INLINE   __inline
#define INLINE      __inline
#define __TASKCONV

#ifdef MV_RT_DEBUG
#define mvOsAssert(cond)        assert(cond)
#else
#define mvOsAssert(cond)
#endif /* MV_RT_DEBUG */

#define mvOsOutput              mvOsVxwPrintf
#define mvOsPrintf              mvOsVxwPrintf
#define mvOsSPrintf             sprintf

#define mvOsStrCmp		        strcmp

#define mvOsTaskLock            taskLock
#define mvOsTaskUnlock          taskUnlock

#define mvOsIntLock             intLock
#define mvOsIntUnlock           intUnlock

#define mvOsMalloc				malloc
#define mvOsCalloc              calloc
#define mvOsMemset              memset
#define mvOsMemcpy              memcpy
#define mvOsMemmove             memmove
#define mvOsRealloc				realloc
#define mvOsFree				free
#define mvOsDelay				sysMsDelay
#define mvOsUDelay              sysUsDelay

#define mvOsIoPhysToVirt(pDev, pPhysAddr)  ((MV_U32)pPhysAddr)

#define mvOsIoVirtToPhy(pDev, pVirtAddr)   CPU_PHY_MEM(pVirtAddr)

/* Task Managment */

MV_STATUS   mvOsTaskCreate(char  *name,
                          unsigned long prio,
                          unsigned long stack,
                          unsigned (__TASKCONV *start_addr)(void*),
                          void   *arglist,
                          unsigned long *tid);
MV_STATUS   mvOsTaskIdent (char *name, unsigned long *tid);
MV_STATUS   mvOsTaskDelete(unsigned long tid);
MV_STATUS   mvOsTaskSuspend(unsigned long tid);
MV_STATUS   mvOsTaskResume(unsigned long tid);

/* Queues Managment */

MV_STATUS   mvOsQueueCreate(char *name, unsigned long size, unsigned long *qid);
MV_STATUS   mvOsQueueDelete(unsigned long qid);
MV_STATUS   mvOsQueueWait(unsigned long qid, void* msg, unsigned long time_out);
MV_STATUS   mvOsQueueSend(unsigned long qid, void* msg);

#define mvOsCacheClear mvOsCacheClear  /* For osServices\mvOs.h Diab error */

/* Cache Managment */
static INLINE MV_ULONG    mvOsCacheClear(void* pDev, void* pVirtAddr, int size)
{
    cacheClear(DATA_CACHE, (pVirtAddr), (size));
    return CPU_PHY_MEM(pVirtAddr);
}

#define mvOsCacheFlush mvOsCacheFlush  /* For osServices\mvOs.h Diab error */
        
static INLINE MV_ULONG    mvOsCacheFlush(void* pDev, void* pVirtAddr, int size)
{
    cacheFlush(DATA_CACHE, (pVirtAddr), (size));
    return CPU_PHY_MEM(pVirtAddr);
}

static INLINE MV_ULONG   mvOsCacheInvalidate (void* pDev, void* pVirtAddr, int size)
{
    cacheInvalidate(DATA_CACHE, (pVirtAddr), (size));
    return CPU_PHY_MEM(pVirtAddr);
}

extern void mv_l2_inv_range(const void *start, const void *end);
static INLINE void mvOsCacheUnmap(void* pDev, const void *virtAddr, MV_U32 size)
{
#ifdef CONFIG_MV_SP_I_FTCH_DB_INV
  mv_l2_inv_range(virtAddr, (const void *)((MV_U32)virtAddr + size));
#endif
}

void mvOsVxwPrintf(const char *  fmt, ...);
void sysMsDelay(UINT ms);
void sysUsDelay(UINT us);

/* register manipulations  */

/******************************************************************************
* This debug function enable the write of each register that u-boot access to 
* to an array in the DRAM, the function record only MV_REG_WRITE access.
* The function could not be operate when booting from flash.
* In order to print the array we use the printreg command.
******************************************************************************/
/* #define REG_DEBUG */
#if defined(REG_DEBUG)
extern int reg_arry[2048][2];
extern int reg_arry_index;
#endif

/* Marvell controller register read/write macros */
#define MV_REG_VALUE(offset)          \
                (MV_MEMIO32_READ((INTER_REGS_BASE | (offset))))

#define MV_REG_READ(offset)             \
        (MV_MEMIO_LE32_READ(INTER_REGS_BASE | (offset)))

#if defined(REG_DEBUG)
#define MV_REG_WRITE(offset, val)    \
        MV_MEMIO_LE32_WRITE((INTER_REGS_BASE | (offset)), (val)); \
        { \
                reg_arry[reg_arry_index][0] = (INTER_REGS_BASE | (offset));\
                reg_arry[reg_arry_index][1] = (val);\
                reg_arry_index++;\
        }
#else
#define MV_REG_WRITE(offset, val)    \
        MV_MEMIO_LE32_WRITE((INTER_REGS_BASE | (offset)), (val));
#endif
                                                
#define MV_REG_BYTE_READ(offset)        \
        (MV_MEMIO8_READ((INTER_REGS_BASE | (offset))))

#if defined(REG_DEBUG)
#define MV_REG_BYTE_WRITE(offset, val)  \
        MV_MEMIO8_WRITE((INTER_REGS_BASE | (offset)), (val)); \
        { \
                reg_arry[reg_arry_index][0] = (INTER_REGS_BASE | (offset));\
                reg_arry[reg_arry_index][1] = (val);\
                reg_arry_index++;\
        }
#else
#define MV_REG_BYTE_WRITE(offset, val)  \
        MV_MEMIO8_WRITE((INTER_REGS_BASE | (offset)), (val))
#endif

#if defined(REG_DEBUG)
#define MV_REG_BIT_SET(offset, bitMask)                 \
        (MV_MEMIO32_WRITE((INTER_REGS_BASE | (offset)), \
         (MV_MEMIO32_READ(INTER_REGS_BASE | (offset)) | \
          MV_32BIT_LE_FAST(bitMask)))); \
        { \
                reg_arry[reg_arry_index][0] = (INTER_REGS_BASE | (offset));\
                reg_arry[reg_arry_index][1] = (MV_MEMIO32_READ(INTER_REGS_BASE | (offset)));\
                reg_arry_index++;\
        }
#else
#define MV_REG_BIT_SET(offset, bitMask)                 \
        (MV_MEMIO32_WRITE((INTER_REGS_BASE | (offset)), \
         (MV_MEMIO32_READ(INTER_REGS_BASE | (offset)) | \
          MV_32BIT_LE_FAST(bitMask))))
#endif
        
#if defined(REG_DEBUG)
#define MV_REG_BIT_RESET(offset,bitMask)                \
        (MV_MEMIO32_WRITE((INTER_REGS_BASE | (offset)), \
         (MV_MEMIO32_READ(INTER_REGS_BASE | (offset)) & \
          MV_32BIT_LE_FAST(~bitMask)))); \
        { \
                reg_arry[reg_arry_index][0] = (INTER_REGS_BASE | (offset));\
                reg_arry[reg_arry_index][1] = (MV_MEMIO32_READ(INTER_REGS_BASE | (offset)));\
                reg_arry_index++;\
        }
#else
#define MV_REG_BIT_RESET(offset,bitMask)                \
        (MV_MEMIO32_WRITE((INTER_REGS_BASE | (offset)), \
         (MV_MEMIO32_READ(INTER_REGS_BASE | (offset)) & \
          MV_32BIT_LE_FAST(~bitMask))))
#endif


#endif /* MV_OS_VXW_H */
