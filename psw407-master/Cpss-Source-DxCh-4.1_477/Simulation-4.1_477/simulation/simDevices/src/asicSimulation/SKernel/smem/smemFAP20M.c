/******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemFAP20M.c
*
* DESCRIPTION:
*       FAP20M memory mapping implementation.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 8 $
*
*******************************************************************************/

#include <os/simTypes.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/smem/smemFAP20M.h>


#define SAND_OFFSETOF(x,y)  ((unsigned long)((GT_UINTPTR)(&(((x *)0)->y))))
#define FAP20M_FDT_INDIRECT_MASK (FAP20M_FDT_MODULE_NUM << SAND_MODULE_SHIFT)
#define FAP20M_FDT_DU2_IND_ADDR  (FAP20M_FDT_INDIRECT_MASK | 0x00000080)
#define FAP20M_FDT_DM2_IND_ADDR  (FAP20M_FDT_INDIRECT_MASK | 0x000000C0)
#define FAP20M_FDT_DU4_IND_ADDR  (FAP20M_FDT_INDIRECT_MASK | 0x00000000)
#define FAP20M_FDT_DM4_IND_ADDR  (FAP20M_FDT_INDIRECT_MASK | 0x00000040)

typedef enum{
  FAP20M_SCH_ROLE_FABRIC,
  FAP20M_SCH_ROLE_EG_TM,
}FAP20M_SCH_ROLE;

typedef enum{
  FAP20M_SCH_RANK_PRIM,
  FAP20M_SCH_RANK_SCND,
  FAP20M_SCH_RANK_LAST,
}FAP20M_SCH_RANK;

#define INVALID_ADDRESS 0xFFFFFFFF

#define FAP20M_SCH_MODULE_NUM_PRIM (8)
#define FAP20M_SCH_MODULE_NUM_SCND (9)

#define FAP20M_SCH_MODULE_NUM(role)                        \
  ((role == FAP20M_SCH_RANK_PRIM)?                          \
  FAP20M_SCH_MODULE_NUM_PRIM:FAP20M_SCH_MODULE_NUM_SCND)

#define SAND_MODULE_MS_BIT           30
#define SAND_MODULE_NUM_BITS         4
#define SAND_MODULE_LS_BIT           (SAND_MODULE_MS_BIT + 1 - SAND_MODULE_NUM_BITS)

#define SAND_MODULE_SHIFT            SAND_MODULE_LS_BIT
#define FAP20M_SCH_INDIRECT_MASK(role)                      \
        (FAP20M_SCH_MODULE_NUM(role) << SAND_MODULE_SHIFT)
#define FAP20M_SCH_LINKA_CAL_0_IND_ADDR(role)  (FAP20M_SCH_INDIRECT_MASK(role) | 0x40000000)

/*
 *  Scheduler Credit Generation Calendar LINKA calendar 1 (CAL) - 2K entries
 */
#define FAP20M_SCH_LINKA_CAL_1_IND_ADDR_(role) \
  (role==FAP20M_SCH_RANK_PRIM? 0x40001000 : 0x40000100)
#define FAP20M_SCH_LINKA_CAL_1_IND_ADDR(role)  (FAP20M_SCH_INDIRECT_MASK(role) | FAP20M_SCH_LINKA_CAL_1_IND_ADDR_(role))
#define FAP20M_SCH_LINKA_CAL_1_IND_SIZE(role)  (role==FAP20M_SCH_RANK_PRIM? 2048 : 512)
/*
 *  Scheduler Credit Generation Calendar LINKB calendar 0 (CAL) - 2K entries
 */
#define FAP20M_SCH_LINKB_CAL_0_IND_ADDR(role)  (FAP20M_SCH_INDIRECT_MASK(role) | 0x40002000)
#define FAP20M_SCH_LINKB_CAL_0_IND_SIZE(role)  (role==FAP20M_SCH_RANK_PRIM? 2048 : 512)
/*
 *  Device Rate Memory (DRM) - 200 entries
 */
#define FAP20M_SCH_DRM_IND_ADDR(role)  (FAP20M_SCH_INDIRECT_MASK(role) | 0x40010000)
#define FAP20M_SCH_DRM_IND_SIZE  (200)
/*
 *  Scheduler Flow-FIP Queue Lookup Table (FFQ) - 16K entries
 */
#define FAP20M_SCH_FFQ_IND_ADDR(role)  (FAP20M_SCH_INDIRECT_MASK(role) | 0x40020000)
#define FAP20M_SCH_FFQ_IND_SIZE(role)  (role==FAP20M_SCH_RANK_PRIM? 16384 : 2048)
/*
 *  Flow Descriptor Memory (FDM) - 16K entries
 */
#define FAP20M_SCH_FDM_IND_ADDR(role) (FAP20M_SCH_INDIRECT_MASK(role) | 0x40030000)
#define FAP20M_SCH_FDM_IND_SIZE(role) (role==FAP20M_SCH_RANK_PRIM? 16384 : 2048)
/*
 *  Shaper Descriptor Memory (SHD) - 16K entries
 */
#define FAP20M_SCH_SHD_IND_ADDR(role) (FAP20M_SCH_INDIRECT_MASK(role) | 0x40040000)
#define FAP20M_SCH_SHD_IND_SIZE  (16384)
/*
 *  Scheduler Enable Memory (SEM) - 160 + 256 entries.
 *  internal 256 entries are used during FAP20 init
 */
#define FAP20M_SCH_SEM_IND_ADDR(role)  (FAP20M_SCH_INDIRECT_MASK(role) | 0x40050000)
#define FAP20M_SCH_SEM_IND_SIZE(role)  (role==FAP20M_SCH_RANK_PRIM? (160 + 256) : 40)
/*
 *  Flow Sub Flow Mapping Memory (FSF) - 512 entries
 */
#define FAP20M_SCH_FSF_IND_ADDR(role)  (FAP20M_SCH_INDIRECT_MASK(role) | 0x40060000)
#define FAP20M_SCH_FSF_IND_SIZE(role)  (role==FAP20M_SCH_RANK_PRIM? 512 : 64)
/*
 *  Flow Group Memory (FGM) - 3328 entries
 */
#define FAP20M_SCH_FGM_IND_ADDR(role)  (FAP20M_SCH_INDIRECT_MASK(role) | 0x40070000)
#define FAP20M_SCH_FGM_IND_SIZE(role)  (role==FAP20M_SCH_RANK_PRIM? 1280 : 320)
/*
 *  HR schedulers configuration (SHC) - 128 entries
 */
#define FAP20M_SCH_SHC_IND_ADDR(role)  (FAP20M_SCH_INDIRECT_MASK(role) | 0x40080000)
#define FAP20M_SCH_SHC_IND_SIZE(role)  (role==FAP20M_SCH_RANK_PRIM? 128 : 32)
/*
 *  CL schedulers configuration (SCC) - 1K entries
 */
