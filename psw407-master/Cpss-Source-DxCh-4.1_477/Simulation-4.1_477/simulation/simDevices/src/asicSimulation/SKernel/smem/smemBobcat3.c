/******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemBobcat3.c
*
* DESCRIPTION:
*       Bobcat3 memory mapping implementation
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/smem/smemBobcat3.h>
#include <asicSimulation/SKernel/cheetahCommon/sregLion2.h>
#include <asicSimulation/SKernel/cheetahCommon/sregBobcat2.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>

/* port per DP --without-- the CPU port */
#define PORTS_PER_DP_CNS    12

/*Gets 'cookiePtr' of the thread*/
extern GT_PTR simOsTaskOwnTaskCookieGet(GT_VOID);

extern void startSimulationLog(void);
/* there are no more then 88 units , without pipe 1 units !!! */
#define BOBCAT3_NUM_UNITS_WITHOUT_PIPE_1_E     88

/* indication of unit in pipe 1*/
#define PIPE_1_INDICATION_CNS (SMAIN_NOT_VALID_CNS-1)

/* the offset of second pipe units */
#define SECOND_PIPE_OFFSET_CNS 0x80000000

/* not used memory */
#define DUMMY_UNITS_BASE_ADDR_CNS(index)              0x70000000 + UNIT_BASE_ADDR_MAC(2*index)

#define SHARED_BETWEEN_PIPE_0_AND_PIPE_1_INDICATION_CNS     DUMMY_NAME_PTR_CNS

typedef struct{
    GT_U32      base_addr;/* base address of unit*/
    GT_CHAR*    nameStr  ;/* name of unit */
    GT_U32      size;/* number of units (each unit is (1<<24) bytes size) */
    GT_CHAR*    orig_nameStr  ;/* when orig_nameStr is not NULL than the nameStr
                                  is name of the duplicated unit , and the
                                  orig_nameStr is original unit.

                                  BUT if SHARED_BETWEEN_PIPE_0_AND_PIPE_1_INDICATION_CNS meaning : shared unit between pipe 0 and pipe 1
                                  */
}UNIT_INFO_STC;

#define UNIT_INFO_MAC(baseAddr,unitName) \
    UNIT_INFO_LARGE_MAC(baseAddr,unitName,1)

#define UNIT_INFO_LARGE_MAC(baseAddr,unitName,size) \
     {baseAddr , STR(unitName) , size}

/* unit in pipe 1 according to pipe 0 */
#define UNIT_INFO_PIPE_1_MAC(unitName) \
     {PIPE_1_INDICATION_CNS , ADD_INSTANCE_OF_UNIT_TO_STR(STR(unitName),1) , 1 , STR(unitName)}

#define UNIT_INFO_PIPE_1_LARGE_MAC(unitName,size) \
     {PIPE_1_INDICATION_CNS , ADD_INSTANCE_OF_UNIT_TO_STR(STR(unitName),1) , size , STR(unitName)}


/* unit shared between pipe 0 and pipe 1 */
#define UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(baseAddr,unitName) \
     UNIT_INFO_LARGE_SHARED_PIPE_0_PIPE_1_MAC(baseAddr,unitName, 1)

/* large unit shared between pipe 0 and pipe 1 */
#define UNIT_INFO_LARGE_SHARED_PIPE_0_PIPE_1_MAC(baseAddr,unitName, size) \
     {baseAddr , STR(unitName) , size , SHARED_BETWEEN_PIPE_0_AND_PIPE_1_INDICATION_CNS}


/* use 4 times the 'BOBCAT3_NUM_UNITS_WITHOUT_PIPE_1_E' because:
   2 times to support duplicated units from pipe 1
   2 time to support that each unit is '8 MSbits' and not '9 MSbits'
*/
static SMEM_UNIT_NAME_AND_INDEX_STC bobcat3UnitNameAndIndexArr[(4*BOBCAT3_NUM_UNITS_WITHOUT_PIPE_1_E)+1]=
{
    /* filled in runtime from bobcat3units[] */
    /* must be last */
    {NULL ,                                SMAIN_NOT_VALID_CNS                     }
};


/* the addresses of the units that the bobk uses */
static GT_U32   bobcat3UsedUnitsAddressesArray[(4*BOBCAT3_NUM_UNITS_WITHOUT_PIPE_1_E)+1]=
{
    0    /* filled in runtime from bobcat3units[] */
};

/* the units of bobcat3 */
static UNIT_INFO_STC bobcat3units[(2*BOBCAT3_NUM_UNITS_WITHOUT_PIPE_1_E)+1] =
{
     UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(0x00000000,UNIT_MG                )
    ,UNIT_INFO_MAC(0x01000000,UNIT_TTI               )
    ,UNIT_INFO_MAC(0x02000000,UNIT_IPCL              )
    ,UNIT_INFO_MAC(0x03000000,UNIT_L2I               )
    ,UNIT_INFO_MAC(0x04000000,UNIT_IPVX              )
    ,UNIT_INFO_MAC(0x05000000,UNIT_IPLR              )
    ,UNIT_INFO_MAC(0x06000000,UNIT_IPLR1             )
    ,UNIT_INFO_MAC(0x07000000,UNIT_IOAM              )
    ,UNIT_INFO_MAC(0x08000000,UNIT_MLL               )
    ,UNIT_INFO_MAC(0x09000000,UNIT_EQ                )
    ,UNIT_INFO_MAC(0x0a000000,UNIT_EGF_EFT           )

    ,UNIT_INFO_MAC(0x0b000000,UNIT_TXQ_DQ            )
    ,UNIT_INFO_MAC(0x0c000000,UNIT_TXQ_DQ_1          )
    ,UNIT_INFO_MAC(0x0d000000,UNIT_TXQ_DQ_2          )

    ,UNIT_INFO_MAC(0x0e000000,UNIT_CNC               )
    ,UNIT_INFO_MAC(0x0f000000,UNIT_CNC_1             )

    ,UNIT_INFO_MAC(0x10000000,UNIT_GOP               )
    ,UNIT_INFO_MAC(0x12000000,UNIT_XG_PORT_MIB       )
    ,UNIT_INFO_MAC(0x13000000,UNIT_SERDES            )
    ,UNIT_INFO_MAC(0x14000000,UNIT_HA                )
    ,UNIT_INFO_MAC(0x15000000,UNIT_ERMRK             )
    ,UNIT_INFO_MAC(0x16000000,UNIT_EPCL              )
    ,UNIT_INFO_MAC(0x17000000,UNIT_EPLR              )
    ,UNIT_INFO_MAC(0x18000000,UNIT_EOAM              )

    ,UNIT_INFO_MAC(0x19000000,UNIT_RX_DMA            )
    ,UNIT_INFO_MAC(0x1a000000,UNIT_RX_DMA_1          )
    ,UNIT_INFO_MAC(0x1b000000,UNIT_RX_DMA_2          )
    ,UNIT_INFO_MAC(0x1c000000,UNIT_TX_DMA            )
    ,UNIT_INFO_MAC(0x1d000000,UNIT_TX_DMA_1          )
    ,UNIT_INFO_MAC(0x1e000000,UNIT_TX_DMA_2          )
    ,UNIT_INFO_MAC(0x1f000000,UNIT_TX_FIFO           )
    ,UNIT_INFO_MAC(0x20000000,UNIT_TX_FIFO_1         )
    ,UNIT_INFO_MAC(0x21000000,UNIT_TX_FIFO_2         )
    ,UNIT_INFO_MAC(0x22000000,UNIT_RX_DMA_GLUE       )
    ,UNIT_INFO_MAC(0x23000000,UNIT_FCU       )
    ,UNIT_INFO_MAC(0x24000000,UNIT_SBC       )
    ,UNIT_INFO_MAC(0x25000000,UNIT_NSEC       )
    ,UNIT_INFO_MAC(0x26000000,UNIT_NSEF       )
    ,UNIT_INFO_MAC(0x27000000,UNIT_GOP_LED_0           )
    ,UNIT_INFO_MAC(0x28000000,UNIT_GOP_LED_1           )
    ,UNIT_INFO_MAC(0x29000000,UNIT_GOP_SMI_0           )
    ,UNIT_INFO_MAC(0x2a000000,UNIT_GOP_SMI_1           )

    ,UNIT_INFO_LARGE_MAC(0x30000000,UNIT_EGF_SHT    , 8)    /* 1. was shared between pipes , but now it is 'per pipe'
                                                               2. was in 0x48000000*/

    /* shared units between pipe 0 and pipe 1 */
    ,UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(0x40000000,UNIT_BM                )
    ,UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(0x41000000,UNIT_BMA               )
    ,UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(0x42000000,UNIT_CPFC              )
    ,UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(0x43000000,UNIT_FDB               )
    ,UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(0x44000000,UNIT_LPM               )
    ,UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(0x45000000,UNIT_LPM_1             )
    ,UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(0x46000000,UNIT_LPM_2             )
    ,UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(0x47000000,UNIT_LPM_3             )
    ,UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(0x50000000,UNIT_EGF_QAG           )
    ,UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(0x51000000,UNIT_MPPM              )
    ,UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(0x52000000,UNIT_TCAM              )
    ,UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(0x53000000,UNIT_TXQ_LL            )
    ,UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(0x54000000,UNIT_TXQ_QCN           )
    ,UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(0x55000000,UNIT_TXQ_QUEUE         )
    ,UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(0x56000000,UNIT_TXQ_BMX           )
    ,UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(0x57000000,UNIT_TAI               )

    ,UNIT_INFO_SHARED_PIPE_0_PIPE_1_MAC(DUMMY_UNITS_BASE_ADDR_CNS(0), UNIT_FDB_TABLE_0 )
    /*********** end of pipe 0 units and shared units **************/

    /*********** start of pipe 1 units **************/

    ,UNIT_INFO_PIPE_1_MAC(UNIT_TTI               )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_IPCL              )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_L2I               )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_IPVX              )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_IPLR              )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_IPLR1             )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_IOAM              )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_MLL               )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_EQ                )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_EGF_EFT           )

    ,UNIT_INFO_PIPE_1_MAC(UNIT_TXQ_DQ            )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_TXQ_DQ_1          )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_TXQ_DQ_2          )

    ,UNIT_INFO_PIPE_1_MAC(UNIT_CNC               )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_CNC_1             )

    ,UNIT_INFO_PIPE_1_MAC(UNIT_GOP               )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_XG_PORT_MIB       )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_SERDES            )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_HA                )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_ERMRK             )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_EPCL              )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_EPLR              )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_EOAM              )

    ,UNIT_INFO_PIPE_1_MAC(UNIT_RX_DMA            )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_RX_DMA_1          )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_RX_DMA_2          )

    ,UNIT_INFO_PIPE_1_MAC(UNIT_TX_DMA            )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_TX_DMA_1          )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_TX_DMA_2          )

    ,UNIT_INFO_PIPE_1_MAC(UNIT_TX_FIFO           )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_TX_FIFO_1         )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_TX_FIFO_2         )

    ,UNIT_INFO_PIPE_1_MAC(UNIT_RX_DMA_GLUE       )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_FCU       )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_SBC       )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_NSEC       )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_NSEF       )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_GOP_LED_0           )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_GOP_LED_1           )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_GOP_SMI_0           )
    ,UNIT_INFO_PIPE_1_MAC(UNIT_GOP_SMI_1           )

    ,UNIT_INFO_PIPE_1_LARGE_MAC(UNIT_EGF_SHT    , 8)


    /* must be last */
    ,{SMAIN_NOT_VALID_CNS,NULL,SMAIN_NOT_VALID_CNS}
};

/* NOTE: all units that are duplicated from pipe 0 to pipe 1 are added into this array in runtime !!!
    it is built from bobcat3units[].orig_nameStr*/
static SMEM_UNIT_DUPLICATION_INFO_STC BOBCAT3_duplicatedUnits[120] =
{
    {STR(UNIT_RX_DMA)  ,2}, /* 2 duplication of this unit */
        {STR(UNIT_RX_DMA_1)},
        {STR(UNIT_RX_DMA_2)},

    {STR(UNIT_TX_FIFO) ,2}, /* 2 duplication of this unit */
        {STR(UNIT_TX_FIFO_1)},
        {STR(UNIT_TX_FIFO_2)},

    {STR(UNIT_TX_DMA)  ,2}, /* 2 duplication of this unit */
        {STR(UNIT_TX_DMA_1)},
        {STR(UNIT_TX_DMA_2)},

    {STR(UNIT_TXQ_DQ)  ,2}, /* 2 duplication of this unit */
        {STR(UNIT_TXQ_DQ_1)},
        {STR(UNIT_TXQ_DQ_2)},

    {STR(UNIT_LPM)  ,3}, /* 3 duplication of this unit */
        {STR(UNIT_LPM_1)},
        {STR(UNIT_LPM_2)},
        {STR(UNIT_LPM_3)},

    /* start duplication on units of instance 0 , to instance 1 !!! */

    /* NOTE: all units that are duplicated from pipe 0 to pipe 1 are added into this array in runtime !!!
        it is built from bobcat3units[].orig_nameStr*/


    {NULL,0} /* must be last */
};

extern SMEM_REGISTER_DEFAULT_VALUE_STC bobcat2Serdes_registersDefaultValueArr[];

typedef enum{
     UNIT_TYPE_NOT_VALID         = 0
    ,UNIT_TYPE_PIPE_0_ONLY_E
    ,UNIT_TYPE_PIPE_1_ONLY_E
    ,UNIT_TYPE_PIPE_0_AND_PIPE_1_E
}UNIT_TYPE_ENT;

/* array of info about '8 MSB' unit addresses */
static UNIT_TYPE_ENT  bobcat3UnitTypeArr[SMEM_CHT_NUM_UNITS_MAX_CNS / 2] =
{
    0 /* set in runtime from bobcat3units[] */
};


static GT_U32 findBaseAddrOfUnit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN UNIT_INFO_STC   *unitsInfoPtr,
    IN GT_CHAR*         unitNamePtr
)
{
    GT_U32  ii;
    UNIT_INFO_STC   *unitInfoPtr = &unitsInfoPtr[0];

    for(ii = 0 ; unitInfoPtr->base_addr != SMAIN_NOT_VALID_CNS; ii++,unitInfoPtr++)
    {
        if(0 == strcmp(unitNamePtr,unitInfoPtr->nameStr))
        {
            return unitInfoPtr->base_addr;
        }
    }

    skernelFatalError("findBaseAddrOfUnit : not found unit named [%s]",
        unitNamePtr);

    return 0;
}

