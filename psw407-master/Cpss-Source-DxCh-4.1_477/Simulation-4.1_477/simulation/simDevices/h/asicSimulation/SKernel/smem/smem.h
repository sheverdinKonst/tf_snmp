/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smain.h
*
* DESCRIPTION:
*       This is a external API definition for SMem module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 39 $
*
*******************************************************************************/
#ifndef __smemh
#define __smemh

#include <os/simTypesBind.h>
#include <asicSimulation/SCIB/scib.h>
#include <asicSimulation/SKernel/smain/smain.h>

/* macro to get name of array and the number of elements */
#define ARRAY_NAME_AND_NUM_ELEMENTS_MAC(arrayName)  arrayName , (sizeof(arrayName) / sizeof(arrayName[0]))

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Global constants definition */
#define SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS               23
#define SMEM_PHY_UNIT_INDEX_CNS                         30

/* Memory access for all types of read (SCIB and SKERNEL) */
#define SMEM_ACCESS_READ_FULL_MAC(type) \
    (IS_WRITE_OPERATION_MAC(type) == 0)

/* Memory access for PCI read/write (SCIB and SKERNEL) */
#define SMEM_ACCESS_PCI_FULL_MAC(type) \
    IS_PCI_OPERATION_MAC(type)

/* Memory access for DFX read/write (SCIB and SKERNEL) */
#define SMEM_ACCESS_DFX_FULL_MAC(type) \
    IS_DFX_OPERATION_MAC(type)

/* Memory access for all types of read/write (SKERNEL) */
#define SMEM_ACCESS_SKERNEL_FULL_MAC(type) \
    IS_SKERNEL_OPERATION_MAC(type)

#define STRING_FOR_UNIT_NAME(unitName)            \
    string_##unitName

#define BUILD_STRING_FOR_UNIT_NAME(unitName)             \
    static GT_CHAR *string_##unitName = STR(unitName)

/* Define the different FDB memory sizes    */
#define SMEM_MAC_TABLE_SIZE_4KB            (0x1000)
#define SMEM_MAC_TABLE_SIZE_8KB            (0x2000)
#define SMEM_MAC_TABLE_SIZE_16KB           (0x4000)
#define SMEM_MAC_TABLE_SIZE_32KB           (0x8000)
#define SMEM_MAC_TABLE_SIZE_64KB           (0x10000)
#define SMEM_MAC_TABLE_SIZE_128KB          (0x20000)
#define SMEM_MAC_TABLE_SIZE_256KB          (0x40000)
#define SMEM_MAC_TABLE_SIZE_384KB          (0x60000)
#define SMEM_MAC_TABLE_SIZE_512KB          (0x80000)

/*******************************************************************************
*  DECLARE_UNIT_MEM_FUNCTION_MAC
*
* DESCRIPTION:
*      find the address in the unit's registers / tables and return the memory
*       pointer.
*
* INPUTS:
*       devObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
*       inMemPtr    - Pointer to the memory to get register's content.
*
* OUTPUTS:
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*******************************************************************************/
#define DECLARE_UNIT_MEM_FUNCTION_MAC(funcName)  \
    GT_U32 * funcName                            \
    (                                            \
        IN SKERNEL_DEVICE_OBJECT * devObjPtr,    \
        IN SCIB_MEMORY_ACCESS_TYPE accessType,   \
        IN GT_U32 address,                       \
        IN GT_U32 memSize,                       \
        IN GT_UINTPTR param                      \
    )

/*******************************************************************************
*  ACTIVE_WRITE_FUNC_PROTOTYPE_MAC
*
* DESCRIPTION:
*      active Write to register
*
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memSize     - size of the requested memory
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
#define ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(funcName)        \
    void (funcName)(                                     \
        IN         SKERNEL_DEVICE_OBJECT * devObjPtr,    \
        IN         GT_U32   address,                     \
        IN         GT_U32   memSize,                     \
        IN         GT_U32 * memPtr,                      \
        IN         GT_UINTPTR param,                     \
        IN         GT_U32 * inMemPtr                     \
    )

/*******************************************************************************
*  ACTIVE_READ_FUNC_PROTOTYPE_MAC
*
* DESCRIPTION:
*      active read of register
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - address for ASIC memory.
*       memSize     - memory size to be read.
*       memPtr      - pointer to the register's memory in the simulation.
*       sumBit      - global summary interrupt bit
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
#define ACTIVE_READ_FUNC_PROTOTYPE_MAC(funcName)        \
    void (funcName)(                                     \
        IN         SKERNEL_DEVICE_OBJECT * devObjPtr,    \
        IN         GT_U32   address,                     \
        IN         GT_U32   memSize,                     \
        IN         GT_U32 * memPtr,                      \
        IN         GT_UINTPTR sumBit,                    \
        OUT        GT_U32 * outMemPtr                    \
    )

#define SMEM_ADDRESS_NOT_IN_FORMULA_CNS 0xFEFEFEFE

#define SMEM_FORMULA_CELL_NUM_CNS           6

#define END_OF_TABLE    0xFFFFFFFF

#define SMEM_FORMULA_END_CNS    {0, 0}

/* set value to target if target is zero (0) */
#define SMEM_SET_IF_NON_ZERO_MAC(target,source) \
    if((target) == 0) (target) = (source)

/* convert number of bytes to number of words */
#define CONVERT_BYTES_TO_WORDS_MAC(numBytes) (((numBytes)+3)/4)

/* convert number of bits to number of words */
#define CONVERT_BITS_TO_WORDS_MAC(numBits) (((numBits)+31)/32)

/* convert start address and end address to number of words */
#define CONVERT_START_ADDR_END_ADDR_TO_WORDS_MAC(startAddr,endAddr) \
    (((endAddr) - (startAddr))/4 + 1)

/*set SMEM_CHUNK_BASIC_STC entry , by the end address */
#define SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC(startAddr,endAddr)  \
    (startAddr),  CONVERT_START_ADDR_END_ADDR_TO_WORDS_MAC(startAddr,endAddr)

/*set SMEM_CHUNK_BASIC_STC entry , by the number of bytes */
#define SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(startAddr,numBytes)\
    (startAddr),   CONVERT_BYTES_TO_WORDS_MAC(numBytes)

/* macro to build the formula section : with single parameter */
#define FORMULA_SINGLE_PARAMETER(numSteps,stepSize) {{numSteps ,stepSize}, SMEM_FORMULA_END_CNS}

/* macro to build the formula section : with 2 parameters */
#define FORMULA_TWO_PARAMETERS(numSteps1,stepSize1,numSteps2,stepSize2) {{numSteps1, stepSize1}, {numSteps2, stepSize2}, SMEM_FORMULA_END_CNS}

/* macro to build the formula section : with 3 parameters */
#define FORMULA_THREE_PARAMETERS(numSteps1,stepSize1,numSteps2,stepSize2,numSteps3,stepSize3) {{numSteps1, stepSize1}, {numSteps2, stepSize2}, {numSteps3, stepSize3}, SMEM_FORMULA_END_CNS}

/* macro to build the formula section : with 4 parameters */
#define FORMULA_FOUR_PARAMETERS(numSteps1,stepSize1,numSteps2,stepSize2,numSteps3,stepSize3,numSteps4,stepSize4) {{numSteps1, stepSize1}, {numSteps2, stepSize2}, {numSteps3, stepSize3}, {numSteps4, stepSize4}, SMEM_FORMULA_END_CNS}

/* set memory entry size and alignment: size - entry size in bits, align - entry alignment in bytes */
#define SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(sizeInBits, alignInBytes) (sizeInBits), (alignInBytes)

#define SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,tableName,index)\
    smemTableEntryAddressGet(&(dev)->tablesInfo.tableName,index,SMAIN_NOT_VALID_CNS)

#define SMEM_TABLE_ENTRY_2_PARAMS_INDEX_GET_MAC(dev,tableName,index1,index2)\
    smemTableEntry2ParamsAddressGet(&(dev)->tablesInfo.tableName,index1,index2,SMAIN_NOT_VALID_CNS)

#define SMEM_TABLE_ENTRY_3_PARAMS_INDEX_GET_MAC(dev,tableName,index1,index2,index3)\
    smemTableEntry3ParamsAddressGet(&(dev)->tablesInfo.tableName,index1,index2,index3,SMAIN_NOT_VALID_CNS)

