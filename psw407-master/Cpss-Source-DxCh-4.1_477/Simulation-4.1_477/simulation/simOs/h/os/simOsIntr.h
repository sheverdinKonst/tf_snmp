/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simOsIntr.h
*
* DESCRIPTION:
*       Operating System wrapper. Interrupt facility.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*******************************************************************************/

#ifndef EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES
    #error "include to those H files should be only for bind purposes"
#endif /*!EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES*/

#ifndef __simOsIntrh
#define __simOsIntrh

/************* Includes *******************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/************ Defines  ********************************************************/

/************* Functions ******************************************************/

/*******************************************************************************
* simOsInterruptSet
*
* DESCRIPTION:
*       Create OS Task/Thread and start it.
*
* INPUTS:
*       intr    - interrupt
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK   - Succeed
*       GT_FAIL - failed
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS simOsInterruptSet
(
    IN  GT_U32 intr
);
/*******************************************************************************
* simOsInitIntrTable
*
* DESCRIPTION:
*
*
* INPUTS:
*       None
*
* OUTPUTS:
*
* RETURNS:
*       None
*
* COMMENTS:
*       None
*
*******************************************************************************/
void simOsInitInterrupt
(
    void
);

#ifdef __cplusplus
}
#endif

#endif  /* __simOsIntrh */

