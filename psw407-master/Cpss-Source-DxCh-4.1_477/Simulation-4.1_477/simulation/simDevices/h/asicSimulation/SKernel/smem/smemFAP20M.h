/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemFAP20M.h
*
* DESCRIPTION:
*       Data definitions for FAP20M memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 3 $
*
*******************************************************************************/

#ifndef __smemFAP20Mh
#define __smemFAP20Mh

#include <asicSimulation/SKernel/smem/smem.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Number of bits in char (unsigned char) - 8.
 */
#define SAND_NOF_BITS_IN_CHAR   8

/*
 * Number of bits in long (unsigned long) - 32.
 */
#define SAND_NOF_BITS_IN_LONG   (SAND_NOF_BITS_IN_CHAR * sizeof(GT_32))

/*
 * SAND cells byte size.
 */
#define SAND_DATA_CELL_BYTE_SIZE 40
#define SAND_DATA_CELL_LONG_SIZE (SAND_DATA_CELL_BYTE_SIZE/sizeof(GT_32))


#define FAP20M_SCH_WORD_NOF_BITS  (96)
#define FAP20M_SCH_WORD_NOF_LONGS (FAP20M_SCH_WORD_NOF_BITS / SAND_NOF_BITS_IN_LONG)

#define FAP20M_NOF_FABRIC_LINKS    (24)
#define FAP20M_FDR_NOF_PROG_DATA_CELL_REGS  (4)

#define FAP20M_IDRAM_WORD_NOF_BITS  (256)
#define FAP20M_IDRAM_WORD_NOF_LONGS (FAP20M_IDRAM_WORD_NOF_BITS / SAND_NOF_BITS_IN_LONG)


typedef struct
{
    GT_U32      startAddr;/* actual start address of the unit*/
    GT_U32      lastAddr;/*actual last address of the unit*/
    GT_U32*     chunkPtr;
}SMEM_FAP20M_CHUNK_INFO_STC;

/*macro to fill the DB of chunks*/
#define SIM_CHUNK_INFO_SET_MAC(dbPtr,memIndex,name)   \
    dbPtr->startAddr = memIndex;                    \
    dbPtr->lastAddr =  memIndex + sizeof(name);     \
    dbPtr->chunkPtr = (GT_U32*)&name


typedef struct
{
  GT_U32   write_data ;
  GT_U32   address ;
  GT_U32   op_trigger ;
  GT_U32   read_data ;
} FAP20M_INDIRECT;


/********************************************************************************
 * Typedef: struct FAP20M_GLOBAL_REGS
 *
 * Description:
 *      Registers memory structure for global registers.
 ********************************************************************************/
typedef struct
{
  GT_U32   version ;               /*0x0000*/
  GT_U32   identification ;        /*0x0001*/
  GT_U32   spare_reg ;             /*0x0002*/
  GT_U32   miscellaneous ;         /*0x0003*/
  GT_U32   fill_00;                /*0x0004*/
  GT_U32   ingress_init ;          /*0x0005*/
  GT_U32   fill_001;               /*0x0006*/
  GT_U32   pd;                     /*0x0007*/
  GT_U32   fill_01[0x8];           /*0x0008-0x000f*/
  GT_U32   general_control;        /*0x0010*/
  GT_U32   SCH_cal_sel ;           /*0x0011*/
  GT_U32   mirror_addr ;           /*0x0012*/
  GT_U32   fill_11;                /*0x0013*/
  GT_U32   prim_sch_select;        /*0x0014*/
  GT_U32   fill_02[0xEB];          /*0x0015-0x00ff*/
  GT_U32   interrupt_block_mask;   /*0x0100*/
  GT_U32   interrupt_block_source; /*0x0101*/
  GT_U32   mask_all;               /*0x0102*/

} SMEM_FAP20M_GLOBAL_REGS;


/********************************************************************************
 * Typedef: struct SMEM_FAP20M_SCH_REGS
 *
 * Description:
 *      Registers memory structure for scheduler registers.
 ********************************************************************************/
