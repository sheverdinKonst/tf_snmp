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
*       $Revision: 6 $
************************************************************************/

/***** Include files ***************************************************/
#include <string.h>
#include <stdio.h>

#include <os/simTypesBind.h>
#include <asicSimulation/SInit/sinit.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SEmbedded/simFS.h>
#include <common/Version/sstream.h>
#include <asicSimulation/SKernel/smain/smain.h>

/***** Defines  ********************************************************/
#define SIM_FS_STRCMP(a,b)    strcmp(a,b)
#define SIM_FS_BZERO(p,l)     memset(p,0,l)
#define SIM_FS_BCOPY(s,d,l)   memcpy(d,s,l)

#define SIM_FS_MAX_OPEN_FILES   10

/***** Private Types ***************************************************/

typedef struct SIM_FS_INODE_STC {
    const char              *name;
    const struct SIM_FS_INODE_STC *next;
    size_t                  size;
    char                    linked_files[20][100];
    char                    data[1];
} SIM_FS_INODE_STC;

typedef struct SIM_FS_FDESCR_STC {
    int                     flags;      /* open flags, 0 means not opened */
    const SIM_FS_INODE_STC  *inode;
    size_t                  pos;        /* current position */
} SIM_FS_FDESCR_STC;

/***** Private Data ****************************************************/
#include <asicSimulation/SEmbedded/simFS_embed_files.inc>

static const SIM_FS_INODE_STC *simFSdir = SIMFS_DIR;
static SIM_FS_FDESCR_STC    fdescr[SIM_FS_MAX_OPEN_FILES];
static const char *last_error = "";
char  simFSiniFileDirectory[512] = {0}; /* temp dir name */

/***** Private Functions ***********************************************/
#ifndef LINUX_SIM /* win32 only code */
    #define EMBEDDED_FS_TEMP_DIR    "%s\\%s\\%s\\"
    #define USER_TEMP_DIR           "TMP"
    static GT_VOID simFsMakeDir(const char* pathtocheck)
    {
        char cmdLine[255];
        if(CHDIR(pathtocheck) == -1)
        {
            sprintf(cmdLine, "mkdir %s", pathtocheck);
            if (system(cmdLine))
            {
                skernelFatalError("simFsMakeDir: failed to cretae directory %s\n", pathtocheck);
            }
        }
    }
#else   /* linux only code */
    #define EMBEDDED_FS_TEMP_DIR    "%s/%s/%s/"
    #define USER_TEMP_DIR           "HOME"
    static GT_VOID simFsMakeDir(const char* pathtocheck)
    {
        char cmdLine[255];
        sprintf(cmdLine, "mkdir -p %s", pathtocheck);
        if (system(cmdLine))
        {
            skernelFatalError("simFsMakeDir: failed to cretae directory \n");
        }
    }
#endif /* borland / VC check */

/* application executable path */
extern char   appExePath[];

/*******************************************************************************
* SIM_FS_CHECK_FD
*
* DESCRIPTION:
*       Check that fd is valid opened file descriptor
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       err_ret  - on error
*
* COMMENTS:
*
*******************************************************************************/
#define FD                  fdescr[fd]
#define SIM_FS_CHECK_FD(err_ret) \
    if (fd < 0 || fd >= SIM_FS_MAX_OPEN_FILES) \
    { \
        last_error = "Wrong file descriptor"; \
        return err_ret; \
    } \
    if (FD.flags == 0) \
    { \
        last_error = "Bad file descriptor"; \
        return err_ret; \
    }

/***** Public Functions ************************************************/

/*******************************************************************************
* simFSinit
*
* DESCRIPTION:
*       Initialize built-in files
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
int simFSinit(void)
{
    /* init built-in files */
    SIM_FS_BZERO(fdescr, sizeof(fdescr));
    return 0;
}

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
const char* simFSlastError(void)
{
    return last_error;
}

