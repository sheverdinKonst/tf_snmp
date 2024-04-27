/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* scib.c
*
* DESCRIPTION:
*       The module is buffer management utility for SKernel modules.
*
*
* FILE REVISION NUMBER:
*       $Revision: 26 $
*******************************************************************************/
#include <os/simTypesBind.h>
#include <asicSimulation/SCIB/scib.h>
#include <asicSimulation/SInit/sinit.h>
#include <asicSimulation/SDistributed/sdistributed.h>

static GT_U32   _non_dest = 0;
static GT_U32   _broker = 0;
static GT_U32   _bus = 0;
static GT_U32   _app = 0;
static GT_U32   _dev = 0;


extern GT_STATUS skernelPortLoopbackForceModeSet
(
    IN  GT_U32                      deviceId,
    IN  GT_U32                      portNum,
    IN GT_U32   /*SKERNEL_PORT_LOOPBACK_FORCE_MODE_ENT*/ mode
);

extern GT_STATUS skernelPortLinkStateSet
(
    IN  GT_U32                      deviceId,
    IN  GT_U32                      portNum,
    IN GT_BOOL                      linkState
);

extern void smemCheetahUpdateCoreClockRegister
(
    IN void*  /*SKERNEL_DEVICE_OBJECT* */    deviceObjPtr,
    IN GT_U32                  coreClockInMHz,
    IN GT_U32                  hwFieldValue
);

/*******************************************************************************
* Private type definition
*******************************************************************************/
/*
 * Typedef: struct SCIB_DEVICE_DB_STC
 *
 * Description:
 *      Describe a SCIB database entry (entry per device).
 *
 * Fields:
 *      valid          : the info is valid (about this device)
 *      deviceObj      : Pointer to opaque deviceObj, is used as parameter
 *                     : for memAccesFun calls.
 *      memAccesFun    : Entry point for R/W SKernel memory function.
 *      intLine        : Interrupt line number.
 *      deviceHwId     : Physical device Id.
 *      addressCompletionStatus :  device enable/disable address completion
 *
 *      origMemAccesFun : saved copy of the memAccesFun used for 'rebind' action
 *                      see function scibReBindDevice
 * Comments:
 */
typedef struct{
    GT_BOOL                 valid;
    void                *   deviceObj;
    SCIB_RW_MEMORY_FUN      memAccesFun;
    GT_U32                  intLine;
    GT_U32                  deviceHwId;
    GT_BOOL                 addressCompletionStatus;

    SCIB_RW_MEMORY_FUN      origMemAccesFun;
}SCIB_DEVICE_DB_STC;

static SCIB_DEVICE_DB_STC      *   scibDbPtr;
static SCIB_DEVICE_DB_STC      *   scibFaDbPtr;
static GT_U32               maxDevicesPerSystem;

#define FA_ADDRESS_BIT_CNS  (1 << 30)

