/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemComModule.c
*
* DESCRIPTION:
*       This is API implementation for Communication Module memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 12$
*
*******************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SCIB/scib.h>
#include <asicSimulation/SInit/sinit.h>
#include <asicSimulation/SKernel/smem/smemComModule.h>
#include <comModule/CSClient_C.h>

#define REGISTER_ADDR_SPACE_CNS                 0
#define PEX_ADDR_SPACE_CNS                      1


GT_U32 defaultAddress;
GT_U32 internalBaseArray[256];

static GT_VOID smemComModuleReadWriteMemory
(
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_VOID * devObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    INOUT GT_U32 * memPtr
);

static GT_VOID smemComModuleSmiRead
(
    GT_U32  cmAdapterId,
    GT_U32  cmInterfaceId,
    GT_U32  regAddr,
    GT_U32* dataPtr
);

#define SIM_ON_READ_SET_BITS31(data)    (data[0] |= 0x80)
#define SIM_ON_WRITE_RESET_BIT31(data)  (data[0] &= 0x7F)
#define SIM_RESET_BIT_30(data)          (data[0] &= 0xBF)
#define SIM_ON_READ_SET_BIT_30(data)    (data[0] |= 0x40)

static GT_STATUS simLongToChar(IN GT_U32 src, OUT GT_U8 dst[4]);

static GT_STATUS simCharToLong(IN GT_U8 * srcPtr, OUT GT_U32 *dst);

static GT_STATUS simConcatCharArray(IN GT_U8 src0[4], IN GT_U8 src1[4],
                                    OUT GT_U8 dst[8]);

#if 0
/*******************************************************************************
*   smemComModuleGetSmiId
*
* DESCRIPTION:
*       SMI hardware Id lookup.
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
*       Function scans SMI bus and find first valid SMI hardware Id connected to the bus.
*
*******************************************************************************/
static GT_STATUS smemComModuleGetSmiId
(
IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SNET_BUS_INTER_TYPE_ENT comInterface;
    GT_U32 regData;
    GT_U32 cmAdapterId;
    GT_U32 cmInterfaceId;
    GT_U32 smiId;

    cmAdapterId = SMEM_CM_INST_ADAPTER_ID_GET_MAC(devObjPtr);
    comInterface = SMEM_CM_INST_INTERFACE_TYPE_GET_MAC(devObjPtr);
    cmInterfaceId = SMEM_CM_INST_INTERFACE_ID_GET_MAC(devObjPtr, comInterface);

    if(comInterface == SNET_BUS_SMI_E)
    {
        for(smiId = 0; smiId < 32; smiId++)
        {
            smemComModuleSmiRead(cmAdapterId, smiId, 0x50, &regData);
            if(regData == 0x11ab)
            {
                /* Set SMI HW Id  */
                SMEM_CM_INST_INTERFACE_ID_SET_MAC(devObjPtr, SNET_BUS_SMI_E, smiId);
                return GT_OK;
            }
        }
    }

    return GT_FAIL;
}
#endif

/*******************************************************************************
*   smemComModuleAdapterInit
*
* DESCRIPTION:
*       Init HW adapter for a Communication Module device.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       dataPtr     - pointer to read data
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
static GT_VOID smemComModuleReadDeviceAndVendorId
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT GT_U32 * dataPtr
)
{
    SNET_BUS_INTER_TYPE_ENT comInterface;
    GT_U32 regData;
    GT_U32 cmAdapterId;
    GT_U32 cmInterfaceId;

    cmAdapterId = SMEM_CM_INST_ADAPTER_ID_GET_MAC(devObjPtr);
    comInterface = SMEM_CM_INST_INTERFACE_TYPE_GET_MAC(devObjPtr);
    cmInterfaceId = SMEM_CM_INST_INTERFACE_ID_GET_MAC(devObjPtr, comInterface);

    smemComModuleSmiRead(cmAdapterId, cmInterfaceId, 0x4c, &regData);
    dataPtr[0] = (regData >> 4) << 16;
    smemComModuleSmiRead(cmAdapterId, cmInterfaceId, 0x50, &regData);
    dataPtr[0] |= (GT_U16)regData;
}
/*******************************************************************************
*   smemComModuleAdapterInit
*
* DESCRIPTION:
*       Init HW adapter for a Communication Module device.
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
static GT_VOID smemComModuleAdapterInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32 retVal;
    int i;
    SNET_BUS_INTER_TYPE_ENT comInterface;
    static int  numOfCmAdapters = 0;        /* Total adapters number */
    static AdpDesc * cmArrPtr = NULL;       /* Array of CM memory instances. Allocated once for Adapters Description */
    static AdpDesc * cmActivePtr = NULL;    /* Pointer to active CM */

    comInterface = SMEM_CM_INST_INTERFACE_TYPE_GET_MAC(devObjPtr);

    if(numOfCmAdapters == 0)
    {
        /* Register the application only once */
        retVal = CS_RegisterClient("appDemoSim");
        if (retVal)
        {
            skernelFatalError("CS_RegisterClient: cannot register CM client");
        }
        /* Detect existing Adapters */
        CS_DetectAdapters();

        /* Get the array of Adapters' Descriptions - the memory for pCmArray
        will be allocated by the CS_GetDetectedAdaptersDescription() function.
        Do not forget to free this memory */
        retVal = CS_GetDetectedAdaptersDescription((int *)&numOfCmAdapters, (AdpDesc **)&cmArrPtr);
        if(retVal)
        {
            skernelFatalError("CS_GetDetectedAdaptersDescription: cannot find CM adapters");
        }

        for(i = 0; i < numOfCmAdapters; i++)
        {
            /* Connected and unlocked ==> available */
            if(cmArrPtr[i]._Status == UNLOCKED)
            {
                if((comInterface == SNET_BUS_SMI_E  && (cmArrPtr[i]._ProtocolId == ProtocolSMI)) ||
                   (comInterface == SNET_BUS_TWSI_E && (cmArrPtr[i]._ProtocolId == ProtocolI2C)))
                {
                    /* Connected adapter found */
                    cmActivePtr = &cmArrPtr[i];
                    break;
                }
            }
        }

        if(cmActivePtr == NULL)
        {
            skernelFatalError("smemComModuleInit: cannot find connected CM adapters");
        }

        switch(comInterface)
        {
            case SNET_BUS_SMI_E:
                retVal = CS_SetSMIConfigParams(cmActivePtr->_AdapterId,
                                               SMEM_CM_FREQUENCY_CNS,
                                               SMEM_CM_MODE_SLOW_CNS);
                if(retVal)
                {
                    skernelFatalError("CS_SetSMIConfigParams: can't set SMI configuration, error %d", retVal);
                }
                break;
            case SNET_BUS_TWSI_E:
                /* CM currently doesn't support TWSI connection */
                skernelFatalError("CS_SetI2CConfigParams: CM currently doesn't support TWSI connection");

                retVal = CS_SetI2CConfigParams(cmActivePtr->_AdapterId,
                                               SMEM_CM_FREQUENCY_CNS);
                if(retVal)
                {
                    skernelFatalError("CS_SetI2CConfigParams: can't set TWSI configuration, error %d", retVal);
                }
                break;
            default:
                break;
        }

        retVal = CS_ConnectToAdapter(cmActivePtr->_AdapterId);
        if(retVal)
        {
            skernelFatalError("ConnectToAdapter: cannot connect to CM adapter %d, error %d\n",
                              cmActivePtr->_AdapterId, retVal);
        }
    }

    /* Bind CM pointer to object structure */
    devObjPtr->cmMemParamPtr->cmActivePtr = (GT_VOID *)cmActivePtr;
