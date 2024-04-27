/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemCapuera.h
*
* DESCRIPTION:
*       Data definitions for xbar capoeira adapter memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*
*******************************************************************************/
#ifndef __smemCapoeirah
#define __smemCapoeirah

#include <asicSimulation/SKernel/smem/smem.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



#define SMEM_CAP_CPUMAILBOX_TAR_PORT_REG       (0x00000084)
#define SMEM_CAP_CPU_MAIL_BOX_REG              (0x00200000)

/*
 * Typedef: struct XBAR_PORT0_MEM
 *          First port = 0x008XYFF , where x!=0xC and Y can be 0 or 8 or 6
 *              if x==0xC then y=0
 * Description:
 *  1. Control registers memory mapping
 *     Address space : 0x00800000 - 0x00800074   (control)
 *  2. HyperGlink registers memory mapping
 *     Address space : 0x00880000 - 0x0088005C   (hyperglink)
 *  3. MCGroupRegs registers memory mapping
 *     Address space : 0x00840000 - 0x00843FF8   (MC group)
 *  4. SerdesRegs registers memory mapping
 *     Address space : 0x008C0000 - 0x008C002C   (serdes)
 *  5. TarPort2DevRegs registers memory mapping
 *     Address space : 0x00804000 - 0x008040FC   (TarPort2Dev)
 *  6. CounterRegs registers memory mapping
 *     Address space : 0x00802080 - 0x00802084   (counter)
 * Fields:
 *
 *      ControlRegs        : Control registers memory space of port 0 .
 *      HyperGlinkRegs     : HyperGlink memory space of port 0 .
 *      MCGroupRegs        : MCGroup memory space of port 0 .
 *      SerdesRegs         : Serdes memory space of port 0 .
 *      TarPort2DevRegs    : TarPort2Dev memory space of port 0 .
 *      CounterRegs        : Counter memory space of port 0 .
 *
 * Comments:
 *
 *  ControlRegs - Registers with address mask 0x00800000 pattern 0x000000XX
 *                ControlRegsNum = 0x28 /4 + 1 = 0xB (11).
 *  HyperGlinkRegs - Registers with address mask 0x00880000 pattern 0x000000XX
 *                   HyperGlinkRegsNum = 0x5C /4 + 1 = 0x18 .
 *  MCGroupRegs - Registers with address mask 0x00840000 pattern 0x0000XXXX
 *                MCGroupRegsNum = 0x3FF8/4 + 1 = 0xFFF .
 *  SerdesRegs  - Registers with address mask 0x008C0000 pattern 0x000000XX
 *                SerdesRegsNum = 0x2C/4 + 1 = 0xC .
 *  TarPort2DevRegs - Registers with address mask 0x00804000 pattern 0x000000XX
 *                    TarPort2DevRegsNum = 0xFC/4 + 1 = 0x40 .
 *  CounterRegs     - Registers with address mask 0x00802080 pattern 0x0000008X
 *                    CounterRegsNum = 0x8/4 + 1 = 0x3 .
 ****/

typedef struct
{
    GT_U32                  ControlRegsNum;
    SMEM_REGISTER         * ControlRegs;
    GT_U32                  HyperGlinkRegsNum;
    SMEM_REGISTER         * HyperGlinkRegs;
    GT_U32                  MCGroupRegsNum;
    SMEM_REGISTER         * MCGroupRegs;
    GT_U32                  SerdesRegsNum;
    SMEM_REGISTER         * SerdesRegs;
    GT_U32                  TarPort2DevRegsNum;
    SMEM_REGISTER         * TarPort2DevRegs;
    GT_U32                  CounterRegsNum;
    SMEM_REGISTER         * CounterRegs;
}XBAR_PORT0_MEM;