#define DEV_NUM_CHECK_MAC(deviceId,funcNam)  \
    if (deviceId >= maxDevicesPerSystem)    \
        skernelFatalError(" %s : device id [%d] out of boundary \n",#funcNam,deviceId)

/* Maximal devices number */
#define SOHO_SMI_DEV_TOTAL          (32)
/* Start address of global register.       */
#define SOHO_GLOBAL_REGS_START_ADDR (0x1b)
/* Start address of ports related register.*/
#define SOHO_PORT_REGS_START_ADDR   (0x10)

extern int osPrintf(const char* format, ...);
/* debug printings */
#define SCIB_DEBUG_PRINT_MAC(x)     if(debugPrint_enable) osPrintf x


/* debug printings */
#define SCIB_DEBUG_PRINT_WRITE_MAC(deviceId,memAddr,length,dataPtr)        \
    {                                                                      \
        GT_U32  index;                                                     \
        SCIB_DEBUG_PRINT_MAC(("write[%d][%8.8x][%8.8x]" ,deviceId,memAddr,dataPtr[0]));\
                                                                           \
        for(index = 1 ;index < length; index++)                            \
        {                                                                  \
            SCIB_DEBUG_PRINT_MAC(("[%8.8x]" ,deviceId,memAddr,dataPtr[index]));\
        }                                                                  \
        SCIB_DEBUG_PRINT_MAC(("\n"));                                      \
    }



static GT_BOOL debugPrint_enable = GT_FALSE;

/*******************************************************************************
*  DMA_READ_FUN
*
* DESCRIPTION:
*      read HOST CPU DMA memory function.
* INPUTS:
*
*       deviceId    - device id. (of the device in the simulation)
*       address     - physical address that PP refer to.
*                     HOST CPU must convert it to HOST memory address
*       memSize     - Size of ASIC memory to read or write.
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
typedef void (* DMA_READ_FUN)
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    OUT GT_U32 * memPtr,
    IN GT_U32  dataIsWords
);

/*******************************************************************************
*  DMA_WRITE_FUN
*
* DESCRIPTION:
*      write to HOST CPU DMA memory function.
* INPUTS:
*
*       deviceId    - device id. (of the device in the simulation)
*       address     - physical address that PP refer to.
*                     HOST CPU must convert it to HOST memory address
*       memSize     - Size of ASIC memory to read or write.
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
typedef void (* DMA_WRITE_FUN)
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_U32 * memPtr,
    IN GT_U32  dataIsWords
);

/*******************************************************************************
*   INTERRUPT_SET_FUN
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
*       because the fa and the pp has the same interrupt link i chosen to use
*       only the interrupt line of pp .
*
*******************************************************************************/
typedef void (*INTERRUPT_SET_FUN)
(
    IN  GT_U32        deviceId
);


/*
 * Typedef: struct SCIB_GLOBAL_DB_STC
 *
 * Description:
 *      Describe a SCIB global database .
 *
 * Fields:
 *
 * interruptSetFun - function set the interrupt line  --> direction is PP to CPU
 * dmaWriteFun - function write DMA memory --> direction is PP to CPU
 * dmaReadFun - function read DMA memory --> direction is PP to CPU
 * Comments:
 */
typedef struct{
    INTERRUPT_SET_FUN   interruptSetFun;
    DMA_READ_FUN        dmaReadFun;
    DMA_WRITE_FUN       dmaWriteFun;
}SCIB_GLOBAL_DB_STC;


#define DMA_READ_PROTOTYPE_BUILD_MAC(funcName)\
    static void funcName                    \
    (                                       \
        IN GT_U32 deviceId,             \
        IN GT_U32 address,              \
        IN GT_U32 memSize,              \
        OUT GT_U32 * memPtr,            \
        IN GT_U32  dataIsWords          \
    )

#define DMA_WRITE_PROTOTYPE_BUILD_MAC(funcName)\
    static void funcName                    \
    (                                       \
        IN GT_U32 deviceId,             \
        IN GT_U32 address,              \
        IN GT_U32 memSize,              \
        IN GT_U32 * memPtr,             \
        IN GT_U32  dataIsWords          \
    )

#define INTERRUPT_SET_PROTOTYPE_BUILD_MAC(funcName)\
    static void funcName                    \
    (                                       \
        IN  GT_U32        deviceId          \
    )

#define REGISTER_READ_WRITE_PROTOTYPE_BUILD_MAC(funcName)\
    static void funcName                           \
    (                                              \
        IN SCIB_MEMORY_ACCESS_TYPE accessType,     \
        IN GT_U32   deviceId,                      \
        IN GT_U32 address,                     \
        IN GT_U32 memSize,                     \
        INOUT GT_U32 * memPtr                  \
    )



DMA_READ_PROTOTYPE_BUILD_MAC(dmaRead);
DMA_READ_PROTOTYPE_BUILD_MAC(dmaRead_distributedApplication);
DMA_READ_PROTOTYPE_BUILD_MAC(dmaRead_distributedAsic);

DMA_WRITE_PROTOTYPE_BUILD_MAC(dmaWrite);
DMA_WRITE_PROTOTYPE_BUILD_MAC(dmaWrite_distributedApplication);
DMA_WRITE_PROTOTYPE_BUILD_MAC(dmaWrite_distributedAsic);

INTERRUPT_SET_PROTOTYPE_BUILD_MAC(interruptSet);
INTERRUPT_SET_PROTOTYPE_BUILD_MAC(interruptSet_distributedApplication);
INTERRUPT_SET_PROTOTYPE_BUILD_MAC(interruptSet_distributedAsic);

REGISTER_READ_WRITE_PROTOTYPE_BUILD_MAC(registerReadWrite_distributedApplication);

/* the functions needed by the "non-distributed" simulation */
static SCIB_GLOBAL_DB_STC nonDistributedObject =
{
    &interruptSet ,
    &dmaRead ,
    &dmaWrite
};

/* the functions needed by the "distributed application side" simulation */
static SCIB_GLOBAL_DB_STC distributedApplicationSideObject =
{
    &interruptSet_distributedApplication ,
    &dmaRead_distributedApplication ,
    &dmaWrite_distributedApplication
};

/* the functions needed by the "distributed asic side" simulation */
static SCIB_GLOBAL_DB_STC distributedAsicSideObject =
{
    &interruptSet_distributedAsic ,
    &dmaRead_distributedAsic ,
    &dmaWrite_distributedAsic
};

/* the functions needed by the "distributed broker" simulation */
static SCIB_GLOBAL_DB_STC distributedBrokerObject =
{
    &interruptSet_distributedApplication ,
    &dmaRead_distributedApplication ,
    &dmaWrite_distributedApplication
};

/* the functions needed by the "distributed interface bus" simulation */
static SCIB_GLOBAL_DB_STC distributedBusObject =
{
    &interruptSet_distributedApplication ,
    &dmaRead_distributedApplication ,
    &dmaWrite_distributedApplication
};

/* pointer to the object that implement the needed functions */
static SCIB_GLOBAL_DB_STC* globalObjectPtr = &nonDistributedObject;

static GT_MUTEX simulationProtectorMtx = (GT_MUTEX)0;

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
void scibInit0(void)
{
    /*create the SCIB layer mutex */
    if (simulationProtectorMtx == (GT_MUTEX)0)
    {
        simulationProtectorMtx = SIM_OS_MAC(simOsMutexCreate)();
    }
}

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
*         the database of the FA is different from the devices because
*         they have the same ID but different types of devices.
*******************************************************************************/
void scibInit
(
    IN GT_U32 maxDevNumber
)
{
    GT_U32              size;

    if (maxDevNumber == 0) {
        skernelFatalError("scibInit: illegal maxDevNumber %lu\n", maxDevNumber);
    }

    switch(sasicgSimulationRole)
    {
        case SASICG_SIMULATION_ROLE_NON_DISTRIBUTED_E:
            /* bind the dynamic functions to the object of "non-distributed"
               architecture , direct accessing no sockets used */
            globalObjectPtr = &nonDistributedObject;
            _non_dest = 1;
            break;

        case SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_E:
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_VIA_BROKER_E:
            _app = 1;
            /* bind the dynamic functions to the object of "distributed- application side"
               architecture , accessing via socket */
            globalObjectPtr = &distributedApplicationSideObject;
            break;

        case SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_E:
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_ASIC_SIDE_VIA_INTERFACE_BUS_BRIDGE_E:
            _dev = 1;
            /* bind the dynamic functions to the object of "distributed- Asic side"
               architecture , accessing via socket */
            globalObjectPtr = &distributedAsicSideObject;
            break;
        case SASICG_SIMULATION_ROLE_BROKER_E:
            _broker = 1;
            globalObjectPtr = &distributedBrokerObject;
            break;
        case SASICG_SIMULATION_ROLE_DISTRIBUTED_INTERFACE_BUS_BRIDGE_E:
            _bus = 1;
            globalObjectPtr = &distributedBusObject;
            break;
        default:
            skernelFatalError("scibInit: sasicgSimulationRole unknown role [%d]\n",sasicgSimulationRole);
    }


    maxDevicesPerSystem = maxDevNumber;

    /* Allocate array of device control structures  */
    size = maxDevNumber * sizeof(SCIB_DEVICE_DB_STC);
    scibDbPtr = malloc(size);
    if ( scibDbPtr == NULL ) {
        skernelFatalError("scibInit: scibDbPtr allocation error\n");
    }
    memset(scibDbPtr, 0, size);

    /* Allocate array of fabric adaptr control structures  */
    scibFaDbPtr = malloc(size);
    if ( scibFaDbPtr == NULL )
    {
        skernelFatalError("scibInit: scibFaDbPtr - allocation error\n");
    }
    memset(scibFaDbPtr, 0, size);
}

/*******************************************************************************
*   scibBindRWMemory
*
* DESCRIPTION:
*       Bind callbacks of SKernel for R/W memory requests.
*
* INPUTS:
*       deviceId  - ID of device, which is equal to PSS Core API device ID.
*       deviceObj - pointer to the opaque for SCIB device object.
*       rwFun     - pointer to the R/W SKernel memory CallBack function.
*       isPP      - boolean if the bind is for PP or for FA.
*       addressCompletionStatus - device enable/disable address completion
* OUTPUTS:
*       None.
*
* RETURNS:
*
*
* COMMENTS:
*       if device is fa , then the id is the one of the device connected to it.
*
*
*******************************************************************************/
void scibBindRWMemory
(
    IN GT_U32               deviceId,
    IN GT_U32               deviceHwId,
    IN void         *       deviceObjPtr,
    IN SCIB_RW_MEMORY_FUN   rwFun,
    IN GT_U8                isPP,
    IN GT_BOOL              addressCompletionStatus
)
{
    SCIB_DEVICE_DB_STC  *scibDevPtr;

    if(_non_dest != 1 &&  _dev != 1)
    {
        /* need to use scibRemoteInit instead of this bind ... */
        skernelFatalError("scibBindRWMemory : bad state \n");
    }

    DEV_NUM_CHECK_MAC(deviceId,scibBindRWMemory);

    if (rwFun == NULL || deviceObjPtr == NULL)
    {
        skernelFatalError(" scibBindRWMemory : no initialization was made for the array \n");
    }

    /* Write data to the device control entry for deviceId. */
    if (isPP)
    {
        scibDevPtr = &scibDbPtr[deviceId];
    }
    else
    {
        /* fa with id of "1" is located at "0" in the scibFaDbPtr */
        scibDevPtr = &scibFaDbPtr[deviceId];
    }

    scibDevPtr->valid = GT_TRUE;
    scibDevPtr->deviceObj = deviceObjPtr;
    scibDevPtr->memAccesFun = rwFun;
    scibDevPtr->deviceHwId =  deviceHwId;
    scibDevPtr->addressCompletionStatus = addressCompletionStatus;

    scibDevPtr->origMemAccesFun = scibDevPtr->memAccesFun;

}

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
)
{
    SCIB_DEVICE_DB_STC  *scibDevPtr;

    DEV_NUM_CHECK_MAC(deviceId,scibRemoteInit);

    if(_non_dest == 1 ||  _dev == 1)
    {
        skernelFatalError("scibRemoteInit : bad state \n");
    }

    /* Write data to the device control entry for deviceId. */
    if (isPp)
    {
        scibDevPtr = &scibDbPtr[deviceId];
    }
    else
    {
        /* fa with id of "1" is located at "0" in the scibFaDbPtr */
        scibDevPtr = &scibFaDbPtr[deviceId];
    }

    scibDevPtr->valid = GT_TRUE;
    scibDevPtr->deviceObj = NULL;  /* not used */
    scibDevPtr->memAccesFun = NULL;  /* not used */
    scibDevPtr->deviceHwId = deviceHwId;
    scibDevPtr->addressCompletionStatus = (GT_BOOL)addressCompletionStatus;

    if(interruptLine != SCIB_INTERRUPT_LINE_NOT_USED_CNS)
    {
        scibSetIntLine(deviceId,interruptLine);
    }

}


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
*       update the FA database for fa devices and pp database for pp devices.
*
*******************************************************************************/
void scibSetIntLine
(
    IN GT_U32               deviceId,
    IN GT_U32               intrline
)
{
    GT_U32 data;
    DEV_NUM_CHECK_MAC(deviceId,scibSetIntLine);

    /* the FA and the PP must have the same interrupt line */
    scibDbPtr[deviceId].intLine = intrline;
    scibFaDbPtr[deviceId].intLine = intrline;

    /* Set int line in PCI register 0x3c bits 0..7 */
    scibPciRegRead(deviceId,0x3c,1,&data);
    data &= 0xffffff00;
    data |= intrline & 0x000000ff;
    scibPciRegWrite(deviceId,0x3c,1,&data);
}
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
*       if the address is fa register then operation is done on fa object.
*
*******************************************************************************/
void scibReadMemory
(
    IN  GT_U32        deviceId,
    IN  GT_U32        memAddr,
    IN  GT_U32        length,
    OUT GT_U32 *      dataPtr
 )
 {
    void * deviceObjPtr;
    SCIB_RW_MEMORY_FUN   rwFun;

    DEV_NUM_CHECK_MAC(deviceId,scibReadMemory);

    if(_app)
    {
        SCIB_SEM_TAKE;
        /* only in the application side of  distributed architecture we need
           function to send the info to the other side (Asic side)*/
        registerReadWrite_distributedApplication(SCIB_MEMORY_READ_E,
                                                 deviceId,
                                                 memAddr,
                                                 length,
                                                 dataPtr);
        SCIB_SEM_SIGNAL;

        return ;
    }

    /* Find device control entry for deviceId. check if it is fa address*/
    if ((memAddr & FA_ADDRESS_BIT_CNS) &&
        (scibFaDbPtr[deviceId].valid == GT_TRUE))
    {
        deviceObjPtr = scibFaDbPtr[deviceId].deviceObj;
        rwFun = scibFaDbPtr[deviceId].memAccesFun;
    }
    else
    {
        deviceObjPtr = scibDbPtr[deviceId].deviceObj;
        rwFun = scibDbPtr[deviceId].memAccesFun;
    }

    /* Read memory from SKernel device.*/
    if (rwFun)
    {
        SCIB_SEM_TAKE;

        rwFun(SCIB_MEMORY_READ_E,
              deviceObjPtr,
              memAddr,
              length,
              dataPtr);

        SCIB_SEM_SIGNAL;
    }
    else
        memset(dataPtr, 0xff, length * sizeof(GT_U32));
 }
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
*      Pci registers are not relevant for the fa device.
*
*******************************************************************************/
void scibPciRegRead
(
    IN  GT_U32        deviceId,
    IN  GT_U32        memAddr,
    IN  GT_U32        length,
    OUT GT_U32 *      dataPtr
 )
 {
    void * deviceObjPtr;

    DEV_NUM_CHECK_MAC(deviceId,scibPciRegRead);

    if(_app)
    {
        SCIB_SEM_TAKE;
        /* only in the application side of  distributed architecture we need
           function to send the info to the other side (Asic side)*/
        registerReadWrite_distributedApplication(SCIB_MEMORY_READ_PCI_E,
                                                 deviceId,
                                                 memAddr,
                                                 length,
                                                 dataPtr);
        SCIB_SEM_SIGNAL;
        return ;
    }

    /* Find device control entry for deviceId. */
    deviceObjPtr = scibDbPtr[deviceId].deviceObj;

    /* Read PCI memory from SKernel device.*/
    if (scibDbPtr[deviceId].memAccesFun)
    {
        SCIB_SEM_TAKE;

        scibDbPtr[deviceId].memAccesFun( SCIB_MEMORY_READ_PCI_E,
                                         deviceObjPtr,
                                         memAddr,
                                         length,
                                         dataPtr);

        SCIB_SEM_SIGNAL;
    }
    else
        memset(dataPtr, 0xff, length * sizeof(GT_U32));

}
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
*      Pci registers are not relevant for the fa device.
*
*******************************************************************************/
void scibWriteMemory
(
    IN  GT_U32        deviceId,
    IN  GT_U32        memAddr,
    IN  GT_U32        length,
    IN  GT_U32 *      dataPtr
)
{
    void *            deviceObjPtr;
    SCIB_RW_MEMORY_FUN   rwFun;
    GT_U32 wordIndex;
    DEV_NUM_CHECK_MAC(deviceId,scibWriteMemory);

    SCIB_DEBUG_PRINT_WRITE_MAC(deviceId,memAddr,length,dataPtr);

    if(_app)
    {
        SCIB_SEM_TAKE;
        /* Loop is needed to ensure explicitly call of smemFindMemChunk function in resolution of memory word.
           This way of memory access gives ASIC ability to control internal buffer memory write */
        for(wordIndex = 0; wordIndex < length; wordIndex++, memAddr+=4)
        {
            /* only in the application side of  distributed architecture we need
               function to send the info to the other side (Asic side) */
            registerReadWrite_distributedApplication(SCIB_MEMORY_WRITE_E,
                                                     deviceId,
                                                     memAddr,
                                                     1,
                                                     &dataPtr[wordIndex]);
        }
        SCIB_SEM_SIGNAL;
        return ;
    }

    /* Find device control entry for deviceId. check if it is fa address*/
    if ((memAddr & FA_ADDRESS_BIT_CNS) &&
        (scibFaDbPtr[deviceId].valid == GT_TRUE))
    {
        deviceObjPtr = scibFaDbPtr[deviceId].deviceObj;
        rwFun = scibFaDbPtr[deviceId].memAccesFun;
    }
    else
    {
        deviceObjPtr = scibDbPtr[deviceId].deviceObj;
        rwFun = scibDbPtr[deviceId].memAccesFun;
    }

    if (rwFun)
    {
        SCIB_SEM_TAKE;
        /* Loop is needed to ensure explicitly call of smemFindMemChunk function in resolution of memory word.
           This way of memory access gives ASIC ability to control internal buffer memory write */
        for(wordIndex = 0; wordIndex < length; wordIndex++, memAddr+=4)
        {
            /* Write memory from SKernel device.*/
            rwFun(SCIB_MEMORY_WRITE_E,
                  deviceObjPtr,
                  memAddr,
                  1,
                  &dataPtr[wordIndex]);
        }
        SCIB_SEM_SIGNAL;
    }
}
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
*
*    Pci registers are not relevant for the fa device.
*******************************************************************************/
void scibPciRegWrite
(
    IN  GT_U32        deviceId,
    IN  GT_U32        memAddr,
    IN  GT_U32        length,
    IN  GT_U32 *      dataPtr
)
{
    void *            deviceObjPtr;

    DEV_NUM_CHECK_MAC(deviceId,scibPciRegWrite);

    if(_app)
    {
        SCIB_SEM_TAKE;
        /* only in the application side of  distributed architecture we need
           function to send the info to the other side (Asic side)*/
        registerReadWrite_distributedApplication(SCIB_MEMORY_WRITE_PCI_E,
                                                 deviceId,
                                                 memAddr,
                                                 length,
                                                 dataPtr);
        SCIB_SEM_SIGNAL;
        return ;
    }

    /* Find device control entry for deviceId. */
    deviceObjPtr = scibDbPtr[deviceId].deviceObj;

    /* Find device control entry for deviceId. */
    if (scibDbPtr[deviceId].memAccesFun)
    {
        SCIB_SEM_TAKE;
        /* Write PCI memory from SKernel device.*/
        scibDbPtr[deviceId].memAccesFun( SCIB_MEMORY_WRITE_PCI_E,
                                         deviceObjPtr,
                                         memAddr,
                                         length,
                                         dataPtr);
        SCIB_SEM_SIGNAL;
    }

}
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
*       because the fa and the pp has the same interrupt link i chosen to use
*       only the interrupt line of pp .
*
*******************************************************************************/
void scibSetInterrupt
(
    IN  GT_U32        deviceId
)
{
    globalObjectPtr->interruptSetFun(deviceId);
}
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
)
{
    GT_U32  memAddr;
    GT_U32  memData;
    GT_U32  index;

    if (smiDev > SOHO_SMI_DEV_TOTAL)
    {
        skernelFatalError("scibSmiRegWrite : smiDev%d is out of boundary \n",
                           smiDev);
    }
    index = (smiDev >= SOHO_GLOBAL_REGS_START_ADDR) ? 0 :
            (smiDev >= SOHO_PORT_REGS_START_ADDR) ? 1 : 2;

    /* Make 32 bit word address */
    memAddr = (index << 28) | (smiDev << 16) | (regAddr << 4);

    scibReadMemory(deviceId, memAddr, 1, &memData);
    /* Read actual Smi register data */
    *data = (GT_U16)memData;
}

