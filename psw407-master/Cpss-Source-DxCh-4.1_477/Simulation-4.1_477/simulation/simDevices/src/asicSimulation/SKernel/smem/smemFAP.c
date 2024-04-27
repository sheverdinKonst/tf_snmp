/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemFAP.c
*
* DESCRIPTION:
*       This is API implementation for Dune Fabric Access Processor memory.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/smem/smemFAP.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>

static void * smemFapFindMem
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
);

static void smemFapAllocSpecMemory(
    INOUT SMEM_FAP_DEV_MEM_INFO  * devMemInfoPtr
);

static GT_U32 *  smemFapConfigReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static GT_U32 *  smemFapDataReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static void smemFapActiveWriteIndirectReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
);

/* Private definition */
#define     UPLINK_REGS_NUM                     (0x40)
#define     TWSI_REGS_NUM                       (0x40)
#define     LBP_REGS_NUM                        (0xFFFFF)
#define     LP_UCTABL_REGS_NUM                  (0x40)
#define     LP_TREE_ROUE_REGS_NUM               (0x3FFF)
#define     LP_RCCL_QUE_TBL_REGS_NUM            (0x40)
#define     QC_TBL_REGS_NUM                     (0x3FFF/4 +1)
#define     QT_TBL_REGS_NUM                     (0xF/4 +1)
#define     RP_TBL_REGS_NUM                     (0xFFFF/4 + 1)
#define     QWRED_TBL_REGS_NUM                  (0xFF/4 +1)
#define     SCH_DESC_TBL_NUM                    (0x5000)
#define     SCH_MAP_TBL_NUM                     (0x400)
#define     SCH_CREDIT_TBL_NUM                  (0x40)
#define     SCH_LOOKUP_TBL_NUM                  (0x800)
#define     QSIZE_MEM_REGS_NUM                  (0x1FFF/4 +1)

/* Active memory table */
static SMEM_ACTIVE_MEM_ENTRY_STC smemFAPActiveTable[] =
{
    {0x40800010, SMEM_FULL_MASK_CNS, NULL, 0 , smemFapActiveWriteIndirectReg, 0},

    /* must be last anyway */
    {0xffffffff, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};

#define SMEM_ACTIVE_MEM_TABLE_SIZE \
    (sizeof(smemFAPActiveTable)/sizeof(smemFAPActiveTable[0]))


/*******************************************************************************
* smemFapInit
*
* DESCRIPTION:
*       Init memory module for the Fabric Access Processor device.
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
void smemFapInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_FAP_DEV_MEM_INFO  * devMemInfoPtr;

    /* alloc SMEM_FAP_DEV_MEM_INFO */
    devMemInfoPtr = (SMEM_FAP_DEV_MEM_INFO *)calloc(1, sizeof(SMEM_FAP_DEV_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
        skernelFatalError("smemChtInit: allocation error\n");
    }

    /* allocate address type specific memories */
    smemFapAllocSpecMemory(devMemInfoPtr);

    devObjPtr->devFindMemFunPtr = (void *)smemFapFindMem;
    devObjPtr->deviceMemory = devMemInfoPtr;
}

/*******************************************************************************
*   smemFapFindMem
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
static void * smemFapFindMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    void                    * memPtr;
    GT_32                 index;
    GT_U32                param;

    if (devObjPtr == 0)
    {
        skernelFatalError("smemFapFindMem: illegal pointer \n");
    }
    memPtr = 0;
    param = 0;

    if (((address & 0xf0000000) >> 28) == 0x4)
    {
        memPtr = smemFapConfigReg(devObjPtr,address,memSize,param);
    }
    else
    {
        memPtr = smemFapDataReg(devObjPtr,address,memSize,param);
    }

    /* find active memory entry */
    if (activeMemPtrPtr != NULL)
    {
        *activeMemPtrPtr = NULL;
        for (index = 0; index < (SMEM_ACTIVE_MEM_TABLE_SIZE - 1); index++)
        {
            /* check address */
            if ((address & smemFAPActiveTable[index].mask)
                 == smemFAPActiveTable[index].address)
                *activeMemPtrPtr = &smemFAPActiveTable[index];
        }
    }

    return memPtr;
}

