/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smem.c
*
* DESCRIPTION:
*       This is a API implementation for SMem module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 81 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/smem/smem.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smem/smemSalsa.h>
#include <asicSimulation/SKernel/smem/smemTwist.h>
#include <asicSimulation/SKernel/smem/smemTiger.h>
#include <asicSimulation/SKernel/smem/smemSoho.h>
#include <asicSimulation/SKernel/smem/smemFaFox.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/smem/smemCapuera.h>
#include <asicSimulation/SKernel/smem/smemFe.h>
#include <asicSimulation/SKernel/smem/smemFAP.h>
#include <asicSimulation/SKernel/smem/smemCheetah2.h>
#include <asicSimulation/SKernel/smem/smemCheetah3.h>
#include <asicSimulation/SKernel/smem/smemXCat3.h>
#include <asicSimulation/SKernel/smem/smemLion.h>
#include <asicSimulation/SKernel/smem/smemXCat2.h>
#include <asicSimulation/SKernel/smem/smemOcelot.h>
#include <asicSimulation/SInit/sinit.h>
#include <asicSimulation/SKernel/sEmbeddedCpu/sEmbeddedCpu.h>
#include <asicSimulation/SKernel/smem/smemComModule.h>
#include <asicSimulation/SKernel/smem/smemPhy.h>
#include <asicSimulation/SKernel/smem/smemMacsec.h>
#include <asicSimulation/SKernel/smem/smemBobcat3.h>
#include <asicSimulation/SLog/simLogInfoTypeMemory.h>
#include <asicSimulation/SLog/simLog.h>

#define MAX_ENTRY_SIZE_IN_WORDS_CNS  64
#define MAX_ALLOC_CNS               0x0FFFFFFF
#define ASICP_socket_max_tbl_entry_size_CNS 256

#if 0/* modify to 1 for debug */
    #define MALLOC_FILE_NAME_DEBUG_PRINT(x) simForcePrintf x
#else
    #define MALLOC_FILE_NAME_DEBUG_PRINT(x)
#endif

/*Gets 'cookiePtr' of the thread*/
extern GT_PTR simOsTaskOwnTaskCookieGet(GT_VOID);

/* global active memory for :
    no active memory to the unit.
   and no need to look all over the active memory of the device */
SMEM_ACTIVE_MEM_ENTRY_STC smemEmptyActiveMemoryArr[] =
{
    /* must be last anyway */
    {END_OF_TABLE, SMEM_FULL_MASK_CNS, NULL,0,NULL,0}
};


typedef enum {
          ASICP_opcodes_read_memory_E,
          ASICP_opcodes_write_memory_E
} ASICP_opcodes_ENT;


typedef struct{
    ASICP_opcodes_ENT           message_type;/* must be ASICP_opcodes_write_memory_E */
    GT_U32                     cpu_id;   /* 1 based number */
    GT_U32                     device_id;/* 0 based number */
    GT_U32                     address;  /* address in memory of the device */
    GT_U32                     length;   /* number of words */
    GT_U32                     entry[ASICP_socket_max_tbl_entry_size_CNS];
} ASICP_msg_info_STC;

void  smemReadWriteMem (
                                IN SCIB_MEMORY_ACCESS_TYPE accessType,
                                IN void * deviceObjPtr,
                                IN GT_U32 address,
                                IN GT_U32 memSize,
                                INOUT GT_U32 * memPtr
                              );

static void * smemFindMemory  (
                                IN SKERNEL_DEVICE_OBJECT * deviceObj,
                                IN GT_U32                  address,
                                IN GT_U32                  memSize,
                                OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr,
                                IN SCIB_MEMORY_ACCESS_TYPE accessType
                              );

static void  smemSendDataToVisualAsic(
                                    IN GT_U32                  deviceObjID,
                                    IN GT_U32                  address,
                                    IN GT_U32                * dataPtr,
                                    IN GT_U32                  dataSize
                              );

static GT_VOID smemChunkInternalWriteDataCheck(
                                    IN SKERNEL_DEVICE_OBJECT    *devObjPtr,
                                    IN GT_U32                   address,
                                    IN SMEM_CHUNK_STC           *chunkPtr,
                                    INOUT GT_U32                *memoryIndexPtr
                                );

static GT_VOID smemUnitChunkOverlapCheck(
                                    IN SMEM_UNIT_CHUNKS_STC    *unitChunksPtr,
                                    IN GT_U32 unitChunkOffset,
                                    IN SMEM_CHUNK_STC * currChunkPtr
                                );
static GT_BOOL smemUnitChunkFormulaOverlapCheck(
                                    IN SMEM_UNIT_CHUNKS_STC     *unitChunksPtr,
                                    IN GT_U32 unitChunkOffset,
                                    IN SMEM_CHUNK_STC * currChunkPtr,
                                    IN GT_U32 addressOffset
                                );
static void smemConvertDevAndAddrToNewDevAndAddr  (
                                IN SKERNEL_DEVICE_OBJECT * deviceObj,
                                IN GT_U32                  address,
                                IN SCIB_MEMORY_ACCESS_TYPE accessType,
                                OUT SKERNEL_DEVICE_OBJECT * *newDeviceObjPtrPtr,
                                OUT GT_U32                  *newAddressPtr
                              );

/*flag to allow to by pass the fatal error on accessing to non-exists memory.
    this allow to use test of memory without the need to stop on every memory

    we can get a list of all registers at once
*/
GT_U32   debugModeByPassFatalErrorOnMemNotExists = 0;

/* build address of the table in the relevant unit */
/* replace the 6 bits of the unit in the 'baseAddr' with 6 bits of unit from the 'unit base address'*/
#define SET_MULTI_INSTANCE_ADDRESS_MAC(baseAddr,unitBaseAddr) \
        SMEM_U32_SET_FIELD(baseAddr,23,9,                     \
            SMEM_U32_GET_FIELD(unitBaseAddr,23,9))

/* check field parameters */
static void paramCheck
(
    IN      GT_U32  startBit ,
    IN      GT_U32  numBits)
{
    if(startBit >= 32)
    {
        skernelFatalError("paramCheck : startBit[%d] >= 32 \n" , startBit);
    }
    if(numBits > 32)
    {
        skernelFatalError("paramCheck : numBits[%d] > 32 \n" , numBits);
    }
    if((startBit + numBits) > 32)
    {
        skernelFatalError("paramCheck : (startBit[%d] + numBits[%d]) > 32 \n" , startBit,numBits);
    }
}

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
*         The binding to the scib rw function needs to be indicated if the
*         device is PP or FA.
*******************************************************************************/
void smemInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    GT_U8     isPP=1;
    GT_U32     devId;
    SCIB_RW_MEMORY_FUN rwFun;     /* pointer to the R/W SKernel memory CallBack function */
    GT_U32      ii;/*iterator*/

    if (!deviceObjPtr)
    {
        skernelFatalError("smemInit : illegal pointer\n");
    }

    devId = deviceObjPtr->deviceId ;
    /* Set default memory access function */
    rwFun = smemReadWriteMem;

    /* check device type (including FA) and call device/FA specific init */
    switch(deviceObjPtr->deviceFamily)
    {
         case SKERNEL_SALSA_FAMILY:
              smemSalsaInit(deviceObjPtr);
         break;
         case SKERNEL_TWIST_D_FAMILY:
         case SKERNEL_SAMBA_FAMILY:
         case SKERNEL_TWIST_C_FAMILY:
              smemTwistInit(deviceObjPtr);
         break;
         case SKERNEL_TIGER_FAMILY:
              smemTigerInit(deviceObjPtr);
         break;
         case SKERNEL_SOHO_FAMILY:
              smemSohoInit(deviceObjPtr);
         break;
         case SKERNEL_FA_FOX_FAMILY:
              isPP=0;
              devId = deviceObjPtr->uplink.partnerDeviceID ;
              smemFaFoxInit(deviceObjPtr);
         break;
         case SKERNEL_CHEETAH_1_FAMILY:
              smemChtInit(deviceObjPtr) ;
              break;
         case SKERNEL_CHEETAH_2_FAMILY:
              smemCht2Init(deviceObjPtr) ;
              break;
         case SKERNEL_CHEETAH_3_FAMILY:
         case SKERNEL_XCAT_FAMILY:
              smemCht3Init(deviceObjPtr) ;
              break;
         case SKERNEL_XCAT3_FAMILY:
              smemXCat3Init(deviceObjPtr);
              break;
         case SKERNEL_XCAT2_FAMILY:
              smemXCat2Init(deviceObjPtr);
              break;
         case SKERNEL_LION_PORT_GROUP_SHARED_FAMILY:
              smemLionInit(deviceObjPtr) ;

              /* shared device not bind with the SCIB , because it has no unique
                 PEX base address of */

               /* !!! THIS IS RETURN AND NOT BREAK !!!!! */
               /* !!! THIS IS RETURN AND NOT BREAK !!!!! */
               /* !!! THIS IS RETURN AND NOT BREAK !!!!! */
              return;
         case SKERNEL_LION_PORT_GROUP_FAMILY:
              smemLionInit(deviceObjPtr) ;
              break;
         case SKERNEL_LION2_PORT_GROUP_SHARED_FAMILY:
         case SKERNEL_LION3_PORT_GROUP_SHARED_FAMILY:
              if(deviceObjPtr->deviceFamily == SKERNEL_LION2_PORT_GROUP_SHARED_FAMILY)
              {
                smemLion2Init(deviceObjPtr) ;
              }
              else
              {
                smemLion3Init(deviceObjPtr) ;
              }

              /* shared device not bind with the SCIB , because it has no unique
                 PEX base address of */

               /* !!! THIS IS RETURN AND NOT BREAK !!!!! */
               /* !!! THIS IS RETURN AND NOT BREAK !!!!! */
               /* !!! THIS IS RETURN AND NOT BREAK !!!!! */
              return;
         case SKERNEL_LION2_PORT_GROUP_FAMILY:
              smemLion2Init(deviceObjPtr) ;
              break;
         case SKERNEL_LION3_PORT_GROUP_FAMILY:
              smemLion3Init(deviceObjPtr) ;
              break;
         case SKERNEL_XBAR_CAPOEIRA_FAMILY: /* xbar is like pp */
              smemCapXbarInit(deviceObjPtr);
         break;
         case SKERNEL_FAP_DUNE_FAMILY:
             if (deviceObjPtr->deviceType == SKERNEL_98FX950)
             {
                 /* FAP20M device mem init*/
                 smemOcelotInit(deviceObjPtr);
             }
             else
             {
                  smemFapInit(deviceObjPtr);

                  /* The FAP device has two interfaces.
                     1. - Uplink interface with PP, as Fox. For this interface
                           a CPU may access to FAP only through PP.
                     2. - ECI host interface, PP independent.

                   bind R/W callback with SCIB as FA to provide uplink interface.
                   ECI interface will provided by second common bind. */
                  scibBindRWMemory(deviceObjPtr->uplink.partnerDeviceID,
                                   deviceObjPtr->deviceHwId,
                                   deviceObjPtr,
                                   rwFun,
                                   0,
                                   ADDRESS_COMPLETION_STATUS_GET_MAC(deviceObjPtr));
             }



         break;
         case SKERNEL_FE_DUNE_FAMILY:
              smemFeInit(deviceObjPtr);
         break;
#if 0    /* we do not support puma in regulare simulation -- only with GM ... see function smemGmInit*/
         case SKERNEL_PUMA_FAMILY:
              smemRegDbInit(deviceObjPtr);
         break;
#endif /*0*/
         case SKERNEL_EMBEDDED_CPU_FAMILY:
              smemEmbeddedCpuInit(deviceObjPtr);
         break;
         case SKERNEL_COM_MODULE_FAMILY:
              /* Set SUB20 memory access function */
              rwFun = smemSub20AdapterInit(deviceObjPtr);
         break;

         case SKERNEL_PHY_SHELL_FAMILY:
              smemPhyInit(deviceObjPtr);
              /* shared device not bind with the SCIB , because it has no unique
                 SMI base address */

               /* !!! THIS IS RETURN AND NOT BREAK !!!!! */
               /* !!! THIS IS RETURN AND NOT BREAK !!!!! */
               /* !!! THIS IS RETURN AND NOT BREAK !!!!! */
              return;
         case SKERNEL_PHY_CORE_FAMILY:
              smemPhyInit(deviceObjPtr);
         break;
         case SKERNEL_MACSEC_FAMILY:
              smemMacsecInit(deviceObjPtr);
               /* need to register the macsec so the PHY can access to it via the scib interface .
               (so active memories in macsec can be activated)*/
              break;
         case SKERNEL_PUMA3_SHARED_FAMILY:
               /* no bind with the SCIB */
               /* this is the port group shared device */
               /* so we need to fill info about it's port group devices */
               for( ii = 0 ; ii < deviceObjPtr->numOfCoreDevs ;ii++)
               {
                   /* each of the port group devices start his ports at multiple of 16
                      the Puma3 hold only 2 MG --> meaning 2 'cores'
                      so each core has 16 NW ports (0..15) + 16 fabric ports(32..47) */
                   deviceObjPtr->coreDevInfoPtr[ii].startPortNum = 16*ii;
               }
              return;
        case SKERNEL_BOBCAT2_FAMILY:
            smemBobcat2Init(deviceObjPtr);
            break;
        case SKERNEL_BOBK_CAELUM_FAMILY:
        case SKERNEL_BOBK_CETUS_FAMILY:
            smemBobkInit(deviceObjPtr);
            break;
        case SKERNEL_BOBCAT3_FAMILY:
            smemBobcat3Init(deviceObjPtr);
            break;
         default:
              skernelFatalError(" smemInit: not valid mode[%d]",
                                   deviceObjPtr->deviceFamily);
         break;
    }

    /* bind R/W callback with SCIB */
    scibBindRWMemory(devId,
                     deviceObjPtr->deviceHwId,
                     deviceObjPtr,
                     rwFun,
                     isPP,
                     ADDRESS_COMPLETION_STATUS_GET_MAC(deviceObjPtr));
}

