/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sfdbSoho.h
*
* DESCRIPTION:
*       This is a external API definition for SFDB module of SKernel Soho.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 3 $
*
*******************************************************************************/
#ifndef __sfdbsoho
#define __sfdbsoho

#include <asicSimulation/SKernel/suserframes/snetSoho.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
*   sfdbSohoMsgProcess
*
* DESCRIPTION:
*       Process FDB update message.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       fdbMsgPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
GT_VOID sfdbSohoMsgProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8                 * fdbMsgPtr
);

/*******************************************************************************
*   sfdbSohoAtuEntryGet
*
* DESCRIPTION:
*       Get ATU database entry in the SRAM 
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       macAddrPtr      - pointer to 48-bit MAC address
*       dbNum           - Vlan database number
*
* OUTPUTS:
*       atuEntryPtr     - ATU database entry pointer
*       
* RETURNS:
*       GT_OK           - ATU database entry found
*       GT_NOT_FOUND    - ATU database entry not found  
*
* COMMENTS:
*
*
*******************************************************************************/
GT_STATUS sfdbSohoAtuEntryAddress
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_U8 * macAddrPtr,
    IN GT_U32 dbNum,
    OUT GT_U32 * address
);

/*******************************************************************************
*   sfdbSohoFreeBinGet
*
* DESCRIPTION:
*       Check if all bins in the bucket are static
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       macAddrPtr      - pointer to 48-bit MAC address
*       dbNum           - vlan database number
*       regAddrPtr      - pointer to free bin address
* OUTPUTS:
*       
* RETURNS:
*       GT_OK           - all entries static
*       GT_NOT_FOUND    - not all entries static
*
* COMMENTS:
*
*
*******************************************************************************/
GT_STATUS sfdbSohoFreeBinGet
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_U8 * macAddrPtr,
    IN GT_U32 dbNum,
    OUT GT_U32 * regAddrPtr
);

/*******************************************************************************
*   sfdbSohoMacTableAging
*
* DESCRIPTION:
*       Age out MAC table entries.
*
* INPUTS:
*       deviceObj   - pointer to device object.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/

void sfdbSohoMacTableAging
(
    IN SKERNEL_DEVICE_OBJECT * deviceObjPtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sfdbsoho */