typedef struct
{
  GT_U32   fill_10[1];               /*0x2700*/
  GT_U32   commands_1 ;              /*0x2701*/
  GT_U32   shaper_config ;           /*0x2702*/
  GT_U32   sec_slow_rate ;           /*0x2703*/
  GT_U32   credit_cnt_cfg1;          /*0x2704*/
  GT_U32   credit_cnt_cfg2;          /*0x2705*/
  GT_U32   fill_30[1] ;              /*0x2706*/
  GT_U32   SMP_msg_cnt_status_cfg ; /*0x2707*/
  GT_U32   fill_40[0x2] ;            /*0x2708 - 0x2709*/
  GT_U32   credit_cnt_cfg_agg;       /*0x270a*/
  GT_U32   fill_45[0x7] ;            /*0x270b - 0x2711*/
  GT_U32   DVS_config ;              /*0x2712*/
  GT_U32   link_a_cal_cfg ;          /*0x2713*/
  GT_U32   link_a_rates   ;          /*0x2714*/
  GT_U32   link_b_cal_cfg ;          /*0x2715*/
  GT_U32   link_b_rates   ;          /*0x2716*/
  GT_U32   cpu_rate      ;           /*0x2717*/
  GT_U32   fill_455  ;               /*0x2718*/
  GT_U32   interface_wfq_cfg  ;      /*0x2719*/
  GT_U32   credit_cnt_cfg_port  ;    /*0x271a*/
  GT_U32   fc_cnt_cfg_port  ;        /*0x271b*/
  GT_U32   fill_46[0x4] ;            /*0x271c - 0x271f*/

  GT_U32   hr_port_enable[0x8]  ;    /*0x2720-0x2727*/
  GT_U32   fill_50[0x8] ;            /*0x2728-0x272f*/

  GT_U32   cpu_force_flow_ctrl[0x8]  ; /*0x2730-0x2737*/
  GT_U32   fill_55[0x48] ;             /*0x2738-0x277f*/


  GT_U32   interrupts ;              /*0x2780*/
  GT_U32   fill_60[0xF];             /*0x2781 - 0x278f*/
  GT_U32   mask_interrupts;          /*0x2790*/
  GT_U32   fill_70[0x10];            /*0x2791 - 0x27a0*/
  GT_U32   indirect_address;                    /*0x27a1*/
  GT_U32   indirect_op_trigger;      /*0x27a2*/
  GT_U32   fill_80[0xD];             /*0x27a3 - 0x27af*/
  GT_U32   fill_800;                 /*0x27B0*/
  GT_U32   fill_805[0x14];           /*0x27b1 - 0x27c4*/

  GT_U32   indirect_write_data[FAP20M_SCH_WORD_NOF_LONGS];   /*0x27c5 - 0x27c7*/
  GT_U32   fill_85[0x5];                                     /*0x27c8 - 0x27cc*/
  GT_U32   indirect_read_data [FAP20M_SCH_WORD_NOF_LONGS];   /*0x27cd - 0x27cf*/
  GT_U32   fill_90[0x30];                                    /*0x27d0 - 0x27ff*/
  GT_U32   credit_cnt ;                                      /*0x2800*/
  GT_U32   fill_100[0x5] ;                                   /*0x2801 - 0x2805*/
  GT_U32   SMP_msg_cnt ;                                     /*0x2806*/
  GT_U32   agg_credit_cnt ;                                  /*0x2807*/
  GT_U32   fill_110[0x2] ;                                   /*0x2808 - 0x2809*/
  GT_U32   SCL_last_flow_restart ;                           /*0x280a*/
  GT_U32   fill_120[0x2] ;                                   /*0x280b - 0x280c*/
  GT_U32   shaper_dbg_bus ;                                  /*0x280d*/
  GT_U32   fill_125[0x2] ;                                   /*0x280e - 0x280f*/
  GT_U32   port_credit_cnt ;                                 /*0x2810*/
  GT_U32   fill_130 ;                                        /*0x2811*/
  GT_U32   port_fc_cnt ;                                     /*0x2812*/
} SMEM_FAP20M_SCH_REGS;


