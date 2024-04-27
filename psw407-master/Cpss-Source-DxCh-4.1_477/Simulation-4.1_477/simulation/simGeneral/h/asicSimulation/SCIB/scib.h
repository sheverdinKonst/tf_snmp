/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* scib.h
*
* DESCRIPTION:
*       This is a API definition for CPU interface block of the Simulation.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 15 $
*
*******************************************************************************/
#ifndef __scibh
#define __scibh

#include <os/simTypes.h>
#include <os/simTypesBind.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SCIB_NOT_EXISTED_DEVICE_ID_CNS 0xffffffff

#define NUM_BYTES_TO_WORDS(bytes) (((bytes)+3) / 4)

/*
 * Typedef: enum SCIB_MEM_ACCESS_CLIENT
 *
 * Description:
 *      list of memory access clients with separate memories.
 *
 * Fields:
 *      SCIB_MEM_ACCESS_PCI_E - PCI/PEX/MBUS external memory access
 *      SCIB_MEM_ACCESS_DFX_E - DFX server external memory access
 *      SCIB_MEM_ACCESS_CORE_E - PP internal memory
 */
typedef enum
{
    SCIB_MEM_ACCESS_PCI_E = 0,
    SCIB_MEM_ACCESS_DFX_E,
    SCIB_MEM_ACCESS_CORE_E,
    SCIB_MEM_ACCESS_LAST_E
}SCIB_MEM_ACCESS_CLIENT;

/* Each bit in the values of enum SCIB_MEMORY_ACCESS_TYPE */
#define OPERATION_BIT       (1<<0) /* bit 0 */  /* 0 - read , 1 - write   */
#define LAYER_BIT           (1<<1) /* bit 1 */  /* 0 - scib , 1 - skernel */
#define PCI_BIT             (1<<2) /* bit 2 */  /* 0 - non PCI, 1 - PCI */
#define DFX_BIT             (1<<3) /* bit 3 */  /* 0 - non DFX, 1 - DFX */

/* Check if operation type is 'write' (or 'read')*/
#define IS_WRITE_OPERATION_MAC(type)    \
    (((type) & OPERATION_BIT) ? 1 : 0)

/* Check if operation type is 'skernel' (or 'scib')*/
#define IS_SKERNEL_OPERATION_MAC(type)  \
    (((type) & LAYER_BIT) ? 1 : 0)

/* Check if operation type is 'PCI' */
#define IS_PCI_OPERATION_MAC(type)  \
    (((type) & PCI_BIT) ? 1 : 0)

/* Check if operation type is 'DFX' */
#define IS_DFX_OPERATION_MAC(type)  \
    (((type) & DFX_BIT) ? 1 : 0)

typedef enum
{
                            /* PCI/DFX/CORE  skernel     write */
    SCIB_MEMORY_READ_E         = 0                                      ,/*0*/
    SCIB_MEMORY_WRITE_E        =                        OPERATION_BIT   ,/*1*/
    SKERNEL_MEMORY_READ_E      =            LAYER_BIT                   ,/*2*/
    SKERNEL_MEMORY_WRITE_E     =            LAYER_BIT | OPERATION_BIT   ,/*3*/
    SCIB_MEMORY_READ_PCI_E     = PCI_BIT                                ,/*4*/
    SCIB_MEMORY_WRITE_PCI_E    = PCI_BIT |              OPERATION_BIT   ,/*5*/
    SKERNEL_MEMORY_READ_PCI_E  = PCI_BIT |  LAYER_BIT                   ,/*6*/
    SKERNEL_MEMORY_WRITE_PCI_E = PCI_BIT |  LAYER_BIT | OPERATION_BIT   ,/*7*/
    SCIB_MEMORY_READ_DFX_E     = DFX_BIT                                ,/*8*/
    SCIB_MEMORY_WRITE_DFX_E    = DFX_BIT |              OPERATION_BIT   ,/*9*/
    SKERNEL_MEMORY_READ_DFX_E  = DFX_BIT |  LAYER_BIT                   ,/*10*/
    SKERNEL_MEMORY_WRITE_DFX_E = DFX_BIT |  LAYER_BIT | OPERATION_BIT   ,/*11*/

    SCIB_MEMORY_LAST_E = 0xFFFF
}SCIB_MEMORY_ACCESS_TYPE;

