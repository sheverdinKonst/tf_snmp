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
#include <silicon/bobk/mvHwsBobKPortIf.h>
#include <serdes/mvHwsSerdesPrvIf.h>
#include <serdes/avago/mvAvagoIf.h>
#include <pcs/mvHwsPcsIf.h>
#include <mac/mvHwsMacIf.h>
#include <common/siliconIf/siliconAddress.h>

/**************************** Definition ***************************************/
extern int mvHwsAvagoSerdesTemperatureGet
(
    unsigned char   devNum,
    unsigned int    portGroup,
    unsigned int    serdesNum,
    int             *temperature
);

/*******************************************************************************
* mvHwsBobKPortSignalDetectMask
*
* DESCRIPTION:
*       implements GEMAC gets wrong signal detect polarity
*
* INPUTS:
*       devNum       - system device number
*       portGroup    - port group (core) number
*       phyPortNum   - physical port number
*       portMode     - port standard metric
*       enable       - enable / disable
*
* OUTPUTS:
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsBobKPortSignalDetectMask
(
	GT_U8                   devNum,
	GT_U32                  portGroup,
	GT_U32                  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	GT_BOOL                 enable
)
{
    MV_HWS_PORT_INIT_PARAMS *curPortParams;
    GT_U32 curLanesList[HWS_MAX_SERDES_NUM];
    GT_U32 i;

    curPortParams = hwsPortModeParamsGet(devNum, portGroup, phyPortNum, portMode);

    /* rebuild active lanes list according to current configuration (redundancy) */
    CHECK_STATUS(mvHwsRebuildActiveLaneList(devNum, portGroup, phyPortNum, portMode, curLanesList));

    if ((portMode == _1000Base_X) || (portMode == SGMII) || (portMode == SGMII2_5) || (portMode == QSGMII))
    {
        for (i = 0; i < curPortParams->numOfActLanes; i++)
        {
            CHECK_STATUS(genUnitRegisterSet(devNum, portGroup, SERDES_UNIT, (curLanesList[i] & 0xFFFF), SERDES_EXTERNAL_CONFIGURATION_0, (enable << 7), (1 << 7)));        
        }
    }

    return GT_OK;
}

/*******************************************************************************
* mvHwsBobKPortExtendedModeCfg
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
GT_STATUS mvHwsBobKPortExtendedModeCfg
(
	GT_U8                   devNum,
	GT_U32                  portGroup,
	GT_U32                  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	GT_BOOL                 extendedMode
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

/*******************************************************************************
* mvHwsPortManualCtleConfig
*
* DESCRIPTION:
*       configures SERDES Ctle parameters for specific one SERDES lane or
*       for all SERDES lanes on port.
*
* INPUTS:
*       devNum      - system device number
*       portGroup   - port group (core) number
*       phyPortNum  - physical port number
*       portMode    - port standard metric
*       laneNum     - Serdes number on port or
*                     0xFF for all Serdes numbers on port
*       configParams- pointer to array of the config params structures
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0  - on success
*       1  - on error
*******************************************************************************/
GT_STATUS mvHwsPortManualCtleConfig
(
    GT_U8                               devNum,
    GT_U32                              portGroup,
    GT_U32                              phyPortNum,
    MV_HWS_PORT_STANDARD                portMode,
    GT_U8                               laneNum,
    MV_HWS_MAN_TUNE_CTLE_CONFIG_DATA    *configParams
)
{
    MV_HWS_PORT_INIT_PARAMS *curPortParams;
    GT_U32  curLanesList[HWS_MAX_SERDES_NUM];
    GT_U32  startLaneNum;
    GT_U32  endLaneNum;
    GT_U32  i;

    if ((phyPortNum >= HWS_CORE_PORTS_NUM(devNum)) || ((GT_U32)portMode >= HWS_DEV_PORT_MODES(devNum))
        || (HWS_DEV_SILICON_TYPE(devNum) != BobK))
    {
        return GT_BAD_PARAM;
    }

    curPortParams = hwsPortModeParamsGet(devNum, portGroup, phyPortNum, portMode);

    /* rebuild active lanes list according to current configuration (redundancy) */
    CHECK_STATUS(mvHwsRebuildActiveLaneList(devNum, portGroup, phyPortNum, portMode, curLanesList));

    /* for all lanes on port */
    if (laneNum == 0xFF)
    {
        startLaneNum = 0;
        endLaneNum = (curPortParams->numOfActLanes) - 1;
    }
    else /* for specific one lane on port */
    {
        startLaneNum = endLaneNum = laneNum;
    }

    for (i=startLaneNum; i <= endLaneNum; i++)
    {
        CHECK_STATUS(mvHwsAvagoSerdesManualCtleConfig(devNum, portGroup, MV_HWS_SERDES_NUM(curLanesList[i]), configParams->dcGain,
                                                      configParams->lowFrequency, configParams->highFrequency, configParams->bandWidth,
                                                      configParams->loopBandwidth, configParams->squelch));
    }

    return GT_OK;
}