/********************************************************************************
 * Typedef: struct SMEM_FAP20M_DRC_REGS
 *
 * Description:
 *      Registers memory structure for drc registers.
 ********************************************************************************/
typedef struct
{
  GT_U32   ddr_control_trig ;  /*0xa00*/
  GT_U32   ddr_mode1 ;         /*0xa01*/
  GT_U32   ddr_mode2 ;         /*0xa02*/
  GT_U32   ddr_ext_mode ;      /*0xa03*/
  GT_U32   ac_op_cond1 ;       /*0xa04*/
  GT_U32   ac_op_cond2 ;       /*0xa05*/
  GT_U32   init_seq_reg ;      /*0xa06*/
  GT_U32   ac_op_cond3 ;       /*0xa07*/
  GT_U32   cpu_comand;         /*0xa08*/
  GT_U32   train_seq_command;  /*0xa09*/
  GT_U32   trn_seq[8];         /*0xa0A - 0xa11*/
  GT_U32   train_seq_addr;     /*0xa12*/
  GT_U32   drc_general_config; /*0xa13*/
  GT_U32   cfg_1;              /*0xa14*/
  GT_U32   cfg_2;              /*0xa15*/
  GT_U32   cfg_3;              /*0xa16*/
  GT_U32   cfg_4;              /*0xa17*/
  GT_U32   ddr_ext_mode_reg2;  /*0xa18*/
  GT_U32   ddr_ext_mode_reg3;  /*0xa19*/
  GT_U32   ac_opr_cond4;       /*0xa1a*/
  GT_U32   compliance_cfg;     /*0xa1b*/

  GT_U32   fill_001[0x3];      /*0xa1c-0xa1e*/
  GT_U32   glue_logic;         /*0xa1f*/
  GT_U32   bist_cfg;           /*0xa20*/

  GT_U32   fill_002[0x1F];     /*0xa21-0xa3f*/
  GT_U32   drc_status;         /*0xa40*/

  GT_U32   fill_01[0xBF];      /*0xa41-0xaff*/

} SMEM_FAP20M_DRC_REGS;


/********************************************************************************
 * Typedef: struct SMEM_FAP20M_FCT_REGS
 *
 * Description:
 *      Registers memory structure for fct registers.
 ********************************************************************************/
typedef struct
{
  GT_U32   enablers ;          /*0x1820*/
  GT_U32   tx_cell_buff[3] ;   /*0x1821*/
  GT_U32   fill_10[0x7];
  GT_U32   tx_cell_link_num ;  /*0x182b*/
  GT_U32   tx_cell_trig ;      /*0x182c*/
  GT_U32   fill_20[0x24];
  GT_U32   fill_30[0x5F] ;
} SMEM_FAP20M_FCT_REGS  ;

/********************************************************************************
 * Typedef: struct SMEM_FAP20M_SMS_REGS
 *
 * Description:
 *      Registers memory structure for sms registers.
 ********************************************************************************/
