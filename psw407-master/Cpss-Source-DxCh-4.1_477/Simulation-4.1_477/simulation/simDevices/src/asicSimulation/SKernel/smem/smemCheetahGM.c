/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemCheetah.c
*
* DESCRIPTION:
*       This is API implementation for Cheetah memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 11 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/smem/smemCheetahGM.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>

/* NOTE : the SFDB_MAC_TBL_ACT_WORDS need to cover the mac table action
    registers starting at address 0x06000004 .
   we need data from registers :
   0x06000004 , 0x06000008 , 0x06000020

   that is why we need continue 8 words from the memory
   so  dataPtr[0] is 0x06000004
       dataPtr[1] is 0x06000008
       dataPtr[7] is 0x06000020
*/
#define SFDB_MAC_TBL_ACT_WORDS  8
#define SFDB_MAC_TBL_ACT_BYTES  (SFDB_MAC_TBL_ACT_WORDS * sizeof(GT_U32))

static void * smemChtGMFindMem
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
);

static GT_U32 *  smemChtGMFatalError(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemChtGMGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemChtGMTransQueReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static GT_U32 *  smemChtGMEtherBrdgReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemChtGMBufMngReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemChtGMPortGroupConfReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemChtGMMacTableReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemChtGMInterBufBankReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemChtGMBufMemoryConfReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemChtGMVlanTableReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemChtGMTriSpeedReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemChtGMPreEgressReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemChtGMPclReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemChtGMPolicerReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemChtGMPciReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemChtGMPhyReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize
);

static void smemChtGMInitFuncArray(
    INOUT SMEM_CHT_DEV_COMMON_MEM_INFO  * devMemInfoPtr
);

static void smemChtGMAllocSpecMemory(
    INOUT SMEM_CHT_DEV_MEM_INFO  * devMemInfoPtr
);

static GT_U32 *  smemChtGMPciReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static void smemChtGMSetPciDefault(
    IN SKERNEL_DEVICE_OBJECT    * devObjPtr,
    INOUT SMEM_CHT_DEV_MEM_INFO * devMemInfoPtr
);

/* Private definition */
#define     GLB_REGS_NUM                        (76)
#define     GLB_SDMA_REGS_NUM                   (1023)
#define     EGR_GLB_REGS_NUM                    (0xffff / 4)
#define     EGR_TRUNK_FILTER_REGS_NUM           (2032)
#define     EGR_PORT_WD_REGS_NUM                (64)
#define     EGR_MIB_CNT_REGS_NUM                (120)
#define     EGR_HYP_MIB_REGS_NUM                (60 + 28)
#define     EGR_XSMI_REGS_NUM                   (2)
#define     EGR_STC_TBL_REGS_NUM                (82)
#define     EGR_TALE_DROP_REGS_NUM              (3132)
#define     EGR_RATE_SHAPES_REGS_NUM            (64)
#define     BRDG_ETH_BRDG_REGS_NUM              (262655)
#define     BUF_MNG_REGS_NUM                    (196)
#define     GOP_CONF_REGS_NUM                   (512)
#define     GOP_MAC_MIB_REGS_NUM                (892)
#define     GOP_LED_REGS_NUM                    (16)
#define     MAC_FDB_REGS_NUM                    (124)
#define     MAC_TBL_REGS_NUM                    (16384 * 4)
#define     BANK_MEM_REGS_NUM                   (98304)
#define     BANK_WRITE_REGS_NUM                 (1)
#define     BUF_MEM_REGS_NUM                    (88)
#define     VTL_VLAN_CONF_REGS_NUM              (4)
#define     VTL_VLAN_TBL_REGS_NUM               (4096 * 3)
#define     VTL_MCST_TBL_REGS_NUM               (4096)
#define     VTL_STP_TBL_REGS_NUM                (256 * 2)
#define     TRI_SPEED_REGS_NUM                  (16383)
#define     PRE_EGR_PORTS_REGS_NUM              (22)
#define     PRE_EGR_GEN_REGS_NUM                (22)
#define     PRE_EGR_TRUNK_TBL_REGS_NUM          (128)
#define     PRE_QOS_TBL_REGS_NUM                (72 + 10)
#define     PRE_CPU_CODE_TBL_REGS_NUM           (256)
#define     PRE_STAT_RATE_TBL_REGS_NUM          (32)
#define     PRE_PORTS_INGR_STC_REGS_NUM         (27 * 3)
#define     PCL_CONF_REGS_NUM                   (4220)
#define     PCL_TCAM_REGS_NUM                   (16384)
#define     PCL_INTERN_REGS_NUM                 (22)
#define     PCL_PORT_VLAN_QOS_REGS_NUM          (64 * 2)
#define     PCL_PROT_BASED_VLAN_QOS_REGS_NUM    (64)
#define     PCL_ACTION_TBL_REGS_NUM             (1024 * 3)
#define     PCL_ID_CONF_TBL_REGS_NUM            (1152)
#define     POLICER_REGS_NUM                    (27)
#define     POLICER_TBL_REGS_NUM                (256 * 4)
#define     POLICER_QOS_REM_TBL_REGS_NUM        (72)
#define     POLICER_COUNT_TBL_REGS_NUM          (16 * 2)
#define     PCI_REGS_NUM                        (280)
#define     PHY_XAUI_DEV_NUM                    (3)
#define     PHY_IEEE_XAUI_REGS_NUM              (0xffff)
#define     PHY_EXT_XAUI_REGS_NUM               (0xffff)
#if 0
    #define     MAC_UPD_FIFO_REGS_NUM               (4 * 16)