static void addDuplicatedUnitIfNotExists
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_CHAR*         origUnitNamePtr,
    IN GT_CHAR*         dupUnitNamePtr
)
{
    GT_U32  ii,jj;
    SMEM_UNIT_DUPLICATION_INFO_STC   *unitInfoPtr_1 = &BOBCAT3_duplicatedUnits[0];
    SMEM_UNIT_DUPLICATION_INFO_STC   *unitInfoPtr_2;
    GT_U32  found_1,found_2;


    for(ii = 0 ; unitInfoPtr_1->unitNameStr != NULL; ii++)
    {
        if(0 == strcmp(origUnitNamePtr,unitInfoPtr_1->unitNameStr))
        {
            found_1 = 1;
        }
        else
        {
            found_1 = 0;
        }

        found_2 = 0;

        unitInfoPtr_2 = unitInfoPtr_1+1;
        for(jj = 0 ; jj < unitInfoPtr_1->numOfUnits ; jj++,unitInfoPtr_2++)
        {
            if(found_1 == 0)
            {
                if(0 == strcmp(origUnitNamePtr,unitInfoPtr_2->unitNameStr))
                {
                    found_1 = 1;
                }
            }

            if(found_2 == 0)
            {
                if(0 == strcmp(dupUnitNamePtr,unitInfoPtr_2->unitNameStr))
                {
                    found_2 = 1;
                }
            }

            if(found_1 && found_2)
            {
                return;
            }
        }

        unitInfoPtr_1 = unitInfoPtr_2;
        ii += jj;
    }

    /* we need to add 2 entries to the DB (and 'NULL' entry) */
    if((ii+2+1) >= (sizeof(BOBCAT3_duplicatedUnits)/sizeof(BOBCAT3_duplicatedUnits[0])))
    {
        skernelFatalError("addDuplicatedUnitIfNotExists : array BOBCAT3_duplicatedUnits[] not large enough \n");
        return;
    }

    BOBCAT3_duplicatedUnits[ii].unitNameStr = origUnitNamePtr;
    BOBCAT3_duplicatedUnits[ii].numOfUnits = 1;

    BOBCAT3_duplicatedUnits[ii+1].unitNameStr = dupUnitNamePtr;
    BOBCAT3_duplicatedUnits[ii+1].numOfUnits = 0;

    BOBCAT3_duplicatedUnits[ii+2].unitNameStr = NULL;
    BOBCAT3_duplicatedUnits[ii+2].numOfUnits = 0;

    return ;
}


static void buildDevUnitAddr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    /* build
        bobcat3UsedUnitsAddressesArray - the addresses of the units that the bobk uses
        bobcat3UnitNameAndIndexArr - name of unit and index in bobcat3UsedUnitsAddressesArray */
    GT_U32  ii,jj;
    GT_U32  index;
    GT_U32  size;
    UNIT_INFO_STC   *unitInfoPtr = &bobcat3units[0];

    index = 0;
    for(ii = 0 ; unitInfoPtr->size != SMAIN_NOT_VALID_CNS; ii++,unitInfoPtr++)
    {
        if(unitInfoPtr->orig_nameStr == SHARED_BETWEEN_PIPE_0_AND_PIPE_1_INDICATION_CNS)
        {
            /* continue as usual */
        }
        else
        if(unitInfoPtr->orig_nameStr)
        {
            if(unitInfoPtr->base_addr == PIPE_1_INDICATION_CNS)
            {
                /* need to get address from 'orig_nameStr'
                    and add to it the 'pipe 1' offset
                */
                GT_U32 baseAddr = findBaseAddrOfUnit(devObjPtr,bobcat3units,unitInfoPtr->orig_nameStr);

                /* update the DB */
                unitInfoPtr->base_addr = baseAddr + SECOND_PIPE_OFFSET_CNS;

                /* check if duplicated unit already in BOBCAT3_duplicatedUnits[] */
            }

            addDuplicatedUnitIfNotExists(devObjPtr,unitInfoPtr->orig_nameStr,unitInfoPtr->nameStr);
        }

        /* the size is in '1<<24' but the arrays are units of '1<<23' so we do
            lopp on : *2 */
        size = unitInfoPtr->size*2;
        for(jj = 0 ; jj < size ; jj++ , index++)
        {
            if(index >= (sizeof(bobcat3UsedUnitsAddressesArray) / sizeof(bobcat3UsedUnitsAddressesArray[0])))
            {
                skernelFatalError("buildDevUnitAddr : over flow of units (1) \n");
            }

            if(index >= (sizeof(bobcat3UnitNameAndIndexArr) / sizeof(bobcat3UnitNameAndIndexArr[0])))
            {
                skernelFatalError("buildDevUnitAddr : over flow of units (2) \n");
            }

            bobcat3UsedUnitsAddressesArray[index] = unitInfoPtr->base_addr ;
            bobcat3UnitNameAndIndexArr[index].unitNameIndex = index;
            bobcat3UnitNameAndIndexArr[index].unitNameStr = unitInfoPtr->nameStr;
        }
    }

    if(index >= (sizeof(bobcat3UnitNameAndIndexArr) / sizeof(bobcat3UnitNameAndIndexArr[0])))
    {
        skernelFatalError("buildDevUnitAddr : over flow of units (3) \n");
    }
    /* indication of no more */
    bobcat3UnitNameAndIndexArr[index].unitNameIndex = SMAIN_NOT_VALID_CNS;
    bobcat3UnitNameAndIndexArr[index].unitNameStr = NULL;

    devObjPtr->devMemUnitNameAndIndexPtr = bobcat3UnitNameAndIndexArr;
    devObjPtr->genericUsedUnitsAddressesArray = bobcat3UsedUnitsAddressesArray;
    devObjPtr->genericNumUnitsAddresses = index+1;
}

/*******************************************************************************
*   smemBobcat3UnitPolicerUnify
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  any of the 3 Policers:
*                   1. iplr 0
*                   2. iplr 1
*                   3. eplr
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr - pointer to the unit chunk
*       plrUnit - PLR unit
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitPolicerUnify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr,
    IN SMEM_SIP5_PP_PLR_UNIT_ENT   plrUnit
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000054)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000060, 0x00000068)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070, 0x00000070)}
            /*registers -- not table/memory !! -- Policer Table Access Data<%n> */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000074 ,8*4),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerTblAccessData)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000B0, 0x000000B8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000C0, 0x000001BC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x0000020C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000220, 0x00000264)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000270, 0x000002EC)}
            /*Policer Timer Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000300, 36), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerTimer)}
            /*Policer Descriptor Sample Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000400, 96), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4)}
            /*Policer Management Counters Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000500, 192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(74,16),SMEM_BIND_TABLE_MAC(policerManagementCounters)}
            /*IPFIX wrap around alert Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000800, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerIpfixWaAlert)}
            /*IPFIX aging alert Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00001000, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerIpfixAgingAlert)}
            /*registers (not memory) : Port%p and Packet Type Translation Table*/
            /*registers -- not table/memory !! -- Port%p and Packet Type Translation Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001800 , 0x00002008), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32,4),SMEM_BIND_TABLE_MAC(policerMeterPointer)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002010, 0x00002014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002100, 0x000026FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003500, 0x00003504)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003510, 0x00003514)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003520, 0x00003524)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003550, 0x00003554)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003560, 0x00003564)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003604, 0x0000360C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003704, 0x0000370C)}
            /*e Attributes Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 262144 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(36,8),SMEM_BIND_TABLE_MAC(policerEPortEVlanTrigger)}
            /*Ingress Policer Re-Marking Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00080000, 8192 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(51,8),SMEM_BIND_TABLE_MAC(policerQos)}
            /*Hierarchical Policing Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00090000, 65536 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(18,4), SMEM_BIND_TABLE_MAC(policerHierarchicalQos)}
            /*Metering Token Bucket Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00100000, 524288 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(211,32),SMEM_BIND_TABLE_MAC(policer)}
            /*Counting Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00200000, 524288 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(229,32),SMEM_BIND_TABLE_MAC(policerCounters)}
            /*Metering Configuration Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00300000, 131072 ) , SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(53,8),SMEM_BIND_TABLE_MAC(policerConfig)}
            /*Metering Conformance Level Sign Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00400000, 65536 ) , SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(2,4),SMEM_BIND_TABLE_MAC(policerConformanceLevelSign)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemBobkPolicerTablesSupport(devObjPtr,numOfChunks,chunksMem,plrUnit);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

static void smemBobcat3UnitIplr0
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

/*policer*/
    /* add common lines of all policers */
    ACTIVE_MEM_POLICER_COMMON_MAC(SMEM_SIP5_PP_PLR_UNIT_IPLR_0_E),

    {0x0000002C, SMEM_FULL_MASK_CNS, NULL, 0 , smemLion3ActiveWriteHierarchicalPolicerControl,  0},

    /* iplr0 policer table */
    {0x00100000, 0xFFF80000, smemXCatActiveReadIplr0Tables, 0 , smemXCatActiveWriteIplr0Tables,  0},
    /* iplr0 policerCounters table */
    {0x00200000, 0xFFF80000, smemXCatActiveReadIplr0Tables, 0 , smemXCatActiveWriteIplr0Tables,  0},
    /* iplr0 policer config table */
    {0x00300000, 0xFFFE0000, smemXCatActiveReadIplr0Tables, 0 , smemXCatActiveWriteIplr0Tables,  0},
    /* iplr0 Metering Conformance Level Sign Memory */
    {0x00400000, 0xFFFE0000, smemXCatActiveReadIplr0Tables, 0 , smemXCatActiveWriteIplr0Tables,  0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    smemBobcat3UnitPolicerUnify(devObjPtr,unitPtr,SMEM_SIP5_PP_PLR_UNIT_IPLR_0_E);
}

static void smemBobcat3UnitIplr1
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

/*policer */
    /* add common lines of all policers */
    ACTIVE_MEM_POLICER_COMMON_MAC(SMEM_SIP5_PP_PLR_UNIT_IPLR_1_E),

    /* iplr1 policer table */
    {0x00100000, 0xFFF80000, smemXCatActiveReadIplr1Tables, 0 , smemXCatActiveWriteIplr1Tables,  0},
    /* iplr1 policerCounters table */
    {0x00200000, 0xFFF80000, smemXCatActiveReadIplr1Tables, 0 , smemXCatActiveWriteIplr1Tables,  0},
    /* iplr1 policer config table */
    {0x00300000, 0xFFFE0000, smemXCatActiveReadIplr1Tables, 0 , smemXCatActiveWriteIplr1Tables,  0},
    /* iplr1 Metering Conformance Level Sign Memory */
    {0x00400000, 0xFFFE0000, smemXCatActiveReadIplr1Tables, 0 , smemXCatActiveWriteIplr1Tables,  0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    smemBobcat3UnitPolicerUnify(devObjPtr,unitPtr,SMEM_SIP5_PP_PLR_UNIT_IPLR_1_E);
}

static void smemBobcat3UnitEplr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

/*policer*/
    /* add common lines of all policers */
    ACTIVE_MEM_POLICER_COMMON_MAC(SMEM_SIP5_PP_PLR_UNIT_EPLR_E),

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    smemBobcat3UnitPolicerUnify(devObjPtr,unitPtr,SMEM_SIP5_PP_PLR_UNIT_EPLR_E);
}