#define SMEM_TABLE_ENTRY_4_PARAMS_INDEX_GET_MAC(dev,tableName,index1,index2,index3,index4)\
    smemTableEntry4ParamsAddressGet(&(dev)->tablesInfo.tableName,index1,index2,index3,index4,SMAIN_NOT_VALID_CNS)

/* table with single parameter , but in duplicated unit */
#define SMEM_TABLE_ENTRY_INDEX_DUP_TBL_GET_MAC(dev,tableName,index , instanceIndex)\
    smemTableEntryAddressGet(&(dev)->tablesInfo.tableName,index,instanceIndex)

/* table with 2 parameters , but in duplicated unit */
#define SMEM_TABLE_ENTRY_2_PARAMS_INDEX_DUP_TBL_GET_MAC(dev,tableName,index1,index2 , instanceIndex)\
    smemTableEntry2ParamsAddressGet(&(dev)->tablesInfo.tableName,index1,index2,instanceIndex)

/* table with 3 parameters , but in duplicated unit */
#define SMEM_TABLE_ENTRY_3_PARAMS_INDEX_DUP_TBL_GET_MAC(dev,tableName,index1,index2,index3 , instanceIndex)\
    smemTableEntry3ParamsAddressGet(&(dev)->tablesInfo.tableName,index1,index2,index3,instanceIndex)

/* return the offset in bytes of field from start of the structure that hold it */
#define FIELD_OFFSET_IN_STC_MAC(field,stc)  \
    ((GT_UINTPTR)(GT_U8*)(&(((stc*)NULL)->field)))

/*set next 2 fields in SMEM_CHUNK_BASIC_STC entry : tableOffsetValid, tableOffsetInBytes */
#define SMEM_BIND_TABLE_MAC(tableName)  \
    1/*tableOffsetValid*/,              \
    FIELD_OFFSET_IN_STC_MAC(tableName,SKERNEL_TABLES_INFO_STC) /*tableOffsetInBytes*/
/* indicator for 'new unit base' */
#define SMAM_SUB_UNIT_AFTER_20_LSB_CNS  0xFFFFFFF0
/* place holder in SMEM_CHUNK_BASIC_STC for the 'new unit base' */
#define SET_SMEM_CHUNK_BASIC_STC_ENTRY_SUB_UNIT_AFTER_20_LSB_MAC(unitAddr23MSBitsAfter20MSBits) \
    unitAddr23MSBitsAfter20MSBits , SMAM_SUB_UNIT_AFTER_20_LSB_CNS


/* init table with 1 param , with array of default value for the entry (all entries the same value) */
#define SMEM_TABLE_1_PARAM_INIT_MAC(_dev,_table, _initValueArr , _numWords , _numEntries)  \
    SMEM_TABLE_ENTRIES_INIT_MAC(_dev,_table, 0 , _initValueArr , _numWords , _numEntries)


/* init table entries with array of default value (duplicate to all entries)*/
#define SMEM_TABLE_ENTRIES_INIT_MAC(_dev,_table, _firstIndex , _initValueArr , _numWords , _numEntries)  \
    {                                                                                      \
        GT_U32  *__memPtr;                                                                 \
        GT_U32  __address;                                                                 \
        GT_U32  __ii;                                                                      \
        GT_U32  __entrySize = (_numWords * (sizeof(GT_U32)));                              \
                                                                                           \
        if(_dev->tablesInfo._table.paramInfo[0].step < __entrySize)                        \
        {                                                                                  \
            skernelFatalError("SMEM_TABLE_1_PARAM_INIT_MAC: entry size oversize \n");      \
        }                                                                                  \
        else                                                                               \
        {                                                                                  \
            for(__ii = _firstIndex ; __ii < _numEntries; __ii++)                                     \
            {                                                                              \
                /* get the address */                                                      \
                __address = SMEM_TABLE_ENTRY_INDEX_GET_MAC(_dev,_table,__ii);              \
                __memPtr = smemMemGet(_dev, __address);                                    \
                /*init the entry according to needed values*/                              \
                memcpy(__memPtr,initValueArr,__entrySize);                                 \
            }                                                                              \
        }                                                                                  \
    }

/* forward declaration */
struct SMEM_CHUNK_STCT;

#define DEBUG_REG_AND_TBL_NAME

typedef struct{
#ifdef DEBUG_REG_AND_TBL_NAME
    GT_CHAR     *tableName;
#endif /*DEBUG_REG_AND_TBL_NAME*/
    GT_U32      baseAddr;
    GT_U32      memType;/*0-internal , 1 - external*/
    GT_U32      entrySize;/* in bits -- number of bits actually used */
    GT_U32      lineAddrAlign;/* in words -- number of words between 2 entries*/
    GT_U32      numOfEntries;
}DEV_TABLE_INFO_STC;

/*******************************************************************************
*   SMEM_FORMULA_CHUNK_FUNC
*
* DESCRIPTION:
*       prototype to function which return the index in the simulation memory chunk
*       that represents the accessed address
*
* INPUTS:
*        devObjPtr  - pointer to device info
*        memCunkPtr - pointer to memory chunk info
*        address    - the accesses address
* OUTPUTS:
*       None
*
* RETURNS:
*       index into memCunkPtr->memPtr[index] that represent 'address'
*       if the return value == SMEM_ADDRESS_NOT_IN_FORMULA_CNS meaning that
*       address not belong to formula
*
* COMMENTS:
*
*
*******************************************************************************/
typedef GT_U32  (*SMEM_FORMULA_CHUNK_FUNC)
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN struct SMEM_CHUNK_STCT* memCunkPtr,
    IN GT_U32                  address
);

/*
 * Typedef: enum SMEM_INDEX_IN_CHUNK_TYPE_ENT
 *
 * Description:
 *      lists the options to calculate the index in the memory chunk that
 *      represents an address
 *
 * Fields:
 *      SMEM_INDEX_IN_CHUNK_TYPE_STRAIGHT_ACCESS_E - straight access , meaning
 *                              index = (address - memFirstAddr) / 4;
 *      SMEM_INDEX_IN_CHUNK_TYPE_MASK_AND_SHIFT_E - use mask and shift , meaning
 *                              index = ((address - memFirstAddr) & mask) >> rightShiftNumBits;
 *      SMEM_INDEX_IN_CHUNK_TYPE_FORMULA_FUNCTION_E - use function to calculate the index
 *                              index = formulaFuncPtr(devObjPtr,memCunkPtr,address);
 *      SMEM_INDEX_IN_CHUNK_TYPE_IGNORED_E - ignore this entry in the memory !
 *                              do not care about the index.
 */
typedef enum{
    SMEM_INDEX_IN_CHUNK_TYPE_STRAIGHT_ACCESS_E,
    SMEM_INDEX_IN_CHUNK_TYPE_MASK_AND_SHIFT_E,
    SMEM_INDEX_IN_CHUNK_TYPE_FORMULA_FUNCTION_E,

    SMEM_INDEX_IN_CHUNK_TYPE_IGNORED_E,
}SMEM_INDEX_IN_CHUNK_TYPE_ENT;

/*
 * Typedef: struct SMEM_FORMULA_DIM_STC
 *
 * Description:
 *      Describes the memory chunk formula expression
 *
 * Fields:
 *      size    - number of registers
 *      step    - memory offset in bytes
 */
typedef struct{
    GT_U32 size;
    GT_U32 step;
}SMEM_FORMULA_DIM_STC;

/*
 * Typedef: struct GT_MEM_CHUNK_STC
 *
 * Description:
 *      Describes the memory chunks , that represent the memories of the device
 *
 * Fields:
 *
 */