#define FAP20M_SCH_SCC_IND_ADDR(role)  (FAP20M_SCH_INDIRECT_MASK(role) | 0x40090000)
#define FAP20M_SCH_SCC_IND_SIZE(role)  (role==FAP20M_SCH_RANK_PRIM? 1024 : 256)
/*
 *  CL-schedulers type (SCT) - 32 entries.
 *  Word Size: 96!
 */
#define FAP20M_SCH_SCT_IND_ADDR(role)  (FAP20M_SCH_INDIRECT_MASK(role) | 0x400a0000)
#define FAP20M_SCH_SCT_IND_SIZE  (32)
/*
 *  Scheduler Init
 */
#define FAP20M_SCH_INIT_TRIG_ADDR(role)  (FAP20M_SCH_INDIRECT_MASK(role) | 0x41000000)
/*
 *  Force Status Message.
 *  This Memory is Write Only.
 */
#define FAP20M_SCH_FORC_STAT_MSG_ADDR(role)  (FAP20M_SCH_INDIRECT_MASK(role) | 0x42000000)
/*
 *  Sub-Flow On Table - 8K entries
 */
#define FAP20M_SCH_FIM_IND_ADDR(role)  (FAP20M_SCH_INDIRECT_MASK(role) | 0x40110000)
#define FAP20M_SCH_FIM_IND_SIZE(role)  (role==FAP20M_SCH_RANK_PRIM? 8192 : 1024)
/*
 *  Sub-Flow On Table - 8K entries
 */
#define FAP20M_SCH_FSM_IND_ADDR(role)  (FAP20M_SCH_INDIRECT_MASK(role) | 0x40120000)
#define FAP20M_SCH_FSM_IND_SIZE(role)  (role==FAP20M_SCH_RANK_PRIM? 8192 : 1024)
/*
 *  DHD - 256 entries.
 *  Word Size: 96!
 */
#define FAP20M_SCH_DHD_IND_ADDR(role)  (FAP20M_SCH_INDIRECT_MASK(role) | 0x40160000)
#define FAP20M_SCH_DHD_IND_SIZE  (256)

#define FAP20M_QDP_INDIRECT_MASK (FAP20M_QDP_MODULE_NUM << SAND_MODULE_SHIFT)
/*
 *  Queue Type Table - 0x3000 entries (0x00000000-0x00002fff)
 */
#define FAP20M_QDP_QUT_IND_ADDR  (FAP20M_QDP_INDIRECT_MASK | 0x00000000)
#define FAP20M_QDP_QUT_IND_SIZE  (0x3000)
/*
 *  Queue Type Parameter Table - 16 entries
 */
#define FAP20M_QDP_QTP_IND_ADDR  (FAP20M_QDP_INDIRECT_MASK | 0x00100000)
#define FAP20M_QDP_QTP_IND_SIZE  (16)

/*
 *  Queue WRED Parameter Table - 64 entries (0x00200000-0x002000ff)
 *  68 bits per queue type and drop precedence. Overall 64 entries of WRED parameters.
 *  The 2 LSB bits of the address determine to which part of the 68bits to access:
 *  00 - [31:0], 01 - [63:32], 10 - [67:64] 11 - not valid. The rest of the address determine the entry.
 */
#define FAP20M_QDP_QWP_IND_ADDR  (FAP20M_QDP_INDIRECT_MASK | 0x00200000)
#define FAP20M_QDP_QWP_IND_SIZE  (0x100)
/*
 *  Queue size Parameter Table - 0x3000 entries (0x300000-0x302FFF)
 */
#define FAP20M_QDP_QUEUE_SIZE_IND_ADDR  (FAP20M_QDP_INDIRECT_MASK | 0x00300000)
#define FAP20M_QDP_QUEUE_SIZE_IND_SIZE  (0x3000)
/*
 *  Queue-Flow Lookup Table - 0x3000 entries
 */
#define FAP20M_QDP_QFL_IND_ADDR  (FAP20M_QDP_INDIRECT_MASK | 0x00800000)
/*
 *  Queue Back-off Table - 0x180 entries
 */
#define FAP20M_QDP_QBK_IND_ADDR  (FAP20M_QDP_INDIRECT_MASK | 0x00D00000)
#define FAP20M_QDP_QBK_IND_SIZE  (0x180)
/*
 *  Unicast Distribution Table - 2048 == 2K entries
 */
#define FAP20M_RTP_UDS_IND_ADDR  (FAP20M_RTP_INDIRECT_MASK | 0x00000000)
#define FAP20M_RTP_UDS_IND_SIZE  (2048)
/*
 *  (Spatial) Multicast Distribution Table -  2048 == 2K entries
 */
#define FAP20M_RTP_SMD_IND_ADDR  (FAP20M_RTP_INDIRECT_MASK | 0x00002000)
#define FAP20M_RTP_SMD_IND_SIZE  (2048)
/*
 *  Scheduler Credit Generation Calendar LINKB calendar 1 (CAL) - 2K entries
 */
#define FAP20M_SCH_LINKB_CAL_1_IND_ADDR(role)  (FAP20M_SCH_INDIRECT_MASK(role) | 0x40003000)
#define FAP20M_SCH_LINKB_CAL_1_IND_SIZE(role)  (role==FAP20M_SCH_RANK_PRIM? 2048 : 512)

#define FAP20M_FDT_MODULE_NUM (6)
#define FAP20M_QDP_MODULE_NUM (4)
#define FAP20M_RTP_MODULE_NUM (5)
#define FAP20M_RTP_INDIRECT_MASK (FAP20M_RTP_MODULE_NUM << SAND_MODULE_SHIFT)


/*
 * Typedef: struct CHIP_SIM_INDIRECT_BLOCK
 *
 * Description:
 *      Describe the entry in the indirect blocks table.
 *
 * Fields:
 *      read_result_address         : Address to direct mem where to write the read results
 *      access_trig_offset          : The address in direct memory , that write to it (bit 0) is trigger to 'indirect action'
 *      access_address_offset       : Address of reg that bit 31 is the read/write indication, other bits(0..30) are line index
 *      write_val_offset            : Address of first reg of data (that is used for indirect write action)
 *                                   (like read_result_address but for write not read)
 *      start_address               : index of start entry (in table)
 *      end_address                 : index of last entry (in table)
 *      nof_longs_to_move           : number of registers for entry (number of registers for write , and for read)
 *      base                        : Pointer to dynamic alloc memory represent the indirect table
 *
 *
 * Comments:
 *      WAS COPIED 'AS IS' FROM THE DUNE DRIVER.
 */


