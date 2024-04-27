/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simTypes.h
*
* DESCRIPTION:    Common types definitions
*
* FILE REVISION NUMBER:
*       $Revision: 15 $
*******************************************************************************/

#ifndef __simTypesh
#define __simTypesh

#include <os/simEnvDepTypes.h>

#ifndef GT_INLINE
#ifdef _VXWORKS
#define GT_INLINE __inline__
#else
#define  GT_INLINE
#endif
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#undef IN
#define IN
#undef OUT
#define OUT
#undef INOUT
#define INOUT

#ifndef __gtEnvDeph
typedef struct
{
    GT_U8       arEther[6];
}GT_ETHERADDR;

typedef void          (*GT_VOIDFUNCPTR) (); /* ptr to function returning void */
typedef unsigned int  (*GT_INTFUNCPTR)  (); /* ptr to function returning int  */

/* support including by cpss/pss */

/*
 * Typedef: enum GT_COMP_RES
 *
 * Description: Values to be returned by compare function
 *
 */
typedef enum
{
    GT_EQUAL   = 0,
    GT_GREATER = 1,
    GT_SMALLER = 2
}GT_COMP_RES;


typedef unsigned int GT_STATUS;
#endif /*__gtEnvDeph*/

/***** generic return codes **********************************/
#define GT_ERROR           (-1)
#define GT_OK              (0x00)   /* Operation succeeded */
#define GT_FAIL            (0x01)   /* Operation failed    */

#define GT_BAD_VALUE       (0x02)   /* Illegal value        */
#define GT_OUT_OF_RANGE    (0x03)   /* Value is out of range*/
#define GT_BAD_PARAM       (0x04)   /* Illegal parameter in function called  */
#define GT_BAD_PTR         (0x05)   /* Illegal pointer value                 */
#define GT_BAD_SIZE        (0x06)   /* Illegal size                          */
#define GT_BAD_STATE       (0x07)   /* Illegal state of state machine        */
#define GT_SET_ERROR       (0x08)   /* Set operation failed                  */
#define GT_GET_ERROR       (0x09)   /* Get operation failed                  */
#define GT_CREATE_ERROR    (0x0A)   /* Fail while creating an item           */
#define GT_NOT_FOUND       (0x0B)   /* Item not found                        */
#define GT_NO_MORE         (0x0C)   /* No more items found                   */
#define GT_NO_SUCH         (0x0D)   /* No such item                          */
#define GT_TIMEOUT         (0x0E)   /* Time Out                              */
#define GT_NO_CHANGE       (0x0F)   /* The parameter is already in this value*/
#define GT_NOT_SUPPORTED   (0x10)   /* This request is not support           */
#define GT_NOT_IMPLEMENTED (0x11)   /* This request is not implemented       */
#define GT_NOT_INITIALIZED (0x12)   /* The item is not initialized           */
#define GT_NO_RESOURCE     (0x13)   /* Resource not available (memory ...)   */
#define GT_FULL            (0x14)   /* Item is full (Queue or table etc...)  */
#define GT_EMPTY           (0x15)   /* Item is empty (Queue or table etc...) */
#define GT_INIT_ERROR      (0x16)   /* Error occurred while INIT process      */
#define GT_NOT_READY       (0x1A)   /* The other side is not ready yet       */
#define GT_ALREADY_EXIST   (0x1B)   /* Tried to create existing item         */
#define GT_OUT_OF_CPU_MEM  (0x1C)   /* Cpu memory allocation failed.         */
#define GT_ABORTED         (0x1D)   /* Operation has been aborted.           */

#ifndef BIT_MASK

/* Return the mask including "numOfBits" bits. for 0..31 bits   */
#define BIT_MASK_0_31(numOfBits) (~(0xFFFFFFFF << (numOfBits)))

/* the macro of BIT_MASK() in VC will act in wrong way for 32 bits , and will
   result 0 instead of 0xffffffff

   so macro BIT_MASK_IMPR - is improvement of BIT_MASK to support 32 bits
*/
#define BIT_MASK(numOfBits)    ((numOfBits) == 32 ? 0xFFFFFFFF : BIT_MASK_0_31(numOfBits))

/* Calculate the field mask for a given offset & length */
/* e.g.: BIT_MASK(8,2) = 0xFFFFFCFF                     */
#define FIELD_MASK_NOT(offset,len)                      \
        (~(BIT_MASK((len)) << (offset)))

/* Calculate the field mask for a given offset & length */
/* e.g.: BIT_MASK(8,2) = 0x00000300                     */
#ifndef FIELD_MASK
#define FIELD_MASK(offset,len)                      \
        ( (BIT_MASK((len)) << (offset)) )
#endif /*FIELD_MASK*/

#ifndef U32_GET_FIELD
    /* Returns the info located at the specified offset & length in data.   */
    #define U32_GET_FIELD(data,offset,length)           \
            (((data) >> (offset)) & ((1u << (length)) - 1))
#endif /*U32_GET_FIELD*/

/* Sets the field located at the specified offset & length in data.     */
#define U32_SET_FIELD(data,offset,length,val)           \
   (data) = (((data) & FIELD_MASK_NOT((offset),(length))) | ((val) <<(offset)))
#endif /* not def BIT_MASK*/

#ifdef MIN
#undef  MIN
#endif  /* MIN */
#define MIN(x,y) (((x) < (y)) ? (x) : (y))

#ifdef MAX
#undef  MAX
#endif  /* MAX */
#define MAX(x,y) (((x) > (y)) ? (x) : (y))

#endif   /* __simTypesh */



