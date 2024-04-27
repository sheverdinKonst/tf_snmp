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
* mvAvagoSerdesIf.c.c
*
* DESCRIPTION:
*         Avago interface
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*
*******************************************************************************/
#ifdef MV_HWS_BIN_HEADER
#include "mvSiliconIf.h"
#include <gtGenTypes.h>
#include "mvHighSpeedEnvSpec.h"
#else
#include <common/siliconIf/mvSiliconIf.h>
#include <common/siliconIf/siliconAddress.h>
#include <serdes/avago/mvAvagoIf.h>
#endif

#if !defined MV_HWS_REDUCED_BUILD_EXT_CM3 || defined MV_HWS_BIN_HEADER
#include "sd28firmware/avago_fw_load.h"
#endif /* MV_HWS_REDUCED_BUILD_EXT_CM3 */

#include "mv_hws_avago_if.h"
/* Avago include */
#include "aapl.h"
#include "aapl_core.h"
#include "sbus.h"

#if !defined(ASIC_SIMULATION) && !defined(MV_HWS_REDUCED_BUILD_EXT_CM3)
#   if AAPL_ENABLE_AACS_SERVER
#   include <aacs_server.h>

#   endif /* AAPL_ENABLE_AACS_SERVER */
#endif /* !defined(ASIC_SIMULATION) && !defined(MV_HWS_REDUCED_BUILD_EXT_CM3) */
/************************* Globals *******************************************************/

unsigned int avagoConnection = AVAGO_SBUS_CONNECTION;
Aapl_t* aaplSerdesDb[HWS_MAX_DEVICE_NUM] = {0};
Aapl_t  aaplSerdesDbDef[HWS_MAX_DEVICE_NUM];

#ifndef MV_HWS_BIN_HEADER
static unsigned int aacsServerEnable = 0;

/* Avago server process Id */
static GT_U32 avagoAACS_ServerTid;
#endif /*MV_HWS_BIN_HEADER*/

#ifdef MARVELL_AVAGO_DB_BOARD
unsigned int mvAvagoDb = 1;
#else
unsigned int mvAvagoDb = 0;
#endif /* MARVELL_AVAGO_DB_BOARD */

GT_U8 serdesToAvagoMap[MAX_AVAGO_SERDES_NUMBER] =
{
    [20] 14, /* Logical SERDES 20  == Physical SERDES 14 on SERDES chain */
    [24] 13, /* Logical SERDES 24  == Physical SERDES 13 on SERDES chain */
    [25] 12, /* Logical SERDES 25  == Physical SERDES 12 on SERDES chain */
    [26] 11, /* Logical SERDES 26  == Physical SERDES 11 on SERDES chain */
    [27] 10, /* Logical SERDES 27  == Physical SERDES 10 on SERDES chain */
    [31] 8,  /* Logical SERDES 31  == Physical SERDES 08 on SERDES chain */
    [30] 7,  /* Logical SERDES 30  == Physical SERDES 07 on SERDES chain */
    [29] 6,  /* Logical SERDES 29  == Physical SERDES 06 on SERDES chain */
    [28] 5,  /* Logical SERDES 28  == Physical SERDES 05 on SERDES chain */
    [32] 4,  /* Logical SERDES 32  == Physical SERDES 04 on SERDES chain */
    [33] 3,  /* Logical SERDES 33  == Physical SERDES 03 on SERDES chain */
    [34] 2,  /* Logical SERDES 34  == Physical SERDES 02 on SERDES chain */
    [35] 1   /* Logical SERDES 35  == Physical SERDES 01 on SERDES chain */

    /* Temp Sensor = Physical SPICO 09 on SERDES chain */
};

GT_U8 avagoToSerdesMap[MAX_AVAGO_SPICO_NUMBER] =
{
    [14] 20, /* Logical SERDES 20  == Physical SERDES 14 on SERDES chain */
    [13] 24, /* Logical SERDES 24  == Physical SERDES 13 on SERDES chain */
    [12] 25, /* Logical SERDES 25  == Physical SERDES 12 on SERDES chain */
    [11] 26, /* Logical SERDES 26  == Physical SERDES 11 on SERDES chain */
    [10] 27, /* Logical SERDES 27  == Physical SERDES 10 on SERDES chain */
    [8]  31, /* Logical SERDES 31  == Physical SERDES 08 on SERDES chain */
    [7]  30, /* Logical SERDES 30  == Physical SERDES 07 on SERDES chain */
    [6]  29, /* Logical SERDES 29  == Physical SERDES 06 on SERDES chain */
    [5]  28, /* Logical SERDES 28  == Physical SERDES 05 on SERDES chain */
    [4]  32, /* Logical SERDES 32  == Physical SERDES 04 on SERDES chain */
    [3]  33, /* Logical SERDES 33  == Physical SERDES 03 on SERDES chain */
    [2]  34, /* Logical SERDES 34  == Physical SERDES 02 on SERDES chain */
    [1]  35  /* Logical SERDES 35  == Physical SERDES 01 on SERDES chain */

    /* Temp Sensor = Physical SPICO 09 on SERDES chain */
};


