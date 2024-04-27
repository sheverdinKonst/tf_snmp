/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simLogToRuntime.c
*
* DESCRIPTION:
*       This file hold engine of read LOG file and act according to the info:
*       1. do 'write from CPU'
*       2. do 'Read from CPU' (may effect active memory)
*       3. inject packets from the SLAN
*
*
*       NOTEs:
*       a. packets from the CPU are ignored as those are byproduct of 'write from CPU'
*       b. packet from 'loopback' ports are ignored because are byproduct of original packet (slan/CPU)
*
*
* FILE REVISION NUMBER:
*       $Revision: 5 $
*
*******************************************************************************/
#include <os/simTypesBind.h>
#include <stdarg.h>
#include <stdio.h>
#include <asicSimulation/SCIB/scib.h>
#include <asicSimulation/SInit/sinit.h>
#include <asicSimulation/SLog/simLog.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SLog/simLogInfoTypePacket.h>

#ifndef NO_GM_USED
static GT_BOOL gmLogSupport = GT_TRUE;
#else
static GT_BOOL gmLogSupport = GT_FALSE;
#endif

/* is the log to runtime active */
GT_BIT simLogToRuntimeIsActive = 0;

/****************** private vars **********************/
/* the pointer to the LOG file that we read from */
static FILE* readFromLogFilePointer = NULL;
/* the pointer to the LOG file that we create for removing read/write CPU/DEV operations  */
static FILE* newLogFilePointer = NULL;


typedef enum{
    /* read/write from CPU */
    DATA_TYPE_WRITE_FROM_CPU_E,
    DATA_TYPE_READ_FROM_CPU_E,

    /* slan INFO */
    DATA_TYPE_START_SLAN_PACKET_E,
    DATA_TYPE_MID_SLAN_PACKET_E,
    DATA_TYPE_END_SLAN_PACKET_E,

    /* thread Type */
    DATA_TYPE_THREAD_TYPE_E,

    /* DMA operation */
    DATA_TYPE_DMA_OPERATION_E,

    /* skernelNumOfPacketsInTheSystem */
    DATA_TYPE_NUM_OF_PACKETS_IN_THE_SYSTEM_E,

    /* port link status changed */
    DATA_TYPE_PORT_LINK_STATUS_CHANGED_E,

    /* slan bind */
    DATA_TYPE_SLAN_BIND_E,
    /* slan unbind */
    DATA_TYPE_SLAN_UNBIND_E,

    /* others */
    DATA_TYPE_OTHERS_E


}DATA_TYPE_ENT;

/* write    CPU   0     0 0x032C0020 0x00000003 0x00000003 000000000*/
typedef struct{
    GT_U32  devNum;
    GT_U32  coreId;
    GT_U32  registerAddress;
    GT_U32  registerValue;
}DATA_TYPE_WRITE_FROM_CPU_STC;
/*  read    CPU   0     0 0x03280040 0x00000000         NA        NA*/
typedef struct{
    GT_U32  devNum;
    GT_U32  coreId;
    GT_U32  registerAddress;
}DATA_TYPE_READ_FROM_CPU_STC;


/*ingress packet from SLAN[slan05]*/
/*dump packet : device[0] core[0] ingress port [0] byteCount[64]*/
typedef struct{
    GT_U32  devNum;
    GT_U32  coreId;
    GT_U32  portNum;
    GT_U32  byteCount;
}DATA_TYPE_START_SLAN_PACKET_STC;

/*thread type*/
typedef struct{
    char       threadTypeName[50];
}DATA_TYPE_THREAD_TYPE_STC;


/*DMA operation*/
typedef struct{
    GT_BOOL     doWrite;/*GT_TRUE - read, GT_FALSE - write */
    GT_U32      numOfWords;
    GT_U32      startAddress;
    /* for read operation this hold the values that will be read from the DMA */
    /* for write operation this hold the values that wrote to the DMA (according to the LOG) */
    GT_U32      dmaWordsArr[SBUF_DATA_SIZE_CNS / 4];
}DATA_TYPE_DMA_OPERATION_STC;

/* Port Link Status Info */
typedef struct{
    GT_U32  devNum;
    GT_U32  coreId;
    GT_U32  port;
    GT_U32  linkStatus;
}DATA_TYPE_PORT_LINK_STATUS_STC;

/* slan bind Info */
typedef struct{
    GT_BOOL                      isBindOrUnbind;/*GT_TRUE - bind , GT_FALSE - unbind*/
    char                         slanNamePtr[SMAIN_SLAN_NAME_SIZE_CNS];/*not relevant in unbind*/
    GT_U32                       devNum;
    GT_U32                       coreId;
    GT_U32                       portNumber;
    GT_BOOL                      bindRx;
    GT_BOOL                      bindTx;
}DATA_TYPE_SLAN_BIND_STC;

static DATA_TYPE_ENT currentLineDataType = DATA_TYPE_OTHERS_E;
static DATA_TYPE_WRITE_FROM_CPU_STC      currentLineWriteFromCpuInfo;
static DATA_TYPE_READ_FROM_CPU_STC       currentLineReadFromCpuInfo;
static DATA_TYPE_START_SLAN_PACKET_STC   currentLineStartSlanPacketInfo;
static DATA_TYPE_THREAD_TYPE_STC         currentThreadTypeInfo;
static DATA_TYPE_DMA_OPERATION_STC       currentDmaOperationInfo;

static DATA_TYPE_PORT_LINK_STATUS_STC    currentPortLinkStatusInfo;
static DATA_TYPE_SLAN_BIND_STC           currentSlanBindInfo;

/* buffer for current line read from file */
#define SIZE_OF_ORIG_LINE_READ_FROM_FILE_CNS    1024
static char origLineReadFromFile[SIZE_OF_ORIG_LINE_READ_FROM_FILE_CNS];
static char *currentLineReadFromFile = &origLineReadFromFile[0];
static GT_U32 currentLineReadFromFileLen;

#define WRITE_CPU_STR                   "write    CPU"
static char* write_cpu_str = WRITE_CPU_STR;
static GT_U32 write_cpu_str_len;
#define READ_CPU_STR                    "read    CPU"
static char* read_cpu_str = READ_CPU_STR;
static GT_U32 read_cpu_str_len;
#define INGRESS_PACKET_FROM_SLAN_START_STR          SIM_LOG_INGRESS_PACKET_FROM_SLAN_STR
static char* ingress_packet_from_slan_start_str = INGRESS_PACKET_FROM_SLAN_START_STR;
static GT_U32 ingress_packet_from_slan_start_str_len;
/*0x0000 : 00 07 77 77 77 77 00 05 55 55 55 55 00 00 00 00*/
#define INGRESS_PACKET_FROM_SLAN_ENDED_STR          SIM_LOG_END_OF_PACKET_DUMP_STR
static char* ingress_packet_from_slan_ended_str = INGRESS_PACKET_FROM_SLAN_ENDED_STR;
static GT_U32 ingress_packet_from_slan_ended_str_len;

#define THREAD_TYPE_STR                    "taskType["
static char* thread_type_str = THREAD_TYPE_STR;
static GT_U32 thread_type_str_len;

#define DMA_OPERATION_STR       SIM_LOG_DMA_OPERATION_STR
static char* dma_operation_str = DMA_OPERATION_STR;
static GT_U32 dma_operation_str_len;


static char*  port_link_status_changed_prefix_str = SIM_LOG_PORT_LINK_STATUS_CHANGED_PREFIX_STR;
static GT_U32 port_link_status_changed_prefix_str_len = 0;

static char*  slan_bind_prefix_str =    SIM_LOG_SLAN_BIND_PREFIX_STR;
static GT_U32 slan_bind_prefix_str_len = 0;

static char*  slan_unbind_prefix_str =  SIM_LOG_SLAN_UNBIND_PREFIX_STR;
static GT_U32 slan_unbind_prefix_str_len = 0;



static char* ready_to_log_str = SIM_LOG_READY_TO_START_LOG_STR;
static GT_U32 ready_to_log_str_len;
static GT_U32 ready_to_log = 0;

/*[(skernelNumOfPacketsInTheSystem)] = [0x0]*/
#define LOG_INFO_NUM_OF_PACKETS_IN_THE_SYSTEM_STR   "[(skernelNumOfPacketsInTheSystem)] = ["
static char*   logInfoNumOfPacketsInTheSystemStr = LOG_INFO_NUM_OF_PACKETS_IN_THE_SYSTEM_STR;
static GT_U32  logInfoNumOfPacketsInTheSystemStrLen;
static GT_U32  logInfoNumOfPacketsInTheSystem = 0;

/* deviceDmaAnswer_ready indication from the parser to the 'dma read/write' that the info is ready */
static GT_U32   deviceDmaAnswer_ready = 0;
/*deviceDmaAnswer_waiting indication that the device wait for DMA read/write info   */
static GT_U32   deviceDmaAnswer_waiting = 0;
/* deviceDmaAnswer_finishedProcessing indication from the 'read read/write' to te parser that it got the info set by parser */
static GT_U32   deviceDmaAnswer_finishedProcessing = 0;

static GT_U32  totalNumOfLines = 0;
static SIM_OS_TASK_PURPOSE_TYPE_ENT  currentThreadType = SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_GENERAL_PURPOSE_E;

/* needed for 'fseek' , this value comes from 'ftell' */
/* this is the current position in the file that the access from the CPU halted
    until the device will finish a current packets in the system */
static GT_U32   currentPositionOfStopCpuProcessing_ftell = 0;
/* indication that currentPositionOfStopCpuProcessing_ftell is ready */
static GT_U32   currentPositionOfStopCpuProcessing_ready = 0;
/* the line number that we got to when decided to do ftell */
static GT_U32   currentPositionOfStopCpuProcessing_lineNum = 0;
/* the line number that we got to when decided to do ftell */
static SIM_OS_TASK_PURPOSE_TYPE_ENT   currentPositionOfStopCpuProcessing_currentThreadType = 0;


/* indication that under 'catch up' of the 'CPU operations' that were delayed
   due to 'device processing of packet' */
static GT_U32   currentPositionOfStopCpuProcessing_doCatchUp = 0;
/* the line number that we got to when decided to start 'catch up'*/
static GT_U32   currentPositionOfStopCpuProcessing_doCatchUp_maxLine = 0;


static char* read_dev_str = "read    DEV";
static GT_U32 read_dev_str_len = 0;
static char* write_dev_str = "write    DEV";
static GT_U32 write_dev_str_len = 0;

static char* packet_from_cpu_str = SIM_LOG_INGRESS_PACKET_FROM_CPU_DMA_STR;
static GT_U32 packet_from_cpu_str_len = 0;

static char* packet_from_cpu_ended_str = SIM_LOG_INGRESS_PACKET_FROM_CPU_DMA_ENDED_STR;
static GT_U32 packet_from_cpu_ended_str_len = 0;


#define HANDLE_ALL_PACKETS  0xFFFFFFFF
/* NOTE: this value is never set to 0 .
    1 based number or 0xFFFFFFFF
*/
static GT_U32   firstPacketInstanceToHandle = HANDLE_ALL_PACKETS;
static GT_U32   numPacketsAlreadyProcessed = 0;
static GT_U32   stopLogParsingDueToPacketInstance = 0;

/* buffer to hold the packet from the slan */
static GT_U8    slanPacketBuffer[SBUF_DATA_SIZE_CNS];
static GT_U32   slanPacketCurrentIndex = 0;

#define SPACE_CHAR_CNS  0x20

/* check if current line is 'empty' ,
   also remove leading special characters (include spaces) in start of line */
static GT_BOOL isEmptyLine(void)
{
    GT_U32  ii;

    for(ii = 0 ; ii < currentLineReadFromFileLen; ii++)
    {
        if(currentLineReadFromFile[ii] > SPACE_CHAR_CNS)
        {
            /* jump the line pointer after all those special characters */
            currentLineReadFromFile += ii;

            /* all the special chars are less then 32 decimal (0x20) */
            return GT_FALSE;
        }
    }

    /*line with spaces or just spaces ... */
    return GT_TRUE;
}


/* read next line , return GT_FALSE of error/EOF */
static GT_BOOL readNextLine(void)
{
    GT_U32  maxCompare;
    char* dummyReadLinePtr;

getNextLine_lbl:
    totalNumOfLines++;

    dummyReadLinePtr = fgets( origLineReadFromFile,
                              SIZE_OF_ORIG_LINE_READ_FROM_FILE_CNS,
                              readFromLogFilePointer);

    currentLineReadFromFile = origLineReadFromFile;
    if(dummyReadLinePtr)
    {
        /* debug only */
        simGeneralPrintf( "%s", currentLineReadFromFile);

        currentLineReadFromFileLen = (GT_U32)strlen(currentLineReadFromFile);

        while(GT_TRUE == isEmptyLine())
        {
            if(GT_FALSE == readNextLine())
            {
                /* no more lines to read */
                return GT_FALSE;
            }
        }

        maxCompare = MIN(currentLineReadFromFileLen ,ready_to_log_str_len);
        if(maxCompare && 0 == strncmp(currentLineReadFromFile,ready_to_log_str,maxCompare))
        {
            /* the task type also have '\n' before it */
            ready_to_log = 1;
        }

        if(ready_to_log == 0)
        {
            /* the LOG file must contain this string !!! */
            goto getNextLine_lbl;
        }


        return GT_TRUE;
    }
    else
    {
        return GT_FALSE;
    }
}

/*  check the state of current operations that we allow to do.

    the read/write operations from the CPU need to wait while the device process any packet
    otherwise the CPU may continue with configurations that belong to issues of 'restore'
    or issues of 'check counters' or reconfiguration for 'before the next packet'


    the devices operations (slan/dma) need to be skipped while we
    'do catch up with CPU operations'

*/
static void checkStateOfAllowedOperations (void)
{
    DECLARE_FUNC_NAME(snetChtPerformFromCpuDma);
    static SKERNEL_DEVICE_OBJECT * devObjPtr = NULL;/*just for __LOG(()) macro*/

    if(skernelNumOfPacketsInTheSystemGet())
    {
        /* once we understand that the device is processing any packet , we should
           not let CPU continue to do operations */

        if(currentPositionOfStopCpuProcessing_ready == 0)
        {
            currentPositionOfStopCpuProcessing_doCatchUp = 0;
            currentPositionOfStopCpuProcessing_ready = 1;
            currentPositionOfStopCpuProcessing_ftell = ftell(readFromLogFilePointer);
            __LOG(("ftell - currentPositionOfStopCpuProcessing_ftell : [%d] \n",
                currentPositionOfStopCpuProcessing_ftell));

            /* save current line */
            currentPositionOfStopCpuProcessing_lineNum = totalNumOfLines;
            /* save current thread name  */
            currentPositionOfStopCpuProcessing_currentThreadType = currentThreadType;

            __LOG_PARAM(currentPositionOfStopCpuProcessing_lineNum);
        }
    }
    else
    if (currentPositionOfStopCpuProcessing_ready)
    {
        /* there are no more packets in the system , so we can do catch up for
           the 'CPU operations' , that we skipped during device processing of
           the packets */
        currentPositionOfStopCpuProcessing_doCatchUp = 1;
        currentPositionOfStopCpuProcessing_ready = 0;

        /* save the current line number */
        currentPositionOfStopCpuProcessing_doCatchUp_maxLine = totalNumOfLines;

        /* restore totalNumOfLines that was when ftell done */
        totalNumOfLines = currentPositionOfStopCpuProcessing_lineNum;
        /* restore currentThreadType that was when ftell done */
        currentThreadType = currentPositionOfStopCpuProcessing_currentThreadType;

        SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)(currentThreadType,NULL);

        if(totalNumOfLines == 0)
        {
            skernelFatalError("checkStateOfAllowedOperations: totalNumOfLines == 0 \n");
        }
        else
        {
            totalNumOfLines --;
        }

        __LOG(("start catch up from line[%d] (was at line[%d]) \n",
            totalNumOfLines,currentPositionOfStopCpuProcessing_doCatchUp_maxLine));

        __LOG(("fseek(readFromLogFilePointer,currentPositionOfStopCpuProcessing_ftell[%d],SEEK_SET) \n",
            currentPositionOfStopCpuProcessing_ftell));
        fseek(readFromLogFilePointer,currentPositionOfStopCpuProcessing_ftell,SEEK_SET);

        if(GT_FALSE == readNextLine())
        {
            skernelFatalError("checkStateOfAllowedOperations: fail to read \n");
        }
    }
    else
    if(currentPositionOfStopCpuProcessing_doCatchUp)
    {
        /* the catch up is until we get to the line that jumped back from it */
        if(currentPositionOfStopCpuProcessing_doCatchUp_maxLine == totalNumOfLines)
        {
            __LOG(("End catch up at line[%d] \n",
                totalNumOfLines));

            /* state that the catch up ended */
            currentPositionOfStopCpuProcessing_doCatchUp = 0;
        }
    }

    return;
}

/* parse write operation by the CPU */
static void parseWriteCpu(void)
{
                         /*0      0 0x050000F8 0x00000000*/
    sscanf(currentLineReadFromFile,
        WRITE_CPU_STR " %d %d %x %x",
        &currentLineWriteFromCpuInfo.devNum,
        &currentLineWriteFromCpuInfo.coreId,
        &currentLineWriteFromCpuInfo.registerAddress,
        &currentLineWriteFromCpuInfo.registerValue);
}

/* parse read operation by the CPU */
static void parseReadCpu(void)
{
                         /*0      0 0x050000F8 */
    sscanf(currentLineReadFromFile,
        READ_CPU_STR " %d %d %x",
        &currentLineReadFromCpuInfo.devNum,
        &currentLineReadFromCpuInfo.coreId,
        &currentLineReadFromCpuInfo.registerAddress);
}

static void parseFromSlan(void)
{
    GT_U32  byteCount;
    GT_U32  ii;
    GT_U32  numOfLinesWithPacketInfo;
    GT_U32  dummyStartByteIndex;
    /* we are now in the line "ingress packet from SLAN[" */
    /* we not care about this line ! as the name of the SLAN is not relevant */
    /* we need the next line to get the device,port .*/

    /*read next line*/
    if(GT_FALSE == readNextLine())
    {
        skernelFatalError("parseFromSlan: bad format 1 \n");
        return;
    }

    sscanf(currentLineReadFromFile,
        "dump packet : device[%d] core[%d] ingress port [%d] byteCount[%d]",
        &currentLineStartSlanPacketInfo.devNum,
        &currentLineStartSlanPacketInfo.coreId,
        &currentLineStartSlanPacketInfo.portNum,
        &currentLineStartSlanPacketInfo.byteCount);

    byteCount = currentLineStartSlanPacketInfo.byteCount;

    if(byteCount >= SBUF_DATA_SIZE_CNS)
    {
        skernelFatalError("parseFromSlan: byteCount[%d] > [%d] \n",
            byteCount,
            SBUF_DATA_SIZE_CNS);
        return;
    }

    /*read next line*/
    if(GT_FALSE == readNextLine())
    {
        skernelFatalError("parseFromSlan: bad format 2 \n");
        return;
    }

    /* 16 bytes of packet in lines */
    numOfLinesWithPacketInfo = ((byteCount + 15) / 16);

    slanPacketCurrentIndex = 0;
    for(ii = 0 ; ii < numOfLinesWithPacketInfo ; ii++)
    {
        sscanf(currentLineReadFromFile,
            "%x : %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
            &dummyStartByteIndex,
            &slanPacketBuffer[ 0 + slanPacketCurrentIndex],
            &slanPacketBuffer[ 1 + slanPacketCurrentIndex],
            &slanPacketBuffer[ 2 + slanPacketCurrentIndex],
            &slanPacketBuffer[ 3 + slanPacketCurrentIndex],
            &slanPacketBuffer[ 4 + slanPacketCurrentIndex],
            &slanPacketBuffer[ 5 + slanPacketCurrentIndex],
            &slanPacketBuffer[ 6 + slanPacketCurrentIndex],
            &slanPacketBuffer[ 7 + slanPacketCurrentIndex],
            &slanPacketBuffer[ 8 + slanPacketCurrentIndex],
            &slanPacketBuffer[ 9 + slanPacketCurrentIndex],
            &slanPacketBuffer[10 + slanPacketCurrentIndex],
            &slanPacketBuffer[11 + slanPacketCurrentIndex],
            &slanPacketBuffer[12 + slanPacketCurrentIndex],
            &slanPacketBuffer[13 + slanPacketCurrentIndex],
            &slanPacketBuffer[14 + slanPacketCurrentIndex],
            &slanPacketBuffer[15 + slanPacketCurrentIndex]
            );

        if(slanPacketCurrentIndex != dummyStartByteIndex)
        {
            skernelFatalError("parseFromSlan: bad format 3 \n");
            return;
        }

        slanPacketCurrentIndex += 16;

        if(GT_FALSE == readNextLine())
        {
            skernelFatalError("parseFromSlan: bad format 4 \n");
            return;
        }
    }

    /* check that current line is "end of packet dump" */
    if(0 != strncmp(currentLineReadFromFile,
            ingress_packet_from_slan_ended_str,
            ingress_packet_from_slan_ended_str_len))
    {
        skernelFatalError("parseFromSlan: bad format 5 \n");
        return;
    }

    return;
}

static void parseThreadType(void)
{
    /*"taskType["*/
    sscanf(currentLineReadFromFile,
        THREAD_TYPE_STR " %s ]",
        currentThreadTypeInfo.threadTypeName);
}

/* the function called from the logger that wait for the device to finish the DMA operation */
static void parserWaitForDeviceForDmaProcess(void)
{
    while(deviceDmaAnswer_finishedProcessing == 0)
    {
        /* in the mean time we let the device finish the DMA operation.
           the device operate in function simLogToRuntime_scibDmaRead / simLogToRuntime_scibDmaWrite*/
        SIM_OS_MAC(simOsSleep)(50);
    }

    /* reset the value for next time */
    deviceDmaAnswer_finishedProcessing = 0;
}

static void parseDmaOperation(void)
{
    static char   dmaOperName[50];
    static char   clientName[50];
    GT_U32  ii;
    GT_U32  address , value;
    /*"DMA Operation : ["*/
    sscanf(currentLineReadFromFile,
        DMA_OPERATION_STR " %s ] the DMA by[ %s ] [%d] words:",
        dmaOperName,
        clientName,
        &currentDmaOperationInfo.numOfWords
        );

    if(0 == strcmp(dmaOperName,SIM_LOG_DMA_OPERATION_WRITE_TO_STR))
    {
        currentDmaOperationInfo.doWrite = GT_TRUE;
    }
    else
    if(0 == strcmp(dmaOperName,SIM_LOG_DMA_OPERATION_READ_FROM_STR))
    {
        currentDmaOperationInfo.doWrite = GT_FALSE;
    }
    else
    {
        skernelFatalError("parseDmaOperation: unknown operation  \n");
        return;
    }

    if(currentDmaOperationInfo.numOfWords >=
        (sizeof(currentDmaOperationInfo.dmaWordsArr) / sizeof(GT_U32)))
    {
        skernelFatalError("parseDmaOperation: numOfWords[%d]  \n",
            currentDmaOperationInfo.numOfWords);
        return;
    }

    for(ii = 0 ; ii < currentDmaOperationInfo.numOfWords; ii++)
    {
        if(GT_FALSE == readNextLine())
        {
            skernelFatalError("parseDmaOperation: bad format 1 \n");
            return;
        }

        /*"address[0x%8.8x] value[0x%8.8x]"*/
        sscanf(currentLineReadFromFile,
            "address[%x] value[%x]",
            &address , &value
            );

        if(ii == 0)
        {
            currentDmaOperationInfo.startAddress = address;
        }

        currentDmaOperationInfo.dmaWordsArr[ii] = value;
    }

    /* state that the info is ready for the other task */
    deviceDmaAnswer_ready = 1;

    /* wait for the device to finish the DMA operation */
    parserWaitForDeviceForDmaProcess();

}

/* get the number of packets in the system from the LOG */
static void parseNumOfPacketsInTheSystem(void)
{
    /*[(skernelNumOfPacketsInTheSystem)] = [0x0]*/
    sscanf(currentLineReadFromFile,
        LOG_INFO_NUM_OF_PACKETS_IN_THE_SYSTEM_STR "%x",
        &logInfoNumOfPacketsInTheSystem);
}

/* get info about the port link change from the LOG */
static void parsePortLinkStatusChanged(void)
{
    /*port link status changed : device[ 0 ] core[ 0 ], port[ 0 ] linkState[ 1 ]*/
    sscanf(currentLineReadFromFile,
        SIM_LOG_PORT_LINK_STATUS_CHANGED_PREFIX_STR
        SIM_LOG_PORT_LINK_STATUS_CHANGED_DETAILS_STR,
        &currentPortLinkStatusInfo.devNum,
        &currentPortLinkStatusInfo.coreId,
        &currentPortLinkStatusInfo.port,
        &currentPortLinkStatusInfo.linkStatus
        );
}

/* get the number of packets in the system from the LOG */
static void parseSlanBindOperation(void)
{
    if(currentLineDataType == DATA_TYPE_SLAN_BIND_E)
    {
        currentSlanBindInfo.isBindOrUnbind = GT_TRUE;/* bind*/

        sscanf(currentLineReadFromFile,
                    SIM_LOG_SLAN_BIND_PREFIX_STR           /*bind*/
                    SIM_LOG_SLAN_BIND_OPERATION_STR
                    SIM_LOG_SLAN_BIND_DETAILS_STR  "\n" ,
                    &currentSlanBindInfo.slanNamePtr,
                    &currentSlanBindInfo.devNum ,
                    &currentSlanBindInfo.coreId ,
                    &currentSlanBindInfo.portNumber ,
                    &currentSlanBindInfo.bindRx,
                    &currentSlanBindInfo.bindTx);
    }
    else
    {
        currentSlanBindInfo.isBindOrUnbind = GT_FALSE;/* unbind*/

        sscanf(currentLineReadFromFile,
                    SIM_LOG_SLAN_UNBIND_PREFIX_STR
                    SIM_LOG_SLAN_BIND_DETAILS_STR  "\n" ,
                    &currentSlanBindInfo.devNum ,
                    &currentSlanBindInfo.coreId ,
                    &currentSlanBindInfo.portNumber ,
                    &currentSlanBindInfo.bindRx,
                    &currentSlanBindInfo.bindTx);
    }
}




/* parse a line and set one of the next info :
    CPU operations (read/write)
    Device DMA operations (read/write)
    packet from SLAN
    thread type
    number of packets in the system

*/
static void parseCurrentLine(void)
{
    GT_U32  maxCompare;
    GT_BIT  allowCpuOperation;
    GT_BIT  allowDeviceOperation;

    currentLineDataType = DATA_TYPE_OTHERS_E;

    /* check the state of current operations */
    checkStateOfAllowedOperations();

    /* check if we allow device operations (Slan,DMA)*/
    allowDeviceOperation = currentPositionOfStopCpuProcessing_doCatchUp ? 0 : 1;
    /* check if we allow CPU operations (read/write registers) */
    allowCpuOperation    = currentPositionOfStopCpuProcessing_ready     ? 0 : 1;

    maxCompare = MIN(currentLineReadFromFileLen ,write_cpu_str_len);
    if(0 == strncmp(currentLineReadFromFile,write_cpu_str,maxCompare))
    {
        if(allowCpuOperation)
        {
            currentLineDataType = DATA_TYPE_WRITE_FROM_CPU_E;
            parseWriteCpu();
        }
        return;
    }

    maxCompare = MIN(currentLineReadFromFileLen ,read_cpu_str_len);
    if(0 == strncmp(currentLineReadFromFile,read_cpu_str,maxCompare))
    {
        if(allowCpuOperation)
        {
            currentLineDataType = DATA_TYPE_READ_FROM_CPU_E;
            parseReadCpu();
        }
        return;
    }

    maxCompare = MIN(currentLineReadFromFileLen ,ingress_packet_from_slan_start_str_len);
    if(0 == strncmp(currentLineReadFromFile,ingress_packet_from_slan_start_str,maxCompare))
    {
        if(allowDeviceOperation)
        {
            currentLineDataType = DATA_TYPE_START_SLAN_PACKET_E;
            parseFromSlan();
        }
        return;
    }

    maxCompare = MIN(currentLineReadFromFileLen ,thread_type_str_len);
    if(0 == strncmp(currentLineReadFromFile,thread_type_str,maxCompare))
    {
        currentLineDataType = DATA_TYPE_THREAD_TYPE_E;
        parseThreadType();
        return;
    }

    maxCompare = MIN(currentLineReadFromFileLen ,dma_operation_str_len);
    if(0 == strncmp(currentLineReadFromFile,dma_operation_str,maxCompare))
    {
        if(allowDeviceOperation)
        {
            currentLineDataType = DATA_TYPE_DMA_OPERATION_E;
            parseDmaOperation();
        }
        return;
    }

    maxCompare = MIN(currentLineReadFromFileLen ,logInfoNumOfPacketsInTheSystemStrLen);
    if(0 == strncmp(currentLineReadFromFile,logInfoNumOfPacketsInTheSystemStr,maxCompare))
    {
        currentLineDataType = DATA_TYPE_NUM_OF_PACKETS_IN_THE_SYSTEM_E;
        parseNumOfPacketsInTheSystem();
        return;
    }

    maxCompare = MIN(currentLineReadFromFileLen ,port_link_status_changed_prefix_str_len);
    if(0 == strncmp(currentLineReadFromFile,port_link_status_changed_prefix_str,maxCompare))
    {
        currentLineDataType = DATA_TYPE_PORT_LINK_STATUS_CHANGED_E;
        parsePortLinkStatusChanged();
        return;
    }

    maxCompare = MIN(currentLineReadFromFileLen ,slan_bind_prefix_str_len);
    if(0 == strncmp(currentLineReadFromFile,slan_bind_prefix_str,maxCompare))
    {
        currentLineDataType = DATA_TYPE_SLAN_BIND_E;
        parseSlanBindOperation();
        return;
    }

    maxCompare = MIN(currentLineReadFromFileLen ,slan_unbind_prefix_str_len);
    if(0 == strncmp(currentLineReadFromFile,slan_unbind_prefix_str,maxCompare))
    {
        currentLineDataType = DATA_TYPE_SLAN_UNBIND_E;
        parseSlanBindOperation();
        return;
    }


    return;
}

/* get device object from deviceId,coreId */
static SKERNEL_DEVICE_OBJECT *getDevicePtr(IN GT_U32  deviceId , IN GT_U32  coreId)
{
    GT_U32  ii,jj;
    SKERNEL_DEVICE_OBJECT * devObjPtr;

    static GT_U32   lastDeviceId = SMAIN_NOT_VALID_CNS;
    static GT_U32   lastCoreId = SMAIN_NOT_VALID_CNS;
    static SKERNEL_DEVICE_OBJECT * lastDevObjPtr = NULL;
    static GT_U32   lastIndex = 0;

    /* implement 'one level cache' */
    if(deviceId == lastDeviceId && coreId == lastCoreId)
    {
        if(lastDevObjPtr == NULL)
        {
            goto not_foundPtr_lbl;
        }
        return lastDevObjPtr;
    }
    else if(deviceId == lastDeviceId)
    {
        /* the core id changed, but the device is the same ... */
        ii = lastIndex;
    }
    else
    {
        ii = 0;
    }


    for(/*already initialized*/ ; ii < SMAIN_MAX_NUM_OF_DEVICES_CNS; ii++)
    {
        devObjPtr = smainDeviceObjects[ii];
        if(devObjPtr == NULL)
        {
            continue;
        }

        if(devObjPtr->deviceId == deviceId)
        {
            if(devObjPtr->shellDevice && devObjPtr->numOfCoreDevs)
            {
                for(jj = 0 ; jj < devObjPtr->numOfCoreDevs; jj++)
                {
                    if(devObjPtr->coreDevInfoPtr[jj].devObjPtr &&
                       devObjPtr->coreDevInfoPtr[jj].devObjPtr->portGroupId == coreId)
                    {
                        devObjPtr = devObjPtr->coreDevInfoPtr[jj].devObjPtr;
                        goto foundPtr_lbl ;
                    }
                }

                goto not_foundPtr_lbl;
            }
            else
            {
                goto foundPtr_lbl ;
            }
        }
    }

not_foundPtr_lbl:
    skernelFatalError("getDevicePtr: not found the device  \n");
    return NULL;

foundPtr_lbl:
    lastDevObjPtr = devObjPtr;
    lastDeviceId = deviceId;
    lastDeviceId = coreId;
    lastIndex = ii;

    return devObjPtr;
}

/* need to evaluate the write register line */
static void evaluateWriteRegister(void)
{
    SKERNEL_DEVICE_OBJECT *devObjPtr;

    devObjPtr = getDevicePtr(currentLineWriteFromCpuInfo.devNum,currentLineWriteFromCpuInfo.coreId);

    scibWriteMemory(devObjPtr->deviceId,
        currentLineWriteFromCpuInfo.registerAddress,
        1,
        &currentLineWriteFromCpuInfo.registerValue);
}

/* need to evaluate the read register line */
static void evaluateReadRegister(void)
{
    SKERNEL_DEVICE_OBJECT *devObjPtr;
    GT_U32  dummyValue;

    devObjPtr = getDevicePtr(currentLineReadFromCpuInfo.devNum,currentLineReadFromCpuInfo.coreId);

    scibReadMemory(devObjPtr->deviceId,
        currentLineReadFromCpuInfo.registerAddress,
        1,
        &dummyValue);
}

/* need to evaluate the slan packet lines */
static void evaluateSlanPacket(void)
{
    SKERNEL_DEVICE_OBJECT *devObjPtr;
    SBUF_BUF_ID            bufferId; /* buffer id */
    SBUF_BUF_STC           *dstBuf_PTR; /* pointer to the allocated buffer from the destination device pool*/
    GT_U32  dataSize = currentLineStartSlanPacketInfo.byteCount;
    GT_U8*  dataPrt  = &slanPacketBuffer[0];

    devObjPtr = getDevicePtr(currentLineStartSlanPacketInfo.devNum,currentLineStartSlanPacketInfo.coreId);

    /* allocate buffer from the 'destination' device pool */
    /* get the buffer and put it in the queue */
    bufferId = sbufAlloc(devObjPtr->bufPool, dataSize);
    if (bufferId == NULL)
    {
        __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK((" evaluateReadRegister : no buffers for process \n"));
        return ;
    }

    skernelNumOfPacketsInTheSystemSet(GT_TRUE);

    dstBuf_PTR = (SBUF_BUF_STC *) bufferId;
    /* use the SLAN type since this is generic packet */
    dstBuf_PTR->srcType = SMAIN_SRC_TYPE_INTERNAL_CONNECTION_E;
    /* set source port of buffer -- the port on the destination device */
    dstBuf_PTR->srcData = currentLineStartSlanPacketInfo.portNum;
    /* state 'regular' frame */
    dstBuf_PTR->dataType = SMAIN_MSG_TYPE_FRAME_E;

    /* copy the frame into the buffer */
    memcpy(dstBuf_PTR->actualDataPtr,dataPrt,dataSize);

    /* set the pointers and buffer */
    sbufDataSet(bufferId, dstBuf_PTR->actualDataPtr,dataSize);

    /* put buffer on the queue of the destination device */
    squeBufPut(devObjPtr->queueId ,SIM_CAST_BUFF(bufferId));

    return ;
}

static void evaluateThreadType(void)
{
    static char* threadTypeNames[SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_GENERAL____LAST__E] =
    {
         STR(CPU_APPLICATION)                 /*SIM_OS_TASK_PURPOSE_TYPE_CPU_APPLICATION_E                  */
        ,STR(CPU_ISR)                         /*SIM_OS_TASK_PURPOSE_TYPE_CPU_ISR_E                          */
        ,STR(PP_AGING_DAEMON)                 /*SIM_OS_TASK_PURPOSE_TYPE_PP_AGING_DAEMON_E                  */
        ,STR(PP_PIPE_PROCESSING_DAEMON)       /*SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_PROCESSING_DAEMON_E        */
        ,STR(PP_PIPE_SDMA_QUEUE_DAEMON)       /*SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_SDMA_QUEUE_DAEMON_E        */
        ,STR(PP_PIPE_OAM_KEEP_ALIVE_DAEMON)   /*SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_OAM_KEEP_ALIVE_DAEMON_E    */
        ,STR(PP_PIPE_GENERAL_PURPOSE)         /*SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_GENERAL_PURPOSE_E          */
        ,STR(PP_PIPE_SOFT_RESET_E)            /*SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_SOFT_RESET_E               */
    };
    GT_U32  ii;
    SIM_OS_TASK_PURPOSE_TYPE_ENT threadType = SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_GENERAL_PURPOSE_E;

    for(ii = 0 ; ii < SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_GENERAL____LAST__E ; ii++)
    {
        if(0 == strcmp(threadTypeNames[ii],currentThreadTypeInfo.threadTypeName))
        {
            /* found match */
            threadType = ii;
            break;
        }
    }

    currentThreadType = threadType;

    SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)(threadType,NULL);
}

/*evaluate the DMA read/write operation*/
static void evaluateDmaOperation(
    IN GT_U32 deviceId,
    IN GT_BOOL doWrite,
    IN GT_U32  numOfWords,
    IN GT_U32  startAddress,
    IN GT_U32 * memPtr
)
{
    GT_U32  ii;

    if(doWrite != currentDmaOperationInfo.doWrite)
    {
        skernelFatalError("evaluateDmaOperation: mismatch between log and actual operation type \n");
        return;
    }
    if(numOfWords != currentDmaOperationInfo.numOfWords)
    {
        skernelFatalError("evaluateDmaOperation: mismatch between log and actual num of words \n");
        return;
    }
#if 0 /*be robust to DMA addresses as the DMA addresses are 'compilation depend' and different between 2 images that build from different baselines*/
    if(startAddress != currentDmaOperationInfo.startAddress)
    {
        skernelFatalError("evaluateDmaOperation: mismatch between log and start Address \n");
        return;
    }
#endif
    if(doWrite == GT_FALSE)/* read DMA */
    {
        for(ii = 0 ; ii < numOfWords; ii++)
        {
            memPtr[ii] = currentDmaOperationInfo.dmaWordsArr[ii];
        }
    }
    else
    {
        /* we can to ignore the DMA write  */
        __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("Ignore the DMA write by dev[%d] , when 'LOG parser' mode \n",
            deviceId));
    }

    /* indicate the LOG parser it may continue until next DMA operation */
    deviceDmaAnswer_finishedProcessing = 1;
}

/*evaluate the port link status change operation*/
static void evaluatePortLinkStatusChange(void)
{
    SKERNEL_DEVICE_OBJECT *devObjPtr;

    if(currentPortLinkStatusInfo.devNum >= SMAIN_MAX_NUM_OF_DEVICES_CNS)
    {
        skernelFatalError("evaluatePortLinkStatusChange: deviceIndex[%d] >= [%d] \n",
            currentPortLinkStatusInfo.devNum,SMAIN_MAX_NUM_OF_DEVICES_CNS);
    }
    else if(currentPortLinkStatusInfo.port >= SMAIN_MAX_NUM_OF_DEVICES_CNS)
    {
        skernelFatalError("evaluatePortLinkStatusChange: port[%d] >= [%d] \n",
            currentPortLinkStatusInfo.port,128);
    }
    else if(currentPortLinkStatusInfo.linkStatus > 1)
    {
        skernelFatalError("evaluatePortLinkStatusChange: linkStatus[%d] > [%d] \n",
            currentPortLinkStatusInfo.linkStatus,1);
    }

    devObjPtr = getDevicePtr(currentPortLinkStatusInfo.devNum,currentPortLinkStatusInfo.coreId);

    snetLinkStateNotify(devObjPtr,
        currentPortLinkStatusInfo.port,
        currentPortLinkStatusInfo.linkStatus);
}

/*evaluate the slan bind/unbind operation*/
static void evaluateSlanBind(void)
{
    SKERNEL_DEVICE_OBJECT *devObjPtr;

    devObjPtr = getDevicePtr(currentSlanBindInfo.devNum,currentSlanBindInfo.coreId);

    if(currentSlanBindInfo.isBindOrUnbind == GT_TRUE)  /*bind*/
    {
        skernelBindDevPort2Slan(devObjPtr->deviceId,
            currentSlanBindInfo.portNumber,
            currentSlanBindInfo.slanNamePtr,
            GT_FALSE);
    }
    else  /*unbind*/
    {
        skernelUnbindDevPort2Slan(devObjPtr->deviceId,
            currentSlanBindInfo.portNumber);
    }
}



/* need to evaluate the current line */
static void evaluateCurrentLine(void)
{
    GT_U32  processPacket;

    switch(currentLineDataType)
    {
        /* read/write from CPU */
        case DATA_TYPE_WRITE_FROM_CPU_E:
            evaluateWriteRegister();
            break;
        case DATA_TYPE_READ_FROM_CPU_E:
            evaluateReadRegister();
            break;

        /* slan INFO */
        case DATA_TYPE_START_SLAN_PACKET_E:
        /*case DATA_TYPE_MID_SLAN_PACKET_E:
          case DATA_TYPE_END_SLAN_PACKET_E:*/
            if(firstPacketInstanceToHandle != HANDLE_ALL_PACKETS)
            {
                if(firstPacketInstanceToHandle == (numPacketsAlreadyProcessed+1))
                {
                    processPacket = 1;
                }
                else
                {
                    processPacket = 0;
                }
            }
            else
            {
                processPacket = 1;
            }

            if(processPacket)
            {
                evaluateSlanPacket();
            }

            if(firstPacketInstanceToHandle == (numPacketsAlreadyProcessed+1))
            {
                stopLogParsingDueToPacketInstance = 1;
            }


            numPacketsAlreadyProcessed++;
            break;

        case DATA_TYPE_THREAD_TYPE_E:
            evaluateThreadType();
            break;

        /*
            evaluation of the DMA operation is done in the context of the device.
        case DATA_TYPE_DMA_OPERATION_E:
            evaluateDmaOperation();
            break;

        */

        case DATA_TYPE_PORT_LINK_STATUS_CHANGED_E:
            evaluatePortLinkStatusChange();
            break;


        case DATA_TYPE_SLAN_BIND_E:
        case DATA_TYPE_SLAN_UNBIND_E:
            evaluateSlanBind();
            break;

        case DATA_TYPE_OTHERS_E:
        case DATA_TYPE_NUM_OF_PACKETS_IN_THE_SYSTEM_E:
        default:
            /* nothing to do */
            break;
    }

    return;
}
/*  simLogToRuntimeSetFirstPacketInstanceToHandle:
*   the function defines the (number-1) of packets to 'skip' before handling the first packet
*  INPUT:
*       firstPacketInstanceToHandle - value 0 means ALL packets !!! (not skip any packet)
*                                     when != 0:
*                                     a. 1 based number of the packet to run.
*                                           1 means - process first packet and don't process any more
*                                           2 means - skip first packet , process second and don't process any more
*                                           ...
*                                           n means - skip (n-1) packets , process the n'th packet and don't process any more
*                                     b. single packet is run
*                                     c. the configuration run from start of the log , till the n'th packet.
*/
void simLogToRuntimeSetFirstPacketInstanceToHandle
(
    IN GT_U32   __firstPacketInstanceToHandle
)
{
    firstPacketInstanceToHandle = __firstPacketInstanceToHandle;

    if(firstPacketInstanceToHandle == 0)
    {
        firstPacketInstanceToHandle = HANDLE_ALL_PACKETS;
    }

    return;
}

/*******************************************************************************
* simLogToRuntime
*
* DESCRIPTION:
*       Engine of read LOG file and act according to the info:
*       1. do 'write from CPU'
*       2. do 'Read from CPU' (may effect active memory)
*       3. inject packets from the SLAN
*
*       NOTEs:
*       a. packets from the CPU are ignored as those are byproduct of 'write from CPU'
*       b. packet from 'loopback' ports are ignored because are byproduct of original packet (slan/CPU)
*
* INPUTS:
*      logFileName - the file of the log to parse.
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       None.
*******************************************************************************/
void simLogToRuntime(
    IN GT_CHAR*    logFileName
)
{
    GT_U32  allowFinalCatchUp = 1;

    if(gmLogSupport == GT_TRUE)
    {
        simGeneralPrintf( "simLogToRuntime : NOT supported in GM !!! \n");
        return;
    }

    if(logFileName == NULL)
    {
        simGeneralPrintf( "simLogToRuntime : log file name is NULL \n");
        return;
    }

    readFromLogFilePointer = fopen( logFileName, "r" );
    if(readFromLogFilePointer == NULL)
    {
        simGeneralPrintf( "simLogToRuntime : log file [%s] could not be open for READ \n" ,
            logFileName);
        return;
    }

    __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("simLogToRuntime : log file [%s] opened for READ \n" ,
        logFileName));

    /* init some values */
    simLogToRuntimeIsActive = 1;
    totalNumOfLines = 0;
    numPacketsAlreadyProcessed = 0;
    stopLogParsingDueToPacketInstance = 0;
    deviceDmaAnswer_ready     = 0;
    deviceDmaAnswer_finishedProcessing = 0;

    currentPositionOfStopCpuProcessing_ftell = 0;
    currentPositionOfStopCpuProcessing_ready = 0;
    currentPositionOfStopCpuProcessing_lineNum = 0;
    currentPositionOfStopCpuProcessing_doCatchUp = 0;
    currentPositionOfStopCpuProcessing_doCatchUp_maxLine = 0;


    write_cpu_str_len       = (GT_U32)strlen(write_cpu_str);
    read_cpu_str_len        = (GT_U32)strlen(read_cpu_str);
    ingress_packet_from_slan_start_str_len      = (GT_U32)strlen(ingress_packet_from_slan_start_str);
    ingress_packet_from_slan_ended_str_len      = (GT_U32)strlen(ingress_packet_from_slan_ended_str);
    thread_type_str_len                         = (GT_U32)strlen(thread_type_str);
    dma_operation_str_len                       = (GT_U32)strlen(dma_operation_str);
    logInfoNumOfPacketsInTheSystemStrLen        = (GT_U32)strlen(logInfoNumOfPacketsInTheSystemStr);

    port_link_status_changed_prefix_str_len     = (GT_U32)strlen(port_link_status_changed_prefix_str);
    slan_bind_prefix_str_len                    = (GT_U32)strlen(slan_bind_prefix_str);
    slan_unbind_prefix_str_len                  = (GT_U32)strlen(slan_unbind_prefix_str);


    ready_to_log_str_len                        = (GT_U32)strlen(ready_to_log_str);
    ready_to_log = 0;


    while(1)
    {
        /*read next line*/
        if(GT_FALSE == readNextLine())
        {
            /*error/EOF*/
            break;
        }

continueFileProcessing_lbl:
        /* parse current line */
        parseCurrentLine();

        /* evaluate current line */
        evaluateCurrentLine();

        if(stopLogParsingDueToPacketInstance)
        {
            __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(( "simLogToRuntime :parsing ended after [%d] packets \n" ,
                firstPacketInstanceToHandle));
            break;
        }
    }

    if(allowFinalCatchUp && stopLogParsingDueToPacketInstance == 0)
    {
        if(currentPositionOfStopCpuProcessing_ready &&
           currentPositionOfStopCpuProcessing_doCatchUp == 0)
        {
            if(logInfoNumOfPacketsInTheSystem == 0)
            {
                /* we ended the log and see that we finished parsing all packets .
                   but the device still processing current packets */
                 while(skernelNumOfPacketsInTheSystemGet())
                 {
                    if(deviceDmaAnswer_waiting)
                    {
                        /* the device waits for DMA answer but we got no info for it !!! */
                        skernelFatalError("simLogToRuntime: the device waits for DMA answer but we got no info for it !!! \n");
                    }

                    SIM_OS_MAC(simOsSleep)(50);
                 }
            }

            /* this operation may rewind us a bit to allow final CPU operations on the last packet */
            checkStateOfAllowedOperations();

            if(currentPositionOfStopCpuProcessing_doCatchUp)
            {
                allowFinalCatchUp = 0;

                __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("we need to do final 'catch up' \n"));
                /* we need to do final 'catch up' */
                goto continueFileProcessing_lbl;
            }
        }
    }

    fclose(readFromLogFilePointer);

    simLogPacketFrameUnitSet(SIM_LOG_FRAME_UNIT___ALLOW_ALL_UNITS___E);/* wild card for LOG without the unit filter */

    __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(( "simLogToRuntime :ended : log file [%s] closed after read [%d] lines \n" ,
        logFileName,totalNumOfLines));

    simLogPacketFrameUnitSet(SIM_LOG_FRAME_UNIT___RESTORE_PREVIOUS_UNIT___E);/* wild card for LOG without the unit filter */

    simLogToRuntimeIsActive = 0;

    if(deviceDmaAnswer_waiting)
    {
        /* the device waits for DMA answer but we got no info for it !!! */
        skernelFatalError("simLogToRuntime: the device waits for DMA answer but we got no info for it !!! \n");
    }

    if(ready_to_log == 0)
    {
        simGeneralPrintf(" the LOG file must contain string [ %s ] at the start (after examples) \n",
            SIM_LOG_READY_TO_START_LOG_STR);
    }


    return;
}

/* empty call - it needs to link simLogToRuntime.c correctly */
/* it force the linker to add it to the image                */
GT_VOID simLogToRuntimeEmptyFunc(GT_VOID)
{
    return;
}
/* the function called from the device that need to access the DMA for read/write */
/* the function wait until the info is read from the LOG */
static void deviceWaitForDmaAnswer(void)
{
    while(deviceDmaAnswer_ready == 0)
    {
        deviceDmaAnswer_waiting = 1;
        /* in the mean time we let the LOG be parsed by
           the main parser in function simLogToRuntime*/
        SIM_OS_MAC(simOsSleep)(50);
    }

    deviceDmaAnswer_waiting = 0;
    /* reset the value for next time */
    deviceDmaAnswer_ready = 0;
}

/*******************************************************************************
* simLogToRuntime_scibDmaRead
*
* DESCRIPTION:
*      Allow the LOG parser to emulate the DMA.
*
* INPUTS:
*       clientName - the DMA client name
*       deviceId    - device id. (of the device in the simulation)
*       address     - physical address that PP refer to.
*                     HOST CPU must convert it to HOST memory address
*       memSize     - the number of words/bytes (according to dataIsWords)
*       dataIsWords - the data to read is words or bytes
*                     1 - words --> swap network order to cpu order
*                     0 - bytes --> NO swap network order to cpu order
*
* OUTPUTS:
*       memPtr     - (pointer to) PP's memory in which HOST CPU memory will be
*                    copied.
*
* RETURN:
*
* COMMENTS:
*
*
*******************************************************************************/
void simLogToRuntime_scibDmaRead
(
    IN SNET_CHT_DMA_CLIENT_ENT clientName,
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    OUT GT_U32 * memPtr,
    IN GT_U32  dataIsWords
)
{
    /* wait for DMA info from the LOG */
    deviceWaitForDmaAnswer();

    /*evaluate the DMA read operation*/
    evaluateDmaOperation(deviceId,GT_FALSE/*read operation*/,memSize,address,memPtr);
}

/*******************************************************************************
*  simLogToRuntime_scibDmaWrite
*
* DESCRIPTION:
*      Allow the LOG parser to emulate the DMA.
*
* INPUTS:
*       clientName - the DMA client name
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
void simLogToRuntime_scibDmaWrite
(
    IN SNET_CHT_DMA_CLIENT_ENT clientName,
    IN GT_U32 deviceId,
    IN GT_U32 address,
    IN GT_U32 memSize,
    IN GT_U32 * memPtr,
    IN GT_U32  dataIsWords
)
{
    /* wait for DMA info from the LOG */
    deviceWaitForDmaAnswer();

    /*evaluate the DMA write operation*/
    evaluateDmaOperation(deviceId,GT_TRUE/*write operation*/,memSize,address,memPtr);

    return;
}

/*******************************************************************************
*   simLogToRuntime_scibSetInterrupt
*
* DESCRIPTION:
*       Allow the LOG parser to emulate the interrupt
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
void simLogToRuntime_scibSetInterrupt
(
    IN  GT_U32        deviceId
)
{
    /* we can to ignore the ISR invocation */
    __LOG_NO_LOCATION_META_DATA__WITH_SCIB_LOCK(("Ignore the ISR invocation by dev[%d] , when 'LOG parser' mode \n",
        deviceId));
    return;
}

static GT_BIT removeReadWriteOperation = 0;
static GT_BIT removeFromCpuPacketOperation = 0;
static GT_BIT startPacketFromCpu = 0;

/* read next line , and copy if not read/write */
static GT_BOOL readLineAndCopy(void)
{
    GT_U32  maxCompare;
    char* dummyReadLinePtr;
    static GT_BOOL is2PreviousEmpty;
    static GT_BOOL isPreviousEmpty = GT_FALSE;
    static GT_BOOL isCurrentEmpty = GT_FALSE;
    static GT_BOOL skipLineWithEmptyLineBeforeIt = GT_FALSE;
    static char emptyLine[] = {'\n',0};

    if(totalNumOfLines == 0)
    {
        is2PreviousEmpty = GT_FALSE;
        isPreviousEmpty = GT_FALSE;
        isCurrentEmpty = GT_FALSE;
        skipLineWithEmptyLineBeforeIt = GT_FALSE;
    }

getNextLine_lbl:

    is2PreviousEmpty = isPreviousEmpty;
    isPreviousEmpty = isCurrentEmpty;

    totalNumOfLines++;

    dummyReadLinePtr = fgets( origLineReadFromFile,
                              SIZE_OF_ORIG_LINE_READ_FROM_FILE_CNS,
                              readFromLogFilePointer);

    currentLineReadFromFile = origLineReadFromFile;
    if(dummyReadLinePtr)
    {
        currentLineReadFromFileLen = (GT_U32)strlen(currentLineReadFromFile);

        /* call this function before the comparing of string to remove the leading spaces */
        isCurrentEmpty = isEmptyLine();

        maxCompare = MIN(currentLineReadFromFileLen ,ready_to_log_str_len);
        if(maxCompare && 0 == strncmp(currentLineReadFromFile,ready_to_log_str,maxCompare))
        {
            /* the task type also have '\n' before it */
            ready_to_log = 1;
        }

        if(ready_to_log == 0)
        {
            /* the LOG file must contain this string !!! */
            /* so until this line we not filter any string */
            goto afterAllFilters_lbl;
        }


        if(removeReadWriteOperation && removeFromCpuPacketOperation)
        {
            /* remove also the 'taskType['*/
            maxCompare = MIN(currentLineReadFromFileLen ,thread_type_str_len);
            if(maxCompare && 0 == strncmp(currentLineReadFromFile,thread_type_str,maxCompare))
            {
                /* the task type also have '\n' before it */
                skipLineWithEmptyLineBeforeIt = GT_TRUE;

                goto getNextLine_lbl;
            }
        }

        if(is2PreviousEmpty == GT_TRUE)
        {
            if(skipLineWithEmptyLineBeforeIt == GT_FALSE)
            {
                /*add empty line for the line that was 2 'Previous lines' before */
                fputs(emptyLine,newLogFilePointer);
            }

            skipLineWithEmptyLineBeforeIt = GT_FALSE;
        }
        else
        if(isPreviousEmpty == GT_TRUE)
        {
            /* the line before current was empty , but the one before that was not */
            /* so we need to add empty line that represents the 'previous' */
            fputs(emptyLine,newLogFilePointer);

            /* remove indication that previous was empty so it will not be
               duplicated into is2PreviousEmpty as well */
            isPreviousEmpty = GT_FALSE;

        }

        if(isCurrentEmpty == GT_TRUE)
        {
            /* we can not automatically allow empty line to the new LOG because
               the next line maybe one that requires removal with the previous '\n' */
            goto getNextLine_lbl;
        }

        skipLineWithEmptyLineBeforeIt = GT_FALSE;

        if(removeReadWriteOperation)
        {
            maxCompare = MIN(currentLineReadFromFileLen ,write_cpu_str_len);
            if(maxCompare && 0 == strncmp(currentLineReadFromFile,write_cpu_str,maxCompare))
            {
                goto getNextLine_lbl;
            }

            maxCompare = MIN(currentLineReadFromFileLen ,read_cpu_str_len);
            if(maxCompare && 0 == strncmp(currentLineReadFromFile,read_cpu_str,maxCompare))
            {
                goto getNextLine_lbl;
            }

            maxCompare = MIN(currentLineReadFromFileLen ,write_dev_str_len);
            if(maxCompare && 0 == strncmp(currentLineReadFromFile,write_dev_str,maxCompare))
            {
                goto getNextLine_lbl;
            }

            maxCompare = MIN(currentLineReadFromFileLen ,read_dev_str_len);
            if(maxCompare && 0 == strncmp(currentLineReadFromFile,read_dev_str,maxCompare))
            {
                goto getNextLine_lbl;
            }
        }

        if(removeFromCpuPacketOperation)
        {
            if(startPacketFromCpu)
            {
                /* check if the FROM_CPU ended */
                maxCompare = MIN(currentLineReadFromFileLen ,packet_from_cpu_ended_str_len);
                if(maxCompare && 0 == strncmp(currentLineReadFromFile,packet_from_cpu_ended_str,maxCompare))
                {
                    /* the from CPU ended */
                    startPacketFromCpu = 0;

                    /* but the current line is skipped */
                }

                /* the from CPU NOT ended yet */
                goto getNextLine_lbl;
            }
            else
            {
                maxCompare = MIN(currentLineReadFromFileLen ,packet_from_cpu_str_len);
                if(maxCompare && 0 == strncmp(currentLineReadFromFile,packet_from_cpu_str,maxCompare))
                {
                    startPacketFromCpu = 1;

                    goto getNextLine_lbl;
                }
            }
        }

afterAllFilters_lbl:
        /* copy the orig line to the other file */
        fputs(origLineReadFromFile,newLogFilePointer);

        return GT_TRUE;
    }
    else
    {
        return GT_FALSE;
    }
}

/* remove from the LOG file the needed sections :
    'from_cpu'
    'read/write' by 'cpu/dev'
*/
void simLogRemoveSections(
    IN GT_CHAR*    logFileName
)
{
    static char fileNameBuffer[512];

    startPacketFromCpu = 0;
    totalNumOfLines = 0;

    if(gmLogSupport == GT_TRUE)
    {
        simGeneralPrintf( "simLogRemoveSections : NOT supported in GM !!! \n");
        return;
    }

    if(logFileName == NULL)
    {
        simGeneralPrintf( "simLogRemoveSections : log file name is NULL \n");
        return;
    }

    readFromLogFilePointer = fopen( logFileName, "r" );
    if(readFromLogFilePointer == NULL)
    {
        simGeneralPrintf( "simLogRemoveSections : log file [%s] could not be open for read \n" ,
            logFileName);
        return;
    }

    simGeneralPrintf("simLogRemoveSections : log file [%s] opened for read \n" ,
        logFileName);

    if(removeReadWriteOperation && removeFromCpuPacketOperation)
    {
        sprintf(fileNameBuffer,"%s%s",
            logFileName,
            ".no_mem.no_packet_from_cpu.txt");
    }
    else
    if(removeFromCpuPacketOperation)
    {
        sprintf(fileNameBuffer,"%s%s",
            logFileName,
            ".no_packet_from_cpu.txt");
    }
    else
    if(removeReadWriteOperation)
    {
        sprintf(fileNameBuffer,"%s%s",
            logFileName,
            ".no_mem.txt");
    }

    newLogFilePointer = fopen( fileNameBuffer, "w" );
    if(newLogFilePointer == NULL)
    {
        simGeneralPrintf( "simLogRemoveSections : the NEW log file [%s] could not be created \n" ,
            fileNameBuffer);
        return;
    }

    simGeneralPrintf("simLogRemoveSections : the NEW log file [%s] created(for write) \n" ,
        fileNameBuffer);


    write_cpu_str_len       = (GT_U32)strlen(write_cpu_str);
    read_cpu_str_len        = (GT_U32)strlen(read_cpu_str);

    write_dev_str_len       = (GT_U32)strlen(write_dev_str);
    read_dev_str_len        = (GT_U32)strlen(read_dev_str);

    packet_from_cpu_str_len        = (GT_U32)strlen(packet_from_cpu_str);
    packet_from_cpu_ended_str_len  = (GT_U32)strlen(packet_from_cpu_ended_str);

    thread_type_str_len                         = (GT_U32)strlen(thread_type_str);

    ready_to_log_str_len                        = (GT_U32)strlen(ready_to_log_str);
    ready_to_log = 0;

    while(1)
    {
        if(GT_FALSE == readLineAndCopy())
        {
            break;
        }
    }

    /* close the 2 files */
    fclose(readFromLogFilePointer);
    fclose(newLogFilePointer);

    simGeneralPrintf("simLogRemoveSections :close the 2 files \n");

    if(ready_to_log == 0)
    {
        simGeneralPrintf(" the LOG file must contain string [ %s ] at the start (after examples) \n",
            SIM_LOG_READY_TO_START_LOG_STR);
    }
}


/*******************************************************************************
* simLogRemoveReadWriteOperation
*
* DESCRIPTION:
*       remove from a LOG the 'read/write' operations 'CPU/DEV'
*       generate new file by the same name with postfix : ".no_mem.txt"
*
* INPUTS:
*      logFileName - the file of the log that need to remove those lines from it.
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       None.
*******************************************************************************/
void simLogRemoveReadWriteOperation(
    IN GT_CHAR*    logFileName
)
{
    removeReadWriteOperation = 1;

    simLogRemoveSections(logFileName);

    removeReadWriteOperation = 0;
}

/*******************************************************************************
* simLogRemoveFromCpuPackets
*
* DESCRIPTION:
*       remove from a LOG the 'FROM_CPU' packets
*       generate new file by the same name with postfix : ".no_packet_from_cpu.txt"
*
* INPUTS:
*      logFileName - the file of the log that need to remove those lines from it.
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       None.
*******************************************************************************/
void simLogRemoveFromCpuPackets(
    IN GT_CHAR*    logFileName
)
{
    removeFromCpuPacketOperation = 1;

    simLogRemoveSections(logFileName);

    removeFromCpuPacketOperation = 0;
}

/*******************************************************************************
* simLogRemoveFromCpuPacketsAndReadWriteOperation
*
* DESCRIPTION:
*       remove from a LOG the 'FROM_CPU' packets  and  the 'read/write' operations 'CPU/DEV'
*       generate new file by the same name with postfix : ".no_mem.no_packet_from_cpu.txt"
* INPUTS:
*      logFileName - the file of the log that need to remove those lines from it.
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       None.
*******************************************************************************/
void simLogRemoveFromCpuPacketsAndReadWriteOperation(
    IN GT_CHAR*    logFileName
)
{
    removeReadWriteOperation = 1;
    removeFromCpuPacketOperation = 1;

    simLogRemoveSections(logFileName);

    removeFromCpuPacketOperation = 0;
    removeReadWriteOperation = 0;
}

