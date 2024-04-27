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
* simOsLinuxIniFileEmu.c
*
* DESCRIPTION:
*       Operating System wrapper. Ini file facility emulation.
*       (For simulation on board)
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*******************************************************************************/

#include <os/simTypes.h>
#define EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES
#include <asicSimulation/SInit/sinit.h>
#include <os/simOsIniFile.h>
#include <os/simOsTask.h>

/************* Defines ***********************************************/
typedef struct{
    char    chapterName[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
    char    fieldName[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
    char    fieldStringValue[SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS];
}DB_EMULATED_INI_FILE_STC;

#define DB_EMULATED_INI_FILE_NUM_FIELDS_CNS         50

static GT_U32   dbEmulatedIniFileNumEntries = 0;

static DB_EMULATED_INI_FILE_STC dbEmulatedIniFile[DB_EMULATED_INI_FILE_NUM_FIELDS_CNS];

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
    GT_U32  ii;

    if(chapNamePtr == NULL || valNamePtr == NULL)
    {
        return GT_BAD_PTR;
    }

    for(ii = 0; ii < dbEmulatedIniFileNumEntries ; ii++)
    {
        if(0 == strcmp(dbEmulatedIniFile[ii].chapterName,chapNamePtr) &&
           0 == strcmp(dbEmulatedIniFile[ii].fieldName,valNamePtr))
        {
            if(valueBufPtr && data_len != 0)
            {
                /* we have a match ,return value from the DB */
                strncpy(valueBufPtr,dbEmulatedIniFile[ii].fieldStringValue,data_len);

                printf("simOsGetCnfValue : chapNamePtr[%s] , valNamePtr[%s] , valueBufPtr[%s] \n",
                    chapNamePtr,valNamePtr,valueBufPtr);
            }

            return GT_TRUE;
        }
    }

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
    /* do nothing */
    return;
}


/*******************************************************************************
* simOsSetCnfValue
*
* DESCRIPTION:
*       Sets a specified parameter value to the emulated ini file.
*
* INPUTS:
*       chap_name    - chapter name
*       val_name     - the value name
*       data_len     - data length to write
*       valueBuf     - the value of the parameter
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK   - if the action succeeds
*       GT_BAD_PTR - if one of the pointers is NULL
*       GT_BAD_PARAM - if one of the strings is too long
*       GT_FULL - if the DB already full
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS simOsSetCnfValue
(
    IN  char     *chapNamePtr,
    IN  char     *valNamePtr,
    IN  char     *valueBufPtr
)
{

    printf("simOsSetCnfValue : chapNamePtr[%s] , valNamePtr[%s] , valueBufPtr[%s] \n",
        chapNamePtr ? chapNamePtr : "NULL" ,
        valNamePtr ? valNamePtr : "NULL",
        valueBufPtr ? valueBufPtr : "NULL");

    if(dbEmulatedIniFileNumEntries >= DB_EMULATED_INI_FILE_NUM_FIELDS_CNS)
    {
        return GT_FULL;
    }

    if(chapNamePtr == NULL || valNamePtr == NULL || valueBufPtr == NULL)
    {
        return GT_BAD_PTR;
    }

    /* check length of fields */
    if(strlen(chapNamePtr) >= SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS)
    {
        return GT_BAD_PARAM;
    }

    if(strlen(valNamePtr) >= SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS)
    {
        return GT_BAD_PARAM;
    }

    if(strlen(valueBufPtr) >= SIM_OS_CNF_FILE_MAX_LINE_LENGTH_CNS)
    {
        return GT_BAD_PARAM;
    }

    /* check if already in DB */
    if(GT_TRUE == simOsGetCnfValue(chapNamePtr,valNamePtr,0,NULL))
    {
        return GT_OK;
    }

    strcpy(dbEmulatedIniFile[dbEmulatedIniFileNumEntries].chapterName      ,chapNamePtr);
    strcpy(dbEmulatedIniFile[dbEmulatedIniFileNumEntries].fieldName        ,valNamePtr);
    strcpy(dbEmulatedIniFile[dbEmulatedIniFileNumEntries].fieldStringValue ,valueBufPtr);

    /* add entry to the DB */
    dbEmulatedIniFileNumEntries++;

    return GT_OK;
}

/*******************************************************************************
* simOsDumpCnfValues
*
* DESCRIPTION:
*       Dump all the values in the emulated ini file.
*
* INPUTS:
*
* OUTPUTS:
*
* RETURNS:
*       GT_OK   - if the action succeeds
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS simOsDumpCnfValues(void)
{
    GT_U32  ii;

    for(ii = 0; ii < dbEmulatedIniFileNumEntries ; ii++)
    {
        printf("simOsDumpCnfValues : chapNamePtr[%s] , valNamePtr[%s] , valueBufPtr[%s] \n",
            dbEmulatedIniFile[ii].chapterName ,
            dbEmulatedIniFile[ii].fieldName,
            dbEmulatedIniFile[ii].fieldStringValue);
    }

    return GT_OK;
}