#if 0
    retVal = smemComModuleGetSmiId(devObjPtr);
    if(retVal)
    {
        skernelFatalError("smemComModuleGetSmiId: cannot find active SMI device, \
                          error %d\n", retVal);
    }
#endif
}

/*******************************************************************************
*   smemComModuleInit
*
* DESCRIPTION:
*       Init memory module for a Communication Module device.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       SCIB_RW_MEMORY_FUN - pointer to function that provides to CPSS/PSS read/write
*       services to manipulate with registers/tables data of real board
*
* COMMENTS:
*
*
*******************************************************************************/
SCIB_RW_MEMORY_FUN smemComModuleInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SCIB_RW_MEMORY_FUN funcRwMemPtr;

    smemComModuleAdapterInit(devObjPtr);

    /* Set CM specific R/W Memory function */
    funcRwMemPtr = smemComModuleReadWriteMemory;

    return funcRwMemPtr;
}

/*******************************************************************************
* smemComModuleSmiRead
*
* DESCRIPTION:
*       Reads the unmasked bits of a register using SMI.
*
* INPUTS:
*       cmAdapterId     - CM adapter ID
*       cmInterfaceId   - device interface ID
*       regAddr - Register address to read from.
*
* OUTPUTS:
*       dataPtr    - Data read from register.
*
* RETURNS:
*       None
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID smemComModuleSmiRead
(
    GT_U32  cmAdapterId,
    GT_U32  cmInterfaceId,
    GT_U32  regAddr,
    GT_U32* dataPtr
)
{
    int rc;
    unsigned long  msb;
    unsigned long  lsb;
    register GT_U32  timeOut;
    unsigned long  stat;

    /* write addr to read */
    msb = regAddr >> 16;
    lsb = regAddr & 0xFFFF;

    rc = CS_WriteSMI(cmAdapterId, cmInterfaceId,
                     SMEM_CM_SMI_READ_ADDRESS_MSB_REGISTER, msb);
    if(rc)
    {
        skernelFatalError("smemComModuleSmiRead: error reading adapter ID %d, address %d\n",
                          cmAdapterId, regAddr);
    }
    rc = CS_WriteSMI(cmAdapterId, cmInterfaceId,
                     SMEM_CM_SMI_READ_ADDRESS_LSB_REGISTER, lsb);
    if (rc)
    {
        skernelFatalError("smemComModuleSmiRead: error reading adapter ID %d, address %d\n",
                          cmAdapterId, regAddr);
    }

    /* wait for read done */
    for (timeOut = SMEM_CM_SMI_TIMEOUT_COUNTER; ; timeOut--)
    {
        rc = CS_ReadSMI(cmAdapterId, cmInterfaceId,
                        SMEM_CM_SMI_STATUS_REGISTER, &stat);
        if (rc)
        {
            skernelFatalError("smemComModuleSmiRead: error reading adapter ID %d, address %d\n",
                              cmAdapterId, regAddr);
        }

        if ((stat & SMEM_CM_SMI_STATUS_READ_READY) != 0)
        {
            break;
        }

        if (0 == timeOut)
        {
            skernelFatalError("smemComModuleSmiRead: error reading adapter ID %d, address %d\n",
                              cmAdapterId, regAddr);
        }
    }

    /* read data */
    rc = CS_ReadSMI(cmAdapterId, cmInterfaceId,
                    SMEM_CM_SMI_READ_DATA_MSB_REGISTER, &msb);
    if (rc)
    {
        skernelFatalError("smemComModuleSmiRead: error reading adapter ID %d, address %d\n",
                          cmAdapterId, regAddr);
    }

    rc = CS_ReadSMI(cmAdapterId, cmInterfaceId,
                    SMEM_CM_SMI_READ_DATA_LSB_REGISTER, &lsb);
    if (rc)
    {
        skernelFatalError("smemComModuleSmiRead: error reading adapter ID %d, address %d\n",
                          cmAdapterId, regAddr);
    }

    *dataPtr = ((msb & 0xFFFF) << 16) | (lsb & 0xFFFF);
}

