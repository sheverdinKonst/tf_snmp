/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemSoho.h
*
* DESCRIPTION:
*       Data definitions for Soho memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 13 $
*
*******************************************************************************/
#ifndef __smemSohoh
#define __smemSohoh

#include <asicSimulation/SKernel/smem/smem.h>
#include <asicSimulation/SKernel/suserframes/snetSoho.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Maximal devices number */
#define SOHO_SMI_DEV_TOTAL          (32)
/* Maximum registers number */
#define SOHO_REG_NUMBER             (32)
/* Maximum device ports number */
#define SOHO_PORTS_NUMBER           (11)

/* MAC table memory size in words */
#define SOHO_MAC_TABLE_8K_SIZE         (1024 * 8 * 3)
/* MAC table memory size in words */
#define SOHO_MAC_TABLE_4K_SIZE         (1024 * 4 * 3)

/* VLAN table entry size in words */
#define SOHO_VLAN_ENTRY_WORDS       3

/* VLAN table entry size in bytes */
#define SOHO_VLAN_ENTRY_BYTES       (SOHO_VLAN_ENTRY_WORDS )

/* VLAN table memory size */
#define SOHO_VLAN_TABLE_SIZE        (4 * 1024 * SOHO_VLAN_ENTRY_BYTES)

/* STU table entry size in words */
#define OPAL_STU_ENTRY_WORDS       3

/* STU table entry size in bytes */
#define OPAL_STU_ENTRY_BYTES       (OPAL_STU_ENTRY_WORDS * 4)

/* STU table memory size */
#define OPAL_STU_TABLE_SIZE        (64 * 22 * OPAL_STU_ENTRY_WORDS)

/* PVT table entry size  */
#define OPAL_PVT_ENTRY_BYTES       (0x200) /* 512 */

/* PVT table memory size */
#define OPAL_PVT_TABLE_SIZE        (512 * SOHO_PORTS_NUMBER)

/* Start address of phy related register.  */
#define SOHO_PHY_REGS_START_ADDR    (0x0)

/* Start address of ports related register.*/
#define SOHO_PORT_REGS_START_ADDR   (0x10)

/* Start address of global register.       */
#define SOHO_GLOBAL_REGS_START_ADDR (0x1b)

typedef enum {
    SOHO_NONE_E,
    SOHO_SRC_VTU_PORT_E,
    SOHO_MISS_VTU_VID_E,
    SOHO_FULL_VTU_E
} SOHO_VTU_VIOLATION_E;

typedef enum {
    SOHO_SRC_ATU_PORT_E,
    SOHO_MISS_ATU_E,
    SOHO_FULL_ATU_E
} SOHO_ATU_VIOLATION_E;

typedef enum
{
    SOHO_PORT_SPEED_10_MBPS_E,
    SOHO_PORT_SPEED_100_MBPS_E,
    SOHO_PORT_SPEED_1000_MBPS_E
} SOHO_PORT_SPEED_MODE_E;

/*
 * Typedef: struct SOHO_DEVICE_MEM
 *
 * Description:
 *      Describe a Soho registers memory object.
 *
 * Fields:
 *      globalRegsNum   - number of global registers
 *      globalRegs      - pointer to global registers memory
 *      global2RegsNum  - number of global 2 registers
 *      global2Regs     - pointer to global 2 registers memory
 *      phyRegsNum      - number of PHY registers
 *      phyRegs         - pointer to Phy registers memory
 *      smiRegsNum      - number of Smi registers
 *      smiRegs         - pointer to Smi registers memory
 *
 * Comments:
 *
 *      globalRegs      - registers with address offset 0x1B
 *                        globalRegsNum = 32 * 0x10
 *      phyRegs         - registers with address offset 0x0
 *                        phyRegsNum = (32 * 0x10) * 0x10
 *      smiRegs         - registers with address offset 0x10
 *                        smiRegsNum = (32 * 0x10) * 0x10
 */

typedef struct {
    GT_U32                  globalRegsNum;
    SMEM_REGISTER         * globalRegs;
    SMEM_REGISTER         * global2Regs;
    GT_U32                  phyRegsNum;
    SMEM_REGISTER         * phyRegs [SOHO_PORTS_NUMBER];
    GT_U32                  smiRegsNum;
    SMEM_REGISTER         * smiRegs [SOHO_PORTS_NUMBER];
}SOHO_DEVICE_MEM;

/*
 * Typedef: struct SOHO_ATU_DB_MEM
 *
 * Description:
 *      Describe a Soho MAC address table memory.
 *
 * Fields:
 *      macTblMemSize  - MAC address memory size
 *      macTblMem      - pointer to MAC address memory
 *
 * Comments:
 *
 *      macTblMemSize  - 4 * 1024 * 2 (4K entry, 2 bytes in the entry)
 */

typedef struct {
    GT_U32                  macTblMemSize;
    SMEM_REGISTER         * macTblMem;
    SOHO_ATU_VIOLATION_E    violation;
    GT_U16                  violationData[5];
}SOHO_ATU_DB_MEM;

/*
 * Typedef: struct OPAL_STU_DB_MEM
 * Description:
 *      Describe a Soho STU table memory.
 *
 * Fields:
 *      stuTblMemSize  - stu table memory size
 *      stuTblMem      - pointer to STU table memory
 *
 * Comments:
 *
 *      STUTblMemSize  - 4 * 1024 * 8 (4K entry, 22 bits in the entry)
 */

typedef struct {
    GT_U32                  stuTblMemSize;
    SMEM_REGISTER         * stuTblMem;
}OPAL_STU_DB_MEM;