/*******************************************************************************
* scibWriteSmiReg
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
)
{
    GT_U32  memAddr, index;

    if (smiDev > SOHO_SMI_DEV_TOTAL)
    {
        skernelFatalError("scibSmiRegWrite : smiDev%d is out of boundary \n",
                           smiDev);
    }
    index = (smiDev >= SOHO_GLOBAL_REGS_START_ADDR) ? 0 :
            (smiDev >= SOHO_PORT_REGS_START_ADDR) ? 1 : 2;

    /* Make 32 bit word address */
    memAddr = (index << 28) | (smiDev << 16) | (regAddr << 4);

    scibWriteMemory(deviceId, memAddr, 1, (GT_U32 *)&data);
}
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
GT_U32 scibGetDeviceId
(
    IN  GT_U32        deviceHwId
)
{
    GT_U32  deviceId;

    for (deviceId = 0; deviceId < maxDevicesPerSystem; deviceId++)
    {
        if (scibDbPtr[deviceId].valid == GT_TRUE)
        {
            if (scibDbPtr[deviceId].deviceHwId == deviceHwId)
            {
                return deviceId;
            }
        }
    }

    /* deviceHwId was not found */
    return SCIB_NOT_EXISTED_DEVICE_ID_CNS;
}

/*******************************************************************************
*   scibAddressCompletionStatusGet
*
* DESCRIPTION:
*       Get address completion status for given hwId
*
* INPUTS:
*       devNum - number of SKernel devices in the Simulation.
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
)
{
    if (scibDbPtr[devNum].valid == GT_FALSE)
    {
        skernelFatalError("scibAddressCompletionStatusGet : illegal device[%ld]\n",devNum);
    }

    /* Check device type for enable/disable address completion */
    return scibDbPtr[devNum].addressCompletionStatus;
}