typedef struct
{
  GT_U32 read_result_address;
  GT_U32 access_trig_offset;
  GT_U32 access_address_offset;
  GT_U32 write_val_offset;

  GT_U32 start_address;
  GT_U32 end_address;

  int nof_longs_to_move;

  /*
   * The actual address
   */
  GT_U32* base ;
} CHIP_SIM_INDIRECT_BLOCK;

#define GET_UNIT_ADDR(unitIndex)    \
    unitIndex ==  SUB_UNIT_INDEX(globReg     ) ?  0x0000 * 4 :        \
    unitIndex ==  SUB_UNIT_INDEX(schReg      ) ?  0x0700 * 4 :        \
    unitIndex ==  SUB_UNIT_INDEX(drcReg0     ) ?  0x0A00 * 4 :        \
    unitIndex ==  SUB_UNIT_INDEX(drcReg1     ) ?  0x0B00 * 4 :        \
    unitIndex ==  SUB_UNIT_INDEX(drcReg2     ) ?  0x0C00 * 4 :        \
    unitIndex ==  SUB_UNIT_INDEX(drcReg3     ) ?  0x0D00 * 4 :        \
    unitIndex ==  SUB_UNIT_INDEX(fctReg      ) ?  0x1820 * 4 :        \
    unitIndex ==  SUB_UNIT_INDEX(smsReg      ) ?  0x1911 * 4 :        \
    unitIndex ==  SUB_UNIT_INDEX(fcrReg      ) ?  0x1b27 * 4 :        \
    unitIndex ==  SUB_UNIT_INDEX(qdpReg      ) ?  0x2000 * 4 :        \
    unitIndex ==  SUB_UNIT_INDEX(rtpReg      ) ?  0x2200 * 4 :        \
    unitIndex ==  SUB_UNIT_INDEX(fdtReg      ) ?  0x2410 * 4 :        \
    unitIndex ==  SUB_UNIT_INDEX(fdrReg      ) ?  0x2600 * 4 :        \
    unitIndex ==  SUB_UNIT_INDEX(schPrimReg  ) ?  0x2700 * 4 :        \
    unitIndex ==  SUB_UNIT_INDEX(mmuReg      ) ?  0x2900 * 4 :        \
    unitIndex ==  SUB_UNIT_INDEX(macSerdesReg) ?  0x2a00 * 4 :        \
    unitIndex ==  SUB_UNIT_INDEX(cfgReg      ) ?  0x2f00 * 4 :        \
        0xFFFFFFFF

/*get address of register from the 'original' dune core code */
#define  GET_SIM_ADDR_MAC(unitName,registerOffsetInStc)             \
    ((SAND_OFFSETOF(SMEM_FAP20M_DEV_MEM_INFO,registerOffsetInStc)) - \
     (SAND_OFFSETOF(SMEM_FAP20M_DEV_MEM_INFO,unitName)) + \
     (GET_UNIT_ADDR(SUB_UNIT_INDEX(unitName))))