/*******************************************************************************
* simFSopen
*
* DESCRIPTION:
*       Open and possible create a new file
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
int simFSopen(const char* name)
{
    int fd;
    const SIM_FS_INODE_STC  *inode;

    /* look for new fd */
    for (fd = 0; fd < SIM_FS_MAX_OPEN_FILES; fd++)
    {
        if (FD.flags == 0)
        {
            break;
        }
    }
    if (fd >= SIM_FS_MAX_OPEN_FILES)
    {
        last_error = "No enough descriptors";
        return -1;
    }
    /* unused file descriptor found. lock it first */
    FD.flags = -1; /* lock */

    /* Scan directory linked list for given file */
    for (inode = simFSdir; inode; inode = inode->next)
    {
        if (!SIM_FS_STRCMP(inode->name, name))
        {
            break;
        }
    }
    /* if not found return error */
    if (inode == NULL)
    {
        FD.flags = 0; /* unlock */
        last_error = "No such file";
        return -1;
    }

    /* initialize file descripttor */
    FD.flags = 1;
    FD.inode = inode;
    FD.pos = 0;

    return fd;
}

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
*       read POSIX close() as reference
*
*******************************************************************************/
int simFSclose(int fd)
{
    SIM_FS_CHECK_FD(-1);
    FD.flags = 0;
    return 0;
}

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
char* simFSgets(int fd, char *buf, int size)
{
    int i;

    SIM_FS_CHECK_FD(NULL);

    if (size < 1)
    {
        last_error = "Bad parameter: size, must be >= 1";
        return NULL;
    }
    if (buf == NULL)
    {
        last_error = "Bad parameter: buf";
        return NULL;
    }

    if (FD.pos >= FD.inode->size)
        return NULL;
    for (i = 0; i < size - 1 && FD.pos < FD.inode->size; )
    {
        buf[i++] = FD.inode->data[FD.pos++];
        if (buf[i-1] == '\n')
            break;
    }
    buf[i] = 0;
    return buf;
}

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
int simFSprint(const char *name)
{
    int fd;
    char buf[1024];
    int k;

    simFSinit();

    simForcePrintf("*************** file: %s\n", name);
    fd = simFSopen(name);
    if (fd < 0)
    {
        simForcePrintf("error opening file: %s\n",simFSlastError());
        return -1;
    }
    k = 0;
    while (simFSgets(fd, buf, sizeof(buf)) != NULL)
    {
        simForcePrintf("%d: %s",++k,buf);
    }
    simFSclose(fd);
    simForcePrintf("*************** done\n");

    return 0;
}

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
int simFSsave(INOUT char *dirName, IN const char *fname, IN char files[20][100])
{
    int  fd;
    char buf[1024];
    char full_fname[512] = {0}; /* full file name */
    FILE *fnameFilePrt;

    ASSERT_PTR(dirName);

    simFSinit();

    /* TEMP directory not exist */
    if(dirName[0] == 0)
    {
        /* Create temporary embedded FS directory per USER and process */
        sprintf(dirName, EMBEDDED_FS_TEMP_DIR, getenv(USER_TEMP_DIR), "embeddedFs",
                CHIP_SIMULATION_STREAM_NAME_CNS);
        simFsMakeDir(dirName);

        /* Change directory to temporary */
        if ( CHDIR(dirName) )
        {
            simForcePrintf("The dir [%s] does not exist \n\n", dirName);
            SIM_OS_MAC(simOsAbort)();
        }
        strcpy(simFSiniFileDirectory, dirName);
    }

    fd = simFSopen(fname);
    if (fd < 0)
    {
        return -1;
    }

    /* append dir name */
    SIM_FS_BCOPY(dirName, full_fname, strlen(dirName));
    strcat(full_fname, fname);

    /* file print */
    if(strlen(full_fname) != 0)
    {
        fnameFilePrt = fopen(full_fname, "w" );
        if(fnameFilePrt)
        {
            while (simFSgets(fd, buf, sizeof(buf)) != NULL)
            {
                if( 0 > fprintf(fnameFilePrt, "%s", buf))
                {
                    skernelFatalError("simFSsave: cannot write to file %s \n", full_fname);
                }
            }
            fflush(fnameFilePrt);
            fclose (fnameFilePrt);
        }
        else
        {
            skernelFatalError("simFSsave: cannot write the file %s \n", full_fname);
        }
    }
    else
    {
        skernelFatalError("simFSsave: wrong file name\n");
    }

    if(files)
    {
        SIM_FS_BCOPY(FD.inode->linked_files, files, sizeof(FD.inode->linked_files));
    }

    simFSclose(fd);
    return 0;
}

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
void simFSprintIniList(void)
{
    const SIM_FS_INODE_STC  *inode;

    simForcePrintf("*******************Embedded INI files list start:\n");

    /* Scan directory linked list for given file */
    for (inode = simFSdir; inode; inode = inode->next)
    {
        if(strstr(inode->name, "ini"))
        {
            simForcePrintf("%s\n", inode->name);
        }
    }

    simForcePrintf("\n*******************Embedded INI files list end\n");
}