/*******************************************************************************
*   smemBobcat3UnitMg
*
* DESCRIPTION:
*       Allocate address type specific memories
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitMg
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    ACTIVE_MEM_MG_COMMON_MAC ,

    /* XSMI Management Register */
    {0x00030000, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteXSmii, 0},
    /* XSMI1 Management Register */
    {0x00032000, SMEM_FULL_MASK_CNS, NULL, 0 , smemChtActiveWriteXSmii, 0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000008, 0x00000008)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000010, 0x00000044)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000004C, 0x00000068)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070, 0x00000084)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000090, 0x00000148)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000150, 0x0000017C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000190, 0x00000198)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000001A0, 0x00000288)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000290, 0x00000298)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002A0, 0x000002A8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002B0, 0x000002B8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002C0, 0x000002C8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002D0, 0x00000324)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000330, 0x00000330)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000338, 0x00000338)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000340, 0x00000344)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000360, 24)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000380, 0x00000388)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000390, 0x00000390)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003A0, 0x000003C0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003F0, 0x000003FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000500, 0x00000540)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x00000604)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000610, 0x00000654)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000670, 0x000006B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000830, 0x00000830)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000b64, 0x00000b64)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000b78, 0x00000b7c)}
            /* sdma */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002600, 0x00002684)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000026C0, 0x000026DC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002700, 0x00002708)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002710, 0x00002718)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002720, 0x00002728)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002730, 0x00002738)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002740, 0x00002748)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002750, 0x00002758)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002760, 0x00002768)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002770, 0x00002778)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002780, 0x00002780)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002800, 0x00002800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000280C, 0x00002868)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002870, 0x000028F4)}
            /*xsmi*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00030000, 8192)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00032000, 8192)}
            /* conf processor */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 131072)}
            /*TWSI*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080000, 0x0008000C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0008001C, 0x0008001C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00091000, 0x00091010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0009101C, 0x0009101C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000910C0, 0x000910E0)}

        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobcat3UnitMppm
*
* DESCRIPTION:
*       Allocate address type specific memories
*
* INPUTS:
*       devObjPtr   - pointer to common device memory object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitMppm
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {   /*bobcat2*/
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000120, 0x00000124)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000150, 0x00000154)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000001F0, 0x000001F0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000378)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000400, 0x00000400)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000500, 0x00000500)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000510, 0x00000518)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x00000600)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000610, 0x00000614)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000620, 0x00000624)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000710, 0x00000714)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x00000804)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000810, 0x00000814)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000900, 0x00000904)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000910, 0x00000914)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A00, 0x00000A04)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B00, 0x00000B04)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000C00, 0x00000C04)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000D00, 0x00000D04)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000E00, 0x00000E04)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000E30, 0x00000E34)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001000, 0x00001004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001050, 0x00001054)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001100, 0x00001104)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001150, 0x00001154)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001200, 0x00001204)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002000, 0x0000200C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000202C, 0x00002030)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002080, 0x00002084)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000020B0, 0x000020B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002120, 0x00002124)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002170, 0x00002174)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000021B0, 0x000021B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000021C0, 0x000021C4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003000, 0x00003004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003030, 0x00003034)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003200, 0x00003204)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003230, 0x00003234)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003400, 0x00003408)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003420, 0x00003420)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003480, 0x00003480)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000034E0, 0x000034E0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003540, 0x00003540)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003580, 0x00003580)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000035E0, 0x000035E0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003640, 0x00003640)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004000, 0x00004000)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobcat3UnitRxDma
*
* DESCRIPTION:
*       Allocate address type specific memories
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitRxDma
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000040)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000084, 0x00000088)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000A0, 0x000000A4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000B0, 0x000000B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000C4, 0x000000D0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000E4, 0x00000160)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000174, 0x00000180)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000198, 0x00000294)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002F0, 0x000002FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000320, 0x0000032C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000350, 0x00000350)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000374, 0x00000374)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000398, 0x000003AC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003C8, 0x000003D4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003E8, 0x000003F4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000408, 0x00000414)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000428, 0x00000434)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000043C, 0x00000448)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000450, 0x0000045C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000004D0, 0x000005E0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000630, 0x0000063C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000710, 0x00000714)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000720, 0x00000724)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000730, 0x00000734)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000740, 0x00000740)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000750, 0x00000750)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000804, 0x00000924)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000950, 0x00000A70)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B00, 0x00000C20)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000E00, 0x00000F28)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000120C, 0x0000132C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001528, 0x0000154C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001704, 0x0000170C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001720, 0x00001744)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001790, 0x00001790)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001800, 0x00001800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001820, 0x00001820)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001830, 0x00001834)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001840, 0x00001840)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001850, 0x0000185C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001864, 0x00001868)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001870, 0x00001870)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001B00, 0x00001C20)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001E00, 0x00001F20)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000020F0, 0x000020F0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002100, 0x00002220)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002400, 0x00002520)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002A00, 0x00002B58)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002B60, 0x00002B60)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003500, 0x00003620)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003A00, 0x00003B20)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004080, 0x00004084)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000040A0, 0x000040A4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000040C0, 0x000040C0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005000, 0x00005000)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    {
        static SMEM_REGISTER_DEFAULT_VALUE_STC myUnit_registersDefaultValueArr[] =
        {
             {DUMMY_NAME_PTR_CNS,         0x00000000,         0x3fff3fff,      8,    0x8      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000040,         0x00003fff,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000000b0,         0x3fffffff,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000000b4,         0x00000001,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000350,         0x00000800,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000374,         0x00008600,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000398,         0x00008100,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x0000039c,         0x00008a88,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000003a8,         0x00008847,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000003ac,         0x00008848,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000005d0,         0xffff0000,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000804,         0x0ffff000,     73,    0x4      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000954,         0x00000001,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000958,         0x00000002,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x0000095c,         0x00000003,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000960,         0x00000004,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000964,         0x00000005,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000968,         0x00000006,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x0000096c,         0x00000007,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000970,         0x00000008,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000974,         0x00000009,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000978,         0x0000000a,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x0000097c,         0x0000000b,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000980,         0x0000000c,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000984,         0x0000000d,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000988,         0x0000000e,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x0000098c,         0x0000000f,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000990,         0x00000010,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000994,         0x00000011,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000998,         0x00000012,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x0000099c,         0x00000013,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009a0,         0x00000014,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009a4,         0x00000015,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009a8,         0x00000016,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009ac,         0x00000017,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009b0,         0x00000018,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009b4,         0x00000019,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009b8,         0x0000001a,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009bc,         0x0000001b,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009c0,         0x0000001c,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009c4,         0x0000001d,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009c8,         0x0000001e,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009cc,         0x0000001f,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009d0,         0x00000020,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009d4,         0x00000021,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009d8,         0x00000022,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009dc,         0x00000023,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009e0,         0x00000024,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009e4,         0x00000025,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009e8,         0x00000026,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009ec,         0x00000027,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009f0,         0x00000028,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009f4,         0x00000029,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009f8,         0x0000002a,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000009fc,         0x0000002b,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a00,         0x0000002c,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a04,         0x0000002d,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a08,         0x0000002e,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a0c,         0x0000002f,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a10,         0x00000030,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a14,         0x00000031,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a18,         0x00000032,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a1c,         0x00000033,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a20,         0x00000034,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a24,         0x00000035,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a28,         0x00000036,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a2c,         0x00000037,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a30,         0x00000038,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a34,         0x00000039,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a38,         0x0000003a,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a3c,         0x0000003b,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a40,         0x0000003c,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a44,         0x0000003d,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a48,         0x0000003e,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a4c,         0x0000003f,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a50,         0x00000040,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a54,         0x00000041,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a58,         0x00000042,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a5c,         0x00000043,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a60,         0x00000044,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a64,         0x00000045,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a68,         0x00000046,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a6c,         0x00000047,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000a70,         0x00000048,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00000b00,         0x00000010,     73,    0x4      }
/*            ,{DUMMY_NAME_PTR_CNS,         0x00001700,         0x00008021,      1,    0x0      }*/
            ,{DUMMY_NAME_PTR_CNS,         0x00001704,         0x00080001,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00001840,         0x00000080,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00001850,         0xaaaaaaaa,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00001858,         0x0000001f,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00001864,         0x00003fff,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00001868,         0x00000002,      1,    0x0      }
/*            ,{DUMMY_NAME_PTR_CNS,         0x000019b0,         0x0601e114,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x000019b4,         0x003896cb,      1,    0x0      }*/
            ,{DUMMY_NAME_PTR_CNS,         0x00001b00,         0x00000015,     73,    0x4      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a00,         0x00004049,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a04,         0x00000004,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a08,         0x83828180,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a0c,         0x87868584,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a10,         0x8b8a8988,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a14,         0x8f8e8d8c,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a18,         0x93929190,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a1c,         0x97969594,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a20,         0x9b9a9998,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a24,         0x9f9e9d9c,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a28,         0xa3a2a1a0,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a2c,         0xa7a6a5a4,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a30,         0xabaaa9a8,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a34,         0xafaeadac,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a38,         0xb3b2b1b0,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a3c,         0xb7b6b5b4,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a40,         0xbbbab9b8,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a44,         0xbfbebdbc,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a48,         0xc3c2c1c0,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a4c,         0xc7c6c5c4,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a50,         0x828180c8,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a54,         0x86858483,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a58,         0x8a898887,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a5c,         0x8e8d8c8b,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a60,         0x9291908f,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a64,         0x96959493,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a68,         0x9a999897,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a6c,         0x9e9d9c9b,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a70,         0xa2a1a09f,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a74,         0xa6a5a4a3,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a78,         0xaaa9a8a7,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a7c,         0xaeadacab,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a80,         0xb2b1b0af,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a84,         0xb6b5b4b3,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a88,         0xbab9b8b7,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a8c,         0xbebdbcbb,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a90,         0xc2c1c0bf,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a94,         0xc6c5c4c3,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a98,         0x8180c8c7,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002a9c,         0x85848382,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002aa0,         0x89888786,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002aa4,         0x8d8c8b8a,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002aa8,         0x91908f8e,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002aac,         0x95949392,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002ab0,         0x99989796,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002ab4,         0x9d9c9b9a,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002ab8,         0xa1a09f9e,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002abc,         0xa5a4a3a2,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002ac0,         0xa9a8a7a6,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002ac4,         0xadacabaa,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002ac8,         0xb1b0afae,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002acc,         0xb5b4b3b2,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002ad0,         0xb9b8b7b6,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002ad4,         0xbdbcbbba,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002ad8,         0xc1c0bfbe,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002adc,         0xc5c4c3c2,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002ae0,         0x80c8c7c6,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002ae4,         0x84838281,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002ae8,         0x88878685,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002aec,         0x8c8b8a89,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002af0,         0x908f8e8d,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002af4,         0x94939291,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002af8,         0x98979695,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002afc,         0x9c9b9a99,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b00,         0xa09f9e9d,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b04,         0xa4a3a2a1,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b08,         0xa8a7a6a5,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b0c,         0xacabaaa9,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b10,         0xb0afaead,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b14,         0xb4b3b2b1,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b18,         0xb8b7b6b5,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b1c,         0xbcbbbab9,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b20,         0xc0bfbebd,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b24,         0xc4c3c2c1,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b28,         0xc8c7c6c5,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b2c,         0x83828180,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b30,         0x87868584,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b34,         0x8b8a8988,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b38,         0x8f8e8d8c,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b3c,         0x93929190,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b40,         0x97969594,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b44,         0x9b9a9998,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b48,         0x9f9e9d9c,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b4c,         0xa3a2a1a0,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b50,         0xa7a6a5a4,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b54,         0xabaaa9a8,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00002b58,         0xafaeadac,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,         0x00003a00,         0x00000001,     73,    0x4      }
            ,{DUMMY_NAME_PTR_CNS,         0x000040c0,         0x00000001,      1,    0x0      }

            ,{NULL,            0,         0x00000000,      0,    0x0      }
        };
        static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC list = {myUnit_registersDefaultValueArr,NULL};
        unitPtr->unitDefaultRegistersPtr = &list;
    }
}

/*******************************************************************************
*   smemBobcat3UnitRxDmaGlue
*
* DESCRIPTION:
*       Allocate address type specific memories
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitRxDmaGlue
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000098)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000160, 0x00000160)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobcat3UnitTxDma
*
* DESCRIPTION:
*       Allocate address type specific memories
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitTxDma
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000020, 0x0000011C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000248)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000500, 0x00000504)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001000, 0x00001014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001020, 0x00001024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002000, 0x00002044)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003000, 0x0000393C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004000, 0x00004158)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004160, 0x00004160)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005000, 0x00005014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000501C, 0x00005078)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005080, 0x0000508C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005100, 0x00005104)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005120, 0x00005124)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005200, 0x00005324)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005500, 0x00005624)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005700, 0x00005824)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005900, 0x00005A24)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006100, 0x00006104)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006300, 0x00006304)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007000, 0x00007004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007100, 0x00007224)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007300, 0x00007300)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007500, 0x00007504)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007700, 0x00007700)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }
}

/*******************************************************************************
*   smemBobcat3UnitTxFifo
*
* DESCRIPTION:
*       Allocate address type specific memories
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitTxFifo
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x0000012C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000400, 0x00000434)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000440, 0x00000444)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000508, 0x0000050C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000518, 0x0000051C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x00000724)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x00000958)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000960, 0x00000960)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001000, 0x00001024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001100, 0x00001224)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001300, 0x00001424)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001500, 0x00001624)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001700, 0x00001824)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001900, 0x00001A24)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001B00, 0x00001C24)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003000, 0x00003004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003100, 0x00003224)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003300, 0x00003300)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 1648)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00012000, 8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00014000, 0x00014000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00015000, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00015200, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00015400, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00015600, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00015800, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00015A00, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00017000, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00017200, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00017400, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00017600, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00017800, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00017A00, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00020000, 2256)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00022000, 4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00024000, 0x00024000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00025000, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00025200, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00025400, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00025600, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00025800, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00025A00, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00027000, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00027200, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00027400, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00027600, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00027800, 296)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00027A00, 296)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }
}

/*******************************************************************************
*   smemBobcat3UnitTai
*
* DESCRIPTION:
*       Allocate address type specific memories
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitTai
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000100)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobcat3UnitBm
*
* DESCRIPTION:
*       Allocate address type specific memories
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitBm
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000000, 0x00000004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000010, 0x00000018)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000020, 0x00000020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000030, 0x00000034)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000050, 0x00000060)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000200, 0x00000228)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000300, 0x00000314)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000400, 0x0000040C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000420, 0x0000042C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000440, 0x0000044C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000460, 0x0000046C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000480, 0x0000048C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000004A0, 0x000004A4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000004C0, 0x000004C8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000004D0, 0x000004D8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000004E0, 0x000004E4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000500, 0x0000074C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00100000, 65536)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00120000, 65536)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00200000, 65536)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00220000, 65536)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00300000, 65536)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }


    {
        static SMEM_REGISTER_DEFAULT_VALUE_STC myUnit_registersDefaultValueArr[] =
        {
             {DUMMY_NAME_PTR_CNS,             0x00000000,         0x3fff0000,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,             0x00000004,         0x80000000,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,             0x00000010,         0xffffffff,      2,    0x4    }
            ,{DUMMY_NAME_PTR_CNS,             0x00000018,         0x0000ffff,      2,    0x8    }
            ,{DUMMY_NAME_PTR_CNS,             0x00000020,         0x0000ffff,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,             0x00000400,         0x00000009,      4,    0x4    }
            ,{DUMMY_NAME_PTR_CNS,             0x000004c0,         0x00fc0fc0,      2,    0x10   }

            ,{DUMMY_NAME_PTR_CNS,             0x00000400,         0x00000008,      4,    0x4      }
            ,{DUMMY_NAME_PTR_CNS,             0x000004c0,         0x0000f0f0,      2,    0x10     }

            ,{NULL,            0,         0x00000000,      0,    0x0      }
        };
        static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC list = {myUnit_registersDefaultValueArr,NULL};
        unitPtr->unitDefaultRegistersPtr = &list;
    }
}

/*******************************************************************************
*   smemBobcat3UnitBma
*
* DESCRIPTION:
*       Allocate address type specific memories
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitBma
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000000, 196608)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00059000, 0x0005900C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00059014, 0x00059018)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00059020, 0x00059054)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00059060, 0x00059074)}
            /*Virtual => Physical source port mapping*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0005A000, 2048),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(7 , 4),SMEM_BIND_TABLE_MAC(bmaPortMapping)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobcat3UnitGop
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the GOP unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitGop
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    #define BC3_NUM_GOP_PORTS_GIG   37
    #define BC3_NUM_GOP_PORTS_XLG   36
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* Gig Ports*/
                /* ports 0..36 */
                 {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000000, 0x00000094)} , FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_GIG , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000000A0, 0x000000A4)} , FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_GIG , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000000C0, 0x000000E0)} , FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_GIG , 0x1000)}

            /* XLG */
                /* ports 0..36 */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000c0000 , 0x000c0024 )}, FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_XLG , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000c002C , 0x000C0030 )}, FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_XLG , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000C0038 , 0x000C0060 )}, FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_XLG , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000C0068 , 0x000C0088 )}, FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_XLG , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000C0090 , 0x000C0098 )}, FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_XLG , 0x1000)}

            /* MPCS */
                /* ports 0..36 */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180008 , 0x00180014 )}, FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_XLG  , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180030 , 0x00180030 )}, FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_XLG  , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0018003C , 0x001800C8 )}, FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_XLG  , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x001800D0 , 0x00180120 )}, FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_XLG  , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180128 , 0x0018014C )}, FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_XLG  , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0018015C , 0x0018017C )}, FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_XLG  , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00180200 , 256)}       , FORMULA_SINGLE_PARAMETER(8  , 0x1000)}

            /* XPCS IP */
                /* ports 0..36 in steps of 2*/
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180400, 0x00180424)}, FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_XLG/2  , 0x1000*2)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0018042C, 0x0018044C)}, FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_XLG/2  , 0x1000*2)}
                /* 6 lanes */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00180450, 0x0044)}, FORMULA_TWO_PARAMETERS(6 , 0x44  , BC3_NUM_GOP_PORTS_XLG/2  , 0x1000*2)                          }

            /*FCA*/
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180600, 0x001806A0)}, FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_GIG  , 0x1000)}
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180704, 0x00180714)}, FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_GIG  , 0x1000)}
            /*PTP*/
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180800, 0x0018087C)}, FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_GIG  , 0x1000)}

            /* Mac-TG Generator */
                ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00180C00 ,0x00180CCC)},  FORMULA_SINGLE_PARAMETER(BC3_NUM_GOP_PORTS_GIG  , 0x1000)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }

    {
        static SMEM_REGISTER_DEFAULT_VALUE_STC myUnit_registersDefaultValueArr[] =
        {
             /*{DUMMY_NAME_PTR_CNS,            0x000a1000,         0x0000ffff,      1,    0x0         }*/
            /* PTP */
             {DUMMY_NAME_PTR_CNS,           0x00180808,         0x00000001,      BC3_NUM_GOP_PORTS_GIG,    0x1000 }
            ,{DUMMY_NAME_PTR_CNS,           0x00180870,         0x000083aa,      BC3_NUM_GOP_PORTS_GIG,    0x1000 }
            ,{DUMMY_NAME_PTR_CNS,           0x00180874,         0x00007e5d,      BC3_NUM_GOP_PORTS_GIG,    0x1000 }
            ,{DUMMY_NAME_PTR_CNS,           0x00180878,         0x00000040,      BC3_NUM_GOP_PORTS_GIG,    0x1000 }

        /*Giga*/
            ,{DUMMY_NAME_PTR_CNS,            0x00000000,         0x00008be5,     BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000004,         0x00000003,     BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000008,         0x0000c048,     BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x0000000c,         0x0000bae8,     BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x0000001c,         0x00000052,     BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x0000002c,         0x0000000c,      2,    0x18,     BC3_NUM_GOP_PORTS_GIG,    0x1000     }
            ,{DUMMY_NAME_PTR_CNS,            0x00000030,         0x0000c815,     BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000048,         0x00000300,     BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000094,         0x00000001,     BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x000000c0,         0x00001004,     BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x000000c4,         0x00000100,     BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x000000c8,         0x000001fd,     BC3_NUM_GOP_PORTS_GIG,    0x1000                      }

            ,{DUMMY_NAME_PTR_CNS,            0x00000090,         0x0000ff9a,      1,    0x0                         }
            ,{DUMMY_NAME_PTR_CNS,            0x00000018,         0x00004b4d,     BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000014,         0x000008c4,     BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x000000d4,         0x000000ff,     BC3_NUM_GOP_PORTS_GIG,    0x1000                      }

        /*XLG*/
            ,{DUMMY_NAME_PTR_CNS,            0x000c0018,         0x00004b4d,     BC3_NUM_GOP_PORTS_XLG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x000c0014,         0x000008c4,     BC3_NUM_GOP_PORTS_XLG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x000c0030,         0x000007ec,     BC3_NUM_GOP_PORTS_XLG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x000c0080,         0x00001000,     BC3_NUM_GOP_PORTS_XLG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x000c0084,         0x00001210,     BC3_NUM_GOP_PORTS_XLG,    0x1000                      }
        /*FCA*/
            ,{DUMMY_NAME_PTR_CNS,            0x00180000+0x600,   0x00000011,      BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x00180004+0x600,   0x00002003,      BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x00180054+0x600,   0x00000001,      BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x00180058+0x600,   0x0000c200,      BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x0018005c+0x600,   0x00000180,      BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x0018006c+0x600,   0x00008808,      BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x00180070+0x600,   0x00000001,      BC3_NUM_GOP_PORTS_GIG,    0x1000                      }
            ,{DUMMY_NAME_PTR_CNS,            0x00180104+0x600,   0x0000ff00,      BC3_NUM_GOP_PORTS_GIG,    0x1000                      }

            ,{NULL,            0,         0x00000000,      0,    0x0      }
        };
        static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC list = {myUnit_registersDefaultValueArr,NULL};
        unitPtr->unitDefaultRegistersPtr = &list;
    }

}

/*******************************************************************************
*   smemBobcat3UnitLpSerdes
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  LpSerdes unit
*
* INPUTS:
*       devObjPtr   - pointer to common device memory object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitLpSerdes
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    SIM_TBD_BOOKMARK /* currently support the same memory space of bc2 ..
        until HWS will support the AVEGO serdes memory space for the 24 SERDESEs */

    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

/*SERDES*/
    /* took registers of offset 0x200 of Lion2 and convert to offset 0x800 */
    /* COMPHY_H %t Registers/KVCO Calibration Control (0x00000800 + t*0x1000: where t (0-35) represents SERDES) */
    {0x00000808, 0xFFF00FFF, NULL, 0, smemLion2ActiveWriteKVCOCalibrationControlReg, 0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* bobcat 2  */
            /* serdes external registers */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x0000000C)} , FORMULA_SINGLE_PARAMETER(36 , 0x1000)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000014, 0x00000038)} , FORMULA_SINGLE_PARAMETER(36 , 0x1000)}
            /* serdes Internal registers File*/
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x00000FFC)} , FORMULA_SINGLE_PARAMETER(36 , 0x1000)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }
    {
        static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC list = {bobcat2Serdes_registersDefaultValueArr,NULL};
        unitPtr->unitDefaultRegistersPtr = &list;
    }
}
/*******************************************************************************
*   smemBobcat3UnitXGPortMib
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  XGPortMib unit
*
* INPUTS:
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitXGPortMib
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    /* XG port MAC MIB Counters */
    {0x00000000, SMEM_BOBCAT2_XG_MIB_COUNT_MSK_CNS, smemLion2ActiveReadMsmMibCounters, 0, NULL,0},

    END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

    /* steps between each port */
    devObjPtr->xgCountersStepPerPort   = BOBCAT2_MIB_OFFSET_CNS;
    devObjPtr->xgCountersStepPerPort_1 = 0;/* not valid */
    /* offset of table xgPortMibCounters_1 */
    devObjPtr->offsetToXgCounters_1 = 0;/* not valid */
    devObjPtr->startPortNumInXgCounters_1 = 0;/* not valid */

    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* base of 0x00000000 */
            /* ports 0..36 */
              {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000000 ,0)}, FORMULA_TWO_PARAMETERS(128/4 , 0x4 , BC3_NUM_GOP_PORTS_GIG , BOBCAT2_MIB_OFFSET_CNS)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, unitPtr);
    }
}

/*******************************************************************************
*   smemBobcat3UnitTxqDq
*
* DESCRIPTION:
*       Allocate address type specific memories -- TXQ DQ
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitTxqDq
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000120)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000208, 0x00000248)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000260, 0x00000268)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000002C0, 0x000003DC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000400, 0x0000051C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x00000604)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000610, 0x00000650)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000660, 0x00000660)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000670, 0x00000670)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000680, 0x00000680)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000690, 0x00000690)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x00000800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000808, 0x00000808)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000900, 0x00000904)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000920, 0x00000924)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A00, 0x00000A00)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B00, 0x00000C24)}
            /* JIRA : TXQ-1265 - registers addresses % 4 are not 0 (1,2,3) - TXQ_IP_dq/Global/Credit Counters/TxDMA Port [n] Baseline. */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000D00, 0x00000D00 + (74*4))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001000, 0x00001000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001008, 0x00001010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001040, 0x0000115C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001240, 0x0000133C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001380, 0x0000149C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001500, 0x00001500)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001510, 0x0000162C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001650, 0x000017A0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000017B0, 0x000017B0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001800, 0x00001800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001904, 0x00001A28)}
            /*Scheduler State Variable RAM*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00002000, 2304), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(168,32),SMEM_BIND_TABLE_MAC(Scheduler_State_Variable)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004000, 0x0000400C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004100, 0x0000421C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004300, 0x0000441C)}
            /*Priority Token Bucket Configuration*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00006000, 6144), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(488,64)}
            /*Port Token Bucket Configuration*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0000A000, 768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(61,8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000C000, 0x0000C000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000C008, 0x0000C044)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000C080, 0x0000C09C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000C100, 0x0000C13C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000C180, 0x0000C37C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000C500, 0x0000C8FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000CA00, 0x0000CA08)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000D000, 0x0000D004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000D010, 0x0000D014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000D040, 0x0000D15C)}
            /*Egress STC Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x0000D800, 1152), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(80,16),SMEM_BIND_TABLE_MAC(egressStc)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000E120, 0x0000E17C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000E220, 0x0000E27C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000E520, 0x0000E57C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000F120, 0x0000F17C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000F220, 0x0000F27C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000F320, 0x0000F37C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000F420, 0x0000F47C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00010120, 0x0001017C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00010220, 0x0001027C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00011000, 384)}


        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobcat3UnitTxqLl
*
* DESCRIPTION:
*       Allocate address type specific memories -- TXQ ll (link list)
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitTxqLl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000000, 18432)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00008000, 18432)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 18432)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00020000, 18432)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00028000, 18432)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00030000, 18432)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 18432)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00110000, 0x00110040)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00110444, 0x00110D40)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00111000, 0x0011103C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00112008, 0x00112030)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00112100, 0x00112144)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00113000, 0x00113000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00113100, 0x00113194)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00113200, 0x00113294)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    {
        static SMEM_REGISTER_DEFAULT_VALUE_STC myUnit_registersDefaultValueArr[] =
        {
             /*{DUMMY_NAME_PTR_CNS,            0x000a1000,         0x0000ffff,      1,    0x0         }*/

            {NULL,            0,         0x00000000,      0,    0x0      }
        };
        static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC list = {myUnit_registersDefaultValueArr,NULL};
        unitPtr->unitDefaultRegistersPtr = &list;
    }

}

/*******************************************************************************
*   smemBobcat3UnitTxqQcn
*
* DESCRIPTION:
*       Allocate address type specific memories -- TXQ Qcn
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitTxqQcn
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000008)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000040, 0x00000040)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000050, 0x00000050)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000060, 0x00000060)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000090, 0x000000AC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000D0, 0x000000D0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000E0, 0x000000E0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000F0, 0x000000F0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000100, 0x00000100)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000110, 0x00000110)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000120, 0x00000128)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000130, 0x00000130)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000140, 0x00000140)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000200)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000210, 0x00000210)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000300, 0x000004FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B00, 0x00000C5C)}
            /*CN Sample Intervals Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 36864), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(36,8),SMEM_BIND_TABLE_MAC(CN_Sample_Intervals)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

}

/*******************************************************************************
*   smemBobcat3UnitTxqQueue
*
* DESCRIPTION:
*       Allocate address type specific memories -- TXQ queue
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitTxqQueue
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    if(devObjPtr->errata.txqEgressMibCountersNotClearOnRead)
    {
        START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
            TXQ_QUEQUE_ACTIVE_MEM_MAC,
        END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
    }
    else
    {
        START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
            TXQ_QUEQUE_ACTIVE_MEM_MAC,
            TXQ_QUEQUE_EGR_PACKET_COUNTERS_CLEAR_ON_READ_ACTIVE_MEM_MAC ,
       END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
    }

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090000, 0x00090014)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090040, 0x00090054)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090060, 0x0009019C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090280, 0x0009039C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090400, 0x00090430)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090480, 0x0009059C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090680, 0x0009079C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090880, 0x000908AC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000908C0, 0x00090904)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000909A0, 0x000909E4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000909F0, 0x000909F0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090A00, 0x00090A0C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090A40, 0x00090A40)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090A80, 0x00090A80)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090A90, 0x00090A94)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00090B00, 0x00090B48)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093000, 0x00093010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093020, 0x00093020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093200, 0x00093214)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093220, 0x00093224)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093230, 0x00093234)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093240, 0x00093244)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093250, 0x00093254)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093260, 0x00093264)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093270, 0x00093274)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093280, 0x00093284)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00093290, 0x00093294)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000936A0, 0x000936A0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0000, 0x000A0044)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0050, 0x000A016C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0200, 0x000A031C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0360, 0x000A03BC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A03D0, 0x000A040C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0440, 0x000A044C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0500, 0x000A063C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0800, 0x000A0800)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0810, 0x000A0810)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0820, 0x000A085C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0900, 0x000A093C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A0A00, 0x000A0A3C)}
            /*Maximum Queue Limits*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000A1000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(34,8),SMEM_BIND_TABLE_MAC(Shared_Queue_Maximum_Queue_Limits)}
            /*Queue Limits DP0 - Enqueue*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000A1800, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(48,8),SMEM_BIND_TABLE_MAC(Queue_Limits_DP0_Enqueue)}
            /*Queue Buffer Limits - Dequeue*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000A2000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(34,8),SMEM_BIND_TABLE_MAC(Queue_Buffer_Limits_Dequeue)}
            /*Queue Descriptor Limits - Dequeue*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000A2800, 512), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14,4),SMEM_BIND_TABLE_MAC(Queue_Descriptor_Limits_Dequeue)}
            /*Queue Limits DP12 - Enqueue*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000A3000, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(48,8),SMEM_BIND_TABLE_MAC(Queue_Limits_DP12_Enqueue)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A4000, 0x000A4004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A4010, 0x000A426C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A4290, 0x000A42AC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A9000, 0x000A9004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A9010, 0x000A9010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A9020, 0x000A9020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A9030, 0x000A9030)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A9200, 0x000A9210)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A9500, 0x000A9504)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000A9510, 0x000A951C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000AA000, 0x000AA040)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000AA050, 0x000AA12C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000AA1B0, 0x000AA1CC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000AA210, 0x000AA210)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000AA230, 0x000AA230)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000AA400, 576), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(64,8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000AA800, 576), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(64,8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000B0000, 0x000B0034)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000B0040, 0x000B0074)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000B0080, 0x000B00B4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000B00C0, 0x000B00F4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000B0100, 0x000B010C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000B1120, 0x000B18FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000B1920, 0x000B20FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000B2120, 0x000B28FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000B2920, 0x000B30FC)}
            /*Q Main Buff*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000C0000, 18432), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(16,4)}
            /*Q Main MC Buff*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x000D0000, 18432), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(20,4)}

        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}


/*******************************************************************************
*   smemBobcat3UnitTxqBmx
*
* DESCRIPTION:
*       Allocate address type specific memories -- TXQ bmx
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitTxqBmx
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x0000002C)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobcat3UnitFcu
*
* DESCRIPTION:
*       Allocate address type specific memories -- FCU
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitFcu
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000068)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000100, 0x0000015C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x0000025C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00001000, 2048)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobcat3Unit__dummy
*
* DESCRIPTION:
*       Allocate address type specific memories -- dummy
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3Unit__dummy
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000000)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobcat3SpecificDeviceUnitAlloc_DP_units
*
* DESCRIPTION:
*       specific initialization units allocation that called before alloc units
*       of any device.
*       DP units -- (non sip units)
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Pointer to object for specific subunit
*
* COMMENTS:
*
*******************************************************************************/
static void smemBobcat3SpecificDeviceUnitAlloc_DP_units
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC    *currUnitChunkPtr;
    GT_U32  unitIndex;

    /* need to know some of Errata during unit allocations and active memories */
    smemBobcat2ErrataCleanUp(devObjPtr);


    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MG)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitMg(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_GOP)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitGop(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_SERDES)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitLpSerdes(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_XG_PORT_MIB)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitXGPortMib(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MPPM)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitMppm(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_RX_DMA)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitRxDma(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_DMA);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitTxDma(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TX_FIFO);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitTxFifo(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_RX_DMA_GLUE)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitRxDmaGlue(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TAI)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitTai(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_BM);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitBm(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_BMA);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitBma(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_DQ)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitTxqDq(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_LL)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitTxqLl(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_QCN)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitTxqQcn(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_QUEUE)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitTxqQueue(devObjPtr,currUnitChunkPtr);
    }

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TXQ_BMX)];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitTxqBmx(devObjPtr,currUnitChunkPtr);
    }


    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_FCU);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitFcu(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_SBC);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3Unit__dummy(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_NSEC);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3Unit__dummy(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_NSEF);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3Unit__dummy(devObjPtr,currUnitChunkPtr);
    }

}

/*******************************************************************************
*   smemBobcat3UnitEgfQag
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion3  EGF-QAG unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitEgfQag
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000000, 1048576),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(48, 8),SMEM_BIND_TABLE_MAC(egfQagEVlanDescriptorAssignmentAttributes)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00800000, 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(44, 8),SMEM_BIND_TABLE_MAC(egfQagEgressEPort)}
            /* TC_DP_Mapper_Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00900000, 16384), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(8, 4),SMEM_BIND_TABLE_MAC(egfQagTcDpMapper)}
            /* VOQ_Mapper_Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00910000, 8192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(6, 4)}
            /* Cpu_Code_To_Loopback_Mapper_Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00920000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(11, 4)}
            /* Port_Target_Attributes_Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00921000, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(17, 4),SMEM_BIND_TABLE_MAC(egfQagPortTargetAttribute)}
            /* Port_Enq_Attributes_Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00922000, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(10, 4),SMEM_BIND_TABLE_MAC(egfQagTargetPortMapper)}
            /* Port_Source_Attributes_Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00923000, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(4, 4) ,SMEM_BIND_TABLE_MAC(egfQagPortSourceAttribute)}
            /* EVIDX_Activity_Status_Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00924000, 4096), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4)}

            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00000, 0x00E00004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00010, 0x00E00018)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00020, 0x00E00020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00170, 0x00E0019C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00E00A00, 0x00E00A00)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00010, 0x00F00010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00020, 0x00F00020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00030, 0x00F00030)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00100, 0x00F00114)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00F00200, 0x00F00200)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);


    }
}
/*******************************************************************************
*   smemBobcat3UnitTcam
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  TCAM unit
*
* INPUTS:
*       commonDevMemInfoPtr - pointer to common device memory object.
*       unitPtr             - pointer to the unit chunk
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemBobcat3UnitTcam
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN SMEM_UNIT_CHUNKS_STC  *unitPtr
)
{
    SMEM_CHUNK_BASIC_STC  chunksMem[]=
    {
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000000, 1572864),
         SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(84, 16),SMEM_BIND_TABLE_MAC(tcamMemory)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00200000, 1572864),
         SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(242, 32),SMEM_BIND_TABLE_MAC(globalActionTable)},

        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00400000, 294912)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500000, 0x00500010)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500100, 0x00500100)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500108, 0x00500108)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500110, 0x00500110)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500118, 0x00500118)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500120, 0x00500120)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500128, 0x00500128)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500130, 0x00500130)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500138, 0x00500138)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500140, 0x00500140)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500148, 0x00500148)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500150, 0x00500150)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00500158, 0x00500158)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00501000, 0x00501010)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00502000, 0x005021C8)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x005021D0, 0x005021DC)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x005021E4, 0x005021F0)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x005021F8, 0x00502204)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0050220C, 0x00502218)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00502300, 0x0050230C)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00503000, 0x00503030)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0050303C, 0x0050303C)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00504010, 0x00504014)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00505000, 0x0050509C)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00506000, 0x00506058)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0050606C, 0x00506078)},
        {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00507000, 0x0050700C)}
    };

    GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
    smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, unitPtr);
}

/*******************************************************************************
*   smemBobcat3UnitTti
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  TTI unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitTti
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000068)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070, 0x00000168)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000170, 0x00000180)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000190, 0x000001BC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000001CC, 0x000001E4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000214)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000300, 0x00000320)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000340, 0x00000340)}
            /*DSCP To DSCP Map - one entry per translation profile
              Each entry holds 64 DSCP values, one per original DSCP value. */