/* WAS COPIED 'AS IS' FROM THE DUNE DRIVER */
static
CHIP_SIM_INDIRECT_BLOCK
  Fap20m_indirect_blocks[]=
{
  /* FDT */
    {
      GET_SIM_ADDR_MAC(fdtReg,fdtReg.indirect.read_data),
      GET_SIM_ADDR_MAC(fdtReg,fdtReg.indirect.op_trigger),
      GET_SIM_ADDR_MAC(fdtReg,fdtReg.indirect.address),
      GET_SIM_ADDR_MAC(fdtReg,fdtReg.indirect.write_data),
      ~FAP20M_FDT_INDIRECT_MASK & FAP20M_FDT_DU2_IND_ADDR,
      (~FAP20M_FDT_INDIRECT_MASK & FAP20M_FDT_DU2_IND_ADDR) + 64 -1,
      1, /* the LBP is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(fdtReg,fdtReg.indirect.read_data),
      GET_SIM_ADDR_MAC(fdtReg,fdtReg.indirect.op_trigger),
      GET_SIM_ADDR_MAC(fdtReg,fdtReg.indirect.address),
      GET_SIM_ADDR_MAC(fdtReg,fdtReg.indirect.write_data),
      ~FAP20M_FDT_INDIRECT_MASK & FAP20M_FDT_DM2_IND_ADDR,
      (~FAP20M_FDT_INDIRECT_MASK & FAP20M_FDT_DM2_IND_ADDR) + 64 -1,
      1, /* the LBP is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(fdtReg,fdtReg.indirect.read_data),
      GET_SIM_ADDR_MAC(fdtReg,fdtReg.indirect.op_trigger),
      GET_SIM_ADDR_MAC(fdtReg,fdtReg.indirect.address),
      GET_SIM_ADDR_MAC(fdtReg,fdtReg.indirect.write_data),
      ~FAP20M_FDT_INDIRECT_MASK & FAP20M_FDT_DU4_IND_ADDR,
      (~FAP20M_FDT_INDIRECT_MASK & FAP20M_FDT_DU4_IND_ADDR) + 64 -1,
      1, /* the LBP is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(fdtReg,fdtReg.indirect.read_data),
      GET_SIM_ADDR_MAC(fdtReg,fdtReg.indirect.op_trigger),
      GET_SIM_ADDR_MAC(fdtReg,fdtReg.indirect.address),
      GET_SIM_ADDR_MAC(fdtReg,fdtReg.indirect.write_data),
      ~FAP20M_FDT_INDIRECT_MASK & FAP20M_FDT_DM4_IND_ADDR,
      (~FAP20M_FDT_INDIRECT_MASK & FAP20M_FDT_DM4_IND_ADDR) + 64 -1,
      1, /* the LBP is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
  /* FDT } */

  /* SCH PRIM{ */
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[2]),
      FAP20M_SCH_LINKA_CAL_0_IND_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_LINKA_CAL_0_IND_ADDR(FAP20M_SCH_RANK_PRIM) + 2*1024 -1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[2]),
      FAP20M_SCH_LINKA_CAL_1_IND_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_LINKA_CAL_1_IND_ADDR(FAP20M_SCH_RANK_PRIM) + 2*1024 -1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[2]),
      FAP20M_SCH_LINKB_CAL_0_IND_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_LINKB_CAL_0_IND_ADDR(FAP20M_SCH_RANK_PRIM) + 2*1024 -1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[2]),
      FAP20M_SCH_LINKB_CAL_1_IND_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_LINKB_CAL_1_IND_ADDR(FAP20M_SCH_RANK_PRIM) + 2*1024 -1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[2]),

      FAP20M_SCH_DRM_IND_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_DRM_IND_ADDR(FAP20M_SCH_RANK_PRIM) + 200 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[2]),

      FAP20M_SCH_FFQ_IND_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_FFQ_IND_ADDR(FAP20M_SCH_RANK_PRIM) + 16*1024 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[2]),

      FAP20M_SCH_FDM_IND_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_FDM_IND_ADDR(FAP20M_SCH_RANK_PRIM) + 16*1024 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[2]),

      FAP20M_SCH_SHD_IND_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_SHD_IND_ADDR(FAP20M_SCH_RANK_PRIM) + 16*1024 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[2]),

      FAP20M_SCH_SEM_IND_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_SEM_IND_ADDR(FAP20M_SCH_RANK_PRIM) + (160 + 256 /* internal 256
                                     entries are used during FAP20 init*/) - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[2]),

      FAP20M_SCH_FSF_IND_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_FSF_IND_ADDR(FAP20M_SCH_RANK_PRIM) + 512 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[2]),

      FAP20M_SCH_FGM_IND_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_FGM_IND_ADDR(FAP20M_SCH_RANK_PRIM) + 1280 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[2]),

      FAP20M_SCH_SHC_IND_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_SHC_IND_ADDR(FAP20M_SCH_RANK_PRIM) + 128 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[2]),

      FAP20M_SCH_SCC_IND_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_SCC_IND_ADDR(FAP20M_SCH_RANK_PRIM) + 1024 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[0]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[0]),

      FAP20M_SCH_SCT_IND_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_SCT_IND_ADDR(FAP20M_SCH_RANK_PRIM) + 32 - 1,
      3, /* the SCH is moving 3 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[2]),
      FAP20M_SCH_INIT_TRIG_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_INIT_TRIG_ADDR(FAP20M_SCH_RANK_PRIM) + 1 -1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[2]),

      FAP20M_SCH_FIM_IND_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_FIM_IND_ADDR(FAP20M_SCH_RANK_PRIM) + 8*1024 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[2]),

      FAP20M_SCH_FSM_IND_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_FSM_IND_ADDR(FAP20M_SCH_RANK_PRIM) + 8*1024 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[0]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[0]),
      FAP20M_SCH_DHD_IND_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_DHD_IND_ADDR(FAP20M_SCH_RANK_PRIM) + FAP20M_SCH_DHD_IND_SIZE -1,
      3, /* the SCH is moving 3 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_address),
      GET_SIM_ADDR_MAC(schPrimReg,schPrimReg.indirect_write_data[2]),
      FAP20M_SCH_FORC_STAT_MSG_ADDR(FAP20M_SCH_RANK_PRIM),
      FAP20M_SCH_FORC_STAT_MSG_ADDR(FAP20M_SCH_RANK_PRIM) + 1 -1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
  /* SCH PRIM} */


  /* SCH SCND{ */
    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[2]),
      FAP20M_SCH_LINKA_CAL_0_IND_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_LINKA_CAL_0_IND_ADDR(FAP20M_SCH_RANK_SCND) + 512 -1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[2]),
      FAP20M_SCH_LINKA_CAL_1_IND_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_LINKA_CAL_1_IND_ADDR(FAP20M_SCH_RANK_SCND) + 512 -1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[2]),
      FAP20M_SCH_LINKB_CAL_0_IND_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_LINKB_CAL_0_IND_ADDR(FAP20M_SCH_RANK_SCND) + 512 -1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[2]),
      FAP20M_SCH_LINKB_CAL_1_IND_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_LINKB_CAL_1_IND_ADDR(FAP20M_SCH_RANK_SCND) + 512 -1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },

    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[2]),

      FAP20M_SCH_DRM_IND_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_DRM_IND_ADDR(FAP20M_SCH_RANK_SCND) + 200 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[2]),

      FAP20M_SCH_FFQ_IND_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_FFQ_IND_ADDR(FAP20M_SCH_RANK_SCND) + 2*1024 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[2]),

      FAP20M_SCH_FDM_IND_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_FDM_IND_ADDR(FAP20M_SCH_RANK_SCND) + 2*1024 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[2]),

      FAP20M_SCH_SHD_IND_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_SHD_IND_ADDR(FAP20M_SCH_RANK_SCND) + 16*1024 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[2]),

      FAP20M_SCH_SEM_IND_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_SEM_IND_ADDR(FAP20M_SCH_RANK_SCND) + 40 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[2]),

      FAP20M_SCH_FSF_IND_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_FSF_IND_ADDR(FAP20M_SCH_RANK_SCND) + 64 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[2]),
      FAP20M_SCH_FGM_IND_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_FGM_IND_ADDR(FAP20M_SCH_RANK_SCND) + 320 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[2]),

      FAP20M_SCH_SHC_IND_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_SHC_IND_ADDR(FAP20M_SCH_RANK_SCND) + 32 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[2]),
      FAP20M_SCH_SCC_IND_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_SCC_IND_ADDR(FAP20M_SCH_RANK_SCND) + 256 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[0]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[0]),

      FAP20M_SCH_SCT_IND_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_SCT_IND_ADDR(FAP20M_SCH_RANK_SCND) + 32 - 1,
      3, /* the SCH is moving 3 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[2]),
      FAP20M_SCH_INIT_TRIG_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_INIT_TRIG_ADDR(FAP20M_SCH_RANK_SCND) + 1 -1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[2]),

      FAP20M_SCH_FIM_IND_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_FIM_IND_ADDR(FAP20M_SCH_RANK_SCND) + 1024 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[2]),

      FAP20M_SCH_FSM_IND_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_FSM_IND_ADDR(FAP20M_SCH_RANK_SCND) + 1024 - 1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[0]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[0]),
      FAP20M_SCH_DHD_IND_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_DHD_IND_ADDR(FAP20M_SCH_RANK_SCND) + FAP20M_SCH_DHD_IND_SIZE -1,
      3, /* the SCH is moving 3 32 bit word*/
      NULL /* bases are NULL*/
    },
    {
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_read_data[2]),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_op_trigger),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_address),
      GET_SIM_ADDR_MAC(schReg,schReg.indirect_write_data[2]),
      FAP20M_SCH_FORC_STAT_MSG_ADDR(FAP20M_SCH_RANK_SCND),
      FAP20M_SCH_FORC_STAT_MSG_ADDR(FAP20M_SCH_RANK_SCND) + 1 -1,
      1, /* the SCH is moving only one 32 bit word*/
      NULL /* bases are NULL*/
    },
  /* SCH SCND} */


  /* QDP { */
  {
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.read_data),
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.op_trigger),
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.address),
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.write_data),
    ~FAP20M_QDP_INDIRECT_MASK & (FAP20M_QDP_QUT_IND_ADDR),
    (~FAP20M_QDP_INDIRECT_MASK & FAP20M_QDP_QUT_IND_ADDR) + 0x3000 -1,
    1, /* the QDP is moving only one 32 bit word*/
    NULL /* bases are NULL*/
  },
  {
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.read_data),
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.op_trigger),
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.address),
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.write_data),
    ~FAP20M_QDP_INDIRECT_MASK & (FAP20M_QDP_QTP_IND_ADDR),
    (~FAP20M_QDP_INDIRECT_MASK & FAP20M_QDP_QTP_IND_ADDR) + 16 -1,
    1, /* the QDP is moving only one 32 bit word*/
    NULL /* bases are NULL*/
  },
  {
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.read_data),
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.op_trigger),
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.address),
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.write_data),
    ~FAP20M_QDP_INDIRECT_MASK & (FAP20M_QDP_QWP_IND_ADDR),
    (~FAP20M_QDP_INDIRECT_MASK & FAP20M_QDP_QWP_IND_ADDR) + FAP20M_QDP_QWP_IND_SIZE -1,
    1, /* the QDP is moving only one 32 bit word*/
    NULL /* bases are NULL*/
  },
  {
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.read_data),
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.op_trigger),
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.address),
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.write_data),
    ~FAP20M_QDP_INDIRECT_MASK & (FAP20M_QDP_QUEUE_SIZE_IND_ADDR),
    (~FAP20M_QDP_INDIRECT_MASK & FAP20M_QDP_QUEUE_SIZE_IND_ADDR) + FAP20M_QDP_QUEUE_SIZE_IND_SIZE -1,
    1, /* the QDP is moving only one 32 bit word*/
    NULL /* bases are NULL*/
  },
  {
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.read_data),
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.op_trigger),
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.address),
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.write_data),
    ~FAP20M_QDP_INDIRECT_MASK & (FAP20M_QDP_QFL_IND_ADDR),
    (~FAP20M_QDP_INDIRECT_MASK & FAP20M_QDP_QFL_IND_ADDR) + 12*1024 -1,
    1, /* the QDP is moving only one 32 bit word*/
    NULL /* bases are NULL*/
  },
  {
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.read_data),
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.op_trigger),
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.address),
    GET_SIM_ADDR_MAC(qdpReg,qdpReg.indirect.write_data),
    ~FAP20M_QDP_INDIRECT_MASK & (FAP20M_QDP_QBK_IND_ADDR),
    (~FAP20M_QDP_INDIRECT_MASK & FAP20M_QDP_QBK_IND_ADDR) + FAP20M_QDP_QBK_IND_SIZE -1,
    1, /* the QDP is moving only one 32 bit word*/
    NULL /* bases are NULL*/
  },
  /* QDP } */

  /* RTP { */
  {
    GET_SIM_ADDR_MAC(rtpReg,rtpReg.indirect.read_data),
    GET_SIM_ADDR_MAC(rtpReg,rtpReg.indirect.op_trigger),
    GET_SIM_ADDR_MAC(rtpReg,rtpReg.indirect.address),
    GET_SIM_ADDR_MAC(rtpReg,rtpReg.indirect.write_data),
    ~FAP20M_RTP_INDIRECT_MASK & (FAP20M_RTP_UDS_IND_ADDR),
    (~FAP20M_RTP_INDIRECT_MASK & FAP20M_RTP_UDS_IND_ADDR) + 2*1024 -1,
    1, /* the RTP is moving only one 32 bit word*/
    NULL /* bases are NULL*/
  },
  {
    GET_SIM_ADDR_MAC(rtpReg,rtpReg.indirect.read_data),
    GET_SIM_ADDR_MAC(rtpReg,rtpReg.indirect.op_trigger),
    GET_SIM_ADDR_MAC(rtpReg,rtpReg.indirect.address),
    GET_SIM_ADDR_MAC(rtpReg,rtpReg.indirect.write_data),
    ~FAP20M_RTP_INDIRECT_MASK & (FAP20M_RTP_SMD_IND_ADDR),
    (~FAP20M_RTP_INDIRECT_MASK & FAP20M_RTP_SMD_IND_ADDR) + 2*1024 -1,
    1, /* the RTP is moving only one 32 bit word*/
    NULL /* bases are NULL*/
  },
  /* RTP } */


  /* MMU { */
  {
    GET_SIM_ADDR_MAC(mmuReg,mmuReg.indirect_read_data[0]),
    GET_SIM_ADDR_MAC(mmuReg,mmuReg.indirect_op_trigger),
    GET_SIM_ADDR_MAC(mmuReg,mmuReg.indirect_address),
    GET_SIM_ADDR_MAC(mmuReg,mmuReg.indirect_write_data[0]),
    0,
    0 + 16 * 1024 -1,
    8, /* the QDP is moving only one 256 bit word*/
    NULL /* bases are NULL*/
  },
  /* MMU } */

  /*
   * last block .. do not remove
   */
  {
    INVALID_ADDRESS,
  }
};


