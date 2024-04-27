#ifndef RS_INC_PKS_INCLUDED
#define RS_INC_PKS_INCLUDED

/*!**************************************************RND Template version 4.1
 *!                  P A C K A G E   S P E C I F I C A T I O N
 *!==========================================================================
 *$ TITLE: Router Server - General definitions
 *!--------------------------------------------------------------------------
 *$ FILENAME: m:\sw\routers\inc\defs.pks
 *!--------------------------------------------------------------------------
 *$ SYSTEM, SUBSYSTEM: Router Server, General definitions
 *!--------------------------------------------------------------------------
 *$ AUTHORS: Michael
 *!--------------------------------------------------------------------------
 *$ CREATION DATE: 29-Dec-94
 *!**************************************************************************
 *!
 *!**************************************************************************
 *!
 *$ GENERAL DESCRIPTION:
 *!
 *$ PACKAGE GLOBAL SERVICES:
 *!
 *$ PACKAGE USAGE:
 *!
 *$ ASSUMPTIONS:
 *!
 *$ SIDE EFFECTS:
 *!
 *!**************************************************************************
 *!*/

/*!**************************************************************************
 *$            EXTERNAL DECLARATIONS (IMPORT AND EXPORT)
 *!**************************************************************************
 *!*/


/*!**************************************************************************
 *$            PUBLIC DECLARATIONS (EXPORT)
 *!**************************************************************************
 *!*/


#ifdef PSS_PPC
 #include <inc/psos/bsp.h>
 #include <inc/psos/psos.h>
#endif

#include <stdio.h>

#include <string.h>
#include <stddef.h>
#include <stdarg.h>

#if (OS_TARGET) == FreeBSD && (OS_TARGET_RELEASE) == 4
        #define _VA_LIST_DEFINED
#endif

#ifdef LINUX_SIM
	#define WINDOWS_COMPATIBLE
#endif

#include <INC/DEFS1.DTD>

#ifdef VXMIPS
 #undef WINDOWS_COMPATIBLE
 #undef I960
 #include <inc/private/vxmips.dtd>
#endif

#ifdef VXPPC
 #undef WINDOWS_COMPATIBLE
 #undef I960
 #include <inc/private/vxppc.dtd>
#endif

#ifdef PSS_MIPS
 #undef WINDOWS_COMPATIBLE
 #include <inc/private/idtmips.dtd>
#endif

#ifdef PSS_PPC
 #undef WINDOWS_COMPATIBLE
 #include <inc/private/psppc.dtd>
#endif

#ifdef I960
 #undef WINDOWS_COMPATIBLE
 #include <inc/private/I960.dtd>
#endif

#ifdef M68K
 #undef WINDOWS_COMPATIBLE
 #include <inc/private/M68K.dtd>
#endif

#ifdef WINDOWS_COMPATIBLE
 #include <windows.h>
#endif

#ifdef _WIN32
    #ifdef _VISUALC
        #include <memory.h>
    #else
        #include <mem.h>
    #endif
 #include <windows.h>
 #include <inc/private/_win32.dtd>
#else
 #ifdef WINDOWS_COMPATIBLE
     #if defined(_linux) || defined(_uClinux)
       #include <INC/PRIVATE/_WIN32.DTD>
     #else
       #include <inc/private/_win32.dtd>
     #endif
 #endif/*WINDOWS_COMPATIBLE*/
#endif

#include <INC/DEFS2.DTD>

/*!**************************************************************************
 *$            PUBLIC VARIABLE DEFINITIONS (EXPORT)
 *!**************************************************************************
 *!*/

/*!**************************************************************************
 *$            PUBLIC FUNCTION DEFINITIONS (EXPORT)
 *!**************************************************************************
 *!*/

/*$ END OF RS_INC */

#endif