/*******************************************************************************
* mvHwsBobKPortExtendedModeCfgGet
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
GT_STATUS mvHwsBobKPortExtendedModeCfgGet
(
	GT_U8                   devNum,
	GT_U32                  portGroup,
	GT_U32                  phyPortNum,
	MV_HWS_PORT_STANDARD    portMode,
	GT_BOOL                 *extendedMode
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

/*******************************************************************************
* mvHwsPortEnhanceTuneMaxDelaySet
*
* DESCRIPTION:
*       Perform Enhance Tuning for finding the best peak of the eye
*       on specific port.
*       This API executes the iCAL (Rx-Training) with Max Delay value.
*       Max Delay value set the maximum iterations for running pCAL in
*       mvHwsAvagoSerdesAutoTuneStatus which included the delay.
*
* INPUTS:
*       devNum     - system device number
*       portGroup  - port group (core) number
*       phyPortNum - physical port number
*       portMode   - mode type of port
*       maxDelay   - set the Max delay (0-15)
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortEnhanceTuneMaxDelaySet
(
    GT_U8       devNum,
    GT_U32      portGroup,
    GT_U32      phyPortNum,
    MV_HWS_PORT_STANDARD    portMode,
    GT_U8       maxDelay
)
{
    MV_HWS_PORT_INIT_PARAMS *curPortParams;
    GT_U32  curLanesList[HWS_MAX_SERDES_NUM];
    GT_U32  i;
    GT_U8   delay;

    /* avoid warnings */
    maxDelay = maxDelay;

    /* validation of PortNum and portMode */
    CHECK_STATUS(mvHwsPortValidate(devNum, portGroup, phyPortNum, portMode));

    if (HWS_DEV_SILICON_TYPE(devNum) != BobK)
    {
        return GT_NOT_SUPPORTED;
    }

    curPortParams = hwsPortModeParamsGet(devNum, portGroup, phyPortNum, portMode);

    /* rebuild active lanes list according to current configuration (redundancy) */
    CHECK_STATUS(mvHwsRebuildActiveLaneList(devNum, portGroup, phyPortNum, portMode, curLanesList));

    delay = (IS_SR_MODE(portMode) || IS_KR_MODE(portMode)) ? 15 : 0;

    /* on each related serdes */
    for (i = 0; i < curPortParams->numOfActLanes; i++)
    {
        CHECK_STATUS(mvHwsAvagoSerdesEnhanceTune(devNum, portGroup, MV_HWS_SERDES_NUM(curLanesList[i]), delay));
    }

    return GT_OK;
}

/*******************************************************************************
* mvHwsPortAvagoRxAutoTuneStart
*
* DESCRIPTION:
*       Start the port Rx Tunning
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
GT_STATUS mvHwsPortAvagoRxAutoTuneStart
(
    GT_U8   devNum,
    GT_U32  portGroup,
    GT_U32  phyPortNum,
    MV_HWS_PORT_STANDARD    portMode
)
{
    MV_HWS_PORT_INIT_PARAMS *curPortParams;
    GT_U32  curLanesList[HWS_MAX_SERDES_NUM];
    GT_U32 i;

    if ((phyPortNum >= HWS_CORE_PORTS_NUM(devNum)) || ((GT_U32)portMode >= HWS_DEV_PORT_MODES(devNum)))
    {
        return GT_BAD_PARAM;
    }

    curPortParams = hwsPortModeParamsGet(devNum, portGroup, phyPortNum, portMode);

    /* rebuild active lanes list according to current configuration (redundancy) */
    CHECK_STATUS(mvHwsRebuildActiveLaneList(devNum, portGroup, phyPortNum, portMode, curLanesList));

    /* on each related serdes */
    for (i = 0; (i < HWS_MAX_SERDES_NUM) && (i < curPortParams->numOfActLanes); i++)
    {
        CHECK_STATUS(mvHwsSerdesRxAutoTuneStart(devNum, portGroup, MV_HWS_SERDES_NUM(curLanesList[i]),
                                                HWS_DEV_SERDES_TYPE(devNum), GT_TRUE));
    }

    return GT_OK;
}

/*******************************************************************************
* mvHwsPortAvagoTxAutoTuneStatus
*
* DESCRIPTION:
*       Get the status of port Tx Tunning
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
*       GT_OK   - on success
*       GT_FAIL - if SerdesAutoTuneStatus returns with TUNE_FAIL
*
*******************************************************************************/
GT_STATUS mvHwsPortAvagoTxAutoTuneStatus
(
    GT_U8   devNum,
    GT_U32  portGroup,
    GT_U32  phyPortNum,
    MV_HWS_PORT_STANDARD    portMode
)
{
    MV_HWS_PORT_INIT_PARAMS *curPortParams;
    MV_HWS_AUTO_TUNE_STATUS txStatus[HWS_MAX_SERDES_NUM];
    GT_U32 curLanesList[HWS_MAX_SERDES_NUM];
    GT_U32 i;

    if ((phyPortNum >= HWS_CORE_PORTS_NUM(devNum)) || ((GT_U32)portMode >= HWS_DEV_PORT_MODES(devNum)))
    {
        return GT_BAD_PARAM;
    }

    curPortParams = hwsPortModeParamsGet(devNum, portGroup, phyPortNum, portMode);
    if (curPortParams->numOfActLanes > HWS_MAX_SERDES_NUM)
    {
        return GT_BAD_PARAM;
    }

    hwsOsMemSetFuncPtr(txStatus, 0, sizeof(txStatus));

    /* rebuild active lanes list according to current configuration (redundancy) */
    CHECK_STATUS(mvHwsRebuildActiveLaneList(devNum, portGroup, phyPortNum, portMode, curLanesList));

    for (i = 0; i < curPortParams->numOfActLanes; i++)
    {
        CHECK_STATUS(mvHwsSerdesAutoTuneStatus(devNum, portGroup, MV_HWS_SERDES_NUM(curLanesList[i]), HWS_DEV_SERDES_TYPE(devNum),
                                               NULL, &txStatus[i]));

        if (txStatus[i] == TUNE_FAIL)
        {
            return GT_FAIL;
        }
    }

    return GT_OK;
}

/*******************************************************************************
* mvHwsPortAvagoTxAutoTuneStart
*
* DESCRIPTION:
*       Start the port Tx Tunning
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
GT_STATUS mvHwsPortAvagoTxAutoTuneStart
(
    GT_U8   devNum,
    GT_U32  portGroup,
    GT_U32  phyPortNum,
    MV_HWS_PORT_STANDARD    portMode
)
{
    MV_HWS_PORT_INIT_PARAMS *curPortParams;
    GT_U32  curLanesList[HWS_MAX_SERDES_NUM];
    GT_U32 i;

    if ((phyPortNum >= HWS_CORE_PORTS_NUM(devNum)) || ((GT_U32)portMode >= HWS_DEV_PORT_MODES(devNum)))
    {
        return GT_BAD_PARAM;
    }

    curPortParams = hwsPortModeParamsGet(devNum, portGroup, phyPortNum, portMode);

    /* rebuild active lanes list according to current configuration (redundancy) */
    CHECK_STATUS(mvHwsRebuildActiveLaneList(devNum, portGroup, phyPortNum, portMode, curLanesList));

    /* on each related serdes */
    for (i = 0; (i < HWS_MAX_SERDES_NUM) && (i < curPortParams->numOfActLanes); i++)
    {
        /* enable TX Tunning */
        CHECK_STATUS(mvHwsSerdesTxAutoTuneStart(devNum, portGroup, MV_HWS_SERDES_NUM(curLanesList[i]),
                                                HWS_DEV_SERDES_TYPE(devNum), GT_TRUE));
    }

    return GT_OK;
}

/*******************************************************************************
* mvHwsPortAvagoDfeCfgSet
*
* DESCRIPTION:
*       Set the DFE (Rx-Traning) configuration mode on port
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       phyPortNum - physical port number
*       portMode   - mode of port
*       dfeMode    - mode of DFE
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - if SerdesAutoTuneStatus returns with TUNE_FAIL
*
*******************************************************************************/
GT_STATUS mvHwsPortAvagoDfeCfgSet
(
    GT_U8   devNum,
    GT_U32  portGroup,
    GT_U32  phyPortNum,
    MV_HWS_PORT_STANDARD    portMode,
    MV_HWS_DFE_MODE         dfeMode
)
{
    MV_HWS_PORT_INIT_PARAMS *curPortParams;
    GT_U32 curLanesList[HWS_MAX_SERDES_NUM];
    GT_U32 i;

    /* validation of PortNum and portMode */
    CHECK_STATUS(mvHwsPortValidate(devNum, portGroup, phyPortNum, portMode));

    curPortParams = hwsPortModeParamsGet(devNum, portGroup, phyPortNum, portMode);

    /* rebuild active lanes list according to current configuration (redundancy) */
    CHECK_STATUS(mvHwsRebuildActiveLaneList(devNum, portGroup, phyPortNum, portMode, curLanesList));

    for (i = 0; i < curPortParams->numOfActLanes; i++)
    {
        CHECK_STATUS(mvHwsSerdesDfeConfig(devNum, portGroup, MV_HWS_SERDES_NUM(curLanesList[i]), HWS_DEV_SERDES_TYPE(devNum),
                                          dfeMode, NULL));
    }

    return GT_OK;
}

/*******************************************************************************
* mvHwsPortAvagoAutoTuneSet
*
* DESCRIPTION:
*       Calling to port Rx or Tx Tunning function
*
* INPUTS:
*       devNum     - system device number
*       portGroup  - port group (core) number
*       phyPortNum - physical port number
*       portMode   - port standard metric
*       portTuningMode - RxTunning or TxTunning
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortAvagoAutoTuneSet
(
    GT_U8   devNum,
    GT_U32  portGroup,
    GT_U32  phyPortNum,
    MV_HWS_PORT_STANDARD        portMode,
    MV_HWS_PORT_AUTO_TUNE_MODE  portTuningMode
)
{
    if ((phyPortNum >= HWS_CORE_PORTS_NUM(devNum)) || ((GT_U32)portMode >= HWS_DEV_PORT_MODES(devNum)))
    {
        return GT_BAD_PARAM;
    }

    switch (portTuningMode)
    {
        case TRxTuneCfg:
            return GT_OK;
        case RxTrainingOnly:
            CHECK_STATUS(mvHwsPortAvagoDfeCfgSet(devNum, portGroup, phyPortNum, portMode, DFE_ICAL));
            break;
        case TRxTuneStart:
            CHECK_STATUS(mvHwsPortAvagoTxAutoTuneStart(devNum, portGroup, phyPortNum, portMode));
            break;
        case TRxTuneStatus:
            CHECK_STATUS(mvHwsPortAvagoTxAutoTuneStatus(devNum, portGroup, phyPortNum, portMode));
            break;
        case RxTrainingAdative: /* running Adaptive pCal */
            CHECK_STATUS(mvHwsPortAvagoDfeCfgSet(devNum, portGroup, phyPortNum, portMode, DFE_START_ADAPTIVE));
            break;
        case RxTrainingStopAdative:
            CHECK_STATUS(mvHwsPortAvagoDfeCfgSet(devNum, portGroup, phyPortNum, portMode, DFE_STOP_ADAPTIVE));
            break;
        default:
            return GT_NOT_SUPPORTED;
    }

    return GT_OK;
}

/*******************************************************************************
* mvHwsPortAvagoAutoTuneSetExt
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
GT_STATUS mvHwsPortAvagoAutoTuneSetExt
(
    GT_U8                       devNum,
    GT_U32                      portGroup,
    GT_U32                      phyPortNum,
    MV_HWS_PORT_STANDARD        portMode,
    MV_HWS_PORT_AUTO_TUNE_MODE  portTuningMode,
    GT_U32                      optAlgoMask,
    void                        *results
)
{
    /* avoid warnings */
    optAlgoMask = optAlgoMask;
    results = results;

    return mvHwsPortAvagoAutoTuneSet(devNum, portGroup, phyPortNum, portMode, portTuningMode);
}

/*******************************************************************************
* mvHwsPortVoltageGet
*
* DESCRIPTION:
*       Get the voltage (in mV) of BobK device
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*
* OUTPUTS:
*       voltage - device voltage value (in mV)
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortVoltageGet
(
    GT_U8       devNum,
    GT_U32      portGroup,
    GT_U32      *voltage
)
{
    if (HWS_DEV_SILICON_TYPE(devNum) != BobK)
    {
        return GT_NOT_SUPPORTED;
    }

    /* Get the voltage (in mV) from Avago Serdes, use Serdes #20 as voltage reference of device */
    CHECK_STATUS(mvHwsAvagoSerdesVoltageGet(devNum, portGroup, 20, voltage));

    return GT_OK;
}

/*******************************************************************************
* mvHwsPortTemperatureGet
*
* DESCRIPTION:
*       Get the Temperature (in C) of BobK device
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*
* OUTPUTS:
*       voltage - device voltage value (in C)
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsPortTemperatureGet
(
    GT_U8       devNum,
    GT_U32      portGroup,
    GT_32       *temperature
)
{
#ifdef ASIC_SIMULATION
    /* avoid warnings */
    devNum = devNum;
    portGroup = portGroup;
    temperature = temperature;
#else

    if (HWS_DEV_SILICON_TYPE(devNum) != BobK)
    {
        return GT_NOT_SUPPORTED;
    }

    /* Get the Temperature (in C) from Avago Serdes, use Serdes #20 as temperature reference of device */
    CHECK_STATUS(mvHwsAvagoSerdesTemperatureGet(devNum, portGroup, 20, temperature));
#endif /* ASIC_SIMULATION */

    return GT_OK;
}


