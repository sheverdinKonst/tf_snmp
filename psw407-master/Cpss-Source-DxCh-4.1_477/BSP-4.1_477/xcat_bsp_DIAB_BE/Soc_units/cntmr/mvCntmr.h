/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell 
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File in accordance with the terms and conditions of the General 
Public License Version 2, June 1991 (the "GPL License"), a copy of which is 
available along with the File in the license.txt file or by writing to the Free 
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or 
on the worldwide web at http://www.gnu.org/licenses/gpl.txt. 

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED 
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY 
DISCLAIMED.  The GPL License provides additional details about this warranty 
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File under the following licensing terms. 
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	    this list of conditions and the following disclaimer. 

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution. 

    *   Neither the name of Marvell nor the names of its contributors may be 
        used to endorse or promote products derived from this software without 
        specific prior written permission. 
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#ifndef __INCmvTmrWtdgh
#define __INCmvTmrWtdgh

/* includes */
#include "mvCommon.h"
#include "mvOs.h"
#include "cntmr/mvCntmrRegs.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"


/* This enumerator describe counters\watchdog numbers       */
typedef enum _mvCntmrID
{
	TIMER0 = 0,               
	TIMER1,
	WATCHDOG,
	TIMER2,               
	TIMER3
}MV_CNTMR_ID;


/* Counter / Timer control structure */
typedef struct _mvCntmrCtrl
{
	MV_BOOL 				enable;		/* enable */
	MV_BOOL					autoEnable;	/* counter/Timer                    */
}MV_CNTMR_CTRL;


/* Functions */

/* Load an init Value to a given counter/timer */
MV_STATUS mvCntmrLoad(MV_U32 countNum, MV_U32 value);

/* Returns the value of the given Counter/Timer */
MV_U32 mvCntmrRead(MV_U32 countNum);

/* Write a value of the given Counter/Timer */
void mvCntmrWrite(MV_U32 countNum,MV_U32 countVal);

/* Set the Control to a given counter/timer */
MV_STATUS mvCntmrCtrlSet(MV_U32 countNum, MV_CNTMR_CTRL *pCtrl);

/* Get the value of a given counter/timer */
MV_STATUS mvCntmrCtrlGet(MV_U32 countNum, MV_CNTMR_CTRL *pCtrl);

/* Set the Enable-Bit to logic '1' ==> starting the counter. */
MV_STATUS mvCntmrEnable(MV_U32 countNum);

/* Stop the counter/timer running, and returns its Value. */
MV_STATUS mvCntmrDisable(MV_U32 countNum);

/* Combined all the sub-operations above to one function: Load,setMode,Enable */
MV_STATUS mvCntmrStart(MV_U32 countNum, MV_U32 value,
                       MV_CNTMR_CTRL *pCtrl);

#endif /* __INCmvTmrWtdgh */
