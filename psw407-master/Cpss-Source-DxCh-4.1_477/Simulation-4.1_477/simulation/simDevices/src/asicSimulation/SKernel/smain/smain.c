/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smain.c
*
* DESCRIPTION:
*       The module is SKERNEL API and SMain functions
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*       $Revision: 170 $
*******************************************************************************/
#include <os/simTypesBind.h>
#include <asicSimulation/SCIB/scib.h>
#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SKernel/smem/smem.h>
#include <asicSimulation/SKernel/sfdb/sfdb.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetTwistL2.h>
#include <asicSimulation/SKernel/suserframes/snetTwistEgress.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahL2.h>
#include <asicSimulation/SKernel/suserframes/snetLion2TrafficGenerator.h>
#include <asicSimulation/SKernel/suserframes/snetLion2Oam.h>
#include <asicSimulation/SKernel/smain/smainSwitchFabric.h>
#include <asicSimulation/SKernel/twistCommon/sregTwist.h>
#include <asicSimulation/SKernel/smem/smemGM.h>
#include <asicSimulation/SKernel/smem/smemCheetah.h>
#include <asicSimulation/SKernel/smem/smemComModule.h>
#include <common/SBUF/sbuf.h>
#include <common/SQue/squeue.h>
#include <gmSimulation/GM/TrafficAPI.h>
#include <gmSimulation/GM/GMApi.h>
#include <asicSimulation/SInit/sinit.h>
#include <asicSimulation/SDistributed/sdistributed.h>
#include <common/SHOST/GEN/INTR/EXP/INTR.H>
#include <asicSimulation/SKernel/sfdb/sfdbCheetah.h>
#include <asicSimulation/SKernel/cheetahCommon/sregCheetah.h>
#include <asicSimulation/SLog/simLog.h>
#include <asicSimulation/SLog/simLogInfoTypePacket.h>
#include <asicSimulation/SKernel/smain/sRemoteTm.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahEgress.h>

/**************************** defines *****************************************/
#define SET_TASK_TYPE_MAC(newTaskType)                                  \
    {                                                                   \
        taskType = newTaskType;                                         \
        /* use cookiePtr as NULL , to not override first initialization ! */ \
        SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)(taskType, NULL);         \
    }

/* logic that used in osIntDisable of _WIN32 and LINUX_SIM*/
/* define macro that WA the numbering issue in  the SHOST */
#ifdef _WIN32
    #define WA_SHOST_NUMBERING(_vector) ((_vector) + 1)
#else /*linux*/
    #define WA_SHOST_NUMBERING(_vector) (_vector)
#endif

#define FILE_MAX_LINE_LENGTH_CNS (4*SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS)

/* minimum size of buffer for sprintf */
#define SMAIN_MIN_SIZE_OF_BUFFER_CNS 100

/* number of buffers in the pool */
#define SMAIN_BUFFERS_NUMBER_CNS                    512

/* end of file */
#define SMAIN_FILE_RECORDS_EOF                  0xffffffff

/* header length for NIC outPut */
#define SMAIN_NIC_TX_HEADER_LENGTH                  14

/* Aging task polling time in milliseconds */
#define SMAIN_AGING_POLLING_TIME_CNS                 5000

/* slan type */
#define SAGNTP_SLANUP_CNS            (0x400 + 3)
#define SAGNTP_SLANDN_CNS            (0x400 + 4)

#define SMAIN_NO_GM_DEVICE                  0xFF

#define SMAIN_PCI_FLAG_BUFF_SIZE            0x10

#define DEBUG_SECTION_CNS   "debug"
/* indication of number of packets that are currently in the system */
static GT_U32   skernelNumOfPacketsInTheSystem = 0;

/* number of devices in the simulation */
extern GT_U32       smainDevicesNumber;/* definition move to sinit.c */

/*are we under shutdown */
static GT_BOOL  skernelUnderShutDown    = GT_FALSE;

/* the last deviceId that was used for cores --> will be initialized to smainDevicesNumber
   in function skernelInit */
static GT_U32       lastDeviceIdUsedForCores = 0;

/* Init status of GM Traffic Manager */
GT_U32       smainGmTrafficInitDone = 0;

/* User debug info */
SKERNEL_USER_DEBUG_INFO_STC skernelUserDebugInfo;

/* allow the Puma3 to do 'external' mac loopback as processing the loop in the
   context of the sending task (not sending it to the 'buffers pool') */
static GT_U32   supportMacLoopbackInContextOfSender = 1;
/* the minimal number of buffers that should be kept , and not allowed for 'loopback port'*/
#define LOOPBACK_MIN_FREE_BUFFERS_CNS   100

/*
 * Typedef: struct SMAIN_DEV_NAME_STC
 *
 * Description:
 *      Describe device specific information.
 *
 * Fields:
 *      gmDevice    : golden model device.
 *      devName     : Pointer to the device name.
 *      devType     : Device type.
 *      portNum     : Number of MAC ports.
 *      cpuPortNum  : cpu port number -> for SLANS
 * Comments:
 */

typedef struct{
    GT_U32                    gmDevType;
    char                    * devName;
    SKERNEL_DEVICE_TYPE       devType;
    GT_U32                    portNum;
    GT_U32                    cpuPortNum;
}SMAIN_DEV_NAME_STC;

/*************************** Local Variables **********************************/

/* macro PORTS_BMP_IS_PORT_SET_MAC
    to check if port is set the bmp of ports (is corresponding bit is 1)

  portsBmpPtr - of type CPSS_PORTS_BMP_STC*
                pointer to the ports bmp
  portNum - the port num to set in the ports bmp

  return 0 -- port not set in bmp
  return 1 -- port set in bmp
*/
#define PORTS_BMP_IS_PORT_SET_MAC(portsBmpPtr,portNum)   \
    (((portsBmpPtr)->ports[(portNum)>>5] & (1 << ((portNum)& 0x1f)))? 1 : 0)

#define BMP_CONTINUES_PORTS_MAC(x)  (((x)!=32) ? ((1<<(x)) - 1) : 0xFFFFFFFF)

/* bmp of ports 32..63 -- physical ports (not virtual) */
#define BMP_PORTS_32_TO_63_MAC(portsBmpPtr, numPorts) \
    (portsBmpPtr)->ports[0] = 0xFFFFFFFF;    \
    (portsBmpPtr)->ports[1] = BMP_CONTINUES_PORTS_MAC(numPorts % 32);    \
    (portsBmpPtr)->ports[2] = 0;    \
    (portsBmpPtr)->ports[3] = 0

/* bmp of ports 64..95 -- physical ports (not virtual) */
#define BMP_PORTS_64_TO_95_MAC(portsBmpPtr, numPorts) \
    (portsBmpPtr)->ports[0] = 0xFFFFFFFF;    \
    (portsBmpPtr)->ports[1] = 0xFFFFFFFF;    \
    (portsBmpPtr)->ports[2] = BMP_CONTINUES_PORTS_MAC(numPorts % 32);    \
    (portsBmpPtr)->ports[3] = 0

/* bmp of ports 64..95 -- physical ports (not virtual) */
#define BMP_PORTS_96_TO_127_MAC(portsBmpPtr, numPorts) \
    (portsBmpPtr)->ports[0] = 0xFFFFFFFF;    \
    (portsBmpPtr)->ports[1] = 0xFFFFFFFF;    \
    (portsBmpPtr)->ports[2] = 0xFFFFFFFF;    \
    (portsBmpPtr)->ports[3] = BMP_CONTINUES_PORTS_MAC(numPorts % 32)

/* bmp of less then 32 ports -- physical ports (not virtual) */
#define BMP_PORTS_LESS_32_MAC(portsBmpPtr,numPorts)    \
    (portsBmpPtr)->ports[0] = BMP_CONTINUES_PORTS_MAC(numPorts);  \
    (portsBmpPtr)->ports[1] = 0; \
    (portsBmpPtr)->ports[2] = 0; \
    (portsBmpPtr)->ports[3] = 0

/* macro to set range of ports in bmp --
   NOTE :  it must be (startPort + numPorts) <= 31
*/
#define PORT_RANGE_MAC(startPort,numPorts)\
    (BMP_CONTINUES_PORTS_MAC(numPorts) << startPort)

/*
 * Typedef: structure PORTS_BMP_STC
 *
 * Description: Defines the bmp of ports (up to 63 ports)
 *
 * Fields:
 *      ports - array of bmp of ports
 *              ports[0] - hold bmp of the ports 0  - 31
 *              ports[1] - hold bmp of the ports 32 - 63
 *
 *  notes:
 *  can use 3 macros:
 *  PORTS_BMP_PORT_SET_MAC , PORTS_BMP_PORT_CLEAR_MAC
 *  PORTS_BMP_IS_PORT_SET_MAC
 *
 */
typedef struct{
    GT_U32      ports[4];
}PORTS_BMP_STC;

static const PORTS_BMP_STC portsBmp24_25_27 = {{(PORT_RANGE_MAC(24,2) |
                                                      PORT_RANGE_MAC(27,1)) ,0}};

static const PORTS_BMP_STC portsBmp0to23_27 = {{(PORT_RANGE_MAC(0,24) |
                                                      PORT_RANGE_MAC(27,1)) ,0}};

static const PORTS_BMP_STC portsBmpCh3_8_Xg = {{(PORT_RANGE_MAC(24,4) |
                                                      PORT_RANGE_MAC(12,1) |
                                                      PORT_RANGE_MAC(10,1) |
                                                      PORT_RANGE_MAC(4 ,1) |
                                                      PORT_RANGE_MAC(0 ,1)) ,0}};

static const PORTS_BMP_STC portsBmpCh3Xg =    {{(PORT_RANGE_MAC(24,4) |
                                                      PORT_RANGE_MAC(22,1) |
                                                      PORT_RANGE_MAC(16,1) |
                                                      PORT_RANGE_MAC(12,1) |
                                                      PORT_RANGE_MAC(10,1) |
                                                      PORT_RANGE_MAC(4 ,1) |
                                                      PORT_RANGE_MAC(0 ,1)) ,0}};

static const PORTS_BMP_STC portsBmp0to3_24_25 = {{(PORT_RANGE_MAC(0,4) |
                                                        PORT_RANGE_MAC(24,2)) ,0}};

static const PORTS_BMP_STC portsBmp0to7_24_25 = {{(PORT_RANGE_MAC(0,8) |
                                                        PORT_RANGE_MAC(24,2)) ,0}};

static const PORTS_BMP_STC portsBmp0to15_24_25 = {{(PORT_RANGE_MAC(0,16) |
                                                         PORT_RANGE_MAC(24,2)) ,0}};

static const PORTS_BMP_STC portsBmp0to15_24to27 = {{(PORT_RANGE_MAC(0,16) |
                                                          PORT_RANGE_MAC(24,4)) ,0}};

static const PORTS_BMP_STC portsBmp56to59and62and64to71 = {{
    0/*0..31*/ ,
    (PORT_RANGE_MAC((56%32),4) /*56..59*/ | PORT_RANGE_MAC((62%32),1)) ,
     PORT_RANGE_MAC((64%32),8),/*64..71*/
    0
}};


/********************/
/* multi mode ports */
/********************/
static const PORTS_BMP_STC portsBmpMultiState_24to27 = {{PORT_RANGE_MAC(24,4) ,0}};
static const PORTS_BMP_STC portsBmpMultiState_0to11 = {{PORT_RANGE_MAC(0,12) ,0}};

static const PORTS_BMP_STC portsBmpMultiState_0to11_32to47 = {{PORT_RANGE_MAC(0,12) ,PORT_RANGE_MAC(0,12)}};
static const PORTS_BMP_STC portsBmpMultiState_48to71 = {{0, 0xFFFF0000, 0x000000FF, 0}};
static const PORTS_BMP_STC portsBmpMultiState_0to71  = {{0xFFFFFFFF, 0xFFFFFFFF, 0x000000FF, 0}};


/* cheetah with 10 ports */
#define  CH_10_DEVICES_MAC       \
SKERNEL_98DX107                           ,\
SKERNEL_98DX106                           ,\
SKERNEL_DXCH_B0

/* cheetah with 27 ports */
#define  CH_27_DEVICES_MAC       \
SKERNEL_98DX270                           ,\
SKERNEL_98DX273                           ,\
SKERNEL_98DX803                           ,\
SKERNEL_98DX249                           ,\
SKERNEL_98DX269                           ,\
SKERNEL_98DX169                           ,\
SKERNEL_DXCH

/* cheetah with 26 ports */
#define  CH_26_DEVICES_MAC       \
SKERNEL_98DX260                           ,\
SKERNEL_98DX263                           ,\
SKERNEL_98DX262                           ,\
SKERNEL_98DX268

/* cheetah with 24 ports */
#define  CH_24_DEVICES_MAC       \
SKERNEL_98DX250                           ,\
SKERNEL_98DX253                           ,\
SKERNEL_98DX243                           ,\
SKERNEL_98DX248

/* cheetah with 25 ports */
#define  CH_25_DEVICES_MAC       \
SKERNEL_98DX133

/* cheetah with 16 ports */
#define  CH_16_DEVICES_MAC       \
SKERNEL_98DX163                           ,\
SKERNEL_98DX166                           ,\
SKERNEL_98DX167

/* cheetah-2 with 28 ports */
#define  CH2_28_DEVICES_MAC       \
SKERNEL_98DX285                           ,\
SKERNEL_98DX804                           ,\
SKERNEL_DXCH2

/* cheetah-2 with 27 ports */
#define  CH2_27_DEVICES_MAC       \
SKERNEL_98DX275

/* cheetah-2 with 26 ports */
#define  CH2_26_DEVICES_MAC       \
SKERNEL_98DX265                         ,\
SKERNEL_98DX145

/* cheetah-2 with 24 ports */
#define  CH2_24_DEVICES_MAC       \
SKERNEL_98DX255

/* cheetah-2 with 12 ports */
#define  CH2_12_DEVICES_MAC       \
SKERNEL_98DX125

/* cheetah-3 with 24 ports ports */
#define  CH3_24_DEVICES_MAC       \
SKERNEL_98DX5124

/* cheetah-3 with 24 ports plus 2 XG ports */
#define  CH3_26_DEVICES_MAC       \
SKERNEL_98DX5126                       ,\
SKERNEL_98DX5127

/* cheetah-3 with 24 ports plus 4 XG ports */
#define  CH3_28_DEVICES_MAC       \
SKERNEL_98DX5128                       ,\
SKERNEL_98DX5128_1                     ,\
SKERNEL_98DX287                        ,\
SKERNEL_98DX5129                       ,\
SKERNEL_DXCH3                          ,\
SKERNEL_98DX287   /*ch3p*/             ,\
SKERNEL_DXCH3P

/* cheetah-3 with 4 1/2.5 Gigabit Ethernet ports + 2XG */
#define  CH3_6_DEVICES_MAC       \
SKERNEL_98DX5151

/* cheetah-3 with 8 1/2.5 Gigabit Ethernet ports + 2XG */
#define  CH3_10_DEVICES_MAC       \
SKERNEL_98DX5152

/* cheetah-3 with 16 1/2.5 Gigabit Ethernet ports + 2XG */
#define  CH3_18_DEVICES_MAC       \
SKERNEL_98DX5154

/* cheetah-3 with 16 1/2.5 Gigabit Ethernet ports + 2XG + 2XGS */
#define  CH3_20_DEVICES_MAC       \
SKERNEL_98DX5155

/* cheetah-3 with 24 1/2.5 Gigabit Ethernet ports + 2XG*/
#define  CH3_24_2_DEVICES_MAC       \
SKERNEL_98DX5156

/* cheetah-3 with 24 1/2.5 Gigabit Ethernet ports + 2XG + 2XGS*/
#define  CH3_24_4_DEVICES_MAC       \
SKERNEL_98DX5157

/* cheetah-3 with 8 XG ports: 8 XG or 6XG + 2 Stack */
#define  CH3_8_XG_DEVICES_MAC       \
SKERNEL_98DX8108                       ,\
SKERNEL_98DX8109

/* cheetah-3 with 10 XG ports: 6 XG + 4 Stack */
#define  CH3_XG_DEVICES_MAC       \
SKERNEL_98DX8110                       ,\
SKERNEL_98DX8110_1                     ,\
SKERNEL_DXCH3_XG

/* Salsa with 24 ports */
#define  SALSA_24_DEVICES_MAC       \
SKERNEL_98DX240                       ,\
SKERNEL_98DX241

/* Salsa with 16 ports */
#define  SALSA_16_DEVICES_MAC       \
SKERNEL_98DX160                       ,\
SKERNEL_98DX161

/* Salsa with 12 ports */
#define  SALSA_12_DEVICES_MAC       \
SKERNEL_98DX120                       ,\
SKERNEL_98DX121

/* Twist-C with 10 ports */
#define  TWISTC_10_DEVICES_MAC     \
SKERNEL_98MX620B

/* Twist-C with 52 ports */
#define  TWISTC_52_DEVICES_MAC     \
SKERNEL_98MX610B                          ,\
SKERNEL_98MX610BS                         ,\
SKERNEL_98EX110BS                         ,\
SKERNEL_98EX111BS                         ,\
SKERNEL_98EX112BS                         ,\
SKERNEL_98EX110B                          ,\
SKERNEL_98EX111B                          ,\
SKERNEL_98EX112B

/* Twist-C with 12 ports */
#define  TWISTC_12_DEVICES_MAC     \
SKERNEL_98EX120B                          ,\
SKERNEL_98EX120B_                         ,\
SKERNEL_98EX121B                          ,\
SKERNEL_98EX122B                          ,\
SKERNEL_98EX128B                          ,\
SKERNEL_98EX129B

/* Twist-D with 52 ports */
#define  TWISTD_52_DEVICES_MAC     \
SKERNEL_98EX100D                          ,\
SKERNEL_98EX110D                          ,\
SKERNEL_98EX115D                          ,\
SKERNEL_98EX110DS                         ,\
SKERNEL_98EX115DS

/* Twist-D with 12 ports */
#define  TWISTD_12_DEVICES_MAC     \
SKERNEL_98EX120D                          ,\
SKERNEL_98EX125D

/* Twist-D with 1XG ports */
#define  TWISTD_1_DEVICES_MAC      \
SKERNEL_98EX130D                          ,\
SKERNEL_98EX135D


/* Samba with 52 ports */
#define  SAMBA_52_DEVICES_MAC      \
SKERNEL_98MX615D                           ,\
SKERNEL_98MX618

/* Samba with 12 ports */
#define  SAMBA_12_DEVICES_MAC      \
SKERNEL_98MX625D                           ,\
SKERNEL_98MX625V0                         ,\
SKERNEL_98MX628

/* Samba with 1XG ports */
#define  SAMBA_1_DEVICES_MAC       \
SKERNEL_98MX635D                           ,\
SKERNEL_98MX638


/* Tiger with 52 ports */
#define  TIGER_52_DEVICES_MAC      \
SKERNEL_98EX116                           ,\
SKERNEL_98EX106                           ,\
SKERNEL_98EX108                           ,\
SKERNEL_98EX116DI

/* Tiger with 12 ports */
#define  TIGER_12_DEVICES_MAC      \
SKERNEL_98EX126                           ,\
SKERNEL_98EX126V0                         ,\
SKERNEL_98EX126DI

/* Tiger with 1XG ports */
#define  TIGER_1_DEVICES_MAC       \
SKERNEL_98EX136                           ,\
SKERNEL_98EX136DI

/* Puma2 devices , 24 ports */
#define PUMA_24_DEVICES_MAC      \
SKERNEL_98EX2106                         ,\
SKERNEL_98EX2206                         ,\
SKERNEL_98MX2306                         ,\
SKERNEL_98EX8261                         ,\
SKERNEL_98EX8301                         ,\
SKERNEL_98EX8303                         ,\
SKERNEL_98EX8501                         ,\
SKERNEL_98EX240                          ,\
SKERNEL_98EX240_1                        ,\
SKERNEL_98MX840
/* Puma2 devices : 2 port XG devices */
#define PUMA_26_DEVICES_MAC      \
SKERNEL_98EX2110                         ,\
SKERNEL_98EX2210                         ,\
SKERNEL_98MX2310                         ,\
SKERNEL_98EX8262                         ,\
SKERNEL_98EX8302                         ,\
SKERNEL_98EX8305                         ,\
SKERNEL_98EX8502                         ,\
SKERNEL_98EX260                          ,\
SKERNEL_98MX860

#define XCAT_8FE_2STACK_PORTS_DEVICES \
SKERNEL_98DX1101,                     \
SKERNEL_98DX2101

#define XCAT_16FE_2STACK_PORTS_DEVICES \
SKERNEL_98DX1111

#define XCAT_16FE_4STACK_PORTS_DEVICES \
SKERNEL_98DX2112

#define XCAT_24FE_4STACK_PORTS_DEVICES  \
SKERNEL_98DX1122,                       \
SKERNEL_98DX1123,                       \
SKERNEL_98DX2122,                       \
SKERNEL_98DX2123,                       \
SKERNEL_98DX2151

#define XCAT_8FE_DEVICES \
XCAT_8FE_2STACK_PORTS_DEVICES

#define XCAT_16FE_DEVICES \
XCAT_16FE_2STACK_PORTS_DEVICES, \
XCAT_16FE_4STACK_PORTS_DEVICES

#define XCAT_24FE_DEVICES \
XCAT_24FE_4STACK_PORTS_DEVICES


#define XCAT_FE_DEVICES \
XCAT_8FE_DEVICES,  \
XCAT_16FE_DEVICES, \
XCAT_24FE_DEVICES

#define XCAT_8GE_2STACK_PORTS_DEVICES \
    SKERNEL_98DX3001       ,\
    SKERNEL_98DX3101       ,\
    SKERNEL_98DX4101

#define XCAT_8GE_4STACK_PORTS_DEVICES   \
    SKERNEL_98DX4102


#define XCAT_6GE_4STACK_PORTS_DEVICES   \
    SKERNEL_98DX4103


#define XCAT_16GE_PORTS_DEVICES   \
    SKERNEL_98DX3010,             \
    SKERNEL_98DX3110

#define XCAT_16GE_2STACK_PORTS_DEVICES   \
    SKERNEL_98DX3011

#define XCAT_16GE_4STACK_PORTS_DEVICES   \
    SKERNEL_98DX3111,                    \
    SKERNEL_98DX4110

#define XCAT_24GE_PORTS_DEVICES   \
    SKERNEL_98DX3120

#define XCAT_24GE_2STACK_PORTS_DEVICES   \
    SKERNEL_98DX4121

#define XCAT_24GE_4STACK_PORTS_DEVICES   \
    SKERNEL_98DX3121,                    \
    SKERNEL_98DX3122,                    \
    SKERNEL_98DX3123,                    \
    SKERNEL_98DX3124,                    \
    SKERNEL_98DX3125,                    \
    SKERNEL_98DX4120,                    \
    SKERNEL_98DX4122,                    \
    SKERNEL_98DX4123

#define XCAT_6GE_DEVICES \
XCAT_6GE_4STACK_PORTS_DEVICES,

#define XCAT_8GE_DEVICES \
XCAT_8GE_2STACK_PORTS_DEVICES, \
XCAT_8GE_4STACK_PORTS_DEVICES

#define XCAT_16GE_DEVICES \
XCAT_16GE_PORTS_DEVICES,       \
XCAT_16GE_2STACK_PORTS_DEVICES,\
XCAT_16GE_4STACK_PORTS_DEVICES

#define XCAT_24GE_DEVICES \
XCAT_24GE_PORTS_DEVICES       ,\
XCAT_24GE_2STACK_PORTS_DEVICES,\
XCAT_24GE_4STACK_PORTS_DEVICES

#define XCAT_GE_DEVICES \
XCAT_6GE_DEVICES, \
XCAT_8GE_DEVICES, \
XCAT_16GE_DEVICES,\
XCAT_24GE_DEVICES

#define XCAT_DEVICES \
XCAT_FE_DEVICES,     \
XCAT_GE_DEVICES      \

/* XCAT 24FE/24GE+4XG */
#define  XCAT_24_4_DEVICES_MAC    \
XCAT_24GE_4STACK_PORTS_DEVICES,   \
XCAT_24FE_4STACK_PORTS_DEVICES,   \
SKERNEL_XCAT_24_AND_4

#define XCAT3_24_6_DEVICES_MAC            \
    SKERNEL_XCAT3_24_AND_6

#define XCAT2_24_4_DEVICES_MAC            \
    SKERNEL_XCAT2_24_AND_4

/* Lion's 'port group' - 12 XG/GE ports */
#define LION_PORT_GROUP_12_DEVICES_MAC    \
SKERNEL_LION_PORT_GROUP_12

/* Lion - 48 XG/GE ports */
#define LION_48_DEVICES_MAC    \
SKERNEL_LION_48

/* Lion2's 'port group' - 12 XG/GE ports */
#define LION2_PORT_GROUP_12_DEVICES_MAC    \
SKERNEL_LION2_PORT_GROUP_12

/* Lion2 - 96 XG/GE ports */
#define LION2_96_DEVICES_MAC    \
SKERNEL_LION2_96

/* Lion3's 'port group' - 12 XG/GE ports */
#define LION3_PORT_GROUP_12_DEVICES_MAC    \
SKERNEL_LION3_PORT_GROUP_12

/* Lion3 - 96 XG/GE ports */
#define LION3_96_DEVICES_MAC    \
SKERNEL_LION3_96


/* Puma3's network core + fabric core - 32 XG/GE ports */
#define PUMA3_NETWORK_FABRIC_DEVICES_MAC    \
SKERNEL_PUMA3_NETWORK_FABRIC

/* Lion2 - 96 XG/GE ports */
#define PUMA3_64_DEVICES_MAC    \
SKERNEL_PUMA3_64

/* Bobcat2 : number of 'MAC' ports */
#define BOBCAT2_NUM_OF_MAC_PORTS_CNS    72

/* Bobcat2 */
#define BOBCAT2_DEVICES_MAC    \
SKERNEL_BOBCAT2

/* bobk-caelum */
#define BOBK_CAELUM_DEVICES_MAC    \
SKERNEL_BOBK_CAELUM

/* bobk-cetus */
#define BOBK_CETUS_DEVICES_MAC    \
SKERNEL_BOBK_CETUS

/* Bobcat3 */
#define BOBCAT3_DEVICES_MAC    \
SKERNEL_BOBCAT3


/* Bobcat3 : number of 'MAC' ports (6*12 = 72)
    there are 6 DP (data path) units each handle 12 ports
*/
#define BOBCAT3_NUM_OF_MAC_PORTS_CNS    72


/*tiger*/
const static SKERNEL_DEVICE_TYPE tgXGlegalDevTypes[] =
{   TIGER_1_DEVICES_MAC , END_OF_TABLE};
const static  SKERNEL_DEVICE_TYPE tg12legalDevTypes[] =
{   TIGER_12_DEVICES_MAC , END_OF_TABLE};
const static  SKERNEL_DEVICE_TYPE tg52legalDevTypes[] =
{   TIGER_52_DEVICES_MAC , END_OF_TABLE};
/*samba*/
const static  SKERNEL_DEVICE_TYPE sambaXGlegalDevTypes[] =
{   SAMBA_1_DEVICES_MAC  , END_OF_TABLE};
const static  SKERNEL_DEVICE_TYPE samba12legalDevTypes[] =
{   SAMBA_12_DEVICES_MAC , END_OF_TABLE};
const static  SKERNEL_DEVICE_TYPE samba52legalDevTypes[] =
{   SAMBA_52_DEVICES_MAC , END_OF_TABLE};
/*twistd*/
const static  SKERNEL_DEVICE_TYPE twistdXGlegalDevTypes[] =
{   TWISTD_1_DEVICES_MAC , END_OF_TABLE};
const static  SKERNEL_DEVICE_TYPE twistd12legalDevTypes[] =
{   TWISTD_12_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE twistd52legalDevTypes[] =
{   TWISTD_52_DEVICES_MAC, END_OF_TABLE };
/*twistc*/
const static  SKERNEL_DEVICE_TYPE twistc10legalDevTypes[] =
{   TWISTC_10_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE twistc12legalDevTypes[] =
{   TWISTC_12_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE twistc52legalDevTypes[] =
{   TWISTC_52_DEVICES_MAC, END_OF_TABLE };

/* salsa */
const static  SKERNEL_DEVICE_TYPE salsa24legalDevTypes[] =
{   SALSA_24_DEVICES_MAC , END_OF_TABLE};
const static  SKERNEL_DEVICE_TYPE salsa16legalDevTypes[] =
{   SALSA_16_DEVICES_MAC , END_OF_TABLE};
const static  SKERNEL_DEVICE_TYPE salsa12legalDevTypes[] =
{   SALSA_12_DEVICES_MAC , END_OF_TABLE};
/* cheetah */
const static  SKERNEL_DEVICE_TYPE ch_27legalDevTypes[] =
{   CH_27_DEVICES_MAC , END_OF_TABLE};
const static  SKERNEL_DEVICE_TYPE ch_26legalDevTypes[] =
{   CH_26_DEVICES_MAC , END_OF_TABLE};
const static  SKERNEL_DEVICE_TYPE ch_24legalDevTypes[] =
{   CH_24_DEVICES_MAC , END_OF_TABLE};
const static  SKERNEL_DEVICE_TYPE ch_25legalDevTypes[] =
{   CH_25_DEVICES_MAC , END_OF_TABLE};
const static  SKERNEL_DEVICE_TYPE ch_16legalDevTypes[] =
{   CH_16_DEVICES_MAC , END_OF_TABLE};
const static  SKERNEL_DEVICE_TYPE ch_10legalDevTypes[] =
{   CH_10_DEVICES_MAC , END_OF_TABLE};
/* cheetah-2 */
const static  SKERNEL_DEVICE_TYPE ch2_28legalDevTypes[] =
{   CH2_28_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE ch2_27legalDevTypes[] =
{   CH2_27_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE ch2_26legalDevTypes[] =
{   CH2_26_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE ch2_24legalDevTypes[] =
{   CH2_24_DEVICES_MAC, END_OF_TABLE };
/* cheetah-3 */
const static  SKERNEL_DEVICE_TYPE ch3_28legalDevTypes[] =
{   CH3_28_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE ch3_26legalDevTypes[] =
{   CH3_26_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE ch3_24legalDevTypes[] =
{   CH3_24_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE ch3_6legalDevTypes[] =
{   CH3_6_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE ch3_10legalDevTypes[] =
{   CH3_10_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE ch3_18legalDevTypes[] =
{   CH3_18_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE ch3_20legalDevTypes[] =
{   CH3_20_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE ch3_24_2legalDevTypes[] =
{   CH3_24_2_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE ch3_24_4legalDevTypes[] =
{   CH3_24_4_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE ch3_8_XGlegalDevTypes[] =
{   CH3_8_XG_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE ch3_XGlegalDevTypes[] =
{   CH3_XG_DEVICES_MAC, END_OF_TABLE };
/* Puma */
const static  SKERNEL_DEVICE_TYPE puma_26legalDevTypes[] =
{   PUMA_26_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE puma_24legalDevTypes[] =
{   PUMA_24_DEVICES_MAC, END_OF_TABLE };

/* xCat */
const static  SKERNEL_DEVICE_TYPE xcat_24_4legalDevTypes[] =
{   XCAT_24_4_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE xcat2_24_4legalDevTypes[] =
{   XCAT2_24_4_DEVICES_MAC, END_OF_TABLE };

/* xCat3 */
const static  SKERNEL_DEVICE_TYPE xcat3_24_6legalDevTypes[] =
{   XCAT3_24_6_DEVICES_MAC, END_OF_TABLE };

/* Lion */
const static  SKERNEL_DEVICE_TYPE lionPortGroup_12_legalDevTypes[] =
{   LION_PORT_GROUP_12_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE lion_48_legalDevTypes[] =
{   LION_48_DEVICES_MAC, END_OF_TABLE };

/* Lion 2 */
const static  SKERNEL_DEVICE_TYPE lion2PortGroup_12_legalDevTypes[] =
{   LION2_PORT_GROUP_12_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE lion2_96_legalDevTypes[] =
{   LION2_96_DEVICES_MAC, END_OF_TABLE };

/* Lion 3 */
const static  SKERNEL_DEVICE_TYPE lion3PortGroup_12_legalDevTypes[] =
{   LION3_PORT_GROUP_12_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE lion3_96_legalDevTypes[] =
{   LION3_96_DEVICES_MAC, END_OF_TABLE };

const static  SKERNEL_DEVICE_TYPE bobcat2_legalDevTypes[] =
{   BOBCAT2_DEVICES_MAC, END_OF_TABLE };

const static  SKERNEL_DEVICE_TYPE bobk_caelum_legalDevTypes[] =
{   BOBK_CAELUM_DEVICES_MAC, END_OF_TABLE };

const static  SKERNEL_DEVICE_TYPE bobk_cetus_legalDevTypes[] =
{   BOBK_CETUS_DEVICES_MAC, END_OF_TABLE };

const static  SKERNEL_DEVICE_TYPE bobcat3_legalDevTypes[] =
{   BOBCAT3_DEVICES_MAC, END_OF_TABLE };



/* other */
const static  SKERNEL_DEVICE_TYPE phy_shell_legalDevTypes[] =
{   SKERNEL_PHY_SHELL, END_OF_TABLE };

const static  SKERNEL_DEVICE_TYPE phy_core_legalDevTypes[] =
{   SKERNEL_PHY_CORE_1540M_1548M, END_OF_TABLE };

const static  SKERNEL_DEVICE_TYPE macsec_legalDevTypes[] =
{   SKERNEL_MACSEC, END_OF_TABLE };

const static  SKERNEL_DEVICE_TYPE empty_legalDevTypes[] =
{   SKERNEL_EMPTY, END_OF_TABLE };

/* puma3 */
const static  SKERNEL_DEVICE_TYPE puma3NetworkFabric_legalDevTypes[] =
{   PUMA3_NETWORK_FABRIC_DEVICES_MAC, END_OF_TABLE };
const static  SKERNEL_DEVICE_TYPE puma3_64_legalDevTypes[] =
{   PUMA3_64_DEVICES_MAC, END_OF_TABLE };



/* default state for devices that have 24G ports in 0..23 and XG ports in 23..27

NOTE: it also support devices with 24G (0..23) + 2XG (24,25)
and also support devices with 2XG (24,25)
and also devices with only 24XG
and also devices with only 12 GE ports (0..11)
...
NOTE: supports also devices with non exists ports in the middle ,
    since we always 'mask' this array with defaultPortsBmpPtr
*/
const static  SKERNEL_PORT_STATE_ENT gig24Xg4Ports[28] =
{
    SKERNEL_PORT_STATE_GE_E,     /* 0 */
    SKERNEL_PORT_STATE_GE_E,     /* 1 */
    SKERNEL_PORT_STATE_GE_E,     /* 2 */
    SKERNEL_PORT_STATE_GE_E,     /* 3 */

    SKERNEL_PORT_STATE_GE_E,     /* 4 */
    SKERNEL_PORT_STATE_GE_E,     /* 5 */
    SKERNEL_PORT_STATE_GE_E,     /* 6 */
    SKERNEL_PORT_STATE_GE_E,     /* 7 */

    SKERNEL_PORT_STATE_GE_E,     /* 8 */
    SKERNEL_PORT_STATE_GE_E,     /* 9 */
    SKERNEL_PORT_STATE_GE_E,     /* 10 */
    SKERNEL_PORT_STATE_GE_E,     /* 11 */

    SKERNEL_PORT_STATE_GE_E,     /* 12 */
    SKERNEL_PORT_STATE_GE_E,     /* 13 */
    SKERNEL_PORT_STATE_GE_E,     /* 14 */
    SKERNEL_PORT_STATE_GE_E,     /* 15 */

    SKERNEL_PORT_STATE_GE_E,     /* 16 */
    SKERNEL_PORT_STATE_GE_E,     /* 17 */
    SKERNEL_PORT_STATE_GE_E,     /* 18 */
    SKERNEL_PORT_STATE_GE_E,     /* 19 */

    SKERNEL_PORT_STATE_GE_E,     /* 20 */
    SKERNEL_PORT_STATE_GE_E,     /* 21 */
    SKERNEL_PORT_STATE_GE_E,     /* 22 */
    SKERNEL_PORT_STATE_GE_E,     /* 23 */

    SKERNEL_PORT_STATE_XG_E,     /* 24 */
    SKERNEL_PORT_STATE_XG_E,     /* 25 */
    SKERNEL_PORT_STATE_XG_E,     /* 26 */
    SKERNEL_PORT_STATE_XG_E      /* 27 */
};

/* xCat3 - 24G ports, 6XG ports and XG mac for CPU port (#31) */
const static  SKERNEL_PORT_STATE_ENT xcat3gig24Xg6PortsAndCpuPort[32] =
{
    SKERNEL_PORT_STATE_GE_E,     /* 0 */
    SKERNEL_PORT_STATE_GE_E,     /* 1 */
    SKERNEL_PORT_STATE_GE_E,     /* 2 */
    SKERNEL_PORT_STATE_GE_E,     /* 3 */

    SKERNEL_PORT_STATE_GE_E,     /* 4 */
    SKERNEL_PORT_STATE_GE_E,     /* 5 */
    SKERNEL_PORT_STATE_GE_E,     /* 6 */
    SKERNEL_PORT_STATE_GE_E,     /* 7 */

    SKERNEL_PORT_STATE_GE_E,     /* 8 */
    SKERNEL_PORT_STATE_GE_E,     /* 9 */
    SKERNEL_PORT_STATE_GE_E,     /* 10 */
    SKERNEL_PORT_STATE_GE_E,     /* 11 */

    SKERNEL_PORT_STATE_GE_E,     /* 12 */
    SKERNEL_PORT_STATE_GE_E,     /* 13 */
    SKERNEL_PORT_STATE_GE_E,     /* 14 */
    SKERNEL_PORT_STATE_GE_E,     /* 15 */

    SKERNEL_PORT_STATE_GE_E,     /* 16 */
    SKERNEL_PORT_STATE_GE_E,     /* 17 */
    SKERNEL_PORT_STATE_GE_E,     /* 18 */
    SKERNEL_PORT_STATE_GE_E,     /* 19 */

    SKERNEL_PORT_STATE_GE_E,     /* 20 */
    SKERNEL_PORT_STATE_GE_E,     /* 21 */
    SKERNEL_PORT_STATE_GE_E,     /* 22 */
    SKERNEL_PORT_STATE_GE_E,     /* 23 */

    SKERNEL_PORT_STATE_XG_E,     /* 24 */
    SKERNEL_PORT_STATE_XG_E,     /* 25 */
    SKERNEL_PORT_STATE_XG_E,     /* 26 */
    SKERNEL_PORT_STATE_XG_E,     /* 27 */
    SKERNEL_PORT_STATE_XG_E,     /* 28 */
    SKERNEL_PORT_STATE_XG_E,     /* 29 */

    SKERNEL_PORT_STATE_NOT_EXISTS_E, /* 30 */

    SKERNEL_PORT_STATE_XG_E      /* 31 */
};

/* default for 48Fe/GE + 4XG devices */
const static  SKERNEL_PORT_STATE_ENT gig48Xg4Ports[52] =
{
    SKERNEL_PORT_STATE_GE_E,       /* 0 */
    SKERNEL_PORT_STATE_GE_E,       /* 1 */
    SKERNEL_PORT_STATE_GE_E,       /* 2 */
    SKERNEL_PORT_STATE_GE_E,       /* 3 */

    SKERNEL_PORT_STATE_GE_E,       /* 4 */
    SKERNEL_PORT_STATE_GE_E,       /* 5 */
    SKERNEL_PORT_STATE_GE_E,       /* 6 */
    SKERNEL_PORT_STATE_GE_E,       /* 7 */

    SKERNEL_PORT_STATE_GE_E,       /* 8 */
    SKERNEL_PORT_STATE_GE_E,       /* 9 */
    SKERNEL_PORT_STATE_GE_E,       /* 10 */
    SKERNEL_PORT_STATE_GE_E,       /* 11 */

    SKERNEL_PORT_STATE_GE_E,       /* 12 */
    SKERNEL_PORT_STATE_GE_E,       /* 13 */
    SKERNEL_PORT_STATE_GE_E,       /* 14 */
    SKERNEL_PORT_STATE_GE_E,       /* 15 */

    SKERNEL_PORT_STATE_GE_E,       /* 16 */
    SKERNEL_PORT_STATE_GE_E,       /* 17 */
    SKERNEL_PORT_STATE_GE_E,       /* 18 */
    SKERNEL_PORT_STATE_GE_E,       /* 19 */

    SKERNEL_PORT_STATE_GE_E,       /* 20 */
    SKERNEL_PORT_STATE_GE_E,       /* 21 */
    SKERNEL_PORT_STATE_GE_E,       /* 22 */
    SKERNEL_PORT_STATE_GE_E,       /* 23 */

    SKERNEL_PORT_STATE_GE_E,       /* 24 */
    SKERNEL_PORT_STATE_GE_E,       /* 25 */
    SKERNEL_PORT_STATE_GE_E,       /* 26 */
    SKERNEL_PORT_STATE_GE_E,       /* 27 */

    SKERNEL_PORT_STATE_GE_E,       /* 28 */
    SKERNEL_PORT_STATE_GE_E,       /* 29 */
    SKERNEL_PORT_STATE_GE_E,       /* 30 */
    SKERNEL_PORT_STATE_GE_E,       /* 31 */

    SKERNEL_PORT_STATE_GE_E,       /* 32 */
    SKERNEL_PORT_STATE_GE_E,       /* 33 */
    SKERNEL_PORT_STATE_GE_E,       /* 34 */
    SKERNEL_PORT_STATE_GE_E,       /* 35 */

    SKERNEL_PORT_STATE_GE_E,       /* 36 */
    SKERNEL_PORT_STATE_GE_E,       /* 37 */
    SKERNEL_PORT_STATE_GE_E,       /* 38 */
    SKERNEL_PORT_STATE_GE_E,       /* 39 */

    SKERNEL_PORT_STATE_GE_E,       /* 40 */
    SKERNEL_PORT_STATE_GE_E,       /* 41 */
    SKERNEL_PORT_STATE_GE_E,       /* 42 */
    SKERNEL_PORT_STATE_GE_E,       /* 43 */

    SKERNEL_PORT_STATE_GE_E,       /* 44 */
    SKERNEL_PORT_STATE_GE_E,       /* 45 */
    SKERNEL_PORT_STATE_GE_E,       /* 46 */
    SKERNEL_PORT_STATE_GE_E,       /* 47 */

    SKERNEL_PORT_STATE_XG_E,       /* 48 */
    SKERNEL_PORT_STATE_XG_E,       /* 49 */
    SKERNEL_PORT_STATE_XG_E,       /* 50 */
    SKERNEL_PORT_STATE_XG_E        /* 51 */
};

/* default for 72 ports : 48 GE + 24 XG devices */
const static  SKERNEL_PORT_STATE_ENT ge48xg24Ports[BOBCAT2_NUM_OF_MAC_PORTS_CNS] =
{
    SKERNEL_PORT_STATE_GE_STACK_A1_E,/* use 'stack GE' to indicate MIB counters logic */       /* 0 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 1 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 2 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 3 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 4 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 5 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 6 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 7 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 8 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 9 */

    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 10 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 11 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 12 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 13 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 14 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 15 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 16 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 17 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 18 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 19 */

    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 20 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 21 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 22 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 23 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 24 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 25 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 26 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 27 */

    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 28 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 29 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 30 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 31 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 32 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 33 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 34 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 35 */

    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 36 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 37 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 38 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 39 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 40 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 41 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 42 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 43 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 44 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 45 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 46 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 47 */



    SKERNEL_PORT_STATE_XG_E,       /* 48 */
    SKERNEL_PORT_STATE_XG_E,       /* 49 */

    SKERNEL_PORT_STATE_XG_E,       /* 50 */
    SKERNEL_PORT_STATE_XG_E,       /* 51 */
    SKERNEL_PORT_STATE_XG_E,       /* 52 */
    SKERNEL_PORT_STATE_XG_E,       /* 53 */
    SKERNEL_PORT_STATE_XG_E,       /* 54 */
    SKERNEL_PORT_STATE_XG_E,       /* 55 */
    SKERNEL_PORT_STATE_XG_E,       /* 56 */
    SKERNEL_PORT_STATE_XG_E,       /* 57 */
    SKERNEL_PORT_STATE_XG_E,       /* 58 */
    SKERNEL_PORT_STATE_XG_E,       /* 59 */

    SKERNEL_PORT_STATE_XG_E,       /* 60 */
    SKERNEL_PORT_STATE_XG_E,       /* 61 */
    SKERNEL_PORT_STATE_XG_E,       /* 62 */
    SKERNEL_PORT_STATE_XG_E,       /* 63 */
    SKERNEL_PORT_STATE_XG_E,       /* 64 */
    SKERNEL_PORT_STATE_XG_E,       /* 65 */
    SKERNEL_PORT_STATE_XG_E,       /* 66 */
    SKERNEL_PORT_STATE_XG_E,       /* 67 */
    SKERNEL_PORT_STATE_XG_E,       /* 68 */
    SKERNEL_PORT_STATE_XG_E,       /* 69 */

    SKERNEL_PORT_STATE_XG_E,       /* 70 */
    SKERNEL_PORT_STATE_XG_E        /* 71 */

};

/* default for 72 ports : 48 GE + 12 XG devices (12 port not exists) :
    0..47 - GE
    48..55 - not exists
    56..59 - XG
    60..63 - not exists
    64..71 - XG

*/
const static  SKERNEL_PORT_STATE_ENT bobk_ge48xg12Ports[BOBCAT2_NUM_OF_MAC_PORTS_CNS] =
{
    SKERNEL_PORT_STATE_GE_STACK_A1_E,/* use 'stack GE' to indicate MIB counters logic */       /* 0 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 1 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 2 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 3 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 4 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 5 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 6 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 7 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 8 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 9 */

    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 10 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 11 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 12 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 13 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 14 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 15 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 16 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 17 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 18 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 19 */

    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 20 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 21 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 22 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 23 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 24 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 25 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 26 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 27 */

    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 28 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 29 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 30 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 31 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 32 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 33 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 34 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 35 */

    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 36 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 37 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 38 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 39 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 40 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 41 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 42 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 43 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 44 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 45 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 46 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,       /* 47 */



    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 48 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 49 */

    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 50 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 51 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 52 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 53 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 54 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 55 */
    SKERNEL_PORT_STATE_XG_E,       /* 56 */
    SKERNEL_PORT_STATE_XG_E,       /* 57 */
    SKERNEL_PORT_STATE_XG_E,       /* 58 */
    SKERNEL_PORT_STATE_XG_E,       /* 59 */

    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 60 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 61 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 62 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 63 */
    SKERNEL_PORT_STATE_XG_E,       /* 64 */
    SKERNEL_PORT_STATE_XG_E,       /* 65 */
    SKERNEL_PORT_STATE_XG_E,       /* 66 */
    SKERNEL_PORT_STATE_XG_E,       /* 67 */
    SKERNEL_PORT_STATE_XG_E,       /* 68 */
    SKERNEL_PORT_STATE_XG_E,       /* 69 */

    SKERNEL_PORT_STATE_XG_E,       /* 70 */
    SKERNEL_PORT_STATE_XG_E        /* 71 */

};

/* default for 72 ports all XG */
const static  SKERNEL_PORT_STATE_ENT xg72Ports[BOBCAT3_NUM_OF_MAC_PORTS_CNS] =
{
/*0..11*/
    SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E,
    SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E,
    SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E,
/*12..23*/
    SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E,
    SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E,
    SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E,
/*24..35*/
    SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E,
    SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E,
    SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E,
/*36..47*/
    SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E,
    SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E,
    SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E,
/*48..59*/
    SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E,
    SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E,
    SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E,
/*60..71*/
    SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E,
    SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E,
    SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E, SKERNEL_PORT_STATE_XG_E
};

/* default for 48FE + 4GE devices */
const static  SKERNEL_PORT_STATE_ENT fast48Gig4Ports[52] =
{
    SKERNEL_PORT_STATE_FE_E,       /* 0 */
    SKERNEL_PORT_STATE_FE_E,       /* 1 */
    SKERNEL_PORT_STATE_FE_E,       /* 2 */
    SKERNEL_PORT_STATE_FE_E,       /* 3 */

    SKERNEL_PORT_STATE_FE_E,       /* 4 */
    SKERNEL_PORT_STATE_FE_E,       /* 5 */
    SKERNEL_PORT_STATE_FE_E,       /* 6 */
    SKERNEL_PORT_STATE_FE_E,       /* 7 */

    SKERNEL_PORT_STATE_FE_E,       /* 8 */
    SKERNEL_PORT_STATE_FE_E,       /* 9 */
    SKERNEL_PORT_STATE_FE_E,       /* 10 */
    SKERNEL_PORT_STATE_FE_E,       /* 11 */

    SKERNEL_PORT_STATE_FE_E,       /* 12 */
    SKERNEL_PORT_STATE_FE_E,       /* 13 */
    SKERNEL_PORT_STATE_FE_E,       /* 14 */
    SKERNEL_PORT_STATE_FE_E,       /* 15 */

    SKERNEL_PORT_STATE_FE_E,       /* 16 */
    SKERNEL_PORT_STATE_FE_E,       /* 17 */
    SKERNEL_PORT_STATE_FE_E,       /* 18 */
    SKERNEL_PORT_STATE_FE_E,       /* 19 */

    SKERNEL_PORT_STATE_FE_E,       /* 20 */
    SKERNEL_PORT_STATE_FE_E,       /* 21 */
    SKERNEL_PORT_STATE_FE_E,       /* 22 */
    SKERNEL_PORT_STATE_FE_E,       /* 23 */

    SKERNEL_PORT_STATE_FE_E,       /* 24 */
    SKERNEL_PORT_STATE_FE_E,       /* 25 */
    SKERNEL_PORT_STATE_FE_E,       /* 26 */
    SKERNEL_PORT_STATE_FE_E,       /* 27 */

    SKERNEL_PORT_STATE_FE_E,       /* 28 */
    SKERNEL_PORT_STATE_FE_E,       /* 29 */
    SKERNEL_PORT_STATE_FE_E,       /* 30 */
    SKERNEL_PORT_STATE_FE_E,       /* 31 */

    SKERNEL_PORT_STATE_FE_E,       /* 32 */
    SKERNEL_PORT_STATE_FE_E,       /* 33 */
    SKERNEL_PORT_STATE_FE_E,       /* 34 */
    SKERNEL_PORT_STATE_FE_E,       /* 35 */

    SKERNEL_PORT_STATE_FE_E,       /* 36 */
    SKERNEL_PORT_STATE_FE_E,       /* 37 */
    SKERNEL_PORT_STATE_FE_E,       /* 38 */
    SKERNEL_PORT_STATE_FE_E,       /* 39 */

    SKERNEL_PORT_STATE_FE_E,       /* 40 */
    SKERNEL_PORT_STATE_FE_E,       /* 41 */
    SKERNEL_PORT_STATE_FE_E,       /* 42 */
    SKERNEL_PORT_STATE_FE_E,       /* 43 */

    SKERNEL_PORT_STATE_FE_E,       /* 44 */
    SKERNEL_PORT_STATE_FE_E,       /* 45 */
    SKERNEL_PORT_STATE_FE_E,       /* 46 */
    SKERNEL_PORT_STATE_FE_E,       /* 47 */

    SKERNEL_PORT_STATE_GE_E,       /* 48 */
    SKERNEL_PORT_STATE_GE_E,       /* 49 */
    SKERNEL_PORT_STATE_GE_E,       /* 50 */
    SKERNEL_PORT_STATE_GE_E        /* 51 */
};


/* default for 28XG devices */
const static  SKERNEL_PORT_STATE_ENT xg28Ports[28] =
{
    SKERNEL_PORT_STATE_XG_E,       /* 0 */
    SKERNEL_PORT_STATE_XG_E,       /* 1 */
    SKERNEL_PORT_STATE_XG_E,       /* 2 */
    SKERNEL_PORT_STATE_XG_E,       /* 3 */

    SKERNEL_PORT_STATE_XG_E,       /* 4 */
    SKERNEL_PORT_STATE_XG_E,       /* 5 */
    SKERNEL_PORT_STATE_XG_E,       /* 6 */
    SKERNEL_PORT_STATE_XG_E,       /* 7 */

    SKERNEL_PORT_STATE_XG_E,       /* 8 */
    SKERNEL_PORT_STATE_XG_E,       /* 9 */
    SKERNEL_PORT_STATE_XG_E,       /* 10 */
    SKERNEL_PORT_STATE_XG_E,       /* 11 */

    SKERNEL_PORT_STATE_XG_E,       /* 12 */
    SKERNEL_PORT_STATE_XG_E,       /* 13 */
    SKERNEL_PORT_STATE_XG_E,       /* 14 */
    SKERNEL_PORT_STATE_XG_E,       /* 15 */

    SKERNEL_PORT_STATE_XG_E,       /* 16 */
    SKERNEL_PORT_STATE_XG_E,       /* 17 */
    SKERNEL_PORT_STATE_XG_E,       /* 18 */
    SKERNEL_PORT_STATE_XG_E,       /* 19 */

    SKERNEL_PORT_STATE_XG_E,       /* 20 */
    SKERNEL_PORT_STATE_XG_E,       /* 21 */
    SKERNEL_PORT_STATE_XG_E,       /* 22 */
    SKERNEL_PORT_STATE_XG_E,       /* 23 */

    SKERNEL_PORT_STATE_XG_E,       /* 24 */
    SKERNEL_PORT_STATE_XG_E,       /* 25 */
    SKERNEL_PORT_STATE_XG_E,       /* 26 */
    SKERNEL_PORT_STATE_XG_E        /* 27 */

};

/* default for puma3Ports : core 0 : 12 NW ports, 12 fabric */
/* default for puma3Ports : core 1 : are 16+ the port numbers of core 0 : for 12 NW ports, 12 fabric*/
const static  SKERNEL_PORT_STATE_ENT puma3Ports[48] =
{
    SKERNEL_PORT_STATE_XG_E,       /* 0 */
    SKERNEL_PORT_STATE_XG_E,       /* 1 */
    SKERNEL_PORT_STATE_XG_E,       /* 2 */
    SKERNEL_PORT_STATE_XG_E,       /* 3 */

    SKERNEL_PORT_STATE_XG_E,       /* 4 */
    SKERNEL_PORT_STATE_XG_E,       /* 5 */
    SKERNEL_PORT_STATE_XG_E,       /* 6 */
    SKERNEL_PORT_STATE_XG_E,       /* 7 */

    SKERNEL_PORT_STATE_XG_E,       /* 8 */
    SKERNEL_PORT_STATE_XG_E,       /* 9 */
    SKERNEL_PORT_STATE_XG_E,       /* 10 */
    SKERNEL_PORT_STATE_XG_E,       /* 11 */

    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 12 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 13 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 14 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 15 */

    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 16 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 17 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 18 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 19 */

    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 20 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 21 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 22 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 23 */

    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 24 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 25 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 26 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 27 */

    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 28 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 29 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 30 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 31 */

    SKERNEL_PORT_STATE_XG_E,       /* 32 */
    SKERNEL_PORT_STATE_XG_E,       /* 33 */
    SKERNEL_PORT_STATE_XG_E,       /* 34 */
    SKERNEL_PORT_STATE_XG_E,       /* 35 */

    SKERNEL_PORT_STATE_XG_E,       /* 36 */
    SKERNEL_PORT_STATE_XG_E,       /* 37 */
    SKERNEL_PORT_STATE_XG_E,       /* 38 */
    SKERNEL_PORT_STATE_XG_E,       /* 39 */

    SKERNEL_PORT_STATE_XG_E,       /* 40 */
    SKERNEL_PORT_STATE_XG_E,       /* 41 */
    SKERNEL_PORT_STATE_XG_E,       /* 42 */
    SKERNEL_PORT_STATE_XG_E,       /* 43 */

    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 44 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 45 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E,       /* 46 */
    SKERNEL_PORT_STATE_NOT_EXISTS_E        /* 47 */

};

/* default for 24FE + 4GE devices */
const static  SKERNEL_PORT_STATE_ENT fast24Gig4Ports[28] =
{
    SKERNEL_PORT_STATE_FE_E,     /* 0 */
    SKERNEL_PORT_STATE_FE_E,     /* 1 */
    SKERNEL_PORT_STATE_FE_E,     /* 2 */
    SKERNEL_PORT_STATE_FE_E,     /* 3 */

    SKERNEL_PORT_STATE_FE_E,     /* 4 */
    SKERNEL_PORT_STATE_FE_E,     /* 5 */
    SKERNEL_PORT_STATE_FE_E,     /* 6 */
    SKERNEL_PORT_STATE_FE_E,     /* 7 */

    SKERNEL_PORT_STATE_FE_E,     /* 8 */
    SKERNEL_PORT_STATE_FE_E,     /* 9 */
    SKERNEL_PORT_STATE_FE_E,     /* 10 */
    SKERNEL_PORT_STATE_FE_E,     /* 11 */

    SKERNEL_PORT_STATE_FE_E,     /* 12 */
    SKERNEL_PORT_STATE_FE_E,     /* 13 */
    SKERNEL_PORT_STATE_FE_E,     /* 14 */
    SKERNEL_PORT_STATE_FE_E,     /* 15 */

    SKERNEL_PORT_STATE_FE_E,     /* 16 */
    SKERNEL_PORT_STATE_FE_E,     /* 17 */
    SKERNEL_PORT_STATE_FE_E,     /* 18 */
    SKERNEL_PORT_STATE_FE_E,     /* 19 */

    SKERNEL_PORT_STATE_FE_E,     /* 20 */
    SKERNEL_PORT_STATE_FE_E,     /* 21 */
    SKERNEL_PORT_STATE_FE_E,     /* 22 */
    SKERNEL_PORT_STATE_FE_E,     /* 23 */

    SKERNEL_PORT_STATE_GE_STACK_A1_E,     /* 24 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,     /* 25 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E,     /* 26 */
    SKERNEL_PORT_STATE_GE_STACK_A1_E      /* 27 */
};


/* simSupportedTypes :
    purpose :
        DB to hold the device types that the sim support

fields :
    devFamily - device family (cheetah/salsa/tiger/../puma)
    numOfPorts - number of ports in the device
    devTypeArray - array of devices for the device family that has same number
                   of ports
    defaultPortsBmpPtr - pointer to special ports bitmap.
                         if this is NULL , that means that the default bitmap
                         defined by the number of ports that considered to be
                         sequential for 0 to the numOfPorts
    multiModePortsBmpPtr - pointer to ports bitmap that support multi state mode.
                         if this is NULL , no port support 'Multi mode'
    existingPortsStatePtr - pointer to array of port's state that state the
                            'default' state of port.
                            NEVER NULL pointer
                          --> NOTE: the relevant ports are only the ports that
                                    set in defaultPortsBmpPtr  !!!

*/
const static struct {
    SKERNEL_DEVICE_FAMILY_TYPE  devFamily;
    GT_U8                       numOfPorts;
    const SKERNEL_DEVICE_TYPE   *devTypeArray;
    const PORTS_BMP_STC    *defaultPortsBmpPtr;
    const PORTS_BMP_STC    *multiModePortsBmpPtr;
    const SKERNEL_PORT_STATE_ENT *existingPortsStatePtr;
}simSupportedTypes[] =
{
    {SKERNEL_TWIST_C_FAMILY   ,52 ,twistc52legalDevTypes ,NULL,NULL,fast48Gig4Ports},
    {SKERNEL_TWIST_C_FAMILY   ,12 ,twistc12legalDevTypes ,NULL,NULL,gig24Xg4Ports},
    {SKERNEL_TWIST_C_FAMILY   ,10 ,twistc10legalDevTypes ,NULL,NULL,gig24Xg4Ports},

    {SKERNEL_TWIST_D_FAMILY   ,52 , twistd52legalDevTypes,NULL,NULL,fast48Gig4Ports},
    {SKERNEL_TWIST_D_FAMILY   ,12 , twistd12legalDevTypes,NULL,NULL,gig24Xg4Ports},
    {SKERNEL_TWIST_D_FAMILY   , 1 , twistdXGlegalDevTypes,NULL,NULL,xg28Ports},

    {SKERNEL_SAMBA_FAMILY   ,52 ,samba52legalDevTypes, NULL, NULL,fast48Gig4Ports},
    {SKERNEL_SAMBA_FAMILY   ,12 ,samba12legalDevTypes, NULL, NULL,gig24Xg4Ports},
    {SKERNEL_SAMBA_FAMILY   , 1 ,sambaXGlegalDevTypes, NULL, NULL,xg28Ports},

    {SKERNEL_TIGER_FAMILY   ,52 ,tg52legalDevTypes, NULL, NULL,fast48Gig4Ports},
    {SKERNEL_TIGER_FAMILY   ,12 ,tg12legalDevTypes, NULL, NULL,gig24Xg4Ports},
    {SKERNEL_TIGER_FAMILY   , 1 ,tgXGlegalDevTypes, NULL, NULL,xg28Ports},

    {SKERNEL_SALSA_FAMILY   ,12 ,salsa12legalDevTypes, NULL, NULL,gig24Xg4Ports},
    {SKERNEL_SALSA_FAMILY   ,16 ,salsa16legalDevTypes, NULL, NULL,gig24Xg4Ports},
    {SKERNEL_SALSA_FAMILY   ,24 ,salsa24legalDevTypes, NULL, NULL,gig24Xg4Ports},

    {SKERNEL_CHEETAH_1_FAMILY   ,27 ,ch_27legalDevTypes, NULL, NULL,gig24Xg4Ports},
    {SKERNEL_CHEETAH_1_FAMILY   ,26 ,ch_26legalDevTypes, NULL, NULL,gig24Xg4Ports},
    {SKERNEL_CHEETAH_1_FAMILY   ,24 ,ch_24legalDevTypes, NULL, NULL,gig24Xg4Ports},
    {SKERNEL_CHEETAH_1_FAMILY   ,25 ,ch_25legalDevTypes, NULL, NULL,gig24Xg4Ports},
    {SKERNEL_CHEETAH_1_FAMILY   ,16 ,ch_16legalDevTypes, NULL, NULL,gig24Xg4Ports},
    {SKERNEL_CHEETAH_1_FAMILY   ,10 ,ch_10legalDevTypes, NULL, NULL,gig24Xg4Ports},

    {SKERNEL_CHEETAH_2_FAMILY   ,28 ,ch2_28legalDevTypes, NULL, NULL,gig24Xg4Ports},
    {SKERNEL_CHEETAH_2_FAMILY   ,27 ,ch2_27legalDevTypes, NULL, NULL,gig24Xg4Ports},
    {SKERNEL_CHEETAH_2_FAMILY   ,26 ,ch2_26legalDevTypes, NULL, NULL,gig24Xg4Ports},
    {SKERNEL_CHEETAH_2_FAMILY   ,24 ,ch2_24legalDevTypes, NULL, NULL,gig24Xg4Ports},

    {SKERNEL_CHEETAH_3_FAMILY   ,28 ,ch3_28legalDevTypes, NULL, NULL,gig24Xg4Ports},
    {SKERNEL_CHEETAH_3_FAMILY   ,26 ,ch3_26legalDevTypes, NULL, NULL,gig24Xg4Ports},
    {SKERNEL_CHEETAH_3_FAMILY   ,24 ,ch3_24legalDevTypes, NULL, NULL,gig24Xg4Ports},
    {SKERNEL_CHEETAH_3_FAMILY   ,26 ,ch3_6legalDevTypes,  &portsBmp0to3_24_25,NULL,gig24Xg4Ports},
    {SKERNEL_CHEETAH_3_FAMILY   ,26 ,ch3_10legalDevTypes, &portsBmp0to7_24_25,NULL,gig24Xg4Ports},
    {SKERNEL_CHEETAH_3_FAMILY   ,26 ,ch3_18legalDevTypes, &portsBmp0to15_24_25,NULL,gig24Xg4Ports},
    {SKERNEL_CHEETAH_3_FAMILY   ,28 ,ch3_20legalDevTypes, &portsBmp0to15_24to27,NULL,gig24Xg4Ports},
    {SKERNEL_CHEETAH_3_FAMILY   ,26 ,ch3_24_2legalDevTypes, NULL,NULL,gig24Xg4Ports},
    {SKERNEL_CHEETAH_3_FAMILY   ,28 ,ch3_24_4legalDevTypes, NULL,NULL,gig24Xg4Ports},
    {SKERNEL_CHEETAH_3_FAMILY   ,28 ,ch3_8_XGlegalDevTypes, &portsBmpCh3_8_Xg,NULL,xg28Ports},
    {SKERNEL_CHEETAH_3_FAMILY   ,28 ,ch3_XGlegalDevTypes, &portsBmpCh3Xg,NULL,xg28Ports},

    {SKERNEL_XCAT_FAMILY        ,24+4 ,xcat_24_4legalDevTypes, NULL,&portsBmpMultiState_24to27,gig24Xg4Ports},
    {SKERNEL_XCAT3_FAMILY       ,24+6+2 ,xcat3_24_6legalDevTypes, NULL,NULL,xcat3gig24Xg6PortsAndCpuPort},
    {SKERNEL_XCAT2_FAMILY       ,24+4 ,xcat2_24_4legalDevTypes, NULL,NULL,fast24Gig4Ports},

    /* support for the Lion's port groups -- 12 ports of XG (support also GE mode)*/
    {SKERNEL_LION_PORT_GROUP_FAMILY   ,12   ,lionPortGroup_12_legalDevTypes, NULL,&portsBmpMultiState_0to11,xg28Ports},
    /* support for the Lion (shell)*/
    {SKERNEL_LION_PORT_GROUP_SHARED_FAMILY   ,0    ,lion_48_legalDevTypes, NULL,NULL,NULL},

    {SKERNEL_PUMA_FAMILY   ,28 ,puma_26legalDevTypes, &portsBmp24_25_27,NULL,xg28Ports},
    {SKERNEL_PUMA_FAMILY   ,28 ,puma_24legalDevTypes, &portsBmp0to23_27,NULL,gig24Xg4Ports},


    /* support for the generic PHY shell */
    {SKERNEL_PHY_SHELL_FAMILY   ,0   ,phy_shell_legalDevTypes, NULL,NULL,NULL},
    /* support for the PHY core */
    {SKERNEL_PHY_CORE_FAMILY   ,2 /* single channel --> 2 ports*/   ,phy_core_legalDevTypes, NULL,NULL,gig24Xg4Ports},

    /* support macsec */
    {SKERNEL_MACSEC_FAMILY   ,8 /* 4 channels --> 8 ports*/   ,macsec_legalDevTypes, NULL,NULL,gig24Xg4Ports},

    {SKERNEL_EMPTY_FAMILY , 0 , empty_legalDevTypes , NULL,NULL,NULL},

    /* support for the Lion2's port groups -- 12 ports of XG (support also GE mode)*/
    {SKERNEL_LION2_PORT_GROUP_FAMILY   ,12   ,lion2PortGroup_12_legalDevTypes, NULL,&portsBmpMultiState_0to11,xg28Ports},
    /* support for the Lion2 (shell)*/
    {SKERNEL_LION2_PORT_GROUP_SHARED_FAMILY   ,0    ,lion2_96_legalDevTypes, NULL,NULL,NULL},

    /* support for the Lion3's port groups -- 12 ports of XG (support also GE mode)*/
    {SKERNEL_LION3_PORT_GROUP_FAMILY,12,lion3PortGroup_12_legalDevTypes, NULL,&portsBmpMultiState_0to11,xg28Ports},
    /* support for the Lion3 (shell)*/
    {SKERNEL_LION3_PORT_GROUP_SHARED_FAMILY,0,lion3_96_legalDevTypes, NULL,NULL,NULL},


    /* support for the Puma3's network+fabric -- 48 ports of XG (support also GE mode)*/
    {SKERNEL_PUMA3_NETWORK_FABRIC_FAMILY   ,48   ,puma3NetworkFabric_legalDevTypes, &portsBmpMultiState_0to11_32to47,&portsBmpMultiState_0to11_32to47,puma3Ports},
    /* support for the Lion2 (shell)*/
    {SKERNEL_PUMA3_SHARED_FAMILY   ,0    ,puma3_64_legalDevTypes, NULL,NULL,NULL},

    /* support for the BobCat2 */
    {SKERNEL_BOBCAT2_FAMILY, BOBCAT2_NUM_OF_MAC_PORTS_CNS, bobcat2_legalDevTypes, NULL, &portsBmpMultiState_48to71, ge48xg24Ports},

    /* support for the bobk-caelum */
    {SKERNEL_BOBK_CAELUM_FAMILY, BOBCAT2_NUM_OF_MAC_PORTS_CNS, bobk_caelum_legalDevTypes, NULL, &portsBmpMultiState_48to71, bobk_ge48xg12Ports},

    /* support for the bobk-cetus */
    {SKERNEL_BOBK_CETUS_FAMILY, BOBCAT2_NUM_OF_MAC_PORTS_CNS, bobk_cetus_legalDevTypes, &portsBmp56to59and62and64to71, &portsBmpMultiState_48to71, bobk_ge48xg12Ports},

    /* support for the BobCat3 */
    {SKERNEL_BOBCAT3_FAMILY, BOBCAT3_NUM_OF_MAC_PORTS_CNS, bobcat3_legalDevTypes, NULL, &portsBmpMultiState_0to71, xg72Ports},


    /* End of list      */
    {END_OF_TABLE, 0, NULL, NULL}
};

/* bmp of ports with no ports in it */
#define EMPTY_PORTS_BMP_CNS {{0,0}}

/* simSpecialDevicesBmp -
 * Purpose : DB to hold the devices with special ports BMP
 *
 * NOTE : devices that his port are sequential for 0 to the numOfPorts
 *        not need to be in this Array !!!
 *
 * fields :
 *  devType - device type that has special ports bmp (that is different from
 *            those of other devices of his family with the same numOfPort)
 *  existingPorts - the special ports bmp of the device
 *  multiModePortsBmp - ports bitmap that support multi state mode.
 *
*/
const static struct {
    SKERNEL_DEVICE_TYPE    devType;
    PORTS_BMP_STC          existingPorts;
    PORTS_BMP_STC          multiModePortsBmp;

}simSpecialDevicesBmp[] =
{
    /* 12 Giga ports (0.11) and XG 24,25 */
    {SKERNEL_98DX145 , {{PORT_RANGE_MAC(0,12) | PORT_RANGE_MAC(24,2) ,0}} , EMPTY_PORTS_BMP_CNS},
    /* 12 Giga ports (0.11) and XG 24 */
    {SKERNEL_98DX133 , {{PORT_RANGE_MAC(0,12) | PORT_RANGE_MAC(24,1) ,0}} , EMPTY_PORTS_BMP_CNS},
    /* No Giga ports and 3XG 24..26 */
    {SKERNEL_98DX803 , {{PORT_RANGE_MAC(24,3) ,0}} , EMPTY_PORTS_BMP_CNS},
    /* No Giga ports and 4XG 24..27 */
    {SKERNEL_98DX804 , {{PORT_RANGE_MAC(24,4) ,0}} , EMPTY_PORTS_BMP_CNS},

    /* End of list      */
    {END_OF_TABLE   ,{{0,0}} , EMPTY_PORTS_BMP_CNS}
};



#define NO_CPU_GMII_PORT_CNS 0xFFFFFFFF
#define SALSA_CPU_PORT_CNS  31
#define PRESTERA_CPU_PORT_CNS 63
#define XCAT3_CPU_PORT_CNS  31

/* device names database */
static SMAIN_DEV_NAME_STC    smainDevNameDb[] =
{
    /* salsa */
    {SMAIN_NO_GM_DEVICE, "98dx160", SKERNEL_98DX160,   16,SALSA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx161", SKERNEL_98DX161,   16,SALSA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx240", SKERNEL_98DX240,   24,SALSA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx241", SKERNEL_98DX241,   24,SALSA_CPU_PORT_CNS},
    /* salsa 2 */
    {SMAIN_NO_GM_DEVICE, "98dx1602", SKERNEL_98DX1602, 16,SALSA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx1612", SKERNEL_98DX1612, 16,SALSA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx2402", SKERNEL_98DX2402, 24,SALSA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx2412", SKERNEL_98DX2412, 24,SALSA_CPU_PORT_CNS},
    /* nic */
    {SMAIN_NO_GM_DEVICE, "nic",      SKERNEL_NIC,       1,NO_CPU_GMII_PORT_CNS},
    /* twist-d*/
    {SMAIN_NO_GM_DEVICE, "98ex100", SKERNEL_98EX100D,  28,NO_CPU_GMII_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98ex110", SKERNEL_98EX110D,  52,NO_CPU_GMII_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98ex115", SKERNEL_98EX115D,  52,NO_CPU_GMII_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98ex120", SKERNEL_98EX120D,  12,NO_CPU_GMII_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98ex125", SKERNEL_98EX125D,  52,NO_CPU_GMII_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98ex130", SKERNEL_98EX130D,  1,NO_CPU_GMII_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98ex135", SKERNEL_98EX135D,  1,NO_CPU_GMII_PORT_CNS},
    /* samba devices */
    {SMAIN_NO_GM_DEVICE, "98mx615", SKERNEL_98MX615D,  52,NO_CPU_GMII_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98mx625", SKERNEL_98MX625D,  12,NO_CPU_GMII_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98mx635", SKERNEL_98MX635D,  1,NO_CPU_GMII_PORT_CNS},
    /* TwistC devices */
    {SMAIN_NO_GM_DEVICE, "98mx610b", SKERNEL_98MX610B, 52,NO_CPU_GMII_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98mx620b", SKERNEL_98MX620B, 10,NO_CPU_GMII_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98ex110b", SKERNEL_98EX110B, 52,NO_CPU_GMII_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98ex120b", SKERNEL_98EX120B, 12,NO_CPU_GMII_PORT_CNS},
    /* Sapphire */
    {SMAIN_NO_GM_DEVICE, "88e6183",  SKERNEL_SAPPHIRE, 10,NO_CPU_GMII_PORT_CNS},
    /* Ruby */
    {SMAIN_NO_GM_DEVICE, "88e6093",  SKERNEL_RUBY, 11,NO_CPU_GMII_PORT_CNS},
    /* Opal */
    {SMAIN_NO_GM_DEVICE, "88e6095", SKERNEL_OPAL, 11,NO_CPU_GMII_PORT_CNS},
    /* Opal Plus*/
    {SMAIN_NO_GM_DEVICE, "88e6097", SKERNEL_OPAL_PLUS, 11,NO_CPU_GMII_PORT_CNS},
    /* Jade */
    {SMAIN_NO_GM_DEVICE, "88e6185", SKERNEL_JADE, 10,NO_CPU_GMII_PORT_CNS},
    /* Tiger - Golden Model */
    {TIGER_GOLDEN_MODEL_MII , "98ex116gm", SKERNEL_98EX116, 52,NO_CPU_GMII_PORT_CNS},
    {TIGER_GOLDEN_MODEL_XII , "98ex126gm", SKERNEL_98EX126, 12,NO_CPU_GMII_PORT_CNS},
    /* Tiger */
    {SMAIN_NO_GM_DEVICE, "98ex116", SKERNEL_98EX116, 52,NO_CPU_GMII_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98ex126", SKERNEL_98EX126, 12,NO_CPU_GMII_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98ex136", SKERNEL_98EX136, 1,NO_CPU_GMII_PORT_CNS},
    /* Fabric Adapter */
    {SMAIN_NO_GM_DEVICE, "98fx900" , SKERNEL_98FX900 , 5,NO_CPU_GMII_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98fx910" , SKERNEL_98FX910 , 5,NO_CPU_GMII_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98fx915" , SKERNEL_98FX915 , 5,NO_CPU_GMII_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98fx920" , SKERNEL_98FX920 , 5,NO_CPU_GMII_PORT_CNS} ,
    {SMAIN_NO_GM_DEVICE, "98fx930" , SKERNEL_98FX930 , 9,NO_CPU_GMII_PORT_CNS} ,
    {SMAIN_NO_GM_DEVICE, "98fx950" , SKERNEL_98FX950 , 9,NO_CPU_GMII_PORT_CNS} ,

    /* Xbar Adapter */
    {SMAIN_NO_GM_DEVICE, "98fx9110" , SKERNEL_98FX9110 , 12,NO_CPU_GMII_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98fx9210" , SKERNEL_98FX9210 , 9,NO_CPU_GMII_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98fx9310" , SKERNEL_98FX9310 , 64,NO_CPU_GMII_PORT_CNS},
    /* Cheetah */
    {SMAIN_NO_GM_DEVICE, "98dx270", SKERNEL_98DX270, 27,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx260", SKERNEL_98DX260, 26,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx250", SKERNEL_98DX250, 24,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx249", SKERNEL_98DX249, 27,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx269", SKERNEL_98DX269, 27,PRESTERA_CPU_PORT_CNS},
    /* Cheetah Plus */
    {SMAIN_NO_GM_DEVICE, "98dx163", SKERNEL_98DX163, 16,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx243", SKERNEL_98DX243, 24,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx253", SKERNEL_98DX253, 24,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx263", SKERNEL_98DX263, 26,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx273", SKERNEL_98DX273, 27,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx803", SKERNEL_98DX803, 27,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx133", SKERNEL_98DX133, 24,PRESTERA_CPU_PORT_CNS},
    /* Cheetah++ */
    {SMAIN_NO_GM_DEVICE, "98dx107" , SKERNEL_98DX107, 24,PRESTERA_CPU_PORT_CNS},
    /* Generic DXCH */
    {SMAIN_NO_GM_DEVICE, "dxch"  , SKERNEL_DXCH     , 27,PRESTERA_CPU_PORT_CNS},/* generic DXCH*/
    {SMAIN_NO_GM_DEVICE, "dxch_b0", SKERNEL_DXCH_B0 , 10,PRESTERA_CPU_PORT_CNS},/* generic DXCH_B0 */
    /* Generic DXCH2 */
    {SMAIN_NO_GM_DEVICE, "dxch2" , SKERNEL_DXCH2    , 28,PRESTERA_CPU_PORT_CNS},/* generic DXCH2*/
    /* Generic DXCH3 */
    {SMAIN_NO_GM_DEVICE, "dxch3" , SKERNEL_DXCH3    , 28,PRESTERA_CPU_PORT_CNS},/* generic DXCH3*/
    /* Generic DXCH3_XG */
    {SMAIN_NO_GM_DEVICE, "dxch3xg" , SKERNEL_DXCH3_XG    , 28,PRESTERA_CPU_PORT_CNS},/* generic DXCH3_XG*/
    /* Generic DXCH3P (cheetah 3+) */
    {SMAIN_NO_GM_DEVICE, "dxch3p" , SKERNEL_DXCH3P  , 28,PRESTERA_CPU_PORT_CNS},/* generic DXCH3 Plus*/
    /* Generic embedded CPU (cheetah 3+) */
    {SMAIN_NO_GM_DEVICE, "embedded_cpu" , SKERNEL_EMBEDDED_CPU , 1,NO_CPU_GMII_PORT_CNS},/* generic Embedded CPU */
    /* Generic xCat */
    {SMAIN_NO_GM_DEVICE, "xcat_24_and_4" , SKERNEL_XCAT_24_AND_4   , 28, PRESTERA_CPU_PORT_CNS},/* xCat 24+4(DX) */
    /* Generic xCat3 */
    {SMAIN_NO_GM_DEVICE, "xcat3_24_and_6" , SKERNEL_XCAT3_24_AND_6   , 30, XCAT3_CPU_PORT_CNS},/* xCat3 24+6(DX) */
    /* Generic xCat2 */
    {SMAIN_NO_GM_DEVICE, "xcat2_24_and_4" , SKERNEL_XCAT2_24_AND_4   , 28, PRESTERA_CPU_PORT_CNS},/* xCat2 24+4(DX) */
    /* Generic XCat2 - GM */
    {GOLDEN_MODEL, "xcat2_24_and_4gm" , SKERNEL_XCAT2_24_AND_4   , 28, PRESTERA_CPU_PORT_CNS},/* xCat2 24+4(DX) */
    /* Generic Lion's port group */
    {SMAIN_NO_GM_DEVICE, "lion_port_group_12" , SKERNEL_LION_PORT_GROUP_12    , 12,PRESTERA_CPU_PORT_CNS},/* Lion's port group 12 ports(DX) */
    /* Generic Lion device */
    {SMAIN_NO_GM_DEVICE, "lion_48" , SKERNEL_LION_48 , 0/*no own ports--> use port group*/,NO_CPU_GMII_PORT_CNS},/* Lion (DX) */
    /* Generic Lion's port group - GM */
    {GOLDEN_MODEL, "lion_port_group_12gm" , SKERNEL_LION_PORT_GROUP_12    , 12,PRESTERA_CPU_PORT_CNS},/* Lion's port group 12 ports(DX) */

    /* Generic Lion2's port group */
    {SMAIN_NO_GM_DEVICE, "lion2_port_group_12" , SKERNEL_LION2_PORT_GROUP_12    , 12,PRESTERA_CPU_PORT_CNS},/* Lion2's port group 12 ports(DX) */
    /* Generic Lion2 device */
    {SMAIN_NO_GM_DEVICE, "lion2_96" , SKERNEL_LION2_96 , 0/*no own ports--> use port group*/,NO_CPU_GMII_PORT_CNS},/* Lion2 (DX) */
    /* Generic Lion2's port group - GM  */
    {GOLDEN_MODEL, "lion2_port_group_12gm" , SKERNEL_LION2_PORT_GROUP_12    , 12,PRESTERA_CPU_PORT_CNS},/* Lion2's port group 12 ports(DX) */

    /* Generic Lion3's port group */
    {SMAIN_NO_GM_DEVICE, "lion3_port_group_12" , SKERNEL_LION3_PORT_GROUP_12    , 12,PRESTERA_CPU_PORT_CNS},/* Lion3's port group 12 ports(DX) */
    /* Generic Lion3 device */
    {SMAIN_NO_GM_DEVICE, "lion3_96" , SKERNEL_LION3_96 , 0/*no own ports--> use port group*/,NO_CPU_GMII_PORT_CNS},/* Lion3 (DX) */
    /* Generic Lion3's port group - GM  */
    {GOLDEN_MODEL, "lion3_port_group_12gm" , SKERNEL_LION3_PORT_GROUP_12    , 12,PRESTERA_CPU_PORT_CNS},/* Lion3's port group 12 ports(DX) */

    /* Generic Bobcat2 - GM */
    {GOLDEN_MODEL   , "bobcat2_gm" , SKERNEL_BOBCAT2, BOBCAT2_NUM_OF_MAC_PORTS_CNS, NO_CPU_GMII_PORT_CNS},/* Bobcat2 GM - DX */
    /* Generic Bobcat2 (non GM device)*/
    {SMAIN_NO_GM_DEVICE, "bobcat2" , SKERNEL_BOBCAT2, BOBCAT2_NUM_OF_MAC_PORTS_CNS, NO_CPU_GMII_PORT_CNS},/* Bobcat2 (not GM) - DX */

    /* Generic Bobcat2 (non GM device)*/
    {SMAIN_NO_GM_DEVICE, "bobk-caelum" , SKERNEL_BOBK_CAELUM, BOBCAT2_NUM_OF_MAC_PORTS_CNS, NO_CPU_GMII_PORT_CNS},/* Bobk-Caelum (not GM) - DX */
    {SMAIN_NO_GM_DEVICE, "bobk-cetus"  , SKERNEL_BOBK_CETUS , BOBCAT2_NUM_OF_MAC_PORTS_CNS, NO_CPU_GMII_PORT_CNS},/* Bobk-Cetus  (not GM) - DX */

    {GOLDEN_MODEL   , "bobk-caelum_gm" , SKERNEL_BOBK_CAELUM, BOBCAT2_NUM_OF_MAC_PORTS_CNS, NO_CPU_GMII_PORT_CNS},/* Bobk-Caelum GM - DX */
    {GOLDEN_MODEL   , "bobk-cetus_gm"  , SKERNEL_BOBK_CETUS , BOBCAT2_NUM_OF_MAC_PORTS_CNS, NO_CPU_GMII_PORT_CNS},/* Bobk-Cetus  GM - DX */

    /* Generic Bobcat3 (non GM device)*/
    {SMAIN_NO_GM_DEVICE, "bobcat3" , SKERNEL_BOBCAT3, BOBCAT3_NUM_OF_MAC_PORTS_CNS, NO_CPU_GMII_PORT_CNS},/* Bobcat3 (not GM) - DX */
    /* Generic Bobcat3 - GM */
    {GOLDEN_MODEL   , "bobcat3_gm" , SKERNEL_BOBCAT3, BOBCAT3_NUM_OF_MAC_PORTS_CNS, NO_CPU_GMII_PORT_CNS},/* Bobcat3 GM - DX */

    /* Cheetah2 */
    {SMAIN_NO_GM_DEVICE, "98dx255" , SKERNEL_98DX255, 24,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx265" , SKERNEL_98DX265, 26,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx275" , SKERNEL_98DX275, 27,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx285" , SKERNEL_98DX285, 28,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx804" , SKERNEL_98DX804, 28,PRESTERA_CPU_PORT_CNS},
    /* Cheetah3 */
    {SMAIN_NO_GM_DEVICE, "98dx286" , SKERNEL_98DX5128,  28,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx806" , SKERNEL_98DX806,   28,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx5128", SKERNEL_98DX5128,  28,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx5128_1", SKERNEL_98DX5128_1,28,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx5124", SKERNEL_98DX5124,  28,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx5126", SKERNEL_98DX5126,  28,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx5127", SKERNEL_98DX5127,  28,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx5129", SKERNEL_98DX5129,  28,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx5151", SKERNEL_98DX5151,  28,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx5152", SKERNEL_98DX5152,  28,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx5154" , SKERNEL_98DX5154, 28,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx5155" , SKERNEL_98DX5155, 28,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx5156", SKERNEL_98DX5156,  28,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx5157" , SKERNEL_98DX5157, 28,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx8108" , SKERNEL_98DX8108, 28,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx8109", SKERNEL_98DX8109,  28,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx8110", SKERNEL_98DX8110,  28,PRESTERA_CPU_PORT_CNS},
    {SMAIN_NO_GM_DEVICE, "98dx8110_1", SKERNEL_98DX8110_1,28,PRESTERA_CPU_PORT_CNS},

    /* Puma */
    {SMAIN_NO_GM_DEVICE, "98ex240"   , SKERNEL_98EX240, 28,NO_CPU_GMII_PORT_CNS},
    {GOLDEN_MODEL      , "98ex240gm" , SKERNEL_98EX240, 28,NO_CPU_GMII_PORT_CNS},
    {GOLDEN_MODEL      , "98ex260gm" , SKERNEL_98EX241, 28,NO_CPU_GMII_PORT_CNS},

    /* Generic Puma3 device */
    {SMAIN_NO_GM_DEVICE, "puma3_64" , SKERNEL_PUMA3_64 , 0/*no own ports--> use port group*/,NO_CPU_GMII_PORT_CNS},
    {GOLDEN_MODEL      , "puma3_nw_fa_gm" , SKERNEL_PUMA3_NETWORK_FABRIC, 48,NO_CPU_GMII_PORT_CNS},

    /* Generic PHY multi channels */
    {SMAIN_NO_GM_DEVICE, "phy" , SKERNEL_PHY_SHELL , 0/*no own ports--> use port group*/,NO_CPU_GMII_PORT_CNS},
    /* PHY core 1540M (single channel) */
    {SMAIN_NO_GM_DEVICE, "phy_core_1540m_1548m" , SKERNEL_PHY_CORE_1540M_1548M , 2 /*single channel - 2 ports*/,NO_CPU_GMII_PORT_CNS},
    /* MACSEC (MAC-security) - 'linkCrypt' */
    {SMAIN_NO_GM_DEVICE, "macsec" , SKERNEL_MACSEC , 8 /* 4 channels - 8 ports*/,NO_CPU_GMII_PORT_CNS},

    /* non-exists device -- for 'skip' devices  */
    {SMAIN_NO_GM_DEVICE, "empty" , SKERNEL_EMPTY , 0,NO_CPU_GMII_PORT_CNS},

    /* Communication Module */
    {SMAIN_NO_GM_DEVICE  , "communication_card", SKERNEL_COM_MODULE,0,NO_CPU_GMII_PORT_CNS},
};

#define SMAIN_DEV_NAME_DB_SIZE (sizeof(smainDevNameDb)/sizeof(smainDevNameDb[1]))

#define INI_FILE_SLAN_FOR_NON_EXIST_PORT_CHECK_MAC(_nonExistPort,_port,_deviceId)    \
            if((_nonExistPort) == GT_TRUE)                          \
            {                                                       \
                if(deviceObjPtr->deviceFamily == SKERNEL_PUMA3_NETWORK_FABRIC_FAMILY) \
                {                                                   \
                    /* no warning -- valid case for puma3*/         \
                }                                                   \
                else                                                \
                {                                                   \
                    simWarningPrintf(" smainDevice2SlanBind : non-exist port [%d] in device [%d] , still you try to bind it to SLAN ?! \n",\
                             (_port) , (_deviceId));                \
                }                                                   \
                continue;                                           \
            }

/* device objects database */
SKERNEL_DEVICE_OBJECT * smainDeviceObjects[SMAIN_MAX_NUM_OF_DEVICES_CNS];

/* struct to hold info about the several buses in the system*/
typedef struct{
    GT_U32      id;   /* the bus id */
    GT_U32      membersBmp[(SMAIN_MAX_NUM_OF_DEVICES_CNS+31)/32];/* bmp of devices that members in the bus */
}BUS_INFO_STC;

#define MAX_BUS_NUM_CNS 32
static BUS_INFO_STC busInfoArr[MAX_BUS_NUM_CNS];
static GT_U32       numberOfBuses = 0;/* current number of buses */

/* init phase semaphore */
static GT_SEM smemInitPhaseSemaphore;
/* slan down semaphore */
static GT_SEM slanDownSemaphore;

/* NIC Rx callback */
static SKERNEL_NIC_RX_CB_FUN smainNicRxCb;

/* NIC Rx callback */
static SKERNEL_DEVICE_OBJECT * smainNicDeviceObjPtr;

/* enable or disable Tx to Visual. Default enable */
static GT_U32 smainVisualDisabled;

static void smainNicRxHandler
(
    IN GT_U8_PTR   segmentPtr,
    IN GT_U32      segmentLen
);

static GT_STATUS deviceTypeInfoGet
(
    INOUT  SKERNEL_DEVICE_OBJECT * devObjPtr
);

static GT_STATUS smainDeviceRevisionGet
(
    IN SKERNEL_DEVICE_OBJECT    * deviceObjPtr,
    IN GT_U32                   devId
);
/* string name of the device families */
static char* familyNamesArr[] =
{
    "SKERNEL_NOT_INITIALIZED_FAMILY",
    "SKERNEL_SALSA_FAMILY",
    "SKERNEL_NIC_FAMILY",
    "SKERNEL_COM_MODULE_FAMILY",
    "SKERNEL_TWIST_D_FAMILY",
    "SKERNEL_TWIST_C_FAMILY",
    "SKERNEL_SAMBA_FAMILY",
    "SKERNEL_SOHO_FAMILY",
    "SKERNEL_CHEETAH_1_FAMILY",
    "SKERNEL_CHEETAH_2_FAMILY",
    "SKERNEL_CHEETAH_3_FAMILY",
    "SKERNEL_LION_PORT_GROUP_SHARED_FAMILY",
    "SKERNEL_LION_PORT_GROUP_FAMILY",
    "SKERNEL_XCAT_FAMILY",
    "SKERNEL_XCAT2_FAMILY",
    "SKERNEL_TIGER_FAMILY",
    "SKERNEL_FA_FOX_FAMILY",
    "SKERNEL_FAP_DUNE_FAMILY",
    "SKERNEL_XBAR_CAPOEIRA_FAMILY",
    "SKERNEL_FE_DUNE_FAMILY",
    "SKERNEL_PUMA_FAMILY",
    "SKERNEL_EMBEDDED_CPU_FAMILY",

    "SKERNEL_PHY_SHELL_FAMILY",
    "SKERNEL_PHY_CORE_FAMILY",

    "SKERNEL_MACSEC_FAMILY",

    "SKERNEL_LION2_PORT_GROUP_SHARED_FAMILY",
    "SKERNEL_LION2_PORT_GROUP_FAMILY",

    "SKERNEL_LION3_PORT_GROUP_SHARED_FAMILY",
    "SKERNEL_LION3_PORT_GROUP_FAMILY",

    "SKERNEL_PUMA3_SHARED_FAMILY",
    "SKERNEL_PUMA3_NETWORK_FABRIC_FAMILY",

    "SKERNEL_BOBCAT2_FAMILY",
    "SKERNEL_BOBK_CAELUM_FAMILY",
    "SKERNEL_BOBK_CETUS_FAMILY",

    "SKERNEL_BOBCAT3_FAMILY",

    "SKERNEL_XCAT3_FAMILY",

    "SKERNEL_EMPTY_FAMILY",

    " *** "
};
/*
debug tool to be able to emulte remote TM for basic testing of
simulation that works with remote TM (udp sockets and asynchronous
messages)*/
static GT_U32   emulateRemoteTm = 1;

static SKERNEL_DEVICE_OBJECT *   skernelDeviceSoftReset
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);
static SKERNEL_DEVICE_OBJECT *   skernelDeviceSoftResetPart2
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
);


static GT_U32 skernelIsLionShellOnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);
static GT_U32 skernelIsPumaShellOnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr);
/*******************************************************************************
* calcSlanBufSize
*
* DESCRIPTION:
*       Calculate length of buffer needed for given frame
*
* INPUTS:
*       origLen - original length of arrived frame
*       slanInfoPtr - slan info entry pointer
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_U32 - length of buffer
*
* COMMENTS:
*       Main purpose of this function is to calculate length of buffer for
*       undersized [ < 60+4CRC bytes] frames - it can be frame sent as
*       undersized and it could be frame which CRC was cut off by NIC
*       Actually this function should be engaged every time when frame comes
*       from outside before call to sbufDataSet
*******************************************************************************/
static GT_U32 calcSlanBufSize
(
    IN GT_U32 origLen,
    IN SMAIN_PORT_SLAN_INFO *slanInfoPtr
)
{
    GT_U32 actualLenNeeded;/* actual length needed for the frame that got from
                            * slan with the length of origLen
                            */

    /* check if need to prepend bytes for 64 bytes
     * (because egress will not send less then 64)
     * so we need to allocate the space for it
     */
    actualLenNeeded = (origLen < SGT_MIN_FRAME_LEN) ? SGT_MIN_FRAME_LEN : origLen;

    actualLenNeeded += slanInfoPtr->deviceObj->prependNumBytes;

    /* add 4 FCS (frame check sum or CRC) bytes to frame from SLAN */
    if (slanInfoPtr->deviceObj->crcPortsBytesAdd == 1)
    {
        actualLenNeeded += 4;
    }

    return actualLenNeeded;
}

/************** private functions ********************************************/
static void smainInterruptsMaskChanged(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32  causeRegAddr,
    IN GT_U32  maskRegAddr,
    IN GT_U32  intRegBit,
    IN GT_U32  currentCauseRegVal,
    IN GT_U32  lastMaskRegVal,
    IN GT_U32  newMaskRegVal
);

extern void snetCheetahInterruptsMaskChanged(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32  causeRegAddr,
    IN GT_U32  maskRegAddr,
    IN GT_U32  intRegBit,
    IN GT_U32  currentCauseRegVal,
    IN GT_U32  lastMaskRegVal,
    IN GT_U32  newMaskRegVal
);

extern void snetTigerInterruptsMaskChanged(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32  causeRegAddr,
    IN GT_U32  maskRegAddr,
    IN GT_U32  intRegBit,
    IN GT_U32  currentCauseRegVal,
    IN GT_U32  lastMaskRegVal,
    IN GT_U32  newMaskRegVal
);

#ifdef _WIN32
extern void SHOSTG_psos_reg_asic_task(void);
#endif /*_WIN32*/

#ifdef _VISUALC
/*
MSVC++ 10.0  _MSC_VER = 1600
MSVC++ 9.0   _MSC_VER = 1500
MSVC++ 8.0   _MSC_VER = 1400
MSVC++ 7.1   _MSC_VER = 1310
MSVC++ 7.0   _MSC_VER = 1300
MSVC++ 6.0   _MSC_VER = 1200
MSVC++ 5.0   _MSC_VER = 1100
*/

#if _MSC_VER >= 1600 /* from VC 10 */
    #define  strlwr _strlwr
#endif

#endif /*_VISUALC*/

extern char * strlwr(char*);

static void smainReceivedPacketDoneFromTm
(
    IN GT_U32   simDeviceId,
    IN GT_VOID* cookiePtr,
    IN GT_U32   tmFinalPort
);

/*******************************************************************************
*   skernelNumOfPacketsInTheSystemSet
*
* DESCRIPTION:
*       increment/decrement the number of packet in the system
*
* INPUTS:
*       increment - GT_TRUE  - increment the number of packet in the system
*                   GT_FALSE - decrement the number of packet in the system
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID skernelNumOfPacketsInTheSystemSet(
    IN GT_BOOL     increment)
{
    SCIB_SEM_TAKE;

    if(increment == GT_TRUE)
    {
        skernelNumOfPacketsInTheSystem++;
    }
    else
    {
        if(skernelNumOfPacketsInTheSystem == 0)
        {
            skernelFatalError("skernelNumOfPacketsInTheSystemSet: can't decrement from 0 \n");
        }
        else
        {
            skernelNumOfPacketsInTheSystem--;
        }
    }

    /* save to the LOG the number of packets in the system */
    __LOG_PARAM_NO_LOCATION_META_DATA((skernelNumOfPacketsInTheSystem));

    SCIB_SEM_SIGNAL;
}

/*******************************************************************************
*   skernelNumOfPacketsInTheSystemGet
*
* DESCRIPTION:
*       Get the number of packet in the system
*
* INPUTS:
*       None.
* OUTPUTS:
*       None.
*
* RETURNS:
*       The number of packet in the system
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelNumOfPacketsInTheSystemGet(GT_VOID)
{
    return skernelNumOfPacketsInTheSystem;
}


/*******************************************************************************
*   smainGmRegSet
*
* DESCRIPTION:
*       GM 'wrapper' to write register with 'Address Completion'.
*       assuming that the device currently work in mode of 'old address completion'
*       (with register 0)
*
* INPUTS:
*       deviceObjPtr   - pointer to device object.
*       address     - address of register.
*       data        - new register's value.
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
static void smainGmRegSet
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  address,
    IN GT_U32                  data
)
{
    GT_U32  value;
    GT_U32  windowIndex = 3;
    GT_U32  addressCompletionReg = 0;

    /* set the unit into the proper window */
    value = ((address >> 24) & 0xFF) << (8*windowIndex);
    /* write to address completion : set the unit of the register */
    scibWriteMemory(deviceObjPtr->deviceId, addressCompletionReg, 1, &value );

    /* offset in the unit + the window index value */
    value = (windowIndex << 24) | (address & 0xFFFFFF);
    /* write the data to the offset of the address in the unit */
    scibWriteMemory(deviceObjPtr->deviceId, value, 1, &data );

    value = 0;
    /* reset the address completion */
    scibWriteMemory(deviceObjPtr->deviceId, addressCompletionReg, 1, &value );

}
/*******************************************************************************
*   smainMemDefaultsLoad
*
* DESCRIPTION:
*       Load default values for memory from file
*
* INPUTS:
*       deviceObjPtr - pointer to the device object.
*       fileNamePtr - default values file name.
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
void smainMemDefaultsLoad
(
    IN SKERNEL_DEVICE_OBJECT    * deviceObjPtr,
    IN GT_CHAR                  * fileNamePtr

)
{
    FILE    * regFilePtr;  /* file object */
    static char  buffer[FILE_MAX_LINE_LENGTH_CNS]; /* string buffer */
    GT_U32 unitAddr = 0;   /* unit address for register, 0 means not valid*/
    GT_U32 baseAddr;       /* base address for register */
    GT_U32 value;     /* default value */
    GT_32  number;    /* number of registers */
    GT_U32 addrDelta; /* delta for several registers */
    GT_U32 scanRes;   /* number scanned elements */
    GT_32    i;         /* iterator */
    GT_U32 address;   /* address of particular register */
    char *  remarkStr;
    GT_CHAR pciAddr[SMAIN_PCI_FLAG_BUFF_SIZE];
    GT_U32  currentLineIndex=0;/*current line number in the 'registers file'*/

    if(SMEM_SKIP_LOCAL_READ_WRITE)
    {
        /* don't write to memory , memory is not valid for it */
        return;
    }

    if (!fileNamePtr)
       return;

    /* open file */
    regFilePtr = fopen(fileNamePtr,"rt");
    if (regFilePtr == NULL)
    {
        skernelFatalError("smainMemDefaultsLoad: registers file not found %s\n", fileNamePtr);
    }

    while (fgets(buffer, FILE_MAX_LINE_LENGTH_CNS, regFilePtr))
    {
        currentLineIndex++;/* 1 based number */
        /* convert to lower case */
        strlwr(buffer);

        remarkStr = strstr(buffer,";");
        if (remarkStr == buffer)
        {
            continue;
        }
        /* UNIT_BASE_ADDR string detection */
        if(!sscanf(buffer, "unit_base_addr %x\n", &unitAddr))
        {
            if(strstr(buffer, "unit_base_addr not_valid"))
            {
                unitAddr = 0;
                continue;
            }
        }
        else
        {
            continue;
        }

        /* record found, parse it */
        baseAddr = 0;
        value = 0;
        number = 0;
        addrDelta = 0; /* if scanRes < 4 => addrDelta is undefined */
        memset(pciAddr, 0, SMAIN_PCI_FLAG_BUFF_SIZE);
        scanRes = sscanf(buffer, "%x %x %d %x %10s", &baseAddr,
                        &value, &number, &addrDelta, pciAddr);

        if (baseAddr == SMAIN_FILE_RECORDS_EOF)
        {
            break;
        }
        if (((baseAddr & 0xFF000000) != 0) && (unitAddr != 0))
        {
            skernelFatalError("When UNIT_BASE_ADDR is valid then the address of register must be 'zero based' %s (%d)\n", fileNamePtr, currentLineIndex);
         }
        baseAddr += unitAddr;
        if ((scanRes < 2) || (scanRes == 3) ||(scanRes > 5))
        {
            /* check end of file */
            if (scanRes == 1)
                break;

            simWarningPrintf("smainMemDefaultsLoad: registers file's bad format in line [%d]\n",currentLineIndex);
            break;
        }

        if(scanRes == 2)
        {
            number = 1;
        }
        /* set registers */
        for(i = 0; i < number; i++)
        {
            address = i * addrDelta + baseAddr;
            if ((strcmp(pciAddr,"dfx") == 0) || (strcmp(pciAddr,"DFX") == 0))
            {
                /* NOTE: the next function not triggering the 'active memory'
                         mechanism , because the purpose is to start with those
                         registers values , and not to activate actions...
                */
                smemDfxRegSet(deviceObjPtr,address,value);
            }
            else
            if ((strcmp(pciAddr,"pci") == 0) || (strcmp(pciAddr,"PCI") == 0))
            {
                /* NOTE: the next function not triggering the 'active memory'
                         mechanism , because the purpose is to start with those
                         registers values , and not to activate actions...
                */
                smemPciRegSet(deviceObjPtr,address,value);
            }
            else
            {
                /* NOTE: the next function not triggering the 'active memory'
                         mechanism , because the purpose is to start with those
                         registers values , and not to activate actions...
                */

                if((i == 0) &&
                    GT_FALSE == smemIsDeviceMemoryOwner(deviceObjPtr,address))
                {
                    /* this chunk not relevant to this device */
                    /* optimize INIT time of 'shared memory' */
                    break;
                }

                if(deviceObjPtr->gmDeviceType == GOLDEN_MODEL)
                {
                    /* called via 'wrapper' to break the write to support 'Address Completion'
                       to properly redirect the call to GM device */
                    smainGmRegSet(deviceObjPtr, address, value);
                }
                else
                {
                    smemRegSet(deviceObjPtr,address,value);
                    /* call need to be done inside the skernel without the SCIB */
                    /*scibWriteMemory(deviceObjPtr->deviceId, address, 1, &value);*/
                }
            }
        }
    }

    fclose(regFilePtr);
}


/*******************************************************************************
*   smainMemConfigLoad
*
* DESCRIPTION:
*       Load memory configuration from file
*
* INPUTS:
*       deviceObjPtr - pointer to the device object.
*       fileNamePtr - default values file name.
*       isSmemUsed  - whether to use smem func instead of scib
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
void smainMemConfigLoad
(
    IN SKERNEL_DEVICE_OBJECT    *deviceObjPtr,
    IN GT_CHAR                  *fileNamePtr,
    IN GT_BOOL                   isSmemUsed
)
{
    FILE    * regFilePtr;  /* file object */
    static char  buffer[FILE_MAX_LINE_LENGTH_CNS]; /* string buffer */
    GT_U32 baseAddr;       /* base address for register */
    GT_U32 value;     /* default value */
    GT_32  number;    /* number of registers */
    GT_U32 addrDelta; /* delta for several registers */
    GT_U32 scanRes;   /* number scanned elements */
    GT_32    i;         /* iterator */
    GT_U32 address;   /* address of particular register */
    char *  remarkStr;
    GT_CHAR pciAddr[SMAIN_PCI_FLAG_BUFF_SIZE];
    GT_U32  currentLineIndex=0;/*current line number in the 'registers file'*/

    if(SMEM_SKIP_LOCAL_READ_WRITE)
    {
        /* don't write to memory , memory is not valid for it */
        return;
    }

    if (!fileNamePtr)
       return;

    /* open file */
    regFilePtr = fopen(fileNamePtr,"rt");
    if (regFilePtr == NULL)
    {
        skernelFatalError("smainMemConfigLoad: registers file not found %s\n", fileNamePtr);
    }

    while (fgets(buffer, FILE_MAX_LINE_LENGTH_CNS, regFilePtr))
    {
        currentLineIndex++;/* 1 based number */
        /* convert to lower case */
        strlwr(buffer);

        remarkStr = strstr(buffer,";");
        if (remarkStr == buffer)
        {
            continue;
        }

        /* record found, parse it */
        baseAddr = 0;
        value = 0;
        number = 0;
        addrDelta = 0; /* if scanRes < 4 => addrDelta is undefined */
        memset(pciAddr, 0, SMAIN_PCI_FLAG_BUFF_SIZE);
        scanRes = sscanf(buffer, "%x %x %d %x %10s", &baseAddr,
                        &value, &number, &addrDelta, pciAddr);

        if (baseAddr == SMAIN_FILE_RECORDS_EOF)
        {
            break;
        }

        if ((scanRes < 2) || (scanRes == 3) ||(scanRes > 5))
        {
            /* check end of file */
            if (scanRes == 1)
                break;

            simWarningPrintf("smainMemConfigLoad: registers file's bad format in line [%d]\n",currentLineIndex);
            break;
        }

        if(scanRes == 2)
        {
            number = 1;
        }
        /* set registers */
        for(i = 0; i < number; i++)
        {
            address = i * addrDelta + baseAddr;
            if(GT_TRUE == isSmemUsed)
            {
                smemMemSet(deviceObjPtr, address, &value, 1);
            }
            else
            {
                if ((strcmp(pciAddr,"dfx") == 0) || (strcmp(pciAddr,"DFX") == 0))
                {
                    /* NOTE: the next function is triggering the 'active memory'
                             mechanism , because the purpose is to configurate memory
                             exactly in accordance with config file content.
                    */
                    scibMemoryClientRegWrite(deviceObjPtr->deviceId,
                        SCIB_MEM_ACCESS_DFX_E,address,1,&value);
                }
                else
                if ((strcmp(pciAddr,"pci") == 0) || (strcmp(pciAddr,"PCI") == 0))
                {
                    /* NOTE: the next function is triggering the 'active memory'
                             mechanism , because the purpose is to configurate memory
                             exactly in accordance with config file content.
                    */
                    scibPciRegWrite(deviceObjPtr->deviceId,address,1,&value);
                }
                else
                {
                    /* NOTE: the next function is triggering the 'active memory'
                             mechanism , because the purpose is to configurate memory
                             exactly in accordance with config file content.
                    */
                    scibWriteMemory(deviceObjPtr->deviceId, address, 1,&value);
                }
            }
        }
    }
    fclose(regFilePtr);
}

/*******************************************************************************
*   smainDevInfoGet
*
* DESCRIPTION:
*       Get device info from string
*
* INPUTS:
*       devTypeName - name of device type.
*
* OUTPUTS:
*       portsNumPtr - number of ports.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smainDevInfoGet
(
    IN    char           * devTypeName,
    INOUT   SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    GT_U32 ii,jj;
    GT_STATUS   rc;

    /* go through the database */
    for (ii=0; ii < SMAIN_DEV_NAME_DB_SIZE; ii++)
    {
        if (strcmp(smainDevNameDb[ii].devName, devTypeName) == 0)
        {
            deviceObjPtr->portsNumber = smainDevNameDb[ii].portNum;
            deviceObjPtr->gmDeviceType = smainDevNameDb[ii].gmDevType ;

            deviceObjPtr->deviceType = smainDevNameDb[ii].devType;
            rc = deviceTypeInfoGet(deviceObjPtr);
            if(rc == GT_NOT_FOUND)
            {
                /* state all ports as exists */
                for(jj =0 ; jj < deviceObjPtr->portsNumber;jj++)
                {
                    deviceObjPtr->portsArr[jj].state = SKERNEL_PORT_STATE_GE_E;
                }

                for(/*continue*/ ; jj < SKERNEL_DEV_MAX_SUPPORTED_PORTS_CNS; jj++)
                {
                    deviceObjPtr->portsArr[jj].state = SKERNEL_PORT_STATE_NOT_EXISTS_E;
                }
            }

            if(smainDevNameDb[ii].cpuPortNum != NO_CPU_GMII_PORT_CNS)
            {
                if(deviceObjPtr->portsNumber < smainDevNameDb[ii].cpuPortNum)
                {
                    /* add another optional Slan for the CPU port */
                    deviceObjPtr->numSlans = smainDevNameDb[ii].cpuPortNum + 1;
                }
                else
                {
                    deviceObjPtr->numSlans = deviceObjPtr->portsNumber;
                }

                /* set CPU port */
                if(!IS_CHT_VALID_PORT(deviceObjPtr,(smainDevNameDb[ii].cpuPortNum)))
                {
                    deviceObjPtr->portsArr[smainDevNameDb[ii].cpuPortNum].state =
                        SKERNEL_PORT_STATE_GE_E;
                }

                deviceObjPtr->portsArr[smainDevNameDb[ii].cpuPortNum].supportMultiState =
                    GT_FALSE;
            }
            else
            {
                /* the device not support the CPU port with MII */
                deviceObjPtr->numSlans = deviceObjPtr->portsNumber;
            }

            return ;
        }
    }

    /* device not found - stop processing */
    skernelFatalError(" smainDevInfoGet: cannot find device %s", devTypeName);

    return ;
}


/*******************************************************************************
* skernelPortLinkStatusChange
*
* DESCRIPTION:
*        Simulates Link Up/Down event.
* INPUTS:
*   deviceNumber - device number
*   portNumber   - port number
*   newStatus    - change link to Up or to Down
*
* RETURNS:
*       GT_OK - success, GT_FAIL otherwise
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS skernelPortLinkStatusChange
(
    IN  GT_U8    deviceNumber,
    IN  GT_U8    portNumber,
    IN  GT_BOOL  newStatus
)
{
    DEVICE_ID_CHECK_MAC(deviceNumber);

    snetLinkStateNotify(smemTestDeviceIdToDevPtrConvert(deviceNumber),
        portNumber, newStatus) ;

    return GT_OK ;
}

/*******************************************************************************
*  smainDeviceBackUpMemory
*
* DESCRIPTION:
*      Definition of backup/restore memory function
*
* INPUTS:
*       deviceNumber    - device ID.
*       readWrite       - backup/restore memory data
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK   - success read/write
*       GT_FAIL - wrong device ID
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS smainDeviceBackUpMemory
(
    IN  GT_U8    deviceNumber,
    IN  GT_BOOL  readWrite
)
{
    SKERNEL_DEVICE_OBJECT * devObjPtr;

    DEVICE_ID_CHECK_MAC(deviceNumber);

    devObjPtr = smemTestDeviceIdToDevPtrConvert(deviceNumber);

    if (devObjPtr->devMemBackUpPtr)
    {
        devObjPtr->devMemBackUpPtr(devObjPtr, readWrite);
    }
    else
    {
        if(devObjPtr->shellDevice == GT_TRUE)
        {
            GT_U32                 dev;           /* core iterator */
            SKERNEL_DEVICE_OBJECT *currDevObjPtr; /* current device object pointer */
            for(dev = 0 ; dev < devObjPtr->numOfCoreDevs ; dev++)
            {
                currDevObjPtr = devObjPtr->coreDevInfoPtr[dev].devObjPtr;
                if(currDevObjPtr != NULL)
                {
                    /* process core */
                    currDevObjPtr->devMemBackUpPtr(currDevObjPtr, readWrite);
                }
            }
        }
    }

    return GT_OK ;
}

/*******************************************************************************
*  smainDeviceBackUpMemoryTest
*
* DESCRIPTION:
*      Test backup memory function
*
* INPUTS:
*       deviceNumber    - device ID.
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK   - success
*       GT_FAIL - wrong device ID
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS smainDeviceBackUpMemoryTest
(
    IN  GT_U8    deviceNumber
)
{
    SKERNEL_DEVICE_OBJECT * devObjPtr;

    DEVICE_ID_CHECK_MAC(deviceNumber);
    devObjPtr = smemTestDeviceIdToDevPtrConvert(deviceNumber);


    /* for multi-core : write different value per core to the same register
       (address = 0x10) , to check such device */
    if(devObjPtr->shellDevice == GT_TRUE)
    {
        GT_U32                 temp  = 0x0AFAFAF0;
        GT_U32                 temp1 = 0;
        GT_U32                 dev;           /* core iterator */
        SKERNEL_DEVICE_OBJECT *currDevObjPtr; /* current device object pointer */
        for(dev = 0 ; dev < devObjPtr->numOfCoreDevs ; dev++)
        {
            currDevObjPtr = devObjPtr->coreDevInfoPtr[dev].devObjPtr;
            if(currDevObjPtr != NULL)
            {
                temp1 = temp + dev;
                smemMemSet(currDevObjPtr, 0x10, &temp1, 1);
            }
        }
    }

    /* do backup for the device */
    smainDeviceBackUpMemory(deviceNumber, 1);

    return GT_OK ;
}

/*******************************************************************************
*  smainDeviceBackUpMemoryTestOneValue
*
* DESCRIPTION:
*      Test backup/restore memory function, one value
*
* INPUTS:
*       deviceNumber    - device ID.
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK   - success
*       GT_FAIL - wrong device ID
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS smainDeviceBackUpMemoryTestOneValue
(
    IN  GT_U8    deviceNumber
)
{
    SKERNEL_DEVICE_OBJECT * devObjPtr;

    GT_U32  var  = 0x00028100;
    GT_U32  *varGetPtr;
    GT_U32  address = 0x10;

    DEVICE_ID_CHECK_MAC(deviceNumber);
    devObjPtr = smemTestDeviceIdToDevPtrConvert(deviceNumber);


    varGetPtr = smemMemGet(devObjPtr, address);
    printf("TEST original value was: 0x%X \n", *varGetPtr);

#if 0
    /* do backup for the device */
    smainDeviceBackUpMemory(deviceNumber, 1);

#endif
    /* do restore for the device */
    smainDeviceBackUpMemory(deviceNumber, 0);

    /*set test var */
    /*smemMemSet(devObjPtr, address, &var, 1);*/

    varGetPtr = smemMemGet(devObjPtr, address);
    printf("TEST got value: 0x%X \n", *varGetPtr);

    /*read test var */
    if(*varGetPtr != var)
    {
        printf("TEST FAILED: got 0x%X instead of 0x%X\n", *varGetPtr, var);
        return GT_FAIL ;
    }

    return GT_OK ;
}


/*******************************************************************************
*   smainDevice2SlanConnect
*
* DESCRIPTION:
*       Bind ports of devices with SLANs
*
* INPUTS:
*       deviceObjPtr - pointer to the device object for task.
*       port_no      - number of port
*       linkState    - 1 for up , 0 for down.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smainDevice2SlanConnect(
        IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
        IN GT_U32   portNum,
        IN GT_U32   linkState
)
{
    SMAIN_PORT_SLAN_INFO *slanInfoPrt;          /* slan port entry pointer */
    char                 updnprefix[10]={0};


    slanInfoPrt=&(deviceObjPtr->portSlanInfo[portNum]);

    if (slanInfoPrt->slanName[0x0] != 0x0)
    {
        memcpy(updnprefix,slanInfoPrt->slanName,2);
        if (memcmp(updnprefix,"ls",2) == 0)
        {
            SIM_OS_MAC(simOsChangeLinkStatus)(slanInfoPrt->slanIdRx , linkState);
        }
        else
        {
            skernelPortLinkStatusChange((GT_U8)slanInfoPrt->deviceObj->deviceId,
                                       (GT_U8)slanInfoPrt->portNumber,
                                       linkState == 1 ? GT_TRUE : GT_FALSE);
        }
    }
    else /* support port with no SLAN */
    {
        skernelPortLinkStatusChange((GT_U8)deviceObjPtr->deviceId,
                                   (GT_U8)portNum,
                                   linkState == 1 ? GT_TRUE : GT_FALSE);
    }
}

/*******************************************************************************
*   resendMessage
*
* DESCRIPTION:
*       re-send message to the Skernel task
*
* INPUTS:
*       deviceObjPtr - pointer to the device
*       bufferId     - the buffer that hold original message.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*    NOTE: the caller must bypass the 'sbufFree' for this buffer on current
*      treatment (not on the 'resend')
*
*******************************************************************************/
static void resendMessage(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN SBUF_BUF_ID             bufferId
)
{
    /* re-send the buffer to the queue */
    squeBufPut(deviceObjPtr->queueId,SIM_CAST_BUFF(bufferId));
}

/*******************************************************************************
*   smainSkernelTask
*
* DESCRIPTION:
*       Skernel task
*
* INPUTS:
*       deviceObjPtr - pointer to the device object for task.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smainSkernelTask(
        IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    SBUF_BUF_ID             bufferId;    /* buffer id */
    SKERNEL_FRAME_DESCR_UNT descritor;   /* frame descriptor */
    SKERNEL_UPLINK_DESC_STC uplinkDesc;  /* uplink descriptor */
    GT_U8                   *dataPtr;    /* pointer to the start of buffer's data */
    GT_U32                  dataSize;    /* size of buffer's data */
    SBUF_BUF_STC            *bufInfo_PTR;
    GT_U32                  portNum;     /*port id for the link change message*/
    GT_U32                  linkPortState;/* link state(1-up,0-down) for the
                                             link change message */
    GENERIC_MSG_FUNC genMsgFunc;/*generic msg function*/
    GT_U32           genMsgTypeSize = sizeof(GENERIC_MSG_FUNC);
    SKERNEL_DEVICE_OBJECT * newDeviceObjPtr;/* new device obj needed for soft reset */
    GT_BIT                  softResetMsg = 0;/*indication of soft reset message*/
    GT_BIT                  allowBuffFree;/* indication that the buffer can be free */

    SIM_OS_TASK_PURPOSE_TYPE_ENT taskType;

    /* notify that task starts to work */
    SIM_OS_MAC(simOsSemSignal)(smemInitPhaseSemaphore);

#ifdef _WIN32
    /* call SHOST to register the application task in the asic task table*/
    SHOSTG_psos_reg_asic_task();
#endif /*_WIN32*/

    /* set task type - only after SHOSTG_psos_reg_asic_task */
    taskType = SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_GENERAL_PURPOSE_E;
    SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)(taskType,
        &deviceObjPtr->task_skernelCookieInfo.generic);

    while(1)
    {
        SET_TASK_TYPE_MAC(SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_GENERAL_PURPOSE_E);

        /* get buffer */
        bufferId = SIM_CAST_BUFF(squeBufGet(deviceObjPtr->queueId));

        allowBuffFree = 1;

        /* we allocated the buffer , and we will free it unless told otherwise */
        bufferId->freeState = SBUF_BUFFER_STATE_ALLOCATOR_CAN_FREE_E;

        if(bufferId->dataType != SMAIN_MSG_TYPE_SOFT_RESET_E)
        {
            if(deviceObjPtr->needToDoSoftReset)
            {
                /* we started 'soft reset' and we need to ignore such messages
                  until device finish re-initialization */

                goto doneWithBuffer_lbl;
            }
        }

        /* process a buffer */
        if (bufferId->srcType == SMAIN_SRC_TYPE_SLAN_E ||
            bufferId->srcType == SMAIN_SRC_TYPE_LOOPBACK_PORT_E ||
            bufferId->srcType == SMAIN_SRC_TYPE_INTERNAL_CONNECTION_E)
        {
            /* process data from SLAN */
            if (bufferId->dataType == SMAIN_MSG_TYPE_FRAME_E)
            {
                SET_TASK_TYPE_MAC(SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_PROCESSING_DAEMON_E);

                /* process frame */
                snetFrameProcess(deviceObjPtr, bufferId, bufferId->srcData);

                skernelNumOfPacketsInTheSystemSet(GT_FALSE);
            }
        }
        else if (bufferId->srcType == SMAIN_SRC_TYPE_CPU_E)
        {
            switch(bufferId->dataType)
            {
                case SMAIN_MSG_TYPE_FDB_UPDATE_E:

                    /* buffer contents FDB update message */
                    sbufDataGet(bufferId,&dataPtr,&dataSize);

                    /* process message */
                    sfdbMsgProcess(deviceObjPtr, dataPtr);

                    break;

                case SMAIN_MSG_TYPE_FRAME_E:

                    /* process frame from SLAN */
                    memset(&descritor, 0, sizeof(descritor));
                    sbufDataGet(bufferId,&dataPtr,&dataSize);
                    descritor.presteraDscr.frameBuf = bufferId;
                    bufInfo_PTR = (SBUF_BUF_STC*)bufferId;

                    /* multicast vidx or unicast treatment */
                    if((descritor.presteraDscr.useVidx = bufInfo_PTR->userInfo.data.sapiTxPacket.useVidx) != 0)
                    {
                        /* Set the vidx field.              */
                        descritor.presteraDscr.bits15_2.useVidx_1.vidx =
                            bufInfo_PTR->userInfo.data.sapiTxPacket.dest.vidx;
                    }
                    else
                    {
                        /* Set the target Dev & port fields */
                        descritor.presteraDscr.bits15_2.useVidx_0.targedDevice =
                            bufInfo_PTR->userInfo.data.sapiTxPacket.dest.devPort.tgtDev;

                        descritor.presteraDscr.bits15_2.useVidx_0.targedPort =
                            bufInfo_PTR->userInfo.data.sapiTxPacket.dest.devPort.tgtPort;
                    }

                    descritor.presteraDscr.srcPort = bufInfo_PTR->srcData;
                    descritor.presteraDscr.byteCount =
                        (GT_U16)bufferId->actualDataSize;
                    descritor.presteraDscr.dropPrecedence =
                        bufInfo_PTR->userInfo.data.sapiTxPacket.dropPrecedence;
                    descritor.presteraDscr.macDaType =
                        bufInfo_PTR->userInfo.data.sapiTxPacket.macDaType;
                    descritor.presteraDscr.userPriorityTag =
                        bufInfo_PTR->userInfo.data.sapiTxPacket.userPrioTag;
                    descritor.presteraDscr.vid =
                        bufInfo_PTR->userInfo.data.sapiTxPacket.vid;

                    /* if buffer supplied by upper layers doesn't contain tag
                     * while in descriptor set that it's tagged frame then no
                     * tag is added by ASIC and frame goes out untagged
                     */
                    if(bufInfo_PTR->userInfo.data.sapiTxPacket.packetTagged)
                        descritor.presteraDscr.vlanCmd = 0; /* don't add tag */
                    else
                        descritor.presteraDscr.vlanCmd = 1; /* add tag if needed */

                    if((descritor.presteraDscr.inputIncapsulation =
                            bufInfo_PTR->userInfo.data.sapiTxPacket.packetEncap)
                        == GT_REGULAR_PCKT)
                    {
                        descritor.presteraDscr.appendCrc =
                            bufInfo_PTR->userInfo.data.sapiTxPacket.recalcCrc;

                    }

                    /* process frame in message */
                    snetCpuTxFrameProcess(deviceObjPtr, &descritor.presteraDscr);

                    break;
                case SMAIN_CPU_FDB_ACT_TRG_E:
                    SET_TASK_TYPE_MAC(SIM_OS_TASK_PURPOSE_TYPE_PP_AGING_DAEMON_E);

                    /* buffer contents FDB update message */
                    sbufDataGet(bufferId,&dataPtr,&dataSize);

                    /* process message */
                    sfdbMacTableTriggerAction(deviceObjPtr, dataPtr);

                    break;
                case SMAIN_CPU_TX_SDMA_QUEUE_E:
                    SET_TASK_TYPE_MAC(SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_PROCESSING_DAEMON_E);

                    /* process SDMA queue message */
                    snetFromCpuDmaProcess(deviceObjPtr, bufferId);

                    break;
                case SMAIN_CPU_FDB_AUTO_AGING_E:
                    SET_TASK_TYPE_MAC(SIM_OS_TASK_PURPOSE_TYPE_PP_AGING_DAEMON_E);
                    /* buffer contents FDB aging message */
                    sbufDataGet(bufferId,&dataPtr,&dataSize);

                    /* process message */
                    sfdbMacTableAutomaticAging(deviceObjPtr, dataPtr);

                    break;
                case SMAIN_LINK_CHG_MSG_E:
                    /* buffer content link changed message */
                    sbufDataGet(bufferId,&dataPtr,&dataSize);
                    portNum = dataPtr[0];
                    linkPortState = dataPtr[1];
                    smainDevice2SlanConnect(deviceObjPtr,
                                            portNum ,
                                            linkPortState);
                    break;
                case SMAIN_INTERRUPTS_MASK_REG_E:
                    /* buffer contents interrupt mask register changed message */
                    sbufDataGet(bufferId,&dataPtr,&dataSize);
                    smainInterruptsMaskChanged(deviceObjPtr,
                       ((GT_U32*)dataPtr)[0],
                       ((GT_U32*)dataPtr)[1],
                       ((GT_U32*)dataPtr)[2],
                       ((GT_U32*)dataPtr)[3],
                       ((GT_U32*)dataPtr)[4],
                       ((GT_U32*)dataPtr)[5]);
                    break;
                case SMAIN_MSG_TYPE_FDB_UPLOAD_E:
                    SET_TASK_TYPE_MAC(SIM_OS_TASK_PURPOSE_TYPE_PP_AGING_DAEMON_E);

                    /* buffer contents FDB upload message */
                    sbufDataGet(bufferId,&dataPtr,&dataSize);

                    /* implementation of the upload action */
                    sfdbMacTableUploadAction(deviceObjPtr,dataPtr);

                    break;
                case SMAIN_MSG_TYPE_CNC_FAST_DUMP_E:
                    SET_TASK_TYPE_MAC(SIM_OS_TASK_PURPOSE_TYPE_PP_AGING_DAEMON_E);

                    /* buffer contents pointer to CNC Fast Dump Trigger Register */
                    sbufDataGet(bufferId,&dataPtr,&dataSize);

                    /* implementation of the upload action */
                    snetCncFastDumpUploadAction(deviceObjPtr, (GT_U32 *)dataPtr);

                    break;

                case SMAIN_MSG_TYPE_SOFT_RESET_E:
                    /* buffer contents pointer to Global Control Register */
                    sbufDataGet(bufferId, &dataPtr, &dataSize);

                    /* allow using __LOG .. to debug the 'soft reset' feature */
                    SET_TASK_TYPE_MAC(SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_SOFT_RESET_E);

                    /* implementation of device soft reset */
                    newDeviceObjPtr = skernelDeviceSoftReset(deviceObjPtr);
                    if(newDeviceObjPtr != NULL)
                    {
                        /* support replacement of the old device with new device */
                        deviceObjPtr = newDeviceObjPtr;
                    }

                    softResetMsg = 1;

                    break;
                case SMAIN_MSG_TYPE_GENERIC_FUNCTION_E:
                    /* buffer contents pointer to generic function */
                    sbufDataGet(bufferId,&dataPtr,&dataSize);
                    memcpy(&genMsgFunc,dataPtr,genMsgTypeSize);
                    dataPtr  += genMsgTypeSize;/* skip the function name */
                    dataSize -= genMsgTypeSize;
                    /* call the generic message callback function */
                    (*genMsgFunc)(deviceObjPtr,dataPtr,dataSize);
                    break;

                default:
                    break;
            }
        }
        else if (bufferId->srcType == SMAIN_SRC_TYPE_UPLINK_E) /*pp->pp , fa->pp*/
        {
           /* buffer contains frame from uplink*/
           if (bufferId->dataType == SMAIN_MSG_TYPE_FRAME_E)
           {
               memset(&uplinkDesc, 0, sizeof(SKERNEL_UPLINK_DESC_STC));
               sbufDataGet(bufferId,&dataPtr,&dataSize);
               memcpy(&uplinkDesc ,dataPtr,sizeof(SKERNEL_UPLINK_DESC_STC));
               uplinkDesc.data.PpPacket.source.frameBuf = bufferId ;
               snetProcessFrameFromUpLink(deviceObjPtr,&uplinkDesc);
           }
        }

        if(deviceObjPtr->needResendMessage)
        {
            GT_U32  numBuffs;
            deviceObjPtr->needResendMessage = 0;
            resendMessage(deviceObjPtr,bufferId);
            numBuffs = sbufAllocatedBuffersNumGet(deviceObjPtr->bufPool);
            if(1 >= numBuffs)
            {
                /* let the skernel time without storming it self */
                deviceObjPtr =
                    skernelSleep(deviceObjPtr,100);
            }

            /* do not allow to free the buffer */
            allowBuffFree = 0;
        }

        doneWithBuffer_lbl:

        if(allowBuffFree &&
           bufferId->freeState == SBUF_BUFFER_STATE_ALLOCATOR_CAN_FREE_E)
        {
            /* free buffer */
            sbufFree(deviceObjPtr->bufPool, bufferId);
        }

        if(softResetMsg)
        {
            /* done only after the buffer that was used to hold the first message was released */
            skernelDeviceSoftResetPart2(deviceObjPtr);
            softResetMsg = 0;
        }

    }
}

/*******************************************************************************
*   smainSagingTask
*
* DESCRIPTION:
*       SAging task
*
* INPUTS:
*       deviceObjPtr - pointer to the device object for task.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smainSagingTask(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    SIM_OS_TASK_PURPOSE_TYPE_ENT taskType;

    /* notify that task starts to work */
    SIM_OS_MAC(simOsSemSignal)(smemInitPhaseSemaphore);

#ifdef _WIN32
    /* call SHOST to register the application task in the asic task table*/
    SHOSTG_psos_reg_asic_task();
#endif /*_WIN32*/

    /* set task type - only after SHOSTG_psos_reg_asic_task */
    taskType = SIM_OS_TASK_PURPOSE_TYPE_PP_AGING_DAEMON_E;
    SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)(taskType,
        &deviceObjPtr->task_sagingCookieInfo.generic);

    while(1)
    {
        sfdbMacTableAging(deviceObjPtr);
        /*Sleep(SMAIN_AGING_POLLING_TIME_CNS);*/

        deviceObjPtr = skernelSleep(deviceObjPtr,SMAIN_AGING_POLLING_TIME_CNS);

        /* set task type - only after SHOSTG_psos_reg_asic_task */
        SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)(
            SIM_OS_TASK_PURPOSE_TYPE_PP_AGING_DAEMON_E,
            &deviceObjPtr->task_sagingCookieInfo.generic);
    }
}

/*******************************************************************************
*   smainOamKeepAliveTask
*
* DESCRIPTION:
*       OAM keepalive task
*
* INPUTS:
*       deviceObjPtr - pointer to the device object for task.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smainOamKeepAliveTask(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    SIM_OS_TASK_PURPOSE_TYPE_ENT taskType;

    /* notify that task starts to work */
    SIM_OS_MAC(simOsSemSignal)(smemInitPhaseSemaphore);

#ifdef _WIN32
    /* call SHOST to register the application task in the asic task table*/
    SHOSTG_psos_reg_asic_task();
#endif /*_WIN32*/

    /* set task type - only after SHOSTG_psos_reg_asic_task */
    taskType = SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_OAM_KEEP_ALIVE_DAEMON_E;
    SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)(taskType,
        &deviceObjPtr->task_oamKeepAliveAgingCookieInfo.generic);

    snetLion2OamKeepAliveAging(deviceObjPtr);
}

/*******************************************************************************
*   smainPacketGeneratorTask
*
* DESCRIPTION:
*       Packet generator main task
*
* INPUTS:
*       tgDataPtr - pointer to the traffic generator data.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID smainPacketGeneratorTask
(
    IN SKERNEL_TRAFFIC_GEN_STC * tgDataPtr
)
{
    /* notify that task starts to work */
    SIM_OS_MAC(simOsSemSignal)(smemInitPhaseSemaphore);

#ifdef _WIN32
    /* call SHOST to register the application task in the asic task table*/
    SHOSTG_psos_reg_asic_task();
#endif /*_WIN32*/

    snetLion2TgPacketGenerator(tgDataPtr);
}

/*******************************************************************************
*   smainSlanPacketRcv
*
* DESCRIPTION:
*       receive packet  (called from slan scope)
*
* INPUTS:
*       msg_code   - message code from SLAN.
*       res        - reason for call.
*       sender_tid - task id of sender.
*       len        - message length.
*
* OUTPUTS:
*       usr_info_PTR - SLAN user info ptr.
*       buff_PTR - message.
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static char *smainSlanPacketRcv(
    IN  GT_U32       msg_code,
    IN  GT_U32       res,
    IN  GT_U32       sender_tid,
    IN  GT_U32       len,
    INOUT  void   *        usr_info_PTR,
    INOUT  char   *        buff_PTR
)
{
    SMAIN_PORT_SLAN_INFO *  slanInfoPrt; /* slan info entry pointer */
    SBUF_BUF_ID             bufferId;    /* buffer */
    GT_U8                *  data_PTR;  /* pointer to the data in the buffer */
    GT_U32                  data_size;  /* data size */
    GT_U32                  buffLen;
    char *                  returnPtr = NULL;

    /* get slan info from user info */
    slanInfoPrt = (SMAIN_PORT_SLAN_INFO *)usr_info_PTR;

    /* wait for semaphore of 'delete slan' */
    /* since the SLAN context must not do 'SCIB lock' when slanInfoPrt->slanName[0] == 0
       because other thread is taking the SCIB and wait for this thread to end !!! */
    SIM_OS_MAC(simOsSemWait)(slanDownSemaphore,SIM_OS_WAIT_FOREVER);

    if(slanInfoPrt->slanName[0] == 0)
    {
#if 0        /* calling SCIB lock not allowed in this case ... so we need to avoid from
           calling OS functions like printf that can cause WIN32 to deadlock on
           SCIB taken by other thread */
        static GT_U32   dropsNum = 0;

        dropsNum++;
        /* the port was unbind from the SLAN */
        printf(" smainSlanPacketRcv:  port was unbound from the SLAN , do not treat this packet -- drop#%d \n",dropsNum);
#endif/*0*/

        returnPtr = NULL;
        goto exitCleanly_lbl;
    }

    if ( msg_code == SAGNTP_SLANUP_CNS )
    { /*LinkUp ....*/
        skernelPortLinkStatusChange((GT_U8)slanInfoPrt->deviceObj->deviceId,
                                    (GT_U8)slanInfoPrt->portNumber,
                                    GT_TRUE);
        returnPtr = NULL;

        goto exitCleanly_lbl;
    }
    else if ( msg_code == SAGNTP_SLANDN_CNS )
    { /*LinkDown ....*/
        skernelPortLinkStatusChange((GT_U8)slanInfoPrt->deviceObj->deviceId,
                                    (GT_U8)slanInfoPrt->portNumber,
                                    GT_FALSE);
        returnPtr = NULL;
        goto exitCleanly_lbl;
    }

    switch ( res )
    {
        case SIM_OS_SLAN_NORMAL_MSG_RSN_CNS :
            returnPtr = NULL;
            break;

        /*! Sagent first call - get buffer and user info */
        case SIM_OS_SLAN_GET_BUFF_RSN_CNS :

            /* calc buff size needed for given frame length */
            buffLen = calcSlanBufSize(len, slanInfoPrt);

            /* get buffer */
            bufferId = sbufAlloc(slanInfoPrt->deviceObj->bufPool, buffLen);

            if ( bufferId == NULL )
            {
                simWarningPrintf(" smainSlanPacketRcv: no buffers for receive\n");
                returnPtr = NULL;

                goto exitCleanly_lbl;
            }

            /* get actual data pointer */
            sbufDataGet(bufferId, &data_PTR, &data_size);

            if(slanInfoPrt->deviceObj->prependNumBytes)
            {
                /* the buffer already has two additional bytes for future
                   prepend two bytes while send to CPU. The calcSlanBufSize
                   take these two bytes into account.
                   But these two bytes must not be used now. The data pointer
                   and data size need to be corrected in order to SLAN not use
                   them. */
                data_PTR += slanInfoPrt->deviceObj->prependNumBytes;
                data_size -= slanInfoPrt->deviceObj->prependNumBytes;

                /* store new data pointer and data size  */
                sbufDataSet(bufferId, data_PTR, data_size);
            }

            /* store buffer id in the slan info */
            slanInfoPrt->buffId = bufferId;

            /* notify the SLAN to start copy the frame after the prepend bytes */
            returnPtr = (char *)data_PTR;

            break;

                        /* Sagent second call -
                set buffer data params and put in queue for process*/
        case SIM_OS_SLAN_GIVE_BUFF_SUCCS_RSN_CNS :

            /* calc buff size needed for given frame length */
            buffLen = calcSlanBufSize(len, slanInfoPrt);

            /* buffer length has two bytes for prepend feature.
               actual data length should be reduced to compensate
               these two bytes */
            data_size = buffLen - slanInfoPrt->deviceObj->prependNumBytes;

            /* get the buffer and put it in the queue */
            sbufDataSet(slanInfoPrt->buffId, (GT_U8*)buff_PTR, data_size);

            /* set user info of buffer */
            memset(&(slanInfoPrt->buffId->userInfo), 0,
                            sizeof(slanInfoPrt->buffId->userInfo));

            skernelNumOfPacketsInTheSystemSet(GT_TRUE);

            /* set source type of buffer */
            slanInfoPrt->buffId->srcType = SMAIN_SRC_TYPE_SLAN_E;

            /* set source port of buffer */
            slanInfoPrt->buffId->srcData = slanInfoPrt->portNumber;

            /* set message type of buffer */
            slanInfoPrt->buffId->dataType = SMAIN_MSG_TYPE_FRAME_E;

            /* put buffer to queue */
            squeBufPut(slanInfoPrt->deviceObj->queueId,SIM_CAST_BUFF(slanInfoPrt->buffId));

            returnPtr = NULL;

            break;

        case SIM_OS_SLAN_GIVE_BUFF_ERR_RSN_CNS:

            /* error reason..  */
            simWarningPrintf(" smainSlanPacketRcv: Frame Discarded "\
                   " in device %d port %d , for slan =[%s] \n",
                    slanInfoPrt->deviceObj->deviceId,
                    slanInfoPrt->portNumber,
                    slanInfoPrt->slanName);

            /* free buffer */
            sbufFree(slanInfoPrt->deviceObj->bufPool, slanInfoPrt->buffId);

            returnPtr = NULL;

            break;
    }

exitCleanly_lbl:

    SIM_OS_MAC(simOsSemSignal)(slanDownSemaphore);

    return returnPtr;
}

/*******************************************************************************
*   smainSlanGMPacketRcv
*
* DESCRIPTION:
*       receive packet  (called from slan task scope)
*
* INPUTS:
*       msg_code   - message code from SLAN.
*       res        - reason for call.
*       sender_tid - task id of sender.
*       len        - message length.
*
* OUTPUTS:
*       usr_info_PTR - SLAN user info ptr.
*       buff_PTR     - message.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static char *smainSlanGMPacketRcv(
    IN  GT_U32       msg_code,
    IN  GT_U32       res,
    IN  GT_U32       sender_tid,
    IN  GT_U32       len,
    INOUT  void   *        usr_info_PTR,
    INOUT  char   *        buff_PTR
)
{
    SMAIN_PORT_SLAN_INFO *  slanInfoPrt; /* slan info entry pointer */
    SBUF_BUF_ID             bufferId;    /* buffer */
    GT_U8                *  data_PTR;  /* pointer to the data in the buffer */
    GT_U32                  data_size;  /* data size */
    GT_U32                  buffLen;

    /* get slan info from user info */
    slanInfoPrt = (SMAIN_PORT_SLAN_INFO *)usr_info_PTR;

    switch ( res )
    {
        case SIM_OS_SLAN_NORMAL_MSG_RSN_CNS :
            return NULL;
        /*! Sagent first call - get buffer and user info */
        case SIM_OS_SLAN_GET_BUFF_RSN_CNS :

            /* calc buff size needed for given frame length */
            buffLen = calcSlanBufSize(len, slanInfoPrt);

            /* get buffer */
            bufferId = sbufAlloc(slanInfoPrt->deviceObj->bufPool, buffLen);

            if ( bufferId == NULL )
            {
                simWarningPrintf(" smainSlanGMPacketRcv: no buffers for receive\n");
                return NULL;
            }

            /* get actual data pointer */
            sbufDataGet(bufferId, &data_PTR, &data_size);

            if(slanInfoPrt->deviceObj->prependNumBytes)
            {
                /* the buffer already has two additional bytes for future
                   prepend two bytes while send to CPU. The calcSlanBufSize
                   take these two bytes into account.
                   But these two bytes must not be used now. The data pointer
                   and data size need to be corrected in order to SLAN not use
                   them. */
                data_PTR += slanInfoPrt->deviceObj->prependNumBytes;
                data_size -= slanInfoPrt->deviceObj->prependNumBytes;

                /* store new data pointer and data size  */
                sbufDataSet(bufferId, data_PTR, data_size);
            }

            /* store buffer id in the slan info */
            slanInfoPrt->buffId = bufferId;

            /* notify the SLAN to start copy the frame after the prepend bytes */
            return (char *)data_PTR;

        /* Sagent second call - set buffer data params and put in queue for process*/
        case SIM_OS_SLAN_GIVE_BUFF_SUCCS_RSN_CNS :

            /* calc buff size needed for given frame length */
            buffLen = calcSlanBufSize(len, slanInfoPrt);

            /* buffer length has two bytes for prepend feature.
               actual data length should be reduced to compensate
               these two bytes */
            data_size = buffLen - slanInfoPrt->deviceObj->prependNumBytes;

            /* get the buffer and put it in the queue */
            sbufDataSet(slanInfoPrt->buffId, (GT_U8*)buff_PTR, data_size);

            if(smainGmTrafficInitDone == 1)
            {
                if (slanInfoPrt->deviceObj->gmDeviceType != GOLDEN_MODEL)
                {
                    ppSendPacket(SMEM_GM_GET_GM_DEVICE_ID(slanInfoPrt->deviceObj),
                                 slanInfoPrt->deviceObj->portGroupId,
                            slanInfoPrt->portNumber,
                            buff_PTR, len);
                }
            }
            sbufFree(slanInfoPrt->deviceObj->bufPool, slanInfoPrt->buffId);
            return NULL;

        case SIM_OS_SLAN_GIVE_BUFF_ERR_RSN_CNS:

            /* error reason..  */
            simWarningPrintf(" smainSlanGMPacketRcv: Frame Discarded "\
                   " in device %d port %d , for slan =[%s] \n",
                    slanInfoPrt->deviceObj->deviceId,
                    slanInfoPrt->portNumber,
                    slanInfoPrt->slanName);

            /* free buffer */
            sbufFree(slanInfoPrt->deviceObj->bufPool, slanInfoPrt->buffId);
            return NULL;

    }

    return NULL;
}



/*******************************************************************************
*   internalSlanBind
*
* DESCRIPTION:
*       do Bind to slan.
*
* INPUTS:
*       slanNamePtr - (pointer to) slan name
*       deviceObjPtr- (pointer to) the device object
*       portNumber  - port number
*       bindRx      - bind to Rx direction ? GT_TRUE - yes , GT_FALSE - no
*       bindTx      - bind to Tx direction ? GT_TRUE - yes , GT_FALSE - no
*       explicitFuncPtr - explicit SLAN receive function , in NULL ignored
* OUTPUTS:
*       None.
*
* RETURNS:
*       None
*
* COMMENTS:
*
*******************************************************************************/
void internalSlanBind (
    IN char                         *slanNamePtr,
    IN SKERNEL_DEVICE_OBJECT        *deviceObjPtr,
    IN GT_U32                       portNumber,
    IN GT_BOOL                      bindRx,
    IN GT_BOOL                      bindTx,
    IN SMAIN_PORT_SLAN_INFO         *slanInfoPrt,
    IN SIM_OS_SLAN_RCV_FUN          explicitFuncPtr
)
{
    SIM_OS_SLAN_RCV_FUN     funcPtr;
    SIM_OS_SLAN_ID          newSlanId;

    slanInfoPrt->portNumber = portNumber;
    strncpy(slanInfoPrt->slanName,slanNamePtr,SMAIN_SLAN_NAME_SIZE_CNS);

    if(explicitFuncPtr)
    {
        funcPtr = explicitFuncPtr;
    }
    else
    if (deviceObjPtr->gmDeviceType != SMAIN_NO_GM_DEVICE &&
       (deviceObjPtr->gmDeviceType != GOLDEN_MODEL))
    {
        funcPtr = smainSlanGMPacketRcv;
    }
    else
    {
        funcPtr = smainSlanPacketRcv;
    }

    newSlanId = SIM_OS_MAC(simOsSlanBind)( slanInfoPrt->slanName, deviceObjPtr->deviceName,
                            slanInfoPrt,funcPtr);

    if(bindRx == GT_TRUE)
    {
        slanInfoPrt->slanIdRx = newSlanId;
    }

    if(bindTx == GT_TRUE)
    {
        slanInfoPrt->slanIdTx = newSlanId;
    }

    if (newSlanId == NULL)
    {
        skernelFatalError("internalSlanBind: SLAN %s bind failed \n",slanInfoPrt->slanName );
    }

    if(bindRx == GT_TRUE)
    {
        /* rx is link indication */
        deviceObjPtr->portsArr[portNumber].linkStateWhenNoForce = SKERNEL_PORT_NATIVE_LINK_UP_E;
    }

    /* send LOG indications */
    simLogSlanBind(slanInfoPrt->slanName,deviceObjPtr,portNumber,bindRx,bindTx);
}

/*******************************************************************************
*   internalSlanUnbind
*
* DESCRIPTION:
*       do Unbind from slan : RX,TX.
*
* INPUTS:
*       slanInfoPrt   - (pointer) slan info
* OUTPUTS:
*       None.
*
* RETURNS:
*       indication that unbind done
*
* COMMENTS:
*
*******************************************************************************/
static GT_BOOL internalSlanUnbind (
    IN SMAIN_PORT_SLAN_INFO         *slanInfoPrt
)
{
    GT_BOOL unbindDone , unbindRxDone = GT_FALSE , unbindTxDone = GT_FALSE;
    /* unbind */
    if (slanInfoPrt->slanIdRx == slanInfoPrt->slanIdTx)
    {
        /* slans are same - unbind only one of them */
        if (slanInfoPrt->slanIdRx != NULL)
        {
            SIM_OS_MAC(simOsSlanUnbind)(slanInfoPrt->slanIdRx);
            slanInfoPrt->slanIdRx = NULL;
            slanInfoPrt->slanIdTx = NULL;
            unbindRxDone = GT_TRUE;
            unbindTxDone = GT_TRUE;
        }
    }
    else
    {
        /* unbind rx slan */
        if (slanInfoPrt->slanIdRx != NULL)
        {
            SIM_OS_MAC(simOsSlanUnbind)(slanInfoPrt->slanIdRx);
            slanInfoPrt->slanIdRx = NULL;
            unbindRxDone = GT_TRUE;
        }

        /* unbind tx slan */
        if (slanInfoPrt->slanIdTx != NULL)
        {
            SIM_OS_MAC(simOsSlanUnbind)(slanInfoPrt->slanIdTx);
            slanInfoPrt->slanIdTx = NULL;
            unbindTxDone = GT_TRUE;
        }
    }

    if(unbindRxDone == GT_TRUE || unbindTxDone == GT_TRUE)
    {
        unbindDone = GT_TRUE;
        /* send LOG indications */
        simLogSlanBind(NULL,slanInfoPrt->deviceObj,slanInfoPrt->portNumber,unbindRxDone,unbindTxDone);
    }
    else
    {
        unbindDone = GT_FALSE;
    }


    return unbindDone;
}


/*******************************************************************************
*   smainDevice2SlanBind
*
* DESCRIPTION:
*       Bind ports of devices with SLANs
*
* INPUTS:
*       deviceObjPtr - pointer to the device object for task.
*       deviceId     - device Id with which we search the INI file
*       startPortInIniFile - the offset from the ports of the device to the
*                       ports we search the INI file
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*         Bind ports for FA also.
*
*******************************************************************************/
static void smainDevice2SlanBind(
        IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
        IN GT_U32                  deviceId,
        IN GT_U32                  startPortInIniFile
)
{
    GT_U32               port; /* port number */
    GT_U32               buffLen;
    char                 keyString[SMAIN_MIN_SIZE_OF_BUFFER_CNS]; /* key string */
    char                 slanName[FILE_MAX_LINE_LENGTH_CNS]; /* slan name */
    SMAIN_PORT_SLAN_INFO *slanInfoPrt;          /* slan port entry pointer */
    GT_BOOL              rxSlan, txSlan; /* SLAN type for port */
    char                 updnprefix[10];
    GT_BOOL              nonExistPort;/* indication that port not exists */

    strcpy(keyString,"dev");
    buffLen=3;

    for (port = 0; port < deviceObjPtr->numSlans; port++)
    {
        slanInfoPrt=&(deviceObjPtr->portSlanInfo[port]);
        slanInfoPrt->deviceObj = deviceObjPtr;
        slanInfoPrt->portNumber = port;

        if(port >= deviceObjPtr->portsNumber)
        {
            while(port < deviceObjPtr->numSlans &&
                  deviceObjPtr->portsArr[port].state == SKERNEL_PORT_STATE_NOT_EXISTS_E)
            {
                port++;
            }

            if(port == deviceObjPtr->numSlans)
            {
                break;
            }

            slanInfoPrt=&(deviceObjPtr->portSlanInfo[port]);
            slanInfoPrt->deviceObj = deviceObjPtr;
            slanInfoPrt->portNumber = port;

        }

        if(!IS_CHT_VALID_PORT(deviceObjPtr,port))
        {
            /* skip no exists ports in the range 0..deviceObjPtr->numSlans */
            nonExistPort = GT_TRUE;
        }
        else
        {
            nonExistPort = GT_FALSE;
        }

        if(SMEM_SKIP_LOCAL_READ_WRITE)
        {
            /* no valid use of those SLANS */
            slanInfoPrt->buffId = NULL;
            slanInfoPrt->slanIdRx = NULL;
            slanInfoPrt->slanIdTx = NULL;
            slanInfoPrt->slanName[0] = 0;
            continue;
        }

        rxSlan = txSlan = GT_FALSE;

        /* build the port name (with the device name) for Rx */
        sprintf(keyString+buffLen,"%d_port%drx",deviceId,port+startPortInIniFile);

        /* get SLAN name for port */
        if(SIM_OS_MAC(simOsGetCnfValue)("ports_map", keyString,
                        SMAIN_SLAN_NAME_SIZE_CNS, slanName))
        {
            INI_FILE_SLAN_FOR_NON_EXIST_PORT_CHECK_MAC(nonExistPort,port+startPortInIniFile,deviceId);

            rxSlan = GT_TRUE;

            /* bind port - RX direction with SLAN */
            internalSlanBind(slanName,deviceObjPtr,port,GT_TRUE,GT_FALSE,slanInfoPrt,NULL);

            /* build the port name (with the device name) for Tx */
            sprintf(keyString+buffLen,"%d_port%dtx",deviceId,port+startPortInIniFile);

            /* get SLAN name for port */
            if(SIM_OS_MAC(simOsGetCnfValue)("ports_map", keyString,
                            SMAIN_SLAN_NAME_SIZE_CNS, slanName))
            {
                INI_FILE_SLAN_FOR_NON_EXIST_PORT_CHECK_MAC(nonExistPort,port+startPortInIniFile,deviceId);

                txSlan = GT_TRUE;

                /* bind port - TX direction with SLAN */
                internalSlanBind(slanName,deviceObjPtr,port,GT_FALSE,GT_TRUE,slanInfoPrt,NULL);
            }
        }
        else
        {
            /* build the port name (with the device name) for Tx */
            sprintf(keyString+buffLen,"%d_port%dtx",deviceId,port+startPortInIniFile);

            /* get SLAN name for port */
            if(SIM_OS_MAC(simOsGetCnfValue)("ports_map", keyString,
                            SMAIN_SLAN_NAME_SIZE_CNS, slanName))
            {
                INI_FILE_SLAN_FOR_NON_EXIST_PORT_CHECK_MAC(nonExistPort,port+startPortInIniFile,deviceId);

                txSlan = GT_TRUE;

                /* bind port - TX direction with SLAN */
                internalSlanBind(slanName,deviceObjPtr,port,GT_FALSE,GT_TRUE,slanInfoPrt,NULL);
            }
        }

        if (txSlan == GT_FALSE && rxSlan == GT_FALSE)
        {
            /* build the port name (with the device name)*/
            sprintf(keyString+buffLen,"%d_port%d",deviceId,port+startPortInIniFile);

            /* get SLAN name for port */
            if(!SIM_OS_MAC(simOsGetCnfValue)("ports_map", keyString,
                            SMAIN_SLAN_NAME_SIZE_CNS, slanName))
            {
                /*set port as 'down'*/
                snetLinkStateNotify(deviceObjPtr, port, 0);
                continue;
            }

            INI_FILE_SLAN_FOR_NON_EXIST_PORT_CHECK_MAC(nonExistPort,port+startPortInIniFile,deviceId);

            /* bind port - RX,TX direction with SLAN */
            internalSlanBind(slanName,deviceObjPtr,port,GT_TRUE,GT_TRUE,slanInfoPrt,NULL);

            /* notify link UP Only if prefix is not "sl"                  */
            /* The "ls" prefix should be the same as SAGENT prefix has    */
            memcpy(updnprefix,slanName,2);
            if (memcmp(updnprefix,"ls",2) != 0)
            {
                snetLinkStateNotify(deviceObjPtr, port, 1);
            }
        }
    }
}


static char *smainNicSlanPacketRcv___dummy(
    IN  GT_U32       msg_code,
    IN  GT_U32       res,
    IN  GT_U32       sender_tid,
    IN  GT_U32       len,
    INOUT  void   *        usr_info_PTR,
    INOUT  char   *        buff_PTR
)
{
    skernelFatalError("smainNicSlanPacketRcv___dummy: should not be called");

    return NULL;
}



/*******************************************************************************
*   smainNicSlanPacketRcv
*
* DESCRIPTION:
*       receive packet  (called from slan scope)
*
* INPUTS:
*       msg_code - message code from SLAN.
*       res          - reason for call.
*       sender_tid - task id of sender.
*       len - message length.
*
* OUTPUTS:
*       usr_info_PTR - SLAN user info ptr.
*       buff_PTR - message.
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static char *smainNicSlanPacketRcv(
    IN  GT_U32       msg_code,
    IN  GT_U32       res,
    IN  GT_U32       sender_tid,
    IN  GT_U32       len,
    INOUT  void   *        usr_info_PTR,
    INOUT  char   *        buff_PTR
)
{
    SMAIN_PORT_SLAN_INFO *  slanInfoPrt; /* slan info entry pointer */
    GT_U8 *   segmentList;               /* Segment list */
    GT_U32    segmentLen;                /* Segment length */

    /* get slan info from user info */
    slanInfoPrt = (SMAIN_PORT_SLAN_INFO *)usr_info_PTR;

    /*if ( msg_code == SIM_OS_SLAN_NORMAL_MSG_RSN_CNS )
        return NULL;*/

    switch ( res )
    {
        case SIM_OS_SLAN_NORMAL_MSG_RSN_CNS :
            return NULL;

        /*! Sagent first call - get buffer and user info */
        case SIM_OS_SLAN_GET_BUFF_RSN_CNS :

            /* check data length */
            if (len > SBUF_DATA_SIZE_CNS)
                skernelFatalError("smainNicSlanPacketRcv: length %d too long",
                                 len);

            return (char *)slanInfoPrt->deviceObj->egressBuffer;

            /* Sagent second call - call callback */
        case SIM_OS_SLAN_GIVE_BUFF_SUCCS_RSN_CNS :

            /* call a callback function, add 4 bytes for CRC */
            segmentList = slanInfoPrt->deviceObj->egressBuffer;
            if (slanInfoPrt->deviceObj->crcPortsBytesAdd == 0)
            {
                segmentLen = len + 4;
            }
            else
            {
                segmentLen = len;
            }
            smainNicRxHandler(segmentList, segmentLen);

            return NULL;

        case SIM_OS_SLAN_GIVE_BUFF_ERR_RSN_CNS:

            /* error reason..  */
            simWarningPrintf(" smainNicSlanPacketRcv: Frame Discarded "\
                   " in device %d port %d , for slan =[%s] \n",
                    slanInfoPrt->deviceObj->deviceId,
                    slanInfoPrt->portNumber,
                    slanInfoPrt->slanName);

            return NULL;

    }

    return NULL;
}

/*******************************************************************************
*   smainNicInit
*
* DESCRIPTION:
*       Init simulation of NIC
*
* INPUTS:
*       deviceObjPtr - pointer to the device object for task.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void smainNicInit(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    GT_U32               port; /* port number */
    char        keyString[SMAIN_MIN_SIZE_OF_BUFFER_CNS]; /* key string */
    char        slanName[FILE_MAX_LINE_LENGTH_CNS]; /* slan name */
    SMAIN_PORT_SLAN_INFO * slanInfoPrt;          /* slan port entry pointer */

    /* bind SLAN */
    for (port = 0; port < deviceObjPtr->numSlans; port++)
    {
        slanInfoPrt = &(deviceObjPtr->portSlanInfo[port]);
        slanInfoPrt->deviceObj = deviceObjPtr;
        slanInfoPrt->portNumber = port;

        if(SMEM_SKIP_LOCAL_READ_WRITE)
        {
            /* no valid use of those SLANS */
            slanInfoPrt->buffId = NULL;
            slanInfoPrt->slanIdRx = NULL;
            slanInfoPrt->slanIdTx = NULL;
            slanInfoPrt->slanName[0] = 0;
            continue;
        }

        /* build the port name (with the device name)*/
        sprintf(keyString,"dev%d_port%d",deviceObjPtr->deviceId,port);

        /* get SLAN name for port */
        if(!SIM_OS_MAC(simOsGetCnfValue)("ports_map", keyString,
                        SMAIN_SLAN_NAME_SIZE_CNS, slanName))
        {
            continue;
        }

        /* bind port - RX,TX direction with SLAN */
        internalSlanBind(slanName,deviceObjPtr,port,GT_TRUE,GT_TRUE,slanInfoPrt,
            smainNicSlanPacketRcv);
    }
}


/*******************************************************************************
*   skernelInitGmDevParse
*
* DESCRIPTION:
*       Init for SKernel GM device
*
* INPUTS:
*       deviceObjPtr - allocated pointer for the device
*       devInfoSection - the device info section in the INI file
*       devId          - the deviceId in the INI file info
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
static void skernelInitGmDevParse
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN char*                   devInfoSection,
    IN GT_U32                  devId
)
{
    GT_TASK_PRIORITY_ENT    taskPriority;
    GT_TASK_HANDLE          taskHandl;          /* task handle */

    /* init memory module */
    smemGmInit(deviceObjPtr);
    /* reg file(s) upload */
    smainMemDefaultsLoadAll(deviceObjPtr, devId, devInfoSection);
    /* memory init part 2 */
    smemGmInit2(deviceObjPtr);

    /* network init */
    snetProcessInit(deviceObjPtr);
    taskPriority = GT_TASK_PRIORITY_ABOVE_NORMAL;

    /* Create SKernel task */
    deviceObjPtr->uplink.partnerDeviceID = deviceObjPtr->deviceId;
    taskHandl = SIM_OS_MAC(simOsTaskCreate)(
                    taskPriority,
                    (unsigned (__TASKCONV *)(void*))smainSkernelTask,
                    (void *) deviceObjPtr);
    if (taskHandl == NULL)
    {
        skernelFatalError(" skernelInit: cannot create main task for"\
                           " device %u",devId);
    }

    /* wait for semaphore */
    SIM_OS_MAC(simOsSemWait)(smemInitPhaseSemaphore,SIM_OS_WAIT_FOREVER);
    deviceObjPtr->numThreadsOnMe++;
}

/*******************************************************************************
*   skernelInitComModuleDevParse
*
* DESCRIPTION:
*       Init for SKernel Communication Module device
*
* INPUTS:
*       deviceObjPtr - allocated pointer for the device
*       devInfoSection - the device info section in the INI file
*       devId - the deviceId in the INI file info
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void skernelInitComModuleDevParse
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN char*                   devInfoSection,
    IN GT_U32                  devId
)
{
    GT_CHAR param_str[FILE_MAX_LINE_LENGTH_CNS];
    GT_CHAR  keyStr[SMAIN_MIN_SIZE_OF_BUFFER_CNS];          /* key string*/
    GT_U32 keyVal;              /* Key value */

    deviceObjPtr->cmMemParamPtr =
        (SMEM_CM_INSTANCE_PARAM_STC *)smemDeviceObjMemoryAlloc(deviceObjPtr,1,sizeof(SMEM_CM_INSTANCE_PARAM_STC));

    if(deviceObjPtr->cmMemParamPtr == 0)
    {
        skernelFatalError("skernelInitComModuleDevParse: CM data memory allocation fail");
    }
    memset(deviceObjPtr->cmMemParamPtr, 0, sizeof(SMEM_CM_INSTANCE_PARAM_STC));

    /* Get active interface connected to the HW board - SMI or TWSI */
    sprintf(keyStr, "cm_interface%u", devId);
    if (SIM_OS_MAC(simOsGetCnfValue)(devInfoSection, keyStr,
                   FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        if(strcmp(param_str, "TWSI") == 0 || strcmp(param_str, "twsi") == 0)
        {
            SMEM_CM_INST_INTERFACE_TYPE_SET_MAC(deviceObjPtr, SNET_BUS_TWSI_E);
        }
        else
        {
            SMEM_CM_INST_INTERFACE_TYPE_SET_MAC(deviceObjPtr, SNET_BUS_SMI_E);
        }
    }
    else
    {
        /* Device interface not defined */
        if(deviceObjPtr->numOfCoreDevs)
        {
            skernelFatalError("skernelInitComModuleDevParse: no interface defined!\n");
        }
    }

        /* Get device SMI base address */
        sprintf(keyStr, "dev%u_cm_smi_id", devId);
        if(SIM_OS_MAC(simOsGetCnfValue)(devInfoSection, keyStr,
                       FILE_MAX_LINE_LENGTH_CNS, param_str))
        {
            sscanf(param_str, "%u", &keyVal);
        }
        else
        {
            keyVal = 0;
        }
        /* Set SMI HW Id  */
        SMEM_CM_INST_INTERFACE_ID_SET_MAC(deviceObjPtr, SNET_BUS_SMI_E, keyVal);

        /* Get device TWSI base address */
        sprintf(keyStr, "dev%u_cm_twsi_id", devId);
        if(SIM_OS_MAC(simOsGetCnfValue)(devInfoSection, keyStr,
                       FILE_MAX_LINE_LENGTH_CNS, param_str))
        {
            sscanf(param_str, "%u", &keyVal);
        }
        else
        {
            keyVal = 0;
        }
        /* Set TWSI HW Id  */
        SMEM_CM_INST_INTERFACE_ID_SET_MAC(deviceObjPtr, SNET_BUS_TWSI_E, keyVal);
    }

/*******************************************************************************
*   skernelInitDevParsePart1
*
* DESCRIPTION:
*       Init for SKernel device (part1)
*
* INPUTS:
*       deviceObjPtr - allocated pointer for the device
*       devInfoSection - the device info section in the INI file
*       devId - the deviceId in the INI file info
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void skernelInitDevParsePart1
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN char*                   devInfoSection,
    IN GT_U32                  devId
)
{
    char                    param_str[FILE_MAX_LINE_LENGTH_CNS];
                            /* string for parameter */
    char                    keyStr[SMAIN_MIN_SIZE_OF_BUFFER_CNS]; /* key string*/
    GT_TASK_PRIORITY_ENT    taskPriority;
    GT_TASK_HANDLE          taskHandl;        /* task handle */
    GT_BOOL                 useSkernelTask = GT_FALSE;/*do we need skernel task*/
    GT_BOOL                 useSagingTask = GT_FALSE;/*do we need saging task*/
    GT_U32                  txQue;

    if(deviceObjPtr->shellDevice == GT_TRUE ||
       SKERNEL_DEVICE_FAMILY_EMBEDDED_CPU(deviceObjPtr->deviceType) ||
       SKERNEL_FABRIC_ADAPTER_DEVICE_FAMILY(deviceObjPtr->deviceType)
    )
    {
        useSkernelTask = GT_FALSE;
        useSagingTask = GT_FALSE;
    }
    else if((deviceObjPtr->gmDeviceType != SMAIN_NO_GM_DEVICE) || /* GM devices */
            skernelIsPhyOnlyDev(deviceObjPtr) ||
            skernelIsMacSecOnlyDev(deviceObjPtr))
    {
        useSkernelTask = GT_TRUE;
        useSagingTask = GT_FALSE;
    }
    else
    {
        /* generic switches */
        useSkernelTask = GT_TRUE;
        useSagingTask = GT_TRUE;

        if(skernelIsLion3PortGroupOnlyDev(deviceObjPtr) &&
           deviceObjPtr->portGroupId != LION3_UNIT_FDB_TABLE_SINGLE_INSTANCE_PORT_GROUP_CNS)
        {
            /* SIP5 devices hold single FDB instance , so single aging daemon needed */
            useSagingTask = GT_FALSE;
        }
    }

    /*
       NOTE:
       1. the simulation set device by default with proper value , without
       the need to add this line into the INI file unless for next special cases :
       2. this setting must be done prior to calling smemInit(...)

       allow a device to state that it is not support the PCI configuration
       memory even though the device have PCI configuration memory !

       like DX246/DX107 that used with the SMI interface although it is like DX250
       that uses the PCI interface
    */
    sprintf(keyStr, "dev%u_not_support_pci_config_memory", devId);
    if (SIM_OS_MAC(simOsGetCnfValue)(devInfoSection, keyStr,
                   FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        sscanf(param_str, "%u", &deviceObjPtr->notSupportPciConfigMemory);
    }

    /* get number of port groups */
    sprintf(keyStr, "dev%u_port_groups_num", devId);
    if (SIM_OS_MAC(simOsGetCnfValue)(devInfoSection, keyStr,
                   FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        sscanf(param_str, "%u", &deviceObjPtr->numOfCoreDevs);
    }

    /* get number of 'cores' == 'port groups' */
    sprintf(keyStr, "dev%u_cores_num", devId);
    if (SIM_OS_MAC(simOsGetCnfValue)(devInfoSection, keyStr,
                   FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        sscanf(param_str, "%u", &deviceObjPtr->numOfCoreDevs);
        deviceObjPtr->useCoreTerminology = GT_TRUE;
    }

    if(deviceObjPtr->numOfCoreDevs)
    {
        /* allocate the port group devices */
        deviceObjPtr->coreDevInfoPtr =
            smemDeviceObjMemoryAlloc(deviceObjPtr,deviceObjPtr->numOfCoreDevs, sizeof(SKERNEL_CORE_DEVICE_INFO_STC));
        if (deviceObjPtr->coreDevInfoPtr == NULL)
        {
            skernelFatalError(" skernelInitDevParsePart1: cannot allocate port group devices in device %u", devId);
        }
    }

    if (deviceObjPtr->deviceFamily == SKERNEL_COM_MODULE_FAMILY)
    {
        skernelInitComModuleDevParse(deviceObjPtr, devInfoSection, devId);
    }

    /* memory init */
    smemInit(deviceObjPtr);

    if (deviceObjPtr->deviceFamily == SKERNEL_COM_MODULE_FAMILY)
    {
        /* No need further initialization for Communication module */
        return;
    }

    /* network init */
    snetProcessInit(deviceObjPtr);

    smainMemDefaultsLoadAll(deviceObjPtr, devId, devInfoSection);

    /* memory init part 2 */
    smemInit2(deviceObjPtr);

    if (!SKERNEL_FABRIC_ADAPTER_DEVICE_FAMILY(deviceObjPtr->deviceType))
    {
        deviceObjPtr->uplink.partnerDeviceID = deviceObjPtr->deviceId;
    }

    taskPriority = GT_TASK_PRIORITY_ABOVE_NORMAL;

    if(deviceObjPtr->softResetOldDevicePtr)
    {
        /* do not create any more tasks , but use existing threads */

        for(txQue = 0; txQue < 8 ; txQue++)
        {
            SIM_TAKE_PARAM_FROM_OLD_DEVICE_MAC(deviceObjPtr,sdmaTransmitInfo.sdmaTransmitData[txQue].taskHandle);
            if(deviceObjPtr->sdmaTransmitInfo.sdmaTransmitData[txQue].taskHandle)
            {
                deviceObjPtr->sdmaTransmitInfo.sdmaTransmitData[txQue].txQueue = txQue;
                deviceObjPtr->sdmaTransmitInfo.sdmaTransmitData[txQue].devObjPtr = deviceObjPtr;
                deviceObjPtr->numThreadsOnMe++;
            }
        }

        if(useSkernelTask == GT_TRUE)
        {
            deviceObjPtr->numThreadsOnMe++;
        }

        if(useSagingTask == GT_TRUE)
        {
            deviceObjPtr->numThreadsOnMe++;
        }

        if(deviceObjPtr->oamSupport.keepAliveSupport)
        {
            deviceObjPtr->numThreadsOnMe++;
        }

    }
    else
    {
        if(useSkernelTask == GT_TRUE)
        {
           taskHandl = SIM_OS_MAC(simOsTaskCreate)(
                            taskPriority,
                            (unsigned (__TASKCONV *)(void*))smainSkernelTask,
                            (void *) deviceObjPtr);
            if (taskHandl == NULL)
            {
                skernelFatalError(" skernelInit: cannot create main task for"\
                                   " device %u",devId);
            }
            /* wait for semaphore */
            SIM_OS_MAC(simOsSemWait)(smemInitPhaseSemaphore,SIM_OS_WAIT_FOREVER);
            deviceObjPtr->numThreadsOnMe++;
        }

        if(useSagingTask == GT_TRUE)
        {
            taskHandl = SIM_OS_MAC(simOsTaskCreate)(taskPriority,
                              (unsigned (__TASKCONV *)(void*))smainSagingTask,
                              (void *) deviceObjPtr);
            if (taskHandl == NULL)
            {
                skernelFatalError(" skernelInit: cannot create aging "\
                                  " task for device %u", devId);
            }
            /* wait for semaphore */
            SIM_OS_MAC(simOsSemWait)(smemInitPhaseSemaphore,SIM_OS_WAIT_FOREVER);
            deviceObjPtr->numThreadsOnMe++;
        }

        if(deviceObjPtr->oamSupport.keepAliveSupport)
        {
            taskHandl = SIM_OS_MAC(simOsTaskCreate)(taskPriority,
                              (unsigned (__TASKCONV *)(void*))smainOamKeepAliveTask,
                              (void *) deviceObjPtr);
            if (taskHandl == NULL)
            {
                skernelFatalError(" skernelInit: cannot create keep alive aging "\
                                  " task for device %u", devId);
            }
            /* wait for semaphore */
            SIM_OS_MAC(simOsSemWait)(smemInitPhaseSemaphore, SIM_OS_WAIT_FOREVER);
            deviceObjPtr->numThreadsOnMe++;
        }
    }

    if (SKERNEL_FABRIC_ADAPTER_DEVICE_FAMILY(deviceObjPtr->deviceType))
    {
        smainSwitchFabricInit(deviceObjPtr);
    }
}

/*******************************************************************************
*   smainBusDevFind
*
* DESCRIPTION:
*       find device on the bus.
*
* INPUTS:
*       busId - the device that we look the device on
*       targetHwDeviceId - the target HW device ID of the device that we look for.
*
* OUTPUTS:
*       targetDeviceIdPtr - (pointer to) the target device number.
*                       with this number we can access the SCIB API to access
*                       this device.
*
* RETURNS:
*       GT_TRUE - device found
*       GT_FALSE - device not found
* COMMENTS:
*
*
*******************************************************************************/
GT_BOOL smainBusDevFind
(
    IN GT_U32   busId,
    IN GT_U32   targetHwDeviceId,
    OUT GT_U32  *targetDeviceIdPtr
)
{
    SKERNEL_DEVICE_OBJECT * deviceObjPtr;/* device object */
    SKERNEL_DEVICE_OBJECT * coreDeviceObjPtr;/* core device object */
    GT_U32  ii,jj,kk;

    for(ii = 0 ; ii < numberOfBuses; ii++)
    {
        if(busInfoArr[ii].id == busId)
        {
            /* the bus found */
            break;
        }
    }

    if(ii == numberOfBuses)
    {
        /* bus not found */
        return GT_FALSE;
    }

    for(jj = 0 ; jj < SMAIN_MAX_NUM_OF_DEVICES_CNS ; jj++)
    {
        deviceObjPtr = smainDeviceObjects[jj];

        if(0 == (busInfoArr[ii].membersBmp[jj/32] & (1 << jj%32)))
        {
            continue;
        }

        if(deviceObjPtr->shellDevice == GT_TRUE)
        {
            /* the shell device register once , so all it's cores not need to */

            /* so we need to find the relevant core */
            for(kk = 0 ; kk < deviceObjPtr->numOfCoreDevs ; kk++ )
            {
                coreDeviceObjPtr = deviceObjPtr->coreDevInfoPtr[kk].devObjPtr;

                if(coreDeviceObjPtr->deviceHwId == targetHwDeviceId)
                {
                    *targetDeviceIdPtr = coreDeviceObjPtr->deviceId;
                    return GT_TRUE;
                }
            }

            /* no core of this device match the targetHwDeviceId */

            /* the shell device don't have 'HW device id' */
            continue;
        }

        if(deviceObjPtr->deviceHwId != targetHwDeviceId)
        {
            continue;
        }

        *targetDeviceIdPtr = deviceObjPtr->deviceId;
        return GT_TRUE;
    }

    return GT_FALSE;
}

/*******************************************************************************
*   smainBusSet
*
* DESCRIPTION:
*       register the device on the bus
*
* INPUTS:
*       deviceObjPtr - allocated pointer for the device
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
static void smainBusSet
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
)
{
    GT_U32  ii;

    for(ii = 0 ; ii < numberOfBuses; ii++)
    {
        if(busInfoArr[ii].id == deviceObjPtr->busId)
        {
            /* the bus found */
            break;
        }
    }

    if(ii == numberOfBuses)
    {
        /* bus not found */
        numberOfBuses++;

        if(numberOfBuses >= MAX_BUS_NUM_CNS)
        {
            skernelFatalError(" skernelInit: not supporting [%d] buses",
                                                    numberOfBuses+1);
        }
    }

    /* register the device on this bus */
    busInfoArr[ii].id = deviceObjPtr->busId;
    /* update the devices on this bus */
    busInfoArr[ii].membersBmp[deviceObjPtr->deviceId / 32] |= 1 << (deviceObjPtr->deviceId % 32);
}


/*******************************************************************************
*   skernelInitDevParse
*
* DESCRIPTION:
*       Init for SKernel device
*
* INPUTS:
*       deviceObjPtr - allocated pointer for the device
*       devInfoSection - the device info section in the INI file
*       devId - the deviceId in the INI file info
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void skernelInitDevParse
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN char*                   devInfoSection,
    IN GT_U32                  devId
)
{
    char                    param_str[FILE_MAX_LINE_LENGTH_CNS];
                            /* string for parameter */
    char                    keyStr[SMAIN_MIN_SIZE_OF_BUFFER_CNS]; /* key string*/
    GT_U32                  intLine;         /* interrupt line number */
    char                    currDevStr[SMAIN_MIN_SIZE_OF_BUFFER_CNS]={0};
    char                    currRightSideDevStr[FILE_MAX_LINE_LENGTH_CNS]={0};
    GT_U32                  devHwId;      /* device Hw Id */
    GT_U32                  tmpRightSizeDev;
    GT_U32                  portGroupDevId=devId;
    GT_U32                  coreClock;

    deviceObjPtr->tmpPeerDeviceId = SMAIN_NOT_VALID_CNS;

    /* default address completion - 4 regions */
    deviceObjPtr->addressCompletionType = SKERNEL_ADDRESS_COMPLETION_TYPE_4_REGIONS_E;

    /* get device type */
    sprintf(keyStr, "device_type%u", devId);
    if (!SIM_OS_MAC(simOsGetCnfValue)(devInfoSection, keyStr,
                   FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        /* device's type not found */
        skernelFatalError(" skernelInit: type for device %u not defined",
                                                devId);
    }

    /* convert to lower case */
    strlwr(param_str);

    strncpy(deviceObjPtr->deviceName, param_str,
                            SKERNEL_DEVICE_NAME_MAX_SIZE_CNS );


    if(deviceObjPtr->portGroupSharedDevObjPtr)
    {
        portGroupDevId = lastDeviceIdUsedForCores++;

        if(portGroupDevId >= SMAIN_MAX_NUM_OF_DEVICES_CNS)
        {
            skernelFatalError(" skernelInit: shared device out of range %u",devId);
        }

        smainDeviceObjects[portGroupDevId] = deviceObjPtr;
    }
    else
    {
        smainDeviceObjects[devId] = deviceObjPtr;
    }

    /* convert device type name to integer and get ports number  */
    smainDevInfoGet(param_str,deviceObjPtr);

    /* get device hwId */
    sprintf(keyStr, "dev%u_hw_id", devId);
    if(SIM_OS_MAC(simOsGetCnfValue)(devInfoSection, keyStr,
                   FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        sscanf(param_str, "%u", &devHwId);
    }
    else
    {
        devHwId = 0;
    }

    if(skernelIsPhyOnlyDev(deviceObjPtr))
    {
        if(devHwId > 31)
        {
            skernelFatalError(" skernelInit: PHY dev[%u] hold HW id > 31 [%u]",devId,devHwId);
        }
    }

    deviceObjPtr->deviceHwId = devHwId;

    if(deviceObjPtr->portGroupSharedDevObjPtr)
    {
        deviceObjPtr->deviceId = portGroupDevId;
    }
    else
    {
        deviceObjPtr->deviceId = devId;
    }

    if(deviceObjPtr->deviceFamily == SKERNEL_EMPTY_FAMILY)
    {
        /* skip this device */
        return;
    }

    /* Get device revision ID */
    smainDeviceRevisionGet(deviceObjPtr, devId);

    /* allocate and reset SLAN table for device */
    if(deviceObjPtr->numSlans)
    {
        deviceObjPtr->portSlanInfo = smemDeviceObjMemoryAlloc(deviceObjPtr,1, sizeof (SMAIN_PORT_SLAN_INFO)
                                                 * deviceObjPtr->numSlans);
    }

    /* Add FCS 4 bytes receive/send packets SLAN */
    sprintf(keyStr, "dev%u_to_slan_fcs_bytes_add", devId);
    if(SIM_OS_MAC(simOsGetCnfValue)(devInfoSection, keyStr,
                   FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        sscanf(param_str, "%u", &deviceObjPtr->crcPortsBytesAdd);
    }
    else
    {
        /* Default value is ADD for a backward compatible */
        deviceObjPtr->crcPortsBytesAdd = 0;
    }

    if (deviceObjPtr->deviceType == SKERNEL_NIC)
    {
        /* store NIC device pointer */
        smainNicDeviceObjPtr = deviceObjPtr;

        /* init NIC simulation */
        smainNicInit(deviceObjPtr);
        return;
    }

    if(skernelIsLionShellOnlyDev(deviceObjPtr) ||
       skernelIsPhyShellOnlyDev(deviceObjPtr) ||
       skernelIsPumaShellOnlyDev(deviceObjPtr))
    {
        deviceObjPtr->shellDevice = GT_TRUE;
    }

    if(deviceObjPtr->shellDevice == GT_FALSE)
    {
        if(deviceObjPtr->softResetOldDevicePtr)
        {
            /* the bufPool,queueId already taken (copied) from the old device */
        }
        else
        {
            /* create buffers pool */
            deviceObjPtr->bufPool = sbufPoolCreate(SMAIN_BUFFERS_NUMBER_CNS);

            /* create SQueue */
            deviceObjPtr->queueId = squeCreate();
        }
    }

    /* Init uplink partner info */
    sprintf(currDevStr,"dev%d",devId);
    if (SIM_OS_MAC(simOsGetCnfValue)("uplink",
                         currDevStr,
                         FILE_MAX_LINE_LENGTH_CNS,
                         currRightSideDevStr))
    {
        /* build dev str */
        sscanf(currRightSideDevStr,"dev%d",&tmpRightSizeDev);
        deviceObjPtr->uplink.partnerDeviceID = tmpRightSizeDev;
    }

    /* Init and runs Golden Model simulation */
    if ((deviceObjPtr->gmDeviceType != SMAIN_NO_GM_DEVICE) &&
        (deviceObjPtr->gmDeviceType != GOLDEN_MODEL))
    {
        /* support removed */
    }
    else
    if (deviceObjPtr->gmDeviceType == GOLDEN_MODEL)
    {
        /* specific GM initializations */
        skernelInitGmDevParse(deviceObjPtr,devInfoSection,devId);
    }
    else
    {
        /* specific device (non-GM) initializations */
        skernelInitDevParsePart1(deviceObjPtr,devInfoSection,devId);
    }

    if(sasicgSimulationRole == SASICG_SIMULATION_ROLE_NON_DISTRIBUTED_E)
    {
        SIM_OS_MAC(simOsSlanInit)();

        if(deviceObjPtr->portGroupSharedDevObjPtr)
        {
            smainDevice2SlanBind(deviceObjPtr,
                                 devId,
                                 deviceObjPtr->portGroupSharedDevObjPtr->coreDevInfoPtr[deviceObjPtr->portGroupId].startPortNum);
        }
        else
        {
            /* bind with SLANs                 */
            smainDevice2SlanBind(deviceObjPtr,devId,0);
        }

        SIM_OS_MAC(simOsSlanStart)();
    }

    /* bind with interrupts */
    sprintf(keyStr, "dev%u_int_line", devId);
    if(SIM_OS_MAC(simOsGetCnfValue)(devInfoSection, keyStr,
                  FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        sscanf(param_str, "%u", &intLine);
        if(sasicgSimulationRole == SASICG_SIMULATION_ROLE_NON_DISTRIBUTED_E)
        {
            scibSetIntLine(deviceObjPtr->deviceId,intLine);
        }
        deviceObjPtr->interruptLine = intLine;
    }
    else
    {
         deviceObjPtr->interruptLine = SCIB_INTERRUPT_LINE_NOT_USED_CNS;
    }

    if(deviceObjPtr->shellDevice == GT_FALSE)
    {
        sprintf(keyStr, "dev%u_core_clock", devId);
        if(SIM_OS_MAC(simOsGetCnfValue)(devInfoSection, keyStr,
                      FILE_MAX_LINE_LENGTH_CNS, param_str))
        {
            sscanf(param_str, "%u", &coreClock);/* value in MHz */

            smemCheetahUpdateCoreClockRegister(
                deviceObjPtr,coreClock,SMAIN_NOT_VALID_CNS);
        }
    }



    /* Add FCS 4 bytes for send to CPU packets */
    sprintf(keyStr, "dev%u_to_cpu_fcs_bytes_add", devId);
    if(SIM_OS_MAC(simOsGetCnfValue)(devInfoSection, keyStr,
                   FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        sscanf(param_str, "%u", &deviceObjPtr->crcBytesAdd);
    }
    else
    {
        /* Default value is ADD for a backward compatible */
        deviceObjPtr->crcBytesAdd = 1;

        if(deviceObjPtr->portGroupSharedDevObjPtr)
        {
            /* we allow the port group device to use the info from the 'portGroupSharedDevObjPtr'
               device */
            if(SIM_OS_MAC(simOsGetCnfValue)(INI_FILE_SYSTEM_SECTION_CNS, keyStr,
                                            FILE_MAX_LINE_LENGTH_CNS, param_str))
            {
                sscanf(param_str, "%u", &deviceObjPtr->crcBytesAdd);
            }
        }
    }

    /* Enable FCS calculation for ethernet packets */
    sprintf(keyStr, "dev%u_calc_fcs_enable", devId);
    if(SIM_OS_MAC(simOsGetCnfValue)(devInfoSection, keyStr,
                   FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        sscanf(param_str, "%u", &deviceObjPtr->calcFcsEnable);
    }
    else
    {
        /* Default value is disable FCS calculation  */
        deviceObjPtr->calcFcsEnable = 0;
    }

    /* the PHY need to know the 'smi_bus_id' that the HW_id is limited to this context */
    sprintf(keyStr, "dev%u_bus_id", devId);
    if(SIM_OS_MAC(simOsGetCnfValue)(devInfoSection, keyStr,FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        sscanf(param_str, "%u", &deviceObjPtr->busId);
    }
    else
    {
        deviceObjPtr->busId = SMAIN_NOT_VALID_CNS;
    }

    /* set bus info */
    smainBusSet(deviceObjPtr);

}

/*******************************************************************************
*   skernelInitDevUplinkParse
*
* DESCRIPTION:
*       Init for SKernel devices with UPlinks
*
* INPUTS:
*       deviceObjPtr - allocated pointer for the device
*       devId - the deviceId in the INI file info
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void skernelInitDevUplinkParse
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  devId
)
{
    char                    currDevStr[SMAIN_MIN_SIZE_OF_BUFFER_CNS]={0};
    char                    currRightSideDevStr[FILE_MAX_LINE_LENGTH_CNS]={0};
    GT_U32                  tmpRightSizeDev;
    SKERNEL_DEVICE_OBJECT   *destDeviceObjPtr;
    GT_U32                  ii;

    deviceObjPtr->uplink.type = SKERNEL_UPLINK_TYPE_NONE_E;

    if (deviceObjPtr->gmDeviceType != SMAIN_NO_GM_DEVICE)
    {
        return;
    }

    sprintf(currDevStr,"dev%d",devId);
    if (!SIM_OS_MAC(simOsGetCnfValue)("uplink",
                        currDevStr,
                        FILE_MAX_LINE_LENGTH_CNS,
                        currRightSideDevStr))
    {
        return;
    }

    /* build dev str */
    sscanf(currRightSideDevStr,"dev%d",&tmpRightSizeDev);
    smainSwitchFabricDevObjGetByUplinkId(tmpRightSizeDev,&destDeviceObjPtr);
    if ( (destDeviceObjPtr != NULL) &&
        (destDeviceObjPtr->deviceId != deviceObjPtr->deviceId ) )
    {
        if(SKERNEL_FABRIC_ADAPTER_DEVICE_FAMILY(destDeviceObjPtr->deviceType))
        {
            if(!(SKERNEL_FABRIC_ADAPTER_DEVICE_FAMILY(deviceObjPtr->deviceType)))
            {
                deviceObjPtr->uplink.type = SKERNEL_UPLINK_TYPE_TO_FA_E;
                deviceObjPtr->uplink.partnerDeviceID = tmpRightSizeDev;
                destDeviceObjPtr->uplink.type = SKERNEL_UPLINK_TYPE_TO_PP_E;
                destDeviceObjPtr->uplink.partnerDeviceID = devId;
            }
            else
            {   /* FAP is not able to be connected to fap/fe with uplink*/
                skernelFatalError(" smainDevInfoGet: illegal \
                                   configuration for devId %d ", devId);
            }
        }
        else
        {
            if (!(SKERNEL_FABRIC_ADAPTER_DEVICE_FAMILY(deviceObjPtr->deviceType)))
            {
                deviceObjPtr->uplink.type = SKERNEL_UPLINK_TYPE_B2B_E;
                deviceObjPtr->uplink.partnerDeviceID = tmpRightSizeDev;
                destDeviceObjPtr->uplink.type = SKERNEL_UPLINK_TYPE_B2B_E;
                destDeviceObjPtr->uplink.partnerDeviceID = devId;

                deviceObjPtr->tmpPeerDeviceId = tmpRightSizeDev;
                destDeviceObjPtr->tmpPeerDeviceId = devId;

                if(destDeviceObjPtr->shellDevice == GT_TRUE)
                {
                    /* set all cores with the info */
                    for(ii = 0 ; ii < destDeviceObjPtr->numOfCoreDevs ;ii++)
                    {
                        destDeviceObjPtr->coreDevInfoPtr[ii].devObjPtr->tmpPeerDeviceId =
                            destDeviceObjPtr->tmpPeerDeviceId;
                    }
                }

                if(deviceObjPtr->shellDevice == GT_TRUE)
                {
                    /* set all cores with the info */
                    for(ii = 0 ; ii < deviceObjPtr->numOfCoreDevs ;ii++)
                    {
                        deviceObjPtr->coreDevInfoPtr[ii].devObjPtr->tmpPeerDeviceId =
                            deviceObjPtr->tmpPeerDeviceId;
                    }
                }

            }
            else
            {
                deviceObjPtr->uplink.type = SKERNEL_UPLINK_TYPE_TO_PP_E;
                deviceObjPtr->uplink.partnerDeviceID = tmpRightSizeDev;
                destDeviceObjPtr->uplink.type = SKERNEL_UPLINK_TYPE_TO_FA_E;
                destDeviceObjPtr->uplink.partnerDeviceID = devId;
            }
        }
    }

    return;
}

/*******************************************************************************
*   skernelInitInternalConnectionParse
*
* DESCRIPTION:
*       Init for SKernel for 'internal connection' that not need slans
*
* INPUTS:
*       deviceObjPtr - allocated pointer for the device
*       devId - the deviceId in the INI file info
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void skernelInitInternalConnectionParse
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  devId
)
{
    char                    currDevStr[SMAIN_MIN_SIZE_OF_BUFFER_CNS]={0}; /*string to build device+port for lookup in INI file */
    char                    currRightSideInfoStr[FILE_MAX_LINE_LENGTH_CNS]={0};/*string to read from INI file*/
    GT_U32                  tmpRightSizeDev; /* right side device id */
    GT_U32                  tmpRightSizePort;/* right side port id */
    SKERNEL_DEVICE_OBJECT   *destDeviceObjPtr;/* (pointer to)right side device object */
    SMAIN_PORT_SLAN_INFO    *slanInfoPtr; /* slan port entry pointer */
    GT_U32                  portIterator; /* port iterator */
    GT_U32                  ii;/* iterator */
    GT_U8                   leftSideCorePort;/* left side port number -- in context of the core */
    GT_U8                   rightSideCorePort;/* right side port number -- in context of the core */
    GT_U32                  portsNumber; /* number of ports to loop over */
    SKERNEL_DEVICE_OBJECT   *leftSideDeviceCoreObjPtr;/*(pointer to) left side device core object*/
    SKERNEL_DEVICE_OBJECT   *rightSideDeviceCoreObjPtr;/*(pointer to) right side device core object*/

    if(deviceObjPtr->shellDevice == GT_TRUE)
    {
        leftSideDeviceCoreObjPtr = deviceObjPtr->coreDevInfoPtr[deviceObjPtr->numOfCoreDevs - 1].devObjPtr;
        if(leftSideDeviceCoreObjPtr == NULL)
        {
            /* this can happen when the core is 'not valid' */
            return;
        }
        portsNumber = deviceObjPtr->coreDevInfoPtr[deviceObjPtr->numOfCoreDevs - 1].startPortNum +
                      leftSideDeviceCoreObjPtr->portsNumber;
        if(portsNumber > SKERNEL_DEV_MAX_SUPPORTED_PORTS_CNS)
        {
            portsNumber = SKERNEL_DEV_MAX_SUPPORTED_PORTS_CNS;
        }
    }
    else
    {
        portsNumber = deviceObjPtr->portsNumber;
    }

    /* initialize ' internal connections' */
    for(portIterator = 0 ; portIterator < (portsNumber + 1) ; portIterator ++)
    {
        if(portIterator == portsNumber)
        {
            /* last iteration */
            if(portIterator > PRESTERA_CPU_PORT_CNS)
            {
                /* CPU port already in the valid range of ports */
                break;
            }

            portIterator = PRESTERA_CPU_PORT_CNS;
        }

        sprintf(currDevStr,"dev%d_port%d",devId,portIterator);
        if (!SIM_OS_MAC(simOsGetCnfValue)("internal_connections",
                            currDevStr,
                            FILE_MAX_LINE_LENGTH_CNS,
                            currRightSideInfoStr))
        {
            continue;
        }

        if(deviceObjPtr->shellDevice == GT_TRUE)
        {
            /* find to which core the port belongs */
            for(ii = 0 ; ii < (deviceObjPtr->numOfCoreDevs - 1) ;ii++)
            {
                if(portIterator < deviceObjPtr->coreDevInfoPtr[ii + 1].startPortNum)
                {
                    break;
                }
            }

            leftSideDeviceCoreObjPtr = deviceObjPtr->coreDevInfoPtr[ii].devObjPtr;
            if(leftSideDeviceCoreObjPtr == NULL)
            {
                /* this can happen when the core is 'not valid' */
                continue;
            }
            leftSideCorePort = portIterator - deviceObjPtr->coreDevInfoPtr[ii].startPortNum;
        }
        else
        {
            leftSideDeviceCoreObjPtr = deviceObjPtr;
            leftSideCorePort = portIterator;
        }

        if(portIterator == PRESTERA_CPU_PORT_CNS)
        {
            /*for CPU port we not check 'state'*/
        }
        else
        if(leftSideCorePort >= leftSideDeviceCoreObjPtr->portsNumber ||
           leftSideDeviceCoreObjPtr->portsArr[leftSideCorePort].state == SKERNEL_PORT_STATE_NOT_EXISTS_E)
        {
            skernelFatalError("skernelInit: cant bind non exists port to internal connection in dev[%d],port[%d] \n",devId, portIterator);
        }

        slanInfoPtr=&(leftSideDeviceCoreObjPtr->portSlanInfo[leftSideCorePort]);
        if(slanInfoPtr->slanName[0] != 0)
        {
            skernelFatalError("skernelInit: cant bind to SLAN and to internal connection in dev[%d],port[%d] \n",devId, portIterator);
        }

        /* build dev , port str */
        sscanf(currRightSideInfoStr,"dev%d_port%d",&tmpRightSizeDev,&tmpRightSizePort);

        destDeviceObjPtr = smainDeviceObjects[tmpRightSizeDev];

        if(tmpRightSizeDev >= smainDevicesNumber || destDeviceObjPtr == NULL)
        {
            skernelFatalError("skernelInit: cant bind to non exists device[%d],port[%d] \n",tmpRightSizeDev, tmpRightSizePort);
        }

        if(destDeviceObjPtr->shellDevice == GT_TRUE)
        {
            /* find to which core the port belongs */
            for(ii = 0 ; ii < (destDeviceObjPtr->numOfCoreDevs - 1) ;ii++)
            {
                if(tmpRightSizePort < destDeviceObjPtr->coreDevInfoPtr[ii + 1].startPortNum)
                {
                    break;
                }
            }

            rightSideDeviceCoreObjPtr = destDeviceObjPtr->coreDevInfoPtr[ii].devObjPtr;
            rightSideCorePort = tmpRightSizePort - destDeviceObjPtr->coreDevInfoPtr[ii].startPortNum;
        }
        else
        {
            rightSideDeviceCoreObjPtr = destDeviceObjPtr;
            rightSideCorePort = tmpRightSizePort;
        }



        if(rightSideCorePort == PRESTERA_CPU_PORT_CNS)
        {
            /*for CPU port we not check 'state'*/
        }
        else
        if(rightSideCorePort >= rightSideDeviceCoreObjPtr->portsNumber ||
           rightSideDeviceCoreObjPtr->portsArr[rightSideCorePort].state == SKERNEL_PORT_STATE_NOT_EXISTS_E )
        {
            skernelFatalError("skernelInit: cant bind non exists port to internal connection in dev[%d],port[%d] \n",tmpRightSizeDev, tmpRightSizePort);
        }

        leftSideDeviceCoreObjPtr->portsArr[leftSideCorePort].linkStateWhenNoForce = SKERNEL_PORT_NATIVE_LINK_UP_E;
        /* notify that the port is up - on left side device */
        snetLinkStateNotify(leftSideDeviceCoreObjPtr, leftSideCorePort, 1);

        leftSideDeviceCoreObjPtr->portsArr[leftSideCorePort].peerInfo.usePeerInfo = GT_TRUE;
        leftSideDeviceCoreObjPtr->portsArr[leftSideCorePort].peerInfo.peerDeviceId = rightSideDeviceCoreObjPtr->deviceId;
        leftSideDeviceCoreObjPtr->portsArr[leftSideCorePort].peerInfo.peerPortNum =  rightSideCorePort;


        rightSideDeviceCoreObjPtr->portsArr[rightSideCorePort].linkStateWhenNoForce = SKERNEL_PORT_NATIVE_LINK_UP_E;
        /* notify that the port is up - on right side device */
        snetLinkStateNotify(rightSideDeviceCoreObjPtr, rightSideCorePort, 1);

        rightSideDeviceCoreObjPtr->portsArr[rightSideCorePort].peerInfo.usePeerInfo = GT_TRUE;
        rightSideDeviceCoreObjPtr->portsArr[rightSideCorePort].peerInfo.peerDeviceId = leftSideDeviceCoreObjPtr->deviceId;
        rightSideDeviceCoreObjPtr->portsArr[rightSideCorePort].peerInfo.peerPortNum =  leftSideCorePort;
    }

    return;
}



/*******************************************************************************
*   skernelInitDistributedDevParse
*
* DESCRIPTION:
*       Init for SKernel for 'Distributed simulation'
*       send to the application side the minimal info needed about the devices
* INPUTS:
*       None
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void skernelInitDistributedDevParse
(
    void
)
{
    SKERNEL_DEVICE_OBJECT * deviceObjPtr;  /* pointer to the device object*/
    SIM_DISTRIBUTED_INIT_DEVICE_STC *devicesArray;
    GT_U32 numDevs = sinitOwnBoardSection.numDevices ? sinitOwnBoardSection.numDevices :smainDevicesNumber;
    GT_U32 ii;
    GT_U32 devId; /* device id loop */

    devicesArray = malloc(numDevs * sizeof(SIM_DISTRIBUTED_INIT_DEVICE_STC));

    for (ii=0,devId = 0; devId < smainDevicesNumber; devId++)
    {
        if (!smainDeviceObjects[devId])
           continue;

        deviceObjPtr = smainDeviceObjects[devId]   ;

        devicesArray[ii].deviceId    = deviceObjPtr->deviceId;
        devicesArray[ii].deviceHwId  = deviceObjPtr->deviceHwId;
        devicesArray[ii].interruptLine = deviceObjPtr->interruptLine;
        devicesArray[ii].addressCompletionStatus = ADDRESS_COMPLETION_STATUS_GET_MAC(deviceObjPtr);
        devicesArray[ii].nicDevice = IS_NIC_DEVICE_MAC(deviceObjPtr);

        switch(deviceObjPtr->deviceFamily)
        {
            case SKERNEL_FAP_DUNE_FAMILY:
            case SKERNEL_FA_FOX_FAMILY:
                devicesArray[ii].isPp = 0;
                break;
            default:
                devicesArray[ii].isPp = 1;
                break;
        }

        ii++;
    }
    /* since next command may wait for the other side to connect ,
       we notify that most issues done , and terminal can be connected.
    */
    simulationInitReady = 1;

    /* sent initialization to the remote SCIB ! */
    simDistributedRemoteInit(numDevs,devicesArray);

    free(devicesArray);
    devicesArray = NULL;

    /* the slan notification involve generation of interrupt */
    /* all messages to Server must come after
       simDistributedRemoteInit(...) */
    for (devId = 0; devId < smainDevicesNumber; devId++)
    {
        if (!smainDeviceObjects[devId])
           continue;

        deviceObjPtr = smainDeviceObjects[devId]   ;

        SIM_OS_MAC(simOsSlanInit)();

        if(deviceObjPtr->portGroupSharedDevObjPtr)
        {
            smainDevice2SlanBind(deviceObjPtr,
                                 devId,
                                 deviceObjPtr->portGroupSharedDevObjPtr->coreDevInfoPtr[deviceObjPtr->portGroupId].startPortNum);
        }
        else
        {
            /* bind with SLANs                 */
            smainDevice2SlanBind(deviceObjPtr,devId,0);
        }

        SIM_OS_MAC(simOsSlanStart)();
    }
}

/*******************************************************************************
*   skernelInitDevicePart1
*
* DESCRIPTION:
*       Init for SKernel device
*
* INPUTS:
*       deviceObjPtr - allocated pointer for the device
*       devId - the deviceId in the INI file info
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void skernelInitDevicePart1
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  devId
)
{
    SKERNEL_DEVICE_OBJECT * portGroupDeviceObjPtr;  /* pointer to the device object*/
    char                    devInfoSection[SMAIN_MIN_SIZE_OF_BUFFER_CNS];/*the device info section in the INI file*/
    char                    param_str[FILE_MAX_LINE_LENGTH_CNS]; /* string for parameter */
    char                    keyStr[SMAIN_MIN_SIZE_OF_BUFFER_CNS]; /* key string*/
    GT_U32                  jj;

    sprintf(devInfoSection, "%s",INI_FILE_SYSTEM_SECTION_CNS);

    /* create semaphore for 'wait' for threads to start process */
    smemInitPhaseSemaphore = SIM_OS_MAC(simOsSemCreate)(0,1);
    if (smemInitPhaseSemaphore == (GT_SEM)0)
    {
        skernelFatalError(" skernelInit: cannot create semaphore");
        return;
    }

    /* call to init the device */
    skernelInitDevParse(deviceObjPtr,devInfoSection,devId);

    if(deviceObjPtr->deviceFamily == SKERNEL_EMPTY_FAMILY)
    {
        /* disconnect from the array of devices */
        smainDeviceObjects[deviceObjPtr->deviceId] = NULL;

        /* free the allocated memory */
        smemDeviceObjMemoryPtrFree(deviceObjPtr,deviceObjPtr);

        simGeneralPrintf("device[%u] skipped \n",devId);

        /* skip this device */
        goto exitCleanly_lbl;
    }

    simForcePrintf("device[%u] ready : %s \n",devId,familyNamesArr[deviceObjPtr->deviceFamily]);
    simForcePrintf("allocated [%u] bytes \n",deviceObjPtr->totalNumberOfBytesAllocated);


    if(deviceObjPtr->numOfCoreDevs == 0 ||
       deviceObjPtr->coreDevInfoPtr[0].devObjPtr)
    {
        goto exitCleanly_lbl;
    }

    simForcePrintf(" ** Multi-core device , start init it's cores \n");

    /* create the egress protecting mutex */
    if(deviceObjPtr->softResetOldDevicePtr)
    {
        SIM_TAKE_PARAM_FROM_OLD_DEVICE_MAC(deviceObjPtr,protectEgressMutex);
        SIM_TAKE_PARAM_FROM_OLD_DEVICE_MAC(deviceObjPtr,fullPacketWalkThroughProtectMutex);
    }
    else
    {
        deviceObjPtr->protectEgressMutex = SIM_OS_MAC(simOsMutexCreate)();
        deviceObjPtr->fullPacketWalkThroughProtectMutex = SIM_OS_MAC(simOsMutexCreate)();
    }

    for(jj = 0 ; jj < deviceObjPtr->numOfCoreDevs; jj++)
    {
        if(deviceObjPtr->useCoreTerminology == GT_TRUE)
        {
            sprintf(devInfoSection, "dev%u_core%u",devId,jj);

            /* get valid port group value */
            sprintf(keyStr,"%s" ,"valid_core");
        }
        else
        {
            sprintf(devInfoSection, "dev%u_port_group%u",devId,jj);

            /* get valid port group value */
            sprintf(keyStr,"%s" ,"valid_port_group");
        }

        if (SIM_OS_MAC(simOsGetCnfValue)(devInfoSection, keyStr,
                       FILE_MAX_LINE_LENGTH_CNS, param_str))
        {
            sscanf(param_str, "%u", &deviceObjPtr->coreDevInfoPtr[jj].validPortGroup);

            if(deviceObjPtr->coreDevInfoPtr[jj].validPortGroup == 0)
            {
                /*we not allocate resources for the 'Non-valid' port group */
                continue;
            }
        }
        else
        {
            /* by default the port group is valid */
            deviceObjPtr->coreDevInfoPtr[jj].validPortGroup = 1;
        }


        portGroupDeviceObjPtr = smemDeviceObjMemoryAlloc(NULL,1, sizeof(SKERNEL_DEVICE_OBJECT));

        /* bind the device object to 'father' device object pointer */
        portGroupDeviceObjPtr->portGroupSharedDevObjPtr = deviceObjPtr;
        /* bind the 'port group device' to the array of device info array */
        deviceObjPtr->coreDevInfoPtr[jj].devObjPtr = portGroupDeviceObjPtr;

        /* set the portGroupId for the 'port group device' */
        portGroupDeviceObjPtr->portGroupId = jj;

        skernelInitDevParse(portGroupDeviceObjPtr, devInfoSection,devId);
        if(deviceObjPtr->useCoreTerminology == GT_TRUE)
        {
            simForcePrintf("device[%u] core[%d] devId[%d] ready : %s \n",
                devId,jj,portGroupDeviceObjPtr->deviceId,
                familyNamesArr[portGroupDeviceObjPtr->deviceFamily]);
        }
        else
        {
            simForcePrintf("device[%u] port group[%d] devId[%d] ready : %s \n",
                devId,jj,portGroupDeviceObjPtr->deviceId,
                familyNamesArr[portGroupDeviceObjPtr->deviceFamily]);
        }
        simForcePrintf("allocated [%u] bytes \n",portGroupDeviceObjPtr->totalNumberOfBytesAllocated);
    }

exitCleanly_lbl:

    /*CloseHandle(smemInitPhaseg);*/
    SIM_OS_MAC(simOsSemDelete)(smemInitPhaseSemaphore);
}

/*******************************************************************************
*   skernelInitDevicePart2
*
* DESCRIPTION:
*       Init for SKernel devices with UPlinks
*
* INPUTS:
*       deviceObjPtr - allocated pointer for the device
*       devId - the deviceId in the INI file info
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
static void skernelInitDevicePart2
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  devId
)
{
    /* initialize UPLINK information */
    skernelInitDevUplinkParse(deviceObjPtr,devId);

    /* initialize internal_connection information */
    skernelInitInternalConnectionParse(deviceObjPtr,devId);
}

/*******************************************************************************
*   skernelForceLinkDownOrLinkUpOnAllPorts
*
* DESCRIPTION:
*       force link UP/DOWN on all ports
*
* INPUTS:
*       devObjPtr - pointer to device object.
*       forceLinkUp -  GT_TRUE  - force link UP   on all ports
*                      GT_FALSE - force link DOWN on all ports
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void skernelForceLinkDownOrLinkUpOnAllPorts
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_BOOL                 forceLinkUpOrLinkDown
)
{
    GT_U32  port;           /* port number */
    SMAIN_PORT_SLAN_INFO *  slanInfoPtr; /* SLAN info entry pointer */
    GT_U32                  value = (forceLinkUpOrLinkDown == GT_TRUE) ? 1 : 0;

    if(devObjPtr->devPortLinkUpdateFuncPtr == NULL)
    {
        return;
    }

    /* Force link of all active SLANs down */
    for (port = 0; port < devObjPtr->numSlans; port++)
    {
        /* get SLAN info for port */
        slanInfoPtr = &(devObjPtr->portSlanInfo[port]);
        if (slanInfoPtr->slanName[0x0] != 0x0)
        {
            /* calling snetChtLinkStateNotify */
            devObjPtr->devPortLinkUpdateFuncPtr(devObjPtr, port, value);
        }
    }
}
/*******************************************************************************
*   skernelDeviceAllSlansUnbind
*
* DESCRIPTION:
*       unbind all SLANs of a device
*
* INPUTS:
*       devObjPtr - pointer to device object.
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void skernelDeviceAllSlansUnbind
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32                port;
    SMAIN_PORT_SLAN_INFO *slanInfoPrt;          /* slan port entry pointer */

    /* make sure that all slans will wait for changing the name 'to indicate' unbind */
    SIM_OS_MAC(simOsSemWait)(slanDownSemaphore,SIM_OS_WAIT_FOREVER);

    /* under traffic we get 'exception' in the 'slan code' unless 'detach' before unbind */
    for (port = 0; port < devObjPtr->numSlans; port++)
    {
        /* get slan info */
        slanInfoPrt = &(devObjPtr->portSlanInfo[port]);
        slanInfoPrt->slanName[0] = 0;/* before the unbind , make sure no one try to send packet to this port */
    }

    SIM_OS_MAC(simOsSemSignal)(slanDownSemaphore);

    /* allow all other tasks that send traffic to not be inside the SLAN that going to be unbound */
    SIM_OS_MAC(simOsSleep)(1);

    for (port = 0; port < devObjPtr->numSlans; port++)
    {
        /* get slan info */
        slanInfoPrt = &(devObjPtr->portSlanInfo[port]);

        /* unbind the port */
        internalSlanUnbind(slanInfoPrt);
    }
}


/*******************************************************************************
*   skernelDeviceSoftResetGeneric
*
* DESCRIPTION:
*       do generic soft reset.
*       Function do soft reset on the following order:
*           - force link of all active SLANs down
*           - disable device interrupts
*           - unbind SCIB interface
*           - create new device to replace old one
*             the new device will keep the threads of the old one + queues + pools
*             the create of new device will reopen the SLANs , interrupt line , SCIB bind...
*           - delete old device object (with all it's memories)
*
* INPUTS:
*       oldDeviceObjPtr - pointer to the old device
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       pointer to the new allocated device.
* COMMENTS:
*       called in context of skernel task
*
*       see function smemLion3ActiveWriteDfxServerResetControlReg that triggers
*       soft reset
*******************************************************************************/
SKERNEL_DEVICE_OBJECT * skernelDeviceSoftResetGeneric
(
    IN SKERNEL_DEVICE_OBJECT * oldDeviceObjPtr
)
{
    DECLARE_FUNC_NAME(skernelDeviceSoftResetGeneric);

    SKERNEL_DEVICE_OBJECT *devObjPtr = oldDeviceObjPtr;/*needed ONLY for __LOG macro */

    SKERNEL_DEVICE_OBJECT * newDeviceObjPtr;
    GT_U32  devId = oldDeviceObjPtr->deviceId;

    __LOG(("--'soft reset' -- part 1-- started \n"));
    simulationPrintf("--'soft reset' -- part 1-- started \n");

    __LOG(("Force link DOWN of all active SLANs \n"));

    /* unbind all SLANs */
    skernelDeviceAllSlansUnbind(oldDeviceObjPtr);

    __LOG(("disconnect the old device from the interrupt line because the new device \n"
       "will bind to it (we not want the SHOST to fatal error) \n"));
    /* disconnect the old device from the interrupt line because the new device
       will bind to it (we not want the SHOST to fatal error) */
#ifdef _WIN32 /* cause Linux to suspend 'current thread' !!! --> 'dead lock' */
    SHOSTG_interrupt_disable_one(WA_SHOST_NUMBERING(oldDeviceObjPtr->interruptLine));
#endif /*_WIN32*/
    /* state that the device is in the middle of 'soft reset'
        do init sequence ..  this will
        1. bind interrupt with CSIB
        2. set init done
        3. smemChtInitStateSet will state that the reset done
    */

    __LOG(("allocate new device object \n"));
    /* allocate new device object */
    newDeviceObjPtr = smemDeviceObjMemoryAlloc(NULL,1, sizeof(SKERNEL_DEVICE_OBJECT));

    /* state that the new device is in process of replacing the old device */
    __LOG(("state that the new device is in process of replacing the old device \n"));
    newDeviceObjPtr->softResetOldDevicePtr = oldDeviceObjPtr;

    /* after the new device exists and point to the old device need to set the
       old device to point to the new device */
    oldDeviceObjPtr->softResetNewDevicePtr = newDeviceObjPtr;

    /* the 'new' device is replacing an old device , but it must be with it's
       bufPool,queueId before 'bind' it to the 'global array of devices'
       MUST be done before calling skernelInitDevicePart1(...)
    */
    __LOG(("Bind bufPool,queueId of 'old device' to 'new device' \n"));
    newDeviceObjPtr->bufPool = oldDeviceObjPtr->bufPool;
    newDeviceObjPtr->queueId = oldDeviceObjPtr->queueId;
    /* reuse also the TM queue */
    newDeviceObjPtr->tmInfo.bufPool_DDR3_TM = oldDeviceObjPtr->tmInfo.bufPool_DDR3_TM;

    /* do initialization for the new device .. part1 , part2 */
    __LOG(("do initialization for the new device .. part1 \n"));
    skernelInitDevicePart1(newDeviceObjPtr , devId);
    __LOG(("do initialization for the new device .. part2 \n"));
    skernelInitDevicePart2(newDeviceObjPtr , devId);

    __LOG(("wait for all threads (other then skernel task) to replace old device with new device \n"));
    /* wait for all threads (other then skernel task) to replace old device with new device */
    while(oldDeviceObjPtr->numThreadsOnMe > 1)/* 1 is the skernel task (our current task) */
    {
        SIM_OS_MAC(simOsSleep)(50);
        simulationPrintf("+");
        __LOG(("+"));
    }
    simulationPrintf("\n");
    __LOG(("\n"));

    /* I point to new device instead of old device because I not want to set it
        NULL (that will cause other tasks to continue as usual) and I not want
        to keep pointer to device object that about to be free !!! */
    newDeviceObjPtr->softResetOldDevicePtr = newDeviceObjPtr;

    /* rebind the device */
    scibReBindDevice(newDeviceObjPtr->deviceId);

    /************************************************************************************/
    /* we are ready to delete the old device as no one should point to it at this stage */
    /************************************************************************************/
    devObjPtr = newDeviceObjPtr;/*needed ONLY for __LOG macro */

    /* register the thread on info on DB in the new device */
    SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)(SIM_OS_TASK_PURPOSE_TYPE_PP_PIPE_GENERAL_PURPOSE_E,
        &newDeviceObjPtr->task_skernelCookieInfo.generic);

    /* (for debug only) print amount of memory used by the old device */
    smemDeviceObjMemoryAllAllocationsSum(oldDeviceObjPtr);

    __LOG(("free all the memory that the device used including it's own pointer !!!! \n"));
    /* free all the memory that the device used including it's own pointer !!!! */
    smemDeviceObjMemoryAllAllocationsFree(oldDeviceObjPtr,GT_TRUE);

    __LOG(("the device pointer is no longer in use !!! \n"));
    /* the device pointer is no longer in use !!! */
    oldDeviceObjPtr = NULL;

    __LOG(("The old king is dead long live the new king !!!! \n"));
    /****************************************************/
    /* The old king is dead long live the new king !!!! */
    /****************************************************/

    /* the pool is still 'suspended' (by sbufAllocAndPoolSuspend(...)) */
    /* and newDeviceObjPtr->softResetOldDevicePtr is not NLUL */

    /* function skernelDeviceSoftResetGenericPart2(...)
       will handle the pool and the queue and will set
       newDeviceObjPtr->softResetOldDevicePtr = NULL */

    __LOG(("--'soft reset' -- part 1-- ended \n"));
    simulationPrintf("'soft reset' -- part 1-- ended\n");

    return newDeviceObjPtr;
}

/*******************************************************************************
*   skernelDeviceSoftResetGenericPart2
*
* DESCRIPTION:
*       part 2 of do generic soft reset.
*       resume pool and queue of the device to empty state.
*       and state newDeviceObjPtr->softResetOldDevicePtr = NULL
*
* INPUTS:
*       newDeviceObjPtr - pointer to the new device !!!
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       !!! NULL !!!
* COMMENTS:
*       called in context of skernel task
*
*       this is part 2 of skernelDeviceSoftResetGeneric
*******************************************************************************/
SKERNEL_DEVICE_OBJECT * skernelDeviceSoftResetGenericPart2
(
    IN SKERNEL_DEVICE_OBJECT * newDeviceObjPtr
)
{
    DECLARE_FUNC_NAME(skernelDeviceSoftResetGeneric);

    SKERNEL_DEVICE_OBJECT *devObjPtr = newDeviceObjPtr;/*needed ONLY for __LOG macro */

    __LOG(("--'soft reset' -- part 2-- started \n"));

    /*
       flush any buffers that were allocated but not free in the pool .
       like by the 'TXQ queue disable' that the descriptor is kept with it's
       buffer and not calling sbufFree(...) for it.

       NOTE: the queue may still hold 'old' messages even that the pool was suspended
       because buffers that 'allocated' before we suspended the pool can still
       be after the buffer of 'soft drop' (buffer that we handle at this moment)

       so first empty the the queue and then reset the content of the pool.
    */
    __LOG(("flush the queue of the device \n"));
    squeFlush(newDeviceObjPtr->queueId);
    __LOG(("flush the pool of the device \n"));
    sbufPoolFlush(newDeviceObjPtr->bufPool);

    /* Allow queue of buffers to resume (it was suspended by the 'old device')
        but the queue is delivered to the new device */
    __LOG(("resume the queue of the device \n"));
    squeResume(newDeviceObjPtr->queueId);
    /* Allow buffers pool to resume (it was suspended by the 'old device')
        but the pool is delivered to the new device */
    __LOG(("resume the pool of the device \n"));
    sbufPoolResume(newDeviceObjPtr->bufPool);

    /* (only after we handled the pool,queue)
       now we are ready to state the other tasks to continue with the new device.
       see function skernelSleep(...) that kept them without continue !
    */
    __LOG(("now we are ready to state the other tasks to continue with the new device. \n"));
    newDeviceObjPtr->softResetOldDevicePtr = NULL;

    __LOG(("--'soft reset' -- part 2-- ended \n"));
    simulationPrintf("'soft reset' -- part 2-- ended \n");

    return NULL; /* must be ignored by the caller */
}


/*******************************************************************************
*   skernelInit
*
* DESCRIPTION:
*       Init SKernel library.
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
*
*******************************************************************************/
void skernelInit
(
    void
)
{
    char                    param_str[FILE_MAX_LINE_LENGTH_CNS]; /* string for parameter */
    GT_U32                  param_val; /* parameter value */
    GT_U32                  devId; /* device id loop */
    SKERNEL_DEVICE_OBJECT * deviceObjPtr;  /* pointer to the device object*/
    GT_U32                  jj;
    GT_U32                  timeStart,timeEnd;

    /* state that this task is the 'initialization' for the simulation logger */
    SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)(SIM_OS_TASK_PURPOSE_TYPE_INITIALIZATION_E,NULL);

    timeStart = SIM_OS_MAC(simOsTickGet)();

    /* bind generic callback to handle 'packet done' messages from the TM */
    sRemoteTmBindCallBack(&smainReceivedPacketDoneFromTm);

    /* create semaphore for 'wait' for threads to start process */
    slanDownSemaphore = SIM_OS_MAC(simOsSemCreate)(1,1);
    if (slanDownSemaphore == (GT_SEM)0)
    {
        skernelFatalError(" skernelInit: cannot create semaphore");
        return;
    }

    /* reset user debug structure */
    memset(&skernelUserDebugInfo, 0, sizeof(skernelUserDebugInfo));

    /* Lookup policer performance level */
    if(SIM_OS_MAC(simOsGetCnfValue)(DEBUG_SECTION_CNS, "policer_conformance_level",
                       FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        sscanf(param_str, "%u", &param_val);
        skernelUserDebugInfo.policerConformanceLevel = param_val;
    }

    /* check if general printing allowed */
    if(SIM_OS_MAC(simOsGetCnfValue)(DEBUG_SECTION_CNS, "print_general_allowed",
                       FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        sscanf(param_str, "%u", &param_val);
        skernelUserDebugInfo.printGeneralAllowed = param_val ? GT_TRUE : GT_FALSE;
    }
    else
    {
        /* by default the general printing is NOT allowed */
        skernelUserDebugInfo.printGeneralAllowed = GT_FALSE;
    }

    /* check if general printing allowed */
    if(SIM_OS_MAC(simOsGetCnfValue)(DEBUG_SECTION_CNS, "print_warning_allowed",
                       FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        sscanf(param_str, "%u", &param_val);
        skernelUserDebugInfo.printWarningAllowed = param_val ? GT_TRUE : GT_FALSE;
    }
    else
    {
        /* by default the warning printing is allowed */
        skernelUserDebugInfo.printWarningAllowed = GT_TRUE;
    }

    /* clear device objects table */
    memset (smainDeviceObjects, 0, sizeof (smainDeviceObjects));

    /* reset NIC callback and object */
    smainNicRxCb = NULL;
    smainNicDeviceObjPtr = NULL;

    /* get to see if visual asic should be enabled */
    if(SIM_OS_MAC(simOsGetCnfValue)("visualizer",  "enable",
                             FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        GT_U32 tmp;
        sscanf(param_str, "%u", &tmp);
        smainVisualDisabled = tmp ? 0 : 1;
    }
    /* get disable visual asic */
    else if(SIM_OS_MAC(simOsGetCnfValue)("visualizer",  "disable",
                             FILE_MAX_LINE_LENGTH_CNS, param_str))
    {
        GT_U32 tmp;
        sscanf(param_str, "%u", &tmp);
        smainVisualDisabled = tmp ? 1 : 0;
    }
    else
    {
         /* set visualizer to be disabled by default*/
        smainVisualDisabled = 1;
    }

    lastDeviceIdUsedForCores = smainDevicesNumber;

    /* init all devices */
    for (devId = 0; devId < smainDevicesNumber; devId++)
    {
        /*skip devices that are not on this board section*/
        if(sinitOwnBoardSection.numDevices)
        {
            for(jj=0;jj<sinitOwnBoardSection.numDevices;jj++)
            {
                if(sinitOwnBoardSection.devicesIdArray[jj] == devId)
                {
                    break;
                }
            }

            if(jj == sinitOwnBoardSection.numDevices)
            {
                /* this device is not on current board section */
                continue;
            }
        }

        /* allocate device object */
        deviceObjPtr = smemDeviceObjMemoryAlloc(NULL,1, sizeof(SKERNEL_DEVICE_OBJECT));
        if (deviceObjPtr == NULL)
        {
            skernelFatalError(" skernelInit: cannot allocate device %u", devId);
        }

        /* do part 1 init */
        skernelInitDevicePart1(deviceObjPtr,devId);
    }/* portGroupId */

    /* NOTE: this code need to run only after all the 'device objects' already
            exists */
    /* initialize UPLINK information */
    /* initialize internal_connection information */
    for (devId = 0; devId < smainDevicesNumber; devId++)
    {
        if (!smainDeviceObjects[devId])
           continue;

       deviceObjPtr = smainDeviceObjects[devId]   ;

        /* do part 2 init */
        skernelInitDevicePart2(deviceObjPtr,devId);
    }/* devId loop */

    /* send to the application side the minimal info needed about the devices */
    if(sasicgSimulationRole != SASICG_SIMULATION_ROLE_NON_DISTRIBUTED_E)
    {
        skernelInitDistributedDevParse();
    }

    timeEnd = SIM_OS_MAC(simOsTickGet)();

    simulationPrintf("skernelInit : start tick[%d] , end tick[%d] --> total ticks[%d] \n",
        timeStart,timeEnd,timeEnd-timeStart);

    return;
}

/*******************************************************************************
*   skernelShutDown
*
* DESCRIPTION:
*       Shut down SKernel library before reboot.
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
*            The function unbinds ports from SLANs
*
*******************************************************************************/
void skernelShutDown
(
    void
)
{
    GT_U32                                    device; /* device index */
    GT_U32                                      port;        /* port index */
    SMAIN_PORT_SLAN_INFO * slanInfoPrt;          /* slan port entry pointer */

    skernelUnderShutDown = GT_TRUE;

    for (device = 0; device < SMAIN_MAX_NUM_OF_DEVICES_CNS; device++)
    {
        if (!smainDeviceObjects[device])
           continue;

        if (!smainDeviceObjects[device]->portSlanInfo)
        {
            /* early stage on initialization ... not allocated yet */
            continue;
        }

        {
            /* when 'killing' simulation under traffic we get 'exception' in the 'slan code' */

            /* make sure that all slans will wait for changing the name 'to indicate' unbind */
            SIM_OS_MAC(simOsSemWait)(slanDownSemaphore,SIM_OS_WAIT_FOREVER);
            for (port = 0; port < smainDeviceObjects[device]->numSlans; port++)
            {
                /* get slan info */
                slanInfoPrt = &(smainDeviceObjects[device]->portSlanInfo[port]);
                slanInfoPrt->slanName[0] = 0;/* before the unbind , make sure no one try to send packet to this port */
            }
            SIM_OS_MAC(simOsSemSignal)(slanDownSemaphore);

            /* allow all other tasks that send traffic to not be inside the SLAN that going to be unbound */
            SIM_OS_MAC(simOsSleep)(1);
        }

        for (port = 0; port < smainDeviceObjects[device]->numSlans; port++)
        {
            /* get slan info */
            slanInfoPrt = &(smainDeviceObjects[device]->portSlanInfo[port]);

            /* unbind the port */
            internalSlanUnbind(slanInfoPrt);
        }
    }

    simLogClose();
}

/*******************************************************************************
*   smainFrame2InternalConnectionSend
*
* DESCRIPTION:
*       Send frame to a correspondent internal connection of a port.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       portNum     - number of port to send frame.
*       dataPrt     - pointer to start of frame.
*       dataSize    - number of bytes to be send.
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
static void smainFrame2InternalConnectionSend
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  portNum,
    IN GT_U8    *              dataPrt,
    IN GT_U32                  dataSize
)
{
    SKERNEL_DEVICE_OBJECT           *destDeviceObjPtr;/* pointer to object of the destination device */
    SBUF_BUF_ID                     bufferId; /* buffer id */
    SBUF_BUF_STC                    *dstBuf_PTR; /* pointer to the allocated buffer from the destination device pool*/
    SKERNEL_INTERNAL_CONNECTION_INFO_STC    *peerInfoPtr;/* info about the peer (destination) device */
    static GT_BOOL                  slanCpuAlreadyDone = GT_FALSE;

    peerInfoPtr = &deviceObjPtr->portsArr[portNum].peerInfo;

    destDeviceObjPtr = smemTestDeviceIdToDevPtrConvert(peerInfoPtr->peerDeviceId);

    if(destDeviceObjPtr->deviceType == SKERNEL_NIC)
    {
        /* call the NIC from this context */
        SMAIN_PORT_SLAN_INFO *slanInfoPrt;          /* slan port entry pointer */
        /* we keep using the 'unique' SLAN for the CPU because the MTS need to
           get packets from 'registered' thread. but we don't want to register
           the 'Asic simulation' tasks */

        /* get SLAN info for port */
        slanInfoPrt = &(destDeviceObjPtr->portSlanInfo[peerInfoPtr->peerPortNum]);

        if(slanInfoPrt->slanIdTx == NULL && slanCpuAlreadyDone == GT_FALSE)
        {
            slanCpuAlreadyDone = GT_TRUE;

            sprintf(slanInfoPrt->slanName,"%s","slanCpu");

            /* bind port - TX direction with SLAN */
            internalSlanBind(slanInfoPrt->slanName,deviceObjPtr,peerInfoPtr->peerPortNum,GT_FALSE,GT_TRUE,slanInfoPrt,
                smainNicSlanPacketRcv___dummy);

            /* bind port - RX direction with SLAN */
            internalSlanBind(slanInfoPrt->slanName,deviceObjPtr,peerInfoPtr->peerPortNum,GT_TRUE,GT_FALSE,slanInfoPrt,
                smainNicSlanPacketRcv);
        }

        /* the slanIdTx is used only to allow the DX device to send packet to the slan
           and then the slanIdRx is the one that gets the packet.
           (for the 'from NIC to the device' we use 'peer info')
        */

        if (slanInfoPrt->slanName[0x0] != 0x0)/* support unbind of slan under traffic */
        {
            SIM_OS_MAC(simOsSlanTransmit)(slanInfoPrt->slanIdTx,SIM_OS_SLAN_MSG_CODE_FRAME_CNS,
                              dataSize,  (char*)dataPrt);
        }

        /* so the SLAN task will call smainNicSlanPacketRcv(...) */
        return;
    }


    /* allocate buffer from the 'destination' device pool */
    /* get the buffer and put it in the queue */
    bufferId = sbufAlloc(destDeviceObjPtr->bufPool, dataSize);
    if (bufferId == NULL)
    {
        simWarningPrintf(" smainFrame2InternalConnectionSend : no buffers for process \n");
        return ;
    }

    skernelNumOfPacketsInTheSystemSet(GT_TRUE);

    dstBuf_PTR = (SBUF_BUF_STC *) bufferId;
    /* use the SLAN type since this is generic packet */
    dstBuf_PTR->srcType = SMAIN_SRC_TYPE_INTERNAL_CONNECTION_E;
    /* set source port of buffer -- the port on the destination device */
    dstBuf_PTR->srcData = peerInfoPtr->peerPortNum;
    /* state 'regular' frame */
    dstBuf_PTR->dataType = SMAIN_MSG_TYPE_FRAME_E;

    /* copy the frame into the buffer */
    memcpy(dstBuf_PTR->actualDataPtr,dataPrt,dataSize);

    /* set the pointers and buffer */
    sbufDataSet(bufferId, dstBuf_PTR->actualDataPtr,dataSize);

    /* put buffer on the queue of the destination device */
    squeBufPut(destDeviceObjPtr->queueId ,SIM_CAST_BUFF(bufferId));

    return ;
}

/*******************************************************************************
*   smainFrame2PortSend
*
* DESCRIPTION:
*       Send frame to a correspondent SLAN of a port.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       portNum     - number of port to send frame.
*       dataPrt     - pointer to start of frame.
*       dataSize    - number of bytes to be send.
*       doLoopback  - loopback on portNum for the cheetah device.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*        if doLoopback is enabled , then the packet is sent to the portNum
*
*******************************************************************************/
void smainFrame2PortSend
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  portNum,
    IN GT_U8    *              dataPrt,
    IN GT_U32                  dataSize  ,
    IN GT_BOOL                 doLoopback
)
{
    smainFrame2PortSendWithSrcPortGroup(deviceObjPtr,portNum,dataPrt,
            dataSize,doLoopback,
            deviceObjPtr->portGroupId);
}

/*******************************************************************************
*   smainFrame2PortSendWithSrcPortGroup
*
* DESCRIPTION:
*       Send frame to a correspondent SLAN of a port. (from context of src port group)
*
* INPUTS:
*       devObjPtr      - pointer to device object.
*       portNum        - number of port to send frame.
*       dataPrt        - pointer to start of frame.
*       dataSize       - number of bytes to be send.
*       doLoopback     - loopback on portNum for the cheetah device.
*       srcPortGroup   - the src port group that in it's context we do packet send
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*        if doLoopback is enabled , then the packet is sent to the portNum
*
*******************************************************************************/
void smainFrame2PortSendWithSrcPortGroup
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32                  portNum,
    IN GT_U8    *              dataPrt,
    IN GT_U32                  dataSize  ,
    IN GT_BOOL                 doLoopback,
    IN GT_U32                  srcPortGroup
)
{
    DECLARE_FUNC_NAME(smainFrame2PortSendWithSrcPortGroup);

    SMAIN_PORT_SLAN_INFO *  slanInfoPrt; /* slan info entry pointer */
    SBUF_BUF_ID bufferId; /* buffer id */
    GT_U8*  tmpPtr;/* tmp pointer in new buffer */
    GT_U32  tmpLength; /* tmp length in new buffer */
    SKERNEL_PORT_INFO_STC   *portInfoPtr;
    GT_U32  retryCount = 50;/* retry up to 500 milliseconds */
    SKERNEL_DEVICE_OBJECT * devObjPtr = deviceObjPtr;/*devObjPtr -- needed by __LOG(()) */

    simLogPacketFrameUnitSet(SIM_LOG_FRAME_UNIT___ALLOW_ALL_UNITS___E);/* wild card for LOG without the unit filter */

    /* check parameters */
    if (portNum >= deviceObjPtr->numSlans)
    {
        skernelFatalError("smainFrame2PortSendWithSrcPortGroup: wrong port %d \n", portNum);
    }

    /* remove 4 FCS (frame check sum or CRC) bytes frame to SLAN */
    if (deviceObjPtr->crcPortsBytesAdd == 1)
    {
        dataSize -=4;

        __LOG(("Egress port [%d] : remove 4 FCS (frame check sum or CRC) bytes frame to SLAN \n",
            portNum));
    }

    /* get SLAN info for port */
    slanInfoPrt = &(deviceObjPtr->portSlanInfo[portNum]);

    /* transmit to SLAN  */
    if (slanInfoPrt->slanName[0x0] != 0x0)/* support unbind of slan under traffic */
    {
        __LOG(("Egress port [%d] : send to SLAN[%s] \n",
            portNum,slanInfoPrt->slanName));
        SIM_OS_MAC(simOsSlanTransmit)(slanInfoPrt->slanIdTx,SIM_OS_SLAN_MSG_CODE_FRAME_CNS,
                          dataSize,  (char*)dataPrt);
    }
    else if(deviceObjPtr->portsArr[portNum].peerInfo.usePeerInfo == GT_TRUE)
    {
        __LOG(("Egress port [%d] : send to internal connection \n",
            portNum));
        smainFrame2InternalConnectionSend(deviceObjPtr,portNum,dataPrt,dataSize);
    }

    /*
        allow to do loopback regardless to the sending to the SLAN/other connection.
        (this is actual behavior of the device)
    */

    portInfoPtr =  &deviceObjPtr->portsArr[portNum];

    /* loopback on portnum*/
    if (doLoopback == GT_TRUE || portInfoPtr->loopbackForceMode == SKERNEL_PORT_LOOPBACK_FORCE_ENABLE_E)
    {
        __LOG(("Do port loopback : Send the packet to this port [%d] queue \n",
            portNum));

        if(supportMacLoopbackInContextOfSender != 0 &&
           deviceObjPtr->gmDeviceType != SMAIN_NO_GM_DEVICE)
        {
            GT_BIT loopedInCurrentContext = 1;
            static GT_U32  packetStackDepth = 0;
            static GT_U32  lastTryEgressPort = 0;/* the last port that packet try to egress from */
            static GT_U32  lastTryEgressCore = 0;/* the last core that packet try egress from */
            static GT_U32  resendCounter = 0;
            GT_BIT resendCounterChanged = 0;
            GT_U32  pipeId = deviceObjPtr->portGroupId;
            GT_U32  localPort = portNum;

            packetStackDepth++;
            switch (deviceObjPtr->deviceFamily)
            {
                case SKERNEL_PUMA3_NETWORK_FABRIC_FAMILY:
                    if(portNum < 0x20 /*network port*/)
                    {
                        /* do not do it on fabric ports to avoid stack overflow due to recursive
                           loops*/

                        /* implement 'internal loopback' for the GM. don't sent the packet to other task */
                        GT_U32  srcPort = portNum;
                        pipeId = (srcPort / 16) + deviceObjPtr->portGroupId;
                        localPort = (srcPort % 16);

                        /* the Puma3 device has 4 pipes that handle packets but there
                           are only 2 MG units . so we have only devObjPtr->portGroupId = 0 and 1 */
                        /* but still we need to notify the GM that packet came from
                            pipe 0 or pipe 1 or pipe 2 or pipe 3 with local port 0..11 */
                        /* multi core device */
                    }
                    else
                    {
                        loopedInCurrentContext = 0;
                    }
                    break;
                default:
                    /* No need pipeId and localPort conversion for non Puma3 devices  */
                    break;
            }

            if(loopedInCurrentContext)
            {
                if(packetStackDepth > 1)
                {
                    if(lastTryEgressCore == deviceObjPtr->portGroupId &&
                       lastTryEgressPort == portNum)
                    {
                        /* we are before sending packet to port that is loopback , that already got traffic from itself */
                        resendCounter++;
                        resendCounterChanged = 1;
                    }

                    if(resendCounter > 2)
                    {
                        skernelFatalError("smainFrame2PortSendWithSrcPortGroup : Simulation detect that egress port[%d] in core[%d] is mac-loopback"
                                          " and send traffic to itself (it may cause 'stack overflow')\n",
                            localPort,pipeId);
                    }
                }

                /* save the values that we try to send to */
                lastTryEgressCore = deviceObjPtr->portGroupId;
                lastTryEgressPort = portNum;

                ppSendPacket(SMEM_GM_GET_GM_DEVICE_ID(deviceObjPtr), pipeId , localPort, (char*)dataPrt, dataSize);

                if(resendCounterChanged)
                {

                    if(resendCounter == 0)
                    {
                        skernelFatalError("smainFrame2PortSendWithSrcPortGroup: bad management of resendCounter \n");
                    }
                    resendCounter--;
                }
            }

            if(packetStackDepth == 0)
            {
                skernelFatalError("smainFrame2PortSendWithSrcPortGroup: bad management of packetStackDepth \n");
            }
            packetStackDepth--;

            if(loopedInCurrentContext)
            {
                return;
            }
        }

        if(dataSize < SGT_MIN_FRAME_LEN)
        {
            dataSize = SGT_MIN_FRAME_LEN;
        }

        dataSize += deviceObjPtr->prependNumBytes;
retryAlloc_lbl:
        /* get the buffer and put it in the queue */
        bufferId = sbufAllocWithProtectedAmount(deviceObjPtr->bufPool,dataSize,LOOPBACK_MIN_FREE_BUFFERS_CNS);

        if (bufferId == NULL)
        {
            if(deviceObjPtr->needToDoSoftReset)
            {
                retryCount = 0;
            }

            if(retryCount-- &&
               deviceObjPtr->portGroupSharedDevObjPtr &&
               srcPortGroup != deviceObjPtr->portGroupId)
            {
                /* do retry only for multi-core device because on single core device
                  the context of this task is the only one that can free those buffers
                  so no meaning to wait for it

                  in multi core the buffer that ask for can be from different core ,
                  so the skernel task of this core may free buffers.
                  */
                SIM_OS_MAC(simOsMutexUnlock)(deviceObjPtr->portGroupSharedDevObjPtr->protectEgressMutex);
                SIM_OS_MAC(simOsMutexUnlock)(deviceObjPtr->portGroupSharedDevObjPtr->fullPacketWalkThroughProtectMutex);

                SIM_OS_MAC(simOsSleep)(10);

                SIM_OS_MAC(simOsMutexLock)(deviceObjPtr->portGroupSharedDevObjPtr->fullPacketWalkThroughProtectMutex);
                SIM_OS_MAC(simOsMutexLock)(deviceObjPtr->portGroupSharedDevObjPtr->protectEgressMutex);
                goto retryAlloc_lbl;
            }

            __LOG(("Do port loopback : ERROR : Failed to Send the packet to this port [%d] queue \n",
                portNum));

            {
                static GT_U32 loopback_dropped_counter = 0;
                /* limit the printings */
                if(loopback_dropped_counter < 50 ||
                   (0 == (loopback_dropped_counter & 0x7ff)))
                {
                    simWarningPrintf(" smainFrame2PortSendWithSrcPortGroup : Do port loopback : ERROR : Failed to Send the packet to this port [%d] queue  \n",
                        portNum);
                }

                loopback_dropped_counter++;
            }

            return ;
        }

        skernelNumOfPacketsInTheSystemSet(GT_TRUE);

        bufferId->srcType = SMAIN_SRC_TYPE_LOOPBACK_PORT_E;
        bufferId->srcData = portNum;
        bufferId->dataType = SMAIN_MSG_TYPE_FRAME_E;

        /* get actual data pointer */
        sbufDataGet(bufferId, &tmpPtr, &tmpLength);

        tmpLength -= deviceObjPtr->prependNumBytes;
        tmpPtr    += deviceObjPtr->prependNumBytes;

        /* copy data to the new buffer , in the needed offset */
        memcpy(tmpPtr, dataPrt , tmpLength);

        /* update the info about the buffer */
        sbufDataSet(bufferId, tmpPtr, tmpLength);

        /* put buffer on the queue */
        squeBufPut(deviceObjPtr->queueId ,SIM_CAST_BUFF(bufferId));
    }

}



/* function should be called only on he asic side of the distributed architecture */
static GT_STATUS smainNicRxCb_distributedAsic (
    IN GT_U8_PTR   segmentList[],
    IN GT_U32      segmentLen[],
    IN GT_U32      numOfSegments
)
{
    GT_U32                  frameLen = 0;   /* frame's length */
    GT_U8                   frameBuffer[SBUF_DATA_SIZE_CNS];
    GT_U32                  ii;

    if(numOfSegments == 1)
    {
        frameLen = segmentLen[0];
    }
    else
    {
        for (ii = 0; ii < numOfSegments; ii++)
        {
            /* check current frame length */
            if ((frameLen + segmentLen[ii]) > SBUF_DATA_SIZE_CNS)
                return GT_FAIL;

            /* copy segment to the buffer */
            memcpy(frameBuffer + frameLen,
                   segmentList[ii],
                   segmentLen[ii]);

            frameLen += segmentLen[ii];
        }
    }


    simDistributedNicRxFrame(frameLen,frameBuffer);

    return GT_OK;
}

/*******************************************************************************
*   smainNicRxHandler
*
* DESCRIPTION:
*       Rx NIC callback function
*
* INPUTS:
*       segmentList   - list of buffers
*       segmentLen    - list of buffer length
*       numOfSegments - number of buffers
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       none
*
* COMMENTS:
*
*******************************************************************************/
static void smainNicRxHandler
(
    IN GT_U8_PTR   segmentPtr,
    IN GT_U32      segmentLen
)
{
    if(smainNicRxCb==NULL)
    {
        return;
    }

    smainNicRxCb(&segmentPtr,&segmentLen,1);
}

/*******************************************************************************
*   skernelNicRxBind
*
* DESCRIPTION:
*       Bind Rx callback routine with skernel.
*
* INPUTS:
*       rxCbFun - callback function for NIC Rx.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void skernelNicRxBind
(
    SKERNEL_NIC_RX_CB_FUN rxCbFun
)
{
    if(sasicgSimulationRole == SASICG_SIMULATION_ROLE_NON_DISTRIBUTED_E)
    {
        smainNicRxCb = rxCbFun;
    }
    else
    {
        /* on the Asic side we send the frame to the other side */
        smainNicRxCb = smainNicRxCb_distributedAsic;
    }
}

/*******************************************************************************
*   skernelNicRxUnBind
*
* DESCRIPTION:
*       UnBind Rx callback routine with skernel.
*
* INPUTS:
*       rxCbFun - callback function for unbind NIC Rx.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void skernelNicRxUnBind
(
    SKERNEL_NIC_RX_CB_FUN rxCbFun
)
{
    smainNicRxCb = NULL;
}

/*******************************************************************************
* skernelNicOutput
*
* DESCRIPTION:
*       This function transmits an Ethernet packet to the NIC
*
* INPUTS:
*       segmentList   - segment list
*       segmentLen    - segment length
*       numOfSegments - number of segments
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK if successful, or
*       GT_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS skernelNicOutput
(
    IN GT_U8_PTR        segmentList[],
    IN GT_U32           segmentLen[],
    IN GT_U32           numOfSegments
)
{
    SMAIN_PORT_SLAN_INFO *  slanInfoPrt;    /* slan info entry pointer */
    GT_U32                  frameLen = 0;   /* frame's length */
    GT_U32                  i;

    for (i = 0; i < numOfSegments; i++)
    {
        /* check current frame length */
        if ((frameLen + segmentLen[i]) > SBUF_DATA_SIZE_CNS)
            return GT_FAIL;

        if (!smainNicDeviceObjPtr)
            return GT_FAIL;

        /* copy segment to the buffer */
        memcpy(smainNicDeviceObjPtr->egressBuffer1 + frameLen,
               segmentList[i],
               segmentLen[i]);

        frameLen += segmentLen[i];
    }

    if(sasicgSimulationRole == SASICG_SIMULATION_ROLE_DISTRIBUTED_APPLICATION_SIDE_E)
    {
        /* only in the application side of  distributed architecture we need
           to send the info to the other side (Asic side) */

        simDistributedNicTxFrame(frameLen,smainNicDeviceObjPtr->egressBuffer1);
        return GT_OK;
    }

    /* add 4 CRC bytes if need */
    if (smainNicDeviceObjPtr->crcPortsBytesAdd != 0)
    {
        frameLen += 4;
    }

    /* get SLAN info for port */
    slanInfoPrt = &(smainNicDeviceObjPtr->portSlanInfo[0]);

    /* for NIC MUST check 'internal connection' before check for SLAN
       because we may have both initialized !

       but in this case the slanIdTx is used only to allow the DX device to send packet to the slan
       and then the slanIdRx is the one that gets the packet.

       So for the 'from NIC to the device' we use 'peer info' .
    */
    if(smainNicDeviceObjPtr->portsArr[0].peerInfo.usePeerInfo == GT_TRUE)
    {
        smainFrame2InternalConnectionSend(smainNicDeviceObjPtr,0/*port num*/,smainNicDeviceObjPtr->egressBuffer1,frameLen);
    }
    else
    if (slanInfoPrt->slanName[0x0] != 0x0)/* support unbind of slan under traffic */
    {
        /* transmit to SLAN */
        SIM_OS_MAC(simOsSlanTransmit)(slanInfoPrt->slanIdTx, SIM_OS_SLAN_MSG_CODE_FRAME_CNS,
                          frameLen, (char*)(smainNicDeviceObjPtr->egressBuffer1));
    }
    else if(skernelUnderShutDown == GT_FALSE) /*do not generate this error when we shutdown*/
    {
        skernelFatalError(" skernelNicOutput: NIC not bound to sent packets (slan/internal connection)");
    }

    return GT_OK;
}

/*******************************************************************************
* smainIsVisualDisabled
*
* DESCRIPTION:
*        Returns the status of the visual asic.
* INPUTS:
*        None

* OUTPUTS:
*        None
*
* RETURNS:
*       0   - Enabled
*       1   - Disabled
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 smainIsVisualDisabled
(
    void
)
{
    return smainVisualDisabled;
}

/*******************************************************************************
* skernelPolicerConformanceLevelForce
*
* DESCRIPTION:
*    force the conformance level for the packets entering the policer
*       (traffic cond)
* INPUTS:
*       dp -  conformance level (drop precedence) - green/yellow/red

* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK - success, GT_FAIL otherwise
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS skernelPolicerConformanceLevelForce(
    IN  GT_U32      dp
)
{
    if(dp == SKERNEL_CONFORM_GREEN ||
       dp == SKERNEL_CONFORM_YELLOW ||
       dp == SKERNEL_CONFORM_RED)
    {
        skernelUserDebugInfo.policerConformanceLevel = dp;

        return GT_OK;
    }
    return GT_FAIL;
}

/*******************************************************************************
* skernelPolicerConformanceLevelForce
*
* DESCRIPTION:
*    force the conformance level for the packets entering the policer
*       (traffic cond)
* INPUTS:
*       timeUnit - the type of unit for the time :
*            0 - SKERNEL_TIME_UNITS_NOT_VALID_E,
*            1 - SKERNEL_TIME_UNITS_MILI_SECONDS_E, 10e-3
*            2 - SKERNEL_TIME_UNITS_MICRO_SECONDS_E,10e-6
*            3 - SKERNEL_TIME_UNITS_NANO_SECONDS_E, 10e-9
*            4 - SKERNEL_TIME_UNITS_PICO_SECONDS_E  10e-12
*       timeValue_low  - the time in units of timeUnit
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK - success, GT_FAIL otherwise
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS skernelPolicerInterFramGapInfoSet(
    GT_U32  timeUnit,
    GT_U32  timeValue
)
{
    SKERNEL_TIME_UNITS_ENT  skernelTimeUnit = timeUnit;

    SCIB_SEM_TAKE;
    __LOG_PARAM_NO_LOCATION_META_DATA(timeUnit);
    __LOG_PARAM_NO_LOCATION_META_DATA(timeValue);
    SCIB_SEM_SIGNAL;

    if(skernelTimeUnit > SKERNEL_TIME_UNITS_PICO_SECONDS_E)
    {
        return GT_FAIL;
    }

    skernelUserDebugInfo.enhancedPolicerIfgTime.timeUnit           = skernelTimeUnit;
    skernelUserDebugInfo.enhancedPolicerIfgTime.timeBetweenPackets = timeValue;

    return GT_OK;
}

/*******************************************************************************
*   smainInterruptsMaskChanged
*
* DESCRIPTION:
*       handle interrupt mask registers
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       causeRegAddr   - cause register address
*       maskRegAddr    - mask  register address
*       intRegBit      - interrupt bit in the global cause register
*       currentCauseRegVal - current cause register values
*       lastMaskRegVal - last mask register values
*       newMaskRegVal  - new mask register values
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static void smainInterruptsMaskChanged(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32  causeRegAddr,
    IN GT_U32  maskRegAddr,
    IN GT_U32  intRegBit,
    IN GT_U32  currentCauseRegVal,
    IN GT_U32  lastMaskRegVal,
    IN GT_U32  newMaskRegVal
)
{
    if(SKERNEL_DEVICE_FAMILY_CHEETAH(devObjPtr))
    {
        snetCheetahInterruptsMaskChanged(devObjPtr,causeRegAddr,maskRegAddr,
                intRegBit,currentCauseRegVal,lastMaskRegVal,newMaskRegVal);
    }
    else if(SKERNEL_DEVICE_FAMILY_TWISTD(devObjPtr->deviceType) ||
            SKERNEL_DEVICE_FAMILY_TIGER(devObjPtr->deviceType))
    {
        snetTigerInterruptsMaskChanged(devObjPtr,causeRegAddr,maskRegAddr,
                intRegBit,currentCauseRegVal,lastMaskRegVal,newMaskRegVal);
    }

    return;
}

/*******************************************************************************
*   smainUpdateFrameFcs
*
* DESCRIPTION:
*       Update FCS value of ethernet frame
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*        frameData  - pointer to the data
*        frameSize  - data size
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
void smainUpdateFrameFcs(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8 * frameData,
    IN GT_U32  frameSize
)
{
    DECLARE_FUNC_NAME(smainUpdateFrameFcs);

    GT_U32 crc;

    /* Calculate FCS enable */
    if (devObjPtr->calcFcsEnable)
    {
        /* calculate the CRC from the packet */
        crc = simCalcCrc32(0xFFFFFFFF, frameData, frameSize - 4);

        /* Set the calculated CRC */
        frameData[frameSize-4] = (GT_U8)(crc >> 24);
        frameData[frameSize-3] = (GT_U8)(crc >> 16);
        frameData[frameSize-2] = (GT_U8)(crc >> 8);
        frameData[frameSize-1] = (GT_U8)(crc);

        /* save info to log */
        __LOG(("Calculate CRC: %X %X %X %X\n",
                                                     frameData[frameSize-4],
                                                     frameData[frameSize-3],
                                                     frameData[frameSize-2],
                                                     frameData[frameSize-1]));
    }
}

/*******************************************************************************
*   skernelIsCheetah3OnlyXgDev
*
* DESCRIPTION:
*       check if device cheetah 3 XG (all ports are XG) -- not supports future
*               devices.
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not ch3 device
*       1 - ch3 device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsCheetah3OnlyXgDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    switch(devObjPtr->deviceType)
    {
        case SKERNEL_98DX8110   :
        case SKERNEL_98DX8108   :
        case SKERNEL_98DX8109   :
        case SKERNEL_98DX8110_1 :
        case SKERNEL_DXCH3_XG   :
            return 1;
        default:
            break;
    }

    return 0;
}

/*******************************************************************************
*   skernelIsCheetah3OnlyDev
*
* DESCRIPTION:
*       check if device should behave like cheetah 3 (or ch3+) and not xcat..
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not ch3 (or ch3+) device
*       1 - ch3 device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsCheetah3OnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    switch(devObjPtr->deviceType)
    {
        case SKERNEL_98DX806    :
        case SKERNEL_98DX5128   :
        case SKERNEL_98DX5128_1 :
        case SKERNEL_98DX5124   :
        case SKERNEL_98DX5126   :
        case SKERNEL_98DX5127   :
        case SKERNEL_98DX5129   :
        case SKERNEL_98DX5151   :
        case SKERNEL_98DX5152   :
        case SKERNEL_98DX5154   :
        case SKERNEL_98DX5155   :
        case SKERNEL_98DX5156   :
        case SKERNEL_98DX5157   :
        case SKERNEL_DXCH3      :
            return 1;
        default:
            break;
    }

    /* ch3 only XG devices */
    if(skernelIsCheetah3OnlyXgDev(devObjPtr))
    {
        return 1;
    }

    if(SKERNEL_IS_CHEETAH3P_ONLY_DEV(devObjPtr))  /* ch3p */
    {
        return 1;
    }

    return 0;
}


/*******************************************************************************
*   skernelIsCheetah3Dev
*
* DESCRIPTION:
*       check if device should behave like cheetah 3
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not ch3 device
*       1 - ch3 device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsCheetah3Dev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    return (skernelIsCheetah3OnlyDev(devObjPtr) ||
            skernelIsXcatDev(devObjPtr));
}

/*******************************************************************************
*   skernelIsXcatOnlyDev
*
* DESCRIPTION:
*       check if device should behave like xCat and not others..
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not xcat device
*       1 - xcat device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsXcatOnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    switch(devObjPtr->deviceType)
    {
        case    SKERNEL_98DX1101:
        case    SKERNEL_98DX1111:
        case    SKERNEL_98DX1122:
        case    SKERNEL_98DX1123:
        case    SKERNEL_98DX1142:

        case    SKERNEL_98DX2101:
        case    SKERNEL_98DX2112:
        case    SKERNEL_98DX2122:
        case    SKERNEL_98DX2123:
        case    SKERNEL_98DX2142:
        case    SKERNEL_98DX2151:
        case    SKERNEL_98DX2161:

        case    SKERNEL_98DX3001:
        case    SKERNEL_98DX3010:
        case    SKERNEL_98DX3011:
        case    SKERNEL_98DX3020:
        case    SKERNEL_98DX3021:
        case    SKERNEL_98DX3022:

        case    SKERNEL_98DX3101:
        case    SKERNEL_98DX3110:
        case    SKERNEL_98DX3111:
        case    SKERNEL_98DX3120:
        case    SKERNEL_98DX3121:
        case    SKERNEL_98DX3122:
        case    SKERNEL_98DX3123:
        case    SKERNEL_98DX3124:
        case    SKERNEL_98DX3125:
        case    SKERNEL_98DX3141:
        case    SKERNEL_98DX3142:

        case    SKERNEL_98DX4101:
        case    SKERNEL_98DX4102:
        case    SKERNEL_98DX4103:
        case    SKERNEL_98DX4110:
        case    SKERNEL_98DX4120:
        case    SKERNEL_98DX4121:
        case    SKERNEL_98DX4122:
        case    SKERNEL_98DX4123:
        case    SKERNEL_98DX4140:
        case    SKERNEL_98DX4141:
        case    SKERNEL_98DX4142:
            return 1;

        case SKERNEL_XCAT_24_AND_4:
            return 1;

        case SKERNEL_XCAT3_24_AND_6:
            return 1;

        default:
            break;
    }

    return 0;
}

/*******************************************************************************
*   skernelIsXcat2OnlyDev
*
* DESCRIPTION:
*       Check if device should behave like xCat2 and not others.
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not xCat2 device
*       1 - xCat2 device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsXcat2OnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    switch(devObjPtr->deviceType)
    {
        case SKERNEL_XCAT2_24_AND_4:
            return 1;

        default:
            break;
    }

    return 0;
}

/*******************************************************************************
*   skernelIsXcat3OnlyDev
*
* DESCRIPTION:
*       Check if device should behave like xCat3 and not others.
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not xCat3 device
*       1 - xCat3 device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsXcat3OnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    switch(devObjPtr->deviceType)
    {
        case SKERNEL_XCAT3_24_AND_6:
            return 1;

        default:
            break;
    }

    return 0;
}

/*******************************************************************************
*   skernelIsLionPortGroupOnlyDev
*
* DESCRIPTION:
*       check if device should behave like Lion's port group
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not Lion's port group device
*       1 - Lion's port group device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsLionPortGroupOnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    switch(devObjPtr->deviceType)
    {
        case SKERNEL_LION_PORT_GROUP_12:
            return 1;

        default:
            break;
    }

    if(skernelIsLion2PortGroupOnlyDev(devObjPtr))
    {
        return 1;
    }

    return 0;
}

/*******************************************************************************
*   skernelIsLionShellOnlyDev
*
* DESCRIPTION:
*       check if device should behave like Lion shell
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not Lion shell device
*       1 - Lion shell device
*
* COMMENTS:
*
*******************************************************************************/
static GT_U32 skernelIsLionShellOnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    switch(devObjPtr->deviceType)
    {
        case SKERNEL_LION_48:
        case SKERNEL_LION2_96:
        case SKERNEL_LION3_96:
            return 1;

        default:
            break;
    }

    return 0;
}
/*******************************************************************************
*   skernelIsPumaShellOnlyDev
*
* DESCRIPTION:
*       check if device should behave like Puma shell
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not Puma shell device
*       1 - Puma shell device
*
* COMMENTS:
*
*******************************************************************************/
static GT_U32 skernelIsPumaShellOnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    switch(devObjPtr->deviceType)
    {
        case SKERNEL_PUMA3_64:
            return 1;

        default:
            break;
    }

    return 0;
}

/*******************************************************************************
*   skernelIsLionPortGroupDev
*
* DESCRIPTION:
*       check if device should behave like Lion shell
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not Lion port group device
*       1 - Lion port group device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsLionPortGroupDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    if (skernelIsLionPortGroupOnlyDev(devObjPtr))
    {
        return 1;
    }

    if(skernelIsXcat2Dev(devObjPtr))
    {
        return 1;
    }

    return 0;
}

/*******************************************************************************
*   skernelIsCheetah2OnlyDev
*
* DESCRIPTION:
*       check if device should behave like cheetah 2  and not ch,ch3..
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not ch2 device
*       1 - ch2 device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsCheetah2OnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{

    switch(devObjPtr->deviceType)
    {
        case SKERNEL_98DX255:
        case SKERNEL_98DX265:
        case SKERNEL_98DX275:
        case SKERNEL_98DX285:
        case SKERNEL_98DX804:
        case SKERNEL_98DX125:
        case SKERNEL_98DX145:
        case SKERNEL_DXCH2:
            return 1;
        default:
            break;
    }

    return 0;
}


/*******************************************************************************
*   skernelIsCheetahDev
*
* DESCRIPTION:
*       check if device should behave like cheetah
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not ch device
*       1 - ch device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsCheetahDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    return (skernelIsCheetah1OnlyDev(devObjPtr) ||
            skernelIsCheetah2OnlyDev(devObjPtr) ||
            skernelIsCheetah3Dev(devObjPtr));
}

/*******************************************************************************
*   skernelIsCheetahDev
*
* DESCRIPTION:
*       check if device should behave like cheetah and not ch2,3...
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not ch device
*       1 - ch device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsCheetah1OnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    switch(devObjPtr->deviceType)
    {
        case SKERNEL_98DX270  :
        case SKERNEL_98DX260  :
        case SKERNEL_98DX250  :
        case SKERNEL_98DX163  :
        case SKERNEL_98DX243  :
        case SKERNEL_98DX253  :
        case SKERNEL_98DX263  :
        case SKERNEL_98DX273  :
        case SKERNEL_98DX107  :
        case SKERNEL_98DX107B0:
        case SKERNEL_98DX133  :
        case SKERNEL_98DX803  :
/*        case SKERNEL_98DX247  : --> same as SKERNEL_98DX253 */
        case SKERNEL_98DX249  :
        case SKERNEL_98DX269  :
        case SKERNEL_DXCH     :  /* generic DXCH  */
        case SKERNEL_DXCH_B0  :  /* generic DXCH_B0  8K FDB*/
            return 1;
        default:
            break;
    }

    return 0;
}

/*******************************************************************************
*   skernelIsXcatDev
*
* DESCRIPTION:
*       check if device should behave like xCat
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not xCat device
*       1 - xCat device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsXcatDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    if(skernelIsXcatOnlyDev(devObjPtr))
    {
        return 1;
    }

    if(skernelIsLionPortGroupDev(devObjPtr))
    {
        return 1;
    }

    return 0;
}

/*******************************************************************************
*   skernelIsXcat2Dev
*
* DESCRIPTION:
*       Check whether device should behave like xCat2
*
* INPUTS:
*        devObjPtr  - pointer to device object
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not xCat2 device
*       1 - xCat2 device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsXcat2Dev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{

    if(skernelIsXcat2OnlyDev(devObjPtr))
    {
        return 1;
    }
    else if (skernelIsLion2PortGroupDev(devObjPtr))
    {
        /* the Lion2 includes features from xcat2 */
        return 1;
    }

    return 0;
}

/*******************************************************************************
*   skernelIsPhyShellOnlyDev
*
* DESCRIPTION:
*       check if device should behave like PHY shell
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not PHY shell device
*       1 - PHY shell device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsPhyShellOnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    switch(devObjPtr->deviceType)
    {
        case SKERNEL_PHY_SHELL:
            return 1;

        default:
            break;
    }

    return 0;
}

/*******************************************************************************
*   skernelIsPhyOnlyDev
*
* DESCRIPTION:
*       check if device should behave like PHY core
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not PHY core device
*       1 - PHY core device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsPhyOnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    switch(devObjPtr->deviceType)
    {
        case SKERNEL_PHY_CORE_1540M_1548M:
            return 1;

        default:
            break;
    }

    return 0;
}

/*******************************************************************************
*   skernelIsMacSecOnlyDev
*
* DESCRIPTION:
*       check if device should behave like macSec
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not macSec device
*       1 - macSec device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsMacSecOnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    switch(devObjPtr->deviceType)
    {
        case SKERNEL_MACSEC:
            return 1;

        default:
            break;
    }

    return 0;
}

/*******************************************************************************
*   skernelIsLion2PortGroupOnlyDev
*
* DESCRIPTION:
*       check if device should behave like Lion2's port group
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not Lion's port group device
*       1 - Lion's port group device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsLion2PortGroupOnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    switch(devObjPtr->deviceType)
    {
        case SKERNEL_LION2_PORT_GROUP_12:
            return 1;

        default:
            break;
    }


    if (skernelIsLion3PortGroupOnlyDev(devObjPtr))
    {
        return 1;
    }

    return 0;
}

/*******************************************************************************
*   skernelIsLion2PortGroupDev
*
* DESCRIPTION:
*       check if device should behave like Lion shell
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not Lion port group device
*       1 - Lion port geoup device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsLion2PortGroupDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    if (skernelIsLion2PortGroupOnlyDev(devObjPtr))
    {
        return 1;
    }

    return 0;
}

/*******************************************************************************
*   skernelIsBobcat3Dev
*
* DESCRIPTION:
*       check if device should behave like Bobcat3
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not Bobcat3 device
*       1 - Bobcat3 device
*
* COMMENTS:
*
*******************************************************************************/
static GT_U32 skernelIsBobcat3Dev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    switch(devObjPtr->deviceType)
    {
        case SKERNEL_BOBCAT3:
            return 1;

        default:
            break;
    }

    return 0;
}
/*******************************************************************************
*   skernelIsBobcat2OnlyDev
*
* DESCRIPTION:
*       check if device should behave like Bobcat2
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not Lion3's port group device
*       1 - Lion3's port group device
*
* COMMENTS:
*
*******************************************************************************/
static GT_U32 skernelIsBobcat2OnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    switch(devObjPtr->deviceType)
    {
        case SKERNEL_BOBCAT2:
        case SKERNEL_BOBK_CAELUM:
        case SKERNEL_BOBK_CETUS:
            return 1;

        default:
            return skernelIsBobcat3Dev(devObjPtr);
    }
}

/*******************************************************************************
*   skernelIsLion3PortGroupOnlyDev
*
* DESCRIPTION:
*       check if device should behave like Lion3's port group
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - not Lion3's port group device
*       1 - Lion3's port group device
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 skernelIsLion3PortGroupOnlyDev(IN SKERNEL_DEVICE_OBJECT * devObjPtr)
{
    switch(devObjPtr->deviceType)
    {
        case SKERNEL_LION3_PORT_GROUP_12:
            return 1;

        default:
            break;
    }

    if (skernelIsBobcat2OnlyDev(devObjPtr))
    {
        return 1;
    }

    return 0;
}

/*******************************************************************************
* deviceTypeInfoGet
*
* DESCRIPTION:
*       This function sets the device info :
*       number of ports ,
*       deviceType ,
*       and returns bmp of ports for a given device.
*
* INPUTS:
*        devObjPtr  - pointer to device object.
*
* OUTPUTS:
*        devObjPtr  - pointer to device object.
*
* RETURNS:
*       GT_OK on success,
*       GT_NOT_FOUND if the given pciDevType is illegal.
*       GT_NOT_SUPPORTED - not properly supported device in DB
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static GT_STATUS deviceTypeInfoGet
(
    INOUT  SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    GT_U32   ii;/* index in simSupportedTypes */
    GT_U32   jj;/* index in simSupportedTypes[ii].devTypeArray */
    GT_U32   kk;/* index in simSpecialDevicesBmp */
    GT_BOOL found = GT_FALSE;
    const PORTS_BMP_STC *existingPortsPtr;
    const PORTS_BMP_STC *multiModePortsBmpPtr;
    PORTS_BMP_STC tmpBmp;
    const SKERNEL_PORT_STATE_ENT *existingPortsStatePtr;

    /* get the info about our device */
    ii = 0;
    while(simSupportedTypes[ii].devFamily != END_OF_TABLE)
    {
        jj = 0;
        while(simSupportedTypes[ii].devTypeArray[jj] != END_OF_TABLE)
        {
            if(devObjPtr->deviceType == simSupportedTypes[ii].devTypeArray[jj])
            {
                found = GT_TRUE;
                break;
            }
            jj++;
        }

        if(found == GT_TRUE)
        {
            break;
        }
        ii++;
    }

    if(found == GT_FALSE)
    {
        devObjPtr->deviceFamily = SKERNEL_DEVICE_FAMILY(devObjPtr);
        if(devObjPtr->deviceFamily == SKERNEL_NOT_INITIALIZED_FAMILY)
        {
            skernelFatalError(" deviceTypeInfoGet: unknown family for device[0x%8.8x]",devObjPtr->deviceType);
        }

        return GT_NOT_FOUND;
    }

    devObjPtr->portsNumber = simSupportedTypes[ii].numOfPorts;

    if(devObjPtr->portsNumber > SKERNEL_DEV_MAX_SUPPORTED_PORTS_CNS)
    {
        skernelFatalError(" deviceTypeInfoGet: not supporting more then %d ports \n",
                          SKERNEL_DEV_MAX_SUPPORTED_PORTS_CNS);
    }

    devObjPtr->deviceFamily = simSupportedTypes[ii].devFamily;

    if(simSupportedTypes[ii].defaultPortsBmpPtr == NULL)
    {
        /* no special default for the ports BMP , use continuous ports bmp */
        existingPortsPtr = &tmpBmp;

        if(simSupportedTypes[ii].numOfPorts < 32)
        {
            BMP_PORTS_LESS_32_MAC((&tmpBmp),
                            simSupportedTypes[ii].numOfPorts);
        }
        else if (simSupportedTypes[ii].numOfPorts < 64)
        {
            BMP_PORTS_32_TO_63_MAC((&tmpBmp),
                            simSupportedTypes[ii].numOfPorts);
        }
        else /* simSupportedTypes[ii].numOfPorts >= 64 */
        {
            BMP_PORTS_64_TO_95_MAC((&tmpBmp),
                            simSupportedTypes[ii].numOfPorts);
        }
    }
    else
    {
        /* use special default for the ports BMP */
        existingPortsPtr = simSupportedTypes[ii].defaultPortsBmpPtr;
    }

    existingPortsStatePtr = simSupportedTypes[ii].existingPortsStatePtr;
    multiModePortsBmpPtr  = simSupportedTypes[ii].multiModePortsBmpPtr;

    /****************************************************************/
    /* add here specific devices BMP of ports that are not standard */
    /****************************************************************/
    kk = 0;
    while(simSpecialDevicesBmp[kk].devType != END_OF_TABLE)
    {
        if(simSpecialDevicesBmp[kk].devType == devObjPtr->deviceType)
        {
            existingPortsPtr = &simSpecialDevicesBmp[kk].existingPorts;
            multiModePortsBmpPtr = & simSpecialDevicesBmp[kk].multiModePortsBmp;
            break;
        }
        kk++;
    }

    for(ii = 0 ; ii < devObjPtr->portsNumber ; ii++)
    {
        if(PORTS_BMP_IS_PORT_SET_MAC(existingPortsPtr,ii))
        {
            /* get default port's state */
            devObjPtr->portsArr[ii].state = existingPortsStatePtr[ii];
            if(multiModePortsBmpPtr)
            {
                devObjPtr->portsArr[ii].supportMultiState =
                    PORTS_BMP_IS_PORT_SET_MAC(multiModePortsBmpPtr,ii) ?
                        GT_TRUE : GT_FALSE ;
            }
            else
            {
                devObjPtr->portsArr[ii].supportMultiState = GT_FALSE;
            }
        }
        else
        {
            /* port not exists */
            devObjPtr->portsArr[ii].state = SKERNEL_PORT_STATE_NOT_EXISTS_E;
            devObjPtr->portsArr[ii].supportMultiState = GT_FALSE;
        }
    }

    /* set all other ports a 'Not exists' */
    for(/* continue */; ii < SKERNEL_DEV_MAX_SUPPORTED_PORTS_CNS; ii++)
    {
        devObjPtr->portsArr[ii].state = SKERNEL_PORT_STATE_NOT_EXISTS_E;
        devObjPtr->portsArr[ii].supportMultiState = GT_FALSE;
    }

    return GT_OK;
}

/*******************************************************************************
*   smainMemAddrDefaultGet
*
* DESCRIPTION:
*       Load default values for memory for specified address from text file
*
* INPUTS:
*       devObjPtr - pointer to the device object.
*       fileNamePtr - default values file name.
*       address     - lookup address.
*
* OUTPUTS:
*       valuePtr    - pointer to lookup address value
*
* RETURNS:
*
*       GT_OK       - lookup address found
*       GT_FAIL     - lookup address not found
*       GT_BAD_PTR  - NULL pointer
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS smainMemAddrDefaultGet
(
    IN SKERNEL_DEVICE_OBJECT    * deviceObjPtr,
    IN GT_CHAR                  * fileNamePtr,
    IN GT_U32                   address,
    OUT GT_U32                  * valuePtr
)
{
    FILE    * regFilePtr;   /* file object */
    static char  buffer[FILE_MAX_LINE_LENGTH_CNS]; /* string buffer */
    GT_U32 unitAddr = 0;    /* unit address for register, 0 means not valid*/
    GT_U32 baseAddr;        /* base address for register */
    GT_U32 value;           /* default value */
    GT_32  number;          /* number of registers */
    GT_U32 addrDelta;       /* delta for several registers */
    GT_U32 scanRes;         /* number scanned elements */
    char *  remarkStr;
    GT_CHAR pciAddr[SMAIN_PCI_FLAG_BUFF_SIZE];
    GT_U32  currentLineIndex = 0;/*current line number in the 'registers file'*/
    GT_STATUS status = GT_FAIL;

    if (!fileNamePtr || !valuePtr)
       return GT_BAD_PTR;

    /* open file */
    regFilePtr = fopen(fileNamePtr,"rt");
    if (regFilePtr == NULL)
    {
        /* avoid mess of messages */
        SIM_OS_MAC(simOsSleep)(1);
        skernelFatalError("smainMemAddrDefaultGet: registers file not found %s\n", fileNamePtr);
    }

    while (fgets(buffer, FILE_MAX_LINE_LENGTH_CNS, regFilePtr))
    {
        currentLineIndex++;/* 1 based number */
        /* convert to lower case */
        strlwr(buffer);

        remarkStr = strstr(buffer,";");
        if (remarkStr == buffer)
        {
            continue;
        }
        /* UNIT_BASE_ADDR string detection */
        if(!sscanf(buffer, "unit_base_addr %x\n", &unitAddr))
        {
            if(strstr(buffer, "unit_base_addr not_valid"))
            {
                unitAddr = 0;
                continue;
            }
        }
        else
        {
            continue;
        }
        /* record found, parse it */
        baseAddr = 0;
        value = 0;
        number = 0;
        addrDelta = 0; /* if scanRes < 4 => addrDelta is undefined */
        memset(pciAddr, 0, SMAIN_PCI_FLAG_BUFF_SIZE);
        scanRes = sscanf(buffer, "%x %x %d %x %10s", &baseAddr,
                        &value, &number, &addrDelta, pciAddr);

        if (baseAddr == SMAIN_FILE_RECORDS_EOF)
        {
            break;
        }
        if (((baseAddr & 0xFF000000) != 0) && (unitAddr != 0))
        {
            skernelFatalError("When UNIT_BASE_ADDR is valid then the address of register must be 'zero based' %s (%d)\n", fileNamePtr, currentLineIndex);
         }

        baseAddr += unitAddr;
        if(baseAddr == address)
        {
            *valuePtr = value;
            status = GT_OK;
            break;
        }

        if ((scanRes < 2) || (scanRes == 3) ||(scanRes > 5))
        {
            /* check end of file */
            if (scanRes == 1)
                break;

            simWarningPrintf("smainMemAddrDefaultGet: registers file's bad format in line [%d]\n",currentLineIndex);
            break;
        }
    }

    fclose(regFilePtr);

    return status;
}

/*******************************************************************************
*   smainMemAdditionalAddrDefaultGet
*
* DESCRIPTION:
*       Load default values for memory for specified address from additional text file
*
* INPUTS:
*       devObjPtr - pointer to the device object.
*       devId     - the deviceId to use when build the name in INI file to look for
*       address   - lookup address.
*
* OUTPUTS:
*       valuePtr  - pointer to lookup address value
*
* RETURNS:
*       GT_OK       - lookup address found
*       GT_FAIL     - lookup address not found
*       GT_BAD_PTR  - NULL pointer
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS smainMemAdditionalAddrDefaultGet
(
    IN SKERNEL_DEVICE_OBJECT    * deviceObjPtr,
    IN GT_U32                   devId,
    IN GT_U32                   address,
    OUT GT_U32                  * valuePtr
)
{
    GT_CHAR keyStr[SMAIN_MIN_SIZE_OF_BUFFER_CNS]; /* key string*/
    GT_CHAR paramStr[FILE_MAX_LINE_LENGTH_CNS];
    GT_STATUS retVal = GT_FAIL;
    GT_U32 regData;
    GT_U32 i;

    sprintf(keyStr, "registers%u", devId);
    if (!SIM_OS_MAC(simOsGetCnfValue)(INI_FILE_SYSTEM_SECTION_CNS, keyStr,
           FILE_MAX_LINE_LENGTH_CNS, paramStr))
    {
        if(deviceObjPtr->portGroupSharedDevObjPtr)
        {
            /* use the register file of the 'father' device object (from the system) */
            return smainMemAdditionalAddrDefaultGet(deviceObjPtr, devId, address, valuePtr);
        }

        return GT_FAIL;
    }

    if(GT_OK == smainMemAddrDefaultGet(deviceObjPtr, paramStr, address, &regData))
    {
        retVal = GT_OK;
        *valuePtr = regData;
    }

    /* Additional defaults registers value, files */
    for(i = 1; i < 100; i++)
    {
        sprintf(keyStr, "registers%u_%2.2u", devId, i);
        if (GT_FALSE == SIM_OS_MAC(simOsGetCnfValue)(INI_FILE_SYSTEM_SECTION_CNS,
                                                     keyStr,
                                                     FILE_MAX_LINE_LENGTH_CNS,
                                                     paramStr))
        {
            /* no more registers files */
            break;
        }

        if(GT_OK == smainMemAddrDefaultGet(deviceObjPtr, paramStr, address, &regData))
        {
            retVal = GT_OK;
            /* let other additional file to override the value */
            *valuePtr = regData;
        }
    }

    return retVal;
}

/*******************************************************************************
*   smainDeviceRevisionGet
*
* DESCRIPTION:
*       Load revision ID values for memory from text file
*
* INPUTS:
*       deviceObjPtr - pointer to the device object.
*       devId  - the deviceId to use when build the name in INI file to look for
* OUTPUTS:
*
* RETURNS:
*       GT_OK       - revision ID found
*       GT_FAIL     - revision ID not found
*       GT_NOT_SUPPORTED - not cheetah device
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS smainDeviceRevisionGet
(
    IN SKERNEL_DEVICE_OBJECT    * deviceObjPtr,
    IN GT_U32                   devId
)
{
    GT_U32 regData;
    GT_STATUS retVal;
    GT_U32  revision = 0xFFFFFFFF;

    /* GM memory may not be initialized, so there is no access to register 0x4c.
       We also don't use deviceObjPtr->deviceRevisionId in GM */
    if (deviceObjPtr->gmDeviceType == GOLDEN_MODEL)
    {
        return GT_NOT_SUPPORTED;
    }

    /* Relevant only for cheetah devices */
    if(SKERNEL_DEVICE_FAMILY_CHEETAH(deviceObjPtr) == 0)
    {
        return GT_NOT_SUPPORTED;
    }

    retVal =
        smainMemAdditionalAddrDefaultGet(deviceObjPtr, devId, 0x4c, &regData);
    if(retVal == GT_OK)
    {
        revision = (regData & 0xf);
        /* let other additional file to override the value */
    }

    if(revision != 0xFFFFFFFF)
    {
        deviceObjPtr->deviceRevisionId = revision;
        return GT_OK;
    }

    return GT_FAIL;
}

/*******************************************************************************
*   smainMemDefaultsLoadAll
*
* DESCRIPTION:
*       Load default values for device memory from all default registers files
*
* INPUTS:
*       deviceObjPtr    - pointer to the device object
*       devId           - device Id
*       sectionPtr      - pointer to section name
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smainMemDefaultsLoadAll
(
    IN SKERNEL_DEVICE_OBJECT    * deviceObjPtr,
    IN GT_U32                   devId,
    IN GT_CHAR                  * sectionPtr
)
{
    GT_CHAR  keyPtr[SMAIN_MIN_SIZE_OF_BUFFER_CNS];                          /* searching key string*/
    GT_CHAR  devInfoSection[SMAIN_MIN_SIZE_OF_BUFFER_CNS];                  /*the device info section in the INI file*/
    GT_CHAR  strVal[FILE_MAX_LINE_LENGTH_CNS];    /* output string value */
    GT_U32  ii;
    SMEM_CHT_DEV_COMMON_MEM_INFO  * commonDevMemInfoPtr = deviceObjPtr->deviceMemory;
    SMEM_UNIT_CHUNKS_STC    *dfxUnitChunkPtr;
    SMEM_CHT_GENERIC_DEV_MEM_INFO *devMemInfoPtr = deviceObjPtr->deviceMemory;

    if (deviceObjPtr->shellDevice)
    {
        return;
    }

    if(deviceObjPtr->registersDefaultsPtr)
    {
        /* the device hold default register values that need to be uploaded before the external 'Register file' loaded */
        smemInitRegistersWithDefaultValues(deviceObjPtr, SKERNEL_MEMORY_WRITE_E, deviceObjPtr->registersDefaultsPtr,NULL);

        for(ii = 0 ; ii < SMEM_CHT_NUM_UNITS_MAX_CNS; ii++)
        {
            if(devMemInfoPtr->unitMemArr[ii].unitDefaultRegistersPtr)
            {
                smemInitRegistersWithDefaultValues(deviceObjPtr, SKERNEL_MEMORY_WRITE_E,
                    /* the default registers of the unit */
                    devMemInfoPtr->unitMemArr[ii].unitDefaultRegistersPtr,
                    /* the unit info */
                    &devMemInfoPtr->unitMemArr[ii]);
            }
        }
    }

    if(deviceObjPtr->registersDfxDefaultsPtr)
    {
        dfxUnitChunkPtr = &commonDevMemInfoPtr->pciExtMemArr[SMEM_UNIT_PCI_BUS_DFX_E].unitMem;

        if(dfxUnitChunkPtr->otherPortGroupDevObjPtr == 0)/* indication that this core actually 'holds' the DFX memory */
        {
            /* the device holds DFX server default register values that need to be uploaded before the external 'Register file' loaded */
            smemInitRegistersWithDefaultValues(deviceObjPtr, SKERNEL_MEMORY_WRITE_DFX_E, deviceObjPtr->registersDfxDefaultsPtr,NULL);
        }
    }

    if(deviceObjPtr->registersPexDefaultsPtr)
    {
        /* the device holds PEX default register values that need to be uploaded before the external 'Register file' loaded */
        smemInitRegistersWithDefaultValues(deviceObjPtr, SKERNEL_MEMORY_WRITE_PCI_E, deviceObjPtr->registersPexDefaultsPtr,NULL);
    }



    sprintf(devInfoSection, "%s", sectionPtr);

    if(deviceObjPtr->portGroupSharedDevObjPtr)
    {
        sprintf(devInfoSection, "%s", INI_FILE_SYSTEM_SECTION_CNS);
    }

    sprintf(keyPtr, "registers%u", devId);

    /* Get default registers value file */
    if(SIM_OS_MAC(simOsGetCnfValue)(devInfoSection, keyPtr,
                                    FILE_MAX_LINE_LENGTH_CNS, strVal))
    {
        smainMemDefaultsLoad(deviceObjPtr, strVal);

        for(ii = 1; ii < 100; ii++)
        {
            sprintf(keyPtr, "registers%u_%2.2u", devId, ii);

            /* Get additional default registers value file */
            if(GT_FALSE == SIM_OS_MAC(simOsGetCnfValue)(devInfoSection, keyPtr,
                                                        FILE_MAX_LINE_LENGTH_CNS, strVal))
            {
                /* no more registers files */
                break;
            }

            smainMemDefaultsLoad(deviceObjPtr, strVal);
        }
    }
}

/*******************************************************************************
*   skernelDeviceSoftReset
*
* DESCRIPTION:
*       Soft reset of single device object
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*       Function do device soft reset on the following order:
*           - disable device interrupts
*           - force link of all active SLANs down
*           - disable SAGING task
*           - reset all AISC memories by default values
*           - enable SAGING task
*           - force link of all active SLANs up
*           - enable device interrupts
*
*******************************************************************************/
static SKERNEL_DEVICE_OBJECT *   skernelDeviceSoftReset
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    if(devObjPtr->devSoftResetFunc)
    {
        return devObjPtr->devSoftResetFunc(devObjPtr);
    }

    return devObjPtr;
}

/*******************************************************************************
*   skernelDeviceSoftResetPart2
*
* DESCRIPTION:
*       Soft reset of single device object - part 2
*
* INPUTS:
*       devObjPtr           - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
static SKERNEL_DEVICE_OBJECT *   skernelDeviceSoftResetPart2
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    if(devObjPtr->devSoftResetFuncPart2)
    {
        return devObjPtr->devSoftResetFuncPart2(devObjPtr);
    }

    return devObjPtr;
}


/*******************************************************************************
*   skernelPortLoopbackForceModeSet
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
*
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
GT_STATUS skernelPortLoopbackForceModeSet
(
    IN  GT_U32                      deviceId,
    IN  GT_U32                      portNum,
    IN SKERNEL_PORT_LOOPBACK_FORCE_MODE_ENT mode
)
{
    SKERNEL_DEVICE_OBJECT * devObjPtr;
    SKERNEL_PORT_INFO_STC   *portInfoPtr;

    devObjPtr = smemTestDeviceIdToDevPtrConvert(deviceId);
    if(portNum >= SKERNEL_DEV_MAX_SUPPORTED_PORTS_CNS)
    {
        return GT_BAD_PARAM;
    }

    switch(mode)
    {
        case SKERNEL_PORT_LOOPBACK_NOT_FORCED_E:
        case SKERNEL_PORT_LOOPBACK_FORCE_ENABLE_E:
            break;
        default:
            return GT_BAD_PARAM;
    }


    portInfoPtr =  &devObjPtr->portsArr[portNum];
    portInfoPtr->loopbackForceMode = mode;

    return GT_OK;
}

/*******************************************************************************
*   skernelPortLinkStateSet
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
GT_STATUS skernelPortLinkStateSet
(
    IN  GT_U32                      deviceId,
    IN  GT_U32                      portNum,
    IN GT_BOOL                      linkState
)
{
    SKERNEL_DEVICE_OBJECT * devObjPtr;


    devObjPtr = smemTestDeviceIdToDevPtrConvert(deviceId);
    if(portNum >= SKERNEL_DEV_MAX_SUPPORTED_PORTS_CNS)
    {
        return GT_BAD_PARAM;
    }

    snetLinkStateNotify(devObjPtr,portNum,linkState);

    return GT_OK;
}
/*******************************************************************************
* prvConvertLocalDevPort
*
* DESCRIPTION:
*       Converts global devId and port to local (core) device and port
*
* INPUTS:
*       deviceIdPtr - (pointer to)the simulation device Id
*       portNumPtr  - (pointer to)the physical port number
*
* OUTPUTS:
*       deviceIdPtr - (pointer to)core device Id
*       portNumPtr  - (pointer to)the local port number
*
* RETURNS:
*       GT_OK         - on success
*       GT_BAD_PARAM  - on bad deviceId or portNumPtr
*       GT_BAD_PTR    - on bad pointers
*
* COMMENTS:
*
*******************************************************************************/
static GT_STATUS prvConvertLocalDevPort
(
    INOUT GT_U32        *deviceIdPtr,
    INOUT GT_U32        *portNumPtr
)
{
    GT_U32          ii;
    SKERNEL_DEVICE_OBJECT *deviceObjPtr; /* device object pointer */

    /* get dev object */
    deviceObjPtr = smemTestDeviceIdToDevPtrConvert(*deviceIdPtr);

    /* find to which core the port belongs */
    for(ii = 0 ; ii < (deviceObjPtr->numOfCoreDevs - 1) ;ii++)
    {
        if((*portNumPtr) < deviceObjPtr->coreDevInfoPtr[ii + 1].startPortNum)
        {
            break;
        }
    }

    if(ii == (deviceObjPtr->numOfCoreDevs - 1))
    {
        if((*portNumPtr) == PRESTERA_CPU_PORT_CNS)
        {
            /* currently simulation implementation will bind 'cpu port' from the
               INI file into core 0 */
            ii = 0;
        }
        else
        {
            if(((*portNumPtr)- deviceObjPtr->coreDevInfoPtr[ii].startPortNum) >= deviceObjPtr->coreDevInfoPtr[ii].devObjPtr->portsNumber)
            {
                simWarningPrintf("prvConvertLocalDevPort: global port [%d] was not found in valid ranges of the cores \n", (*portNumPtr));
                /* global port was not found in valid ranges of the cores */
                return GT_BAD_PARAM;
            }
        }
    }

    /* get local devId */
    *deviceIdPtr = deviceObjPtr->coreDevInfoPtr[ii].devObjPtr->deviceId;

    /* get local port */
    *portNumPtr -= deviceObjPtr->coreDevInfoPtr[ii].startPortNum;

    return GT_OK;
}

/*******************************************************************************
*   skernelUnbindDevPort2Slan
*
* DESCRIPTION:
*       The function unbinds given dev and port from slan
*
* INPUTS:
*       deviceId     - the simulation device Id
*       portNum         - the physical port number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK         - on success
*       GT_BAD_PARAM  - on bad deviceId or portNum
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS skernelUnbindDevPort2Slan
(
    IN  GT_U32                      deviceId,
    IN  GT_U32                      portNum
)
{
    GT_STATUS              st;
    SKERNEL_DEVICE_OBJECT *deviceObjPtr; /* device object pointer */
    SMAIN_PORT_SLAN_INFO  *slanInfoPrt;  /* slan port entry pointer */
    GT_U32          port = portNum;
    GT_BOOL         unbindDone = GT_FALSE;

    DEVICE_ID_CHECK_MAC(deviceId);
    /* get device object */
    deviceObjPtr = smemTestDeviceIdToDevPtrConvert(deviceId);

    /* check for multiport group father device */
    if(deviceObjPtr->shellDevice == GT_TRUE)
    {
        /* get local dev id and port */
        st = prvConvertLocalDevPort(&deviceId, &port);
        if(GT_OK != st)
        {
            return st;
        }
        /* get device object */
        deviceObjPtr = smemTestDeviceIdToDevPtrConvert(deviceId);
    }

    if((port >= deviceObjPtr->numSlans) ||
       (deviceObjPtr->portsArr[port].state == SKERNEL_PORT_STATE_NOT_EXISTS_E))
    {
        simWarningPrintf("skernelUnbindDevPort2Slan: wrong port [%d]\n", port);
        return GT_BAD_PARAM;
    }

    /* get slan info */
    slanInfoPrt = &(deviceObjPtr->portSlanInfo[port]);

    SIM_OS_MAC(simOsSemWait)(slanDownSemaphore,SIM_OS_WAIT_FOREVER);
    slanInfoPrt->slanName[0] = 0; /* before the unbind , make sure no one try to send packet to this port */
    SIM_OS_MAC(simOsSemSignal)(slanDownSemaphore);

    deviceObjPtr->portsArr[port].linkStateWhenNoForce = SKERNEL_PORT_NATIVE_LINK_DOWN_E;

    /* allow all other tasks that send traffic to not be inside the SLAN that going to be unbound */
    SIM_OS_MAC(simOsSleep)(1);

    /* unbind the port */
    unbindDone = internalSlanUnbind(slanInfoPrt);

    if(unbindDone == GT_TRUE)
    {
        /*set port as 'down'*/
        snetLinkStateNotify(deviceObjPtr, port, 0);
    }

    return GT_OK;
}

/*******************************************************************************
* skernelBindDevPort2Slan
*
* DESCRIPTION:
*       The function binds given dev and port to slan
*
* INPUTS:
*       deviceId     - the simulation device Id
*       portNum      - the physical port number
*       slanNamePtr  - slan name string,
*       unbindOtherPortsOnThisSlan -
*                      GT_TRUE  - if the given slanName is bound to any other port(s)
*                                 unbind it from this port(s) before bind it to the new port.
*                                 This will cause link change (link down) on this other port(s).
*                      GT_FALSE - bind the port with slanName regardless
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK         - on success
*       GT_BAD_PARAM  - on bad deviceId, portNum, slan name
*       GT_FAIL       - on error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS skernelBindDevPort2Slan
(
    IN  GT_U32                      deviceId,
    IN  GT_U32                      portNum,
    IN  GT_CHAR                    *slanNamePtr,
    IN  GT_BOOL                     unbindOtherPortsOnThisSlan
)
{
    GT_STATUS              st;
    SKERNEL_DEVICE_OBJECT *deviceObjPtr; /* device object pointer */
    SKERNEL_DEVICE_OBJECT *currDeviceObjPtr; /* current device object pointer */
    SMAIN_PORT_SLAN_INFO  *slanInfoPrt;  /* slan port entry pointer */
    GT_U32                 ii,jj;       /* iterators */
    GT_U32          port = portNum;

    DEVICE_ID_CHECK_MAC(deviceId);
    deviceObjPtr = smemTestDeviceIdToDevPtrConvert(deviceId);

    /* check for multiport group father device */
    if(deviceObjPtr->shellDevice)
    {
        /* get local dev id and port */
        st = prvConvertLocalDevPort(&deviceId, &port);
        if(GT_OK != st)
        {
            return st;
        }
        /* get device object */
        deviceObjPtr = smemTestDeviceIdToDevPtrConvert(deviceId);
    }

    /* check port */
    if((port >= deviceObjPtr->numSlans) ||
       (deviceObjPtr->portsArr[port].state == SKERNEL_PORT_STATE_NOT_EXISTS_E))
    {
        simWarningPrintf("skernelBindDevPort2Slan: wrong port [%d]\n", port);
        return GT_BAD_PARAM;
    }

    /* check slanNamePtr */
    if (NULL == slanNamePtr)
    {
        simWarningPrintf("skernelBindDevPort2Slan: wrong slan name pointer[NULL]\n");
        return GT_BAD_PARAM;
    }

    /* get slan info */
    slanInfoPrt = &(deviceObjPtr->portSlanInfo[port]);

    if(0 == strcmp(slanInfoPrt->slanName,slanNamePtr))
    {
        /* The slan name was not changed , do not trigger unbind and re-bind */
        return GT_OK;
    }

    /* unbind this port from previous bind */
    st = skernelUnbindDevPort2Slan(deviceId, port);
    if(GT_OK != st)
    {
        return st;
    }

    /* check link with the same slan name (if need) */
    if(unbindOtherPortsOnThisSlan == GT_TRUE)
    {
        /* search the same slan name in all devices */
        for (ii = 0; ii < SMAIN_MAX_NUM_OF_DEVICES_CNS; ii++)
        {
            currDeviceObjPtr = smainDeviceObjects[ii];
            if (!currDeviceObjPtr)
            {
                continue;
            }

            if(currDeviceObjPtr->shellDevice == GT_TRUE)
            {
                continue;
            }

            /* port loop */
            for (jj = 0; jj < currDeviceObjPtr->numSlans; jj++)
            {
                /* get slan info */
                slanInfoPrt = &(currDeviceObjPtr->portSlanInfo[jj]);

                if ( strcmp(slanInfoPrt->slanName, slanNamePtr) == 0 )
                {
                    /* unbind slan */
                    st = skernelUnbindDevPort2Slan(ii, jj);
                    if(GT_OK != st)
                    {
                        return st;
                    }
                }
            }
        }
    }

    /* get slan info */
    slanInfoPrt = &(deviceObjPtr->portSlanInfo[port]);

    slanInfoPrt->deviceObj  = deviceObjPtr;
    slanInfoPrt->portNumber = port;

    /* bind port - RX,TX direction with SLAN */
    internalSlanBind(slanNamePtr,deviceObjPtr,port,GT_TRUE,GT_TRUE,slanInfoPrt,NULL);

    /* notify link UP */
    snetLinkStateNotify(deviceObjPtr, port, 1);

    return GT_OK;
}

/*******************************************************************************
* skernelDevPortSlanGet
*
* DESCRIPTION:
*       The function get the slanName of a port of device
*
* INPUTS:
*       deviceId      - the simulation device Id
*       portNum       - the physical port number
*       slanMaxLength - the length of the buffer (for the string) that is
*                       allocated by the caller for slanNamePtr
*                       when slanNamePtr != NULL then slanMaxLength must be >= 8
*
* OUTPUTS:
*       portBoundPtr - (pointer to)is port bound.
*       slanNamePtr  - (pointer to)slan name (string).
*                      The caller function must cares about memory allocation.
*                      NOTE:
*                      1. this parameter can be NULL ... meaning that caller not
*                         care about the slan name only need to know that port
*                         bound / not bound
*                      2. if slanMaxLength is less then the actual length of
*                         the slanName then only part of the name will be returned.
*
* RETURNS:
*       GT_OK         - on success
*       GT_BAD_PARAM  - on bad deviceId or portNum.
*                       when slanNamePtr!=NULL but slanMaxLength < 8
*       GT_BAD_PTR    - on NULL pointer portBoundPtr
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS skernelDevPortSlanGet
(
    IN  GT_U32                deviceId,
    IN  GT_U32                portNum,
    IN  GT_U32                slanMaxLength,
    OUT GT_BOOL              *portBoundPtr,
    OUT GT_CHAR              *slanNamePtr
)
{
    GT_STATUS              st;
    SKERNEL_DEVICE_OBJECT *deviceObjPtr; /* device object pointer */
    SMAIN_PORT_SLAN_INFO  *slanInfoPtr;  /* slan port entry pointer */
    GT_U32          port = portNum;

    DEVICE_ID_CHECK_MAC(deviceId);
    deviceObjPtr = smemTestDeviceIdToDevPtrConvert(deviceId);

    /* check for multiport group father device */
    if(deviceObjPtr->shellDevice)
    {
        /* get local dev id and port */
        st = prvConvertLocalDevPort(&deviceId, &port);
        if(GT_OK != st)
        {
            return st;
        }
        /* get device object */
        deviceObjPtr = smemTestDeviceIdToDevPtrConvert(deviceId);
    }

    if((port >= deviceObjPtr->numSlans) ||
       (deviceObjPtr->portsArr[port].state == SKERNEL_PORT_STATE_NOT_EXISTS_E))
    {
        return GT_BAD_PARAM;
    }

    /* get slan info */
    slanInfoPtr = &(deviceObjPtr->portSlanInfo[port]);

    if(slanInfoPtr && (slanInfoPtr->slanName[0] != 0))
    {
        *portBoundPtr = GT_TRUE;

        if(slanNamePtr)
        {
            if(slanMaxLength < 8)
            {
                simForcePrintf("skernelDevPortSlanGet: slanMaxLength [%d] < 8 \n", slanMaxLength);
                return GT_BAD_PARAM;
            }

            strncpy(slanNamePtr,slanInfoPtr->slanName,slanMaxLength);

            slanNamePtr[slanMaxLength-1] = 0;/* terminate the string in case the slanMaxLength was not enough */
        }
    }
    else
    {
        *portBoundPtr = GT_FALSE;
    }

    return GT_OK;
}

/*******************************************************************************
* skernelDevPortSlanPrint
*
* DESCRIPTION:
*       The function print the slanName of a port of device
*
* INPUTS:
*       deviceId     - the simulation device Id
*       portNum         - the physical port number
*
* OUTPUTS:
*       none
*
* RETURNS:
*       GT_OK         - on success
*       GT_BAD_PARAM  - on bad deviceId or portNum
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS skernelDevPortSlanPrint
(
    IN  GT_U32                deviceId,
    IN  GT_U32                portNum
)
{
    GT_CHAR slanName[SMAIN_SLAN_NAME_SIZE_CNS];
    GT_STATUS st;
    GT_BOOL isPortBound;

    st = skernelDevPortSlanGet(deviceId, portNum,SMAIN_SLAN_NAME_SIZE_CNS ,&isPortBound , slanName);
    if(GT_OK != st)
    {
        return st;
    }

    if(isPortBound == GT_TRUE)
    {
        simForcePrintf("portNum[%d] with slan[%s] \n", portNum, slanName);
    }

    return GT_OK;
}

/*******************************************************************************
* skernelDevSlanPrint
*
* DESCRIPTION:
*       The function print ports slanName of the device
*
* INPUTS:
*       deviceId     - the simulation device Id
*
* OUTPUTS:
*       none
*
* RETURNS:
*       GT_OK         - on success
*       GT_BAD_PARAM  - on bad deviceId
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS skernelDevSlanPrint
(
    IN  GT_U32                deviceId
)
{
    SKERNEL_DEVICE_OBJECT *deviceObjPtr; /* device object pointer */
    SKERNEL_DEVICE_OBJECT *currDeviceObjPtr; /* current device object pointer */
    GT_U32  dev,port;                 /* iterators */
    GT_U32  globalPort;

    DEVICE_ID_CHECK_MAC(deviceId);
    deviceObjPtr = smemTestDeviceIdToDevPtrConvert(deviceId);

    simForcePrintf("start device[%d] slan printings \n", deviceId);

    if(deviceObjPtr->shellDevice == GT_TRUE)
    {

        for(dev = 0 ; dev < deviceObjPtr->numOfCoreDevs ; dev++)
        {
            currDeviceObjPtr = deviceObjPtr->coreDevInfoPtr[dev].devObjPtr;
            if(currDeviceObjPtr == NULL)
            {
                continue;
            }

            globalPort =  deviceObjPtr->coreDevInfoPtr[dev].startPortNum;

            for(port = 0 ; port < currDeviceObjPtr->portsNumber ; port++ , globalPort++)
            {
                if(!IS_CHT_VALID_PORT(currDeviceObjPtr,port))
                {
                    continue;
                }

                (void)skernelDevPortSlanPrint(deviceId,globalPort);
            }
        }
    }
    else
    {
        for(port = 0 ; port < deviceObjPtr->portsNumber ; port++)
        {
            if(!IS_CHT_VALID_PORT(deviceObjPtr,port))
            {
                continue;
            }

            (void)skernelDevPortSlanPrint(deviceId,port);
        }
    }

    /* try to print the CPU port (if exists) */
    (void)skernelDevPortSlanPrint(deviceId,PRESTERA_CPU_PORT_CNS);


    simForcePrintf("end device slan printings \n");

    return GT_OK;
}

/*******************************************************************************
* skernelFatherDeviceIdFromSonDeviceIdGet
*
* DESCRIPTION:
*       The function convert 'son device id' to 'father deviceId'.
*       for device with no 'father' --> return 'son deviceId'
*
* INPUTS:
*       sonDeviceId     - the simulation device Id of the 'son'
*
* OUTPUTS:
*       fatherDeviceIdPtr - (pointer to)the simulation device Id of the 'father'
*
* RETURNS:
*       GT_OK         - on success
*       GT_BAD_PARAM  - on bad sonDeviceId .
*       GT_BAD_PTR    - on NULL pointer
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS skernelFatherDeviceIdFromSonDeviceIdGet
(
    IN  GT_U32                sonDeviceId,
    OUT GT_U32               *fatherDeviceIdPtr
)
{
    GT_U32  ii;
    SKERNEL_DEVICE_OBJECT   *sonDeviceObjPtr;
    SKERNEL_DEVICE_OBJECT   *fatherDeviceObjPtr;

    DEVICE_ID_CHECK_MAC(sonDeviceId);
    sonDeviceObjPtr = smemTestDeviceIdToDevPtrConvert(sonDeviceId);

    /* use the father device */
    if(sonDeviceObjPtr->portGroupSharedDevObjPtr == NULL)
    {
        *fatherDeviceIdPtr = sonDeviceId;
        return GT_OK;
    }

    fatherDeviceObjPtr = sonDeviceObjPtr->portGroupSharedDevObjPtr;

    /* find the 'device Id' of the father */
    for(ii = 0 ; ii < SMAIN_MAX_NUM_OF_DEVICES_CNS; ii++)
    {
        if(smainDeviceObjects[ii] == fatherDeviceObjPtr)
        {
            break;
        }
    }

    *fatherDeviceIdPtr = ii;
    return GT_OK;
}


/*******************************************************************************
* skernelSupportMacLoopbackInContextOfSender
*
* DESCRIPTION:
*       The function set the device as 'Support Mac Loopback In Context Of Sender'
*       allow the device to do 'external' mac loopback as processing the loop in the
*       context of the sending task (not sending it to the 'buffers pool')
*
*       the function is for performance issues.
*
* INPUTS:
*       enable     - the simulation device Id.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK         - on success
*
* COMMENTS:
*       currently implemented only for : Puma3
*******************************************************************************/
GT_STATUS skernelSupportMacLoopbackInContextOfSender
(
    IN  GT_BOOL                enable
)
{
    supportMacLoopbackInContextOfSender = enable;
    return GT_OK;
}

/*******************************************************************************
*   skernelCreatePacketGenerator
*
* DESCRIPTION:
*       Create packet generator task
*
* INPUTS:
*       deviceObjPtr - allocated pointer for the device
*       tgNumber     - packet generator number
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
GT_VOID skernelCreatePacketGenerator
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr,
    IN GT_U32 tgNumber
)
{
    SKERNEL_TRAFFIC_GEN_STC * trafficGenDataPtr;/* pointer to packet generator */
    GT_TASK_HANDLE          taskHandle;          /* task handle */

    if(deviceObjPtr->trafficGeneratorSupport.tgSupport)
    {
        smemInitPhaseSemaphore = SIM_OS_MAC(simOsSemCreate)(0,1);

        trafficGenDataPtr =
            &deviceObjPtr->trafficGeneratorSupport.trafficGenData[tgNumber];
        trafficGenDataPtr->deviceObjPtr = deviceObjPtr;
        trafficGenDataPtr->trafficGenNumber = tgNumber;

        taskHandle = SIM_OS_MAC(simOsTaskCreate)(GT_TASK_PRIORITY_ABOVE_NORMAL,
                          (unsigned (__TASKCONV *)(void*))smainPacketGeneratorTask,
                          (void *) trafficGenDataPtr);
        if (taskHandle == NULL)
        {
            skernelFatalError(" skernelInit: cannot create traffic generator "\
                              " task for device %u", deviceObjPtr->deviceId);
        }
        /* wait for semaphore */
        SIM_OS_MAC(simOsSemWait)(smemInitPhaseSemaphore, SIM_OS_WAIT_FOREVER);

        SCIB_SEM_TAKE;
        deviceObjPtr->numThreadsOnMe++;
        SCIB_SEM_SIGNAL;

        /* Set packet generator state in database */
        deviceObjPtr->trafficGeneratorSupport.trafficGenData[tgNumber].trafficGenActive = GT_TRUE;

        SIM_OS_MAC(simOsSemDelete)(smemInitPhaseSemaphore);
    }
}

/*******************************************************************************
*   skernelStatusGet
*
* DESCRIPTION:
*       Get status (Idle or Busy) of all Simulation Kernel tasks.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0 - Simulation Kernel Tasks are Idle
*       other - Simulation Kernel Tasks are busy
* COMMENTS:
*
*
*******************************************************************************/
GT_U32 skernelStatusGet
(
    void
)
{
    GT_U32      device;     /* device index */
    GT_U32      devStatus = 0;  /* device status */

    SCIB_SEM_TAKE;
    for (device = 0; device < SMAIN_MAX_NUM_OF_DEVICES_CNS; device++)
    {
        /* skip not exist devices */
        if (!smainDeviceObjects[device])
        {
            continue;
        }

        /* skip devices without queues */
        if (smainDeviceObjects[device]->queueId == NULL)
        {
            continue;
        }

        /* get queue status */
        devStatus = squeStatusGet(smainDeviceObjects[device]->queueId);
        if (devStatus != 0)
        {
            /* device is busy */
            break;
        }

        if(smainDeviceObjects[device]->softResetOldDevicePtr)
        {
            /* we are under 'soft reset' :
               the new device already replaced the old device as pointer in
               smainDeviceObjects[device] ... but still not hold
               the 'queue' of the old device  */

            /* device is busy doing soft reset */
            devStatus = 1;
            break;
        }

        if(sRemoteTmUsed && smainDeviceObjects[device]->tmInfo.bufPool_DDR3_TM)
        {
            /* the device is connected to remote TM simulator */
            /* this pool may hold buffers that represents packets that still
               need to egress the device */
            devStatus = sbufAllocatedBuffersNumGet(smainDeviceObjects[device]->tmInfo.bufPool_DDR3_TM);
            if (devStatus != 0)
            {
                /* device is busy */
                break;
            }
        }
    }

    /* all devices are Idle */
    SCIB_SEM_SIGNAL;
    return devStatus;
}

/*******************************************************************************
*   skernelBitwiseOperator
*
* DESCRIPTION:
*       calculate bitwise operator result:
*       result =  (((value1Operator) on value1) value2Operator on value2)
*
* INPUTS:
*        value1  - value 1
*        value1Operator - operator to be done on value 1
*                       valid are:
*                       SKERNEL_BITWISE_OPERATOR_NONE_E
*                       SKERNEL_BITWISE_OPERATOR_NOT_E  (~value1)
*        value2  - value 2
*        value2Operator - operator to be done on value 1
*                       valid are:
*                       SKERNEL_BITWISE_OPERATOR_NONE_E -> (value 2 ignored !)
*                       SKERNEL_BITWISE_OPERATOR_XOR_E  -> ^ value2
*                       SKERNEL_BITWISE_OPERATOR_AND_E  -> & value2
*                       SKERNEL_BITWISE_OPERATOR_OR_E   -> | value2
*                       SKERNEL_BITWISE_OPERATOR_NOT_E  -> this is 'added' to on of 'xor/or/and'
*                                                       -> ^ (~vlaue2)
*                                                       -> & (~vlaue2)
*                                                       -> | (~vlaue2)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       the operator result
*
* COMMENTS:
*
*
*******************************************************************************/
GT_U32  skernelBitwiseOperator
(
    IN GT_U32                       value1,
    IN SKERNEL_BITWISE_OPERATOR_ENT value1Operator,

    IN GT_U32                       value2,
    IN SKERNEL_BITWISE_OPERATOR_ENT value2Operator
)
{
    GT_U32  result;
    GT_U32  useNotOnValue2;
    GT_U32  value1new;

    switch(value1Operator)
    {
        case SKERNEL_BITWISE_OPERATOR_NONE_E :/* no operation */
            value1new = value1;
            break;
        case SKERNEL_BITWISE_OPERATOR_NOT_E :/*NOT ~*/
            value1new = ~value1;
            break;
        default:
            skernelFatalError("skernelBitwiseOperator: value1Operator can not be [%d] \n",
                value1Operator);
            return 0;
    }

    if(value2Operator == SKERNEL_BITWISE_OPERATOR_NOT_E)
    {
        skernelFatalError("skernelBitwiseOperator: value2Operator can not be [%d] \n",
            value2Operator);
        return 0;
    }

    useNotOnValue2 = (value2Operator & SKERNEL_BITWISE_OPERATOR_NOT_E) ? 1 : 0;

    switch(value2Operator & (~SKERNEL_BITWISE_OPERATOR_NOT_E))
    {
        case SKERNEL_BITWISE_OPERATOR_NONE_E :/* no operation */
            result = value1new;
            break;
        case SKERNEL_BITWISE_OPERATOR_XOR_E:
            if(useNotOnValue2)
            {
                result = value1new ^ (~value2);
            }
            else
            {
                result = value1new ^ value2;
            }
            break;
        case SKERNEL_BITWISE_OPERATOR_AND_E:
            if(useNotOnValue2)
            {
                result = value1new & (~value2);
            }
            else
            {
                result = value1new & value2;
            }
            break;
        case SKERNEL_BITWISE_OPERATOR_OR_E :
            if(useNotOnValue2)
            {
                result = value1new | (~value2);
            }
            else
            {
                result = value1new | value2;
            }
            break;
        default:
            skernelFatalError("skernelBitwiseOperator: value2Operator can not be [%d] \n",
                value2Operator);
            result = 0;
            break;
    }


    return result;
}


/*******************************************************************************
*   skernelDebugFreeBuffersNumPrint
*
* DESCRIPTION:
*       debug function to print the number of free buffers.
*
* INPUTS:
*       devNum - device number as stated in the INI file.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None
*
* COMMENTS:
*
*
*******************************************************************************/
void skernelDebugFreeBuffersNumPrint
(
    IN  GT_U32    devNum
)
{
    SKERNEL_DEVICE_OBJECT* deviceObjPtr = smemTestDeviceIdToDevPtrConvert(devNum);
    SKERNEL_DEVICE_OBJECT* currDeviceObjPtr;
    GT_U32  dev;
    GT_U32  freeBuffersNum;

    if(deviceObjPtr->shellDevice == GT_TRUE)
    {
        simulationPrintf(" multi-core device [%d] \n",devNum);
        for(dev = 0 ; dev < deviceObjPtr->numOfCoreDevs ; dev++)
        {
            currDeviceObjPtr = deviceObjPtr->coreDevInfoPtr[dev].devObjPtr;
            freeBuffersNum = sbufFreeBuffersNumGet(currDeviceObjPtr->bufPool);

            simulationPrintf(" coreId[%d] with free buffers[%d] \n",dev,freeBuffersNum);
        }
    }
    else
    {
        freeBuffersNum = sbufFreeBuffersNumGet(deviceObjPtr->bufPool);

        simulationPrintf(" device[%d] with free buffers[%d] \n",devNum,freeBuffersNum);
    }



}

extern GT_PTR simOsTaskOwnTaskCookieGet(GT_VOID);
/*******************************************************************************
* skernelSleep
*
* DESCRIPTION:
*       Puts current task to sleep for specified number of millisecond.
*       this function needed instead of direct call to SIM_OS_MAC(simOsSleep)
*       because it allow the thread that calls this function to replace the 'old device'
*       with 'new device' as part of 'soft reset' processing.
*
* INPUTS:
*       devObjPtr      - the device that needs the sleep
*       timeInMilliSec - time to sleep in milliseconds
*
* OUTPUTS:
*       None
*
* RETURNS:
*       pointer to device object as the device object may have been replaces due
*       to 'soft reset' at this time
*
* COMMENTS:
*       None
*
*******************************************************************************/
SKERNEL_DEVICE_OBJECT  * skernelSleep
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_U32                   timeInMilliSec
)
{
    static GT_U32   numTicksFor1000Mili = 0;

    SKERNEL_DEVICE_OBJECT  * newDevObjPtr;
    GT_U32  ii,iiMax,timeModulo,timeMinimal = 100;
    GT_U32                 timeStart,timeEnd;

    if(devObjPtr == NULL)
    {
        skernelFatalError("skernelSleep: can't have NULL device \n");
        return NULL;
    }

    if(timeInMilliSec >= 1000)
    {
        if(numTicksFor1000Mili == 0)
        {
            SCIB_SEM_TAKE;
            if(numTicksFor1000Mili == 0)/*second check inside the 'SCIB LOCK' */
            {
                timeStart = SIM_OS_MAC(simOsTickGet)();
                SIM_OS_MAC(simOsSleep)(1000);
                timeEnd = SIM_OS_MAC(simOsTickGet)();

                numTicksFor1000Mili = timeEnd - timeStart;
                timeInMilliSec -= 1000;
            }
            SCIB_SEM_SIGNAL;


            /*simulationPrintf("numTicksFor1000Mili = [%d] \n",numTicksFor1000Mili);*/

            if(numTicksFor1000Mili == 0)
            {
                skernelFatalError("skernelSleep: can't calc numTicksFor1000Mili \n");
            }

        }
    }


    timeStart = SIM_OS_MAC(simOsTickGet)();
    timeModulo = timeInMilliSec % timeMinimal;
    iiMax = timeInMilliSec / timeMinimal;
    for(ii = 0 ; ii < iiMax; ii++)
    {
        if(devObjPtr->softResetNewDevicePtr)
        {
            timeModulo = 0;/* no need extra sleep as we want to do soft reset ASAP */
            break;
        }

        /* break sleep to not be more then 100 mili every time
           to allow 'soft reset' to modify device pointer */
        SIM_OS_MAC(simOsSleep)(timeMinimal);

        timeEnd = SIM_OS_MAC(simOsTickGet)();
        if(numTicksFor1000Mili && (((ii+1) != iiMax) || timeModulo))/* not last iteration or extra sleep needed after the loop */
        {
            if(((1000* (timeEnd - timeStart)) / numTicksFor1000Mili) > timeInMilliSec)
            {
                /* already sleep enough */
                timeModulo = 0;

#if 0 /*we not want continues printings in WIN32 ... */
                /* NOTE: it seems that in WIN32 we get here when ii is about 90% of iiMax !!! */
                /* NOTE: it seems that Linux is more accurate and we get here exactly at last iteration */
                simulationPrintf("skernelSleep: already sleep[%d]ms needed[%d]ms after[%d] out of[%d]\n",
                    ((1000* (timeEnd - timeStart)) / numTicksFor1000Mili),
                    timeInMilliSec,
                    (ii+1),
                    iiMax);
#endif
                break;
            }
        }
    }

    if(timeModulo)
    {
        SIM_OS_MAC(simOsSleep)(timeModulo);
    }

    /* check if need to replace old device with new device */
    /* are we in the middle of soft reset */
    if(devObjPtr->softResetNewDevicePtr)
    {
        /* see function skernelDeviceSoftResetGeneric(...) that will bind the old
           device with new device */

        newDevObjPtr = devObjPtr->softResetNewDevicePtr;

        SCIB_SEM_TAKE;
        devObjPtr->numThreadsOnMe--;
        SCIB_SEM_SIGNAL;

        while(newDevObjPtr->softResetOldDevicePtr)
        {
            /* wait for the new device to be fully ready (and detached from info from the old device) */
            SIM_OS_MAC(simOsSleep)(50);
            simulationPrintf("-");
        }

        /* modify taskOwnTaskCookie if required */
        {
            GT_UINTPTR  cookie;
            cookie = (GT_UINTPTR)simOsTaskOwnTaskCookieGet();
            if (    (cookie >= (GT_UINTPTR)devObjPtr) &&
                    (cookie < (GT_UINTPTR)devObjPtr + sizeof(*devObjPtr)))
            {
                /* cookie present and points to devObjPtr */
                SIM_OS_TASK_PURPOSE_TYPE_ENT purposeType;
                if (SIM_OS_MAC(simOsTaskOwnTaskPurposeGet)(&purposeType) == GT_OK)
                {
                    /* modify cookie to new value */
                    cookie -= (GT_UINTPTR)devObjPtr;
                    cookie += (GT_UINTPTR)newDevObjPtr;
                    SIM_OS_MAC(simOsTaskOwnTaskPurposeSet)(purposeType, (GT_PTR)cookie);
                }
            }
        }

        /* return the new device */
        return newDevObjPtr;
    }

    /* return original device */
    return devObjPtr;
}


/*******************************************************************************
*   skernelSoftResetTest
*
* DESCRIPTION:
*       debug function to test the 'soft reset'
*       function valid for SIP5 and above
* INPUTS:
*       devNum - device number as stated in the INI file.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None
*
* COMMENTS:
*
*
*******************************************************************************/
void skernelSoftResetTest
(
    IN  GT_U32    devNum
)
{
    SKERNEL_DEVICE_OBJECT* oldDeviceObjPtr = smemTestDeviceIdToDevPtrConvert(devNum);
    SKERNEL_DEVICE_OBJECT* newDeviceObjPtr;
    GT_U32                 timeStart,timeEnd;
    GT_U32                 regValue;
    GT_U32                 regAddr;
    GT_U32                 regAddrArray[5];
    GT_U32                 ii,iiMax = 5;
    CHT_AUQ_MEM   * auqMemPtr , origAuq;
    CHT2_FUQ_MEM  * fuqMemPtr;
    GT_U32                  dfxServerResetControlRegisterValue;
    GT_U32                  globalInterruptMaskRegRegAddr = 0x34;
    GT_U32                  numErrors = 0;

    /* disable the device from sending interrupts to the CPU
       NOTE: we will not need to restore it because no meaning to restore it
       without FULL interrupts tree.
    */
    regValue = 0;
    scibWriteMemory(devNum,globalInterruptMaskRegRegAddr,1,&regValue);

    auqMemPtr = SMEM_CHT_MAC_AUQ_MEM_GET(oldDeviceObjPtr);
    origAuq = *auqMemPtr;
    fuqMemPtr = SMEM_CHT_MAC_FUQ_MEM_GET(oldDeviceObjPtr);

    regAddr = SMEM_LION2_DFX_SERVER_RESET_CONTROL_REG(oldDeviceObjPtr);

    scibMemoryClientRegRead(devNum,SCIB_MEM_ACCESS_DFX_E,regAddr,1,&dfxServerResetControlRegisterValue);
    /*<MG Soft Reset Trigger> --> should be 1 'NO trigger' */
    if(SMEM_U32_GET_FIELD(dfxServerResetControlRegisterValue,1,1) == 0)
    {
        skernelFatalError("skernelSoftResetTest: <MG Soft Reset Trigger> --> should be 1 ('NO trigger') \n");
    }

    /* write to registers values before the soft reset */
    ii = 0;
    /* MG register */
    regAddrArray[ii++] = (SMEM_CHT_MAC_REG_DB_GET(oldDeviceObjPtr))->MG.interruptCoalescingConfig.interruptCoalescingConfig;
    /* TTI register */
    regAddrArray[ii++] = SMEM_LION3_TTI_INGR_TPID_TAG_TYPE_REG(oldDeviceObjPtr);
    /* L2i register */
    regAddrArray[ii++] = SMEM_LION3_L2I_BRIDGE_ACCESS_MATRIX_DEFAULT_REG(oldDeviceObjPtr);
    /* EQ register */
    regAddrArray[ii++] = SMEM_CHT2_IP_PROT_CPU_CODE_VALID_CONF_REG(oldDeviceObjPtr);
    /* TXQ register */
    regAddrArray[ii++] = SMEM_LION_STAT_EGRESS_MIRROR_REG(oldDeviceObjPtr,0);

    /* set the registers with value of their addresses */
    for(ii = 0 ; ii < iiMax ; ii ++)
    {
        /* check content of the register that it changed */
        regValue = regAddrArray[ii];
        scibWriteMemory(devNum,regAddrArray[ii],1,&regValue);
    }

    /* check content of the registers before the soft reset */
    for(ii = 0 ; ii < iiMax ; ii ++)
    {
        scibReadMemory(devNum,regAddrArray[ii],1,&regValue);

        if(regValue != regAddrArray[ii])
        {
            simulationPrintf(" the register ii=[%d] in address [0x%8.8x] not changed value to [0x%8.8x] before the 'soft reset' \n",
                ii,regAddrArray[ii],regValue);
            numErrors ++;
        }
    }

    /* take the time before the reset */
    timeStart = SIM_OS_MAC(simOsTickGet)();

    /*********************/
    /* trigger the reset */
    /*********************/

    /*<MG Soft Reset Trigger> --> set to 0 to trigger it */
    dfxServerResetControlRegisterValue &= ~ (1<<1);
    scibMemoryClientRegWrite(devNum,SCIB_MEM_ACCESS_DFX_E,regAddr,1,&dfxServerResetControlRegisterValue);

    oldDeviceObjPtr = NULL;/* stop using the old device */
#ifndef _WIN32 /* it seems that linux miss something */
    SIM_OS_MAC(simOsSleep)(100);
#endif /*_WIN32*/

    /********************************/
    /* wait until the reset is done */
    /********************************/
    while(skernelStatusGet())
    {
        SIM_OS_MAC(simOsSleep)(10);
        simulationPrintf("7");
    }


    /* take the time that reset took */
    timeEnd = SIM_OS_MAC(simOsTickGet)();

    simulationPrintf("\n");
    simulationPrintf("skernelSoftResetTest : start tick[%d] , end tick[%d] --> total ticks[%d] \n",
        timeStart,timeEnd,timeEnd-timeStart);

    /* get pointer to the new device */
    newDeviceObjPtr = smemTestDeviceIdToDevPtrConvert(devNum);

    /* check content of the registers that it changed */
    for(ii = 0 ; ii < iiMax ; ii ++)
    {
        scibReadMemory(devNum,regAddrArray[ii],1,&regValue);

        if(regValue == regAddrArray[ii])
        {
            simulationPrintf(" the register ii=[%d] in address [0x%8.8x] not changed value after the 'soft reset' \n",
                ii,regAddrArray[ii]);
            numErrors ++;
        }
    }

    /* check the AUQ that done reset and the FUQ */
    auqMemPtr = SMEM_CHT_MAC_AUQ_MEM_GET(newDeviceObjPtr);
    fuqMemPtr = SMEM_CHT_MAC_FUQ_MEM_GET(newDeviceObjPtr);

    if(origAuq.auqBaseValid)    /* the device did CPSS initialization */
    {
        if(
            auqMemPtr->auqBase              ||
            auqMemPtr->auqBaseValid         ||
            auqMemPtr->auqBaseSize          ||
            auqMemPtr->auqShadow            ||
            auqMemPtr->auqShadowValid       ||
            auqMemPtr->auqShadowSize        ||
            auqMemPtr->auqOffset            ||
            auqMemPtr->baseInit
        )
        {
            simulationPrintf(" AUQ was not reset properly \n ");
            numErrors ++;
        }

        if(
            fuqMemPtr->fuqBase              ||
            fuqMemPtr->fuqBaseValid         ||
            fuqMemPtr->fuqShadow            ||
            fuqMemPtr->fuqShadowValid       ||
            fuqMemPtr->fuqOffset            ||
            fuqMemPtr->baseInit             ||
            fuqMemPtr->fuqNumMessages
        )
        {
            simulationPrintf(" FUQ was not reset properly \n ");
            numErrors ++;
        }
    }

    /* just for the test :  make sure that interrupts will not cause any issue */
    regValue = 0xFFFFFF;
    scibWriteMemory(devNum,globalInterruptMaskRegRegAddr,1,&regValue);


    if(numErrors == 0)
    {
        simulationPrintf("\n\n\n **** TEST PASS :-) **** \n\n\n");
    }
    else
    {
        simulationPrintf("\n\n\n **** TEST FAILED :-( **** \n\n\n");
    }
}


/*******************************************************************************
*   skernelSoftResetTest
*
* DESCRIPTION:
*       multiple run of test : debug function to test the 'soft reset'
*       function valid for SIP5 and above
* INPUTS:
*       devNum - device number as stated in the INI file.
*       numOfRepetitions - number of repetitions.
* OUTPUTS:
*       None.
*
* RETURNS:
*       None
*
* COMMENTS:
*
*
*******************************************************************************/
void skernelSoftResetTest_multiple
(
    IN  GT_U32    devNum,
    IN GT_U32   numOfRepetitions
)
{
    GT_U32  ii;

    if(numOfRepetitions == 0)
    {
        numOfRepetitions = 1;
    }

    for(ii = 0 ; ii < numOfRepetitions ; ii++ )
    {
        skernelSoftResetTest(devNum);
        simulationPrintf("test[%d] ended \n",ii+1);
    }

    return;
}

/*******************************************************************************
*  smainReceivedPacketDoneFromTm
*
* DESCRIPTION:
*      'callback' function that will be called  when the 'packet done' message
*       is returned from the TM (for specific packetId).
*
* INPUTS:
*       simDeviceId  - device id.
*       cookiePtr    - the cookie that was attached to the packet
*       tmFinalPort  - the TM final port
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*        the function is called from 'UDP socket listener' task context.
*        so the CB function must not to 'sleep' or 'heavy' operations
*
*******************************************************************************/
static void smainReceivedPacketDoneFromTm
(
    IN GT_U32   simDeviceId,
    IN GT_VOID* cookiePtr,
    IN GT_U32   tmFinalPort
)
{
    SKERNEL_DEVICE_OBJECT   *devObjPtr;
    SBUF_BUF_ID             bufferId;    /* buffer id */
    GT_U32                  messageSize;   /*message size*/
    GT_U8  * dataPtr;         /* pointer to the data in the buffer */
    GT_U32 dataSize;          /* data size */
    GENERIC_MSG_FUNC genFunc = snetChtEgressAfterTmUnit;/* generic function */

    /* the sender that built this cookie is function startProcessInTmUnit(...) */
    /* we need to here the same logic */
    devObjPtr = smemTestDeviceIdToDevPtrConvert(simDeviceId);

    /* send the device a message to finish processing the packet that returned
       from the TM unit */

    messageSize = sizeof(GENERIC_MSG_FUNC) + sizeof(cookiePtr) + sizeof(tmFinalPort);

    /* allocate buffer from the 'destination' device pool */
    /* get the buffer and put it in the queue */
    bufferId = sbufAlloc(devObjPtr->bufPool, messageSize);
    if (bufferId == NULL)
    {
        simWarningPrintf(" smainReceivedPacketDoneFromTm : no buffers for process \n");
        return ;
    }

    /* Get actual data pointer */
    sbufDataGet(bufferId, (GT_U8 **)&dataPtr, &dataSize);

    /* put the name of the function into the message */
    memcpy(dataPtr,&genFunc,sizeof(GENERIC_MSG_FUNC));
    dataPtr+=sizeof(GENERIC_MSG_FUNC);

    /* save parameter 1 */
    memcpy(dataPtr,&cookiePtr,sizeof(cookiePtr));
    dataPtr+=sizeof(cookiePtr);
    /* save parameter 2 */
    memcpy(dataPtr,&tmFinalPort,sizeof(tmFinalPort));
    dataPtr+=sizeof(tmFinalPort);

    /* set source type of buffer                    */
    bufferId->srcType = SMAIN_SRC_TYPE_CPU_E;

    /* set message type of buffer                   */
    bufferId->dataType = SMAIN_MSG_TYPE_GENERIC_FUNCTION_E;

    /* put buffer to queue                          */
    squeBufPut(devObjPtr->queueId, SIM_CAST_BUFF(bufferId));
}

/*******************************************************************************
*   smainEmulateRemoteTm
*
* DESCRIPTION:
*       debug tool to be able to emulte remote TM for basic testing of
*       simulation that works with remote TM (udp sockets and asynchronous
*       messages).
*
* INPUTS:
*       simDeviceId     - Simulation device ID.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smainEmulateRemoteTm
(
    IN GT_U32  emulate
)
{
    emulateRemoteTm = emulate;
}

/*******************************************************************************
*   smainIsEmulateRemoteTm
*
* DESCRIPTION:
*       debug tool to be able to know if we emulte remote TM for basic testing of
*       simulation that works with remote TM (udp sockets and asynchronous
*       messages).
*
* INPUTS:
*       simDeviceId     - Simulation device ID.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       value != 0 indication that we     use emulation of TM unit.
*       value = 0  indication that we NOT use emulation of TM unit.
* COMMENTS:
*
*******************************************************************************/
GT_U32 smainIsEmulateRemoteTm
(
    void
)
{
    return emulateRemoteTm;
}


/*******************************************************************************
*   smainRemoteTmConnect
*
* DESCRIPTION:
*       create the needed sockets for TM device.
*       using UDP sockets (as used by the TM device)
*       and wait for the 'remote TM' to be ready.
*
* INPUTS:
*       simDeviceId     - Simulation device ID.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smainRemoteTmConnect
(
    IN GT_U32   simDeviceId
)
{
    SKERNEL_DEVICE_OBJECT   *devObjPtr;
    devObjPtr = smemTestDeviceIdToDevPtrConvert(simDeviceId);

    if(devObjPtr->supportTrafficManager == 0)
    {
        skernelFatalError("sMainRemoteTmConnect: device not supports traffic manager \n");
    }

    /* call to create sockets on 'our size' (as clients) */
    sRemoteTmCreateUdpSockets(devObjPtr->deviceId);

    if(emulateRemoteTm)
    {
        /* open 'server' sockets as emulation to the remote TM , with dummy implementations */
        sRemoteTmTest_emulateDummyTmDevice();
    }
    else
    {
        /* wait for the TM server to be ready */
        sRemoteTmWaitForTmServer(simDeviceId);
    }

}

/*******************************************************************************
*   smainGetDevicesString
*
* DESCRIPTION:
*       function to return the names of the devices.
*
* INPUTS:
*       devicesNamesString - (pointer to) the place to set the names of devices
*       sizeOfString    - number of bytes in devicesNamesString.
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*******************************************************************************/
void smainGetDevicesString
(
    IN char   *devicesNamesString,
    IN GT_U32   sizeOfString
)
{
    static char fullStringName[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
    static char tmp[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
    GT_U32      device;     /* device index */
    SKERNEL_DEVICE_OBJECT * devObjPtr;
    char*      deviceNamePtr;
    char*      revisionNamePtr;
    GT_U8       deviceRevisionId;
    GT_U32      offset = 0;
    GT_U32      instance = 0;
    GT_U32      ii;
    GT_U32      numOfCoreDevs;
    GT_BOOL     gmDevice;/* is GM device*/

    fullStringName[0] = 0;

    for (device = 0; device < SMAIN_MAX_NUM_OF_DEVICES_CNS; device++)
    {
        /* skip not exist devices */
        if (!smainDeviceObjects[device])
        {
            continue;
        }

        devObjPtr = smainDeviceObjects[device];
        if(devObjPtr->shellDevice == GT_TRUE)
        {
        }
        else if(devObjPtr->portGroupSharedDevObjPtr)/* this is core in multi-core device */
        {
            /* we represent the device by it's 'father' */
            continue;
        }
        else if(devObjPtr->deviceFamily == SKERNEL_NIC_FAMILY)
        {
            /* not really device */
            continue;
        }

        instance++;
    }



    instance = 0;

    for (device = 0; device < SMAIN_MAX_NUM_OF_DEVICES_CNS; device++)
    {
        /* skip not exist devices */
        if (!smainDeviceObjects[device])
        {
            continue;
        }

        devObjPtr = smainDeviceObjects[device];
        numOfCoreDevs = devObjPtr->numOfCoreDevs;

        if(devObjPtr->shellDevice == GT_TRUE)
        {
            /* get one of the cores */
            for(ii = 0 ; ii < devObjPtr->numOfCoreDevs ; ii++)
            {
                if(devObjPtr->coreDevInfoPtr[ii].devObjPtr)
                {
                    devObjPtr = devObjPtr->coreDevInfoPtr[ii].devObjPtr;
                    break;
                }
            }
        }
        else if(devObjPtr->portGroupSharedDevObjPtr)/* this is core in multi-core device */
        {
            /* we represent the device by it's 'father' */
            continue;
        }
        else if(devObjPtr->deviceFamily == SKERNEL_NIC_FAMILY)
        {
            /* not really device */
            continue;
        }

        deviceNamePtr = NULL;
        revisionNamePtr = NULL;
        deviceRevisionId = devObjPtr->deviceRevisionId;
        gmDevice = (devObjPtr->gmDeviceType == SMAIN_NO_GM_DEVICE) ? GT_FALSE : GT_TRUE;

        switch(devObjPtr->deviceFamily)
        {
            case    SKERNEL_SALSA_FAMILY:                        deviceNamePtr =  "salsa"        ;  break;
            case    SKERNEL_NIC_FAMILY:                          deviceNamePtr =  "nic"          ;  break;
            case    SKERNEL_COM_MODULE_FAMILY:                   deviceNamePtr =  "com_module"   ;  break;
            case    SKERNEL_TWIST_D_FAMILY:                      deviceNamePtr =  "twist_d"      ;  break;
            case    SKERNEL_TWIST_C_FAMILY:                      deviceNamePtr =  "twist_c"      ;  break;
            case    SKERNEL_SAMBA_FAMILY:                        deviceNamePtr =  "samba"        ;  break;
            case    SKERNEL_SOHO_FAMILY:                         deviceNamePtr =  "soho"         ;  break;
            case    SKERNEL_CHEETAH_1_FAMILY:                    deviceNamePtr =  "cheetah_1"    ;  break;
            case    SKERNEL_CHEETAH_2_FAMILY:                    deviceNamePtr =  "cheetah_2"    ;  break;
            case    SKERNEL_CHEETAH_3_FAMILY:                    deviceNamePtr =  "cheetah_3"    ;  break;
            case    SKERNEL_LION_PORT_GROUP_FAMILY:
                    deviceNamePtr =  "lion"         ;
                    revisionNamePtr = deviceRevisionId == 0 ? "A0 -- NOT supported !!!!":
                                      deviceRevisionId == 2 ? "B0" :
                                      NULL;
                    break;

            case    SKERNEL_XCAT_FAMILY:                         deviceNamePtr =  "xcat"         ;  break;
            case    SKERNEL_XCAT2_FAMILY:                        deviceNamePtr =  "xcat2"        ;  break;
            case    SKERNEL_TIGER_FAMILY:                        deviceNamePtr =  "tiger"        ;  break;
            case    SKERNEL_FA_FOX_FAMILY:                       deviceNamePtr =  "fa_fox"       ;  break;
            case    SKERNEL_FAP_DUNE_FAMILY:                     deviceNamePtr =  "fap_dune"     ;  break;
            case    SKERNEL_XBAR_CAPOEIRA_FAMILY:                deviceNamePtr =  "xbar_capoeira";  break;
            case    SKERNEL_FE_DUNE_FAMILY:                      deviceNamePtr =  "fe_dune"      ;  break;
            case    SKERNEL_PUMA_FAMILY:                         deviceNamePtr =  "puma"         ;  break;
            case    SKERNEL_EMBEDDED_CPU_FAMILY:                 deviceNamePtr =  "embedded_cpu" ;  break;

            case    SKERNEL_PHY_SHELL_FAMILY   :
            case    SKERNEL_PHY_CORE_FAMILY:
                                                                 deviceNamePtr =  "phy_shell"    ;  break;


            case    SKERNEL_MACSEC_FAMILY:                       deviceNamePtr =  "macsec"       ;  break;

            case    SKERNEL_LION2_PORT_GROUP_FAMILY:
                deviceNamePtr =  numOfCoreDevs == 8 ? "lion2" :
                                 numOfCoreDevs == 4 ? "hooper" :
                                 "lion2(unknown)";
                if(numOfCoreDevs == 8)
                {
                    revisionNamePtr = deviceRevisionId == 0 ? "A0 -- NOT supported !!!!" :
                                      deviceRevisionId == 1 ? "B0" :
                                      deviceRevisionId == 2 ? "B1" :
                                      NULL;
                }
                else/* Hooper */
                {
                    revisionNamePtr = deviceRevisionId == 0 ? "A0" :
                                      NULL;
                }

                break;


            case    SKERNEL_LION3_PORT_GROUP_FAMILY:             deviceNamePtr =  "lion3"        ;  break;


            case    SKERNEL_PUMA3_NETWORK_FABRIC_FAMILY:         deviceNamePtr =  "puma3"        ;  break;


            case    SKERNEL_BOBCAT2_FAMILY:
                deviceNamePtr =  "bobcat2"      ;
                revisionNamePtr = deviceRevisionId == 0 ? "A0" :
                                  deviceRevisionId == 1 ? "B0" :
                                  NULL;
                break;
            case SKERNEL_BOBK_CAELUM_FAMILY:
            case SKERNEL_BOBK_CETUS_FAMILY:
                if (devObjPtr->deviceFamily == SKERNEL_BOBK_CAELUM_FAMILY)
                {
                    deviceNamePtr =  "bobk-caelum (bobcat2)";
                }
                else
                {
                    deviceNamePtr =  "bobk-cetus (bobcat2)";
                }
                revisionNamePtr = deviceRevisionId == 0 ? "A0" :
                                  deviceRevisionId == 1 ? "B0" :
                                  NULL;
                break;
            case    SKERNEL_BOBCAT3_FAMILY:
                deviceNamePtr =  "bobcat3"      ;
                revisionNamePtr = deviceRevisionId == 0 ? "A0" :
                                  deviceRevisionId == 1 ? "B0" :
                                  NULL;

            default:
                break;
        }

        deviceNamePtr = devObjPtr->deviceName;
        if(deviceNamePtr == NULL)
        {
            continue;
        }

        if(revisionNamePtr)
        {
            sprintf(tmp,"%s{%s %s}",
                gmDevice == GT_TRUE ? "GM" : "" ,
                deviceNamePtr,
                revisionNamePtr);
        }
        else
        {
            sprintf(tmp,"%s{%s rev %d}",
                gmDevice == GT_TRUE ? "GM" : "" ,
                deviceNamePtr,
                deviceRevisionId);
        }

        if((strlen(tmp) + offset + 2) >= SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS)
        {
            /* can not add this name .. too long */
            break;
        }

        if(instance)
        {
            /* add "," */
            sprintf(fullStringName+offset,",%s",tmp);
        }
        else
        {
            /* add ":" */
            sprintf(fullStringName+offset,":%s",tmp);
        }

        /* re-calc the offset to end of string */
        offset = (GT_U32)strlen(fullStringName);

        instance++;


    }

    if(offset < sizeOfString)
    {
        strcpy(devicesNamesString,fullStringName);
    }
    else
    {
        strncpy(devicesNamesString,fullStringName,sizeOfString-1);

        devicesNamesString[sizeOfString-1] = 0;
    }


}

