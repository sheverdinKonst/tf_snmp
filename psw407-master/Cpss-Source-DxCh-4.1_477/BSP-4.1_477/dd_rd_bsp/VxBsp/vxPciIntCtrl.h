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
* vxPciIntCtrl.h - Header File for : PCI Interface interrupt controller
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       Main interrupt controller driver.
*
*******************************************************************************/

#ifndef __INCvxPciIntCtrllh
#define __INCvxPciIntCtrllh

/* includes */

#ifdef __cplusplus
extern "C" {
#endif

/* defines  */

/* typedefs */

/* PCI_ERR_CAUSE is an enumerator that describes the PCI error causes.      */
/* the enumerator describes the error cause bits positions in the IDMA      */
/* cause register.                                                          */

typedef enum _pciErrCause
{
    PCI_ERR_CAUSE_START= -1, /* Cause Start boundry                        	  */
    PCI_DP_ERR		,	/* Internal data path error 						  */
	PCI_S_WR_PERR 	,	/* PCI slave detection of bad write data parity.	  */
	PCI_S_RD_PERR 	,   /* PCIx_PERRn response to read data driven by slave   */
	PCI_RESERVED1	,	/* Reserved											  */
	PCI_M_IO_ERR   	,	/* I/O transaction that crosses DWORD boundary		  */
	PCI_M_WR_PERR 	,   /* PCIx_PERRn response to write data driven by master */
	PCI_M_RD_PERR	,	/* Bad data parity detection during a PCI master 	  */
						/* read transaction or bad parity detection during 	  */
						/* split completion to a write transaction 			  */
						/* initiated by the master							  */
	PCI_M_C_T_ABORT	,	/* Master generates Target Abort for split completion */
	PCI_M_M_ABORT	,	/* PCI master generation of master abort.			  */
	PCI_M_T_ABORT	,	/* PCI master detection of target abort.			  */
	PCI_M_DIS		,	/* Generate PCI transaction while master is disabled  */
	PCI_M_RETRY		,   /* PCI master reaching retry counter limit.			  */
	PCI_M_DISCARD	,	/* PCI master Split Completion Discard Timer expired  */
	PCI_M_UN_EXP	,	/* PCI master receives Split Completion which its 	  */
						/* byte count does no much the original request.	  */
	PCI_M_ERR_MSG	,	/* PCI master receiving Massage error.				  */
    PCI_RESERVED2	,	/* Reserved											  */
	PCI_S_C_M_ABORT	,	/* PCI slave generate master abort as a completer.	  */
	PCI_S_T_ABORT	,	/* PCI slave generate Target Abort.					  */
	PCI_S_C_T_ABORT	,	/* PCI slave as a completer, detection of target abort*/
	PCI_RESERVED3	,	/* Reserved											  */
	PCI_S_RD_BUF	,	/* PCI slave's read buffer, discard timer expires.	  */
	PCI_ARB			,	/* Internal arbiter detection of a 'broken' PCI master*/
	PCI_S_RETRY		,	/* PCI slave reaching the retry counter limit when 	  */
						/* attempting to generate a Split Completion.		  */
	PCI_S_C_DEST_RD	,	/* PCI slave Split Completion transaction from a 	  */
						/* non prefetchable BAR space, that is terminated 	  */
						/* with Master or Target abort.						  */
    PCI_BIST		,	/* PCI BIST Interrupt								  */
	PCI_PMG			,	/* PCI Power Management Interrupt					  */
	PCI_PRST		,	/* PCI Reset Assert									  */
	PCI_ERR_CAUSE_END   /* Cause End boundry                              	  */
} PCI_ERR_CAUSE;

/* vxPciIntCtrl.h API list */
void   vxPciIntCtrlInit(void);
STATUS vxPciIntConnect (MV_U32 pciIf, PCI_ERR_CAUSE cause, VOIDFUNCPTR routine,
                          int parameter, int prio);
STATUS vxPciIntEnable  (MV_U32 pciIf, PCI_ERR_CAUSE cause, MV_BOOL serrMask);
STATUS vxPciIntDisable (MV_U32 pciIf, PCI_ERR_CAUSE cause, MV_BOOL serrMask);
STATUS vxPciIntClear   (MV_U32 pciIf, PCI_ERR_CAUSE cause);

void   pciIntErrHandler(MV_U32 pciIf);       /* PCI error events handler */

#ifdef __cplusplus
}
#endif

#endif /* __INCvxPciIntCtrllh */

