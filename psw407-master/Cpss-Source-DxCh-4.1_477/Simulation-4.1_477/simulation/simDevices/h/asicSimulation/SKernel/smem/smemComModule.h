/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemComModule.h
*
* DESCRIPTION:
*       Data definitions for Communication Module memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 4$
*
*******************************************************************************/
#ifndef __smemComModuleh
#define __smemComModuleh

#include <asicSimulation/SKernel/smem/smem.h>
#include <comModule/libsub.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 *  Typedef: enum SNET_BUS_INTER_TYPE_ENT
 *
 *  Description: Defines CPU bus interface type
 *
 */
typedef enum {
    SNET_BUS_SMI_E = 0,
    SNET_BUS_TWSI_E,
    SNET_BUS_LAST_E,
}SNET_BUS_INTER_TYPE_ENT;

/**
 *  Typedef: struct SMEM_CM_INSTANCE_PARAM_STC
 *
 *      deviceCmInterType   : Communication Module interface type
 *      deviceCmInterfaceId : Communication Module Interface base address
 *      cmActivePtr         : Active CM adapter pointers
*/
typedef struct SMEM_CM_INSTANCE_PARAM_STCT{
    SNET_BUS_INTER_TYPE_ENT deviceCmInterType;
    GT_U32 deviceCmInterfaceId[SNET_BUS_LAST_E];
    GT_VOID * cmActivePtr;
} SMEM_CM_INSTANCE_PARAM_STC;

#define SIM_MAX_SLAVE_DEV   128
/**
 *  Typedef: struct SMEM_SUB20_INSTANCE_PARAM_STC
 *
 *      adapter             - SUB20 adapter instance
 *      handle              - Handle to SUB20 adapter
 *      adapterIdStr        - SUB20 adapter ID
 *      i2cSlaveAddessArr   - Slave I2C address array
*/
typedef struct SMEM_SUB20_INSTANCE_PARAM_STCT{
    sub_device  adapter;
    sub_handle  handle;
    char adapterIdStr[256];
    char i2cSlaveAddessArr[SIM_MAX_SLAVE_DEV];
} SMEM_SUB20_INSTANCE_PARAM_STC;

/* Adapter's frequency */
#define SMEM_CM_FREQUENCY_CNS               0
/* Slow/fast mode */
#define SMEM_CM_MODE_SLOW_CNS               0

/* Set Communication Module interface type */
#define SMEM_CM_INST_INTERFACE_TYPE_SET_MAC(dev, type) \
    (((dev)->cmMemParamPtr)->deviceCmInterType = (type))

/* Get Communication Module interface type */
#define SMEM_CM_INST_INTERFACE_TYPE_GET_MAC(dev) \
    (((dev)->cmMemParamPtr)->deviceCmInterType)

/* Set Communication Module interface ID */
#define SMEM_CM_INST_INTERFACE_ID_SET_MAC(dev, interface, value) \
        (((dev)->cmMemParamPtr)->deviceCmInterfaceId[interface] = (value));

/* Get device interface ID */
#define SMEM_CM_INST_INTERFACE_ID_GET_MAC(dev, interface) \
        (((dev)->cmMemParamPtr)->deviceCmInterfaceId[interface])

/* Get CM Adapter ID  */
#define SMEM_CM_INST_ADAPTER_ID_GET_MAC(dev) \
        (((AdpDesc *)(((dev)->cmMemParamPtr)->cmActivePtr))->_AdapterId)

/* Get SUB20 handle */
#define SMEM_SUB20_HANDLE_GET_MAC(dev) \
        (((dev)->sub20InfoPtr)->handle)

/* Get SUB20 adapter ID */
#define SMEM_SUB20_ADAPTER_ID_GET_MAC(dev) \
        (((dev)->sub20InfoPtr)->adapterIdStr)

/* Specific for DX */
#define  SMEM_CM_SMI_WRITE_ADDRESS_MSB_REGISTER   (0x00)
#define  SMEM_CM_SMI_WRITE_ADDRESS_LSB_REGISTER   (0x01)
#define  SMEM_CM_SMI_WRITE_DATA_MSB_REGISTER      (0x02)
#define  SMEM_CM_SMI_WRITE_DATA_LSB_REGISTER      (0x03)

#define  SMEM_CM_SMI_READ_ADDRESS_MSB_REGISTER    (0x04)
#define  SMEM_CM_SMI_READ_ADDRESS_LSB_REGISTER    (0x05)
#define  SMEM_CM_SMI_READ_DATA_MSB_REGISTER       (0x06)
#define  SMEM_CM_SMI_READ_DATA_LSB_REGISTER       (0x07)

#define  SMEM_CM_SMI_STATUS_REGISTER              (0x1f)

#define SMEM_CM_SMI_STATUS_WRITE_DONE             (0x02)
#define SMEM_CM_SMI_STATUS_READ_READY             (0x01)

#define SMEM_CM_SMI_TIMEOUT_COUNTER               (10000)

/*******************************************************************************
*   smemComModuleInit
*
* DESCRIPTION:
*       Init memory module for a Communication Module device.
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
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
);

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
);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smemComModuleh */


