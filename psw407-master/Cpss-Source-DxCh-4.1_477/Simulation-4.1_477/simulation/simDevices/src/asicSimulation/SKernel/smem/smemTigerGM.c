/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemTigerGM.c
*
* DESCRIPTION:
*       This is API implementation for Tiger GM memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 9 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/smem/smemTigerGM.h>
#include <asicSimulation/SKernel/twistCommon/sregTwist.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/suserframes/snetTwistTrafficCond.h>
#include <asicSimulation/SKernel/suserframes/snet.h>


static GT_U32 *  smemTigerGMFatalError(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTigerGMGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTigerGMTransQueReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static GT_U32 *  smemTigerGMEtherBrdgReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTigerGMLxReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTigerGMBufMngReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTigerGMMacReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTigerGMPortGroupConfReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTigerGMMacTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static GT_U32 *  smemTigerGMVlanTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);
static void smemTigerGMInitFuncArray(
    INOUT TIGER_GM_DEV_MEM_INFO  * devMemInfoPtr
);
static void smemTigerGMAllocSpecMemory(
    INOUT TIGER_GM_DEV_MEM_INFO  * devMemInfoPtr
);

static GT_U32 *  smemTigerGMPciReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static void smemMemSizeToBytes(
        IN GT_CHAR * memSizePtr,
        INOUT GT_U32 * memBytesPtr
);