/*******************************************************************************
*   smemInit2
*
* DESCRIPTION:
*       Init memory module for a device - after the load of the default
*           registers file
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
void smemInit2
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{

    switch(deviceObjPtr->deviceFamily)
    {
        case SKERNEL_CHEETAH_1_FAMILY:
        case SKERNEL_CHEETAH_2_FAMILY:
        case SKERNEL_CHEETAH_3_FAMILY:
        case SKERNEL_XCAT_FAMILY:
            smemChtInit2(deviceObjPtr);
            break;
        case SKERNEL_XCAT3_FAMILY:
            smemXCat3Init2(deviceObjPtr);
            break;
        case SKERNEL_XCAT2_FAMILY:
            smemXCat2Init2(deviceObjPtr);
            break;
        case SKERNEL_LION_PORT_GROUP_SHARED_FAMILY:
        case SKERNEL_LION_PORT_GROUP_FAMILY:
            smemLionInit2(deviceObjPtr);
            break;
        case SKERNEL_LION2_PORT_GROUP_SHARED_FAMILY:
        case SKERNEL_LION2_PORT_GROUP_FAMILY:
            smemLion2Init2(deviceObjPtr);
            break;
        case SKERNEL_LION3_PORT_GROUP_SHARED_FAMILY:
        case SKERNEL_LION3_PORT_GROUP_FAMILY:
            smemLion3Init2(deviceObjPtr);
            break;
        case SKERNEL_PHY_CORE_FAMILY:
            smemPhyInit2(deviceObjPtr);
            break;
        case SKERNEL_MACSEC_FAMILY:
            smemMacsecInit2(deviceObjPtr);
            break;
        case SKERNEL_BOBCAT2_FAMILY:
            smemBobcat2Init2(deviceObjPtr);
            break;
        case SKERNEL_BOBK_CAELUM_FAMILY:
        case SKERNEL_BOBK_CETUS_FAMILY:
            smemBobkInit2(deviceObjPtr);
            break;
        case SKERNEL_BOBCAT3_FAMILY:
            smemBobcat3Init2(deviceObjPtr);
            break;
        default:
            break;
    }

    if(SKERNEL_DEVICE_FAMILY_CHEETAH_3_ONLY(deviceObjPtr))
    {
        /* Sleep for 1 seconds */
        /* it needs to prevent crash on ch3 (linux simulation) */
        SIM_OS_MAC(simOsSleep)(1000);
    }

    /* Init done */
    /* State init status to global control registers */
    smemChtInitStateSet(deviceObjPtr);

    return;
}

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
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  address,
    IN GT_U32                  data
)
{
    GT_U32                  * findMemPtr;
    GT_U32                    memSize;
    SCIB_MEMORY_ACCESS_TYPE accessType = SKERNEL_MEMORY_WRITE_E;
    memSize = 1;

    smemConvertDevAndAddrToNewDevAndAddr(deviceObjPtr,address,accessType,
        &deviceObjPtr,&address);

    /* check device type and call device search memory function*/
    findMemPtr = smemFindMemory(deviceObjPtr,address,  memSize, NULL,
                                accessType);

    /* log memory information */
    simLogMemory(deviceObjPtr, SIM_LOG_MEMORY_WRITE_E, SIM_LOG_MEMORY_DEV_E,
                                            address, data, *findMemPtr);

    /* Write memory to the register's memory */
    *findMemPtr = data;

    /* send write register message to Visual address ASIC */
    smemSendDataToVisualAsic(deviceObjPtr->deviceId, address, findMemPtr, 1);
}

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
)
{
    GT_U32 regValue, fieldMask;

    if (deviceObjPtr == 0)
    {
        skernelFatalError("smemRegSet : illegal pointer\n");
    }

    paramCheck(fldFirstBit,fldSize);

    /* Read register value */
    smemRegGet(deviceObjPtr, address, &regValue);

    /* Align data right according to the first field bit */
    regValue >>= fldFirstBit;

    fldSize = fldSize % 32;
    /* get field's bits mask */
    if (fldSize)
    {
        fieldMask = ~(0xffffffff << fldSize);
    }
    else
    {
        fieldMask = (0xffffffff);
    }

    *data = regValue & fieldMask;
}

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
)
{
    GT_U32 * findMemPtr;
    GT_U32  regValue, fieldMask;
    SCIB_MEMORY_ACCESS_TYPE accessType = SKERNEL_MEMORY_READ_PCI_E;

    smemConvertDevAndAddrToNewDevAndAddr(deviceObjPtr,address,accessType,
        &deviceObjPtr,&address);

    paramCheck(fldFirstBit,fldSize);

    /* check device type and call device search memory function*/
    findMemPtr = smemFindMemory(deviceObjPtr, address, 1, NULL,
                                accessType);
    regValue = findMemPtr[0];

    /* Align data right according to the first field bit */
    regValue >>= fldFirstBit;

    fldSize = fldSize % 32;
    /* get field's bits mask */
    if (fldSize)
    {
        fieldMask = ~(0xffffffff << fldSize);
    }
    else
    {
        fieldMask = (0xffffffff);
    }

    *data = regValue & fieldMask;

    /* log memory information */
    simLogMemory(deviceObjPtr, SIM_LOG_MEMORY_PCI_READ_E, SIM_LOG_MEMORY_DEV_E,
                                            address, *data, 0);
}
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
)
{
    GT_U32 regValue, fieldMask;

    if (deviceObjPtr == 0)
    {
        skernelFatalError("smemRegSet : illegal pointer\n");
    }

    paramCheck(fldFirstBit,fldSize);

    fldSize = fldSize % 32;
    /* get field's bits mask */
    if (fldSize)
    {
        fieldMask = ~(0xffffffff << fldSize);
    }
    else
    {
        fieldMask = (0xffffffff);
    }

    /* Get register value */
    smemRegGet(deviceObjPtr, address, &regValue);
    /* Set field value in the register value using
    inverted real field mask in the place of the field */
    regValue &= ~(fieldMask << fldFirstBit);
    regValue |= (data << fldFirstBit);
    smemRegSet(deviceObjPtr, address, regValue);
}


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
)
{
    smemPciRegFldSet(deviceObjPtr, address, 0, 32, data);
}

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
)
{
    GT_U32 * findMemPtr;
    GT_U32 regValue, fieldMask;
    SCIB_MEMORY_ACCESS_TYPE accessType = SKERNEL_MEMORY_WRITE_PCI_E;

    smemConvertDevAndAddrToNewDevAndAddr(deviceObjPtr,address,accessType,
        &deviceObjPtr,&address);

    paramCheck(fldFirstBit,fldSize);

    fldSize = fldSize % 32;
    /* get field's bits mask */
    if (fldSize)
    {
        fieldMask = ~(0xffffffff << fldSize);
    }
    else
    {
        fieldMask = 0xffffffff;
    }

    /* Get register value */
    findMemPtr = smemFindMemory(deviceObjPtr, address, 1, NULL,
                                accessType);
    regValue = findMemPtr[0];
    /* Set field value in the register value using
    inverted real field mask in the place of the field */
    regValue &= ~(fieldMask << fldFirstBit);
    regValue |= (data << fldFirstBit);

    /* log memory information */
    simLogMemory(deviceObjPtr, SIM_LOG_MEMORY_PCI_WRITE_E, SIM_LOG_MEMORY_DEV_E,
                                            address, regValue, *findMemPtr);
    *findMemPtr = regValue;
}

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
)
{
    GT_U32                  * findMemPtr = 0;
    GT_U32                    memSize;
    SCIB_MEMORY_ACCESS_TYPE accessType = SKERNEL_MEMORY_READ_E;

    memSize = 1;

    smemConvertDevAndAddrToNewDevAndAddr(deviceObjPtr,address,accessType,
        &deviceObjPtr,&address);

    /* check device type and call device search memory function */
    findMemPtr = smemFindMemory(deviceObjPtr, address, memSize, NULL,
                                accessType);

    /* log memory information */
    simLogMemory(deviceObjPtr, SIM_LOG_MEMORY_READ_E, SIM_LOG_MEMORY_DEV_E,
                                            address, *findMemPtr, 0);

    /* Read memory from the register's memory */
    *data = *findMemPtr;
}

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
)
{
    GT_U32                  * findMemPtr;
    GT_U32                  memSize;
    SCIB_MEMORY_ACCESS_TYPE accessType = SKERNEL_MEMORY_READ_E;

    smemConvertDevAndAddrToNewDevAndAddr(deviceObjPtr,address,accessType,
        &deviceObjPtr,&address);

    memSize = 1;

    /* check device type and call device search memory function*/
    findMemPtr = smemFindMemory(deviceObjPtr, address, memSize, NULL,
                                accessType);

    /* Does memory exist check */
    if (findMemPtr)
    {
        /* log memory information */
        simLogMemory(deviceObjPtr, SIM_LOG_MEMORY_READ_E, SIM_LOG_MEMORY_DEV_E,
                                                address, *findMemPtr, 0);
    }

    return findMemPtr;
}

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
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  address,
    IN GT_U32                * dataPtr,
    IN GT_U32                  dataSize
)
{
    GT_U32                  * findMemPtr;
    GT_U32                    i;
    SCIB_MEMORY_ACCESS_TYPE accessType = SKERNEL_MEMORY_WRITE_E;

    smemConvertDevAndAddrToNewDevAndAddr(deviceObjPtr,address,accessType,
        &deviceObjPtr,&address);

    /* check device type and call device search memory function*/
    findMemPtr = smemFindMemory(deviceObjPtr, address, dataSize, NULL,
                                accessType);

    if (dataSize == 0)
    {
        skernelFatalError("smemMemSet : memory size not valid \n");
    }

    /* Write memory to the register's or tables's memory */
    for (i = 0; i < dataSize; i++)
    {
        /* log memory information */
        simLogMemory(deviceObjPtr, SIM_LOG_MEMORY_WRITE_E, SIM_LOG_MEMORY_DEV_E,
                                                address, dataPtr[i], findMemPtr[i]);
        findMemPtr[i] = dataPtr[i];
    }

    /* send to Visual ASIC */
    smemSendDataToVisualAsic(
            deviceObjPtr->deviceId, address, findMemPtr, dataSize);

}

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
)
{
    SKERNEL_DEVICE_OBJECT   * kernelDevObjPtr; /* pointer to a deviceObject*/
    SMEM_ACTIVE_MEM_ENTRY_STC * activeMemPtr = NULL;
                                /* pointer to active memory entry*/
    GT_U32                  * findMemPtr; /* pointer to a memory location */
    GT_U32                    i; /* iterator */

    SIM_LOG_MEMORY_SOURCE_ENT memAccessSource = SIM_LOG_MEMORY_DEV_E;
    SIM_LOG_MEMORY_ACTION_ENT memAccessAction;

    if(!IS_SKERNEL_OPERATION_MAC(accessType))
    {
        memAccessSource = SIM_LOG_MEMORY_CPU_E;
    }

    if(IS_PCI_OPERATION_MAC(accessType))
    {
        if(IS_WRITE_OPERATION_MAC(accessType))
        {
            memAccessAction = SIM_LOG_MEMORY_PCI_WRITE_E;
        }
        else
        {
            memAccessAction = SIM_LOG_MEMORY_PCI_READ_E;
        }
    }
    else
    if(IS_DFX_OPERATION_MAC(accessType))
    {
        if(IS_WRITE_OPERATION_MAC(accessType))
        {
            memAccessAction = SIM_LOG_MEMORY_DFX_WRITE_E;
        }
        else
        {
            memAccessAction = SIM_LOG_MEMORY_DFX_READ_E;
        }
    }
    else
    {
        if(IS_WRITE_OPERATION_MAC(accessType))
        {
            memAccessAction = SIM_LOG_MEMORY_WRITE_E;
        }
        else
        {
            memAccessAction = SIM_LOG_MEMORY_READ_E;
        }
    }

    /* check device type and call device search memory function*/
    kernelDevObjPtr = (SKERNEL_DEVICE_OBJECT *)deviceObjPtr;

    smemConvertDevAndAddrToNewDevAndAddr(kernelDevObjPtr,address,accessType,
        &kernelDevObjPtr,&address);

    findMemPtr = smemFindMemory(kernelDevObjPtr, address, memSize,
                                &activeMemPtr, accessType);

    if (memSize == 0)
    {
        skernelFatalError("smemReadWriteMem : memory size not valid \n");
    }
    /* Read memory from the register's or tables's memory */
    if (SMEM_ACCESS_READ_FULL_MAC(accessType))
    {

        /* check active memory existance */
        if ((activeMemPtr != NULL) && (activeMemPtr->readFun != NULL))
        {
            /* log memory information */
            simLogMemory(deviceObjPtr, memAccessAction, memAccessSource,
                                                      address, findMemPtr[0], 0);

            activeMemPtr->readFun(kernelDevObjPtr,address,memSize,findMemPtr,
                                    activeMemPtr->readFunParam,memPtr);

            /* those active read may change the value of the registers */
            /* send to Visual ASIC */
            smemSendDataToVisualAsic(kernelDevObjPtr->deviceId,
                                     address,findMemPtr,memSize);
        }
        else
        {
            for (i = 0; i < memSize; i++)
            {
                /* log memory information */
                simLogMemory(deviceObjPtr, memAccessAction, memAccessSource,
                        address, findMemPtr[i]/*value*/, 0);
                memPtr[i] = findMemPtr[i];
            }
        }
    }
    /* Write memory to the register's or tables's memory */
    else
    {
        /* check active memory existance */
        if ((activeMemPtr != NULL) && (activeMemPtr->writeFun != NULL))
        {
            activeMemPtr->writeFun(kernelDevObjPtr,address,memSize,findMemPtr,
                                    activeMemPtr->writeFunParam,memPtr);

            /* log memory information */
            simLogMemory(deviceObjPtr, memAccessAction, memAccessSource,
                                               address, *findMemPtr, *memPtr);
        }
        else
        {
            for (i = 0; i < memSize; i++)
            {
                /* log memory information */
                simLogMemory(deviceObjPtr, memAccessAction, memAccessSource,
                    address, memPtr[i]/* new value*/, findMemPtr[i]/* old value*/);

                findMemPtr[i] = memPtr[i];
            }
        }

        /* send to Visual ASIC */
        smemSendDataToVisualAsic(kernelDevObjPtr->deviceId,
                                 address,findMemPtr,memSize);
    }
}


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
)
{
    debugModeByPassFatalErrorOnMemNotExists = byPassFatalErrorOnMemNotExists;
}


/*******************************************************************************
*   smemFindMemory
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
*        find memory of fa adapter memory space also.
*
*******************************************************************************/
static void * smemFindMemory
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  address,
    IN GT_U32                  memSize,
    OUT SMEM_ACTIVE_MEM_ENTRY_STC ** activeMemPtrPtr,
    IN SCIB_MEMORY_ACCESS_TYPE accessType
)
{
    void *  returnMemory;
    static GT_U32   debugMemoryArray[256];
    SMEM_DEV_MEMORY_FIND_FUN   findMemFunc = deviceObjPtr->devFindMemFunPtr;
    static GT_U32   dummyPciConfigMemoryValue_read = SMAIN_NOT_VALID_CNS;/*dummy space to read from*/
    static GT_U32   dummyPciConfigMemoryValue_write;/*dummy space to write to */

    if (SMEM_ACCESS_PCI_FULL_MAC(accessType) &&
        deviceObjPtr->notSupportPciConfigMemory)
    {
        /* the device NOT supports PCI config memory */
        /* return pointer to dummy value */
        if (SMEM_ACCESS_READ_FULL_MAC(accessType))
        {
            return &dummyPciConfigMemoryValue_read;
        }
        else
        {
            /* dummy place to write to (not impact the 'read' value !)*/
            return &dummyPciConfigMemoryValue_write;
        }


    }



    if ((address & 0x3) != 0)
    {
        if(deviceObjPtr->supportAnyAddress == GT_FALSE)
        {
            skernelFatalError("smemFindMemory : illegal address: [0x%8.8x] ((address& 0x3) != 0)\n",address);
        }
    }
    /* Try to find address in common memory first */
    returnMemory = smemFindCommonMemory(deviceObjPtr, address, memSize, accessType , activeMemPtrPtr);
    if(returnMemory)
    {
        return returnMemory;
    }

    returnMemory =  findMemFunc(deviceObjPtr, accessType, address,memSize, activeMemPtrPtr);

    if (returnMemory == NULL)
    {
        if(GT_FALSE == skernelUserDebugInfo.disableFatalError)
        {
            if(debugModeByPassFatalErrorOnMemNotExists)
            {
                debugModeByPassFatalErrorOnMemNotExists--;/* limit the number of times */


                returnMemory = debugMemoryArray;

                simWarningPrintf("smemFindMemory : memory not exist [0x%8.8x]\n",address);
            }
            else
            {
                GT_CHAR*    unitNamePtr ;

                if(SMEM_ACCESS_PCI_FULL_MAC(accessType))
                {
                    unitNamePtr = "PCI/PEX registers";
                }
                else if(SMEM_ACCESS_DFX_FULL_MAC(accessType))
                {
                    unitNamePtr = "DFX memory/registers";
                }
                else
                {
                    unitNamePtr = smemUnitNameByAddressGet(deviceObjPtr,address);
                }

                if(unitNamePtr)
                {
                    skernelFatalError("smemFindMemory : memory not exist [0x%8.8x] in unit [%s] \n",address,unitNamePtr);
                }
                else
                {
                    skernelFatalError("smemFindMemory : memory not exist [0x%8.8x]\n",address);
                }
            }
        }
    }

    return  returnMemory;
}


/*******************************************************************************
*   smemSendDataToVisualAsic
*
* DESCRIPTION:
*       send data to VISUAL ASIC on the named pipe
*
* INPUTS:
*
*       deviceID        - device ID
*            address                -
*            memSize,    - size, in bytes, of write buffer
*            memPtr                - pointer to write buffer
*
*
* OUTPUTS:
*       None.
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*  the function will send as many msg as needed to send the total data .
*
*
*******************************************************************************/
static void smemSendDataToVisualAsic
(
    IN GT_U32                      deviceObjID,
    IN GT_U32                      address,
    IN GT_U32                      * dataPtr,
    IN GT_U32                      numWords
)
{
    GT_U32                  i;
    GT_U32                  msgLengh;
    ASICP_msg_info_STC      message;
    GT_U32                  currentNumOfWordsLeft = numWords;
    GT_U32                  numWordsToSendInCurrentLoop;
    GT_U32                  currentAddress = address;
    GT_U32                  isVisualAsicDisabled;

    if(SMEM_SKIP_LOCAL_READ_WRITE)
    {
        skernelFatalError("smemSendDataToVisualAsic : the Application side not holding the Asic so no data to the VisualAsic\n");
        /* the Application side not holding the Asic so no data to the VisualAsic */
        return;
    }

    message.message_type = ASICP_opcodes_write_memory_E;
    message.cpu_id = 1;
    message.device_id = deviceObjID;

    isVisualAsicDisabled = smainIsVisualDisabled();

    /*if visual asic is disabled (1), no need to send messages*/
    if (isVisualAsicDisabled == 1) {
        return;
    }

    do{
        numWordsToSendInCurrentLoop =
            currentNumOfWordsLeft > ASICP_socket_max_tbl_entry_size_CNS ?
            ASICP_socket_max_tbl_entry_size_CNS :
            currentNumOfWordsLeft;

        message.address = currentAddress;
        message.length = numWordsToSendInCurrentLoop;

        for (i = 0; i < numWordsToSendInCurrentLoop; i++)
        {
            message.entry[i] = dataPtr[i];
        }

        msgLengh = sizeof(message.cpu_id) +
                   sizeof(message.device_id) +
                   sizeof(message.message_type) +
                   sizeof(message.address) +
                   sizeof(message.length) +
                   numWordsToSendInCurrentLoop * sizeof(GT_32);

        /*CallNamedPipe(ASICP_STR_NAMED_PIPE_CNS, (void*)&message, msgLengh, NULL,
                      0,&bytesRead, 200);*/
        SIM_OS_MAC(simOsSendDataToVisualAsic)((void*)&message,msgLengh);


        currentNumOfWordsLeft -= numWordsToSendInCurrentLoop;
        currentAddress += numWordsToSendInCurrentLoop*4;
    }while(currentNumOfWordsLeft);
}

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
)
{
    SKERNEL_DEVICE_OBJECT   * kernelDevObjPtr; /* pointer to a deviceObject*/
    SMEM_ACTIVE_MEM_ENTRY_STC * activeMemPtr = NULL;
                                /* pointer to active memory entry*/
    GT_U32                  * findMemPtr; /* pointer to a memory location */
    GT_U32                    i; /* iterator */

    /* check device type and call device search memory function*/
    kernelDevObjPtr = (SKERNEL_DEVICE_OBJECT *)deviceObjPtr;

    smemConvertDevAndAddrToNewDevAndAddr(kernelDevObjPtr,address,accessType,
        &kernelDevObjPtr,&address);

    findMemPtr = smemFindMemory(kernelDevObjPtr, address, memSize,
                                &activeMemPtr, accessType);
    if (findMemPtr == 0)
    {
        skernelFatalError("smemReadWriteMem : memory not exist \n");
    }
    if (memSize == 0)
    {
        skernelFatalError("smemReadWriteMem : memory size not valid \n");
    }
    /* Read memory from the register's or tables's memory */
    if (SMEM_ACCESS_READ_FULL_MAC(accessType))
    {

        /* check active memory existance */
        if ((activeMemPtr != NULL) && (activeMemPtr->readFun != NULL))
        {
            activeMemPtr->readFun(kernelDevObjPtr,address,memSize,findMemPtr,
                                  activeMemPtr->readFunParam  ,
                                  memPtr);
            /* those active read may change the value of the registers */
            /* send to Visual ASIC */
            smemSendDataToVisualAsic(kernelDevObjPtr->deviceId,
                                     address,findMemPtr,memSize);
        }
        else
        {
            for (i = 0; i < memSize; i++)
            {
                memPtr[i] = findMemPtr[i];
            }
        }
    }
    /* Write memory to the register's or tables's memory */
    else
    {
        /* check active memory existance */
        if ((activeMemPtr != NULL) && (activeMemPtr->writeFun != NULL))
        {
            activeMemPtr->writeFun(kernelDevObjPtr,address,memSize,findMemPtr,
                                    activeMemPtr->writeFunParam| 0x80000000,memPtr);
        }
        else
        {
            for (i = 0; i < memSize; i++)
            {
                findMemPtr[i] = memPtr[i];
            }
        }

        /* send to Visual ASIC */
        smemSendDataToVisualAsic(kernelDevObjPtr->deviceId,
                                 address,findMemPtr,memSize);
    }
}

