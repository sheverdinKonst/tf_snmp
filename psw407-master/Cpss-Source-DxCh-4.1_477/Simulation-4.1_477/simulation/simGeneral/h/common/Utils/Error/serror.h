/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* serror.h
*
* DESCRIPTION:
*
*       Fatal error function API
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*
*******************************************************************************/
#ifndef __serrorh
#define __serrorh

#include <os/simTypes.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void (* SUTILS_FATAL_ERROR_FUN)
(
    IN char * messagePtr
);
/*******************************************************************************
*   sUtilsFatalErrorBind
*
* DESCRIPTION:
*       Bind specific fatal error function.
*
* INPUTS:
*       fatalErrorFunPtr - entry point to the fatal error function.
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
void sUtilsFatalErrorBind
(
    IN SUTILS_FATAL_ERROR_FUN fatalErrorFunPtr
);
/*******************************************************************************
*   sUtilsFatalError
*
* DESCRIPTION:
*       Fatal error handler for Simulation's common libs
*
* INPUTS:
*       format - format for printing.
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
void sUtilsFatalError
(
    IN char * format, ...
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __serrorh */

