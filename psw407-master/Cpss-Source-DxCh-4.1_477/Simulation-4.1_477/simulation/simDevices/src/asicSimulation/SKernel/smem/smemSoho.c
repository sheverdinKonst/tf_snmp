/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemSoho.c
*
* DESCRIPTION:
*       This is API implementation for Soho memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 32 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/smem/smemSoho.h>
#include <asicSimulation/SKernel/sohoCommon/sregSoho.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/suserframes/snetTwistTrafficCond.h>
#include <asicSimulation/SKernel/suserframes/snetSohoEgress.h>


static GT_U32 *  smemSohoFatalError(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemSohoGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemSohoPhyReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static GT_U32 *  smemSohoSmiReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static GT_U32 *  smemSohoMacTableReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemSohoVlanTableReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemSohoStuTableReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemSohoPvtTableReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemSohoStatsCountReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static void smemSohoInitFuncArray(
    INOUT SOHO_DEV_MEM_INFO  * devMemInfoPtr
);

static void smemSohoAllocSpecMemory(
    INOUT SOHO_DEV_MEM_INFO  * devMemInfoPtr,
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);

static void smemSohoActiveWriteStatsOp (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32   address,
    IN GT_U32   memSize,
    IN GT_U32 * memPtr,
    IN GT_UINTPTR   param,
    IN GT_U32 * inMemPtr
);

static void smemSohoActiveWriteVtuOp (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32   address,
    IN GT_U32   memSize,
    IN GT_U32 * memPtr,
    IN GT_UINTPTR   param,
    IN GT_U32 * inMemPtr
);

static void smemSohoActiveWriteAtuOp (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32   address,
    IN GT_U32   memSize,
    IN GT_U32 * memPtr,
    IN GT_UINTPTR   param,
    IN GT_U32 * inMemPtr
);

static void smemSohoActiveWriteTrunkMask (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32   address,
    IN GT_U32   memSize,
    IN GT_U32 * memPtr,
    IN GT_UINTPTR   param,
    IN GT_U32 * inMemPtr
);


static void smemSohoActiveReadTrunkMask (
    IN   SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN   GT_U32   address,
    IN   GT_U32   memSize,
    IN   GT_U32 * memPtr,
    IN   GT_UINTPTR   param,
    OUT  GT_U32 * outMemPtr
);

static void smemSohoActiveWriteTrunkRout (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32   address,
    IN GT_U32   memSize,
    IN GT_U32 * memPtr,
    IN GT_UINTPTR   param,
    IN GT_U32 * inMemPtr
);

static void smemSohoActiveReadTrunkRout (
    IN   SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN   GT_U32   address,
    IN   GT_U32   memSize,
    IN   GT_U32 * memPtr,
    IN   GT_UINTPTR   param,
    OUT  GT_U32 * outMemPtr
);

static void smemSohoActiveWriteTrgDevice (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32   address,
    IN GT_U32   memSize,
    IN GT_U32 * memPtr,
    IN GT_UINTPTR   param,
    IN GT_U32 * inMemPtr
);

static void smemSohoActiveWriteFlowCtrlDelay (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32   address,
    IN GT_U32   memSize,
    IN GT_U32 * memPtr,
    IN GT_UINTPTR   param,
    IN GT_U32 * inMemPtr
);

static void smemSohoActiveReadGlobalStat (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static void smemSohoActiveReadPhyInterruptStat (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
) ;

static void smemOpalAddStuEntry (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 * vtuWordPtr
);

static GT_STATUS smemOpalGetNextStuEntry (
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN  GT_U32 sid,
    OUT GT_U32 * stuWordPtr
);

static void smemSohoAddVtuEntry (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 * vtuWordPtr
);

static void smemSohoDeleteVtuEntry (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 vid
);

static GT_STATUS smemSohoGetNextVtuEntry (
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN  GT_U32 vid,
    OUT GT_U32 * vtuWordPtr
);

static void smemSohoActiveWriteGobalStatus(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN  GT_U32   address,
    IN  GT_U32   memSize,
    IN  GT_U32 * memPtr,
    IN  GT_UINTPTR   param,
    IN  GT_U32 * inMemPtr
);

static void smemSohoActiveWriteForceLinkDown (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

typedef enum {
    SOHO_SMI_SINGLE_ADDR_MODE = 0,
    SOHO_SMI_MULTI_ADDR_MODE
} SOHO_REG_ACCESS_E;

/* Statistic Unit Opcodes */
typedef enum {
    NO_OPERATION = 0,
    FLUSH_ALL_CNT_ALL_E,
    FLUSH_ALL_CNT_FOR_PORT_E,
    RESERVED,
    READ_CAPTURED_CNT_E,
    CAPTURE_ALL_CNT_FOR_PORT_E
} SOHO_STAT_OP_E;

/* VTU Opcodes */
typedef enum {
    VTU_NO_OPERATION_E = 0,
    VTU_FLUSH_ALL_VTU_ALL_E = 1,
    VTU_LOAD_PURGE_ENTRY_E = 3,
    VTU_GET_NEXT_E = 4,
    STU_LOAD_PURGE_E = 5,
    STU_GET_NEXT_E = 6,
    VTU_GET_CLEAR_VIOLATION_E = 7
} SOHO_VTU_OP_E;

/* Stats operation counter number */
#define SOHO_RMON_CNT_NUM                   (0x26)
/* the Opal use registers 0xA...0xF */
/* order of registers :
    0xB ,0xC,0xD,0xE,0xF
    0xA
*/
/* the Opal+ use also register 1 --> put register 0x1 to be after 0xA*/
/* order of registers :
    0xB ,0xC,0xD,0xE,0xF
    0xA , 0x1
*/
#define SMEM_SOHO_ATU_MSG_WORDS_NUM         (7)
#define SMEM_SOHO_LINK_FORCE_MSG_SIZE        (0x5)


/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemSohoActiveTable[] =
{
    /* Stats operation register */
    {0x001b01d0, SMEM_FULL_MASK_CNS, NULL, 0 , smemSohoActiveWriteStatsOp, 0},
    /* VTU operation register */
    {0x001b0050, SMEM_FULL_MASK_CNS, NULL, 0 , smemSohoActiveWriteVtuOp, 0},
    /* ATU operation register */
    {0x001b00b0, SMEM_FULL_MASK_CNS, NULL, 0 , smemSohoActiveWriteAtuOp, 0},
    /* Switch Global Status Register */
    {0x001b0000, SMEM_FULL_MASK_CNS, smemSohoActiveReadGlobalStat, 0, NULL, 0},
    /* PHY Interrupt Status Register */
    {0x20000130, 0xFF00FFFF, smemSohoActiveReadPhyInterruptStat, 0, NULL, 0},
    /* Flow Control Delay Memory */
    {0x001c0040, SMEM_FULL_MASK_CNS, NULL, 0 , smemSohoActiveWriteFlowCtrlDelay, 0},
    /* Target device Memory */
    {0x001c0060, SMEM_FULL_MASK_CNS, NULL, 0 , smemSohoActiveWriteTrgDevice, 0},
    /* Trunk Mask Memory */
    {0x001c0070, SMEM_FULL_MASK_CNS, smemSohoActiveReadTrunkMask, 0 , smemSohoActiveWriteTrunkMask, 0},
    /* Trunk Route Memory */
    {0x001c0080, SMEM_FULL_MASK_CNS, smemSohoActiveReadTrunkRout, 0 , smemSohoActiveWriteTrunkRout, 0},
    /* Switch Global Control Register */
    {0x001b0040, SMEM_FULL_MASK_CNS, NULL, 0 , smemSohoActiveWriteGobalStatus, 0},
    /* PCS Port Control register */
    {0x10100010, 0xFFF0FFFF, NULL, 0 , smemSohoActiveWriteForceLinkDown, 0},
    /* must be last anyway */
    {0xffffffff, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};


#define SMEM_ACTIVE_MEM_TABLE_SIZE \
    (sizeof(smemSohoActiveTable)/sizeof(smemSohoActiveTable[0]))

/*******************************************************************************
*   smemSohoInit
*
* DESCRIPTION:
*       Init memory module for a Twist device.
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
void smemSohoInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SOHO_DEV_MEM_INFO  * devMemInfoPtr; /* device's memory pointer */
    /* string for parameter */
    char   param_str[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
    GT_U32 param_val; /* parameter value */

    devObjPtr->notSupportPciConfigMemory = 1;/* the device not support PCI/PEX configuration memory space */

    /* alloc SOHO_DEV_MEM_INFO */
    devMemInfoPtr = (SOHO_DEV_MEM_INFO *)calloc(1, sizeof(SOHO_DEV_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
            skernelFatalError("smemSohoInit: allocation error\n");
    }

    if (!SIM_OS_MAC(simOsGetCnfValue)("system",  "access_mode",
                             SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        /* Default access mode single-chip addressing mode */
        devMemInfoPtr->accessMode = SOHO_SMI_SINGLE_ADDR_MODE;
    }
    else
    {
        sscanf(param_str, "%u", &param_val);
        devMemInfoPtr->accessMode = param_val;
    }

    /* init specific functions array */
    smemSohoInitFuncArray(devMemInfoPtr);

    /* allocate address type specific memories */
    smemSohoAllocSpecMemory(devMemInfoPtr,devObjPtr);

    devObjPtr->devFindMemFunPtr = (void *)smemSohoFindMem;
    devObjPtr->deviceMemory = devMemInfoPtr;
}
/*******************************************************************************
*   smemSohoFindMem
*
* DESCRIPTION:
*       Return pointer to the register's or tables's memory.
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
void * smemSohoFindMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    void              * memPtr;         /* registers's memory pointer */
    SOHO_DEV_MEM_INFO * devMemInfoPtr;  /* device's memory pointer */
    GT_U32              index;          /* register's memory offset */
    GT_U32              param;          /* additional parameter */

    memPtr = 0;
    devMemInfoPtr = (SOHO_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    index = (address >> 28) & 0xf;
    /* Call register spec function to obtain pointer to register memory */
    param   = devMemInfoPtr->specFunTbl[index].specParam;
    memPtr  = devMemInfoPtr->specFunTbl[index].specFun(devObjPtr,
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
            /* check address */
            if ((address & smemSohoActiveTable[index].mask)
                 == smemSohoActiveTable[index].address)
                *activeMemPtrPtr = &smemSohoActiveTable[index];
        }
    }

    return memPtr;
}

/*******************************************************************************
*   smemSohoGlobalReg
*
* DESCRIPTION:
*       Global configuration Registers.
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
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemSohoGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SOHO_DEV_MEM_INFO  * devMemInfoPtr; /* device's memory pointer */
    GT_U32               index;         /* register's memory offset */
    SMEM_REGISTER      * globalRegsPtr; /* global register's pointer */

    devMemInfoPtr = (SOHO_DEV_MEM_INFO  *)devObjPtr->deviceMemory;
    /* Global  or global 2 memory space */
    globalRegsPtr = ((address & 0x001b0000) == 0x001b0000) ?
                    devMemInfoPtr->devRegs.globalRegs :
                     devMemInfoPtr->devRegs.global2Regs;



    /* Global registers */
    index = (address & 0xffff) >> 4;
    CHECK_MEM_BOUNDS(globalRegsPtr,
                     devMemInfoPtr->devRegs.globalRegsNum,
                     index, memSize);

    return &globalRegsPtr[index];
}

/*******************************************************************************
*   smemSohoPhyReg
*
* DESCRIPTION:
*       Device PHY's registers access.
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
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemSohoPhyReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SOHO_DEV_MEM_INFO  * devMemInfoPtr;     /* device's memory pointer */
    GT_U32               index, smiPort;    /* device and register offsets */
    GT_U32              * regValPtr;        /* pointer to register's value */

    devMemInfoPtr = (SOHO_DEV_MEM_INFO  *)devObjPtr->deviceMemory;
    regValPtr = 0;

    /* Retrieve port address */
    smiPort = (address >> 16) & 0xff;

    /* force Phy Port Summary register to be common for all ports */
    if ((address & 0xFF00FFFF) == PHY_INTERRUPT_PORT_SUM_REG)
    {
        smiPort = 0 ;
    }

    if (smiPort >= (SOHO_PHY_REGS_START_ADDR + SOHO_PORTS_NUMBER))
    {
        skernelFatalError("smemSohoPhyReg: \
                           index or memory size is out of range\n");
    }

    index = (address & 0xffff) >> 4;
    CHECK_MEM_BOUNDS(devMemInfoPtr->devRegs.phyRegs[smiPort],
                     devMemInfoPtr->devRegs.phyRegsNum,
                     index, memSize);

    regValPtr = &devMemInfoPtr->devRegs.phyRegs[smiPort][index];

    return regValPtr;
}


/*******************************************************************************
*   smemSohoSmiReg
*
* DESCRIPTION:
*       Device SMI's registers access.
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
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemSohoSmiReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SOHO_DEV_MEM_INFO  * devMemInfoPtr;     /* device's memory pointer */
    GT_U32               index, smiPort;    /* device and register offsets */
    GT_U32              * regValPtr;        /* pointer to register's value */

    devMemInfoPtr = (SOHO_DEV_MEM_INFO  *)devObjPtr->deviceMemory;
    regValPtr = 0;

    /* Retrieve port address */
    smiPort = (address >> 16) & 0xff;
    /* Allign SMI device address to zero based index */
    if (smiPort >= (SOHO_PORT_REGS_START_ADDR + SOHO_PORTS_NUMBER))
    {
        skernelFatalError("smemSohoPhyReg: \
                           index or memory size is out of range\n");
    }
    smiPort %= SOHO_PORTS_NUMBER;
    index = (address & 0xffff) >> 4;
    CHECK_MEM_BOUNDS(devMemInfoPtr->devRegs.smiRegs[smiPort],
                     devMemInfoPtr->devRegs.smiRegsNum,
                     index, memSize);

    regValPtr = &devMemInfoPtr->devRegs.smiRegs[smiPort][index];

    return regValPtr;
}


/*******************************************************************************
*   smemSohoMacTableReg
*
* DESCRIPTION:
*       MAC table memory access
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
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemSohoMacTableReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SOHO_DEV_MEM_INFO   * devMemInfoPtr;    /* device's memory pointer */
    GT_U32              * regValPtr;        /* pointer to register's value */
    GT_U32              index;/* index of word in memory */

    regValPtr = 0;
    devMemInfoPtr = (SOHO_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    index =  (address>>2) & 0xffff;
    index += ((address>>1)& 0x1)?1:0;

    if (index >= devMemInfoPtr->macDbMem.macTblMemSize)
    {
        skernelFatalError("Wrong MAC table address %X, exceed maximal \
                          size %X",
                          address,
                          devMemInfoPtr->macDbMem.macTblMemSize);
    }
    regValPtr = &devMemInfoPtr->macDbMem.macTblMem[index];

    return regValPtr;
}

/*******************************************************************************
*   smemSohoStatsCountReg
*
* DESCRIPTION:
*       Stats counters memory access
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
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemSohoStatsCountReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SOHO_DEV_MEM_INFO   * devMemInfoPtr;    /* device's memory pointer */
    GT_U32              * regValPtr;        /* pointer to register's value */
    GT_U32               index, smiPort;    /* device and register offsets */

    regValPtr = 0;
    devMemInfoPtr = (SOHO_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Retrieve port address */
    smiPort = (address >> 16) & 0xff;
    /* Allign SMI device address to zero based index */
    if (smiPort >= (SOHO_PORT_REGS_START_ADDR + SOHO_PORTS_NUMBER))
    {
        skernelFatalError("smemSohoStatsCountReg: \
                           index or memory size is out of range\n");
    }
    smiPort -= SOHO_PORT_REGS_START_ADDR;
    index = (address & 0xffff) >> 4;
    if (index >= devMemInfoPtr->statsCntMem.cntStatsTblSize)
    {
        skernelFatalError("Wrong Stats Count table address %X, exceed maximal \
                          size %X",
                          address,
                          devMemInfoPtr->statsCntMem.cntStatsTblSize);
    }
    regValPtr = &devMemInfoPtr->statsCntMem.cntStatsTblMem[smiPort][index];

    return regValPtr;
}


/*******************************************************************************
*   smemSohoVlanTableReg
*
* DESCRIPTION:
*       VLAN memory table accsess
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
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemSohoVlanTableReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SOHO_DEV_MEM_INFO  * devMemInfoPtr; /* device's memory pointer */
    GT_U32             * regValPtr;     /* pointer to register's value */
    GT_U32 index;                       /* VLAN table memory offset */
    GT_U32 vid;                         /* VLAN id */
    GT_U32 word;                        /* word number 0 or 1 */

    regValPtr = 0;
    devMemInfoPtr = (SOHO_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    vid = (address >> 4) & 0xfff;
    word = (address & 0xf) / 4;

    index = (vid * SOHO_VLAN_ENTRY_WORDS) + word;
    if (index >= devMemInfoPtr->vlanDbMem.vlanTblMemSize)
    {
        skernelFatalError("Wrong VLAN table address %X, exceed maximal \
                          size %X",
                          address,
                          devMemInfoPtr->vlanDbMem.vlanTblMemSize);
    }
    regValPtr = &devMemInfoPtr->vlanDbMem.vlanTblMem[index];
    /* Vlan Table Registers */
    return regValPtr;
}

/*******************************************************************************
*   smemSohoStuTableReg
*
* DESCRIPTION:
*       STU memory table accsess
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
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemSohoStuTableReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SOHO_DEV_MEM_INFO  * devMemInfoPtr; /* device's memory pointer */
    GT_U32             * regValPtr;     /* pointer to register's value */
    GT_U32 index;                       /* SIDtable memory offset */
    GT_U32 sid;                         /* VSID id */
    GT_U32 word;                        /* word number 0 or 1 */

    regValPtr = 0;
    devMemInfoPtr = (SOHO_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    sid = (address >> 4) & 0x3f;
    word = (address & 0xf) / 4;

    index = (sid * SOHO_VLAN_ENTRY_WORDS) + word;
    if (index >= devMemInfoPtr->stuDbMem.stuTblMemSize)
    {
        skernelFatalError("Wrong STU table address %X, exceed maximal \
                          size %X",
                          address,
                          devMemInfoPtr->stuDbMem.stuTblMemSize);
    }
    regValPtr = &devMemInfoPtr->stuDbMem.stuTblMem[index];
    /* Vlan Table Registers */
    return regValPtr;
}

/*******************************************************************************
*   smemSohoPvtTableReg
*
* DESCRIPTION:
*       PVT memory table accsess
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
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemSohoPvtTableReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SOHO_DEV_MEM_INFO  * devMemInfoPtr; /* device's memory pointer */
    GT_U32             * regValPtr;     /* pointer to register's value */
    GT_U32 index;                       /* pvt table memory offset */
    GT_U32 pid;                         /* VSID id */
    GT_U32 word;                        /* word number 0 or 1 */

    regValPtr = 0;
    devMemInfoPtr = (SOHO_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    pid = (address >> 4) & 0x3f;
    word = (address & 0xf) / 4;

    index = (pid * SOHO_VLAN_ENTRY_WORDS) + word;
    if (index >= devMemInfoPtr->pvtDbMem.pvtTblMemSize)
    {
        skernelFatalError("Wrong PVT table address %X, exceed maximal \
                          size %X",
                          address,
                          devMemInfoPtr->pvtDbMem.pvtTblMemSize);
    }
    regValPtr = &devMemInfoPtr->pvtDbMem.pvtTblMem[index];
    /* Pvt Table Registers */
    return regValPtr;
}

/*******************************************************************************
*   smemSohoFatalError
*
* DESCRIPTION:
*       Memory access error funktion
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
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32 *  smemSohoFatalError(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    skernelFatalError("smemSohoFatalError: illegal function pointer, \n \
                       device %d,  address 0x%x", devObjPtr, address);

    return 0;
}
/*******************************************************************************
*   smemSohoInitMemArray
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
*
*
*******************************************************************************/
static void smemSohoInitFuncArray(
    INOUT SOHO_DEV_MEM_INFO  * devMemInfoPtr
)
{
    GT_U32              i;

    for (i = 0; i < 64; i++) {
        devMemInfoPtr->specFunTbl[i].specFun    = smemSohoFatalError;
    }

    devMemInfoPtr->specFunTbl[0].specFun        = smemSohoGlobalReg;
    devMemInfoPtr->specFunTbl[1].specFun        = smemSohoSmiReg;
    devMemInfoPtr->specFunTbl[2].specFun        = smemSohoPhyReg;
    devMemInfoPtr->specFunTbl[3].specFun        = smemSohoVlanTableReg;
    devMemInfoPtr->specFunTbl[4].specFun        = smemSohoMacTableReg;
    devMemInfoPtr->specFunTbl[5].specFun        = smemSohoStatsCountReg;
    devMemInfoPtr->specFunTbl[6].specFun        = smemSohoStuTableReg;
    devMemInfoPtr->specFunTbl[7].specFun        = smemSohoPvtTableReg;
}

/*******************************************************************************
*   smemSohoAllocSpecMemory
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
static void smemSohoAllocSpecMemory(
    INOUT SOHO_DEV_MEM_INFO  * devMemInfoPtr,
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32 smiDev;      /* current SMI device */
    GT_U32 port;        /* current device port */

    /* Global registers memory allocation */
    devMemInfoPtr->devRegs.globalRegsNum = SOHO_REG_NUMBER;
    devMemInfoPtr->devRegs.globalRegs =
                        calloc(SOHO_REG_NUMBER, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->devRegs.globalRegs == 0)
    {
        skernelFatalError("smemSohoAllocSpecMemory: allocation error\n");
    }
    /* Global 2 registers memory allocation */
    devMemInfoPtr->devRegs.global2Regs =
                        calloc(SOHO_REG_NUMBER, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->devRegs.global2Regs == 0)
    {
        skernelFatalError("smemSohoAllocSpecMemory: allocation error\n");
    }
    /* Smi registers memory allocation */
    devMemInfoPtr->devRegs.smiRegsNum = SOHO_PORTS_NUMBER * SOHO_REG_NUMBER;
    for (smiDev = 0; smiDev < SOHO_PORTS_NUMBER; smiDev++)
    {
        devMemInfoPtr->devRegs.smiRegs[smiDev] =
                        calloc(SOHO_REG_NUMBER, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->devRegs.smiRegs[smiDev] == 0)
        {
            skernelFatalError("smemSohoAllocSpecMemory: allocation error\n");
        }
    }

    /* Phy registers memory allocation */
    devMemInfoPtr->devRegs.phyRegsNum = SOHO_PORTS_NUMBER * SOHO_REG_NUMBER;
    for (smiDev = 0; smiDev < SOHO_PORTS_NUMBER; smiDev++)
    {
        devMemInfoPtr->devRegs.phyRegs[smiDev] =
                        calloc(SOHO_REG_NUMBER, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->devRegs.phyRegs[smiDev] == 0)
        {
            skernelFatalError("smemSohoAllocSpecMemory: allocation error\n");
        }
    }

    /* Mac table memory allocation */
    devMemInfoPtr->macDbMem.macTblMemSize = SOHO_MAC_TABLE_8K_SIZE;
    devMemInfoPtr->macDbMem.macTblMem =
        calloc(devMemInfoPtr->macDbMem.macTblMemSize, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macDbMem.macTblMem == 0)
    {
        skernelFatalError("smemSohoAllocSpecMemory: allocation error\n");
    }

    /* Vlan table memory allocation */
    devMemInfoPtr->vlanDbMem.vlanTblMemSize = SOHO_VLAN_TABLE_SIZE;
    devMemInfoPtr->vlanDbMem.vlanTblMem =
        calloc(SOHO_VLAN_TABLE_SIZE, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->vlanDbMem.vlanTblMem == 0)
    {
        skernelFatalError("smemSohoAllocSpecMemory: allocation error\n");
    }
    devMemInfoPtr->vlanDbMem.violation = SOHO_NONE_E;

    if(SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
    {
        /* Stu table memory allocation */
        devMemInfoPtr->stuDbMem.stuTblMemSize = OPAL_STU_TABLE_SIZE;
        devMemInfoPtr->stuDbMem.stuTblMem =
            calloc(OPAL_STU_TABLE_SIZE, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->stuDbMem.stuTblMem == 0)
        {
            skernelFatalError("smemSohoAllocSpecMemory: allocation error\n");
        }
    }

    if(SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
    {
        /* PVT table memory allocation */
        devMemInfoPtr->pvtDbMem.pvtTblMemSize = OPAL_PVT_TABLE_SIZE;
        devMemInfoPtr->pvtDbMem.pvtTblMem =
            calloc(OPAL_PVT_TABLE_SIZE, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->pvtDbMem.pvtTblMem == 0)
        {
            skernelFatalError("smemSohoAllocSpecMemory: allocation error\n");
        }
    }

    /* RMON statistic counters allocation */
    devMemInfoPtr->statsCntMem.cntStatsTblSize =
        SOHO_PORTS_NUMBER * SOHO_RMON_CNT_NUM;
    for (port = 0; port < SOHO_PORTS_NUMBER; port++)
    {
        devMemInfoPtr->statsCntMem.cntStatsTblMem[port] =
            calloc(SOHO_RMON_CNT_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->statsCntMem.cntStatsTblMem[port] == 0)
        {
            skernelFatalError("smemSohoAllocSpecMemory: allocation error\n");
        }
    }
    /* RMON captured counters allocation */
    devMemInfoPtr->statsCntMem.cntStatsCaptureMem =
        calloc(SOHO_RMON_CNT_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->statsCntMem.cntStatsCaptureMem == 0)
    {
        skernelFatalError("smemSohoAllocSpecMemory: allocation error\n");
    }
}
/*******************************************************************************
*  smemSohoActiveWriteStatsOp
*
* DESCRIPTION:
*      Provides CPU interface for the operations on the stat counters
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemSohoActiveWriteStatsOp (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32 fldValue;        /* register's and register' field value */
    SOHO_STAT_OP_E statOp;          /* statistic unit opcode */
    GT_U32 port;                    /* SMI device's port */
    SOHO_DEV_MEM_INFO * devMemInfoPtr; /* device's memory pointer */
    SMEM_REGISTER * statsMemPtr;    /* pointer to statistic register */
    GT_U32 counter, counterValue;   /* counter's index and counter's value */
    GT_U32 setIntr = 0;             /* interrupt's flag */

    *memPtr = *inMemPtr;

    /* Get pointer to the device memory */
    devMemInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);

    /* Statistic unit Opcode */
    statOp = SMEM_U32_GET_FIELD(memPtr[0], 12, 3);
    if (statOp == FLUSH_ALL_CNT_ALL_E)
    {
        for (port = 0; port < SOHO_PORTS_NUMBER; port++)
        {
            statsMemPtr = devMemInfoPtr->statsCntMem.cntStatsTblMem[port];
            for (counter = 0; counter < SOHO_RMON_CNT_NUM; counter++)
            {
                statsMemPtr[counter] = 0;
            }
        }
        setIntr = 1;
    }
    else
    if (statOp == FLUSH_ALL_CNT_FOR_PORT_E)
    {
        port = SMEM_U32_GET_FIELD(memPtr[0], 0, 6);
        statsMemPtr = devMemInfoPtr->statsCntMem.cntStatsTblMem[port];
        for (counter = 0; counter < SOHO_RMON_CNT_NUM; counter++)
        {
            statsMemPtr[counter] = 0;
        }
        setIntr = 1;
    }
    else
    if (statOp == READ_CAPTURED_CNT_E)
    {
        counter = SMEM_U32_GET_FIELD(memPtr[0], 0, 6);
        counterValue = devMemInfoPtr->statsCntMem.cntStatsCaptureMem[counter];
        fldValue = (counterValue >> 24);
        smemRegFldSet(devObjPtr, GLB_STATS_CNT3_2_REG, 8, 8, fldValue);

        fldValue = (counterValue >> 16 & 0xff);
        smemRegFldSet(devObjPtr, GLB_STATS_CNT3_2_REG, 0, 8, fldValue);

        fldValue = (counterValue >> 8 & 0xff);
        smemRegFldSet(devObjPtr, GLB_STATS_CNT1_0_REG, 8, 8, fldValue);

        fldValue = (counterValue & 0xff);
        smemRegFldSet(devObjPtr, GLB_STATS_CNT1_0_REG, 0, 8, fldValue);
        setIntr = 1;
    }
    else
    if (statOp == CAPTURE_ALL_CNT_FOR_PORT_E)
    {
        port = SMEM_U32_GET_FIELD(memPtr[0], 0, 6);
        statsMemPtr = devMemInfoPtr->statsCntMem.cntStatsTblMem[port];
        for (counter = 0; counter < SOHO_RMON_CNT_NUM; counter++)
        {
            devMemInfoPtr->statsCntMem.cntStatsCaptureMem[counter] =
                statsMemPtr[counter];
        }
        setIntr = 1;
    }
    /* Clear Statistic Unit Busy bit */
    SMEM_U32_SET_FIELD(memPtr[0], 15, 1, 0);
    if (setIntr == 1)
    {
        /* check that interrupt enabled */
        smemRegFldGet(devObjPtr, GLB_CTRL_REG, 6, 1, &fldValue);
        if (fldValue)
        {
            /* Statistic done interrupt */
            smemRegFldSet(devObjPtr, GLB_STATUS_REG, 6, 1, 1);
            scibSetInterrupt(devObjPtr->deviceId);
        }
    }
}
/*******************************************************************************
*  smemSohoActiveWriteVtuOp
*
* DESCRIPTION:
*      Provides CPU interface for the operations on the VTU
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void  smemSohoActiveWriteVtuOp (
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN  GT_U32   address,
    IN  GT_U32   memSize,
    IN  GT_U32 * memPtr,
    IN  GT_UINTPTR   param,
    IN  GT_U32 * inMemPtr
)
{
    GT_U32 fldValue;                        /* register's and register' field value */
    SOHO_VTU_OP_E vtuOp;                    /* VTU opcode */
    SOHO_DEV_MEM_INFO * memInfoPtr;         /* device's memory pointer */
    GT_U32 vtuWord[SOHO_VLAN_ENTRY_WORDS];  /* VTU words buffer */
    GT_U32 stuWord[OPAL_STU_ENTRY_WORDS];  /* VTU words buffer */
    GT_U32 vid, valid,sid;                  /* VID and valid bit */
    GT_STATUS status;                       /* return status value */
    GT_U16 * wordDataPtr = NULL;            /* violation data pointers */
    GT_U32 * opalMemPtr = NULL;             /* Pointer to the register's memory */

    *memPtr = *inMemPtr;

    if(SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
    {/* VLan entry information is taken 3 bytes before thr byte with the operatipn  */
        opalMemPtr = memPtr - 0x3;
    }

    /* Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);

    vtuOp = SMEM_U32_GET_FIELD(memPtr[0], 12, 3);

    if (vtuOp == VTU_FLUSH_ALL_VTU_ALL_E)
    {
        memset(memInfoPtr->vlanDbMem.vlanTblMem, 0,
                            memInfoPtr->vlanDbMem.vlanTblMemSize);
        if(SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
            memset(memInfoPtr->stuDbMem.stuTblMem, 0,
                   memInfoPtr->stuDbMem.stuTblMemSize);
    }
    else
    if (vtuOp == VTU_LOAD_PURGE_ENTRY_E)
    {

        valid = SMEM_U32_GET_FIELD(memPtr[1], 12, 1);

        memset(vtuWord, 0, sizeof(vtuWord));
        /* Valid */
        if (valid == 1)
        {
            if(!SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
            {
                /* DBNum/SPID[3:0] */
                vtuWord[0] =  SMEM_U32_GET_FIELD(memPtr[0], 0, 4);
                /* VID */
                vtuWord[0] |= SMEM_U32_GET_FIELD(memPtr[1], 0, 12) << 4;
                /* Ports 0-3 */
                vtuWord[0] |= SMEM_U32_GET_FIELD(memPtr[2], 0, 16) << 16;
                /* Ports 4-7 */
                vtuWord[1] = SMEM_U32_GET_FIELD(memPtr[3], 0, 16);
                /* Ports 8-9 and Vid PRI */
                vtuWord[1] |= SMEM_U32_GET_FIELD(memPtr[4], 0, 16) << 16;
                /* Valid */
                vtuWord[2] = valid;
                /* DBNum[7:4] */
                vtuWord[2] |= SMEM_U32_GET_FIELD(memPtr[0], 8, 4) << 1;
            }
            else
            {
                /* DBNum/SPID[3:0] , VID policy --- 13 bits */
                vtuWord[0] =  SMEM_U32_GET_FIELD(opalMemPtr[0], 0, 13);  /* register 0x02 */
                /* SID */
                vtuWord[0] |= SMEM_U32_GET_FIELD(opalMemPtr[1], 0, 6) << 13; /* register 0x03 */
                /* VID */
                vtuWord[0] |= SMEM_U32_GET_FIELD(opalMemPtr[4], 0, 12) << 19; /* register 0x06 */
                /* Valid */
                vtuWord[0] |= valid << 31;
                /* Ports 0-3 */
                vtuWord[1] |= SMEM_U32_GET_FIELD(opalMemPtr[5], 0, 16) ; /* register 0x07 */
                /* Ports 4-7 */
                vtuWord[1] |= SMEM_U32_GET_FIELD(opalMemPtr[6], 0, 16) << 16; /* register 0x08 */
                /* Ports 8-9 and Vid PRI */
                vtuWord[2] |= SMEM_U32_GET_FIELD(opalMemPtr[7], 0, 16) ; /* register 0x09 */

            }

            /* Add new VTU entry to SRAM */
            smemSohoAddVtuEntry(devObjPtr, vtuWord);
        }
        else
        {
            if (!(SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType)))
            {
                vid = SMEM_U32_GET_FIELD(memPtr[1], 0, 12);
            }
            else
            {
                vid = SMEM_U32_GET_FIELD(opalMemPtr[3], 0, 12);
            }
            /* Delete VTU entry from SRAM */
            smemSohoDeleteVtuEntry(devObjPtr, vid);
        }
    }
    else
    if (vtuOp == VTU_GET_NEXT_E)
    {

        vid = SMEM_U32_GET_FIELD(memPtr[1], 0, 12);
         /* Find the next higher VID currently in the VTU's database */
        status = smemSohoGetNextVtuEntry(devObjPtr, vid, vtuWord);
        if (status == GT_OK)
        {
            if(!SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
            {
                 /* DBNum/SPID[3:0] */
                fldValue = vtuWord[0] & 0xf;
                SMEM_U32_SET_FIELD(memPtr[0], 0, 4, fldValue);
                /* VID */
                fldValue = (vtuWord[0] >> 4) & 0xfff;
                SMEM_U32_SET_FIELD(memPtr[1], 0, 12, fldValue);
                /* Ports 0-3 */
                fldValue = (vtuWord[0] >> 16) & 0xffff;
                SMEM_U32_SET_FIELD(memPtr[2], 0, 16, fldValue);
                /* Ports 4-7 */
                fldValue = vtuWord[1] & 0xffff;
                SMEM_U32_SET_FIELD(memPtr[3], 0, 16, fldValue);
                /* Ports 8-9 and Vid PRI */
                fldValue = (vtuWord[1] >> 16) & 0xffff;
                SMEM_U32_SET_FIELD(memPtr[4], 0, 16, fldValue);
                /* Valid */
                fldValue = vtuWord[2] & 0x1;
                SMEM_U32_SET_FIELD(memPtr[1], 12, 1, fldValue);
                /* DBNum[7:4] */
                fldValue = (vtuWord[2] >> 1) & 0xf;
                SMEM_U32_SET_FIELD(memPtr[0], 8, 4, fldValue);
            }
            else
            {
                 /* DBNum/SPID[11:0] ,  VID policy ---> 13 bits*/
                fldValue = vtuWord[0] & 0x1fff;
                SMEM_U32_SET_FIELD(opalMemPtr[0], 0, 13, fldValue); /* register 0x02 */

                /* SID */
                fldValue = (vtuWord[0] >> 13) & 0x3f;
                SMEM_U32_SET_FIELD(opalMemPtr[1], 0, 6, fldValue);  /* register 0x03 */

                /* VID , valid -- 13 bits */
                fldValue = (vtuWord[0] >> 19) & 0x1fff;
                SMEM_U32_SET_FIELD(opalMemPtr[4], 0, 13, fldValue);  /* register 0x06 */

                /* Ports 0-3 */
                fldValue = vtuWord[1] & 0xffff;
                SMEM_U32_SET_FIELD(opalMemPtr[5], 0, 16, fldValue);  /* register 0x07 */
                /* Ports 4-7 */
                fldValue = (vtuWord[1] >> 16) & 0xffff;
                SMEM_U32_SET_FIELD(opalMemPtr[6], 0, 16, fldValue);  /* register 0x08 */
                /* Ports 8-9 and Vid PRI */
                fldValue = vtuWord[2] & 0xffff;
                SMEM_U32_SET_FIELD(opalMemPtr[7], 0, 16, fldValue);  /* register 0x09 */
            }

        }
        else
        {

                /* Next VID was not found */
                SMEM_U32_SET_FIELD(memPtr[1], 12, 1, 0);
                SMEM_U32_SET_FIELD(memPtr[1], 0, 12, 0xfff);

        }
    }
    else
    if (vtuOp == VTU_GET_CLEAR_VIOLATION_E)
    {
        wordDataPtr = memInfoPtr->vlanDbMem.violationData;
        /* Copy violation data */
        memcpy(memPtr, wordDataPtr, 5 * sizeof(GT_U16));

        if (memInfoPtr->vlanDbMem.violation == SOHO_MISS_VTU_VID_E)
        {
            /* Set VTU miss violation */
            SMEM_U32_SET_FIELD(memPtr[0], 5, 1, 1);
        }
        else
        if (memInfoPtr->vlanDbMem.violation == SOHO_SRC_VTU_PORT_E)
        {
            /* Set Source Port member violation */
            SMEM_U32_SET_FIELD(memPtr[0], 6, 1, 1);
        }
    }
    else if(SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
    {
        if (vtuOp == STU_LOAD_PURGE_E)
        {
                    memset(stuWord, 0, sizeof(stuWord));
                    /* SID*/
                    stuWord[0] |= SMEM_U32_GET_FIELD(opalMemPtr[1], 0, 6);
                    /* Valid */
                    stuWord[0] |= SMEM_U32_GET_FIELD(opalMemPtr[4], 12, 1) << 6;
                    /* Ports 0-3 */
                    stuWord[1] = SMEM_U32_GET_FIELD(opalMemPtr[5], 0, 16) ;
                    /* Ports 4-7 */
                    stuWord[1] = SMEM_U32_GET_FIELD(opalMemPtr[6], 0, 16) ;
                    /* Ports 8-10  */
                    stuWord[2] |= SMEM_U32_GET_FIELD(opalMemPtr[7], 0, 12) ;
                    /* Add new STU entry to SRAM */
                    smemOpalAddStuEntry(devObjPtr, stuWord);


        }
        else
        if (vtuOp == STU_GET_NEXT_E)
        {
            sid = SMEM_U32_GET_FIELD(opalMemPtr[1], 0, 6);
            /* Find the next higher sid currently in the STU's database */
            status = smemOpalGetNextStuEntry(devObjPtr, sid, stuWord);
            if (status == GT_OK)
            {
                    /* Ports 0-3 */
                    stuWord[1] = SMEM_U32_GET_FIELD(opalMemPtr[5], 0, 16) << 16;
                    /* Ports 4-7 */
                    stuWord[1] |= SMEM_U32_GET_FIELD(opalMemPtr[6], 0, 16);
                    /* Ports 8-9 and Vid PRI */
                    stuWord[2] = SMEM_U32_GET_FIELD(opalMemPtr[7], 0, 16) << 16;

                    smemRegFldSet(devObjPtr, GLB_VTU_SID_REG, 0, 6, sid + 1);
            }
        }
    }

    /* Clear VTU busy */
    SMEM_U32_SET_FIELD(memPtr[0], 15, 1, 0);

    /* check that interrupt enabled */
    smemRegFldGet(devObjPtr, GLB_CTRL_REG, 4, 1, &fldValue);
    if (fldValue)
    {
        /* VTU done interrupt */
        smemRegFldSet(devObjPtr, GLB_STATUS_REG, 4, 1, 1);
        scibSetInterrupt(devObjPtr->deviceId);
    }
}

/*******************************************************************************
*  smemSohoAddVtuEntry
*
* DESCRIPTION:
*      Add VTU entry to vlan table SRAM
* INPUTS:
*       devObjPtr   - device object PTR.
*       vtuWord     - pointer to VTU words
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemSohoAddVtuEntry (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 * vtuWordPtr
)
{
    GT_U32 vid;                         /* VLAN id */
    GT_U32 address = 0;                 /* VLAN table memory address */

    if (!(SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType)))
    {
        vid = (vtuWordPtr[0] >> 4) & 0xfff;
    }
    else
    {
        vid = (vtuWordPtr[0] >> 19) & 0xfff;
    }
    /* Make 32 bit word address */
    address = (3 << 28) | (vid << 4);
    smemMemSet(devObjPtr, address, &vtuWordPtr[0], 1);
    address += 0x4;
    smemMemSet(devObjPtr, address, &vtuWordPtr[1], 1);
    address += 0x4;
    smemMemSet(devObjPtr, address, &vtuWordPtr[2], 1);
}


