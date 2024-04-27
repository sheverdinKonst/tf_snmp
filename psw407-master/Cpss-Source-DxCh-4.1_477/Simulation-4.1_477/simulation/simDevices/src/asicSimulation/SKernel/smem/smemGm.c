/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smemGm.c
*
* DESCRIPTION:
*       This is API implementation for GM devices memories.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 37 $
*
*******************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/smem/smemTigerGM.h>
#include <asicSimulation/SKernel/smem/smemGM.h>
#include <gmSimulation/GM/GmInitInterfaceAPI.h>
#include <gmSimulation/GM/ManagementAPI.h>
#include <gmSimulation/GM/GMApi.h>
#include <asicSimulation/SCIB/scib.h>
#include <asicSimulation/SInit/sinit.h>
#include <asicSimulation/SKernel/smem/smemCheetah3.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>


extern void smemChtGMInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObj
);
/* path to the embedded file system */
extern char   embeddedFsPath[];

void *simulationRegProtectorSem = NULL;

static SKERNEL_DEVICE_OBJECT * gmSingleDeviceObjPtr = NULL;

static void  smemGMRWMem (
                                IN SCIB_MEMORY_ACCESS_TYPE accessType,
                                IN void * deviceObjPtr,
                                IN GT_U32 address,
                                IN GT_U32 memSize,
                                INOUT GT_U32 * memPtr
                              );

static void smemGmCreateInterruptTask
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);
static void smemGMTask
(
    IN void * deviceObjPtr
);


#define NOT_VALID_ADDRESS 0xFFFFFFFF
static GT_U32   lastRegisterAccessed = NOT_VALID_ADDRESS;

/* indication that the GM init was done and we can start read/write memory */
static GT_U32   gmInitDone = 0;

#define SMEM_GM_GET_DEVICE_ID(deviceId,coreId) \
    ((smainDeviceObjects[deviceId]->shellDevice == GT_FALSE) ? \
    smainDeviceObjects[deviceId]->deviceId : \
    smainDeviceObjects[deviceId]->coreDevInfoPtr[coreId].devObjPtr->deviceId )

#define GM_BAD_ADDR_CNS     0x000badad

/*******************************************************************************
*
* DESCRIPTION:
*
*       Packet receive function.
*
* INPUTS:
*       sourcePort - the packet's out going port number.
*       packetBuff  - the out going packet in a character buffer.
*       packetSize  - the packet size in bytes.
* RETURNS:
*       GT_VOID
*
* COMMENTS:
*       This function is registered by the user traffic manager and called
*       by the Golden Model whenever packet is send out from a specific port
*
*******************************************************************************/
void gmPacketPortOutput (
    IN  unsigned int    deviceId,
    IN  unsigned int    coreId,
    IN  unsigned int    egrPort,
    IN const char *     packetBuff,
    IN unsigned int     packetSize)
{
    if(packetSize >= 0x3000)
    {
        /* the GM goes wild ! ... ignore those packets */
        return;
    }

    if((coreId == 2 || coreId == 3) &&
       (smainDeviceObjects[SMEM_GM_GET_DEVICE_ID(deviceId,0)]->deviceFamily ==
            SKERNEL_PUMA3_NETWORK_FABRIC_FAMILY))
    {
        /* the Puma3 device has 4 pipes that handle packets but there
           are only 2 MG units . so we have only devObjPtr->portGroupId = 0 and 1 */
        /* but still we need to get from the GM packet need to be sent to
            pipe 0 or pipe 1 or pipe 2 or pipe 3 with local port 0..11 */

        coreId  -=  2;
        egrPort += 32;
    }

    smainFrame2PortSend(smainDeviceObjects[SMEM_GM_GET_DEVICE_ID(deviceId,coreId)],
        egrPort,(GT_U8* )packetBuff, packetSize, GT_FALSE);

    return;
}

/*******************************************************************************
*   smemInterruptCB
*
* DESCRIPTION:
*       interrupt callback function
*
* INPUTS:
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*           call back functions for device DMA memory access
*******************************************************************************/
static void smemInterruptCB (IN  unsigned int    deviceId,
                             IN  unsigned int    coreId)
{
    unsigned simDevId = SMEM_GM_GET_DEVICE_ID(deviceId,coreId);
    if ((smainDeviceObjects[simDevId]->gmDeviceType != GOLDEN_MODEL))
    {
        printf("got interrupt\n");
    }
    else
    {
        scibSetInterrupt(simDevId);
    }

    return ;
}

/*******************************************************************************
*   writeDma
*
* DESCRIPTION:
*       write buffer into the dma
*
* INPUTS:
*       addr    - pointer to the device object for task.
*       cBuffer - pointer of buffer
*       len     - length of buffer.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*   call back functions for device DMA memory access
*******************************************************************************/
static GT_VOID writeDma(unsigned int addr, unsigned char* cBuffer, unsigned int len)
{
    /* memcpy((void*)addr, cBuffer, len);*/
    scibDmaWrite(gmSingleDeviceObjPtr->deviceId,addr,NUM_BYTES_TO_WORDS(len), (void*)cBuffer,SCIB_DMA_WORDS);
}

/*******************************************************************************
*   writeDmaPuma
*
* DESCRIPTION:
*       write buffer into the dma
*
* INPUTS:
*       addr    - pointer to the device object for task.
*       cBuffer - pointer of buffer
*       len     - length of buffer.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*   call back functions for device DMA memory access
*******************************************************************************/
static GT_VOID writeDmaPuma(IN  unsigned int    deviceId,
                            IN  unsigned int    coreId,
                            unsigned int addr, char* cBuffer, unsigned int len)
{
    /*memcpy((void*)addr, cBuffer, len);*/
    scibDmaWrite(SMEM_GM_GET_DEVICE_ID(deviceId,coreId),
                 addr,NUM_BYTES_TO_WORDS(len), (void*)cBuffer,SCIB_DMA_WORDS);
}

/*******************************************************************************
*   readDma
*
* DESCRIPTION:
*       read buffer from dma
*
* INPUTS:
*       addr    - pointer to the device object for task.
*       cBuffer - pointer of buffer
*       len     - length of buffer.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*   call back functions for device DMA memory access
*******************************************************************************/
static GT_VOID readDma(unsigned int addr, unsigned char* cBuffer, unsigned int len)
{
    /*memcpy(cBuffer,(void*)addr, len);*/
    scibDmaRead(gmSingleDeviceObjPtr->deviceId,addr,NUM_BYTES_TO_WORDS(len), (void*)cBuffer,SCIB_DMA_WORDS);
}

/*******************************************************************************
*   readDma
*
* DESCRIPTION:
*       read buffer from dma
*
* INPUTS:
*       addr    - pointer to the device object for task.
*       cBuffer - pointer of buffer
*       len     - length of buffer.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*   call back functions for device DMA memory access
*******************************************************************************/
static GT_VOID readDmaPuma(unsigned int deviceId,
                           unsigned int    coreId,
                           unsigned int addr, char* cBuffer, unsigned int len)
{
    /*memcpy(cBuffer,(void*)addr, len);*/
    scibDmaRead(SMEM_GM_GET_DEVICE_ID(deviceId,coreId),
                addr,NUM_BYTES_TO_WORDS(len), (void*)cBuffer,SCIB_DMA_WORDS);
}

/*******************************************************************************
* osGmPrintf
*
* DESCRIPTION:
*       Write a formatted string to the standard output stream.
*
* INPUTS:
*       format  - format string to write
*
* OUTPUTS:
*       None
*
* RETURNS:
*       The number of characters written, or a negative value if
*       an output error occurs.
*
* COMMENTS:
*       None
*
*******************************************************************************/
int osGmPrintf(const char* format, ...)
{
    char buff[2048];
    va_list args;

    if(lastRegisterAccessed != NOT_VALID_ADDRESS)
    {
        /* print the last register address that was accessed */

        /* only when we are in the context of 'read'/'write' register/memory */
        printf("osGmPrintf: reg[0x%8.8x]\n",lastRegisterAccessed);
    }

    va_start(args, format);
    vsprintf(buff, format, args);
    va_end(args);

    return printf("%s", buff);
}

/*******************************************************************************
*   smemGmInternalCpuInterfaceInit
*
* DESCRIPTION:
*       Init CPU internal interface for a GM device.
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       devInfoSection  - the device info section in the INI file
*       devId           - the deviceId in the INI file info
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
static void smemGmInternalCpuInterfaceInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    SMEM_CHT_DEV_COMMON_MEM_INFO  * devMemInfoPtr;

    if (devObjPtr->supportCpuInternalInterface == 0)
    {
        return;
    }

    devMemInfoPtr = (SMEM_CHT_DEV_COMMON_MEM_INFO  *)devObjPtr->deviceMemory;
    devMemInfoPtr->pciUnitBaseAddrMask = 0x000f0000;
    devMemInfoPtr->pciUnitBaseAddr = 0x00040000;
    /* MBus base address */
    devMemInfoPtr->pciExtMemArr[SMEM_UNIT_PCI_BUS_MBUS_E].unitBaseAddr =
        devMemInfoPtr->pciUnitBaseAddr;
    /* Allocate MBus memory space*/
    smemXCatA1UnitPex(devObjPtr,
                      &devMemInfoPtr->pciExtMemArr[SMEM_UNIT_PCI_BUS_MBUS_E].unitMem,
                      devMemInfoPtr->pciExtMemArr[SMEM_UNIT_PCI_BUS_MBUS_E].unitBaseAddr);
}

/*******************************************************************************
*   smemGmInit
*
* DESCRIPTION:
*       Init memory module for a GM device.
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemGmInit
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    GT_U32      regData;        /* Register data */
    SMEM_CHT_DEV_COMMON_MEM_INFO  * devMemInfoPtr;
    GT_U32      mask;/*mask*/

    if(deviceObjPtr->shellDevice == GT_TRUE)
    {
        return;
    }

    /* MUST bind before call to gmCreate , because application may already try to
    access the PEX registers before the GM is done initialized */

    /* alloc SMEM_CHT_DEV_MEM_INFO */
    devMemInfoPtr = (SMEM_CHT_DEV_COMMON_MEM_INFO *)calloc(1, sizeof(SMEM_CHT_DEV_COMMON_MEM_INFO));
    if (devMemInfoPtr == 0)
    {
            skernelFatalError("smemGmInit: allocation error\n");
    }

    deviceObjPtr->deviceMemory = (void *)devMemInfoPtr;

    /* bind R/W callback with SCIB */
    scibBindRWMemory(deviceObjPtr->deviceId ,
                 deviceObjPtr->deviceHwId,
                 deviceObjPtr,
                 &smemGMRWMem,
                 GT_TRUE,
                 ADDRESS_COMPLETION_STATUS_GET_MAC(deviceObjPtr));

    if (deviceObjPtr->gmDeviceType != GOLDEN_MODEL)
    {
        if(gmSingleDeviceObjPtr == NULL)
        {
            /* register the single GM device that can run on this process */
            gmSingleDeviceObjPtr = deviceObjPtr;
        }
        else
        {
            /* not supported option , the design and implementation not allow more
                then single GM device on single process */
            skernelFatalError(" smemGmInit: trying to initialize more then single GM device \n");
        }

        registerCallBack(smemInterruptCB, writeDma, readDma);

        if (SKERNEL_DEVICE_FAMILY_TIGER(deviceObjPtr->deviceType))
        {
            smemTigerGMInit(deviceObjPtr);
        }
        else if (SKERNEL_DEVICE_FAMILY_CHEETAH(deviceObjPtr))
        {
            smemChtGMInit(deviceObjPtr);
        }
        /* notify that the GM is ready for memory access */
        gmInitDone = 1;
    }
    else
    {
        /* should create the GM only after we bind memory using scibBindRWMemory !!! */

        /* for each device init one GM for all the cores */
        if (deviceObjPtr->portGroupId == 0)
        {
            CALLBACKS_SERVICE pParam;
            char gmDevType[SKERNEL_DEVICE_NAME_MAX_SIZE_CNS];
            char param_str[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
            GT_CHAR keyStr[30];                  /* key string*/
            FILE    * gmIniFilePtr;             /* GM ini file object */

            pParam.pfGetPacketFromPort = gmPacketPortOutput;
            pParam.pfInterruptCallBack = smemInterruptCB;
            pParam.pfReadMemoryBufferFromCPU = readDmaPuma;
            pParam.pfWriteMemoryBufferToCPU = writeDmaPuma;

            switch (deviceObjPtr->deviceFamily)
            {
            case SKERNEL_PUMA_FAMILY:
                strcpy(gmDevType,"puma2");
                break;
            case SKERNEL_PUMA3_NETWORK_FABRIC_FAMILY:
                strcpy(gmDevType,"puma3");
                break;
            case SKERNEL_LION_PORT_GROUP_FAMILY:
                strcpy(gmDevType,"lionb");
                break;
            case SKERNEL_LION2_PORT_GROUP_FAMILY:
            case SKERNEL_LION3_PORT_GROUP_FAMILY:
                strcpy(gmDevType,"aslan");
                break;
            case SKERNEL_XCAT2_FAMILY:
                strcpy(gmDevType,"xcat2");
                deviceObjPtr->supportCpuInternalInterface = 1;
                break;
            case SKERNEL_BOBK_CAELUM_FAMILY:
            case SKERNEL_BOBK_CETUS_FAMILY: /* TBD */
            case SKERNEL_BOBCAT2_FAMILY:
            case SKERNEL_BOBCAT3_FAMILY:
                strcpy(gmDevType,"sip5");
                break;
            default:
                skernelFatalError(" smemGmInit: missing device type for GM init \n");
            }

            smemGmInternalCpuInterfaceInit(deviceObjPtr);

            /* get the GM ini file  */
            sprintf(keyStr, "dev%u_gmIniFile",
                    SMEM_GM_GET_GM_DEVICE_ID(deviceObjPtr));

            if (!SIM_OS_MAC(simOsGetCnfValue)(INI_FILE_SYSTEM_SECTION_CNS, keyStr,
                                              SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS, param_str))
            {
                 /* GM ini file path is wrong */
                skernelFatalError(" smemGmInit: no GM ini file path");
            }

            gmIniFilePtr = fopen(param_str,"rt");
            if (gmIniFilePtr == NULL)
            {
                 /* GM ini file not found */
                skernelFatalError(" smemGmInit: GM ini file not found");
            }

            fclose(gmIniFilePtr);

            /* Supply to ppGmInit full path to embedded file system */
            if(embeddedFsPath[0] != 0)
            {
                strcpy(param_str, strcat(embeddedFsPath, param_str));
            }

            ppGmInit(SMEM_GM_GET_GM_DEVICE_ID(deviceObjPtr), gmDevType, &pParam,
                     param_str);

#ifdef  GM_FATAL_ERROR_BIND
            /* the GM support ability to bind to 'fatal error' of the simulation */
            /* can be called only after ppGmInit(...) */
            ppSetErrorCallback((unsigned int)skernelFatalError);
#endif  /*GM_FATAL_ERROR_BIND*/

            /* notify that the GM is ready for memory access */
            gmInitDone = 1;
        }

        switch (deviceObjPtr->deviceFamily)
        {
            case SKERNEL_BOBCAT2_FAMILY:
            case SKERNEL_BOBK_CAELUM_FAMILY:
            case SKERNEL_BOBK_CETUS_FAMILY: /* TBD */
            case SKERNEL_LION3_PORT_GROUP_FAMILY:
            case SKERNEL_BOBCAT3_FAMILY:
                SMEM_CHT_IS_SIP5_GET(deviceObjPtr) = GT_TRUE;
                deviceObjPtr->addressCompletionType =
                    SKERNEL_ADDRESS_COMPLETION_TYPE_8_REGIONS_E;
                break;
            default: break;
        }

        /*set the base address-- b2b WA*/
        smemGMRWMem(SCIB_MEMORY_WRITE_PCI_E, deviceObjPtr, 0x14,
                    1, &deviceObjPtr->deviceHwId);

        /*Set port group ID*/
        if(deviceObjPtr->portGroupSharedDevObjPtr)/* this is core in multi-core device */
        {
            switch(deviceObjPtr->deviceFamily)
            {
                case SKERNEL_LION_PORT_GROUP_FAMILY:
                    mask = 0x3;
                    break;
                case SKERNEL_LION2_PORT_GROUP_FAMILY:
                case SKERNEL_LION3_PORT_GROUP_FAMILY:
                case SKERNEL_PUMA3_NETWORK_FABRIC_FAMILY:
                    mask = 0x7;
                    break;
                default:
                    mask = 0x7;
                    break;
            }

            /* Read Global Control Register */
            smemGMRWMem(SCIB_MEMORY_READ_E, deviceObjPtr, 0x58, 1, &regData);
            /* Set port group ID - bit 9:10 register lobal control */
            regData &= ~(mask << 9);
            regData |= (deviceObjPtr->portGroupId << 9);

            smemGMRWMem(SCIB_MEMORY_WRITE_E, deviceObjPtr, 0x58, 1, &regData);
        }
    }

    if (deviceObjPtr->gmDeviceType != GOLDEN_MODEL)
    {
        smemGmCreateInterruptTask(deviceObjPtr);
    }
}

/*******************************************************************************
*   smemGmInit2
*
* DESCRIPTION:
*       Init memory module for a GM device (Phase 2).
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void smemGmInit2
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    GT_U32 regData;

    /* done after processing of the registers file */

    /* Read revision from 0x4c Register */
    smemGMRWMem(SCIB_MEMORY_READ_E, deviceObjPtr, 0x4c, 1, &regData);
    deviceObjPtr->deviceRevisionId = regData & 0xf;

    if (deviceObjPtr->supportCpuInternalInterface)
    {
        /* Set the MBus registers according to info also got from the 'registers' file */
        smemChtInitPciRegistres(deviceObjPtr);
    }

}

/*******************************************************************************
*   smemGMRWMem
*
* DESCRIPTION:
*       Return pointer to the register's or tables's memory.
*
* INPUTS:
*       accessType     - Read or Write flag
*       deviceObjPtr   - pointer to device object.
*       address        - address of memory(register or table).
*       memSize        - size of memory.
*       memPtr         - pointer to memory.
*
* OUTPUTS:
*
*
* RETURNS:
*        pointer to the memory location
*        NULL - if memory not exist
*
* COMMENTS:
*
*
*******************************************************************************/
static void smemGMRWMem
(
    IN SCIB_MEMORY_ACCESS_TYPE accessType,
    IN void * deviceObjPtr,
    IN GT_U32 address,
    IN GT_U32 memSize,
    INOUT GT_U32 * memPtr
)
{

    SKERNEL_DEVICE_OBJECT   * kernelDevObjPtr;  /* pointer to a deviceObject*/
    GT_U32                  * commonMemPtr;
    GT_BOOL                 decodeAddress;      /* need to decode address */
    GT_U32                  regAddr;            /* register address */
    GT_U32                  addrCompletionRegion; /* Adress compeltion region index */
    GT_BOOL                 unitInUse;          /* unit is in use */
    GT_U32                  addressMask;        /* mask of address part in PCI window*/
    GT_BOOL                 isSip5Dev;          /* GT_TRUE - SIP5 device, GT_FALSE - other */

    if (deviceObjPtr == 0)
    {
        skernelFatalError("smemGMRWMem: illegal pointer \n");
    }

    /* check device type and call device search memory function*/
    kernelDevObjPtr = (SKERNEL_DEVICE_OBJECT *)deviceObjPtr;

    if ((address & 0x3) != 0)
    {
        if(kernelDevObjPtr->supportAnyAddress == GT_FALSE)
        {
            skernelFatalError("smemGMRWMem : illegal address: [0x%8.8x] ((address& 0x3) != 0)\n",address);
        }
    }

    while(gmInitDone==0)
    {
        printf(".");
        /* wait for  the GM init to be done (and ready for memory access) */
        SIM_OS_MAC(simOsSleep)(100);
    }

    if(IS_DFX_OPERATION_MAC(accessType))
    {
        /* dummy ... the GM not implement the DFX memories */
        if(IS_WRITE_OPERATION_MAC(accessType))
        {
            return;
        }
        else
        {
            *memPtr = GM_BAD_ADDR_CNS;
            return;
        }
    }

    if (kernelDevObjPtr->gmDeviceType != GOLDEN_MODEL)
    {
        switch (accessType)
        {

        case SCIB_MEMORY_WRITE_PCI_E:
            pciConfigWrite(kernelDevObjPtr->deviceId , address,
                           (GT_U8 *)memPtr ,  sizeof(GT_U32) * memSize);
            break;

        case SCIB_MEMORY_READ_PCI_E:
            pciConfigRead(kernelDevObjPtr->deviceId ,address,
                            (GT_U8 *)memPtr,  sizeof(GT_U32) * memSize);
            break;

        case SCIB_MEMORY_WRITE_E:
            pciWrite(kernelDevObjPtr->deviceId, address,
                    (GT_U8 *)memPtr, sizeof(GT_U32) * memSize );
            break;

        case SCIB_MEMORY_READ_E:
            pciRead(kernelDevObjPtr->deviceId, address,
                    (GT_U8 *)memPtr, sizeof(GT_U32) * memSize );
            break;

        default:
            skernelFatalError(" smemGMRWMem: not valid mode[%d]",
                                accessType);
         break;

        }
    }
    else
    {
        /* memory mutex lock is done in SCIB level */

        /* relevant to gmDeviceType == GOLDEN_MODEL */
        isSip5Dev = SMEM_CHT_IS_SIP5_GET(kernelDevObjPtr);

        /* decoding state machine works alwasys */
        /* to be ready to the binding of        */
        /* cpssDxChHwIsUnitUsedFuncPtr          */
        switch (accessType)
        {
            case SCIB_MEMORY_WRITE_E:
                decodeAddress = GT_TRUE;
                if (kernelDevObjPtr->addressCompletionType ==
                    SKERNEL_ADDRESS_COMPLETION_TYPE_4_REGIONS_E)
                {
                    addressMask = 0x03FFFFFF;
                }
                else
                {
                    /*SKERNEL_ADDRESS_COMPLETION_TYPE_8_REGIONS_E*/
                    addressMask = 0x003FFFFF;
                }
                /* address completion for 4-region mode */
                if ((address & addressMask) == 0)
                {
                    /* this is access to address completion register */
                    /* save into the DB the new content, so we don't need to
                       get it from the GM */
                    kernelDevObjPtr->addressCompletionContent = *memPtr;
                }

                if (isSip5Dev == GT_TRUE)
                {
                    /* address completion for 8-region mode 0x120-0x13C */
                    if ((address & (addressMask & 0xFFFFFFE0)) == 0x120)
                    {
                        addrCompletionRegion = ((address & 0x1C) >> 2);
                        kernelDevObjPtr->addressCompletionBaseShadow[
                            addrCompletionRegion] = *memPtr;
                    }
                    else if ((address & addressMask) == 0x140)
                    {
                        /* switching between 4-region aqnd 8-region modes */
                        if (((*memPtr) & (1 << 16)) == 0)
                        {
                            kernelDevObjPtr->addressCompletionType =
                                SKERNEL_ADDRESS_COMPLETION_TYPE_8_REGIONS_E;
                        }
                        else
                        {
                            kernelDevObjPtr->addressCompletionType =
                                SKERNEL_ADDRESS_COMPLETION_TYPE_4_REGIONS_E;
                        }
                    }
                }
                break;
            case SCIB_MEMORY_READ_E:
                decodeAddress = GT_TRUE;
                break;
            default:
                decodeAddress = GT_FALSE;
                break;
        }

       /* Avoid memory access to GM for not supported units */
        if (kernelDevObjPtr->cpssDxChHwIsUnitUsedFuncPtr)
        {
            if(decodeAddress)
            {
                if (kernelDevObjPtr->addressCompletionType ==
                    SKERNEL_ADDRESS_COMPLETION_TYPE_4_REGIONS_E)
                {
                    addrCompletionRegion = (address >> 24) & 0x3;
                }
                else
                {
                    addrCompletionRegion = (address >> 19) & 0x7;
                }
                if(addrCompletionRegion != 0)
                {
                    if (kernelDevObjPtr->addressCompletionType ==
                        SKERNEL_ADDRESS_COMPLETION_TYPE_4_REGIONS_E)
                    {
                        /* build address from the unitIndex from addrComplition , by shift 23 bits */
                        regAddr = (SMEM_U32_GET_FIELD(
                            kernelDevObjPtr->addressCompletionContent,
                            (8 * addrCompletionRegion), 8)) << 24;
                    }
                    else
                    {
                        regAddr =
                            kernelDevObjPtr->addressCompletionBaseShadow[addrCompletionRegion]
                             << 19;
                    }

                    unitInUse = kernelDevObjPtr->cpssDxChHwIsUnitUsedFuncPtr(
                        kernelDevObjPtr->cpssDevNum, 0, regAddr);

                    if(unitInUse == GT_FALSE)
                    {
                        if(accessType == SCIB_MEMORY_READ_E)
                        {
                            *memPtr = GM_BAD_ADDR_CNS;
                            return;
                        }
                        else
                        {
                            return;
                        }
                    }
                }
            }
        }


        /* Try to find address in comon memory first */
        commonMemPtr = smemFindCommonMemory(kernelDevObjPtr, address, memSize, accessType , NULL/*not support active memories-- not needed yet*/);
        if(commonMemPtr)
        {
            /* memory found we can write/read to/from it (not support ‘active memory’) */
            switch (accessType)
            {
                case SCIB_MEMORY_WRITE_PCI_E:
                case SCIB_MEMORY_WRITE_E:
                    *commonMemPtr = *memPtr;
                    break;
                case SCIB_MEMORY_READ_PCI_E:
                case SCIB_MEMORY_READ_E:
                    *memPtr  = *commonMemPtr;
                    break;
                default:
                  skernelFatalError(" smemGMRWMem: not valid mode[%d]",
                                       accessType);
            }

            return;
        }

        lastRegisterAccessed = address;

        switch (accessType)
        {
        case SCIB_MEMORY_WRITE_PCI_E:
            ppPciConfigWrite(SMEM_GM_GET_GM_DEVICE_ID(kernelDevObjPtr),
                             kernelDevObjPtr->portGroupId,address,
                             (GT_U8 *)memPtr, sizeof(GT_U32) *memSize);
            break;

        case SCIB_MEMORY_READ_PCI_E:
            ppPciConfigRead(SMEM_GM_GET_GM_DEVICE_ID(kernelDevObjPtr),
                            kernelDevObjPtr->portGroupId,address,
                            (GT_U8 *)memPtr, sizeof(GT_U32) * memSize);
            break;

        case SCIB_MEMORY_WRITE_E:
            ppPciWrite(SMEM_GM_GET_GM_DEVICE_ID(kernelDevObjPtr),
                       kernelDevObjPtr->portGroupId,address,
                       (GT_U8 *)memPtr, sizeof(GT_U32) * memSize);
            break;

        case SCIB_MEMORY_READ_E:
            ppPciRead(SMEM_GM_GET_GM_DEVICE_ID(kernelDevObjPtr),
                      kernelDevObjPtr->portGroupId,address,
                      (GT_U8 *)memPtr, sizeof(GT_U32) * memSize);
            break;
         default:
            skernelFatalError(" smemGMRWMem: not valid mode[%d]",
                                accessType);
            break;
        }

        lastRegisterAccessed = NOT_VALID_ADDRESS;

        /* memory mutex unlock is done in SCIB level */
    }

    return ;
}

/*******************************************************************************
*   smemGmCreateInterruptTask
*
* DESCRIPTION:
*       Initiate interrupt task pooling
*
* INPUTS:
*       deviceID   - ID of device.
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
static void smemGmCreateInterruptTask
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    GT_TASK_HANDLE          taskHandl;        /* task handle */
    GT_TASK_PRIORITY_ENT    taskPriority;

    taskPriority = GT_TASK_PRIORITY_HIGHEST;
    taskHandl = SIM_OS_MAC(simOsTaskCreate)(
                                taskPriority,
                                (unsigned (__TASKCONV *)(void*))smemGMTask,
                                (void *) deviceObjPtr);

    if (taskHandl == NULL)
    {
        skernelFatalError(" smemGmCreateInterruptTask: cannot create pooling task\
                            for device %u",deviceObjPtr->deviceId);
    }
}


/*******************************************************************************
*   smemGMTask
*
* DESCRIPTION:
*       Initiate interrupt task pooling
*
* INPUTS:
*       deviceID   - pointer to device object.
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
static void smemGMTask
(
    IN void * deviceObjPtr
)
{
    GT_U32      retValue;
    GT_U32      address=0x114;
    SKERNEL_DEVICE_OBJECT * deviceObj = (SKERNEL_DEVICE_OBJECT *)deviceObjPtr;
    GT_U32      devId = deviceObj->deviceId;
    typedef unsigned int (*pciReadPtrType) (unsigned int uDeviceId,
                                        unsigned int uAddress,
                                        unsigned char* retValue,
                                        unsigned int size);
    pciReadPtrType  pciReadPtr;

    if (SKERNEL_DEVICE_FAMILY_TIGER(deviceObj->deviceType))
    {
        address = 0x114;
        pciReadPtr = pciConfigRead;
    }
    else
    {
         address = 0x30;
         pciReadPtr = pciRead;
    }

    while (GT_TRUE)
    {
        pciReadPtr(devId, address, (GT_U8 *)&retValue, sizeof(GT_U32) );
        if (retValue & 0x1)
        {
            scibSetInterrupt(devId);
        }

        SIM_OS_MAC(simOsSleep)(1000);
    }
}

/*******************************************************************************
*   smemGmUnitUsedFuncInit
*
* DESCRIPTION:
*       Init unit memory use checking function
*
* INPUTS:
*       simDeviceId     - Simulation device ID.
*       cpssDevNum      - CPSS device ID.
*       unitUsedFuncPtr - CPSS unit memory use function.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*       Relevant for Bobcat2 GM
*
*******************************************************************************/
void smemGmUnitUsedFuncInit
(
    IN GT_U32       simDeviceId,
    IN GT_U32       cpssDevNum,
    IN SMEM_GM_HW_IS_UNIT_USED_PROC_FUN unitUsedFuncPtr
)
{
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr;

    deviceObjPtr = smemTestDeviceIdToDevPtrConvert(simDeviceId);
    /* Save data in object DB */
    deviceObjPtr->cpssDxChHwIsUnitUsedFuncPtr = unitUsedFuncPtr;
    deviceObjPtr->cpssDevNum = cpssDevNum;
}