/*
 * Typedef: struct XBAR_PORT1_MEM
 *          First port = 0x009XYFF , where x= 6 , 0 , 4  , 8 or C
 *              if x==0xC then y=0
 * Description:
 *  1. ControlRegs registers memory mapping
 *     Address space : 0x00900000 - 0x00900074
 *  2. HyperGlink registers memory mapping
 *     Address space : 0x00980000 - 0x0098005C
 *  3. MCGroupRegs registers memory mapping
 *     Address space : 0x00940000 - 0x00943FF8
 *  4. SerdesRegs registers memory mapping
 *     Address space : 0x009C0000 - 0x009C002C
 *  5. TarPort2DevRegs memory mapping
 *     Address space : 0x00904000 - 0x009040FC
 *  6. CounterRegs memory mapping
 *     Address space : 0x00902080 - 0x00902084
 * Fields:
 *      ControlRegs        : Control registers memory space of port 1 .
 *      HyperGlinkRegs     : HyperGlink memory space of port 1 .
 *      MCGroupRegs        : MCGroup memory space of port 1 .
 *      SerdesRegs         : Serdes memory space of port 1 .
 *      TarPort2DevRegs    : TarPort2Dev memory space of port 1 .
 *      CounterRegs        : Counter memory space of port 1 .
 *
 * Comments:
 *
 *  PGRegs           - Registers with address mask 0x00960000 pattern 0x000000XX
 *                     PGRegsNum = 0x1C / 4 + 1 = 0x7
 *  ControlRegs      - Registers with address mask 0x00900000 pattern 0x000000XX
 *                     ControlRegsNum = 0x74 /4 + 1 = 0x1E .
 *  HyperGlinkRegs   - Registers with address mask 0x00980000 pattern 0x000000XX
 *                     HyperGlinkRegsNum = 0x5C /4 + 1 = 0x18 .
 *  MCGroupRegs      - Registers with address mask 0x00940000 pattern 0x0000XXXX
 *                     MCGroupRegsNum = 0x3FF8/4 + 1 = 0xFFF .
 *  SerdesRegs       - Registers with address mask 0x009C0000 pattern 0x000000XX
 *                     SerdesRegsNum = 0x2C/4 + 1 = 0xC .
 *  TarPort2DevRegs  - Registers with address mask 0x00904000 pattern 0x000000XX
 *                     TarPort2DevRegsNum = 0xFC/4 + 1 = 0x40 .
 *  CounterRegs      - Registers with address mask 0x00902080 pattern 0x0000008X
 *                     CounterRegsNum = 0x8/4 + 1 = 0x3 .
 ****/

typedef struct
{
    GT_U32                  ControlRegsNum;
    SMEM_REGISTER         * ControlRegs;
    GT_U32                  HyperGlinkRegsNum;
    SMEM_REGISTER         * HyperGlinkRegs;
    GT_U32                  MCGroupRegsNum;
    SMEM_REGISTER         * MCGroupRegs;
    GT_U32                  SerdesRegsNum;
    SMEM_REGISTER         * SerdesRegs;
    GT_U32                  TarPort2DevRegsNum;
    SMEM_REGISTER         * TarPort2DevRegs;
    GT_U32                  CounterRegsNum;
    SMEM_REGISTER         * CounterRegs;
}XBAR_PORT1_MEM;



/*
 * Typedef: struct XBAR_PORT2_MEM
 *          First port = 0x00AXYFF ,where x= 6 , 0 , 4  , 8 or C
 *              if x==0xC then y=0
 * Description:
 *  1. Control registers registers memory mapping
 *     Address space : 0x00A00000 - 0x00A00074
 *  2. HyperGlink registers memory mapping
 *     Address space : 0x00A80000 - 0x00A8005C
 *  3. MCGroup registers memory mapping
 *     Address space : 0x00A40000 - 0x00A43FF8
 *  4. SerdesRegs registers memory mapping
 *     Address space : 0x00AC0000 - 0x00AC002C
 *  5. TarPort2Dev registers memory mapping
 *     Address space : 0x00A04000 - 0x00A040FC
 *  6. Counter registers memory mapping
 *     Address space : 0x00A02080 - 0x00A02084
 *
 * Fields:
 *      ControlRegs        : Control registers memory space of port 2 .
 *      HyperGlinkRegs     : HyperGlink memory space of port 2 .
 *      MCGroupRegs        : MCGroup memory space of port 2 .
 *      SerdesRegs         : Serdes memory space of port 2 .
 *      TarPort2DevRegs    : TarPort2Dev memory space of port 2 .
 *      CounterRegs        : Counter memory space of port 2 .
 *
 * Comments:
 *
 *  ControlRegs       - Registers with address mask 0x00A00000 pattern 0x000000XX
 *                      port2_BRegsNum = 0x74 /4 + 1 = 0x1E .
 *  HyperGlinkRegs    - Registers with address mask 0x00A80000 pattern 0x000000XX
 *                      port2_CRegsNum = 0x5c /4 + 1 = 0x18 .
 *  MCGroupRegs       - Registers with address mask 0x00A40000 pattern 0x0000XXXX
 *                      port2_DRegsNum = 0x3FF8/4 + 1 = 0xFFF .
 *  SerdesRegs        - Registers with address mask 0x00AC0000 pattern 0x000000XX
 *                      port2_ERegsNum = 0x2C/4 + 1 = 0xC .
 *  TarPort2DevRegs   - Registers with address mask 0x00A04000 pattern 0x000000XX
 *                      port2_FRegsNum = 0xFC/4 + 1 = 0x40 .
 *  CounterRegs       - Registers with address mask 0x00A02080 pattern 0x0000008X
 *                      port2_GRegsNum = 0x8/4 + 1 = 0x3 .
 ****/

typedef struct
{
    GT_U32                  ControlRegsNum;
    SMEM_REGISTER         * ControlRegs;
    GT_U32                  HyperGlinkRegsNum;
    SMEM_REGISTER         * HyperGlinkRegs;
    GT_U32                  MCGroupRegsNum;
    SMEM_REGISTER         * MCGroupRegs;
    GT_U32                  SerdesRegsNum;
    SMEM_REGISTER         * SerdesRegs;
    GT_U32                  TarPort2DevRegsNum;
    SMEM_REGISTER         * TarPort2DevRegs;
    GT_U32                  CounterRegsNum;
    SMEM_REGISTER         * CounterRegs;
}XBAR_PORT2_MEM;



