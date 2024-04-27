/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemMacsec.c
*
* DESCRIPTION:
*       This is API implementation for Macsec memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 7 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/smem/smemMacsec.h>

/* convert address of macsec register , as described in datasheet to address in simulation.

    NOTE: this macro is to be used ONLY when :
    1. 'allocating memories' --> functions smemMacsecUnitxxx
    2. ' looking for memory' --> function smemMacsecFindInUnitChunk

    because this macro is ONLY for building address memory spaces !!!

    --> in all other places don't use it

    regAddr - register address as given in the datasheet of the macsec
            NOTE: regAddr has 1 based granularity (and not 4 like in prestera devices)
*/
#define INTERNAL_MEM_MACSEC_REG_ADDR_MAC(regAddr) \
    ((regAddr) << 2)



/* Active memory table */

static SMEM_ACTIVE_MEM_ENTRY_STC smemMacsecActiveTable[] =
{
    /* dummy entry to avoid warnings from 'Coverity' .
       NOTE: remove this entry once there is read entry in the table ! */
    {0,SMEM_FULL_MASK_CNS, NULL,0,NULL,0},

    /* must be last anyway */
    {0xffffffff, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};


#define SMEM_ACTIVE_MEM_TABLE_SIZE \
    (sizeof(smemMacsecActiveTable)/sizeof(smemMacsecActiveTable[0]))



/*******************************************************************************
*   smemMacsecFindInUnitChunk
*
* DESCRIPTION:
*       find the memory in the specified unit chunk
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemMacsecFindInUnitChunk
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{

    /* adjust address before looking for it in the memory */
    address = INTERNAL_MEM_MACSEC_REG_ADDR_MAC(address);

    return smemDevFindInUnitChunk(devObjPtr,accessType,address,memSize,param);
}
/*******************************************************************************
*   smemMacsecFindInUnitChunk_4
*
* DESCRIPTION:
*       find the memory in the specified unit chunk - unit 4
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
static GT_U32 *  smemMacsecFindInUnitChunk_4
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    SMEM_UNIT_CHUNKS_STC  *unitPtr;
    MACSEC_DEV_MEM_INFO * devMemInfoPtr;  /* device's memory pointer */
    GT_U32  dbAddress = INTERNAL_MEM_MACSEC_REG_ADDR_MAC(address);

    devMemInfoPtr = devObjPtr->deviceMemory;

    /* start with last part of the unit */
    unitPtr = &devMemInfoPtr->petStat;
    if(unitPtr->chunksArray[0].memFirstAddr <= dbAddress)
    {
        return smemMacsecFindInUnitChunk(devObjPtr,accessType,address,memSize,(GT_UINTPTR)(void*)unitPtr);
    }

    unitPtr = &devMemInfoPtr->global;
    if(unitPtr->chunksArray[0].memFirstAddr <= dbAddress)
    {
        return smemMacsecFindInUnitChunk(devObjPtr,accessType,address,memSize,(GT_UINTPTR)(void*)unitPtr);
    }

    unitPtr = &devMemInfoPtr->macStat;
    return smemMacsecFindInUnitChunk(devObjPtr,accessType,address,memSize,(GT_UINTPTR)(void*)unitPtr);

}

/*******************************************************************************
*   smemMacsecFindMem
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
static void * smemMacsecFindMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    void              * memPtr;           /* registers' memory pointer */
    MACSEC_DEV_MEM_INFO * devMemInfoPtr;  /* device's memory pointer */
    GT_U32              index;            /* register's memory offset */
    GT_U32              param;            /* additional parameter */

    devMemInfoPtr = devObjPtr->deviceMemory;

    index = MACSEC_UNIT_INDEX_GET_MAC(address);

    if(index >= MACSEC_NUM_MEMORY_SPACES_CNS)
    {
        /* memory not covered by the device */
        return NULL;
    }

    /* Call register spec function to obtain pointer to register memory */
    param   = devMemInfoPtr->specFunTbl[index].specParam;
    memPtr  = devMemInfoPtr->specFunTbl[index].specFun(devObjPtr,
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
            if ((address & smemMacsecActiveTable[index].mask)
                 == smemMacsecActiveTable[index].address)
            {
                *activeMemPtrPtr = &smemMacsecActiveTable[index];
            }
        }
    }

    return memPtr;
}


