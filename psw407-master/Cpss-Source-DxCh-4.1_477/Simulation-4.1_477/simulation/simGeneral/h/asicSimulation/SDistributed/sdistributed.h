/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sdistributed.h
*
* DESCRIPTION:
*       This module is the distribution manager of simulation.
*
*
* FILE REVISION NUMBER:
*       $Revision: 4 $
*
*******************************************************************************/
#ifndef __sdistributedh
#define __sdistributedh

#include <os/simTypes.h>
#include <asicSimulation/SCIB/scib.h>
/* next needed for H file <asicSimulation/SDistributed/new_message.h> */
#define USE_GT_TYPES
#include <asicSimulation/SDistributed/new_message.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*******************************************************************/
/* next relevant only to broker and to application connected to it */
/*******************************************************************/
#define BROKER_USE_SOCKET       1
#define BROKER_NOT_USE_SOCKET   2
/* simulation flag indicating that we need to treat DMA memory as shared memory */
extern GT_U32   brokerDmaMode;
/* simulation flag indicating that we need to treat interrupt with signal instead send to socket */
extern GT_U32   brokerInterruptMode;

/* the mode of interrupt mask for application that connected to broker. */
extern BROKER_INTERRUPT_MASK_MODE   sdistAppViaBrokerInterruptMaskMode;
/****************************************************/
/* end of broker and to application connected to it */
/****************************************************/


/*
 * Typedef: struct SIM_DISTRIBUTED_INIT_DEVICE_STC
 *
 * Description:
 *      Describe the parameters for the initialization of device
 *
 * Fields:
 *       numOfDevices - number of devices that sent on this message
 *       deviceInfo - the array of devices with there info:
 *             deviceId - ID of device, which is equal to PSS Core API device ID
 *             deviceHwId - Physical device Id.
 *             interruptLine - interrupt line of the device.
 *             isPp - (GT_BOOL) is PP of FA
 *       addressCompletionStatus - device enable/disable address completion
 *       nicDevice - is nic device 0 - no NIC , 1 - NIC
 *
 *
 * Comments:
 *
 */
typedef struct{
    GT_U32  deviceId;
    GT_U32  deviceHwId;
    GT_U32  interruptLine;
    GT_U32  isPp;
    GT_U32  addressCompletionStatus;
    GT_U32  nicDevice;
}SIM_DISTRIBUTED_INIT_DEVICE_STC;


/*
 * Typedef: enum DEBUG_SIM_DISTRIBUTED_PERFORMANCE_CHECK_ENT
 *
 * Description:
 *      list the different type of performance tests. --- debug utility
 *
 * Fields:
 *      DEBUG_SIM_DISTRIBUTED_PERFORMANCE_CHECK_REGISTER_READ_E  :
 *          test the performance of read register from application to asic.
 *      DEBUG_SIM_DISTRIBUTED_PERFORMANCE_CHECK_REGISTER_WRITE_E :
 *          test the performance of write register from application to asic.
 *      DEBUG_SIM_DISTRIBUTED_PERFORMANCE_CHECK_REGISTER_WRITE_AND_READ_E :
 *          test the performance of write register from application to asic.
 *          then read the register that was just written, and check that read
 *          value match the write value
 *
 *
 *
 * Comments:
 */
typedef enum{
    DEBUG_SIM_DISTRIBUTED_PERFORMANCE_CHECK_REGISTER_READ_E,
    DEBUG_SIM_DISTRIBUTED_PERFORMANCE_CHECK_REGISTER_WRITE_E,
    DEBUG_SIM_DISTRIBUTED_PERFORMANCE_CHECK_REGISTER_WRITE_AND_READ_E,
}DEBUG_SIM_DISTRIBUTED_PERFORMANCE_CHECK_ENT;

/*
 * Typedef: enum DEBUG_SIM_DISTRIBUTED_TRACE_ACTIONS_ENT
 *
 * Description:
 *      list the different type of TRACE actions. --- debug utility
 *
 * Fields:
 *      DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_ADD_E  :
 *          action is to add a flag to TRACE
 *      DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_REMOVE_E :
 *          action is to remove a flag from TRACE
 *      DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_ADD_ALL_E :
 *          action is to add all flags to TRACE
 *      DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_REMOVE_ALL_E:
 *          action is to remove all flags from TRACE
 *
 * Comments:
 */