/************************* * Pre-Declarations *******************************************************/
#ifndef ASIC_SIMULATION
extern GT_STATUS mvHwsAvagoInitI2cDriver(GT_VOID);
#endif /* ASIC_SIMULATION */

void mvHwsAvagoAccessValidate(unsigned char devNum, uint sbus_addr);
GT_STATUS mvHwsAvagoCheckSerdesAccess(unsigned int devNum, unsigned char portGroup, unsigned char serdesNum);

/***************************************************************************************************/

int mvHwsAvagoInitializationCheck
(
    unsigned char devNum,
    unsigned int  serdesNum
)
{
    return (aaplSerdesDb[devNum] == NULL) ? GT_NOT_INITIALIZED : GT_OK;
}

int mvHwsAvagoConvertSerdesToSbusAddr
(
    unsigned char devNum,
    unsigned int  serdesNum,
    unsigned int  *sbusAddr
)
{
    CHECK_STATUS(mvHwsAvagoInitializationCheck(devNum, serdesNum));

    if(serdesNum >= MAX_AVAGO_SERDES_NUMBER)
    {
        return GT_BAD_PARAM;
    }

    *sbusAddr = serdesToAvagoMap[serdesNum];

    return GT_OK;
}

int mvHwsAvagoConvertSbusAddrToSerdes
(
    unsigned char *serdesNum,
    unsigned int  sbusAddr
)
{
    if(sbusAddr >= MAX_AVAGO_SPICO_NUMBER)
    {
        return GT_BAD_PARAM;
    }

    *serdesNum = avagoToSerdesMap[sbusAddr];

    return GT_OK;
}


#ifndef MV_HWS_REDUCED_BUILD_EXT_CM3
/*******************************************************************************
* mvHwsAvagoEthDriverInit
*
* DESCRIPTION:
*       Initialize Avago related configurations
*
* INPUTS:
*       devNum    - system device number
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
unsigned int mvHwsAvagoEthDriverInit(unsigned char devNum)
{
#ifndef ASIC_SIMULATION
    /* Set default values: */

    char ip_address[] = "10.5.32.124";
    int     tcp_port     = 90; /* Default port for Avago HS1/PS1 */

    aapl_connect(aaplSerdesDb[devNum], ip_address, tcp_port);
    if(aaplSerdesDb[devNum]->return_code < 0)
    {
        osPrintf("aapl_connect failed (return code 0x%x)\n", aaplSerdesDb[devNum]->return_code);
        return GT_INIT_ERROR;
    }

#endif /* ASIC_SIMULATION */
    return GT_OK;
}

#if AAPL_ENABLE_AACS_SERVER

#ifndef MV_HWS_BIN_HEADER
extern unsigned int osTaskCreate(const char *name, unsigned int prio, unsigned int stack,
    unsigned (__TASKCONV *start_addr)(void*), void *arglist, unsigned int *tid);

/*******************************************************************************
* mvHwsAvagoSerdesDebugInit
*
* DESCRIPTION:
*       Create Avago AACS Server Process
*
* INPUTS:
*       task name   - AACS server process identifier
*       taskRoutine - AACS server process entry point
*       devNum        - system device number
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
GT_STATUS mvHwsAvagoSerdesDebugInit
(
    char *name,
    unsigned (__TASKCONV *taskRoutine)(GT_VOID*),
    unsigned char devNum
)
{
    void * params = (void *)(GT_UINTPTR)devNum;

    if((osTaskCreate(name, 250, _8KB, taskRoutine, params, &avagoAACS_ServerTid)) != GT_OK)
    {
        return GT_FAIL;
    }

    return GT_OK;
}

/*******************************************************************************
* avagoAACS_ServerRoutine
*
* DESCRIPTION:
*       Initialize Avago AACS Server
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0  - on success
*******************************************************************************/
static unsigned __TASKCONV avagoAACS_ServerRoutine
(
    IN GT_VOID * param
)
{
    unsigned int tcpPort = 90;

    avago_aacs_server(aaplSerdesDb[0], tcpPort);

    return GT_OK;
}

/*******************************************************************************
* avagoSerdesAacsServerExec
*
* DESCRIPTION:
*       Initialize the Avago AACS Server
*
* INPUTS:
*       devNum - system device number
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0  - on success
*       1  - on error
*******************************************************************************/
#ifndef ASIC_SIMULATION
int avagoSerdesAacsServerExec(unsigned char devNum)
{
    int res;

    res = mvHwsAvagoSerdesDebugInit("mvHwsAvagoAACS_Server", avagoAACS_ServerRoutine, devNum);
    if (res != GT_OK)
    {
        osPrintf("Failed to init Avago AACS Server\n");
        return res;
    }

    return GT_OK;
}
#endif /* ASIC_SIMULATION */

#endif /* MV_HWS_BIN_HEADER */
#endif /* AAPL_ENABLE_AACS_SERVER */
#endif /* MV_HWS_REDUCED_BUILD_EXT_CM3 */

#if !defined MV_HWS_REDUCED_BUILD_EXT_CM3 || defined MV_HWS_BIN_HEADER

