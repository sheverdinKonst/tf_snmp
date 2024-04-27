/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemFE.c
*
* DESCRIPTION:
*       This is API implementation for Dune Fabric Element memory.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/smem/smemFe.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>

static void * smemFeFindMem
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
);


static GT_U32 *  smemFeGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static void smemFeAllocSpecMemory(
    INOUT SMEM_FE_DEV_MEM_INFO  * devMemInfoPtr
);


/* Private definition */
#define     CONFIG1_REGS_NUM                    (0xFFFF / 4) + 1
#define     CONFIG2_REGS_NUM                    (0xffff / 4) + 1

/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemFEActiveTable[] =
{
    /* must be last anyway */
    {0xffffffff, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};

#define SMEM_ACTIVE_MEM_TABLE_SIZE \
    (sizeof(smemFEActiveTable)/sizeof(smemFEActiveTable[0]))

/*******************************************************************************
* smemFeInit
*
* DESCRIPTION:
*       Init memory module for the Fabric Element device.
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
void smemFeInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_FE_DEV_MEM_INFO  * devMemInfoPtr;

    /* alloc SMEM_FE_DEV_MEM_INFO */
    devMemInfoPtr = (SMEM_FE_DEV_MEM_INFO *)calloc(1, sizeof(SMEM_FE_DEV_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
        skernelFatalError("smemFEInit: allocation error\n");
    }

    /* allocate address type specific memories */
    smemFeAllocSpecMemory(devMemInfoPtr);

    devObjPtr->devFindMemFunPtr = (void *)smemFeFindMem;
    devObjPtr->deviceMemory = devMemInfoPtr;
}
/*******************************************************************************
*   smemFeFindMem
*
* DESCRIPTION:
*       Return pointer to the register or tables memory.
*
* INPUTS:
*       deviceObj   - pointer to device object.
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
*
*
*******************************************************************************/
static void * smemFeFindMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    void                * memPtr;
    GT_32                 index;
    GT_U32                param;

    if (devObjPtr == 0)
    {
        skernelFatalError("smemFeFindMem: illegal pointer \n");
    }
    memPtr = 0;
    param = 0;

    /* Find PCI registers memory  */
    if (SMEM_ACCESS_PCI_FULL_MAC(accessType))
    {
        skernelFatalError("smemFeFindMem: illegal pci request \n");
    }

    memPtr = smemFeGlobalReg(devObjPtr,address,memSize,param);

    /* find active memory entry */
    if (activeMemPtrPtr != NULL)
    {
        *activeMemPtrPtr = NULL;
        for (index = 0; index < SMEM_ACTIVE_MEM_TABLE_SIZE; index++)
        {
            /* check address */
            if ((address & smemFEActiveTable[index].mask)
                 == smemFEActiveTable[index].address)
                *activeMemPtrPtr = &smemFEActiveTable[index];
        }
    }

    return memPtr;
}

/*******************************************************************************
* smemFeGlobalReg
*
* DESCRIPTION:
*       Global FE configuration registers
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter might be used in
*
* OUTPUTS:
*       None.
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemFeGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_FE_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_FE_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* config1 registers */
    if ((address & 0x3FFF0000) == 0x30000000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->feConfigMem.feRange1Reg ,
                         devMemInfoPtr->feConfigMem.feRange1RegNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->feConfigMem.feRange1Reg[index];
    }
    /* config2 registers */
    else
    if ((address & 0xFFFF0000) == 0x00000000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->feConfigMem.feRange2Reg ,
                         devMemInfoPtr->feConfigMem.feRange2RegNum ,
                         index, memSize);
        regValPtr = &devMemInfoPtr->feConfigMem.feRange2Reg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemFeAllocSpecMemory
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
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemFeAllocSpecMemory(
    INOUT SMEM_FE_DEV_MEM_INFO  * devMemInfoPtr
)
{
    devMemInfoPtr->feConfigMem.feRange1RegNum = CONFIG1_REGS_NUM;
    devMemInfoPtr->feConfigMem.feRange1Reg =
        calloc(CONFIG1_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->feConfigMem.feRange1Reg == 0)
    {
        skernelFatalError("smemFeAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->feConfigMem.feRange2RegNum = CONFIG2_REGS_NUM;
    devMemInfoPtr->feConfigMem.feRange2Reg =
        calloc(CONFIG2_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->feConfigMem.feRange2Reg == 0)
    {
        skernelFatalError("smemFeAllocSpecMemory: allocation error\n");
    }
}

