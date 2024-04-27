/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemPhy.c
*
* DESCRIPTION:
*       This is API implementation for PHY memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 4 $
*
*******************************************************************************/

#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/smem/smemPhy.h>

/* active functions for Write */
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemPhyActiveWritePageReg);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemPhyActiveWriteLinkCryptRead);
ACTIVE_WRITE_FUNC_PROTOTYPE_MAC(smemPhyActiveWriteLinkCryptWrite);

/* active functions for Read */
ACTIVE_READ_FUNC_PROTOTYPE_MAC(smemPhyActiveReadPageReg);

/* convert address of PHY register , as described in datasheet to address in simulation.

    NOTE: this macro is to be used ONLY when :
    1. 'allocating memories' --> function smemPhyPage
    2. ' looking for memory' --> function smemPhyFindInUnitChunk

    because this macro is ONLY for building address memory spaces !!!

    --> in all other places use only PHY_REG_ADDR_MAC

    regPage - the page of the register
            NOTE: value 0..255 (8bits)
    regAddr - register address as given in the datasheet of the PHY
            NOTE: regAddr is 5 bits value 0..31
*/
#define INTERNAL_MEM_PHY_REG_ADDR_MAC(regPage , regAddr) \
    (((regPage) << PHY_PAGE_OFFSET_CNS) | (regAddr) << 2)



/* Active memory table */

/*
    NOTE: all the addresses that developer adds here must contain 'page address' !!!
          but once the active memory function is called there is no page indication in the 'address'

*/

static SMEM_ACTIVE_MEM_ENTRY_STC smemPhyActiveTable[] =
{
    /* register 22 --> 'Page' register - valid in all pages */
    {PHY_PAGE_ADDRESS_REG, PHY_REG_ADDR_MAC(0,0x1f) , smemPhyActiveReadPageReg, 0 , smemPhyActiveWritePageReg, 0},

    /* register 0 in page 16 - 'LinkCrypt ReadAddress'  */
    {PHY_LINKCRYPT_READ_REG, SMEM_FULL_MASK_CNS , NULL, 0 , smemPhyActiveWriteLinkCryptRead, 0},
    /* register 1 in page 16 - 'LinkCrypt WriteAddress'  */
    {PHY_LINKCRYPT_WRITE_REG, SMEM_FULL_MASK_CNS , NULL, 0 , smemPhyActiveWriteLinkCryptWrite, 0},

    /* must be last anyway */
    {0xffffffff, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};


#define SMEM_ACTIVE_MEM_TABLE_SIZE \
    (sizeof(smemPhyActiveTable)/sizeof(smemPhyActiveTable[0]))


/*******************************************************************************
*   smemPhyFindInUnitChunk
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
static GT_U32 *  smemPhyFindInUnitChunk
(
    IN SKERNEL_DEVICE_OBJECT *devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_UINTPTR param
)
{
    /* adjust address before looking for it in the memory */
    address = INTERNAL_MEM_PHY_REG_ADDR_MAC(0 , address);

    return smemDevFindInUnitChunk(devObjPtr,accessType,address,memSize,param);
}

/*******************************************************************************
*  smemPhyActiveWritePageReg
*
* DESCRIPTION:
*      Write page register.
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memSize     - memory size to be read.
*       memPtr      - Pointer to the register's memory in the simulation.
*       writeMask   - 32 bits mask of writable bits.
*       inMemPtr    - Pointer to the memory to set register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemPhyActiveWritePageReg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR writeMask,
    IN         GT_U32 * inMemPtr
)
{
    /* write to page register in page 0 */
    smemRegSet(devObjPtr,PHY_PAGE_ADDRESS_REG,*inMemPtr);

    /* update the current page value in the 'shadow' */
    ((PHY_DEV_MEM_INFO *)(devObjPtr->deviceMemory))->currentActivePage = SMEM_U32_GET_FIELD( *inMemPtr , 0,8);

    return;
}

/*******************************************************************************
*  smemPhyActiveReadPageReg
*
* DESCRIPTION:
*      Read page register .
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
void smemPhyActiveReadPageReg
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR sumBit,
    OUT        GT_U32 * outMemPtr
)
{
    /* read from page register in page 0 */
    smemRegGet(devObjPtr,PHY_PAGE_ADDRESS_REG,outMemPtr);
}