/*******************************************************************************
* mvHwsAvagoSpicoLoad
*
* DESCRIPTION:
*       Load Avago SPICO Firmware
*******************************************************************************/
int mvHwsAvagoSpicoLoad(unsigned char devNum, unsigned int sbus_addr)
{
#ifdef FW_DOWNLOAD_FROM_SERVER
    AVAGO_DBG(("Loading file: %s to SBus address %x\n", serdesFileName, sbus_addr));
    avago_spico_upload_file(aaplSerdesDb[devNum], sbus_addr, FALSE, serdesFileName);

    if (serdesSwapFileName != NULL)
    {
        AVAGO_DBG(("Loading swap file: %s to SBus address %x\n", serdesSwapFileName, sbus_addr));
        avago_spico_upload_file(aaplSerdesDb[devNum], sbus_addr, FALSE,serdesSwapFileName);
    }
#else /* Download from embedded firmware file */
    AVAGO_DBG(("Loading to SBus address %x data[0]=%x\n", sbus_addr, serdesFwPtr[0]));
    CHECK_STATUS(avago_spico_upload(aaplSerdesDb[devNum], sbus_addr, FALSE,
                                    AVAGO_SERDES_FW_IMAGE_SIZE, serdesFwPtr));
#ifdef AVAGO_FW_SWAP_IMAGE_EXIST
    CHECK_STATUS(avago_spico_upload_swap_image(aaplSerdesDb[devNum], sbus_addr,
                                               AVAGO_SERDES_FW_SWAP_IMAGE_SIZE, serdesFwDataSwapPtr));
#endif /* AVAGO_FW_SWAP_IMAGE_EXIST */

#endif /* FW_DOWNLOAD_FROM_SERVER */

    return GT_OK;
}

/*******************************************************************************
* mvHwsAvagoSbusMasterLoad
*
* DESCRIPTION:
*       Load Avago Sbus Master Firmware
*******************************************************************************/
int mvHwsAvagoSbusMasterLoad(unsigned char devNum)
{
    unsigned int sbus_addr = AVAGO_SBUS_MASTER_ADDRESS;

#ifdef FW_DOWNLOAD_FROM_SERVER
    AVAGO_DBG(("Loading file: %s to SBus address %x\n", sbusMasterFileName, sbus_addr));
    avago_spico_upload_file(aaplSerdesDb[devNum], sbus_addr , FALSE, sbusMasterFileName);
#else
    AVAGO_DBG(("Loading to SBus address %x  data[0]=%x\n", sbus_addr , sbusMasterFwPtr[0]));
    CHECK_STATUS(avago_spico_upload(aaplSerdesDb[devNum], sbus_addr, FALSE,
                                    AVAGO_SBUS_MASTER_FW_IMAGE_SIZE, sbusMasterFwPtr));
#endif /* FW_DOWNLOAD_FROM_SERVER */

    return GT_OK;
}

