/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemFaFox.c
*
* DESCRIPTION:
*       This is API implementation for Fox/Leopard fabric adapter memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 17 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/smem/smemFaFox.h>


static GT_U32 *  smemFaFoxBuffMngGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemFaFoxTwsiGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemFaFoxFtdllGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemFaFoxXbarExtMemTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemFaFoxVoqTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemFaFoxXbarTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemFaFoxCrxTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemFaFoxSegConfigReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static void smemFaActiveWriteTxdControl(
       IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
       IN         GT_U32   address,
       IN         GT_U32   memSize,
       IN         GT_U32 * memPtr,
       IN         GT_UINTPTR   param,
       IN         GT_U32 * inMemPtr
);
static void smemFaActiveWritePingMessage(
       IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
       IN         GT_U32   address,
       IN         GT_U32   memSize,
       IN         GT_U32 * memPtr,
       IN         GT_UINTPTR   param,
       IN         GT_U32 * inMemPtr
);
static void smemFaActiveCpuMailReadReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);
static void smemFaFoxInitFuncArray(
    INOUT FA_DEV_MEM_INFO  * devMemInfoPtr
);
static void smemFaFoxAllocSpecMemory(
    INOUT FA_DEV_MEM_INFO  * devMemInfoPtr
);


/* Private definition */
#define     GLOB_REGS_NUM_CNS               (0xFF/4  + 1)
#define     FTDLL_REGS_NUM_CNS              (0xFF/4  + 1)
#define     TWSI_REGS_NUM_CNS               (0xFF/4  + 1)
#define     BUFFMNG_REGS_NUM_CNS            (0xFFFF/4  + 1)
#define     VOQ_GLOB_REGS_NUM_CNS           (0xFFFF/4  + 1)
#define     CRX_GLOB_REGS_NUM_CNS           (0xFFFF/4  + 1)
#define     XBAR_CONF_REGS_NUM_CNS          (0xFF/4  + 1)
#define     XBAR_GRP_REGS_NUM_CNS           (0xFFFFF/4  + 1)
#define     XBAR_SDES_REGS_NUM_CNS          (0xFFFFF/4  + 1)

#define     SEG_REGS_NUM_CNS                (0xFF/4+1)
#define     SEG_CPUMAIL_REGS_NUM_CNS        (0xFF/4+1)
#define     RAM_REGS_NUM_CNS                (0x1FF/4  + 1)
#define     INTER_REGS_NUM_CNS              (0xFF/4  + 1)


/* more constants bits 23:26 for every register address
   determines the index to the address partitioning function */
#define     REG_FA_SPEC_FUNC_INDEX              (0x7800000)

#define     REG_FA_CPU_MAILBOX_MSG_SIZE         (0x64)

#define     SMEM_ACTIVE_MEM_TABLE_SIZE \
    (sizeof(smemFaActiveTable)/sizeof(smemFaActiveTable[0]))

#define     SMEM_FA_CPU_CPU_MAIL_BOX_REGISTERS  (0x41900000)

#define     SMEM_CPUTRIG_MASK_CNS               (1 << 16)

#define     SMEM_PCSPINGTRIG_MASK_CNS           (1 << 24)

#define     FA_MG_CELLS_TARGET_REG              (0x42108058)

#define     FA_CPU_MAIL_BOX_REG                 (0x41900000)

#define     FA_PING_CELL_TX_REG                 (0x42080018)

static GT_U32 ingPortOffset[]={0,0x8000,0x10000,0x100000,0x108000};
/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemFaActiveTable[] =
{
    {0x42080018,SMEM_FULL_MASK_CNS,
                                NULL,0,smemFaActiveWritePingMessage,  0},
    {0x42088018,SMEM_FULL_MASK_CNS,
                                NULL,0,smemFaActiveWritePingMessage,  0},
    {0x42090018,SMEM_FULL_MASK_CNS,
                                NULL,0,smemFaActiveWritePingMessage,  0},
    {0x42180018,SMEM_FULL_MASK_CNS,
                                NULL,0,smemFaActiveWritePingMessage,  0},
    {0x41800000,SMEM_FULL_MASK_CNS,
                                NULL,0,smemFaActiveWriteTxdControl,  0},
    {0x4100400,SMEM_FULL_MASK_CNS,
                                smemFaActiveCpuMailReadReg,0,NULL,0},
    /* must be last anyway */
    {0xffffffff, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};

/*******************************************************************************
*   smemFaFoxInit
*
* DESCRIPTION:
*       Initialize memory module for a fabric adapter device.
*
* INPUTS:
*       deviceObj   - pointer to device object.
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
void smemFaFoxInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    FA_DEV_MEM_INFO  * devMemInfoPtr;

    /* allocation of FA_DEV_MEM_INFO structure */
    devMemInfoPtr = (FA_DEV_MEM_INFO *)calloc(1, sizeof(FA_DEV_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
            skernelFatalError("smemFaFoxInit: allocation error\n");
    }
    /* init specific functions array */
    smemFaFoxInitFuncArray(devMemInfoPtr);

    /* allocate address type specific memories */
    smemFaFoxAllocSpecMemory(devMemInfoPtr);

    deviceObjPtr->devFindMemFunPtr = (void *)smemFaFoxFindMem;
    deviceObjPtr->deviceMemory = devMemInfoPtr;
}

/*******************************************************************************
*   smemFaFoxFindMem
*
* DESCRIPTION:
*       Return pointer to the register's or tables's memory.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       accessType  - read or write memory .
*       address     - address of memory(register or table).
*
* OUTPUTS:
*     activeMemPtrPtr - pointer to the active memory entry or NULL if not exist.
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*       implementation of finding register memory address in fa.
*
*******************************************************************************/
void * smemFaFoxFindMem
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    void                * memPtr;
    FA_DEV_MEM_INFO     * devMemInfoPtr;
    GT_32               index;
    GT_U32              param;

    if (deviceObjPtr == 0)
    {
        skernelFatalError("smemFaFoxFindMem: illegal pointer \n");
    }

    if (SMEM_ACCESS_PCI_FULL_MAC(accessType))
    {
        return NULL;
    }

    memPtr = 0;
    devMemInfoPtr = (FA_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    index = (address & REG_FA_SPEC_FUNC_INDEX) >> 23;
    if (index > 63)
    {
        skernelFatalError("smemFaFoxFindMem: index is out of range\n");
    }
    /* Call register spec function to obtain pointer to register memory */
    param   = devMemInfoPtr->specFunTbl[index].specParam;
    memPtr  = devMemInfoPtr->specFunTbl[index].specFun(deviceObjPtr,
                                                       accessType,
                                                       address,
                                                       memSize,
                                                       param);
    /* find active memory entry */
    if (activeMemPtrPtr != NULL)
    {
        *activeMemPtrPtr = NULL;
        for (index = 0; index < (SMEM_ACTIVE_MEM_TABLE_SIZE - 1); index++)
        {
            /* check address , take into account the mask */
            if ((address & smemFaActiveTable[index].mask)
                 == smemFaActiveTable[index].address)
                *activeMemPtrPtr = &smemFaActiveTable[index];
        }
    }

    return memPtr;
}

/*******************************************************************************
*   smemFaFoxBuffMngGlobalReg
*
* DESCRIPTION:
*       Global , UPLINK , Hot Swap and GPP  Registers.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*      none.
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*       buffer management memory space manager.
*
*******************************************************************************/
static GT_U32 *  smemFaFoxBuffMngGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    FA_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32           * regValPtr=NULL;
    GT_U32             index;

    regValPtr = 0;
    devMemInfoPtr = (FA_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Global registers */
    if ((address & 0xFFFFFF00) == 0x40000000)
    {/* Global registers */
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.globRegs ,
                         devMemInfoPtr->globalMem.globRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.globRegs[index];
    }
    else if ((address & 0xFFFF0000)==0x40010000)
    {/* Buffer management */
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.BuffMngRegs  ,
                         devMemInfoPtr->globalMem.BuffMngRegsNum   ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.BuffMngRegs[index];
    }

    return regValPtr;
}


/*******************************************************************************
*   smemFaFoxTwsiGlobalReg
*
* DESCRIPTION:
*       TWSI Registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*      none.
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*       TWSI memory space manager.
*
*******************************************************************************/
static GT_U32 *  smemFaFoxTwsiGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    FA_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32           * regValPtr=NULL;
    GT_U32             index;

    regValPtr = 0;
    devMemInfoPtr = (FA_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Global registers */
    if ((address & 0xFFFFFF00) == 0x43800000)
    {/* FTDLL registers */
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.twsiRegs,
                         devMemInfoPtr->globalMem.twsiRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.twsiRegs[index];
    }
    return regValPtr;

}


/*******************************************************************************
*   smemFaFoxFtdllGlobalReg
*
* DESCRIPTION:
*       Ftdll  Registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*      none.
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*       DFCDL memory space manager.
*
*******************************************************************************/
static GT_U32 *  smemFaFoxFtdllGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    FA_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32 * regValPtr=NULL;
    GT_U32 index;

    regValPtr = 0;
    devMemInfoPtr = (FA_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;

    if ((address & 0xFFFFFF00) == 0x43000000)
    {/* ft dll registers */
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.ftdllRegs,
                         devMemInfoPtr->globalMem.ftdllRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.ftdllRegs[index];
    }

    return regValPtr;
}



/*******************************************************************************
*   smemFaFoxXbarExtMemTableReg
*
* DESCRIPTION:
*       external memory registers Registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*      none.
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*       External memory space manager.
*
*******************************************************************************/
static GT_U32 *  smemFaFoxXbarExtMemTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    FA_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32 * regValPtr=NULL;
    GT_U32 index;

    regValPtr = 0;
    devMemInfoPtr = (FA_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Global registers */
    if ((address & 0xFFFFF000) == 0x42800000)
    {/* external memory registers */
        index = (address & 0x1FF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->RamMem.DbRegs,
                         devMemInfoPtr->RamMem.DbRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->RamMem.DbRegs[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemFaVoqTableReg
*
* DESCRIPTION:
*       Describe a Virtual Output Queue Registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*      none.
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*       VOQ memory space manager.
*
*******************************************************************************/
static GT_U32 *  smemFaFoxVoqTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    FA_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32           * regValPtr=NULL;
    GT_U32             index;

    regValPtr = 0;
    devMemInfoPtr = (FA_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;

    if ((address & 0xFFFF0000) == 0x40800000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->VOQMem.VOQRegs   ,
                         devMemInfoPtr->VOQMem.VOQRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->VOQMem.VOQRegs[index];
    }
    else if ((address & 0xFFFF0000) == 0x40880000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->VOQMem.VOQEntryRegs   ,
                         devMemInfoPtr->VOQMem.VOQEntryRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->VOQMem.VOQEntryRegs[index];
    }
    else if ((address & 0xFFFF0000) == 0x40810000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->VOQMem.VOQFCRegs   ,
                         devMemInfoPtr->VOQMem.VOQFCRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->VOQMem.VOQFCRegs[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemFaFoxXbarTableReg
*
* DESCRIPTION:
*
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*      none.
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*       Serdes memory space manager.
*
*******************************************************************************/
static GT_U32 *  smemFaFoxXbarTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    FA_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32           * regValPtr=NULL;
    GT_U32             index;

    regValPtr = 0;
    devMemInfoPtr = (FA_DEV_MEM_INFO *)deviceObjPtr->deviceMemory;

    if ((address & 0xFFFFFF00) == 0x42700000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->XbarMem.configRegs,
                         devMemInfoPtr->XbarMem.configRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->XbarMem.configRegs[index];
    }
    else if ((address & 0xFFF00000) == 0x42000000)
    {
        index = (address & 0xFFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->XbarMem.group1Regs,
                         devMemInfoPtr->XbarMem.group1RegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->XbarMem.group1Regs[index];
    }
    else if ((address & 0xFFF00000) == 0x42100000)
    {
        index = (address & 0xFFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->XbarMem.group2Regs,
                         devMemInfoPtr->XbarMem.group2RegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->XbarMem.group2Regs[index];
    }
    else if ((address & 0xFFFFFF00) == 0x42080000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->HyperGLinkinterMem.HyperGLink0InterRegs,
                         devMemInfoPtr->HyperGLinkinterMem.HyperGLink0InterRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->HyperGLinkinterMem.HyperGLink0InterRegs[index];
    }
    else if ((address & 0xFFFFFF00) == 0x42088000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->HyperGLinkinterMem.HyperGLink1InterRegs,
                         devMemInfoPtr->HyperGLinkinterMem.HyperGLink1InterRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->HyperGLinkinterMem.HyperGLink1InterRegs[index];
    }
    else if ((address & 0xFFFFFF00) == 0x42090000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->HyperGLinkinterMem.HyperGLink2InterRegs,
                         devMemInfoPtr->HyperGLinkinterMem.HyperGLink2InterRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->HyperGLinkinterMem.HyperGLink2InterRegs[index];
    }
    else if ((address & 0xFFFFFF00) == 0x42180000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->HyperGLinkinterMem.HyperGLink3InterRegs,
                         devMemInfoPtr->HyperGLinkinterMem.HyperGLink3InterRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->HyperGLinkinterMem.HyperGLink3InterRegs[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemFaFoxBuffMngGlobalReg
*
* DESCRIPTION:
*       Global , UPLINK , Hot Swap , TWSI and GPP  Registers.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*      none.
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*       Crx table memory management.
*
*******************************************************************************/
static GT_U32 *  smemFaFoxCrxTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    FA_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32           * regValPtr=NULL;
    GT_U32             index=0;

    regValPtr = 0;
    devMemInfoPtr = (FA_DEV_MEM_INFO *)deviceObjPtr->deviceMemory;

    if ((address & 0xFFFF0000) == 0x41000000)
    {
        index = (address & 0xFFFF) / 0x4 ;
        CHECK_MEM_BOUNDS(devMemInfoPtr->CRXMem.cellRegs ,
                         devMemInfoPtr->CRXMem.cellRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->CRXMem.cellRegs[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemFaFoxSegConfigReg
*
* DESCRIPTION:
*       CPU mailbox and PCS ping messages.
*
* INPUTS:
*       deviceObjPtr   - pointer to device memory object.
*       address        - address of register
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       none.
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*        Tx memory space manager.
*
*******************************************************************************/
static GT_U32 *  smemFaFoxSegConfigReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    FA_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32           * regValPtr=NULL;
    GT_U32             index = 0;

    regValPtr = 0;
    devMemInfoPtr = (FA_DEV_MEM_INFO *)deviceObjPtr->deviceMemory;

    if ((address & 0xFFFFFF00) == 0x41800000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->segRegsMem.segmentRegs  ,
                         devMemInfoPtr->segRegsMem.segmentRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->segRegsMem.segmentRegs[index];
    }
    else if ((address & 0xFFFFFF00) == 0x41900000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->segRegsMem.segmentCpuMailRegs ,
                         devMemInfoPtr->segRegsMem.segmentCpuMailRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->segRegsMem.segmentCpuMailRegs[index];
    }

    return regValPtr;
}


/*******************************************************************************
*   smemFaFatalError
*
* DESCRIPTION:
*      invoke exception in the fabric adapter simulated memory.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       None.
*
* RETURNS:
*        NULL
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFaFoxFatalError(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    skernelFatalError("smemFaFoxFatalError: illegal function pointer\n");

    return 0;
}

/*******************************************************************************
*  smemFaActiveWriteTxdControl
*
* DESCRIPTION:
*      Definition of the Active register read function.
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter -
*                      global interrupt bit number.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS: when simulation writes to 0x41800000 , the function is invoked
*           cpu sends new mailbox message to remote CPU.
*******************************************************************************/
static void smemFaActiveWriteTxdControl(
       IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
       IN         GT_U32   address,
       IN         GT_U32   memSize,
       IN         GT_U32 * memPtr,
       IN         GT_UINTPTR   param,
       IN         GT_U32 * inMemPtr
)
{
    GT_U32 trgFportBMP;
    GT_U32 dataLen;
    SBUF_BUF_ID bufferId;   /* buffer */
    GT_U8 *dataPtr;    /* pointer to the data in the buffer */
    GT_U32 dataSize;   /* data size */

    *memPtr = *inMemPtr;

    /**********************************************************************/
    /*  The format for the CPU MAILBOX message is                         */
    /*  BYTE 0 : SMAIN_SRC_TYPE_CPU_E(=2)                                 */
    /*  BYTE 1 : SMAIN_CPU_MAILBOX_MSG_E(=2)                              */
    /*  BYTE 2 : Target Fport                                             */
    /*  BYTE 3 : Message length ( in words !)                             */
    /*  BYTE4 ... : Data                                                  */
    /**********************************************************************/
    if (*memPtr & SMEM_CPUTRIG_MASK_CNS)
    {
        /* send message to SKernel task to process it. */
        /* get buffer */
        bufferId = sbufAlloc(deviceObjPtr->bufPool,
                             REG_FA_CPU_MAILBOX_MSG_SIZE * sizeof(GT_U32));
        if (bufferId == NULL)
        {
          printf(" smemFaActiveWriteTxdControl:no buffers to update MAC table\n");
          return;
        }
        /* get actual data pointer */
        sbufDataGet(bufferId, &dataPtr, &dataSize);

        /* save the data that the message is pcs ping one */
        *dataPtr =  SMAIN_SRC_TYPE_CPU_E;   /*1st byte */
        dataPtr++;
        *dataPtr =  SMAIN_CPU_MAILBOX_MSG_E;  /*2nd byte */
        dataPtr++;

        /* read register of 0x42108058 which saves the target fports ,            *
         * it must be always this address because the infport of fox is always 4  *
         * take a look in the function of xbarSetMgTrgPort(PSS)                   */
        smemRegFldGet(deviceObjPtr, FA_MG_CELLS_TARGET_REG, 0, 5, &trgFportBMP);
        trgFportBMP = (trgFportBMP & 0xF) ;/* set the target fports bmp            */
        memcpy(dataPtr , &trgFportBMP , sizeof(GT_U32));
        dataPtr++;

        /* find the celldata length from the register of 0x41800000 , bits[17:21],*/
        /* the register is updated in xbarCpuMailSend(PSS)                        */
        dataLen =  ( (((*memPtr) >> 17) & (0x1F)) +1); /* datalen in words */

        memcpy(dataPtr , &dataLen , sizeof(GT_U8));
        dataPtr++;

        /* read the celldata from the register of 0x41900000/ xbarCpuMailSend()   */
        memcpy(dataPtr, smemMemGet(deviceObjPtr,FA_CPU_MAIL_BOX_REG),(dataLen+1)*sizeof(GT_U32));

        /* set source type of buffer */
        bufferId->srcType = SMAIN_SRC_TYPE_CPU_E;

        /* set message type of buffer */
        bufferId->dataType = SMAIN_CPU_MAILBOX_MSG_E;

        /* put buffer to queue */
        squeBufPut(deviceObjPtr->queueId, SIM_CAST_BUFF(bufferId));
    }
}

/*******************************************************************************
*  smemFaActiveWritePingMessage
*
* DESCRIPTION:
*      Definition of the Active register read function.
*      cpu tries to write and send pcs ping message
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter -
*                      global interrupt bit number.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS: when simulation writes to 0x42080018,0x42088018,0x42090018,0x42180018
*           the function is invoked.  CPU sends new PCS ping to remote CPU.
*******************************************************************************/
static void smemFaActiveWritePingMessage(
    IN   SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN   GT_U32   address,
    IN   GT_U32   memSize,
    IN   GT_U32 * memPtr,
    IN   GT_UINTPTR   param,
    IN   GT_U32 * inMemPtr
)
{
    GT_U8 trgFport;
    GT_U8 trgInx;
    SBUF_BUF_ID bufferId;   /* buffer */
    GT_U8 *dataPtr;    /* pointer to the data in the buffer */
    GT_U32 dataSize;
    GT_U32 data;

    *memPtr = *inMemPtr;

    /**********************************************************************/
    /*  The format for the PCS Ping message is                            */
    /*  BYTE 0 : SMAIN_SRC_TYPE_CPU_E(=2)                                 */
    /*  BYTE 1 : SMAIN_CPU_PCSPING_MSG_E(=3)                              */
    /*  BYTE 2 : Target Fport                                             */
    /*  BYTE 3 : Message length ( in words !) , always 1                  */
    /*  BYTE4 ... : Data                                                  */
    /**********************************************************************/
    if (*memPtr & SMEM_PCSPINGTRIG_MASK_CNS)
    {
        /* send message to SKernel task to process it. */
        /* get buffer */
        bufferId = sbufAlloc(deviceObjPtr->bufPool,
                             REG_FA_CPU_MAILBOX_MSG_SIZE * sizeof(GT_U32));
        if (bufferId == NULL)
        {
            printf(" smemFaActiveWriteTxdControl:no buffers to update MAC table\n");
            return;
        }
        /* get actual data pointer */
        sbufDataGet(bufferId, &dataPtr, &dataSize);

        /* save the data that the message contains */
        *dataPtr =  SMAIN_SRC_TYPE_CPU_E;   /*1st byte */
        dataPtr++;
        *dataPtr =  SMAIN_CPU_PCSPING_MSG_E;  /*2nd byte */
        dataPtr++;

        /* find the target fports to send the pcs ping . (0x42080018=fport 0... */
        /* always saves it as a bitmap , the same as the cpu mail box */
        for (trgInx = 0;trgInx <= 3; trgInx++)
        {
            if (address == FA_PING_CELL_TX_REG + ingPortOffset[trgInx])
                break;
        }
        trgFport = ( 1 << trgInx); /* save the target port as a bmp , like cpu mb*/
        memcpy(dataPtr , &trgFport , sizeof(GT_U8) ); /*3rd byte */
        dataPtr++;

        /* set the length of the message in words!,always one in ping message  */
        *dataPtr = 1;
        dataPtr++;
        /* mask(0x42080018...) with bits[0:23] , because they are the *
         * only relevant bits for the pcs cell data information     */
        data = (*memPtr & 0xFFFFFF);
        memcpy(dataPtr,&data,sizeof(GT_U32)); /*4,5,6 bytes */

        /* set source type of buffer */
        bufferId->srcType = SMAIN_SRC_TYPE_CPU_E;

        /* set message type of buffer */
        bufferId->dataType = SMAIN_CPU_PCSPING_MSG_E;

        /* put buffer to queue */
        squeBufPut(deviceObjPtr->queueId, SIM_CAST_BUFF(bufferId));
    }
}

/*******************************************************************************
*   smemFaActiveCpuMailReadReg
*
* DESCRIPTION:
*   Read the content of a new received mailbox message
*
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter -
*                      global interrupt bit number.
*
* OUTPUTS:
*       outMemPtr - output memory register.
*
* RETURNS:
*
* COMMENTS:
*   There is only one word which is dedicated for new received mailbox message ,
*   When the cpu tries to read this word a new word replaces the old one in FIFO
*   manner.
*   The address of this register is 0x4100400
*******************************************************************************/
static void smemFaActiveCpuMailReadReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    FA_DEV_MEM_INFO *devMemInfoPtr;
    GT_U32  data;

    /* copy registers content to the output memory */
    *outMemPtr = *memPtr;

    devMemInfoPtr = (FA_DEV_MEM_INFO *)deviceObjPtr->deviceMemory;
    devMemInfoPtr->RcvCpuMailMsg.CPUMailMsgCounter++;
    data = devMemInfoPtr->RcvCpuMailMsg.CPUMailMsgData
            [devMemInfoPtr->RcvCpuMailMsg.CPUMailMsgCounter];
    /* the counter of CPUMailMsgCounter is zero in snetFaSendCpuMailToCpu */

    /* set the register of 0x4100400 for the new incoming mailbox message */
    smemRegSet(deviceObjPtr, address, data);
}

/*******************************************************************************
*   smemFaFoxInitFuncArray
*
* DESCRIPTION:
*       Init specific functions array.
*
* INPUTS:
*       devMemInfoPtr   - pointer to device memory object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*           0000 = Buffer Management, Global registers and Uplink
*           0001 = VOQ
*           0010 = Re-assembly processor (CRX)
*           0011 = Segmentation Processor (TxD) - not relevant for simu.
*           0100 = SXB SERDES and sunit registers
*           0101 = LD registers
*           0110 = Uplink Delay line configuration
*           0111 = TWSI registers
*******************************************************************************/
static void smemFaFoxInitFuncArray(
    INOUT FA_DEV_MEM_INFO  * devMemInfoPtr
)
{
    GT_U32              i;

    for (i = 0; i < 64; i++)
    {
        devMemInfoPtr->specFunTbl[i].specFun    = smemFaFoxFatalError;
    }
    devMemInfoPtr->specFunTbl[0].specFun        = smemFaFoxBuffMngGlobalReg;
    devMemInfoPtr->specFunTbl[1].specFun        = smemFaFoxVoqTableReg;
    devMemInfoPtr->specFunTbl[2].specFun        = smemFaFoxCrxTableReg;
    devMemInfoPtr->specFunTbl[3].specFun        = smemFaFoxSegConfigReg;
    devMemInfoPtr->specFunTbl[4].specFun        = smemFaFoxXbarTableReg;
    devMemInfoPtr->specFunTbl[5].specFun        = smemFaFoxXbarExtMemTableReg;
    devMemInfoPtr->specFunTbl[6].specFun        = smemFaFoxFtdllGlobalReg;
    devMemInfoPtr->specFunTbl[7].specFun        = smemFaFoxTwsiGlobalReg ;
}

/*******************************************************************************
*   smemFaFoxAllocSpecMemory
*
* DESCRIPTION:
*       Allocate address type specific memories.
*
* INPUTS:
*       devMemInfoPtr   - pointer to device memory object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       void
* COMMENTS:
*
*
*******************************************************************************/
static void smemFaFoxAllocSpecMemory(
    INOUT FA_DEV_MEM_INFO  * devMemInfoPtr
)
{
    /* Global register memory allocation */
    devMemInfoPtr->globalMem.globRegsNum = GLOB_REGS_NUM_CNS;
    devMemInfoPtr->globalMem.globRegs    =
            calloc(GLOB_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.globRegs == 0)
    {
        skernelFatalError("smemFaFoxAllocSpecMemory: allocation \
                           of globRegs error\n");
    }
    devMemInfoPtr->globalMem.ftdllRegsNum = FTDLL_REGS_NUM_CNS;
    devMemInfoPtr->globalMem.ftdllRegs    =
            calloc(FTDLL_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.ftdllRegs == 0)
    {
        skernelFatalError("smemFaFoxAllocSpecMemory: allocation \
                           of ftdll register error\n");
    }
    devMemInfoPtr->globalMem.twsiRegsNum = TWSI_REGS_NUM_CNS;
    devMemInfoPtr->globalMem.twsiRegs    =
            calloc(TWSI_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.twsiRegs == 0)
    {
        skernelFatalError("smemFaFoxAllocSpecMemory: allocation \
                           of twsi register error\n");
    }
    devMemInfoPtr->globalMem.BuffMngRegsNum = BUFFMNG_REGS_NUM_CNS;
    devMemInfoPtr->globalMem.BuffMngRegs    =
            calloc(BUFFMNG_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.BuffMngRegs == 0)
    {
        skernelFatalError("smemFaFoxAllocSpecMemory: allocation \
                           of buffer management register error\n");
    }

    /* VOQ global memory allocation */
    devMemInfoPtr->VOQMem.VOQRegsNum = VOQ_GLOB_REGS_NUM_CNS;
    devMemInfoPtr->VOQMem.VOQRegs    =
            calloc(VOQ_GLOB_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->VOQMem.VOQRegs == 0)
    {
        skernelFatalError("smemFaFoxAllocSpecMemory: allocation\
                           of VOQ registers error\n");
    }
    devMemInfoPtr->VOQMem.VOQEntryRegsNum = VOQ_GLOB_REGS_NUM_CNS;
    devMemInfoPtr->VOQMem.VOQEntryRegs    =
            calloc(VOQ_GLOB_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->VOQMem.VOQEntryRegs == 0)
    {
        skernelFatalError("smemFaFoxAllocSpecMemory: allocation\
                           of VOQ Entry registers error\n");
    }
    devMemInfoPtr->VOQMem.VOQFCRegsNum = VOQ_GLOB_REGS_NUM_CNS;
    devMemInfoPtr->VOQMem.VOQFCRegs    =
            calloc(VOQ_GLOB_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->VOQMem.VOQFCRegs == 0)
    {
        skernelFatalError("smemFaFoxAllocSpecMemory: allocation\
                           of VOQFC registers error\n");
    }

    /* CRX global */
    devMemInfoPtr->CRXMem.cellRegsNum = CRX_GLOB_REGS_NUM_CNS;
    devMemInfoPtr->CRXMem.cellRegs    =
            calloc(CRX_GLOB_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->CRXMem.cellRegs == 0)
    {
        skernelFatalError("smemFaFoxAllocSpecMemory: allocation \
                           of crx cell registers error\n");
    }

    /* Xbar registers */
    devMemInfoPtr->XbarMem.configRegsNum = XBAR_CONF_REGS_NUM_CNS;
    devMemInfoPtr->XbarMem.configRegs    =
            calloc(XBAR_CONF_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->XbarMem.configRegs == 0)
    {
        skernelFatalError("smemFaFoxAllocSpecMemory: allocation \
                           of config registers error\n");
    }
    devMemInfoPtr->XbarMem.group1RegsNum  = XBAR_SDES_REGS_NUM_CNS;
    devMemInfoPtr->XbarMem.group1Regs =
            calloc(XBAR_SDES_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->XbarMem.group1Regs == 0)
    {
        skernelFatalError("smemFaFoxAllocSpecMemory: allocation \
                           of gport1 registers error\n");
    }
    devMemInfoPtr->XbarMem.group2RegsNum  = XBAR_GRP_REGS_NUM_CNS;
    devMemInfoPtr->XbarMem.group2Regs    =
            calloc(XBAR_GRP_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->XbarMem.group2Regs == 0)
    {
        skernelFatalError("smemFaFoxAllocSpecMemory: allocation \
                           of gport2 registers error\n");
    }

    devMemInfoPtr->segRegsMem.segmentRegsNum = SEG_REGS_NUM_CNS;
    devMemInfoPtr->segRegsMem.segmentRegs  =
        calloc(SEG_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->segRegsMem.segmentRegs == 0)
    {
        skernelFatalError("smemFaFoxAllocSpecMemory: allocation \
                           of segmentRegs error\n");
    }
    devMemInfoPtr->segRegsMem.segmentCpuMailRegsNum = SEG_CPUMAIL_REGS_NUM_CNS;
    devMemInfoPtr->segRegsMem.segmentCpuMailRegs =
        calloc(SEG_CPUMAIL_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->segRegsMem.segmentCpuMailRegs == 0)
    {
        skernelFatalError("smemFaFoxAllocSpecMemory: allocation \
                           of segmentCpuMail registers error\n");
    }

    devMemInfoPtr->RamMem.DbRegsNum = RAM_REGS_NUM_CNS;
    devMemInfoPtr->RamMem.DbRegs =
        calloc (RAM_REGS_NUM_CNS , sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->RamMem.DbRegs == 0)
    {
        skernelFatalError("smemFaFoxAllocSpecMemory: allocation \
                           of DB registers error\n");
    }

    devMemInfoPtr->HyperGLinkinterMem.HyperGLink0InterRegsNum = INTER_REGS_NUM_CNS;
    devMemInfoPtr->HyperGLinkinterMem.HyperGLink0InterRegs =
        calloc (INTER_REGS_NUM_CNS , sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->HyperGLinkinterMem.HyperGLink0InterRegs == 0)
    {
        skernelFatalError("smemFaFoxAllocSpecMemory: allocation of \
                           HyperGLink0 interrupt register error\n");
    }
    devMemInfoPtr->HyperGLinkinterMem.HyperGLink1InterRegsNum = INTER_REGS_NUM_CNS;
    devMemInfoPtr->HyperGLinkinterMem.HyperGLink1InterRegs =
        calloc (INTER_REGS_NUM_CNS , sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->HyperGLinkinterMem.HyperGLink1InterRegs == 0)
    {
        skernelFatalError("smemFaFoxAllocSpecMemory: allocation of \
                           HyperGLink1 interrupt register error\n");
    }
    devMemInfoPtr->HyperGLinkinterMem.HyperGLink2InterRegsNum = INTER_REGS_NUM_CNS;
    devMemInfoPtr->HyperGLinkinterMem.HyperGLink2InterRegs =
        calloc (INTER_REGS_NUM_CNS , sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->HyperGLinkinterMem.HyperGLink2InterRegs == 0)
    {
        skernelFatalError("smemFaFoxAllocSpecMemory: allocation of \
                           HyperGLink2 interrupt register error\n");
    }
    devMemInfoPtr->HyperGLinkinterMem.HyperGLink3InterRegsNum = INTER_REGS_NUM_CNS;
    devMemInfoPtr->HyperGLinkinterMem.HyperGLink3InterRegs =
        calloc (INTER_REGS_NUM_CNS , sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->HyperGLinkinterMem.HyperGLink3InterRegs == 0)
    {
        skernelFatalError("smemFaFoxAllocSpecMemory: allocation of \
                           HyperGLink3 interrupt register error\n");
    }
}

