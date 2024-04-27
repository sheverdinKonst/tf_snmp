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
* simEnvDepTypes.h
*
* DESCRIPTION:    Hardware environment depended types definition
*
* DEPENDENCIES:   Operating System.
*
* FILE REVISION NUMBER:
*       $Revision: 12 $
*******************************************************************************/

#ifdef __gtEnvDeph
/* support including by cpss/pss */
    #define __simEnvDepTypesh
#endif /*__gtEnvDeph*/

#ifdef _VISUALC
/*
    for some reason some of the 'Disable warning messages' must come at the end
    of this file (probably the H files of VC6) open those warnings.

    So see for another section of disable warnings at end of file ....
*/

    /* Disable warning messages */
    #pragma warning( disable : 4054 )
    #pragma warning( disable : 4055 )
    #pragma warning( disable : 4057 )
    #pragma warning( disable : 4100 )
    #pragma warning( disable : 4127 )
    #pragma warning( disable : 4152 )
    /*#pragma warning( disable : 4214 ) --> this one is at the end of file ...*/
    #pragma warning( disable : 4244 )
    /*#pragma warning( disable : 4245 )*/
    #pragma warning( disable : 4366 )

/* define macro to be used for preprocessor warning
 * Usage:
 *    #pragma message (__LOC_WARN__ "the warning message")
 */
#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC_WARN__ __FILE__ "("__STR1__(__LINE__)") : warning: "

    /* WA to avoid next warning :
       due to include to : #include <windows.h>
        c:\program files\microsoft visual studio\vc98\include\rpcasync.h(45) :
        warning C4115: '_RPC_ASYNC_STATE' : named type definition in parentheses
    */
    struct _RPC_ASYNC_STATE;
#endif /* _VISUALC */

#ifndef __simImplGtChar
#define __simImplGtChar

#ifndef IMPL_GT_CHAR
    typedef char  GT_CHAR, *GT_CHAR_PTR;
#endif /* IMPL_GT_CHAR */

#endif

#ifndef __simEnvDepTypesh
#define __simEnvDepTypesh

#if !((OS_TARGET) == FreeBSD && (OS_TARGET_RELEASE) == 4)
#include <stdlib.h>
#endif

#if (!defined(OS_TARGET) || OS_TARGET != FreeBSD) && (! defined  VXWORKS)
#include <malloc.h>
#endif

#include <string.h>
#include <stdio.h>




typedef void *GT_VOID_PTR;
#define GT_VOID void

#ifndef UNIX
typedef char  GT_8,   *GT_8_PTR;
typedef short GT_16,  *GT_16_PTR;
typedef unsigned char  GT_U8,   *GT_U8_PTR;
typedef unsigned short GT_U16,  *GT_U16_PTR;

#define VAR_32BIT   long

#if defined(CPU) && defined(SIMSPARCSOLARIS)
    #if (CPU==SIMSPARCSOLARIS)
        #undef VAR_32BIT
        #define VAR_32BIT   int
    #endif
#endif

typedef VAR_32BIT   GT_32,  *GT_32_PTR;
typedef unsigned VAR_32BIT   GT_U32,  *GT_U32_PTR;
typedef unsigned VAR_32BIT   GT_BIT,  *GT_BIT_PTR;

#ifndef IS_64BIT_OS
typedef VAR_32BIT           GT_INTPTR;
typedef unsigned VAR_32BIT  GT_UINTPTR;
typedef VAR_32BIT           GT_PTRDIFF;
typedef unsigned VAR_32BIT  GT_SIZE_T;
#else /* IS_64BIT_OS */
#define VAR_64BIT           long long
typedef VAR_64BIT           GT_INTPTR;
typedef unsigned VAR_64BIT  GT_UINTPTR;
typedef VAR_64BIT           GT_PTRDIFF;
typedef unsigned VAR_64BIT  GT_SIZE_T;
#endif /* IS_64BIT_OS */

#else /* UNIX */
/* use system defined types */
#include <inttypes.h>
#include <stddef.h>
typedef int8_t      GT_8,   *GT_8_PTR;
typedef uint8_t     GT_U8,  *GT_U8_PTR;
typedef int16_t     GT_16,  *GT_16_PTR;
typedef uint16_t    GT_U16, *GT_U16_PTR;
typedef int32_t     GT_32,  *GT_32_PTR;
typedef uint32_t    GT_U32, *GT_U32_PTR;
typedef uint32_t    GT_BIT, *GT_BIT_PTR;

typedef intptr_t    GT_INTPTR;
typedef uintptr_t   GT_UINTPTR;
typedef ptrdiff_t   GT_PTRDIFF;
typedef size_t      GT_SIZE_T;
#endif

typedef void   *GT_PTR;

typedef union
{
    GT_U8   c[8];
    GT_U16  s[4];
    GT_U32  l[2];
}GT_U64;

/*
 * typedef: enum GT_BOOL
 *
 * Description: Enumeration of boolean.
 *
 * Enumerations:
 *    GT_FALSE - false.
 *    GT_TRUE  - true.
 */
typedef enum
{
    GT_FALSE = 0,
    GT_TRUE  = 1
} GT_BOOL;


#if defined(_linux) || defined(__FreeBSD__)
extern char* strlwr (char*);
#endif

#endif   /* __simEnvDepTypesh */

#ifndef __simEnvDepTypesh_part2
#define __simEnvDepTypesh_part2

typedef GT_U32  UINT_32;
typedef GT_U16  UINT_16;
typedef GT_U8   UINT_8;

typedef UINT_8 BOOLEAN;

#if (defined _WIN32)
    #define WINDOWS_COMPATIBLE
#endif

#ifdef WINDOWS_COMPATIBLE
    #include <windows.h>
#else /*!WINDOWS_COMPATIBLE*/
    /* extra "windows" types */
    typedef GT_U32  DWORD;
    typedef void*   HANDLE;
    typedef unsigned short WCHAR;
    typedef WCHAR *LPWSTR;
    typedef LPWSTR LPTSTR;
    typedef GT_U8  BOOL;
#endif /*!WINDOWS_COMPATIBLE*/

/*! Standard type constant definitions:   */
#ifndef MAX_UINT_8
    #define MAX_UINT_8           ((UINT_8) 0xFF)
#endif /*MAX_UINT_8*/
#define MAX_UINT_16          ((UINT_16)0xFFFF)
#define MAX_UINT_32          ((UINT_32)0xFFFFFFFF)

#define MAX_INT_8           ((INT_8) 0x7F)
#define MAX_INT_16          ((INT_16)0x7FFF)
#define MAX_INT_32          ((INT_32)0x7FFFFFFF)

#define MIN_INT_8           ((INT_8) 0x80)
#define MIN_INT_16          ((INT_16)0x8000)
#define MIN_INT_32          ((INT_32)0x80000000)
/*
#ifdef UNIX_SIM
    #define PRAGMA_NOALIGN  #pragma pack(1)
    #define PRAGMA_ALIGN    #pragma pack(4)
#else */
#ifdef I960
    #define PRAGMA_NOALIGN  #pragma noalign
    #define PRAGMA_ALIGN    #pragma align
#else
    #define PRAGMA_NOALIGN
    #define PRAGMA_ALIGN
#endif
/*#endif*/

/*
for VC :
instead of PRAGMA_NOALIGN
use
#pragma pack(1)
instead of PRAGMA_ALIGN
use
#pragma pack()
*/


#endif   /* __simEnvDepTypesh_part2 */


#ifdef _VISUALC
    /* Disable warning messages */
    #pragma warning( disable : 4214 )
#endif /* _VISUALC */

