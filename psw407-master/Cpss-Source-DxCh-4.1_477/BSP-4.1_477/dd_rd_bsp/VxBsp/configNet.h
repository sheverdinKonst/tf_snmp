/*******************************************************************************
*                Copyright 2004, MARVELL SEMICONDUCTOR, LTD.                   *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL.                      *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
*                                                                              *
* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, *
* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL    *
* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K.  *
* (MJKK), MARVELL ISRAEL LTD. (MSIL).                                          *
*******************************************************************************/
/*******************************************************************************
* configNet.h - Header File for : END Network configuration
*
* DESCRIPTION:
*       This file defines the network devices that can be attached to the MUX.
*       This file main purpose to prepare endDevTbl [] table which describes
*       the ethernet devices that will be attached to the MUX.
*
* DEPENDENCIES:
*       WRS endLib library.
*
*******************************************************************************/

#ifndef __INCconfigNeth
#define __INCconfigNeth

/* includes */
#include "vxWorks.h"
#include "end.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* defines  */

/* Intel's NIC device support */
#ifdef INCLUDE_FEI_END
	#define FEI_LOAD_FUNC		sysFei82557EndLoad
    #define FEI_BUFF_LOAN   1           /* we support buffer loan */
    #define FEI_LOAD_STRING_0 feiEndLoadStr[0]    
    #define FEI_LOAD_STRING_1 feiEndLoadStr[1]    
    #define FEI_LOAD_STRING_2 feiEndLoadStr[2]    
    #define FEI_LOAD_STRING_3 feiEndLoadStr[3]    
	
    char feiEndLoadStr [FEI_MAX_UNITS][END_DESC_MAX] = {{0},{0},{0},{0}};

    IMPORT END_OBJ* FEI_LOAD_FUNC(char*, void*);
#endif

#ifdef INCLUDE_MGI_END
IMPORT END_OBJ* mgiEndLoad(char *, void*);
#endif /* INCLUDE_MGI_END */

#ifdef INCLUDE_GEI_END
    #define GEI82543_LOAD_FUNC       gei82543EndLoad  /* external interface */
    #define GEI82543_BUFF_LOAN       1                /* enable buffer loan */
    #define GEI82543_LOAD_STRING_0 geiEndLoadStr[0]
    #define GEI82543_LOAD_STRING_1 geiEndLoadStr[1]
    #define GEI82543_LOAD_STRING_2 geiEndLoadStr[2]
    #define GEI82543_LOAD_STRING_3 geiEndLoadStr[3]   
        
    char geiEndLoadStr [GEI_MAX_UNITS][END_DESC_MAX] = {{0},{0},{0},{0}};

    IMPORT END_OBJ* GEI82543_LOAD_FUNC (char*, void*);

#endif /* INCLUDE_GEI_END */
#if defined(INCLUDE_SYSKONNECT)
    #define SGI_LOAD_FUNC       skGeEndLoad	 	 /* external interface */
    #define SGI_BUFF_LOAN       1                /* enable buffer loan */
	#define SGI_LOAD_STRING_0 sgiEndLoadStr[0]
	#define SGI_LOAD_STRING_1 sgiEndLoadStr[1]
	#define SGI_LOAD_STRING_2 sgiEndLoadStr[2]
	#define SGI_LOAD_STRING_3 sgiEndLoadStr[3]

	char sgiEndLoadStr [SGI_MAX_UNITS] [END_DESC_MAX] = {{0},{0},{0},{0}};

    IMPORT END_OBJ* SGI_LOAD_FUNC (char*, void*);

#endif /* INCLUDE_SYSKONNECT */

/* max number of ipAttachments we can have */
#undef  IP_MAX_UNITS
#define IP_MAX_UNITS (NELEMENTS (endDevTbl) - 1)


END_TBL_ENTRY endDevTbl [] =
{
#ifdef INCLUDE_MGI_END
    { 0, mgiEndLoad, "0", TRUE, NULL, FALSE},
    { 1, mgiEndLoad, "1", TRUE, NULL, TRUE}, 
    { 2, mgiEndLoad, "2", TRUE, NULL, TRUE},
    { 3, mgiEndLoad, "3", TRUE, NULL, TRUE},
#endif /* INCLUDE_MGI_END    */

#ifdef INCLUDE_GEI_END
    { 0, GEI82543_LOAD_FUNC, GEI82543_LOAD_STRING_0, GEI82543_BUFF_LOAN, NULL, TRUE},
    { 1, GEI82543_LOAD_FUNC, GEI82543_LOAD_STRING_1, GEI82543_BUFF_LOAN, NULL, TRUE},
    { 2, GEI82543_LOAD_FUNC, GEI82543_LOAD_STRING_2, GEI82543_BUFF_LOAN, NULL, TRUE},
    { 3, GEI82543_LOAD_FUNC, GEI82543_LOAD_STRING_3, GEI82543_BUFF_LOAN, NULL, TRUE},  
#endif /* INCLUDE_GEI_END */

#ifdef INCLUDE_FEI_END
    { 0, FEI_LOAD_FUNC, FEI_LOAD_STRING_0, FEI_BUFF_LOAN, NULL, TRUE},
    { 1, FEI_LOAD_FUNC, FEI_LOAD_STRING_1, FEI_BUFF_LOAN, NULL, TRUE},
    { 2, FEI_LOAD_FUNC, FEI_LOAD_STRING_2, FEI_BUFF_LOAN, NULL, TRUE},
    { 3, FEI_LOAD_FUNC, FEI_LOAD_STRING_3, FEI_BUFF_LOAN, NULL, TRUE},
#endif

#if defined(INCLUDE_SYSKONNECT)
    { 0, SGI_LOAD_FUNC, SGI_LOAD_STRING_0, SGI_BUFF_LOAN, NULL, TRUE},
    { 1, SGI_LOAD_FUNC, SGI_LOAD_STRING_1, SGI_BUFF_LOAN, NULL, TRUE},
    { 2, SGI_LOAD_FUNC, SGI_LOAD_STRING_2, SGI_BUFF_LOAN, NULL, TRUE},
    { 3, SGI_LOAD_FUNC, SGI_LOAD_STRING_3, SGI_BUFF_LOAN, NULL, TRUE},
#endif /* INCLUDE_SYSKONNECT */

    { 0, END_TBL_END, NULL, 0, NULL, FALSE}     /* End of END device table */
};

#ifdef __cplusplus
}
#endif

#endif /* __INCconfigNeth */
