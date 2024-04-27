/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simOsBindOwn.h
*
* DESCRIPTION:
*       allow bind of the simOs functions to the simulation
*
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/

#ifndef EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES
    #error "include to those H files should be only for bind purposes"
#endif /*!EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES*/

#ifndef __simOsBindOwnh
#define __simOsBindOwnh

/************* Includes *******************************************************/

#include <os/simTypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* simOsFuncBindOwnSimOs
*
* DESCRIPTION:
*       the functions of simOs will be bound to the simulation
*
* INPUTS:
*       None
*
* OUTPUTS:
*
* RETURNS:
*       none
*
* COMMENTS:
*       None
*
*******************************************************************************/
void simOsFuncBindOwnSimOs
(
    void
);

/*******************************************************************************
* simOsTaskOwnTaskPurposeSet
*
* DESCRIPTION:
*       Sets type of the thread.
*
* INPUTS:
*       type       - task type
*       cookiePtr  - cookie pointer
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK      - success
*       GT_FAIL    - fail, should never happen
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS simOsTaskOwnTaskPurposeSet
(
    IN SIM_OS_TASK_PURPOSE_TYPE_ENT      type,
    IN SIM_OS_TASK_COOKIE_INFO_STC*      cookiePtr
);

/*******************************************************************************
* simOsTaskOwnTaskPurposeGet
*
* DESCRIPTION:
*       Gets type of the thread.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       type       - task type
*
* RETURNS:
*       GT_OK      - success
*       GT_FAIL    - fail, should never happen
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS simOsTaskOwnTaskPurposeGet
(
    OUT SIM_OS_TASK_PURPOSE_TYPE_ENT   *type
);


#ifdef __cplusplus
}
#endif

#endif  /* __simOsBindOwnh */