typedef struct SMEM_CHUNK_STCT{
    GT_U32  memFirstAddr;/* the first address this chunk represents */
    GT_U32  memLastAddr;/* the last address this chunk represents (this address is still in the chunk)*/
    GT_U32  *memPtr;/*pointer to dynamically allocated memory */
    GT_U32  memSize;/*size of dynamically allocated memory in words */

    GT_U32  enrtyNumWordsAlignement;    /* if not zero - memory entry alignment in words(used for direct memory write access by last word) */
    GT_U32  lastEntryWordIndex; /* Last word index in entry (used for direct memory write access by last word) */

    GT_U32  tableOffsetInBytes;/* table offset (in bytes)-- offset from the tables memory place holder.
                           for example : the DXCH devices hold table DB of :
                           devObjPtr->tablesInfo.<table_name>
                           So the offset is from the first table : meaning from
                           (GT_UINTPTR)((GT_U32*)&(devObjPtr->tablesInfo))

                           if value == SMAIN_NOT_VALID_CNS --> not used
                           */

    /* parameters to get index in the memPtr[] */
    SMEM_INDEX_IN_CHUNK_TYPE_ENT calcIndexType;/* how to calculate the index in the memPtr[] */

    /* info for the 'Mask and shift' index calculation */
    GT_U32  memMask;/* mask to set on the address */
    GT_U32  rightShiftNumBits;/* after applying the mask , how many bits to shift to get the index */

    /* info for the 'Formula' index calculation */
    SMEM_FORMULA_CHUNK_FUNC formulaFuncPtr;/* pointer to formula to access the chunk
                                              when NULL --> straight access  */
    SMEM_FORMULA_DIM_STC formulaData[SMEM_FORMULA_CELL_NUM_CNS];   /* memory chunk access formula */
}SMEM_CHUNK_STC;

typedef enum{
     SMEM_UNIT_CHUNK_TYPE_9_MSB_E/*unit support relative address of 23 LSB*/
    ,SMEM_UNIT_CHUNK_TYPE_8_MSB_E/*unit support relative address of 24 LSB*/
    ,SMEM_UNIT_CHUNK_TYPE_23_MSB_AFTER_20_LSB_E/*unit support relative address of 20 LSB in addresses of 43bits */
}SMEM_UNIT_CHUNK_TYPE_ENT;



typedef struct{
    GT_U32  memFirstAddr;/* the first address this chunk represents */
    GT_U32  numOfRegisters;/* the number of registers to hold */
    GT_U32  enrtySizeBits; /* entry size in bits(used for direct memory write access by last word) */
    GT_U32  enrtyNumBytesAlignement;    /* if not zero - memory entry alignment in bytes(used for indirect memory access) */
    GT_BIT  tableOffsetValid;/* is the 'tableOffset' field valid */
    GT_U32  tableOffsetInBytes;/* table offset (in bytes)-- offset from the tables memory place holder.
                           for example : the DXCH devices hold table DB of :
                           devObjPtr->tablesInfo.<table_name>
                           So the offset is from the first table : meaning from
                           (GT_UINTPTR)((GT_U32*)&(devObjPtr->tablesInfo))
                           */
}SMEM_CHUNK_BASIC_STC;

/*
 * Typedef: struct SMEM_CHUNK_EXTENDED_STC
 *
 * Description:
 *      Describes the memory chunks, that represents the memories of the device
 *      and access to it - straight and by formula
 *
 * Fields:
 *
 */
typedef struct{
    SMEM_CHUNK_BASIC_STC memChunkBasic;                 /* basic memory chunk info */
    SMEM_FORMULA_DIM_STC formulaCellArr[SMEM_FORMULA_CELL_NUM_CNS]; /* memory chunk access formula */
}SMEM_CHUNK_EXTENDED_STC;


/* Typedef of SMEM register */
typedef GT_U32 SMEM_REGISTER;

/* Typedef of SMEM PHY register */
typedef GT_U16 SMEM_PHY_REGISTER;

/* Typedef of wide SRAM */
typedef GT_U32 SMEM_WSRAM;

/* Typedef of narrow SRAM */
typedef GT_U32 SMEM_NSRAM;

/* Typedef of flow DRAM */
typedef GT_U32 SMEM_FDRAM;

/* Check memory bounds */
#define CHECK_MEM_BOUNDS(ptr, arraySize, index, size)                           \
    if((index + size) > (arraySize)) {                                          \
        if (skernelUserDebugInfo.disableFatalError == GT_FALSE)                 \
        {                                                                       \
            skernelFatalError("CHECK_MEM_BOUNDS: address[0x%8.8x]\
                                    index or memory size is out of range\n",address);   \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            return 0;                                                           \
        }                                                                       \
    }

/* Return the mask including "numOfBits" bits.          */
#define SMEM_BIT_MASK(numOfBits) \
   (((numOfBits) == 32) ? 0xFFFFFFFF : (~(0xFFFFFFFF << (numOfBits))))

/* Calculate the field mask for a given offset & length */
/* e.g.: BIT_MASK(8,2) = 0xFFFFFCFF                     */
#define SMEM_FIELD_MASK_NOT(offset,len)                      \
        (~(SMEM_BIT_MASK((len)) << (offset)))

/* Calculate the field mask for a given offset & length */
/* e.g.: BIT_MASK(8,2) = 0x00000300                     */
#define SMEM_FIELD_MASK(offset,len)                      \
        ( (SMEM_BIT_MASK((len)) << (offset)) )

/*
    NOTE: see also function snetFieldValueGet
*/
/* Returns the info located at the specified offset & length in data.   */
#define SMEM_U32_GET_FIELD(data,offset,length)           \
        (((data) >> (offset)) & SMEM_BIT_MASK(length))

/*
    NOTE: see also function snetFieldValueSet
*/
/* Sets the field located at the specified offset & length in data.     */
#define SMEM_U32_SET_FIELD(data,offset,length,val)           \
(data) = (((data) & SMEM_FIELD_MASK_NOT((offset),(length))) | \
                  (((val) & SMEM_BIT_MASK(length)) << (offset)))

/* all bits mask */
#define SMEM_FULL_MASK_CNS 0xFFFFFFFF

/* macro to determine if a read/write need to be done in the Skernel level */
#define SMEM_SKIP_LOCAL_READ_WRITE \
    ((sasicgSimulationRoleIsDevices == GT_TRUE || \
      sasicgSimulationRole == SASICG_SIMULATION_ROLE_NON_DISTRIBUTED_E) ? 0 : 1)

/*******************************************************************************
*  SMEM_ACTIVE_MEM_READ_FUN
*
* DESCRIPTION:
*      Definition of the Active register read function.
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memPtr      - Pointer to the register's memory in the simulation.
*       param       - Registers' specific parameter.
*
* OUTPUTS:
*       outMemPtr   - Pointer to the memory to copy register's content.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
typedef void (* SMEM_ACTIVE_MEM_READ_FUN ) (
                              IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
                              IN         GT_U32   address,
                              IN         GT_U32   memSize,
                              IN         GT_U32 * memPtr,
                              IN         GT_UINTPTR param,
                              OUT        GT_U32 * outMemPtr);
/*******************************************************************************
*  SMEM_ACTIVE_MEM_WRITE_FUN
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
typedef void (* SMEM_ACTIVE_MEM_WRITE_FUN ) (
                              IN         SKERNEL_DEVICE_OBJECT * deviceObjPtr,
                              IN         GT_U32   address,
                              IN         GT_U32   memSize,
                              IN         GT_U32 * memPtr,
                              IN         GT_UINTPTR param,
                              IN         GT_U32 * inMemPtr);

/*
 * Typedef: struct SMEM_ACTIVE_MEM_ENTRY_STC
 *
 * Description:
 *      Describe the entry in the active memory table.
 *
 * Fields:
 *      address         : Address of the register.
 *      mask            : Mask to apply to an address.
 *      readFun         : Entry point of read active memory function.
 *      readFunParam    : Parameter for readFun function.
 *      writeFun        : Entry point of write active memory function.
 *      writeFunParam   : Parameter for writeFun function.
 * Comments:
 */

typedef struct SMEM_ACTIVE_MEM_ENTRY_T {
    GT_U32                    address;
    GT_U32                    mask;
    SMEM_ACTIVE_MEM_READ_FUN  readFun;
    GT_U32                    readFunParam;
    SMEM_ACTIVE_MEM_WRITE_FUN writeFun;
    GT_U32                    writeFunParam;
} SMEM_ACTIVE_MEM_ENTRY_STC;
/* last line in every active memory table */
#define  SMEM_ACTIVE_MEM_ENTRY_LAST_LINE_CNS    \
        /* must be last anyway */               \
        {END_OF_TABLE, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}

#define START_BIND_UNIT_ACTIVE_MEM_MAC(_devObjPtr,_unitPtr) \
    static SMEM_ACTIVE_MEM_ENTRY_STC unitActiveTable[] =    \
    {

#define END_BIND_UNIT_ACTIVE_MEM_MAC(_devObjPtr,_unitPtr) \
        /* last line in every active memory table */      \
        SMEM_ACTIVE_MEM_ENTRY_LAST_LINE_CNS               \
    };/*end of unitActiveTable */                         \
    _unitPtr->unitActiveMemPtr = unitActiveTable;