typedef struct
{
  GT_U32   sms_config_0 ;             /* 0x1911 */
  GT_U32   sms_config_1 ;             /* 0x1912 */
  GT_U32   sms_config_2 ;             /* 0x1913 */
  GT_U32   sms_config_3 ;             /* 0x1914 */

  GT_U32   fill_10[0xb];              /* 0x1915-0x191f */

  GT_U32   sms_status_reg ;           /* 0x1920 */
  GT_U32   queue_0_credits_gen_cnt ;  /* 0x1921 */
  GT_U32   queue_1_credits_gen_cnt ;  /* 0x1922 */
  GT_U32   queue_2_credits_gen_cnt ;  /* 0x1923 */
  GT_U32   queue_3_credits_gen_cnt ;  /* 0x1924 */
  GT_U32   unicast_credits_gen_cnt ;  /* 0x1925 */
  GT_U32   mci0_on_event_cnt;         /* 0x1926 */
  GT_U32   mci1_on_event_cnt;         /* 0x1927 */
  GT_U32   gci1_event_cnt;            /* 0x1928 */
  GT_U32   gci2_event_cnt;            /* 0x1929 */
  GT_U32   gci3_event_cnt;            /* 0x192a */
  GT_U32   flow_status_overflow_cnt;  /* 0x192b */
  GT_U32   leaky_buckets_value ;      /* 0x192c */
  GT_U32   spatial_mc_flow_statuses ; /* 0x192d */

  GT_U32   fill_20[0x2];              /* 0x192e-0x192f */

  GT_U32   shaper_parameters;         /* 0x1930 */

  GT_U32   fill_30[0xf];              /* 0x1931-0x193f */

  GT_U32   in_messages_cnt;           /* 0x1940 */
  GT_U32   out_messages_cnt;          /* 0x1941 */
  GT_U32   buffer_size_peak_detector; /* 0x1942 */
  GT_U32   off_messages_discard_cnt;  /* 0x1943 */
  GT_U32   on_messages_discard_cnt;   /* 0x1944 */

} SMEM_FAP20M_SMS_REGS;


/********************************************************************************
 * Typedef: struct SMEM_FAP20M_FCR_REGS
 *
 * Description:
 *      Registers memory structure fcr  registers.
 ********************************************************************************/
typedef struct
{
  GT_U32   enablers ;                /*0x1B27*/
  GT_U32   fill_10[0x8];
  GT_U32   connectivity_map[FAP20M_NOF_FABRIC_LINKS]; /*0x1B30*/
  GT_U32   fill_20[0x8];
  GT_U32   connectivity_map_change;  /*0x1B50*/

  GT_U32   fill_30[0x2F];

  GT_U32   interrupts ;              /*0x1B80*/
  GT_U32   fill_40[0xF];
  GT_U32   mask_interrupts ;         /*0x1B90*/
  GT_U32   fill_50[0xF];

  GT_U32   fill_60[0x2];
  GT_U32   rx_ctrl_cell[3];          /*0x1BA2*/

} SMEM_FAP20M_FCR_REGS;


/********************************************************************************
 * Typedef: struct SMEM_FAP20M_QDP_REGS
 *
 * Description:
 *      Registers memory structure for qdp registers.
 ********************************************************************************/