/* Active memory table -- will be dynamically allocated */
static SMEM_ACTIVE_MEM_ENTRY_STC *smemFAP20MActiveTable = NULL;

void smemFAP20MIndirectBlocksInit(void);

void smemFAP20MActiveMemTableInit(void);

void smemFAP20MRegsInfoDump
(
    IN SKERNEL_DEVICE_OBJECT *  devObjPtr
);

void smemFAP20MDirectMemCheck
(
    IN SKERNEL_DEVICE_OBJECT *  devObjPtr
);

void smemFAP20MActiveMemReadFunc
(
      IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
      IN         GT_U32   address,
      IN         GT_U32   memSize,
      IN         GT_U32 * memPtr,
      IN         GT_UINTPTR   param,     /* index of the entry in the indirect blocks table */
      OUT        GT_U32 * outMemPtr
);

void smemFAP20MActiveMemWriteFunc
(
      IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
      IN         GT_U32   address,
      IN         GT_U32   memSize,
      IN         GT_U32 * memPtr,
      IN         GT_UINTPTR   param,        /* index of the entry in the indirect blocks table */
      IN         GT_U32 * inMemPtr
);

/*******************************************************************************
* smemFAP20MInit
*
* DESCRIPTION:
*       Init memory module for the FAP20M device.
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
void smemFAP20MInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_FAP20M_DEV_MEM_INFO  * devMemInfoPtr;
    SMEM_FAP20M_CHUNK_INFO_STC  *currChunkInfoPtr;
    GT_U32 startAddress;

    devMemInfoPtr = (SMEM_FAP20M_DEV_MEM_INFO *)calloc(1, sizeof(SMEM_FAP20M_DEV_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
        if (skernelUserDebugInfo.disableFatalError == GT_FALSE)
        {
            skernelFatalError("smemFAP20MInit: allocation error\n");
        }
    }

    devObjPtr->deviceMemory = devMemInfoPtr;

    currChunkInfoPtr = &devMemInfoPtr->chunkInfoArray[0];

    startAddress = GET_UNIT_ADDR(SUB_UNIT_INDEX(globReg));
    SIM_CHUNK_INFO_SET_MAC(currChunkInfoPtr , startAddress, devMemInfoPtr->globReg);
    currChunkInfoPtr++;
    startAddress = GET_UNIT_ADDR(SUB_UNIT_INDEX(schReg));
    SIM_CHUNK_INFO_SET_MAC(currChunkInfoPtr , startAddress, devMemInfoPtr->schReg);
    currChunkInfoPtr++;
    startAddress = GET_UNIT_ADDR(SUB_UNIT_INDEX(drcReg0));
    SIM_CHUNK_INFO_SET_MAC(currChunkInfoPtr , startAddress, devMemInfoPtr->drcReg[0]);
    currChunkInfoPtr++;
    startAddress = GET_UNIT_ADDR(SUB_UNIT_INDEX(drcReg1));
    SIM_CHUNK_INFO_SET_MAC(currChunkInfoPtr , startAddress, devMemInfoPtr->drcReg[1]);
    currChunkInfoPtr++;
    startAddress = GET_UNIT_ADDR(SUB_UNIT_INDEX(drcReg2));
    SIM_CHUNK_INFO_SET_MAC(currChunkInfoPtr , startAddress, devMemInfoPtr->drcReg[2]);
    currChunkInfoPtr++;
    startAddress = GET_UNIT_ADDR(SUB_UNIT_INDEX(drcReg3));
    SIM_CHUNK_INFO_SET_MAC(currChunkInfoPtr , startAddress, devMemInfoPtr->drcReg[3]);
    currChunkInfoPtr++;
    startAddress = GET_UNIT_ADDR(SUB_UNIT_INDEX(fctReg));
    SIM_CHUNK_INFO_SET_MAC(currChunkInfoPtr , startAddress, devMemInfoPtr->fctReg);
    currChunkInfoPtr++;
    startAddress = GET_UNIT_ADDR(SUB_UNIT_INDEX(smsReg));
    SIM_CHUNK_INFO_SET_MAC(currChunkInfoPtr , startAddress, devMemInfoPtr->smsReg);
    currChunkInfoPtr++;
    startAddress = GET_UNIT_ADDR(SUB_UNIT_INDEX(fcrReg));
    SIM_CHUNK_INFO_SET_MAC(currChunkInfoPtr , startAddress, devMemInfoPtr->fcrReg);
    currChunkInfoPtr++;
    startAddress = GET_UNIT_ADDR(SUB_UNIT_INDEX(qdpReg));
    SIM_CHUNK_INFO_SET_MAC(currChunkInfoPtr , startAddress, devMemInfoPtr->qdpReg);
    currChunkInfoPtr++;
    startAddress = GET_UNIT_ADDR(SUB_UNIT_INDEX(rtpReg));
    SIM_CHUNK_INFO_SET_MAC(currChunkInfoPtr , startAddress, devMemInfoPtr->rtpReg);
    currChunkInfoPtr++;
    startAddress = GET_UNIT_ADDR(SUB_UNIT_INDEX(fdtReg));
    SIM_CHUNK_INFO_SET_MAC(currChunkInfoPtr , startAddress, devMemInfoPtr->fdtReg);
    currChunkInfoPtr++;
    startAddress = GET_UNIT_ADDR(SUB_UNIT_INDEX(fdrReg));
    SIM_CHUNK_INFO_SET_MAC(currChunkInfoPtr , startAddress, devMemInfoPtr->fdrReg);
    currChunkInfoPtr++;
    startAddress = GET_UNIT_ADDR(SUB_UNIT_INDEX(schPrimReg));
    SIM_CHUNK_INFO_SET_MAC(currChunkInfoPtr , startAddress, devMemInfoPtr->schPrimReg);
    currChunkInfoPtr++;
    startAddress = GET_UNIT_ADDR(SUB_UNIT_INDEX(mmuReg));
    SIM_CHUNK_INFO_SET_MAC(currChunkInfoPtr , startAddress, devMemInfoPtr->mmuReg);
    currChunkInfoPtr++;
    startAddress = GET_UNIT_ADDR(SUB_UNIT_INDEX(macSerdesReg));
    SIM_CHUNK_INFO_SET_MAC(currChunkInfoPtr , startAddress, devMemInfoPtr->macSerdesReg);
    currChunkInfoPtr++;
    startAddress = GET_UNIT_ADDR(SUB_UNIT_INDEX(cfgReg));
    SIM_CHUNK_INFO_SET_MAC(currChunkInfoPtr , startAddress, devMemInfoPtr->cfgReg);
    currChunkInfoPtr++;

    /* bind the find memory function */
    devObjPtr->devFindMemFunPtr = (void *)smemFAP20MReg;
    devObjPtr->deviceMemory = devMemInfoPtr;

    /* initialize Fap20m_indirect_blocks table */
    smemFAP20MIndirectBlocksInit();

    /* initialize active memory table (smemFAP20MActiveTable)*/
    smemFAP20MActiveMemTableInit();

    /* Debug funcs */