/* global active memory for :
    no active memory to the unit.
   and no need to look all over the active memory of the device */
extern SMEM_ACTIVE_MEM_ENTRY_STC smemEmptyActiveMemoryArr[];

/* chunks of memory for a unit */
typedef struct SMEM_UNIT_CHUNKS_STCT{
    GT_U32          chunkIndex; /* chunk unit base index (chunkBase << 23 is base address) */
    SMEM_CHUNK_STC *chunksArray;/* array of chunks */
    GT_U32          numOfChunks;/* number of chunks in the array */

    struct SMEM_UNIT_CHUNKS_STCT *hugeUnitSupportPtr;/* (when not NULL) pointer to a huge sub unit that also includes this sub unit ,
                                                        and is already allocated all it's registers/memories */
    void*           otherPortGroupDevObjPtr;/*(when not NULL) pointer to device object of the actual device that hold the memory
                                            supporting 'shared memory' between port groups */

    SMEM_UNIT_CHUNK_TYPE_ENT    chunkType;
    GT_U32                      numOfUnits;/*number of Consecutive units (for huge units)*/
    SMEM_ACTIVE_MEM_ENTRY_STC   *unitActiveMemPtr;/* active memory dedicated to the unit */

    SMEM_LINK_LIST_REGISTER_DEFAULT_VALUE_STC   *unitDefaultRegistersPtr;/* default register values for the unit */
}SMEM_UNIT_CHUNKS_STC;
/*
 * Typedef: struct SMEM_ACTIVE_MEM_ENTRY_REPLACEMENT_STC
 *
 * Description:
 *      Describe the address in the active memory that should be replaced with
 *      another entry info.
 *
 * Fields:
 *      address         : Address in the table that should be found and replaced
 *      newEntry        : info about the replacing entry
 * Comments:
 *      this used for example for xCat that wants to use most info from ch3 but
 *      still to 'override' some of it
 *
*/
typedef struct{
    GT_U32                      oldAddress;
    SMEM_ACTIVE_MEM_ENTRY_STC   newEntry;
}SMEM_ACTIVE_MEM_ENTRY_REPLACEMENT_STC;

/*******************************************************************************
*  SMEM_SPEC_MEMORY_FIND_FUN
*
* DESCRIPTION:
*      Definition of memory address type specific search function of SKERNEL.
* INPUTS:
*       deviceObjPtr - device object PTR.
*       address     - Address for ASIC memory.
*       memSize     - Size of ASIC memory to read or write.
*       param       - Function specific parameter.
*
* OUTPUTS:
*       None
* RETURNS:
*                pointer to the memory location in the database
*       NULL - if address not exist, or memSize > than existed size
*
* COMMENTS:
*
*******************************************************************************/
typedef GT_U32 * (* SMEM_SPEC_MEMORY_FIND_FUN ) (
                        IN SKERNEL_DEVICE_OBJECT *  deviceObjPtr,
                        IN SCIB_MEMORY_ACCESS_TYPE  accessType,
                        IN GT_U32                   address,
                        IN GT_U32                   memSize,
                        IN GT_UINTPTR               param);

/*******************************************************************************
*  SMEM_DEV_MEMORY_FIND_FUN
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
*       activeMemPtrPtr - pointer to the active memory entry or NULL if not
*                         exist.
* RETURNS:
*       pointer to the memory location in the database
*       NULL - if address not exist, or memSize > than existed size
*
* COMMENTS:
*
*******************************************************************************/
typedef void * (* SMEM_DEV_MEMORY_FIND_FUN ) (
                    IN struct SKERNEL_DEVICE_OBJECT_T *     deviceObjPtr,
                    IN SCIB_MEMORY_ACCESS_TYPE              accessType,
                    IN GT_U32                               address,
                    IN GT_U32                               memSize,
                    OUT struct SMEM_ACTIVE_MEM_ENTRY_T **   activeMemPtrPtr);


/*
 * Typedef: struct SMEM_SPEC_FUN_ENTRY_STC
 *
 * Description:
 *      Describe the entry in the address type specific R/W functions table.
 *
 * Fields:
 *      specFun         : Entry point of the function.
 *      specParam       : Additional address type specific parameter specFun .
 *      unitActiveMemPtr: pointer to active memory of the unit.
 *                        when != NULL , using ONLY this active memory.
 *                        when NULL , using the active memory of the device.
 * Comments:
 */

typedef struct  {
    SMEM_SPEC_MEMORY_FIND_FUN   specFun;
    GT_UINTPTR                  specParam;
    SMEM_ACTIVE_MEM_ENTRY_STC   *unitActiveMemPtr;
} SMEM_SPEC_FIND_FUN_ENTRY_STC;

/*******************************************************************************
 * Typedef: enum SMEM_UNIT_PCI_BUS_ENT
 *
 * Description:
 *      Units that can be used for lookup on PCI (bus) or other external memory space
 *      before accessing the memory of the device
 *
 * Fields:
 *      SMEM_UNIT_PCI_BUS_MBUS_E - MBUS external memory
 *      SMEM_UNIT_PCI_BUS_DFX_E - DFX server external memory
 *
 *******************************************************************************/
typedef enum{
    SMEM_UNIT_PCI_BUS_MBUS_E,
    SMEM_UNIT_PCI_BUS_DFX_E,
    SMEM_UNIT_PCI_BUS_UNIT_LAST_E
}SMEM_UNIT_PCI_BUS_ENT;

/*******************************************************************************
 * Typedef: struct SMEM_UNIT_CHUNK_BASE_ADDRESS_STC
 *
 * Description:
 *      Unit chunks memory structure
 *
 *******************************************************************************/
typedef struct{
    SMEM_UNIT_CHUNKS_STC            unitMem;
    GT_U32                          unitBaseAddr;
} SMEM_UNIT_CHUNK_BASE_ADDRESS_STC;

/*******************************************************************************
*   smemInit
*
* DESCRIPTION:
*       Init memory module for a device.
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
void smemInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);


/*******************************************************************************
*   smemInit2
*
* DESCRIPTION:
*       Init memory module for a device - after the load of the default
*           registers file
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
void smemInit2
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);

/*******************************************************************************
*   smemRegSet
*
* DESCRIPTION:
*       Write data to the register.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of register.
*       data        - new register's value.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*          The function aborts application if address not exist.
*
*******************************************************************************/
void smemRegSet
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj,
    IN GT_U32                  address,
    IN GT_U32                  data
);
/*******************************************************************************
*   smemRegGet
*
* DESCRIPTION:
*       Read data from the register.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of register.
*
* OUTPUTS:
*       data        - pointer to register's value.
*
* RETURNS:
*
* COMMENTS:
*          The function aborts application if address not exist.
*
*******************************************************************************/
void smemRegGet
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  address,
    OUT GT_U32                * data
);

/*******************************************************************************
*   smemMemGet
*
* DESCRIPTION:
*       Return pointer to the register's or tables's memory.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*
* OUTPUTS:
*       None.
*
* RETURNS:
*        pointer to the memory location
*
* COMMENTS:
*      The function aborts application if address not exist.
*
*******************************************************************************/
void * smemMemGet
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  address
);

/*******************************************************************************
*   smemMemSet
*
* DESCRIPTION:
*       Write data to a register or table.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       dataPtr     - new data location for memory .
*       dataSize    - size of memory location to be updated.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*      The function aborts application if address not exist.
*
*******************************************************************************/
void smemMemSet
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj,
    IN GT_U32                  address,
    IN GT_U32        *         dataPtr,
    IN GT_U32                  dataSize
);

