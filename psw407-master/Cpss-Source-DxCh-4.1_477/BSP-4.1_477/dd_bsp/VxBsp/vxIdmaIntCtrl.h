/*******************************************************************************
*                Copyright 2004, MARVELL SEMICONDUCTOR, LTD.                   *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.                      *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
*                                                                              *
* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, *
* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL    *
* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.  *
* (MJKK), MARVELL ISRAEL LTD. (MSIL).                                          *
*******************************************************************************/
/*******************************************************************************
* vxIdmaIntCtrl.h - Header File for : IDMA interrupt controller driver
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       Main interrupt controller driver.
*
*******************************************************************************/

#ifndef __INCvxIdmaIntCtrlh
#define __INCvxIdmaIntCtrlh

/* includes */

#ifdef __cplusplus
extern "C" {
#endif

/* defines  */

/* typedefs */

/* IDMA_ERR_CAUSE is an enumerator that describes the IDMA error causes.    */
/* the enumerator describes the error cause bits positions in the IDMA      */
/* cause register.                                                          */
/* NOTE:                                                                    */
/* Eventhough this driver supports connecting the IDMA completion event,    */
/* the user can connect to the IDMA completion bit using the Main           */
/* Interupt controller (VxIntConnect), thus saving the calling to this      */
/* driver handler. Note that the completion interrupt is acknowledged       */
/* in the IDMA cause register and not in the main cause register, so the    */
/* user ISR must use this driver interrupt clear routine vxIdmaIntClear ()  */

typedef enum _idmaErrCause
{
    IDMA_ERR_CAUSE_START= -1, /* Cause Start boundry                        */
    IDMA_COMP     = 0,    /* Channel completion                             */
    IDMA_ADDRMISS = 1,    /* Channel Address Miss. Failed address decoding. */
    IDMA_ACCPROT  = 2,    /* Channel Access Protect Violation               */
    IDMA_WRPROT   = 3,    /* Channel Write Protect                          */
    IDMA_OWN      = 4,    /* Channel Descriptor Ownership Violation         
                             Attempt to access descriptor owned by the CPU. */
    IDMA_EOTINT   = 5,    /* End Of Transfer interrupt                      */
    IDMA_DPERR    = 7,    /* Aunit internal data path parity error detected.*/
    IDMA_ERR_CAUSE_END    /* Cause End boundry                              */
} IDMA_ERR_CAUSE;

/* vxIdmaIntCtrl.h API list */
void   vxIdmaIntCtrlInit(void);
STATUS vxIdmaIntConnect (MV_U32 chan, IDMA_ERR_CAUSE cause,
                         VOIDFUNCPTR routine, int arg1, int arg2, int prio);
STATUS vxIdmaIntEnable  (MV_U32 chan, IDMA_ERR_CAUSE cause);
STATUS vxIdmaIntDisable (MV_U32 chan, IDMA_ERR_CAUSE cause);
STATUS vxIdmaIntClear   (MV_U32 chan, IDMA_ERR_CAUSE cause);

void idmaErrHandler(void);         /* IDMA error events handler */

#ifdef __cplusplus
}
#endif

#endif /* __INCvxIdmaIntCtrlh */

