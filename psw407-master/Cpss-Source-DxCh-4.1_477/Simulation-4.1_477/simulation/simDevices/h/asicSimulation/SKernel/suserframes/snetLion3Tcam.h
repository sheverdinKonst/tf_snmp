/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetLion3Tcam.h
*
* DESCRIPTION:
*       This is a external API definition for SIP5 Tcam
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 9 $
*
*******************************************************************************/
#ifndef __snetLion3Tcamh
#define __snetLion3Tcamh

#include <asicSimulation/SKernel/smain/smain.h>
#include <asicSimulation/SKernel/suserframes/snetCheetahPclSrv.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* max number of parallel lookups (hitNum) */
#define SIP5_TCAM_MAX_NUM_OF_HITS_CNS               4

/* max number of floors */
#define SIP5_TCAM_MAX_NUM_OF_FLOORS_CNS             12

/* number of relevant banks in each floor */
#define SIP5_TCAM_NUM_OF_BANKS_IN_FLOOR_CNS         12

/* number of X lines in the bank */
#define SIP5_TCAM_NUM_OF_X_LINES_IN_BANK_CNS       256

/* number of tcam groups */
#define SIP5_TCAM_NUM_OF_GROUPS_CNS                 5

/* max size of key array (bytes) */
#define SIP5_TCAM_MAX_SIZE_OF_KEY_ARRAY_CNS        84


typedef enum{
    SIP5_TCAM_CLIENT_TTI_E      ,     /* TTI           */
    SIP5_TCAM_CLIENT_IPCL0_E    ,     /* Ingress Pcl 0 */
    SIP5_TCAM_CLIENT_IPCL1_E    ,     /* Ingress Pcl 1 */
    SIP5_TCAM_CLIENT_IPCL2_E    ,     /* Ingress Pcl 2 */
    SIP5_TCAM_CLIENT_EPCL_E     ,     /* Egress Pcl    */

    SIP5_TCAM_CLIENT_LAST_E           /* last value */
}SIP5_TCAM_CLIENT_ENT;


/* Enum values represent byte size of each key */
typedef enum{
    SIP5_TCAM_KEY_SIZE_10B_E = 1,     /* 10 bytes */
    SIP5_TCAM_KEY_SIZE_20B_E = 2,     /* 20 bytes */
    SIP5_TCAM_KEY_SIZE_30B_E = 3,     /* 30 bytes */
    SIP5_TCAM_KEY_SIZE_40B_E = 4,     /* 40 bytes */
    SIP5_TCAM_KEY_SIZE_50B_E = 5,     /* 50 bytes */
    SIP5_TCAM_KEY_SIZE_60B_E = 6,     /* 60 bytes */
    SIP5_TCAM_KEY_SIZE_80B_E = 8,     /* 80 bytes */

    SIP5_TCAM_KEY_SIZE_LAST_E         /* last value */
}SIP5_TCAM_KEY_SIZE_ENT;


/*******************************************************************************
*   sip5TcamLookup
*
* DESCRIPTION:
*       sip5 function that do lookup in tcam for given key and fill the
*       results array
*
* INPUTS:
*       devObjPtr     - (pointer to) the device object
*       tcamClient    - tcam client
*       keyArrayPtr   - key array (size up to 80 bytes)
*       keySize       - size of the key
*
* OUTPUTS:
*       resultPtr     - array of action table results, up to
*                       SIP5_TCAM_MAX_NUM_OF_HITS_CNS results are supported
*
* RETURNS:
*       number of hits found
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 sip5TcamLookup
(
    IN  SKERNEL_DEVICE_OBJECT    *devObjPtr,
    IN  SIP5_TCAM_CLIENT_ENT      tcamClient,
    IN  GT_U32                   *keyArrayPtr,
    IN  SIP5_TCAM_KEY_SIZE_ENT    keySize,
    OUT GT_U32                    resultArr[SIP5_TCAM_MAX_NUM_OF_HITS_CNS]
);

/*******************************************************************************
*   snetLion3TcamGetKeySizeBits
*
* DESCRIPTION:
*       sip5 function that returns size bits (4 bits),
*       these bits must be added to each chunk (bits 0..3)
*
* INPUTS:
*       keySize - key size
*
* OUTPUTS:
*       None
*
* RETURNS:
*       sizeBits value
*
* COMMENTS:
*
*******************************************************************************/
GT_U32 snetLion3TcamGetKeySizeBits
(
    IN  SIP5_TCAM_KEY_SIZE_ENT    keySize
);

/*******************************************************************************
*   snetSip5PclTcamLookup
*
* DESCRIPTION:
*       sip5 function that do pcl lookup in tcam
*
* INPUTS:
*       devObjPtr     - (pointer to) the device object
*       iPclTcamClient    - tcam client
*       u32keyArrayPtr- key array (GT_U32)
*       keyFormat     - format of the key
*
* OUTPUTS:
*       matchIdxPtr   - (pointer to) match indexes array
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID snetSip5PclTcamLookup
(
    IN  SKERNEL_DEVICE_OBJECT    *devObjPtr,
    IN  SIP5_TCAM_CLIENT_ENT      iPclTcamClient,
    IN  GT_U32                   *u32keyArrayPtr,
    IN  CHT_PCL_KEY_FORMAT_ENT    keyFormat,
    OUT GT_U32                   *matchIndexPtr
);

/*******************************************************************************
*   sip5TcamConvertPclKeyFormatToKeySize
*
* DESCRIPTION:
*       sip5 function that do convertation of old tcam key format to sip5 key size
*
* INPUTS:
*       keyFormat     - format of the key
*
* OUTPUTS:
*       sip5KeySizePtr   - sip5 key size
*
* COMMENTS:
*
*******************************************************************************/
GT_VOID sip5TcamConvertPclKeyFormatToKeySize
(
    IN  CHT_PCL_KEY_FORMAT_ENT    keyFormat,
    OUT SIP5_TCAM_KEY_SIZE_ENT   *sip5KeySizePtr
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __snetLion3Tcamh */