/*******************************************************************************
* mvHwsAvagoSerdesInit
*
* DESCRIPTION:
*       Initialize Avago related configurations
*
* INPUTS:
*       devNum - system device number
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0  - on success
*       1  - on error
*******************************************************************************/
#ifndef ASIC_SIMULATION
int mvHwsAvagoSerdesInit(unsigned char devNum)
{
    unsigned int sbus_addr;
    unsigned int curr_adr;
    unsigned int chip=devNum;
    unsigned int ring =0; /*we have only one ring*/
    Avago_addr_t addr_struct;
#ifndef AVAGO_AAPL_LGPL
    unsigned int addr = avago_make_addr3(AVAGO_BROADCAST, AVAGO_BROADCAST, AVAGO_BROADCAST);
#endif
    unsigned char serdes_num;

    /* Validate AAPL */
    if (aaplSerdesDb[devNum] != NULL)
    {
        /* structures were already initialized */
        return GT_ALREADY_EXIST;
    }

    /* Construct AAPL structure */
    aapl_init(&(aaplSerdesDbDef[devNum]));
    aaplSerdesDb[devNum] = &(aaplSerdesDbDef[devNum]);

    aaplSerdesDb[devNum]->devNum    = devNum;
    aaplSerdesDb[devNum]->portGroup = 0;

    /*Take Avago device out of reset */
    user_supplied_sbus_soft_reset(aaplSerdesDb[devNum]);

#ifndef MV_HWS_BIN_HEADER
    if (avagoConnection == AVAGO_ETH_CONNECTION)
    {
        aaplSerdesDb[devNum]->communication_method = AAPL_DEFAULT_COMM_METHOD;
        CHECK_STATUS(mvHwsAvagoEthDriverInit(devNum));
    }
    else if (avagoConnection == AVAGO_I2C_CONNECTION)
    {
        aaplSerdesDb[devNum]->communication_method = AVAGO_USER_SUPPLIED_I2C;
        CHECK_STATUS(mvHwsAvagoInitI2cDriver());
    }
    else if (avagoConnection == AVAGO_SBUS_CONNECTION)
    {
        aaplSerdesDb[devNum]->communication_method = AVAGO_USER_SUPPLIED_SBUS_DIRECT;
    }
    else
    {
        osPrintf("mvHwsAvagoSerdesInit : unknown communication method %x\n",
                          aaplSerdesDb[devNum]->communication_method);
        return GT_INIT_ERROR;
    }

    /* Validate access to Avago device */
    mvHwsAvagoAccessValidate(devNum, 0);

    /* Initialize AAPL structure */
    aapl_get_ip_info(aaplSerdesDb[devNum],1);

#ifndef AVAGO_AAPL_LGPL
    /* Print AAPL structure */
    aapl_print_struct(aaplSerdesDb[devNum],aaplSerdesDb[devNum]->debug > 0, addr, 0);
#endif /* AVAGO_AAPL_LGPL */

#else /* MV_HWS_BIN_HEADER */
    /* Validate access to Avago device */
    mvHwsAvagoAccessValidate(devNum, 0);

    /* Initialize AAPL structure */
    aapl_get_ip_info(aaplSerdesDb[devNum],1);
#endif /*MV_HWS_BIN_HEADER*/
    AVAGO_DBG(("Loading Avago Firmware.......\n"));

    /* Converts the address into an address structure */
    addr_struct.chip = devNum;
    addr_struct.ring = ring;

    /* Download Serdes's Firmware */
    /* ========================== */
    for( curr_adr = 1; curr_adr <= aaplSerdesDb[devNum]->max_sbus_addr[chip][ring]; curr_adr++ )
    {
        if (aaplSerdesDb[devNum]->ip_type[0][0][curr_adr] == AVAGO_SERDES)
        {
            addr_struct.sbus = curr_adr;
            sbus_addr = avago_struct_to_addr(&addr_struct);

            CHECK_STATUS(mvHwsAvagoConvertSbusAddrToSerdes(&serdes_num, curr_adr));
            if (mvHwsAvagoCheckSerdesAccess(devNum, 0, serdes_num) == GT_NOT_INITIALIZED)
            {
                CHECK_STATUS(mvHwsAvagoSpicoLoad(devNum, sbus_addr));
            }
        }
    }

    /* Download SBus_Master Firmware */
    /* ============================= */
    CHECK_STATUS(mvHwsAvagoSbusMasterLoad(devNum));

    if(aaplSerdesDb[devNum]->return_code < 0)
    {
        osPrintf("Avago FW Load Failed (return code 0x%x)\n", aaplSerdesDb[devNum]->return_code);
        return GT_INIT_ERROR;
    }

    AVAGO_DBG(("Done\n"));

#ifndef MV_HWS_BIN_HEADER
    if (avagoConnection != AVAGO_ETH_CONNECTION)
    {
        if (aacsServerEnable)
        {
            CHECK_STATUS(avagoSerdesAacsServerExec(devNum));
        }
    }
#endif /* MV_HWS_BIN_HEADER */

    return GT_OK;
}
#endif /* ASIC_SIMULATION */

#elif defined MV_HWS_REDUCED_BUILD_EXT_CM3  && !defined MV_HWS_BIN_HEADER

/*******************************************************************************
* mvHwsAvagoSerdesInit
*
* DESCRIPTION:
*       Initialize Avago related configurations
*
* INPUTS:
*       devNum - system device number
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0  - on success
*       1  - on error
*******************************************************************************/
int mvHwsAvagoSerdesInit(unsigned char devNum)
{
    /* Validate AAPL */
    if (aaplSerdesDb[devNum] != NULL)
    {
        /* structures were already initialized */
        return GT_ALREADY_EXIST;
    }

    /* Initialize AAPL structure in CM3 */

    return GT_OK;
}

#endif /* MV_HWS_REDUCED_BUILD_EXT_CM3 */

#ifndef ASIC_SIMULATION
/*******************************************************************************
* mvHwsAvagoSerdesSpicoInterrupt
*
* DESCRIPTION:
*       Issue the interrupt to the Spico processor.
*       The return value is the interrupt number.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       serdesNum - physical serdes number
*       interruptCode - Code of interrupt
*       interruptData - Data to write
*
* OUTPUTS:
*       result - spico interrupt return value
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
int mvHwsAvagoSerdesSpicoInterrupt
(
    unsigned char   devNum,
    unsigned int    portGroup,
    unsigned int    serdesNum,
    unsigned int    interruptCode,
    unsigned int    interruptData,
    unsigned int    *result
)
{
    unsigned int  sbus_addr;

    CHECK_STATUS(mvHwsAvagoConvertSerdesToSbusAddr(devNum, serdesNum, &sbus_addr));

    if (result == NULL)
    {
        avago_spico_int(aaplSerdesDb[devNum], sbus_addr, interruptCode, interruptData);
    }
    else
    {
        *result = avago_spico_int(aaplSerdesDb[devNum], sbus_addr, interruptCode, interruptData);
    }

    CHECK_AVAGO_RET_CODE();

    return GT_OK;
}

/*******************************************************************************
* mvHwsAvagoSerdesTemperatureGet
*
* DESCRIPTION:
*       Get the Temperature (in C) from Avago Serdes
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       serdesNum - physical serdes number
*
* OUTPUTS:
*       temperature - Serdes temperature degree value
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
int mvHwsAvagoSerdesTemperatureGet
(
    unsigned char   devNum,
    unsigned int    portGroup,
    unsigned int    serdesNum,
    int             *temperature
)
{
    unsigned int  sbus_addr;

    if ((serdesNum != 20) && ((serdesNum < 24) || (serdesNum > 27)))
    {
        return GT_BAD_PARAM;
    }

    CHECK_STATUS(mvHwsAvagoConvertSerdesToSbusAddr(devNum, serdesNum, &sbus_addr));

    /* get the Serdes Temperature in degrees */
    *temperature = (avago_sbm_get_temperature(aaplSerdesDb[devNum], 9, sbus_addr));
    CHECK_AVAGO_RET_CODE();

    return GT_OK;
}

