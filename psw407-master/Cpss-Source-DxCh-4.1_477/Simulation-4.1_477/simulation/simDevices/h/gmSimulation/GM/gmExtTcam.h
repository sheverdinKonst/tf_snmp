/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* gmExtTcam.h
*
* DESCRIPTION:
*       External TCAM wrapper.
*
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*******************************************************************************/

#ifndef __gmExtTcamh
#define __gmExtTcamh

#ifdef __cplusplus
extern "C" {
#endif

/************* Includes *******************************************************/

/************* Definitions ****************************************************/

/*******************************************************************************
* GM_EXT_TCAM_INIT_FUNC
*
* DESCRIPTION:
*       Initialize the external TCAM.
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*GM_EXT_TCAM_INIT_FUNC)(
);

/*******************************************************************************
* GM_EXT_TCAM_SHUTDOWN_FUNC
*
* DESCRIPTION:
*       Shutdown the external TCAM.
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*GM_EXT_TCAM_SHUTDOWN_FUNC)(
);

/*******************************************************************************
* GM_EXT_TCAM_REQUEST_FUNC
*
* DESCRIPTION:
*       Send operation request to the external TCAM and return the external 
*		TCAM answer.
*
* INPUTS:
*       requestData  - track that contains the request data 
*
* OUTPUTS:
*       requestAns  - track that contains the external TCAM answer 
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*GM_EXT_TCAM_REQUEST_FUNC)(
	IN char *requestData, 
	OUT char *requestAns
);


/* GM_EXT_TCAM_BIND_STC -
    structure that hold the external TCAM functions needed be bound to gm.

*/
typedef struct{
    GM_EXT_TCAM_INIT_FUNC         extTcamInitFunc;
    GM_EXT_TCAM_SHUTDOWN_FUNC     extTcamShutDownFunc;
    GM_EXT_TCAM_REQUEST_FUNC      extTcamRequestFunc;
}GM_EXT_TCAM_FUNC_BIND_STC;


#ifdef __cplusplus
}
#endif

#endif  /* __gmExtTcamh */