/*******************************************************************************
*  smemPhyActiveWriteLinkCryptRead
*
* DESCRIPTION:
*      Write 'LinkCrypt Read' register
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memSize     - memory size to be read.
*       memPtr      - Pointer to the register's memory in the simulation.
*       writeMask   - 32 bits mask of writable bits.
*       inMemPtr    - Pointer to the memory to set register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemPhyActiveWriteLinkCryptRead
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR writeMask,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32          data;/* data read from the macsec */
    GT_U32          macsecDeviceId;/* macsec device id */

    *memPtr = *inMemPtr;

    /* the PHY core hold the connection to the macsec */
    macsecDeviceId = devObjPtr->tmpPeerDeviceId;

    if(macsecDeviceId != SMAIN_NOT_VALID_CNS)
    {
        scibReadMemory(macsecDeviceId,(*inMemPtr),1,&data);
    }
    else
    {
        data = 0xFFFFFFFF;
    }

    /* LinkCrypt DataLo */
    smemRegSet(devObjPtr,PHY_LINKCRYPT_DATA_LO_REG,data & 0xFFFF);
    /* LinkCrypt DataHi */
    smemRegSet(devObjPtr,PHY_LINKCRYPT_DATA_HI_REG,data >> 16);

    return;
}

/*******************************************************************************
*  smemPhyActiveWriteLinkCryptWrite
*
* DESCRIPTION:
*      Write 'LinkCrypt Write' register
* INPUTS:
*       devObjPtr   - device object PTR.
*       address     - Address for ASIC memory.
*       memSize     - memory size to be read.
*       memPtr      - Pointer to the register's memory in the simulation.
*       writeMask   - 32 bits mask of writable bits.
*       inMemPtr    - Pointer to the memory to set register's content.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smemPhyActiveWriteLinkCryptWrite
(
    IN         SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN         GT_U32   address,
    IN         GT_U32   memSize,
    IN         GT_U32 * memPtr,
    IN         GT_UINTPTR writeMask,
    IN         GT_U32 * inMemPtr
)
{
    GT_U32          dataLo,dataHi;/* data to write to the macsec */
    GT_U32          data;/* data to write to the macsec */
    GT_U32          macsecDeviceId;/* macsec device id */

    *memPtr = *inMemPtr;

    if(devObjPtr->portGroupSharedDevObjPtr == NULL)
    {
        /* the PHY core hold the connection to the macsec */
        macsecDeviceId = devObjPtr->tmpPeerDeviceId;
    }
    else
    {
        /* the PHY shell hold the connection to the macsec */
        macsecDeviceId = devObjPtr->portGroupSharedDevObjPtr->tmpPeerDeviceId;
    }

    if(macsecDeviceId != SMAIN_NOT_VALID_CNS)
    {
        /* LinkCrypt DataLo */
        smemRegGet(devObjPtr,PHY_LINKCRYPT_DATA_LO_REG,&dataLo);
        /* LinkCrypt DataHi */
        smemRegGet(devObjPtr,PHY_LINKCRYPT_DATA_HI_REG,&dataHi);

        data = ((dataHi << 16)| dataLo);
        /* write to the macsec */
        scibWriteMemory(macsecDeviceId,(*inMemPtr),1,&data);
    }
    else
    {
        /* do nothing */
    }

    return;
}

/*******************************************************************************
*   smemPhyFindMem
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
static void * smemPhyFindMem
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr
)
{
    void              * memPtr;         /* registers's memory pointer */
    PHY_DEV_MEM_INFO * devMemInfoPtr;  /* device's memory pointer */
    GT_U32              index;          /* register's memory offset */
    GT_U32              param;          /* additional parameter */
    GT_U32              activeMemoryAddr;/* address to be used for active memory */

    devMemInfoPtr = devObjPtr->deviceMemory;

    if(IS_SKERNEL_OPERATION_MAC(accessType))
    {
        /*********************/
        /* skernel operation */
        /*********************/

        /* address not contain page value */
        /* support finding memory with 'page value' embedded into it -->
           1. this will support write from the 'reg file' during initialization
           2. this will support getting value from register from different page
              then PHY currently set for
        */
        index = SMEM_U32_GET_FIELD(address,PHY_PAGE_OFFSET_CNS,8);

        address &= 0xFFFF;/* remove the page indication */
    }
    else
    {
        /******************/
        /* SCIB accessing */
        /******************/

        /* act according to value in the 'page register' */
        index = devMemInfoPtr->currentActivePage;
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
        activeMemoryAddr = PHY_REG_ADDR_MAC(index,address);

        *activeMemPtrPtr = NULL;
        for (index = 0; index < (SMEM_ACTIVE_MEM_TABLE_SIZE - 1); index++)
        {
            /* check address */
            if ((activeMemoryAddr & smemPhyActiveTable[index].mask)
                 == smemPhyActiveTable[index].address)
            {
                *activeMemPtrPtr = &smemPhyActiveTable[index];
                break;
            }
        }
    }

    return memPtr;
}