typedef enum{
    DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_ADD_E,
    DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_REMOVE_E,
    DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_ADD_ALL_E,
    DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_REMOVE_ALL_E,
}DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_ENT;

/*
 * Typedef: enum DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_ENT
 *
 * Description:
 *      list the different type of TRACE flags. --- debug utility
 *
 * Fields:
 *      DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_REGISTER_E  :
 *          trace flag for read/write registers actions
 *      DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_DMA_E :
 *          trace flag for read/write dma actions
 *      DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_INTERRUPT_E :
 *          trace flag for interrupt set action
 *      DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_NIC_RX_E:
 *          trace flag for nic rx frame send action
 *      DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_NIC_TX_E:
 *          trace flag for nic tx frame send action
 *      DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_SEQUENCE_NUM_E:
 *          trace flag for sequence number
 *      DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_CONNECTION_INIT_E:
 *          trace flag for connection init action
 *      DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_REMOTE_INIT_E:
 *          trace flag for remote init action
 *      DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_SOCKET_ID_E:
 *          trace flag for socketId on which we send/receive
 *      DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_DATA_READ_WRITE_E:
 *          trace flag for the data the read/write register/dma did
 *
 *
 *
 * Comments:
 */
typedef enum{
    DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_REGISTER_E,
    DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_DMA_E,
    DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_INTERRUPT_E,
    DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_NIC_RX_E,
    DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_NIC_TX_E,
    DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_SEQUENCE_NUM_E,
    DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_CONNECTION_INIT_E,
    DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_REMOTE_INIT_E,
    DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_SOCKET_ID_E,
    DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_DATA_READ_WRITE_E,

}DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_ENT;

/*******************************************************************************
*   simDistributedInit
*
* DESCRIPTION:
*       Init the simulation distribution functionality.
*       initialization is done according to global parameter sasicgSimulationRole
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedInit
(
    void
);

/*******************************************************************************
*   simDistributedExit
*
* DESCRIPTION:
*       Exit (Kill) the simulation distribution functionality.
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedExit
(
    void
);

/*******************************************************************************
*  simDistributedRemoteInit
*
* DESCRIPTION:
*      do remote Asic init function. --> direction is asic to application
*      function should be called only on the Asic side
* INPUTS:
*       numOfDevices - number of device to initialize
*       devicesArray  - devices array , and info.
* OUTPUTS:
*       none
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedRemoteInit
(
    IN GT_U32   numOfDevices,
    IN SIM_DISTRIBUTED_INIT_DEVICE_STC *devicesArray
);

/*******************************************************************************
*  simDistributedRegisterRead
*
* DESCRIPTION:
*      Read Skernel memory function. --> direction is cpu from asic
*      function should be called only on the cpu side
* INPUTS:
*       accessType  - Define access operation Read or Write.
*       deviceId    - device id.
*       address     - Address for ASIC memory.
*       memSize     - Size of ASIC memory to read or write.
*
* OUTPUTS:
*       memPtr     - pointer to application memory in which
*                     ASIC memory will be copied.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedRegisterRead
(
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    OUT GT_U32 * memPtr
);

/*******************************************************************************
*  simDistributedRegisterWrite
*
* DESCRIPTION:
*      write Skernel memory function. --> direction is cpu to asic
*      function should be called only on the cpu side
* INPUTS:
*       accessType  - Define access operation Read or Write.
*       deviceId    - device id.
*       address     - Address for ASIC memory.
*       memSize     - Size of ASIC memory to read or write.
*       memPtr      - For Write this pointer to application memory,which
*                     will be copied to the ASIC memory .
*
* OUTPUTS:
*       none
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedRegisterWrite
(
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_U32 * memPtr
);


/*******************************************************************************
*  simDistributedDmaRead
*
* DESCRIPTION:
*      Read HOST CPU DMA memory function. --> direction is asic from cpu
*      function should be called only on the asic side
* INPUTS:
*       deviceId    - device id. (of the device in the simulation)
*       address     - Address in HOST DMA memory.
*       memSize     - Size of DMA memory to read .
*       dataIsWords - the data to read is words or bytes
*                     1 - words --> swap network order to cpu order
*                     0 - bytes --> NO swap network order to cpu order
* OUTPUTS:
*       memPtr     - pointer to application memory in which
*                     ASIC memory will be copied.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedDmaRead
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    OUT GT_U32 * memPtr,
    IN GT_U32  dataIsWords
);

/*******************************************************************************
*  simDistributedDmaWrite
*
* DESCRIPTION:
*      write HOST CPU DMA memory function. --> direction is asic to cpu
*      function should be called only on the asic side
* INPUTS:
*       deviceId    - device id. (of the device in the simulation)
*       address     - Address in HOST DMA memory.
*       memSize     - Size of ASIC memory to read or write.
*       memPtr      - For Write this pointer to application memory,which
*                     will be copied to the ASIC memory .
*       dataIsWords - the data to read is words or bytes
*                     1 - words --> swap network order to cpu order
*                     0 - bytes --> NO swap network order to cpu order
*
* OUTPUTS:
*       none
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedDmaWrite
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    INOUT GT_U32 * memPtr,
    IN GT_U32  dataIsWords
);


/*******************************************************************************
* simDistributedNicTxFrame
*
* DESCRIPTION:
*       This function transmits an Ethernet packet from the NIC of CPU.
*       function should be called only on the cpu side
*
* INPUTS:
*       frameLength  - length of frame.
*       framePtr  - pointer the frame (array of bytes).
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
void simDistributedNicTxFrame
(
    IN GT_U32       frameLength,
    IN GT_U8        *framePtr
);

/*******************************************************************************
* simDistributedNicRxFrame
*
* DESCRIPTION:
*       This function transmit an Ethernet packet to the NIC of CPU.
*       function should be called only on the asic side
*
* INPUTS:
*       frameLength  - length of frame.
*       framePtr  - pointer the frame (array of bytes).
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       none
*
* COMMENTS:
*       None.
*
*******************************************************************************/
void simDistributedNicRxFrame
(
    IN GT_U32       frameLength,
    IN GT_U8        *framePtr
);