/*
 * Typedef: struct XBAR_PORT3_MEM
 *          First port = 0x00BXYFF , where x= 6 , 0 , 4  , 8 or C
 *              if x==0xC then y=0
 * Description:
 *  2. Control registers memory mapping
 *     Address space : 0x00B00000 - 0x00B00074
 *  3. HyperGlink registers memory mapping
 *     Address space : 0x00B80000 - 0x00B8005C
 *  4. MCGroup registers memory mapping
 *     Address space : 0x00B40000 - 0x00B43FF8
 *  5. SerdesRegs registers memory mapping
 *     Address space : 0x00BC0000 - 0x00BC002C
 *  6. TarPort2Dev registers memory mapping
 *     Address space : 0x00B04000 - 0x00B040FC
 *  7. Counter registers memory mapping
 *     Address space : 0x00B02080 - 0x00B02084
 *
 * Fields:
 *      ControlRegs        : Control registers memory space of port 3 .
 *      HyperGlinkRegs     : HyperGlink memory space of port 3 .
 *      MCGroupRegs        : MCGroup memory space of port 3 .
 *      SerdesRegs         : Serdes memory space of port 3 .
 *      TarPort2DevRegs    : TarPort2Dev memory space of port 3 .
 *      CounterRegs        : Counter memory space of port 3 .
 *
 * Comments:
 *
 *  ControlRegs      - Registers with address mask 0x00B00000 pattern 0x000000XX
 *                     ControlRegsNum = 0x74/4 + 1 = 0x1E .
 *  HyperGlinkRegs   - Registers with address mask 0x00B80000 pattern 0x000000XX
 *                     HyperGlinkRegsNum = 0x5c /4 + 1 = 0x18 .
 *  MCGroupRegs      - Registers with address mask 0x00B40000 pattern 0x0000XXXX
 *                     MCGroupRegsNum = 0x3FF8/4 + 1 = 0xFFF .
 *  SerdesRegs       - Registers with address mask 0x00BC0000 pattern 0x000000XX
 *                     SerdesRegsNum = 0x2C/4 + 1 = 0xC .
 *  TarPort2DevRegs  - Registers with address mask 0x00B04000 pattern 0x000000XX
 *                     TarPort2DevRegsNum = 0xFC/4 + 1 = 0x40 .
 *  CounterRegs      - Registers with address mask 0x00B02080 pattern 0x0000008X
 *                     CounterRegsNum = 0x8/4 + 1 = 0x3 .
 ****/

typedef struct
{
    GT_U32                  ControlRegsNum;
    SMEM_REGISTER         * ControlRegs;
    GT_U32                  HyperGlinkRegsNum;
    SMEM_REGISTER         * HyperGlinkRegs;
    GT_U32                  MCGroupRegsNum;
    SMEM_REGISTER         * MCGroupRegs;
    GT_U32                  SerdesRegsNum;
    SMEM_REGISTER         * SerdesRegs;
    GT_U32                  TarPort2DevRegsNum;
    SMEM_REGISTER         * TarPort2DevRegs;
    GT_U32                  CounterRegsNum;
    SMEM_REGISTER         * CounterRegs;
}XBAR_PORT3_MEM;



/*
 * Typedef: struct XBAR_PORT4_MEM
 *          fourth port = 0x008XYFF , where x!=0xC and Y can be 8 or 0 or C
 *              if x==0xC then y=2
 * Description:
 *  1. Control registers memory mapping
 *     Address space : 0x00808000 - 0x00808074
 *  2. HyperGlink registers memory mapping
 *     Address space : 0x00888000 - 0x0088805C
 *  3. SerdesRegs registers memory mapping
 *     Address space : 0x008C2000 - 0x008C202C
 *  4. TarPort2Dev registers memory mapping
 *     Address space : 0x0080C000 - 0x0080C0FC
 *  5. CounterRegs registers memory mapping
 *     Address space : 0x0080A080 - 0x0080A084
 *
 * Fields:
 *      ControlRegs        : Control registers memory space of port 4 .
 *      HyperGlinkRegs     : HyperGlink memory space of port 4 .
 *      SerdesRegs         : Serdes memory space of port 4 .
 *      TarPort2DevRegs    : TarPort2Dev memory space of port 4 .
 *      CounterRegs        : Counter memory space of port 4 .
 *
 * Comments:
 *
 *  ControlRegs      - Registers with address mask 0x00808000 pattern 0x000000XX
 *                     CounterRegsNum = 0x74 / 4 + 1 =  0x1E .
 *  HyperGlinkRegs   - Registers with address mask 0x00888000 pattern 0x000000XX
 *                     HyperGlinkRegsNum = 0x5C /4 + 1 =  0x18 .
 *  SerdesRegs       - Registers with address mask 0x008C2018 pattern 0x000000XX
 *                     SerdesRegsNum = 0x2C /4 + 1   = 0xC   .
 *  TarPort2DevRegs  - RegistDrs with address mask 0x0080C000 pattern 0x000000XX
 *                     TarPort2DevRegsNum = 0xFC/4 + 1 = 0x40 .
 *  CounterRegs      - Registers with address mask 0x0080A080 pattern 0x0000008X
 *                     CounterRegsNum = 0x8/4 + 1 = 0x3 .
 ****/