typedef struct
{
  GT_U32   initialization_trigger ;  /*2000*/
  GT_U32   fill_10[0xF];
  GT_U32   triger_packet_tx_from_q;  /*2010*/
  GT_U32   triger_packet_tx_q_num;   /*2011*/
  GT_U32   fill_20[0xE];
  GT_U32   auto_credit_params;       /*2020*/
  GT_U32   fill_25[0xF];

  GT_U32   commands;                 /*2030*/
  GT_U32   qsize_scrubber_params;    /*2031*/
  GT_U32   backoff_thresholds;       /*2032*/
  GT_U32   low_latency_queues[3];    /*2033 - 2035*/
  GT_U32   hp_queue_cfg ;            /*2036*/
  GT_U32   be_queues_cfg[3];         /*2037 - 2039*/
  GT_U32   fill_30;
  GT_U32   delete_hysteresis;        /*203b*/
  GT_U32   min_rate_queuess_and_rate_rates[3];        /*203c-203e*/
  GT_U32   metal_chicken_bits;       /*203f*/
  GT_U32   top_q_blk[3];             /*2040*/

  GT_U32   fill_40[0xD];
  GT_U32   global_ptr_dts;           /*2050*/
  GT_U32   top_buff_blk[3];          /*2051*/
  GT_U32   link_SRAM_conf_mode;      /*2054*/
  GT_U32   qdr_phy_interface_conf;   /*2055*/

  GT_U32   fill_50[0xA];
  GT_U32   en_q_pkt_cnt;             /*2060*/
  GT_U32   en_q_word_cnt;            /*2061*/
  GT_U32   hd_del_pkt_cnt;           /*2062*/
  GT_U32   hd_del_word_cnt;          /*2063*/
  GT_U32   de_q_pkt_cnt;             /*2064*/
  GT_U32   de_q_word_cnt;            /*2065*/
  GT_U32   tail_del_pkt_cnt;         /*2066*/
  GT_U32   tail_del_word_cnt;        /*2067*/
  GT_U32   flow_sts_msg_cnt;         /*2068*/
  GT_U32   msil0;                    /*2069*/
  GT_U32   msil1;                    /*206a*/
  GT_U32   msil2;                    /*206b*/

  GT_U32   fill_60[0x4];
  GT_U32   prg_en_q_pkt_cnt;         /*2070*/
  GT_U32   prg_en_q_word_cnt;        /*2071*/
  GT_U32   prg_hd_del_pkt_cnt;       /*2072*/
  GT_U32   prg_hd_del_word_cnt;      /*2073*/
  GT_U32   prg_de_q_pkt_cnt;         /*2074*/
  GT_U32   prg_de_q_word_cnt;        /*2075*/
  GT_U32   prg_tail_del_pkt_cnt;     /*2076*/
  GT_U32   prg_tail_del_word_cnt;    /*2077*/
  GT_U32   prg_flow_sts_msg_cnt;     /*2078*/
  GT_U32   peak_qsize;               /*2079*/
  GT_U32   fill_70[0x5];
  GT_U32   prg_cnt_queue_select;     /*207f*/

  GT_U32   interrupts ;              /*2080*/
  GT_U32   fill_04[0xF];
  GT_U32   mask_interrupts ;         /*2090*/
  GT_U32   fill_05[0xF];
  FAP20M_INDIRECT indirect;                 /*20a0 - 20a3*/
  GT_U32   fill_100[0xc];            /*20a4 - 20a*/
  GT_U32   wred_fix; /*0x20b0*/
} SMEM_FAP20M_QDP_REGS;
/********************************************************************************
 * Typedef: struct SMEM_FAP20M_RTP_REGS
 *
 * Description:
 *      Registers memory structure for rtp registers.
 ********************************************************************************/
typedef struct
{
  GT_U32   enablers ;            /*0x2200*/
  GT_U32   bypassing_the_tables;
  GT_U32   fill_05[2] ;
  GT_U32   fill_10[0x3];
  GT_U32   fap20_coexist ;
  GT_U32   fill_15[0x1];
  GT_U32   MulLinkUp;            /*0x2209 Rev B */
  GT_U32   fill_20[0x76];

  GT_U32   interrupts ;

  GT_U32   active_link_map ;

  GT_U32   fill_40[0xE];
  GT_U32   mask_interrupts ;
  GT_U32   fill_50 [0xF];
  FAP20M_INDIRECT indirect;

} SMEM_FAP20M_RTP_REGS;


/********************************************************************************
 * Typedef: struct SMEM_FAP20M_FDT_REGS
 *
 * Description:
 *      Registers memory structure for fdt registers.
 ********************************************************************************/
typedef struct
{
  GT_U32   enablers ;          /*0x2410*/
  GT_U32   tx_cell_buff[10] ;  /*0x2411-0x241a*/
  GT_U32   tx_cell_link_num ;  /*0x241b*/
  GT_U32   tx_cell_trig ;      /*0x241c*/
  GT_U32   fill_10[0x3];       /*0x241d-0x241f*/

  GT_U32   tx_data_cell_cnt ;  /*0x2420*/
  GT_U32   fill_15[0x2] ;      /*0x2421-0x2422*/
  GT_U32   duplicated_unicast; /*0x2423*/
  GT_U32   fill_16[0xc] ;      /*0x2424-0x242f*/

  GT_U32   eci_to_fdt_LinkRrobiMask[4];  /*0x2430 - 0x2433*/
  GT_U32   fill_20[0x4C];      /*0x2434-0x247f*/

  GT_U32   interrupts ;        /*0x2480*/
  GT_U32   fill_40[0xF];
  GT_U32   mask_interrupts ;   /*0x2490*/

  GT_U32   fill_50[0xF];

  FAP20M_INDIRECT indirect;           /*0x24a0-0x24a3*/
  GT_U32   fill_55[0xC];       /*0x24a4-0x24af*/
  GT_U32   accept_types;       /*0x24b0*/

} SMEM_FAP20M_FDT_REGS;