/*******************************************************************************
*  smemOpalAddStuEntry
*
* DESCRIPTION:
*      Add VTU entry to vlan table SRAM
* INPUTS:
*       devObjPtr   - device object PTR.
*       stuWord     - pointer to STU words
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemOpalAddStuEntry (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 * stuWordPtr
)
{
    GT_U32 address = 0;                 /* stu table memory address */
    GT_U32 sid;                         /* Vbit id */

    sid = stuWordPtr[0] & 0x3f;
    /* Make 32 bit word address */
    address = (6 << 28) | (sid << 4 );

    smemMemSet(devObjPtr, address, &stuWordPtr[0], 1);
    address += 0x4;
    smemMemSet(devObjPtr, address, &stuWordPtr[1], 1);
    address += 0x4;
    smemMemSet(devObjPtr, address, &stuWordPtr[2], 1);
}

/*******************************************************************************
*  smemSohoDeleteVtuEntry
*
* DESCRIPTION:
*      Delete VTU entry from vlan table SRAM
* INPUTS:
*       devObjPtr   - device object PTR.
*       vtuEntryPtr - pointer to VTU structure
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static            void smemSohoDeleteVtuEntry (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 vid
)
{
    GT_U32 address = 0;         /* VLAN table memory address */
    GT_U32 regVal = 0;          /* register's value to write */

    /* Make 32 bit word address */
    if (!(SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType)))
    {
        address = (3 << 28) | (vid << 4);
    }
    else
    {
        address = (3 << 28) | (vid << 19);
    }
    smemMemSet(devObjPtr, address, &regVal, 1);
    address += 0x4;
    smemMemSet(devObjPtr, address, &regVal, 1);
    address += 0x4;
    smemMemSet(devObjPtr, address, &regVal, 1);
}