/*******************************************************************************
*  dmaRead
*
* DESCRIPTION:
*      read HOST CPU DMA memory function.
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
static void dmaRead
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    OUT GT_U32 * memPtr,
    IN GT_U32  dataIsWords
)
{
    deviceId = deviceId;/* don't care !!! */
    dataIsWords = dataIsWords;/* don't care !!! */

    memcpy((void*)memPtr,
           (void*)(GT_UINTPTR)address,
           memSize * sizeof(GT_U32));
}

/*******************************************************************************
*  dmaWrite
*
* DESCRIPTION:
*      write to HOST CPU DMA memory function.
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
static void dmaWrite
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_U32 * memPtr,
    IN GT_U32  dataIsWords
)
{
    deviceId = deviceId;/* don't care !!! */
    dataIsWords = dataIsWords;/* don't care !!! */

    memcpy((void*)(GT_UINTPTR)address,
           (void*)memPtr,
           memSize * sizeof(GT_U32));
}

/*******************************************************************************
*   interruptSet
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
*       because the fa and the pp has the same interrupt link i chosen to use
*       only the interrupt line of pp .
*
*******************************************************************************/
static void interruptSet
(
    IN  GT_U32        deviceId
)
{

    DEV_NUM_CHECK_MAC(deviceId,interruptSet);

    /* the fa and pp must have the same interrupt line */

    /* increment intLine to fix Host bug */

    /* Call simOs to generate interrupt for interrupt line */
    SIM_OS_MAC(simOsInterruptSet)(scibDbPtr[deviceId].intLine + 1); /* increment intLine to fix Host bug */
}