/* value for interrupt line that is not used */
#define SCIB_INTERRUPT_LINE_NOT_USED_CNS    0xFFFFFFFF


/*
    problem was seen only in WIN32 , due to OS implementation (and not needed in Linux):

    the 'lock'/'unlock' (scibAccessLock,scibAccessUnlock)was created for next purpose:
    the application don't want to 'lock task' that is currently take the one of
    the simulation semaphore  (like during sbufAlloc) , so we give the
    application a semaphore that it application can use , so it can take this
    semaphore when 'lock tasks/interrupts' so application will know that no task
    is currently inside the simulation when locking it.

    (see in CPSS the use :
        <gtOs\win32\osWin32IntrSim.c>
        <gtOs\win32\osWin32Task.c> )

    NOTE: the protector needed only on NON-distributed simulation.
         because on distributed simulation , on PP side , all actions are
         serialized
*/

#define SCIB_SEM_TAKE scibAccessLock()
#define SCIB_SEM_SIGNAL scibAccessUnlock()

/*******************************************************************************
*  SCIB_RW_MEMORY_FUN
*
* DESCRIPTION:
*      Definition of R/W Skernel memory function.
* INPUTS:
*       accessType  - Define access operation Read or Write.
*       devObjPtr   - Opaque for SCIB device id.
*       address     - Address for ASIC memory.
*       memSize     - Size of ASIC memory to read or write.
*       memPtr      - For Write this pointer to application memory,which
*                     will be copied to the ASIC memory .
*
* OUTPUTS:
*       memPtr     - For Read this pointer to application memory in which
*                     ASIC memory will be copied.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
typedef void (* SCIB_RW_MEMORY_FUN) (
                                   IN SCIB_MEMORY_ACCESS_TYPE accessType,
                                   IN void * deviceObjPtr,
                                   IN GT_U32 address,
                                   IN GT_U32 memSize,
                                   INOUT GT_U32 * memPtr );

/*******************************************************************************
* scibInit0
*
* DESCRIPTION:
*       Init SCIB mutex for scibAccessLock(), scibAccessUnlock
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void scibInit0(void);

/*******************************************************************************
* scibInit
*
* DESCRIPTION:
*       Init SCIB library.
*
* INPUTS:
*       maxDevNumber - maximal number of SKernel devices in the Simulation.
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
void scibInit
(
    IN GT_U32 maxDevNumber
);

/*******************************************************************************
*   scibBindRWMemory
*
* DESCRIPTION:
*       Bind callbacks of SKernel for R/W memory requests.
*
* INPUTS:
*    deviceId   - ID of device, which is equal to PSS Core API device ID.
*    deviceHwId - physical device Id.
*    devObjPtr  - pointer to the opaque for SCIB device object.
*    rwFun      - pointer to the R/W SKernel memory CallBack function.
*    isPP       - bind to pp object or to fa.
*    addressCompletionStatus - device enable/disable address completion
* OUTPUTS:
*       None.
*
* RETURNS:
*
*
* COMMENTS:
*       device object can be fa also.
*
*******************************************************************************/
void scibBindRWMemory
(
    IN GT_U32           deviceId,
    IN GT_U32           deviceHwId,
    IN void         *       deviceObjPtr,
    IN SCIB_RW_MEMORY_FUN   rwFun,
    IN GT_U8                isPP,
    IN GT_BOOL              addressCompletionStatus
);

/*******************************************************************************
*   scibSetIntLine
*
* DESCRIPTION:
*       Set interrupt line for SKernel device.
*
* INPUTS:
*       deviceId  - ID of device, which is equal to PSS Core API device ID.
*       intrline  - number of interrupt line.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
*
* COMMENTS:
*       device object can be fa also.
*
*******************************************************************************/
void scibSetIntLine
(
    IN GT_U32               deviceId,
    IN GT_U32               intrline
);
/*******************************************************************************
*   scibReadMemory
*
* DESCRIPTION:
*       Read memory from SKernel device.
*
* INPUTS:
*       deviceId  - ID of device, which is equal to PSS Core API device ID.
*       memAddr - address of first word to read.
*       length - number of words to read.
*
* OUTPUTS:
*       dataPtr - pointer to copy read data.
*
* RETURNS:
*
*
* COMMENTS:
*
*
*******************************************************************************/
void scibReadMemory
(
    IN  GT_U32        deviceId,
    IN  GT_U32        memAddr,
    IN  GT_U32        length,
    OUT GT_U32 *      dataPtr
 );
/*******************************************************************************
*   scibWriteMemory
*
* DESCRIPTION:
*       Write to memory of a SKernel device.
*
* INPUTS:
*       deviceId  - ID of device, which is equal to PSS Core API device ID.
*       memAddr - address of first word to read.
*       length - number of words to read.
*       dataPtr - pointer to copy read data.
*
* OUTPUTS:
*       none.
*
* RETURNS:
*
*
* COMMENTS:
*       device object can be fa also.
*
*******************************************************************************/
void scibWriteMemory
(
    IN  GT_U32        deviceId,
    IN  GT_U32        memAddr,
    IN  GT_U32        length,
    IN  GT_U32 *      dataPtr
 );
/*******************************************************************************
*   scibPciRegRead
*
* DESCRIPTION:
*       Read PCI registers memory from SKernel device.
*
* INPUTS:
*       deviceId  - ID of device, which is equal to PSS Core API device ID.
*       memAddr - address of first word to read.
*       length - number of words to read.
*
* OUTPUTS:
*       dataPtr - pointer to copy read data.
*
* RETURNS:
*
*
* COMMENTS:
*       device object can not be fa.
*
*******************************************************************************/
void scibPciRegRead
(
    IN  GT_U32        deviceId,
    IN  GT_U32        memAddr,
    IN  GT_U32        length,
    OUT GT_U32 *      dataPtr
);

/*******************************************************************************
*   scibPciRegWrite
*
* DESCRIPTION:
*       Write to PCI memory of a SKernel device.
*
* INPUTS:
*       deviceId  - ID of device, which is equal to PSS Core API device ID.
*       memAddr - address of first word to read.
*       length - number of words to read.
*       dataPtr - pointer to copy read data.
*
* OUTPUTS:
*       none.
*
* RETURNS:
*
*
* COMMENTS:
*        device object can not be fa.
*
*******************************************************************************/
void scibPciRegWrite
(
    IN  GT_U32        deviceId,
    IN  GT_U32        memAddr,
    IN  GT_U32        length,
    IN  GT_U32 *      dataPtr
);


/*******************************************************************************
*   scibSetInterrupt
*
* DESCRIPTION:
*       Generate interrupt for SKernel device.
*
* INPUTS:
*       deviceId  - ID of device, which is equal to PSS Core API device ID.
*
* OUTPUTS:
*       none.
*
* RETURNS:
*
*
* COMMENTS:
*
*
*******************************************************************************/
void scibSetInterrupt
(
    IN  GT_U32        deviceId
);

/*******************************************************************************
* scibSmiRegRead
*
* DESCRIPTION:
*       This function reads a switch's port register.
*
* INPUTS:
*       deviceId - Device object Id.
*       smiDev   - Smi device to read the register for
*       regAddr  - The register's address.
*
* OUTPUTS:
*       data    - The read register's data.
*
* RETURNS:
*       GT_OK on success, or
*       GT_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
void scibSmiRegRead
(
    IN  GT_U32      deviceId,
    IN  GT_U32      smiDev,
    IN  GT_U32      regAddr,
    OUT GT_U32      *data
);

/*******************************************************************************
* scibSmiRegWrite
*
* DESCRIPTION:
*       This function writes to a switch's port register.
*
* INPUTS:
*       deviceId - Device object Id
*       smiDev   - Smi device number to read the register for.
*       regAddr  - The register's address.
*       data     - The data to be written.
* OUTPUTS:
*
*
* RETURNS:
*       GT_OK on success, or
*       GT_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
void scibSmiRegWrite
(
    IN  GT_U32      deviceId,
    IN  GT_U32      smiDev,
    IN  GT_U32      regAddr,
    IN  GT_U32      data
);

/*******************************************************************************
*   scibGetDeviceId
*
* DESCRIPTION:
*       scans for index of the entry with given hwId
*
* INPUTS:
*       deviceHwId - maximal number of SKernel devices in the Simulation.
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
GT_U32     scibGetDeviceId
(
    IN  GT_U32        deviceHwId
 );

/*******************************************************************************
*   scibAddressCompletionStatusGet
*
* DESCRIPTION:
*       Get address completion  for given hwId
*
* INPUTS:
*       devNum - device ID.
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
GT_BOOL  scibAddressCompletionStatusGet
(
    IN  GT_U32        devNum
);

/* defines to be used for value dataIsWords in APIs of : scibDmaRead,scibDmaWrite*/
#define SCIB_DMA_BYTE_STREAM    0
#define SCIB_DMA_WORDS          1

/* define to be used for value deviceId in APIs of : scibDmaRead,scibDmaWrite
   can use it in:
   1. non-distributed
   2. on application side (when distributed)

   on the Asic side (when distributed) must use the real deviceId of the device
*/
#define SCIB_DMA_ACCESS_DUMMY_DEV_NUM_CNS 0xee

/*******************************************************************************
*  scibDmaRead
*
* DESCRIPTION:
*      read HOST CPU DMA memory function.
*      Asic is calling this function to read DMA.
* INPUTS:
*
*       deviceId    - device id. (of the device in the simulation)
*       address     - physical address that PP refer to.
*                     HOST CPU must convert it to HOST memory address
*       memSize     - number of words of ASIC memory to read .
*       dataIsWords - the data to read is words or bytes
*                     1 - words --> swap network order to cpu order
*                     0 - bytes --> NO swap network order to cpu order
*
* OUTPUTS:
*       memPtr     - (pointer to) PP's memory in which HOST CPU memory will be
*                    copied.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void scibDmaRead
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    OUT GT_U32 * memPtr,
    IN GT_U32  dataIsWords
);

/*******************************************************************************
*  scibDmaWrite
*
* DESCRIPTION:
*      write to HOST CPU DMA memory function.
*      Asic is calling this function to write DMA.
* INPUTS:
*
*       deviceId    - device id. (of the device in the simulation)
*       address     - physical address that PP refer to.
*                     HOST CPU must convert it to HOST memory address
*       memSize     - number of words of ASIC memory to write .
*       memPtr     - (pointer to) data to write to HOST CPU memory.
*       dataIsWords - the data to read is words or bytes
*                     1 - words --> swap network order to cpu order
*                     0 - bytes --> NO swap network order to cpu order
*
* OUTPUTS:
*       none
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void scibDmaWrite
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_U32 * memPtr,
    IN GT_U32  dataIsWords
);

/*******************************************************************************
*  scibRemoteInit
*
* DESCRIPTION:
*      init a device info in Scib , when working in distributed architecture and
*      this is the application side .
*      Asic send message to application side , and on application side this
*      function is called.
* INPUTS:
*    deviceId - ID of device, which is equal to PSS Core API device ID
*    deviceHwId - Physical device Id.
*    interruptLine - interrupt line of the device.
*    isPp - (GT_BOOL) is PP of FA
*    addressCompletionStatus - device enable/disable address completion
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void scibRemoteInit
(
    IN GT_U32  deviceId,
    IN GT_U32  deviceHwId,
    IN GT_U32  interruptLine,
    IN GT_U32  isPp,
    IN GT_U32  addressCompletionStatus
);

/*******************************************************************************
*  scibAccessUnlock
*
* DESCRIPTION:
*       function to protect the accessing to the SCIB layer .
*       the function UN-LOCK the access.
*       The mutex implementations allow reentrant of the locking task.
*
* INPUTS:
*       none.
*
* OUTPUTS:
*       none
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void scibAccessUnlock(void);

/*******************************************************************************
*  scibAccessLock
*
* DESCRIPTION:
*       function to protect the accessing to the SCIB layer .
*       the function LOCK the access.
*       The mutex implementations allow reentrant of the locking task.
*
* INPUTS:
*       none.
*
* OUTPUTS:
*       none
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void scibAccessLock(void);


/*******************************************************************************
*   scibUnBindDevice
*
* DESCRIPTION:
*       unBind the BUS from read/write register functions
*
* INPUTS:
*       deviceId  - ID of device, which is equal to PSS Core API device ID.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*       accessing to the 'read'/'write' registers will cause fatal error.
*******************************************************************************/
void scibUnBindDevice
(
    IN GT_U32               deviceId
);