/*******************************************************************************
*  smemInitBasicMemChunk
*
* DESCRIPTION:
*      init unit memory chunk
*
* INPUTS:
*       unitChunksPtr       - pointer to allocated unit chunks
*       basicMemChunkPtr    - pointer to basic memory chunks info
*       basicMemChunkIndex  - basic memory chunk index
*
* OUTPUTS:
*       currChunkPtr  - pointer to allocated current memory chunk
*       fileNamePtr -  file name that called the allocation
*       line        -  line in the file that called the allocation
*
* RETURNS:
*        none
*
* COMMENTS:
*
*******************************************************************************/
static void smemInitBasicMemChunk
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SMEM_UNIT_CHUNKS_STC  *unitChunksPtr,
    IN SMEM_CHUNK_BASIC_STC * basicMemChunkPtr,
    IN GT_U32               basicMemChunkIndex,
    OUT SMEM_CHUNK_STC      * currChunkPtr,
    IN const char*     fileNamePtr,
    IN GT_U32          line
)
{
    GT_U32 totalMemWords; /* total size of allocated memory in words */

    if(basicMemChunkPtr->memFirstAddr == SMAIN_NOT_VALID_CNS)
    {
        memset(currChunkPtr,0,sizeof(currChunkPtr));
        /* indication that this chunk is skipped */
        currChunkPtr->calcIndexType = SMEM_INDEX_IN_CHUNK_TYPE_IGNORED_E;
        return;
    }


    currChunkPtr->memFirstAddr = basicMemChunkPtr->memFirstAddr;
    currChunkPtr->memLastAddr  =
        (basicMemChunkPtr->memFirstAddr + (basicMemChunkPtr->numOfRegisters * 4)) - 4;

    currChunkPtr->enrtyNumWordsAlignement = 0;

    if(basicMemChunkPtr->enrtyNumBytesAlignement)
    {
        if(basicMemChunkPtr->enrtySizeBits == 0)
        {
            skernelFatalError("smemInitBasicMemChunk: memory entry size can not be zero \n" );
        }
        currChunkPtr->lastEntryWordIndex =
            (basicMemChunkPtr->enrtySizeBits - 1) / 32;

        if(basicMemChunkPtr->enrtyNumBytesAlignement % 4 != 0)
        {
            skernelFatalError("smemInitBasicMemChunk: memory entry wrong alignment \n" );
        }
        currChunkPtr->enrtyNumWordsAlignement =
            basicMemChunkPtr->enrtyNumBytesAlignement / 4;

        if(currChunkPtr->lastEntryWordIndex >= currChunkPtr->enrtyNumWordsAlignement)
        {
            skernelFatalError("smemInitBasicMemChunk: memory entry alignment not valid \n" );
        }
    }

    totalMemWords = basicMemChunkPtr->numOfRegisters;

    if(currChunkPtr->lastEntryWordIndex != 0)
    {
        /* needed support for 'write table on last word' only for memories that
           are not 1 word entry */
        totalMemWords += currChunkPtr->enrtyNumWordsAlignement;
    }

    currChunkPtr->memSize = totalMemWords;
    currChunkPtr->memPtr = smemDeviceObjMemoryAlloc__internal(devObjPtr,totalMemWords , sizeof(SMEM_REGISTER),
        fileNamePtr,line);
    if(currChunkPtr->memPtr == NULL)
    {
        skernelFatalError("smemInitBasicMemChunk: allocation failed for [%d] bytes \n" ,
                            totalMemWords * sizeof(SMEM_REGISTER));
    }
    currChunkPtr->calcIndexType = SMEM_INDEX_IN_CHUNK_TYPE_STRAIGHT_ACCESS_E;
    currChunkPtr->memMask = 0xFFFFFFFC;
    currChunkPtr->rightShiftNumBits = 2;
    currChunkPtr->formulaFuncPtr = NULL;

    if(basicMemChunkPtr->tableOffsetValid)
    {
        currChunkPtr->tableOffsetInBytes = basicMemChunkPtr->tableOffsetInBytes;
    }
    else
    {
        currChunkPtr->tableOffsetInBytes = SMAIN_NOT_VALID_CNS;
    }

    /* Check chunk for overlapping */
    smemUnitChunkOverlapCheck(unitChunksPtr, basicMemChunkIndex, currChunkPtr);
}

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
)
{
    GT_U32  ii;
    SMEM_CHUNK_STC *currChunkPtr;/*current memory chunk */

    unitChunksPtr->chunksArray = smemDeviceObjMemoryAlloc__internal(devObjPtr,numOfChunks,sizeof(SMEM_CHUNK_STC),fileNamePtr,line);
    if(unitChunksPtr->chunksArray == NULL)
    {
        skernelFatalError("initMemChunk: chunk array allocation failed \n" );
    }

    unitChunksPtr->numOfChunks = numOfChunks;
    currChunkPtr = unitChunksPtr->chunksArray;

    for(ii = 0 ; ii < numOfChunks; ii++, currChunkPtr++ )
    {
        smemInitBasicMemChunk(devObjPtr,unitChunksPtr, &basicMemChunksArr[ii], ii, currChunkPtr,
            fileNamePtr,line);
    }
}

/*******************************************************************************
*  smemFindMemChunk
*
* DESCRIPTION:
*      init memory chunk
*
* INPUTS:
*       devObjPtr   - the device
*       accessType  - memory access type
*       unitChunksPtr   - pointer to chunks of memory for a unit
*       address - the address to find in the
* OUTPUTS:
*       indexInChunkArrayPtr  - (pointer to) the index in chunksMemArr[x] , that
*                               is the chosen chunk.
*                               valid only when the chunk was found (see return value)
*       indexInChunkMemoryPtr - (pointer to) the index in chunksMemArr[x]->memPtr[y],
*                               that represent the 'address'
*                               valid only when the chunk was found (see return value)
* RETURNS:
*        GT_TRUE  - address belong to the chunks
*        GT_FALSE - address not belong to the chunks
*
* COMMENTS:
*
*******************************************************************************/
static GT_BOOL smemFindMemChunk
(
    IN SKERNEL_DEVICE_OBJECT    *devObjPtr,
    IN SCIB_MEMORY_ACCESS_TYPE  accessType,
    IN SMEM_UNIT_CHUNKS_STC    *unitChunksPtr,
    IN GT_U32   address,
    OUT GT_U32  *indexInChunkArrayPtr,
    OUT GT_U32  *indexInChunkMemoryPtr
)
{
    GT_U32  ii;
    GT_U32  offset;
    SMEM_CHUNK_STC  * memChunkArr = unitChunksPtr->chunksArray;
    GT_U32 numOfChunks = unitChunksPtr->numOfChunks;

    /* Set init value */
    *indexInChunkMemoryPtr = 0;

    for(ii = 0 ; ii < numOfChunks; ii++)
    {
        if(address < memChunkArr[ii].memFirstAddr ||
           address > memChunkArr[ii].memLastAddr )
        {
            continue;
        }

        offset = address - memChunkArr[ii].memFirstAddr;

        *indexInChunkArrayPtr = ii;

        switch(memChunkArr[ii].calcIndexType)
        {
            case SMEM_INDEX_IN_CHUNK_TYPE_STRAIGHT_ACCESS_E:
                *indexInChunkMemoryPtr = offset / 4;
                break;
            case SMEM_INDEX_IN_CHUNK_TYPE_MASK_AND_SHIFT_E:
                *indexInChunkMemoryPtr = (offset & memChunkArr[ii].memMask) >> memChunkArr[ii].rightShiftNumBits;
                break;
            case SMEM_INDEX_IN_CHUNK_TYPE_FORMULA_FUNCTION_E:
                if(memChunkArr[ii].formulaFuncPtr == NULL)
                {
                    skernelFatalError("smemFindMemChunk : NULL function pointer \n");
                    break;
                }
                *indexInChunkMemoryPtr = memChunkArr[ii].formulaFuncPtr(devObjPtr,&memChunkArr[ii],address);

                if((*indexInChunkMemoryPtr) == SMEM_ADDRESS_NOT_IN_FORMULA_CNS)
                {
                    /* the address is not in this chunk , need to keep looking
                       for it in other chunks */
                    continue;
                }

                break;
            case SMEM_INDEX_IN_CHUNK_TYPE_IGNORED_E:
                /* this entry is ignored ! */
                continue;
            default:
                skernelFatalError("smemFindMemChunk : bad type \n");
                break;
        }

        if(accessType == SCIB_MEMORY_WRITE_E)
        {
            smemChunkInternalWriteDataCheck(devObjPtr, address, &memChunkArr[ii],
                                            indexInChunkMemoryPtr);
        }

        return GT_TRUE;
    }


    return GT_FALSE;
}

/*******************************************************************************
*   smemChunkFormulaCellCompare
*
* DESCRIPTION:
*       Function compare two components of formula array
*
* INPUTS:
*        cell1Ptr   - pointer to device formula cell array
*        cell2Ptr   - pointer to device formula cell array
* OUTPUTS:
*       None
*
* RETURNS:
*       The return value of this function represents whether cell1Ptr is considered
*       less than, equal, or grater than cell2Ptr by returning, respectively,
*       a negative value, zero or a positive value.
*
* COMMENTS:
*       function's prototype must be defined return int to avoid warnings and be
*       consistent with qsort() function prototype
******************************************************************************/
int smemChunkFormulaCellCompare
(
    const GT_VOID * cell1Ptr,
    const GT_VOID * cell2Ptr
)
{
    SMEM_FORMULA_DIM_STC * cellFirstPtr = (SMEM_FORMULA_DIM_STC *)cell1Ptr;
    SMEM_FORMULA_DIM_STC * cellNextPtr = (SMEM_FORMULA_DIM_STC *)cell2Ptr;

    return (cellNextPtr->step - cellFirstPtr->step);
}

/*******************************************************************************
*   smemChunkFormulaSort
*
* DESCRIPTION:
*       Function sorts components of formula array in memory chunk in descending
*       order
*
* INPUTS:
*        formulaCellPtr  - pointer to device formula cell array
*        formulaCellNum - formula cell array number
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*       Function should be called in phase of memory chunk allocation
*       and ensure descending order of formula steps to be like:
*       0x02040800 + ii * 0x8000 + jj * 0x1000,  where ii 0..5   , jj 0..7
*       This format is used for further calculation of simulation memory indexes.
*
*******************************************************************************/
static GT_VOID  smemChunkFormulaSort
(
    IN SMEM_FORMULA_DIM_STC * formulaCellPtr,
    IN GT_U32              formulaCellNum
)
{
    if(formulaCellNum > 1)
    {
        qsort(formulaCellPtr, formulaCellNum, sizeof(SMEM_FORMULA_DIM_STC),
              smemChunkFormulaCellCompare);

    }
}

/*******************************************************************************
*   smemUnitChunkAddressCompare
*
* DESCRIPTION:
*       Function compare two components of chunks array
*
* INPUTS:
*        chunk1Ptr   - pointer to unit chunks array
*        chunk2Ptr   - pointer to unit chunks array
* OUTPUTS:
*       None
*
* RETURNS:
*       The return value of this function represents whether chunk1Ptr is considered
*       less than, equal, or grater than chunk2Ptr by returning, respectively,
*       a negative value, zero or a positive value.
*
* COMMENTS:
*       function's prototype must be defined return int to avoid warnings and be
*       consistent with qsort() function prototype
******************************************************************************/
int smemUnitChunkAddressCompare
(
    const GT_VOID * chunk1Ptr,
    const GT_VOID * chunk2Ptr
)
{
    SMEM_CHUNK_STC * chunkFirstPtr = (SMEM_CHUNK_STC *)chunk1Ptr;
    SMEM_CHUNK_STC * chunkNextPtr = (SMEM_CHUNK_STC *)chunk2Ptr;

    return (chunkNextPtr->memFirstAddr - chunkFirstPtr->memFirstAddr);
}

/*******************************************************************************
*  smemFormulaChunkAddressCheck
*
* DESCRIPTION:
*      Check of address belonging to address range according to specific formula
*
* INPUTS:
*       address - address to be checked
*       memChunkPtr  - pointer to memory chunk
*
* OUTPUTS:
*       indexArrayPtr  - (pointer to) array of indexes according to
*                        specific address and formula.
*
* RETURNS:
*       GT_TRUE - address match found, GT_FALSE - otherwise
*
* COMMENTS:
*       If match not found, array indexes value are not relevant.
*
*******************************************************************************/
static GT_BOOL  smemFormulaChunkAddressCheck
(
    IN GT_U32               address,
    IN SMEM_CHUNK_STC     * memChunkPtr,
    OUT GT_U32            * indexArrayPtr
)
{
    GT_U32 compIndex;                   /* formula component index */
    GT_U32 addressOffset;               /* offset from chunk base address */
    SMEM_FORMULA_DIM_STC * formulaCellPtr;

    memset(indexArrayPtr, 0, SMEM_FORMULA_CELL_NUM_CNS * sizeof(GT_U32));

    addressOffset = address - memChunkPtr->memFirstAddr;
    /* Address is base address of chunk */
    if(addressOffset == 0)
    {
        return GT_TRUE;
    }

    /* Parse formula components to find indexes  */
    for(compIndex = 0; compIndex < SMEM_FORMULA_CELL_NUM_CNS; compIndex++)
    {
        formulaCellPtr = &memChunkPtr->formulaData[compIndex];

        if (formulaCellPtr->size == 0 || formulaCellPtr->step == 0)
            break;

        indexArrayPtr[compIndex] = addressOffset / formulaCellPtr->step;
        if(indexArrayPtr[compIndex] >= formulaCellPtr->size)
        {
            /* the address not match the formula */
            return GT_FALSE;
        }

        addressOffset %= formulaCellPtr->step;
    }

    /* If reminder not zero - address doesn't belong to chunk range */
    return (addressOffset) ? GT_FALSE : GT_TRUE;
}

/*******************************************************************************
*   smemFormulaChunkIndexGet
*
* DESCRIPTION:
*       Function which returns the index in the simulation memory chunk
*       that represents the accessed address by formula
*
* INPUTS:
*        devObjPtr  - pointer to device info
*        memChunkPtr - pointer to memory chunk info
*        address    - the accesses address
* OUTPUTS:
*       None
*
* RETURNS:
*       index into memChunkPtr->memPtr[index] that represent 'address'
*       if the return value == SMEM_ADDRESS_NOT_IN_FORMULA_CNS meaning that
*       address not belong to formula
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_U32  smemFormulaChunkIndexGet
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SMEM_CHUNK_STC        * memChunkPtr,
    IN GT_U32                  address
)
{
    GT_U32 destMemIndex = 0;                /* destination memory index */
    GT_U32 compIndex;                       /* formula component index */
    SMEM_FORMULA_DIM_STC * formulaCellPtr;  /* pointer to formula cell */
    GT_U32 formulaIndexArr[SMEM_FORMULA_CELL_NUM_CNS]; /* array of memory indexes */
    GT_BOOL rc;

    rc = smemFormulaChunkAddressCheck(address, memChunkPtr, formulaIndexArr);
    if(rc == GT_FALSE)
    {
        return SMEM_ADDRESS_NOT_IN_FORMULA_CNS;
    }

    for(compIndex = 0; compIndex < SMEM_FORMULA_CELL_NUM_CNS; compIndex++)
    {
        formulaCellPtr = &memChunkPtr->formulaData[compIndex];

        if (formulaCellPtr->size == 0 || formulaCellPtr->step == 0)
            break;

        /* Memory segment index */
        if(destMemIndex)
        {
            destMemIndex *= formulaCellPtr->size;
        }

        /* Absolute memory index */
        destMemIndex += formulaIndexArr[compIndex];
    }

    return destMemIndex;
}