/********************************************************************************
 * Typedef: struct SMEM_FAP20M_FDR_REGS
 *
 * Description:
 *      Registers memory structure for fdr registers.
 ********************************************************************************/
typedef struct
{
  GT_U32   recv_sr_data_cell_a[SAND_DATA_CELL_LONG_SIZE] ;   /*0x2600*/
  GT_U32   fill_05[6] ;
  GT_U32   recv_sr_data_cell_b[SAND_DATA_CELL_LONG_SIZE] ;   /*0x2610*/

  GT_U32   fill_10[2] ;
  GT_U32   fill_15[1] ;                   /*0x261C*/
  GT_U32   overflow_and_fifo_statuses;    /*0x261D*/
  GT_U32   fill_20[3] ;
  GT_U32   prog_data_cell[FAP20M_FDR_NOF_PROG_DATA_CELL_REGS]; /*0x2621-0x2624*/
  GT_U32   enablers ;                    /*0x2625*/
  GT_U32   fill_25[0xA] ;                /*0x2626-0x262F*/
  GT_U32   ttl_good_reassembled_pcks;    /*0x2630*/
  GT_U32   ttl_pcks_discarded;           /*0x2631*/
  GT_U32   reassemble_cntr_1;            /*0x2632*/
  GT_U32   reassemble_cntr_2;            /*0x2633*/
  GT_U32   reassemble_cntr_3;            /*0x2634*/
  GT_U32   last_reassembeled_pct_header0;/*0x2635*/
  GT_U32   last_reassembeled_pct_header1;/*0x2636*/

  GT_U32   fill_30[0x49] ;               /*0x2637-0x267F*/
  GT_U32   interrupts ;                  /*0x2680*/
  GT_U32   fill_40[0xF];
  GT_U32   mask_interrupts ;
  GT_U32   fill_50[0xF];
  GT_U32   fill_60[0x10];
  GT_U32   accept_types;/*0x26b0*/
} SMEM_FAP20M_FDR_REGS;
/********************************************************************************
 * Typedef: struct SMEM_FAP20M_MMU_REGS
 *
 * Description:
 *      Registers memory structure for  registers.
 ********************************************************************************/
typedef struct
{
  GT_U32   enablers;       /*0x2900*/
  GT_U32   fill_00;        /*0x2901*/
  GT_U32   BAC_enablers;   /*0x2902*/
  GT_U32   fill_01;        /*0x2903*/
  GT_U32   addr_mapping;   /*0x2904*/
  GT_U32   qdf_thresholds; /*0x2905*/

  GT_U32   fill_10[0x7A];

  GT_U32   interrupts ;         /*0x2980*/
  GT_U32   fill_15[0x2];        /*0x2981-0x2982*/
  GT_U32   unicast_corrupt_cntr;/*0x2983*/
  GT_U32   fill_20[0xC];
  GT_U32   mask_interrupts ;    /*0x2990*/

  GT_U32   fill_30[0x10];
  GT_U32   indirect_address;    /*0x29A1*/
  GT_U32   indirect_op_trigger; /*0x29A2*/
  GT_U32   fill_35;             /*0x29A3*/
  GT_U32   mem_dram_rw16b;      /*0x29A4*/
  GT_U32   fill_40[0x1B];

  GT_U32   indirect_write_data[FAP20M_IDRAM_WORD_NOF_LONGS]; /*0x29C0*/
  GT_U32   indirect_read_data [FAP20M_IDRAM_WORD_NOF_LONGS]; /*0x29C8-0x29cf*/

  GT_U32   Last_hdr0;     /*0x29d0*/
  GT_U32   fill_60[0x6];  /*0x29d1-0x29d6*/
  GT_U32   Last_hdr7;     /*0x29d7*/

} SMEM_FAP20M_MMU_REGS;
/********************************************************************************
 * Typedef: struct SMEM_FAP20M_MAC_SERDES_REGS
 *
 * Description:
 *      Registers memory structure for mac serdes registers.
 ********************************************************************************/