/*******************************************************************************
*   smemMacsecUnitPorts
*
* DESCRIPTION:
*       Allocate address type specific memories - per port unit
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
static void smemMacsecUnitPorts
(
    IN      SKERNEL_DEVICE_OBJECT *devObjPtr,
    INOUT   MACSEC_DEV_MEM_INFO  * devMemInfoPtr,
    INOUT   SMEM_UNIT_CHUNKS_STC  *unitPtr,
    GT_U32  port,
    GT_U32  unitIndex
)
{
    GT_U32  ii;/*iterator */

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* registers */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0)    ,INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x33))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x40) ,INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x47))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x50) ,INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x57))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x70) ,INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x7f))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x3c0),INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x3c4))}
            /* tables */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x80)  ,   16*4), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(32, 4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x100) , 32*8*4), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(5*32, 8*4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x200) , 32*8*4), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(8*32, 8*4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x300) , 32*4*4), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(128, 4*4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x400) , 32*4*4), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(128, 4*4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x480) , 32*4*4), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(128, 4*4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x500) , 32*4*4), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(128, 4*4)}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_NUM_BYTES_MAC(INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x580) , 32*4*4), SET_SMEM_ENTRY_SIZE_AND_ALIGN_MAC(128, 4*4)}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);


        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            chunksMem[ii].memFirstAddr += (INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x800) * port);
        }

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    devMemInfoPtr->specFunTbl[unitIndex].specFun   = smemMacsecFindInUnitChunk;
    devMemInfoPtr->specFunTbl[unitIndex].specParam = (GT_UINTPTR)(void*)unitPtr;

}

/*******************************************************************************
*   smemMacsecUnitMacStat
*
* DESCRIPTION:
*       Allocate address type specific memories - Mac statistics
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
static void smemMacsecUnitMacStat
(
    IN      SKERNEL_DEVICE_OBJECT *devObjPtr,
    INOUT   MACSEC_DEV_MEM_INFO  * devMemInfoPtr,
    INOUT   SMEM_UNIT_CHUNKS_STC  *unitPtr,
    GT_U32  unitIndex
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x2000) ,INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x20FF))}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    devMemInfoPtr->specFunTbl[unitIndex].specFun   = smemMacsecFindInUnitChunk_4;
    devMemInfoPtr->specFunTbl[unitIndex].specParam = (GT_UINTPTR)(void*)unitPtr;/* not relevant to unit 4 */

}

/*******************************************************************************
*   smemMacsecUnitGlobal
*
* DESCRIPTION:
*       Allocate address type specific memories - global registers
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
static void smemMacsecUnitGlobal
(
    IN      SKERNEL_DEVICE_OBJECT *devObjPtr,
    INOUT   MACSEC_DEV_MEM_INFO  * devMemInfoPtr,
    INOUT   SMEM_UNIT_CHUNKS_STC  *unitPtr,
    GT_U32  unitIndex
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x2100) ,INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x2110))}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    devMemInfoPtr->specFunTbl[unitIndex].specFun   = smemMacsecFindInUnitChunk_4;
    devMemInfoPtr->specFunTbl[unitIndex].specParam = (GT_UINTPTR)(void*)unitPtr;/* not relevant to unit 4 */

}

/*******************************************************************************
*   smemMacsecUnitPetStat
*
* DESCRIPTION:
*       Allocate address type specific memories - PET statistics
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
static void smemMacsecUnitPetStat
(
    IN      SKERNEL_DEVICE_OBJECT *devObjPtr,
    INOUT   MACSEC_DEV_MEM_INFO  * devMemInfoPtr,
    INOUT   SMEM_UNIT_CHUNKS_STC  *unitPtr,
    GT_U32  unitIndex
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x2200) ,INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x229f))}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    devMemInfoPtr->specFunTbl[unitIndex].specFun   = smemMacsecFindInUnitChunk_4;
    devMemInfoPtr->specFunTbl[unitIndex].specParam = (GT_UINTPTR)(void*)unitPtr; /* not relevant to unit 4 */

}