typedef struct
{
    GT_U32                  ControlRegsNum;
    SMEM_REGISTER         * ControlRegs;
    GT_U32                  HyperGlinkRegsNum;
    SMEM_REGISTER         * HyperGlinkRegs;
    GT_U32                  SerdesRegsNum;
    SMEM_REGISTER         * SerdesRegs;
    GT_U32                  TarPort2DevRegsNum;
    SMEM_REGISTER         * TarPort2DevRegs;
    GT_U32                  CounterRegsNum;
    SMEM_REGISTER         * CounterRegs;
}XBAR_PORT4_MEM;




/*
 * Typedef: struct XBAR_PORT5_MEM
 *          fourth port = 0x009XYFF , where x!=0xC and Y can be 0 , 8 or C
 *              if x==0xC then y=2
 * Description:
 *  1. Control registers memory mapping
 *     Address space : 0x00908000 - 0x00908074
 *  2. HyperGlink registers memory mapping
 *     Address space : 0x00988000 - 0x0098805C
 *  3. SerdesRegs registers memory mapping
 *     Address space : 0x009C2000 - 0x009C202C
 *  4. TarPort2Dev registers memory mapping
 *     Address space : 0x0090C000 - 0x0090C0FC
 *  5. CounterRegs registers memory mapping
 *     Address space : 0x0090A080 - 0x0090A084
 *
 * Fields:
 *      ControlRegs        : Control registers memory space of port 5 .
 *      HyperGlinkRegs     : HyperGlink memory space of port 5 .
 *      SerdesRegs         : Serdes memory space of port 5 .
 *      TarPort2DevRegs    : TarPort2Dev memory space of port 5 .
 *      CounterRegs        : Counter memory space of port 5 .
 *
 * Comments:
 *
 *  ControlRegs      - Registers with address mask 0x00908000 pattern 0x000000XX
 *                     port5_1RegsNum = 0x74 / 4 + 1 = 0x1E  .
 *  HyperGlinkRegs   - Registers with address mask 0x00988000 pattern 0x000000XX
 *                     port5_2RegsNum = 0x5C /4 + 1  =  0x18 .
 *  SerdesRegs       - Registers with address mask 0x009C2018 pattern 0x000000X8
 *                     port5_3RegsNum = 0x2C /4 + 1   = 0xC   .
 *  TarPort2Dev      - Registers with address mask 0x0090C000 pattern 0x000000XX
 *                     port5_4RegsNum = 0xFC/4 + 1   = 0x40  .
 *  CounterRegs      - Registers with address mask 0x0090A080 pattern 0x0090A08X
 *                     port5_4RegsNum = 0x8/4 + 1    = 0x3   .
 ****/

typedef struct
{
    GT_U32                  ControlRegsNum;
    SMEM_REGISTER         * ControlRegs;
    GT_U32                  HyperGlinkRegsNum;
    SMEM_REGISTER         * HyperGlinkRegs;
    GT_U32                  SerdesRegsNum;
    SMEM_REGISTER         * SerdesRegs;
    GT_U32                  TarPort2DevRegsNum;
    SMEM_REGISTER         * TarPort2DevRegs;
    GT_U32                  CounterRegsNum;
    SMEM_REGISTER         * CounterRegs;
}XBAR_PORT5_MEM;


/*
 * Typedef: struct XBAR_PORT6_MEM
 *          fourth port = 0x00AXYFF , where x!=0xC and Y can be 8 or 0 or C
 *              if x==0xC then y=2
 * Description:
 *  1. Control registers memory mapping
 *     Address space : 0x00A08000 - 0x00A08074
 *  2. HyperGlink registers memory mapping
 *     Address space : 0x00A88000 - 0x00A8805C
 *  3. SerdesRegs registers memory mapping
 *     Address space : 0x00AC2018 - 0x00AC202C
 *  4. TarPort2Dev registers memory mapping
 *     Address space : 0x00A0C000 - 0x00A0C0FC
 *  5. CounterRegs registers memory mapping
 *     Address space : 0x00A0A080 - 0x00A0A084
 *
 * Fields:
 *      ControlRegs        : Control registers memory space of port 6 .
 *      HyperGlinkRegs     : HyperGlink memory space of port 6 .
 *      SerdesRegs         : Serdes memory space of port 6 .
 *      TarPort2DevRegs    : TarPort2Dev memory space of port 6 .
 *      CounterRegs        : Counter memory space of port 6 .
 *
 * Comments:
 *
 *  ControlRegs      - Registers with address mask 0x00A08000 pattern 0x000000XX
 *                     port6_ARegsNum = 0x74 / 4 + 1 =  0x1E
 *  HyperGlinkRegs   - Registers with address mask 0x00A88000 pattern 0x000000XX
 *                     port6_BRegsNum = 0x5C /4 + 1  =  0x18
 *  SerdesRegs       - Registers with address mask 0x00AC2018 pattern 0x000000XX
 *                     port6_CRegsNum = 0x8 /4 + 1   =  0xC.
 *  TarPort2DevRegs  - Registers with address mask 0x00A0C000 pattern 0x000000XX
 *                     port6_DRegsNum = 0xFC/4 + 1   =  0x40.
 *  CounterRegs      - Registers with address mask 0x00A0A080 pattern 0x0000008X
 *                     port6_ERegsNum = 0x8/4 + 1    =  0x3.
 ****/

typedef struct
{
    GT_U32                  ControlRegsNum;
    SMEM_REGISTER         * ControlRegs;
    GT_U32                  HyperGlinkRegsNum;
    SMEM_REGISTER         * HyperGlinkRegs;
    GT_U32                  SerdesRegsNum;
    SMEM_REGISTER         * SerdesRegs;
    GT_U32                  TarPort2DevRegsNum;
    SMEM_REGISTER         * TarPort2DevRegs;
    GT_U32                  CounterRegsNum;
    SMEM_REGISTER         * CounterRegs;
}XBAR_PORT6_MEM;


/*
 * Typedef: struct XBAR_PORT7_MEM
 *          fourth port = 0x00BXYFF , where x!=0xC and Y can be 8 or A or C
 *              if x==0xC then y=2
 * Description:
 *  1. Control registers memory mapping
 *     Address space : 0x00B08000 - 0x00B08074
 *  2. HyperGlink registers memory mapping
 *     Address space : 0x00B88000 - 0x00B8805C
 *  3. SerdesRegs registers memory mapping
 *     Address space : 0x00BC2000 - 0x00BC202C
 *  4. TarPort2Dev registers memory mapping
 *     Address space : 0x00B0C000 - 0x00B0C0FC
 *  5. CounterRegs registers memory mapping
 *     Address space : 0x00B0A080 - 0x00B0A084
 *
 * Fields:
 *      ControlRegs        : Control registers memory space of port 7 .
 *      HyperGlinkRegs     : HyperGlink memory space of port 7 .
 *      SerdesRegs         : Serdes memory space of port 7 .
 *      TarPort2DevRegs    : TarPort2Dev memory space of port 7 .
 *      CounterRegs        : Counter memory space of port 7 .
 *
 * Comments:
 *
 *  ControlRegs      - Registers with address mask 0x00B08000 pattern 0x000000XX
 *                     port7_ARegsNum = 0x74 / 4 + 1 =  0x1E .
 *  HyperGlinkRegs      - Registers with address mask 0x00B88000 pattern 0x000000XX
 *                     port7_BRegsNum = 0x5C /4 + 1 =  0x18 .
 *  SerdesRegs      - Registers with address mask 0x00BC2018 pattern 0x000000XX
 *                     port7_CRegsNum = 0x2C /4 + 1 = 0xC .
 *  TarPort2DevRegs      - Registers with address mask 0x00B0C000 pattern 0x000000XX
 *                     port7_DRegsNum = 0xFC/4 + 1 = 0x40 .
 *  CounterRegs      - Registers with address mask 0x00B0A080 pattern 0x0000008X
 *                     port7_ERegsNum = 0x8/4 + 1 = 0x3 .
 ****/

typedef struct
{
    GT_U32                  ControlRegsNum;
    SMEM_REGISTER         * ControlRegs;
    GT_U32                  HyperGlinkRegsNum;
    SMEM_REGISTER         * HyperGlinkRegs;
    GT_U32                  SerdesRegsNum;
    SMEM_REGISTER         * SerdesRegs;
    GT_U32                  TarPort2DevRegsNum;
    SMEM_REGISTER         * TarPort2DevRegs;
    GT_U32                  CounterRegsNum;
    SMEM_REGISTER         * CounterRegs;
}XBAR_PORT7_MEM;