/*******************************************************************************
*  smemInitMemChunkExt__internal
*
* DESCRIPTION:
*      Allocates and init unit memory as flat model or using formula
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
)
{
    GT_U32  ii;
    GT_U32 memoryWords;
    GT_U32 compIndex;
    SMEM_CHUNK_STC *currChunkPtr;   /*current memory chunk */
    SMEM_FORMULA_DIM_STC * formulaCellPtr;
    GT_U32 totalMemWords; /* total size of allccated memory in words */
    GT_BIT isFormulaHidenParam;/*is the formula hold 'hiden' parameter via the 'numOfRegisters' */

    unitChunksPtr->chunksArray = smemDeviceObjMemoryAlloc__internal(devObjPtr,numOfChunks, sizeof(SMEM_CHUNK_STC),fileNamePtr,line);
    if(unitChunksPtr->chunksArray == NULL)
    {
        skernelFatalError("smemInitMemChunkExt__internal: chunk array allocation failed \n" );
    }

    unitChunksPtr->numOfChunks = numOfChunks;

    currChunkPtr = unitChunksPtr->chunksArray;

    for(ii = 0 ; ii < numOfChunks; ii++, currChunkPtr++ )
    {
        if(chunksMemArrExt[ii].formulaCellArr[0].size ||
           chunksMemArrExt[ii].formulaCellArr[0].step)
        {
            currChunkPtr->enrtyNumWordsAlignement = 0;
            currChunkPtr->memFirstAddr = chunksMemArrExt[ii].memChunkBasic.memFirstAddr;
            memoryWords = 1;
            currChunkPtr->memLastAddr = currChunkPtr->memFirstAddr;

            isFormulaHidenParam = 0;
            /* Memory will be allocated according to formula */
            for(compIndex = 0; compIndex < SMEM_FORMULA_CELL_NUM_CNS; compIndex++)
            {
                formulaCellPtr =
                    &chunksMemArrExt[ii].formulaCellArr[compIndex];

                if (formulaCellPtr->size == 0 || formulaCellPtr->step == 0)
                {
                    /* numOfRegisters == 1 only cause confusion and not change calculations
                       so need to ignore it */
                    if(chunksMemArrExt[ii].memChunkBasic.numOfRegisters > 1)
                    {
                        /* use this info as 'last formula' parameter (size,step) */
                        formulaCellPtr->size = chunksMemArrExt[ii].memChunkBasic.numOfRegisters;
                        formulaCellPtr->step = 4;

                        isFormulaHidenParam = 1;/* cause 'break' after calculating new info */
                    }
                    else
                    {
                        break;
                    }
                }

                memoryWords *= formulaCellPtr->size;
                currChunkPtr->memLastAddr +=
                    (formulaCellPtr->size - 1) * formulaCellPtr->step;

                if(isFormulaHidenParam)
                {
                    /* increment the value due to use by smemChunkFormulaSort() */
                    compIndex++;
                    break;
                }
            }


            totalMemWords = memoryWords;
            currChunkPtr->memSize = totalMemWords;
            currChunkPtr->memPtr = smemDeviceObjMemoryAlloc__internal(devObjPtr,totalMemWords, sizeof(SMEM_REGISTER),fileNamePtr,line);
            if(currChunkPtr->memPtr == 0)
            {
                skernelFatalError("smemInitMemChunkExt__internal: allocation failed for [%d] bytes \n" ,
                    totalMemWords * sizeof(SMEM_REGISTER));
            }
            currChunkPtr->calcIndexType = SMEM_INDEX_IN_CHUNK_TYPE_FORMULA_FUNCTION_E;
            currChunkPtr->formulaFuncPtr = smemFormulaChunkIndexGet;
            memcpy(&currChunkPtr->formulaData[0],
                   &chunksMemArrExt[ii].formulaCellArr[0],
                   sizeof(currChunkPtr->formulaData));

            if(chunksMemArrExt[ii].memChunkBasic.tableOffsetValid)
            {
                currChunkPtr->enrtyNumWordsAlignement = /* support tables bound to this formula*/
                    chunksMemArrExt[ii].memChunkBasic.enrtyNumBytesAlignement / 4;
                currChunkPtr->tableOffsetInBytes = chunksMemArrExt[ii].memChunkBasic.tableOffsetInBytes;
            }
            else
            {
                currChunkPtr->tableOffsetInBytes = SMAIN_NOT_VALID_CNS;
            }

            smemChunkFormulaSort(&currChunkPtr->formulaData[0], compIndex);

            /* Check chunk for overlapping */
            smemUnitChunkOverlapCheck(unitChunksPtr, ii, currChunkPtr);
        }
        else
        {
            smemInitBasicMemChunk(devObjPtr,unitChunksPtr,
                                  &chunksMemArrExt[ii].memChunkBasic,
                                  ii, currChunkPtr,
                                  fileNamePtr,line);
        }
    }
}

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
)
{
    GT_U32 i;
    SMEM_CHUNK_STC currChunk;
    SMEM_CHUNK_STC *tempChunksArray;

    GT_U32  combinedNumChunks = firstUnitChunks->numOfChunks + secondUnitChunks->numOfChunks;

    /* Check for chunks overlapping in source and destination units */
    for(i = 0; i < secondUnitChunks->numOfChunks; i++)
    {
        currChunk.memFirstAddr = secondUnitChunks->chunksArray[i].memFirstAddr;
        currChunk.memLastAddr = secondUnitChunks->chunksArray[i].memLastAddr;
        currChunk.formulaFuncPtr = secondUnitChunks->chunksArray[i].formulaFuncPtr;
        memcpy(&currChunk.formulaData[0],
               &secondUnitChunks->chunksArray[i].formulaData[0],
               sizeof(currChunk.formulaData));
        /* Check chunk for overlapping */
        smemUnitChunkOverlapCheck(firstUnitChunks, firstUnitChunks->numOfChunks,
                                  &currChunk);
    }

    /* resize the allocation of the second (the source) unit */
    tempChunksArray = smemDeviceObjMemoryRealloc__internal(devObjPtr,secondUnitChunks->chunksArray, combinedNumChunks*sizeof(SMEM_CHUNK_STC),
        fileNamePtr,line);
    if (tempChunksArray == NULL) {
        skernelFatalError("smemInitMemCombineUnitChunks__internal: re-allocation failed for [%d] bytes \n" ,
                          combinedNumChunks*sizeof(SMEM_CHUNK_STC));
    }
    secondUnitChunks->chunksArray = tempChunksArray;

    /* copy the info from second unit after the end of the first unit */
    memcpy(&secondUnitChunks->chunksArray[secondUnitChunks->numOfChunks],
           &firstUnitChunks->chunksArray[0],
           sizeof(SMEM_CHUNK_STC)* firstUnitChunks->numOfChunks);

    /* update the number of chunks in the first unit */
    firstUnitChunks->numOfChunks = combinedNumChunks;

    /* free the unused pointer of the first unit */
    smemDeviceObjMemoryPtrFree__internal(devObjPtr,firstUnitChunks->chunksArray,fileNamePtr,line);

    /* update the first unit with the memories of the 'second' */
    firstUnitChunks->chunksArray = secondUnitChunks->chunksArray;

    return;
}

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
)
{
    GT_U32 ii, jj;
    SMEM_CHUNK_STC *newChunkPtr, *foundChunkPtr = 0;
    GT_U32 numRegs;
    GT_U32 totalMemWords; /* total size of allccated memory in words */
    GT_U32  *tempMemPtr;

    ii = 0;
    /* Go through all "append" chunks array */
    while(ii < unitChunksAppendPtr->numOfChunks)
    {
        foundChunkPtr = 0;
        newChunkPtr = &unitChunksAppendPtr->chunksArray[ii];
        /* Find matching in "append to" chunks array */
        for(jj = 0; jj < unitChunksAppendToPtr->numOfChunks; jj++)
        {
            if(newChunkPtr->memFirstAddr ==
               unitChunksAppendToPtr->chunksArray[jj].memFirstAddr)
            {
                /* Chunk found in "append to" chunks array*/
                foundChunkPtr = &unitChunksAppendToPtr->chunksArray[jj];
                /* No chunks duplication in chunks array */
                break;
            }
        }

        if(foundChunkPtr)
        {
            /* Override old chunk info by new one */
            foundChunkPtr->calcIndexType = newChunkPtr->calcIndexType;
            memcpy(foundChunkPtr->formulaData, newChunkPtr->formulaData,
                   sizeof(newChunkPtr->formulaData));
            foundChunkPtr->formulaFuncPtr = newChunkPtr->formulaFuncPtr;
            foundChunkPtr->memFirstAddr = newChunkPtr->memFirstAddr;
            foundChunkPtr->memLastAddr = newChunkPtr->memLastAddr;
            foundChunkPtr->lastEntryWordIndex = newChunkPtr->lastEntryWordIndex;
            foundChunkPtr->enrtyNumWordsAlignement = newChunkPtr->enrtyNumWordsAlignement;
            foundChunkPtr->memMask = newChunkPtr->memMask;
            numRegs = (newChunkPtr->memLastAddr - newChunkPtr->memFirstAddr + 4) / 4;

            totalMemWords = numRegs + foundChunkPtr->enrtyNumWordsAlignement;
            foundChunkPtr->memSize = totalMemWords;
            tempMemPtr = smemDeviceObjMemoryRealloc__internal(devObjPtr,foundChunkPtr->memPtr,
                                 totalMemWords * sizeof(SMEM_REGISTER),fileNamePtr,line);
            if (tempMemPtr == NULL) {
                skernelFatalError("smemInitMemUpdateUnitChunks__internal: re-allocation failed for [%d] bytes \n" ,
                                  totalMemWords * sizeof(SMEM_REGISTER));
            }
            foundChunkPtr->memPtr = tempMemPtr;
            foundChunkPtr->rightShiftNumBits = newChunkPtr->rightShiftNumBits;

            if(unitChunksAppendPtr->numOfChunks)
            {
                /* Decrease number of chunks to  be appended to "append to" chunks array */
                unitChunksAppendPtr->numOfChunks--;
                /* Index inside chunks range */
                if(ii < unitChunksAppendPtr->numOfChunks)
                {
                    /* Remove chunk from "append" chunks array */
                    memmove(&unitChunksAppendPtr->chunksArray[ii],
                            &unitChunksAppendPtr->chunksArray[ii + 1],
                            sizeof(SMEM_CHUNK_STC) * (unitChunksAppendPtr->numOfChunks - ii));
                }
            }
        }
        else
        {
            /* Skip to next chunk in "append" chunks array */
            ii++;
        }
    }

    if(unitChunksAppendPtr->numOfChunks)
    {
        /* Combine chunks array */
        smemInitMemCombineUnitChunks__internal(devObjPtr,unitChunksAppendToPtr, unitChunksAppendPtr,
            fileNamePtr,line);
    }
}

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
)
{
    SMEM_UNIT_CHUNKS_STC  tmpUnitChunk;

    /* create temp unit */
    smemInitMemChunk__internal(devObjPtr, basicChunkPtr, numBasicElements, &tmpUnitChunk,fileNamePtr,line);

    /*add the tmp unit chunks to the main unit */
    smemInitMemCombineUnitChunks__internal(devObjPtr, unitChunkPtr, &tmpUnitChunk,fileNamePtr,line);
}


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
)
{
    if(deviceId >= SMAIN_MAX_NUM_OF_DEVICES_CNS || smainDeviceObjects[deviceId] == NULL)
    {
        skernelFatalError("snetChtTestDeviceIdToDevPtrConvert: unknown device-id[%d] \n",deviceId);
    }

    return smainDeviceObjects[deviceId];
}

/*******************************************************************************
* activeMemoryTableValidity
*
* DESCRIPTION:
*        test utility -- check that all the active memory addresses exists
*                        in the device memory
*
* INPUTS:
*    activeMemPtr - active memory table (of unit / device)
*
* OUTPUTS:
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
static void  activeMemoryTableValidity(

    IN GT_U32 devNum,
    IN SMEM_ACTIVE_MEM_ENTRY_STC  *activeMemPtr
)
{
    GT_U32  ii;
    GT_U32  data;

    /* read from the registers , if not exists it will cause fatal error */
    for(ii = 0 ;activeMemPtr[ii].address != END_OF_TABLE ;ii++)
    {
        scibReadMemory(devNum,
            activeMemPtr[ii].address & activeMemPtr[ii].mask,
            1,&data);
    }
}

/*******************************************************************************
* activeMemoryTestValidity
*
* DESCRIPTION:
*        test utility -- check that all the active memory addresses exists
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
static GT_U32  activeMemoryTestValidity(
    IN GT_U32 devNum
)
{
    GT_U32  ii;
    SKERNEL_DEVICE_OBJECT    *devObjPtr = smemTestDeviceIdToDevPtrConvert(devNum);
    SMEM_ACTIVE_MEM_ENTRY_STC  *activeMemPtr = devObjPtr->activeMemPtr;
    SMEM_CHT_DEV_COMMON_MEM_INFO  * commonDevMemInfoPtr;

    /* validate the active memory of the device */
    activeMemoryTableValidity(devNum,activeMemPtr);

    /* the device memory always starts with 'common info' */
    commonDevMemInfoPtr = devObjPtr->deviceMemory;

    if(devObjPtr->supportActiveMemPerUnit)
    {
        for(ii = 0 ; ii < SMEM_CHT_NUM_UNITS_MAX_CNS ; ii++)
        {
            if(commonDevMemInfoPtr->specFunTbl[ii].specParam == 0 ||
               commonDevMemInfoPtr->specFunTbl[ii].unitActiveMemPtr == NULL)
            {
                continue;
            }

            /* active memory of the unit */
            activeMemPtr = commonDevMemInfoPtr->specFunTbl[ii].unitActiveMemPtr;

            /* validate the active memory of the unit */
            activeMemoryTableValidity(devNum,activeMemPtr);
        }
    }

    return 0;
}

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
)
{
    *tablesPtr = (SKERNEL_TABLE_INFO_STC*)(void*)(
                (GT_UINTPTR)(void*)&devObjPtr->tablesInfo.placeHolderFor1Parameter + sizeof(devObjPtr->tablesInfo.placeHolderFor1Parameter));
    *numTablesPtr = ((GT_UINTPTR)(void*)&devObjPtr->tablesInfo.placeHolderFor2Parameters) -
                ((GT_UINTPTR)(void*)(*tablesPtr)) ;
    *numTablesPtr /= sizeof(SKERNEL_TABLE_INFO_STC);

    *tables2ParamsPtr = (SKERNEL_TABLE_2_PARAMETERS_INFO_STC*)(void*)(
                (GT_UINTPTR)(void*)&devObjPtr->tablesInfo.placeHolderFor2Parameters + sizeof(devObjPtr->tablesInfo.placeHolderFor2Parameters));
    *numTables2ParamsPtr = ((GT_UINTPTR)(void*)(&devObjPtr->tablesInfo.placeHolderFor3Parameters)) -
                       ((GT_UINTPTR)(void*)(*tables2ParamsPtr));
    *numTables2ParamsPtr /= sizeof(SKERNEL_TABLE_2_PARAMETERS_INFO_STC);

    *tables3ParamsPtr = (SKERNEL_TABLE_3_PARAMETERS_INFO_STC*)(void*)(
                (GT_UINTPTR)(void*)&devObjPtr->tablesInfo.placeHolderFor3Parameters + sizeof(devObjPtr->tablesInfo.placeHolderFor3Parameters));
    *numTables3ParamsPtr = ((GT_UINTPTR)(void*)(&devObjPtr->tablesInfo.placeHolderFor4Parameters)) -
                       ((GT_UINTPTR)(void*)(*tables3ParamsPtr));
    *numTables3ParamsPtr /= sizeof(SKERNEL_TABLE_3_PARAMETERS_INFO_STC);

    *tables4ParamsPtr = (SKERNEL_TABLE_4_PARAMETERS_INFO_STC*)(void*)(
                (GT_UINTPTR)(void*)&devObjPtr->tablesInfo.placeHolderFor4Parameters + sizeof(devObjPtr->tablesInfo.placeHolderFor4Parameters));
    *numTables4ParamsPtr = ((GT_UINTPTR)(void*)(&devObjPtr->tablesInfo.placeHolder_MUST_BE_LAST)) -
                       ((GT_UINTPTR)(void*)(*tables4ParamsPtr));
    *numTables4ParamsPtr /= sizeof(SKERNEL_TABLE_4_PARAMETERS_INFO_STC);

}


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
)
{
    GT_U32  ii,jj;
    GT_U32  data;
    SKERNEL_DEVICE_OBJECT  *devObjPtr = smemTestDeviceIdToDevPtrConvert(devNum);
    SKERNEL_TABLE_INFO_STC  *tablesPtr;/* pointer to tables with single parameter */
    GT_U32  numTables;/* number of tables with single parameter */
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC  *tables2ParamsPtr; /* pointer to tables with 2 parameters */
    GT_U32  numTables2Params;/* number of tables with 2 parameters */
    SKERNEL_TABLE_3_PARAMETERS_INFO_STC  *tables3ParamsPtr; /* pointer to tables with 3 parameters */
    GT_U32  numTables3Params;/* number of tables with 3 parameters */
    SKERNEL_TABLE_4_PARAMETERS_INFO_STC  *tables4ParamsPtr; /* pointer to tables with 4 parameters */
    GT_U32  numTables4Params;/* number of tables with 4 parameters */
    GT_U32  baseAddress;
    GT_U32  firstValidAddress;/* allow the first valid address to differ from baseAddress
                                used in validation only if firstValidAddress != 0 */

    smemGenericTablesSectionsGet(devObjPtr,
                            &tablesPtr,&numTables,
                            &tables2ParamsPtr,&numTables2Params,
                            &tables3ParamsPtr,&numTables3Params,
                            &tables4ParamsPtr,&numTables4Params);

    /* read from the registers , if not exists it will cause fatal error */
    for(ii = 0 ;ii < numTables ;ii++)
    {
        if(tablesPtr[ii].commonInfo.baseAddress == 0)
        {
            /* not supported table in the device */
            continue;
        }

        baseAddress = tablesPtr[ii].commonInfo.baseAddress;

        firstValidAddress = tablesPtr[ii].commonInfo.firstValidAddress ?
            tablesPtr[ii].commonInfo.firstValidAddress :
            baseAddress ;

        smemRegGet(devObjPtr,
                   firstValidAddress,
                   &data);

        /* support multi instance tables (duplications between units) */
        for(jj = 0 ; jj < tablesPtr[ii].commonInfo.multiInstanceInfo.numBaseAddresses ; jj++)
        {
            SET_MULTI_INSTANCE_ADDRESS_MAC(firstValidAddress,
                    tablesPtr[ii].commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr[jj]);

            if(tablesPtr[ii].commonInfo.multiInstanceInfo.multiUnitsBaseAddressValidPtr &&
               tablesPtr[ii].commonInfo.multiInstanceInfo.multiUnitsBaseAddressValidPtr[jj] == GT_FALSE)
            {
                /* this instance of the multi instances is not valid ... skip it */
                continue;
            }

            smemRegGet(devObjPtr,
                       firstValidAddress,
                       &data);
        }
    }

    /* read from the registers , if not exists it will cause fatal error */
    for(ii = 0 ;ii < numTables2Params ;ii++)
    {
        if(tables2ParamsPtr[ii].commonInfo.baseAddress == 0)
        {
            /* not supported table in the device */
            continue;
        }

        baseAddress = tables2ParamsPtr[ii].commonInfo.baseAddress;

        firstValidAddress = tables2ParamsPtr[ii].commonInfo.firstValidAddress ?
            tablesPtr[ii].commonInfo.firstValidAddress :
            baseAddress ;

        smemRegGet(devObjPtr,
                   firstValidAddress,
                   &data);

        /* support multi instance tables (duplications between units) */
        for(jj = 0 ; jj < tables2ParamsPtr[ii].commonInfo.multiInstanceInfo.numBaseAddresses ; jj++)
        {
            SET_MULTI_INSTANCE_ADDRESS_MAC(firstValidAddress,
                    tables2ParamsPtr[ii].commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr[jj]);

            if(tablesPtr[ii].commonInfo.multiInstanceInfo.multiUnitsBaseAddressValidPtr &&
               tablesPtr[ii].commonInfo.multiInstanceInfo.multiUnitsBaseAddressValidPtr[jj] == GT_FALSE)
            {
                /* this instance of the multi instances is not valid ... skip it */
                continue;
            }

            smemRegGet(devObjPtr,
                       firstValidAddress,
                       &data);
        }
    }

    /* read from the registers , if not exists it will cause fatal error */
    for(ii = 0 ;ii < numTables3Params ;ii++)
    {
        if(tables3ParamsPtr[ii].commonInfo.baseAddress == 0)
        {
            /* not supported table in the device */
            continue;
        }

        baseAddress = tables3ParamsPtr[ii].commonInfo.baseAddress;

        firstValidAddress = tables3ParamsPtr[ii].commonInfo.firstValidAddress ?
            tablesPtr[ii].commonInfo.firstValidAddress :
            baseAddress ;

        smemRegGet(devObjPtr,
                   firstValidAddress,
                   &data);

        /* support multi instance tables (duplications between units) */
        for(jj = 0 ; jj < tables3ParamsPtr[ii].commonInfo.multiInstanceInfo.numBaseAddresses ; jj++)
        {
            SET_MULTI_INSTANCE_ADDRESS_MAC(firstValidAddress,
                    tables3ParamsPtr[ii].commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr[jj]);

            if(tablesPtr[ii].commonInfo.multiInstanceInfo.multiUnitsBaseAddressValidPtr &&
               tablesPtr[ii].commonInfo.multiInstanceInfo.multiUnitsBaseAddressValidPtr[jj] == GT_FALSE)
            {
                /* this instance of the multi instances is not valid ... skip it */
                continue;
            }

            smemRegGet(devObjPtr,
                       firstValidAddress,
                       &data);
        }
    }

    /* read from the registers , if not exists it will cause fatal error */
    for(ii = 0 ;ii < numTables4Params ;ii++)
    {
        if(tables4ParamsPtr[ii].commonInfo.baseAddress == 0)
        {
            /* not supported table in the device */
            continue;
        }

        baseAddress = tables4ParamsPtr[ii].commonInfo.baseAddress;

        firstValidAddress = tables4ParamsPtr[ii].commonInfo.firstValidAddress ?
            tablesPtr[ii].commonInfo.firstValidAddress :
            baseAddress ;

        smemRegGet(devObjPtr,
                   firstValidAddress,
                   &data);

        /* support multi instance tables (duplications between units) */
        for(jj = 0 ; jj < tables4ParamsPtr[ii].commonInfo.multiInstanceInfo.numBaseAddresses ; jj++)
        {
            SET_MULTI_INSTANCE_ADDRESS_MAC(firstValidAddress,
                    tables4ParamsPtr[ii].commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr[jj]);

            if(tablesPtr[ii].commonInfo.multiInstanceInfo.multiUnitsBaseAddressValidPtr &&
               tablesPtr[ii].commonInfo.multiInstanceInfo.multiUnitsBaseAddressValidPtr[jj] == GT_FALSE)
            {
                /* this instance of the multi instances is not valid ... skip it */
                continue;
            }

            smemRegGet(devObjPtr,
                       firstValidAddress,
                       &data);
        }
    }


    return 0;
}



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
)
{
    GT_U32 *memAddrPtr;
    GT_U32  ii,jj,kk,data,iiMax,address, *dataPtr;
    DEV_TABLE_INFO_STC *tablePtr;
    GT_U32  tableEntry[MAX_ENTRY_SIZE_IN_WORDS_CNS];/*enough space for 'Table entry' */
    GT_U32  numWordsInTableEntry;
    SKERNEL_DEVICE_OBJECT * devObjPtr = smemTestDeviceIdToDevPtrConvert(devNum);

    if(testActiveMemory == GT_TRUE)
    {
        /* check the active memory table */
        activeMemoryTestValidity(devNum);
    }

    /*check the generic table base addresses */
    genericTablesTestValidity(devNum);

    /**********************/
    /* checking registers */
    /**********************/

    iiMax = numRegisters;
    memAddrPtr = registersAddrPtr;


    /* fill the registers with data */
    for(ii = 0 ; ii < iiMax; ii++,memAddrPtr++)
    {
        /* write to each register it's address as value */
        data = *memAddrPtr;
        address = *memAddrPtr;

        smemMemSet(devObjPtr, address, &data, 1);
    }

    memAddrPtr = registersAddrPtr;
    /* check the values in those registers */
    for(ii = 0 ; ii < iiMax; ii++, memAddrPtr++)
    {
        /* write to each register it's address as value */
        address = *memAddrPtr;

        dataPtr = smemMemGet(devObjPtr, address);
        data = *dataPtr;
        if(data != (*memAddrPtr))
        {
            simForcePrintf("reg : address [0x%8.8x] hold data [0x%8.8x] instead of [0x%8.8x] \n",
            address,data,(*memAddrPtr));
        }
    }

    /********************/
    /* end of registers */
    /********************/

    /*******************/
    /* checking tables */
    /*******************/

    iiMax = numTables;
    tablePtr = tablesInfoPtr;
    /* fill the memories with data */
    for(ii = 0 ; ii < iiMax; ii++,tablePtr++)
    {
        numWordsInTableEntry = (tablePtr->entrySize + 31) / 32;

        if(numWordsInTableEntry >= MAX_ENTRY_SIZE_IN_WORDS_CNS)
        {
            skernelFatalError(" smemTestDevice: entry size [%d] over limit [%d]\n",
                numWordsInTableEntry,
                MAX_ENTRY_SIZE_IN_WORDS_CNS);
        }

        /* loop on all entries of the memory */
        for(kk = 0 ; kk < tablePtr->numOfEntries ; kk++)
        {
            /*calculate the base address of the current memory entry */
            address = tablePtr->baseAddr + (kk * tablePtr->lineAddrAlign * 4);

            /* write to each table word it's address as value */
            for(jj = 0; jj < numWordsInTableEntry; jj++,address+=4)
            {
                tableEntry[jj] = address;
                smemMemSet(devObjPtr, address, &tableEntry[jj], 1);
            }
        }
    }



    tablePtr = tablesInfoPtr;
    /* check the data in the memories */
    for(ii = 0 ; ii < iiMax; ii++,tablePtr++)
    {
        numWordsInTableEntry = (tablePtr->entrySize + 31) / 32;

        /* loop on all entries of the memory */
        for(kk = 0 ; kk < tablePtr->numOfEntries ; kk++)
        {
            /*calculate the base address of the current memory entry */
            address = tablePtr->baseAddr + (kk * tablePtr->lineAddrAlign * 4);

            /* write to each table word it's address as value */
            for(jj = 0; jj < numWordsInTableEntry; jj++,address+=4)
            {
                dataPtr = smemMemGet(devObjPtr, address);
                tableEntry[jj] = *dataPtr;

                if(tableEntry[jj] != address)
                {
                    simForcePrintf("tbl[%s] : address [0x%8.8x] hold data [0x%8.8x] instead of [0x%8.8x] line[%d] word[%d] (table index[%d]) \n",
                    tablePtr->tableName,address,tableEntry[jj],address,kk,jj,ii);
                }
            }
        }
    }
}