/*******************************************************************************
*   smemMacsecUnitMacsecStat
*
* DESCRIPTION:
*       Allocate address type specific memories - macsec statistics
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
static void smemMacsecUnitMacsecStat
(
    IN      SKERNEL_DEVICE_OBJECT *devObjPtr,
    INOUT   MACSEC_DEV_MEM_INFO  * devMemInfoPtr,
    INOUT   SMEM_UNIT_CHUNKS_STC  *unitPtr,
    GT_U32  unitIndex
)
{
    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x2800) ,INTERNAL_MEM_MACSEC_REG_ADDR_MAC(0x2d3f))}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);

        smemInitMemChunk(devObjPtr,chunksMem,numOfChunks,unitPtr);
    }

    devMemInfoPtr->specFunTbl[unitIndex].specFun   = smemMacsecFindInUnitChunk;
    devMemInfoPtr->specFunTbl[unitIndex].specParam = (GT_UINTPTR)(void*)unitPtr;

}

/*******************************************************************************
*   smemMacsecAllocSpecMemory
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
static void smemMacsecAllocSpecMemory(
    INOUT MACSEC_DEV_MEM_INFO  * devMemInfoPtr,
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32              ii;
    GT_U32              unitIndex;/* unit index */

    /* set by default that all units bind to 'Fatal error' */
    for (ii = 0; ii < MACSEC_NUM_MEMORY_SPACES_CNS; ii++)
    {
        devMemInfoPtr->specFunTbl[ii].specFun    = (void*)skernelFatalError;
    }

    for (unitIndex = 0; unitIndex < MACSEC_NUM_PORTS_CNS; unitIndex++)
    {
        smemMacsecUnitPorts(devObjPtr,devMemInfoPtr,&devMemInfoPtr->ports[unitIndex],unitIndex,unitIndex);
    }
    /* continue with ii */
    /* ii stay with same value for next 3 units because those are mapped to the same unit */
    smemMacsecUnitMacStat(devObjPtr,devMemInfoPtr,&devMemInfoPtr->macStat,unitIndex);
    smemMacsecUnitGlobal(devObjPtr,devMemInfoPtr,&devMemInfoPtr->global,unitIndex);
    smemMacsecUnitPetStat(devObjPtr,devMemInfoPtr,&devMemInfoPtr->petStat,unitIndex);
    unitIndex++;
    smemMacsecUnitMacsecStat(devObjPtr,devMemInfoPtr,&devMemInfoPtr->macsecStatistics,unitIndex);
}

/*******************************************************************************
*   smemMacsecInit
*
* DESCRIPTION:
*       Init memory module for a Twist device.
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
void smemMacsecInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    MACSEC_DEV_MEM_INFO  * devMemInfoPtr; /* device's memory pointer */

    devObjPtr->notSupportPciConfigMemory = 1;/* the device not support PCI/PEX configuration memory space */

    /* alloc MACSEC_DEV_MEM_INFO */
    devMemInfoPtr = (MACSEC_DEV_MEM_INFO *)calloc(1, sizeof(MACSEC_DEV_MEM_INFO));
    if (devMemInfoPtr == NULL)
    {
        skernelFatalError("smemMacsecInit: allocation error\n");
    }

    devObjPtr->devFindMemFunPtr = (void *)smemMacsecFindMem;
    devObjPtr->deviceMemory = devMemInfoPtr;
    devObjPtr->supportAnyAddress = GT_TRUE;

    /* allocate address type specific memories */
    smemMacsecAllocSpecMemory(devMemInfoPtr,devObjPtr);

}

/*******************************************************************************
*   smemMacsecInit2
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
void smemMacsecInit2
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
)
{
    /* currently nothing more to do */
    return;
}

/*******************************************************************************
*   smemMacsecFrameProcess
*
* DESCRIPTION:
*       Process frames in the macsec
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       bufferId     - frame data buffer Id
*       srcPort      - source port number
*
*******************************************************************************/
static GT_VOID smemMacsecFrameProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SBUF_BUF_ID bufferId,
    IN GT_U32 srcPort
)
{
    GT_U32  destPort;
    /* currently every packet that ingress will egress 'as is' without any processing */

    if(srcPort & 1)
    {
        destPort = srcPort - 1;
    }
    else
    {
        destPort = srcPort + 1;
    }

    smainFrame2PortSend(devObjPtr,destPort,
                        bufferId->actualDataPtr,
                        bufferId->actualDataSize,
                        GT_FALSE);
}

/*******************************************************************************
*   smemMacsecProcessInit
*
* DESCRIPTION:
*       Init the frame processing module.
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
void smemMacsecProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    /* initiation of internal functions */
    devObjPtr->devFrameProcFuncPtr = smemMacsecFrameProcess;
}