/*******************************************************************************
* smemComModuleSmiWrite
*
* DESCRIPTION:
*       Writes the unmasked bits of a register using SMI.
*
* INPUTS:
*       cmAdapterId     - CM adapter ID
*       cmInterfaceId   - Device interface ID
*       regAddr         - Register address to write to.
*       data            - Data to be written to register.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID smemComModuleSmiWrite
(
    GT_U32  cmAdapterId,
    GT_U32  cmInterfaceId,
    GT_U32  regAddr,
    GT_U32  data
)
{
    GT_STATUS   rc;
    unsigned long  msb;
    unsigned long  lsb;
    register GT_U32 timeOut;
    unsigned long  stat;

    /* wait for write done */
    for (timeOut = SMEM_CM_SMI_TIMEOUT_COUNTER; ; timeOut--)
    {
        rc = CS_ReadSMI(cmAdapterId, cmInterfaceId,
                        SMEM_CM_SMI_STATUS_REGISTER, &stat);
        if(rc)
        {
            skernelFatalError("smemComModuleSmiWrite: error writing adapter ID %d, address %d\n",
                              cmAdapterId, regAddr);
        }

        if ((stat & SMEM_CM_SMI_STATUS_WRITE_DONE) != 0)
        {
            break;
        }

        if (0 == timeOut)
        {
            skernelFatalError("smemComModuleSmiWrite: error writing adapter ID %d, address %d\n",
                              cmAdapterId, regAddr);
        }
    }

    /* write addr to write */
    msb = regAddr >> 16;
    lsb = regAddr & 0xFFFF;

    rc = CS_WriteSMI(cmAdapterId, cmInterfaceId,
                     SMEM_CM_SMI_WRITE_ADDRESS_MSB_REGISTER, msb);
    if (rc)
    {
        skernelFatalError("smemComModuleSmiWrite: error writing adapter ID %d, address %d\n",
                          cmAdapterId, regAddr);
    }

    rc = CS_WriteSMI(cmAdapterId, cmInterfaceId,
                     SMEM_CM_SMI_WRITE_ADDRESS_LSB_REGISTER, lsb);
    if (rc)
    {
        skernelFatalError("smemComModuleSmiWrite: error writing adapter ID %d, address %d\n",
                          cmAdapterId, regAddr);
    }


    /* write data to write */
    msb = data >> 16;
    lsb = data & 0xFFFF;
    rc = CS_WriteSMI(cmAdapterId, cmInterfaceId,
                     SMEM_CM_SMI_WRITE_DATA_MSB_REGISTER, msb);
    if(rc)
    {
        skernelFatalError("smemComModuleSmiWrite: error writing adapter ID %d, address %d\n",
                          cmAdapterId, regAddr);
    }

    rc = CS_WriteSMI(cmAdapterId, cmInterfaceId,
                     SMEM_CM_SMI_WRITE_DATA_LSB_REGISTER, lsb);
    if (rc)
    {
        skernelFatalError("smemComModuleSmiWrite: error writing adapter ID %d, address %d\n",
                          cmAdapterId, regAddr);
    }
}

/*******************************************************************************
* smemComModuleTwsiRead
*
* DESCRIPTION:
*       Reads the unmasked bits of a register using TWSI.
*
* INPUTS:
*       cmAdapterId     - CM adapter ID
*       cmInterfaceId   - device interface ID
*       pciRead         - PCI memory read
*       regAddr         - Register address to read from.
*
* OUTPUTS:
*       dataPtr    - Data read from register.
*
* RETURNS:
*       None
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID smemComModuleTwsiRead
(
    IN GT_U32   cmAdapterId,
    IN GT_U32   cmInterfaceId,
    IN GT_BIT   pciRead,
    IN GT_U32   regAddr,
    INOUT GT_U32 * dataPtr
)
{
    GT_U8 * twsiRdDataBuffPtr;
    GT_U8 regCharAddr[4];
    GT_U8 cmCharInterfaceId;
    GT_U8 dstPtr[4];
    GT_STATUS rc;

    /*Phase 1: Master Drives Address and Data over TWSI*/
    simLongToChar(cmInterfaceId, dstPtr);
    cmCharInterfaceId = dstPtr[3];
    simLongToChar(regAddr, regCharAddr);
    SIM_ON_READ_SET_BITS31(regCharAddr);
    if(pciRead == 0)
    {
        SIM_RESET_BIT_30(regCharAddr);
    }
    else
    {
        /* Read from PCI registers */
        SIM_ON_READ_SET_BIT_30(regCharAddr);
    }

    rc = CS_WriteI2C(cmAdapterId, cmCharInterfaceId, 4, regCharAddr, GT_FALSE);
    if(rc)
    {
        skernelFatalError("smemComModuleTwsiRead: error reading adapter ID %d, address %d\n",
                          cmAdapterId, regAddr);
    }

    rc = CS_ReadI2C(cmAdapterId, cmCharInterfaceId, 4, &twsiRdDataBuffPtr, GT_TRUE);
    if(rc)
    {
        skernelFatalError("smemComModuleTwsiRead: error reading adapter ID %d, address %d\n",
                          cmAdapterId, regAddr);
    }

    if(twsiRdDataBuffPtr)
    {
        simCharToLong(twsiRdDataBuffPtr, dataPtr);
        free(twsiRdDataBuffPtr);
    }
}

/*******************************************************************************
* smemComModuleTwsiWrite
*
* DESCRIPTION:
*       Writes the unmasked bits of a register using TWSI.
*
* INPUTS:
*       cmAdapterId     - CM adapter ID
*       cmInterfaceId   - device interface ID
*       regAddr         - Register address to read from.
*
* OUTPUTS:
*       dataPtr    - data to be written to register.
*
* RETURNS:
*       GT_OK       - on success
*       GT_FAIL     - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS smemComModuleTwsiWrite
(
    IN GT_U32   cmAdapterId,
    IN GT_U32 cmInterfaceId,
    IN GT_BIT   pciRead,
    IN GT_U32   regAddr,
    IN GT_U32 * dataPtr
)
{
    GT_U8 regCharAddr[4];
    GT_U8 regCharData[4];
    GT_U8 regCharAddrData[8];
    GT_U8 cmCharInterfaceId;
    GT_U8 dstPtr[4];
    GT_STATUS rc;

    /*Phase 1: Master Drives Address and Data over TWSI*/
    simLongToChar(cmInterfaceId, dstPtr);
    cmCharInterfaceId = dstPtr[3];
    simLongToChar (regAddr, regCharAddr);
    SIM_ON_WRITE_RESET_BIT31 (regCharAddr);

    simLongToChar (dataPtr[0], regCharData);
    simConcatCharArray(regCharAddr, regCharData, regCharAddrData);

    rc = CS_WriteI2C(cmAdapterId, cmCharInterfaceId, 8, regCharAddrData, GT_TRUE);
    if(rc)
    {
        skernelFatalError("smemComModuleTwsiWrite: error writing adapter ID %d, address %d\n",
                          cmAdapterId, regAddr);
    }

    return GT_OK;
}

