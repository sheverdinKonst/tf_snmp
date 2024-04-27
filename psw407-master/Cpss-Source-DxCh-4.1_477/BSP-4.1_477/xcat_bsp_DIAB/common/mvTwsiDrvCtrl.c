/*******************************************************************************
*              Copyright 2001, GALILEO TECHNOLOGY, LTD.
*
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL. NO RIGHTS ARE GRANTED
* HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT OF MARVELL OR ANY THIRD
* PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE DISCRETION TO REQUEST THAT THIS
* CODE BE IMMEDIATELY RETURNED TO MARVELL. THIS CODE IS PROVIDED "AS IS".
* MARVELL MAKES NO WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS
* ACCURACY, COMPLETENESS OR PERFORMANCE. MARVELL COMPRISES MARVELL TECHNOLOGY
* GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, MARVELL INTERNATIONAL LTD. (MIL),
* MARVELL TECHNOLOGY, INC. (MTI), MARVELL SEMICONDUCTOR, INC. (MSI), MARVELL
* ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K. (MJKK), GALILEO TECHNOLOGY LTD. (GTL)
* AND GALILEO TECHNOLOGY, INC. (GTI).
********************************************************************************
* mvTwsiDrvCtrl.h
*
* DESCRIPTION:
*       API implementation for TWSI facilities.
*
* DEPENDENCIES:
*
*       $Revision: 1 $
*******************************************************************************/

#include "mvTwsiDrvCtrl.h"
#include "mvTwsi.h"
#include "mvBoardEnvSpec.h"

static GT_U32 mvTwsiIfSpeed = 50000 /*TWSI_SPEED*/;
static GT_BOOL mvTwsiInitDone = GT_FALSE;

#define MV_ON_WRITE_SET_BIT1(data)  (data[3] |= 0x2)
#define MV_ON_READ_RESET_BIT1(data) (data[3] &= ~0x2)

#define TWSI_CHANNEL 0

/*******************************************************************************
* mvDirectTwsiBufferCfg
*
* DESCRIPTION:
*       Open twsi buffer on management board
*
* INPUTS:
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS mvDirectTwsiBufferCfg
(
    VOID
)
{
    MV_U8 data = 0;
    MV_TWSI_SLAVE pTwsiSlave;

    pTwsiSlave.slaveAddr.address = 0x27;
    pTwsiSlave.slaveAddr.type = ADDR7_BIT;
    pTwsiSlave.validOffset = MV_TRUE;
    pTwsiSlave.moreThen256 = MV_FALSE;
    pTwsiSlave.offset = 0x6;

    data = 0xff;
    if (MV_OK != mvTwsiWrite (TWSI_CHANNEL, &pTwsiSlave, &data, 1))
    {
        mvOsPrintf ("\nwrite fail to twsi 0x27 register 6\n");
        return MV_ERROR;
    }
    pTwsiSlave.offset = 0x7;

    data = 0xb0;
    if (MV_OK != mvTwsiWrite (TWSI_CHANNEL, &pTwsiSlave, &data, 1))
    {
        mvOsPrintf ("\nwrite fail to twsi 0x27 register 7\n");
        return MV_ERROR;
    }

    pTwsiSlave.offset = 0x3;

    data = 0x40;
    if (MV_OK != mvTwsiWrite (TWSI_CHANNEL, &pTwsiSlave, &data, 1))
    {
        mvOsPrintf ("\nwrite fail to twsi 0x27 register 3\n");
        return MV_ERROR;
    }

    return MV_OK;
}

/*******************************************************************************
* mvDirectTwsiWaitNotBusy
*
* DESCRIPTION:
*       Wait for TWSI interface not BUSY
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS mvDirectTwsiWaitNotBusy
(
    GT_VOID
)
{
    return GT_OK;  /* all read/write transactions should be completed */
}

/*******************************************************************************
* mvDirectTwsiMasterReadTrans
*
* DESCRIPTION:
*       do TWSI interface Transaction
*
* INPUTS:
*    devId - I2c slave ID
*    pData - Pointer to array of chars (address / data)
*    len   - pData array size (in chars).
*    stop  - Indicates if stop bit is needed.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS mvDirectTwsiMasterReadTrans
(
    IN GT_U8   devId,               /* I2c slave ID                              */
    IN GT_U8 * pData,               /* Pointer to array of chars (address / data)*/
    IN GT_U8   len,                 /* pData array size (in chars).              */
    IN GT_BOOL stop                 /* Indicates if stop bit is needed in the end  */
)
{
    MV_TWSI_SLAVE   twsiSlave;

    if (!mvTwsiInitDone)
    {
        mvDirectTwsiInitDriver ();
    }

    twsiSlave.slaveAddr.type = ADDR7_BIT;
    twsiSlave.slaveAddr.address = devId;
    twsiSlave.validOffset = MV_FALSE;
    twsiSlave.offset = 0;
    twsiSlave.moreThen256 = MV_FALSE;

    if (mvTwsiRead (TWSI_CHANNEL, &twsiSlave, pData, len) != MV_OK)
    {
        return GT_FAIL;
    }

    return GT_OK;
}

