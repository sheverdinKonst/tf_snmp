/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvCpssDxChPortDynamicPAPortSpeedDB.c
*
* DESCRIPTION:
*       bobcat2 and higher dynamic (algorithmic) pizza arbiter Port Speed DB
*
* FILE REVISION NUMBER:
*       $Revision: 3 $
*******************************************************************************/
#ifndef __PRV_CPSS_DXCH_PORT_DYNAMIC_PA_PORT_SPEED_DB_H
#define __PRV_CPSS_DXCH_PORT_DYNAMIC_PA_PORT_SPEED_DB_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include <cpss/generic/cpssTypes.h>

extern GT_STATUS prvCpssDxChPortDynamicPAPortSpeedDBInit
(
    IN    GT_U8    devNum
);

extern GT_STATUS prvCpssDxChPortDynamicPAPortSpeedDBSet
(
    IN    GT_U8                       devNum,
    IN    GT_PHYSICAL_PORT_NUM        portNum,
    IN    GT_U32                      speedInMBit
);

extern GT_STATUS prvCpssDxChPortDynamicPAPortSpeedDBGet
(
    IN    GT_U8                      devNum,
    IN    GT_PHYSICAL_PORT_NUM       portNum,
    OUT   GT_BOOL                   *isInitilizedPtr,
    OUT   GT_U32                    *speedInMBitPtr
);

extern GT_STATUS prvCpssDxChPortDynamicPAIlknChannelSpeedDBSet
(
    IN    GT_U8                       devNum,
    IN    GT_PHYSICAL_PORT_NUM        portNum,
    IN    GT_U32                      speedInMBit
);

extern GT_STATUS prvCpssDxChPortDynamicPAIlknChannelSpeedDBGet
(
    IN    GT_U8                      devNum,
    IN    GT_PHYSICAL_PORT_NUM       portNum,
    OUT   GT_BOOL                   *isInitilizedPtr,
    OUT   GT_U32                    *speedInMBitPtr
);


extern GT_STATUS prvCpssDxChPortDynamicPAIlknSpeedResolutionSet
(
    IN    GT_U8                       devNum,
    IN    GT_U32                      speedResulutionMBps,
    IN    GT_U32                      ilknIFnaxBWMbps
);


extern GT_STATUS prvCpssDxChPortDynamicPAIlknSpeedResolutionGet
(
    IN    GT_U8                       devNum,
    OUT   GT_U32                     *speedResulutionMBpsPtr,
    OUT   GT_U32                     *ilknIFnaxBWMbpsPtr
);



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif

