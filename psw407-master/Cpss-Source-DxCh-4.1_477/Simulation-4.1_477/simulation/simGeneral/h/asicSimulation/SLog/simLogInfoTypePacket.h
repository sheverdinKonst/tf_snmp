/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simLogInfoTypePacket.h
*
* DESCRIPTION:
*       simulation logger packet info type functions
*
* FILE REVISION NUMBER:
*       $Revision: 9 $
*
*******************************************************************************/
#ifndef __simLogPacket_h__
#define __simLogPacket_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <asicSimulation/SKernel/smain/smain.h>
#include <os/simTypes.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahL2.h>

/* save packet descriptor */
#define SIM_LOG_PACKET_DESCR_SAVE                                   \
    {                                                               \
        SKERNEL_FRAME_CHEETAH_DESCR_STC *__descrPtrSavedPtr=NULL;   \
        SKERNEL_FRAME_CHEETAH_DESCR_STC __descrPtrSaved;            \
        if(simLogIsOpen())                                          \
        {                                                           \
            __descrPtrSaved = *descrPtr;                            \
            __descrPtrSavedPtr = &__descrPtrSaved;                  \
        }

/* log packet descriptor changes -
   (to be called to NOT terminate scope of SIM_LOG_PACKET_DESCR_SAVE) */
#define SIM_LOG_PACKET_DESCR_COMPARE_AND_KEEP_FUNCTION(func)                 \
        if(__descrPtrSavedPtr)                                  \
        {                                                       \
            simLogPacketDescrCompare(devObjPtr, __descrPtrSavedPtr, descrPtr, func);   \
        }

/* terminate scope of 'SIM_LOG_PACKET_DESCR_SAVE') */
#define SIM_LOG_PACKET_DESCR_COMPARE_END                 \
    }

/* log packet descriptor changes -
    (to be called when need terminate scope of 'SIM_LOG_PACKET_DESCR_SAVE') */
#define SIM_LOG_PACKET_DESCR_COMPARE(func)                      \
        SIM_LOG_PACKET_DESCR_COMPARE_AND_KEEP_FUNCTION(func)    \
        SIM_LOG_PACKET_DESCR_COMPARE_END



/* declare ports BMP */
#define SIM_LOG_TARGET_BMP_PORTS_DECLARE(portsBmpNamePtr)           \
    /* unique name for the pointer */                           \
    SKERNEL_PORTS_BMP_STC *__saved_obj_ptr_##portsBmpNamePtr=NULL; \
    /* unique name for the object */                            \
    SKERNEL_PORTS_BMP_STC __saved_obj_##portsBmpNamePtr

/* save ports BMP */
#define SIM_LOG_TARGET_BMP_PORTS_SAVE(portsBmpNamePtr)              \
    if(simLogIsOpen())                                          \
    {                                                           \
        __saved_obj_##portsBmpNamePtr = *portsBmpNamePtr;        \
        __saved_obj_ptr_##portsBmpNamePtr = &__saved_obj_##portsBmpNamePtr;\
    }

/* log packet descriptor changes */
#define SIM_LOG_TARGET_BMP_PORTS_COMPARE(portsBmpNamePtr)       \
    if(__saved_obj_ptr_##portsBmpNamePtr)                        \
    {                                                           \
        simLogPortsBmpCompare(devObjPtr,                        \
            __FILE__,                                           \
            functionNameString,/*the name of the calling function*/ \
            #portsBmpNamePtr,                                   \
            __saved_obj_ptr_##portsBmpNamePtr->ports,/*old*/     \
            portsBmpNamePtr->ports);/*new*/                     \
    }




/* declare ports BMP */
#define SIM_LOG_TARGET_ARR_PORTS_DECLARE(portsArrNamePtr)           \
    /* unique name for the pointer */                           \
    SKERNEL_PORTS_BMP_STC *__saved_obj_ptr_##portsArrNamePtr=NULL; \
    /* unique name for the object */                            \
    SKERNEL_PORTS_BMP_STC __saved_obj_##portsArrNamePtr

/* save ports BMP */
#define SIM_LOG_TARGET_ARR_PORTS_SAVE(portsArrNamePtr)          \
    if(simLogIsOpen())                                          \
    {                                                           \
        memcpy(__saved_obj_##portsArrNamePtr.ports,portsArrNamePtr,sizeof(SKERNEL_PORTS_BMP_STC));        \
        __saved_obj_ptr_##portsArrNamePtr = &__saved_obj_##portsArrNamePtr;\
    }

/* log packet descriptor changes */
#define SIM_LOG_TARGET_ARR_PORTS_COMPARE(portsArrNamePtr)       \
    if(__saved_obj_ptr_##portsArrNamePtr)                        \
    {                                                           \
        simLogPortsBmpCompare(devObjPtr,                        \
            __FILE__,                                           \
            functionNameString,/*the name of the calling function*/ \
            #portsArrNamePtr,                                   \
            __saved_obj_ptr_##portsArrNamePtr->ports,/*old*/     \
            portsArrNamePtr);/*new*/                            \
    }


/* declare Array of ports */
#define SIM_LOG_TARGET_ARR_DECLARE(arrNamePtr)                  \
    /* unique name for the pointer */                           \
    SKERNEL_PORTS_BMP_STC *__saved_obj_ptr_##arrNamePtr=NULL;   \
    /* unique name for the object */                            \
    SKERNEL_PORTS_BMP_STC __saved_obj_##arrNamePtr

/* save Array of ports */
#define SIM_LOG_TARGET_ARR_SAVE(arrNamePtr)                            \
    if(simLogIsOpen())                                                 \
    {                                                                  \
        /* loop on the array */                                        \
        GT_U32  _ii;                                                   \
        for(_ii = 0; _ii < SKERNEL_CHEETAH_EGRESS_MAX_PORT_CNS; _ii++) \
        {                                                              \
            if(arrNamePtr[_ii])                                        \
            {                                                          \
                /* the port is in the array --> set it to the BMP */   \
                SKERNEL_PORTS_BMP_ADD_PORT_MAC((&__saved_obj_##arrNamePtr),_ii);\
            }                                                          \
            else                                                        \
            {                                                           \
                /* the port is in NOT in the the array --> unset it to the BMP */    \
                SKERNEL_PORTS_BMP_DEL_PORT_MAC((&__saved_obj_##arrNamePtr),_ii);         \
            }                                                           \
        }                                                              \
        __saved_obj_ptr_##arrNamePtr = &__saved_obj_##arrNamePtr;      \
    }

/* log packet descriptor changes */
#define SIM_LOG_TARGET_ARR_COMPARE(arrNamePtr)                          \
    if(__saved_obj_ptr_##arrNamePtr)                                    \
    {                                                                   \
        SKERNEL_PORTS_BMP_STC _newBmp;                                  \
        /* loop on the array */                                         \
        GT_U32  _ii;                                                    \
        for(_ii = 0; _ii < SKERNEL_CHEETAH_EGRESS_MAX_PORT_CNS; _ii++)  \
        {                                                               \
            if(arrNamePtr[_ii])                                         \
            {                                                           \
                /* the port is in the array --> set it to the BMP */    \
                SKERNEL_PORTS_BMP_ADD_PORT_MAC((&_newBmp),_ii);         \
            }                                                           \
            else                                                        \
            {                                                           \
                /* the port is in NOT in the the array --> unset it to the BMP */    \
                SKERNEL_PORTS_BMP_DEL_PORT_MAC((&_newBmp),_ii);         \
            }                                                           \
        }                                                               \
        simLogPortsBmpCompare(devObjPtr,                                \
            __FILE__,                                                   \
            functionNameString,/*the name of the calling function*/     \
            #arrNamePtr,                                                \
            __saved_obj_ptr_##arrNamePtr->ports,/*old*/                 \
            _newBmp.ports);/*new*/                                      \
    }



/*******************************************************************************
* simLogPacketDescrCompare
*
* DESCRIPTION:
*       log changes between saved packet descriptor and given
*
* INPUTS:
*       devObjPtr - pointer to device object.
*       old       - old packet descriptor pointer
*       new       - new packet descriptor pointer
*       funcName  - pointer to function name
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID simLogPacketDescrCompare
(
    IN SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC *old,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC *new,
    IN GT_CHAR                         *funcName
);

/*******************************************************************************
* simLogPacketDescrFrameDump
*
* DESCRIPTION:
*       log frame dump
*
* INPUTS:
*       devObjPtr - pointer to device object
*       descr     - packet descriptor pointer
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID simLogPacketDescrFrameDump
(
    IN SKERNEL_DEVICE_OBJECT           const *devObjPtr,
    IN SKERNEL_FRAME_CHEETAH_DESCR_STC const *descrPtr
);

/* print packetCmd enum value */
GT_VOID simLogPacketDescrPacketCmdDump
(
    IN SKERNEL_DEVICE_OBJECT const *devObjPtr,
    IN SKERNEL_EXT_PACKET_CMD_ENT packetCmd
);
/* print cpu code enum value */
GT_VOID simLogPacketDescrCpuCodeDump
(
    IN SKERNEL_DEVICE_OBJECT const *devObjPtr,
    IN SNET_CHEETAH_CPU_CODE_ENT    cpuCode
);

/*******************************************************************************
* simLogPortsBmpCompare
*
* DESCRIPTION:
*       ports BMP changes between saved one (old) and given one (new)
*       dump to log the DIFF
*
* INPUTS:
*       devObjPtr - pointer to device object.
*       fileNameString - the name (string) of the calling file
*       functionNameString - the name (string) of the calling function
*       variableNamePtr - the name (string) of the variable
*       oldArr       - old ports BMP pointer (pointer to the actual words)
*       newArr       - new ports BMP pointer (pointer to the actual words)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID simLogPortsBmpCompare
(
    IN SKERNEL_DEVICE_OBJECT           *devObjPtr,
    IN GT_CHAR                         *fileNameString,
    IN GT_CHAR                         *functionNameString,
    IN GT_CHAR                         *variableNamePtr,
    IN GT_U32                           *oldArr,
    IN GT_U32                           *newArr
);


/*******************************************************************************
* simLogPacketDump
*
* DESCRIPTION:
*       log frame dump
*
* INPUTS:
*       devObjPtr -  pointer to device object
*       ingressDirection - indication for direction:
*                   GT_TRUE - ingress
*                   GT_FALSE - egress
*       portNum - port number of ingress/egress
*       startFramePtr -  start of packet
*       byteCount - number of bytes to dump
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
GT_VOID simLogPacketDump
(
    IN SKERNEL_DEVICE_OBJECT        const *devObjPtr,
    IN GT_BOOL                      ingressDirection,
    IN GT_U32                       portNum,
    IN GT_U8                        *startFramePtr,
    IN GT_U32                       byteCount
);

/*******************************************************************************
* simLogPacketFrameUnitSet
*
* DESCRIPTION:
*       Set the unit Id of current thread (own thread).
*
* INPUTS:
*       frameUnit -  the unit id
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
GT_VOID simLogPacketFrameUnitSet(
    IN SIM_LOG_FRAME_UNIT_ENT frameUnit
);

/*******************************************************************************
* simLogPacketFrameCommandSet
*
* DESCRIPTION:
*       Set the command of current thread (own thread).
*
* INPUTS:
*       frameCommand -  the command id
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
GT_VOID simLogPacketFrameCommandSet(
    IN SIM_LOG_FRAME_COMMAND_TYPE_ENT frameCommand
);

/*******************************************************************************
* simLogPacketFrameMyLogInfoGet
*
* DESCRIPTION:
*       Get the pointer to the log info of current thread (own thread)
*
* INPUTS:
*       None.
* OUTPUTS:
*       None.
*
* RETURNS:
*       pointer to the log info of current thread (own thread)
*       (may be NULL)
*
* COMMENTS:
*
*
*******************************************************************************/
SIM_LOG_FRAME_INFO_STC * simLogPacketFrameMyLogInfoGet(
    GT_VOID
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __simLogPacket_h__ */

