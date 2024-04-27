/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* new_message.h
*
* DESCRIPTION:
*       This file defines the messages format send between the two sides of
*       distributed simulation.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*
*******************************************************************************/
#ifndef _NEW_MESSAGE_H_
#define _NEW_MESSAGE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* check if we can use GT_ types */
#ifdef USE_GT_TYPES
    #define UINT32  GT_U32
#endif /*USE_GT_U32*/


/* define variable type .
all variables sent on messages must has same size */
#define VARIABLE_TYPE       UINT32
#define PLACE_HOLDER_TYPE   VARIABLE_TYPE*

/*
 * Typedef: enum MSG_TYPE
 *
 * Description:
 *      list the different type of messages
 *
 * Fields:
 *      MSG_TYPE_READ  : read register request , sent from Application to Device
 *      MSG_TYPE_WRITE : write register request , sent from Application to Device
 *      MSG_TYPE_READ_RESPONSE : read register response, sent from Device to Application
 *      MSG_TYPE_DMA_READ  : read DMA request , sent from Device to Application
 *      MSG_TYPE_DMA_WRITE : write DMA request , sent from Device to Application
 *      MSG_TYPE_DMA_READ_RESPONSE : read DMA response, sent from Application to Device
 *      MSG_TYPE_NIC_TX_FRAME : send Ethernet frame from CPU's Nic to ingress the Device,
 *                              sent from Application to Device
 *      MSG_TYPE_NIC_RX_FRAME : send Ethernet frame to CPU's Nic to egress the Device'
 *                              sent from Device to Application
 *      MSG_TYPE_INIT_PARAMS : initialization parameters , sent from Device to Application.
 *      MSG_TYPE_INIT_RESET : Reset message from the application to all
 *                            distributed processes in the system
 *      MSG_TYPE_DEBUG_LEVEL_SET : set debug level of the distributed mechanism
 *
 ******the following messages are exchanged only between application(s) and broker
 *
 *      MSG_TYPE_APP_PID - application send to the broker it's
 *                  processId , so broker can signal application on interrupt
 *      MSG_TYPE_REGISTER_DMA - application send to the broker
 *                  info about a DMA shared memory. So broker can read/write from
 *                  this shared memory without the application knowledge.
 *      MSG_TYPE_REGISTER_INTERRUPT - application send to the
 *                  broker request to register itself as one that want to be
 *                  signaled when interrupt arrive.
 *                  signaled when interrupt arrive.
 *      MSG_TYPE_UNMASK_INTERRUPT - application send to the
 *                  broker request that broker will be able to signal
 *                  application on interrupt.
 *                  NOTE : the issue is that once the broker signal the application
 *                  about interrupt, the broker will not signal it again even if
 *                  received another interrupt from device , until application
 *                  will send MSG_TYPE_UNMASK_INTERRUPT
 *                  NOTE : see mode of BROKER_INTERRUPT_MASK_MODE
 *     MSG_TYPE_MASK_INTERRUPT - application send to the
 *                  broker request that broker will NOT be able to signal
 *                  application on interrupt.
 *                  NOTE : the issue is that once the broker signal the application
 *                  about interrupt, the application wants to silent the broker.
 *                  application will 'unmask' the broker when application ready
 *                  for new signal.
 *                  NOTE : see mode of BROKER_INTERRUPT_MASK_MODE
 *
 * Comments:
 *
 */
typedef enum
{
    MSG_TYPE_READ,
    MSG_TYPE_WRITE,
    MSG_TYPE_READ_RESPONSE,
    MSG_TYPE_DMA_READ,
    MSG_TYPE_DMA_WRITE,
    MSG_TYPE_DMA_READ_RESPONSE,
    MSG_TYPE_INTERRUPT_SET,
    MSG_TYPE_NIC_TX_FRAME,
    MSG_TYPE_NIC_RX_FRAME,
    MSG_TYPE_INIT_PARAMS,
    MSG_TYPE_INIT_RESET,
    MSG_TYPE_DEBUG_LEVEL_SET,

    /* the following messages are sent only from application(s) to broker */
    MSG_TYPE_APP_PID = 50,
    MSG_TYPE_REGISTER_DMA,
    MSG_TYPE_REGISTER_INTERRUPT,
    MSG_TYPE_UNMASK_INTERRUPT,
    MSG_TYPE_MASK_INTERRUPT,

    MSG_TYPE_LAST /* must be last */
} MSG_TYPE;