#define  lion3_dscpToDscpMap_size  64
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000400, 768),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(384 , lion3_dscpToDscpMap_size),SMEM_BIND_TABLE_MAC(dscpToDscpMap)}
            /*EXP To QosProfile Map - Each entry represents Qos translation profile
              Each entry holds QoS profile for each of 8 EXP values*/
#define  lion3_expToQoSProfile_size  16
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000700, 192),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(80, lion3_expToQoSProfile_size),SMEM_BIND_TABLE_MAC(expToQoSProfile)}
            /*DSCP To QosProfile Map - holds 12 profiles, each defining a 10bit QoSProfile per each of the 64 DSCP values : 0x400 + 0x40*p: where p (0-11) represents profile*/
#define  lion3_dscpToQoSProfile_size  128
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000800, 1536),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(640 , lion3_dscpToQoSProfile_size),SMEM_BIND_TABLE_MAC(dscpToQoSProfile)}
            /*UP_CFI To QosProfile Map - Each entry holds a QoS profile per each value of {CFI,UP[2:0]} */
#define  lion3_upToQoSProfile_size  32
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000E00, 384),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(160, lion3_upToQoSProfile_size),SMEM_BIND_TABLE_MAC(upToQoSProfile)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000013B0, 0x000013DC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000013F8, 0x0000148C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001500, 0x00001540)}

            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003000, 0x00003004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000300C, 0x0000300C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003030, 0x00003034)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003040, 0x0000307C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003090, 0x000030AC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00006000, 0x000065FC)}

            /* Default Port Protocol eVID and QoS Configuration Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 65536), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(portProtocolVidQoSConf)}
            /* PCL User Defined Bytes Configuration Memory -- 70 udb's in each entry */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00020000, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(770, 128),SMEM_BIND_TABLE_MAC(ipclUserDefinedBytesConf)}
            /* TTI User Defined Bytes Configuration Memory -- TTI keys based on UDB's : 8 entries support 8 keys*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 1280), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(330, 64),SMEM_BIND_TABLE_MAC(ttiUserDefinedBytesConf)}
            /* VLAN Translation Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00050000, 16384), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(13, 4),SMEM_BIND_TABLE_MAC(ingressVlanTranslation)}
             /*Physical Port Attributes*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00100000, 4096), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(58, 8),SMEM_BIND_TABLE_MAC(ttiPhysicalPortAttribute)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00110000, 0x00110400)}
             /*Physical Port Attributes 2*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00160000, 16384), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(251, 32),SMEM_BIND_TABLE_MAC(ttiPhysicalPort2Attribute)}
            /*Default ePort Attributes (pre-tti lookup eport table)*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00210000, 16384), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(123, 16),SMEM_BIND_TABLE_MAC(ttiPreTtiLookupIngressEPort)}
            /*ePort Attributes (post-tti lookup eport table)*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00240000, 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(17, 4),SMEM_BIND_TABLE_MAC(ttiPostTtiLookupIngressEPort)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);

    }

    { /* sip5: chunks with formulas */
        {
            SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
            {
                /* MAC2ME Registers */ /* registers not table -- SMEM_BIND_TABLE_MAC(macToMe)*/
                 {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00001600,0)}, FORMULA_TWO_PARAMETERS(6 , 0x4 , 128, 0x20)}
            };

            GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
            SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

            smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

            /*add the tmp unit chunks to the main unit */
            smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
        }
    }

    {
        static SMEM_REGISTER_DEFAULT_VALUE_STC myUnit_registersDefaultValueArr[] =
        {
             {DUMMY_NAME_PTR_CNS,            0x00000100,         0xffffffff,      4,    0x4      }
            ,{DUMMY_NAME_PTR_CNS,            0x000000f0,         0xffffffff,      4,    0x4      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000018,         0x65586558,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000028,         0x000088e7,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000030,         0x88488847,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000034,         0x65586558,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000038,         0x00003232,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x0000014c,         0x00ffffff,      1,    0x0      }
/*            ,{DUMMY_NAME_PTR_CNS,            0x000001c8,         0x0000ffff,      1,    0x0      }*/
            ,{DUMMY_NAME_PTR_CNS,            0x00000300,         0x81008100,      4,    0x4      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000310,         0xffffffff,      4,    0x4      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000000,         0x30002503,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000140,         0x00a6c01b,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x0000000c,         0x00000020,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000068,         0x1b6db81b,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000110,         0x0000004b,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000118,         0x00001320,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x0000011c,         0x0000001b,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000084,         0x000fff00,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000088,         0x3fffffff,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x0000008c,         0x3fffffff,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000200,         0x030022f3,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000204,         0x00400000,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000208,         0x12492492,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x0000020c,         0x00092492,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000210,         0x0180c200,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00003000,         0x88f788f7,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00003004,         0x013f013f,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x0000300c,         0x00000570,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00003094,         0x88b588b5,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00003098,         0x00000001,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00001500,         0x00602492,      1,    0x0      }
/*            ,{DUMMY_NAME_PTR_CNS,            0x00001000,         0x3fffffff,     43,    0x4      }*/
            ,{DUMMY_NAME_PTR_CNS,            0x00001608,         0xffff0fff,    128,    0x20     }
            ,{DUMMY_NAME_PTR_CNS,            0x00001600,         0x00000fff,    128,    0x20     }
            ,{DUMMY_NAME_PTR_CNS,            0x0000160c,         0xffffffff,    128,    0x20     }
            ,{DUMMY_NAME_PTR_CNS,            0x00000040,         0xff000000,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000044,         0x00000001,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x0000004c,         0xff020000,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000050,         0xff000000,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000054,         0xffffffff,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000058,         0xffffffff,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x0000005c,         0xffffffff,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000070,         0x00008906,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000150,         0x0000301f,      1,    0x0      }

            ,{DUMMY_NAME_PTR_CNS,            0x0000001c,         0x0000000e,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x0000003c,         0x0000000d,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000140,         0x20A6C01B,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000144,         0x24924924,      2,    0x4      }

            ,{DUMMY_NAME_PTR_CNS,            0x00000170,         0x0fffffff,      4,    0x4      }
            ,{DUMMY_NAME_PTR_CNS,            0x000013fc,         0x000001da,      1,    0x0      }

                ,{NULL,            0,         0x00000000,      0,    0x0      }
        };
        static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC list = {myUnit_registersDefaultValueArr,NULL};
        unitPtr->unitDefaultRegistersPtr = &list;
    }
}

/*******************************************************************************
*   smemBobcat3UnitIpcl
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the IPCL unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitIpcl
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{

    {
        START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

        /* PCL Unit Interrupt Cause */
        {0x00000004, SMEM_FULL_MASK_CNS, smemChtActiveReadIntrCauseReg, 0, smemChtActiveWriteIntrCauseReg,0},
        {0x00000008, SMEM_FULL_MASK_CNS, NULL, 0, smemChtActiveGenericWriteInterruptsMaskReg, 0},

        END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
    }

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000000, 0x0000003C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000044, 0x00000048)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x0000005C, 0x0000005C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000070, 0x00000078)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000080, 0x000000BC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x000005C0, 0x000005FC),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(pearsonHash)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000730, 0x0000073C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000744, 0x0000077C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC  (0x00000800, 0x00000850)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000C00, 512),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(74, 16),SMEM_BIND_TABLE_MAC(crcHashMask)}
            /* next are set below as formula of 3 tables
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 18432)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00020000, 18432)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00030000, 18432)}*/

            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 7168),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(506, 64),SMEM_BIND_TABLE_MAC( ipcl0UdbSelect)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00042000, 7168),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(506, 64),SMEM_BIND_TABLE_MAC(ipcl1UdbSelect)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00044000, 7168),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(506, 64),SMEM_BIND_TABLE_MAC(ipcl2UdbSelect)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);

    }

    /* chunks with formulas */
    {
        GT_U32  numPhyPorts = devObjPtr->limitedResources.phyPort;

        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* IPCL0,IPCL1,IPCL2 Configuration Table */
            {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00010000 ,18432), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(27, 4),SMEM_BIND_TABLE_MAC(pclConfig)}, FORMULA_SINGLE_PARAMETER(3 , 0x00010000)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        /* number of entries : 4K + numPhyPorts . keep alignment and use for memory size */
        chunksMem[0].memChunkBasic.numOfRegisters = ((4*1024) + numPhyPorts) * (chunksMem[0].memChunkBasic.enrtyNumBytesAlignement / 4);

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }


    {
        static SMEM_REGISTER_DEFAULT_VALUE_STC myUnit_registersDefaultValueArr[] =
        {
             {DUMMY_NAME_PTR_CNS,            0x00000000,         0x00000001,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x0000000c,         0x02801000,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000018,         0x0000ffff,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x0000001c,         0x00000028,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000020,         0x00000042,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000034,         0x00000fff,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000044,         0x3fffffff,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000048,         0x0000ffff,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000730,         0x00ff0080,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00000738,         0x00080008,      1,    0x0      }

            ,{NULL,            0,         0x00000000,      0,    0x0      }
        };



        static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC list = {myUnit_registersDefaultValueArr,NULL};
        unitPtr->unitDefaultRegistersPtr = &list;
    }

}

/*******************************************************************************
*   smemLion3UnitL2i
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the Lion3  L2i unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitL2i
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000020, 0x0000002C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000040, 0x00000048)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000120, 0x00000120)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000130, 0x00000130)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000140, 0x0000014C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000160, 0x00000164)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000170, 0x00000170)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000180, 0x0000019C)}

            /*,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x0000063C)} --> see ieeeRsrvMcCpuIndex */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000700, 0x0000071C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000900, 0x0000093C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000C00, 0x00000C3C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000D00, 0x00000D1C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000E00, 0x00000E08)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001000, 0x00001018)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001100, 0x0000112C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001200, 0x0000120C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001220, 0x00001224)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001300, 0x00001308)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001400, 0x0000140C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001500, 0x00001508)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001510, 0x00001514)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001520, 0x00001528)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001530, 0x00001544)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001550, 0x00001564)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001600, 0x0000161C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002000, 0x00002000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002010, 0x00002010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002020, 0x00002020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002100, 0x00002104)}
            /* Source Trunk Attribute Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00202000 ,4096 ),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(40, 8),SMEM_BIND_TABLE_MAC(bridgeIngressTrunk)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }

    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
            /* IEEE Reserved Multicast Configuration register */ /* register and not table SMEM_BIND_TABLE_MAC(ieeeRsrvMcConfTable) */
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000200 ,0),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(ieeeRsrvMcConfTable)}, FORMULA_TWO_PARAMETERS(16 , 0x4 , 8 ,0x80)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000600 ,0),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(ieeeRsrvMcCpuIndex)}, FORMULA_SINGLE_PARAMETER(16 , 0x4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
    {
        SMEM_CHUNK_BASIC_STC  chunksMemBc2A0[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x0000081C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A00, 0x00000A1C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B00, 0x00000B1C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001240, 0x000012BC)}
        };

        SMEM_CHUNK_BASIC_STC  chunksMemBc2B0[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x00000880)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A00, 0x00000A80)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B00, 0x00000B80)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001240, 0x000012DC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002120, 0x00002120)}
        };

        if(SMEM_CHT_IS_SIP5_10_GET(devObjPtr) == 0)
        {
            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2A0));
        }
        else
        {
            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc2B0));
        }
    }

    /* to support bobk :
       add tables that depend on number of vlans and number of eport and number
       of physical ports */
    {
        GT_U32  numPhyPorts = devObjPtr->limitedResources.phyPort;
        GT_U32  numEPorts = devObjPtr->limitedResources.ePort;
        GT_U32  numEVlans = devObjPtr->limitedResources.eVid;
        GT_U32  numStg = 1 << devObjPtr->flexFieldNumBitsSupport.stgId;

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /*Ingress Bridge physical Port Table*/
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00200000 ,1024 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(29, 4),SMEM_BIND_TABLE_MAC(bridgePhysicalPortEntry)}
                /*Ingress Bridge physical Port Rate Limit Counters Table*/
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00201000 ,1024 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(22, 4),SMEM_BIND_TABLE_MAC(bridgePhysicalPortRateLimitCountersEntry)}
            };

            /* number of entries : numPhyPorts . keep alignment and use for memory size */
            chunksMem[0].numOfRegisters = numPhyPorts * (chunksMem[0].enrtyNumBytesAlignement / 4);
            /* number of entries : numPhyPorts . keep alignment and use for memory size*/
            chunksMem[1].numOfRegisters = numPhyPorts * (chunksMem[1].enrtyNumBytesAlignement / 4);

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* ingress EPort learn prio Table */
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00210000 ,8192 ),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(40, 8),SMEM_BIND_TABLE_MAC(bridgeIngressEPortLearnPrio)}
            };

            /* number of entries : numEPorts/8 . keep alignment and use for memory size */
            chunksMem[0].numOfRegisters = (numEPorts / 8) * (chunksMem[0].enrtyNumBytesAlignement / 4);

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* Ingress Spanning Tree State Table */
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00240000 , 262144 ),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(1024, 128),SMEM_BIND_TABLE_MAC(stp)}
            };
            GT_U32  numLines = numStg;

            /* each entry : 2 bits per physical port */
            chunksMem[0].enrtySizeBits = (2 * numPhyPorts);

            if(SMEM_CHT_IS_SIP5_20_GET(devObjPtr))
            {
                /* there are 2 'entries' per line */
                numLines /= 2;
            }

            /* alignment is according to entry size */
            chunksMem[0].enrtyNumBytesAlignement = ((chunksMem[0].enrtySizeBits + 31)/32)*4;
            /* number of entries : numStg . use calculated alignment and use for memory size */
            chunksMem[0].numOfRegisters = (numLines) * (chunksMem[0].enrtyNumBytesAlignement / 4);

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* Ingress Port Membership Table */
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00280000 , 262144), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(512 , 64),SMEM_BIND_TABLE_MAC(bridgeIngressPortMembership)}
            };

            GT_U32  numLines = numEVlans / 2;

            /* each entry : 1 bit per physical port */
            chunksMem[0].enrtySizeBits = numPhyPorts;
            /* alignment is according to entry size */
            chunksMem[0].enrtyNumBytesAlignement = ((chunksMem[0].enrtySizeBits + 31)/32)*4;
            /* number of entries : numEVlans . use calculated alignment and use for memory size */
            chunksMem[0].numOfRegisters = (numLines ) * (chunksMem[0].enrtyNumBytesAlignement / 4) ;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }

        {
            SMEM_CHUNK_BASIC_STC  chunksMem[]=
            {
                /* Ingress Vlan Table */
                {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x002C0000 , 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(113 , 16),SMEM_BIND_TABLE_MAC(vlan)}
            };

            /* number of entries : numEVlans . keep alignment and use for memory size */
            chunksMem[0].numOfRegisters = (numEVlans ) * (chunksMem[0].enrtyNumBytesAlignement / 4) ;

            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMem));
        }

        {
            SMEM_CHUNK_BASIC_STC  chunksMemBc3[]=
            {
                /*Bridge Ingress ePort Table*/
                 {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00300000 ,131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(80 ,16),SMEM_BIND_TABLE_MAC(bridgeIngressEPort)}
                /* Ingress Span State Group Index Table */
                ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00218000 , 32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12 , 4),SMEM_BIND_TABLE_MAC(ingressSpanStateGroupIndex)}
            };
            /* number of entries : numEPorts . keep alignment and use for memory size */
            chunksMemBc3[0].numOfRegisters = (numEPorts ) * (chunksMemBc3[0].enrtyNumBytesAlignement / 4) ;
            /* number of entries : numEVlans . keep alignment and use for memory size */
            chunksMemBc3[1].numOfRegisters = (numEVlans ) * (chunksMemBc3[1].enrtyNumBytesAlignement / 4) ;
            /* number of bits : stgId */
            chunksMemBc3[1].enrtySizeBits = devObjPtr->flexFieldNumBitsSupport.stgId;
            smemUnitChunkAddBasicChunk(devObjPtr,unitPtr,
                ARRAY_NAME_AND_NUM_ELEMENTS_MAC(chunksMemBc3));
        }

    }


}