/*******************************************************************************
*   checkIndexOutOfRange
*
* DESCRIPTION:
*       function check if specific index of dimension of table is out of range.
*       if check 'fails' then cause FATAL_ERROR !
* INPUTS:
*       tablePtr - pointer to table info
*       indexInTablePtr_paramInfo - index in tablePtr->paramInfo[]
*       indexToCheck - index to check
* OUTPUTS:
*
*
* RETURNS:
*        NONE.
*
* COMMENTS:
*
*
*******************************************************************************/
static void checkIndexOutOfRange(
    IN  SKERNEL_TABLE_2_PARAMETERS_INFO_STC    *tablePtr,
    IN  GT_U32                    indexInTablePtr_paramInfo,
    IN  GT_U32                    indexToCheck
)
{
    GT_U32  outOfRangeIndex = tablePtr->paramInfo[indexInTablePtr_paramInfo].outOfRangeIndex;

    if (outOfRangeIndex == 0)
    {
        /* this 'dimension' of the table not hold validation value ! */
        return;
    }

    if (indexToCheck >= outOfRangeIndex)
    {
        skernelFatalError("checkIndexOutOfRange: table[%s] : indexToCheck[%d] out of range[%d] paramIndex[%d]\n",
            tablePtr->commonInfo.nameString,indexToCheck,outOfRangeIndex,indexInTablePtr_paramInfo);
    }
}

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
)
{
    GT_U32  divider;
    GT_U32  baseAddress;

    divider = tablePtr->paramInfo[0].divider ?
              tablePtr->paramInfo[0].divider : 1;

    if(tablePtr->paramInfo[0].modulo)
    {
        index %= tablePtr->paramInfo[0].modulo;
    }

    baseAddress = tablePtr->commonInfo.baseAddress;
    if(instanceIndex != SMAIN_NOT_VALID_CNS && tablePtr->commonInfo.multiInstanceInfo.numBaseAddresses)
    {
        if(instanceIndex >= tablePtr->commonInfo.multiInstanceInfo.numBaseAddresses)
        {
            skernelFatalError("smemTableEntryAddressGet: table[%s] : instanceIndex[%d] out of range[%d]\n",
                tablePtr->commonInfo.nameString);
        }
        /* set the 6 bits of the unit base address instead of the address of the 'baseAddress'*/
        SET_MULTI_INSTANCE_ADDRESS_MAC(baseAddress,
                tablePtr->commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr[instanceIndex]);

        if(tablePtr->commonInfo.multiInstanceInfo.multiUnitsBaseAddressValidPtr &&
           tablePtr->commonInfo.multiInstanceInfo.multiUnitsBaseAddressValidPtr[instanceIndex] == GT_FALSE)
        {
            /* this instance of the multi instances is not valid ... Error */
            skernelFatalError("smemTableEntryAddressGet: table[%s] : instanceIndex[%d] not valid\n",
                tablePtr->commonInfo.nameString,instanceIndex);
        }
    }

    if(baseAddress == 0)
    {
        skernelFatalError("smemTableEntryAddressGet: table[%s] : hold no valid baseAddress \n",
            tablePtr->commonInfo.nameString);
    }

    checkIndexOutOfRange(tablePtr,0/*index in tablePtr->paramInfo[]*/,index);

    return baseAddress +
           tablePtr->paramInfo[0].step * (index / divider);
}

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
)
{
    GT_U32  divider1,divider2;
    GT_U32  baseAddress;

    divider1 = tablePtr->paramInfo[0].divider ? tablePtr->paramInfo[0].divider : 1;
    divider2 = tablePtr->paramInfo[1].divider ? tablePtr->paramInfo[1].divider : 1;

    if(tablePtr->paramInfo[0].modulo)
    {
        index1 %= tablePtr->paramInfo[0].modulo;
    }

    if(tablePtr->paramInfo[1].modulo)
    {
        index2 %= tablePtr->paramInfo[1].modulo;
    }

    baseAddress = tablePtr->commonInfo.baseAddress;
    if(instanceIndex != SMAIN_NOT_VALID_CNS && tablePtr->commonInfo.multiInstanceInfo.numBaseAddresses)
    {
        if(instanceIndex >= tablePtr->commonInfo.multiInstanceInfo.numBaseAddresses)
        {
            skernelFatalError("smemTableEntry2ParamsAddressGet: instanceIndex[%d] out of range[%d]\n",
                instanceIndex,tablePtr->commonInfo.multiInstanceInfo.numBaseAddresses);
        }
        /* set the 6 bits of the unit base address instead of the address of the 'baseAddress'*/
        SET_MULTI_INSTANCE_ADDRESS_MAC(baseAddress,
                tablePtr->commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr[instanceIndex]);
    }

    if(baseAddress == 0)
    {
        skernelFatalError("smemTableEntry2ParamsAddressGet: table[%s] : hold no valid baseAddress \n",
            tablePtr->commonInfo.nameString);
    }

    checkIndexOutOfRange(tablePtr,0/*index in tablePtr->paramInfo[]*/,index1);
    checkIndexOutOfRange(tablePtr,1/*index in tablePtr->paramInfo[]*/,index2);


    return baseAddress +
           tablePtr->paramInfo[0].step * (index1 / divider1) +
           tablePtr->paramInfo[1].step * (index2 / divider2) ;
}


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
)
{
    GT_U32  divider[3];
    GT_U32  ii;
    GT_U32  baseAddress;

    for(ii = 0 ; ii < 3 ; ii++)
    {
        divider[ii] = tablePtr->paramInfo[ii].divider ?
                      tablePtr->paramInfo[ii].divider : 1;
    }

    ii = 0;
    if(tablePtr->paramInfo[ii].modulo)
    {
        index1 %= tablePtr->paramInfo[ii].modulo;
    }
    ii ++;
    if(tablePtr->paramInfo[ii].modulo)
    {
        index2 %= tablePtr->paramInfo[ii].modulo;
    }
    ii ++;
    if(tablePtr->paramInfo[ii].modulo)
    {
        index3 %= tablePtr->paramInfo[ii].modulo;
    }

    baseAddress = tablePtr->commonInfo.baseAddress;
    if(instanceIndex != SMAIN_NOT_VALID_CNS && tablePtr->commonInfo.multiInstanceInfo.numBaseAddresses)
    {
        if(instanceIndex >= tablePtr->commonInfo.multiInstanceInfo.numBaseAddresses)
        {
            skernelFatalError("smemTableEntry3ParamsAddressGet: instanceIndex[%d] out of range[%d]\n",
                instanceIndex,tablePtr->commonInfo.multiInstanceInfo.numBaseAddresses);
        }
        /* set the 6 bits of the unit base address instead of the address of the 'baseAddress'*/
        SET_MULTI_INSTANCE_ADDRESS_MAC(baseAddress,
                tablePtr->commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr[instanceIndex]);
    }

    if(baseAddress == 0)
    {
        skernelFatalError("smemTableEntry3ParamsAddressGet: table[%s] : hold no valid baseAddress \n",
            tablePtr->commonInfo.nameString);
    }

    checkIndexOutOfRange(tablePtr,0/*index in tablePtr->paramInfo[]*/,index1);
    checkIndexOutOfRange(tablePtr,1/*index in tablePtr->paramInfo[]*/,index2);
    checkIndexOutOfRange(tablePtr,2/*index in tablePtr->paramInfo[]*/,index3);

    return baseAddress
           + tablePtr->paramInfo[0].step * (index1 / divider[0])
           + tablePtr->paramInfo[1].step * (index2 / divider[1])
           + tablePtr->paramInfo[2].step * (index3 / divider[2])
           ;
}

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
)
{
    GT_U32  divider[4];
    GT_U32  ii;
    GT_U32  baseAddress;

    for(ii = 0 ; ii < 4 ; ii++)
    {
        divider[ii] = tablePtr->paramInfo[ii].divider ?
                      tablePtr->paramInfo[ii].divider : 1;
    }

    ii = 0;
    if(tablePtr->paramInfo[ii].modulo)
    {
        index1 %= tablePtr->paramInfo[ii].modulo;
    }
    ii ++;
    if(tablePtr->paramInfo[ii].modulo)
    {
        index2 %= tablePtr->paramInfo[ii].modulo;
    }
    ii ++;
    if(tablePtr->paramInfo[ii].modulo)
    {
        index3 %= tablePtr->paramInfo[ii].modulo;
    }
    ii ++;
    if(tablePtr->paramInfo[ii].modulo)
    {
        index4 %= tablePtr->paramInfo[ii].modulo;
    }

    baseAddress = tablePtr->commonInfo.baseAddress;
    if(instanceIndex != SMAIN_NOT_VALID_CNS && tablePtr->commonInfo.multiInstanceInfo.numBaseAddresses)
    {
        if(instanceIndex >= tablePtr->commonInfo.multiInstanceInfo.numBaseAddresses)
        {
            skernelFatalError("smemTableEntry4ParamsAddressGet: instanceIndex[%d] out of range[%d]\n",
                instanceIndex,tablePtr->commonInfo.multiInstanceInfo.numBaseAddresses);
        }
        /* set the 6 bits of the unit base address instead of the address of the 'baseAddress'*/
        SET_MULTI_INSTANCE_ADDRESS_MAC(baseAddress,
                tablePtr->commonInfo.multiInstanceInfo.multiUnitsBaseAddressPtr[instanceIndex]);
    }

    if(baseAddress == 0)
    {
        skernelFatalError("smemTableEntry4ParamsAddressGet: table[%s] : hold no valid baseAddress \n",
            tablePtr->commonInfo.nameString);
    }

    checkIndexOutOfRange(tablePtr,0/*index in tablePtr->paramInfo[]*/,index1);
    checkIndexOutOfRange(tablePtr,1/*index in tablePtr->paramInfo[]*/,index2);
    checkIndexOutOfRange(tablePtr,2/*index in tablePtr->paramInfo[]*/,index3);
    checkIndexOutOfRange(tablePtr,3/*index in tablePtr->paramInfo[]*/,index4);

    return baseAddress
           + tablePtr->paramInfo[0].step * (index1 / divider[0])
           + tablePtr->paramInfo[1].step * (index2 / divider[1])
           + tablePtr->paramInfo[2].step * (index3 / divider[2])
           + tablePtr->paramInfo[3].step * (index4 / divider[3])
           ;
}

