/******************************************************************************
*              Copyright (c) Marvell International Ltd. and its affiliates
*
* This software file (the "File") is owned and distributed by Marvell
* International Ltd. and/or its affiliates ("Marvell") under the following
* alternative licensing terms.
* If you received this File from Marvell, you may opt to use, redistribute
* and/or modify this File under the following licensing terms.
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*  -   Redistributions of source code must retain the above copyright notice,
*       this list of conditions and the following disclaimer.
*  -   Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*  -    Neither the name of Marvell nor the names of its contributors may be
*       used to endorse or promote products derived from this software without
*       specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************
* mvHwsXlgMac28nmIf.h
*
* DESCRIPTION:
*       XLG MAC interface
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*
*******************************************************************************/

#ifndef __mvHwsXlgMac28nmIf_H
#define __mvHwsXlgMac28nmIf_H

#include <mac/mvHwsMacIf.h>

/*******************************************************************************
* hwsXlgMac28nmIfInit
*
* DESCRIPTION:
*       Init XLG MAC configuration sequences and IF functions.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS hwsXlgMac28nmIfInit
(
	GT_U8 devNum,
	MV_HWS_MAC_FUNC_PTRS *funcPtrArray
);

/*******************************************************************************
* mvHwsXlgMac28nmReset
*
* DESCRIPTION:
*       Set the GE MAC to reset or exit from reset.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       macNum    - MAC number
*       action    - normal or reset
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsXlgMac28nmReset
(
    GT_U8                   devNum,
    GT_UOPT                 portGroup,
    GT_UOPT                 macNum,
    MV_HWS_RESET            action
);

/*******************************************************************************
* mvHwsXlgMac28nmModeCfg
*
* DESCRIPTION:
*       Set the internal mux's to the required MAC in the GOP.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       macNum    - MAC number
*       numOfLanes - number of lanes aggregated by current mac/port
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsXlgMac28nmModeCfg
(
    GT_U8                   devNum,
    GT_UOPT                 portGroup,
    GT_UOPT                 macNum,
    GT_UOPT                 numOfLanes
);

/*******************************************************************************
* mvHwsXlgMac28nmLoopbackCfg
*
* DESCRIPTION:
*       Configure MAC loopback.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       macNum    - MAC number
*       lbType    - supported loopback type
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsXlgMac28nmLoopbackCfg
(
    GT_U8                   devNum,
    GT_UOPT                 portGroup,
    GT_UOPT                 macNum,
    MV_HWS_PORT_LB_TYPE     lbType
);

/*******************************************************************************
* mvHwsXlgMac28nmLoopbackStatusGet
*
* DESCRIPTION:
*       Retrive MAC loopback status.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       macNum    - MAC number
*
* OUTPUTS:
*       lbType    - supported loopback type
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsXlgMac28nmLoopbackStatusGet
(
    GT_U8                   devNum,
    GT_UOPT                 portGroup,
    GT_UOPT                 macNum,
    MV_HWS_PORT_LB_TYPE     *lbType
);

/*******************************************************************************
* mvHwsXlgMac28nmActiveStatusGet
*
* DESCRIPTION:
*       Return number of MAC active lanes or 0, if current MAC isn't active.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       macNum    - MAC number
*
* OUTPUTS:
*       numOfLanes - number of lanes agregated in PCS
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsXlgMac28nmActiveStatusGet
(
    GT_U8                   devNum,
    GT_UOPT                 portGroup,
    GT_UOPT                 macNum,
    GT_UOPT                 *numOfLanes
);

/*******************************************************************************
* mvHwsXlgMac28nmLinkStatus
*
* DESCRIPTION:
*       Get MAC link status.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsXlgMac28nmLinkStatus
(
    GT_U8                   devNum,
    GT_UOPT                 portGroup,
    GT_UOPT                 macNum,
    GT_BOOL                 *linkStatus
);

/*******************************************************************************
* mvHwsXlgMacModeSelectDefaultCfg
*
* DESCRIPTION:
*       MAC mode select default value configuration (needed for AP mechanism).
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       macNum    - MAC number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsXlgMacModeSelectDefaultCfg
(
    GT_U8                 devNum,
    GT_UOPT               portGroup,
    GT_UOPT               macNum
);

/*******************************************************************************
* hwsXlgMac28nmIfClose
*
* DESCRIPTION:
*       Release all system resources allocated by MAC IF functions.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
*******************************************************************************/
void hwsXlgMac28nmIfClose(void);

#endif /* __mvHwsXlgMacIf_H */