/*******************************************************************************
*  simDistributedInterruptSet
*
* DESCRIPTION:
*      Set interrupt line for a device function. --> direction is cpu from asic
*      function should be called only on the application side
* INPUTS:
*       deviceId    - the device id that set the interrupt
*
* OUTPUTS:
*       none
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedInterruptSet
(
    IN  GT_U32        deviceId
);

/*******************************************************************************
*  simDistributedRegisterDma
*
* DESCRIPTION:
*      register DMA info to broker --> direction is application to broker
* INPUTS:
*       startAddress  - DMA start address
*       size - size of DMA in bytes
*       key - the key of shared memory that represent the DMA
*       dmaMode - the DMA mode (socket/shared memory)
*
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedRegisterDma
(
    IN GT_U32  startAddress,
    IN GT_U32  size,
    IN GT_SH_MEM_KEY  key,
    IN BROKER_DMA_MODE dmaMode
);

/*******************************************************************************
*  simDistributedRegisterInterrupt
*
* DESCRIPTION:
*      register Interrupt info to broker --> direction is application to broker
* INPUTS:
*       interruptLineId - interrupt line ID that when device triggers it ,
*                       the broker will signal the application with the signalId
*       signalId      - the signal ID to send on 'Interrupt set'
*       mode          - broker interrupt mode , one of BROKER_INTERRUPT_MASK_MODE
*
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedRegisterInterrupt
(
    IN GT_U32  interruptLineId,
    IN GT_U32  signalId,
    IN BROKER_INTERRUPT_MASK_MODE  maskMode
);

/*******************************************************************************
*  simDistributedInterruptUnmask
*
* DESCRIPTION:
*      send "unmask interrupt" message : application send to the
*             broker request that broker will be able to signal
*             application on interrupt.
*             NOTE : the issue is that once the broker signal the application
*             about interrupt, the broker will not signal it again even if
*             received another interrupt from device , until application
*             will send MSG_TYPE_UNMASK_INTERRUPT
*          -- see modes of BROKER_INTERRUPT_MASK_MODE --
* INPUTS:
*       interruptLineId - interrupt line ID that when device triggers it ,
*                       the broker will signal the application with the signalId
*                       may use value ALL_INTERRUPT_LINES_ID
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedInterruptUnmask
(
    IN GT_U32 interruptLineId
);

/*******************************************************************************
*  simDistributedInterruptMask
*
* DESCRIPTION:
*      send "mask interrupt" message : application send to the
*             broker request that broker will NOT be able to signal
*             application on interrupt.
*             NOTE : see modes of BROKER_INTERRUPT_MASK_MODE
* INPUTS:
*       interruptLineId - interrupt line ID that when device triggers it ,
*                       the broker will NOT signal the application with the
*                       signalId
*                       may use value ALL_INTERRUPT_LINES_ID
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void simDistributedInterruptMask
(
    IN GT_U32 interruptLineId
);


/*******************************************************************************
* debugSimDistributedPerformanceCheck
*
* DESCRIPTION:
*       debug function to test performances . debug utility
*
*       APPLICABLE SIDE : only on Application side
*
* INPUTS:
*       type        - one of DEBUG_SIM_DISTRIBUTED_PERFORMANCE_CHECK_ENT
*       loopSize    - number of loops to read/write
*       address     - the address of the register to read/write
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK - check was done , successfully
*       GT_BAD_STATE - function called not on application side
*       GT_GET_ERROR - read after write failed , the read value was different
*                      then the write value
*       GT_BAD_PARAM - bad type value
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS debugSimDistributedPerformanceCheck
(
    IN GT_U32   type,
    IN GT_U32   loopSize,
    IN GT_U32   address
);

/*******************************************************************************
* debugSimDistributedTraceSet
*
* DESCRIPTION:
*       debug function to set the flags of the trace printings .  debug utility
*
*       APPLICABLE SIDE : both sides
*
* INPUTS:
*       actionType - the action type : one of DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_ENT
*       flag    - the trace flag to set : one of DEBUG_SIM_DISTRIBUTED_TRACE_FLAG_ENT
*                       relevant when actionType is:
*                           DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_ADD_E or
*                           DEBUG_SIM_DISTRIBUTED_TRACE_ACTION_REMOVE_E
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK - on success
*       GT_BAD_PARAM - bad actionType or flag
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS debugSimDistributedTraceSet
(
    IN GT_U32   actionType,
    IN GT_U32   flag
);

/*******************************************************************************
* debugSimDistributedSet
*
* DESCRIPTION:
*       debug function to set/unset the option to have debug printings . debug utility
*
*       APPLICABLE SIDE : both sides
*
* INPUTS:
*       usedDebug - 0 - don't use debug printings (trace)
*                   otherwise - use debug printings (trace)
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK - on success
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS debugSimDistributedSet
(
    IN GT_U32   usedDebug
);

/*******************************************************************************
*  simDistributedRemoteDebugLevelSend
*
* DESCRIPTION:
*      application send to single/all the distributed part(s) on the system
*      the 'Debug level set' message
* INPUTS:
*      mainTarget        - the target of the message (broker/bus/device(s)/all)
*                      on of TARGET_OF_MESSAGE_ENT
*      secondaryTarget - the more specific target with in the main target (mainTarget)
*                      may be 0..0xfffffffe for specific ID of the secondary target
*                      may be ALL_SECONDARY_TARGETS means 'all' secondary targets
*                      for example when there are several devices processes,
*                      we can distinguish between devices by the board part
*                      (secondaryTarget) or set secondaryTarget to
*                      ALL_SECONDARY_TARGETS to apply to all board parts
*      debugLevel      - the debug level bmp
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK - on success
*       GT_BAD_PARAM - when parameter not valid to this part of simulation
*
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS simDistributedRemoteDebugLevelSend
(
    IN TARGET_OF_MESSAGE_ENT  mainTarget,
    IN GT_U32                 secondaryTarget,
    IN GT_U32                 debugLevel
);

/*******************************************************************************
*  simDistributedRemoteDebugLevelSendAll
*
* DESCRIPTION:
*      application send to all the distributed parts on the system
*      the FULL bmp of 'Debug level set' message
* INPUTS:
*      doDebug        - open or close debug level
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK - on success
*       GT_BAD_PARAM - when parameter not valid to this part of simulation
*
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS simDistributedRemoteDebugLevelSendAll
(
    IN GT_U32   doDebug
);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sdistributedh */


