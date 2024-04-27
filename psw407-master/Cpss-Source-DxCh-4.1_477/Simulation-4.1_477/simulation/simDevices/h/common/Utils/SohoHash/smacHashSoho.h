/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* smacHashSoho.h
*
* DESCRIPTION:
*       Hash calculate for MAC address table implementation for Soho.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*
*******************************************************************************/
#ifndef __smacHashSohoh
#define __smacHashSohoh

#include <os/simTypes.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


GT_STATUS sohoMacHashCalc
(
    IN  GT_ETHERADDR    *addr,
    OUT GT_U32          *hash
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __smacHashSohoh */


