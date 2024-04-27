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
* vxDramIntCtrl.h - Header File for : CPU Interface interrupt controller
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       Main interrupt controller driver.
*
*******************************************************************************/

#ifndef __INCvxDramIntCtrlh
#define __INCvxDramIntCtrlh

/* includes */


#ifdef __cplusplus
extern "C" {
#endif

/* defines  */

/* typedefs */

/* DRAM_ERR_CAUSE is an enumerator that describes the DRAM error causes.    */
/* the enumerator describes the error cause bits positions in the DRAM      */
/* cause register.                                                          */

typedef enum _dramErrCause
{
    DRAM_ERR_CAUSE_START= -1,   /* Cause Start boundry                      */
    DRAM_SINGLE_BIT	,	        /* Single bit ECC error threshold expired   */
    DRAM_DOUBLE_BIT	,	        /* Double bit ECC error detected			*/
    DRAM_DPERR      ,           /* Dunit internal data path parity error    */
    DRAM_ERR_CAUSE_END          /* Cause End boundry                        */
} DRAM_ERR_CAUSE;

/* vxIdmaIntCtrl.h API list */
void   vxDramIntCtrlInit(void);
STATUS vxDramIntConnect (DRAM_ERR_CAUSE cause, VOIDFUNCPTR routine,
                          int parameter, int prio);
STATUS vxDramIntEnable  (DRAM_ERR_CAUSE cause);
STATUS vxDramIntDisable (DRAM_ERR_CAUSE cause);
STATUS vxDramIntClear   (DRAM_ERR_CAUSE cause);

void dramIntErrHandler(void);     

#ifdef __cplusplus
}
#endif

#endif /* __INCvxDramIntCtrlh */