/*******************************************************************************
* mvHwsAvagoSerdesCalCodeSet
*
* DESCRIPTION:
*       Set the calibration code(value) for Rx or Tx
*
* INPUTS:
*       devNum    - device number
*       portGroup - port group (core) number
*       serdesNum - SERDES number
*       calCode   - Rx or Tx calibration code
*       mode      - True for Rx mode, False for Tx mode
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
unsigned int mvHwsAvagoSerdesCalCodeSet(int devNum, int portGroup, int serdesNum, int calCode, BOOL mode)
{
    unsigned int data;

    /* get the calibration code */
    CHECK_STATUS(mvHwsAvagoSerdesSpicoInterrupt(devNum, portGroup, serdesNum, 0x28, 0x4000 | (mode << 15), &data));
    /* get only bits #7-15 */
    data &= 0xFF80;

    /* set calibration code */
    CHECK_STATUS(mvHwsAvagoSerdesSpicoInterrupt(devNum, portGroup, serdesNum, 0x28, data | calCode | (mode<<15), NULL));

    return GT_OK;
}

/*******************************************************************************
* mvHwsAvagoSerdesCalCodeGet
*
* DESCRIPTION:
*       Get the calibration code(value) for Rx or Tx
*
* INPUTS:
*       devNum    - device number
*       portGroup - port group (core) number
*       serdesNum - SERDES number
*       mode      - True for Rx mode, False for Tx mode
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
unsigned int mvHwsAvagoSerdesCalCodeGet(int devNum, int portGroup, int serdesNum, BOOL mode)
{
    unsigned int data;

    /* get the calibration code */
    mvHwsAvagoSerdesSpicoInterrupt(devNum, portGroup, serdesNum, 0x28, 0x4000 | (mode << 15), &data);

    /* cf code are only bits 0..6 */
    return data & 0x7F;
}

/*******************************************************************************
* mvHwsAvagoSerdesVcoConfig
*
* DESCRIPTION:
*       Compensate the VCO calibration value according to Temperature in order
*       to enable Itemp operation
*
* INPUTS:
*       devNum    - device number
*       portGroup - port group (core) number
*       serdesNum - SERDES number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
unsigned int mvHwsAvagoSerdesVcoConfig
(
    unsigned char       devNum,
    unsigned int        portGroup,
    unsigned int        serdesNum
)
{
    int temperature, res;
    unsigned int rxCalCode = mvHwsAvagoSerdesCalCodeGet(devNum, portGroup, serdesNum, TRUE);
    unsigned int txCalCode = mvHwsAvagoSerdesCalCodeGet(devNum, portGroup, serdesNum, FALSE);

    /* get the Temperature from Serdes #20 */
    CHECK_STATUS(mvHwsAvagoSerdesTemperatureGet(devNum, portGroup, 20, &temperature));

    if (temperature < -20)
    {
        rxCalCode += 2;
        txCalCode += 2;
    }
    else if ((temperature >= -20) && (temperature <= 0))
    {
        rxCalCode += 1;
        txCalCode += 1;
    }
    else if ((temperature > 30) && (temperature <= 75))
    {
        rxCalCode -= 1;
        txCalCode -= 1;
    }
    else if ((temperature > 75) && (temperature <= 125))
    {
        rxCalCode -= 2;
        txCalCode -= 2;
    }

    /* Set the calibration code for Rx */
    res = mvHwsAvagoSerdesCalCodeSet(devNum, portGroup, serdesNum, rxCalCode, TRUE);
    if (res != GT_OK)
    {
        osPrintf("mvHwsAvagoSerdesCalCodeSet failed (%d)\n", res);
        return GT_FAIL;
    }

    /* Set the calibration code for Tx */
    res = mvHwsAvagoSerdesCalCodeSet(devNum, portGroup, serdesNum, txCalCode, FALSE);
    if (res != GT_OK)
    {
        osPrintf("mvHwsAvagoSerdesCalCodeSet failed (%d)\n", res);
        return GT_FAIL;
    }

    return GT_OK;
}
#endif /* ASIC_SIMULATION */

