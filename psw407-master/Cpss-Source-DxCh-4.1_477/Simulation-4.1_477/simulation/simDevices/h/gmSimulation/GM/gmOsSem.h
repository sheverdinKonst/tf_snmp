/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* gmOsSem.h
*
* DESCRIPTION:
*       Operating System wrapper. Semaphore facility.
*
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*******************************************************************************/

#ifndef __gmOsSemh
#define __gmOsSemh

#ifdef __cplusplus
extern "C" {
#endif

/************* Includes *******************************************************/
#include <os/simTypes.h>
/*
#include <extServices/os/gtOs/gtGenTypes.h>
*/

/************* Typedefs *******************************************************/

/* semaphore for protecting resource */
typedef void * GM_OS_MUTEX_SEM;

/* semaphore for signaling between tasks */
typedef void * GM_OS_SIG_SEM;

/*
 * Typedef: enumeration GM_OS_SEMB_STATE_ENT
 *
 * Description: An enumeration for semaphore creation state
 *
 * Fields:
 *       GM_OS_SEMB_EMPTY_E - create the semaphore empty
 *                              a user must wait for the semaphore to be
 *                              signaled before it can take it
 *       GM_OS_SEMB_FULL_E  - create the semaphore full
 *                              a user not need to wait for a signaled before it
 *                              can lock the semaphore
 *
 */
typedef enum
{
    GM_OS_SEMB_EMPTY_E = 0,
    GM_OS_SEMB_FULL_E
}GM_OS_SEMB_STATE_ENT;


/************* Defines ********************************************************/

/*******************************************************************************
* GM_OS_MUTEX_SEM_BIN_CREATE_FUNC
*
* DESCRIPTION:
*       Create and initialize a binary semaphore , used as "mutex" (mutual
*       exclusion).
*       This semaphore used for protecting resources in HW or in SW , that
*       accessed from more then one SW task.
*
* INPUTS:
*       name   - "mutex" semaphore Name
*       init   - init value of "mutex" semaphore (full or empty)
*
* OUTPUTS:
*       smidPtr - (pointer to)"mutex" semaphore Id
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       see also :
*       1. GM_OS_MUTEX_SEM_BIN_CREATE_FUNC
*       2. GM_OS_MUTEX_SEM_DELETE_FUNC
*       3. GM_OS_MUTEX_SEM_WAIT_FUNC
*       4. GM_OS_MUTEX_SEM_SIGNAL_FUNC
*
*******************************************************************************/
typedef GT_STATUS (*GM_OS_MUTEX_SEM_BIN_CREATE_FUNC)
(
    IN  char                    *name,
    IN  GM_OS_SEMB_STATE_ENT  init,
    OUT GM_OS_MUTEX_SEM       *smidPtr
);

/*******************************************************************************
* GM_OS_MUTEX_SEM_DELETE_FUNC
*
* DESCRIPTION:
*       Delete "mutex" semaphore.
*
* INPUTS:
*       smid - "mutex" semaphore Id
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       see also :
*       1. GM_OS_MUTEX_SEM_BIN_CREATE_FUNC
*       2. GM_OS_MUTEX_SEM_DELETE_FUNC
*       3. GM_OS_MUTEX_SEM_WAIT_FUNC
*       4. GM_OS_MUTEX_SEM_SIGNAL_FUNC
*
*******************************************************************************/
typedef GT_STATUS (*GM_OS_MUTEX_SEM_DELETE_FUNC)
(
    IN GM_OS_MUTEX_SEM smid
);

/*******************************************************************************
* GM_OS_MUTEX_SEM_WAIT_FUNC
*
* DESCRIPTION:
*       Wait on "mutex" semaphore.
*
* INPUTS:
*       smid    - "mutex" semaphore Id
*       timeOut - time out in milliseconds or 0 to wait forever
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*       GT_TIMEOUT - on time out
*
* COMMENTS:
*       see also :
*       1. GM_OS_MUTEX_SEM_BIN_CREATE_FUNC
*       2. GM_OS_MUTEX_SEM_DELETE_FUNC
*       3. GM_OS_MUTEX_SEM_WAIT_FUNC
*       4. GM_OS_MUTEX_SEM_SIGNAL_FUNC
*
*******************************************************************************/
typedef GT_STATUS (*GM_OS_MUTEX_SEM_WAIT_FUNC)
(
    IN GM_OS_MUTEX_SEM  smid,
    IN GT_U32       timeOut
);

/*******************************************************************************
* GM_OS_MUTEX_SEM_SIGNAL_FUNC
*
* DESCRIPTION:
*       Signal a "mutex" semaphore.
*
* INPUTS:
*       smid    - "mutex" semaphore Id
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       see also :
*       1. GM_OS_MUTEX_SEM_BIN_CREATE_FUNC
*       2. GM_OS_MUTEX_SEM_DELETE_FUNC
*       3. GM_OS_MUTEX_SEM_WAIT_FUNC
*       4. GM_OS_MUTEX_SEM_SIGNAL_FUNC
*
*******************************************************************************/
typedef GT_STATUS (*GM_OS_MUTEX_SEM_SIGNAL_FUNC)
(
    IN GM_OS_MUTEX_SEM smid
);

/*******************************************************************************
* GM_OS_SIG_SEM_BIN_CREATE_FUNC
*
* DESCRIPTION:
*       Create and initialize a binary semaphore for signaling between tasks or
*       between Interrupt context and tasks.
*
* INPUTS:
*       name   - "signaling" semaphore Name
*       init   - init value of "signaling" semaphore (full or empty)
*
* OUTPUTS:
*       smidPtr - (pointer to)semaphore "signaling" Id
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       see also :
*       1. GM_OS_SIG_SEM_BIN_CREATE_FUNC
*       2. GM_OS_SIG_SEM_DELETE_FUNC
*       3. GM_OS_SIG_SEM_WAIT_FUNC
*       4. GM_OS_SIG_SEM_SIGNAL_FUNC
*
*******************************************************************************/
typedef GT_STATUS (*GM_OS_SIG_SEM_BIN_CREATE_FUNC)
(
    IN  char                    *name,
    IN  GM_OS_SEMB_STATE_ENT  init,
    OUT GM_OS_SIG_SEM         *smidPtr
);

/*******************************************************************************
* GM_OS_SIG_SEM_DELETE_FUNC
*
* DESCRIPTION:
*       Delete "signaling" semaphore.
*
* INPUTS:
*       smid - semaphore Id (as created by
*                 GM_OS_SIG_SEM_BIN_CREATE_FUNC)
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       see also :
*       1. GM_OS_SIG_SEM_BIN_CREATE_FUNC
*       2. GM_OS_SIG_SEM_DELETE_FUNC
*       3. GM_OS_SIG_SEM_WAIT_FUNC
*       4. GM_OS_SIG_SEM_SIGNAL_FUNC
*
*******************************************************************************/
typedef GT_STATUS (*GM_OS_SIG_SEM_DELETE_FUNC)
(
    IN GM_OS_SIG_SEM smid
);

/*******************************************************************************
* GM_OS_SIG_SEM_WAIT_FUNC
*
* DESCRIPTION:
*       Wait on "signaling" semaphore.
*
* INPUTS:
*       smid    - "signaling" semaphore Id (as created by
*                 GM_OS_SIG_SEM_BIN_CREATE_FUNC)
*       timeOut - time out in milliseconds or 0 to wait forever
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*       GT_TIMEOUT - on time out
*
* COMMENTS:
*       see also :
*       1. GM_OS_SIG_SEM_BIN_CREATE_FUNC
*       2. GM_OS_SIG_SEM_DELETE_FUNC
*       3. GM_OS_SIG_SEM_WAIT_FUNC
*       4. GM_OS_SIG_SEM_SIGNAL_FUNC
*
*******************************************************************************/
typedef GT_STATUS (*GM_OS_SIG_SEM_WAIT_FUNC)
(
    IN GM_OS_SIG_SEM  smid,
    IN GT_U32       timeOut
);

/*******************************************************************************
* GM_OS_SIG_SEM_SIGNAL_FUNC
*
* DESCRIPTION:
*       Signal a "signaling" semaphore.
*
* INPUTS:
*       smid    - "signaling" semaphore Id (as created by
*                 GM_OS_SIG_SEM_BIN_CREATE_FUNC)
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       see also :
*       1. GM_OS_SIG_SEM_BIN_CREATE_FUNC
*       2. GM_OS_SIG_SEM_DELETE_FUNC
*       3. GM_OS_SIG_SEM_WAIT_FUNC
*       4. GM_OS_SIG_SEM_SIGNAL_FUNC
*
*******************************************************************************/
typedef GT_STATUS (*GM_OS_SIG_SEM_SIGNAL_FUNC)
(
    IN GM_OS_SIG_SEM smid
);


/* GM_OS_SEM_BIND_STC -
    structure that hold the "os semaphore" functions needed be bound to gm.

*/
typedef struct{
    GM_OS_MUTEX_SEM_BIN_CREATE_FUNC   osMutexSemBinCreateFunc;
    GM_OS_MUTEX_SEM_DELETE_FUNC       osMutexSemDeleteFunc;
    GM_OS_MUTEX_SEM_WAIT_FUNC         osMutexSemWaitFunc;
    GM_OS_MUTEX_SEM_SIGNAL_FUNC       osMutexSemSignalFunc;

    GM_OS_SIG_SEM_BIN_CREATE_FUNC     osSigSemBinCreateFunc;
    GM_OS_SIG_SEM_DELETE_FUNC         osSigSemDeleteFunc;
    GM_OS_SIG_SEM_WAIT_FUNC           osSigSemWaitFunc;
    GM_OS_SIG_SEM_SIGNAL_FUNC         osSigSemSignalFunc;
}GM_OS_SEM_BIND_STC;

#ifdef __cplusplus
}
#endif

#endif  /* __gmOsSemh */