typedef struct
{
  GT_U32   fill_01[1]; /*A:0x2A00  B:0x2B00*/
  GT_U32   rx_cell_cnt;
  GT_U32   tx_cell_cnt;
  GT_U32   loopback_and_prbs;
  GT_U32   serdes_conf;
  GT_U32   fill_02[2];
  GT_U32   ber_counters;
  GT_U32   fill_03[8];
} SMEM_FAP20M_MAC_SERDES_PER_LINK_CONF;

typedef struct
{
  GT_U32   power_down;/*A:0x2AE0  B:0x2BE0*/
  GT_U32   serdes_reset;
  GT_U32   fill_10[0xe];

} SMEM_FAP20M_MAC_SERDES_B_PER_HALF_CONF;

typedef struct
{
  GT_U32   mac_enabler ;         /*A:0x2AF0  B:0x2BF0*/
  GT_U32   leaky_bucket_ctrl ;
  GT_U32   cfg_01;
  GT_U32   serdes_reset;
  GT_U32   mac_rx_reset;
  GT_U32   fill_02[1] ;
  GT_U32   fill_03[2] ;

  GT_U32   interrupts_1 ;
  GT_U32   mask_interrupts_1 ;
  GT_U32   interrupts_2 ;
  GT_U32   mask_interrupts_2 ;
  GT_U32   interrupts_3 ;
  GT_U32   mask_interrupts_3 ;
  GT_U32   interrupts_4 ;
  GT_U32   mask_interrupts_4 ;

} SMEM_FAP20M_MAC_SERDES_PER_HALF_CONF;

typedef struct
{
  SMEM_FAP20M_MAC_SERDES_PER_LINK_CONF link_conf_a[FAP20M_NOF_FABRIC_LINKS/2];
  GT_U32   fill_10[0x20] ;
  SMEM_FAP20M_MAC_SERDES_B_PER_HALF_CONF half_conf_a_rev_b;
  SMEM_FAP20M_MAC_SERDES_PER_HALF_CONF   half_conf_a;

  SMEM_FAP20M_MAC_SERDES_PER_LINK_CONF link_conf_b[FAP20M_NOF_FABRIC_LINKS/2];
  GT_U32   fill_30[0x20] ;
  SMEM_FAP20M_MAC_SERDES_B_PER_HALF_CONF half_conf_b_rev_b;
  SMEM_FAP20M_MAC_SERDES_PER_HALF_CONF half_conf_b;
} SMEM_FAP20M_MAC_SERDES_REGS  ;

/********************************************************************************
 * Typedef: struct SMEM_FAP20M_CFG_REGS
 *
 * Description:
 *      Registers memory structure for cfg registers.
 ********************************************************************************/
typedef struct
{

  GT_U32   command_1 ;               /*0x2f00*/
  GT_U32   fill_10[1];
  GT_U32   command_2 ;               /*0x2f02*/
  GT_U32   command_3 ;               /*0x2f03*/
  GT_U32   fill_30[0x3C];
  GT_U32   counter;                  /*0x2f40*/

} SMEM_FAP20M_CFG_REGS;


/* find index of unit according to it's name */
#define SUB_UNIT_INDEX(subUnit)\
    SMEM_FAP20M_SUB_UNITS_##subUnit
