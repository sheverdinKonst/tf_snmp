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
* simCrc.h
*
* DESCRIPTION:
*       Includes definitions for CRC calculation function.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 5 $
*
*******************************************************************************/
#ifndef __simCrch
#define __simCrch

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/********* Include files ******************************************************/
#include <os/simTypes.h>

/*******************************************************************************
* simCalcCrc32
*
* DESCRIPTION:
*       Calculate CRC 32 bit for input data.
*
* INPUTS:
*       crc     - CRC start value.
*       buffer  - pointer to the buffer.
*       byteNum - number of bytes in the buffer.
*
* OUTPUTS:
*       none
*
* RETURNS:
*       32 bit CRC.
*
* COMMENTS:
*   For calculation a new CRC the value of CRC should be 0xffffffff.
*
*******************************************************************************/
GT_U32 simCalcCrc32
(
    IN GT_U32 crc,
    IN GT_U8  *buffer,
    IN GT_U32 byteNum
);

/*******************************************************************************
* simCalcCrc16
*
* DESCRIPTION:
*       Calculate CRC 16 bit for input data.
*
* INPUTS:
*       crc     - CRC start value.
*       buffer  - pointer to the buffer.
*       byteNum - number of bytes in the buffer.
*
* OUTPUTS:
*       none
*
* RETURNS:
*       32 bit CRC.
*
* COMMENTS:
*   For calculation a new CRC the value of CRC should be 0xffff.
*
*******************************************************************************/
GT_U16 simCalcCrc16
(
    IN GT_U16 crc,
    IN GT_U8  *buffer,
    IN GT_U32 byteNum
);

/*******************************************************************************
* simCalcCrc8
*
* DESCRIPTION:
*       Calculate CRC 8 bit for input data.
*
* INPUTS:
*       crc     - CRC start value.
*       buffer  - pointer to the buffer.
*       byteNum - number of bytes in the buffer.
*
* OUTPUTS:
*       none
*
* RETURNS:
*       32 bit CRC.
*
* COMMENTS:
*   For calculation a new CRC the value of CRC should be 0xff.
*
*******************************************************************************/
GT_U8 simCalcCrc8
(
    IN GT_U8  crc,
    IN GT_U8  *buffer,
    IN GT_U32 byteNum
);


/*******************************************************************************
* simCalcHashFor70BytesCrc6
*
* DESCRIPTION:
*       Calculate CRC 6 bit for 70 bytes input data.
*
* INPUTS:
*       crc     - CRC start value.
*       bufPtr  - pointer to the bufPtr (of 70 bytes)
*       byteNum - number of bytes in the bufPtr -- MUST be 70 bytes
*
* OUTPUTS:
*       none
*
* RETURNS:
*       6 bit CRC.
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 simCalcHashFor70BytesCrc6
(
    IN GT_U32  crc,
    IN GT_U8  *bufPtr,
    IN GT_U32 byteNum
);

/*******************************************************************************
* simCalcHashFor70BytesCrc16
*
* DESCRIPTION:
*       Calculate CRC 16 bit for 70 bytes input data.
*
* INPUTS:
*       crc     - CRC start value.
*       bufPtr  - pointer to the bufPtr (of 70 bytes)
*       byteNum - number of bytes in the bufPtr -- MUST be 70 bytes
*
* OUTPUTS:
*       none
*
* RETURNS:
*       16 bit CRC.
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 simCalcHashFor70BytesCrc16
(
    IN GT_U32  crc,
    IN GT_U8  *bufPtr,
    IN GT_U32 byteNum
);

/*******************************************************************************
* simCalcHashFor70BytesCrc32
*
* DESCRIPTION:
*       Calculate CRC 32 bit for 70 bytes input data.
*       CRC-32 hash value calculation is performed using the CRC-32-IEEE 802.3 polynomial
* INPUTS:
*       crc     - CRC start value.
*       bufPtr  - pointer to the bufPtr (of 70 bytes)
*       byteNum - number of bytes in the bufPtr -- MUST be 70 bytes
*
* OUTPUTS:
*       none
*
* RETURNS:
*       32 bit CRC.
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 simCalcHashFor70BytesCrc32
(
    IN GT_U32  crc,
    IN GT_U8  *bufPtr,
    IN GT_U32 byteNum
);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __simCrch */

