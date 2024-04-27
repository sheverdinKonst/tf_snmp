/*******************************************************************************
*       	 Copyright 2001, Marvell International Ltd.
* This code contains confidential information of Marvell semiconductor, inc.
* no rights are granted herein under any patent, mask work right or copyright
* of Marvell or any third party.
* Marvell reserves the right at its sole discretion to request that this code
* be immediately returned to Marvell. This code is provided "as is".
* Marvell makes no warranties, express, implied or otherwise, regarding its
* accuracy, completeness or performance.
********************************************************************************
* mvHwsPortCfgIf.c
*
* DESCRIPTION:
*           This file contains API for port configuartion and tuning parameters
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*       $Revision: 48 $
******************************************************************************/
#include <common/siliconIf/mvSiliconIf.h>
#include <mvHwsPortInitIf.h>
#include <private/mvHwsPortPrvIf.h>
#include <private/mvHwsPortMiscIf.h>
#include <private/mvPortModeElements.h>
#include <silicon/hooper/mvHwsHooperPortIf.h>
#include <serdes/mvHwsSerdesPrvIf.h>
#include <pcs/mvHwsPcsIf.h>
#include <mac/mvHwsMacIf.h>
#include <common/siliconIf/siliconAddress.h>

extern GT_STATUS mvHwsLion2PortRxAutoTuneSetExt (GT_U8 devNum, GT_U32  portGroup, GT_U32  phyPortNum,
						 MV_HWS_PORT_STANDARD portMode, MV_HWS_PORT_AUTO_TUNE_MODE portTuningMode,
						 GT_U32 optAlgoMask, void *results);

extern GT_STATUS mvHwsLion2PortTxAutoTuneSetExt(GT_U8 devNum, GT_U32 portGroup, GT_U32 phyPortNum,
                         MV_HWS_PORT_STANDARD portMode, MV_HWS_PORT_AUTO_TUNE_MODE portTuningMode,
                         GT_U32 optAlgoMask);

extern GT_STATUS mvHwsLion2PortTxAutoTuneSet (GT_U8 devNum, GT_U32  portGroup, GT_U32  phyPortNum,
						 MV_HWS_PORT_STANDARD portMode, MV_HWS_PORT_AUTO_TUNE_MODE portTuningMode,
						 GT_U32 optAlgoMask);

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
)
{
	MV_HWS_PORT_INIT_PARAMS *curPortParams;

	if ((phyPortNum >= HWS_CORE_PORTS_NUM(devNum)) ||
		((GT_U32)portMode >= HWS_DEV_PORT_MODES(devNum)))
	{
		return GT_BAD_PARAM;
	}

	curPortParams = hwsPortModeParamsGet(devNum, portGroup, phyPortNum, portMode);

	switch (portMode)
	{
        case _10GBase_KR:
        case _40GBase_KR:
        case _10GBase_SR_LR:
        case _40GBase_SR_LR:
        case _12GBaseR:
        case _48GBaseR:
        case _12GBase_SR:
        case _48GBase_SR:
            CHECK_STATUS(genUnitRegisterSet(devNum, portGroup, MMPCS_UNIT, curPortParams->portPcsNumber,
                                            FEC_DEC_DMA_WR_DATA, enable, 1));
            break;
        case RXAUI:
            CHECK_STATUS(genUnitRegisterSet(devNum, portGroup, XPCS_UNIT, curPortParams->portPcsNumber,
                                            XPCS_Internal_Metal_Fix, enable, 1));
            break;
        default:
            osPrintf("mvHwsPCSMarkModeSet: portMode %d is not supported for Mark/Un-mark PCS unit\n", portMode);
            return GT_BAD_PARAM;
	}

	return GT_OK;
}

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
)
{
    if (portTuningMode == RxTrainingOnly)
    {
        return mvHwsLion2PortRxAutoTuneSetExt(devNum, portGroup, phyPortNum, portMode, portTuningMode, optAlgoMask, results);
    }
    else
    {
        return mvHwsLion2PortTxAutoTuneSetExt(devNum, portGroup, phyPortNum, portMode, portTuningMode, optAlgoMask);
    }
}

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
)
{
	MV_HWS_PORT_INIT_PARAMS *curPortParams;

	portGroup = portGroup;

	if ((phyPortNum >= HWS_CORE_PORTS_NUM(devNum)) ||
		((GT_U32)portMode >= HWS_DEV_PORT_MODES(devNum)))
	{
		return GT_BAD_PARAM;
	}

	curPortParams = hwsPortModeParamsGet(devNum, portGroup, phyPortNum, portMode);

	/* extended ports supported only in Lion2A0, Lion2B0 and Hooper, ports 9 or 11 */
	switch (phyPortNum)
	{
	case 9:
		switch (portMode)
		{
		/* normal and extended modes */
		case SGMII:
		case _1000Base_X:
		case SGMII2_5:
		case _10GBase_KR:
		case _10GBase_SR_LR:
		case _12_5GBase_KR:
			curPortParams->portPcsNumber = (extendedMode == GT_TRUE) ? 12 : 9;
			curPortParams->portMacNumber = (extendedMode == GT_TRUE) ? 12 : 9;
			curPortParams->firstLaneNum = (extendedMode == GT_TRUE) ? 18 : 13;
			curPortParams->activeLanesList[0] = (extendedMode == GT_TRUE) ? 18 : 13;
			break;
		default:
			return GT_NOT_SUPPORTED;
		}
		break;
	case 11:
		switch (portMode)
		{
		/* normal and extended modes */
		case SGMII:
		case _1000Base_X:
		case SGMII2_5:
		case _10GBase_KR:
		case _10GBase_SR_LR:
		case _12_5GBase_KR:
			curPortParams->portPcsNumber = (extendedMode == GT_TRUE) ? 14 : 11;
			curPortParams->portMacNumber = (extendedMode == GT_TRUE) ? 14 : 11;
			curPortParams->firstLaneNum = (extendedMode == GT_TRUE) ? 20 : 15;
			curPortParams->activeLanesList[0] = (extendedMode == GT_TRUE) ? 20 : 15;
			break;
		default:
			return GT_NOT_SUPPORTED;
		}
		break;
	default:
		return GT_NOT_SUPPORTED;
	}

	return GT_OK;
}

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
)
{
	/* avoid warnings */
	devNum		 = devNum;
	portGroup    = portGroup;
	phyPortNum   = phyPortNum;
	portMode     = portMode;
	extendedMode = extendedMode;

	return GT_NOT_SUPPORTED;
}