/*******************************************************************************
*  smemSohoGetNextVtuEntry
*
* DESCRIPTION:
*      Get next valid VTU entry from vlan table SRAM
* INPUTS:
*       devObjPtr   - device object PTR.
*       vtuEntryPtr - pointer to VTU structure
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS smemSohoGetNextVtuEntry (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 vid,
    OUT GT_U32 * vtuWordPtr
)
{
    SOHO_DEV_MEM_INFO * memInfoPtr;         /* device's memory pointer */
    GT_U32 vtuOffset = vid + 1, vtuSize;    /* VLAN table memory offset */
    SMEM_REGISTER * vlanTblMemPtr;          /* VLAN table memory pointer */
    GT_32 index;
    GT_U32 validBit;

    /* mask the vtu offset */
    vtuOffset = vtuOffset & 0xFFF;
    if (vid == 0xfff)
    {
        vtuOffset = 0 ;
    }

    /* Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);
    vlanTblMemPtr = memInfoPtr->vlanDbMem.vlanTblMem;

    vtuSize = memInfoPtr->vlanDbMem.vlanTblMemSize / SOHO_VLAN_ENTRY_BYTES;
    if (vtuOffset >= vtuSize)
    {
        skernelFatalError("Wrong VLAN table address %X, exceed maximal \
                          address %X",
                          vtuOffset,
                          memInfoPtr->vlanDbMem.vlanTblMemSize);
    }

    while (vtuOffset <  vtuSize)
    {
        index = vtuOffset * SOHO_VLAN_ENTRY_WORDS;

        if (!(SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType)))
        {
            validBit = SMEM_U32_GET_FIELD(vlanTblMemPtr[index + 2], 0, 1);
        }
        else
        {
            validBit = SMEM_U32_GET_FIELD(vlanTblMemPtr[index], 31, 1);
        }
        if ( validBit )
        {
            memcpy(vtuWordPtr, &vlanTblMemPtr[index],
                    SOHO_VLAN_ENTRY_WORDS * sizeof(GT_U32));

            return GT_OK;
        }

        vtuOffset++;
    }

    /* End of the VLAN table was reached with no new valid entries */
    return GT_NOT_FOUND;
}


