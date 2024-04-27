/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemCapuera.c
*
* DESCRIPTION:
*       This is API implementation for xbar capoeira device memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 7 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/smem/smemCapuera.h>

static GT_U32 *  smemCapXbarFatalError
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static void smemCapXbarInitFuncArray
(
    INOUT CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr
);
static void smemCapXbarAllocSpecMemory
(
    INOUT CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr
);
static GT_U32 *  smemCapXbarGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemCapXbarPort0Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemCapXbarPort1Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemCapXbarPort2Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemCapXbarPort3Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemCapXbarPort4Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemCapXbarPort5Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemCapXbarPort6Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemCapXbarPort7Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemCapXbarPort8Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemCapXbarPort9Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemCapXbarPort10Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemCapXbarPort11Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemCapXbarPGReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static void smemCapXbarIniIndexArr(
void
);
static void smemCapXbarActiveWritePingMessage(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32   address,
    IN GT_U32   memSize,
    IN GT_U32 * memPtr,
    IN GT_UINTPTR   param,
    IN GT_U32 * inMemPtr
);
static void smemCapXbarActiveSendCellMessage(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32   address,
    IN GT_U32   memSize,
    IN GT_U32 * memPtr,
    IN GT_UINTPTR   param,
    IN GT_U32 * inMemPtr
);

static GT_U8 smemXbarFuncsIndex[0xBFF];



/* registers definition */
#define     SMEM_CAP_PG_REGS_NUM_CNS                (0x8)
#define     SMEM_CAP_CONTROL_REGS_NUM_CNS           (0x1E)
#define     SMEM_CAP_HYPERGLINK_REGS_NUM_CNS        (0x18)
#define     SMEM_CAP_MCGROUP_REGS_NUM_CNS           (0xFFF)
#define     SMEM_CAP_SERDES_REGS_NUM_CNS            (0xC)
#define     SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS       (0x40)
#define     SMEM_CAP_COUNTER_REGS_NUM_CNS           (0x3)
#define     SMEM_CAP_GLOB_REGS_NUM_CNS              (0xC0)

/* more constants */
#define     SMEM_ACTIVE_MEM_TABLE_SIZE \
            (sizeof(smemCapXbarActiveTable)/sizeof(smemCapXbarActiveTable[0]))

#define     SMEM_CAP_PCSPINGTRIG_MASK_CNS       (1 << 15)
#define     SMEM_CAP_CPUTRIG_MASK_CNS           (1 << 16)
#define     SMEM_CAP_PCSPING_MSG_SIZE           (1)
#define     SMEM_CAP_CPU_MAILBOX_MSG_SIZE       (0x64)

#define     SMEM_CAP_PING_CELL_TX(n)            (0x00880000 + ((n)%4)*   \
                                                 0x00100000 + ((n)/4)*   \
                                                 0x00008000 + 0x18)