/*******************************************************************************
*  smemChunkInternalWriteDataCheck
*
* DESCRIPTION:
*       Controls internal buffer memory write
*
* INPUTS:
*       devObjPtr   - pointer to device pbject
*       address - memory chunk address to be controlled
*       chunkPtr  - pointer to chunksMemArr[x] , that is the chosen chunk.
*       memoryIndexPtr - the index in chunksMemArr[x]->memPtr[y],
*                       that represent the 'address' valid only when the chunk was found
*
* OUTPUT:
*       memoryIndexPtr - the index in local buffer when writing to non last word of entry, or
*                       original index of the found chunk when writing to last word of entry
*
* RETURNS:
*       None
*
* COMMENTS:
*       Returns local buffer while writing to non last word of entry.
*       On last word writing it copies start words from local buffer into
*       real memory and returns last word of real memory.
*
*******************************************************************************/
static GT_VOID smemChunkInternalWriteDataCheck
(
    IN SKERNEL_DEVICE_OBJECT    *devObjPtr,
    IN GT_U32                   address,
    IN SMEM_CHUNK_STC           *chunkPtr,
    INOUT GT_U32                *memoryIndexPtr
)
{
    GT_U32 currWordOffset;  /* entry current word offset */
    GT_U32 internalDataOffset; /* internal data entry offset */

    /* check if this memory is "Direct memory write by last word" */
    if (chunkPtr->lastEntryWordIndex == 0)
    {
        return;
    }

    /* Current word offset calculation in words */
    currWordOffset = (address >> 2) % chunkPtr->enrtyNumWordsAlignement;
    /* Get offset of internal entry buffer */
    internalDataOffset = (chunkPtr->memLastAddr - chunkPtr->memFirstAddr + 4) / 4;

    if (currWordOffset == chunkPtr->lastEntryWordIndex)
    {
        /* Copy data from the internal buffer to memory */
        memcpy(&chunkPtr->memPtr[*memoryIndexPtr - currWordOffset],
               &chunkPtr->memPtr[internalDataOffset],
               chunkPtr->enrtyNumWordsAlignement * sizeof(SMEM_REGISTER));
    }
    else
    {
        /* The word is not last one. Return index to internal memory */
        *memoryIndexPtr = internalDataOffset + currWordOffset;
    }
}

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
)
{
    GT_U32 unitsCount;          /* Units count */
    GT_U32 curUnit;
    GT_U32 curChunk;
    SMEM_CHUNK_STC * chunkPtr;
    SMEM_UNIT_CHUNKS_STC  * memPtr;
    GT_U32 byteCount;

    SMEM_CHT_DEV_COMMON_MEM_INFO  * commonMemPtr =
        (SMEM_CHT_DEV_COMMON_MEM_INFO  *)devObjPtr->deviceMemory;

    memPtr = (SMEM_UNIT_CHUNKS_STC *)commonMemPtr->unitChunksBasePtr;

    byteCount = commonMemPtr->unitChunksSizeOf;

    /* Number of allocated structures to be reset */
    unitsCount = byteCount / sizeof(SMEM_UNIT_CHUNKS_STC);
    for(curUnit = 0; curUnit < unitsCount; curUnit++)
    {
        for(curChunk = 0; curChunk < memPtr[curUnit].numOfChunks; curChunk++)
        {
            chunkPtr = &memPtr[curUnit].chunksArray[curChunk];
            memset(chunkPtr->memPtr, 0, chunkPtr->memSize * sizeof(SMEM_REGISTER));
        }
    }
}

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
)
{
    GT_U32 i;
    SMEM_CALLOC_STC * callocMemPtr;

    SMEM_CHT_DEV_COMMON_MEM_INFO  * commonMemPtr =
        (SMEM_CHT_DEV_COMMON_MEM_INFO  *)devObjPtr->deviceMemory;

    if (commonMemPtr->callocMemSize < SMEM_MAX_CALLOC_MEM_SIZE)
    {
        i = commonMemPtr->callocMemSize++;
        callocMemPtr = &commonMemPtr->callocMemArray[i];
        callocMemPtr->regPtr = smemDeviceObjMemoryAlloc(devObjPtr,sizeObjCount, sizeObjSizeof);
        callocMemPtr->regNum = sizeObjCount / 4;

        return callocMemPtr->regPtr;
    }
    else
    {
        skernelFatalError("smemChtCalloc: number of allocations exceeds maximum allowed\n");
    }

    return NULL;
}

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
)
{
    GT_U32                  chunkIndex;/*index of the chunk that hold the memory */
    GT_U32                  memIndex;  /* index in the memory that represent the address*/
    SMEM_UNIT_CHUNKS_STC    *unitChunksPtr;

    /* convert the param to be the pointer to the unit chunk */
    unitChunksPtr = (SMEM_UNIT_CHUNKS_STC*)(GT_U32*)param;

    if(unitChunksPtr->hugeUnitSupportPtr)
    {
        /* support for huge units */
        unitChunksPtr = unitChunksPtr->hugeUnitSupportPtr;
    }

    if(GT_FALSE == smemFindMemChunk(devObjPtr, accessType, unitChunksPtr, address,
                                    &chunkIndex, &memIndex))
    {
        /* memory was not found */
        return NULL;
    }

    return &unitChunksPtr->chunksArray[chunkIndex].memPtr[memIndex];
}

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
)
{
    SMEM_CHT_DEV_COMMON_MEM_INFO* deviceCommonMemPtr;
    void * findMemPtr = NULL;
    SMEM_UNIT_CHUNKS_STC * currentChunkMemPtr;
    GT_U32  ii;/*iterator*/
    SMEM_ACTIVE_MEM_ENTRY_STC   *unitActiveMemPtr = NULL;/*active memory dedicated to the unit*/
    SMEM_ACTIVE_MEM_ENTRY_STC   *activeMemPtr = NULL;/*active memory of the device/unit*/

    if (activeMemPtrPtr != NULL)
    {
        *activeMemPtrPtr = NULL;
    }

    deviceCommonMemPtr = (SMEM_CHT_DEV_COMMON_MEM_INFO*)devObjPtr->deviceMemory;

    if (SMEM_ACCESS_PCI_FULL_MAC(accessType))
    {
        if((address & deviceCommonMemPtr->pciUnitBaseAddrMask) == 0)
        {
            /* support accessing the memory also ‘0’ based */
            address |= deviceCommonMemPtr->pciExtMemArr[SMEM_UNIT_PCI_BUS_MBUS_E].unitBaseAddr;
        }
        currentChunkMemPtr =
            &deviceCommonMemPtr->pciExtMemArr[SMEM_UNIT_PCI_BUS_MBUS_E].unitMem;
        /* check if there is dedicated active memory for the unit */
        unitActiveMemPtr =
            currentChunkMemPtr->unitActiveMemPtr;

        activeMemPtr =
            unitActiveMemPtr;

        findMemPtr = (void *)smemDevFindInUnitChunk(devObjPtr, accessType,
                                                    address, memSize,
                                                    (GT_UINTPTR)currentChunkMemPtr);
    }
    else
    if (SMEM_ACCESS_DFX_FULL_MAC(accessType))
    {
        if(SMEM_CHT_IS_DFX_SERVER(devObjPtr) == GT_FALSE)
        {
            skernelFatalError("smemFindCommonMemory : DFX server not supported \n");
        }

        currentChunkMemPtr =
            &deviceCommonMemPtr->pciExtMemArr[SMEM_UNIT_PCI_BUS_DFX_E].unitMem;

        /* Device is port group and accessed from SKERNEL scope */
        if (SMEM_ACCESS_SKERNEL_FULL_MAC(accessType))
        {
            /* get representative core */
            devObjPtr = currentChunkMemPtr->otherPortGroupDevObjPtr ?
                    currentChunkMemPtr->otherPortGroupDevObjPtr : devObjPtr;
        }

        /* check if there is dedicated active memory for the unit */
        unitActiveMemPtr =
            currentChunkMemPtr->unitActiveMemPtr;

        findMemPtr = (void *)smemDevFindInUnitChunk(devObjPtr, accessType,
                                                    address, memSize,
                                                    (GT_UINTPTR)currentChunkMemPtr);
        if(findMemPtr == 0)
        {
            skernelFatalError("smemFindCommonMemory : DFX server memory address not found: [0x%8.8x]: \n", address);
        }

        activeMemPtr = unitActiveMemPtr;

    }

    /* find active memory entry */
    if (activeMemPtrPtr != NULL && activeMemPtr)
    {
        *activeMemPtrPtr = NULL;

        for (ii = 0; activeMemPtr[ii].address != END_OF_TABLE; ii++)
        {
            /* check address    */
            if ((address & activeMemPtr[ii].mask)
                 == activeMemPtr[ii].address)
            {
                *activeMemPtrPtr = &activeMemPtr[ii];
                break;
            }
        }
    }

    return findMemPtr;
}

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
)
{
    GT_U32  ii;
    GT_U32  regValue;
    GT_U32  regAddr = address;

    if(GT_FALSE ==
        smemIsDeviceMemoryOwner(deviceObjPtr,regAddr))
    {
        /* the address not belongs to the device */
        return;
    }

    for(ii = 0 ; ii < dataSize; ii++ , regAddr += 4)
    {
        smemRegGet(deviceObjPtr,regAddr,&regValue);
        scibWriteMemory(deviceObjPtr->deviceId,regAddr,1,&regValue);
    }

    return;
}

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
)
{
    GT_U32  ii;
    GT_U32  jj;
    GT_U32  tblIndex;
    SMEM_UNIT_CHUNKS_STC  *currentChunksPtr;
    SMEM_CHUNK_STC        *currentChunkPtr;
    SKERNEL_TABLE_INFO_STC  *tablesPtr;/* pointer to tables with single parameter */
    GT_U32  numTables;/* number of tables with single parameter */
    SKERNEL_TABLE_2_PARAMETERS_INFO_STC  *tables2ParamsPtr; /* pointer to tables with 2 parameters */
    GT_U32  numTables2Params;/* number of tables with 2 parameters */
    SKERNEL_TABLE_3_PARAMETERS_INFO_STC  *tables3ParamsPtr; /* pointer to tables with 3 parameters */
    GT_U32  numTables3Params;/* number of tables with 3 parameters */
    SKERNEL_TABLE_4_PARAMETERS_INFO_STC  *tables4ParamsPtr; /* pointer to tables with 4 parameters */
    GT_U32  numTables4Params;/* number of tables with 3 parameters */
    GT_U32  numBytesToStartOfTables1Param ;/* number of bytes to start of tables with 1 parameters */
    GT_U32  numBytesToStartOfTables2Params;/* number of bytes to start of tables with 2 parameters */
    GT_U32  numBytesToStartOfTables3Params;/* number of bytes to start of tables with 3 parameters */
    GT_U32  numBytesToStartOfTables4Params;/* number of bytes to start of tables with 4 parameters */

    smemGenericTablesSectionsGet(devObjPtr,
                            &tablesPtr,&numTables,
                            &tables2ParamsPtr,&numTables2Params,
                            &tables3ParamsPtr,&numTables3Params,
                            &tables4ParamsPtr,&numTables4Params);

    numBytesToStartOfTables1Param =  ((GT_U8*)(void*)tablesPtr -
                                      (GT_U8*)(void*)(&devObjPtr->tablesInfo));

    numBytesToStartOfTables2Params = ((GT_U8*)(void*)tables2ParamsPtr -
                                      (GT_U8*)(void*)(&devObjPtr->tablesInfo));

    numBytesToStartOfTables3Params = ((GT_U8*)(void*)tables3ParamsPtr -
                                      (GT_U8*)(void*)(&devObjPtr->tablesInfo));

    numBytesToStartOfTables4Params = ((GT_U8*)(void*)tables4ParamsPtr -
                                      (GT_U8*)(void*)(&devObjPtr->tablesInfo));

    currentChunksPtr = &chunksPtr[0];

    for(ii = 0 , currentChunksPtr = chunksPtr; ii < numOfUnits; ii++,currentChunksPtr++)
    {
        currentChunkPtr = &currentChunksPtr->chunksArray[0];
        for(jj = 0 ; jj < currentChunksPtr->numOfChunks ; jj++,currentChunkPtr++)
        {
            if(currentChunkPtr->tableOffsetInBytes == SMAIN_NOT_VALID_CNS)
            {
                continue;
            }

            /**************************************/
            /* check tables with single parameter */
            /**************************************/

            for(tblIndex = 0 ; tblIndex < numTables;tblIndex++)
            {
                if((currentChunkPtr->tableOffsetInBytes - numBytesToStartOfTables1Param) ==
                    (tblIndex) * sizeof(SKERNEL_TABLE_INFO_STC))
                {
                    /* found the table */
                    tablesPtr[tblIndex].commonInfo.baseAddress = currentChunkPtr->memFirstAddr;
                    tablesPtr[tblIndex].paramInfo[0].step =
                        currentChunkPtr->enrtyNumWordsAlignement * 4;/* step in bytes */
                    tablesPtr[tblIndex].commonInfo.memChunkPtr = currentChunkPtr;
                    break;
                }
            }

            if(tblIndex != numTables)
            {
                /* found table */
                continue;
            }

            /**********************************/
            /* check tables with 2 parameters */
            /**********************************/
            for(tblIndex = 0 ; tblIndex < numTables2Params;tblIndex++)
            {
                if((currentChunkPtr->tableOffsetInBytes - numBytesToStartOfTables2Params) ==
                    (tblIndex) * sizeof(SKERNEL_TABLE_2_PARAMETERS_INFO_STC))
                {
                    /* found the table */
                    tables2ParamsPtr[tblIndex].commonInfo.baseAddress = currentChunkPtr->memFirstAddr;
                    tables2ParamsPtr[tblIndex].paramInfo[0].step =
                        currentChunkPtr->enrtyNumWordsAlignement * 4;/* step in bytes */
                    tables2ParamsPtr[tblIndex].commonInfo.memChunkPtr = currentChunkPtr;
                    break;
                }
            }

            if(tblIndex != numTables2Params)
            {
                /* found table */
                continue;
            }


            /**********************************/
            /* check tables with 3 parameters */
            /**********************************/
            for(tblIndex = 0 ; tblIndex < numTables3Params;tblIndex++)
            {
                if((currentChunkPtr->tableOffsetInBytes - numBytesToStartOfTables3Params) ==
                    (tblIndex) * sizeof(SKERNEL_TABLE_3_PARAMETERS_INFO_STC))
                {
                    /* found the table */
                    tables3ParamsPtr[tblIndex].commonInfo.baseAddress = currentChunkPtr->memFirstAddr;
                    tables3ParamsPtr[tblIndex].paramInfo[0].step =
                        currentChunkPtr->enrtyNumWordsAlignement * 4;/* step in bytes */
                    tables3ParamsPtr[tblIndex].commonInfo.memChunkPtr = currentChunkPtr;
                    break;
                }
            }

            if(tblIndex != numTables3Params)
            {
                /* found table */
                continue;
            }

            /**********************************/
            /* check tables with 4 parameters */
            /**********************************/
            for(tblIndex = 0 ; tblIndex < numTables4Params;tblIndex++)
            {
                if((currentChunkPtr->tableOffsetInBytes - numBytesToStartOfTables4Params) ==
                    (tblIndex) * sizeof(SKERNEL_TABLE_4_PARAMETERS_INFO_STC))
                {
                    /* found the table */
                    tables4ParamsPtr[tblIndex].commonInfo.baseAddress = currentChunkPtr->memFirstAddr;
                    tables4ParamsPtr[tblIndex].paramInfo[0].step =
                        currentChunkPtr->enrtyNumWordsAlignement * 4;/* step in bytes */
                    tables4ParamsPtr[tblIndex].commonInfo.memChunkPtr = currentChunkPtr;
                    break;
                }
            }

            if(tblIndex != numTables4Params)
            {
                /* found table */
                continue;
            }
        }/*jj*/
    }/*ii*/
}
/*******************************************************************************
*  smemUnitChunksOverlapCheck
*
* DESCRIPTION:
*      Checks current chunk overlapping in unit's memory chunks
*
* INPUTS:
*       unitChunksPtr   - (pointer to) the unit memories chunks
*       unitChunkOffset - offset of chunk in the unit
*       currChunkPtr    - (pointer to) current chunk to be allocated
*
* OUTPUTS:
*       currChunkPtr    - (pointer to) current chunk to be allocated
*
* RETURNS:
*        none
*
* COMMENTS:
*       Check base address offset for currChunkPtr->memFirstAddr and currChunkPtr->memLastAddr.
*       If zero, make unit based adress:
*           memFirstAddr += unitChunksPtr->chunkIndex << 23
*           memLastAddr += unitChunksPtr->chunkIndex << 23
*
*******************************************************************************/
static GT_VOID smemUnitChunkOverlapCheck
(
    IN SMEM_UNIT_CHUNKS_STC * unitChunksPtr,
    IN GT_U32 unitChunkOffset,
    INOUT SMEM_CHUNK_STC * currChunkPtr
)
{
    GT_U32 i;
    GT_BOOL rc;

    for(i = 0; i < unitChunkOffset; i++)
    {
        /* Check chunk FIRST address */
        if(currChunkPtr->memFirstAddr >= unitChunksPtr->chunksArray[i].memFirstAddr &&
           currChunkPtr->memFirstAddr <= unitChunksPtr->chunksArray[i].memLastAddr)
        {
            if(unitChunksPtr->chunksArray[i].formulaFuncPtr)
            {
                rc = smemUnitChunkFormulaOverlapCheck(unitChunksPtr, i,
                                                      currChunkPtr,
                                                      currChunkPtr->memFirstAddr);
                if(rc == GT_TRUE)
                {
                    /* Don't check last address, skip to next chunk */
                    continue;
                }
            }
            else
            {
                simWarningPrintf("smemUnitChunkOverlapCheck: "
                                  "overlapped chunk range:  = [0x%8.8x] [0x%8.8x]\n"
                                  "unit chunk range: [0x%8.8x] [0x%8.8x]\n",
                                  currChunkPtr->memFirstAddr, currChunkPtr->memLastAddr,
                                  unitChunksPtr->chunksArray[i].memFirstAddr,
                                  unitChunksPtr->chunksArray[i].memLastAddr);
                /* Don't check last address, skip to next chunk */
                continue;
            }
        }

        /* Check chunk LAST address */
        if(currChunkPtr->memLastAddr >= unitChunksPtr->chunksArray[i].memFirstAddr &&
           currChunkPtr->memLastAddr <= unitChunksPtr->chunksArray[i].memLastAddr)
        {
            if(unitChunksPtr->chunksArray[i].formulaFuncPtr)
            {
                if(currChunkPtr->formulaFuncPtr)
                {
                    /* already checked all formula cells with memFirstAddr */
                    continue;
                }
                smemUnitChunkFormulaOverlapCheck(unitChunksPtr, i, currChunkPtr,
                                                 currChunkPtr->memLastAddr);
            }
            else
            {
                simWarningPrintf("smemUnitChunkOverlapCheck: "
                                  "overlapped chunk range:  = [0x%8.8x] [0x%8.8x]\n"
                                  "unit chunk range: [0x%8.8x] [0x%8.8x]\n",
                                  currChunkPtr->memFirstAddr, currChunkPtr->memLastAddr,
                                  unitChunksPtr->chunksArray[i].memFirstAddr,
                                  unitChunksPtr->chunksArray[i].memLastAddr);
            }
        }
    }
}




/*******************************************************************************
*  smemUnitChunkFormulaOverlapCheck_subFormula
*
* DESCRIPTION:
*      Checks (sub) formula of specific chunk in existing unit
*
* INPUTS:
*       unitChunksPtr   - (pointer to) the unit memories chunks
*       unitChunkOffset - offset of chunk in the unit
*       currChunkPtr    - (pointer to) current chunk to be checked
*       currAddr        - current address that belongs to currChunkPtr
*       formulaLevel    - index in currChunkPtr->formulaData[] to be checked on
*                         currAddr (from previous level)
*
* OUTPUTS:
*
*
* RETURNS:
*       GT_TRUE - chunk is overlapped and GT_FALSE - chunk is not overlapped
*
* COMMENTS:
*       Recursive function
*
*******************************************************************************/
static GT_BOOL smemUnitChunkFormulaOverlapCheck_subFormula
(
    IN SMEM_UNIT_CHUNKS_STC * unitChunksPtr,
    IN GT_U32 unitChunkOffset,
    IN SMEM_CHUNK_STC * currChunkPtr,
    IN GT_U32 currAddr ,
    IN GT_U32 formulaLevel
)
{
    GT_U32 formulaIndexArr[SMEM_FORMULA_CELL_NUM_CNS];  /* array of memory indexes */
    GT_BOOL     rc;
    SMEM_FORMULA_DIM_STC * formulaCellPtr;              /* pointer to formula cell array */
    GT_U32  ii;
    GT_U32  newAddr;
    GT_BOOL lastLevel;

    formulaCellPtr = &currChunkPtr->formulaData[formulaLevel];

    if(formulaCellPtr->size == 0 || formulaCellPtr->step == 0)
    {
        lastLevel = GT_TRUE;
    }
    else
    if((formulaLevel + 1) == SMEM_FORMULA_CELL_NUM_CNS)
    {
        lastLevel = GT_TRUE;
    }
    else
    {
        lastLevel = GT_FALSE;
    }
#if 0
    #define __PRINT_PARAM(param) simGeneralPrintf(" %s = [0x%8.8x] \n",#param,param)
#else
    #define __PRINT_PARAM(param)
#endif

    __PRINT_PARAM(lastLevel);

    if(lastLevel == GT_TRUE)
    {
        newAddr = currAddr;

        rc = smemFormulaChunkAddressCheck(newAddr,
                                          &unitChunksPtr->chunksArray[unitChunkOffset],
                                          formulaIndexArr);
        if(rc == GT_TRUE)
        {
            simWarningPrintf("smemUnitChunkFormulaOverlapCheck_subFormula: "
                             "overlapped chunk with formula start address = [0x%8.8x], current address = [0x%8.8x]\n"
                             "unit chunk range: [0x%8.8x] [0x%8.8x]\n",
                             currChunkPtr->memFirstAddr, newAddr,
                             unitChunksPtr->chunksArray[unitChunkOffset].memFirstAddr,
                             unitChunksPtr->chunksArray[unitChunkOffset].memLastAddr);
            /*return GT_TRUE;*/
        }

        __PRINT_PARAM(rc);
        return rc;
    }


    __PRINT_PARAM(formulaCellPtr->size);
    __PRINT_PARAM(formulaCellPtr->step);


    for(ii = 0; ii < formulaCellPtr->size; ii++)
    {
        newAddr = currAddr + (ii * formulaCellPtr->step);

        __PRINT_PARAM(ii);
        __PRINT_PARAM(newAddr);

        rc = smemUnitChunkFormulaOverlapCheck_subFormula(
                        unitChunksPtr,
                        unitChunkOffset,
                        currChunkPtr,
                        newAddr,
                        (formulaLevel + 1)/*do recursion to check deeper level */ );
        if(rc == GT_TRUE)
        {
            __PRINT_PARAM(rc);
            /* the address (or it's sub formula) from current chunk already
                documented as exists in the unit ... no need to continue with this chunk */
            return GT_TRUE;
        }
    }

    return GT_FALSE;

}