/*******************************************************************************
*   smemBobcat3UnitHa
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the HA unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitHa
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000058)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000060, 0x00000070)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000080, 0x0000009C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000100, 0x00000144)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000150, 0x0000016C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000300, 0x00000314)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003C0, 0x000003C4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000003D0, 0x000003D0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000408, 0x00000410)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000420, 0x00000424)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000430, 0x00000434)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000500, 0x0000051C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000550, 0x00000560)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x0000063C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000710, 0x00000710)}
            /*HA Physical Port Table 1*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00001000, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(27, 4) , SMEM_BIND_TABLE_MAC(haEgressPhyPort1)}
            /*HA Physical Port Table 2*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00002000, 8192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(81, 16) , SMEM_BIND_TABLE_MAC(haEgressPhyPort2)}
            /*HA QoS Profile to EXP Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00004000 , 4096), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(3, 4) , SMEM_BIND_TABLE_MAC(haQosProfileToExp)}
            /*HA Global MAC SA Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00005000, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(48, 8) , SMEM_BIND_TABLE_MAC(haGlobalMacSa)}
            /*EVLAN Table (was 'vlan translation' in legacy device)*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 65536), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(36, 8) , SMEM_BIND_TABLE_MAC(egressVlanTranslation)}
            /*VLAN MAC SA Table (was 'VLAN/Port MAC SA Table' in legacy device)*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00020000, 16384), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(8, 4) , SMEM_BIND_TABLE_MAC(vlanPortMacSa)}
            /*PTP Domain table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 20480) , SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(35, 8) , SMEM_BIND_TABLE_MAC(haPtpDomain)}
            /*Generic TS Profile table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00050000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(537,128), SMEM_BIND_TABLE_MAC(tunnelStartGenericIpProfile) }
            /*EPCL User Defined Bytes Configuration Memory*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00060000, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(600,128) , SMEM_BIND_TABLE_MAC(haEpclUserDefinedBytesConfig)}
            /*HA Egress ePort Attribute Table 1*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00100000, 262144), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(88,16) , SMEM_BIND_TABLE_MAC(haEgressEPortAttr1)}
            /*HA Egress ePort Attribute Table 2*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00200000, 65536), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(25, 4) , SMEM_BIND_TABLE_MAC(haEgressEPortAttr2)}
            /*Router ARP DA and Tunnel Start Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00400000, 2097152), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(384,64) , SMEM_BIND_TABLE_MAC(arp)/*tunnelStart*/}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobcat3UnitEq
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  Eq unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitEq
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        START_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)

        ACTIVE_MEM_EQ_COMMON_MAC,

        END_BIND_UNIT_ACTIVE_MEM_MAC(devObjPtr,unitPtr)
    }

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000001C, 0x00000020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000003C, 0x00000040)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000058, 0x0000007C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000088, 0x0000008C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000A0, 0x000000A4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x0000060C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000110, 0x0000011C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005000, 0x00005000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005010, 0x00005018)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005020, 0x00005024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005030, 0x00005030)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005050, 0x00005054)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007000, 0x0000703C),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(tcpUdpDstPortRangeCpuCode)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007800, 0x0000783C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00007C00, 0x00007C10),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(ipProtCpuCode)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000A000, 0x0000A000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000A008, 0x0000A008)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000A010, 0x0000A010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000A020, 0x0000A034)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000A100, 0x0000A13C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000AA00, 0x0000AA08)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000AB00, 0x0000AB5C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000AC00, 0x0000AC08)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000AD00, 0x0000AD70)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000AF00, 0x0000AF08)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000AF10, 0x0000AF10)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000AF30, 0x0000AF30)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000B000, 0x0000B008)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000B200, 0x0000B218)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x0000B300, 256),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(eqPhysicalPortIngressMirrorIndexTable)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000D000, 0x0000D004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0000D010, 0x0000D010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00010000, 0x0001000C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x0002000C, 0x00020010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 8192)}/*SMEM_BIND_TABLE_MAC(ingrStc)*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00080000, 0x000803F8)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00100000, 1024),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(statisticalRateLimit)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00110000, 4096),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(14, 4),SMEM_BIND_TABLE_MAC(qosProfile)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00120000, 1024),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(cpuCode)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00130000, 1024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00160000, 16384),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(28, 4),SMEM_BIND_TABLE_MAC(eqTrunkLtt)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00700000, 16384),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(eqIngressEPort)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00800000, 65536),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(28, 4),SMEM_BIND_TABLE_MAC(eqL2EcmpLtt)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00900000, 65536),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(24, 4),SMEM_BIND_TABLE_MAC(eqL2Ecmp)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00A00000, 2048),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(txProtectionSwitchingTable)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00A80000, 32768),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(22, 4),SMEM_BIND_TABLE_MAC(ePortToLocMappingTable)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00B00000, 1024),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(oamProtectionLocStatusTable)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00B10000, 1024),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(oamTxProtectionLocStatusTable)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00C00000, 65536),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(22, 4),SMEM_BIND_TABLE_MAC(eqE2Phy)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
    /* chunks with formulas */
    {
        SMEM_CHUNK_EXTENDED_STC  chunksMem[]=
        {
             {{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000620 ,0)}, FORMULA_TWO_PARAMETERS(2,0x4, 32,0x10)}
            ,{{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00010020 ,0)}, FORMULA_TWO_PARAMETERS(2,0x4, 32,0x10)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_EXTENDED_STC);
        SMEM_UNIT_CHUNKS_STC    tmpUnitChunk;

        smemInitMemChunkExt(devObjPtr,chunksMem, numOfChunks, &tmpUnitChunk);

        /*add the tmp unit chunks to the main unit */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr,&tmpUnitChunk);
    }
}
/*******************************************************************************
*   smemBobcat3UnitEgfEft
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the EGF-EFT unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitEgfEft
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
                /* Secondary Target Port Table */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00000000, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(8, 4) , SMEM_BIND_TABLE_MAC(secondTargetPort)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001000, 0x00001000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001020, 0x00001020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001030, 0x00001030)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001040, 0x00001040)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000010A0, 0x000010A0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000010B0, 0x000010B0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001110, 0x00001110)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001130, 0x00001130)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001140, 0x00001140)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00001150, 0x0000138C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002000, 0x00002004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002100, 0x00002100)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002108, 0x00002108)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000021D0, 0x000021D0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00002200, 0x000022BC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003000, 0x0000309C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00003100, 0x000031FC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004000, 0x00004008)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00004010, 0x0000402C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00005000, 0x00005000)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00010000, 0x00010004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00010040, 0x000101BC)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00010200, 0x0001067C)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);


        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    {
        static SMEM_REGISTER_DEFAULT_VALUE_STC myUnit_registersDefaultValueArr[] =
        {
             {DUMMY_NAME_PTR_CNS,            0x00001000,         0x000007e7,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00001020,         0x08e00800 ,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00001030,         0x0000000e,      1,    0x0      }
/*            ,{DUMMY_NAME_PTR_CNS,            0x00001040,         0x3f3f3f3f,      4,    0x4      }*/
            ,{DUMMY_NAME_PTR_CNS,            0x00001110,         0xffff0000,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00001150,         0x3f3f3f3f,     64,    0x4      }
            ,{DUMMY_NAME_PTR_CNS,            0x00002108,         0x08080808,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x000021d0,         0x0870e1c3,      1,    0x0      }
/*            ,{DUMMY_NAME_PTR_CNS,            0x000021e0,         0x000002c3,      1,    0x0      }*/
/*            ,{DUMMY_NAME_PTR_CNS,            0x000021f4,         0x55555555,      4,    0x10     }
            ,{DUMMY_NAME_PTR_CNS,            0x000021f8,         0xaaaaaaaa,      4,    0x10     }
            ,{DUMMY_NAME_PTR_CNS,            0x000021fc,         0xffffffff,      1,    0x0      }*/
            ,{DUMMY_NAME_PTR_CNS,            0x0000220c,         0xffffffff,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x0000221c,         0xffffffff,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x0000222c,         0xffffffff,      2,    0x24     }
            ,{DUMMY_NAME_PTR_CNS,            0x00002254,         0xffffffff,      3,    0x4      }
            ,{DUMMY_NAME_PTR_CNS,            0x00003000,         0x00000010,      8,    0x4      }
            ,{DUMMY_NAME_PTR_CNS,            0x00010000,         0x00000801,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,            0x00010004,         0x000fff39,      1,    0x0      }

            ,{NULL,            0,         0x00000000,      0,    0x0      }
        };
        static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC list = {myUnit_registersDefaultValueArr,NULL};
        unitPtr->unitDefaultRegistersPtr = &list;
    }
}

/*******************************************************************************
*   smemBobcat3UnitEgfSht
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the EGF-SHT unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitEgfSht
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    {

        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* L2 Port Isolation Table */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00000000 ,69632), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(256, 32),SMEM_BIND_TABLE_MAC(l2PortIsolation)}
            /* Egress Spanning Tree State Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02040000 , 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(256, 32),SMEM_BIND_TABLE_MAC(egressStp)}
            /* Non Trunk Members 2 Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02060000 , 131072 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(256, 32),SMEM_BIND_TABLE_MAC(nonTrunkMembers2)}
            /* Source ID Members Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02080000 , 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(256, 32),SMEM_BIND_TABLE_MAC(sst)}
            /* Eport EVlan Filter*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x020A0000, 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(64, 8),SMEM_BIND_TABLE_MAC(egfShtEportEVlanFilter)}
            /* Multicast Groups Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x020C0000 , 262144), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(512, 64),SMEM_BIND_TABLE_MAC(mcast)}
             /* Device Map Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02100000 ,  16384), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(16, 4),SMEM_BIND_TABLE_MAC(deviceMapTable)}
             /* Vid Mapper Table*/
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02110000 ,  32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12, 4),SMEM_BIND_TABLE_MAC(egfShtVidMapper)}
             /* Designated Port Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02120000 ,   8192), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(512, 64),SMEM_BIND_TABLE_MAC(designatedPorts)}
            /* Egress EPort table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02200000 , 65536), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(15, 4),SMEM_BIND_TABLE_MAC(egfShtEgressEPort)}
            /* Non Trunk Members Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x02400000 , 16384 ), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(512, 64),SMEM_BIND_TABLE_MAC(nonTrunkMembers)}
            /* Egress vlan table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03000000 ,262144), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(256, 32),SMEM_BIND_TABLE_MAC(egressVlan)}
            /* EVlan Attribute table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03400000 , 32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(6, 4),SMEM_BIND_TABLE_MAC(egfShtEVlanAttribute)}
            /* EVlan Spanning table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x03440000 , 32768), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12, 4),SMEM_BIND_TABLE_MAC(egfShtEVlanSpanning)}
            /* L3 Port Isolation Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x04000000 ,69632), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(256, 32),SMEM_BIND_TABLE_MAC(l3PortIsolation)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06020000, 0x06020010)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06020020, 0x06020020)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06020030, 0x06020030)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06020040, 0x0602083C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x06020880, 0x06020900)}
        };

        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    {
        static SMEM_REGISTER_DEFAULT_VALUE_STC myUnit_registersDefaultValueArr[] =
        {
             {DUMMY_NAME_PTR_CNS,           0x06020000,         0x00004007 ,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,           0x06020004,         0x00000001,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,           0x06020008,         0x0000000f,      1,    0x0      }
            ,{DUMMY_NAME_PTR_CNS,           0x06020040,         0xffffffff,      16,    0x4,    }
            ,{DUMMY_NAME_PTR_CNS,           0x06020140,         0xffffffff,      16,    0x4,    }
            ,{DUMMY_NAME_PTR_CNS,           0x060201c0,         0xffffffff,      16,    0x4      }

            ,{DUMMY_NAME_PTR_CNS,            0x02100000,         0x00000002,   4096,    0x4      }
            ,{DUMMY_NAME_PTR_CNS,            0x02120000,         0xffffffff,   1024,    0x4      }
            ,{DUMMY_NAME_PTR_CNS,            0x020a0000,         0xffffffff,  32768,    0x4      }
            ,{DUMMY_NAME_PTR_CNS,            0x020e0000,         0xffffffff,   2048,    0x4      }
            ,{DUMMY_NAME_PTR_CNS,            0x02080000,         0xffffffff,  32768,    0x4      }
            /* vidx 0xfff - 512 members */
            ,{DUMMY_NAME_PTR_CNS,            0x020C0000 +(0xfff*0x40) ,         0xffffffff,      16,    0x4      }

            /* vlan 1 members */
            ,{DUMMY_NAME_PTR_CNS,            0x03000000 + (1*0x20),         0xffffffff,      8,    0x4      }

            ,{NULL,            0,         0x00000000,      0,    0x0      }
        };
        static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC list = {myUnit_registersDefaultValueArr,NULL};
        unitPtr->unitDefaultRegistersPtr = &list;
    }



}

