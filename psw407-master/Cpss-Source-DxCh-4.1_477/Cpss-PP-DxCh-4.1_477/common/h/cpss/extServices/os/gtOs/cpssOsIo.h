/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* cpssOsIo.h
*
* DESCRIPTION:
*       Operating System wrapper. Input/output facility.
*
*
* FILE REVISION NUMBER:
*       $Revision: 3 $
*******************************************************************************/

#ifndef __cpssOsIoh
#define __cpssOsIoh

#ifdef __cplusplus
extern "C" {
#endif

/************* Includes *******************************************************/

#include <cpss/extServices/os/gtOs/gtGenTypes.h>
#include <stdio.h>
#include <stdarg.h>

/************* Definitions ****************************************************/

typedef int (*CPSS_OS_BIND_STDOUT_FUNC_PTR)(void* userPtr, const char* buffer, int length);

/*******************************************************************************
* CPSS_OS_IO_BIND_STDOUT_FUNC
*
* DESCRIPTION:
*       Bind Stdout to handler
*
* INPUTS:
*       writeFunction - function to call for output
*       userPtr       - first parameter to pass to write function
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*CPSS_OS_IO_BIND_STDOUT_FUNC)(
    IN CPSS_OS_BIND_STDOUT_FUNC_PTR writeFunction,
    IN void* userPtr
);

/*******************************************************************************
* CPSS_OS_IO_PRINTF_FUNC
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
typedef int (*CPSS_OS_IO_PRINTF_FUNC)(
    IN  const char* format,
    IN  ...
);

/*******************************************************************************
* osVprintf
*
* DESCRIPTION:
*       Write a formatted string to the standard output stream.
*
* INPUTS:
*       format  - format string to write
*       args    - va_list
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
*******************************************************************************/
typedef int (*CPSS_OS_IO_VPRINTF_FUNC)(
    IN  const char* format,
    IN  va_list args
);

/*******************************************************************************
* CPSS_OS_IO_SPRINTF_FUNC
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
typedef int (*CPSS_OS_IO_SPRINTF_FUNC)(
    IN  char * buffer,
    IN  const char* format,
    IN  ...
);

/*******************************************************************************
* osVsprintf
*
* DESCRIPTION:
*       Write a formatted string to a buffer.
*
* INPUTS:
*       buffer  - buffer to write to
*       format  - format string
*       args    - va_list
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
typedef int (*CPSS_OS_IO_VSPRINTF_FUNC)(
    IN  char * buffer,
    IN  const char* format,
    IN  va_list args
);

/*******************************************************************************
* CPSS_OS_IO_PRINT_SYNC_FUNC
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
typedef int (*CPSS_OS_IO_PRINT_SYNC_FUNC)(
    IN  const char* format,
    IN  ...
);

/*******************************************************************************
* CPSS_OS_IO_GETS_FUNC
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
typedef char * (*CPSS_OS_IO_GETS_FUNC)(
    INOUT char * bufferPtr
);

/*******************************************************************************
* CPSS_OS_IO_FOPEN_FUNC
*
* DESCRIPTION:
*       Opens the file whose name is specified in the parameter filename and associates it with a stream
*       that can be identified in future operations by the FILE pointer returned.
*       
*
* INPUTS:
*       fileNamePtr  - (pointer to) string containing the name of the file to be opened
*       modePtr - (pointer to) string containing a file access mode
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Pointer to FILE object that can be used to identify the stream on future operations or null if opening fails
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef FILE * (*CPSS_OS_IO_FOPEN_FUNC)(
    IN const char * fileNamePtr,
    IN const char * modePtr
);

/*******************************************************************************
* CPSS_OS_IO_FCLOSE_FUNC
*
* DESCRIPTION:
*       Closes the file associated with the stream and disassociates it.
*
* INPUTS:
*       streamPtr  - (pointer to) FILE object that specifies the stream.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       If the stream is successfully closed, a zero value is returned.
*       On failure, EOF is returned.
*
* COMMENTS:
*       Nome
*
*******************************************************************************/
typedef int (*CPSS_OS_IO_FCLOSE_FUNC)(
    IN FILE * streamPtr
);

/*******************************************************************************
* CPSS_OS_IO_REWIND_FUNC
*
* DESCRIPTION:
*       Sets the position indicator associated with stream to the beginning of the file.
*
* INPUTS:
*       streamPtr  - (pointer to) FILE object that specifies the stream.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*       Nome
*
*******************************************************************************/
typedef void (*CPSS_OS_IO_REWIND_FUNC)(
    IN FILE * streamPtr
);

/*******************************************************************************
* CPSS_OS_IO_FPRINTF_FUNC
*
* DESCRIPTION:
*       Write a formatted string to the stream.
*
* INPUTS:
*       streamPtr  - (pointer to) FILE object that specifies the stream.
*       format  - format string to write
*       ... - parameters of the 'format'
*
* OUTPUTS:
*       None
*
* RETURNS:
*       On success, the total number of characters written is returned.
*       If a writing error occurs, the error indicator (ferror) is set and a negative number is returned.
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef int (*CPSS_OS_IO_FPRINTF_FUNC)(
    IN FILE * streamPtr,
    IN const char* format,
    IN  ...
);

/*******************************************************************************
* CPSS_OS_IO_FGETS_FUNC
*
* DESCRIPTION:
*       Reads characters from stream and stores them as a string into buffer until (num-1) characters have been read
*       or either a newline or the end-of-file is reached, whichever happens first.
*
* INPUTS:
*       bufferPtr  - (pointer to) buffer to write to
*       num - maximum number of characters to be copied into str (including the terminating null-character)
*       streamPtr - (pointer to) FILE object that specifies the stream.
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
typedef char * (*CPSS_OS_IO_FGETS_FUNC)(
    INOUT char * bufferPtr,
    IN int num,
    IN FILE * streamPtr
);


/* CPSS_OS_IO_BIND_STC -
    structure that hold the "os I/O" functions needed be bound to cpss.

*/
typedef struct{
    CPSS_OS_IO_BIND_STDOUT_FUNC    osIoBindStdOutFunc;
    CPSS_OS_IO_PRINTF_FUNC         osIoPrintfFunc;
    CPSS_OS_IO_VPRINTF_FUNC        osIoVprintfFunc;
    CPSS_OS_IO_SPRINTF_FUNC        osIoSprintfFunc;
    CPSS_OS_IO_VSPRINTF_FUNC       osIoVsprintfFunc;
    CPSS_OS_IO_PRINT_SYNC_FUNC     osIoPrintSynchFunc;
    CPSS_OS_IO_GETS_FUNC           osIoGetsFunc;
    CPSS_OS_IO_FOPEN_FUNC          osIoFopenFunc; 
    CPSS_OS_IO_FCLOSE_FUNC         osIoFcloseFunc;
    CPSS_OS_IO_REWIND_FUNC         osIoRewindFunc;
    CPSS_OS_IO_FPRINTF_FUNC        osIoFprintfFunc;
    CPSS_OS_IO_FGETS_FUNC          osIoFgets;
}CPSS_OS_IO_BIND_STC;


#ifdef __cplusplus
}
#endif

#endif  /* __cpssOsIoh */