/*******************************************************************************
*  smemUnitChunkFormulaOverlapCheck
*
* DESCRIPTION:
*      Checks formula chunk's overlapping in unit's memory chunks
*
* INPUTS:
*       unitChunksPtr   - (pointer to) the unit memories chunks
*       unitChunkOffset - offset of chunk in the unit
*       currChunkPtr    - (pointer to) current chunk to be allocated
*       addressOffset   - chunk address offset
*
* OUTPUTS:
*
*
* RETURNS:
*       GT_TRUE - chunk is overlapped and GT_FALSE - chunk is not overlapped
*
* COMMENTS:
*       Expands chunk formula into addresses and checks for overlapping
*       into the unit chunk
*
*******************************************************************************/
static GT_BOOL smemUnitChunkFormulaOverlapCheck
(
    IN SMEM_UNIT_CHUNKS_STC * unitChunksPtr,
    IN GT_U32 unitChunkOffset,
    IN SMEM_CHUNK_STC * currChunkPtr,
    IN GT_U32 addressOffset
)
{
    GT_U32 formulaIndexArr[SMEM_FORMULA_CELL_NUM_CNS];  /* array of memory indexes */
    GT_BOOL rc = GT_FALSE;

    /* Current chunk without formula */
    if(currChunkPtr->formulaFuncPtr == 0)
    {
        rc = smemFormulaChunkAddressCheck(addressOffset,
                                          &unitChunksPtr->chunksArray[unitChunkOffset],
                                          formulaIndexArr);

        if(rc == GT_TRUE)
        {
            simWarningPrintf("smemUnitChunkFormulaOverlapCheck: "
                              "overlapped chunk with formula start address = [0x%8.8x], current address = [0x%8.8x]\n"
                              "unit chunk range: [0x%8.8x] [0x%8.8x]\n",
                              currChunkPtr->memFirstAddr, addressOffset,
                              unitChunksPtr->chunksArray[unitChunkOffset].memFirstAddr,
                              unitChunksPtr->chunksArray[unitChunkOffset].memLastAddr);
        }

        return rc;
    }


    rc = smemUnitChunkFormulaOverlapCheck_subFormula(
                    unitChunksPtr,
                    unitChunkOffset,
                    currChunkPtr,
                    addressOffset,
                    0/*start the recursion*/);

    return rc;
}

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
)
{
    GT_U32  unitBaseAddressOffset =  23/*SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS*/;
    GT_U32  ii;
    SMEM_CHUNK_STC  *chunkPtr;
    GT_U32  unitId;
    GT_U32  baseAddr = (unitInfoPtr->chunkIndex << unitBaseAddressOffset);

    for(ii = 0 ; ii < unitInfoPtr->numOfChunks ; ii ++)
    {
        chunkPtr = &unitInfoPtr->chunksArray[ii];

        if(isZeroBased == GT_FALSE)
        {
            unitId = SMEM_U32_GET_FIELD(chunkPtr->memFirstAddr,unitBaseAddressOffset,9);

            if( unitId != 0 && unitId != unitInfoPtr->chunkIndex)
            {
                if(unitInfoPtr->chunkType == SMEM_UNIT_CHUNK_TYPE_8_MSB_E &&
                  (((unitId>>1) == 0) || ((unitId>>1) == (unitInfoPtr->chunkIndex >> 1))
                   ))
                {
                    /* all is ok in unit of 8 MSBits */
                }
                else
                {
                    simGeneralPrintf(" segment in chunk ,address : [0x%8.8x] not belong to unit [0x%8.8x] \n",chunkPtr->memFirstAddr,
                        (unitInfoPtr->chunkIndex << unitBaseAddressOffset));
                }
            }

            unitId = SMEM_U32_GET_FIELD(chunkPtr->memLastAddr,unitBaseAddressOffset,9);

            if( unitId != 0 && unitId != unitInfoPtr->chunkIndex)
            {
                if(unitInfoPtr->chunkType == SMEM_UNIT_CHUNK_TYPE_8_MSB_E &&
                  (((unitId>>1) == 0) || ((unitId>>1) == (unitInfoPtr->chunkIndex >> 1))
                   ))
                {
                    /* all is ok in unit of 8 MSBits */
                }
                else
                {
                    simGeneralPrintf(" segment in chunk ,last address : [0x%8.8x] not belong to unit [0x%8.8x] \n",chunkPtr->memLastAddr,
                        (unitInfoPtr->chunkIndex << unitBaseAddressOffset));
                }
            }

            /* update the addresses */
            if(unitInfoPtr->chunkType == SMEM_UNIT_CHUNK_TYPE_8_MSB_E)
            {
                SMEM_U32_SET_FIELD(chunkPtr->memFirstAddr,unitBaseAddressOffset+1,8,unitInfoPtr->chunkIndex>>1);
                SMEM_U32_SET_FIELD(chunkPtr->memLastAddr ,unitBaseAddressOffset+1,8,unitInfoPtr->chunkIndex>>1);
            }
            else
            {
                SMEM_U32_SET_FIELD(chunkPtr->memFirstAddr,unitBaseAddressOffset,9,unitInfoPtr->chunkIndex);
                SMEM_U32_SET_FIELD(chunkPtr->memLastAddr,unitBaseAddressOffset,9,unitInfoPtr->chunkIndex);
            }
        }
        else
        {
            /* use += to support 'huge' units with addresses from several 'sub units' */

            chunkPtr->memFirstAddr += baseAddr;
            chunkPtr->memLastAddr  += baseAddr;
        }
    }


    return;
}

static GT_BOOL addrAlignForceReset = GT_FALSE;
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
)
{
    addrAlignForceReset = forceReset;
}


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
)
{
    smemGenericUnitAddressesAlignToBaseAddress1(devObjPtr,unitInfoPtr,GT_FALSE);
}


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
)
{
    GT_U32  unitBaseAddressOffset =  23/*SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS*/;
    GT_U32  ii;
    GT_U32  baseAddr = (unitInfoPtr->chunkIndex << unitBaseAddressOffset);

    for(ii = 0 ; ii < numOfRegisters ; ii ++)
    {
        if(registersArr[ii] == SMAIN_NOT_VALID_CNS)
        {
            /* not used register */
            continue;
        }

        if(isZeroBased == GT_FALSE)
        {
            if(addrAlignForceReset == GT_FALSE)
            {
                if( SMEM_U32_GET_FIELD(registersArr[ii],unitBaseAddressOffset,9) != 0 &&
                    SMEM_U32_GET_FIELD(registersArr[ii],unitBaseAddressOffset,9) != unitInfoPtr->chunkIndex)
                {
                    simGeneralPrintf(" register, address : [0x%8.8x] not belong to unit [0x%8.8x] \n",registersArr[ii],
                        (unitInfoPtr->chunkIndex << unitBaseAddressOffset));
                }
            }
            /* update the address */
            SMEM_U32_SET_FIELD(registersArr[ii],unitBaseAddressOffset,9,unitInfoPtr->chunkIndex);
        }
        else
        {
            /* use += to support 'huge' units with addresses from several 'sub units' */

            registersArr[ii] += baseAddr;
        }
    }


    return;
}

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
)
{
    smemGenericRegistersArrayAlignToUnit1(registersArr,numOfRegisters,unitInfoPtr,GT_FALSE);
}

/*******************************************************************************
*   smemGenericFindMem
*
* DESCRIPTION:
*       Return pointer to the register's or tables's memory.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
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
)
{
    void                    * memPtr = NULL;
    SMEM_CHT_GENERIC_DEV_MEM_INFO  * devMemInfoPtr;
    GT_U32                    index,ii;
    GT_UINTPTR                param;
    SMEM_ACTIVE_MEM_ENTRY_STC   *unitActiveMemPtr = NULL;/*active memory dedicated to the unit*/
    SMEM_ACTIVE_MEM_ENTRY_STC   *activeMemPtr;/*active memory of the device/unit*/
    GT_BIT                    forceIsPex;

    if (devObjPtr == 0)
    {
        skernelFatalError("smemGenericFindMem: illegal pointer \n");
    }

    if (((address >> SMEM_PHY_UNIT_INDEX_CNS) & 0x3) == 0x3)
    {
        memPtr  = smemChtPhyReg(devObjPtr, address, memSize);

        /* find active memory entry */
        if (activeMemPtrPtr != NULL)
        {
            *activeMemPtrPtr = NULL;
        }

        return memPtr;
    }

    devMemInfoPtr = devObjPtr->deviceMemory;

    if(devMemInfoPtr->common.accessPexMemorySpaceOnlyOnExplicitAction == 0 &&
        devMemInfoPtr->common.pciUnitBaseAddr &&
       (address & 0xFFFF0000) == devMemInfoPtr->common.pciUnitBaseAddr)
    {
        forceIsPex = 1;
    }
    else
    {
        forceIsPex = 0;
    }


    /* Find PCI registers memory */
    if (SMEM_ACCESS_PCI_FULL_MAC(accessType) || forceIsPex)
    {
        if((address & 0xFFFF0000) == devMemInfoPtr->common.pciUnitBaseAddr)
        {
            /* support direct access in offset 0x000F0000 */
        }
        else if ((address & 0xFFFF0000) == 0x00000000)
        {
            /* support direct access in offset 0x00000000 */
            address += devMemInfoPtr->common.pciUnitBaseAddr;
        }
        else
        {
            skernelFatalError("smemGenericFindMem: unknown PEX memory [0x%8.8x] \n" ,address );
        }
        /* continue and find the memory */

        if(devMemInfoPtr->common.isPartOfGeneric && devMemInfoPtr->PEX_UnitMem.numOfChunks)
        {
            /* the device hold special unit for the PEX registers ..
               not need to look at devMemInfoPtr->common.specFunTbl functions */

            param   = (GT_UINTPTR)(void*)&devMemInfoPtr->PEX_UnitMem;
            memPtr  = smemDevFindInUnitChunk(devObjPtr, accessType, address, memSize, param);
            if(memPtr == NULL)
            {
                skernelFatalError("smemGenericFindMem: not found PEX memory [0x%8.8x] \n" ,address );
                return NULL;
            }
            /* check if there is dedicated active memory for the unit */
            unitActiveMemPtr =
                devMemInfoPtr->PEX_UnitMem.unitActiveMemPtr;
        }
    }

    if(memPtr)/* already got memory */
    {
        /* nothing to do as we got memory already
           (and maybe also active memory of unit) */
    }
    else
    {
        if(SMEM_CHT_IS_SIP5_GET(devObjPtr))
        {
            /* support 8 * 64 = 512 sub units !!! */
            /* not only 64 sub units as in previous device */
            index = address >> SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS;
        }
        else
        {
            index = (address & REG_SPEC_FUNC_INDEX) >>
                         SMEM_CHT_UNIT_INDEX_FIRST_BIT_CNS;
        }

        if(index >= SMEM_CHT_NUM_UNITS_MAX_CNS)
        {
            skernelFatalError("unitIndex[%d] >= max of [%d] \n" ,
                index,SMEM_CHT_NUM_UNITS_MAX_CNS);
        }


        param   = devMemInfoPtr->common.specFunTbl[index].specParam;
        memPtr  = devMemInfoPtr->common.specFunTbl[index].specFun(
                            devObjPtr, accessType, address, memSize, param);
        /*  pointer to active memory of the unit.
            when != NULL , using ONLY this active memory.
            when NULL , using the active memory of the device.
        */
        unitActiveMemPtr =
            devMemInfoPtr->common.specFunTbl[index].unitActiveMemPtr;
    }

    /* find active memory entry */
    if (activeMemPtrPtr != NULL)
    {
        *activeMemPtrPtr = NULL;

        activeMemPtr = unitActiveMemPtr ?
                       unitActiveMemPtr :
                       devObjPtr->activeMemPtr;

        for (ii = 0; activeMemPtr[ii].address != END_OF_TABLE; ii++)
        {
            /* check address    */
            if ((address & activeMemPtr[ii].mask)
                 == activeMemPtr[ii].address)
            {
                *activeMemPtrPtr = &activeMemPtr[ii];
                break;
            }
        }
    }

    return memPtr;
}


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
)
{
    if(devObjPtr->devIsOwnerMemFunPtr)
    {
        return devObjPtr->devIsOwnerMemFunPtr(devObjPtr,address);
    }

    /* the device own the address (BWC) */
    return GT_TRUE;
}

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
)
{
    GT_U32 * findMemPtr;
    GT_U32  regValue, fieldMask;
    SCIB_MEMORY_ACCESS_TYPE accessType = SKERNEL_MEMORY_READ_DFX_E;

    smemConvertDevAndAddrToNewDevAndAddr(deviceObjPtr,address,accessType,
        &deviceObjPtr,&address);

    paramCheck(fldFirstBit,fldSize);

    /* check device type and call device search memory function*/
    findMemPtr = smemFindMemory(deviceObjPtr, address, 1, NULL,
                                accessType);
    regValue = findMemPtr[0];

    /* Align data right according to the first field bit */
    regValue >>= fldFirstBit;

    fldSize = fldSize % 32;
    /* get field's bits mask */
    if (fldSize)
    {
        fieldMask = ~(0xffffffff << fldSize);
    }
    else
    {
        fieldMask = (0xffffffff);
    }

    *data = regValue & fieldMask;

    /* log memory information */
    simLogMemory(deviceObjPtr, SIM_LOG_MEMORY_DFX_READ_E, SIM_LOG_MEMORY_DEV_E,
                                            address, *data, 0);
}

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
)
{
    GT_U32 * findMemPtr;
    GT_U32 regValue, fieldMask;
    SCIB_MEMORY_ACCESS_TYPE accessType = SKERNEL_MEMORY_WRITE_DFX_E;

    smemConvertDevAndAddrToNewDevAndAddr(deviceObjPtr,address,accessType,
        &deviceObjPtr,&address);

    paramCheck(fldFirstBit,fldSize);

    fldSize = fldSize % 32;
    /* get field's bits mask */
    if (fldSize)
    {
        fieldMask = ~(0xffffffff << fldSize);
    }
    else
    {
        fieldMask = 0xffffffff;
    }

    /* Get register value */
    findMemPtr = smemFindMemory(deviceObjPtr, address, 1, NULL,
                                accessType);
    regValue = findMemPtr[0];
    /* Set field value in the register value using
    inverted real field mask in the place of the field */
    regValue &= ~(fieldMask << fldFirstBit);
    regValue |= (data << fldFirstBit);

    /* log memory information */
    simLogMemory(deviceObjPtr, SIM_LOG_MEMORY_DFX_WRITE_E, SIM_LOG_MEMORY_DEV_E,
                                            address, regValue, *findMemPtr);
    *findMemPtr = regValue;
}

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
)
{
    smemDfxRegFldSet(deviceObjPtr, address, 0, 32, data);
}

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
)
{
    smemDfxRegFldGet(deviceObjPtr, address, 0, 32, dataPtr);
}

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
)
{
    SKERNEL_DEVICE_MEMORY_ALLOCATION_STC *currentPtr;
    SKERNEL_DEVICE_MEMORY_ALLOCATION_STC *nextPtr;
    GT_BIT          isFirst = 1;

    if(devObjPtr == NULL)
    {
        skernelFatalError("smemDeviceObjFreeAllMemoryAllocations : not valid for NULL device \n");
    }

    SCIB_SEM_TAKE;

    currentPtr = &devObjPtr->myDeviceAllocations;
    /* skip first element as this is the 'device itself' */
    currentPtr = currentPtr->nextElementPtr;

    while(currentPtr)
    {
        nextPtr = currentPtr->nextElementPtr;
        /* free the allocation needed by the device */
        if(currentPtr->myMemoryPtr)/* free only if not NULL , because maybe already released */
        {
            MALLOC_FILE_NAME_DEBUG_PRINT(("free ptr[%x] from smemDeviceObjMemoryAllAllocationsFree, that was allocated from file[%s] line[%d] size[%d]bytes \n",
                    currentPtr->myMemoryPtr,
                    currentPtr->fileNamePtr,
                    currentPtr->line,
                    currentPtr->myMemoryNumOfBytes
                    ));

            free(currentPtr->myMemoryPtr);
        }

        if(isFirst == 0)/* first element was not dynamic allocated */
        {
            /* free the element itself */
            free(currentPtr);
        }

        isFirst = 0;

        /* update the 'current' to be 'next' */
        currentPtr = nextPtr;
    }

    if(freeMyObj == GT_TRUE)
    {
        /* free the device itself !!!! */
        free(devObjPtr);
    }
    else
    {
        /* must reset next info */
        devObjPtr->myDeviceAllocations.myMemoryPtr = NULL;
        devObjPtr->myDeviceAllocations.myMemoryNumOfBytes = 0;
        devObjPtr->myDeviceAllocations.nextElementPtr = NULL;
        devObjPtr->lastDeviceAllocPtr = NULL;
    }

    SCIB_SEM_SIGNAL;
}

/* compress file name from:
   x:\cpss\sw\prestera\simulation\simdevices\src\asicsimulation\skernel\suserframes\snetcheetahingress.c
   To:
   snetcheetahingress.c */