/*******************************************************************************
* mvDirectTwsiMasterWriteTrans
*
* DESCRIPTION:
*       do TWSI interface Transaction
*
* INPUTS:
*    devId - I2c slave ID
*    pData - Pointer to array of chars (address / data)
*    len   - pData array size (in chars).
*    stop  - Indicates if stop bit is needed.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS mvDirectTwsiMasterWriteTrans
(
    IN GT_U8   devId,               /* I2c slave ID                              */
    IN GT_U8 * pData,               /* Pointer to array of chars (address / data)*/
    IN GT_U8   len,                 /* pData array size (in chars).              */
    IN GT_BOOL stop                 /* Indicates if stop bit is needed in the end  */
)
{
    MV_TWSI_SLAVE   twsiSlave;

    if (!mvTwsiInitDone)
    {
        mvDirectTwsiInitDriver ();
    }
    twsiSlave.slaveAddr.type = ADDR7_BIT;
    twsiSlave.slaveAddr.address = devId;
    twsiSlave.validOffset = MV_FALSE;
    twsiSlave.offset = 0;
    twsiSlave.moreThen256 = MV_FALSE;

    if (mvTwsiWrite (TWSI_CHANNEL, &twsiSlave, pData, len) != MV_OK)
    {
        return GT_FAIL;
    }

    return GT_OK;
}

/*******************************************************************************
* mvTwsiIfSpeedSelect
*
* DESCRIPTION:
*       Init TWSI Inerface speed
*
* INPUTS:
*    speed - I2c slave ID
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*
*******************************************************************************/
void mvTwsiIfSpeedSelect(GT_U32 speed)
{
    mvTwsiIfSpeed = speed;
    return;
}

MV_U32 mvGetBoardMuxAddress()
{
    /*
     * for xCatA1/A2
     */

    return 0x70;
}

/*******************************************************************************
* mvDirectTwsiSetMuxChannel
*
* DESCRIPTION:
*       Set the TWSI mux channel
*
* INPUTS:
*    channel - the mux channel to set
*    enable  - enable the channel
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS mvDirectTwsiSetMuxChannel
(
    IN GT_U32  channel /* mux channel */,
    IN GT_BOOL enable
)
{
    MV_U8 data = 0;
    MV_TWSI_SLAVE pTwsiSlave;

    if (enable == MV_FALSE)
    {
        data = 0;
    }
    else
    {
        data = channel;
    }

    pTwsiSlave.slaveAddr.address = mvGetBoardMuxAddress ();
    pTwsiSlave.slaveAddr.type = ADDR7_BIT;
    pTwsiSlave.validOffset = MV_TRUE;
    pTwsiSlave.moreThen256 = MV_FALSE;
    pTwsiSlave.offset = 0;

    if (MV_OK != mvTwsiWrite (TWSI_CHANNEL, &pTwsiSlave, &data, 1))
    {
        mvOsPrintf ("\nwrite fail to twsi MUX register 0x%x\n", mvGetBoardMuxAddress ());
        return MV_ERROR;
    }

    return MV_OK;
}

/*******************************************************************************
* mvDirectTwsiInitDriver
*
* DESCRIPTION:
*       Init the TWSI interface
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS mvDirectTwsiInitDriver
(
    GT_VOID
)
{
    MV_TWSI_ADDR twsiAddr;

    if (mvTwsiInitDone)
    {
        return GT_OK;
    }
    twsiAddr.address = MV_BOARD_CTRL_I2C_ADDR;
    twsiAddr.type = ADDR7_BIT;

    mvTwsiInit (TWSI_CHANNEL, mvTwsiIfSpeed, mvBoardTclkGet (), &twsiAddr, 0);

    mvTwsiInitDone = GT_TRUE;
    return GT_OK;
}

/*******************************************************************************
* mvTwsiReadWithOffset
*
* DESCRIPTION:
*       do TWSI interface Transaction
*
* INPUTS:
*    devId - I2c slave ID
*    pData - Pointer to array of chars (address / data)
*    len   - pData array size (in chars).
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS mvTwsiReadWithOffset
(
    IN GT_U8  twsiAddr,             /* I2c slave address            */
    IN GT_U8  offset,               /* register address             */
    IN GT_U8 *pData,                /* Pointer to array of chars    */
    IN GT_U8  len                   /* data length                  */
)
{
    MV_TWSI_SLAVE   twsiSlave;

    if (!mvTwsiInitDone)
    {
        mvDirectTwsiInitDriver ();
    }
    twsiSlave.slaveAddr.type = ADDR7_BIT;
    twsiSlave.slaveAddr.address = twsiAddr;
    twsiSlave.validOffset = MV_TRUE;
    twsiSlave.offset = offset;
    twsiSlave.moreThen256 = MV_FALSE;

    if (mvTwsiRead (TWSI_CHANNEL, &twsiSlave, pData, len) != MV_OK)
    {
        return GT_FAIL;
    }

    return GT_OK;
}