/*******************************************************************************
*   smemPhyPage
*
* DESCRIPTION:
*       Allocate address type specific memories
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
static void smemPhyPage
(
    IN      SKERNEL_DEVICE_OBJECT *devObjPtr,
    INOUT   PHY_DEV_MEM_INFO  * devMemInfoPtr,
    INOUT   SMEM_UNIT_CHUNKS_STC  *unitPtr,
    GT_U32  page
)
{
    GT_U32  ii;/*iterator */
    GT_U32  index;/* current index*/

    {
        SMEM_CHUNK_BASIC_STC  chunksMem[]=
        {
            /* page 0 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(0),0) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(0),10))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(0),12) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(0),23))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(0),26) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(0),26))}
            /* page 1 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(1),0) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(1),8))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(1),15) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(1),19))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(1),21) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(1),26))}
            /* page 2 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(2),16) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(2),16))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(2),18) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(2),21))}
            /* page 3 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(3),16) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(3),19))}
            /* page 4 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(4),0) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(4),1))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(4),4) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(4),6))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(4),16) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(4),27))}
            /* page 5 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(5),16) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(5),28))}
            /* page 6 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(6),16) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(6),18))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(6),20) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(6),20))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(6),23) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(6),26))}
            /* page 7 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(7),16) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(7),21))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(7),25) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(7),28))}
            /* page 8 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(8),0) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(8),2))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(8),8) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(8),15))}
            /* page 9 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(9),0) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(9),3))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(9),5) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(9),5))}
            /* page 12 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(12),0) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(12),5))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(12),8) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(12),15))}
            /* page 14 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(14),0) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(14),3))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(14),8) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(14),8))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(14),14) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(14),15))}
            /* page 16 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(16),0) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(16),3))}
            /* page 18 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(18),16) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(18),17))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(18),20) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(18),20))}
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(18),25) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(18),27))}
            /* page 253 - not documented but accessed by application */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(253),0) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(253),31))}
            /* page 255 */
            ,{SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(255),0) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(255),31))}
        };
        GT_U32  numOfChunks = sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC);
        SMEM_CHUNK_BASIC_STC  tmpChunksMem[sizeof(chunksMem)/sizeof(SMEM_CHUNK_BASIC_STC)];
        SMEM_CHUNK_BASIC_STC  pageRegChunkMem[1] =
        {
            /* page 0..255 - register 22 */
             {SET_SMEM_CHUNK_BASIC_STC_ENTRY_BY_END_ADDR_MAC (INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(0),22) ,INTERNAL_MEM_PHY_REG_ADDR_MAC(PAGE_MAC(0),22))}
        };
        SMEM_UNIT_CHUNKS_STC    pageRegUnitChunk;

        index = 0;
        for(ii = 0 ; ii < numOfChunks ; ii++)
        {
            if(((chunksMem[ii].memFirstAddr >> PHY_PAGE_OFFSET_CNS) & 0xFF) != page)
            {
                continue;
            }

            /* copy content */
            tmpChunksMem[index] = chunksMem[ii];

            /* remove the page indication from the address */
            /* because when accessing to address you not give in this action
               the page in the address */
            tmpChunksMem[index].memFirstAddr &= 0xFFFF;

            index++;
        }

        smemInitMemChunk(devObjPtr,tmpChunksMem,index,unitPtr);

        smemInitMemChunk(devObjPtr,pageRegChunkMem, 1, &pageRegUnitChunk);

        /*add the 'page' register chunk into the other registers of the page */
        smemInitMemCombineUnitChunks(devObjPtr,unitPtr ,&pageRegUnitChunk);
    }

    devMemInfoPtr->specFunTbl[page].specFun   = smemPhyFindInUnitChunk;
    devMemInfoPtr->specFunTbl[page].specParam = (GT_UINTPTR)(void*)unitPtr;

}


