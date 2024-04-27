/****************************************************
* cmdFS.h
*
* DESCRIPTION:
*       file system API.
*       Currently required for Lua running on board
*
* DEPENDENCIES:
*
* COMMENTS:
*
* FILE REVISION NUMBER:
*       $Revision: 4 $
*******************************************************************************/

#ifndef __cmdFS_h__
#define __cmdFS_h__

/***** Defines  ********************************************************/
#define CMD_FS_O_RDONLY 0x01
#define CMD_FS_O_WRONLY 0x02
#define CMD_FS_O_RDWR   0x03
#define CMD_FS_O_CREAT  0x04
#define CMD_FS_O_EXCL   0x08
#define CMD_FS_O_TRUNC  0x10
#define CMD_FS_O_APPEND 0x20
/* special flag to compress data on new created file */
#define CMD_FS_O_COMPRESS 0x40

#define CMD_FS_SEEK_SET 0
#define CMD_FS_SEEK_CUR 1
#define CMD_FS_SEEK_END 2

/* bitmask for the file type bitfields */
#define CMD_FS_S_IFMT     0170000
/* regular file */
#define CMD_FS_S_IFREG    0100000
/* directory */
#define CMD_FS_S_IFDIR    0040000
/* is it a regular file? */
#define CMD_FS_S_ISREG(m) ((m) & CMD_FS_S_IFREG)
/* directory? */
#define CMD_FS_S_ISDIR(m) ((m) & CMD_FS_S_IFDIR)


#define CMD_FS_NAME_MAX 256

/***** Public Types ****************************************************/
typedef struct CMD_FS_STAT_STC {
    int st_size; /* total size, in bytes */
    int st_mode; /* file mode, see CMD_FS_S_* */
    unsigned st_ctime; /* creation time (time_t) */
    unsigned st_mtime; /* modification time (time_t) */
} CMD_FS_STAT_STC;

typedef struct CMD_FS_DIRENT_STC {
    char d_name [CMD_FS_NAME_MAX+1];
} CMD_FS_DIRENT_STC;
/***** Public Functions ************************************************/

/*******************************************************************************
* cmdFSinit
*
* DESCRIPTION:
*       Initialize cmdFS, initialize built-in files
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
int cmdFSinit(void);

/*******************************************************************************
* cmdFSlastError
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
const char* cmdFSlastError(void);




/*******************************************************************************
* cmdFSopen
*
* DESCRIPTION:
*       Open and possible create a new file
*
* INPUTS:
*       name    - file name
*       flags   - open flags, see CMD_FS_O_*,
*                 like POSIX open()
*
* OUTPUTS:
*       None
*
* RETURNS:
*       file descriptor or < 0 if error
*
* COMMENTS:
*       read POSIX open() as reference
*
*******************************************************************************/
int cmdFSopen(const char* name, int flags);

/*******************************************************************************
* cmdFSclose
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
int cmdFSclose(int fd);

/*******************************************************************************
* cmdFSread
*
* DESCRIPTION:
*       read from a file descriptor
*
* INPUTS:
*       fd      - file descriptor
*       count   - number of bytes to read
*
* OUTPUTS:
*       buf     - read data to buffer started at this pointer
*
* RETURNS:
*       The number of bytes read or <0 of error
*
* COMMENTS:
*       read POSIX read() as reference
*
*******************************************************************************/
int cmdFSread(int fd, void *buf, int count);

/*******************************************************************************
* cmdFSwrite
*
* DESCRIPTION:
*       Write to a file descriptor
*
* INPUTS:
*       fd      - file descriptor
*       buf     - write from buffer started at this pointer
*       count   - number of bytes to write
*
* OUTPUTS:
*       None
*
* RETURNS:
*       The number of bytes written or <0 of error
*
* COMMENTS:
*       read POSIX write() as reference
*
*******************************************************************************/
int cmdFSwrite(int fd, const void *buf, int count);