/*******************************************************************************
*  smemOpalGetNextStuEntry
*
* DESCRIPTION:
*      Get next valid VTU entry from vlan table SRAM
* INPUTS:
*       devObjPtr   - device object PTR.
*       vtuEntryPtr - pointer to VTU structure
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS smemOpalGetNextStuEntry (
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 sid,
    OUT GT_U32 * stuWordPtr
)
{
    SOHO_DEV_MEM_INFO * memInfoPtr;         /* device's memory pointer */
    GT_U32 stuOffset = sid + 1, stuSize;    /* STU table memory offset */
    SMEM_REGISTER * stuTblMemPtr;          /* STU table memory pointer */
    GT_32 index;
    GT_U32 validBit;

    /* mask the vtu offset */
    stuOffset = stuOffset & 0xFFF;
    if (sid == 0xfff)
    {
        stuOffset = 0 ;
    }

    /* Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);
    stuTblMemPtr = memInfoPtr->stuDbMem.stuTblMem;

    stuSize = memInfoPtr->stuDbMem.stuTblMemSize / OPAL_STU_ENTRY_BYTES;
    if (stuOffset >= stuSize)
    {
        skernelFatalError("Wrong STU table address %X, exceed maximal \
                          address %X",
                          stuOffset,
                          memInfoPtr->stuDbMem.stuTblMemSize);
    }

    while (stuOffset <  stuSize)
    {
        index = stuOffset * OPAL_STU_ENTRY_WORDS;

        validBit = SMEM_U32_GET_FIELD(stuTblMemPtr[index], 6, 1);
        if ( validBit )
        {
            memcpy(stuWordPtr, &stuTblMemPtr[index],
                    OPAL_STU_ENTRY_WORDS * sizeof(GT_U32));

            return GT_OK;
        }
        if (stuOffset != 0xfff)
        {
            stuOffset++;
        }
    }

    /* End of the VLAN table was reached with no new valid entries */
    return GT_NOT_FOUND;
}

/*******************************************************************************
*  smemSohoActiveReadGlobalStat
*
* DESCRIPTION:
*      Provides read from the global status register
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter -
*                      global interrupt bit number.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemSohoActiveReadGlobalStat (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{

    *outMemPtr = *memPtr;

    /* Clear all ROC fields */
    *memPtr &= (~(1 << 0));
    *memPtr &= (~(1 << 2));
    *memPtr &= (~(1 << 4));
    *memPtr &= (~(1 << 6));
}


/*******************************************************************************
*  smemSohoActiveWriteForceLinkDown
*
* DESCRIPTION:
*      Write Message to the main task - Link change on port.
*
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memSize     - size of the requested memory
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*       The function invoked when the linkdown bit (0) on
*       port<n> Auto-Negotiation has been changed.
*******************************************************************************/
static void smemSohoActiveWriteForceLinkDown (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32 fieldValLinkStatus;    /* LinkDown flag */
    GT_U32 currLinkState=0;          /* field value bit '0' in port Auto-Negotiation*/
    GT_U32 port_no;             /* port number */
    GT_U32 addressOfLinkStatus; /* current status of link */
    GT_U8  * dataPtr;           /* pointer to the data in the buffer */
    GT_U32 dataSize;            /* data size */
    SBUF_BUF_ID bufferId;       /* buffer */
    GT_BOOL doForceLinkChange = GT_FALSE;


    *memPtr = *inMemPtr ;

    if(*inMemPtr & (1 << 4))
    {
        doForceLinkChange = GT_TRUE;
    }

    /* find the port number */
    port_no =   ( (address >> 0x10 ) & 0xF );
    /* find the current state of the link */
    addressOfLinkStatus = SWITCH_PORT0_STATUS_REG + (port_no * 0x10000);
    smemRegFldGet(devObjPtr, addressOfLinkStatus, 11, 1, &currLinkState);

    if (doForceLinkChange == GT_FALSE)
    {
       fieldValLinkStatus = 1;
    }
    else
    {
        /* New Message Trigger - link change bit has been changed */
        fieldValLinkStatus = SMEM_U32_GET_FIELD(inMemPtr[0], 5, 1);
    }

    /* check if there was a change in the link state.if no - return from func.*/
    if (currLinkState == fieldValLinkStatus)
            return;

    /* Get buffer      */
    bufferId = sbufAlloc(devObjPtr->bufPool, SMEM_SOHO_LINK_FORCE_MSG_SIZE);
    if (bufferId == NULL)
    {
       printf(" smemSohoActiveWriteForceLinkDown : no buffers for  \
                 force link bit \n");
       return;
    }

    /* Get actual data pointer */
    sbufDataGet(bufferId, (GT_U8 **)&dataPtr, &dataSize);
    /* copy MAC table action 1 word to buffer  */
    memcpy(dataPtr, &port_no , sizeof(GT_U32) );
    dataPtr++;
    memcpy(dataPtr, &fieldValLinkStatus , sizeof(GT_U32) );

    /* set source type of buffer                    */
    bufferId->srcType = SMAIN_SRC_TYPE_CPU_E;

    /* set message type of buffer                   */
    bufferId->dataType = SMAIN_LINK_CHG_MSG_E;

    /* put buffer to queue                          */
    squeBufPut(devObjPtr->queueId, SIM_CAST_BUFF(bufferId));
}

