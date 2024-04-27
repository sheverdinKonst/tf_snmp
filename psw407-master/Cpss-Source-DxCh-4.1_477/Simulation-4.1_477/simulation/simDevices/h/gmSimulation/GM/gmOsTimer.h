/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* gmOsTimer.h
*
* DESCRIPTION:
*       Operating System wrapper. Timer facility.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/

#ifndef __gmOsTimerh
#define __gmOsTimerh

#ifdef __cplusplus
extern "C" {
#endif

/************* Includes *******************************************************/
#include <os/simTypes.h>
/*
#include <extServices/os/gtOs/gtGenTypes.h>
*/
/*******************************************************************************
* GM_OS_TIME_WK_AFTER_FUNC
*
* DESCRIPTION:
*       Puts current task to sleep for specified number of millisecond.
*
* INPUTS:
*       mils - time to sleep in milliseconds
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*GM_OS_TIME_WK_AFTER_FUNC)
(
    IN GT_U32 mils
);

/*******************************************************************************
* GM_OS_TIME_TICK_GET_FUNC
*
* DESCRIPTION:
*       Gets the value of the kernel's tick counter.
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       The tick counter value.
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_U32 (*GM_OS_TIME_TICK_GET_FUNC)
(
    void
);


/*******************************************************************************
* GM_OS_TIME_GET_FUNC
*
* DESCRIPTION:
*       Gets number of seconds passed since system boot
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       The second counter value.
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_U32 (*GM_OS_TIME_GET_FUNC)
(
    void
);

/* GM_OS_TIME_BIND_STC -
    structure that hold the "os time" functions needed be bound to gm.

*/
typedef struct{
    GM_OS_TIME_WK_AFTER_FUNC    osTimeWkAfterFunc;
    GM_OS_TIME_TICK_GET_FUNC    osTimeTickGetFunc;
    GM_OS_TIME_GET_FUNC         osTimeGetFunc;

}GM_OS_TIME_BIND_STC;


#ifdef __cplusplus
}
#endif

#endif  /* __gmOsTimerh */