/*******************************************************************************
*   smemComModuleReadMemory
*
* DESCRIPTION:
*       Provides CM SMI or TWSI hardware read memory transaction.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       address     - address of memory(register or table).
*       memPtr      - pointer to memory.
*
* OUTPUTS:
*
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID smemComModuleReadMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 address,
    INOUT GT_U32 * memPtr
)
{
    SNET_BUS_INTER_TYPE_ENT comInterface;
    GT_U32 cmAdapterId;
    GT_U32 cmInterfaceId;

    cmAdapterId = SMEM_CM_INST_ADAPTER_ID_GET_MAC(devObjPtr);
    comInterface = SMEM_CM_INST_INTERFACE_TYPE_GET_MAC(devObjPtr);
    cmInterfaceId = SMEM_CM_INST_INTERFACE_ID_GET_MAC(devObjPtr, comInterface);

    if(comInterface == SNET_BUS_SMI_E)
    {
        smemComModuleSmiRead(cmAdapterId, cmInterfaceId, address, memPtr);
    }
    else
    {
        smemComModuleTwsiRead(cmAdapterId, cmInterfaceId,
                              REGISTER_ADDR_SPACE_CNS, address, memPtr);
    }
}

/*******************************************************************************
*   smemComModuleWriteMemory
*
* DESCRIPTION:
*       Provides CM SMI or TWSI hardware write memory transaction.
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       address         - address of memory(register or table).
*       memSize         - size of memory.
*       memPtr          - pointer to memory.
*
* OUTPUTS:
*
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID smemComModuleWriteMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 address,
    IN GT_U32 * memPtr
)
{
    SNET_BUS_INTER_TYPE_ENT comInterface;
    GT_U32 cmAdapterId;
    GT_U32 cmInterfaceId;

    cmAdapterId = SMEM_CM_INST_ADAPTER_ID_GET_MAC(devObjPtr);
    comInterface = SMEM_CM_INST_INTERFACE_TYPE_GET_MAC(devObjPtr);
    cmInterfaceId = SMEM_CM_INST_INTERFACE_ID_GET_MAC(devObjPtr, comInterface);

    if(comInterface == SNET_BUS_SMI_E)
    {
        smemComModuleSmiWrite(cmAdapterId, cmInterfaceId, address, memPtr[0]);
    }
    else
    {
        smemComModuleTwsiWrite(cmAdapterId,cmInterfaceId, 0, address, memPtr);
    }
}

/*******************************************************************************
*   smemComModulePciReadMemory
*
* DESCRIPTION:
*       Emulates PCI hardware read memory transaction.
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       address         - address of memory(register or table).
*       memPtr          - pointer to memory.
*
* OUTPUTS:
*
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID smemComModulePciReadMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 address,
    INOUT GT_U32 * memPtr
)
{
    SNET_BUS_INTER_TYPE_ENT comInterface;
    GT_U32 regData;
    GT_U32 cmAdapterId;
    GT_U32 cmInterfaceId;

    cmAdapterId = SMEM_CM_INST_ADAPTER_ID_GET_MAC(devObjPtr);
    comInterface = SMEM_CM_INST_INTERFACE_TYPE_GET_MAC(devObjPtr);
    cmInterfaceId = SMEM_CM_INST_INTERFACE_ID_GET_MAC(devObjPtr, comInterface);

    if(comInterface == SNET_BUS_SMI_E)
    {
        switch(address)
        {
            /* Device and vendor ID */
            case (0x0):
                smemComModuleReadDeviceAndVendorId(devObjPtr, memPtr);
                break;
            case (0x8):
                /* Revision ID */
                smemComModuleSmiRead(cmAdapterId, cmInterfaceId, 0x4c, &regData);
                memPtr[0] = regData & 0xf;
                break;
            case (0x10):
                /* Internal PCI base */
                smemComModuleReadDeviceAndVendorId(devObjPtr, &internalBaseArray[devObjPtr->deviceId]);
                *memPtr = (GT_UINTPTR)&internalBaseArray[devObjPtr->deviceId];
                break;
            case (0x14):
                /* PCI base address */
                *memPtr = devObjPtr->deviceHwId;
                break;
            default:
                *memPtr = (GT_UINTPTR)&defaultAddress;
                break;
        }
    }
    else
    {
        smemComModuleTwsiRead(cmAdapterId, cmInterfaceId,
                              PEX_ADDR_SPACE_CNS, address, memPtr);
    }
}

/*******************************************************************************
*   smemComModulePciWriteMemory
*
* DESCRIPTION:
*       Emulates PCI hardware write memory transaction.
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       address         - address of memory(register or table).
*       memPtr          - pointer to memory.
*
* OUTPUTS:
*
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID smemComModulePciWriteMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 address,
    IN GT_U32 * memPtr
)
{
    SNET_BUS_INTER_TYPE_ENT comInterface;
    GT_U32 cmAdapterId;
    GT_U32 cmInterfaceId;

    cmAdapterId = SMEM_CM_INST_ADAPTER_ID_GET_MAC(devObjPtr);
    comInterface = SMEM_CM_INST_INTERFACE_TYPE_GET_MAC(devObjPtr);
    cmInterfaceId = SMEM_CM_INST_INTERFACE_ID_GET_MAC(devObjPtr, comInterface);

    if(comInterface == SNET_BUS_SMI_E)
    {
        switch(address)
        {
            /* Device and vendor ID */
            case (0x0):
            /* Revision ID */
            case (0x8):
                skernelFatalError("smemComModulePciWriteMemory: read only memory -write not allowed  %d, address %d\n",
                                  cmAdapterId, address);
                break;
            default:
                break;
        }

    }
    else
    {
        smemComModuleTwsiWrite(cmAdapterId,cmInterfaceId, 1, address, memPtr);
    }
}

