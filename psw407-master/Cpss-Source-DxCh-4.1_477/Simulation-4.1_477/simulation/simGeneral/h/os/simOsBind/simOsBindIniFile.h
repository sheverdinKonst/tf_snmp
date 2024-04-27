/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simOsBindIniFile.h
*
* DESCRIPTION:
*       Operating System wrapper. Ini file facility.
*
*       simOsGetCnfValue            SIM_OS_GET_CNF_VALUE_FUN
*       simOsSetCnfFile             SIM_OS_SET_CNF_FILE_FUN
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/

#ifndef __simOsBindIniFileh
#define __simOsBindIniFileh

/************* Includes *******************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/************ Defines  ********************************************************/

#define   SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS   128


/************* Functions ******************************************************/

/*******************************************************************************
* SIM_OS_GET_CNF_VALUE_FUN
*
* DESCRIPTION:
*       Gets a specified parameter value from ini file.
*
* INPUTS:
*       chap_name    - chapter name
*       val_name     - the value name
*       data_len     - data length to be read
*
* OUTPUTS:
*       valueBuf     - the value of the parameter
*
* RETURNS:
*       GT_TRUE   - if the action succeeds
*       GT_FALSE - if the action fails
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_BOOL (*SIM_OS_GET_CNF_VALUE_FUN)
(
    IN  char     *chapNamePtr,
    IN  char     *valNamePtr,
    IN  GT_U32   data_len,
    OUT char     *valueBufPtr
);

/*******************************************************************************
* SIM_OS_SET_CNF_FILE_FUN
*
* DESCRIPTION:
*       Sets the config file name.
*
* INPUTS:
*       fileNamePtr - pointer to file name
*
* OUTPUTS:
*
* RETURNS:
*       The value of the desired parameter
*       NULL - if no such parameter found
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef void (*SIM_OS_SET_CNF_FILE_FUN)
(
    IN  char *fileNamePtr
);

/* SIM_OS_FUNC_BIND_INI_FILE_STC -
*    structure that hold the "os INI file" functions needed be bound to SIM.
*
*/
typedef struct{
    SIM_OS_GET_CNF_VALUE_FUN        simOsGetCnfValue;
    SIM_OS_SET_CNF_FILE_FUN         simOsSetCnfFile;
}SIM_OS_FUNC_BIND_INI_FILE_STC;

extern   SIM_OS_GET_CNF_VALUE_FUN       SIM_OS_MAC(simOsGetCnfValue);
extern   SIM_OS_SET_CNF_FILE_FUN        SIM_OS_MAC(simOsSetCnfFile);

#ifdef __cplusplus
}
#endif

#endif  /* __simOsBindIniFileh */

