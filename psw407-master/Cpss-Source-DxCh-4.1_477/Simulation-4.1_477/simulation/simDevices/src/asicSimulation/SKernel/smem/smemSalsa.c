/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemSalsa.c
*
* DESCRIPTION:
*       This is API implementation for Salsa memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 14 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/smem/smemSalsa.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>


static GT_U32 *  smemSalsaFatalError(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemSalsaGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemSalsaTransQueReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static GT_U32 *  smemSalsaEtherBrdgReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemSalsaBufMngReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemSalsaPortGroupConfReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemSalsaMacTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemSalsaInterBufBankReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemSalsaBufMemoryConfReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemSalsaVlanTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static void smemSalsaInitFuncArray(
    INOUT SALSA_DEV_MEM_INFO  * devMemInfoPtr
);
static void smemSalsaAllocSpecMemory(
    INOUT SALSA_DEV_MEM_INFO  * devMemInfoPtr
);
static void smemSalsaActiveReadIntReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static void smemSalsaActiveWriteFdbMsg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static void smemSalsaActiveReadMsg2Cpu (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

static void smemSalsaActiveWriteSmii (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

static void smemSalsaActiveReadCntrs (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
);

/* Private definition */
#define     GLOB_REGS_NUM               (0xB8 / 4 + 1)
#define     TWSI_INT_REGS_NUM           (0x1C / 4 + 1)
#define     GEN_REGS_NUM                (0x17C/ 4 + 1)
#define     CTRL_REGS_NUM               2048
#define     MAC_PORTS_NUM               24
#define     PER_PORT_TYPES_NUM          (0x14 / 4 + 1)
#define     GROUPS_NUM                  (0x4)
#define     PER_GROUPS_TYPE_NUM         (0x120)
#define     MAC_CNT_TYPES_NUM           (0x7C / 4 + 1)
#define     BRG_PORTS_NUM               25
#define     PORT_REGS_GROUPS_NUM        (0x10 / 4 + 1)
#define     PORT_PROT_VID_NUM           2
#define     BRG_GEN_REGS_NUM            (0x27C / 4 + 1)
#define     EGRS_GEN_REGS_NUM           (0x1FFFF / 4 + 1)
#define     STACK_CFG_REG_NUM           (0x1C / 4 + 1)
#define     TX_QUE_REG_NUM              (0x1C / 4 + 1)
#define     BUF_MEM_REGS_NUM            3
#define     MAC_REGS_NUM                (0x2FF /4 + 1)
#define     MAC_TBL_REGS_NUM            (8 * 1024 * 4)
#define     MAC_UPD_FIFO_REGS_NUM       (4 * 16)
#define     VLAN_REGS_NUM               2048
#define     VLAN_TABLE_REGS_NUM         (256 * 4)
#define     STP_TABLE_REGS_NUM          (32 * 2)
#define     BANKS_REGS_NUM              (0x8000)

/* Register special function index in function array (Bits 23:28)*/
#define     REG_SPEC_FUNC_INDEX         0x1F800000

/* global interrup register */
#define     SMEM_SALSA_GLOB_INTR_REG_ADDR_CNS 0x00000030

/* number of words in the FDB update message */
#define     SMEM_SALSA_FDB_UPD_MSG_WORDS_NUM    4

/* invalid MAC message  */
#define     SMEM_SALSA_INVALID_MAC_MSG_CNS    0xffffffff

/* MAC Counters address mask */
#define     SMEM_COUNT_ADDR_MASK_CNS    0xfe7ffc00

/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemSalsaActiveTable[] =
{
    /* GOP<n> Interrupt Cause Register0*/
    {0x04004000, SMEM_FULL_MASK_CNS, smemSalsaActiveReadIntReg, 7 , NULL,0},
    {0x04804000, SMEM_FULL_MASK_CNS, smemSalsaActiveReadIntReg, 9 , NULL,0},
    {0x05004000, SMEM_FULL_MASK_CNS, smemSalsaActiveReadIntReg, 11 , NULL,0},
    {0x05804000, SMEM_FULL_MASK_CNS, smemSalsaActiveReadIntReg, 13 , NULL,0},

    /* MAC Table Interrupt Cause Register */
    {0x06000018, SMEM_FULL_MASK_CNS, smemSalsaActiveReadIntReg, 6 , NULL,0},

    /* Message from CPU Register3 */
    {0x0600004C, SMEM_FULL_MASK_CNS, NULL, 0 , smemSalsaActiveWriteFdbMsg,0},

    /* Message to CPU register  */
    {0x06000034, SMEM_FULL_MASK_CNS, smemSalsaActiveReadMsg2Cpu, 0 , NULL,0},

    /* SMI0 Management Register */
    {0x04004054, SMEM_FULL_MASK_CNS, NULL, 0 , smemSalsaActiveWriteSmii,0},

    /* SMI1 Management Register */
    {0x05004054, SMEM_FULL_MASK_CNS, NULL, 0 , smemSalsaActiveWriteSmii,0},

    /* MAC counters */
    {0x04010000, SMEM_COUNT_ADDR_MASK_CNS, smemSalsaActiveReadCntrs, 0, NULL,0},

    /* must be last anyway */
    {0xffffffff, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};

#define SMEM_ACTIVE_MEM_TABLE_SIZE \
    (sizeof(smemSalsaActiveTable)/sizeof(smemSalsaActiveTable[0]))

/*******************************************************************************
*   smemSalsaInit
*
* DESCRIPTION:
*       Init memory module for a Salsa device.
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
void smemSalsaInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    SALSA_DEV_MEM_INFO  * devMemInfoPtr;

    deviceObjPtr->notSupportPciConfigMemory = 1;/* the device not support PCI/PEX configuration memory space */

    /* alloc SALSA_DEV_MEM_INFO */
    devMemInfoPtr = (SALSA_DEV_MEM_INFO *)calloc(1, sizeof(SALSA_DEV_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
        skernelFatalError("smemSalsaInit: allocation error\n");
    }

    /* init specific functions array */
    smemSalsaInitFuncArray(devMemInfoPtr);

    /* allocate address type specific memories */
    smemSalsaAllocSpecMemory(devMemInfoPtr);

    deviceObjPtr->devFindMemFunPtr = (void *)smemSalsaFindMem;
    deviceObjPtr->deviceMemory = devMemInfoPtr;
}
/*******************************************************************************
*   smemSalsaFindMem
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
void * smemSalsaFindMem
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    void                * memPtr;
    SALSA_DEV_MEM_INFO  * devMemInfoPtr;
    GT_32               index;
    GT_U32              param;

    if (deviceObjPtr == 0)
    {
        skernelFatalError("smemSalsaFindMem: illegal pointer \n");
    }
    memPtr = 0;
    devMemInfoPtr = (SALSA_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    index = (address & REG_SPEC_FUNC_INDEX) >> 23;
    if (index > 63)
    {
        skernelFatalError("smemSalsaFindMem: index is out of range\n");
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
            /* check address */
            if ((address & smemSalsaActiveTable[index].mask)
                 == smemSalsaActiveTable[index].address)
                *activeMemPtrPtr = &smemSalsaActiveTable[index];
        }
    }

    return memPtr;
}

/*******************************************************************************
*   smemSalsaGlobalReg
*
* DESCRIPTION:
*       Global, TWSI, GPP & CPU Port Configuration Registers.
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
static GT_U32 *  smemSalsaGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SALSA_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (SALSA_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Global registers */
    if ((address & 0xFFFFFF00) == 0x0){
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.globRegs,
                         devMemInfoPtr->globalMem.globRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.globRegs[index];
    }else
     /* TWSI registers */
    if ((address & 0xFFFFFF00) == 0x00400000) {
        index = (address & 0x1f) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.twsiIntRegs,
                         devMemInfoPtr->globalMem.twsiIntRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.twsiIntRegs[index];
    }

    return regValPtr;
}
/*******************************************************************************
*   smemSalsaTransQueReg
*
* DESCRIPTION:
*       Egress, Transmit (Tx) Queue and VLAN Table (VLT) Configuration Registers
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
static GT_U32 *  smemSalsaTransQueReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SALSA_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (SALSA_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* General registers */
    if ((address & 0xE07E0000) == 0x0){
        index = (address & 0x1FFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egressMem.genRegs,
                         devMemInfoPtr->egressMem.genRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egressMem.genRegs[index];
    }
    else
    /* Stack config registers */
    if ((address & 0xE07F0000) == 0x00040000) {
        index = (address & 0x1f) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egressMem.stackCfgRegs,
                         devMemInfoPtr->egressMem.stackCfgRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egressMem.stackCfgRegs[index];
    }
    else
    /*  tx queu regs*/
    if ((address & 0xE07F0000) == 0x00050000) {
        index = (address & 0x1f) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egressMem.txQueRegs,
                         devMemInfoPtr->egressMem.txQueRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egressMem.txQueRegs[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemSalsaEtherBrdgReg
*
* DESCRIPTION:
*       Bridge Configuration Registers
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
static GT_U32 *  smemSalsaEtherBrdgReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SALSA_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index, port;

    regValPtr = 0;
    devMemInfoPtr = (SALSA_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Port Bridge control registers */
    if ((address & 0xE07E0F00) == 0x0){
        port = (address & 0x1f000) >> 12;
        index = ((address & 0x1f) / 0x4 * BRG_PORTS_NUM) + port;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bridgeMngMem.portRegs,
                         devMemInfoPtr->bridgeMngMem.portRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bridgeMngMem.portRegs[index];
    }
    else
    /* Ports Protocol Based VLANs configuration Registers */
    if ((address & 0xE07E0800) == 0x00000800) {
        port = (address & 0x1f000) >> 12;
        index = (((address & 0xff) / 0x4) * BRG_PORTS_NUM) + port;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bridgeMngMem.portProtVidRegs,
                         devMemInfoPtr->bridgeMngMem.portProtVidRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bridgeMngMem.portProtVidRegs[index];
    }
    else
    /* General Bridge managment registers array */
    if ((address & 0xE07F0000) == 0x00040000) {
        index = (address & 0x3ff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bridgeMngMem.genRegs,
                         devMemInfoPtr->bridgeMngMem.genRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bridgeMngMem.genRegs[index];
    }
    return regValPtr;
}

/*******************************************************************************
*   smemSalsaBufMngReg
*
* DESCRIPTION:
*       Describe a device's buffer managment registers memory object
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
static GT_U32 *  smemSalsaBufMngReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SALSA_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (SALSA_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* General buffers managment registers array */
    if ((address & 0xFFFFFE00) == 0x03000000){
        index = (address & 0x1ff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.genRegs,
                         devMemInfoPtr->bufMngMem.genRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.genRegs[index];
    }
    else
    /* Control Linked List buffers managment registers array */
    if ((address & 0x00003000) > 0x0) {
        index = (address & 0xfff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.cntrlRegs,
                         devMemInfoPtr->bufMngMem.cntrlRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.cntrlRegs[index];
    }
    return regValPtr;
}

/*******************************************************************************
*   smemSalsaPortGroupConfReg
*
* DESCRIPTION:
*       Describe a device's Port registers memory object
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
static GT_U32 *  smemSalsaPortGroupConfReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SALSA_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;
    GT_U32              typeNum, portNum;
    GT_U32              groupNum;

    regValPtr = 0;
    devMemInfoPtr = (SALSA_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Per port registres */
    if ((address & 0xE07FF800) == 0x00000000){
        portNum  = param + ((address & 0xF00) >> 8);
        typeNum  = (address & 0xff) / 0x4;
        index    = (typeNum * MAC_PORTS_NUM) + portNum;
        CHECK_MEM_BOUNDS(devMemInfoPtr->portMem.perPortRegs,
                         devMemInfoPtr->portMem.perPortRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->portMem.perPortRegs[index];
    }
    else
    /* Per group registres */
    if ((address & 0xE07F4000) == 0x00004000) {
        /* Translate group number to zero base number (from 0-3),
          parameter is 0, 6, 12, 18 for different GOP */
        groupNum =  param / 6;
        typeNum = (address & 0xfff) / 0x4;
        index    = (groupNum * PER_GROUPS_TYPE_NUM) + typeNum;
        CHECK_MEM_BOUNDS(devMemInfoPtr->portMem.perGroupRegs,
                         devMemInfoPtr->portMem.perGroupRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->portMem.perGroupRegs[index];
    }
    else
    /* MAC counters array */
    if ((address & 0xE071F000) == 0x00010000) {
        index = ((address & 0x3ff) / 0x4) + (param * MAC_CNT_TYPES_NUM)
        + (param / 6) * MAC_CNT_TYPES_NUM;
        CHECK_MEM_BOUNDS(devMemInfoPtr->portMem.macCnts,
                         devMemInfoPtr->portMem.macCntsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->portMem.macCnts[index];
    }
    return regValPtr;
}

/*******************************************************************************
*   smemSalsaMacTableReg
*
* DESCRIPTION:
*       Describe a device's Bridge registers and FDB
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
static GT_U32 *  smemSalsaMacTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SALSA_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (SALSA_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Registers, index is group number */
    if ((address & 0xFFFFF000) == 0x06000000){
        index = (address & 0xfff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->macFbdMem.regs,
                         devMemInfoPtr->macFbdMem.regsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->macFbdMem.regs[index];
    }
    else
    /* MAC Table Registers */
    if ((address & 0xFFF00000) == 0x06100000) {
        index = (address & 0x3ffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->macFbdMem.macTblRegs,
                         devMemInfoPtr->macFbdMem.macTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->macFbdMem.macTblRegs[index];
    }
    return regValPtr;
}

/*******************************************************************************
*   smemSalsaInterBufBankReg
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
static GT_U32 *  smemSalsaInterBufBankReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SALSA_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (SALSA_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    index = (address & 0x7fff);

    if (param == 0)
    {
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufBanksMem.bank0Regs,
                         devMemInfoPtr->bufBanksMem.bank0RegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufBanksMem.bank0Regs[index];
    }
    else
    {
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufBanksMem.bank1Regs,
                         devMemInfoPtr->bufBanksMem.bank1RegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufBanksMem.bank1Regs[index];
    }
    return regValPtr;
}

/*******************************************************************************
*   smemSalsaBufMemoryConfReg
*
* DESCRIPTION:
*       Registers, index is group number
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
static GT_U32 *  smemSalsaBufMemoryConfReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SALSA_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (SALSA_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Registers, index is group number */
    if ((address & 0xE07FFFF0) == 0x00000000){
        index = (address & 0xf) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMem.regs,
                         devMemInfoPtr->bufMem.regsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMem.regs[index];
    }
    return regValPtr;
}

/*******************************************************************************
*   smemSalsaVlanTableReg
*
* DESCRIPTION:
*       Describe a device's Bridge registers and FDB
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
static GT_U32 *  smemSalsaVlanTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SALSA_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (SALSA_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* VLT Registers */
    if ((address & 0x0000FFFF) < 0x2000){
        index = (address & 0x1fff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->vlanTblMem.vltRegs,
                         devMemInfoPtr->vlanTblMem.vltRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->vlanTblMem.vltRegs[index];
    }
    else
    /* Vlan Table Registers */
    if ((address & 0x0000FFFF) >= 0x2000 &&
        (address & 0x0000FFFF) < 0x3000){
        index = (address & 0xfff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->vlanTblMem.vlanTblRegs,
                         devMemInfoPtr->vlanTblMem.vlanTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->vlanTblMem.vlanTblRegs[index];
    }
    else
    /* Registers with address mask 0x0000FFFF 0x4000 <= pattern < 0x4100 */
    if ((address & 0x0000FFFF) >= 0x4000 &&
        (address & 0x0000FFFF) < 0x4100){
        index = (address & 0xff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->vlanTblMem.mcastTblRegs,
                         devMemInfoPtr->vlanTblMem.mcastTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->vlanTblMem.mcastTblRegs[index];
    }
    else
    /* Registers with address mask 0x0000FFFF 0x6000 <= pattern < 0x6100 */
    if ((address & 0x0000FFFF) >= 0x6000 &&
        (address & 0x0000FFFF) < 0x6100){
        index = (address & 0xff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->vlanTblMem.stpTblRegs,
                         devMemInfoPtr->vlanTblMem.stpTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->vlanTblMem.stpTblRegs[index];
    }
    return regValPtr;
}

static GT_U32 *  smemSalsaFatalError(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    skernelFatalError("smemSalsaFatalError: illegal function pointer\n");

    return 0;
}
/*******************************************************************************
*   smemSalsaInitMemArray
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
static void smemSalsaInitFuncArray(
    INOUT SALSA_DEV_MEM_INFO  * devMemInfoPtr
)
{
    GT_U32              i;

    for (i = 0; i < 64; i++) {
        devMemInfoPtr->specFunTbl[i].specFun    = smemSalsaFatalError;
    }
    devMemInfoPtr->specFunTbl[0].specFun        = smemSalsaGlobalReg;

    devMemInfoPtr->specFunTbl[3].specFun        = smemSalsaTransQueReg;

    devMemInfoPtr->specFunTbl[4].specFun        = smemSalsaEtherBrdgReg;

    devMemInfoPtr->specFunTbl[6].specFun        = smemSalsaBufMngReg;

    devMemInfoPtr->specFunTbl[8].specFun        = smemSalsaPortGroupConfReg;
    devMemInfoPtr->specFunTbl[8].specParam      = 0;

    devMemInfoPtr->specFunTbl[9].specFun        = smemSalsaPortGroupConfReg;
    devMemInfoPtr->specFunTbl[9].specParam      = 6;

    devMemInfoPtr->specFunTbl[10].specFun       = smemSalsaPortGroupConfReg;
    devMemInfoPtr->specFunTbl[10].specParam     = 12;

    devMemInfoPtr->specFunTbl[11].specFun       = smemSalsaPortGroupConfReg;
    devMemInfoPtr->specFunTbl[11].specParam     = 18;

    devMemInfoPtr->specFunTbl[12].specFun       = smemSalsaMacTableReg;

    devMemInfoPtr->specFunTbl[13].specFun       = smemSalsaInterBufBankReg;
    devMemInfoPtr->specFunTbl[13].specParam     = 0;

    devMemInfoPtr->specFunTbl[14].specFun       = smemSalsaInterBufBankReg;
    devMemInfoPtr->specFunTbl[14].specParam     = 1;

    devMemInfoPtr->specFunTbl[15].specFun       = smemSalsaBufMemoryConfReg;

    devMemInfoPtr->specFunTbl[20].specFun       = smemSalsaVlanTableReg;
}
/*******************************************************************************
*   smemSalsaAllocSpecMemory
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
static void smemSalsaAllocSpecMemory(
    INOUT SALSA_DEV_MEM_INFO  * devMemInfoPtr
)
{
    /* Global register memory allocation */
    devMemInfoPtr->globalMem.globRegsNum = GLOB_REGS_NUM;
    devMemInfoPtr->globalMem.globRegs =
            calloc(GLOB_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.globRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->globalMem.twsiIntRegsNum = TWSI_INT_REGS_NUM;
    devMemInfoPtr->globalMem.twsiIntRegs =
        calloc(TWSI_INT_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.twsiIntRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    /* Buffer management register memory allocation */
    devMemInfoPtr->bufMngMem.genRegsNum = GEN_REGS_NUM;
    devMemInfoPtr->bufMngMem.genRegs =
        calloc(GEN_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.genRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->bufMngMem.cntrlRegsNum = CTRL_REGS_NUM;
    devMemInfoPtr->bufMngMem.cntrlRegs =
        calloc(CTRL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.cntrlRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    /* Network ports register memory allocation */
    devMemInfoPtr->portMem.macPortsNum = MAC_PORTS_NUM;
    devMemInfoPtr->portMem.perPortRegsNum = MAC_PORTS_NUM * PER_PORT_TYPES_NUM;
    devMemInfoPtr->portMem.perPortRegs =
        calloc((MAC_PORTS_NUM) * (PER_PORT_TYPES_NUM), sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->portMem.perPortRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->portMem.perGroupRegsNum = GROUPS_NUM * PER_GROUPS_TYPE_NUM;
    devMemInfoPtr->portMem.perGroupRegs =
        calloc((GROUPS_NUM) * (PER_GROUPS_TYPE_NUM), sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->portMem.perGroupRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    /* There are 24 MAC ports + 4 capture ports with MAC counters.*/
    devMemInfoPtr->portMem.macCntsNum = MAC_CNT_TYPES_NUM * (MAC_PORTS_NUM + 4);
    devMemInfoPtr->portMem.macCnts =
        calloc(devMemInfoPtr->portMem.macCntsNum, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->portMem.macCnts == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    /* Bridge configuration register memory allocation */
    devMemInfoPtr->bridgeMngMem.brgPortsNum = BRG_PORTS_NUM;
    devMemInfoPtr->bridgeMngMem.portRegsNum =
            BRG_PORTS_NUM * PORT_REGS_GROUPS_NUM;
    devMemInfoPtr->bridgeMngMem.portRegs =
        calloc((BRG_PORTS_NUM) * (PORT_REGS_GROUPS_NUM), sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bridgeMngMem.portRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->bridgeMngMem.portProtVidRegsNum =
            BRG_PORTS_NUM * PORT_PROT_VID_NUM;
    devMemInfoPtr->bridgeMngMem.portProtVidRegs =
        calloc((BRG_PORTS_NUM) * (PORT_PROT_VID_NUM), sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bridgeMngMem.portProtVidRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->bridgeMngMem.genRegsNum = BRG_GEN_REGS_NUM;
    devMemInfoPtr->bridgeMngMem.genRegs =
        calloc(BRG_GEN_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bridgeMngMem.genRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    /* Egress configuration register memory allocation */
    devMemInfoPtr->egressMem.genRegsNum = EGRS_GEN_REGS_NUM;
    devMemInfoPtr->egressMem.genRegs =
        calloc(EGRS_GEN_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.genRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egressMem.stackCfgRegNum = STACK_CFG_REG_NUM;
    devMemInfoPtr->egressMem.stackCfgRegs =
        calloc(STACK_CFG_REG_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.stackCfgRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egressMem.txQueRegNum = TX_QUE_REG_NUM;
    devMemInfoPtr->egressMem.txQueRegs =
        calloc(TX_QUE_REG_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.txQueRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    /* Buffers memory register memory allocation */
    devMemInfoPtr->bufMem.regsNum = BUF_MEM_REGS_NUM;
    devMemInfoPtr->bufMem.regs =
        calloc(BUF_MEM_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMem.regs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    /* MAC MIB counters register memory allocation */
    devMemInfoPtr->macFbdMem.regsNum = MAC_REGS_NUM;
    devMemInfoPtr->macFbdMem.regs =
        calloc(MAC_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macFbdMem.regs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->macFbdMem.macTblRegNum = MAC_TBL_REGS_NUM;
    devMemInfoPtr->macFbdMem.macTblRegs =
        calloc(MAC_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macFbdMem.macTblRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->macFbdMem.macUpdFifoRegsNum = MAC_UPD_FIFO_REGS_NUM;
    devMemInfoPtr->macFbdMem.macUpdFifoRegs =
        malloc(MAC_UPD_FIFO_REGS_NUM * sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macFbdMem.macUpdFifoRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }
    memset(devMemInfoPtr->macFbdMem.macUpdFifoRegs, 0xFF,
        MAC_UPD_FIFO_REGS_NUM * sizeof(SMEM_REGISTER));

    /* VLAN translation table register memory allocation */
    devMemInfoPtr->vlanTblMem.vltRegsNum = VLAN_REGS_NUM;
    devMemInfoPtr->vlanTblMem.vltRegs =
        calloc(VLAN_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->vlanTblMem.vltRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->vlanTblMem.vlanTblRegNum = VLAN_TABLE_REGS_NUM;
    devMemInfoPtr->vlanTblMem.vlanTblRegs =
        calloc(VLAN_TABLE_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->vlanTblMem.vlanTblRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->vlanTblMem.mcastTblRegNum = VLAN_TABLE_REGS_NUM;
    devMemInfoPtr->vlanTblMem.mcastTblRegs =
        calloc(VLAN_TABLE_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->vlanTblMem.mcastTblRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->vlanTblMem.stpTblRegNum = STP_TABLE_REGS_NUM;
    devMemInfoPtr->vlanTblMem.stpTblRegs =
        calloc(STP_TABLE_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->vlanTblMem.stpTblRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }
    /* Buffers memory address space Bank0 */
    devMemInfoPtr->bufBanksMem.bank0RegsNum = BANKS_REGS_NUM;
    devMemInfoPtr->bufBanksMem.bank0Regs =
        calloc(BANKS_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufBanksMem.bank0Regs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }
    /* Buffers memory address space Bank1 */
    devMemInfoPtr->bufBanksMem.bank1RegsNum = BANKS_REGS_NUM;
    devMemInfoPtr->bufBanksMem.bank1Regs =
        calloc(BANKS_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufBanksMem.bank1Regs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }

}
/*******************************************************************************
*  smemSalsaReadIntReg
*
* DESCRIPTION:
*      Definition of the Active register read function.
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
* COMMENTS:
*
*******************************************************************************/
static void smemSalsaActiveReadIntReg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U32 regData;

    /* copy registers content to the output memory */
    *outMemPtr = *memPtr;

    /* clear register */
    smemRegSet(deviceObjPtr,address,0);

    /* get global interrupt registers value */
    smemRegGet(deviceObjPtr, SMEM_SALSA_GLOB_INTR_REG_ADDR_CNS, &regData);

    /* clear correspondent bit in the global interrupt register */
    regData &= (~(1 << param));

    /* check enother causes existence */
    if ((regData & 0xfffffffe) == 0)
    {
        /* clear if not any cause of interrupt */
        regData = 0;
    }

    smemRegSet(deviceObjPtr, SMEM_SALSA_GLOB_INTR_REG_ADDR_CNS, regData);
}
/*******************************************************************************
*  smemSalsaActiveWriteFdbMsg
*
* DESCRIPTION:
*      Write to the Message from CPU Register3 - activate update FDB message.
* INPUTS:
*       deviceObjPtr - device object PTR.
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
static void smemSalsaActiveWriteFdbMsg (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    SBUF_BUF_ID             bufferId;    /* buffer */
    GT_U8                *  data_PTR;  /* pointer to the data in the buffer */
    GT_U32                  data_size;  /* data size */
    GT_U32               *  msgFromCpu0Addr; /* pointer to the memory of
                                               Message from CPU Register0*/
    *memPtr = *inMemPtr;

    /* check "activate" bit */
    if ((*memPtr) & 0x80000000)
    {
        /* send message to SKernel task to process it. */

        /* get buffer */
        bufferId = sbufAlloc(deviceObjPtr->bufPool,
                    SMEM_SALSA_FDB_UPD_MSG_WORDS_NUM * sizeof(GT_U32));

        if ( bufferId == NULL )
        {
            printf(" smemSalsaActiveWriteFdbMsg: "\
                    "no buffers to update MAC table\n");
            return;
}

        /* get actual data pointer */
        sbufDataGet(bufferId, &data_PTR, &data_size);

        /* find location of Message from CPU Register0 Offset: 0x06000040*/
        msgFromCpu0Addr = memPtr - 3;

        /* copy fdb update message to buffer */
        memcpy(data_PTR, (GT_U8_PTR)msgFromCpu0Addr,
                SMEM_SALSA_FDB_UPD_MSG_WORDS_NUM * sizeof(GT_U32));

        /* set source type of buffer */
        bufferId->srcType = SMAIN_SRC_TYPE_CPU_E;

        /* set message type of buffer */
        bufferId->dataType = SMAIN_MSG_TYPE_FDB_UPDATE_E;

        /* put buffer to queue */
        squeBufPut(deviceObjPtr->queueId,SIM_CAST_BUFF(bufferId));

        /* reset 31 bit */
        *memPtr = *memPtr & 0x7fffffff;
    }
}
/*******************************************************************************
*  smemSalsaActiveReadMsg2Cpu
*
* DESCRIPTION:
*      Read Message to CPU Register - simulate read from FDB FIFO.
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
* COMMENTS:
*
*******************************************************************************/
static void smemSalsaActiveReadMsg2Cpu (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    GT_U32              * fifoBufferPrt; /* pointer to MAC msg FIFO buffer */
    SALSA_DEV_MEM_INFO  * memInfoPtr; /* pointer to Salsa memory */
    GT_U32                i;

    /* get pointer to the start of FIFO for MAC update messages */
    memInfoPtr =  (SALSA_DEV_MEM_INFO *)(deviceObjPtr->deviceMemory);
    fifoBufferPrt = memInfoPtr->macFbdMem.macUpdFifoRegs;

    if (*fifoBufferPrt == SMEM_SALSA_INVALID_MAC_MSG_CNS)
    {
        /* buffer is empty, there is no new message */
        *outMemPtr = SMEM_SALSA_INVALID_MAC_MSG_CNS;
    }
    else
    {
        /* output value */
        *outMemPtr = *fifoBufferPrt;

        /* shift other words up */
        for (i = 1; i < MAC_UPD_FIFO_REGS_NUM; i++)
        {
            fifoBufferPrt[i - 1] = fifoBufferPrt[i];
        }

        /* invalidate last word */
        fifoBufferPrt[MAC_UPD_FIFO_REGS_NUM - 1] =
                                                SMEM_SALSA_INVALID_MAC_MSG_CNS;
    }
}
/*******************************************************************************
*  smemSalsaActiveWriteSmii
*
* DESCRIPTION:
*     Write to the SMI0 Management Register 0x04004054 - start SMII transaction.
* INPUTS:
*       deviceObjPtr - device object PTR.
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
static void smemSalsaActiveWriteSmii (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    /* copy content */
    *memPtr = * inMemPtr;

    /* set ReadValid bit */
    *memPtr |= (1 << 27);
}

/*******************************************************************************
*  smemSalsaActiveReadCounters
*
* DESCRIPTION:
*      Read counters.
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
* COMMENTS:
*         Output counter value and reset it.
*
*******************************************************************************/
static void smemSalsaActiveReadCntrs (
    IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    OUT        GT_U32 * outMemPtr
)
{
    /* output actual value of counter */
    * outMemPtr = * memPtr;

    /* reset counters value */
    * memPtr = 0;
}