/*    smemFAP20MRegsInfoDump(devObjPtr);
    smemFAP20MDirectMemCheck(devObjPtr);
*/
    devObjPtr->activeMemPtr = smemFAP20MActiveTable;
}

/*******************************************************************************
*  smemFAP20MIndirectBlocksInit
*
* DESCRIPTION:
*      Initialization and allocation of Fap20m_indirect_blocks table
* INPUTS:
*       NONE.
*
* OUTPUTS:
*       NONE.
*
* RETURNS:
*       NONE.
*
* COMMENTS:
*
*******************************************************************************/
void smemFAP20MIndirectBlocksInit(void)
{
    GT_U32      block_size_in_longs =0;
    CHIP_SIM_INDIRECT_BLOCK* blockPtr;

    for (blockPtr=Fap20m_indirect_blocks; blockPtr->read_result_address!=INVALID_ADDRESS; blockPtr++)
    {
        block_size_in_longs = (blockPtr->end_address - blockPtr->start_address) + 1;
        /* multiple number of registers in entry by number of entries by size of single register */
        blockPtr->base = calloc(block_size_in_longs ,sizeof(GT_U32)*blockPtr->nof_longs_to_move);

        if(blockPtr->base == NULL)
        {
            if (skernelUserDebugInfo.disableFatalError == GT_FALSE)
            {
                skernelFatalError("smemFAP20MIndirectBlocksInit: out of memory for alloc");
            }
        }
    }
}

