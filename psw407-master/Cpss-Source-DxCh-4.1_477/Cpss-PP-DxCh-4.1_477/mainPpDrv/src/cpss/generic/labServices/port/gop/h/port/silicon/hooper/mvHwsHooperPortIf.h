/*******************************************************************************
*                Copyright 2001, Marvell International Ltd.
* This code contains confidential information of Marvell semiconductor, inc.
* no rights are granted herein under any patent, mask work right or copyright
* of Marvell or any third party.
* Marvell reserves the right at its sole discretion to request that this code
* be immediately returned to Marvell. This code is provided "as is".
* Marvell makes no warranties, express, implied or otherwise, regarding its
* accuracy, completeness or performance.
********************************************************************************
* mvHwsHooperPortIf.h
*
* DESCRIPTION:
*
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*       $Revision: 18 $
******************************************************************************/

#ifndef __mvHwServicesHooperPortIf_H
#define __mvHwServicesHooperPortIf_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gtOs/gtGenTypes.h>
#include <mvHwsPortInitIf.h>

/*******************************************************************************
* mvHwsHooperPortInit
*
* DESCRIPTION:
*       Init physical port. Configures the port mode and all it's elements
*       accordingly.
*       Does not verify that the selected mode/port number is valid at the
*       core level.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*       lbPort     - if true, init port without serdes activity
*       refClock   - Reference clock frequency
*       refClockSrc - Reference clock source line
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsHooperPortInit
(
    GT_U8                   devNum,
    GT_U32                  portGroup,
    GT_U32                  phyPortNum,
    MV_HWS_PORT_STANDARD    portMode,
    GT_BOOL                 lbPort,
    MV_HWS_REF_CLOCK_SUP_VAL refClock,
    MV_HWS_REF_CLOCK_SOURCE  refClockSource
);

/*******************************************************************************
* mvHwsHooperPortReset
*
* DESCRIPTION:
*       Clears the port mode and release all its resources according to selected.
*       Does not verify that the selected mode/port number is valid at the core
*       level and actual terminated mode.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*       action    - Power down or reset
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsHooperPortReset
(
    GT_U8                   devNum,
    GT_U32                  portGroup,
    GT_U32                  phyPortNum,
    MV_HWS_PORT_STANDARD    portMode,
    MV_HWS_PORT_ACTION      action
);

/*******************************************************************************
* mvHwsHooperPortAutoTuneSetExt
*
* DESCRIPTION:
*       Sets the port Tx and Rx parameters according to different working
*       modes/topologies.
*       Can be run any time after create port.
*
* INPUTS:
*       devNum      - system device number
*       portGroup   - port group (core) number
*       phyPortNum  - physical port number
*       portMode    - port standard metric
*       optAlgoMask - bit mask for optimization algorithms
*       portTuningMode - port tuning mode
*
* OUTPUTS:
*       Tuning results for recommended settings.(TBD)
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsHooperPortAutoTuneSetExt
(
	GT_U8   devNum,
	GT_U32  portGroup,
	GT_U32  phyPortNum,
	MV_HWS_PORT_STANDARD        portMode,
	MV_HWS_PORT_AUTO_TUNE_MODE      portTuningMode,
	GT_U32  optAlgoMask,
	void *  results
);

/*******************************************************************************
* hwsPortHooperExtendedModeCfg
*
* DESCRIPTION:
*       Enable / disable extended mode on port specified.
*       Extended ports supported only in Lion2 and Alleycat3 devices.
*       For Lion2:      1G, 10GBase-R, 20GBase-R2, RXAUI - can be normal or extended
*       				XAUI, DXAUI, 40GBase-R - only extended
*       	For Alleycat3:  ports 25 and 27 can be 10GBase_KR, 10GBase_SR_LR - normal or extended modes
*       					port 27 can be 20GBase_KR, 20GBase_SR_LR - only in extended mode
*
* INPUTS:
*       devNum       - system device number
*       portGroup    - port group (core) number
*       phyPortNum   - physical port number
*       portMode     - port standard metric
*       extendedMode - enable / disable
*
* OUTPUTS:
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS hwsPortHooperExtendedModeCfg
(
	GT_U8   devNum,
	GT_U32  portGroup,
	GT_U32  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	GT_BOOL extendedMode
);

/*******************************************************************************
* mvHwsHooperPortExtendedModeCfgGet
*
* DESCRIPTION:
*       Returns the extended mode status on port specified.
*
* INPUTS:
*       devNum       - system device number
*       portGroup    - port group (core) number
*       phyPortNum   - physical port number
*       portMode     - port standard metric
*
* OUTPUTS:
*       extendedMode - enable / disable
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsHooperPortExtendedModeCfgGet
(
	GT_U8   devNum,
	GT_U32  portGroup,
	GT_U32  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	GT_BOOL *extendedMode
);

/*******************************************************************************
* mvHwsHooperPCSMarkModeSet
*
* DESCRIPTION:
*       Mark/Un-mark PCS unit
*
* INPUTS:
*       devNum      - system device number
*       portGroup   - port group (core) number
*       phyPortNum  - physical port number
*       portMode    - port standard metric
*       enable      - GT_TRUE  for mark the PCS,
*       	      GT_FALSE for un-mark the PCS
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   	- on success
*       GT_BAD_PARAM    - on wrong parameter
*******************************************************************************/
GT_STATUS mvHwsHooperPCSMarkModeSet
(
	GT_U8   devNum,
	GT_UOPT portGroup,
	GT_U32  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	GT_BOOL enable
);

#ifdef __cplusplus
}
#endif

#endif /* __mvHwServicesHooperPortIf_H */