/*******************************************************************************
* mvTwsiWriteWithOffset
*
* DESCRIPTION:
*       do TWSI interface write Transaction with offset
*
* INPUTS:
*    twsiAddr - I2c slave address
*    offset - register addrees for TWSI slave
*    pData - Pointer to array of data
*    len   - pData array size (in chars).
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on success
*       GT_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS mvTwsiWriteWithOffset
(
    IN GT_U8  twsiAddr,             /* I2c slave address            */
    IN GT_U8  offset,               /* register address             */
    IN GT_U8 *pData,                /* Pointer to array of chars    */
    IN GT_U8  len                   /* data length                  */
)
{
    MV_TWSI_SLAVE   twsiSlave;

    if (!mvTwsiInitDone)
    {
        mvDirectTwsiInitDriver ();
    }
    twsiSlave.slaveAddr.type = ADDR7_BIT;
    twsiSlave.slaveAddr.address = twsiAddr;
    twsiSlave.validOffset = MV_TRUE;
    twsiSlave.offset = offset;
    twsiSlave.moreThen256 = MV_FALSE;

    if (mvTwsiWrite (TWSI_CHANNEL, &twsiSlave, pData, len) != MV_OK)
    {
        return GT_FAIL;
    }

    return GT_OK;
}

/*******************************************************************************
* mvLongToByte
*
* DESCRIPTION:
*       Transforms unsigned long int type to 4 separate chars.
*
* INPUTS:
*       src - source unsigned long integer.
*
* OUTPUTS:
*       dst - Array of 4 chars
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL   - on failure
*
* COMMENTS:
*           MSB is copied to dst[0]!!
*
*******************************************************************************/
GT_STATUS mvLongToByte (IN GT_U32 src,
                        OUT GT_U8 dst[4])
{
    GT_U32 i;

    for (i = 4 ; i > 0 ; i--)
    {
        dst[i - 1] = (GT_U8) src & 0xFF;
        src >>= 8;
    }

    return GT_OK;
}

/*******************************************************************************
* mvByteToLong
*
* DESCRIPTION:
*       Transforms an array of 4 separate chars to unsigned long integer type
*
* INPUTS:
*       src - Source Array of 4 chars
*
* OUTPUTS:
*       dst - Unsigned long integer number
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL   - on failure
*
* COMMENTS:
*           MSB resides in src[0]!!
*
*******************************************************************************/
GT_STATUS mvByteToLong (IN GT_U8    src[4],
                        OUT GT_U32 *dst)
{
    GT_U32  i;
    GT_U32  tempU32 = 0x00000000;

    for (i = 4 ; i > 0 ; i--)
    {
        tempU32 += ((GT_U32)src[i - 1]) << (8 * (4 - i));
    }

    *dst = tempU32;
    return GT_OK;
}

/*******************************************************************************
* mvConcatByteArray
*
* DESCRIPTION:
*       Concatenate 2 Arrays of Chars to one Array of chars
*
* INPUTS:
*       src0 - Source Array of 4 chars long
*       src1 - Source Array of 4 chars long
*
* OUTPUTS:
*       dst - Concatenated Array of 8 chars long {src1,src0}
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL   - on failure
*
* COMMENTS:
*           None
*
*******************************************************************************/
GT_STATUS mvConcatByteArray (IN GT_U8  src0[4],
                             IN GT_U8  src1[4],
                             OUT GT_U8 dst[8])
{
    GT_U32 i = 0, j; /*Source and Dest Counters*/

    for (j = 0; j < 8; j++)
    {
        if (j < 4)
        {
            dst[j] = src0[i++];
        }
        else if (j == 4)
        {
            dst[j] = src1[0];
            i = 1;
        }
        else
        {
            dst[j] = src1[i++];
        }
    }
    return GT_OK;
}