/*******************************************************************************
*  dmaRead_distributedApplication
*
* DESCRIPTION:
*      read HOST CPU DMA memory function.
*      in distributed architecture on application size
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
static void dmaRead_distributedApplication
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    OUT GT_U32 * memPtr,
    IN GT_U32  dataIsWords
)
{
    dmaRead(deviceId,address,memSize,memPtr,dataIsWords);

    /* the caller will return the read data to the "other side" */
}

/*******************************************************************************
*  dmaWrite_distributedApplication
*
* DESCRIPTION:
*      write to HOST CPU DMA memory function.
*      in distributed architecture on application size
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
static void dmaWrite_distributedApplication
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_U32 * memPtr,
    IN GT_U32  dataIsWords
)
{
    dmaWrite(deviceId,address,memSize,memPtr,dataIsWords);
}

/*******************************************************************************
*   interruptSet_distributedApplication
*
* DESCRIPTION:
*       Generate interrupt for SKernel device.
*      in distributed architecture on application size
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
*       because the fa and the pp has the same interrupt link i chosen to use
*       only the interrupt line of pp .
*
*******************************************************************************/
static void interruptSet_distributedApplication
(
    IN  GT_U32        deviceId
)
{
    interruptSet(deviceId);
}


/*******************************************************************************
*  dmaRead_distributedAsic
*
* DESCRIPTION:
*      read HOST CPU DMA memory function.
*      in distributed architecture on Asic size
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
static void dmaRead_distributedAsic
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    OUT GT_U32 * memPtr,
    IN GT_U32  dataIsWords
)
{
    simDistributedDmaRead(deviceId,address,memSize,memPtr,dataIsWords);
}

/*******************************************************************************
*  dmaWrite_distributedAsic
*
* DESCRIPTION:
*      write to HOST CPU DMA memory function.
*      in distributed architecture on Asic size
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
static void dmaWrite_distributedAsic
(
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_U32 * memPtr,
    IN GT_U32  dataIsWords
)
{
    simDistributedDmaWrite(deviceId,address,memSize,memPtr,dataIsWords);
}

/*******************************************************************************
*   interruptSet_distributedAsic
*
* DESCRIPTION:
*       Generate interrupt for SKernel device.
*      in distributed architecture on Asic size
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
*       because the fa and the pp has the same interrupt link i chosen to use
*       only the interrupt line of pp .
*
*******************************************************************************/
static void interruptSet_distributedAsic
(
    IN  GT_U32        deviceId
)
{
    simDistributedInterruptSet(deviceId);
}


/*******************************************************************************
*  registerReadWrite_distributedApplication
*
* DESCRIPTION:
*      FUNCTION of R/W Skernel memory function.
*      in distributed architecture on Application size
* INPUTS:
*       accessType  - Define access operation Read or Write.
*       deviceId  - ID of device, which is equal to PSS Core API device ID.
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
static void registerReadWrite_distributedApplication
(
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN GT_U32   deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    INOUT GT_U32 * memPtr
)
{
    switch(accessType)
    {
        case SCIB_MEMORY_READ_E:
        case SCIB_MEMORY_READ_PCI_E:
            simDistributedRegisterRead(accessType,deviceId,address,memSize,memPtr);
            break;
        case SCIB_MEMORY_WRITE_E:
        case SCIB_MEMORY_WRITE_PCI_E:
            simDistributedRegisterWrite(accessType,deviceId,address,memSize,memPtr);
            break;
        default:
            skernelFatalError("registerReadWrite_distributedApplication: unknown accessType[%d]\n", accessType);
    }
}


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
)
{
    if (address == 0 || memPtr == NULL)
       return;
    globalObjectPtr->dmaReadFun(deviceId,address,memSize,memPtr,dataIsWords);
}

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
)
{
    if (address == 0 || memPtr == NULL)
       return;
    globalObjectPtr->dmaWriteFun(deviceId,address,memSize,memPtr,dataIsWords);
}

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
void scibAccessLock(void)
{
    SIM_OS_MAC(simOsMutexLock)(simulationProtectorMtx);
}


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
void scibAccessUnlock(void)
{
    SIM_OS_MAC(simOsMutexUnlock)(simulationProtectorMtx);
}

/* function to create fatal error whan called */
static void fatalErrorAccess(void)
{
    skernelFatalError(" fatalErrorAccess : try to access none exists device ");

    return;
}

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
)
{
    DEV_NUM_CHECK_MAC(deviceId,scibUnBindDevice);

    if(scibDbPtr[deviceId].memAccesFun == NULL)
    {
        skernelFatalError(" scibUnBindDevice : device id %d not bound \n",deviceId);
    }

    scibDbPtr[deviceId].memAccesFun = (void*)fatalErrorAccess;
}

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
)
{
    DEV_NUM_CHECK_MAC(deviceId,scibUnBindDevice);

    if(scibDbPtr[deviceId].memAccesFun == NULL)
    {
        skernelFatalError(" scibReBindDevice : device id %d was never bound , so can't re-bind \n",deviceId);
    }

    scibDbPtr[deviceId].memAccesFun = scibDbPtr[deviceId].origMemAccesFun;
}


/*******************************************************************************
*   scibDebugPrint
*
* DESCRIPTION:
*       allow printings of the registers settings
*
* INPUTS:
*       enable - enable/disable the printings.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void scibDebugPrint(GT_BOOL   enable)
{
    debugPrint_enable = enable;
}

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
)
{
    GT_STATUS   rc;

    DEV_NUM_CHECK_MAC(deviceId,scibPortLoopbackForceModeSet);

    if(_app)
    {
        rc = GT_NOT_IMPLEMENTED;
        return rc;
    }

    SCIB_SEM_TAKE;

    rc = skernelPortLoopbackForceModeSet(deviceId,portNum,mode);

    SCIB_SEM_SIGNAL;

    return rc;
}

/*******************************************************************************
*   scibPortLinkStateSet
*
* DESCRIPTION:
*       the function set the 'link state' on a port of device.
*       this function needed for devices that not support 'link change' from the
*       'MAC registers' of the ports.
*       this is relevant to 'GM devices'
*
* INPUTS:
*       deviceId  - the simulation device Id .
*       portNum   - the physical port number .
*       linkState - the link state to set for the port.
*                   GT_TRUE  - force 'link UP'
*                   GT_FALSE - force 'link down'
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
GT_STATUS scibPortLinkStateSet
(
    IN  GT_U32                      deviceId,
    IN  GT_U32                      portNum,
    IN GT_BOOL                      linkState
)
{
    GT_STATUS   rc;

    DEV_NUM_CHECK_MAC(deviceId,scibPortLinkStateSet);

    if(_app)
    {
        rc = GT_NOT_IMPLEMENTED;
        return rc;
    }

    SCIB_SEM_TAKE;

    rc = skernelPortLinkStateSet(deviceId,portNum,linkState);

    SCIB_SEM_SIGNAL;

    return rc;
}

/*******************************************************************************
*   scibMemoryClientRegRead
*
* DESCRIPTION:
*       Generic read of SCIB client registers memory from SKernel device.
*
* INPUTS:
*       deviceId  - ID of device, which is equal to PSS Core API device ID.
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
)
{
    void * deviceObjPtr;
    SCIB_MEMORY_ACCESS_TYPE accessType;

    DEV_NUM_CHECK_MAC(deviceId, scibMemoryClientRegRead);

    /* Find device control entry for deviceId. */
    deviceObjPtr = scibDbPtr[deviceId].deviceObj;

    switch(scibClient)
    {
        case SCIB_MEM_ACCESS_PCI_E:
            accessType = SCIB_MEMORY_READ_PCI_E;
            break;
        case SCIB_MEM_ACCESS_DFX_E:
            accessType = SCIB_MEMORY_READ_DFX_E;
            break;
        case SCIB_MEM_ACCESS_CORE_E:
            accessType = SCIB_MEMORY_READ_E;
            break;
        default:
            accessType = SCIB_MEMORY_LAST_E;
            skernelFatalError("scibMemoryClientRegRead: illegal client type %lu\n", scibClient);
    }

    /* Read DFX memory from SKernel device.*/
    if (scibDbPtr[deviceId].memAccesFun)
    {
        SCIB_SEM_TAKE;

        scibDbPtr[deviceId].memAccesFun(accessType,
                                        deviceObjPtr,
                                        memAddr,
                                        length,
                                        dataPtr);

        SCIB_SEM_SIGNAL;
    }
    else
        memset(dataPtr, 0xff, length * sizeof(GT_U32));
}

/*******************************************************************************
*   scibMemoryClientRegWrite
*
* DESCRIPTION:
*       Generic write to registers of SKernel device according to SCIB client.
*
* INPUTS:
*       deviceId  - ID of device, which is equal to PSS Core API device ID.
*       memAddr - address of first word to read.

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
)
{
    void * deviceObjPtr;
    SCIB_MEMORY_ACCESS_TYPE accessType;

    DEV_NUM_CHECK_MAC(deviceId, scibMemoryClientRegWrite);

    /* Find device control entry for deviceId. */
    deviceObjPtr = scibDbPtr[deviceId].deviceObj;

    switch(scibClient)
    {
        case SCIB_MEM_ACCESS_PCI_E:
            accessType = SCIB_MEMORY_WRITE_PCI_E;
            break;
        case SCIB_MEM_ACCESS_DFX_E:
            accessType = SCIB_MEMORY_WRITE_DFX_E;
            break;
        case SCIB_MEM_ACCESS_CORE_E:
            accessType = SCIB_MEMORY_WRITE_E;
            break;
        default:
            accessType = SCIB_MEMORY_LAST_E;
            skernelFatalError("scibMemoryClientRegWrite: illegal client type %lu\n", scibClient);
    }

    /* Read DFX memory from SKernel device.*/
    if (scibDbPtr[deviceId].memAccesFun)
    {
        SCIB_SEM_TAKE;

        scibDbPtr[deviceId].memAccesFun(accessType,
                                        deviceObjPtr,
                                        memAddr,
                                        length,
                                        dataPtr);

        SCIB_SEM_SIGNAL;
    }
}

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
)
{
    void * deviceObjPtr;

    DEV_NUM_CHECK_MAC(deviceId, scibCoreClockRegisterUpdate);

    /* Find device control entry for deviceId. */
    deviceObjPtr = scibDbPtr[deviceId].deviceObj;

    SCIB_SEM_TAKE;

    smemCheetahUpdateCoreClockRegister(deviceObjPtr,coreClockInMHz,hwFieldValue);

    SCIB_SEM_SIGNAL;
}