/*******************************************************************************
*  smemFAP20MActiveMemTableInit
*
* DESCRIPTION:
*      Allocation and initialization of smemFAP20MActiveTable from Dune table Fap20m_indirect_blocks.
* INPUTS:
*       NONE.
*
* OUTPUTS:
*       NONE.
*
* RETURNS:
*       NONE.
*
* COMMENTS:
*
*******************************************************************************/
void smemFAP20MActiveMemTableInit(void)
{
    CHIP_SIM_INDIRECT_BLOCK*  blockPtr;
    GT_U32                    tempOpTrigger = INVALID_ADDRESS;
    GT_U32                    entryCounter = 0;   /* counter for active table entries*/
    GT_U32                    i = 0;
    GT_U32                    index = 0;          /* index in the indirect blocks table */


    /* count number of entries needed to allocate for the active memory table */
    for (blockPtr=Fap20m_indirect_blocks; blockPtr->read_result_address!=INVALID_ADDRESS; blockPtr++)
    {
        if(tempOpTrigger != blockPtr->access_trig_offset)
        {
            /* new block reached - increase the counter */
            entryCounter++;
            tempOpTrigger = blockPtr->access_trig_offset;
        }
    }

    /*add one more for the end table entry*/
    entryCounter++;

    /* allocate active memory table */
    smemFAP20MActiveTable = malloc(entryCounter*sizeof(SMEM_ACTIVE_MEM_ENTRY_STC));
    if(smemFAP20MActiveTable == NULL)
    {
        if (skernelUserDebugInfo.disableFatalError == GT_FALSE)
        {
            skernelFatalError("smemFAP20MActiveMemTableInit: out of memory for alloc\n");
        }
        return;
    }

    tempOpTrigger = INVALID_ADDRESS;

    /* fill the active memory table */
    for (blockPtr=Fap20m_indirect_blocks; blockPtr->read_result_address!=INVALID_ADDRESS; blockPtr++, index++)
    {
        if(tempOpTrigger != blockPtr->access_trig_offset)
        {
            smemFAP20MActiveTable[i].address = blockPtr->access_trig_offset;
            smemFAP20MActiveTable[i].mask = SMEM_FULL_MASK_CNS; /* exact match on trigger register */
            smemFAP20MActiveTable[i].readFun = NULL;
            smemFAP20MActiveTable[i].readFunParam = 0;
            smemFAP20MActiveTable[i].writeFun = (void *)smemFAP20MActiveMemWriteFunc;
            smemFAP20MActiveTable[i].writeFunParam = index;
            tempOpTrigger = blockPtr->access_trig_offset;
            i++;
        }
    }

    /* must be last anyway */
    smemFAP20MActiveTable[i].address = SMEM_FULL_MASK_CNS;
    smemFAP20MActiveTable[i].mask = SMEM_FULL_MASK_CNS; /* exact match on trigger register */
    smemFAP20MActiveTable[i].readFun = NULL;/*(void *)smemFAP20MActiveMemReadFunc;*/
    smemFAP20MActiveTable[i].readFunParam = 0;
    smemFAP20MActiveTable[i].writeFun = NULL;
    smemFAP20MActiveTable[i].writeFunParam = 0;
}

/*******************************************************************************
*  smemFAP20MActiveMemWriteFunc
*
* DESCRIPTION:
*      Definition of the Active register write function.
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemFAP20MActiveMemWriteFunc
(
      IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
      IN         GT_U32   address,
      IN         GT_U32   memSize,
      IN         GT_U32 * memPtr,
      IN         GT_UINTPTR   param,        /* index of the entry in the indirect blocks table */
      IN         GT_U32 * inMemPtr
)
{
    CHIP_SIM_INDIRECT_BLOCK *currIndirectPtr = &Fap20m_indirect_blocks[param];
    GT_U32                  regAddr;            /* register's address */
    GT_U32                  regValue;           /* register value*/
    GT_BOOL                 found=GT_FALSE;
    GT_U32                  indirectOffset=0;   /* offset in the indirect table from start*/
    GT_U32                  rdWr=0;             /* Read =1 or write = 0 operation */
    GT_U32                  *indirectMemPtr;
    GT_U32                  *regPtr;            /* pointer to direct memory */

    /*write the content into the register*/
    *memPtr = *inMemPtr;

    if((*inMemPtr & 1) == 0)
    {
        /* the operation is not activated yet */
        return;
    }

    for(/*already done*/; currIndirectPtr->access_trig_offset == address ; currIndirectPtr++)
    {
        regAddr = currIndirectPtr->access_address_offset;
        smemRegGet(deviceObjPtr,regAddr,&regValue);

        indirectOffset = U32_GET_FIELD(regValue,0,31);
        rdWr =  U32_GET_FIELD(regValue,31,1);

        if((currIndirectPtr->start_address <= indirectOffset) && (currIndirectPtr->end_address >= indirectOffset))
        {
            found = GT_TRUE;
            break;
        }
    }

    if(found == GT_FALSE)
    {
        if (skernelUserDebugInfo.disableFatalError == GT_FALSE)
        {
            skernelFatalError("smemFAP20MActiveMemWriteFunc: indirect access is unknown , index [%d]\n" , param);
        }
    }

    indirectMemPtr = &currIndirectPtr->base[indirectOffset-currIndirectPtr->start_address];

    if(rdWr)
    {
        /*read*/
        regPtr = smemMemGet(deviceObjPtr, (currIndirectPtr->read_result_address));

        memcpy(regPtr,indirectMemPtr,currIndirectPtr->nof_longs_to_move*4);
    }
    else
    {
        /*write*/
        regPtr = smemMemGet(deviceObjPtr, (currIndirectPtr->write_val_offset));

        memcpy(indirectMemPtr,regPtr,currIndirectPtr->nof_longs_to_move*4);
    }

    /*clear the trigger bit*/
    *memPtr &= ~1;

    return ;
}
/*******************************************************************************
*  smemFAP20MReg
*
* DESCRIPTION:
*      Definition of memory address type device specific search function
* INPUTS:
*       deviceObjPtr    - device object PTR.
*       address         - Address for ASIC memory.
        accessType      - Memory access type
*       memSize         - Size of ASIC memory to read or write.
*
* OUTPUTS:
*       activeMemPtrPtr - pointer (to pointer) to the active memory entry or NULL if not
*                         exist.
* RETURNS:
*       pointer to the memory location in the database
*       NULL - if address not exist, or memSize > than existed size
*
* COMMENTS:
*
*******************************************************************************/
GT_U32* smemFAP20MReg
(
    IN SKERNEL_DEVICE_OBJECT *     deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE     accessType,
    IN GT_U32                      address,
    IN GT_U32                      memSize,
    INOUT SMEM_ACTIVE_MEM_ENTRY_STC **  activeMemPtrPtr
)
{
    SMEM_FAP20M_DEV_MEM_INFO        *devMemInfoPtr;
    SMEM_FAP20M_CHUNK_INFO_STC      *currChunkInfoPtr;
    GT_U32                          index=0;
    GT_U32                          ii;

    devMemInfoPtr = (SMEM_FAP20M_DEV_MEM_INFO*)deviceObjPtr->deviceMemory;

    currChunkInfoPtr = &devMemInfoPtr->chunkInfoArray[0];

    /* loop on all chunks to find our memory */
    for(ii = 0 ; ii < NUM_CHUNKS_CNS ; ii++,currChunkInfoPtr++)
    {
        if (address >= currChunkInfoPtr->startAddr && address < currChunkInfoPtr->lastAddr)
        {
            /* we are in the needed chunk */

            /* the memSize requested overflow the boundary of the chunk */
            if(address+(memSize*4) > currChunkInfoPtr->lastAddr)
            {
                if (skernelUserDebugInfo.disableFatalError == GT_FALSE)
                {
                    skernelFatalError("smemFAP20MReg: requested mem size is beyond boundary of chunk\n");
                }
            }

            /*get index as the offset of registers from start of chunk */
            index = (address - currChunkInfoPtr->startAddr) / 0x4;
            break;
        }
    }

    if(ii == NUM_CHUNKS_CNS)
    {
        if (skernelUserDebugInfo.disableFatalError == GT_FALSE)
        {
            skernelFatalError("unknown address [0x%8.8x] accessing \n",address);
        }

        return NULL;
    }

    /* find active memory entry */
    if (activeMemPtrPtr != NULL)
    {
        *activeMemPtrPtr = NULL;
        for (ii = 0; smemFAP20MActiveTable[ii].address != 0xffffffff; ii++)
        {
            /* check address */
            if ((address & smemFAP20MActiveTable[ii].mask)
                 == smemFAP20MActiveTable[ii].address)
            {
                *activeMemPtrPtr = &smemFAP20MActiveTable[ii];
                break;
            }
        }
    }

    /*return the pointer to the needed memory */
    return &currChunkInfoPtr->chunkPtr[index];
}