/*******************************************************************************
*   smemBobcat3UnitOamUnify
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  any of the 2 OAMs:
*                   1. ioam
*                   2. eoam
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr     - pointer to the unit chunk
*       bindTable   - bind table with memory chunks
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
static void smemBobcat3UnitOamUnify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr,
    IN GT_BOOL bindTable
)
{

    /* chunks with flat memory (no formulas) */
    {
        /* start with tables */
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000004)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000014, 0x0000003C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000050, 0x0000005C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000070, 0x0000007C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000084, 0x00000088)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000090, 0x000000E0)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x000000F0, 0x00000148)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000160, 0x00000160)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x0000021C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000300, 0x0000031C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000400, 0x0000041C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000500, 0x0000051C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000600, 0x0000061C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000700, 0x0000071C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x0000081C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000900, 0x0000091C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A00, 0x00000A1C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A40, 0x00000A44)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A50, 0x00000A58)}
            /* Aging Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00007000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(oamAgingTable)}
            /* Meg Exception Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(oamMegExceptionTable)}
            /* Source Interface Exception Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00018000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(oamSrcInterfaceExceptionTable)}
            /* Invalid Keepalive Hash Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00020000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(oamInvalidKeepAliveHashTable)}
            /* Excess Keepalive Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00028000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(oamExcessKeepAliveTable)}
            /* OAM Exception Summary Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00030000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(oamExceptionSummaryTable)}
            /* RDI Status Change Exception Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00038000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(oamRdiStatusChangeExceptionTable)}
            /* Tx Period Exception Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 1024), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4), SMEM_BIND_TABLE_MAC(oamTxPeriodExceptionTable)}
            /* OAM Opcode Packet Command Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00060000, 2048), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(40, 8), SMEM_BIND_TABLE_MAC(oamOpCodePacketCommandTable)}
            /* OAM Table */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00080000, 262144), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(104, 16), SMEM_BIND_TABLE_MAC(oamTable)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);


        GT_U32  ii;

        for(ii = 0 ; ii < numOfChunks; ii++)
        {
            if (bindTable == GT_FALSE)
            {
                /* make sure that table are not bound to eoam(only to ioam) */
                chunksMem[ii].tableOffsetValid = 0;
                chunksMem[ii].tableOffsetInBytes = 0;
            }
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }
}

/*******************************************************************************
*   smemBobcat3UnitIOam
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the IOAM
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr - pointer to the unit chunk
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
static void smemBobcat3UnitIOam
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    smemBobcat3UnitOamUnify(devObjPtr, unitPtr, GT_TRUE);
}
/*******************************************************************************
*   smemBobcat3UnitEOam
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the EOAM
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitPtr - pointer to the unit chunk
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
static void smemBobcat3UnitEOam
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{
    smemBobcat3UnitOamUnify(devObjPtr, unitPtr, GT_FALSE);
}

/*******************************************************************************
*   smemBobcat3UnitIpvx
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  ipvx unit
*
* INPUTS:
*       commonDevMemInfoPtr - pointer to common device memory object.
*       unitPtr             - pointer to the unit chunk
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smemBobcat3UnitIpvx
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN SMEM_UNIT_CHUNKS_STC  *unitPtr
)
{
    SMEM_CHUNK_BASIC_STC  chunksMem[]=
    {
         {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000010)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000020, 0x00000024)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000100, 0x00000104)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000200, 0x00000204)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000250, 0x0000026C)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000278, 0x00000294)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000360, 0x00000364)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000380, 0x00000380)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000900, 0x00000924)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000940, 0x00000948)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000950, 0x00000954)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000968, 0x00000978)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000980, 0x00000984)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A00, 0x00000A24)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A80, 0x00000A84)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B00, 0x00000B24)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B80, 0x00000B84)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000C00, 0x00000C24)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000C80, 0x00000C84)}
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000E00, 0x00000E2C)}
        /* Router QoS Profile Offsets Table */
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00010000, 1024),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(12, 4),SMEM_BIND_TABLE_MAC(ipvxQoSProfileOffsets)}
        /* Router Next Hop Table Age Bits */
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00020000, 3072),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4),SMEM_BIND_TABLE_MAC(routeNextHopAgeBits)}
        /* Router Acces Matrix Table */
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00060000, 3072),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(128, 16),SMEM_BIND_TABLE_MAC(ipvxAccessMatrix)}
        /* Router EVlan Table */
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00100000, 131072),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(66, 16),SMEM_BIND_TABLE_MAC(ipvxIngressEVlan)}
        /* Router EPort Table */
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(0x00200000, 32768),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(112, 16),SMEM_BIND_TABLE_MAC(ipvxIngressEPort)}
        /* Router Next Hop Table */
        ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00400000, 393216),SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(106, 16),SMEM_BIND_TABLE_MAC(ipvxNextHop)}

    };

    GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
    smemInitMemChunk(devObjPtr,chunksMem, numOfChunks, unitPtr);

    {
        static SMEM_REGISTER_DEFAULT_VALUE_STC myUnit_registersDefaultValueArr[] =
        {
             {DUMMY_NAME_PTR_CNS,           0x00000000,         0x00000047,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000004,         0x017705dc,      4,    0x4    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000020,         0x0380001c,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000024,         0x00000fff,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000100,         0x1b79b01b,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000104,         0x001b665b,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000200,         0x0179b01b,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000204,         0x0000661B,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000250,         0xffc0fe80,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000254,         0xfe00fc00,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000264,         0x00000001,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000268,         0x00000003,      2,    0x4    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000360,         0x00006140,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000364,         0xffffffff,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000380,         0x001b9360,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000968,         0x0000ffff,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000978,         0x99abadad,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000e00,         0x88878685,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000e04,         0x8c8b8a89,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000e08,         0x9f8f8e8d,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000e0c,         0xa3a2a1a0,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000e10,         0xa7a6a5a4,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000e14,         0xabaaa9a8,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000e18,         0xafaeadac,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000e1c,         0x90b6b1b0,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000e20,         0x91b5b4b3,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000e24,         0xcbcac9c8,      1,    0x0    }
            ,{DUMMY_NAME_PTR_CNS,           0x00000e28,         0x9392cdcc,      1,    0x0    }

            ,{NULL,            0,         0x00000000,      0,    0x0      }
        };
        static SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC list = {myUnit_registersDefaultValueArr,NULL};
        unitPtr->unitDefaultRegistersPtr = &list;
    }
}

/*******************************************************************************
*   smemBobcat3UnitMll
*
* DESCRIPTION:
*       Allocate address type specific memories -- for the  MLL unit
*
* INPUTS:
*       commonDevMemInfoPtr   - pointer to common device memory object.
*       unitPtr - pointer to the unit chunk
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemBobcat3UnitMll
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC  * unitPtr
)
{

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000000, 0x00000018)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000020, 0x00000024)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000030, 0x00000034)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000040, 0x00000040)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000050, 0x00000058)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000060, 0x00000064)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000080, 0x00000080)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000100, 0x00000204)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000210, 0x00000220)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000300, 0x0000030C)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000400, 0x00000404)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000800, 0x00000804)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000900, 0x00000900)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000980, 0x00000984)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A00, 0x00000A00)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000A80, 0x00000A84)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000B04, 0x00000B04)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000C00, 0x00000C00)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000C80, 0x00000C84)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000D00, 0x00000D00)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000D80, 0x00000D84)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000E00, 0x00000E08)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (0x00000E20, 0x00000E28)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00040000, 131072), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(20, 4),SMEM_BIND_TABLE_MAC(l2MllLtt)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC (0x00100000, 1048576), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(162, 32) , SMEM_BIND_TABLE_MAC(mll)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

}

/*******************************************************************************
*   smemBobcat3SpecificDeviceUnitAlloc_SIP_units
*
* DESCRIPTION:
*       specific initialization units allocation that called before alloc units
*       of any device.
*       SIP units
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Pointer to object for specific subunit
*
* COMMENTS:
*
*******************************************************************************/
static void smemBobcat3SpecificDeviceUnitAlloc_SIP_units
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC    *currUnitChunkPtr;
    GT_U32  unitIndex;

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPLR);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitIplr0(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPLR1);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitIplr1(devObjPtr, currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EPLR);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitEplr(devObjPtr,currUnitChunkPtr);
    }


    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EGF_QAG);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitEgfQag(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TTI);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitTti(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_TCAM);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitTcam(devObjPtr, currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPCL);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitIpcl(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_L2I);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitL2i(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_HA);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitHa(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EQ);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitEq(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EGF_EFT);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitEgfEft(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EGF_SHT);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitEgfSht(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IOAM);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitIOam(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_EOAM);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitEOam(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_IPVX);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitIpvx(devObjPtr,currUnitChunkPtr);
    }

    unitIndex = UNIT_INDEX_FROM_ENUM_GET_MAC(devObjPtr,UNIT_MLL);
    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[unitIndex];
    if(currUnitChunkPtr->numOfChunks == 0)
    {
        smemBobcat3UnitMll(devObjPtr,currUnitChunkPtr);
    }
}

/*******************************************************************************
*   smemBobcat3SpecificDeviceUnitAlloc
*
* DESCRIPTION:
*       specific initialization units allocation that called before alloc units
*       of any device
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Pointer to object for specific subunit
*
* COMMENTS:
*
*******************************************************************************/
static void smemBobcat3SpecificDeviceUnitAlloc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = devObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC    *currUnitChunkPtr;
    GT_U32  unitIndex;

    currUnitChunkPtr = &devMemInfoPtr->unitMemArr[0];

    for (unitIndex = 0 ; unitIndex < SMEM_CHT_NUM_UNITS_MAX_CNS;
        unitIndex+=2,currUnitChunkPtr += 2)
    {
        currUnitChunkPtr->chunkType = SMEM_UNIT_CHUNK_TYPE_8_MSB_E;

        /* point the next 'half' unit on 'me' */
        currUnitChunkPtr[1].hugeUnitSupportPtr = currUnitChunkPtr;
        currUnitChunkPtr[1].numOfUnits = 1;
        currUnitChunkPtr[1].chunkType = SMEM_UNIT_CHUNK_TYPE_9_MSB_E;
    }

    {
        GT_U32  ii,jj;
        UNIT_INFO_STC   *unitInfoPtr = &bobcat3units[0];

        for(ii = 0 ; unitInfoPtr->size != SMAIN_NOT_VALID_CNS; ii++,unitInfoPtr++)
        {
            jj = unitInfoPtr->base_addr >> SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS;

            currUnitChunkPtr = &devMemInfoPtr->unitMemArr[jj];

            if(currUnitChunkPtr->chunkIndex != jj)
            {
                skernelFatalError("smemBobcat3Init : not matched index");
            }
            currUnitChunkPtr->numOfUnits = unitInfoPtr->size;
        }
    }

    /* allocate the specific units that we NOT want the bc2_init , lion3_init , lion2_init
       to allocate. */

    smemBobcat3SpecificDeviceUnitAlloc_DP_units(devObjPtr);

    smemBobcat3SpecificDeviceUnitAlloc_SIP_units(devObjPtr);



}

/*******************************************************************************
*   smemBobcat3SpecificDeviceMemInitPart2
*
* DESCRIPTION:
*       specific part 2 of initialization that called from init 1 of
*       Lion3
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
void smemBobcat3SpecificDeviceMemInitPart2
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{

    /* bind active memories to units ... need to be before
        smemBobcat2SpecificDeviceMemInitPart2 ... so bc2 will not override it */
/*    bindActiveMemoriesOnUnits(devObjPtr);*/

    /* call bobcat2 */
    smemBobcat2SpecificDeviceMemInitPart2(devObjPtr);
}

/*******************************************************************************
*   smemBobcat3ConvertDevAndAddrToNewDevAndAddr
*
* DESCRIPTION:
*       Bobcat3 : Convert {dev,address} to new {dev,address}.
*       needed for multi-pipe device.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       address     - address of memory(register or table).
*       accessType  - the access type
* OUTPUTS:
*       newDevObjPtrPtr      - (pointer to) pointer to new device object.
*       newAddressPtr        - (pointer to) address of memory (register or table).
*
* RETURNS:
*        None
*
* COMMENTS:
*       function MUST be called before calling smemFindMemory()
*
*******************************************************************************/
static void smemBobcat3ConvertDevAndAddrToNewDevAndAddr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                  address,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    OUT SKERNEL_DEVICE_OBJECT * *newDevObjPtrPtr,
    OUT GT_U32                  *newAddressPtr
)
{

    GT_U32  unitIndex;
    GT_U32  newAddress;
    SIM_OS_TASK_COOKIE_INFO_STC  *myTaskInfoPtr;
    GT_U32  currentPipeId;

    *newDevObjPtrPtr = devObjPtr;

    if(0 == IS_SKERNEL_OPERATION_MAC(accessType) ||
       IS_DFX_OPERATION_MAC(accessType))
    {
        /* the CPU can only access 'pipe 0 device' that hold all units */
        /* the DFX also is only on 'pipe 0 device' */
        *newAddressPtr = address;
        return;
    }

    if(address & SECOND_PIPE_OFFSET_CNS)
    {
        /* explicit access to pipe 1 , do not modify */
        *newAddressPtr = address;
        return;
    }


    myTaskInfoPtr = simOsTaskOwnTaskCookieGet();
    if(myTaskInfoPtr)
    {
        currentPipeId = myTaskInfoPtr->currentPipeId;
    }
    else
    {
        currentPipeId = 0;
    }

    if(currentPipeId == 0)
    {
        /* no address update needed for pipe 0 accessing */
        *newAddressPtr = address;
        return;
    }

    /* now need to check how to convert the address */

    /* bobcat3UnitTypeArr[] indexed by '8 MSB' index */
    unitIndex = address >> (1 + SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS);

    switch(bobcat3UnitTypeArr[unitIndex])
    {
        case UNIT_TYPE_PIPE_0_AND_PIPE_1_E:
            /* no change in address as it is shared between the pipes */
            newAddress = address;
            break;
        case UNIT_TYPE_PIPE_0_ONLY_E:
            /* we access pipe 0 units , but we mean to access the pipe 1 units ! */
            newAddress = address | SECOND_PIPE_OFFSET_CNS;
            break;
        default:
        case UNIT_TYPE_NOT_VALID:
            /* no change in address as it is not valid unit ...
               it will cause fatal error , somewhere next ... */
            newAddress = address;
            break;
        case UNIT_TYPE_PIPE_1_ONLY_E:
            /* we are in 'pipe 1 aware mode' ?! */

            /* should not happen */
            skernelFatalError("smemBobcat3ConvertDevAndAddrToNewDevAndAddr: 'pipe 1 device' not designed for 'aware mode' \n");

            newAddress = address;
            break;
    }

    *newAddressPtr = newAddress;

    return;
}