#endif /*0*/

/* invalid MAC message  */
#define     SMEM_CHEETAH_INVALID_MAC_MSG_CNS    0xffffffff


/* Register special function index in function array (Bits 23:28)*/
#define     REG_SPEC_FUNC_INDEX                 0x1F800000
/* Port<n> Interrupt Cause Register Mask */
#define     SMEM_CHT_GOPINT_MSK_CNS             0xfff000ff
/* MAC Counters address mask */
#define     SMEM_CHT_COUNT_MSK_CNS              0xfffff000

/*******************************************************************************
*   smemChtGMInit
*
* DESCRIPTION:
*       Init memory module for a Cheetah golden model device.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
void smemChtGMInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_DEV_MEM_INFO  * devMemInfoPtr;

    /* alloc SMEM_CHT_DEV_MEM_INFO */
    devMemInfoPtr = (SMEM_CHT_DEV_MEM_INFO *)calloc(1, sizeof(SMEM_CHT_DEV_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
            skernelFatalError("smemChtGMInit: allocation error\n");
    }

    /* init specific functions array */
    smemChtGMInitFuncArray(&devMemInfoPtr->common);

    /* allocate address type specific memories */
    smemChtGMAllocSpecMemory(devMemInfoPtr);

    /* init PCI registers */
    smemChtGMSetPciDefault(devObjPtr,devMemInfoPtr);

    devObjPtr->devFindMemFunPtr = (void *)smemChtGMFindMem;
    devObjPtr->deviceMemory = devMemInfoPtr;
}

/*******************************************************************************
*   smemChtGMFindMem
*
* DESCRIPTION:
*       Return pointer to the register's or tables's memory.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static void * smemChtGMFindMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    void                * memPtr;
    SMEM_CHT_DEV_COMMON_MEM_INFO  * devMemInfoPtr;
    GT_32                 index;
    GT_U32                param;

    if (devObjPtr == 0)
    {
        skernelFatalError("smemChtGMFindMem: illegal pointer \n");
    }
    memPtr = 0;
    devMemInfoPtr = &(((SMEM_CHT_DEV_MEM_INFO  *)devObjPtr->deviceMemory)->common);

    /* Find PCI registers memory  */
    if (SMEM_ACCESS_PCI_FULL_MAC(accessType))
    {
        memPtr = smemChtGMPciReg(devObjPtr, address, memSize, 0);
        return memPtr;
    }

    index = (address & REG_SPEC_FUNC_INDEX) >>
                 SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS;

    if (((address >> SMEM_PHY_UNIT_INDEX_CNS) & 0x3) == 0x3)
    {
      memPtr  = smemChtGMPhyReg(devObjPtr, address, memSize);
    }
    else
    {
      param   = devMemInfoPtr->specFunTbl[index].specParam;
      memPtr  = devMemInfoPtr->specFunTbl[index].specFun(devObjPtr,
                                                         accessType,
                                                         address,
                                                         memSize,
                                                         param);
    }

    return memPtr;
}