/*******************************************************************************
*   smemRegFldGet
*
* DESCRIPTION:
*       Read data from the register's field
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of register.
*       fldFirstBit - field first Bit
*       fldSize     - field size
*       data        - pointer to register's field value.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*          The function aborts application if address not exist.
*
*******************************************************************************/
void smemRegFldGet
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  address,
    IN GT_U32                  fldFirstBit,
    IN GT_U32                  fldSize,
    OUT GT_U32               * data
);
/*******************************************************************************
*   smemPciRegFldGet
*
* DESCRIPTION:
*       Read data from the PCI register's field
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of register.
*       fldFirstBit - field first Bit
*       fldSize     - field size
*       data        - pointer to register's field value.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*          The function aborts application if address not exist.
*
*******************************************************************************/
void smemPciRegFldGet
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  address,
    IN GT_U32                  fldFirstBit,
    IN GT_U32                  fldSize,
    OUT GT_U32               * data
);
/*******************************************************************************
*   smemRegFldSet
*
* DESCRIPTION:
*       Write data to the register's field
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of register.
*       fldFirstBit - field first Bit
*       fldSize     - field size
*       data        - pointer to register's field value.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*          The function aborts application if address not exist.
*
*******************************************************************************/
void smemRegFldSet
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  address,
    IN GT_U32                  fldFirstBit,
    IN GT_U32                  fldSize,
    IN GT_U32                  data
);

/*******************************************************************************
*   smemPciRegFldSet
*
* DESCRIPTION:
*       Write data to the PCI register's field
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of register.
*       fldFirstBit - field first Bit
*       fldSize     - field size
*       data        - pointer to register's field value.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*          The function aborts application if address not exist.
*
*******************************************************************************/
void smemPciRegFldSet
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  address,
    IN GT_U32                  fldFirstBit,
    IN GT_U32                  fldSize,
    IN GT_U32                  data
);

/*******************************************************************************
*   smemPciRegSet
*
* DESCRIPTION:
*       Write data to the PCI register's
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of register.
*       data        - pointer to register's field value.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*          The function aborts application if address not exist.
*
*******************************************************************************/
void smemPciRegSet
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  address,
    IN GT_U32                  data
);

/*******************************************************************************
*   smemRmuReadWriteMem
*
* DESCRIPTION:
*       Read/Write data to/from a register or table.
*
* INPUTS:
*       accessType  - define access operation Read or Write.
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       dataPtr     - new data location for memory .
*       dataSize    - size of memory location to be updated.
*       memPtr      - for Write this pointer to application memory,which
*                     will be copied to the ASIC memory .
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*      The function aborts application if address not exist.
*
*******************************************************************************/
void  smemRmuReadWriteMem
(
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN void * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    INOUT GT_U32 * memPtr
);

/*******************************************************************************
*   smemReadWriteMem
*
* DESCRIPTION:
*       Read/Write data to/from a register or table.
*
* INPUTS:
*       accessType  - define access operation Read or Write.
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       dataPtr     - new data location for memory .
*       dataSize    - size of memory location to be updated.
*       memPtr      - for Write this pointer to application memory,which
*                     will be copied to the ASIC memory .
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*      The function aborts application if address not exist.
*
*******************************************************************************/
void  smemReadWriteMem
(
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN void * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    INOUT GT_U32 * memPtr
);


/*******************************************************************************
*  smemInitMemChunk__internal
*
* DESCRIPTION:
*      init unit memory chunk
*
* INPUTS:
*       chunksMemArr[] - array of basic memory chunks info
*       numOfChunks  - number of basic chunks
*       fileNamePtr -  file name that called the allocation
*       line        -  line in the file that called the allocation
*
* OUTPUTS:
*       unitChunksPtr  - (pointer to) the unit memories chunks
*
* RETURNS:
*        none
*
* COMMENTS:
*
*******************************************************************************/
void smemInitMemChunk__internal
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SMEM_CHUNK_BASIC_STC  basicMemChunksArr[],
    IN GT_U32           numOfChunks,
    OUT SMEM_UNIT_CHUNKS_STC  *unitChunksPtr,
    IN const char*     fileNamePtr,
    IN GT_U32          line
);

#define smemInitMemChunk(devObjPtr,basicMemChunksArr,numOfChunks,unitChunksPtr) \
        smemInitMemChunk__internal(devObjPtr,basicMemChunksArr,numOfChunks,unitChunksPtr,__FILE__,__LINE__)

/*******************************************************************************
*  smemInitMemChunkExt__internal
*
* DESCRIPTION:
*      Allocates and init unit memory at flat model and using formula
*
* INPUTS:
*       chunksMemArrExt[] - array of extended memory chunks info
*       numOfChunks  - number of extended chunks
*       fileNamePtr -  file name that called the allocation
*       line        -  line in the file that called the allocation
*
* OUTPUTS:
*       unitChunksPtr  - (pointer to) the unit memories chunks
*
* RETURNS:
*        none
*
* COMMENTS:
*
*******************************************************************************/
void smemInitMemChunkExt__internal
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SMEM_CHUNK_EXTENDED_STC  chunksMemArrExt[],
    IN GT_U32                   numOfChunks,
    OUT SMEM_UNIT_CHUNKS_STC    *unitChunksPtr,
    IN const char*     fileNamePtr,
    IN GT_U32          line
);
#define smemInitMemChunkExt(devObjPtr,chunksMemArrExt,numOfChunks,unitChunksPtr)    \
        smemInitMemChunkExt__internal(devObjPtr,chunksMemArrExt,numOfChunks,unitChunksPtr,__FILE__,__LINE__)

/*******************************************************************************
*  smemInitMemCombineUnitChunks__internal
*
* DESCRIPTION:
*      takes 2 unit chunks and combine them into first unit.
*      NOTEs:
*       1. the function will re-allocate memories for the combined unit
*       2. all the memories of the second unit chunks will be free/reused by
*          first unit after the action done. so don't use those memories afterwards.
*       3. the 'second' memory will have priority over the memory of the 'first' memory.
*          meaning that if address is searched and it may be found in 'first' and
*          in 'second' , then the 'second' memory is searched first !
*          this is to allow 'override' of memories from the 'first' by memories
*          of the 'second'
*
* INPUTS:
*       firstUnitChunks[] - (pointer to) the first unit memories chunks
*       secondUnitChunks[] - (pointer to) the second unit memories chunks
*       fileNamePtr -  file name that called the allocation
*       line        -  line in the file that called the allocation
*
* OUTPUTS:
*       none
*
* RETURNS:
*        none
*
* COMMENTS:
*
*******************************************************************************/
void smemInitMemCombineUnitChunks__internal
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SMEM_UNIT_CHUNKS_STC     *firstUnitChunks,
    IN SMEM_UNIT_CHUNKS_STC     *secondUnitChunks,
    IN const char*     fileNamePtr,
    IN GT_U32          line
);

#define smemInitMemCombineUnitChunks(devObjPtr,firstUnitChunks,secondUnitChunks) \
        smemInitMemCombineUnitChunks__internal(devObjPtr,firstUnitChunks,secondUnitChunks,__FILE__,__LINE__)

/*******************************************************************************
*  smemInitMemUpdateUnitChunks__internal
*
* DESCRIPTION:
*      takes 2 unit chunks and combine them into first unit.
*      NOTEs:
*       1. Find all duplicated chunks in destination chunks array and reallocate
*          them according to appended chunk info.
*       2. The function will re-allocate memories for the combined unit
*       3. All the memories of the "append" unit chunks will be free/reused by
*          first unit after the action done. so don't use those memories afterwards.
*
* INPUTS:
*       unitChunksAppendToPtr[] - (pointer to) the destination unit memories chunks
*       unitChunksAppendPtr[] - (pointer to) the unit memories chunks to be appended
*       fileNamePtr -  file name that called the allocation
*       line        -  line in the file that called the allocation
*
* OUTPUTS:
*       none
*
* RETURNS:
*        none
*
* COMMENTS:
*
*******************************************************************************/
void smemInitMemUpdateUnitChunks__internal
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SMEM_UNIT_CHUNKS_STC     *unitChunksAppendToPtr,
    IN SMEM_UNIT_CHUNKS_STC     *unitChunksAppendPtr,
    IN const char*     fileNamePtr,
    IN GT_U32          line
);
#define smemInitMemUpdateUnitChunks(devObjPtr,unitChunksAppendToPtr,unitChunksAppendPtr) \
        smemInitMemUpdateUnitChunks__internal(devObjPtr,unitChunksAppendToPtr,unitChunksAppendPtr,__FILE__,__LINE__)