/*******************************************************************************
*   smemBobcat3PreparePipe1Recognition
*
* DESCRIPTION:
*       prepare pipe 1 recognition
*
* INPUTS:
*       pipe0_devObjPtr   - pointer to device object of pipe 0
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
static void smemBobcat3PreparePipe1Recognition
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_DEV_COMMON_MEM_INFO  * commonDevMemInfoPtr;
    UNIT_INFO_STC   *unitInfoPtr = &bobcat3units[0];
    GT_U32  unitIndex;
    UNIT_TYPE_ENT   unitType;
    GT_U32  ii,jj;

    /* point pipe1 to look directly to pipe 0 memory */
    /* NOTE: the function smemBobcat3ConvertDevAndAddrToNewDevAndAddr will help
       any memory access via pipe1 to access proper unit in the 'pipe 0 device' */
    commonDevMemInfoPtr = devObjPtr->deviceMemory;
    commonDevMemInfoPtr->smemConvertDevAndAddrToNewDevAndAddrFunc =
        smemBobcat3ConvertDevAndAddrToNewDevAndAddr;


    /* NOTE: at this stage the units of pipe 1 are already allocated by 'pipe 0 device' */

    /* set 'pipe1' to point on the 'high' units as if they are in the 'low' units */

    for(ii = 0 ; unitInfoPtr->size != SMAIN_NOT_VALID_CNS; ii++,unitInfoPtr++)
    {
        if(unitInfoPtr->orig_nameStr ==
            SHARED_BETWEEN_PIPE_0_AND_PIPE_1_INDICATION_CNS)
        {
            /* this unit used both by pipe 0 and pip 1 */
            unitType = UNIT_TYPE_PIPE_0_AND_PIPE_1_E;
        }
        else
        if(unitInfoPtr->base_addr & SECOND_PIPE_OFFSET_CNS)
        {
            /* this unit belongs to pip 1 */
            unitType = UNIT_TYPE_PIPE_1_ONLY_E;
        }
        else
        {
            /* this unit belongs to pip 0 */
            unitType = UNIT_TYPE_PIPE_0_ONLY_E;
        }

        /* bobcat3UnitTypeArr[] indexed by '8 MSB' index */
        unitIndex = unitInfoPtr->base_addr >> (1 + SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS);

        for(jj = 0 ; jj < unitInfoPtr->size ; jj++)
        {
            bobcat3UnitTypeArr[unitIndex + jj] = unitType;
        }
    }


}



/*******************************************************************************
*   smemBobcat3Init
*
* DESCRIPTION:
*       Init memory module for a Bobcat3 device.
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
void smemBobcat3Init
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_BOOL isBobcat3 = GT_FALSE;

    /* state the supported features */
    SMEM_CHT_IS_SIP5_20_GET(devObjPtr) = 1;

    if(devObjPtr->devMemUnitNameAndIndexPtr == NULL)
    {
        buildDevUnitAddr(devObjPtr);

        isBobcat3 = GT_TRUE;
    }

    if(devObjPtr->registersDefaultsPtr == NULL)
    {
        /*devObjPtr->registersDefaultsPtr = &linkListElementsBobcat3_RegistersDefaults;*/
    }

    if(devObjPtr->registersDefaultsPtr_unitsDuplications == NULL)
    {
        devObjPtr->registersDefaultsPtr_unitsDuplications = BOBCAT3_duplicatedUnits;
        devObjPtr->unitsDuplicationsPtr = BOBCAT3_duplicatedUnits;
    }


    if (isBobcat3 == GT_TRUE)
    {
        /* state 'data path' structure */
        devObjPtr->multiDataPath.supportMultiDataPath =  1;
        devObjPtr->multiDataPath.maxDp = 3;/* 6 DP units for the device */
        /* there is TXQ,dq per 'data path' */
        devObjPtr->multiDataPath.numTxqDq           = devObjPtr->multiDataPath.maxDp;
        devObjPtr->multiDataPath.txqDqNumPortsPerDp = 96;

        devObjPtr->multiDataPath.supportRelativePortNum = 1;

        devObjPtr->supportTrafficManager_notAllowed = 1;

        devObjPtr->dmaNumOfCpuPort = 72;/* 'global' port in the egress RXDMA/TXDMA units */
        devObjPtr->multiDataPath.localIndexOfDmaOfCpuPortNum = 12;

        devObjPtr->numOfPipes = 2;
        devObjPtr->numOfPortsPerPipe =
            devObjPtr->multiDataPath.maxDp*PORTS_PER_DP_CNS;

        devObjPtr->txqNumPorts =
            devObjPtr->multiDataPath.txqDqNumPortsPerDp *
            devObjPtr->multiDataPath.numTxqDq *
            devObjPtr->numOfPipes;/*the TXQ of Bc3 support 576 ports */

        {
            GT_U32  index;
            for(index = 0 ; index < devObjPtr->multiDataPath.maxDp ; index++)
            {
                /* each DP supports 12 ports + 1 CPU port (index 12) */
                devObjPtr->multiDataPath.info[index].dataPathFirstPort  = PORTS_PER_DP_CNS*index;
                devObjPtr->multiDataPath.info[index].dataPathNumOfPorts = PORTS_PER_DP_CNS;
            }
            devObjPtr->multiDataPath.info[0].cpuPortDmaNum = PORTS_PER_DP_CNS;

            devObjPtr->memUnitBaseAddrInfo.rxDma[0] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_RX_DMA);
            devObjPtr->memUnitBaseAddrInfo.rxDma[1] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_RX_DMA_1);
            devObjPtr->memUnitBaseAddrInfo.rxDma[2] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_RX_DMA_2);

            devObjPtr->memUnitBaseAddrInfo.txDma[0] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TX_DMA);
            devObjPtr->memUnitBaseAddrInfo.txDma[1] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TX_DMA_1);
            devObjPtr->memUnitBaseAddrInfo.txDma[2] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TX_DMA_2);

            devObjPtr->memUnitBaseAddrInfo.txFifo[0] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TX_FIFO);
            devObjPtr->memUnitBaseAddrInfo.txFifo[1] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TX_FIFO_1);
            devObjPtr->memUnitBaseAddrInfo.txFifo[2] = UNIT_BASE_ADDR_GET_MAC(devObjPtr,UNIT_TX_FIFO_2);
        }

        devObjPtr->tcam_numBanksForHitNumGranularity = 2;
    }

    devObjPtr->lpmRam.numOfLpmRams = 20;
    devObjPtr->lpmRam.numOfEntriesBetweenRams = 16*1024;

    {
        SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.phyPort,9);
        SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.stgId,12); /* 4K*/
        SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.tunnelstartPtr , 16);/*64K*/

        SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.phyPort , 9);
        SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.trunkId , 12);
        SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.ePort , 14);/*16K*/
        SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.eVid , 13);/*8K*/
        SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.eVidx , 14);/*16K*/
        SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.hwDevNum , 10);/*1K*/
        SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.sstId , 12 );/*4K*/
        SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.tunnelstartPtr , 16);/*64K*/
        SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.arpPtr ,  4 * devObjPtr->flexFieldNumBitsSupport.tunnelstartPtr);
        SET_IF_ZERO_MAC(devObjPtr->flexFieldNumBitsSupport.fid , 13);


        SET_IF_ZERO_MAC(devObjPtr->limitedResources.eVid,1<<devObjPtr->flexFieldNumBitsSupport.eVid);
        SET_IF_ZERO_MAC(devObjPtr->limitedResources.ePort,1<<devObjPtr->flexFieldNumBitsSupport.ePort);
        SET_IF_ZERO_MAC(devObjPtr->limitedResources.phyPort,1<<devObjPtr->flexFieldNumBitsSupport.phyPort);
        SET_IF_ZERO_MAC(devObjPtr->limitedResources.l2Ecmp,16*1024);
        SET_IF_ZERO_MAC(devObjPtr->limitedResources.nextHop,24*1024);

        SET_IF_ZERO_MAC(devObjPtr->limitedResources.mllPairs,32*1024);
        SET_IF_ZERO_MAC(devObjPtr->limitedResources.l2LttMll,32*1024);
        SET_IF_ZERO_MAC(devObjPtr->limitedResources.l3LttMll,16*1024);/*routing ECMP table*/


        SET_IF_ZERO_MAC(devObjPtr->fdbMaxNumEntries , SMEM_MAC_TABLE_SIZE_256KB);
        SET_IF_ZERO_MAC(devObjPtr->lpmRam.perRamNumEntries , 20*1024);

        SET_IF_ZERO_MAC(devObjPtr->policerSupport.eplrTableSize   , (16 * _1K));
        SET_IF_ZERO_MAC(devObjPtr->policerSupport.iplrTableSize   , (16 * _1K));
        SET_IF_ZERO_MAC(devObjPtr->policerSupport.iplrMemoriesSize[0] , 15 * _1K);
        SET_IF_ZERO_MAC(devObjPtr->policerSupport.iplrMemoriesSize[1] , _1K - 256);
        SET_IF_ZERO_MAC(devObjPtr->policerSupport.iplrMemoriesSize[2] , /*256 */
            devObjPtr->policerSupport.iplrTableSize -
            (devObjPtr->policerSupport.iplrMemoriesSize[0] + devObjPtr->policerSupport.iplrMemoriesSize[1]));

        SET_IF_ZERO_MAC(devObjPtr->policerSupport.iplr0TableSize ,
                        devObjPtr->policerSupport.iplrMemoriesSize[1] +
                        devObjPtr->policerSupport.iplrMemoriesSize[2]) ;
        SET_IF_ZERO_MAC(devObjPtr->policerSupport.iplr1TableSize ,
                    devObjPtr->policerSupport.iplrTableSize -
                    devObjPtr->policerSupport.iplr0TableSize);

        SET_IF_ZERO_MAC(devObjPtr->oamNumEntries,8*_1K);

        SET_IF_ZERO_MAC(devObjPtr->cncClientSupportBitmap,SNET_SIP5_20_CNC_CLIENTS_BMP_ALL_CNS);
    }

    /* function will be called from inside smemLion2AllocSpecMemory(...) */
    if(devObjPtr->devMemSpecificDeviceUnitAlloc == NULL)
    {
        devObjPtr->devMemSpecificDeviceUnitAlloc = smemBobcat3SpecificDeviceUnitAlloc;
    }

    /* function will be called from inside smemLion3Init(...) */
    if(devObjPtr->devMemSpecificDeviceMemInitPart2 == NULL)
    {
        devObjPtr->devMemSpecificDeviceMemInitPart2 = smemBobcat3SpecificDeviceMemInitPart2;
    }

    smemBobkInit(devObjPtr);

/*    smemBobcat3InitPostBobk(devObjPtr);*/

    /* Init the bobk interrupts */
/*    smemBobcat3InitInterrupts(devObjPtr);*/

    /* prepare pipe 1 recognition */
    smemBobcat3PreparePipe1Recognition(devObjPtr);
}

/*******************************************************************************
*   smemBobcat3Init2
*
* DESCRIPTION:
*       Init memory module for a device - after the load of the default
*           registers file
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
void smemBobcat3Init2
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    static int my_dummy = 0;

    smemBobkInit2(devObjPtr);


    if(my_dummy)
    {
        GT_U32  pipeId;
        GT_U32  globalPortNum;
        /* <DeviceEn>*/
        smemDfxRegFldSet(devObjPtr, SMEM_LION2_DFX_SERVER_RESET_CONTROL_REG(devObjPtr), 0, 1, 1);

        for(pipeId = 0 ; pipeId < devObjPtr->numOfPipes ; pipeId++)
        {
            GT_U32  ii;
            GT_U32  regAddress = SMEM_LION2_EGF_EFT_VIRTUAL_2_PHYSICAL_PORT_REMAP_REG(devObjPtr,0);
            GT_U32  value;

            /* /Cider/EBU-IP/SIP/EGF_IP/SIP 5.20/EGF_IP {Current}/EGF_IP_eft/Units/EGF_IP_eft/Egress Filter Configurations/Port Remap <%n>*/
            /* set mapping 1:1 */

            regAddress += pipeId * SECOND_PIPE_OFFSET_CNS;

            /* 2 ports per register*/
            for(ii = 0 ; ii < 256 ; ii++ , regAddress+=4)
            {
                value = (2*ii) + (((2*ii)+1)<<9);
                smemRegSet(devObjPtr,regAddress,value);
            }
        }

        globalPortNum = 0;
        for(pipeId = 0 ; pipeId < devObjPtr->numOfPipes ; pipeId++)
        {
            GT_U32  ii;
            GT_U32  regAddress;
            GT_U32  offset;
            GT_U32  value;
            GT_U32  dpUnitInPipe;
            GT_U32  portInPipe = 0;

            for(dpUnitInPipe = 0 ; dpUnitInPipe < devObjPtr->multiDataPath.maxDp; dpUnitInPipe ++)
            {
                offset = 0;

                portInPipe = PORTS_PER_DP_CNS*dpUnitInPipe;
                regAddress = SMEM_LION3_RXDMA_SCDMA_CONFIG_1_REG(devObjPtr,portInPipe);
                regAddress += pipeId * SECOND_PIPE_OFFSET_CNS;

                /* port per register */
                for(ii = 0 ; ii < (PORTS_PER_DP_CNS+1) ; ii++ , offset+=4)
                {
                    if(ii == PORTS_PER_DP_CNS)
                    {
                        value = SNET_CHT_CPU_PORT_CNS;
                    }
                    else
                    {
                        value = globalPortNum;

                        globalPortNum ++;
                    }

                    /*perform RXDMA mapping from local port to 'virual' port on the field of:
                      localDevSrcPort */
                    smemRegFldSet(devObjPtr,regAddress + offset,
                        0, 9, value);
                }
            }

        }

        {
            GT_U32  ii;
            GT_U32  regAddress = SMEM_BOBCAT2_BMA_PORT_MAPPING_TBL_MEM(devObjPtr,0);
            GT_U32  value;

            /* /Cider/EBU/Bobcat3/Bobcat3 {Current}/Switching Core/BMA_IP/Tables/<BMA_IP> BMA_IP Features/Port Mapping/Port Mapping entry */
            /* set mapping 1:1 */

            /* port per register */
            for(ii = 0 ; ii < 512 ; ii++ , regAddress+=4)
            {
                value = ii & 0x7f;/* 7 bits */
                smemRegSet(devObjPtr,regAddress,value);
            }
        }

        for(pipeId = 0 ; pipeId < devObjPtr->numOfPipes ; pipeId++)
        {
            GT_U32  dpUnitInPipe;

            for(dpUnitInPipe = 0 ; dpUnitInPipe < devObjPtr->multiDataPath.maxDp; dpUnitInPipe ++)
            {
                GT_U32  ii;
                GT_U32  regAddress = SMEM_LION3_TXQ_DQ_TXQ_PORT_TO_TX_DMA_MAP_REG(devObjPtr,0,dpUnitInPipe);
                GT_U32  value;

                regAddress += pipeId * SECOND_PIPE_OFFSET_CNS;

                /* /Cider/EBU-IP/TXQ_IP/SIP6.0/TXQ_IP {Current}/TXQ_DQ/Units/TXQ_IP_dq/Global/Global DQ Config/Port <%n> To DMA Map Table */
                /* set mapping 1:1 */

                /* port per register */
                for(ii = 0 ; ii < 72 ; ii++ , regAddress+=4)
                {
                    value = ii;
                    smemRegSet(devObjPtr,regAddress,value);
                }
            }
        }


#if     0

        {
            GT_U32  initValueArr[MAX_INIT_WORDS_CNS];
            GT_U32  numWords;
            GT_U32  numEntries;
            GT_U32  startIndex;

            startIndex = 0xFFF / 2;
            numEntries = 1;
            numWords = 0;
            initValueArr[numWords++] = 0xFFFFFFFF;
            initValueArr[numWords++] = 0xFFFFFFFF;
            initValueArr[numWords++] = 0xFFFFFFFF;
            initValueArr[numWords++] = 0xFFFFFFFF;
            initValueArr[numWords++] = 0xFFFFFFFF;
            initValueArr[numWords++] = 0xFFFFFFFF;
            initValueArr[numWords++] = 0xFFFFFFFF;
            initValueArr[numWords++] = 0xFFFFFFFF;
            /* set vidx entry for legacy mode 0xfff */
            SMEM_TABLE_ENTRIES_INIT_MAC(devObjPtr,mcast,startIndex,initValueArr,numWords,numEntries);
        }
        /*startSimulationLog();*/
#endif
    }
}


