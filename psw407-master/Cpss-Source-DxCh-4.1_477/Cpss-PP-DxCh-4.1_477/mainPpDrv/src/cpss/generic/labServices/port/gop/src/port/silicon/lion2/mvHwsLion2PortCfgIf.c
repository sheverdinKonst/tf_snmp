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
#include <silicon/lion2/mvHwsLion2PortIf.h>
#include <serdes/comPhyHRev2/mvComPhyHRev2If.h>
#include <serdes/mvHwsSerdesPrvIf.h>
#include <pcs/mvHwsPcsIf.h>
#include <mac/mvHwsMacIf.h>
#include <common/siliconIf/siliconAddress.h>

/**************************** Definition *************************************************/
/* #define GT_DEBUG_HWS */
#ifdef  GT_DEBUG_HWS
#include <common/os/gtOs.h>
#define DEBUG_HWS_FULL(level,s) if (optPrintLevel >= level) {osPrintf s;}
#else
#define DEBUG_HWS_FULL(level,s)
#endif

GT_STATUS hwsPortFixAlign90ExtMultipleLaneOptimization (
	GT_U8 devNum, GT_U32 portGroup, GT_U32 phyPortNum,
	MV_HWS_PORT_STANDARD portMode,  GT_U32 optAlgoMask);


extern GT_STATUS hwsPortFixAlign90Flow(GT_U8 devNum, GT_U32 portGroup, GT_U32 *serdesList,
						GT_U32 numOfActLanes, GT_BOOL *allLanesPass);

/*******************************************************************************
* mvHwsLion2PCSMarkModeSet
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
GT_STATUS mvHwsLion2PCSMarkModeSet
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
* mvHwsLion2PortFixAlign90Ext
*
* DESCRIPTION:
*       Fix Align90 parameters
*
* INPUTS:
*       devNum      - system device number
*       portGroup   - port group (core) number
*       phyPortNum  - physical port number
*       portMode    - port standard metric
*       optAlgoMask - bit mask for optimization algorithms
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsLion2PortFixAlign90Ext
(
	GT_U8	devNum,
	GT_U32	portGroup,
	GT_U32	phyPortNum,
	MV_HWS_PORT_STANDARD	portMode,
	GT_U32	optAlgoMask
)
{
	switch (portMode)
	{
	case _10GBase_KR:
	case _20GBase_KR:
	case _40GBase_KR:
	case _10GBase_SR_LR:
	case _20GBase_SR_LR:
	case _40GBase_SR_LR:
	case _12GBaseR:
    case _48GBaseR:
    case _12GBase_SR:
    case _48GBase_SR:
		if (hwsDeviceSpecInfo[devNum].devType == Lion2A0)
		{
			CHECK_STATUS(mvHwsPortFixAlign90(devNum, portGroup, phyPortNum, portMode, optAlgoMask));
		}
		else /* Lion2B0, HooperA0*/
		{
			CHECK_STATUS(hwsPortFixAlign90ExtMultipleLaneOptimization(devNum, portGroup, phyPortNum, portMode, optAlgoMask));
		}
		break;
	default:
		break;
	}

	return GT_OK;
}