/*
enumeration of subunits memories in the FAP20M
*/
typedef enum{
    SMEM_FAP20M_SUB_UNITS_globReg       ,
    SMEM_FAP20M_SUB_UNITS_schReg        ,
    SMEM_FAP20M_SUB_UNITS_drcReg0       ,
    SMEM_FAP20M_SUB_UNITS_drcReg1       ,
    SMEM_FAP20M_SUB_UNITS_drcReg2       ,
    SMEM_FAP20M_SUB_UNITS_drcReg3       ,
    SMEM_FAP20M_SUB_UNITS_fctReg        ,
    SMEM_FAP20M_SUB_UNITS_smsReg        ,
    SMEM_FAP20M_SUB_UNITS_fcrReg        ,
    SMEM_FAP20M_SUB_UNITS_qdpReg        ,
    SMEM_FAP20M_SUB_UNITS_rtpReg        ,
    SMEM_FAP20M_SUB_UNITS_fdtReg        ,
    SMEM_FAP20M_SUB_UNITS_fdrReg        ,
    SMEM_FAP20M_SUB_UNITS_schPrimReg    ,
    SMEM_FAP20M_SUB_UNITS_mmuReg        ,
    SMEM_FAP20M_SUB_UNITS_macSerdesReg  ,
    SMEM_FAP20M_SUB_UNITS_cfgReg        ,

    NUM_CHUNKS_CNS
}SMEM_FAP20M_SUB_UNITS_ENT;


/********************************************************************************
 * Typedef: struct SMEM_FAP20M_DEV_MEM_INFO
 *
 * Description:
 *      Registers memory structure for the whole FAP20M.
 *
 * Comments:
 *
 *  globReg     - start at: 0x0000
 *
 *  schReg      - start at: 0x0700
 *
 *  drcReg0     - start at: 0x0A00
 *  drcReg1     - start at: 0x0B00
 *  drcReg2     - start at: 0x0C00
 *  drcReg3     - start at: 0x0D00
 *
 *  fctReg      - start at: 0x1820
 *
 *  smsReg      - start at: 0x1911
 *
 *  fcrReg      - start at: 0x1b27
 *
 *  qdpReg      - start at: 0x2000
 *
 *  rtpReg      - start at: 0x2200
 *
 *  fdtReg      - start at: 0x2410
 *
 *  fdrReg      - start at: 0x2600
 *
 *  schPrimReg  - start at: 0x2700 (same structure as schReg)
 *
 *  mmuReg      - start at: 0x2900
 *
 *  macSerdesReg- start at: 0x2a00
 *
 *  cfgReg      - start at: 0x2f00
 ********************************************************************************/

typedef struct {
    SMEM_FAP20M_GLOBAL_REGS      globReg;
    SMEM_FAP20M_SCH_REGS         schReg;
    SMEM_FAP20M_DRC_REGS         drcReg[4];
    SMEM_FAP20M_FCT_REGS         fctReg;
    SMEM_FAP20M_SMS_REGS         smsReg;
    SMEM_FAP20M_FCR_REGS         fcrReg;
    SMEM_FAP20M_QDP_REGS         qdpReg;
    SMEM_FAP20M_RTP_REGS         rtpReg;
    SMEM_FAP20M_FDT_REGS         fdtReg;
    SMEM_FAP20M_FDR_REGS         fdrReg;
    SMEM_FAP20M_SCH_REGS         schPrimReg;
    SMEM_FAP20M_MMU_REGS         mmuReg;
    SMEM_FAP20M_MAC_SERDES_REGS  macSerdesReg;
    SMEM_FAP20M_CFG_REGS         cfgReg;

    /********MUST BE AT THE END OF THE CHUNKS *****************/
    SMEM_FAP20M_CHUNK_INFO_STC   chunkInfoArray[NUM_CHUNKS_CNS];

}SMEM_FAP20M_DEV_MEM_INFO;


/*******************************************************************************
* smemFAP20MInit
*
* DESCRIPTION:
*       Init memory module for a FAP20M device.
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
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);

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
*       activeMemPtr - pointer to the active memory entry or NULL if not exist.
*
* RETURNS:
*       pointer to the memory location in the database
*       NULL - if address not exist, or memSize > than existed size
*
* COMMENTS:
*
*******************************************************************************/
GT_U32*  smemFAP20MReg
(
    IN SKERNEL_DEVICE_OBJECT *     deviceObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE     accessType,
    IN GT_U32                      address,
    IN GT_U32                      memSize,
    INOUT SMEM_ACTIVE_MEM_ENTRY_STC **   activeMemPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemCht3h */