/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemCapXbarActiveTable[] =
{
    /* must be last anyway */
    {0x00880008,SMEM_FULL_MASK_CNS,
                                NULL,0,smemCapXbarActiveWritePingMessage,0},
    {0x00980008,SMEM_FULL_MASK_CNS,
                                NULL,0,smemCapXbarActiveWritePingMessage,0},
    {0x00A80008,SMEM_FULL_MASK_CNS,
                                NULL,0,smemCapXbarActiveWritePingMessage,0},
    {0x00B80008,SMEM_FULL_MASK_CNS,
                                NULL,0,smemCapXbarActiveWritePingMessage,0},
    {0x00888008,SMEM_FULL_MASK_CNS,
                                NULL,0,smemCapXbarActiveWritePingMessage,0},
    {0x00988008,SMEM_FULL_MASK_CNS,
                                NULL,0,smemCapXbarActiveWritePingMessage,0},
    {0x00A88008,SMEM_FULL_MASK_CNS,
                                NULL,0,smemCapXbarActiveWritePingMessage,0},
    {0x00B88008,SMEM_FULL_MASK_CNS,
                                NULL,0,smemCapXbarActiveWritePingMessage,0},
    {0x00890008,SMEM_FULL_MASK_CNS,
                                NULL,0,smemCapXbarActiveWritePingMessage,0},
    {0x00990008,SMEM_FULL_MASK_CNS,
                                NULL,0,smemCapXbarActiveWritePingMessage,0},
    {0x00A90008,SMEM_FULL_MASK_CNS,
                                NULL,0,smemCapXbarActiveWritePingMessage,0},
    {0x00B90008,SMEM_FULL_MASK_CNS,
                                NULL,0,smemCapXbarActiveWritePingMessage,0},
    {0x00000080,SMEM_FULL_MASK_CNS,
                                NULL,0,smemCapXbarActiveSendCellMessage,0},
    {0xffffffff, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};


/*******************************************************************************
*   smemCapXbarInit
*
* DESCRIPTION:
*       Initialize memory module for a capoeira xbar device.
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
void smemCapXbarInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr;

    /* alloc FA_DEV_MEM_INFO */
    devMemInfoPtr = (CAP_XBAR_DEV_MEM_INFO *)calloc(1,sizeof(CAP_XBAR_DEV_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
            skernelFatalError("smemCapXbarInit: allocation error\n");
    }

    /* init specific functions array */
    smemCapXbarInitFuncArray(devMemInfoPtr);

    /* allocate address type specific memories */
    smemCapXbarAllocSpecMemory(devMemInfoPtr);

    deviceObjPtr->devFindMemFunPtr = (void *)smemCapXbarFindMem;
    deviceObjPtr->deviceMemory = devMemInfoPtr;
}

/*******************************************************************************
*   smemCapXbarFindMem
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
void * smemCapXbarFindMem
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    void                    * memPtr;
    CAP_XBAR_DEV_MEM_INFO   * devMemInfoPtr;
    GT_32                     num;
    GT_U32                    param=0;
    GT_U8                     index=0;

    if (deviceObjPtr == 0)
    {
        skernelFatalError("smemCapXbarFindMem : illegal pointer \n");
    }

    if (SMEM_ACCESS_PCI_FULL_MAC(accessType))
    {
           return NULL;
    }

    memPtr = 0;
    devMemInfoPtr = (CAP_XBAR_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    num = ((address >> 12) & 0xFFF) ;
    index = smemXbarFuncsIndex[num];
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
            if ((address & smemCapXbarActiveTable[index].mask)
                 == smemCapXbarActiveTable[index].address)
                *activeMemPtrPtr = &smemCapXbarActiveTable[index];
        }
    }

    return memPtr;
}

/*******************************************************************************
*   smemCapXbarFatalError
*
* DESCRIPTION:
*      invoke exception in the cpoeira xbar device simulated memory.
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
static GT_U32 *  smemCapXbarFatalError(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    skernelFatalError("smemCapXbarFatalError: illegal function pointer\n");

    return 0;
}

/*******************************************************************************
*   smemCapXbarInitFuncArray
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
*******************************************************************************/
static void smemCapXbarInitFuncArray(
    INOUT CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr
)
{
    GT_U8              i;

    devMemInfoPtr->specFunTbl[0].specFun      = smemCapXbarGlobalReg;
    devMemInfoPtr->specFunTbl[1].specFun      = smemCapXbarPGReg;
    devMemInfoPtr->specFunTbl[2].specFun      = smemCapXbarPort0Reg;
    devMemInfoPtr->specFunTbl[3].specFun      = smemCapXbarPort1Reg;
    devMemInfoPtr->specFunTbl[4].specFun      = smemCapXbarPort2Reg;
    devMemInfoPtr->specFunTbl[5].specFun      = smemCapXbarPort3Reg;
    devMemInfoPtr->specFunTbl[6].specFun      = smemCapXbarPort4Reg;
    devMemInfoPtr->specFunTbl[7].specFun      = smemCapXbarPort5Reg;
    devMemInfoPtr->specFunTbl[8].specFun      = smemCapXbarPort6Reg;
    devMemInfoPtr->specFunTbl[9].specFun      = smemCapXbarPort7Reg;
    devMemInfoPtr->specFunTbl[10].specFun     = smemCapXbarPort8Reg;
    devMemInfoPtr->specFunTbl[11].specFun     = smemCapXbarPort9Reg;
    devMemInfoPtr->specFunTbl[12].specFun     = smemCapXbarPort10Reg;
    devMemInfoPtr->specFunTbl[13].specFun     = smemCapXbarPort11Reg;

    for (i = 14 ; i < 64; i++)
    {
      devMemInfoPtr->specFunTbl[i].specFun    = smemCapXbarFatalError;
    }
}

/*******************************************************************************
*   smemCapXbarGlobalReg
*
* DESCRIPTION:
*       global capoeira xbar device registers.
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
*
*******************************************************************************/
static GT_U32 *  smemCapXbarGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32 * regValPtr=NULL;
    GT_U32 index;

    regValPtr = 0;
    devMemInfoPtr = (CAP_XBAR_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Global registers */
    if ((address & 0xFFFFF000) == 0x00000000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->GlobalMem.globRegs   ,
                         devMemInfoPtr->GlobalMem.globRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->GlobalMem.globRegs[index];
    }

    return regValPtr;
}


/*******************************************************************************
*   smemCapXbarPort0Reg
*
* DESCRIPTION:
*       first port capoeira registers.
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
*
*******************************************************************************/
static GT_U32 *  smemCapXbarPort0Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32 * regValPtr=NULL;
    GT_U32 index;
    GT_U32 subAddr;

    devMemInfoPtr = (CAP_XBAR_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    subAddr = ((address & 0x00FF0000) >> 16) ;

    switch (subAddr)
    {
    case 0x80:
    {
        switch (address & 0xFFFFFF00)
        {
        case 0x00800000:
           index = (address & 0xFF) / 0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport0Mem.ControlRegs ,
                            devMemInfoPtr->Fport0Mem.ControlRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport0Mem.ControlRegs[index];
           break;
        case 0x00804000:
           index = (address & 0xFF) / 0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport0Mem.TarPort2DevRegs ,
                            devMemInfoPtr->Fport0Mem.TarPort2DevRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport0Mem.TarPort2DevRegs[index];
           break;
        default :
           index = (address & 0xF) / 0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport0Mem.CounterRegs ,
                            devMemInfoPtr->Fport0Mem.CounterRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport0Mem.CounterRegs[index];
           break;
        }
    }
    break;
    case 0x88:
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->Fport0Mem.HyperGlinkRegs ,
                         devMemInfoPtr->Fport0Mem.HyperGlinkRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->Fport0Mem.HyperGlinkRegs[index];
        break;
    case 0x84:
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->Fport0Mem.MCGroupRegs ,
                         devMemInfoPtr->Fport0Mem.MCGroupRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->Fport0Mem.MCGroupRegs[index];
        break;
    case 0x8C:
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->Fport0Mem.SerdesRegs ,
                         devMemInfoPtr->Fport0Mem.SerdesRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->Fport0Mem.SerdesRegs[index];
        break;
    }

    return regValPtr;
}


/*******************************************************************************
*   smemCapXbarPort1Reg
*
* DESCRIPTION:
*       first port capoeira Registers.
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
*
*******************************************************************************/
static GT_U32 *  smemCapXbarPort1Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                 * regValPtr=NULL;
    GT_U32                   index;
    GT_U32                   subAddr;

    devMemInfoPtr = (CAP_XBAR_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    subAddr = ((address & 0x00FF0000) >> 16) ;

    switch (subAddr)
    {
    case 0x90:
    {
        switch (address & 0xFFFFFF00)
        {
        case 0x00900000:
           index = (address & 0xFF) / 0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport1Mem.ControlRegs ,
                            devMemInfoPtr->Fport1Mem.ControlRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport1Mem.ControlRegs[index];
           break;
        case 0x00904000 :
           index = (address & 0xFF) / 0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport1Mem.TarPort2DevRegs ,
                            devMemInfoPtr->Fport1Mem.TarPort2DevRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport1Mem.TarPort2DevRegs[index];
           break;
        default :
           index = (address & 0xF) / 0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport1Mem.CounterRegs ,
                            devMemInfoPtr->Fport1Mem.CounterRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport1Mem.CounterRegs[index];
           break;
        }
    }
    break;
    case 0x98:
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->Fport1Mem.HyperGlinkRegs ,
                         devMemInfoPtr->Fport1Mem.HyperGlinkRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->Fport1Mem.HyperGlinkRegs[index];
        break ;
    case 0x94:
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->Fport1Mem.MCGroupRegs ,
                         devMemInfoPtr->Fport1Mem.MCGroupRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->Fport1Mem.MCGroupRegs[index];
        break ;
    case 0x9C :
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->Fport1Mem.SerdesRegs ,
                         devMemInfoPtr->Fport1Mem.SerdesRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->Fport1Mem.SerdesRegs[index];
        break;
    }

    return regValPtr;
}


/*******************************************************************************
*   smemCapXbarPort2Reg
*
* DESCRIPTION:
*       second port capoeira Registers.
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
*
*******************************************************************************/
static GT_U32 *  smemCapXbarPort2Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                 * regValPtr=NULL;
    GT_U32                   index;
    GT_U32                   subAddr;

    devMemInfoPtr = (CAP_XBAR_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    subAddr = ((address & 0x00FF0000) >> 16) ;

    switch (subAddr)
    {
    case 0xA0:
    {
        switch (address & 0xFFFFFF00)
        {
        case 0x00A00000:
           index = (address & 0xFF) / 0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport2Mem.ControlRegs ,
                            devMemInfoPtr->Fport2Mem.ControlRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport2Mem.ControlRegs[index];
           break;
        case 0x00A04000:
           index = (address & 0xFF) / 0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport2Mem.TarPort2DevRegs ,
                            devMemInfoPtr->Fport2Mem.TarPort2DevRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport2Mem.TarPort2DevRegs[index];
           break ;
        default :
           index = (address & 0xF) / 0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport2Mem.CounterRegs ,
                            devMemInfoPtr->Fport2Mem.CounterRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport2Mem.CounterRegs[index];
           break;
        }
    }
    break;
    case 0xA8:
      index = (address & 0xFF) / 0x4;
      CHECK_MEM_BOUNDS(devMemInfoPtr->Fport2Mem.HyperGlinkRegs ,
                       devMemInfoPtr->Fport2Mem.HyperGlinkRegsNum ,
                       index, memSize);
      regValPtr = &devMemInfoPtr->Fport2Mem.HyperGlinkRegs[index];
      break ;
    case 0xA4:
      index = (address & 0xFFF) / 0x4;
      CHECK_MEM_BOUNDS(devMemInfoPtr->Fport2Mem.MCGroupRegs ,
                       devMemInfoPtr->Fport2Mem.MCGroupRegsNum ,
                       index, memSize);
      regValPtr = &devMemInfoPtr->Fport2Mem.MCGroupRegs[index];
      break;
    case 0xAC:
      index = (address & 0xFFF) / 0x4;
      CHECK_MEM_BOUNDS(devMemInfoPtr->Fport2Mem.SerdesRegs ,
                       devMemInfoPtr->Fport2Mem.SerdesRegsNum ,
                       index, memSize);
      regValPtr = &devMemInfoPtr->Fport2Mem.SerdesRegs[index];
      break;
    }

    return regValPtr;
}


/*******************************************************************************
*   smemCapXbarPort3Reg
*
* DESCRIPTION:
*       third port capoeira Registers.
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
*
*******************************************************************************/
static GT_U32 *  smemCapXbarPort3Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                 * regValPtr=NULL;
    GT_U32                   index;
    GT_U32                   subAddr;

    devMemInfoPtr = (CAP_XBAR_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    subAddr = ((address & 0x00FF0000) >>16) ;

    switch  (subAddr)
    {
    case 0xB0:
    {
        switch (address & 0xFFFFFF00)
        {
        case 0x00B00000:
           index = (address & 0xFF) / 0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport3Mem.ControlRegs ,
                            devMemInfoPtr->Fport3Mem.ControlRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport3Mem.ControlRegs[index];
           break;
        case 0x00B04000:
           index = (address & 0xFF) / 0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport3Mem.TarPort2DevRegs ,
                            devMemInfoPtr->Fport3Mem.TarPort2DevRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport3Mem.TarPort2DevRegs[index];
           break;
        default:
           index = (address & 0xF) / 0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport3Mem.CounterRegs ,
                            devMemInfoPtr->Fport3Mem.CounterRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport3Mem.CounterRegs[index];
           break;
        }
    }
    break;
    case 0xB8:
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->Fport3Mem.HyperGlinkRegs ,
                         devMemInfoPtr->Fport3Mem.HyperGlinkRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->Fport3Mem.HyperGlinkRegs[index];
        break;
    case 0xB4:
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->Fport3Mem.MCGroupRegs ,
                         devMemInfoPtr->Fport3Mem.MCGroupRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->Fport3Mem.MCGroupRegs[index];
        break;
    case 0xBC:
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->Fport3Mem.SerdesRegs ,
                         devMemInfoPtr->Fport3Mem.SerdesRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->Fport3Mem.SerdesRegs[index];
        break;
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCapXbarPort4Reg
*
* DESCRIPTION:
*       4th port capoeira Registers.
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
*
*******************************************************************************/
static GT_U32 *  smemCapXbarPort4Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32             * regValPtr=NULL;
    GT_U32             index;
    GT_U32             subAddr;

    devMemInfoPtr = (CAP_XBAR_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    subAddr = ((address & 0x00FF0000) >>16) ;

    switch (subAddr)
    {
    case 0x80:
    {
        switch (address & 0xFFFFFF00)
        {
        case 0x00808000:
           index = (address & 0xFF) / 0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport4Mem.ControlRegs ,
                            devMemInfoPtr->Fport4Mem.ControlRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport4Mem.ControlRegs[index];
           break;
        case 0x0080C000:
           index = (address & 0xFF) / 0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport4Mem.TarPort2DevRegs ,
                            devMemInfoPtr->Fport4Mem.TarPort2DevRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport4Mem.TarPort2DevRegs[index];
           break;
        default :
           index = (address & 0xF) / 0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport4Mem.CounterRegs ,
                            devMemInfoPtr->Fport4Mem.CounterRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport4Mem.CounterRegs[index];
           break;
        }
    }
    break;
    case 0x88:
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->Fport4Mem.HyperGlinkRegs ,
                         devMemInfoPtr->Fport4Mem.HyperGlinkRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->Fport4Mem.HyperGlinkRegs[index];
        break;
    case 0x8C:
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->Fport4Mem.SerdesRegs ,
                         devMemInfoPtr->Fport4Mem.SerdesRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->Fport4Mem.SerdesRegs[index];
        break;
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCapXbarPort5Reg
*
* DESCRIPTION:
*       5th port capoeira Registers.
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
*
*******************************************************************************/
static GT_U32 *  smemCapXbarPort5Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                 * regValPtr=NULL;
    GT_U32                   index;
    GT_U32                   subAddr;

    devMemInfoPtr = (CAP_XBAR_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    subAddr = ((address & 0x00FF0000) >> 16) ;

    switch (subAddr)
    {
    case 0x90:
    {
        switch (address & 0xFFFFFF00)
        {
        case 0x00908000:
           index = (address & 0xFF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport5Mem.ControlRegs ,
                            devMemInfoPtr->Fport5Mem.ControlRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport5Mem.ControlRegs[index];
           break;
        case 0x0090C000:
           index = (address & 0xFF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport5Mem.TarPort2DevRegs ,
                            devMemInfoPtr->Fport5Mem.TarPort2DevRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport5Mem.TarPort2DevRegs[index];
           break;
        default:
           index = (address & 0xF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport5Mem.CounterRegs ,
                            devMemInfoPtr->Fport5Mem.CounterRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport5Mem.CounterRegs[index];
           break;
        }
    }
    break;
    case 0x98:
      index = (address & 0xFF)/0x4;
      CHECK_MEM_BOUNDS(devMemInfoPtr->Fport5Mem.HyperGlinkRegs ,
                       devMemInfoPtr->Fport5Mem.HyperGlinkRegsNum ,
                       index, memSize);
      regValPtr = &devMemInfoPtr->Fport5Mem.HyperGlinkRegs[index];
      break;
    case 0x9C:
      index = (address & 0xFFF)/0x4;
      CHECK_MEM_BOUNDS(devMemInfoPtr->Fport5Mem.SerdesRegs ,
                       devMemInfoPtr->Fport5Mem.SerdesRegsNum ,
                       index, memSize);
      regValPtr = &devMemInfoPtr->Fport5Mem.SerdesRegs[index];
      break;
    }

    return regValPtr;
}


/*******************************************************************************
*   smemCapXbarPort6Reg
*
* DESCRIPTION:
*       6th port capoeira Registers.
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
*
*******************************************************************************/
static GT_U32 *  smemCapXbarPort6Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                 * regValPtr=NULL;
    GT_U32                   index;
    GT_U32                   subAddr;

    devMemInfoPtr = (CAP_XBAR_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    subAddr = ((address & 0x00FF0000) >> 16) ;

    switch (subAddr)
    {
    case 0xA0:
    {
        switch (address & 0xFFFFFF00)
        {
        case 0x00A08000:
           index = (address & 0xFF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport6Mem.ControlRegs ,
                            devMemInfoPtr->Fport6Mem.ControlRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport6Mem.ControlRegs[index];
           break;
        case 0x00A0C000:
           index = (address & 0xFF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport6Mem.TarPort2DevRegs ,
                            devMemInfoPtr->Fport6Mem.TarPort2DevRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport6Mem.TarPort2DevRegs[index];
           break;
        default:
           index = (address & 0xF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport6Mem.CounterRegs ,
                            devMemInfoPtr->Fport6Mem.CounterRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport6Mem.CounterRegs[index];
           break;
        }
    }
    break;
    case 0xA8:
      index = (address & 0xFF)/0x4;
      CHECK_MEM_BOUNDS(devMemInfoPtr->Fport6Mem.HyperGlinkRegs ,
                       devMemInfoPtr->Fport6Mem.HyperGlinkRegsNum ,
                       index, memSize);
      regValPtr = &devMemInfoPtr->Fport6Mem.HyperGlinkRegs[index];
      break;
    case 0xAC:
      index = (address & 0xFFF)/0x4;
      CHECK_MEM_BOUNDS(devMemInfoPtr->Fport6Mem.SerdesRegs ,
                       devMemInfoPtr->Fport6Mem.SerdesRegsNum ,
                       index, memSize);
      regValPtr = &devMemInfoPtr->Fport6Mem.SerdesRegs[index];
      break;
    }
    return regValPtr;
}


/*******************************************************************************
*   smemCapXbarPort7Reg
*
* DESCRIPTION:
*       7th port capoeira Registers.
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
*
*******************************************************************************/
static GT_U32 *  smemCapXbarPort7Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                 * regValPtr=NULL;
    GT_U32                   index;
    GT_U32                   subAddr;

    devMemInfoPtr = (CAP_XBAR_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    subAddr = ((address & 0x00FF0000) >> 16) ;

    switch (subAddr)
    {
    case 0xB0:
    {
        switch (address & 0xFFFFFF00)
        {
        case 0x00B08000:
           index = (address & 0xFF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport7Mem.ControlRegs ,
                            devMemInfoPtr->Fport7Mem.ControlRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport7Mem.ControlRegs[index];
           break;
        case 0x00B0C000:
           index = (address & 0xFF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport7Mem.TarPort2DevRegs ,
                            devMemInfoPtr->Fport7Mem.TarPort2DevRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport7Mem.TarPort2DevRegs[index];
           break;
        default :
           index = (address & 0xF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport7Mem.CounterRegs ,
                            devMemInfoPtr->Fport7Mem.CounterRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport7Mem.CounterRegs[index];
           break;
        }
    }
    break;
    case 0xB8:
        index = (address & 0xFF)/0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->Fport7Mem.HyperGlinkRegs ,
                         devMemInfoPtr->Fport7Mem.HyperGlinkRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->Fport7Mem.HyperGlinkRegs[index];
        break;
    case 0xBC:
        index = (address & 0xFFF)/0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->Fport7Mem.SerdesRegs ,
                         devMemInfoPtr->Fport7Mem.SerdesRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->Fport7Mem.SerdesRegs[index];
        break;
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCapXbarPort8Reg
*
* DESCRIPTION:
*       8 port capoeira Registers.
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
*
*******************************************************************************/
static GT_U32 *  smemCapXbarPort8Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                 * regValPtr=NULL;
    GT_U32                   index;
    GT_U32                   subAddr;

    devMemInfoPtr = (CAP_XBAR_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    subAddr = ((address & 0x00FF0000) >> 16) ;

    switch (subAddr)
    {
    case 0x81:
    {
        switch (address & 0xFFFFFF00)
        {
        case 0x00810000:
           index = (address & 0xFF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport8Mem.ControlRegs ,
                            devMemInfoPtr->Fport8Mem.ControlRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport8Mem.ControlRegs[index];
           break;
        case 0x00814000:
           index = (address & 0xFF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport8Mem.TarPort2DevRegs ,
                            devMemInfoPtr->Fport8Mem.TarPort2DevRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport8Mem.TarPort2DevRegs[index];
           break;
        default :
           index = (address & 0xF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport8Mem.CounterRegs ,
                            devMemInfoPtr->Fport8Mem.CounterRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport8Mem.CounterRegs[index];
           break;
        }
    }
    break;
    case 0x89:
      index = (address & 0xFF)/0x4;
      CHECK_MEM_BOUNDS(devMemInfoPtr->Fport8Mem.HyperGlinkRegs ,
                       devMemInfoPtr->Fport8Mem.HyperGlinkRegsNum ,
                       index, memSize);
      regValPtr = &devMemInfoPtr->Fport8Mem.HyperGlinkRegs[index];
      break;
    case 0x8C:
      index = (address & 0xFFF)/0x4;
      CHECK_MEM_BOUNDS(devMemInfoPtr->Fport8Mem.SerdesRegs ,
                       devMemInfoPtr->Fport8Mem.SerdesRegsNum ,
                       index, memSize);
      regValPtr = &devMemInfoPtr->Fport8Mem.SerdesRegs[index];
      break;
    }

    return regValPtr;
}


/*******************************************************************************
*   smemCapXbarPort9Reg
*
* DESCRIPTION:
*       9 port capoeira Registers.
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
*
*******************************************************************************/
static GT_U32 *  smemCapXbarPort9Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                 * regValPtr=NULL;
    GT_U32                   index;
    GT_U32                   subAddr;

    devMemInfoPtr = (CAP_XBAR_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    subAddr = ((address & 0x00FF0000) >> 16) ;

    switch (subAddr)
    {
    case 0x91:
    {
        switch (address & 0xFFFFFF00)
        {
        case 0x00910000:
           index = (address & 0xFF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport9Mem.ControlRegs ,
                            devMemInfoPtr->Fport9Mem.ControlRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport9Mem.ControlRegs[index];
           break;
        case 0x00914000:
           index = (address & 0xFF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport9Mem.TarPort2DevRegs ,
                            devMemInfoPtr->Fport9Mem.TarPort2DevRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport9Mem.TarPort2DevRegs[index];
           break;
        default :
          index = (address & 0xF)/0x4;
          CHECK_MEM_BOUNDS(devMemInfoPtr->Fport9Mem.CounterRegs ,
                           devMemInfoPtr->Fport9Mem.CounterRegsNum ,
                           index, memSize);
          regValPtr = &devMemInfoPtr->Fport9Mem.CounterRegs[index];
          break;
        }
    }
    break;
    case 0x99:
      index = (address & 0xFF)/0x4;
      CHECK_MEM_BOUNDS(devMemInfoPtr->Fport9Mem.HyperGlinkRegs ,
                       devMemInfoPtr->Fport9Mem.HyperGlinkRegsNum ,
                       index, memSize);
      regValPtr = &devMemInfoPtr->Fport9Mem.HyperGlinkRegs[index];
    break;
    case 0x9C:
      index = (address & 0xFFF)/0x4;
      CHECK_MEM_BOUNDS(devMemInfoPtr->Fport9Mem.SerdesRegs ,
                       devMemInfoPtr->Fport9Mem.SerdesRegsNum ,
                       index, memSize);
      regValPtr = &devMemInfoPtr->Fport9Mem.SerdesRegs[index];
      break;
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCapXbarPort10Reg
*
* DESCRIPTION:
*       10 port capoeira Registers.
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
*
*******************************************************************************/
static GT_U32 *  smemCapXbarPort10Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                 * regValPtr=NULL;
    GT_U32                   index;
    GT_U32                   subAddr;

    devMemInfoPtr = (CAP_XBAR_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    subAddr = ((address & 0x00FF0000) >> 16) ;

    switch (subAddr)
    {
    case 0xA1:
    {
        switch (address & 0xFFFFFF00)
        {
        case 0x00A10000:
           index = (address & 0xFF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport10Mem.ControlRegs ,
                            devMemInfoPtr->Fport10Mem.ControlRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport10Mem.ControlRegs[index];
           break;
        case 0x00A14000:
           index = (address & 0xFF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport10Mem.TarPort2DevRegs ,
                            devMemInfoPtr->Fport10Mem.TarPort2DevRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport10Mem.TarPort2DevRegs[index];
           break;
        default:
           index = (address & 0xF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport10Mem.CounterRegs ,
                            devMemInfoPtr->Fport10Mem.CounterRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport10Mem.CounterRegs[index];
           break;
        }
    }
    break;
    case 0xA9:
      index = (address & 0xFF)/0x4;
      CHECK_MEM_BOUNDS(devMemInfoPtr->Fport10Mem.HyperGlinkRegs ,
                       devMemInfoPtr->Fport10Mem.HyperGlinkRegsNum ,
                       index, memSize);
      regValPtr = &devMemInfoPtr->Fport10Mem.HyperGlinkRegs[index];
      break;
    case 0xAC:
      index = (address & 0xFFF)/0x4;
      CHECK_MEM_BOUNDS(devMemInfoPtr->Fport10Mem.SerdesRegs ,
                       devMemInfoPtr->Fport10Mem.SerdesRegsNum ,
                       index, memSize);
      regValPtr = &devMemInfoPtr->Fport10Mem.SerdesRegs[index];
      break;
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCapXbarPort11Reg
*
* DESCRIPTION:
*       first port capoeira Registers.
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
*
*******************************************************************************/
static GT_U32 *  smemCapXbarPort11Reg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                 * regValPtr=NULL;
    GT_U32                   index;
    GT_U32                   subAddr;

    devMemInfoPtr = (CAP_XBAR_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    subAddr = ((address & 0x00FF0000) >> 16) ;

    switch (subAddr)
    {
    case 0xB1:
    {
        switch (address & 0xFFFFFF00)
        {
        case 0x00B10000:
           index = (address & 0xFF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport11Mem.ControlRegs ,
                            devMemInfoPtr->Fport11Mem.ControlRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport11Mem.ControlRegs[index];
           break;
        case 0x00B14000:
           index = (address & 0xFF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport11Mem.TarPort2DevRegs ,
                            devMemInfoPtr->Fport11Mem.TarPort2DevRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport11Mem.TarPort2DevRegs[index];
           break;
        default:
           index = (address & 0xF)/0x4;
           CHECK_MEM_BOUNDS(devMemInfoPtr->Fport11Mem.CounterRegs ,
                            devMemInfoPtr->Fport11Mem.CounterRegsNum ,
                            index, memSize);
           regValPtr = &devMemInfoPtr->Fport11Mem.CounterRegs[index];
           break;
        }
    }
    break;
    case 0xB9:
        index = (address & 0xFF)/0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->Fport11Mem.HyperGlinkRegs ,
                         devMemInfoPtr->Fport11Mem.HyperGlinkRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->Fport11Mem.HyperGlinkRegs[index];
        break;
    case 0xBC:
        index = (address & 0xFFF)/0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->Fport11Mem.SerdesRegs ,
                         devMemInfoPtr->Fport11Mem.SerdesRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->Fport11Mem.SerdesRegs[index];
        break;
    }

    return regValPtr;
}


/*******************************************************************************
*   smemCapXbarPGReg
*
* DESCRIPTION:
*       first port capoeira Registers.
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
*
*******************************************************************************/
static GT_U32 *  smemCapXbarPGReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32             * regValPtr=NULL;
    GT_U32             index;
    GT_U32             subAddr;

    devMemInfoPtr = (CAP_XBAR_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    subAddr = ((address & 0x00FF0000) >> 16) ;

    switch (subAddr)
    {
    case 0x86:
          index = (address & 0xFF)/0x4;
          CHECK_MEM_BOUNDS(devMemInfoPtr->PGMem.PG0Regs  ,
                           devMemInfoPtr->PGMem.PGRegsNum ,
                           index, memSize);
          regValPtr = &devMemInfoPtr->PGMem.PG0Regs[index];
          break;
    case 0x96:
        index = (address & 0xFF)/0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->PGMem.PG1Regs ,
                         devMemInfoPtr->PGMem.PGRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->PGMem.PG1Regs[index];
        break;
    case 0xA6:
        index = (address & 0xFFF)/0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->PGMem.PG2Regs ,
                         devMemInfoPtr->PGMem.PGRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->PGMem.PG2Regs[index];
        break;
    case 0xB6:
        index = (address & 0xFFF)/0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->PGMem.PG3Regs ,
                         devMemInfoPtr->PGMem.PGRegsNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->PGMem.PG3Regs[index];
        break;
    }

    return regValPtr;
}

/*******************************************************************************
*   smemCapXbarAllocSpecMemory
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
static void smemCapXbarAllocSpecMemory(
    INOUT CAP_XBAR_DEV_MEM_INFO  * devMemInfoPtr
)
{
    /* port 0 register memory allocation */
    devMemInfoPtr->Fport0Mem.ControlRegsNum = SMEM_CAP_CONTROL_REGS_NUM_CNS;
    devMemInfoPtr->Fport0Mem.ControlRegs    =
            calloc(SMEM_CAP_CONTROL_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport0Mem.ControlRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           ControlRegs error\n");
    }
    devMemInfoPtr->Fport0Mem.HyperGlinkRegsNum = SMEM_CAP_HYPERGLINK_REGS_NUM_CNS;
    devMemInfoPtr->Fport0Mem.HyperGlinkRegs    =
            calloc(SMEM_CAP_HYPERGLINK_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport0Mem.HyperGlinkRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           HyperGlinkRegs error\n");
    }
    devMemInfoPtr->Fport0Mem.MCGroupRegsNum = SMEM_CAP_MCGROUP_REGS_NUM_CNS;
    devMemInfoPtr->Fport0Mem.MCGroupRegs    =
            calloc(SMEM_CAP_MCGROUP_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport0Mem.MCGroupRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           MCGroupRegs error\n");
    }
    devMemInfoPtr->Fport0Mem.SerdesRegsNum = SMEM_CAP_SERDES_REGS_NUM_CNS;
    devMemInfoPtr->Fport0Mem.SerdesRegs    =
            calloc(SMEM_CAP_SERDES_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport0Mem.SerdesRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           SerdesRegs error\n");
    }
    devMemInfoPtr->Fport0Mem.TarPort2DevRegsNum = SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS;
    devMemInfoPtr->Fport0Mem.TarPort2DevRegs    =
            calloc(SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport0Mem.TarPort2DevRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           TarPort2DevRegs error\n");
    }
    devMemInfoPtr->Fport0Mem.CounterRegsNum = SMEM_CAP_COUNTER_REGS_NUM_CNS;
    devMemInfoPtr->Fport0Mem.CounterRegs    =
            calloc(SMEM_CAP_COUNTER_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport0Mem.CounterRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           CounterRegs error\n");
    }

    /* port 1 register memory allocation */
    devMemInfoPtr->Fport1Mem.ControlRegsNum = SMEM_CAP_CONTROL_REGS_NUM_CNS;
    devMemInfoPtr->Fport1Mem.ControlRegs    =
            calloc(SMEM_CAP_CONTROL_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport1Mem.ControlRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           ControlRegs error\n");
    }
    devMemInfoPtr->Fport1Mem.HyperGlinkRegsNum = SMEM_CAP_HYPERGLINK_REGS_NUM_CNS;
    devMemInfoPtr->Fport1Mem.HyperGlinkRegs    =
            calloc(SMEM_CAP_HYPERGLINK_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport1Mem.HyperGlinkRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           HyperGlinkRegs error\n");
    }
    devMemInfoPtr->Fport1Mem.MCGroupRegsNum = SMEM_CAP_MCGROUP_REGS_NUM_CNS;
    devMemInfoPtr->Fport1Mem.MCGroupRegs    =
            calloc(SMEM_CAP_MCGROUP_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport1Mem.MCGroupRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           MCGroupRegs error\n");
    }
    devMemInfoPtr->Fport1Mem.SerdesRegsNum = SMEM_CAP_SERDES_REGS_NUM_CNS;
    devMemInfoPtr->Fport1Mem.SerdesRegs    =
            calloc(SMEM_CAP_SERDES_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport1Mem.SerdesRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           SerdesRegs error\n");
    }
    devMemInfoPtr->Fport1Mem.TarPort2DevRegsNum = SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS;
    devMemInfoPtr->Fport1Mem.TarPort2DevRegs    =
            calloc(SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport1Mem.TarPort2DevRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           TarPort2DevRegs error\n");
    }
    devMemInfoPtr->Fport1Mem.CounterRegsNum = SMEM_CAP_COUNTER_REGS_NUM_CNS;
    devMemInfoPtr->Fport1Mem.CounterRegs    =
            calloc(SMEM_CAP_COUNTER_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport1Mem.CounterRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           CounterRegs error\n");
    }

    /* port 2 register memory allocation */
    devMemInfoPtr->Fport2Mem.ControlRegsNum = SMEM_CAP_CONTROL_REGS_NUM_CNS;
    devMemInfoPtr->Fport2Mem.ControlRegs    =
            calloc(SMEM_CAP_CONTROL_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport2Mem.ControlRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           ControlRegs error\n");
    }
    devMemInfoPtr->Fport2Mem.HyperGlinkRegsNum = SMEM_CAP_HYPERGLINK_REGS_NUM_CNS;
    devMemInfoPtr->Fport2Mem.HyperGlinkRegs    =
            calloc(SMEM_CAP_HYPERGLINK_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport2Mem.HyperGlinkRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           HyperGlinkRegs error\n");
    }
    devMemInfoPtr->Fport2Mem.MCGroupRegsNum = SMEM_CAP_MCGROUP_REGS_NUM_CNS;
    devMemInfoPtr->Fport2Mem.MCGroupRegs    =
            calloc(SMEM_CAP_MCGROUP_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport2Mem.MCGroupRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           MCGroupRegs error\n");
    }
    devMemInfoPtr->Fport2Mem.SerdesRegsNum = SMEM_CAP_SERDES_REGS_NUM_CNS;
    devMemInfoPtr->Fport2Mem.SerdesRegs    =
            calloc(SMEM_CAP_SERDES_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport2Mem.SerdesRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           SerdesRegs error\n");
    }
    devMemInfoPtr->Fport2Mem.TarPort2DevRegsNum = SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS;
    devMemInfoPtr->Fport2Mem.TarPort2DevRegs    =
            calloc(SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport2Mem.TarPort2DevRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           TarPort2DevRegs error\n");
    }
    devMemInfoPtr->Fport2Mem.CounterRegsNum = SMEM_CAP_COUNTER_REGS_NUM_CNS;
    devMemInfoPtr->Fport2Mem.CounterRegs    =
            calloc(SMEM_CAP_COUNTER_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport2Mem.CounterRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           CounterRegs error\n");
    }

    /* port 3 register memory allocation */
    devMemInfoPtr->Fport3Mem.ControlRegsNum = SMEM_CAP_CONTROL_REGS_NUM_CNS;
    devMemInfoPtr->Fport3Mem.ControlRegs    =
            calloc(SMEM_CAP_CONTROL_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport3Mem.ControlRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           ControlRegs error\n");
    }
    devMemInfoPtr->Fport3Mem.HyperGlinkRegsNum = SMEM_CAP_HYPERGLINK_REGS_NUM_CNS;
    devMemInfoPtr->Fport3Mem.HyperGlinkRegs    =
            calloc(SMEM_CAP_HYPERGLINK_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport3Mem.HyperGlinkRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           HyperGlinkRegs error\n");
    }
    devMemInfoPtr->Fport3Mem.MCGroupRegsNum = SMEM_CAP_MCGROUP_REGS_NUM_CNS;
    devMemInfoPtr->Fport3Mem.MCGroupRegs    =
            calloc(SMEM_CAP_MCGROUP_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport3Mem.MCGroupRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           MCGroupRegs error\n");
    }
    devMemInfoPtr->Fport3Mem.SerdesRegsNum = SMEM_CAP_SERDES_REGS_NUM_CNS;
    devMemInfoPtr->Fport3Mem.SerdesRegs    =
            calloc(SMEM_CAP_SERDES_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport3Mem.SerdesRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           SerdesRegs error\n");
    }
    devMemInfoPtr->Fport3Mem.TarPort2DevRegsNum = SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS;
    devMemInfoPtr->Fport3Mem.TarPort2DevRegs    =
            calloc(SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport3Mem.TarPort2DevRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           TarPort2DevRegs error\n");
    }
    devMemInfoPtr->Fport3Mem.CounterRegsNum = SMEM_CAP_COUNTER_REGS_NUM_CNS;
    devMemInfoPtr->Fport3Mem.CounterRegs    =
            calloc(SMEM_CAP_COUNTER_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport3Mem.CounterRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           CounterRegs error\n");
    }

    /* port 4 register memory allocation */
    devMemInfoPtr->Fport4Mem.ControlRegsNum = SMEM_CAP_CONTROL_REGS_NUM_CNS;
    devMemInfoPtr->Fport4Mem.ControlRegs    =
            calloc(SMEM_CAP_CONTROL_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport4Mem.ControlRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           ControlRegs error\n");
    }
    devMemInfoPtr->Fport4Mem.HyperGlinkRegsNum = SMEM_CAP_HYPERGLINK_REGS_NUM_CNS;
    devMemInfoPtr->Fport4Mem.HyperGlinkRegs    =
            calloc(SMEM_CAP_HYPERGLINK_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport4Mem.HyperGlinkRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           HyperGlinkRegs error\n");
    }
    devMemInfoPtr->Fport4Mem.SerdesRegsNum = SMEM_CAP_SERDES_REGS_NUM_CNS;
    devMemInfoPtr->Fport4Mem.SerdesRegs    =
            calloc(SMEM_CAP_SERDES_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport4Mem.SerdesRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           SerdesRegs error\n");
    }
    devMemInfoPtr->Fport4Mem.TarPort2DevRegsNum = SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS;
    devMemInfoPtr->Fport4Mem.TarPort2DevRegs    =
            calloc(SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport4Mem.TarPort2DevRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           TarPort2DevRegs error\n");
    }
    devMemInfoPtr->Fport4Mem.CounterRegsNum = SMEM_CAP_COUNTER_REGS_NUM_CNS;
    devMemInfoPtr->Fport4Mem.CounterRegs    =
            calloc(SMEM_CAP_COUNTER_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport4Mem.CounterRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           CounterRegs error\n");
    }

    /* port 5 register memory allocation */
    devMemInfoPtr->Fport5Mem.ControlRegsNum = SMEM_CAP_CONTROL_REGS_NUM_CNS;
    devMemInfoPtr->Fport5Mem.ControlRegs    =
            calloc(SMEM_CAP_CONTROL_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport5Mem.ControlRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           ControlRegs error\n");
    }
    devMemInfoPtr->Fport5Mem.HyperGlinkRegsNum = SMEM_CAP_HYPERGLINK_REGS_NUM_CNS;
    devMemInfoPtr->Fport5Mem.HyperGlinkRegs    =
            calloc(SMEM_CAP_HYPERGLINK_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport5Mem.HyperGlinkRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           HyperGlinkRegs error\n");
    }
    devMemInfoPtr->Fport5Mem.SerdesRegsNum = SMEM_CAP_SERDES_REGS_NUM_CNS;
    devMemInfoPtr->Fport5Mem.SerdesRegs    =
            calloc(SMEM_CAP_SERDES_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport5Mem.SerdesRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           SerdesRegs error\n");
    }
    devMemInfoPtr->Fport5Mem.TarPort2DevRegsNum = SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS;
    devMemInfoPtr->Fport5Mem.TarPort2DevRegs    =
            calloc(SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport5Mem.TarPort2DevRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           TarPort2DevRegs error\n");
    }
    devMemInfoPtr->Fport5Mem.CounterRegsNum = SMEM_CAP_COUNTER_REGS_NUM_CNS;
    devMemInfoPtr->Fport5Mem.CounterRegs    =
            calloc(SMEM_CAP_COUNTER_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport5Mem.CounterRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           CounterRegs error\n");
    }

    /* port 6 register memory allocation */
    devMemInfoPtr->Fport6Mem.ControlRegsNum = SMEM_CAP_CONTROL_REGS_NUM_CNS;
    devMemInfoPtr->Fport6Mem.ControlRegs    =
            calloc(SMEM_CAP_CONTROL_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport6Mem.ControlRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           ControlRegs error\n");
    }
    devMemInfoPtr->Fport6Mem.HyperGlinkRegsNum = SMEM_CAP_HYPERGLINK_REGS_NUM_CNS;
    devMemInfoPtr->Fport6Mem.HyperGlinkRegs    =
            calloc(SMEM_CAP_HYPERGLINK_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport6Mem.HyperGlinkRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           HyperGlinkRegs error\n");
    }
    devMemInfoPtr->Fport6Mem.SerdesRegsNum = SMEM_CAP_SERDES_REGS_NUM_CNS;
    devMemInfoPtr->Fport6Mem.SerdesRegs    =
            calloc(SMEM_CAP_SERDES_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport6Mem.SerdesRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           SerdesRegs error\n");
    }
    devMemInfoPtr->Fport6Mem.TarPort2DevRegsNum = SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS;
    devMemInfoPtr->Fport6Mem.TarPort2DevRegs    =
            calloc(SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport6Mem.TarPort2DevRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           TarPort2DevRegs error\n");
    }
    devMemInfoPtr->Fport6Mem.CounterRegsNum = SMEM_CAP_COUNTER_REGS_NUM_CNS;
    devMemInfoPtr->Fport6Mem.CounterRegs    =
            calloc(SMEM_CAP_COUNTER_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport6Mem.CounterRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           CounterRegs error\n");
    }

    /* port 7 register memory allocation */
    devMemInfoPtr->Fport7Mem.ControlRegsNum = SMEM_CAP_CONTROL_REGS_NUM_CNS;
    devMemInfoPtr->Fport7Mem.ControlRegs    =
            calloc(SMEM_CAP_CONTROL_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport7Mem.ControlRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           ControlRegs error\n");
    }
    devMemInfoPtr->Fport7Mem.HyperGlinkRegsNum=SMEM_CAP_HYPERGLINK_REGS_NUM_CNS;
    devMemInfoPtr->Fport7Mem.HyperGlinkRegs    =
            calloc(SMEM_CAP_HYPERGLINK_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport7Mem.HyperGlinkRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           HyperGlinkRegs error\n");
    }
    devMemInfoPtr->Fport7Mem.SerdesRegsNum = SMEM_CAP_SERDES_REGS_NUM_CNS;
    devMemInfoPtr->Fport7Mem.SerdesRegs    =
            calloc(SMEM_CAP_SERDES_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport7Mem.SerdesRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           SerdesRegs error\n");
    }
    devMemInfoPtr->Fport7Mem.TarPort2DevRegsNum = SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS;
    devMemInfoPtr->Fport7Mem.TarPort2DevRegs    =
            calloc(SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport7Mem.TarPort2DevRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           TarPort2DevRegs error\n");
    }
    devMemInfoPtr->Fport7Mem.CounterRegsNum = SMEM_CAP_COUNTER_REGS_NUM_CNS;
    devMemInfoPtr->Fport7Mem.CounterRegs    =
            calloc(SMEM_CAP_COUNTER_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport7Mem.CounterRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           CounterRegs error\n");
    }

    /* port 8 register memory allocation */
    devMemInfoPtr->Fport8Mem.ControlRegsNum = SMEM_CAP_CONTROL_REGS_NUM_CNS;
    devMemInfoPtr->Fport8Mem.ControlRegs    =
            calloc(SMEM_CAP_CONTROL_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport8Mem.ControlRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           ControlRegs error\n");
    }
    devMemInfoPtr->Fport8Mem.HyperGlinkRegsNum = SMEM_CAP_HYPERGLINK_REGS_NUM_CNS;
    devMemInfoPtr->Fport8Mem.HyperGlinkRegs    =
            calloc(SMEM_CAP_HYPERGLINK_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport8Mem.HyperGlinkRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           HyperGlinkRegs error\n");
    }
    devMemInfoPtr->Fport8Mem.SerdesRegsNum = SMEM_CAP_SERDES_REGS_NUM_CNS;
    devMemInfoPtr->Fport8Mem.SerdesRegs    =
            calloc(SMEM_CAP_SERDES_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport8Mem.SerdesRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           SerdesRegs error\n");
    }
    devMemInfoPtr->Fport8Mem.TarPort2DevRegsNum = SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS;
    devMemInfoPtr->Fport8Mem.TarPort2DevRegs    =
            calloc(SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport8Mem.TarPort2DevRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           TarPort2DevRegs error\n");
    }
    devMemInfoPtr->Fport8Mem.CounterRegsNum = SMEM_CAP_COUNTER_REGS_NUM_CNS;
    devMemInfoPtr->Fport8Mem.CounterRegs    =
            calloc(SMEM_CAP_COUNTER_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport8Mem.CounterRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           CounterRegs error\n");
    }

    /* port 9 register memory allocation */
    devMemInfoPtr->Fport9Mem.ControlRegsNum = SMEM_CAP_CONTROL_REGS_NUM_CNS;
    devMemInfoPtr->Fport9Mem.ControlRegs    =
            calloc(SMEM_CAP_CONTROL_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport9Mem.ControlRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           ControlRegs error\n");
    }
    devMemInfoPtr->Fport9Mem.HyperGlinkRegsNum = SMEM_CAP_HYPERGLINK_REGS_NUM_CNS;
    devMemInfoPtr->Fport9Mem.HyperGlinkRegs    =
            calloc(SMEM_CAP_HYPERGLINK_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport9Mem.HyperGlinkRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           HyperGlinkRegs error\n");
    }
    devMemInfoPtr->Fport9Mem.SerdesRegsNum = SMEM_CAP_SERDES_REGS_NUM_CNS;
    devMemInfoPtr->Fport9Mem.SerdesRegs    =
            calloc(SMEM_CAP_SERDES_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport9Mem.SerdesRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           SerdesRegs error\n");
    }
    devMemInfoPtr->Fport9Mem.TarPort2DevRegsNum = SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS;
    devMemInfoPtr->Fport9Mem.TarPort2DevRegs    =
            calloc(SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport9Mem.TarPort2DevRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           TarPort2DevRegs error\n");
    }
    devMemInfoPtr->Fport9Mem.CounterRegsNum = SMEM_CAP_COUNTER_REGS_NUM_CNS;
    devMemInfoPtr->Fport9Mem.CounterRegs    =
            calloc(SMEM_CAP_COUNTER_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport9Mem.CounterRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           CounterRegs error\n");
    }

    /* port 10 register memory allocation */
    devMemInfoPtr->Fport10Mem.ControlRegsNum = SMEM_CAP_CONTROL_REGS_NUM_CNS;
    devMemInfoPtr->Fport10Mem.ControlRegs    =
            calloc(SMEM_CAP_CONTROL_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport10Mem.ControlRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           ControlRegs error\n");
    }
    devMemInfoPtr->Fport10Mem.HyperGlinkRegsNum = SMEM_CAP_HYPERGLINK_REGS_NUM_CNS;
    devMemInfoPtr->Fport10Mem.HyperGlinkRegs    =
            calloc(SMEM_CAP_HYPERGLINK_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport10Mem.HyperGlinkRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           HyperGlinkRegs error\n");
    }
    devMemInfoPtr->Fport10Mem.SerdesRegsNum = SMEM_CAP_SERDES_REGS_NUM_CNS;
    devMemInfoPtr->Fport10Mem.SerdesRegs    =
            calloc(SMEM_CAP_SERDES_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport10Mem.SerdesRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           SerdesRegs error\n");
    }
    devMemInfoPtr->Fport10Mem.TarPort2DevRegsNum = SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS;
    devMemInfoPtr->Fport10Mem.TarPort2DevRegs    =
            calloc(SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport10Mem.TarPort2DevRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           TarPort2DevRegs error\n");
    }
    devMemInfoPtr->Fport10Mem.CounterRegsNum = SMEM_CAP_COUNTER_REGS_NUM_CNS;
    devMemInfoPtr->Fport10Mem.CounterRegs    =
            calloc(SMEM_CAP_COUNTER_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport10Mem.CounterRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           CounterRegs error\n");
    }

    /* port 11 register memory allocation */
    devMemInfoPtr->Fport11Mem.ControlRegsNum = SMEM_CAP_CONTROL_REGS_NUM_CNS;
    devMemInfoPtr->Fport11Mem.ControlRegs    =
            calloc(SMEM_CAP_CONTROL_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport11Mem.ControlRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           ControlRegs error\n");
    }
    devMemInfoPtr->Fport11Mem.HyperGlinkRegsNum = SMEM_CAP_HYPERGLINK_REGS_NUM_CNS;
    devMemInfoPtr->Fport11Mem.HyperGlinkRegs    =
            calloc(SMEM_CAP_HYPERGLINK_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport11Mem.HyperGlinkRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           HyperGlinkRegs error\n");
    }
    devMemInfoPtr->Fport11Mem.SerdesRegsNum = SMEM_CAP_SERDES_REGS_NUM_CNS;
    devMemInfoPtr->Fport11Mem.SerdesRegs    =
            calloc(SMEM_CAP_SERDES_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport11Mem.SerdesRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           SerdesRegs error\n");
    }
    devMemInfoPtr->Fport11Mem.TarPort2DevRegsNum = SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS;
    devMemInfoPtr->Fport11Mem.TarPort2DevRegs    =
            calloc(SMEM_CAP_TARPORT2DEV_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport11Mem.TarPort2DevRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           TarPort2DevRegs error\n");
    }
    devMemInfoPtr->Fport11Mem.CounterRegsNum = SMEM_CAP_COUNTER_REGS_NUM_CNS;
    devMemInfoPtr->Fport11Mem.CounterRegs    =
            calloc(SMEM_CAP_COUNTER_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->Fport11Mem.CounterRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           CounterRegs error\n");
    }

    /* PG register memory allocation */
    devMemInfoPtr->PGMem.PGRegsNum = SMEM_CAP_PG_REGS_NUM_CNS;
    devMemInfoPtr->PGMem.PG0Regs =
            calloc(SMEM_CAP_PG_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->PGMem.PG0Regs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           PG error\n");
    }
    devMemInfoPtr->PGMem.PG1Regs =
            calloc(SMEM_CAP_PG_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->PGMem.PG1Regs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           PG error\n");
    }
    devMemInfoPtr->PGMem.PG2Regs =
            calloc(SMEM_CAP_PG_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->PGMem.PG2Regs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           PG error\n");
    }
    devMemInfoPtr->PGMem.PG3Regs =
            calloc(SMEM_CAP_PG_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->PGMem.PG3Regs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           PG error\n");
    }

    /* PG register memory allocation */
    devMemInfoPtr->GlobalMem.globRegsNum   = SMEM_CAP_GLOB_REGS_NUM_CNS;
    devMemInfoPtr->GlobalMem.globRegs =
            calloc(SMEM_CAP_GLOB_REGS_NUM_CNS, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->GlobalMem.globRegs == 0)
    {
        skernelFatalError("smemCapXbarAllocSpecMemory: allocation \
                           global error\n");
    }

    smemCapXbarIniIndexArr();
}

/*******************************************************************************
*   smemCapXbarIniIndexArr
*
* DESCRIPTION:
*       Allocate address type number for specific memories.
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
static void smemCapXbarIniIndexArr(void)
{
    /* pg - function number 1 handles this registers*/
    smemXbarFuncsIndex[0x860] = 1;
    smemXbarFuncsIndex[0x960] = 1;
    smemXbarFuncsIndex[0xA60] = 1;
    smemXbarFuncsIndex[0xB60] = 1;

    /* port 0 - function number 2 handles this port */
    smemXbarFuncsIndex[0x800] = 2;
    smemXbarFuncsIndex[0x880] = 2;
    smemXbarFuncsIndex[0x840] = 2;
    smemXbarFuncsIndex[0x841] = 2;
    smemXbarFuncsIndex[0x842] = 2;
    smemXbarFuncsIndex[0x843] = 2;
    smemXbarFuncsIndex[0x8C0] = 2;
    smemXbarFuncsIndex[0x804] = 2;
    smemXbarFuncsIndex[0x802] = 2;

    /* port 1 - function number 3 handles this port */
    smemXbarFuncsIndex[0x900] = 3;
    smemXbarFuncsIndex[0x980] = 3;
    smemXbarFuncsIndex[0x940] = 3;
    smemXbarFuncsIndex[0x941] = 3;
    smemXbarFuncsIndex[0x942] = 3;
    smemXbarFuncsIndex[0x943] = 3;
    smemXbarFuncsIndex[0x9C0] = 3;
    smemXbarFuncsIndex[0x904] = 3;
    smemXbarFuncsIndex[0x902] = 3;
    smemXbarFuncsIndex[0x9C0] = 3;
    smemXbarFuncsIndex[0x904] = 3;
    smemXbarFuncsIndex[0x902] = 3;

    /* port 2 - function number 4 handles this port */
    smemXbarFuncsIndex[0xA00] = 4;
    smemXbarFuncsIndex[0xA80] = 4;
    smemXbarFuncsIndex[0xA40] = 4;
    smemXbarFuncsIndex[0xA41] = 4;
    smemXbarFuncsIndex[0xA42] = 4;
    smemXbarFuncsIndex[0xA43] = 4;
    smemXbarFuncsIndex[0xAC0] = 4;
    smemXbarFuncsIndex[0xA04] = 4;
    smemXbarFuncsIndex[0xA02] = 4;
    smemXbarFuncsIndex[0xA04] = 4;
    smemXbarFuncsIndex[0xA02] = 4;

    /* port 3 - function number 5 handles this port */
    smemXbarFuncsIndex[0xB00] = 5;
    smemXbarFuncsIndex[0xB80] = 5;
    smemXbarFuncsIndex[0xB40] = 5;
    smemXbarFuncsIndex[0xB41] = 5;
    smemXbarFuncsIndex[0xB42] = 5;
    smemXbarFuncsIndex[0xB43] = 5;
    smemXbarFuncsIndex[0xBC0] = 5;
    smemXbarFuncsIndex[0xB04] = 5;
    smemXbarFuncsIndex[0xB02] = 5;
    smemXbarFuncsIndex[0xB04] = 5;
    smemXbarFuncsIndex[0xB02] = 5;

    /* port 4 - function number 6 handles this port */
    smemXbarFuncsIndex[0x808] = 6;
    smemXbarFuncsIndex[0x888] = 6;
    smemXbarFuncsIndex[0x8C2] = 6;
    smemXbarFuncsIndex[0x80C] = 6;
    smemXbarFuncsIndex[0x80A] = 6;

    /* port 5 - function number 7 handles this port */
    smemXbarFuncsIndex[0x908] = 7;
    smemXbarFuncsIndex[0x988] = 7;
    smemXbarFuncsIndex[0x9C2] = 7;
    smemXbarFuncsIndex[0x90C] = 7;
    smemXbarFuncsIndex[0x90A] = 7;

     /* port 6 - function number 8 handles this port */
    smemXbarFuncsIndex[0xA08] = 8;
    smemXbarFuncsIndex[0xA88] = 8;
    smemXbarFuncsIndex[0xAC2] = 8;
    smemXbarFuncsIndex[0xA0C] = 8;
    smemXbarFuncsIndex[0xA0A] = 8;

     /* port 7 - function number 9 handles this port */
    smemXbarFuncsIndex[0xB08] = 9;
    smemXbarFuncsIndex[0xB88] = 9;
    smemXbarFuncsIndex[0xBC2] = 9;
    smemXbarFuncsIndex[0xB0C] = 9;
    smemXbarFuncsIndex[0xB0A] = 9;

     /* port 8 - function number 10 handles this port */
    smemXbarFuncsIndex[0x810] = 10;
    smemXbarFuncsIndex[0x890] = 10;
    smemXbarFuncsIndex[0x8C4] = 10;
    smemXbarFuncsIndex[0x814] = 10;
    smemXbarFuncsIndex[0x812] = 10;

     /* port 9 - function number 11 handles this port */
    smemXbarFuncsIndex[0x910] = 11;
    smemXbarFuncsIndex[0x990] = 11;
    smemXbarFuncsIndex[0x9C4] = 11;
    smemXbarFuncsIndex[0x914] = 11;
    smemXbarFuncsIndex[0x912] = 11;

     /* port 10 - function number 12 handles this port */
    smemXbarFuncsIndex[0xA10] = 12;
    smemXbarFuncsIndex[0xA90] = 12;
    smemXbarFuncsIndex[0xAC4] = 12;
    smemXbarFuncsIndex[0xA14] = 12;
    smemXbarFuncsIndex[0xA12] = 12;

     /* port 11 - function number 13 handles this port */
    smemXbarFuncsIndex[0xB10] = 13;
    smemXbarFuncsIndex[0xB90] = 13;
    smemXbarFuncsIndex[0xBC4] = 13;
    smemXbarFuncsIndex[0xB14] = 13;
    smemXbarFuncsIndex[0xB12] = 13;
}

/*******************************************************************************
*  smemCapXbarActiveWritePingMessage
*
* DESCRIPTION:
*      Definition of the Active register read function.
*      cpu tries to write and send pcs ping message
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter -
*                      global interrupt bit number.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS: when application writes to
*           0x00880018 0x00980018 0x00A80018 0x00B80018
*           0x00888018 0x00988018 0x00A88018 0x00B88018
*           0x00890018 0x00990018 0x00A90018 0x00B90018
*           the function is invoked. Local CPU sends new PCS ping to remote CPU.
*******************************************************************************/
static void smemCapXbarActiveWritePingMessage(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32   address,
    IN GT_U32   memSize,
    IN GT_U32 * memPtr,
    IN GT_UINTPTR param,
    IN GT_U32 * inMemPtr
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
    /*  BYTE 0 : SMAIN_SRC_TYPE_CPU_E (=2)                                */
    /*  BYTE 1 : SMAIN_CPU_PCSPING_MSG_E (=3)                             */
    /*  BYTE 2 : Target Fport                                             */
    /*  BYTE 3 : Message length ( in words !) , always 1                  */
    /*  BYTE4 ... : Data                                                  */
    /**********************************************************************/
    if (*memPtr & SMEM_CAP_PCSPINGTRIG_MASK_CNS)
    {
        /* send message to SKernel task to process it. */
        /* get buffer */
        bufferId = sbufAlloc(deviceObjPtr->bufPool,
                             SMEM_CAP_PCSPING_MSG_SIZE * sizeof(GT_U32));
        if (bufferId == NULL)
        {
          printf(" smemCapXbarActiveWritePingMessage:no buffers for PCS ping msg\n");
          return;
        }
        /* get actual data pointer */
        sbufDataGet(bufferId, &dataPtr, &dataSize);

        /* save the data that the message contains */
        *dataPtr =  SMAIN_SRC_TYPE_CPU_E;   /*1st byte */
        dataPtr++;
        *dataPtr =  SMAIN_CPU_PCSPING_MSG_E;  /*2nd byte */
        dataPtr++;

        /* find the target fports to send the pcs ping . (0x00880018,=fport 0... */
        /* always saves it as a bitmap , the same as the cpu mail box */
        for (trgInx = 0;trgInx <= deviceObjPtr->portsNumber ; trgInx++)
        {
          if  (address == (GT_U32)SMEM_CAP_PING_CELL_TX(trgInx))
            break;
        }
        trgFport = ( 1 << trgInx); /* save the target port as a bmp , like cpu mb*/
        memcpy(dataPtr , &trgFport , sizeof(GT_U8) ); /*3rd byte */
        dataPtr++;

        /* set the length of the message in words!,always one in ping message  */
        *dataPtr = 1;
        dataPtr++;

        /* mask(0x00880018...) with bits[0:23] , because they are the *
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
*  smemCapXbarActiveSendCellMessage
*
* DESCRIPTION:
*      Definition of the Active register read function.
*      cpu tries to write and send pcs ping message
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter -
*                      global interrupt bit number.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS: when application writes to
*           0x00200000
*           the function is invoked. Local CPU sends new MB msg to remote CPU.
*******************************************************************************/
static void smemCapXbarActiveSendCellMessage(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32   address,
    IN GT_U32   memSize,
    IN GT_U32 * memPtr,
    IN GT_UINTPTR param,
    IN GT_U32 * inMemPtr
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
    if (*memPtr & SMEM_CAP_CPUTRIG_MASK_CNS)
    {
        /* send message to SKernel task to process it. */
        /* get buffer */
        bufferId = sbufAlloc(deviceObjPtr->bufPool,
                             SMEM_CAP_CPU_MAILBOX_MSG_SIZE * sizeof(GT_U32));
        if (bufferId == NULL)
        {
          printf(" smemCapXbarActiveSendCellMessage:" \
                 " no buffers for the mailbox message \n");
          return;
        }
        /* get actual data pointer */
        sbufDataGet(bufferId, &dataPtr, &dataSize);

        /* save the data that the message is pcs ping one */
        *dataPtr =  SMAIN_SRC_TYPE_CPU_E;   /*1st byte */
        dataPtr++;
        *dataPtr =  SMAIN_CPU_MAILBOX_MSG_E;  /*2nd byte */
        dataPtr++;

        /* read register of 0x00000084 which saves the target fports ,            */
        smemRegFldGet(deviceObjPtr,SMEM_CAP_CPUMAILBOX_TAR_PORT_REG,0,11,&trgFportBMP);
        trgFportBMP = (trgFportBMP & 0x7FF) ;/* set the target fports bmp            */
        memcpy(dataPtr , &trgFportBMP , sizeof(GT_U32));
        dataPtr++;

        /* find the celldata length from the register of 0x00000080 , bits[5:2] */
        /* the register is updated in capacpumail.c::xbarCpuMailSend(PSS)       */
        dataLen =  (((*memPtr) >> 2) & (0xF)); /* datalen in words */

        /* the datLen is saved by devision to 12(datLen/12) see function        *
         *  capacpumail.c::xbarCpuMailSend() , the simulation doesnot need this *
         *  devision , so i choosed to multipy the number by 12 , here for      *
         *  handling all the CPU mailbox message                                */
        dataLen = ( dataLen + 1 ) * 12 ;
        memcpy(dataPtr , &dataLen , sizeof(GT_U8));
        dataPtr++;

        /* read the celldata from the register of 0x00200000/ xbarCpuMailSend()   */
        memcpy(dataPtr,
               smemMemGet(deviceObjPtr,SMEM_CAP_CPU_MAIL_BOX_REG),
               (dataLen)*sizeof(GT_U32));

        /* set source type of buffer */
        bufferId->srcType = SMAIN_SRC_TYPE_CPU_E;

        /* set message type of buffer */
        bufferId->dataType = SMAIN_CPU_MAILBOX_MSG_E;

        /* put buffer to queue */
        squeBufPut(deviceObjPtr->queueId, SIM_CAST_BUFF(bufferId));
    }
}