static GT_CHAR * compressFileName
(
    IN GT_CHAR const *fileName
)
{
    GT_CHAR *tempName;
    GT_CHAR *compressedFileName = (GT_CHAR *)fileName;
    while(NULL != (tempName = strchr(compressedFileName,'\\')))
    {
        compressedFileName = tempName+1;
    }

    return compressedFileName;
}

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
    IN const char*     fileNamePtr,
    IN GT_U32          line
)
{
    GT_PTR *allocPtr;
    SKERNEL_DEVICE_MEMORY_ALLOCATION_STC    *nextElementPtr;

    if(size > MAX_ALLOC_CNS || nitems > MAX_ALLOC_CNS || (size*nitems) > MAX_ALLOC_CNS)
    {
        /* fix SV.TAINTED.ALLOC_SIZE warning from klockwork that nitems or size comes from 'sscanf'
           and we need to protect the allocation */
        skernelFatalError("smemDeviceObjMemoryAlloc__internal : suspected 'Tainted data' for calloc %d bytes \n" ,
            (nitems*size));
        return NULL;
    }

    allocPtr = calloc(nitems,size);/*allocate memory needed by the caller*/

    if(allocPtr == NULL)
    {
        skernelFatalError("smemDeviceObjMemoryAlloc__internal : calloc of %d bytes failed \n" ,
            (nitems*size));
        return NULL;
    }

    SCIB_SEM_TAKE;

    if(devObjPtr)
    {
        devObjPtr->totalNumberOfBytesAllocated += (nitems*size) + /* allocation done for the caller */
                    sizeof(SKERNEL_DEVICE_MEMORY_ALLOCATION_STC); /* allocation done for the 'nextElementPtr' */

        if(devObjPtr->lastDeviceAllocPtr == NULL)
        {
            /* first allocation register the 'device itself' as first element */
            devObjPtr->lastDeviceAllocPtr = &devObjPtr->myDeviceAllocations;

            devObjPtr->myDeviceAllocations.myMemoryPtr = devObjPtr;
            devObjPtr->myDeviceAllocations.myMemoryNumOfBytes = sizeof(SKERNEL_DEVICE_OBJECT);
        }

        /* new allocation so need new 'element' */
        nextElementPtr = malloc(sizeof(*nextElementPtr));
        if(nextElementPtr == NULL)
        {
            skernelFatalError("smemDeviceObjMemoryAlloc__internal : malloc of %d bytes failed \n" ,
                sizeof(*nextElementPtr));

            SCIB_SEM_SIGNAL;

            return NULL;
        }
        /* bind the previous element to the new element */
        devObjPtr->lastDeviceAllocPtr->nextElementPtr = nextElementPtr;

        nextElementPtr->myMemoryPtr    = allocPtr;
        nextElementPtr->myMemoryNumOfBytes = (GT_U32)(nitems*size);
        nextElementPtr->nextElementPtr = NULL;
        nextElementPtr->fileNamePtr = compressFileName(fileNamePtr);
        nextElementPtr->line = line;

        MALLOC_FILE_NAME_DEBUG_PRINT(("alloc ptr[%x] from file[%s] line[%d] size[%d]bytes \n",
                allocPtr,
                nextElementPtr->fileNamePtr,
                nextElementPtr->line,
                nextElementPtr->myMemoryNumOfBytes
                ));

        /* update last element pointer to the newly allocated element*/
        devObjPtr->lastDeviceAllocPtr = nextElementPtr;
    }
    else
    {
        MALLOC_FILE_NAME_DEBUG_PRINT(("non-dev:alloc ptr[%x] from file[%s] line[%d] size[%d]bytes \n",
                allocPtr,
                compressFileName(fileNamePtr),
                line,
                nitems*size
                ));
    }

    SCIB_SEM_SIGNAL;

    return allocPtr;
}

/*******************************************************************************
*   smemDeviceObjMemoryFind
*
* DESCRIPTION:
*       return pointer to 'STC memory allocation' memory that hold 'searchForPtr'
*
* INPUTS:
*       devObjPtr - pointer to device object
*       searchForPtr - the pointer of memory that was returned by
*           smemDeviceObjMemoryAlloc__internal or smemDeviceObjMemoryRealloc__internal
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       pointer to 'STC memory allocation' memory that hold 'searchForPtr'
* COMMENTS:
*       assuming that function is under 'SCIB lock'
*
*******************************************************************************/
static SKERNEL_DEVICE_MEMORY_ALLOCATION_STC* smemDeviceObjMemoryFind
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_PTR                   searchForPtr
)
{
    SKERNEL_DEVICE_MEMORY_ALLOCATION_STC *currentPtr;

    if(devObjPtr == NULL)
    {
        skernelFatalError("smemDeviceObjMemoryFind : not valid for NULL device \n");
        return NULL;
    }

    currentPtr = &devObjPtr->myDeviceAllocations;

    do
    {
        if(currentPtr->myMemoryPtr == searchForPtr)
        {
            /* got match */
            return currentPtr;
        }

        /* update the 'current' to be 'next' */
        currentPtr = currentPtr->nextElementPtr;
    }while(currentPtr);

    skernelFatalError("smemDeviceObjMemoryFind : memory not found <-- simulation management error \n");
    return NULL;
}

/*******************************************************************************
*   smemDeviceObjMemoryRealloc__internal
*
* DESCRIPTION:
*       return new memory allocation after 'realloc' old memory to new size.
*       the oldPointer should be pointer that smemDeviceObjMemoryAlloc__internal(..) returned.
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
)
{
    SKERNEL_DEVICE_MEMORY_ALLOCATION_STC    *elementPtr;
    GT_PTR *reallocPtr;/*reallocate memory needed by the caller*/

    if(size > MAX_ALLOC_CNS)
    {
        /* fix SV.TAINTED.ALLOC_SIZE warning from klockwork that size comes from 'sscanf'
           and we need to protect the allocation */
        skernelFatalError("smemDeviceObjMemoryRealloc__internal : suspected 'Tainted data' for realloc %d bytes \n" ,
            size);
        return NULL;
    }

    if(devObjPtr == NULL)
    {
        /* bypass 'device' management */
        reallocPtr = realloc(oldPointer,size);

        MALLOC_FILE_NAME_DEBUG_PRINT(("non-dev:realloc new ptr[%x] from file[%s] line[%d] size[%d],that was ptr[%x] (unknown src file/size) \n",
                reallocPtr,
                compressFileName(fileNamePtr),
                line,
                size,
                oldPointer
                ));

        return reallocPtr;
    }

    if(oldPointer == NULL)
    {
        return smemDeviceObjMemoryAlloc__internal(devObjPtr,size,1,fileNamePtr,line);
    }


    SCIB_SEM_TAKE;

    elementPtr = smemDeviceObjMemoryFind(devObjPtr,oldPointer);
    if(elementPtr == NULL)
    {
        SCIB_SEM_SIGNAL;
        /* fatal error already asserted */
        return NULL;
    }

    /* NOTE that : elementPtr->myMemoryPtr == oldPointer !!! */

    reallocPtr = realloc(oldPointer,size);
    if(reallocPtr == NULL)
    {
        skernelFatalError("smemDeviceObjMemoryRealloc__internal : realloc of %d bytes failed \n" ,
            size);

        SCIB_SEM_SIGNAL;

        return NULL;
    }

    MALLOC_FILE_NAME_DEBUG_PRINT(("realloc new ptr[%x] from file[%s] line[%d] size[%d], that was ptr[%x] allocated from file[%s] line[%d] size[%d]bytes \n",
            reallocPtr,
            compressFileName(fileNamePtr),
            line,
            size,
            elementPtr->myMemoryPtr,
            elementPtr->fileNamePtr,
            elementPtr->line,
            elementPtr->myMemoryNumOfBytes
            ));

    elementPtr->myMemoryPtr = reallocPtr;
    elementPtr->myMemoryNumOfBytes = (GT_U32)size;
    elementPtr->fileNamePtr = compressFileName(fileNamePtr);
    elementPtr->line = line;

    SCIB_SEM_SIGNAL;

    return reallocPtr;

}

/*******************************************************************************
*   smemDeviceObjMemoryPtrFree__internal
*
* DESCRIPTION:
*       free memory that was allocated by smemDeviceObjMemoryAlloc__internal or
*           smemDeviceObjMemoryRealloc__internal
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

)
{
    SKERNEL_DEVICE_MEMORY_ALLOCATION_STC    *elementPtr;

    if(oldPointer == NULL)
    {
        /* it seems that there are places that will get us here !!! */
        return ;
    }

    if(devObjPtr == NULL)
    {
        MALLOC_FILE_NAME_DEBUG_PRINT(("non-dev : free ptr[%x] from file[%s] line[%d](unknown src file/size) \n",
                oldPointer,
                compressFileName(fileNamePtr),
                line
                ));

        /* bypass 'device' management */
        free(oldPointer);
        return;
    }

    SCIB_SEM_TAKE;

    elementPtr = smemDeviceObjMemoryFind(devObjPtr,oldPointer);
    if(elementPtr == NULL)
    {
        SCIB_SEM_SIGNAL;
        /* fatal error already asserted */
        return;
    }

    MALLOC_FILE_NAME_DEBUG_PRINT(("free ptr[%x] from file[%s] line[%d], that was allocated from file[%s] line[%d] size[%d]bytes \n",
            oldPointer,
            compressFileName(fileNamePtr),
            line,
            elementPtr->fileNamePtr,
            elementPtr->line,
            elementPtr->myMemoryNumOfBytes
            ));

    /* NOTE that : elementPtr->myMemoryPtr == oldPointer !!! */
    free(oldPointer);

    elementPtr->myMemoryPtr = NULL;
    elementPtr->myMemoryNumOfBytes = 0;
    elementPtr->fileNamePtr = NULL;
    elementPtr->line = 0;

    SCIB_SEM_SIGNAL;

    /* keep the element and not change the 'link list'*/
    return;
}

/*******************************************************************************
*   smemDeviceObjMemoryPtrMemSetZero
*
* DESCRIPTION:
*       'memset 0' to memory that was allocated by smemDeviceObjMemoryAlloc__internal or
*           smemDeviceObjMemoryRealloc__internal
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
)
{
    SKERNEL_DEVICE_MEMORY_ALLOCATION_STC    *elementPtr;

    if(memPointer == NULL)
    {
        skernelFatalError("smemDeviceObjMemoryPtrMemSetZero : try to memset NULL pointer \n");
        return ;
    }

    if(devObjPtr == NULL)
    {
        skernelFatalError("smemDeviceObjMemoryPtrMemSetZero : can not manage memory that is not attached to device \n");
        return ;
    }

    SCIB_SEM_TAKE;

    elementPtr = smemDeviceObjMemoryFind(devObjPtr,memPointer);
    if(elementPtr == NULL)
    {
        /* fatal error already asserted */
        SCIB_SEM_SIGNAL;
        return;
    }

    /* NOTE that : elementPtr->myMemoryPtr == memPointer !!! */

    /* memset to 0 the memory according to it's length */
    memset(elementPtr->myMemoryPtr,0,elementPtr->myMemoryNumOfBytes);

    SCIB_SEM_SIGNAL;

    return;
}

/*******************************************************************************
*   smemDeviceObjMemoryAllAllocationsSum
*
* DESCRIPTION:
*       debug function to print the sum of all allocations done for a device
*       allocations that done by smemDeviceObjMemoryAlloc__internal or
*           smemDeviceObjMemoryRealloc__internal
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
)
{
    SKERNEL_DEVICE_MEMORY_ALLOCATION_STC *currentPtr;
    GT_U32      totalNumBytes = 0;
    GT_U32      totalNumElements = 0;
    GT_U32      overheadNumBytes = sizeof(SKERNEL_DEVICE_MEMORY_ALLOCATION_STC);

    if(devObjPtr == NULL)
    {
        /* already did fatal error */
        return ;
    }

    SCIB_SEM_TAKE;

    currentPtr = &devObjPtr->myDeviceAllocations;

    do
    {
        totalNumBytes += currentPtr->myMemoryNumOfBytes;
        totalNumElements ++;

        /* update the 'current' to be 'next' */
        currentPtr = currentPtr->nextElementPtr;
    }while(currentPtr);

    SCIB_SEM_SIGNAL;

    /* total overhead for management is the STC of SKERNEL_DEVICE_MEMORY_ALLOCATION_STC */
    overheadNumBytes *= (totalNumElements - 1);/* minus 1 because first element is not dynamic allocated */

    simGeneralPrintf("device[%d] did dynamic allocated of [%d] bytes  (for usage), in [%d] elements , additional management \n",
        devObjPtr->deviceId,totalNumBytes,totalNumElements);
    simGeneralPrintf("and additional dynamic allocated [%d] bytes for the 'allocations' management  \n" ,
        overheadNumBytes);
    return;

}

/*******************************************************************************
*   smemConvertDevAndAddrToNewDevAndAddr
*
* DESCRIPTION:
*       Convert {dev,address} to new {dev,address}.
*       needed for multi-pipe device.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       address     - address of memory(register or table).
*       accessType  - the access type
* OUTPUTS:
*       newDevObjPtrPtr   - (pointer to) pointer to new device object.
*       newAddressPtr     - (pointer to) address of memory (register or table).
*
* RETURNS:
*        None
*
* COMMENTS:
*       function MUST be called before calling smemFindMemory()
*
*******************************************************************************/
static void smemConvertDevAndAddrToNewDevAndAddr
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32                  address,
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    OUT SKERNEL_DEVICE_OBJECT * *newDevObjPtrPtr,
    OUT GT_U32                  *newAddressPtr
)
{
    SMEM_CHT_DEV_COMMON_MEM_INFO  * commonDevMemInfoPtr;

    if(devObjPtr == NULL)
    {
        skernelFatalError("smemConvertDevAndAddrToNewDevAndAddr : NULL device \n");
    }

    commonDevMemInfoPtr = devObjPtr->deviceMemory;

    if(NULL == commonDevMemInfoPtr->smemConvertDevAndAddrToNewDevAndAddrFunc)
    {
        /* no conversion needed */
        *newDevObjPtrPtr = devObjPtr;
        *newAddressPtr = address;
        return;
    }

    /* call the specific function of the device to convert device,address */
    commonDevMemInfoPtr->smemConvertDevAndAddrToNewDevAndAddrFunc(
        devObjPtr,address,accessType,newDevObjPtrPtr,newAddressPtr);

    if((*newDevObjPtrPtr) == NULL)
    {
        skernelFatalError("smemConvertDevAndAddrToNewDevAndAddr : NULL 'converted' device \n");
    }

    return;
}

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
)
{
    DECLARE_FUNC_NAME(smemConvertGlobalPortToCurrentPipeId);

    SIM_OS_TASK_COOKIE_INFO_STC  *myTaskInfoPtr;
    GT_U32  currentPipeId;
    GT_U32  localPort;

    if(devObjPtr->numOfPipes == 0)
    {
        skernelFatalError("smemConvertGlobalPortToCurrentPipeId : only for multi-pipe \n");
    }

    if(globalPortNum == devObjPtr->dmaNumOfCpuPort)
    {         /* keep DMA of CPU port */
        localPort        = devObjPtr->dmaNumOfCpuPort;
        currentPipeId    = 0;
    }
    else
    {
        /* update the src port (relative to the ports of the pipe) */
        localPort = globalPortNum % devObjPtr->numOfPortsPerPipe;
        currentPipeId    = globalPortNum / devObjPtr->numOfPortsPerPipe;
    }


    __LOG(("Converted global port [%d] to pipe[%d] port[%d] \n",
        globalPortNum , currentPipeId , localPort));

    if(currentPipeId >= devObjPtr->numOfPipes)
    {
        skernelFatalError("smemConvertGlobalPortToCurrentPipeId : currentPipeId[%d] >= numOfPipes[%d] \n",
            currentPipeId,devObjPtr->numOfPipes);
    }

    *localPortNumberPtr = localPort;

    myTaskInfoPtr = simOsTaskOwnTaskCookieGet();

    if(myTaskInfoPtr == NULL)
    {
        /* the 'startSimulationLog' may get here when called from 'aplication' .
          when it is doing :
            simLogSlanCatchUp();
            simLogLinkStateCatchUp();
                and calling for all ports : snetLinkStateNotify()
        */
        /*skernelFatalError("smemConvertGlobalPortToCurrentPipeId : not registered task ?! \n");*/
        return;
    }

    myTaskInfoPtr->currentPipeId = currentPipeId;

    return;
}


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
)
{
    DECLARE_FUNC_NAME(smemConvertCurrentPipeIdAndLocalPortToGlobal);

    SIM_OS_TASK_COOKIE_INFO_STC  *myTaskInfoPtr;
    GT_U32  currentPipeId;
    GT_U32  globalPort;

    if(devObjPtr->numOfPipes == 0)
    {
        skernelFatalError("smemConvertCurrentPipeIdAndLocalPortToGlobal : only for multi-pipe \n");
    }

    myTaskInfoPtr = simOsTaskOwnTaskCookieGet();

    if(myTaskInfoPtr == NULL)
    {
        skernelFatalError("smemConvertCurrentPipeIdAndLocalPortToGlobal : not registered task ?! \n");
    }

    currentPipeId = myTaskInfoPtr->currentPipeId;

    if(localPortNum == devObjPtr->dmaNumOfCpuPort)
    {         /* keep DMA of CPU port */
        globalPort = devObjPtr->dmaNumOfCpuPort;
    }
    else
    {
        /* update the port (relative to the ports of the pipe) */
        globalPort = localPortNum + (devObjPtr->numOfPortsPerPipe) * currentPipeId;
    }

    __LOG(("Converted port [%d] of pipe[%d] to global port[%d] \n",
        localPortNum , currentPipeId , globalPort));

    if(currentPipeId >= devObjPtr->numOfPipes)
    {
        skernelFatalError("smemConvertCurrentPipeIdAndLocalPortToGlobal : pipe device not found ?! \n");
    }

    *globalPortNumPtr = globalPort;

    return;
}

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
)
{
    SIM_OS_TASK_COOKIE_INFO_STC  *myTaskInfoPtr;

    if(currentPipeId >= devObjPtr->numOfPipes)
    {
        skernelFatalError("smemSetCurrentPipeId : currentPipeId[%d] >= numOfPipes[%d] \n",
            currentPipeId,devObjPtr->numOfPipes);
    }

    myTaskInfoPtr = simOsTaskOwnTaskCookieGet();
    if(myTaskInfoPtr == NULL)
    {
        skernelFatalError("smemSetCurrentPipeId : not registered task ?! \n");
    }

    myTaskInfoPtr->currentPipeId = currentPipeId;

    return;
}
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
)
{
    SIM_OS_TASK_COOKIE_INFO_STC  *myTaskInfoPtr;

    myTaskInfoPtr = simOsTaskOwnTaskCookieGet();
    if(myTaskInfoPtr == NULL)
    {
        skernelFatalError("smemGetCurrentPipeId : not registered task ?! \n");
    }

    return myTaskInfoPtr->currentPipeId;
}

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
)
{
    DECLARE_FUNC_NAME(smemConvertPipeIdAndLocalPortToGlobal_withPipeAndDpIndex);

    GT_U32  globalPort;
    DATA_PATH_INFO_STC  *dpInfoPtr;

    if(pipeIndex >= devObjPtr->numOfPipes || dpIndex >= MAX_DP_CNS)
    {
        skernelFatalError("smemConvertPipeIdAndLocalPortToGlobal_withPipeAndDpIndex : only for multi-pipe \n");
    }

    dpInfoPtr = &devObjPtr->multiDataPath.info[dpIndex];

    if(localPortNum == devObjPtr->multiDataPath.info[dpIndex].cpuPortDmaNum &&
        dpIndex == 0)
    {
        /* mapped to CPU port */
        globalPort = devObjPtr->dmaNumOfCpuPort;
    }
    else
    {
        if(localPortNum >= dpInfoPtr->dataPathNumOfPorts)
        {
            skernelFatalError("smemConvertPipeIdAndLocalPortToGlobal_withPipeAndDpIndex : local port can't belong to DP \n");
        }

        globalPort = pipeIndex * devObjPtr->numOfPortsPerPipe + dpInfoPtr->dataPathFirstPort + localPortNum;
    }

    __LOG(("Converted local port [%d] of DP[%d] of pipe[%d] to global port[%d]  \n",
        localPortNum , dpIndex , pipeIndex , globalPort));

    *globalPortNumPtr = globalPort;

    return;
}