/*******************************************************************************
*   scibReBindDevice
*
* DESCRIPTION:
*       re-bind the BUS to read/write register functions
*
* INPUTS:
*       deviceId  - ID of device, which is equal to PSS Core API device ID.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void scibReBindDevice
(
    IN GT_U32               deviceId
);

/*******************************************************************************
*   scibPortLoopbackForceModeSet
*
* DESCRIPTION:
*       the function set the 'loopback force mode' on a port of device.
*       this function needed for devices that not support loopback on the ports
*       and need 'external support'
*
* INPUTS:
*       deviceId  - the simulation device Id .
*       portNum   - the physical port number .
*       mode      - the loopback force mode.
*               0 - SKERNEL_PORT_LOOPBACK_NOT_FORCED_E,
*               1 - SKERNEL_PORT_LOOPBACK_FORCE_ENABLE_E,
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK - on success
*       GT_BAD_PARAM     - on bad portNum or mode
*
* COMMENTS:
*       function do fatal error on non-exists device or out of range device.
*
*******************************************************************************/
GT_STATUS scibPortLoopbackForceModeSet
(
    IN  GT_U32                      deviceId,
    IN  GT_U32                      portNum,
    IN  GT_U32                      mode
);

/*******************************************************************************
*   scibMemoryClientRegRead
*
* DESCRIPTION:
*       Generic read of SCIB client registers from SKernel device.
*
* INPUTS:
*       deviceId  - ID of SKErnel device, which is equal to PSS Core API device ID.
*       scibClient - memory access client: PCI/Core/DFX
*       memAddr - address of first word to read.
*       length - number of words to read.
*
* OUTPUTS:
*       dataPtr - pointer to copy read data.
*
* RETURNS:
*       None
*
* COMMENTS:
*       None
*
*******************************************************************************/
void scibMemoryClientRegRead
(
    IN  GT_U32                  deviceId,
    IN  SCIB_MEM_ACCESS_CLIENT  scibClient,
    IN  GT_U32                  memAddr,
    IN  GT_U32                  length,
    OUT GT_U32 *                dataPtr
);

/*******************************************************************************
*   scibMemoryClientRegWrite
*
* DESCRIPTION:
*       Generic write to registers of SKernel device according to SCIB client.
*
* INPUTS:
*       deviceId  - ID of SKernel device, which is equal to PSS Core API device ID.
*       memAddr - address of first word to read.
*       scibClient - memory access client: PCI/Core/DFX
*       length - number of words to read.
*       dataPtr - pointer to copy read data.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*       None
*
*******************************************************************************/
void scibMemoryClientRegWrite
(
    IN  GT_U32                  deviceId,
    IN  SCIB_MEM_ACCESS_CLIENT  scibClient,
    IN  GT_U32                  memAddr,
    IN  GT_U32                  length,
    IN  GT_U32 *                dataPtr
);

/*******************************************************************************
* scibCoreClockRegisterUpdate
*
* DESCRIPTION:
*       Update the value of the Core Clock in the register.
*       there are 2 options:
*       1. give 'coreClockInMHz' which will be translated to proper 'hw field value'
*           and then written to the HW.
*       2. give 'hwFieldValue' which written directly to the HW.
*
* INPUTS:
*       deviceId         - the device id as appear in the INI file
*       coreClockInMHz  - core clock in MHz.
*                           value SMAIN_NOT_VALID_CNS means ignored
*       hwFieldValue -  value to set to the core clock field in the HW.
*                           NOTE: used only when coreClockInMHz == 0xFFFFFFFF.
* OUTPUTS:
*       refCoreClkPtr  - Pp's reference core clock in MHz
*       coreClkPtr     - Pp's core clock in MHz
*
* RETURNS:
*       GT_OK        - on success,
*       GT_BAD_STATE - can't map HW value to core clock value.
*       GT_FAIL      - otherwise.
*
* COMMENTS:
*
*******************************************************************************/
void scibCoreClockRegisterUpdate
(
    IN GT_U32                  deviceId,
    IN GT_U32                  coreClockInMHz,
    IN GT_U32                  hwFieldValue
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __scibh */