/*Print regs addresses*/
void smemFAP20MRegsInfoDump
(
    IN SKERNEL_DEVICE_OBJECT *  devObjPtr
)
{
    SMEM_FAP20M_DEV_MEM_INFO          * devMemInfoPtr;
    SMEM_FAP20M_CHUNK_INFO_STC        * currChunkInfoPtr;
    GT_U32                              ii;

    devMemInfoPtr = (SMEM_FAP20M_DEV_MEM_INFO*)devObjPtr->deviceMemory;

    currChunkInfoPtr = &devMemInfoPtr->chunkInfoArray[0];

    printf("start address: \t last address: \t data : \n");

    /* loop on all chunks to print their info */
    for(ii = 0 ; ii < NUM_CHUNKS_CNS ; ii++,currChunkInfoPtr++)
    {
        printf("0x%8.8x\t 0x%8.8x\t %d\n", currChunkInfoPtr->startAddr, currChunkInfoPtr->lastAddr,*currChunkInfoPtr->chunkPtr);
    }
}

/* direct memory check */
void smemFAP20MDirectMemCheck
(
    IN SKERNEL_DEVICE_OBJECT *  devObjPtr
)
{
    SMEM_FAP20M_DEV_MEM_INFO          * devMemInfoPtr;
    SMEM_FAP20M_CHUNK_INFO_STC        * currChunkInfoPtr;
    GT_U32                              i,j;
    GT_U32                            * currChunkPtr;
    GT_U32                              size[NUM_CHUNKS_CNS];

    devMemInfoPtr = (SMEM_FAP20M_DEV_MEM_INFO*)devObjPtr->deviceMemory;
    currChunkInfoPtr = &devMemInfoPtr->chunkInfoArray[0];

    i=0;
    size[i++] = sizeof(devMemInfoPtr->globReg)/4;
    size[i++] = sizeof(devMemInfoPtr->schReg)/4;
    size[i++] = sizeof(devMemInfoPtr->drcReg[0])/4;
    size[i++] = sizeof(devMemInfoPtr->drcReg[1])/4;
    size[i++] = sizeof(devMemInfoPtr->drcReg[2])/4;
    size[i++] = sizeof(devMemInfoPtr->drcReg[3])/4;
    size[i++] = sizeof(devMemInfoPtr->fctReg)/4;
    size[i++] = sizeof(devMemInfoPtr->smsReg)/4;
    size[i++] = sizeof(devMemInfoPtr->fcrReg)/4;
    size[i++] = sizeof(devMemInfoPtr->qdpReg)/4;
    size[i++] = sizeof(devMemInfoPtr->rtpReg)/4;
    size[i++] = sizeof(devMemInfoPtr->fdtReg)/4;
    size[i++] = sizeof(devMemInfoPtr->fdrReg)/4;
    size[i++] = sizeof(devMemInfoPtr->schPrimReg)/4;
    size[i++] = sizeof(devMemInfoPtr->mmuReg)/4;
    size[i++] = sizeof(devMemInfoPtr->macSerdesReg)/4;
    size[i++] = sizeof(devMemInfoPtr->cfgReg)/4;

    for(i = 0; i < NUM_CHUNKS_CNS; i++, currChunkInfoPtr++)
    {
        for(currChunkPtr = (GT_U32*)currChunkInfoPtr->chunkPtr, j=0; j<size[i]; currChunkPtr++,j++)
        {
            *currChunkPtr = j;
        }
    }

    currChunkInfoPtr = &devMemInfoPtr->chunkInfoArray[0];

    for(i = 0; i < NUM_CHUNKS_CNS; i++, currChunkInfoPtr++)
    {
        for(currChunkPtr = (GT_U32*)currChunkInfoPtr->chunkPtr, j=0; j<size[i]; currChunkPtr++,j++)
        {
            if(*currChunkPtr != j)
            {
                printf("smemFAP20MRegsCheckDirectMem : ERROR : set value doesn't match get value! start addr = 0x%8.8x\n",
                       currChunkInfoPtr->startAddr);
            }
        }
    }

    /* print the values*/
/*    currChunkInfoPtr = &devMemInfoPtr->chunkInfoArray[0];

    printf("i\t data\n",i, *currChunkPtr);

    for(i = 0; i < NUM_CHUNKS_CNS; i++, currChunkInfoPtr++)
    {
        for(currChunkPtr = (GT_U32*)currChunkInfoPtr->chunkPtr, j=0; j<size[i]; currChunkPtr++,j++)
        {
            printf("%d\t %d\n",i, *currChunkPtr);
        }
    }
*/

    smemFAP20MRegsInfoDump(devObjPtr);

}