/*******************************************************************************
*   smemComModuleReadWriteMemory
*
* DESCRIPTION:
*       Provides CM hardware write/read memory transaction.
*
* INPUTS:
*       accessType      - Read or Write flag
*       devObjPtr    - pointer to device object.
*       address         - address of memory(register or table).
*       memSize         - size of memory.
*       memPtr          - pointer to memory.
*
* OUTPUTS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID smemComModuleReadWriteMemory
(
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_VOID * devObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    INOUT GT_U32 * memPtr
)
{
    /* Memory mutex lock is done in SCIB level */

    switch (accessType)
    {
        case SCIB_MEMORY_WRITE_PCI_E:
            smemComModulePciWriteMemory(devObjPtr, address, memPtr);
            break;

        case SCIB_MEMORY_READ_PCI_E:
            smemComModulePciReadMemory(devObjPtr, address, memPtr);
            break;

        case SCIB_MEMORY_WRITE_E:
            smemComModuleWriteMemory(devObjPtr, address, memPtr);
            break;

        case SCIB_MEMORY_READ_E:
            smemComModuleReadMemory(devObjPtr, address, memPtr);
            break;

        default:
            skernelFatalError(" smemComModuleRWMem: not valid mode[%d]", accessType);
            break;
    }

    /* Memory mutex unlock is done in SCIB level */

    return ;
}

/*******************************************************************************
* simLongToChar
*
* DESCRIPTION:
*       Transforms unsigned long int type to 4 separate chars.
*
* INPUTS:
*       src - source unsigned long integer.
*
* OUTPUTS:
*       dst - Array of 4 chars
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL   - on failure
*
* COMMENTS:
*           MSB is copied to dst[0]!!
*
*******************************************************************************/
static GT_STATUS simLongToChar (IN GT_U32 src, OUT GT_U8 dst[4])
{
    GT_U32 i;

    for (i = 4 ; i > 0 ; i--)
    {
        dst[i-1] = (GT_U8) src & 0xFF;
        src>>=8;
    }

  return GT_OK;
}

/*******************************************************************************
* simCharToLong
*
* DESCRIPTION:
*       Transforms an array of 4 separate chars to unsigned long integer type
*
* INPUTS:
*       src - Source Array of 4 chars
*
* OUTPUTS:
*       dst - Unsigned long integer number
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL   - on failure
*
* COMMENTS:
*           MSB resides in src[0]!!
*
*******************************************************************************/
static GT_STATUS simCharToLong (IN GT_U8 * srcPtr, OUT GT_U32 *dst)
{
    GT_U32  i;
    GT_U8 tempU8;
    *dst = 0;

    for (i = 4 ; i > 0 ; i--)
    {
        tempU8 = srcPtr[i-1];
        *dst |= (GT_U32)tempU8 << (8*(4-i));
    }

    return GT_OK;
}

/*******************************************************************************
* mvConcatCharArrays
*
* DESCRIPTION:
*       Concatenate 2 Arrays of Chars to one Array of chars
*
* INPUTS:
*       src0 - Source Array of 4 chars long
*       src1 - Source Array of 4 chars long
*
* OUTPUTS:
*       dst - Concatenated Array of 8 chars long {src1,src0}
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL   - on failure
*
* COMMENTS:
*           None
*
*******************************************************************************/
static GT_STATUS simConcatCharArray (IN GT_U8 src0[4], IN GT_U8 src1[4],
                                    OUT GT_U8 dst[8])
{
    GT_U32 i=0,j; /*Source and Dest Counters*/

    for (j=0; j < 8; j++)
    {
        if (j < 4)
            dst[j] = src0[i++];
        else if (j == 4)
        {
            dst[j] = src1[0];
            i=1;
        } else
            dst[j] = src1[i++];
    }
    return GT_OK;
}