/*
 * Typedef: struct XBAR_PORT8_MEM
 *          First port = 0x008XYFF , where can X can be  1 or 9 or C
 *              if x==0xC then y=4
 * Description:
 *  1. Control registers memory mapping
 *     Address space : 0x00810000 - 0x00810074
 *  2. HyperGlink registers memory mapping
 *     Address space : 0x00890000 - 0x0089005C
 *  3. SerdesRegs registers memory mapping
 *     Address space : 0x008C4000 - 0x008C402C
 *  4. TarPort2Dev registers memory mapping
 *     Address space : 0x00814000 - 0x008140FC
 *  5. CounterRegs registers memory mapping
 *     Address space : 0x00812080 - 0x00802084
 * Fields:
 *      ControlRegs        : Control registers memory space of port 8 .
 *      HyperGlinkRegs     : HyperGlink memory space of port 8 .
 *      SerdesRegs         : Serdes memory space of port 8 .
 *      TarPort2DevRegs    : TarPort2Dev memory space of port 8 .
 *      CounterRegs        : Counter memory space of port 8 .
 *
 * Comments:
 *
 *  ControlRegs      - Registers with address mask 0x00810000 pattern 0x000000XX
 *                     ControlRegsNum = 0x74 /4 + 1 =  0x1E
 *  HyperGlinkRegs   - Registers with address mask 0x00890000 pattern 0x000000XX
 *                     HyperGlinkRegsNum = 0x5C /4 + 1 = 0x18 .
 *  SerdesRegs       - Registers with address mask 0x008C4000 pattern 0x0000XXXX
 *                     SerdesRegsNum = 0x2C/4 + 1 =  0xC
 *  TarPort2DevRegs  - Registers with address mask 0x00814000 pattern 0x000000XX
 *                     TarPort2DevRegsNum = 0xFC/4 + 1 = 0x40 .
 *  CounterRegs      - Registers with address mask 0x00812080 pattern 0x0000008X
 *                     CounterRegsNum = 0x8/4 + 1 = 0x3
 ****/

typedef struct
{
    GT_U32                  ControlRegsNum;
    SMEM_REGISTER         * ControlRegs;
    GT_U32                  HyperGlinkRegsNum;
    SMEM_REGISTER         * HyperGlinkRegs;
    GT_U32                  SerdesRegsNum;
    SMEM_REGISTER         * SerdesRegs;
    GT_U32                  TarPort2DevRegsNum;
    SMEM_REGISTER         * TarPort2DevRegs;
    GT_U32                  CounterRegsNum;
    SMEM_REGISTER         * CounterRegs;
}XBAR_PORT8_MEM;


/*
 * Typedef: struct XBAR_PORT9_MEM
 *          First port = 0x009XYFF , where X can be 1 or 9 or C
 *              if x==0xC then y=4
 * Description:
 *  1. Control registers memory mapping
 *     Address space : 0x00910000 - 0x00910074
 *  2. HyperGlink registers memory mapping
 *     Address space : 0x00990000 - 0x0099005C
 *  3. SerdesRegs registers memory mapping
 *     Address space : 0x009C4000 - 0x009C402C
 *  4. TarPort2Dev registers memory mapping
 *     Address space : 0x00914000 - 0x009140FC
 *  5. CounterRegs registers memory mapping
 *     Address space : 0x00912080 - 0x00902084
 * Fields:
 *      ControlRegs        : Control registers memory space of port 9 .
 *      HyperGlinkRegs     : HyperGlink memory space of port 9 .
 *      SerdesRegs         : Serdes memory space of port 9 .
 *      TarPort2DevRegs    : TarPort2Dev memory space of port 9 .
 *      CounterRegs        : Counter memory space of port 9 .
 *
 * Comments:
 *
 *  ControlRegs      - Registers with address mask 0x00910000 pattern 0x000000XX
 *                     CounterRegsNum = 0x74 /4 + 1 = 0x1E
 *  HyperGlinkRegs   - Registers with address mask 0x00990000 pattern 0x000000XX
 *                     HyperGlinkRegsNum = 0x5C /4 + 1 = 0x18
 *  SerdesRegs       - Registers with address mask 0x009C4000 pattern 0x0000XXXX
 *                     SerdesRegsNum = 0x2C/4 + 1 = 0xC
 *  TarPort2DevRegs  - Registers with address mask 0x00914000 pattern 0x000000XX
 *                     TarPort2DevRegsNum = 0xFC/4 + 1 = 0x40
 *  CounterRegs      - Registers with address mask 0x00912080 pattern 0x0000008X
 *                     CounterRegsNum = 0x8/4 + 1 =0x3
 ****/

typedef struct
{
    GT_U32                  ControlRegsNum;
    SMEM_REGISTER         * ControlRegs;
    GT_U32                  HyperGlinkRegsNum;
    SMEM_REGISTER         * HyperGlinkRegs;
    GT_U32                  SerdesRegsNum;
    SMEM_REGISTER         * SerdesRegs;
    GT_U32                  TarPort2DevRegsNum;
    SMEM_REGISTER         * TarPort2DevRegs;
    GT_U32                  CounterRegsNum;
    SMEM_REGISTER         * CounterRegs;
}XBAR_PORT9_MEM;


