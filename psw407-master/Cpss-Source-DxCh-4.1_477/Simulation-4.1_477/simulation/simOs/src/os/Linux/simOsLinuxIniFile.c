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
* simOsLinuxIniFile.c
*
* DESCRIPTION:
*       Operating System wrapper. Ini file facility.
*
* DEPENDENCIES:
*
*
* FILE REVISION NUMBER:
*       $Revision: 7 $
*******************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include <os/simTypes.h>
#include <os/simTypesBind.h>
#ifndef EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES
#define EXPLICIT_INCLUDE_TO_SIM_OS_H_FILES
#endif
#include <os/simOsIniFile.h>
#include <os/simOsTask.h>

/************* Externals *********************************************/
extern int access(const char *, int);

/************* Defines ***********************************************/
#define CNFG_max_line_length_CNS 128

/************ Local variables *************************************************/
static char config_file_name[128];

/************ Forward declarations ********************************************/
static char* CNFG_chck_str
(
    IN  char    *str
);
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
    FILE      *fp;
    char      *p, *s, buf[ CNFG_max_line_length_CNS];
    unsigned  cmd = 0;
    GT_BOOL   ret_val = GT_FALSE;

    if ( (fp = fopen( config_file_name, "rt")) == NULL)
    {
        return GT_FALSE;
    }

    while ( fgets(buf, CNFG_max_line_length_CNS, fp))
    {
        if ( (p = CNFG_chck_str(buf)) == NULL)
        {
            /* skip empty lines and comments */
            continue;
        }
        if ( (s = strchr(buf, '[')) != NULL)
        {
            if (cmd)
                break;
            s = CNFG_chck_str( ++s);
            if( ! strncasecmp( s, chapNamePtr,  strlen(chapNamePtr)))
                cmd = 1;
            else
                cmd = 0;
            continue;
        }

        if( cmd)
        {
            if ( ! strncasecmp( p, valNamePtr, strlen(valNamePtr))
                &&
                /* check that the match is for the total word not only to the
                 same start of string !!! */
                ( isspace(p[strlen(valNamePtr)]) ||
                  p[strlen(valNamePtr)] == '=')
               )
            {
                if ( (s = strchr( p, '=')) != NULL)
                {
                    if ( (p = CNFG_chck_str(++s)) != NULL)
                    {
                        strncpy(valueBufPtr, p, data_len);
                        ret_val = GT_TRUE;
                        break;
                    }
                }
            }

        }
    }
    fclose( fp);

    return ret_val;
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
    if((access( fileNamePtr, F_OK )) == -1 )
    {
        printf(" simOsSetCnfFile: the INI file [%s] not exist \n",
                 fileNamePtr);

        simOsAbort();
    }
    else
        strcpy(config_file_name, fileNamePtr);
}


/*******************************************************************************
* CNFG_chck_str
*
* DESCRIPTION:
*       Strip leading and tailing spaces and comment lines
*
* INPUTS:
*       str          - pointer to string
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
static char* CNFG_chck_str
(
    IN  char    *str
)
{
    char    *s;


    if (str)
    {
        /* skip leading spaces */
        while( isspace(*str)) str++;

        s = strchr(str, 0);
        for (--s; s >= str && (*s == '\r' || *s == '\n' || isspace(*s));)
            *(s--) = 0;


        if ((*str >= ' ') && (*str <= '~') && (*str != ';'))
            return str;

    }
    return NULL;
}