/*******************************************************************************
* mvHwsAvagoSerdesPowerCtrlImpl
*
* DESCRIPTION:
*       Power up SERDES list.
*
* INPUTS:
*       devNum    - device number
*       portGroup - port group (core) number
*       serdesNum - SERDES number
*       powerUp   - True for PowerUP, False for PowerDown
*       divider   - divider of Serdes speed
*       refClock  - ref clock value
*       refClockSource - ref clock source (primary line or secondary)
*       media     - RXAUI or XAUI
*       mode      - Serdes bus modes: 10Bits/20Bits/40Bits
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
int mvHwsAvagoSerdesPowerCtrlImpl
(
    unsigned char       devNum,
    unsigned int        portGroup,
    unsigned int        serdesNum,
    unsigned char       powerUp,
    unsigned int        divider,
    unsigned char       refClock,
    unsigned char       refClockSource,
    unsigned char       media,
    unsigned char       mode
)
{
    Avago_serdes_init_config_t *config;
    Avago_serdes_init_config_t configDef;

    unsigned int sbus_addr;
    unsigned int errors;
    unsigned int data;
    int res;

    CHECK_STATUS(mvHwsAvagoConvertSerdesToSbusAddr(devNum, serdesNum, &sbus_addr));
    if (sbus_addr == AVAGO_INVALID_SBUS_ADDR)
    {
        osPrintf("Invalid HW configuration !!!\n");
        return GT_OK;
    }

#ifndef MV_HWS_BIN_HEADER
    /* for Serdes PowerDown */
    if (powerUp == GT_FALSE)
    {
        /* Rx, Tx, Output = disable */
        CHECK_STATUS(mvHwsAvagoSerdesSpicoInterrupt(devNum, portGroup, serdesNum, 0x1, 0, NULL));

        /* Serdes Digital UnReset */
        CHECK_STATUS(mvHwsAvagoSerdesResetImpl(devNum, portGroup, serdesNum, GT_FALSE, GT_FALSE, GT_FALSE));

        return GT_OK;
    }
#endif /* MV_HWS_BIN_HEADER */

    /* for Serdes PowerUp */
    /* Initialize the SerDes slice */
    avago_serdes_init_config(&configDef);
    config = &configDef;

    /* Change the config struct values from their defaults: */
    if (divider != NA_VALUE) config->tx_divider = config->rx_divider = divider;

    /* initializes the Avago_serdes_init_config_t struct */
    config->sbus_reset = FALSE;
    config->signal_ok_threshold = 2;

    /* Select the Rx & Tx data path width */
    if (mode == _10BIT_ON) config->rx_width = config->tx_width = 10;
    else if (mode == _20BIT_ON) config->rx_width = config->tx_width = 20;
    else if (mode == _40BIT_ON) config->rx_width = config->tx_width = 40;

    if (mvAvagoDb)
    {
        config->init_mode = AVAGO_PRBS31_ILB;
        osPrintf("Note: This is Avago DB configuration (with PRBS)\n");
    }
    else
    {
        config->init_mode = AVAGO_INIT_ONLY;
    }

    AVAGO_DBG(("mvHwsAvagoSerdesPowerCtrlImpl init_configuration:\n"));
    AVAGO_DBG(("   sbus_reset = %x \n",config->sbus_reset));
    AVAGO_DBG(("   init_tx = %x \n",config->init_tx));
    AVAGO_DBG(("   init_rx = %x \n",config->init_rx));
    AVAGO_DBG(("   init_mode = %x \n",config->init_mode));
    AVAGO_DBG(("   tx_divider = 0x%x \n",config->tx_divider));
    AVAGO_DBG(("   rx_divider = 0x%x \n",config->rx_divider));
    AVAGO_DBG(("   tx_width = 0x%x \n",config->tx_width));
    AVAGO_DBG(("   rx_width = 0x%x \n",config->rx_width));
    AVAGO_DBG(("   tx_phase_cal = %x \n",config->tx_phase_cal));
    AVAGO_DBG(("   tx_output_en = %x \n",config->tx_output_en));
    AVAGO_DBG(("   signal_ok_en = %x \n",config->signal_ok_en));
    AVAGO_DBG(("   signal_ok_threshold= %x \n",config->signal_ok_threshold));

    /* Serdes Analog Un Reset*/
    CHECK_STATUS(mvHwsAvagoSerdesResetImpl(devNum, portGroup, serdesNum, GT_FALSE, GT_TRUE, GT_TRUE));

    /* config media */
    data = (media == RXAUI_MEDIA) ? (1 << 2) : 0;
    CHECK_STATUS(hwsSerdesRegSetFuncPtr(devNum, portGroup, EXTERNAL_REG, serdesNum, SERDES_EXTERNAL_CONFIGURATION_0, data, (1 << 2)));

    /* Reference clock source */
    data = ((refClockSource == PRIMARY) ? 0 : 1) << 8;
    CHECK_STATUS(hwsSerdesRegSetFuncPtr(devNum, portGroup, EXTERNAL_REG, serdesNum, SERDES_EXTERNAL_CONFIGURATION_0, data, (1 << 8)));
    errors = avago_serdes_init(aaplSerdesDb[devNum], sbus_addr, config);

    CHECK_AVAGO_RET_CODE();
    if (errors > 0)
    {
#ifndef MV_HWS_REDUCED_BUILD
        osPrintf("SerDes init complete for SerDes at addr %s; Errors in ILB: %d. \n", aapl_addr_to_str(sbus_addr), errors);
#else
        osPrintf("SerDes init complete for SerDes at addr 0x%x; Errors in ILB: %d. \n", sbus_addr, errors);
#endif /* MV_HWS_REDUCED_BUILD */
    }
    if (errors == 0 && aapl_get_return_code(aaplSerdesDb[devNum]) == 0)
    {
#ifndef MV_HWS_REDUCED_BUILD
        AVAGO_DBG(("The SerDes at address %s is initialized.\n", aapl_addr_to_str(sbus_addr)));
#else
        AVAGO_DBG(("The SerDes at address 0x%x is initialized.\n", sbus_addr));
#endif /* MV_HWS_REDUCED_BUILD */
    }