/*******************************************************************************
* mvHwsLion2PortRxAutoTuneSetExt
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
GT_STATUS mvHwsLion2PortRxAutoTuneSetExt
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
	MV_HWS_AUTO_TUNE_RESULTS res[HWS_MAX_SERDES_NUM];
	MV_HWS_PORT_INIT_PARAMS *curPortParams;
	MV_HWS_AUTO_TUNE_STATUS rxStatus[HWS_MAX_SERDES_NUM];
	MV_HWS_AUTO_TUNE_STATUS txStatus[HWS_MAX_SERDES_NUM];
	GT_U32  curLanesList[HWS_MAX_SERDES_NUM];
	GT_BOOL autoTunePass[HWS_MAX_SERDES_NUM];
	GT_BOOL laneFail = GT_FALSE;
	GT_U32  sqleuch, ffeR, ffeC, align90;
	GT_U32  txAmp, txEmph0, txEmph1;
	GT_U32  i;
	GT_U32 numOfActLanes;

	/* avoid warnings */
	portTuningMode = portTuningMode;
	(void)results;

	if ((phyPortNum >= HWS_CORE_PORTS_NUM(devNum)) ||
		((GT_U32)portMode >= HWS_DEV_PORT_MODES(devNum)))
	{
		return GT_BAD_PARAM;
	}

	curPortParams = hwsPortModeParamsGet(devNum, portGroup, phyPortNum, portMode);
	hwsOsMemSetFuncPtr(autoTunePass, 0, sizeof(autoTunePass));
	hwsOsMemSetFuncPtr(res, 0, sizeof(res));
	hwsOsMemSetFuncPtr(rxStatus, 0, sizeof(rxStatus));
	hwsOsMemSetFuncPtr(txStatus, 0, sizeof(txStatus));

	/* rebuild active lanes list according to current configuration (redundancy) */
	CHECK_STATUS(mvHwsRebuildActiveLaneList(devNum, portGroup, phyPortNum, portMode, curLanesList));

	/* reset PCS Rx only */
	if (curPortParams->portPcsType == MMPCS)
	{
		CHECK_STATUS(mvHwsPcsRxReset(devNum, portGroup, curPortParams->portPcsNumber,
							curPortParams->portPcsType, RESET));
	}

	numOfActLanes = curPortParams->numOfActLanes;

	/* on each related serdes */
	for (i = 0; (i < HWS_MAX_SERDES_NUM) && (i < numOfActLanes); i++)
	{
		if (autoTunePass[i] == GT_FALSE)
		{
			CHECK_STATUS(mvHwsSerdesRxAutoTuneStart(devNum, (portGroup + ((curLanesList[i] >> 16) & 0xFF)),
										(curLanesList[i] & 0xFFFF), HWS_DEV_SERDES_TYPE(devNum), GT_TRUE));
		}
	}

	/* check status and start auto tune again if not passed */
	for (i = 0; (i < HWS_MAX_SERDES_NUM) && (i < numOfActLanes); i++)
	{
		if (autoTunePass[i] == GT_FALSE)
		{
			mvHwsSerdesAutoTuneStatus(devNum, (portGroup + ((curLanesList[i] >> 16) & 0xFF)),
							(curLanesList[i] & 0xFFFF), HWS_DEV_SERDES_TYPE(devNum), &rxStatus[i], NULL);
			if (rxStatus[i] == TUNE_PASS)
			{
				autoTunePass[i] = GT_TRUE;
			}
			else
			{
				laneFail = GT_TRUE;
			}
		}
	}

	hwsOsTimerWkFuncPtr(1);

	if (laneFail == GT_TRUE)
	{
		/* unreset PCS Rx only */
		if (curPortParams->portPcsType == MMPCS)
		{
			CHECK_STATUS(mvHwsPcsRxReset(devNum, portGroup, curPortParams->portPcsNumber,
								curPortParams->portPcsType, UNRESET));
		}

		return GT_FAIL;
	}

	/* get results */
	sqleuch = ffeR = ffeC = align90 = 0;
	txAmp = txEmph0 = txEmph1 = 0;
	for (i = 0; (i < HWS_MAX_SERDES_NUM) && (i < numOfActLanes); i++)
	{
		CHECK_STATUS(mvHwsSerdesAutoTuneResult(devNum, (portGroup + ((curLanesList[i] >> 16) & 0xFF)),
								(curLanesList[i] & 0xFFFF), HWS_DEV_SERDES_TYPE(devNum), &res[i]));
		sqleuch += res[i].sqleuch;
		ffeR += res[i].ffeR;
		ffeC += res[i].ffeC;
		txAmp += res[i].txAmp;
		txEmph0 += res[i].txEmph0;
		txEmph1 += res[i].txEmph1;
	 }

	/* disable training on each related serdes */
	for (i = 0; (i < HWS_MAX_SERDES_NUM) && (i < numOfActLanes); i++)
	{
		 CHECK_STATUS(mvHwsSerdesRxAutoTuneStart(devNum, (portGroup + ((curLanesList[i] >> 16) & 0xFF)),
								(curLanesList[i] & 0xFFFF), HWS_DEV_SERDES_TYPE(devNum), GT_FALSE));
	}

	/* calculate average tune parameters for all lanes and force values only on Lion2A0 */
	/* on each related serdes */
	for (i = 0; (i < HWS_MAX_SERDES_NUM) && (hwsDeviceSpecInfo[devNum].devType == Lion2A0) && (i < numOfActLanes); i++)
	{
		CHECK_STATUS(mvHwsSerdesDfeOpti(devNum, (portGroup + ((curLanesList[i] >> 16) & 0xFF)),
							(curLanesList[i] & 0xFFFF), HWS_DEV_SERDES_TYPE(devNum), &ffeR));
		CHECK_STATUS(mvHwsSerdesFfeConfig(devNum, (portGroup + ((curLanesList[i] >> 16) & 0xFF)),
							(curLanesList[i] & 0xFFFF), HWS_DEV_SERDES_TYPE(devNum), ffeR,
							(ffeC / numOfActLanes), res[i].sampler));
	}

	CHECK_STATUS(mvHwsLion2PortFixAlign90Ext(devNum, portGroup, phyPortNum, portMode, optAlgoMask));

	/* unreset PCS Rx only */
	hwsOsTimerWkFuncPtr(1);

	if (curPortParams->portPcsType == MMPCS)
	{
		CHECK_STATUS(mvHwsPcsRxReset(devNum, portGroup, curPortParams->portPcsNumber,
							curPortParams->portPcsType, UNRESET));
	}

	if (portMode == RXAUI)
	{
		/* call to WA */
		mvHwsXPcsConnect(devNum, portGroup, phyPortNum);
	}

	return GT_OK;
}

/*******************************************************************************
* mvHwsLion2PortTxAutoTuneSetExt
*
* DESCRIPTION:
*       Sets the port Tx only parameters according to different working
*       modes/topologies.
*       Can be run any time after create port.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*       portTuningMode - port TX related tuning mode
*
* OUTPUTS:
*       Tuning results for recommended settings.(TBD)
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsLion2PortTxAutoTuneSetExt
(
	GT_U8   devNum,
	GT_U32  portGroup,
	GT_U32  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	MV_HWS_PORT_AUTO_TUNE_MODE      portTuningMode,
	GT_U32  optAlgoMask
)
{
	CHECK_STATUS(hwsPortTxAutoTuneStartSet(devNum, portGroup, phyPortNum, portMode, portTuningMode, optAlgoMask));

	if (portTuningMode == TRxTuneStatus)
	{
		CHECK_STATUS(mvHwsLion2PortFixAlign90Ext(devNum, portGroup, phyPortNum, portMode, optAlgoMask));

		CHECK_STATUS(hwsPortTxAutoTuneActivateSet(devNum, portGroup, phyPortNum, portMode, portTuningMode));

		if (portMode == RXAUI)
		{
			/* call to WA */
			mvHwsXPcsConnect(devNum, portGroup, phyPortNum);
		}
	}

	return GT_OK;
}


/*******************************************************************************
* mvHwsLion2PortAutoTuneSetExt
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
GT_STATUS mvHwsLion2PortAutoTuneSetExt
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
* mvHwsPortAutoTuneDelayInit
*
* DESCRIPTION:
*       Sets the delay values which are used in Serdes training optimization
*       algorithm
*
* INPUTS:
*       dynamicDelayInterval - determines the number of training iteration in
*       which the delay will be executed (DFE algorithm)
*       dynamicDelayDuration - delay duration in mSec (DFE algorithm)
*       staticDelayDuration  - delay duration in mSec (Align90 algorithm)
*
* OUTPUTS:
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortAutoTuneDelayInit
(
	GT_U32  dynamicDelayInterval, /* DFE */
	GT_U32  dynamicDelayDuration, /* DFE */
	GT_U32  staticDelayDuration   /* Align90 */
)
{
    return mvHwsComHRev2SerdesTrainingOptDelayInit(dynamicDelayInterval, dynamicDelayDuration, staticDelayDuration);
}

/*******************************************************************************
* mvHwsPortTxAutoTuneUpdateWaExt
*
* DESCRIPTION:
*       Update auto tune parameters according to tune result
*
* INPUTS:
*       devNum      - system device number
*       portGroup   - port group (core) number
*       phyPortNum  - physical port number
*       portMode    - port standard metric
*       curLanesList - active Serdes lanes list according to configuration
*
* OUTPUTS:
*       Tuning results for recommended settings.(TBD)
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortTxAutoTuneUpdateWaExt
(
	GT_U8   devNum,
	GT_U32  portGroup,
	GT_U32  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	GT_U32  *curLanesList
)
{
	GT_U32 i;
	GT_U32  sqleuch, ffeR, ffeC, align90;
	GT_U32  txAmp, txEmph0, txEmph1;
	MV_HWS_AUTO_TUNE_RESULTS res[HWS_MAX_SERDES_NUM];
	MV_HWS_PORT_INIT_PARAMS *curPortParams = hwsPortModeParamsGet(devNum, portGroup, phyPortNum, portMode);

	hwsOsMemSetFuncPtr(res, 0, sizeof(res));

	if (HWS_DEV_SERDES_TYPE(devNum) == COM_PHY_H)
	{
		sqleuch = ffeR = ffeC = align90 = 0;
		txAmp = txEmph0 = txEmph1 = 0;

		for (i = 0; (i < HWS_MAX_SERDES_NUM) && (i < curPortParams->numOfActLanes); i++)
		{
			CHECK_STATUS(mvHwsSerdesAutoTuneResult(devNum, (portGroup + ((curLanesList[i] >> 16) & 0xFF)),
								(curLanesList[i] & 0xFFFF), HWS_DEV_SERDES_TYPE(devNum), &res[i]));

			sqleuch += res[i].sqleuch;
			ffeR += res[i].ffeR;
			ffeC += res[i].ffeC;
			txAmp += res[i].txAmp;
			txEmph0 += res[i].txEmph0;
			txEmph1 += res[i].txEmph1;
		}

		/* calculate average tune parameters for all lanes and force values on each related serdes */
		for (i = 0; (i < HWS_MAX_SERDES_NUM) && (i < curPortParams->numOfActLanes); i++)
		{
			CHECK_STATUS(mvHwsSerdesDfeOpti(devNum, (portGroup + ((curLanesList[i] >> 16) & 0xFF)),
								(curLanesList[i] & 0xFFFF), HWS_DEV_SERDES_TYPE(devNum), &ffeR));
			CHECK_STATUS(mvHwsSerdesFfeConfig(devNum, (portGroup + ((curLanesList[i] >> 16) & 0xFF)),
								(curLanesList[i] & 0xFFFF), HWS_DEV_SERDES_TYPE(devNum), ffeR,
								(ffeC / curPortParams->numOfActLanes), res[i].sampler));
		}
	}

	return GT_OK;
}

/*******************************************************************************
* hwsPortFixAlign90ExtMultipleLaneOptimization
*
* DESCRIPTION:
*       Run fix Align90 process on current port.
*       Can be run any time after create port.
*       == Hooper A0, Lion2 B0 ==
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS hwsPortFixAlign90ExtMultipleLaneOptimization
(
	GT_U8	devNum,
	GT_U32	portGroup,
	GT_U32	phyPortNum,
	MV_HWS_PORT_STANDARD	portMode,
	GT_U32	optAlgoMask
)
{
	MV_HWS_PORT_INIT_PARAMS *curPortParams;
	GT_U32 curLanesList[HWS_MAX_SERDES_NUM];
	GT_U32 regulatorLanesList[HWS_MAX_SERDES_NUM];
	GT_U32 tmpSerdesList[HWS_MAX_SERDES_NUM];
	GT_U32 numOfActLanes;
	MV_HWS_PORT_MAN_TUNE_MODE portTuningMode = StaticLongReach;
	GT_BOOL isTrainingMode = GT_FALSE;
	GT_BOOL allLanesPass = GT_TRUE;
	GT_U32 align90Count = 0;
	GT_U32 i;

	if ((phyPortNum >= HWS_CORE_PORTS_NUM(devNum)) ||
		((GT_U32)portMode >= HWS_DEV_PORT_MODES(devNum)))
	{
		return GT_BAD_PARAM;
	}

	curPortParams = hwsPortModeParamsGet(devNum, portGroup, phyPortNum, portMode);

	/* rebuild active lanes list according to current configuration (redundancy) */
	CHECK_STATUS(mvHwsRebuildActiveLaneList(devNum, portGroup, phyPortNum, portMode, curLanesList));

	numOfActLanes = curPortParams->numOfActLanes;
	for (i = 0; i < numOfActLanes; i++)
	{
		tmpSerdesList[i] = curLanesList[i];
	}

	do
	{
		DEBUG_HWS_FULL(2, ("\nAlign90 Iteration %d\n", align90Count));

		/* for Lion2/Hooper we try first to change the regulator. if it won't work, the optimization
		algorithm will be executed (later) */
		if (align90Count == 10)
		{
			DEBUG_HWS_FULL(0, ("Configure regulator to high value\n"));

			CHECK_STATUS(mvHwsComHRev2SerdesConfigRegulator(devNum, portGroup, tmpSerdesList,
										numOfActLanes, GT_FALSE));

			/* save the SERDES with update regulator */
			for (i = 0; i < numOfActLanes; i++)
			{
				regulatorLanesList[i] = tmpSerdesList[i];
			}
		}


		CHECK_STATUS(hwsPortFixAlign90Flow(devNum, portGroup, tmpSerdesList, numOfActLanes, &allLanesPass));
		align90Count++;
	} while ((allLanesPass != GT_TRUE) && (align90Count < 20));

	if (align90Count > 10)
	{
		DEBUG_HWS_FULL(0, ("Configure regulator to low value\n"));

		CHECK_STATUS(mvHwsComHRev2SerdesConfigRegulator(devNum, portGroup, regulatorLanesList,
									numOfActLanes, GT_TRUE));
	}

	if (allLanesPass == GT_FALSE)
	{
		DEBUG_HWS_FULL(2, ("****** Align90 failed for 20 iterations\n"));
	}

	/* For Lion2 B0/Hooper only we run optimization algorithms to tune the Serdes better */
	if (allLanesPass == GT_TRUE)
	{
		/* check if port mode needs to be tuned and if it's Short/Long reach */
		CHECK_STATUS(hwsPortGetTuneMode(portMode, &portTuningMode, &isTrainingMode));

		if (isTrainingMode == GT_TRUE)
		{
			CHECK_STATUS(mvHwsComHRev2SerdesTrainingOptimization(devNum, portGroup, curLanesList,
									numOfActLanes, portTuningMode, optAlgoMask));
		}
	}

	for (i = 0; (i < HWS_MAX_SERDES_NUM) && (i < numOfActLanes); i++)
	{
		mvHwsSerdesRev2DfeCheck(devNum, (portGroup + ((curLanesList[i] >> 16) & 0xFF)),
					(curLanesList[i] & 0xFFFF), (MV_HWS_SERDES_TYPE)HWS_DEV_SERDES_TYPE(devNum));
	}

	if (allLanesPass)
	{
		return GT_OK;
	}

	return GT_FAIL;
}

/*******************************************************************************
* hwsPortLion2ExtendedModeCfg
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
GT_STATUS hwsPortLion2ExtendedModeCfg
(
	GT_U8   devNum,
	GT_U32  portGroup,
	GT_U32  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	GT_BOOL extendedMode
)
{
	MV_HWS_PORT_INIT_PARAMS *curPortParams;

	/* avoid warnings */
	portGroup = portGroup;

	if ((phyPortNum >= HWS_CORE_PORTS_NUM(devNum)) ||
		((GT_U32)portMode >= HWS_DEV_PORT_MODES(devNum)))
	{
		return GT_BAD_PARAM;
	}

	curPortParams = hwsPortModeParamsGet(devNum, portGroup, phyPortNum, portMode);

	/* extended ports supported only in Lion2A0, Lion2B0 and Hooper, ports 9 or 11 */
	if ((HWS_DEV_SILICON_TYPE(devNum) == Lion2A0) ||
		(HWS_DEV_SILICON_TYPE(devNum) == Lion2B0))
	{
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
                    case _12GBaseR:
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
                    case _12GBaseR:
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
	}

	return GT_OK;
}

/*******************************************************************************
* mvHwsPortLion2ExtendedModeCfgGet
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
GT_STATUS mvHwsPortLion2ExtendedModeCfgGet
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

	return GT_BAD_PARAM;
}

/*******************************************************************************
* mvHwsPortSetOptAlgoParams
*
* DESCRIPTION:
*       Change default thresholds of:
*		ffe optimizer for first iteration, final iteration and f0d stop threshold
*
* INPUTS:
*       ffeFirstTh - threshold value for changing ffe optimizer for first iteration
*       ffeFinalTh - threshold value for changing ffe optimizer for final iteration
*       f0dStopTh - value for changing default of f0d stop threshold
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortSetOptAlgoParams
(
    GT_U8     devNum,
    GT_U32    portGroup,
    GT_32     ffeFirstTh,
    GT_32     ffeFinalTh,
    GT_U32    f0dStopTh
)
{
    /* avoid warnings */
    devNum = devNum;
    portGroup = portGroup;

    return mvHwsComHRev2SerdesOptAlgoParams(ffeFirstTh, ffeFinalTh, f0dStopTh);
}