/*******************************************************************************
* smemSub20TwsiWrite
*
* DESCRIPTION:
*       Writes the unmasked bits of a register using TWSI.
*
* INPUTS:
*       devObjPtr       - CM adapter ID
*       regAddr         - Register address to read from.
*       dataPtr         - data to be written to register.
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK           - on success write
*       GT_FAIL         - on write fail
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS smemSub20TwsiWrite
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32   regAddr,
    IN GT_U32   data
)
{
    int rc;
    sub_handle  handle;
    GT_U8 buffer[4];
    int twsiId;

    handle = SMEM_SUB20_HANDLE_GET_MAC(devObjPtr);
    twsiId = SMEM_CM_INST_INTERFACE_ID_GET_MAC(devObjPtr, SNET_BUS_TWSI_E);
    simLongToChar(data, buffer);

    rc = sub_i2c_write(handle, twsiId, regAddr, 4, (char *)buffer, 4);
    if(rc)
    {
        return GT_FAIL;
    }

    return GT_OK;
}

/*******************************************************************************
* smemSub20TwsiRead
*
* DESCRIPTION:
*       Read the unmasked bits from a register using TWSI.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       regAddr         - register address to read from.
*
* OUTPUTS:
*       dataPtr         - read register data.
*
* RETURNS:
*       GT_OK           - on success read
*       GT_FAIL         - on read fail
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS smemSub20TwsiRead
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32   regAddr,
    OUT GT_U32 * dataPtr
)
{
    int rc;
    GT_U8 buffer[4];
    sub_handle  handle;
    int twsiId;

    handle = SMEM_SUB20_HANDLE_GET_MAC(devObjPtr);
    twsiId = SMEM_CM_INST_INTERFACE_ID_GET_MAC(devObjPtr, SNET_BUS_TWSI_E);


    rc = sub_i2c_read(handle, twsiId, regAddr, 4, (char *)buffer, 4);
    if(rc)
    {
        return GT_FAIL;
    }

    simCharToLong(buffer, dataPtr);

    return GT_OK;
}

/*******************************************************************************
* smemSub20SmiRead
*
* DESCRIPTION:
*       Reads the unmasked bits of a register using SMI.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       regAddr         - Register address to read from.
*
* OUTPUTS:
*       dataPtr         - Data read from register.
*
* RETURNS:
*       GT_OK           - on success read
*       GT_FAIL         - on fail read
*       GT_TIMEOUT      - transaction timeout
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS smemSub20SmiRead
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    GT_U32  regAddr,
    GT_U32* dataPtr
)
{
    int rc;
    int  msb;
    int  lsb;
    register GT_U32  timeOut;
    int  stat;
    sub_handle  handle;
    int smiId;
    int val;

    handle = SMEM_SUB20_HANDLE_GET_MAC(devObjPtr);
    smiId = SMEM_CM_INST_INTERFACE_ID_GET_MAC(devObjPtr, SNET_BUS_SMI_E);

    /* write addr to read */
    msb = regAddr >> 16;
    lsb = regAddr & 0xFFFF;

    rc = sub_mdio22(handle,
                    SUB_MDIO22_WRITE,
                    smiId,
                    SMEM_CM_SMI_READ_ADDRESS_MSB_REGISTER,
                    msb, &val);
    if(rc)
    {
        return GT_FAIL;
    }

    rc = sub_mdio22(handle,
                    SUB_MDIO22_WRITE,
                    smiId,
                    SMEM_CM_SMI_READ_ADDRESS_LSB_REGISTER,
                    lsb, 0);
    if (rc)
    {
        return GT_FAIL;
    }

    /* wait for read done */
    for (timeOut = SMEM_CM_SMI_TIMEOUT_COUNTER; ; timeOut--)
    {
        rc = sub_mdio22(handle,
                        SUB_MDIO22_READ,
                        smiId,
                        SMEM_CM_SMI_STATUS_REGISTER,
                        0, &stat);
        if (rc)
        {
            return GT_FAIL;
        }

        if ((stat & SMEM_CM_SMI_STATUS_READ_READY) != 0)
        {
            break;
        }

        if (0 == timeOut)
        {
            skernelFatalError("smemSub20SmiRead: timeout - SMI reading "
                              "ID %s, address 0x%08x\n",
                              SMEM_SUB20_ADAPTER_ID_GET_MAC(devObjPtr), regAddr);
        }
    }

    /* read data */
    rc = sub_mdio22(handle,
                    SUB_MDIO22_READ,
                    smiId,
                    SMEM_CM_SMI_READ_DATA_MSB_REGISTER,
                    0, &msb);

    if (rc)
    {
        return GT_FAIL;
    }

    rc = sub_mdio22(handle,
                    SUB_MDIO22_READ,
                    smiId,
                    SMEM_CM_SMI_READ_DATA_LSB_REGISTER,
                    0, &lsb);
    if (rc)
    {
        return GT_FAIL;
    }

    *dataPtr = ((msb & 0xFFFF) << 16) | (lsb & 0xFFFF);

    return GT_OK;
}

/*******************************************************************************
* smemSub20SmiWrite
*
* DESCRIPTION:
*       Writes the unmasked bits of a register using SUB20 SMI interface.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       regAddr         - Register address to write to.
*       data            - Data to be written to register.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK           - on success write
*       GT_FAIL         - on write fail
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS smemSub20SmiWrite
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    GT_U32  regAddr,
    GT_U32  data
)
{
    GT_STATUS   rc;
    int  msb;
    int  lsb;
    register GT_U32 timeOut;
    int  stat;
    sub_handle  handle;
    int smiId;

    handle = SMEM_SUB20_HANDLE_GET_MAC(devObjPtr);
    smiId = SMEM_CM_INST_INTERFACE_ID_GET_MAC(devObjPtr, SNET_BUS_SMI_E);

    /* wait for write done */
    for (timeOut = SMEM_CM_SMI_TIMEOUT_COUNTER; ; timeOut--)
    {
        rc = sub_mdio22(handle,
                        SUB_MDIO22_READ,
                        smiId,
                        SMEM_CM_SMI_STATUS_REGISTER,
                        0, &stat);
        if(rc)
        {
            return GT_FAIL;
        }

        if ((stat & SMEM_CM_SMI_STATUS_WRITE_DONE) != 0)
        {
            break;
        }

        if (0 == timeOut)
        {
            skernelFatalError("smemSub20SmiWrite: timeout - SMI writing "
                              "ID %s, address 0x%08x\n",
                              SMEM_SUB20_ADAPTER_ID_GET_MAC(devObjPtr), regAddr);
        }
    }

    /* write addr to write */
    msb = regAddr >> 16;
    lsb = regAddr & 0xFFFF;

    rc = sub_mdio22(handle,
                    SUB_MDIO22_WRITE,
                    smiId,
                    SMEM_CM_SMI_WRITE_ADDRESS_MSB_REGISTER,
                    msb, 0);
    if (rc)
    {
        return GT_FAIL;
    }

    rc = sub_mdio22(handle,
                    SUB_MDIO22_WRITE,
                    smiId,
                    SMEM_CM_SMI_WRITE_ADDRESS_LSB_REGISTER,
                    lsb, 0);
    if (rc)
    {
        return GT_FAIL;
    }

    /* write data to write */
    msb = data >> 16;
    lsb = data & 0xFFFF;
    rc = sub_mdio22(handle,
                    SUB_MDIO22_WRITE,
                    smiId,
                    SMEM_CM_SMI_WRITE_DATA_MSB_REGISTER,
                    msb, 0);
    if(rc)
    {
        return GT_FAIL;
    }

    rc = sub_mdio22(handle,
                    SUB_MDIO22_WRITE,
                    smiId,
                    SMEM_CM_SMI_WRITE_DATA_LSB_REGISTER,
                    lsb, 0);
    if (rc)
    {
        return GT_FAIL;
    }

    return GT_OK;
}