/*******************************************************************************
*  smemSohoActiveReadPhyInterruptStat
*
* DESCRIPTION:
*      Provides read from the Phy Interrupt status register
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter -
*                      global interrupt bit number.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemSohoActiveReadPhyInterruptStat (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U8      phyNum ;
    GT_U32     *dataPtr ;

    *outMemPtr = *memPtr;

    /* Clear all bits of Phy Interupt status */
    *memPtr = 0;

    phyNum = (GT_U8)((address >> 16) & 0xFF) ;

    /* clear Phy interrupt port summary port-bit */
    dataPtr = (GT_U32*)smemMemGet(deviceObjPtr, PHY_INTERRUPT_PORT_SUM_REG) ;
    *dataPtr &= (~ (1 << phyNum)) ;

    /* clear global interrupt status cause bit1 */
    dataPtr = (GT_U32*)smemMemGet(deviceObjPtr, GLB_STATUS_REG) ;
    *dataPtr &= (~ (1 << 1)) ;

}

/*******************************************************************************
*  smemSohoActiveWriteAtuOp
*
* DESCRIPTION:
*      Provides CPU interface for the operations on the ATU
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemSohoActiveWriteAtuOp (
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN  GT_U32   address,
    IN  GT_U32   memSize,
    IN  GT_U32 * memPtr,
    IN  GT_UINTPTR   param,
    IN  GT_U32 * inMemPtr
)
{
    SBUF_BUF_ID             bufferId;   /* buffer */
    GT_U8                *  dataPtr;    /* pointer to the data in the buffer */
    GT_U32                  dataSize;   /* data size */
    GT_U32               * tmpAtuControlPtr ;

    *memPtr = *inMemPtr;

    /* send message to SKernel task to process it. */
    /* get buffer */
    bufferId = sbufAlloc(devObjPtr->bufPool,
                         SMEM_SOHO_ATU_MSG_WORDS_NUM * sizeof(GT_U32));

    if (bufferId == NULL)
    {
        printf(" smemSohoActiveWriteAtuOp: "\
                "no buffers to update MAC table\n");
        return;
    }

    /* get actual data pointer */
    sbufDataGet(bufferId, &dataPtr, &dataSize);

    tmpAtuControlPtr = (GT_U32*)dataPtr + (5);
    /* copy ATU update message to buffer */
    memcpy(dataPtr, (GT_U8 *)memPtr,
           (SMEM_SOHO_ATU_MSG_WORDS_NUM -1) * sizeof(GT_U32));

    /* set the word 5 from the info of the ATU control register ,
      Offset 0x0A
      this is needed for the Opal,Jade that support dbNum up to 255
      */
    smemRegGet(devObjPtr,GLB_ATU_CTRL_REG ,tmpAtuControlPtr);

    tmpAtuControlPtr = (GT_U32*)dataPtr + (6);
    /* set the word 6 from the info of the ATU FID register ,
      Offset 0x01
      this is needed for the Opal+
      */
    smemRegGet(devObjPtr,GLB_ATU_FID_REG ,tmpAtuControlPtr);

    /* set source type of buffer */
    bufferId->srcType = SMAIN_SRC_TYPE_CPU_E;

    /* set message type of buffer */
    bufferId->dataType = SMAIN_MSG_TYPE_FDB_UPDATE_E;

    /* Set ATU Busy */
    SMEM_U32_SET_FIELD(memPtr[0], 15, 1, 1);

    /* put buffer to queue */
    squeBufPut(devObjPtr->queueId, SIM_CAST_BUFF(bufferId));

    /* make minimal sleep to give CPU time for
    the Simulation task for message processing*/
    SIM_OS_MAC(simOsSleep)(1);
}

