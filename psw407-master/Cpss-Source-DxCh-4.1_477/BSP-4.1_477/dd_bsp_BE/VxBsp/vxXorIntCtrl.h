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
* vxIdmaIntCtrl.h - Header File for : XOR interrupt controller driver
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       Main interrupt controller driver.
*
*******************************************************************************/

#ifndef __INCvxXorIntCtrlh
#define __INCvxXorIntCtrlh

/* includes */


#ifdef __cplusplus
extern "C" {
#endif

/* defines  */

/* typedefs */

/* XOR_ERR_CAUSE is an enumerator that describes the XOR various causes.   	*/
/* the enumerator describes the error cause bits positions in the XOR      	*/
/* cause register.                                                          */
/* NOTE:                                                                    */
/* Eventhough this driver supports connecting the XOR completion event,    	*/
/* the user can connect to the XOR completion bit using the Main           	*/
/* Interupt controller (VxIntConnect), thus saving the calling to this      */
/* driver handler. Note that the completion interrupt is acknowledged       */
/* in the XOR cause register and not in the main cause register, so the    	*/
/* user ISR must use this driver interrupt clear routine vxXorIntClear ()  	*/

typedef enum _xorErrCause
{
    XOR_ERR_CAUSE_START= -1, /* Cause Start boundry                        	*/
    XOR_EOD      ,  /* Channel finished transfer of the current desc  		*/
    XOR_EOC      ,  /* Channel finished transfer of a complete chain		*/
	XOR_STOPPED  ,	/* XOR Engine completed stopping routine				*/
	XOR_PAUSED   ,	/* XOR Engine completed Pausing routine					*/
	XOR_ADDR_DEC ,	/* Failed address decoding. Address is not in any
						   window or matches more than one window.			*/
    XOR_ACCPROT  ,  /* Channel Access Protect Violation               		*/
    XOR_WRPROT   ,  /* Channel Write Protect                          		*/
    XOR_OWN      ,  /* Channel Descriptor Ownership Violation         
                           Attempt to access descriptor owned by the CPU. 	*/
    XOR_PAR_ERR	 ,	/* Parity error. caused by internal buffer read			*/
	XOR_XBAR_ERR ,  /* Crossbar Parity error. Caused by erroneous read 	
						   response from the Crossbar.						*/
    XOR_ERR_CAUSE_END    /* Cause End boundry                              	*/
} XOR_ERR_CAUSE;


/* vxXorIntCtrl.h API list */
void   vxXorIntCtrlInit(void);
STATUS vxXorIntConnect (MV_U32 chan, XOR_ERR_CAUSE cause,
                         VOIDFUNCPTR routine, int arg1, int arg2, int prio);
STATUS vxXorIntEnable  (MV_U32 chan, XOR_ERR_CAUSE cause);
STATUS vxXorIntDisable (MV_U32 chan, XOR_ERR_CAUSE cause);
STATUS vxXorIntClear   (MV_U32 chan, XOR_ERR_CAUSE cause);

void xorIntErrHandler (UINT32 chan);

#ifdef __cplusplus
}
#endif

#endif /* __INCvxXorIntCtrlh */