/*******************************************************************************
*  smemUnitChunkAddBasicChunk__internal
*
* DESCRIPTION:
*      add basic chunk to unit chunk.
*      NOTEs:
*
* INPUTS:
*       unitChunkPtr - (pointer to) unit memories chunk
*       basicChunkPtr - (pointer to) basic chunk
*       numBasicElements - number of elements in the basic chunk
*       fileNamePtr -  file name that called the allocation
*       line        -  line in the file that called the allocation
*
* OUTPUTS:
*       none
*
* RETURNS:
*        none
*
* COMMENTS:
*
*******************************************************************************/
void smemUnitChunkAddBasicChunk__internal
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SMEM_UNIT_CHUNKS_STC     *unitChunkPtr,
    IN SMEM_CHUNK_BASIC_STC     *basicChunkPtr,
    IN GT_U32                   numBasicElements,
    IN const char*     fileNamePtr,
    IN GT_U32          line
);

#define smemUnitChunkAddBasicChunk(devObjPtr,unitChunkPtr,basicChunkPtr_and_numBasicElements)    \
        smemUnitChunkAddBasicChunk__internal(devObjPtr,unitChunkPtr,basicChunkPtr_and_numBasicElements,__FILE__,__LINE__)


/*******************************************************************************
* smemTestDeviceIdToDevPtrConvert
*
* DESCRIPTION:
*        test utility -- convert device Id to device pointer
*
* INPUTS:
*       deviceId       - device id to refer to
*
* OUTPUTS:
*
* RETURN:
*       the device pointer
* COMMENTS:
*   if not found --> fatal error
*
*******************************************************************************/
SKERNEL_DEVICE_OBJECT* smemTestDeviceIdToDevPtrConvert
(
    IN  GT_U32                      deviceId
);

/*******************************************************************************
* genericTablesTestValidity
*
* DESCRIPTION:
*        test utility -- check that all the general table base addresses exists
*                        in the device memory
*
* INPUTS:
*    devNum - the device index
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32  genericTablesTestValidity(
    IN GT_U32 devNum
);

/*******************************************************************************
* smemTestDevice
*
* DESCRIPTION:
*        test utility -- function to test memory of device
*
* INPUTS:
*    devNum - the device index
*    registersAddrPtr - array of addresses of registers
*    numRegisters - number of registers
*    tablesInfoPtr - array of tables info
*    numTables - number of tables
*    testActiveMemory - to test (or not) the addresses of the active memory to
*                       see if exists
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
void   smemTestDevice
(
    IN GT_U32               devNum,
    IN GT_U32               *registersAddrPtr,
    IN GT_U32               numRegisters,
    IN DEV_TABLE_INFO_STC   *tablesInfoPtr,
    IN GT_U32               numTables,
    IN GT_BOOL              testActiveMemory
);

/*******************************************************************************
*   smemDebugModeByPassFatalErrorOnMemNotExists
*
* DESCRIPTION:
*   debug utill - set a flag to allow to by pass the fatal error on accessing to
*   non-exists memory. this allow to use test of memory without the need to stop
*   on every memory .
*   we can get a list of all registers at once.
*
* INPUTS:
*
*
* OUTPUTS:
*     byPassFatalErrorOnMemNotExists - bypass or no the fatal error on accessing
*     to non exists memory
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
void smemDebugModeByPassFatalErrorOnMemNotExists(
    IN GT_U32 byPassFatalErrorOnMemNotExists
);


/*******************************************************************************
*   smemTableEntryAddressGet
*
* DESCRIPTION:
*   function return the address of the entry in the table.
*       NOTE: no checking on the out of range violation
*
* INPUTS:
*       tablePtr - pointer to table info
*       index - index of the entry in the table
*       instanceIndex - index of instance. for multi-instance support.
*                   ignored when == SMAIN_NOT_VALID_CNS
* OUTPUTS:
*
*
* RETURNS:
*        the address of the specified entry in the specified table
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32  smemTableEntryAddressGet(
    IN  SKERNEL_TABLE_INFO_STC    *tablePtr,
    IN  GT_U32                    index,
    IN  GT_U32                    instanceIndex
);

/*******************************************************************************
*   smemTableEntry2ParamsAddressGet
*
* DESCRIPTION:
*   function return the address of the entry in the table.
*       NOTE: no checking on the out of range violation
*
* INPUTS:
*       tablePtr - pointer to table info
*       index1 - index1 of the entry in the table
*       index2 - index2 of the entry in the table
*       instanceIndex - index of instance. for multi-instance support.
*                   ignored when == SMAIN_NOT_VALID_CNS
* OUTPUTS:
*
*
* RETURNS:
*        the address of the specified entry in the specified table
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32  smemTableEntry2ParamsAddressGet(
    IN  SKERNEL_TABLE_2_PARAMETERS_INFO_STC    *tablePtr,
    IN  GT_U32                    index1,
    IN  GT_U32                    index2,
    IN  GT_U32                    instanceIndex
);

/*******************************************************************************
*   smemTableEntry3ParamsAddressGet
*
* DESCRIPTION:
*   function return the address of the entry in the table.
*       NOTE: no checking on the out of range violation
*
* INPUTS:
*       tablePtr - pointer to table info
*       index1 - index1 of the entry in the table
*       index2 - index2 of the entry in the table
*       index3 - index3 of the entry in the table
*       instanceIndex - index of instance. for multi-instance support.
*                   ignored when == SMAIN_NOT_VALID_CNS
* OUTPUTS:
*
*
* RETURNS:
*        the address of the specified entry in the specified table
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32  smemTableEntry3ParamsAddressGet(
    IN  SKERNEL_TABLE_3_PARAMETERS_INFO_STC    *tablePtr,
    IN  GT_U32                    index1,
    IN  GT_U32                    index2,
    IN  GT_U32                    index3,
    IN  GT_U32                    instanceIndex
);

/*******************************************************************************
*   smemTableEntry4ParamsAddressGet
*
* DESCRIPTION:
*   function return the address of the entry in the table.
*       NOTE: no checking on the out of range violation
*
* INPUTS:
*       tablePtr - pointer to table info
*       index1 - index1 of the entry in the table
*       index2 - index2 of the entry in the table
*       index3 - index3 of the entry in the table
*       index4 - index4 of the entry in the table
*       instanceIndex - index of instance. for multi-instance support.
*                   ignored when == SMAIN_NOT_VALID_CNS
* OUTPUTS:
*
*
* RETURNS:
*        the address of the specified entry in the specified table
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32  smemTableEntry4ParamsAddressGet(
    IN  SKERNEL_TABLE_4_PARAMETERS_INFO_STC    *tablePtr,
    IN  GT_U32                    index1,
    IN  GT_U32                    index2,
    IN  GT_U32                    index3,
    IN  GT_U32                    index4,
    IN  GT_U32                    instanceIndex
);

/*******************************************************************************
*   smemUnitChunksReset
*
* DESCRIPTION:
*       Reset memory chunks by zeroes for Cheetah3 and above devices.
*
* INPUTS:
*       devObjPtr - pointer to device object
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
void smemUnitChunksReset
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr
);

/*******************************************************************************
*   smemDevMemoryCalloc
*
* DESCRIPTION:
*       Allocate device memory and store reference to allocations into internal array.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       sizeObjCount    - count of objects to be allocated
*       sizeObjSizeof   - size of object to be allocated
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Pointer to allocated memory
*
* COMMENTS:
*       The function calls standard calloc function.
*       Internal data base will hold reference for all allocations
*       for future use in SW reset
*
*******************************************************************************/
void * smemDevMemoryCalloc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 sizeObjCount,
    IN GT_U32 sizeObjSizeof
);

/*******************************************************************************
*   smemDevFindInUnitChunk
*
* DESCRIPTION:
*       find the memory in the specified unit chunk
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       unitChunksPtr - pointer to the unit chunk
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*       param       - extra parameter --> used as pointer to the memory unit chunk
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
GT_U32 *  smemDevFindInUnitChunk
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
);

/*******************************************************************************
*   smemFindCommonMemory
*
* DESCRIPTION:
*       Return pointer to the register's common memory.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       address     - address of memory(register or table).
*       memSize     - size of the requested memory
*
* OUTPUTS:
*       activeMemPtrPtr - (pointer to) the active memory entry or NULL if not
*                         exist.
*                         (if activeMemPtrPtr == NULL ignored)
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*******************************************************************************/
void * smemFindCommonMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    OUT struct SMEM_ACTIVE_MEM_ENTRY_T **   activeMemPtrPtr
);