/*
 * Typedef: enum READ_WRITE_ACCESS_TYPE
 *
 * Description:
 *      list the different register accessing
 *
 * Fields:
 *       REGISTER_MEMORY_ACCESS - Device's registers (via SMI/PCI/PEX/TWSI...)
 *       PCI_REGISTERS_ACCESS   - PCI/PEX configuration registers
 *
 * Comments:
 *
 */
typedef enum
{
    REGISTER_MEMORY_ACCESS,
    PCI_REGISTERS_ACCESS
} READ_WRITE_ACCESS_TYPE;

/*
 * Typedef: struct MSG_HDR
 *
 * Description:
 *      Describe the header of the messages send via sockets
 *
 * Fields:
 *      type    : the message type , one of the : MSG_TYPE
 *      msgLen  : length of message excluding Hdr in Bytes
 *
 * Comments:
 */
typedef struct
{
    VARIABLE_TYPE type;
    VARIABLE_TYPE msgLen;
} MSG_HDR;

/*
 * Typedef: struct WRITE_MSG
 *
 * Description:
 *      Describe the "write register" message
 *
 * Fields:
 *       hdr         - the header that all messages start with
 *       deviceId    - device id. (of the device in the simulation)
 *       accessType  - Define access operation Read or Write.
 *                     one of READ_WRITE_ACCESS_TYPE
 *       writeLen    - number of registers to write.
 *       address     - Address of first register to write.
 *       dataPtr     - (place holder) start here to place the data to write
 * Comments:
 *       sent from Application side to Device side.
 */
typedef struct
{
    MSG_HDR hdr;
    VARIABLE_TYPE deviceId;
    VARIABLE_TYPE accessType;
    VARIABLE_TYPE writeLen;
    VARIABLE_TYPE address;
    PLACE_HOLDER_TYPE dataPtr;
} WRITE_MSG;

/*
 * Typedef: struct READ_MSG
 *
 * Description:
 *      Describe the "read register" message
 *
 * Fields:
 *       hdr         - the header that all messages start with
 *       deviceId    - device id. (of the device in the simulation)
 *       accessType  - Define access operation Read or Write.
 *                     one of READ_WRITE_ACCESS_TYPE
 *       readLen     - number of registers to read.
 *       address     - Address of first register to read.
 * Comments:
 *       sent from Application side to Device side.
 *
 *       the reply to this message will be in the format of READ_RESPONSE_MSG
 *       (from Device to application)
 */
typedef struct
{
    MSG_HDR hdr;
    VARIABLE_TYPE deviceId;
    VARIABLE_TYPE accessType;
    VARIABLE_TYPE readLen;
    VARIABLE_TYPE address;
} READ_MSG;

/*
 * Typedef: struct READ_RESPONSE_MSG
 *
 * Description:
 *      Describe the "reply to read register" message
 *
 * Fields:
 *       hdr         - the header that all messages start with
 *       readLen     - number of registers read.
 *       dataPtr     - (place holder) start here to place the data that read
 * Comments:
 *       sent from Device side to Application side.(as response to message
 *       READ_MSG)
 */
typedef struct
{
    MSG_HDR hdr;
    VARIABLE_TYPE readLen;
    PLACE_HOLDER_TYPE dataPtr;
} READ_RESPONSE_MSG;

/*
 * Typedef: struct DMA_WRITE_MSG
 *
 * Description:
 *      Describe the "write DMA" message
 *
 * Fields:
 *       hdr         - the header that all messages start with
 *       writeLen    - number of "words" (4 bytes) to write.
 *       address     - Address of first "word" to write.
 *       dataPtr     - (place holder) start here to place the data to write
 * Comments:
 *       sent from Device side to Application side.
 *
 */
typedef struct
{
    MSG_HDR hdr;
    VARIABLE_TYPE writeLen;
    VARIABLE_TYPE address;
    PLACE_HOLDER_TYPE dataPtr;
} DMA_WRITE_MSG;

/*
 * Typedef: struct DMA_READ_MSG
 *
 * Description:
 *      Describe the "read DMA" message
 *
 * Fields:
 *       hdr         - the header that all messages start with
 *       readLen     - number of "words" (4 bytes) to read.
 *       address     - Address of first "word" to read.
 * Comments:
 *       sent from Device side to Application side.
 *
 *       the reply to this message will be in the format of DMA_READ_RESPONSE_MSG
 *       (from Application to Device)
 */
typedef struct
{
    MSG_HDR hdr;
    VARIABLE_TYPE readLen;
    VARIABLE_TYPE address;
} DMA_READ_MSG;

/*
 * Typedef: struct DMA_READ_RESPONSE_MSG
 *
 * Description:
 *      Describe the "reply to Read DMA" message
 *
 * Fields:
 *       hdr         - the header that all messages start with
 *       readLen     - number of "words" (4 bytes) read.
 *       dataPtr     - (place holder) start here to place the data that read
 * Comments:
 *       sent from Application side to Device side.(as response to message
 *       DMA_READ_MSG)
 */
typedef struct
{
    MSG_HDR hdr;
    VARIABLE_TYPE readLen;
    PLACE_HOLDER_TYPE dataPtr;
} DMA_READ_RESPONSE_MSG;

/*
 * Typedef: struct INTERRUPT_SET_MSG
 *
 * Description:
 *      Describe the "set interrupt" message
 *
 * Fields:
 *       hdr         - the header that all messages start with
 *       deviceId    - device id. (of the device in the simulation)
 * Comments:
 *      Sent from Device Side to Application side
 */
typedef struct
{
    MSG_HDR hdr;
    VARIABLE_TYPE  deviceId;
} INTERRUPT_SET_MSG;

/*
 * Typedef: struct INIT_PARAM_SINGLE_DEVICE_INFO
 *
 * Description:
 *      Describe the Device init parameters (per device)
 *
 * Fields:
 *       hdr          - the header that all messages start with
 *       deviceId     - device id. (of the device in the simulation)
 *       deviceHwId   - Physical device Id.
 *       interruptLine - interrupt line of the device.
 *       isPp          - 1 is PP , 0 - is FA
 *       addressCompletionStatus - device enable/disable address completion
 *       nicDevice - is nic device 0 - no NIC , 1 - NIC
 * Comments:
 *
 */
typedef struct
{
    VARIABLE_TYPE  deviceId;
    VARIABLE_TYPE  deviceHwId;
    VARIABLE_TYPE  interruptLine;
    VARIABLE_TYPE  isPp;
    VARIABLE_TYPE  addressCompletionStatus;
    VARIABLE_TYPE  nicDevice;
}INIT_PARAM_SINGLE_DEVICE_INFO;

/*
 * Typedef: struct INIT_PARAM_MSG
 *
 * Description:
 *      Describe the header of the initialization parameters of a device.
 *      message sent from Device to Application
 *
 * Fields:
 *       hdr          - the header that all messages start with
 *       deviceInfo   - the info about the device
 *
 * Comments:
 *
 */
typedef struct
{
    MSG_HDR hdr;
    INIT_PARAM_SINGLE_DEVICE_INFO deviceInfo;
} INIT_PARAM_MSG;

/*
 * Typedef: struct RESET_PARAM_MSG
 *
 * Description:
 *      Describe the header of the Reset message.
 *      message sent from Application to all distributed part of system
 *
 * Fields:
 *       hdr          - the header that all messages start with
 *       deviceInfo   - the info about the device
 *
 * Comments:
 *
 */
typedef struct
{
    MSG_HDR hdr;
} RESET_PARAM_MSG;

/*
 * Typedef: enumeration TARGET_OF_MESSAGE_ENT
 *
 * Description:
 *      define all types of target parts in the system
 *
 * Fields:
 *       TARGET_OF_MESSAGE_APPLICATION_E - application part
 *       TARGET_OF_MESSAGE_BROKER_E - broker part
 *       TARGET_OF_MESSAGE_BUS_E - bus part
 *       TARGET_OF_MESSAGE_DEVICE_E - device part
 *       TARGET_OF_MESSAGE_ALL_E - all parts
 *
 * Comments:
 *
 */
typedef enum{
    TARGET_OF_MESSAGE_APPLICATION_E,
    TARGET_OF_MESSAGE_BROKER_E,
    TARGET_OF_MESSAGE_BUS_E,
    TARGET_OF_MESSAGE_DEVICE_E,
    TARGET_OF_MESSAGE_ALL_E,

}TARGET_OF_MESSAGE_ENT;

/* value for the field of secondaryTarget that indicate ALL secondary targets */
#define ALL_SECONDARY_TARGETS   0xffffffff

/*
 * Typedef: struct DEBUG_LEVEL_MSG
 *
 * Description:
 *      Describe the "debug level " message : application send to the
 *      message sent from Application to all distributed part of system
 *
 * Fields:
 *       hdr           - the header that all messages start with
 *       mainTarget        - the target of the message (broker/bus/device(s)/all)
 *                       on of TARGET_OF_MESSAGE_ENT
 *       secondaryTarget - the more specific target with in the main target (mainTarget)
 *                       may be 0..0xfffffffe for specific ID of the secondary target
 *                       may be ALL_SECONDARY_TARGETS means 'all' secondary targets
 *                       for example when there are several devices processes,
 *                       we can distinguish between devices by the board part
 *                       (secondaryTarget) or set secondaryTarget to
 *                       ALL_SECONDARY_TARGETS to apply to all board parts
 *       debugLevel      - the debug level bmp
 *
 * Comments:
 *
 */
typedef struct
{
    MSG_HDR         hdr;
    VARIABLE_TYPE   mainTarget;
    VARIABLE_TYPE   secondaryTarget;
    VARIABLE_TYPE   debugLevel;
}DEBUG_LEVEL_MSG;

/*
 * Typedef: struct APP_PID_MSG
 *
 * Description:
 *      Describe the "application process ID" message : application send to the
 *       broker it's processId , so broker can signal application on interrupt
 *
 * Fields:
 *       hdr         - the header that all messages start with
 *       processId   - application's process Id
 * Comments:
 *      Sent from Application Side to broker side
 */
typedef struct
{
    MSG_HDR hdr;
    VARIABLE_TYPE processId;
}APP_PID_MSG;

/*
 * Typedef: enum BROKER_DMA_MODE
 *
 * Description:
 *      Describe the "DMA mode" : how broker handle read/write DMA messages
 *
 * Fields:
 *       BROKER_DMA_MODE_SOCKET - the broker will send the message to the application
 *       BROKER_DMA_MODE_SHARED_MEMORY - the broker will do read/write DMA from
 *              the shared memory
 *
 * Comments:
 *      Sent from Application Side to broker side
 *
 */
typedef enum{
    BROKER_DMA_MODE_SOCKET,
    BROKER_DMA_MODE_SHARED_MEMORY
}BROKER_DMA_MODE;

/*
 * Typedef: struct REGISTER_DMA_MSG
 *
 * Description:
 *      Describe the "register DMA" message : application send to the
 *          broker info about a DMA shared memory. So broker can read/write from
 *          this shared memory without the application knowledge.
 *
 * Fields:
 *       hdr           - the header that all messages start with
 *       startAddress  - the start address of the DMA memory
 *       size          - number of bytes the DMA uses
 *       key           - the key that represents the DMA memory (shared memory)
 *       dmaMode       - the DMA mode : broker send message via socket to
 *                          application or broker do read/write DMA from shared
 *                          memory (one of BROKER_DMA_MODE)
 *
 * Comments:
 *      Sent from Application Side to broker side
 */
typedef struct
{
    MSG_HDR hdr;
    VARIABLE_TYPE startAddress;
    VARIABLE_TYPE size;
    VARIABLE_TYPE key;
    VARIABLE_TYPE dmaMode;
}REGISTER_DMA_MSG;

/*
 * Typedef: enum BROKER_INTERRUPT_MASK_MODE
 *
 * Description:
 *      Describe the "Interrupt mask mode" : how broker handle interrupt messages
 *
 * Fields:
 *       INTERRUPT_MODE_BROKER_AUTOMATICALLY_MASK_INTERRUPT_LINE - the broker
 *              will signal application on interrupt , and then will state itself
 *              into 'Masked state' , meaning that it can't signal the application
 *              until receiving 'Unmask interrupt' message
 *       INTERRUPT_MODE_BROKER_USE_MASK_INTERRUPT_LINE_MSG - the broker
 *              will signal application on interrupt , the broker will NOT state
 *              itself into 'Masked state'
 *              in order for the broker to be in 'Masked state' , application
 *              need to send message 'Maks interrupt'
 *
 * Comments:
 *      Sent from Application Side to broker side
 *
 */
typedef enum{
    INTERRUPT_MODE_BROKER_AUTOMATICALLY_MASK_INTERRUPT_LINE,
    INTERRUPT_MODE_BROKER_USE_MASK_INTERRUPT_LINE_MSG
}BROKER_INTERRUPT_MASK_MODE;

/* value to treat all interrupt lines regardless of their value */
#define ALL_INTERRUPT_LINES_ID      0xffeeddcc

/*
 * Typedef: struct REGISTER_INTERRUPT_MSG
 *
 * Description:
 *      Describe the "register interrupt" message : application send to the
 *                  broker request to register itself as one that want to be
 *                  signaled when interrupt arrive.
 *
 * Fields:
 *       hdr           - the header that all messages start with
 *       interruptLineId - interrupt line ID that when device triggers it ,
 *                       the broker will signal the application with the signalId
 *                       may use value ALL_INTERRUPT_LINES_ID
 *       signalId      - the signal ID to send on 'Interrupt set'
 *       maskMode      - broker interrupt mask mode , one of BROKER_INTERRUPT_MASK_MODE
 * Comments:
 *      Sent from Application Side to broker side
 *
 */
typedef struct
{
    MSG_HDR hdr;
    VARIABLE_TYPE   interruptLineId;
    VARIABLE_TYPE   signalId;
    VARIABLE_TYPE   maskMode;
}REGISTER_INTERRUPT_MSG;

/*
 * Typedef: struct UNMASK_INTERRUPT_MSG
 *
 * Description:
 *      Describe the "unmask interrupt" message : application send to the
 *             broker request that broker will be able to signal
 *             application on interrupt.
 *             NOTE : the issue is that once the broker signal the application
 *             about interrupt, the broker will not signal it again even if
 *             received another interrupt from device , until application
 *             will send MSG_TYPE_UNMASK_INTERRUPT
 *          -- see modes of BROKER_INTERRUPT_MASK_MODE --
 * Fields:
 *       hdr           - the header that all messages start with
 *       interruptLineId - interrupt line ID that when device triggers it ,
 *                       the broker will signal the application with the signalId
 *                       may use value ALL_INTERRUPT_LINES_ID
 * Comments:
 *      Sent from Application Side to broker side
 *
 */
typedef struct
{
    MSG_HDR         hdr;
    VARIABLE_TYPE   interruptLineId;
}UNMASK_INTERRUPT_MSG;

/*
 * Typedef: struct MASK_INTERRUPT_MSG
 *
 * Description:
 *      Describe the "mask interrupt" message : application send to the
 *             broker request that broker will NOT be able to signal
 *             application on interrupt.
 *             NOTE : see modes of BROKER_INTERRUPT_MASK_MODE
 *
 * Fields:
 *       hdr           - the header that all messages start with
 *       interruptLineId - interrupt line ID that when device triggers it ,
 *                       the broker will NOT signal the application with the
 *                       signalId
 *                       may use value ALL_INTERRUPT_LINES_ID
 * Comments:
 *      Sent from Application Side to broker side
 *
 */
typedef struct
{
    MSG_HDR         hdr;
    VARIABLE_TYPE   interruptLineId;
}MASK_INTERRUPT_MSG;



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _NEW_MESSAGE_H_ */