/*******************************************************************************
*   smemSub20ReadDeviceAndVendorId
*
* DESCRIPTION:
*       Device and Vendor ID Read
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       dataPtr     - pointer to read data
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID smemSub20ReadDeviceAndVendorId
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    OUT GT_U32 * dataPtr
)
{
    SNET_BUS_INTER_TYPE_ENT comInterface;
    GT_U32 regData;
    int rc;

    comInterface = SMEM_CM_INST_INTERFACE_TYPE_GET_MAC(devObjPtr);

    if(comInterface != SNET_BUS_SMI_E)
    {
        skernelFatalError("smemSub20ReadDeviceAndVendorId: error communication interface"
                          "ID %s\n",
                          SMEM_SUB20_ADAPTER_ID_GET_MAC(devObjPtr));
    }

    rc = smemSub20SmiRead(devObjPtr, 0x4c, &regData);
    if(rc)
    {
        skernelFatalError("smemSub20SmiRead: error SMI reading adapter "
                          "ID %s, address 0x4c, error: %s\n",
                          SMEM_SUB20_ADAPTER_ID_GET_MAC(devObjPtr), sub_strerror(sub_errno));
    }
    dataPtr[0] = (regData >> 4) << 16;
    rc = smemSub20SmiRead(devObjPtr, 0x50, &regData);
    if(rc)
    {
        skernelFatalError("smemSub20SmiRead: error SMI reading adapter "
                          "ID %s, address 0x50, error: %s\n",
                          SMEM_SUB20_ADAPTER_ID_GET_MAC(devObjPtr), sub_strerror(sub_errno));
    }
    dataPtr[0] |= (GT_U16)regData;
}

/*******************************************************************************
*   smemSub20PciWriteMemory
*
* DESCRIPTION:
*       Emulates SUB20 PCI hardware write memory transaction.
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       regAddr      - address of memory(register or table).
*       memPtr       - pointer to memory.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*
*******************************************************************************/
static GT_VOID smemSub20PciWriteMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 regAddr,
    IN GT_U32 * memPtr
)
{
    GT_STATUS status;
    SNET_BUS_INTER_TYPE_ENT comInterface;
    comInterface = SMEM_CM_INST_INTERFACE_TYPE_GET_MAC(devObjPtr);

    if(comInterface == SNET_BUS_SMI_E)
    {
        switch(regAddr)
        {
            /* Device and vendor ID */
            case (0x0):
            /* Revision ID */
            case (0x8):
                skernelFatalError("smemSub20PciWriteMemory: error SMI write  0x%08x, address %d\n",
                                  SMEM_SUB20_ADAPTER_ID_GET_MAC(devObjPtr), regAddr);
                break;
            default:
                break;
        }

    }
    else
    {
        status = smemSub20TwsiWrite(devObjPtr, regAddr, memPtr[0]);
        if(status != GT_OK)
        {
            skernelFatalError("smemSub20TwsiWrite: error TWSI write adapter "
                              "ID %s, address 0x%08x, error: %s\n",
                              SMEM_SUB20_ADAPTER_ID_GET_MAC(devObjPtr), regAddr,
                              sub_strerror(sub_errno));
        }
    }
}

/*******************************************************************************
*   smemSub20PciReadMemory
*
* DESCRIPTION:
*       Emulates PCI hardware read memory transaction.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       regAddr         - address of memory(register or table).
*
* OUTPUTS:
*       memPtr          - pointer to read memory.
*
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID smemSub20PciReadMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 regAddr,
    INOUT GT_U32 * memPtr
)
{
    SNET_BUS_INTER_TYPE_ENT comInterface;
    GT_U32 regData;
    GT_STATUS rc;

    comInterface = SMEM_CM_INST_INTERFACE_TYPE_GET_MAC(devObjPtr);

    if(comInterface == SNET_BUS_SMI_E)
    {
        switch(regAddr)
        {
            /* Device and vendor ID */
            case (0x0):
                smemSub20ReadDeviceAndVendorId(devObjPtr, memPtr);
                break;
            case (0x8):
                /* Revision ID */
                rc = smemSub20SmiRead(devObjPtr, 0x4c, &regData);
                if(rc)
                {
                    skernelFatalError("smemSub20SmiRead: error SMI reading adapter "
                                      "ID %s, address 0x4c, error: %s\n",
                          SMEM_SUB20_ADAPTER_ID_GET_MAC(devObjPtr), sub_strerror(sub_errno));
                }
                memPtr[0] = regData & 0xf;
                break;
            case (0x10):
                /* Internal PCI base */
                smemSub20ReadDeviceAndVendorId(devObjPtr,
                                               &internalBaseArray[devObjPtr->deviceId]);
                *memPtr = (GT_UINTPTR)&internalBaseArray[devObjPtr->deviceId];
                break;
            case (0x14):
                /* PCI base address */
                *memPtr = devObjPtr->deviceHwId;
                break;
            default:
                *memPtr = (GT_UINTPTR)&defaultAddress;
                break;
        }
    }
    else
    {
        rc = smemSub20TwsiRead(devObjPtr, regAddr, memPtr);
        if(rc)
        {
            skernelFatalError("smemSub20TwsiRead: error TWSI reading adapter "
                              "ID %s, address 0x4c, error: %s\n",
                              SMEM_SUB20_ADAPTER_ID_GET_MAC(devObjPtr), regAddr,
                              sub_strerror(sub_errno));
        }
    }
}