/*******************************************************************************
*   smemRegUpdateAfterRegFile
*
* DESCRIPTION:
*       The function read the register and write it back , so active memory can
*       be activated on this address after loading register file.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*       dataSize    - number of words to update
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemRegUpdateAfterRegFile
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  address,
    IN GT_U32                  dataSize
);


/*******************************************************************************
*   smemBindTablesToMemories
*
* DESCRIPTION:
*       bind tables from devObjPtr->tablesInfo.<tableName> to memories specified
*       in chunksPtr
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       chunksPtr   - pointer to array of chunks
*       numOfUnits  - number of chunks in chunksPtr
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemBindTablesToMemories(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMEM_UNIT_CHUNKS_STC     *chunksPtr,
    IN GT_U32                   numOfUnits
);

/*******************************************************************************
* smemGenericTablesSectionsGet
*
* DESCRIPTION:
*        get the number of tables from each section and the pointer to the sections:
*       section of '1 parameter' tables
*       section of '2 parameters' tables
*       section of '3 parameters' tables
*       section of '4 parameters' tables
*
* INPUTS:
*       deviceObj   - pointer to device object.
* OUTPUTS:
*       tablesPtr   - pointer to tables with single parameter
*       numTablesPtr -  number of tables with single parameter
*       tables2ParamsPtr - pointer to tables with 2 parameters
*       numTables2ParamsPtr -number of tables with 2 parameters
*       tables3ParamsPtr - pointer to tables with 3 parameters
*       numTables3ParamsPtr - number of tables with 3 parameters
*       tables4ParamsPtr - pointer to tables with 4 parameters
*       numTables4ParamsPtr - number of tables with 4 parameters
*
*
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
void  smemGenericTablesSectionsGet(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    OUT SKERNEL_TABLE_INFO_STC  **tablesPtr,
    OUT GT_U32  *numTablesPtr,
    OUT SKERNEL_TABLE_2_PARAMETERS_INFO_STC  **tables2ParamsPtr,
    OUT GT_U32  *numTables2ParamsPtr,
    OUT SKERNEL_TABLE_3_PARAMETERS_INFO_STC  **tables3ParamsPtr,
    OUT GT_U32  *numTables3ParamsPtr,
    OUT SKERNEL_TABLE_4_PARAMETERS_INFO_STC  **tables4ParamsPtr,
    OUT GT_U32  *numTables4ParamsPtr
);

/*******************************************************************************
*   smemGenericUnitAddressesAlignToBaseAddress
*
* DESCRIPTION:
*       align all addresses of the unit according to base address of the unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitInfoPtr - pointer to the relevant unit - before alignment of addresses.
*
* OUTPUTS:
*       unitInfoPtr - pointer to the relevant unit - after alignment of addresses.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemGenericUnitAddressesAlignToBaseAddress(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC *unitInfoPtr
);

/*******************************************************************************
*   smemGenericUnitAddressesAlignToBaseAddress1
*
* DESCRIPTION:
*       align all addresses of the unit according to base address of the unit
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       unitInfoPtr - pointer to the relevant unit - before alignment of addresses.
*       isZeroBased - GT_FALSE - the same as function smemGenericUnitAddressesAlignToBaseAddress
*                     GT_TRUE  - the addresses in unitInfoPtr are '0' based and
*                       need to add to all of then the unit base address from unitInfoPtr->chunkIndex
*
* OUTPUTS:
*       unitInfoPtr - pointer to the relevant unit - after alignment of addresses.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemGenericUnitAddressesAlignToBaseAddress1(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SMEM_UNIT_CHUNKS_STC *unitInfoPtr,
    IN    GT_BOOL               isZeroBased
);
/*******************************************************************************
*   smemGenericRegistersArrayAlignForceUnitReset
*
* DESCRIPTION:
*       indication that addresses will be overriding the previous address without
*       checking that address belong to unit.
*
* INPUTS:
*       forceReset - indication to force override
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemGenericRegistersArrayAlignForceUnitReset(
    IN    GT_BOOL               forceReset
);

/*******************************************************************************
*   smemGenericRegistersArrayAlignToUnit
*
* DESCRIPTION:
*       align all addresses of the array of registers according to
*       to base address of the unit
*
* INPUTS:
*       registersArr - array of registers
*       numOfRegisters - number of registers in registersArr
*       unitInfoPtr - pointer to the relevant unit .
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemGenericRegistersArrayAlignToUnit(
    IN GT_U32               registersArr[],
    IN GT_U32               numOfRegisters,
    IN SMEM_UNIT_CHUNKS_STC *unitInfoPtr
);

/*******************************************************************************
*   smemGenericRegistersArrayAlignToUnit1
*
* DESCRIPTION:
*       align all addresses of the array of registers according to
*       to base address of the unit
*
* INPUTS:
*       registersArr - array of registers
*       numOfRegisters - number of registers in registersArr
*       unitInfoPtr - pointer to the relevant unit .
*       isZeroBased - GT_FALSE - the same as function smemGenericRegistersArrayAlignToUnit
*                     GT_TRUE  - the addresses in unitInfoPtr are '0' based and
*                       need to add to all of then the unit base address from unitInfoPtr->chunkIndex
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemGenericRegistersArrayAlignToUnit1(
    IN GT_U32               registersArr[],
    IN GT_U32               numOfRegisters,
    IN SMEM_UNIT_CHUNKS_STC *unitInfoPtr,
    IN    GT_BOOL               isZeroBased
);

/*******************************************************************************
*   smemGenericFindMem
*
* DESCRIPTION:
*       Return pointer to the register's or tables's memory.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       accessType  - Read/Write operation
*       address     - address of memory(register or table).
*       memsize     - size of memory
*
* OUTPUTS:
*       activeMemPtrPtr - pointer to the active memory entry or NULL if not exist.
*
* RETURNS:
*       pointer to the memory location
*       NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
void * smemGenericFindMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
);

/*******************************************************************************
*   smemIsDeviceMemoryOwner
*
* DESCRIPTION:
*       Return indication that the device is the owner of the memory.
*       relevant to multi port groups where there is 'shared memory' between port groups.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of memory(register or table).
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_TRUE  -  the device is     the owner of the memory.
*       GT_FALSE -  the device is NOT the owner of the memory.
*
* COMMENTS:
*
*
*******************************************************************************/
GT_BOOL smemIsDeviceMemoryOwner
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                  address
);

/*******************************************************************************
*   smemDfxRegFldGet
*
* DESCRIPTION:
*       Read data from the DFX register's field
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of register.
*       fldFirstBit - field first Bit
*       fldSize     - field size
*       data        - pointer to register's field value.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*       The function aborts application if address not exist.
*
*******************************************************************************/
void smemDfxRegFldGet
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  address,
    IN GT_U32                  fldFirstBit,
    IN GT_U32                  fldSize,
    OUT GT_U32               * data
);

/*******************************************************************************
*   smemDfxRegFldSet
*
* DESCRIPTION:
*       Write data to the DFX register's field
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of register.
*       fldFirstBit - field first Bit
*       fldSize     - field size
*       data        - pointer to register's field value.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*          The function aborts application if address not exist.
*
*******************************************************************************/
void smemDfxRegFldSet
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  address,
    IN GT_U32                  fldFirstBit,
    IN GT_U32                  fldSize,
    IN GT_U32                  data
);

/*******************************************************************************
*   smemDfxRegSet
*
* DESCRIPTION:
*       Write data to the DFX register's
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of register.
*       data        - pointer to register's field value.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*          The function aborts application if address not exist.
*
*******************************************************************************/
void smemDfxRegSet
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  address,
    IN GT_U32                  data
);

/*******************************************************************************
*   smemDfxRegGet
*
* DESCRIPTION:
*       Read data from the DFX register's
*
* INPUTS:
*       deviceObj   - pointer to device object.
*       address     - address of register.
*       data        - pointer to register's field value.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*          The function aborts application if address not exist.
*
*******************************************************************************/
void smemDfxRegGet
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  address,
    IN GT_U32                * dataPtr
);

