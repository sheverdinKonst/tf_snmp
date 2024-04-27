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
* sMath.c
*
* DESCRIPTION:
*
*   Mathematical operations on 64 bit values
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*       $Revision: 3 $
*******************************************************************************/

#include <os/simEnvDepTypes.h>
#include <common/Utils/Math/sMath.h>
/************* Defines ***********************************************/

/************ Public Functions ************************************************/

/*******************************************************************************
* prvSimMathAdd64
*
* DESCRIPTION:
*       Summarize two 64 bits values.
*
* INPUTS:
*       x - first value for sum.
*       y - second value for sum
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Calculated sum.
*
* COMMENTS:
*       This function does not care 65 bit overflow.
*
*******************************************************************************/
GT_U64  prvSimMathAdd64
(
    IN  GT_U64 x,
    IN  GT_U64 y
)
{
    GT_U64  z;
    GT_U32  maxVal;

    maxVal = y.l[0];
    if (x.l[0] > y.l[0])
        maxVal = x.l[0];

    z.l[0] = x.l[0] + y.l[0];           /* low order word sum  */
    z.l[1] = x.l[1] + y.l[1];           /* high order word sum */
    z.l[1] += (z.l[0] < maxVal) ? 1:0;  /* low-word overflow */

    return z;
}

/*******************************************************************************
* prvSimMathSub64
*
* DESCRIPTION:
*       Subtract two 64 bits values.
*
* INPUTS:
*       x - first value for difference.
*       y - second value for difference
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Calculated difference x - y.
*
* COMMENTS:
*
*******************************************************************************/
GT_U64  prvSimMathSub64
(
    IN  GT_U64 x,
    IN  GT_U64 y
)
{
    GT_U64  z;

    z.l[0] = x.l[0] - y.l[0];            /* low order word difference  */
    z.l[1] = x.l[1] - y.l[1];            /* high order word difference */
    z.l[1] -= (x.l[0] < y.l[0]) ? 1 : 0; /* low-word borrow            */

    return z;
}
