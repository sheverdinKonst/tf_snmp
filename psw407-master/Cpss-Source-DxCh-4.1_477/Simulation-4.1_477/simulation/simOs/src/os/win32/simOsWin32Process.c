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
*       Win32 Operating System Simulation. Process facility implementation.
*
* FILE REVISION NUMBER:
*       $Revision: 2 $
*******************************************************************************/

/* WA to avoid next warning :
   due to include to : #include <windows.h>
    c:\program files\microsoft visual studio\vc98\include\rpcasync.h(45) :
    warning C4115: '_RPC_ASYNC_STATE' : named type definition in parentheses
*/
struct _RPC_ASYNC_STATE;
#include <windows.h>
#include <os/simTypesBind.h>

typedef struct
{
    GT_SIGNAL_ID        signalId;
    PROCESS_HADLER_FUN  handlerFunc;
}MESSAGE_HANDLER_STC;

#define MESSAGE_HANDLER_MAX_CNS 20

static GT_U32   munMsgHandler = 0;
static MESSAGE_HANDLER_STC msgHandlerDb[MESSAGE_HANDLER_MAX_CNS];

/* ID of thread that is used for messaging the 'process' ,
   we create a task that 'represent' our process to get messages from other
   processes */
static     DWORD processRepresentativeThreadId = 0;

static unsigned __TASKCONV processTask(void*   dummyPtr)
{
    GT_U32  ii;
    MSG     msg;

    processRepresentativeThreadId = GetCurrentThreadId();

    /* wait for messages from another process */
    while(GetMessage(&msg,NULL,0,0))
    {
        if(msg.message < WM_USER)
        {
            /* ???? */
            continue;
        }

        for(ii = 0 ; ii < munMsgHandler; ii++)
        {
            if(msgHandlerDb[ii].signalId == (GT_SIGNAL_ID)(msg.message - WM_USER))
            {
                msgHandlerDb[ii].handlerFunc((msg.message - WM_USER));
            }
        }
    }

    return 0;
}

/*

    Description
    SIM_OS_SHARED_MEM_GET_FUN() returns the identifier of the shared memory segment associated with
    the value of the argument key. A new shared memory segment, with size equal
    to the value of size rounded up to a multiple of PAGE_SIZE, is created if
    key has the value IPC_PRIVATE or key isn't IPC_PRIVATE, no shared memory
    segment corresponding to key exists, and IPC_CREAT is specified in sharedMemFlag.
    If sharedMemFlag specifies both IPC_CREAT and IPC_EXCL and a shared memory segment
    already exists for key, then SIM_OS_SHARED_MEM_GET_FUN() fails with errno set to EEXIST.
    (This is analogous to the effect of the combination O_CREAT | O_EXCL for
    open(2).)

    The value sharedMemFlag is composed of:

    SHM_HUGETLB
        used for allocating HUGETLB pages for shared memory. IPC_CREAT to create
        a new segment. If this flag is not used, then SIM_OS_SHARED_MEM_GET_FUN() will find the
        segment associated with key and check to see if the user has
        permission to access the segment.
    IPC_EXCL
        used with IPC_CREAT to ensure failure if the segment already exists.
        mode_flags
        (least significant 9 bits) specifying the permissions granted to the
        owner, group, and world. These bits have the same format, and the same
        meaning, as the mode argument of open(2). Presently, the execute
        permissions are not used by the system.
    SHM_HUGETLB (since Linux 2.6)
        Allocate the segment using "huge pages." See the kernel source file
        Documentation/vm/hugetlbpage.txt for further information.
    SHM_NORESERVE (since Linux 2.6.15)
        This flag serves the same purpose as the mmap(2) MAP_NORESERVE flag.
        Do not reserve swap space for this segment. When swap space is reserved,
        one has the guarantee that it is possible to modify the segment.
        When swap space is not reserved one might get SIGSEGV upon a write if no
        physical memory is available. See also the discussion of the file
        /proc/sys/vm/overcommit_memory in proc(5).
        When a new shared memory segment is created, its contents are
        initialised to zero values, and its associated data structure,
        shmid_ds (see shmctl(2)), is initialised as follows:

    shm_perm.cuid
        and shm_perm.uid are set to the effective user ID of the calling process.
    shm_perm.cgid
        and shm_perm.gid are set to the effective group ID of the calling process.
        The least significant 9 bits of
        shm_perm.mode are set to the least significant 9 bit of sharedMemFlag.
    shm_segsz
        is set to the value of size.
    shm_lpid,
        shm_nattch, shm_atime and shm_dtime are set to 0.
    shm_ctime
        is set to the current time.
        If the shared memory segment already exists, the permissions are verified,
        and a check is made to see if it is marked for destruction.



    Return Value:

    A valid segment identifier, shmid, is returned on success, -1 on error.
    Errors
    On failure, errno is set to one of the following:

    EACCES
        The user does not have permission to access the shared memory segment,
        and does not have the CAP_IPC_OWNER capability.
    EEXIST
        IPC_CREAT | IPC_EXCL was specified and the segment exists.
    EINVAL
        A new segment was to be created and size < SHMMIN or size > SHMMAX, or
        no new segment was to be created, a segment with given key existed, but
        size is greater than the size of that segment.
    ENFILE
        The system limit on the total number of open files has been reached.
    ENOENT
        No segment exists for the given key, and IPC_CREAT was not specified.
    ENOMEM
        No memory could be allocated for segment overhead.
    ENOSPC
        All possible shared memory IDs have been taken (SHMMNI), or allocating a
        segment of the requested size would cause the system to exceed the
        system-wide limit on shared memory (SHMALL).
    EPERM
        The SHM_HUGETLB flag was specified, but the caller was not privileged
        (did not have the CAP_IPC_LOCK capability).
    Notes
        IPC_PRIVATE isn't a flag field but a key_t type. If this special value
        is used for key, the system call ignores everything but the least
        significant 9 bits of sharedMemFlag and creates a new shared memory segment
        (on success).
        The following limits on shared memory segment resources affect the SIM_OS_SHARED_MEM_GET_FUN() call:

    SHMALL
        System wide maximum of shared memory pages (on Linux, this limit can be
        read and modified via /proc/sys/kernel/shmall).
    SHMMAX
        Maximum size in bytes for a shared memory segment: policy dependent
        (on Linux, this limit can be read and modified via /proc/sys/kernel/shmmax).
    SHMMIN
        Minimum size in bytes for a shared memory segment: implementation
        dependent (currently 1 byte, though PAGE_SIZE is the effective minimum size).
    SHMMNI
        System wide maximum number of shared memory segments: implementation
        dependent (currently 4096, was 128 before Linux 2.3.99; on Linux,
        this limit can be read and modified via /proc/sys/kernel/shmmni).
        The implementation has no specific limits for the per process maximum
        number of shared memory segments (SHMSEG).

*/
GT_STATUS simOsSharedMemGet/* linux shmget , win32 CreateFileMapping,OpenFileMappings    */
(
    IN  GT_SH_MEM_KEY       key,
    IN  GT_U32              size,
    IN  GT_FLAGS            sharedMemFlag,
    OUT GT_SH_MEM_ID        *sharedMemIdPtr
)
{
    char                  bpBuff[30] ;
    HANDLE                hMyMapH ;

    if(sharedMemIdPtr == NULL)
    {
        return GT_BAD_PTR;
    }

    sprintf(bpBuff,"sharedMem%ld",(GT_U32)key) ;

    /* try to open the slan first */
    hMyMapH = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, bpBuff) ;

    if(!hMyMapH)
    {
        /* file is not open we need to create the file */
        hMyMapH = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE,
                                      0,size,bpBuff) ;
        if(!hMyMapH)
            return GT_FAIL;
    }

    *sharedMemIdPtr = (GT_SH_MEM_ID)((void*)hMyMapH);

    return GT_OK;
}

/*
    DESCRIPTION :
        The function SIM_OS_SHARED_MEM_ATTACH_FUN attaches the shared memory segment identified by
        sharedMemId to the address space of the calling process.
        The attaching address is specified by sharedMemAddr with one of the following
        criteria:
        If sharedMemAddr is NULL, the system chooses a suitable (unused) address at
        which to attach the segment.

        If sharedMemAddr isn't NULL and SHARED_MEM_RND is asserted in sharedMemFlags, the attach
        occurs at the address equal to sharedMemAddr rounded down to the nearest
        multiple of SHMLBA.
        Otherwise sharedMemAddr must be a page aligned address at which the attach occurs.

        If SHARED_MEM_READ_ONLY is asserted in sharedMemFlags, the segment is attached for reading
        and the process must have read permission for the segment.
        Otherwise the segment is attached for read and write and the process must
        have read and write permission for the segment.
        There is no notion of a write-only shared memory segment.

        The brk value of the calling process is not altered by the attach.
        The segment will automatically be detached at process exit.
        The same segment may be attached as a read and as a read-write one,
        and more than once, in the process's address space.

        On a successful SIM_OS_SHARED_MEM_ATTACH_FUN call the system updates the members of the
        sharedMemId_ds structure associated to the shared memory segment as follows:

        shm_atime is set to the current time.
        shm_lpid is set to the process-ID of the calling process.
        shm_nattch is incremented by one.
        Note that the attach succeeds also if the shared memory segment is marked
        to be deleted.

        The function shmdt detaches the shared memory segment located at the
        address specified by sharedMemAddr from the address space of the calling
        process. The to-be-detached segment must be currently attached with
        sharedMemAddr equal to the value returned by the its attaching SIM_OS_SHARED_MEM_ATTACH_FUN call.

        On a successful shmdt call the system updates the members of the
        sharedMemId_ds structure associated with the shared memory segment as follows:

        shm_dtime is set to the current time.
        shm_lpid is set to the process-ID of the calling process.
        shm_nattch is decremented by one. If it becomes 0 and the segment is
        marked for deletion, the segment is deleted.
        The occupied region in the user space of the calling process is unmapped.

    RETURN VALUE
        On failure both functions return -1 with errno indicating the error.
        On success SIM_OS_SHARED_MEM_ATTACH_FUN returns the address of the attached shared memory
        segment, and shmdt returns 0.
    ERRORS
        When SIM_OS_SHARED_MEM_ATTACH_FUN fails, errno is set to one of the following:
        EACCES
            The calling process has no access permissions for the requested attach type.
        EINVAL
            Invalid sharedMemId value, unaligned (i.e., not page-aligned and SHARED_MEM_RND
            was not specified) or invalid sharedMemAddr value, or failing attach at brk.
        ENOMEM
            Could not allocate memory for the descriptor or for the page tables.
            The function shmdt can fail only if there is no shared memory segment
            attached at sharedMemAddr, in such a case at return errno will be set to EINVAL.

*/
void * simOsSharedMemAttach/* linux shmat , win32 MapViewOfFile*/
(
    IN  GT_SH_MEM_ID sharedMemId,
    IN  const void *sharedMemAddr,
    IN  GT_FLAGS    sharedMemFlags
)
{
    void*   lpPtr;

    lpPtr = MapViewOfFile((HANDLE)((void*)sharedMemId), FILE_MAP_ALL_ACCESS,0,0,0) ;

    return lpPtr;
}


/*
    Description
    SIM_OS_PROCESS_ID_GET_FUNs() returns the process ID of the current process.
    (This is often used by routines that generate unique temporary filenames.)
*/
GT_STATUS simOsProcessIdGet/* linux-getpid ,win32 GetCurrentProcessId,OpenProcess     */
(
    OUT GT_PROCESS_ID   *processIdPtr
)
{
    GT_TASK_HANDLE  taskHandle;

    if(processIdPtr == NULL)
    {
        return GT_BAD_PTR;
    }

    if(processRepresentativeThreadId == 0)
    {
        /* crate task only for the first time */
        taskHandle = SIM_OS_MAC(simOsTaskCreate(GT_TASK_PRIORITY_NORMAL,processTask,NULL));
        if(taskHandle == NULL)
        {
            return GT_FAIL;
        }

        while(processRepresentativeThreadId == 0)
        {
            /* allow task to start running */
            SIM_OS_MAC(simOsSleep(10));
        }
    }

    *processIdPtr = (GT_PROCESS_ID)(void*)processRepresentativeThreadId;

    return GT_OK;
}


/*******************************************************************************
* SIM_OS_PROCESS_NOTIFY_FUN
*
* DESCRIPTION:
*       send notification to other process. with specific notification id.
*
*       see function 'kill' in Linux , ''
*
* INPUTS:
*       processId         - process Id
*       notificationId    - notification Id
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
GT_STATUS simOsProcessNotify /*linux - kill , win32 - PostMessage */
(
    IN GT_PROCESS_ID    processId,
    IN GT_U32           notificationId
)
{
    int        err;
    BOOL       ret;

    ret = PostThreadMessage((DWORD)((void*)processId),WM_USER + notificationId,0,0);

/*    ret = PostMessage((HWND)((void*)processId),WM_USER + notificationId,0,0);*/

    if(ret == FALSE)
    {
        err = GetLastError();
        return GT_FAIL;
    }


    return GT_OK;
}

/*
    DESCRIPTION
        The sigaction system call is used to change the action taken by a
        process on receipt of a specific signal.
        signum specifies the signal and can be any valid signal except SIGKILL
        and SIGSTOP.

        If act is non-null, the new action for signal signum is installed from
        act. If oldact is non-null, the previous action is saved in oldact.

        The sigaction structure is defined as something like


        struct sigaction {
            void (*sa_handler)(int);
            void (*sa_sigaction)(int, siginfo_t *, void *);
            sigset_t sa_mask;
            int sa_flags;
            void (*sa_restorer)(void);
        }

        On some architectures a union is involved - do not assign to both
        sa_handler and sa_sigaction.

        The sa_restorer element is obsolete and should not be used.
        POSIX does not specify a sa_restorer element.

        sa_handler specifies the action to be associated with signum and may be
        SIG_DFL for the default action, SIG_IGN to ignore this signal, or a
        pointer to a signal handling function.

        sa_mask gives a mask of signals which should be blocked during execution
        of the signal handler. In addition, the signal which triggered the
        handler will be blocked, unless the SA_NODEFER or SA_NOMASK flags are used.

        sa_flags specifies a set of flags which modify the behaviour of the signal
        handling process. It is formed by the bitwise OR of zero or more of the following:

    SA_NOCLDSTOP
        If signum is SIGCHLD, do not receive notification when child processes
        stop (i.e., when child processes receive one of SIGSTOP, SIGTSTP, SIGTTIN
        or SIGTTOU).
    SA_ONESHOT or SA_RESETHAND
        Restore the signal action to the default state once the signal handler
        has been called.
    SA_ONSTACK
        Call the signal handler on an alternate signal stack provided by
        sigaltstack(2). If an alternate stack is not available, the default
        stack will be used.
    SA_RESTART
        Provide behaviour compatible with BSD signal semantics by making certain
        system calls restartable across signals.
    SA_NOMASK or SA_NODEFER
        Do not prevent the signal from being received from within its own signal
        handler.
    SA_SIGINFO
        The signal handler takes 3 arguments, not one. In this case, sa_sigaction
        should be set instead of sa_handler. (The sa_sigaction field was added
        in Linux 2.1.86.)
        The siginfo_t parameter to sa_sigaction is a struct with the following
        elements

    siginfo_t {
        int      si_signo;  * Signal number *
        int      si_errno;  * An errno value *
        int      si_code;   * Signal code *
        pid_t    si_pid;    * Sending process ID *
        uid_t    si_uid;    * Real user ID of sending process *
        int      si_status; * Exit value or signal *
        clock_t  si_utime;  * User time consumed *
        clock_t  si_stime;  * System time consumed *
        sigval_t si_value;  * Signal value *
        int      si_int;    * POSIX.1b signal *
        void *   si_ptr;    * POSIX.1b signal *
        void *   si_addr;   * Memory location which caused fault *
        int      si_band;   * Band event *
        int      si_fd;     * File descriptor *
    }

*/
GT_STATUS simOsProcessHandler   /* linux - sigaction , win32- task that wait for events/messages */
(
    IN  GT_SIGNAL_ID        signalId,
    IN  PROCESS_HADLER_FUN  handlerFunc
)
{
    GT_U32  ii;

    for(ii = 0 ; ii < munMsgHandler; ii++)
    {
        if(msgHandlerDb[ii].signalId == signalId &&
           msgHandlerDb[ii].handlerFunc == handlerFunc)
        {
            /* already in DB */
            return GT_OK;
        }
    }

    /* not found */

    if(munMsgHandler == MESSAGE_HANDLER_MAX_CNS)
    {
        return GT_FULL;
    }

    msgHandlerDb[munMsgHandler].signalId = signalId;
    msgHandlerDb[munMsgHandler].handlerFunc = handlerFunc;

    munMsgHandler++;

    return GT_OK;
}