/*******************************************************************************
*   smemDeviceObjMemoryAllAllocationsFree
*
* DESCRIPTION:
*       free all memory allocations that the device ever done.
*       optionally to including 'own' pointer !!!
*       this device must do 'FULL init sequence' if need to be used.
*
* INPUTS:
*       devObjPtr - pointer to device object
*       freeMyObj - indication to free the memory of 'device itself'
*                   GT_TRUE  - free devObjPtr
*                   GT_FALSE - do not free devObjPtr
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
void smemDeviceObjMemoryAllAllocationsFree
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_BOOL                  freeMyObj
);

/*******************************************************************************
*   smemDeviceObjMemoryAlloc__internal
*
* DESCRIPTION:
*       allocate memory (calloc style) that is registered with the device object.
*       this call instead of direct call to calloc/malloc needed for 'soft reset'
*       where all the dynamic memory of the device should be free/cleared.
*
* INPUTS:
*       devObjPtr - pointer to device object (ignored if NULL)
*       nitems - alloc for number of elements (calloc style)
*       size   - size of each element         (calloc style)
*       fileNamePtr -  file name that called the allocation
*       line        -  line in the file that called the allocation
* OUTPUTS:
*       None.
*
* RETURNS:
*       pointer to allocated memory
* COMMENTS:
*
*
*******************************************************************************/
GT_PTR smemDeviceObjMemoryAlloc__internal
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN size_t   nitems,
    IN size_t   size,
    IN const char*              fileNamePtr,
    IN GT_U32                   line
);
#define smemDeviceObjMemoryAlloc(devObjPtr,nitems,size) \
        smemDeviceObjMemoryAlloc__internal(devObjPtr,nitems,size,__FILE__,__LINE__)

/*******************************************************************************
*   smemDeviceObjMemoryRealloc__internal
*
* DESCRIPTION:
*       return new memory allocation after 'realloc' old memory to new size.
*       the oldPointer should be pointer that smemDeviceObjMemoryAlloc(..) returned.
*       this call instead of direct call to realloc needed for 'soft reset'
*       where all the dynamic memory of the device should be free/cleared.
* INPUTS:
*       devObjPtr - pointer to device object (ignored if NULL)
*       oldPointer - pointer to the 'old' element (realloc style)
*       size   - number of bytes for new segment (realloc style)
*       fileNamePtr -  file name that called the re-allocation
*       line        -  line in the file that called the re-allocation
* OUTPUTS:
*       None.
*
* RETURNS:
*       pointer to reallocated memory
* COMMENTS:
*
*
*******************************************************************************/
GT_PTR smemDeviceObjMemoryRealloc__internal
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_PTR                   oldPointer,
    IN size_t                   size,
    IN const char*              fileNamePtr,
    IN GT_U32                   line
);
#define smemDeviceObjMemoryRealloc(devObjPtr,oldPointer,size) \
        smemDeviceObjMemoryRealloc__internal(devObjPtr,oldPointer,size,__FILE__,__LINE__)

/*******************************************************************************
*   smemDeviceObjMemoryPtrFree__internal
*
* DESCRIPTION:
*       free memory that was allocated by smemDeviceObjMemoryAlloc or
*           smemDeviceObjMemoryRealloc
*       this call instead of direct call to free needed for 'soft reset'
*       where all the dynamic memory of the device should be free/cleared.
*
* INPUTS:
*       devObjPtr - pointer to device object (ignored if NULL)
*       oldPointer - pointer to the 'old' pointer that need to be free
*       fileNamePtr -  file name that called the free
*       line        -  line in the file that called the free
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       none.
* COMMENTS:
*
*
*******************************************************************************/
void smemDeviceObjMemoryPtrFree__internal
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_PTR                   oldPointer,
    IN const char*              fileNamePtr,
    IN GT_U32                   line
);
#define smemDeviceObjMemoryPtrFree(devObjPtr,oldPointer) \
        smemDeviceObjMemoryPtrFree__internal(devObjPtr,oldPointer,__FILE__,__LINE__)

/*******************************************************************************
*   smemDeviceObjMemoryPtrMemSetZero
*
* DESCRIPTION:
*       'memset 0' to memory that was allocated by smemDeviceObjMemoryAlloc or
*           smemDeviceObjMemoryRealloc
*
* INPUTS:
*       devObjPtr - pointer to device object
*       memPointer - pointer to the memory that need to be set to 0
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       none.
* COMMENTS:
*
*
*******************************************************************************/
void smemDeviceObjMemoryPtrMemSetZero
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_PTR                   memPointer
);

/*******************************************************************************
*   smemDeviceObjMemoryAllAllocationsSum
*
* DESCRIPTION:
*       debug function to print the sum of all allocations done for a device
*       allocations that done by smemDeviceObjMemoryAlloc or
*           smemDeviceObjMemoryRealloc
*
* INPUTS:
*       devObjPtr - pointer to device object
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       none.
* COMMENTS:
*
*
*******************************************************************************/
void smemDeviceObjMemoryAllAllocationsSum
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr
);

/*******************************************************************************
*   smemConvertGlobalPortToCurrentPipeId
*
* DESCRIPTION:
*       Convert 'global port' of local port and pipe-id.
*       NOTE: the pipe-id is saved as 'currentPipeId' in 'SKERNEL_TASK_COOKIE_INFO_STC'
*           info 'per task'
*       needed for multi-pipe device.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       globalPortNum  - global port number of the device.
* OUTPUTS:
*       pipeDevObjPtrPtr   - (pointer to) pointer to 'pipe' device object.
*       pipePortPtr     - (pointer to) the local port number in the 'pipe'.
*
* RETURNS:
*        None
*
* COMMENTS:
*       function MUST be called when packet ingress the device.
*
*******************************************************************************/
void smemConvertGlobalPortToCurrentPipeId
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                  globalPortNum,
    OUT GT_U32                 *localPortNumberPtr
);

/*******************************************************************************
*   smemConvertCurrentPipeIdAndLocalPortToGlobal
*
* DESCRIPTION:
*       Convert {currentPipeId,port} of local port on pipe to {dev,port} of 'global'.
*       NOTE: the pipe-id is taken from 'currentPipeId' in 'SKERNEL_TASK_COOKIE_INFO_STC'
*           info 'per task'
*       needed for multi-pipe device.
*
* INPUTS:
*       devObjPtr   - pointer to device object of the pipe.
*       localPortNum  - local port number of the pipe.
* OUTPUTS:
*       globalPortPtr     - (pointer to) the global port number in the device.
*
* RETURNS:
*        None
*
* COMMENTS:
*       function MUST be called before packet egress the device.
*
*******************************************************************************/
void smemConvertCurrentPipeIdAndLocalPortToGlobal
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                  localPortNum,
    OUT GT_U32                 *globalPortNumPtr
);

/*******************************************************************************
*   smemSetCurrentPipeId
*
* DESCRIPTION:
*       set current pipe-id (saved as 'currentPipeId' in 'SKERNEL_TASK_COOKIE_INFO_STC'
*           info 'per task')
*       needed for multi-pipe device.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       currentPipeId  - the current pipeId to set.
* OUTPUTS:
*
* RETURNS:
*        None
*
* COMMENTS:
*
*******************************************************************************/
void smemSetCurrentPipeId
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                  currentPipeId
);

/*******************************************************************************
*   smemGetCurrentPipeId
*
* DESCRIPTION:
*       get current pipe-id (saved as 'currentPipeId' in 'SKERNEL_TASK_COOKIE_INFO_STC'
*           info 'per task')
*       needed for multi-pipe device.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
* OUTPUTS:
*
* RETURNS:
*       currentPipeId  - the current pipeId to set.
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 smemGetCurrentPipeId
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);
/*******************************************************************************
*   smemConvertPipeIdAndLocalPortToGlobal_withPipeAndDpIndex
*
* DESCRIPTION:
*       Convert {PipeId,dpIndex,localPort} of local port on DP to {dev,port} of 'global'.
*       needed for multi-pipe device.
*
* INPUTS:
*       devObjPtr   - pointer to device object of the pipe.
*       pipeIndex   - pipeIndex
*       dpIndex     - dpIndex (relative to pipe)
*       localPortNum  - local port number of the DP.
*
* OUTPUTS:
*       globalPortPtr     - (pointer to) the global port number in the device.
*
* RETURNS:
*        None
*
* COMMENTS:
*
*
*******************************************************************************/
void smemConvertPipeIdAndLocalPortToGlobal_withPipeAndDpIndex
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                  pipeIndex,
    IN GT_U32                  dpIndex,
    IN GT_U32                  localPortNum,
    OUT GT_U32                 *globalPortNumPtr
);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemh */


