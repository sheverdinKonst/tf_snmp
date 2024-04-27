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
* vxPexIntCtrl.h - Header File for : PCI Interface interrupt controller
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       Main interrupt controller driver.
*
*******************************************************************************/

#ifndef __INCvxPexIntCtrllh
#define __INCvxPexIntCtrllh

/* includes */

#ifdef __cplusplus
extern "C" {
#endif

/* defines  */

/* typedefs */

/* PEX_CAUSE is an enumerator that describes the PEX interrupt events.     	*/
/* the enumerator describes the event cause bits positions in the PEX       */
/* cause register.                                                          */

typedef enum _pexCause
{
    PEX_CAUSE_START= -1 	, /* Cause Start boundry                        	  */
	PEX_TX_REQ_IN_DLDOWN_ERR,/*0 Receive TxReq while PEX is in Dl-Down  */
	PEX_M_DIS		    	, /*1 Generate PEX transaction while master disabled   */
	PEX_RESERVED2			,
	PEX_ERR_WR_TO_REG		, /*3 Erroneous write attempt to PEX internal register */
	PEX_HIT_DFLT_WIN_ERR	, /*4 Hit Default Window Error 	  					  */
	PEX_RESERVED5			,
	PEX_RX_RAM_PAR_ERR		, /*6  Rx RAM Parity Error  */
	PEX_TX_RAM_PAR_ERR		, /*7  Tx RAM Parity Error  */
	PEX_COR_ERR_DET			, /*8 Correctable Error Detected				  	      */
	PEX_N_F_ERR_DET			, /*9 Non-Fatal Error Detected						  */
	PEX_F_ERR_DET			, /*10 Fatal Error Detected	  						  */
	PEX_D_STATE_CHANGE		, /*11 Dstate Change Indication. Endpoint only.         */
	PEX_BIST				, /*12 PCI-Express BIST activated						  */
	PEX_RESERVED13			,
	PEX_FLOW_CTRL_PROTOCOL	, /*14 Flow Control Protocol Error. */
	PEX_RCV_UR_CA_ERR 		, /*15 Received either UR or CA completion status. */
	PEX_RCV_ERR_FATAL		, /*16 Received ERR_FATAL message. Root Complex only.	  */
	PEX_RCV_ERR_NON_FATAL	, /*17 Received ERR_NONFATAL message. Root Complex only.*/
	PEX_RCV_ERR_COR			, /*18 Received ERR_COR message. Root Complex only. 	  */
	PEX_RCV_CRS		    	, /*19 Received CRS completion status					  */
	PEX_PEX_SLV_HOT_RESET	, /*20 Received Hot Reset Indication. Endpoint only. 	  */
    PEX_PEX_SLV_DIS_LINK	, /*21 Slave Disable Link Indication. Endpoint only.	  */
	PEX_PEX_SLV_LB			, /*22 Slave Loopback Indication					      */
	PEX_PEX_LINK_FAIL		, /*23 Link Failure indication						  */
    PEX_RCV_INT_A   		, /*24 Set when IntA_Assert message received.           */
	PEX_RCV_INT_B   		, /*25 Set when IntB_Assert message received.           */
	PEX_RCV_INT_C   		, /*26 Set when IntC_Assert message received.           */
	PEX_RCV_INT_D   		, /*27 Set when IntC_Assert message received.           */
	PEX_RCV_PM_PME   		, /*28 Set when IntD_Assert message received.           */
	PEX_RCV_Turn_Off 		, /*29 If EndPoint => RcvTurnOffMsg: Receive Turn OFF MSG from the RC.    
							       If PEX=RootComplex => Ready4TurnOff: RC notifies the Host that the TurnOff
								   Process is done. (whether normally or by timer).*/
	PEX_RESERVED30   		, /*30            */
	PEX_RCV_MSI   			, /*31 RC receive MemWr that hit the MSI attributes.  */ 
	PEX_CAUSE_END   	      /* Cause End boundry                             	  */
} PEX_CAUSE;

typedef enum _pexCorrErrCause
{
    PEX_CORR_ERR_START= -1 , /* Cause Start boundry                        	  */
	PEX_CORR_RCV_ERR	, /* Receiver Error Status							  */
    PEX_CORR_RESERVED1	,
    PEX_CORR_RESERVED2	,
    PEX_CORR_RESERVED3	,
    PEX_CORR_RESERVED4	,
    PEX_CORR_RESERVED5	,
	PEX_CORR_BAD_TLP_ERR, /* Bad TLP Status								      */
	PEX_CORR_BAD_DLLP_ERR,/* Bad DLLP Status                                  */
	PEX_CORR_RPLY_RLLOVR_ERR, /* Replay Number Rollover Status                */
    PEX_CORR_RESERVED9	,
    PEX_CORR_RESERVED10	,
    PEX_CORR_RESERVED11	,
	PEX_CORR_RPLY_TO_ERR,  /* Replay Timer Timeout status                      */
	PEX_CORE_NON_FATAL_ERR,/* Advisory Non-Fatal Error Status                  */
    PEX_CORR_ERR_END     /* Cause Start boundry                        	  */
}PEX_CORR_ERR_CAUSE;

typedef enum _pexUnCorrErrCause
{
    PEX_UNCORR_ERR_START= -1 , /* Cause Start boundry                        	  */
    PEX_UNCORR_RESERVED0	 ,
    PEX_UNCORR_RESERVED1	 ,
    PEX_UNCORR_RESERVED2	 ,
    PEX_UNCORR_RESERVED3	 ,
	PEX_UNCORR_RCV_DLPRT_ERR , /* Data Link Protocol Error Status		  */
    PEX_UNCORR_RESERVED5	 ,
    PEX_UNCORR_RESERVED6	 ,
    PEX_UNCORR_RESERVED7	 ,
    PEX_UNCORR_RESERVED8	 ,
    PEX_UNCORR_RESERVED9	 ,
    PEX_UNCORR_RESERVED10	 ,
    PEX_UNCORR_RESERVED11	 ,
	PEX_UNCORR_RPSN_TLP_ERR	 , /* Poisoned TLP Status */
    PEX_UNCORR_RESERVED13	 ,
	PEX_UNCORR_CMP_TO_ERR	 , /*  Completion Timeout Status         */
	PEX_UNCORR_CA_ERR		 , /*  Completer Abort Status            */
	PEX_UNCORR_UN_EXP_CMP_ERR,/* Unexpected Completion Status       */
    PEX_UNCORR_RESERVED17	 ,
	PEX_UNCORR_MALF_TLP_ERR	 , /*  Malformed TLP Status              */
    PEX_UNCORR_RESERVED19	 ,
	PEX_UNCORR_UR_ERR		 , /* Unsupported Request Error Status                  */
    PEX_UNCORR_ERR_END        /* Cause Start boundry                        	  */
}PEX_UNCORR_ERR_CAUSE;


#define PEX_RCV_INT(intPin)		(PEX_RCV_INT_A -1 + intPin)
#define PEX_INT_ABCD_MASK		((1 << PEX_RCV_INT_A) |	\
								 (1 << PEX_RCV_INT_B) |	\
								 (1 << PEX_RCV_INT_C) |	\
								 (1 << PEX_RCV_INT_D))
/* vxPexIntCtrl.h API list */
void   vxPexIntCtrlInit(void);
void   vxPexIntCtrlInit2(void);
STATUS vxPexIntConnect (int pexIf, PEX_CAUSE cause, VOIDFUNCPTR routine,
                          int parameter, int prio);
STATUS vxPexIntEnable  (int pexIf, PEX_CAUSE cause);
STATUS vxPexIntDisable (int pexIf, PEX_CAUSE cause);
STATUS vxPexIntClear   (int pexIf, PEX_CAUSE cause);

/* Interrupt Controller's available handlers */
void pexIntHandler(int pexIf);       /* Handle int A,B,C,D events  */
void pexIntErrHandler(int pexIf);    /* Handle PEX Errors events   */

#ifdef __cplusplus
}
#endif

#endif /* __INCvxPexIntCtrllh */