#ifndef ASIC_SIMULATION
    /* Compensate the VCO calibration value according to Temperature */
    res = mvHwsAvagoSerdesVcoConfig(devNum, portGroup, serdesNum);
    if(res != GT_OK)
    {
        osPrintf("mvHwsAvagoSerdesVcoConfig failed (%d)\n", res);
        return GT_FAIL;
    }
#endif /* ASIC_SIMULATION */

    /* for ICAL: disable the one-shot pCal, send SPICO interrupt before iCal is enabled */
    CHECK_STATUS(mvHwsAvagoSerdesSpicoInterrupt(devNum, portGroup, serdesNum, 0x26, 0x5B01, NULL));

    /* Serdes Digital UnReset */
    CHECK_STATUS(mvHwsAvagoSerdesResetImpl(devNum, portGroup, serdesNum, GT_FALSE, GT_FALSE, GT_FALSE));

    return GT_OK;
}

/*******************************************************************************
* mvHwsAvagoSerdesResetImpl
*
* DESCRIPTION:
*       Per SERDES Clear the serdes registers (back to defaults.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       serdesNum - serdes number
*       analogReset - Analog Reset (On/Off)
*       digitalReset - digital Reset (On/Off)
*       syncEReset - SyncE Reset (On/Off)
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
unsigned int mvHwsAvagoSerdesResetImpl
(
    unsigned char    devNum,
    unsigned int     portGroup,
    unsigned int     serdesNum,
    unsigned int     analogReset,
    unsigned int     digitalReset,
    unsigned int     syncEReset
)
{
#ifndef ASIC_SIMULATION
    GT_U32  data;

    /* SERDES SD RESET/UNRESET init */
    data = (analogReset == GT_TRUE) ? 0 : 1;
    CHECK_STATUS(hwsSerdesRegSetFuncPtr(devNum, portGroup, EXTERNAL_REG, serdesNum, SERDES_EXTERNAL_CONFIGURATION_1, (data << 2), (1 << 2)));

    /* SERDES RF RESET/UNRESET init */
    data = (digitalReset == GT_TRUE) ? 0 : 1;
    CHECK_STATUS(hwsSerdesRegSetFuncPtr(devNum, portGroup, EXTERNAL_REG, serdesNum, SERDES_EXTERNAL_CONFIGURATION_1, (data << 3), (1 << 3)));

    /* SERDES SYNCE RESET init */
    if(syncEReset == GT_TRUE)
    {
        CHECK_STATUS(hwsSerdesRegSetFuncPtr(devNum, portGroup, EXTERNAL_REG, serdesNum, SERDES_EXTERNAL_CONFIGURATION_0, 0, (1 << 6)));
    }
    else /* SERDES SYNCE UNRESET init */
    {
        CHECK_STATUS(hwsSerdesRegSetFuncPtr(devNum, portGroup, EXTERNAL_REG, serdesNum, SERDES_EXTERNAL_CONFIGURATION_0, (1 << 6), (1 << 6)));
        CHECK_STATUS(hwsSerdesRegSetFuncPtr(devNum, portGroup, EXTERNAL_REG, serdesNum, SERDES_EXTERNAL_CONFIGURATION_1, 0xDD00, 0xFF00));
    }

#endif /* ASIC_SIMULATION */
    return GT_OK;
}

/*******************************************************************************
* mvHwsAvagoSerdesPolarityConfigImpl
*
* DESCRIPTION:
*       Per Serdes invert the Tx or Rx.
*       Can be run after create port.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       serdesNum - physical Serdes number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
int mvHwsAvagoSerdesPolarityConfigImpl
(
    unsigned char     devNum,
    unsigned int      portGroup,
    unsigned int      serdesNum,
    unsigned int     invertTx,
    unsigned int     invertRx
)
{
#ifndef ASIC_SIMULATION
    unsigned int sbus_addr;

    CHECK_STATUS(mvHwsAvagoConvertSerdesToSbusAddr(devNum, serdesNum, &sbus_addr));

    /* Tx polarity En */
    avago_serdes_set_tx_invert(aaplSerdesDb[devNum], sbus_addr, invertTx);
    CHECK_AVAGO_RET_CODE();

    /* Rx Polarity En */
    avago_serdes_set_rx_invert(aaplSerdesDb[devNum], sbus_addr, invertRx);
    CHECK_AVAGO_RET_CODE();

#endif /* ASIC_SIMULATION */
    return GT_OK;
}

/*******************************************************************************
* mvHwsAvagoSerdesPolarityConfigGetImpl
*
* DESCRIPTION:
*       Returns the Tx and Rx SERDES invert state.
*       Can be run after create port.
*
* INPUTS:
*       devNum    - system device number
*       portGroup - port group (core) number
*       serdesNum - physical serdes number
*
* OUTPUTS:
*       invertTx - invert TX polarity (GT_TRUE - invert, GT_FALSE - don't)
*       invertRx - invert RX polarity (GT_TRUE - invert, GT_FALSE - don't)
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
int mvHwsAvagoSerdesPolarityConfigGetImpl
(
    unsigned char    devNum,
    unsigned int     portGroup,
    unsigned int     serdesNum,
    unsigned int     *invertTx,
    unsigned int     *invertRx
)
{
#ifndef ASIC_SIMULATION
    unsigned int sbus_addr;

    CHECK_STATUS(mvHwsAvagoConvertSerdesToSbusAddr(devNum, serdesNum, &sbus_addr));

    /* Get the TX inverter polarity mode: TRUE - inverter is enabled, FALSE - data is not being inverted */
    *invertTx = avago_serdes_get_tx_invert(aaplSerdesDb[devNum], sbus_addr);
    CHECK_AVAGO_RET_CODE();

    /* Get the RX inverter polarity mode: TRUE - inverter is enabled, FALSE - data is not being inverted */
    *invertRx = avago_serdes_get_rx_invert(aaplSerdesDb[devNum], sbus_addr);
    CHECK_AVAGO_RET_CODE();

#endif /* ASIC_SIMULATION */
    return GT_OK;
}

/*******************************************************************************
* mvHwsAvagoSerdesSbmVoltageGet
*
* DESCRIPTION:
*       Gets the voltage data from a given AVAGO_THERMAL_SENSOR sensor.
*       Returns the voltage in milli-volt.
*
* INPUTS:
*       devNum      - system device number
*       portGroup   - port group (core) number
*       serdesNum   - physical serdes number
*       sensorAddr  - SBus address of the AVAGO_THERMAL_SENSOR
*
* OUTPUTS:
*       voltage - Serdes voltage in milli-volt
*
* RETURNS:
*       0  - on success
*       1  - on error
*
*******************************************************************************/
int mvHwsAvagoSerdesSbmVoltageGet
(
    unsigned char   devNum,
    unsigned int    portGroup,
    unsigned int    serdesNum,
    unsigned int    sensorAddr,
    unsigned int    *voltage
)
{
#ifndef ASIC_SIMULATION
    unsigned int    sbus_addr;
    GT_U32          data, interruptData;
    Avago_addr_t    addr_struct;
    unsigned int addr = avago_make_sbus_master_addr(sensorAddr);

    CHECK_STATUS(mvHwsAvagoConvertSerdesToSbusAddr(devNum, serdesNum, &sbus_addr));

    avago_addr_to_struct(sensorAddr, &addr_struct);

    interruptData = (sbus_addr << 12) | (addr_struct.sbus & 0xff);
    data = avago_spico_int(aaplSerdesDb[devNum], addr, 0x18, interruptData);
    CHECK_AVAGO_RET_CODE();

    /* result is a 12b signed value in 0.5mv increments */
    if(data & 0x8000)
    {  /* bit[15] indicates result is good */
        data &= 0x0FFF;         /* Mask down to 12b voltage value */
        *voltage = (data / 2);  /* Scale to milli-volt. */
    }
    else
        return GT_BAD_VALUE;  /* voltage not valid */

#endif /* ASIC_SIMULATION */
    return GT_OK;
}

/*******************************************************************************
* mvHwsAvagoAccessValidate
*
* DESCRIPTION:
*       Validate access to Avago device
*
* INPUTS:
*       unsigned char devNum
*       uint sbus_addr
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
*******************************************************************************/
void mvHwsAvagoAccessValidate
(
    unsigned char devNum,
    uint sbus_addr
)
{
    AVAGO_DBG(("Validate SBUS access (sbus_addr 0x%x)- ", sbus_addr));
    if (avago_diag_sbus_rw_test(aaplSerdesDb[devNum], avago_make_sbus_controller_addr(sbus_addr), 2) == TRUE)
    {
        AVAGO_DBG(("Access Verified\n"));
    }
    else
    {
        AVAGO_DBG(("Access Failed\n"));
    }
}

/*******************************************************************************
* mvHwsAvagoCheckSerdesAccess
*
* DESCRIPTION:
*       Validate access to Avago Serdes
*
* INPUTS:
*       unsigned int  devNum
*       unsigned char portGroup
*       unsigned char serdesNum
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
*******************************************************************************/
GT_STATUS mvHwsAvagoCheckSerdesAccess
(
    unsigned int  devNum,
    unsigned char portGroup,
    unsigned char serdesNum
)
{
  GT_UREG_DATA data;

  /* check analog reset */
  CHECK_STATUS(hwsSerdesRegGetFuncPtr(devNum, portGroup, EXTERNAL_REG, serdesNum,
                                      SERDES_EXTERNAL_CONFIGURATION_1, &data, 0));

  if(((data >> 3) & 1) == 0)
      return GT_NOT_INITIALIZED;

  return GT_OK;
}