/*******************************************************************************
*  smemSohoActiveWriteTrunkMask
*
* DESCRIPTION:
*      Provides CPU interface for the Trunk Mask register
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemSohoActiveWriteTrunkMask(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN  GT_U32   address,
    IN  GT_U32   memSize,
    IN  GT_U32 * memPtr,
    IN  GT_UINTPTR   param,
    IN  GT_U32 * inMemPtr
)
{
    SOHO_DEV_MEM_INFO * memInfoPtr; /* device's memory pointer */
    GT_U32 entryIdx;                /* trunk mask index */
    SMEM_REGISTER   * trunkMaskPtr; /* trunk mask pointer */
    GT_BOOL         update;

    *memPtr = *inMemPtr;

    /* Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);

    update = SMEM_U32_GET_FIELD(memPtr[0], 15, 1);

    entryIdx = SMEM_U32_GET_FIELD(memPtr[0], 12, 3);
    trunkMaskPtr = &memInfoPtr->trunkMaskMem.trunkTblMem[entryIdx];

    if(update == 1)
    {
        *trunkMaskPtr = SMEM_U32_GET_FIELD(memPtr[0], 0, 11);
    }
    else
    {
        /* set the value needed for read */
        memInfoPtr->trunkMaskMem.readRegVal = *trunkMaskPtr;
    }

    /* Clear update bit */
    SMEM_U32_SET_FIELD(memPtr[0], 15, 1, 0);
}

/*******************************************************************************
*  smemSohoActiveReadTrunkMask
*
* DESCRIPTION:
*      Provides CPU interface for the Trunk mask memory
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemSohoActiveReadTrunkMask(
    IN   SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN   GT_U32   address,
    IN   GT_U32   memSize,
    IN   GT_U32 * memPtr,
    IN   GT_UINTPTR   param,
    OUT  GT_U32 * outMemPtr
)
{
    SOHO_DEV_MEM_INFO * memInfoPtr; /* device's memory pointer */

    /* Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);

    /* get the value */
    *outMemPtr = memInfoPtr->trunkMaskMem.readRegVal;
}


/*******************************************************************************
*  smemSohoActiveWriteTrunkRout
*
* DESCRIPTION:
*      Provides CPU interface for the Trunk routing memory
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemSohoActiveWriteTrunkRout(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN  GT_U32   address,
    IN  GT_U32   memSize,
    IN  GT_U32 * memPtr,
    IN  GT_UINTPTR   param,
    IN  GT_U32 * inMemPtr
)
{
    SOHO_DEV_MEM_INFO * memInfoPtr; /* device's memory pointer */
    GT_U32 trunkIdx;                /* trunk rout index */
    SMEM_REGISTER   * trunkRoutPtr; /* trunk rout pointer */
    GT_BOOL         update;

    *memPtr = *inMemPtr;

    /* Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);

    update = SMEM_U32_GET_FIELD(memPtr[0], 15, 1);
    trunkIdx = SMEM_U32_GET_FIELD(memPtr[0], 11, 4);
    trunkRoutPtr = &memInfoPtr->trunkRouteMem.trouteTblMem[trunkIdx];

    if(update == 1)
    {
        *trunkRoutPtr = (SMEM_U32_GET_FIELD(memPtr[0], 0, 11));
    }
    else
    {
        /* set the value needed for read */
        memInfoPtr->trunkRouteMem.readRegVal = *trunkRoutPtr;
    }

    /* Clear update bit */
    SMEM_U32_SET_FIELD(memPtr[0], 15, 1, 0);
}