/*******************************************************************************
* cmdFSlseek
*
* DESCRIPTION:
*       reposition read/write file offset
*
* INPUTS:
*       fd      - file descriptor
*       offset  - 
*       whence  - one of
*                 CMD_FS_SEEK_SET   - The offset is set to offset bytes
*                 CMD_FS_SEEK_CUR   - The offset is set to current location
*                                     plus offset bytes
*                 CMD_FS_SEEK_END   - The offset is set to size of the file
*                                     PLUS offset bytes
*
* OUTPUTS:
*       None
*
* RETURNS:
*       the resulting offset location as measured in bytes from the
*                 beginning of the file.
*       -1 if error
*
* COMMENTS:
*       read POSIX lseek() as reference
*
*******************************************************************************/
int cmdFSlseek(int fd, int offset, int whence);

/*******************************************************************************
* cmdFSmmap
*
* DESCRIPTION:
*       Map file content to RAM
*
* INPUTS:
*       fd      - file descriptor
*
* OUTPUTS:
*       None
*
* RETURNS:
*       pointer to file data
*
* COMMENTS:
*       
*
*******************************************************************************/
void* cmdFSmmap(int fd);

/*******************************************************************************
* cmdFSmunmap
*
* DESCRIPTION:
*       Unmap 
*
* INPUTS:
*       fd      - file descriptor
*       ptr     - pointer to mapped memory
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*       read POSIX write() as reference
*
*******************************************************************************/
void cmdFSmunmap(int fd, void *ptr);

/*******************************************************************************
* cmdFSstat
*
* DESCRIPTION:
*       get file status
*
* INPUTS:
*       name    - file name
*
* OUTPUTS:
*       buf     - pointer to stat structure
*
* RETURNS:
*       0 if success
*       -1 if error
*
* COMMENTS:
*       read POSIX stat() as reference
*
*******************************************************************************/
int cmdFSstat(const char* name, CMD_FS_STAT_STC *buf);

/*******************************************************************************
* cmdFSunlink
*
* DESCRIPTION:
*       delete a name and possibly the file it refers to
*
* INPUTS:
*       name    - file name
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0   - on success
*
* COMMENTS:
*       read POSIX unlink() as reference
*
*******************************************************************************/
int cmdFSunlink(const char* name);


/*******************************************************************************
* cmdFSopendir
*
* DESCRIPTION:
*       open a directory
*
* INPUTS:
*       name    - directory name
*                 (will beignored in current implementation)
*
* OUTPUTS:
*       None
*
* RETURNS:
*       directory file descriptor or <0 if error
*
* COMMENTS:
*
*******************************************************************************/
int cmdFSopendir(const char *name);

/*******************************************************************************
* cmdFSclosedir
*
* DESCRIPTION:
*       close a directory
*
* INPUTS:
*       fd      - directory file descriptor
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0 if success or <0 if error
*
* COMMENTS:
*
*******************************************************************************/
int cmdFSclosedir(int fd);


/*******************************************************************************
* cmdFSreaddir
*
* DESCRIPTION:
*       read a directory entry
*
* INPUTS:
*       fd      - directory file descriptor
*
* OUTPUTS:
*       dirPtr  - pointer to directory entry structure
*
* RETURNS:
*       1 On success
*       0 On end of directory
*       <0 On error
*
* COMMENTS:
*
*******************************************************************************/
int cmdFSreaddir(int fd, CMD_FS_DIRENT_STC *dirPtr);



#endif /* __cmdFS_h__ */

#if undef
/* Sample */
int ls(void)
{
    CMD_FS_DIRENT_STC   dirent;
    CMD_FS_STAT_STC     stat;
    int                     dirFd;
    int                     rc;

    dirFd = cmdFSopendir(NULL);
    if (dirFd < 0)
    {
        printf("err\n");
        return -1;
    }
    rc = 1;
    while (rc == 1)
    {
         rc = cmdFSreaddir(dirFd, &dirent);
         if (rc == 1)
         {
             cmdFSstat(dirent.d_name, &stat);
             /* print file name and size */
             printf("%-30s %d\n", dirent.d_name, stat.st_size);
         }
    }
    cmdFSclosedir(dirFd);
    return 0;
}
#endif