/*******************************************************************************
*   smemChtGMGlobalReg
*
* DESCRIPTION:
*       Global Registers, TWSI, CPU port, SDMA and PCI internal registers
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
static GT_U32 *  smemChtGMGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Global registers */
    if ((address & 0x1FFFFF00) == 0x0)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.globReg,
                         devMemInfoPtr->globalMem.globRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.globReg[index];
    }
    else
     /* SDMA registers */
    if ((address & 0x1FFFF000) == 0x00002000) {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.sdmaReg,
                         devMemInfoPtr->globalMem.sdmaRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.sdmaReg[index];
    }

    return regValPtr;
}
/*******************************************************************************
*   smemChtGMTransQueReg
*
* DESCRIPTION:
*       Egress, Transmit Queue and VLAN Configuration Register Map Table,
*       Port TxQ Configuration Register, Hyper.GStack Ports MIB Counters
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
static GT_U32 *  smemChtGMTransQueReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;
    GT_U32                group;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Common registers */
    if ((address & 0x1FFF0000) == 0x01800000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.genReg,
                         devMemInfoPtr->egrMem.genRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.genReg[index];
    }
    else
    /* Trunks Filtering and Multicast Distribution Configuration Registers,
       Device Map Table, Source-ID Egress Filtering Registers*/
    if ((address & 0x1FFFF000) == 0x01A40000)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.trnFilterReg,
                         devMemInfoPtr->egrMem.trnFilterRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.trnFilterReg[index];
    }
    else
    /* Port Watchdog Configuration Registers */
    if ((address & 0x1FFF0000) == 0x01A80000)
    {
        index = (address & 0xFFF) / 0x200;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.portWdReg,
                         devMemInfoPtr->egrMem.portWdRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.portWdReg[index];
    }
    else
    /* Egress MIB Counters */
    if ((address & 0x1FFFFF00) == 0x01B40100)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.egrMibCntReg,
                         devMemInfoPtr->egrMem.egrMibCntRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.egrMibCntReg[index];
    }
    else
    /* Hyper.GStack Ports MIB Counters */
    if ((address & 0x1FFF0000) >= 0x01C00000 &&
        (address & 0x1FFF0000) <= 0x01C80000)
    {
        group = ((address >> SMEM_CHT_GROUP_INDEX_FIRST_BIT_CNS) & 0xF) / 0x4;
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.hprMibCntReg[group],
                         devMemInfoPtr->egrMem.hprMibCntRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.hprMibCntReg[group][index];
    }
    else
    /* Master XSMI Interface Configuration Registers */
    if ((address & 0x1FFF0000) == 0x01CC0000)
    {
        index = (address & 0xF) / 0x8;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.xsmiReg,
                         devMemInfoPtr->egrMem.xsmiRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.xsmiReg[index];
    }
    else
    /* Egress STC Table */
    if ((address & 0x1FFF0000) == 0x01D40000)
    {
        group = (address & 0xF) / 0x4;
        index = (address & 0xFFF) / 0x200;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.stcEgrReg[group],
                         devMemInfoPtr->egrMem.stcEgrRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.stcEgrReg[group][index];
    }
    else
    /* Tail Drop Profile Configuration Registers */
    if ((address & 0x1FFFF000) == 0x01940000)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.tailDropReg,
                         devMemInfoPtr->egrMem.tailDropRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.tailDropReg[index];
    }
    else
    /* Port<n> Token Bucket Configuration Registers */
    if ((address & 0x1FFF000F) == 0x01AC0008)
    {
        index = (address & 0xFFF) / 0x200;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.egrRateShpReg[0],
                         devMemInfoPtr->egrMem.egrRateShpRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.egrRateShpReg[0][index];
    }
    else
    /* Port<n> TC<t> Token Bucket Configuration Registers */
    if (address >= 0x01AC0000 && address <= 0x01AFFE00)
    {
        group = (address & 0xFFF) / 0x200;
        index = (address & 0xFFFF) / 0x8000;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egrMem.egrRateShpReg[group],
                         devMemInfoPtr->egrMem.egrRateShpRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egrMem.egrRateShpReg[group][index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemChtGMEtherBrdgReg
*
* DESCRIPTION:
*       Bridge Configuration Registers
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
static GT_U32 *  smemChtGMEtherBrdgReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT_DEV_MEM_INFO  *)devObjPtr->deviceMemory;
    if ((address & 0xFF000000) == 0X02000000)
    {
        index = (address & 0x000FFFFF) / 4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->brdgMngMem.genReg,
                         devMemInfoPtr->brdgMngMem.genRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->brdgMngMem.genReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemChtGMBufMngReg
*
* DESCRIPTION:
*       Describe a device's buffer management registers memory object
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
static GT_U32 *  smemChtGMBufMemoryConfReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Buffers Memory Registers */
    if ((address & 0xFFF00000) == 0x07800000)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMem.bufMemReg,
                         devMemInfoPtr->bufMem.bufMemRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMem.bufMemReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemChtGMPortGroupConfReg
*
* DESCRIPTION:
*       Describe a device's Port registers memory object
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
static GT_U32 *  smemChtGMPortGroupConfReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;
    GT_U32                groupNum;
    GT_U32                gopMaskResult = 0;
    GT_U32                macMibMaskResult = 0;
    GT_U32                ledMaskResult = 0;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    groupNum = param;

    if (groupNum == 0)
    {
        gopMaskResult =     0x04004000;
        macMibMaskResult =  0x04010000;
        ledMaskResult =     0x04005100;
    }
    else
    if (groupNum == 1)
    {
        gopMaskResult =     0x04804000;
        macMibMaskResult =  0x04810000;
        ledMaskResult =     0x04805100;
    }
    else
    if (groupNum == 2)
    {
        gopMaskResult =     0x05004000;
        macMibMaskResult =  0x05010000;
        ledMaskResult =     0x05005100;
    }
    else
    if (groupNum == 3)
    {
        gopMaskResult =     0x05804000;
        macMibMaskResult =  0x05810000;
        ledMaskResult =     0x05805100;
    }

    /*  Group configuration registers */
    if ((address & 0xFFFFF000) == gopMaskResult)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->gopCnfMem.gopReg[groupNum],
                         devMemInfoPtr->gopCnfMem.gopRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->gopCnfMem.gopReg[groupNum][index];
    }
    else
    /* MAC MIB Counters registers */
    if ((address & 0xFFFF0000) == macMibMaskResult)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->gopCnfMem.macMibCountReg[groupNum],
                         devMemInfoPtr->gopCnfMem.macMibCountRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->gopCnfMem.macMibCountReg[groupNum][index];
    }
    else
    /* LED Interfaces Configuration Registers */
    if ((address & 0xFFFFFF00) == ledMaskResult)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->gopCnfMem.ledReg[groupNum],
                         devMemInfoPtr->gopCnfMem.ledRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->gopCnfMem.ledReg[groupNum][index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemChtGMMacTableReg
*
* DESCRIPTION:
*       Describe a device's Bridge registers and FDB
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
static GT_U32 *  smemChtGMMacTableReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT_DEV_MEM_INFO  *)devObjPtr->deviceMemory;
    /* FDB Global Configuration Registers */
    if ((address & 0xFF00000) == 0x06000000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->macFdbMem.fdbReg,
                         devMemInfoPtr->macFdbMem.fdbRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->macFdbMem.fdbReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemChtGMInterBufBankReg
*
* DESCRIPTION:
*
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
static GT_U32 *  smemChtGMInterBufBankReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;
    GT_U32                groupNum;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    groupNum = param;
    /* Buffers Memory Bank0 Write Data Register number */
    if (address == 0x06840000)
    {
        regValPtr = &devMemInfoPtr->banksMem.bankWriteReg[0];
    }
    else
    {
        index = (address & 0xFFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->banksMem.bankMemReg[groupNum],
                         devMemInfoPtr->banksMem.bankMemRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->banksMem.bankMemReg[groupNum][index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemChtGMBufMemoryConfReg
*
* DESCRIPTION:
*       Registers, index is group number
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
static GT_U32 *  smemChtGMBufMngReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    if ((address & 0xFFFFF000) == 0x03000000)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.bmReg,
                         devMemInfoPtr->bufMngMem.bmRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.bmReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemChtGMVlanTableReg
*
* DESCRIPTION:
*       Describe a device's Bridge registers and FDB
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
static GT_U32 *  smemChtGMVlanTableReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    if ((address & 0xFF000000) == 0x0A000000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->vlanTblMem.vlanCnfReg,
                         devMemInfoPtr->vlanTblMem.vlanCnfRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->vlanTblMem.vlanCnfReg[index];
    }

    return regValPtr;
}

static GT_U32 *  smemChtGMFatalError(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    skernelFatalError("smemChtGMFatalError: illegal function pointer\n");

    return 0;
}
/*******************************************************************************
*   smemChtGMInitMemArray
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
static void smemChtGMInitFuncArray(
    INOUT SMEM_CHT_DEV_COMMON_MEM_INFO  * devMemInfoPtr
)
{
    GT_U32              i;

    for (i = 0; i < 64; i++) {
        devMemInfoPtr->specFunTbl[i].specFun    = smemChtGMFatalError;
    }
    devMemInfoPtr->specFunTbl[0].specFun        = smemChtGMGlobalReg;

    devMemInfoPtr->specFunTbl[3].specFun        = smemChtGMTransQueReg;

    devMemInfoPtr->specFunTbl[4].specFun        = smemChtGMEtherBrdgReg;

    devMemInfoPtr->specFunTbl[6].specFun        = smemChtGMBufMngReg;

    devMemInfoPtr->specFunTbl[8].specFun        = smemChtGMPortGroupConfReg;
    devMemInfoPtr->specFunTbl[8].specParam      = 0;

    devMemInfoPtr->specFunTbl[9].specFun        = smemChtGMPortGroupConfReg;
    devMemInfoPtr->specFunTbl[9].specParam      = 1;

    devMemInfoPtr->specFunTbl[10].specFun       = smemChtGMPortGroupConfReg;
    devMemInfoPtr->specFunTbl[10].specParam     = 2;

    devMemInfoPtr->specFunTbl[11].specFun       = smemChtGMPortGroupConfReg;
    devMemInfoPtr->specFunTbl[11].specParam     = 3;

    devMemInfoPtr->specFunTbl[12].specFun       = smemChtGMMacTableReg;

    devMemInfoPtr->specFunTbl[13].specFun       = smemChtGMInterBufBankReg;
    devMemInfoPtr->specFunTbl[13].specParam     = 0;

    devMemInfoPtr->specFunTbl[14].specFun       = smemChtGMInterBufBankReg;
    devMemInfoPtr->specFunTbl[14].specParam     = 1;

    devMemInfoPtr->specFunTbl[15].specFun       = smemChtGMBufMemoryConfReg;

    devMemInfoPtr->specFunTbl[20].specFun       = smemChtGMVlanTableReg;

    devMemInfoPtr->specFunTbl[21].specFun       = smemChtGMTriSpeedReg;

    devMemInfoPtr->specFunTbl[22].specFun       = smemChtGMPreEgressReg;

    devMemInfoPtr->specFunTbl[23].specFun       = smemChtGMPclReg;

    devMemInfoPtr->specFunTbl[24].specFun       = smemChtGMPolicerReg;
}
/*******************************************************************************
*   smemChtGMAllocSpecMemory
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
static void smemChtGMAllocSpecMemory(
    INOUT SMEM_CHT_DEV_MEM_INFO  * devMemInfoPtr
)
{
    GT_U8 i;

    /* Global register, TWSI, CPU port, SDMA and PCI internal registers */

    devMemInfoPtr->globalMem.globRegNum = GLB_REGS_NUM;
    devMemInfoPtr->globalMem.globReg =
        calloc(GLB_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.globReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->globalMem.sdmaRegNum = GLB_SDMA_REGS_NUM;
    devMemInfoPtr->globalMem.sdmaReg =
        calloc(GLB_SDMA_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.sdmaReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    /* Egress, Transmit Queue and VLAN Configuration Register Map Table,
       Port TxQ Configuration Register, Hyper.GStack Ports MIB Counters */

    devMemInfoPtr->egrMem.genRegNum = EGR_GLB_REGS_NUM;
    devMemInfoPtr->egrMem.genReg =
        calloc(EGR_GLB_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.genReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.trnFilterRegNum = EGR_TRUNK_FILTER_REGS_NUM;
    devMemInfoPtr->egrMem.trnFilterReg =
        calloc(EGR_TRUNK_FILTER_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.trnFilterReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.portWdRegNum = EGR_PORT_WD_REGS_NUM;
    devMemInfoPtr->egrMem.portWdReg =
        calloc(EGR_PORT_WD_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.portWdReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.egrMibCntRegNum = EGR_MIB_CNT_REGS_NUM;
    devMemInfoPtr->egrMem.egrMibCntReg =
        calloc(EGR_MIB_CNT_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.egrMibCntReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.hprMibCntRegNum = EGR_HYP_MIB_REGS_NUM;
    for (i = 0; i < 3; i++)
    {
        devMemInfoPtr->egrMem.hprMibCntReg[i] =
            calloc(EGR_HYP_MIB_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->egrMem.hprMibCntReg[i] == 0)
        {
            skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
        }
    }
    devMemInfoPtr->egrMem.xsmiRegNum = EGR_XSMI_REGS_NUM;
    devMemInfoPtr->egrMem.xsmiReg =
        calloc(EGR_XSMI_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.xsmiReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.stcEgrRegNum = EGR_STC_TBL_REGS_NUM;
    for (i = 0; i < 3; i++)
    {
        devMemInfoPtr->egrMem.stcEgrReg[i] =
            calloc(EGR_STC_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->egrMem.stcEgrReg[i] == 0)
        {
            skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
        }
    }

    devMemInfoPtr->egrMem.tailDropRegNum = EGR_TALE_DROP_REGS_NUM;
    devMemInfoPtr->egrMem.tailDropReg =
        calloc(EGR_TALE_DROP_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egrMem.tailDropReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->egrMem.egrRateShpRegNum = EGR_RATE_SHAPES_REGS_NUM;
    for (i = 0; i < 9; i++)
    {
        devMemInfoPtr->egrMem.egrRateShpReg[i] =
            calloc(EGR_RATE_SHAPES_REGS_NUM/* 200 */, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->egrMem.egrRateShpReg[i] == 0)
        {
            skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
        }
    }

    /* Bridge Management registers memory object */

    devMemInfoPtr->brdgMngMem.genRegsNum = BRDG_ETH_BRDG_REGS_NUM;
    devMemInfoPtr->brdgMngMem.genReg =
        calloc(BRDG_ETH_BRDG_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->brdgMngMem.genReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    /* Buffer management registers memory object */

    devMemInfoPtr->bufMngMem.bmRegNum = BUF_MNG_REGS_NUM;
    devMemInfoPtr->bufMngMem.bmReg =
        calloc(BUF_MNG_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.bmRegNum == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    /* Ports groups configuration registers (port0 through port5),
       LEDs interfaces configuration registers and Master SMI
       interfaces registers                                         */

    devMemInfoPtr->gopCnfMem.gopRegNum = GOP_CONF_REGS_NUM;
    devMemInfoPtr->gopCnfMem.macMibCountRegNum = GOP_MAC_MIB_REGS_NUM;
    devMemInfoPtr->gopCnfMem.ledRegNum = GOP_LED_REGS_NUM;
    for (i = 0; i < 4; i++)
    {
        devMemInfoPtr->gopCnfMem.gopReg[i] =
            calloc(devMemInfoPtr->gopCnfMem.gopRegNum, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->gopCnfMem.gopReg[i] == 0)
        {
            skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
        }

        devMemInfoPtr->gopCnfMem.macMibCountReg[i] =
            calloc(devMemInfoPtr->gopCnfMem.macMibCountRegNum,
                                                    sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->gopCnfMem.macMibCountReg[i] == 0)
        {
            skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
        }

        devMemInfoPtr->gopCnfMem.ledReg[i] =
            calloc(devMemInfoPtr->gopCnfMem.ledRegNum, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->gopCnfMem.ledReg[i] == 0)
        {
            skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
        }
    }

    /* Bridge registers and FDB */

    devMemInfoPtr->macFdbMem.fdbRegNum = MAC_FDB_REGS_NUM;
    devMemInfoPtr->macFdbMem.fdbReg =
        calloc(MAC_FDB_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macFdbMem.fdbReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->macFdbMem.macTblRegNum = MAC_TBL_REGS_NUM;
    devMemInfoPtr->macFdbMem.macTblReg =
        calloc(MAC_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macFdbMem.macTblReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    /* Buffers memory banks data register */

    devMemInfoPtr->banksMem.bankWriteRegNum = BANK_WRITE_REGS_NUM;
    devMemInfoPtr->banksMem.bankWriteReg =
        calloc(BANK_WRITE_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->banksMem.bankWriteReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->banksMem.bankMemRegNum = BANK_MEM_REGS_NUM;
    for (i = 0; i < 2; i++)
    {
        devMemInfoPtr->banksMem.bankMemReg[i] =
            calloc(BANK_MEM_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->banksMem.bankMemReg[i] == 0)
        {
            skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
        }
    }

    /* Buffers Memory, Ingress MAC Errors Indications and
       Egress Header Alteration Register Map Table registers            */

    devMemInfoPtr->bufMem.bufMemRegNum = BUF_MEM_REGS_NUM;
    devMemInfoPtr->bufMem.bufMemReg =
        calloc(BUF_MEM_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMem.bufMemReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    /* VLAN Table, Multicast Groups Table, and Span State Group Table */

    devMemInfoPtr->vlanTblMem.vlanCnfRegNum = VTL_VLAN_CONF_REGS_NUM;
    devMemInfoPtr->vlanTblMem.vlanCnfReg =
        calloc(VTL_VLAN_CONF_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->vlanTblMem.vlanCnfReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->vlanTblMem.vlanTblRegNum = VTL_VLAN_TBL_REGS_NUM;
    devMemInfoPtr->vlanTblMem.vlanTblReg =
        calloc(VTL_VLAN_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->vlanTblMem.vlanTblReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->vlanTblMem.mcstTblRegNum = VTL_MCST_TBL_REGS_NUM;
    devMemInfoPtr->vlanTblMem.mcstTblReg =
        calloc(VTL_MCST_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->vlanTblMem.mcstTblReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->vlanTblMem.spanTblRegNum = VTL_STP_TBL_REGS_NUM;
    devMemInfoPtr->vlanTblMem.spanTblReg =
        calloc(VTL_STP_TBL_REGS_NUM/*5000*/, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->vlanTblMem.spanTblReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    /* Tri-Speed Ports MAC */

    devMemInfoPtr->triSpeedPortsMem.triSpeedPortsRegNum = TRI_SPEED_REGS_NUM;
    devMemInfoPtr->triSpeedPortsMem.triSpeedPortsReg =
        calloc(TRI_SPEED_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->triSpeedPortsMem.triSpeedPortsReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    /* Pre-Egress Engine Registers Map Table, Trunk Table,
       QoSProfile to QoS Table, CPU Code Table, Data Access Statistical Rate
       Limits Table Data Access, Ingress STC Table Registers                */

    devMemInfoPtr->preegressMem.portsRegNum = PRE_EGR_PORTS_REGS_NUM;
    devMemInfoPtr->preegressMem.portsReg =
        calloc(PRE_EGR_PORTS_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->preegressMem.portsReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->preegressMem.genRegNum = PRE_EGR_GEN_REGS_NUM;
    devMemInfoPtr->preegressMem.genReg =
        calloc(PRE_EGR_GEN_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->preegressMem.genReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->preegressMem.trunkTblRegNum = PRE_EGR_TRUNK_TBL_REGS_NUM;
    for (i = 0; i < 8; i++)
    {
        devMemInfoPtr->preegressMem.trunkTblReg[i] =
            calloc(PRE_EGR_TRUNK_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->preegressMem.trunkTblReg[i] == 0)
        {
            skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
        }
    }

    devMemInfoPtr->preegressMem.qosTblRegNum = PRE_QOS_TBL_REGS_NUM;
    devMemInfoPtr->preegressMem.qosTblReg =
        calloc(PRE_QOS_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->preegressMem.qosTblReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->preegressMem.cpuCodeTblRegNum = PRE_CPU_CODE_TBL_REGS_NUM;
    devMemInfoPtr->preegressMem.cpuCodeTblReg =
        calloc(PRE_CPU_CODE_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->preegressMem.cpuCodeTblReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->preegressMem.statRateTblRegNum = PRE_STAT_RATE_TBL_REGS_NUM;
    devMemInfoPtr->preegressMem.statRateTblReg =
        calloc(PRE_STAT_RATE_TBL_REGS_NUM/*300*/, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->preegressMem.statRateTblReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->preegressMem.portInStcTblRegNum = PRE_PORTS_INGR_STC_REGS_NUM;
    devMemInfoPtr->preegressMem.portInStcTblReg =
        calloc(PRE_PORTS_INGR_STC_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->preegressMem.portInStcTblReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    /* PCL registers and TCAM memory space */

    devMemInfoPtr->pclMem.pclConfRegNum = PCL_CONF_REGS_NUM;
    devMemInfoPtr->pclMem.pclConfReg =
        calloc(PCL_CONF_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pclMem.pclConfReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->pclMem.tcamRegNum = PCL_TCAM_REGS_NUM;
    devMemInfoPtr->pclMem.tcamReg =
        calloc(PCL_TCAM_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pclMem.tcamReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->pclMem.pclIntRegNum = PCL_INTERN_REGS_NUM;
    devMemInfoPtr->pclMem.pclIntReg =
        calloc(PCL_INTERN_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pclMem.pclIntReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->pclMem.portVlanQosTblRegNum = PCL_PORT_VLAN_QOS_REGS_NUM;
    devMemInfoPtr->pclMem.portVlanQosTblReg =
        calloc(PCL_PORT_VLAN_QOS_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pclMem.portVlanQosTblReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->pclMem.protBasedVlanQosTblRegNum =
                                            PCL_PROT_BASED_VLAN_QOS_REGS_NUM;
    for (i = 0; i < 8; i++)
    {
        devMemInfoPtr->pclMem.protBasedVlanQosTblReg[i] =
            calloc(PCL_PROT_BASED_VLAN_QOS_REGS_NUM, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->pclMem.protBasedVlanQosTblReg[i] == 0)
        {
            skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
        }
    }

    devMemInfoPtr->pclMem.pceActionsTblRegNum = PCL_ACTION_TBL_REGS_NUM;
    devMemInfoPtr->pclMem.pceActionsTblReg =
        calloc(PCL_ACTION_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pclMem.pceActionsTblReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->pclMem.pclIdTblRegNum = PCL_ID_CONF_TBL_REGS_NUM;
    devMemInfoPtr->pclMem.pclIdTblReg =
        calloc(PCL_ID_CONF_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pclMem.pclIdTblReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    /* Policer registers and meters memory space */
    devMemInfoPtr->policerMem.policerRegNum = POLICER_REGS_NUM;
    devMemInfoPtr->policerMem.policerReg =
        calloc(POLICER_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->policerMem.policerReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->policerMem.policerTblRegNum = POLICER_TBL_REGS_NUM;
    devMemInfoPtr->policerMem.policerTblReg =
        calloc(POLICER_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->policerMem.policerTblReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->policerMem.policerQosRmTblRegNum =
                                                POLICER_QOS_REM_TBL_REGS_NUM;
    devMemInfoPtr->policerMem.policerQosRmTblReg =
        calloc(POLICER_QOS_REM_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->policerMem.policerQosRmTblReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->policerMem.policerCntTblRegNum = POLICER_COUNT_TBL_REGS_NUM;
    devMemInfoPtr->policerMem.policerCntTblReg =
        calloc(POLICER_COUNT_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->policerMem.policerCntTblReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    /* PCI Registers */
    devMemInfoPtr->pciMem.pciRegNum = PCI_REGS_NUM;
    devMemInfoPtr->pciMem.pciReg =
        calloc(PCI_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pciMem.pciReg == 0)
    {
        skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
    }

    /* PHY related Registers ieeeXauiReg[0..3]->0x000 - 0x020 */
    /* extXauiReg[0..3]->0x000 - 0x02D */
    devMemInfoPtr->common.phyMem.PhyIdieeeXauiRegNum = PHY_IEEE_XAUI_REGS_NUM;
    devMemInfoPtr->common.phyMem.PhyIdextXauiRegNum = PHY_EXT_XAUI_REGS_NUM;
    for (i = 0; i < PHY_XAUI_DEV_NUM; i++)
    {
        devMemInfoPtr->common.phyMem.PhyId0ieeeXauiReg[i] =
            calloc(PHY_IEEE_XAUI_REGS_NUM, sizeof(SMEM_PHY_REGISTER));
        if (devMemInfoPtr->common.phyMem.PhyId0ieeeXauiReg[i] == 0)
        {
            skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
        }
        devMemInfoPtr->common.phyMem.PhyId0extXauiReg[i] =
            calloc(PHY_EXT_XAUI_REGS_NUM, sizeof(SMEM_PHY_REGISTER));
        if (devMemInfoPtr->common.phyMem.PhyId0extXauiReg[i] == 0)
        {
            skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
        }

        devMemInfoPtr->common.phyMem.PhyId1ieeeXauiReg[i] =
            calloc(PHY_IEEE_XAUI_REGS_NUM, sizeof(SMEM_PHY_REGISTER));
        if (devMemInfoPtr->common.phyMem.PhyId1ieeeXauiReg[i] == 0)
        {
            skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
        }
        devMemInfoPtr->common.phyMem.PhyId1extXauiReg[i] =
            calloc(PHY_EXT_XAUI_REGS_NUM, sizeof(SMEM_PHY_REGISTER));
        if (devMemInfoPtr->common.phyMem.PhyId1extXauiReg[i] == 0)
        {
            skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
        }

        devMemInfoPtr->common.phyMem.PhyId2ieeeXauiReg[i] =
            calloc(PHY_IEEE_XAUI_REGS_NUM, sizeof(SMEM_PHY_REGISTER));
        if (devMemInfoPtr->common.phyMem.PhyId2ieeeXauiReg[i] == 0)
        {
            skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
        }
        devMemInfoPtr->common.phyMem.PhyId2extXauiReg[i] =
            calloc(PHY_EXT_XAUI_REGS_NUM, sizeof(SMEM_PHY_REGISTER));
        if (devMemInfoPtr->common.phyMem.PhyId2extXauiReg[i] == 0)
        {
            skernelFatalError("smemChtGMAllocSpecMemory: allocation error\n");
        }
    }

#if 0
    /* CPU fifo memory */
    devMemInfoPtr->macFdbMem.macUpdFifoRegsNum = MAC_UPD_FIFO_REGS_NUM;
    devMemInfoPtr->macFdbMem.macUpdFifoRegs =
        malloc(MAC_UPD_FIFO_REGS_NUM * sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macFdbMem.macUpdFifoRegs == 0)
    {
        skernelFatalError("smemSalsaAllocSpecMemory: allocation error\n");
    }
    memset(devMemInfoPtr->macFdbMem.macUpdFifoRegs, 0xFF,
        MAC_UPD_FIFO_REGS_NUM * sizeof(SMEM_REGISTER));
#endif/*0*/
}



/*******************************************************************************
*   smemChtGMPciReg
*
* DESCRIPTION:
*       PCI memory access
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
static GT_U32 *  smemChtGMPciReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    index = (address & 0xFFF) / 0x4;
    CHECK_MEM_BOUNDS(devMemInfoPtr->pciMem.pciReg,
                     devMemInfoPtr->pciMem.pciRegNum,
                     index, memSize);
    regValPtr = &devMemInfoPtr->pciMem.pciReg[index];

    return regValPtr;
}

/*******************************************************************************
*   smemChtGMPclReg
*
* DESCRIPTION:
*       PCL registers and TCAM memory space access
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
static GT_U32 *  smemChtGMPclReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                * regValPtr;
    GT_U32                  index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* PCL Registers */
    if ((address & 0xFFFF0000) == 0x0B800000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.pclConfReg,
                         devMemInfoPtr->pclMem.pclConfRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.pclConfReg[index];
    }
    else
    /* TCAM Registers */
    if ((address & 0xFFFF0000) == 0x0B810000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.tcamReg,
                         devMemInfoPtr->pclMem.tcamRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.tcamReg[index];
    }
    else
    /* PCL internal registers */
    if ((address & 0xFFFFFF00) == 0x0B820000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pclMem.pclIntReg,
                         devMemInfoPtr->pclMem.pclIntRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pclMem.pclIntReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemChtGMPolicerReg
*
* DESCRIPTION:
*       Policer registers and meters memory space access
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
static GT_U32 *  smemChtGMPolicerReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_32               index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Policer Registers */
    if ((address & 0xFFFFFF00) == 0x0C000000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->policerMem.policerReg,
                         devMemInfoPtr->policerMem.policerRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->policerMem.policerReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemChtGMPhyReg
*
* DESCRIPTION:
*       PHY related Registers access
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
static GT_U32 *  smemChtGMPhyReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize
)
{
    SMEM_CHT_DEV_MEM_INFO  * devMemInfoPtr;
    SMEM_PHY_REGISTER     * regValPtr;
    GT_32                   index;
    GT_32                   device;
    GT_32                   phyid;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    phyid = ((address >> SMEM_CHT_GROUP_INDEX_FIRST_BIT_CNS) & 0x1F) % 3;
    device = ((address >> SMEM_CHT_GROUP_INDEX_SECOND_BIT_CNS) & 0x1F) % 3;
    index = (address & 0xFF);

    if (phyid == 0)
    {
      if ((address & 0xF000) == 0x8000)
      {
          CHECK_MEM_BOUNDS(devMemInfoPtr->common.phyMem.PhyId0extXauiReg[device],
                           devMemInfoPtr->common.phyMem.PhyIdextXauiRegNum,
                           index, memSize);
          regValPtr = &devMemInfoPtr->common.phyMem.PhyId0extXauiReg[device][index];
      }
      else
      {
          CHECK_MEM_BOUNDS(devMemInfoPtr->common.phyMem.PhyId0ieeeXauiReg[device],
                           devMemInfoPtr->common.phyMem.PhyIdieeeXauiRegNum,
                           index, memSize);
          regValPtr = &devMemInfoPtr->common.phyMem.PhyId0ieeeXauiReg[device][index];
      }
    }
    else
    if (phyid == 1)
    {
      if ((address & 0xF000) == 0x8000)
      {
          CHECK_MEM_BOUNDS(devMemInfoPtr->common.phyMem.PhyId1extXauiReg[device],
                           devMemInfoPtr->common.phyMem.PhyIdextXauiRegNum,
                           index, memSize);
          regValPtr = &devMemInfoPtr->common.phyMem.PhyId1extXauiReg[device][index];
      }
      else
      {
          CHECK_MEM_BOUNDS(devMemInfoPtr->common.phyMem.PhyId1ieeeXauiReg[device],
                           devMemInfoPtr->common.phyMem.PhyIdieeeXauiRegNum,
                           index, memSize);
          regValPtr = &devMemInfoPtr->common.phyMem.PhyId1ieeeXauiReg[device][index];
      }
    }
    else
    {
      if ((address & 0xF000) == 0x8000)
      {
          CHECK_MEM_BOUNDS(devMemInfoPtr->common.phyMem.PhyId2extXauiReg[device],
                           devMemInfoPtr->common.phyMem.PhyIdextXauiRegNum,
                           index, memSize);
          regValPtr = &devMemInfoPtr->common.phyMem.PhyId2extXauiReg[device][index];
      }
      else
      {
          CHECK_MEM_BOUNDS(devMemInfoPtr->common.phyMem.PhyId2ieeeXauiReg[device],
                           devMemInfoPtr->common.phyMem.PhyIdieeeXauiRegNum,
                           index, memSize);
          regValPtr = &devMemInfoPtr->common.phyMem.PhyId2ieeeXauiReg[device][index];
      }
    }

    return (GT_U32 *)regValPtr;
}

/*******************************************************************************
*   smemChtGMTriSpeedReg
*
* DESCRIPTION:
*      Describe a Tri-Speed Ports MAC, CPU Port MAC, and 1.25 Gbps SERDES
*      Configuration Registers Map Table,  Hyper.GStack Ports MAC and
*      XAUI PHYs Configuration Registers Map Table memory access
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
static GT_U32 *  smemChtGMTriSpeedReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_32               index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Tri-Speed Ports */
    if ((address & 0xFFFF0000) == 0x0A800000)
    {
        index = (address & 0x3FFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->triSpeedPortsMem.triSpeedPortsReg,
                         devMemInfoPtr->triSpeedPortsMem.triSpeedPortsRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->triSpeedPortsMem.triSpeedPortsReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemChtGMPreEgressReg
*
* DESCRIPTION:
*      Pre-Egress Engine Registers Map Table, Trunk Table,
*      QoSProfile to QoS Table, CPU Code Table,
*      Data Access Statistical Rate Limits Table Data Access,
*      Ingress STC Table Registers memory access
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
static GT_U32 *  smemChtGMPreEgressReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_CHT_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                * regValPtr;
    GT_U32                  index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_CHT_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    /* Pre-Egress Engine Registers */
    if ((address & 0xFFFFFF00) == 0x0B000000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->preegressMem.portsReg,
                         devMemInfoPtr->preegressMem.portsRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->preegressMem.portsReg[index];
    }
    else
    /* Ports registers */
    if ((address & 0xFFFFF000) >= 0x0B001000)
    {
        index = (address & 0xFFFF) / 0x1000;
        index += (address & 0xF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->preegressMem.genReg,
                         devMemInfoPtr->preegressMem.genRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->preegressMem.genReg[index];
    }


    return regValPtr;
}

/*******************************************************************************
*  smemChtGMSetPciDefault
*
* DESCRIPTION:
*       Set PCI registers default values
*
* INPUTS:
*    devObjPtr        - device object PTR.
*    devMemInfoPtr    - pointer to device memory object.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemChtGMSetPciDefault(
    IN SKERNEL_DEVICE_OBJECT    * devObjPtr,
    INOUT SMEM_CHT_DEV_MEM_INFO * devMemInfoPtr
)
{
    GT_U32              index;
    GT_U32              revision;

    /* Device and Vendor ID */
    index = 0;

    devMemInfoPtr->pciMem.pciReg[index] = devObjPtr->deviceType;

    revision = 0x02000000;
    /* Class Code and Revision ID */
    index = 0x08 / 4;
    devMemInfoPtr->pciMem.pciReg[index] = revision;

    /* Base address */
    index = 0x14 / 4;
    devMemInfoPtr->pciMem.pciReg[index] = devObjPtr->deviceHwId;
}