static GT_U32 *  smemTigerGMFaMem(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static GT_U32 *  smemTigerGMExternalMem(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

static GT_VOID smemTigerGMNsramConfigUpdate (
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);


/* Private definition */
#define     GLOB_REGS_NUM               (0x80 / 4 + 1)
#define     TWSI_INT_REGS_NUM           (0x28 / 4 + 1)
#define     SDMA_REGS_NUM               (0x8FF / 4 + 1)
#define     BM_REGS_NUM                 (0x4FFF / 4 + 1)
#define     EPF_REGS_NUM                (0x4FFF / 4 + 1)
#define     EGR_REGS_NUM                (0x28 / 4 + 1)
#define     LX_GEN_REGS_NUM             (0x1FF / 4 + 1)
#define     DSCP_REGS_NUM               (0x3c / 4 + 1)
/* was (0x8FFC / 4) + 1, matches the "Prestera PP reisters */
/* according to working code there are two segments  */
/* from 0x02D40000 and from 0x02D48000 for PCE# %8 0-3 and 4-7 */
#define     PCL_TBL_REGS_NUM            (0xFFFC / 4 + 1)
#define     COS_REGS_NUM                (0x3DC / 4 + 1)
#define     TRUNK_TBL_REGS_NUM          (0x1FFC / 4 + 1)
#define     TIGERGM_C_TRUNK_TBL_REGS_NUM  (0x2DC / 4 + 1)
#define     COUNT_BLOCK_REGS_NUM        (0x178 / 4 + 1)
#define     IPV4_TC_NUM                 (0xFFFFF / 4 + 1)
#define     MPLS_NUM                    (0x178 / 4 + 1)
#define     EXT_MEM_CONF_NUM            (0x2CF / 4 + 1)
#define     MAC_REGS_NUM                (0xEC / 4 + 1)
#define     MAC_PORTS_NUM               (0x3F)
#define     PER_PORT_TYPES_NUM          (0x04 / 4 + 1)
#define     PER_GROUPS_TYPE_NUM         (0x48)
#define     MAC_CNT_TYPES_NUM           (0xD7C / 4 + 1)
#define     SFLOW_NUM                   (0x0C / 4 + 1)
#define     BRG_PORTS_NUM                (64)
#define     PORT_REGS_GROUPS_NUM        (0x14 / 4 + 1)
#define     PORT_PROT_VID_NUM           (0x20)
#define     BRG_GEN_REGS_NUM            (0xB270 / 4 + 1)
#define     GEN_EGRS_GROUP_NUM          (0x1FFF / 4 + 1)
#define     TC_QUE_DESCR_NUM            (8)
#define     STACK_CFG_REG_NUM           (0x1C / 4 + 1)
#define     BUF_MEM_REGS_NUM            (0x3)
#define     MAC_TBL_REGS_NUM            (16 * 1024 * 4)
#define     MAC_UPD_FIFO_REGS_NUM       (4 * 16)
#define     VLAN_REGS_NUM               (2048)
#define     VLAN_TABLE_REGS_NUM         (8 * 1024 * 4 * 4)
#define     STP_TABLE_REGS_NUM          (32 * 2)
#define     PCI_MEM_REGS_NUM            (0x3C / 4 + 1)
#define     PCI_INTERNAL_REGS_NUM       (0x18 / 4 + 1)
#define     PHY_XAUI_DEV_NUM            (6)
#define     PHY_IEEE_XAUI_REGS_NUM      (0xff * PHY_XAUI_DEV_NUM)
#define     PHY_EXT_XAUI_REGS_NUM       (0xff * PHY_XAUI_DEV_NUM)
#define     TCAM_TBL_REG_NUM            (0xfffc /4+1)
#define     PCE_ACTIONS_TBL_REG_NUM     (0x3ffc /4+1)
#define     COS_LOOKUP_TBL_REG_NUM      (0x24   /4+1)
#define     COS_TABLES_REG_NUM          (0x1ff  /4+1)
#define     DSCP_DP_TO_COS_MARK_REMARK_TABLE_REG_NUM (0x3dc /4+1)
#define     INTERNAL_INLIF_TBL_REG_NUM               (0x360 /4+1)
#define     FLOW_TEMPLATE_HASH_CONFIG_TBL_NUM        (0x3c  /4+1)
#define     FLOW_TEMPLATE_HASH_SELECT_TBL_NUM        (0x3fc /4+1)
#define     FLOW_TEMPLATE_SELECT_TBL_NUM             (0x2ff /4+1)
#define     COUNT_BLOCK_2_REGS_NUM        (0x118 / 4 + 1)
#define     EGR_TRUNK_REGS_NUM                       (48)


/* Register special function index in function array (Bits 23:28)*/
#define     REG_SPEC_FUNC_INDEX         0x1F800000

/* GOP interrup cause registers */
#define     SMEM_TigerGM_PORT_INTR_CAUSE0_CNS 0x03800090
#define     SMEM_TigerGM_PORT_INTR_CAUSE1_CNS 0x03800098
#define     SMEM_TigerGM_PORT_INTR_CAUSE5_CNS 0x038000B8

/* number of words in the FDB update message */
#define     SMEM_TigerGM_FDB_UPD_MSG_WORDS_NUM    4

/* invalid MAC message  */
#define     SMEM_TigerGM_INVALID_MAC_MSG_CNS  0xffffffff

#define     SMEM_TIGER_GM_NSRAM_INTERNAL     (32 * 0x400)
#define     SMEM_TIGER_GM_NSRAM_EXTERNAL      (8 * 0x100000)


/* temporary patch for FA/XBAR familly */
static GT_U32 smemFaMemory[0xff];

/*******************************************************************************
*   smemTigerNsramConfigUpdate
*
* DESCRIPTION:
*       Set up narrow sram configuration
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
static GT_VOID smemTigerGMNsramConfigUpdate
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    TIGER_GM_DEV_MEM_INFO * devMemInfoPtr; /* device memory info pointer */
    char   param_str[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
                                        /* string for parameter */
    GT_U32 param_val;                   /* parameter value */
    char   keyString[20];               /* key string */
    TIGER_GM_NSRAM_MODE_ENT nSramMode;     /* narrow SRAM configuration */
    GT_U32 nSramSize;                   /* external NSRAM size */

    devMemInfoPtr = (TIGER_GM_DEV_MEM_INFO*)(deviceObjPtr->deviceMemory);

    /* narow sram type */
    nSramSize = SMEM_TIGER_GM_NSRAM_EXTERNAL / sizeof(SMEM_REGISTER);

    sprintf(keyString, "dev%d_nsram", deviceObjPtr->deviceId);
    if (SIM_OS_MAC(simOsGetCnfValue)("system",  keyString,
                          SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS,
                          param_str))
    {
        /* get nsram size in bytes */
        smemMemSizeToBytes(param_str, &nSramSize);
        /* convert to words number */
        nSramSize /= sizeof(SMEM_REGISTER);
    }

    if(SKERNEL_DEVICE_FAMILY_TIGER(deviceObjPtr->deviceType))
    {
        nSramMode = TIGER_GM_NSRAM_ALL_EXTERNAL_E;

        /* narow sram type */
        sprintf(keyString, "dev%d_nsram_mode", deviceObjPtr->deviceId);
        if (SIM_OS_MAC(simOsGetCnfValue)("system", keyString,
                              SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS,
                              param_str))
        {
            sscanf(param_str, "%d", &param_val);
            nSramMode = param_val;
        }

        switch (nSramMode)
        {
        case TIGER_GM_NSRAM_ONE_FOUR_INTERNAL_E:
            devMemInfoPtr->nsramsMem[0].nSramSize = 0;
            devMemInfoPtr->nsramsMem[1].nSramSize = nSramSize;
            devMemInfoPtr->nsramsMem[2].nSramSize = nSramSize;
            devMemInfoPtr->nsramsMem[3].nSramSize = SMEM_TIGER_GM_NSRAM_INTERNAL;
        break;
        case TIGER_GM_NSRAM_ALL_EXTERNAL_E:
            devMemInfoPtr->nsramsMem[0].nSramSize = 0;
            devMemInfoPtr->nsramsMem[1].nSramSize = nSramSize;
            devMemInfoPtr->nsramsMem[2].nSramSize = nSramSize;
            devMemInfoPtr->nsramsMem[3].nSramSize = 0;
            break;
        case TIGER_GM_NSRAM_ONE_TWO_INTERNAL_E:
            devMemInfoPtr->nsramsMem[0].nSramSize = 0;
            devMemInfoPtr->nsramsMem[1].nSramSize = nSramSize;
            devMemInfoPtr->nsramsMem[2].nSramSize = SMEM_TIGER_GM_NSRAM_INTERNAL;
            devMemInfoPtr->nsramsMem[3].nSramSize = 0;
            break;
        case TIGER_GM_NSRAM_THREE_FOUR_INTERNAL_E:
            devMemInfoPtr->nsramsMem[0].nSramSize = 0;
            devMemInfoPtr->nsramsMem[1].nSramSize = nSramSize;
            devMemInfoPtr->nsramsMem[2].nSramSize = SMEM_TIGER_GM_NSRAM_INTERNAL;
            devMemInfoPtr->nsramsMem[3].nSramSize = SMEM_TIGER_GM_NSRAM_INTERNAL;
            break;
        case TIGER_GM_NSRAM_ALL_INTERNAL_E:
            devMemInfoPtr->nsramsMem[0].nSramSize = 0;
            devMemInfoPtr->nsramsMem[1].nSramSize = 0;
            devMemInfoPtr->nsramsMem[2].nSramSize = SMEM_TIGER_GM_NSRAM_INTERNAL;
            devMemInfoPtr->nsramsMem[3].nSramSize = SMEM_TIGER_GM_NSRAM_INTERNAL;
            break;
        default:
            skernelFatalError("smemTigerGMNsramConfigUpdate: invalid NSRAM mode\n");
        }
    }
    else
    {
        devMemInfoPtr->nsramsMem[0].nSramSize = nSramSize;
    }
}

/*******************************************************************************
*   smemTigerGMInit
*
* DESCRIPTION:
*       Init memory module for a TigerGM device.
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
void smemTigerGMInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    TIGER_GM_DEV_MEM_INFO  * devMemInfoPtr;

    /* string for parameter */
    char   param_str[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
    /* key string */
    char   keyString[20];

    /* alloc TIGER_GM_DEV_MEM_INFO */
    devMemInfoPtr = (TIGER_GM_DEV_MEM_INFO *)calloc(1, sizeof(TIGER_GM_DEV_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
            skernelFatalError("smemTigerGMInit: allocation error\n");
    }

    deviceObjPtr->deviceMemory = devMemInfoPtr;

    /* get wide SRAM size from *.ini file */
    sprintf(keyString, "dev%d_wsram", deviceObjPtr->deviceId);
    if (!SIM_OS_MAC(simOsGetCnfValue)("system",  keyString,
                             SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        /* use internal 128Kb ram */
        devMemInfoPtr->wsramMem.wSramSize = 128 * 1024 / sizeof(SMEM_REGISTER);
    }
    else
    {
        /* get wsram size in bytes */
        smemMemSizeToBytes(param_str, &devMemInfoPtr->wsramMem.wSramSize);
        /* convert to words number */
        devMemInfoPtr->wsramMem.wSramSize /= sizeof(SMEM_REGISTER);
    }

    /* init narrow sram lpm access per ip octets for ipv4 & ipv6 */
    smemTigerGMNsramConfigUpdate(deviceObjPtr);

    /* get flow DRAM size from *.ini file */
    sprintf(keyString, "dev%d_fdram", deviceObjPtr->deviceId);
    if (!SIM_OS_MAC(simOsGetCnfValue)("system",  keyString,
                             SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        devMemInfoPtr->fdramMem.fDramSize = 0;
    }
    else
    {
        /* get FDram size in bytes */
        smemMemSizeToBytes(param_str, &devMemInfoPtr->fdramMem.fDramSize);
        /* convert to words number */
        devMemInfoPtr->fdramMem.fDramSize /= sizeof(SMEM_REGISTER);
    }

    /* init specific functions array */
    smemTigerGMInitFuncArray(devMemInfoPtr);

    /* allocate address type specific memories */
    smemTigerGMAllocSpecMemory(devMemInfoPtr);

    deviceObjPtr->devFindMemFunPtr = (void *)smemTigerGMFindMem;

    /* init FA registers - temporary patch */
    smemFaMemory[(0xAC/4)] = 0x11AB;

}
/*******************************************************************************
*   smemTigerGMFindMem
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
void * smemTigerGMFindMem
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    void                    *   memPtr;
    TIGER_GM_DEV_MEM_INFO   *   devMemInfoPtr;
    GT_32                       index;
    GT_U32                      param;
    GT_32                       dramType;

    if (deviceObjPtr == 0)
    {
        skernelFatalError("smemTigerGMFindMem: illegal pointer \n");
    }
    if (activeMemPtrPtr != NULL)
    {
        *activeMemPtrPtr = NULL;
    }

    memPtr = 0;
    devMemInfoPtr = (TIGER_GM_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Find PCI registers memory  */
    if (SMEM_ACCESS_PCI_FULL_MAC(accessType))
    {
        return smemTigerGMPciReg(deviceObjPtr, address, memSize, 0);
    }

    if ((address & 0x40000000) > 0 )
    {
        /* FA memory */
        memPtr = smemTigerGMFaMem(deviceObjPtr, address, memSize, 0);
        return memPtr;
    }
    else if ((address & 0x20000000) > 0 ) {
        /* Find external memory */
        dramType = ((address >> 27) & 0x03);

        memPtr = smemTigerGMExternalMem(deviceObjPtr, address, memSize, dramType);
    }
    else {
        /* Find common registers memory */
        index = (address & REG_SPEC_FUNC_INDEX) >> 23;
        if (index > 63)
        {
            skernelFatalError("smemTigerGMFindMem: index is out of range\n");
        }
        /* Call register spec function to obtain pointer to register memory */
        param   = devMemInfoPtr->specFunTbl[index].specParam;
        memPtr  = devMemInfoPtr->specFunTbl[index].specFun(deviceObjPtr,
                                                           accessType,
                                                           address,
                                                           memSize,
                                                           param);
    }

    return memPtr;
}

/*******************************************************************************
*   smemTigerGMGlobalReg
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
static GT_U32 *  smemTigerGMGlobalReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_GM_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                 * regValPtr;
    GT_U32                  index;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_GM_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Global registers */
    if ((address & 0x1FFFFF00) == 0x0){
        index = (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.globRegs,
                         devMemInfoPtr->globalMem.globRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.globRegs[index];
    }else
     /* TWSI registers */
    if ((address & 0x1FFFFF00) == 0x00400000) {
        index = (address & 0x18) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.twsiIntRegs,
                         devMemInfoPtr->globalMem.twsiIntRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.twsiIntRegs[index];
    }
     /* SDMA registers */
    if ((address & 0x1FFFF000) == 0x00002000) {
        index = (address & 0x8FF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->globalMem.sdmaRegs,
                         devMemInfoPtr->globalMem.sdmaRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->globalMem.sdmaRegs[index];
    }

    return regValPtr;
}
/*******************************************************************************
*   smemTigerGMTransQueReg
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
static GT_U32 *  smemTigerGMTransQueReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_GM_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              port, offset, index;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_GM_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;

    /* Port Traffic Class Descriptor Limit Register */
    if ((address & 0x1E7F0FFF) >= 0x200 &&
        (address & 0x1E7F0FFF) < 0x280){
        port = ((address >> 12) & 0xFF);
        offset =  ((address >> 4) & 0xF);
        index = port + offset;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egressMem.tcRegs,
                         devMemInfoPtr->egressMem.tcRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egressMem.tcRegs[index];
    }
    else
    /* Port Transmit Configuration Register */
    if ((address & 0x1E7F0FFF) == 0x0){
        index = ((address >> 12) & 0xFF);
        CHECK_MEM_BOUNDS(devMemInfoPtr->egressMem.transConfRegs,
                         devMemInfoPtr->egressMem.transConfRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egressMem.transConfRegs[index];
    }
    else
    /* Port Descriptor Limit Register */
    if ((address & 0x1E7F0FFF) == 0x100){
        index = ((address >> 12) & 0xFF);
        CHECK_MEM_BOUNDS(devMemInfoPtr->egressMem.descLimRegs,
                         devMemInfoPtr->egressMem.descLimRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egressMem.descLimRegs[index];
    }
    else
    /* Trunk registers 0x0180.280 , 0x0181.280 , 0x0182.280 , 0x0183.280
        24 registers for low (8 non-trunk , 16 designated ports )
        24 registers for Hi  (8 non-trunk , 16 designated ports )*/
    if ((address & 0x1E7C0000) == 0x0 && ((address & 0xFFF)==0x280)){
        index = ((address&0x20000)?1:0)*24 + ((address & 0x1F000)>>12);
        CHECK_MEM_BOUNDS(devMemInfoPtr->egressMem.trunkMemRegs,
                         devMemInfoPtr->egressMem.trunkRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egressMem.trunkMemRegs[index];
    }
    else
    /* General registers */
    if ((address & 0x1E700000) == 0x0){
        index = (address & 0x1FFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egressMem.genRegs,
                         devMemInfoPtr->egressMem.genRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egressMem.genRegs[index];
    }
    else
    /* External Memory configuration register */
    if ((address & 0x11900000) == 0x01900000){
        index = ((address & 0x294) >> 4) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->egressMem.extMemRegs,
                         devMemInfoPtr->egressMem.extMemRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->egressMem.extMemRegs[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemTigerGMEtherBrdgReg
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
static GT_U32 *  smemTigerGMEtherBrdgReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_GM_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index, port;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_GM_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Port Bridge control registers */
    if ((address & 0x1DFC0800) == 0x0){
        port = (address & 0x3F000) >> 12;
        index = ((address & 0x1F) / 0x4 * BRG_PORTS_NUM) + port;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bridgeMngMem.portRegs,
                         devMemInfoPtr->bridgeMngMem.portRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bridgeMngMem.portRegs[index];
    }
    else
    /* Ports Protocol Based VLANs configuration Registers */
    if ((address & 0x1DFC0F00) == 0x00000800) {
        port = (address & 0x3F000) >> 12;
        index = (((address & 0x1F) / 0x4) * BRG_PORTS_NUM) + port;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bridgeMngMem.portProtVidRegs,
                         devMemInfoPtr->bridgeMngMem.portProtVidRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bridgeMngMem.portProtVidRegs[index];
    }
    else
    /* General Bridge managment registers array */
    if ((address & 0x1DFFF000) == 0x00040000) {
        index = (address & 0x3FF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bridgeMngMem.genRegs,
                         devMemInfoPtr->bridgeMngMem.genRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bridgeMngMem.genRegs[index];
    }
    return regValPtr;
}

/*******************************************************************************
*   smemTigerGMIpV4Reg
*
* DESCRIPTION:
*       IPv4 Processor Registers
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
static GT_U32 *  smemTigerGMLxReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_GM_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;
    GT_BOOL             sambaDev ;

    sambaDev = (deviceObjPtr->deviceFamily == SKERNEL_SAMBA_FAMILY)?
                GT_TRUE:GT_FALSE;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_GM_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* General LX unit registers */
    if ((address & 0x007f0000) == 0x00000000){ /*0x0280....*/
        index = (address & 0x1FF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.genRegs,
                         devMemInfoPtr->lxMem.genRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.genRegs[index];
    }
    else
    /* MPLS Processor  Registers array */
    if ((address & 0x007f0000) == 0x00500000){ /*0x02d0....*/
        index = (address & 0xff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.mplsRegs,
                         devMemInfoPtr->lxMem.mplsRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.mplsRegs[index];
    }
    else
    /* DSCP to CoS Table */
    if ((address & 0x007f0000) == 0x00520000) { /*0x02d2....*/
        index = (address & 0x3ff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.dscpRegs,
                         devMemInfoPtr->lxMem.dscpRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.dscpRegs[index];
    }
    else
    /* PCL tables registers number */
    if ((address & 0x007f0000) == 0x00540000) { /*0x02d4....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.pclTblRegs,
                         devMemInfoPtr->lxMem.pclTblRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.pclTblRegs[index];
    }
    else
    /* Class of Service Re-marking Table Register */
    if ((address & 0x007f0000) == 0x00560000) { /*0x02d6....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.cosRegs,
                         devMemInfoPtr->lxMem.cosRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.cosRegs[index];
    }
    else
    /* Trunk Member Table Register number */
    if ((address & 0x007f0000) == 0x00580000) { /*0x02d8....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.trunkRegs,
                         devMemInfoPtr->lxMem.trunkRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.trunkRegs[index];
    }
    else
    /* Counter block registers registers */
    if ((address & 0x007f0000) == 0x004C0000) { /*0x02cc....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.countRegs,
                         devMemInfoPtr->lxMem.countRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.countRegs[index];
    }
    /* cos Lookup Table registers */
    else if ((address & 0x007f0000) == 0x00400000) { /*0x02c0....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.cosLookupTblReg,
                         devMemInfoPtr->lxMem.cosLookupTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.cosLookupTblReg[index];
    }
    /* cos Tables registers */
    else if ((address & 0x007f0000) == 0x00480000) { /*0x02c8....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.cosTablesReg,
                         devMemInfoPtr->lxMem.cosTablesRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.cosTablesReg[index];
    }
    /*  dscp and Dp to Cos Mark and Remark Table registers */
    else if ((address & 0x007f0000) == 0x004a0000) { /*0x02ca....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.dscpDptoCosMarkRemarkTableReg,
                         devMemInfoPtr->lxMem.dscpDptoCosMarkRemarkTableRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.dscpDptoCosMarkRemarkTableReg[index];
    }
    /*   internal Inlif table registers */
    else if ((address & 0x007f0000) == 0x00460000) { /*0x02c6....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.internalInlifTblReg,
                         devMemInfoPtr->lxMem.internalInlifTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.internalInlifTblReg[index];
    }
    /*  flow Template Hash Config table registers */
    else if ((address & 0x007f8000) == 0x00428000) { /*0x02c28...*/
        index = (address & 0x7fff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.flowTemplateHashConfigTbl,
                         devMemInfoPtr->lxMem.flowTemplateHashConfigTblNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.flowTemplateHashConfigTbl[index];
    }
    /*  flow Template Hash select table registers */
    else if ((address & 0x007f8000) == 0x00420000) { /*0x02c20...*/
        index = (address & 0x7fff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.flowTemplateHashSelectTbl,
                         devMemInfoPtr->lxMem.flowTemplateHashSelectTblNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.flowTemplateHashSelectTbl[index];
    }
    /*  flow Template select table registers */
    else if ((address & 0x007f8000) == 0x00440000) { /*0x02c4....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.flowTemplateSelectTbl,
                         devMemInfoPtr->lxMem.flowTemplateSelectTblNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.flowTemplateSelectTbl[index];
    }
    /* Counter block 2 registers registers */
    if ((address & 0x007f0000) == 0x004e0000) { /*0x02ce....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.count2Regs,
                         devMemInfoPtr->lxMem.count2RegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.count2Regs[index];
    }
    else
    /* External Memory configuration registers */
    if ((address & 0x007f0000) == 0x00600000) { /*0x02e0....*/
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.extMemRegs,
                         devMemInfoPtr->lxMem.extMemRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.extMemRegs[index];
    }
    else if(sambaDev && ((address & 0x007f0000) == 0x00620000)) /*0x02e2....*/
    {
        /* tcam data memory */
        index = (address & 0xffff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.tcamTblReg,
                         devMemInfoPtr->lxMem.tcamTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.countRegs[index];
    }
    else if(sambaDev && ((address & 0x007f4000) == 0x00634000)) /*0x02e34...*/
    {
        /* pce table memory */
        index = (address & 0x3fff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->lxMem.pceActionsTblReg,
                         devMemInfoPtr->lxMem.pceActionsTblRegNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->lxMem.countRegs[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemTigerGMBufMngReg
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
static GT_U32 *  smemTigerGMBufMngReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_GM_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_GM_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Control Linked List buffers managment registers array */
    if ((address & 0x1CFF0000) == 0x00010000) {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.bmRegs,
                         devMemInfoPtr->bufMngMem.bmRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.bmRegs[index];
    }
    else
    /* Control Linked List buffers managment registers array */
    if ((address & 0x1CFF0000) == 0x00020000) {
        index = (address & 0xFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.epfRegs,
                         devMemInfoPtr->bufMngMem.epfRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.epfRegs[index];
    }
    else
    /* Control Linked List buffers managment registers array */
    if ((address & 0x1CF0F000) == 0x00004000) {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.egrRegs,
                         devMemInfoPtr->bufMngMem.egrRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.egrRegs[index];
    }
    else if(((address & 0x03000000) == 0x03000000) &&
            ((address & 0x00fff000) == 0x00000000))
    {
        index = (address & 0x000002DC) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->bufMngMem.trunkRegs,
                         devMemInfoPtr->bufMngMem.trunkRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->bufMngMem.trunkRegs[index];
    }

    return regValPtr;
}

/*******************************************************************************
*   smemTigerGMPciReg
*
* DESCRIPTION:
*       PCI memory access
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
static GT_U32 *  smemTigerGMPciReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_GM_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_GM_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Control Linked List buffers managment registers array */
    if ((address & 0xFFFFF00) == 0x00000000) {
        index = (address & 0x3C) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pciMem.pciRegsRegs,
                         devMemInfoPtr->pciMem.pciRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pciMem.pciRegsRegs[index];
    }
    else
    /* Prestera specific PCI registers */
    if ((address & 0xFFFFFF00) == 0x00000100) {
        index = (address & 0x18) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->pciMem.pciInterRegs,
                         devMemInfoPtr->pciMem.pciInterRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->pciMem.pciInterRegs[index];
    }

    return regValPtr;
}
/*******************************************************************************
*   smemTigerGMMacReg
*
* DESCRIPTION:
*       Describe a MAC registers
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
static GT_U32 *  smemTigerGMMacReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_GM_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_GM_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Control Linked List buffers managment registers array */
    if ((address & 0x1C7FFF00) == 0x00000000) {
        index = (address & 0xff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->macMem.genRegs,
                         devMemInfoPtr->macMem.genRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->macMem.genRegs[index];
    }
    else
    if ((address & 0x1C700100) == 0x00000100) {
        index = (address & 0xff) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->macMem.sflowRegs,
                         devMemInfoPtr->macMem.sflowRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->macMem.sflowRegs[index];
    }
    return regValPtr;
}
/*******************************************************************************
*   smemTigerGMPortGroupConfReg
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
static GT_U32 *  smemTigerGMPortGroupConfReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_GM_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_GM_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Per port MAC Control Register */
    if ((address & 0x1E7F4000) == 0x00000000){
        index  = 24 * param + ((address & 0xFF00) >> 8);
        CHECK_MEM_BOUNDS(devMemInfoPtr->portMem.perPortCtrlRegs,
                         devMemInfoPtr->portMem.perPortCtrlRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->portMem.perPortCtrlRegs[index];
    }
    else
    /* Per port MAC Status Register */
    if ((address & 0x1E7F4004) == 0x00000004){
        index  = 24 * param + ((address & 0xFF00) >> 8);
        CHECK_MEM_BOUNDS(devMemInfoPtr->portMem.perPortStatRegs,
                         devMemInfoPtr->portMem.perPortStatRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->portMem.perPortStatRegs[index];
    }
    else
    /* Per group registres */
    if ((address & 0x1E7F4000) == 0x00004000) {
        index = param * (PER_GROUPS_TYPE_NUM / 2) + (address & 0xFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->portMem.perGroupRegs,
                         devMemInfoPtr->portMem.perGroupRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->portMem.perGroupRegs[index];
    }
    else
    /* MAC counters array */
    if ((address & 0x1E010000) == 0x00010000) {
        index = (address & 0xFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->portMem.macCnts,
                         devMemInfoPtr->portMem.macCntsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->portMem.macCnts[index];
    }
    return regValPtr;
}

/*******************************************************************************
*   smemTigerGMMacTableReg
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
static GT_U32 *  smemTigerGMMacTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_GM_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_GM_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Registers, index is group number */
    if ((address & 0x16000000) == 0x06000000){
        index = (address & 0xFFFFF) / 0x4;
        CHECK_MEM_BOUNDS(devMemInfoPtr->macFbdMem.macTblRegs,
                         devMemInfoPtr->macFbdMem.macTblRegsNum,
                         index, memSize);
        regValPtr = &devMemInfoPtr->macFbdMem.macTblRegs[index];
    }
    return regValPtr;
}


/*******************************************************************************
*   smemTigerGMVlanTableReg
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
static GT_U32 *  smemTigerGMVlanTableReg(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_GM_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;

    regValPtr = 0;
    devMemInfoPtr = (TIGER_GM_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;
    /* Vlan Table Registers */
    index = (address & 0xFFFFF) / 0x4;
    CHECK_MEM_BOUNDS(devMemInfoPtr->vlanTblMem.vlanTblRegs,
                     devMemInfoPtr->vlanTblMem.vlanTblRegNum,
                     index, memSize);
    regValPtr = &devMemInfoPtr->vlanTblMem.vlanTblRegs[index];


    return regValPtr;
}

static GT_U32 *  smemTigerGMFatalError(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    skernelFatalError("smemTigerGMFatalError: illegal function pointer\n");

    return 0;
}
/*******************************************************************************
*   smemTigerGMInitMemArray
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
static void smemTigerGMInitFuncArray(
    INOUT TIGER_GM_DEV_MEM_INFO  * devMemInfoPtr
)
{
    GT_U32              i;

    for (i = 0; i < 64; i++) {
        devMemInfoPtr->specFunTbl[i].specFun    = smemTigerGMFatalError;
    }
    devMemInfoPtr->specFunTbl[0].specFun        = smemTigerGMGlobalReg;

    devMemInfoPtr->specFunTbl[1].specFun        = smemTigerGMPortGroupConfReg;
    devMemInfoPtr->specFunTbl[1].specParam      = 0;

    devMemInfoPtr->specFunTbl[2].specFun        = smemTigerGMPortGroupConfReg;
    devMemInfoPtr->specFunTbl[2].specParam      = 1;

    devMemInfoPtr->specFunTbl[3].specFun        = smemTigerGMTransQueReg;

    devMemInfoPtr->specFunTbl[4].specFun        = smemTigerGMEtherBrdgReg;

    devMemInfoPtr->specFunTbl[5].specFun        = smemTigerGMLxReg;

    devMemInfoPtr->specFunTbl[6].specFun        = smemTigerGMBufMngReg;

    devMemInfoPtr->specFunTbl[7].specFun        = smemTigerGMMacReg;

    devMemInfoPtr->specFunTbl[12].specFun       = smemTigerGMMacTableReg;

    devMemInfoPtr->specFunTbl[20].specFun       = smemTigerGMVlanTableReg;
}
/*******************************************************************************
*   smemTigerGMAllocSpecMemory
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
static void smemTigerGMAllocSpecMemory(
    INOUT TIGER_GM_DEV_MEM_INFO  * devMemInfoPtr
)
{
    int i;                      /* index of memory array */

    /* Global register memory allocation */
    devMemInfoPtr->globalMem.globRegsNum = GLOB_REGS_NUM;
    devMemInfoPtr->globalMem.globRegs =
            calloc(GLOB_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.globRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->globalMem.twsiIntRegsNum = TWSI_INT_REGS_NUM;
    devMemInfoPtr->globalMem.twsiIntRegs =
        calloc(TWSI_INT_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.twsiIntRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->globalMem.sdmaRegsNum = SDMA_REGS_NUM;
    devMemInfoPtr->globalMem.sdmaRegs =
        calloc(SDMA_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->globalMem.sdmaRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }
    devMemInfoPtr->globalMem.currentDescrPtr = 0;

    /* Buffer management register memory allocation */
    devMemInfoPtr->bufMngMem.bmRegsNum = BM_REGS_NUM;
    devMemInfoPtr->bufMngMem.bmRegs =
        calloc(BM_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.bmRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->bufMngMem.epfRegsNum = EPF_REGS_NUM;
    devMemInfoPtr->bufMngMem.epfRegs =
        calloc(EPF_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.epfRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* Egress Mirrored Ports registers memory allocation */
    devMemInfoPtr->bufMngMem.egrRegsNum = EGR_REGS_NUM;
    devMemInfoPtr->bufMngMem.egrRegs =
        calloc(EGR_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.egrRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->bufMngMem.trunkRegsNum = TIGERGM_C_TRUNK_TBL_REGS_NUM;
    devMemInfoPtr->bufMngMem.trunkRegs =
        calloc(TIGERGM_C_TRUNK_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bufMngMem.trunkRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }


    /*  IPV4 General registers memory allocation */
    devMemInfoPtr->lxMem.genRegsNum = LX_GEN_REGS_NUM;
    devMemInfoPtr->lxMem.genRegs =
        calloc(LX_GEN_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.genRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }
    /* MPLS Processor Registers array */
    devMemInfoPtr->lxMem.mplsRegsNum = MPLS_NUM;
    devMemInfoPtr->lxMem.mplsRegs =
        calloc(MPLS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.mplsRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* DSCP registers memory allocation */
    devMemInfoPtr->lxMem.dscpRegsNum = DSCP_REGS_NUM;
    devMemInfoPtr->lxMem.dscpRegs =
        calloc(DSCP_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.dscpRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }
    /* PCL table Registers  */
    devMemInfoPtr->lxMem.pclTblRegsNum = PCL_TBL_REGS_NUM;
    devMemInfoPtr->lxMem.pclTblRegs =
        calloc(PCL_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.pclTblRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->lxMem.tcamTblRegNum = TCAM_TBL_REG_NUM;
    devMemInfoPtr->lxMem.tcamTblReg =
        calloc(TCAM_TBL_REG_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.tcamTblReg == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->lxMem.pceActionsTblRegNum = PCE_ACTIONS_TBL_REG_NUM;
    devMemInfoPtr->lxMem.pceActionsTblReg =
        calloc(PCE_ACTIONS_TBL_REG_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.pceActionsTblReg == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* Class of Service Re-marking Table Register number */
    devMemInfoPtr->lxMem.cosRegsNum = COS_REGS_NUM;
    devMemInfoPtr->lxMem.cosRegs =
        calloc(COS_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.cosRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* Trunk Member Table Register array */
    devMemInfoPtr->lxMem.trunkRegsNum = TRUNK_TBL_REGS_NUM;
    devMemInfoPtr->lxMem.trunkRegs =
        calloc(TRUNK_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.trunkRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* Counter block registers memory allocation */
    devMemInfoPtr->lxMem.countRegsNum = COUNT_BLOCK_REGS_NUM;
    devMemInfoPtr->lxMem.countRegs =
        calloc(COUNT_BLOCK_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.countRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* Counter block 2 registers memory allocation */
    devMemInfoPtr->lxMem.count2RegsNum = COUNT_BLOCK_2_REGS_NUM;
    devMemInfoPtr->lxMem.count2Regs =
        calloc(COUNT_BLOCK_2_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.count2Regs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* cos lookup tbl reg */
    devMemInfoPtr->lxMem.cosLookupTblRegNum = COS_LOOKUP_TBL_REG_NUM;
    devMemInfoPtr->lxMem.cosLookupTblReg =
        calloc(COS_LOOKUP_TBL_REG_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.cosLookupTblReg == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* cos tables reg*/
    devMemInfoPtr->lxMem.cosTablesRegNum = COS_TABLES_REG_NUM;
    devMemInfoPtr->lxMem.cosTablesReg =
        calloc(COS_TABLES_REG_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.cosTablesReg == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* dscp dp to cos mark remark table reg */
    devMemInfoPtr->lxMem.dscpDptoCosMarkRemarkTableRegNum = DSCP_DP_TO_COS_MARK_REMARK_TABLE_REG_NUM;
    devMemInfoPtr->lxMem.dscpDptoCosMarkRemarkTableReg =
        calloc(DSCP_DP_TO_COS_MARK_REMARK_TABLE_REG_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.dscpDptoCosMarkRemarkTableReg == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* internal inlif tbl reg*/
    devMemInfoPtr->lxMem.internalInlifTblRegNum = INTERNAL_INLIF_TBL_REG_NUM ;
    devMemInfoPtr->lxMem.internalInlifTblReg =
        calloc(INTERNAL_INLIF_TBL_REG_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.internalInlifTblReg == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* flow template hash config tbl*/
    devMemInfoPtr->lxMem.flowTemplateHashConfigTblNum = FLOW_TEMPLATE_HASH_CONFIG_TBL_NUM;
    devMemInfoPtr->lxMem.flowTemplateHashConfigTbl =
        calloc(FLOW_TEMPLATE_HASH_CONFIG_TBL_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.flowTemplateHashConfigTbl == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* flow template hash select tbl */
    devMemInfoPtr->lxMem.flowTemplateHashSelectTblNum = FLOW_TEMPLATE_HASH_SELECT_TBL_NUM;
    devMemInfoPtr->lxMem.flowTemplateHashSelectTbl =
        calloc(FLOW_TEMPLATE_HASH_SELECT_TBL_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.flowTemplateHashSelectTbl == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* flow template select tbl */
    devMemInfoPtr->lxMem.flowTemplateSelectTblNum = FLOW_TEMPLATE_SELECT_TBL_NUM;
    devMemInfoPtr->lxMem.flowTemplateSelectTbl =
        calloc(FLOW_TEMPLATE_SELECT_TBL_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.flowTemplateSelectTbl == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }


    devMemInfoPtr->lxMem.extMemRegsNum = EXT_MEM_CONF_NUM;
    devMemInfoPtr->lxMem.extMemRegs =
        calloc(EXT_MEM_CONF_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->lxMem.extMemRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* Serial management interface registers memory allocation */
    devMemInfoPtr->macMem.genRegsNum = MAC_REGS_NUM;
    devMemInfoPtr->macMem.genRegs =
        calloc(MAC_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macMem.genRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* Network ports register memory allocation */
    devMemInfoPtr->portMem.perPortCtrlRegsNum = MAC_PORTS_NUM;
    devMemInfoPtr->portMem.perPortCtrlRegs =
        calloc(MAC_PORTS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->portMem.perPortCtrlRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->portMem.perPortStatRegsNum = MAC_PORTS_NUM;
    devMemInfoPtr->portMem.perPortStatRegs =
        calloc(MAC_PORTS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->portMem.perPortStatRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->portMem.perGroupRegsNum = PER_GROUPS_TYPE_NUM;
    devMemInfoPtr->portMem.perGroupRegs =
        calloc(MAC_PORTS_NUM * PER_GROUPS_TYPE_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->portMem.perGroupRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->portMem.macCntsNum = MAC_CNT_TYPES_NUM;
    devMemInfoPtr->portMem.macCnts =
        calloc(MAC_CNT_TYPES_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->portMem.macCnts == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }
    devMemInfoPtr->macMem.sflowRegsNum = SFLOW_NUM;
    devMemInfoPtr->macMem.sflowRegs = calloc(SFLOW_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macMem.sflowRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }
    memset(devMemInfoPtr->macMem.sflowFifo, 0,
             TIGER_SFLOW_FIFO_SIZE * sizeof(GT_U32));

    /* Bridge configuration register memory allocation */
    devMemInfoPtr->bridgeMngMem.portRegsNum =
            BRG_PORTS_NUM * PORT_REGS_GROUPS_NUM;
    devMemInfoPtr->bridgeMngMem.portRegs =
        calloc(BRG_PORTS_NUM * PORT_REGS_GROUPS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bridgeMngMem.portRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->bridgeMngMem.portProtVidRegsNum =
            BRG_PORTS_NUM * PORT_PROT_VID_NUM;
    devMemInfoPtr->bridgeMngMem.portProtVidRegs =
        calloc(BRG_PORTS_NUM * PORT_PROT_VID_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bridgeMngMem.portProtVidRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->bridgeMngMem.genRegsNum = BRG_GEN_REGS_NUM;
    devMemInfoPtr->bridgeMngMem.genRegs =
        calloc(BRG_GEN_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->bridgeMngMem.genRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* Egress configuration register memory allocation */
    devMemInfoPtr->egressMem.genRegsNum = GEN_EGRS_GROUP_NUM;
    devMemInfoPtr->egressMem.genRegs =
        calloc(GEN_EGRS_GROUP_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.genRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }
    /* Port Traffic Class Descriptor Limit Register */
    devMemInfoPtr->egressMem.tcRegsNum = TC_QUE_DESCR_NUM * MAC_PORTS_NUM;
    devMemInfoPtr->egressMem.tcRegs =
        calloc(TC_QUE_DESCR_NUM * MAC_PORTS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.tcRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }
    /* Port Transmit Configuration Register */
    devMemInfoPtr->egressMem.transConfRegsNum = MAC_PORTS_NUM;
    devMemInfoPtr->egressMem.transConfRegs =
        calloc(MAC_PORTS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.transConfRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* Port Descriptor Limit Register */
    devMemInfoPtr->egressMem.descLimRegsNum = MAC_PORTS_NUM;
    devMemInfoPtr->egressMem.descLimRegs =
        calloc(MAC_PORTS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.descLimRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* External memory configuration registers */
    devMemInfoPtr->egressMem.extMemRegsNum = EXT_MEM_CONF_NUM;
    devMemInfoPtr->egressMem.extMemRegs =
        calloc(EXT_MEM_CONF_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.extMemRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* trunk non-trunk table and designated trunk ports registers */
    devMemInfoPtr->egressMem.trunkRegsNum = EGR_TRUNK_REGS_NUM;
    devMemInfoPtr->egressMem.trunkMemRegs =
        calloc(EGR_TRUNK_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->egressMem.trunkMemRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* MAC MIB counters register memory allocation */
    devMemInfoPtr->macFbdMem.macTblRegsNum = MAC_TBL_REGS_NUM;
    devMemInfoPtr->macFbdMem.macTblRegs =
        calloc(MAC_TBL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->macFbdMem.macTblRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* VLAN translation table register memory allocation */
    devMemInfoPtr->vlanTblMem.vlanTblRegNum = VLAN_TABLE_REGS_NUM;
    devMemInfoPtr->vlanTblMem.vlanTblRegs =
        calloc(VLAN_TABLE_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->vlanTblMem.vlanTblRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    /* External memory allocation */
    devMemInfoPtr->wsramMem.wSram =
        calloc(devMemInfoPtr->wsramMem.wSramSize, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->wsramMem.wSram == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    for (i = 0; i < 4; i++)
    {
        if (devMemInfoPtr->nsramsMem[i].nSramSize == 0)
        {
            continue;
        }
        devMemInfoPtr->nsramsMem[i].nSram =
            calloc(devMemInfoPtr->nsramsMem[i].nSramSize, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->nsramsMem[i].nSram == 0)
        {
            skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
        }
    }

    if (devMemInfoPtr->fdramMem.fDramSize)
    {
        devMemInfoPtr->fdramMem.fDram =
            calloc(devMemInfoPtr->fdramMem.fDramSize, sizeof(SMEM_REGISTER));
        if (devMemInfoPtr->fdramMem.fDram == 0)
        {
            skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
        }
    }

    /* PCI memory allocation */
    devMemInfoPtr->pciMem.pciRegsNum = PCI_MEM_REGS_NUM;
    devMemInfoPtr->pciMem.pciRegsRegs =
        calloc(PCI_MEM_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pciMem.pciRegsRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }

    devMemInfoPtr->pciMem.pciInterRegsNum = PCI_INTERNAL_REGS_NUM;
    devMemInfoPtr->pciMem.pciInterRegs =
        calloc(PCI_INTERNAL_REGS_NUM, sizeof(SMEM_REGISTER));
    if (devMemInfoPtr->pciMem.pciInterRegs == 0)
    {
        skernelFatalError("smemTigerGMAllocSpecMemory: allocation error\n");
    }
}

/*******************************************************************************
*  smemTigerGMExternalMem
*
* DESCRIPTION:
*       External memory access
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
*       01 - SRAM 0, the wide one
       10 - SRAM 1, the large one
       11 - Flow DDR, the one with 16 bit
*
*******************************************************************************/
static GT_U32 *  smemTigerGMExternalMem(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    TIGER_GM_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32              * regValPtr;
    GT_U32              index;
    regValPtr = 0;
    devMemInfoPtr = (TIGER_GM_DEV_MEM_INFO  *)deviceObjPtr->deviceMemory;

    index = (address & 0x7FFFFFF) / 0x4;

    if(SKERNEL_DEVICE_FAMILY_TIGER(deviceObjPtr->deviceType))
    {
        CHECK_MEM_BOUNDS(devMemInfoPtr->nsramsMem[param].nSram,
                         devMemInfoPtr->nsramsMem[param].nSramSize,
                         index, memSize);
        return &devMemInfoPtr->nsramsMem[param].nSram[index];
    }

    switch(param) {
        case 1: /* 0x28000000 */
            CHECK_MEM_BOUNDS(devMemInfoPtr->wsramMem.wSram,
                             devMemInfoPtr->wsramMem.wSramSize,
                             index, memSize);
            regValPtr = &devMemInfoPtr->wsramMem.wSram[index];
        break;
        case 2: /* 0x30000000 */
            CHECK_MEM_BOUNDS(devMemInfoPtr->nsramsMem[0].nSram,
                             devMemInfoPtr->nsramsMem[0].nSramSize,
                             index, memSize);
            regValPtr = &devMemInfoPtr->nsramsMem[0].nSram[index];
        break;
        case 3: /* 0x38000000 */
            CHECK_MEM_BOUNDS(devMemInfoPtr->fdramMem.fDram,
                             devMemInfoPtr->fdramMem.fDramSize,
                             index, memSize);
            regValPtr = &devMemInfoPtr->fdramMem.fDram[index];
        break;
    }

    return regValPtr;
}

/*******************************************************************************
*  smemMemSizeToBytes
*
* DESCRIPTION:
*       Conversion DRAM memory size from INI files to number of bytes
* INPUTS:
*       memSize - DRAM memory size
*       memBytes - memory size in bytes
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemMemSizeToBytes(
        IN GT_CHAR * memSizePtr,
        INOUT GT_U32 * memBytesPtr
)
{
        GT_U32 len;
        GT_U32 megaByte = 1;

        ASSERT_PTR(memSizePtr);
        ASSERT_PTR(memBytesPtr);

        len = (GT_U32)strlen(memSizePtr);
        if (memSizePtr[len - 1] == 'M' ||
                memSizePtr[len - 1] == 'm')
        {
                memSizePtr[len - 1] = 0;
                megaByte = 0x100000;
        }
        sscanf(memSizePtr, "%u", memBytesPtr);
        *memBytesPtr *= megaByte;
}

/*******************************************************************************
*  smemTigerGMFaMem
*
* DESCRIPTION:
*       External memory access
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for FA memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers's specific parameter.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*       01 - SRAM 0, the wide one
       10 - SRAM 1, the large one
       11 - Flow DDR, the one with 16 bit
*
*******************************************************************************/
static GT_U32 *  smemTigerGMFaMem(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    /* TIGER_GM_DEV_MEM_INFO  * devMemInfoPtr; */
    GT_U32              * regValPtr;
    GT_U32                index;

    index = (address & 0xff) / 4;
    regValPtr = &smemFaMemory[index];

    if ((address & 0xff) == 0x5c)
    {
        smemFaMemory[index] = 0x3;
    }

    return regValPtr;
}