/*******************************************************************************
*   smemPhyAllocSpecMemory
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
static void smemPhyAllocSpecMemory(
    INOUT PHY_DEV_MEM_INFO  * devMemInfoPtr,
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32              ii;

    /* set by default that all units bind to 'Fatal error' */
    for (ii = 0; ii < NUM_PAGES_CNS; ii++)
    {
        devMemInfoPtr->specFunTbl[ii].specFun    = (void*)skernelFatalError;
    }

    smemPhyPage(devObjPtr,devMemInfoPtr,&devMemInfoPtr->page0  , 0  );
    smemPhyPage(devObjPtr,devMemInfoPtr,&devMemInfoPtr->page1  , 1  );
    smemPhyPage(devObjPtr,devMemInfoPtr,&devMemInfoPtr->page2  , 2  );
    smemPhyPage(devObjPtr,devMemInfoPtr,&devMemInfoPtr->page3  , 3  );
    smemPhyPage(devObjPtr,devMemInfoPtr,&devMemInfoPtr->page4  , 4  );
    smemPhyPage(devObjPtr,devMemInfoPtr,&devMemInfoPtr->page5  , 5  );
    smemPhyPage(devObjPtr,devMemInfoPtr,&devMemInfoPtr->page6  , 6  );
    smemPhyPage(devObjPtr,devMemInfoPtr,&devMemInfoPtr->page7  , 7  );
    smemPhyPage(devObjPtr,devMemInfoPtr,&devMemInfoPtr->page8  , 8  );
    smemPhyPage(devObjPtr,devMemInfoPtr,&devMemInfoPtr->page9  , 9  );
    smemPhyPage(devObjPtr,devMemInfoPtr,&devMemInfoPtr->page12 , 12 );
    smemPhyPage(devObjPtr,devMemInfoPtr,&devMemInfoPtr->page14 , 14 );
    smemPhyPage(devObjPtr,devMemInfoPtr,&devMemInfoPtr->page16 , 16 );
    smemPhyPage(devObjPtr,devMemInfoPtr,&devMemInfoPtr->page18 , 18 );
    smemPhyPage(devObjPtr,devMemInfoPtr,&devMemInfoPtr->page253, 253);
    smemPhyPage(devObjPtr,devMemInfoPtr,&devMemInfoPtr->page255, 255);
}

/*******************************************************************************
*   smemPhyInit
*
* DESCRIPTION:
*       Init memory module for a Twist device.
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
void smemPhyInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32  ii;
    PHY_DEV_MEM_INFO  * devMemInfoPtr; /* device's memory pointer */

    devObjPtr->notSupportPciConfigMemory = 1;/* the device not support PCI/PEX configuration memory space */

    if(devObjPtr->numOfCoreDevs)
    {
        /* this is the core shared device */
        /* so we need to fill info about it's core devices */
        for( ii = 0 ; ii < devObjPtr->numOfCoreDevs ;ii++)
        {
            /* each of the core devices start his ports at multiple of 10
               (each core has 2 ports) */
            devObjPtr->coreDevInfoPtr[ii].startPortNum = 10*ii;
        }
        return;
    }

    /* alloc PHY_DEV_MEM_INFO */
    devMemInfoPtr = (PHY_DEV_MEM_INFO *)calloc(1, sizeof(PHY_DEV_MEM_INFO));
    if (devMemInfoPtr == NULL)
    {
        skernelFatalError("smemPhyInit: allocation error\n");
    }

    devObjPtr->devFindMemFunPtr = (void *)smemPhyFindMem;
    devObjPtr->deviceMemory = devMemInfoPtr;
    devObjPtr->supportAnyAddress = GT_TRUE;

    /* allocate address type specific memories */
    smemPhyAllocSpecMemory(devMemInfoPtr,devObjPtr);

}

/*******************************************************************************
*   smemPhyInit2
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
void smemPhyInit2
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
)
{
    /* currently nothing more to do */
    return;
}

/*******************************************************************************
*   smemPhyFrameProcess
*
* DESCRIPTION:
*       Process frames in the phy
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       bufferId     - frame data buffer Id
*       srcPort      - source port number
*
*******************************************************************************/
static GT_VOID smemPhyFrameProcess
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
*   smemPhyProcessInit
*
* DESCRIPTION:
*       Init the frame processing module.
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
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
void smemPhyProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    /* initiation of internal functions */
    devObjPtr->devFrameProcFuncPtr = smemPhyFrameProcess;
}

