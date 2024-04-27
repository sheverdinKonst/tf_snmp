/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* gmOsIo.h
*
* DESCRIPTION:
*       Operating System wrapper. Input/output facility.
*
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/

#ifndef __gmOsIoh
#define __gmOsIoh

#ifdef __cplusplus
extern "C" {
#endif

/************* Includes *******************************************************/

#include <os/simTypes.h>
/*
#include <extServices/os/gtOs/gtGenTypes.h>
#include <stdio.h>
*/

/************* Definitions ****************************************************/

/*******************************************************************************
* GM_OS_IO_PRINTF_FUNC
*
* DESCRIPTION:
*       Write a formatted string to the standard output stream.
*
* INPUTS:
*       format  - format string to write
*       ... - parameters of the 'format'
* OUTPUTS:
*       None
*
* RETURNS:
*       The number of characters written, or a negative value if
*       an output error occurs.
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef int (*GM_OS_IO_PRINTF_FUNC)(
    IN  const char* format,
    IN  ...
);

/*******************************************************************************
* GM_OS_IO_SPRINTF_FUNC
*
* DESCRIPTION:
*       Write a formatted string to a buffer.
*
* INPUTS:
*       buffer  - buffer to write to
*       format  - format string
*       ... - parameters of the 'format'
*
* OUTPUTS:
*       None
*
* RETURNS:
*       The number of characters copied to buffer, not including
*       the NULL terminator.
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef int (*GM_OS_IO_SPRINTF_FUNC)(
    IN  char * buffer,
    IN  const char* format,
    IN  ...
);

/*******************************************************************************
* GM_OS_IO_PRINT_SYNC_FUNC
*
* DESCRIPTION:
*       Write a formatted string to the standard output stream.
*
* INPUTS:
*       format  - format string to write
*       ... - parameters of the 'format'
*
* OUTPUTS:
*       None
*
* RETURNS:
*       The number of characters written, or a negative value if
*       an output error occurs.
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef int (*GM_OS_IO_PRINT_SYNC_FUNC)(
    IN  const char* format,
    IN  ...
);

/*******************************************************************************
* GM_OS_IO_GETS_FUNC
*
* DESCRIPTION:
*       Reads characters from the standard input stream into the array
*       'bufferPtr' until end-of-file is encountered or a new-line is read.
*       Any new-line character is discarded, and a null character is written
*       immediately after the last character read into the array.
*
* INPUTS:
*       bufferPtr  - (pointer to) buffer to write to
*
* OUTPUTS:
*       bufferPtr  - (pointer to)buffer with read data
*
* RETURNS:
*       A pointer to 'bufferPtr', or a null pointer if end-of-file is
*       encountered and no characters have been read, or there is a read error.
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef char * (*GM_OS_IO_GETS_FUNC)(
    INOUT char * bufferPtr
);


/* GM_OS_IO_BIND_STC -
    structure that hold the "os I/O" functions needed be bound to gm.

*/
typedef struct{
    GM_OS_IO_PRINTF_FUNC         osIoPrintfFunc;
    GM_OS_IO_SPRINTF_FUNC        osIoSprintfFunc;
    GM_OS_IO_PRINT_SYNC_FUNC     osIoPrintSynchFunc;
    GM_OS_IO_GETS_FUNC           osIoGetsFunc;
}GM_OS_IO_BIND_STC;


#ifdef __cplusplus
}
#endif

#endif  /* __gmOsIoh */