/*
 * Typedef: struct OPAL_PVT_DB_MEM
 * Description:
 *      Describe a OPAL PVT table memory.
 *
 * Fields:
 *      pvtTblMemSize  - pvt table memory size
 *      pvtTblMem      - pointer to PVT table memory
 *
 * Comments:
 *
 *      PVTTblMemSize  - 11 * 512 (1/2 K entry, 11 bits in the entry)
 */

typedef struct {
    GT_U32                  pvtTblMemSize;
    SMEM_REGISTER         * pvtTblMem;
}OPAL_PVT_DB_MEM;

/*
 * Typedef: struct SOHO_VLAN_DB_MEM
 *
 * Description:
 *      Describe a Soho VLAN table memory.
 *
 * Fields:
 *      vlanTblMemSize  - VLAN table memory size
 *      vlanTblMem      - pointer to VLAN table memory
 *
 * Comments:
 *
 *      vlanTblMemSize  - 4 * 1024 * 8 (4K entry, 64 bits in the entry)
 */

typedef struct {
    GT_U32                  vlanTblMemSize;
    SMEM_REGISTER         * vlanTblMem;
    SOHO_VTU_VIOLATION_E    violation;
    GT_U16                  violationData[5];
}SOHO_VLAN_DB_MEM;

/*
 * Typedef: struct SOHO_TRUNK_DB_MEM  -- for trunk mask table
 *
 * Description:
 *      Describe a Soho TRUNK table memory.
 *
 * Fields:
 *      trunkTblMem      - TRUNK table memory
 *      readRegVal       - value of register 0x07 on global registers 2
 *
 * Comments:
 *
 */

typedef struct {
    SMEM_REGISTER           trunkTblMem[8];
    GT_U32                  readRegVal;
}SOHO_TRUNK_DB_MEM;

/*
 * Typedef: struct SOHO_TROUT_DB_MEM
 *
 * Description:
 *      Describe a Soho Trunk route table memory.
 *
 * Fields:
 *      trouteTblMem      - Trunk route table memory
 *      readRegVal       - value of register 0x08 on global registers 2
 *
 * Comments:
 *
 */

typedef struct {
    SMEM_REGISTER           trouteTblMem[16];
    GT_U32                  readRegVal;
}SOHO_TROUT_DB_MEM;


/*
 * Typedef: struct SOHO_DEV_ROUT_MEM
 *
 * Description:
 *      Describe a Soho target device table memory.
 *
 * Fields:
 *      deviceTblMem      - target device table memory
 *
 * Comments:
 *
 */

typedef struct {
    SMEM_REGISTER           deviceTblMem[32];
}SOHO_DEV_ROUT_MEM;

/*
 * Typedef: struct SOHO_CNT_MEM
 *
 * Description:
 *      Describe a Soho RMON statistic counters.
 *
 * Fields:
 *      cntStatTblSize          - RMON counters table size
 *      cntStatTblMem           - RMON counters table memory
 *
 * Comments:
 *
 */

typedef struct {
    GT_U32                  cntStatsTblSize;
    SMEM_REGISTER         * cntStatsTblMem[SOHO_PORTS_NUMBER];
    SMEM_REGISTER         * cntStatsCaptureMem;
}SOHO_CNT_MEM;


/*
 * Typedef: struct SOHO_DEV_FC_DELAY_MEM
 *
 * Description:
 *      Soho flow control delay memory
 *
 * Fields:
 *      fcDelayMem      - Flow Control Delay Time
 *
 * Comments:
 *
 */

typedef struct {
    SMEM_REGISTER           fcDelayMem[3];
}SOHO_DEV_FC_DELAY_MEM;

/*
 * Typedef: struct SOHO_DEV_MEM_INFO
 *
 * Description:
 *      Describe a device's memory object in the simulation.
 *
 * Fields:
 *      memMutex        : Memory mutex for device's memory.
 *      specFunTbl      : Address type specific R/W functions.
 *      devRegs         : Device registers (phy, ports and globals).
 *      macDbMem        : MAC table memory.
 *      vlanDbMem       : VLAN table memory.
 *
 * Comments:
 */

typedef struct {
    SMEM_SPEC_FIND_FUN_ENTRY_STC    specFunTbl[64];
    GT_U8                           accessMode;
    SOHO_DEVICE_MEM                 devRegs;
    SOHO_ATU_DB_MEM                 macDbMem;
    SOHO_VLAN_DB_MEM                vlanDbMem;
    SOHO_CNT_MEM                    statsCntMem;
    SOHO_TRUNK_DB_MEM               trunkMaskMem;
    SOHO_DEV_ROUT_MEM               trgDevMem;
    SOHO_TROUT_DB_MEM               trunkRouteMem;
    SOHO_DEV_FC_DELAY_MEM           flowCtrlDelayMem;
    OPAL_STU_DB_MEM                 stuDbMem;
    OPAL_PVT_DB_MEM                 pvtDbMem;
}SOHO_DEV_MEM_INFO;

/*******************************************************************************
*   smemSohoInit
*
* DESCRIPTION:
*       Init memory module for a Soho device.
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
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);

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
    IN SKERNEL_DEVICE_OBJECT * deviceObj,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
);

/*******************************************************************************
*  smemSohoGetVtuEntry
*
* DESCRIPTION:
*      Get VTU entry from vlan table SRAM
* INPUTS:
*       devObjPtr   - device object PTR.
*       vid         - VLAN ID
*       vtuEntryPtr - pointer to VTU entry in SRAM
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*******************************************************************************/
GT_STATUS smemSohoVtuEntryGet (
    IN  SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN  GT_U32 vid,
    OUT SNET_SOHO_VTU_STC * vtuEntry
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemSohoh */


