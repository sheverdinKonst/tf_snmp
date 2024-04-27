/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* gmExtServices.h
*
* DESCRIPTION:
*       External Driver wrapper. definitions for bind OS , external driver
*       dependent services to GM .
*
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*******************************************************************************/

#ifndef __gmExtServicesh
#define __gmExtServicesh

#ifdef __cplusplus
extern "C" {
#endif

/************* Includes *******************************************************/
/*#include "gtGenTypes.h"*/
#include "gmOsMem.h"
#include "gmOsTimer.h"
#include "gmOsSem.h"
#include "gmOsTask.h"
#include "gmOsIo.h"

#include "gmExtTcam.h"

/************* functions ******************************************************/

/* GM_OS_FUNC_BIND_STC -
*    structure that hold the "os" functions needed be bound to gm.
*
*       osMemBindInfo -  set of call back functions -
*                        CPU memory manipulation
*       osSemBindInfo - set of call back functions -
*                           semaphore manipulation
*       osIoBindInfo - set of call back functions -
*                           I/O manipulation
*       osTimeBindInfo - set of call back functions -
*                           time manipulation
*       osTaskBindInfo - set of call back functions -
*                           tasks manipulation
*/
typedef struct{
    GM_OS_MEM_BIND_STC  osMemBindInfo;
    GM_OS_SEM_BIND_STC  osSemBindInfo;
    GM_OS_IO_BIND_STC   osIoBindInfo;
    GM_OS_TIME_BIND_STC osTimeBindInfo;
    GM_OS_TASK_BIND_STC osTaskBindInfo;
}GM_OS_FUNC_BIND_STC;


/************* functions ******************************************************/
/*******************************************************************************
* gmExtServicesBindOs
*
* DESCRIPTION:
*       bind the gm with OS  functions.
* INPUTS:
*       osFuncBindPtr - (pointer to) set of call back functions
*
* OUTPUTS:
*       none
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS   gmExtServicesBindOs(
    IN GM_OS_FUNC_BIND_STC        *osFuncBindPtr
);

/*******************************************************************************
* gmExtServicesBindExtTcam
*
* DESCRIPTION:
*       bind the gm with external TCAM  functions.
* INPUTS:
*       extTcamFuncBindPtr - (pointer to) set of call back functions
*
* OUTPUTS:
*       none
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS   gmExtServicesBindExtTcam(
    IN GM_EXT_TCAM_FUNC_BIND_STC        *extTcamFuncBindPtr
);

/*******************************************************************************
* initExtTcamParams
*
* DESCRIPTION:
*       Init external TCAM parameters according to current simulation.
* INPUTS:
*       extTcamFuncBind - pointer of GM_EXT_TCAM_FUNC_BIND_STC
*
* OUTPUTS:
*       none
*
* RETURNS:
*       None
*
* COMMENTS:
*
*******************************************************************************/
void initExtTcamParams(GM_EXT_TCAM_FUNC_BIND_STC* extTcamFuncBind);

#ifdef __cplusplus
}
#endif

#endif  /* __gmExtServicesh */