/*
 * Typedef: struct XBAR_PORT10_MEM
 *          First port = 0x00AXYFF , where X can be 1 or 9 or C
 *              if x==0xC then y=4
 * Description:
 *  1. Control registers memory mapping
 *     Address space : 0x00A10000 - 0x00A10074
 *  2. HyperGlink registers memory mapping
 *     Address space : 0x00A90000 - 0x00A9005C
 *  3. SerdesRegs registers memory mapping
 *     Address space : 0x00AC4000 - 0x00AC402C
 *  4. TarPort2Dev registers memory mapping
 *     Address space : 0x00A14000 - 0x00A140FC
 *  5. CounterRegs registers memory mapping
 *     Address space : 0x00A12080 - 0x00A02084
 * Fields:
 *      ControlRegs        : Control registers memory space of port 10 .
 *      HyperGlinkRegs     : HyperGlink memory space of port 10 .
 *      SerdesRegs         : Serdes memory space of port 10 .
 *      TarPort2DevRegs    : TarPort2Dev memory space of port 10 .
 *      CounterRegs        : Counter memory space of port 10 .
 *
 * Comments:
 *
 *  ControlRegs     - Registers with address mask 0x00A10000 pattern 0x000000XX
 *                     port10_ARegsNum = 0x74 /4 + 1 = 0x1E
 *  HyperGlinkRegs  - Registers with address mask 0x00A90000 pattern 0x000000XX
 *                     port10_BRegsNum = 0x5C /4 + 1 = 0x18 .
 *  SerdesRegs      - Registers with address mask 0x00AC4000 pattern 0x00000XXX
 *                     port10_CRegsNum = 0x2C/4 + 1 = 0xC
 *  TarPort2DevRegs - Registers with address mask 0x00A14000 pattern 0x000000XX
 *                     port10_DRegsNum = 0xFC/4 + 1 = 0x40 .
 *  CounterRegsNum  - Registers with address mask 0x00A12080 pattern 0x0000008X
 *                     port10_ERegsNum = 0x8/4 + 1 = 0x3
 ****/

typedef struct
{
    GT_U32                  ControlRegsNum;
    SMEM_REGISTER         * ControlRegs;
    GT_U32                  HyperGlinkRegsNum;
    SMEM_REGISTER         * HyperGlinkRegs;
    GT_U32                  SerdesRegsNum;
    SMEM_REGISTER         * SerdesRegs;
    GT_U32                  TarPort2DevRegsNum;
    SMEM_REGISTER         * TarPort2DevRegs;
    GT_U32                  CounterRegsNum;
    SMEM_REGISTER         * CounterRegs;
}XBAR_PORT10_MEM;

/*
 * Typedef: struct XBAR_PORT11_MEM
 *          First port = 0x00BXYFF , where X can be 1 or 9 or C
 *              if x==0xC then y=4
 * Description:
 *  1. Control registers memory mapping
 *     Address space : 0x00B10000 - 0x00B10074
 *  2. HyperGlink registers memory mapping
 *     Address space : 0x00B90000 - 0x00B9005C
 *  3. SerdesRegs registers memory mapping
 *     Address space : 0x00BC4000 - 0x00BC402C
 *  4. TarPort2Dev registers memory mapping
 *     Address space : 0x00B14000 - 0x00B140FC
 *  5. CounterRegs registers memory mapping
 *     Address space : 0x00B12080 - 0x00B02084
 *
 * Fields:
 *      ControlRegs        : Control registers memory space of port 11 .
 *      HyperGlinkRegs     : HyperGlink memory space of port 11 .
 *      SerdesRegs         : Serdes memory space of port 11 .
 *      TarPort2DevRegs    : TarPort2Dev memory space of port 11 .
 *      CounterRegs        : Counter memory space of port 11 .
 *
 * Comments:
 *
 *  ControlRegs      - Registers with address mask 0x00B10000 pattern 0x000000XX
 *                     ControlRegsNum = 0x74 /4 + 1 =    0x1E .
 *  HyperGlinkRegs   - Registers with address mask 0x00B90000 pattern 0x000000XX
 *                     HyperGlinkRegsNum = 0x5C /4 + 1 = 0x18 .
 *  SerdesRegs       - Registers with address mask 0x00BC4000 pattern 0x00000XXX
 *                     SerdesRegsNum = 0x2C/4 + 1 = 0xC .
 *  TarPort2DevRegs  - Registers with address mask 0x00B14000 pattern 0x000000XX
 *                     TarPort2DevRegsNum = 0xFC/4 + 1 = 0x40 .
 *  CounterRegs      - Registers with address mask 0x00B12080 pattern 0x0000008X
 *                     CounterRegsNum = 0x8/4 + 1 = 3 .
 ****/

typedef struct
{
    GT_U32                  ControlRegsNum;
    SMEM_REGISTER         * ControlRegs;
    GT_U32                  HyperGlinkRegsNum;
    SMEM_REGISTER         * HyperGlinkRegs;
    GT_U32                  SerdesRegsNum;
    SMEM_REGISTER         * SerdesRegs;
    GT_U32                  TarPort2DevRegsNum;
    SMEM_REGISTER         * TarPort2DevRegs;
    GT_U32                  CounterRegsNum;
    SMEM_REGISTER         * CounterRegs;
}XBAR_PORT11_MEM;



