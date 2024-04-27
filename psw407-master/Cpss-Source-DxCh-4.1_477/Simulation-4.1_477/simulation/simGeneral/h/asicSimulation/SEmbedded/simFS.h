/***********************************************************************
* simFS.h
*
* DESCRIPTION:
*       Read-only file system API.
*       Required for iniFiles and registerFiles to be built-in
*       into appDemoSim image
*
* DEPENDENCIES:
*
* COMMENTS:
*
* FILE REVISION NUMBER:
*       $Revision: 4 $
************************************************************************/


#ifndef __simFS_h__
#define __simFS_h__

#ifndef LINUX_SIM /* win32 only code */
#ifdef _BORLAND /* borland compiler */
#include <dir.h>
#define CHDIR chdir
#define GET_CURRENT_DIR getcwd
#else           /* VC compiler */
#include <direct.h>
#define CHDIR _chdir
#define GET_CURRENT_DIR _getcwd
#endif          /* win32 compiler check */

#else           /* linux only code */
#include <unistd.h>
#define CHDIR chdir
#define GET_CURRENT_DIR getcwd

#endif          /* win32 only code */


/***** Defines  ********************************************************/
/***** Public Types ****************************************************/
/***** Public Data *****************************************************/
extern char  simFSiniFileDirectory[512];/* temp dir name, default empty */

/***** Public Functions ************************************************/

/*******************************************************************************
* simFSinit
*
* DESCRIPTION:
*       Initialize simFS, initialize built-in files
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0   - on success
*
* COMMENTS:
*
*******************************************************************************/
int simSinit(void);

/*******************************************************************************
* simFSlastError
*
* DESCRIPTION:
*       Return string with last error description
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0   - on success
*
* COMMENTS:
*
*******************************************************************************/
const char* simFSlastError(void);

/*******************************************************************************
* simFSopen
*
* DESCRIPTION:
*       Open file for read-only
*
* INPUTS:
*       name    - file name
*
* OUTPUTS:
*       None
*
* RETURNS:
*       file descriptor or < 0 if error
*
* COMMENTS:
*
*******************************************************************************/
int simFSopen(const char* name);

/*******************************************************************************
* simFSclose
*
* DESCRIPTION:
*       Close a file descriptor
*
* INPUTS:
*       fd      - file descriptor
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0 if success, < 0 if error
*
* COMMENTS:
*
*******************************************************************************/
int simFSclose(int fd);

/*******************************************************************************
* simFSgets
*
* DESCRIPTION:
*       Return non-zero if end of file reached
*
* INPUTS:
*       fd      - file descriptor
*       size    - number of bytes to read
*
* OUTPUTS:
*       buf     - buffer to store string
*
* RETURNS:
*       Return pointer to string or NULL if end of file reached
*
* COMMENTS:
*
*******************************************************************************/
char* simFSgets(int fd, char *buf, int size);


/*******************************************************************************
* simFSprint
*
* DESCRIPTION:
*       print file name
*
* INPUTS:
*       name    - file name
*       Return non-zero if end of file reached
*
* OUTPUTS:
*       none
*
* RETURNS:
*       0 if success, < 0 if error
*
* COMMENTS:
*
*******************************************************************************/
int simFSprint(const char* name);


/*******************************************************************************
* simFSsave
*
* DESCRIPTION:
*       Save embedded ini file and it registers files to temporary directory
*       per user for unique process
*
* INPUTS:
*       dirName - directory name
*       fname   - ini file name
*
* OUTPUTS:
*       dirName - directory name
*
* RETURNS:
*       0 if success, < 0 if error
*
* COMMENTS:
*
*******************************************************************************/
int simFSsave(INOUT char *dirName, IN const char *fname, IN char files[20][100]);


/*******************************************************************************
* simFSprintIniList
*
* DESCRIPTION:
*       print  list of embedded ini files to stdout
*
* INPUTS:
*       none
*
* OUTPUTS:
*       none
*
* RETURNS:
*       none
*
* COMMENTS:
*
*******************************************************************************/
void simFSprintIniList(void);

#endif /* __simFS_h__ */