/*******************************************************************************
*  smemSohoActiveReadTrunkRout
*
* DESCRIPTION:
*      Provides CPU interface for the Trunk routing memory
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemSohoActiveReadTrunkRout(
    IN   SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN   GT_U32   address,
    IN   GT_U32   memSize,
    IN   GT_U32 * memPtr,
    IN   GT_UINTPTR   param,
    OUT  GT_U32 * outMemPtr
)
{
    SOHO_DEV_MEM_INFO * memInfoPtr; /* device's memory pointer */

    /* Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);

    /* get the value */
    *outMemPtr = memInfoPtr->trunkRouteMem.readRegVal;
}

/*******************************************************************************
*  smemSohoActiveWriteFlowCtrlDelay
*
* DESCRIPTION:
*      Provides CPU interface for the Flow Control Delay Memory
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemSohoActiveWriteFlowCtrlDelay(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN  GT_U32   address,
    IN  GT_U32   memSize,
    IN  GT_U32 * memPtr,
    IN  GT_UINTPTR   param,
    IN  GT_U32 * inMemPtr
)
{
    SOHO_DEV_MEM_INFO * memInfoPtr;     /* device's memory pointer */
    SOHO_PORT_SPEED_MODE_E spd;         /* speed Number */
    GT_U32 delay;                       /* delay time */

    *memPtr = *inMemPtr;

    /* Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);

    spd = SMEM_U32_GET_FIELD(memPtr[0], 13, 2);
    delay = SMEM_U32_GET_FIELD(memPtr[0], 0, 13);

    /* Set flow control port delay */
    memInfoPtr->flowCtrlDelayMem.fcDelayMem[spd] = delay;

    /* Clear update bit */
    SMEM_U32_SET_FIELD(memPtr[0], 15, 1, 0);

}

/*******************************************************************************
*  smemSohoActiveWriteTrgDevice
*
* DESCRIPTION:
*      Provides CPU interface for the Target Device memory
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemSohoActiveWriteTrgDevice(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN  GT_U32   address,
    IN  GT_U32   memSize,
    IN  GT_U32 * memPtr,
    IN  GT_UINTPTR   param,
    IN  GT_U32 * inMemPtr
)
{
    SOHO_DEV_MEM_INFO * memInfoPtr; /* device's memory pointer */
    GT_U32 deviceIdx;               /* target device index */
    SMEM_REGISTER   * devMemPtr;    /* target device memory pointer */
    GT_BIT update;

    *memPtr = *inMemPtr;

    /* Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);

    update = SMEM_U32_GET_FIELD(memPtr[0], 15, 1);

    deviceIdx = SMEM_U32_GET_FIELD(memPtr[0], 8, 5);
    devMemPtr = &memInfoPtr->trgDevMem.deviceTblMem[deviceIdx];
    if (update)
    {
        *devMemPtr = (SMEM_U32_GET_FIELD(memPtr[0], 8, 5) << 8) |
                     (SMEM_U32_GET_FIELD(memPtr[0], 0, 4));
    }
    else
    {
        *memPtr = *devMemPtr;
    }

    /* Clear update bit */
    SMEM_U32_SET_FIELD(memPtr[0], 15, 1, 0);
}

/*******************************************************************************
*  smemSohoActiveWriteGobalStatus
*
* DESCRIPTION:
*      Provides CPU interface for the Target Device memory
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemSohoActiveWriteGobalStatus(
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN  GT_U32   address,
    IN  GT_U32   memSize,
    IN  GT_U32 * memPtr,
    IN  GT_UINTPTR   param,
    IN  GT_U32 * inMemPtr
)
{
    GT_U32 ppuEn;                       /* PHY Polling Unit Enabled */

    *memPtr = *inMemPtr;

    /* Get PPU state */
    ppuEn = SMEM_U32_GET_FIELD(memPtr[0], 14, 1);

    if (ppuEn)
    {
        /* PPU is Active detecting and initializing external PHYs */
        smemRegFldSet(devObjPtr, GLB_STATUS_REG, 14, 2, 1);
    }
    else
    {
        /* PPU Disabled after Initialization */
        smemRegFldSet(devObjPtr, GLB_STATUS_REG, 14, 2, 2);
    }

    /* Clear update bit */
    SMEM_U32_SET_FIELD(memPtr[0], 15, 1, 0);
}

/*******************************************************************************
*  smemSohoGetNextVtuEntry
*
* DESCRIPTION:
*      Get VTU entry from vlan table SRAM
* INPUTS:
*       devObjPtr   - device object PTR.
*       vid         - vlan id being searched for
*
* OUTPUTS:
*       vtuWordPtr  - pointer to VTU entry in SRAM
*
* RETURNS:
*
* COMMENTS:
*******************************************************************************/
GT_STATUS smemSohoVtuEntryGet (
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN  GT_U32 vid,
    OUT SNET_SOHO_VTU_STC * vtuEntry
)
{
    GT_STATUS status;               /* return status value */
    GT_U32 vtuWord[SOHO_VLAN_ENTRY_WORDS];/* VTU database entry and pointer */
    GT_U32 portMapBits;

    ASSERT_PTR(vtuEntry);

    if (vid == 0)
    {
        return GT_NOT_FOUND;
    }

    portMapBits = SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType) ?  4 :  2;

    /* Perform search with value one less than the one being searched for  */
    status = smemSohoGetNextVtuEntry(devObjPtr, vid - 1, vtuWord);
    if (status == GT_OK)
    {


        if (!(SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType)))
        {
            vtuEntry->dbNum = SMEM_U32_GET_FIELD(vtuWord[0], 0, 4);
            vtuEntry->vid = SMEM_U32_GET_FIELD(vtuWord[0], 4, 12);
            vtuEntry->pri = SMEM_U32_GET_FIELD(vtuWord[0], 12, 4);
            /* Get ports bitmap */
            vtuEntry->portsMap[0] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[0], 16, portMapBits);
            vtuEntry->portsMap[1] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[0], 20, portMapBits);
            vtuEntry->portsMap[2] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[0], 24, portMapBits);
            vtuEntry->portsMap[3] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[0], 28, portMapBits);
            vtuEntry->portsMap[4] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[1],  0, portMapBits);
            vtuEntry->portsMap[5] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[1],  4, portMapBits);
            vtuEntry->portsMap[6] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[1],  8, portMapBits);
            vtuEntry->portsMap[7] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[1], 12, portMapBits);
            vtuEntry->portsMap[8] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[1], 16, portMapBits);
            vtuEntry->portsMap[9] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[1], 20, portMapBits);
            /* Ruby */
            if ((devObjPtr->deviceType == SKERNEL_RUBY) ||
                (devObjPtr->deviceType == SKERNEL_OPAL))
            {
                vtuEntry->portsMap[10] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[1], 24, portMapBits);

            }
        }
        else
        {
            vtuEntry->dbNum = SMEM_U32_GET_FIELD(vtuWord[0], 0, 12);
            vtuEntry->vidPolicy = SMEM_U32_GET_FIELD(vtuWord[0], 12, 1);
            vtuEntry->sid = SMEM_U32_GET_FIELD(vtuWord[0],13, 6);
            vtuEntry->vid = SMEM_U32_GET_FIELD(vtuWord[0], 19, 12);
            vtuEntry->valid = SMEM_U32_GET_FIELD(vtuWord[0], 31, 1);

            /* Get ports bitmap */
            vtuEntry->portsMap[0] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[1], 0, portMapBits);
            vtuEntry->portsMap[1] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[1], 4, portMapBits);
            vtuEntry->portsMap[2] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[1], 8, portMapBits);
            vtuEntry->portsMap[3] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[1], 12, portMapBits);
            vtuEntry->portsMap[4] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[1],  16, portMapBits);
            vtuEntry->portsMap[5] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[1],  20, portMapBits);
            vtuEntry->portsMap[6] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[1],  24, portMapBits);
            vtuEntry->portsMap[7] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[1], 28, portMapBits);
            vtuEntry->portsMap[8] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[2], 0, portMapBits);
            vtuEntry->portsMap[9] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[2], 4, portMapBits);
            vtuEntry->portsMap[10] =
                (GT_U8)SMEM_U32_GET_FIELD(vtuWord[2], 8, portMapBits);

            vtuEntry->pri = SMEM_U32_GET_FIELD(vtuWord[2], 12, 3);
            vtuEntry->usepri = SMEM_U32_GET_FIELD(vtuWord[2], 15, 1);
        }
    }

    return status;
}