/*******************************************************************************
*   smemFapConfigReg
*
* DESCRIPTION:
*       UpLink Registers, TWSI, and Label Processing internal registers
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
static GT_U32 *  smemFapConfigReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_FAP_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_FAP_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    if ((address & 0x4FFFFF00) == 0x40000000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->configMem.uplinkReg ,
                         devMemInfoPtr->configMem.uplinkRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->configMem.uplinkReg[index];
    }
    else
    if ((address & 0x4FFFFF00) == 0x43800000)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->configMem.twsiReg,
                         devMemInfoPtr->configMem.twsiRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->configMem.twsiReg[index];
    }
    else
    if ((address & 0x4FF00000) == 0x40800000)
    {
        index = (address & 0xFFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->configMem.lbpReg,
                         devMemInfoPtr->configMem.lbpRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->configMem.lbpReg[index];
    }
    else
    if ((address & 0x4FFF0000) == 0x40010000)
    {
        index = (address & 0x13FFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->configMem.schDescTblReg,
                         devMemInfoPtr->configMem.schDescTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->configMem.schDescTblReg[index];
    }
    else
    if ((address & 0x4FFFF000) == 0x40020000)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->configMem.schMapTblReg,
                         devMemInfoPtr->configMem.schMapTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->configMem.schMapTblReg[index];
    }
    else
    if ((address & 0x4FFFFF00) == 0x40040000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->configMem.schCreditGenTblReg,
                         devMemInfoPtr->configMem.schCreditGenTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->configMem.schCreditGenTblReg[index];
    }
    else
    if ((address & 0x4FFF0000) == 0x40050000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->configMem.schLookUpGenTblReg,
                         devMemInfoPtr->configMem.schLookUpGenTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->configMem.schLookUpGenTblReg[index];
    }
    else
    if ((address & 0x4FFFFFFF) == 0x40100000)
    {
        index = 1;
        regValPtr = &devMemInfoPtr->configMem.schTriggerReg[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemFapDataReg
*
* DESCRIPTION:
*       Data Table Simulated Memory
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
static GT_U32 *  smemFapDataReg(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_FAP_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32                index;

    regValPtr = 0;
    devMemInfoPtr = (SMEM_FAP_DEV_MEM_INFO  *)devObjPtr->deviceMemory;

    if ((address & 0xFFFFFF00) == 0x00000000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->tblMem.lpUnicastTablReg,
                         devMemInfoPtr->tblMem.lpUnicastTablRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->tblMem.lpUnicastTablReg[index];
    }
    else
    if ((address & 0xFFFF0000) == 0x01000000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->tblMem.lpTreeRouteTblReg,
                         devMemInfoPtr->tblMem.lpTreeRouteTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->tblMem.lpTreeRouteTblReg[index];
    }
    else
    if ((address & 0xF2FFFF00) == 0x02000000)
    {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->tblMem.lpRecyclQueueTblReg,
                         devMemInfoPtr->tblMem.lpRecyclQueueTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->tblMem.lpRecyclQueueTblReg[index];
    }
    else
    if ((address & 0xF8FF0000) == 0x08000000)
    {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->tblMem.lpRouteProcessorTablReg,
                         devMemInfoPtr->tblMem.lpRouteProcessorTablRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->tblMem.lpRouteProcessorTablReg[index];
    }
    else
    if ((address & 0xFFFF0000) == 0x30000000)
    {
        index = (address & 0x3FFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->tblMem.qcTblReg,
                         devMemInfoPtr->tblMem.qcTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->tblMem.qcTblReg[index];
    }
    else
    if ((address & 0xFFFFFFF0) == 0x30100000)
    {
        index = (address & 0xF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->tblMem.qtParamMemTblReg,
                         devMemInfoPtr->tblMem.qtParamMemTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->tblMem.qtParamMemTblReg[index];
    }
    else
    if ((address & 0xFFFFFF00) == 0x30200000)
    {
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->tblMem.qwredParamTblReg,
                         devMemInfoPtr->tblMem.qwredParamTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->tblMem.qwredParamTblReg[index];
    }
    else
    if ((address & 0xFFFF0000) == 0x30300000)
    {
        index = (address & 0x1FFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->tblMem.qsizeMemReg,
                         devMemInfoPtr->tblMem.qsizeMemRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->tblMem.qsizeMemReg[index];
    }
    else
    if ((address & 0xFFFF0000) == 0x30800000)
    {
        index = (address & 0x1FFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->tblMem.qflowMemReg,
                         devMemInfoPtr->tblMem.qflowMemRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->tblMem.qflowMemReg[index];
    }

    return regValPtr;
}


/*******************************************************************************
*   smemFapAllocSpecMemory
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
static void smemFapAllocSpecMemory(
    INOUT SMEM_FAP_DEV_MEM_INFO  * devMemInfoPtr
)
{
    devMemInfoPtr->configMem.uplinkRegNum = UPLINK_REGS_NUM;
    devMemInfoPtr->configMem.uplinkReg =
        calloc(UPLINK_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->configMem.uplinkReg == 0)
    {
        skernelFatalError("smemFapAllocSpecMemory: uplink registers \
                                                   allocation error\n");
    }

    devMemInfoPtr->configMem.twsiRegNum = TWSI_REGS_NUM;
    devMemInfoPtr->configMem.twsiReg =
        calloc(TWSI_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->configMem.twsiReg == 0)
    {
        skernelFatalError("smemFapAllocSpecMemory: twsi registers \
                                                    allocation error\n");
    }

    devMemInfoPtr->configMem.lbpRegNum = LBP_REGS_NUM;
    devMemInfoPtr->configMem.lbpReg =
        calloc(LBP_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->configMem.lbpReg == 0)
    {
        skernelFatalError("smemFapAllocSpecMemory: lbp registers \
                                                        allocation error\n");
    }

    devMemInfoPtr->configMem.schDescTblRegNum = SCH_DESC_TBL_NUM;
    devMemInfoPtr->configMem.schDescTblReg =
        calloc(SCH_DESC_TBL_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->configMem.schDescTblReg == 0)
    {
        skernelFatalError("smemFapAllocSpecMemory: schedular desc. registers \
                                                        allocation error\n");
    }

    devMemInfoPtr->configMem.schMapTblRegNum = SCH_MAP_TBL_NUM;
    devMemInfoPtr->configMem.schMapTblReg =
        calloc(SCH_MAP_TBL_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->configMem.schMapTblReg == 0)
    {
        skernelFatalError("smemFapAllocSpecMemory: schedular map registers \
                                                        allocation error\n");
    }

    devMemInfoPtr->configMem.schCreditGenTblRegNum = SCH_CREDIT_TBL_NUM;
    devMemInfoPtr->configMem.schCreditGenTblReg =
        calloc(SCH_CREDIT_TBL_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->configMem.schCreditGenTblReg == 0)
    {
        skernelFatalError("smemFapAllocSpecMemory: schedular credit registers \
                                                        allocation error\n");
    }

    devMemInfoPtr->configMem.schLookUpGenTblRegNum  = SCH_LOOKUP_TBL_NUM;
    devMemInfoPtr->configMem.schLookUpGenTblReg =
        calloc(SCH_LOOKUP_TBL_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->configMem.schLookUpGenTblReg == 0)
    {
        skernelFatalError("smemFapAllocSpecMemory: schedular lookup registers \
                                                        allocation error\n");
    }

    devMemInfoPtr->configMem.schTriggerRegNum = 1;
    devMemInfoPtr->configMem.schTriggerReg  =
        calloc(1, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->configMem.schTriggerReg == 0)
    {
        skernelFatalError("smemFapAllocSpecMemory: schedular trigger registers \
                                                        allocation error\n");
    }

    devMemInfoPtr->tblMem.lpUnicastTablRegNum = LP_UCTABL_REGS_NUM;
    devMemInfoPtr->tblMem.lpUnicastTablReg =
        calloc(LP_UCTABL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->tblMem.lpUnicastTablReg == 0)
    {
        skernelFatalError("smemFapAllocSpecMemory: unicast table \
                                                    allocation error\n");
    }

    devMemInfoPtr->tblMem.lpTreeRouteTblRegNum = LP_TREE_ROUE_REGS_NUM;
    devMemInfoPtr->tblMem.lpTreeRouteTblReg =
        calloc(LP_TREE_ROUE_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->tblMem.lpTreeRouteTblReg == 0)
    {
        skernelFatalError("smemFapAllocSpecMemory: route register \
                                                    allocation error\n");
    }

    devMemInfoPtr->tblMem.lpRecyclQueueTblRegNum = LP_RCCL_QUE_TBL_REGS_NUM;
    devMemInfoPtr->tblMem.lpRecyclQueueTblReg =
        calloc(LP_RCCL_QUE_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->tblMem.lpRecyclQueueTblReg == 0)
    {
        skernelFatalError("smemFapAllocSpecMemory: recycle queue register \
                                                    allocation error\n");
    }

    devMemInfoPtr->tblMem.lpRouteProcessorTablRegNum = RP_TBL_REGS_NUM;
    devMemInfoPtr->tblMem.lpRouteProcessorTablReg =
        calloc(RP_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->tblMem.lpRouteProcessorTablReg == 0)
    {
        skernelFatalError("smemFapAllocSpecMemory: route processor register \
                                                    allocation error\n");
    }

    devMemInfoPtr->tblMem.qcTblRegNum = QC_TBL_REGS_NUM;
    devMemInfoPtr->tblMem.qcTblReg =
        calloc(QC_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->tblMem.qcTblReg == 0)
    {
        skernelFatalError("smemFapAllocSpecMemory: qc register     \
                                                    allocation error\n");
    }

    devMemInfoPtr->tblMem.qtParamMemTblRegNum = QT_TBL_REGS_NUM;
    devMemInfoPtr->tblMem.qtParamMemTblReg =
        calloc(QT_TBL_REGS_NUM  , sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->tblMem.qtParamMemTblReg == 0)
    {
        skernelFatalError("smemFapAllocSpecMemory: qtParamMemTblReg register     \
                                                    allocation error\n");
    }

    devMemInfoPtr->tblMem.qwredParamTblRegNum = QWRED_TBL_REGS_NUM;
    devMemInfoPtr->tblMem.qwredParamTblReg =
        calloc(QWRED_TBL_REGS_NUM , sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->tblMem.qwredParamTblReg == 0)
    {
        skernelFatalError("smemFapAllocSpecMemory: rpMcReplicationTblReg register     \
                                                    allocation error\n");
    }

    devMemInfoPtr->tblMem.qsizeMemRegNum = QSIZE_MEM_REGS_NUM;
    devMemInfoPtr->tblMem.qsizeMemReg =
        calloc(QSIZE_MEM_REGS_NUM , sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->tblMem.qsizeMemReg == 0)
    {
        skernelFatalError("smemFapAllocSpecMemory: qsizeMemReg register     \
                                                    allocation error\n");
    }

    devMemInfoPtr->tblMem.qflowMemRegNum = 0x1FFF/4 +1;
    devMemInfoPtr->tblMem.qflowMemReg =
        calloc(0x1FFF/4 +1 , sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->tblMem.qflowMemReg == 0)
    {
        skernelFatalError("smemFapAllocSpecMemory: qflow register     \
                                                    allocation error\n");
    }
}
/*******************************************************************************
*  smemFapActiveWriteIndirectReg
*
* DESCRIPTION:
*
*    Indirect Memories action register
*
* INPUTS:
*    devObjPtr  - device object PTR.
*    address    - Address for ASIC memory.
*    memPtr     - Pointer to the register's memory in the simulation.
*    memSize    - Size of memory to be written
*    param      - Registers' specific parameter.
*    inMemPtr   - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemFapActiveWriteIndirectReg (
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR   param,
    IN         GT_U32 * inMemPtr
)
{
    /* The simulation reset anyway the register */
    *memPtr = 0;
}