/*
 * Typedef: struct XBAR_PG_MEM

 * Description:
 *  1. PG0 registers memory mapping
 *     Address space : 0x00860000 - 0x0086001C   (PG group)
 *  2. PG1 registers memory mapping
 *     Address space : 0x00960000 - 0x0096001C
 *  3. PG2 registers memory mapping
 *     Address space : 0x00A60000 - 0x00A6001C
 *  4. PG3 registers memory mapping
 *     Address space : 0x00B60000 - 0x00B6001C
 *
 * Fields:
 *      PG0Regs            : PG registers number memory space of port 1 .
 *      PG1Regs            : PG registers number memory space of port 2 .
 *      PG2Regs            : PG registers number memory space of port 3 .
 *      PG3Regs            : PG registers number memory space of port 4 .
 *
 * Comments:
 *
 *  PG0Regs  - Registers with address mask 0x00860000 pattern 0x000000XX
 *             PGRegsNum = 0x1C / 4 + 1 = 0x7
 *  PG1Regs  - Registers with address mask 0x00960000 pattern 0x000000XX
 *             PGRegsNum = 0x1C / 4 + 1 = 0x7
 *  PG2Regs  - Registers with address mask 0x00A60000 pattern 0x000000XX
 *             PG3RegsNum = 0x1C / 4 + 1 = 0x7 .
 *  PG3Regs  - Registers with address mask 0x00B60000 pattern 0x000000XX
 *             PG4RegsNum = 0x1C / 4 + 1 = 0x7
 ****/

typedef struct
{
    GT_U32                  PGRegsNum;
    SMEM_REGISTER         * PG0Regs;
    SMEM_REGISTER         * PG1Regs;
    SMEM_REGISTER         * PG2Regs;
    SMEM_REGISTER         * PG3Regs;
}XBAR_PG_MEM;


/*
 * Typedef: struct XBAR_GLOBAL_MEM
 *
 * Description:
 *  1. Global memory register
 *     Address space : 0x0000000  - 0x00000284
 *
 * Fields:
 *      globRegsNum : first memory space of port  11 .
 *      globRegs    : second memory space of port 11 .
 *
 * Comments:
 *
 *  globRegs         - Registers with address mask 0x0000000 pattern 0x00000XXX
 *                     globRegsNum = 0x2FF/4 + 1 = 0xC0 .
 ****/

typedef struct
{
    GT_U32                  globRegsNum;
    SMEM_REGISTER         * globRegs;
}XBAR_GLOBAL_MEM;



/*
 * Typedef: struct XBAR_DEV_MEM_INFO
 *
 * Description:
 *      Describe a device's memory object in the simulation.
 *
 * Fields:
 *      memMutex          : Memory mutex for device's memory.
 *      specFunTbl        : Address type specific R/W functions.
 *      Fport1Mem         : fabric port 1 resgiters.
 *      Fport2Mem         : fabric port 2 resgiters.
 *      Fport3Mem         : fabric port 3 resgiters.
 *      Fport4Mem         : fabric port 4 resgiters.
 *      Fport5Mem         : fabric port 5 resgiters.
 *      Fport6Mem         : fabric port 6 resgiters.
 *      Fport7Mem         : fabric port 7 resgiters.
 *      Fport8Mem         : fabric port 8 resgiters.
 *      Fport9Mem         : fabric port 9 resgiters.
 *      Fport10Mem        : fabric port 10 resgiters.
 *      Fport11Mem        : fabric port 11 resgiters.
 *      GlobalMem         : Global memory registers.
 *
 * Comments:
 */

typedef struct
{
    SMEM_SPEC_FIND_FUN_ENTRY_STC specFunTbl[64];
    XBAR_PORT0_MEM               Fport0Mem;
    XBAR_PORT1_MEM               Fport1Mem;
    XBAR_PORT2_MEM               Fport2Mem;
    XBAR_PORT3_MEM               Fport3Mem;
    XBAR_PORT4_MEM               Fport4Mem;
    XBAR_PORT5_MEM               Fport5Mem;
    XBAR_PORT6_MEM               Fport6Mem;
    XBAR_PORT7_MEM               Fport7Mem;
    XBAR_PORT8_MEM               Fport8Mem;
    XBAR_PORT9_MEM               Fport9Mem;
    XBAR_PORT10_MEM              Fport10Mem;
    XBAR_PORT11_MEM              Fport11Mem;
    XBAR_GLOBAL_MEM              GlobalMem;
    XBAR_PG_MEM                  PGMem;
}CAP_XBAR_DEV_MEM_INFO;


/*******************************************************************************
* smemCapXbarInit
*
* DESCRIPTION:
*       Init memory module for capoeira crossbar device.
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
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);

/*******************************************************************************
* smemCapXbarFindMem
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
void * smemCapXbarFindMem
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemCapoeira */


