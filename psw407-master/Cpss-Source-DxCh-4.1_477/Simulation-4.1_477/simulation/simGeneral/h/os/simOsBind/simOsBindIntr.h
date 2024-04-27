/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simOsBindIntr.h
*
* DESCRIPTION:
*       Operating System wrapper. Interrupt facility.
*
*       simOsInterruptSet           SIM_OS_INTERRUPT_SET_FUN
*       simOsInitInterrupt          SIM_OS_INIT_INTERRUPT_FUN
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/

#ifndef __simOsBindIntrh
#define __simOsBindIntrh

/************* Includes *******************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/************ Defines  ********************************************************/

/************* Functions ******************************************************/

/*******************************************************************************
* SIM_OS_INTERRUPT_SET_FUN
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
typedef GT_STATUS (*SIM_OS_INTERRUPT_SET_FUN)
(
    IN  GT_U32 intr
);
/*******************************************************************************
* SIM_OS_INIT_INTERRUPT_FUN
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
typedef void (*SIM_OS_INIT_INTERRUPT_FUN)
(
    void
);

/* SIM_OS_FUNC_BIND_INTERRUPTS_STC -
*    structure that hold the "os interrupts" functions needed be bound to SIM.
*
*/
typedef struct{
    SIM_OS_INTERRUPT_SET_FUN      simOsInterruptSet;    /* needed only on application side */
    SIM_OS_INIT_INTERRUPT_FUN     simOsInitInterrupt;   /* needed only on application side */
}SIM_OS_FUNC_BIND_INTERRUPTS_STC;

#ifndef DEVICES_SIDE_ONLY
extern   SIM_OS_INTERRUPT_SET_FUN      SIM_OS_MAC(simOsInterruptSet);
extern   SIM_OS_INIT_INTERRUPT_FUN     SIM_OS_MAC(simOsInitInterrupt);
#endif /*!DEVICES_SIDE_ONLY*/

#ifdef __cplusplus
}
#endif

#endif  /* __simOsBindIntrh */