/*******************************************************************************
*   smemSub20ReadMemory
*
* DESCRIPTION:
*       Provides SUB20 SMI or TWSI hardware read memory transaction.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       regAddr     - address of memory(register or table).
*       memPtr      - pointer to memory.
*
* OUTPUTS:
*
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID smemSub20ReadMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 regAddr,
    INOUT GT_U32 * memPtr
)
{
    SNET_BUS_INTER_TYPE_ENT comInterface;
    GT_STATUS status;

    comInterface = SMEM_CM_INST_INTERFACE_TYPE_GET_MAC(devObjPtr);
    if(comInterface == SNET_BUS_SMI_E)
    {
        status = smemSub20SmiRead(devObjPtr, regAddr, memPtr);
        if(status != GT_OK)
        {
            skernelFatalError("smemSub20SmiRead: error SMI reading adapter "
                              "ID %s, address 0x%08x, error: %s\n",
                              SMEM_SUB20_ADAPTER_ID_GET_MAC(devObjPtr), regAddr,
                              sub_strerror(sub_errno));
        }
    }
    else
    {
        status = smemSub20TwsiRead(devObjPtr, regAddr, memPtr);
        if(status != GT_OK)
        {
            skernelFatalError("smemSub20SmiRead: error TWSI reading adapter "
                              "ID %s, address 0x%08x, error: %s\n",
                              SMEM_SUB20_ADAPTER_ID_GET_MAC(devObjPtr), regAddr,
                              sub_strerror(sub_errno));
        }
    }
}
/*******************************************************************************
*   smemSub20WriteMemory
*
* DESCRIPTION:
*       Provides SUB20 SMI or TWSI hardware write memory transaction.
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       regAddr         - address of memory(register or table).
*       memPtr          - pointer to memory.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID smemSub20WriteMemory
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 regAddr,
    IN GT_U32 * memPtr
)
{
    SNET_BUS_INTER_TYPE_ENT comInterface;
    GT_STATUS status;

    comInterface = SMEM_CM_INST_INTERFACE_TYPE_GET_MAC(devObjPtr);
    if(comInterface == SNET_BUS_SMI_E)
    {
        status = smemSub20SmiWrite(devObjPtr, regAddr, memPtr[0]);
        if(status != GT_OK)
        {
            skernelFatalError("smemSub20TwsiWrite: error SMI write adapter "
                              "ID %s, address 0x%08x, error: %s\n",
                              SMEM_SUB20_ADAPTER_ID_GET_MAC(devObjPtr), regAddr,
                              sub_strerror(sub_errno));
        }
    }
    else
    {
        status = smemSub20TwsiWrite(devObjPtr, regAddr, memPtr[0]);
        if(status !=  GT_OK)
        {
            skernelFatalError("smemSub20TwsiWrite: error TWSI write adapter "
                              "ID %s, address 0x%08x, error: %s\n",
                              SMEM_SUB20_ADAPTER_ID_GET_MAC(devObjPtr), regAddr,
                              sub_strerror(sub_errno));
        }
    }
}

/*******************************************************************************
*   smemComModuleReadWriteMemory
*
* DESCRIPTION:
*       Provides CM hardware write/read memory transaction.
*
* INPUTS:
*       accessType      - Read or Write flag
*       devObjPtr       - pointer to device object.
*       regAddr         - address of memory(register or table).
*       memSize         - size of memory.
*       memPtr          - pointer to memory.
*
* OUTPUTS:
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_VOID smemSub20ReadWriteMemory
(
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_VOID * devObjPtr,
    IN GT_U32 regAddr,
    IN GT_U32 memSize,
    INOUT GT_U32 * memPtr
)
{
    /* Memory mutex lock is done in SCIB level */
    switch (accessType)
    {
        case SCIB_MEMORY_WRITE_PCI_E:
            smemSub20PciWriteMemory(devObjPtr, regAddr, memPtr);
            break;

        case SCIB_MEMORY_READ_PCI_E:
            smemSub20PciReadMemory(devObjPtr, regAddr, memPtr);
            break;

        case SCIB_MEMORY_WRITE_E:
            smemSub20WriteMemory(devObjPtr, regAddr, memPtr);
            break;

        case SCIB_MEMORY_READ_E:
            smemSub20ReadMemory(devObjPtr, regAddr, memPtr);
            break;

        default:
            skernelFatalError("smemSub20ReadWriteMemory: not valid mode[%d]", accessType);
            break;
    }

    /* Memory mutex unlock is done in SCIB level */

    return ;
}

/*******************************************************************************
*   smemSub20Init
*
* DESCRIPTION:
*       Init HW adapter for a SUB20 device.
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
static GT_VOID smemSub20Init
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    int rc;

    static SMEM_SUB20_INSTANCE_PARAM_STC * sub20InfoPtr = 0;

    /* Allocate SUB20 structure only once */
    if(sub20InfoPtr == 0)
    {
        sub20InfoPtr =
            (SMEM_SUB20_INSTANCE_PARAM_STC *)malloc(sizeof(SMEM_SUB20_INSTANCE_PARAM_STC));

        if(sub20InfoPtr == 0)
        {
            skernelFatalError("smemSub20Init: SUB20 structure allocation fail");
        }

        sub20InfoPtr->handle = sub_open(0);

        if(!sub20InfoPtr->handle)
        {
            skernelFatalError("sub_open: %s\n", sub_strerror(sub_errno));
        }
        rc = sub_get_product_id(sub20InfoPtr->handle,
                                sub20InfoPtr->adapterIdStr,
                                sizeof(sub20InfoPtr->adapterIdStr));
        if(rc < 0)
        {
            skernelFatalError("sub_get_product_id: %s\n", sub_strerror(sub_errno));
        }
    }

    /* Bind sub20 structure to the device */
    devObjPtr->sub20InfoPtr = sub20InfoPtr;
}

/*******************************************************************************
*   smemSub20AdapterInit
*
* DESCRIPTION:
*       Init memory module for a SUB20 device.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       SCIB_RW_MEMORY_FUN - pointer to function that provides to CPSS/PSS read/write
*       services to manipulate with registers/tables data of real board
*
* COMMENTS:
*
*
*******************************************************************************/
SCIB_RW_MEMORY_FUN smemSub20AdapterInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SCIB_RW_MEMORY_FUN funcRwMemPtr;

    smemSub20Init(devObjPtr);

    /* Set CM specific R/W Memory function */
    funcRwMemPtr = smemSub20ReadWriteMemory;

    return funcRwMemPtr;
}

