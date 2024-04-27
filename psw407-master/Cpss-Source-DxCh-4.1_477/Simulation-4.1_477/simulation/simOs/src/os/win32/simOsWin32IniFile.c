/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simOsWin32Task.c
*
* DESCRIPTION:
*       Operating System wrapper. Ini file facility.
*
* DEPENDENCIES:
*       Win32
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*******************************************************************************/

#include <os/simTypesBind.h>
#include <io.h>
#include <common/SHOST/LOCAL/H_CONF/EXP/H_CONF.H>

/************* Defines ***********************************************/

/************ Public Functions ************************************************/

/*******************************************************************************
* simOsGetCnfValue
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
*       GT_OK   - if the action succeeds
*       GT_FAIL - if the action fails
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_BOOL simOsGetCnfValue
(
    IN  char     *chapNamePtr,
    IN  char     *valNamePtr,
    IN  GT_U32   data_len,
    OUT char     *valueBufPtr
)
{
    if(CNFG_get_cnf_value(chapNamePtr, valNamePtr,data_len,valueBufPtr))
        return GT_TRUE;
    else
        return GT_FALSE;

}

/*******************************************************************************
* simOsSetCnfFile
*
* DESCRIPTION:
*       Sets the config file name.
*
* INPUTS:
*       chap_name    - chapter name
*       val_name     - the value name
*       max_data_len - maximum line length to be read
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
void simOsSetCnfFile
(
    IN  char *fileNamePtr
)
{
        /* check if the INI file EXISTS */
    if((_access( fileNamePtr, 0 )) == -1 )
    {
        printf(" simOsSetCnfFile: the INI file [%s] not exist \n",
                fileNamePtr);

        SIM_OS_MAC(simOsAbort)();
    }
    else
        CNFG_set_cnf_file((GT_U8 *)fileNamePtr);
}

