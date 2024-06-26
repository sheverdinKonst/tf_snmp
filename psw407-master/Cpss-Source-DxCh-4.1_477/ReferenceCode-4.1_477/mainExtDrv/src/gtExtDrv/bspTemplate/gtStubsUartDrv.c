/*******************************************************************************
*                   Copyright 2002, GALILEO TECHNOLOGY, LTD.                   *
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
* (MJKK), GALILEO TECHNOLOGY LTD. (GTL) AND GALILEO TECHNOLOGY, INC. (GTI).    *
********************************************************************************
* gtStubsUartDrv.c - UART interface driver.
*
* DESCRIPTION:
*       Enable the use of specific UART operations.
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/

#include <prestera/os/gtTypes.h>
#include <gtExtDrv/drivers/gtUartDrv.h>
#include <gtOs/gtOsIo.h>

/*******************************************************************************
* extDrvUartInit
*
* DESCRIPTION:
*       Setting the UART environment to work with PSS polling mode.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS extDrvUartInit ( void )
{

  return (GT_OK);
}

/*******************************************************************************
* extDrvUartTxBuffer
*
* DESCRIPTION:
*       Sending ASCII buffer to the UART port in pooling mode
*
* INPUTS:
*       buffer      - pointer to buffer containing the data to be send to the UART port.
*       buffSize    - The size of the buffer to be sent to the UART port.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS extDrvUartTxBuffer
(
  char * buffer,
  GT_U32  buffSize
)
{
  return (GT_OK);
}

