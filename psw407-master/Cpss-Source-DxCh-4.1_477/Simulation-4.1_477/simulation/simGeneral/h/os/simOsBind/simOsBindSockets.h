/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* simOsBindSockets.h
*
* DESCRIPTION:
*       This file define prototypes for sockets.
*
* FILE REVISION NUMBER:
*       $Revision: 6 $
*******************************************************************************/

#ifndef __simOsBindSocketsh
#define __simOsBindSocketsh

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

/* handle to df_set */
typedef struct{
    GT_VOID*  fd_set;
}GT_SOCK_FD_SET_STC;


/* check that not define by PSS/Cpss that already set types */
#ifndef __gtStackTypes
/* Data block size generic type */
typedef int  GT_SIZE, *GT_SIZE_PTR;
/* File descriptor generic type */
#ifndef _WIN32
typedef int GT_FD ;
#else
typedef GT_UINTPTR GT_FD ;
#endif
#endif /*!__gtStackTypes*/

#ifndef __gtOsSocket_h
#define GT_SOCKET_DEFAULT_SIZE    -1

/* Socket file descriptor and socket port generic type */
#ifndef _WIN32
typedef int GT_SOCKET_FD ;
#else
typedef GT_UINTPTR GT_SOCKET_FD ;
#endif
/* Socket file descriptor and socket port generic type */
typedef int GT_SOCKET_PORT ;

/*******************************************************************************
 * Typedef:     struct   SOCKET_LINGER
 *
 * Description: Structure sent to setsockopt in order to set the SO_LINGER
 *              option for the socket
 *
 * Fields :
 *       lOnOff     - enable/disable the linger option when closing socket.
 *       lLinger    - indicates the amount of time to linger.
 * Comments:
 *
 *******************************************************************************/
typedef struct
{
    GT_32   lOnOff;
    GT_32   lLinger;
}SOCKET_LINGER;



/*******************************************************************************
 * Typedef:     enum   SOCKET_SHUTDOWN_OPERATION
 *
 * Description: enum that describes what types of operation will no longer be allowed
 *
 * Fields :
 *       lOnOff     - enable/disable the linger option when closing socket.
 *       lLinger    - indicates the amount of time to linger.
 * Comments:
 *
 *******************************************************************************/
typedef enum
{
    SOCKET_SHUTDOWN_CLOSE_INPUT,                  /* receives disallowed */
    SOCKET_SHUTDOWN_CLOSE_OUTPUT,                 /* sends disallowed */
    SOCKET_SHUTDOWN_CLOSE_ALL                     /* sends and receives disallowed */
}SOCKET_SHUTDOWN_OPERATION;



/* Define socket's errors */
#define GT_SOCKET_ERROR_BASE    0x30000
/* Sockets definitions of regular Berkeley error constants */
#define GT_SOCKET_EAGAIN        (GT_SOCKET_ERROR_BASE + 11)
#define GT_SOCKET_EINTR         (GT_SOCKET_ERROR_BASE + 4)
#define GT_SOCKET_EINVAL        (GT_SOCKET_ERROR_BASE + 22)
#define GT_SOCKET_EMSGSIZE      (GT_SOCKET_ERROR_BASE + 36)
#define GT_SOCKET_EISCONN       (GT_SOCKET_ERROR_BASE + 56)
#define GT_SOCKET_SHUTDOWN      (GT_SOCKET_ERROR_BASE + 58)
#define GT_SOCKET_EWOULDBLOCK   (GT_SOCKET_ERROR_BASE + 70)
#define GT_SOCKET_EINPROGRESS   (GT_SOCKET_ERROR_BASE + 68)
#define GT_SOCKET_EALREADY      (GT_SOCKET_ERROR_BASE + 69)

#endif /*__gtOsSocket_h*/

/*******************************************************************************
* osSocketTcpCreate()
*
* DESCRIPTION:
*       Create a socket from TCP type
*
* INPUTS:
*       sockSize - the size of the receive & send socket buffer. -1 indicates
*                  the default size.
*
* OUTPUTS:
*
* RETURNS:
*       Valid file descriptor on success
*       -1 On fail
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_SOCKET_FD (*SIM_OS_SOCKET_TCP_CREATE_FUN) (
    IN  GT_32 sockSize
);


/*******************************************************************************
* osSocketUdpCreate()
*
* DESCRIPTION:
*       sockSize - the size of the receive & send socket buffer. -1 indicates
*                  the default size.*
* INPUTS:
*       None
*
* OUTPUTS:
*
* RETURNS:
*       Valid file descriptor on success
*       -1 On fail
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_SOCKET_FD (*SIM_OS_SOCKET_UDP_CREATE_FUN) (
    IN  GT_32 sockSize
);


/*******************************************************************************
* osSocketTcpDestroy()
*
* DESCRIPTION:
*       Destroy a TCP socket
*
* INPUTS:
*       socketFd - Socket descriptor
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on fail
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*SIM_OS_SOCKET_TCP_DESTROY_FUN)
(
    IN GT_SOCKET_FD   socketFd
);


/*******************************************************************************
* osSocketUdpDestroy()
*
* DESCRIPTION:
*       Destroy a UDP socket
*
* INPUTS:
*       socketFd - Socket descriptor
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on fail
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*SIM_OS_SOCKET_UDP_DESTROY_FUN)
(
    IN  GT_SOCKET_FD   socketFd
);


/*******************************************************************************
* osSocketCreateAddr()
*
* DESCRIPTION:
*       Build a TCP/IP address to be used across all other functions
*
* INPUTS:
*       ipAddr       - String represents a dotted decimal IP address, such as
*                      "10.2.40.10"
*       ipPort       - IP Port
*
* OUTPUTS:
*       sockAdd      - Pointer to a buffer pointer sent by the caller that is
*                      used as the address. The caller is unaware of what is
*                      the sockAddr size and type. The function allocates the
*                      required size for the structure
*       sockAddrLen  - Length of the size used in sockAdd
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on fail
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*SIM_OS_SOCKET_CREATE_ADDR_FUN)
(
    IN  GT_CHAR *        ipAddr,
    IN  GT_SOCKET_PORT   ipPort,
    IN  GT_VOID **       sockAddr,
    IN  GT_SIZE *        sockAddrLen
);


/*******************************************************************************
* osSocketDestroyAddr()
*
* DESCRIPTION:
*       Destroy a TCP/IP address object built via  osSocketBuildAddr()
*
* INPUTS:
*       sockAdd      - Pointer to a buffer pointer sent by the caller that is
*                      used as the address. The caller is unaware of what is
*                      the sockAddr size and type.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_VOID (*SIM_OS_SOCKET_DESTROY_ADDR_FUN)
(
    IN  GT_VOID *       sockAddr
);


/*******************************************************************************
* osSocketBind()
*
* DESCRIPTION:
*       Bind a name to a TCP or UDP socket
*
* INPUTS:
*       socketFd    - Socket descriptor
*       sockAddr    - A pointer to the local socket address structure,
*                     probably filled out by osSocketBuildAddr.The caller
*                     is unaware of what is the sockAddr type.
*       sockAddrLen - Length of socketAddr
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on fail
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*SIM_OS_SOCKET_BIND_FUN)
(
    IN  GT_SOCKET_FD     socketFd ,
    IN  GT_VOID *        sockAddr,
    IN  GT_SIZE          sockAddrLen
);


/*******************************************************************************
* osSocketListen()
*
* DESCRIPTION:
*       Listen for new connections on a TCP socket
*
* INPUTS:
*       socketFd       - Socket descriptor
*       maxConnections - Number of connections to queue
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on fail
*
* COMMENTS:
*       Used only for TCP connection oriented sockets
*
*******************************************************************************/
typedef GT_STATUS (*SIM_OS_SOCKET_LISTEN_FUN)
(
    IN  GT_SOCKET_FD     socketFd ,
    IN  GT_SIZE          maxConnections
);


/*******************************************************************************
* osSocketAccept()
*
* DESCRIPTION:
*       Extracts the first TCP connection request on the queue of pending
*       connections and creates a new connected socket.
*
* INPUTS:
*       socketFd    - Socket descriptor
*
* OUTPUTS:
*       sockAddr    - A pointer to a socket address structure, represents the
*                     client (target) address that requests the connection.
*                     If the server is not interested in the Client's protocol
*                     information, a NULL should be provided.
*       sockAddrLen - A value-result parameter, initialized to the size of
*                     the allocated buffer sockAddr, and modified on return
*                     to indicate the actual size of the address stored there.
*                     If the server is not interested in the Client's protocol
*                     information, a NULL should be provided.
*
* RETURNS:
*       socketFd - Socket descriptor
*
* COMMENTS:
*       Used only for TCP connection oriented sockets
*
*******************************************************************************/
typedef GT_SOCKET_FD (*SIM_OS_SOCKET_ACCEPT_FUN)
(
    IN  GT_SOCKET_FD     socketFd,
    OUT  GT_VOID *        sockAddr,
    OUT  GT_SIZE *        sockAddrLen
);


/*******************************************************************************
* osSocketConnect()
*
* DESCRIPTION:
*       Connect socket to a remote address:
*       For UDP sockets, specifies the address to which datagrams are sent by
*       default, and the only address from which datagrams are received. For
*       TCP sockets, used by the TCP client that attempts to make a connection
*       to the TCP server.
*
* INPUTS:
*       socketFd    - Socket descriptor
*       sockAddr    - A pointer to a socket address structure, represents the
*                     remote address to connect to. For TCP, it is the server
*                     address.
*       sockAddrLen - Length of sockAddr
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on fail
*
* COMMENTS:
*
*******************************************************************************/
typedef GT_STATUS (*SIM_OS_SOCKET_CONNECT_FUN)
(
    IN  GT_SOCKET_FD     socketFd,
    IN  GT_VOID *        sockAddr,
    IN  GT_SIZE          sockAddrLen
);


/*******************************************************************************
* osSocketSetNonBlock()
*
* DESCRIPTION:
*       Set a socket option to be non-blocked
*
* INPUTS:
*       socketFd - Socket descriptor
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on fail
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*SIM_OS_SOCKET_SET_NON_BLOCK_FUN)
(
    IN  GT_SOCKET_FD     socketFd
);


/*******************************************************************************
* osSocketSetBlock()
*
* DESCRIPTION:
*       Set a socket option to be blocked
*
* INPUTS:
*       socketFd    - Socket descriptor
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on fail
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*SIM_OS_SOCKET_SET_BLOCK_FUN)
(
    IN  GT_SOCKET_FD     socketFd
);


/*******************************************************************************
* osSocketSend()
*
* DESCRIPTION:
*       Send data to a TCP socket
*
* INPUTS:
*       socketFd    - Socket descriptor
*       buff        - Buffer to send
*       buffLen     - Max length to send
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Number of bytes sent
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_SIZE (*SIM_OS_SOCKET_SEND_FUN)
(
    IN  GT_SOCKET_FD   socketFd ,
    IN  GT_VOID *      buff,
    IN  GT_SIZE        buffLen
);


/*******************************************************************************
* osSocketSendTo()
*
* DESCRIPTION:
*       Send data to a UDP socket
*
* INPUTS:
*       socketFd    - Socket descriptor
*       buff        - Buffer to send
*       buffLen     - Max length to send
*       sockAddr    - A pointer to the target socket address structure,
*                     probably filled out by osSocketBuildAddr.  The caller
*                     is unaware of what is the sockAddr type.
*       sockAddrLen - Length of socketAddr
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Number of bytes sent
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_SIZE (*SIM_OS_SOCKET_SEND_TO_FUN)
(
    IN  GT_SOCKET_FD   socketFd ,
    IN  GT_VOID *      buff,
    IN  GT_SIZE        buffLen,
    IN  GT_VOID *      sockAddr,
    IN  GT_SIZE        sockAddrLen
);


/*******************************************************************************
* osSocketRecv()
*
* DESCRIPTION:
*       Receive data from a TCP socket
*
* INPUTS:
*       socketFd    - Socket descriptor
*       buff        - Buffer to receive
*       buffLen     - Max length to send
*       removeFlag  - Flag indicate if to remove or no remove the data from
*                     the socket buffer
*
* OUTPUTS:
*
* RETURNS:
*       Number of bytes sent
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_SIZE (*SIM_OS_SOCKET_RECV_FUN)
(
    IN  GT_SOCKET_FD   socketFd,
    IN  GT_VOID *      buff,
    IN  GT_SIZE        buffLen,
    IN  GT_BOOL        removeFlag
);


/*******************************************************************************
* osSocketRecvFrom()
*
* DESCRIPTION:
*       Receive data from a UDP socket
*
* INPUTS:
*       socketFd    - Socket descriptor
*       buff        - Buffer to send
*       buffLen     - Max length to send
*       removeFlag  - Flag indicate if to remove or no remove the data from
*                     the socket buffer
*
* OUTPUTS:
*       sockAddr    - A pointer to a socket address structure, represents
*                     the source (target) address that has sent the packet.
*                     the caller can then compare the address later.
*                     If this is NULL, this information will not be provided.
*       sockAddrLen - A value-result parameter, initialized to the size of
*                     the allocated buffer sockAddr, and modified on return
*                     to indicate the actual size of the address stored there.
*
* RETURNS:
*       Number of bytes sent
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_SIZE (*SIM_OS_SOCKET_RECV_FROM_FUN)
(
    IN  GT_SOCKET_FD   socketFd ,
    IN  GT_VOID *      buff,
    IN  GT_SIZE        buffLen,
    IN  GT_BOOL        removeFlag,
    OUT GT_VOID *      sockAddr,
    OUT GT_SIZE *      sockAddrLen
);


/*******************************************************************************
* osSocketSetSocketNoLinger()
*
* DESCRIPTION:
*       Set the socket option to be no linger when closing connection.
*
* INPUTS:
*       socketFd    - Socket descriptor
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on fail
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*SIM_OS_SOCKET_SET_SOCKET_NO_LINGER_FUN)
(
    IN  GT_SOCKET_FD     socketFd
);

/*******************************************************************************
* osSocketExtractIpAddrFromSocketAddr()
*
* DESCRIPTION:
*       Extracts the IP address from sockaddr_in structure.
*
* INPUTS:
*       sockAddr    - pointer to sockaddr_in.
*       ipAddr      - pointer to address in GT_U32 format to be filled by the
*                     routine.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on fail
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*SIM_OS_SOCKET_EXTRACT_IP_ADDR_FROM_SOCKET_ADDR_FUN)
(
    IN  GT_VOID*   sockAddr,
    OUT GT_U32*    ipAddr
);

/*******************************************************************************
* osSocketGetSocketAddrSize()
*
* DESCRIPTION:
*       Returns the size of sockaddr_in.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       sockAddrSize - to be filled with sockaddr_in size.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on fail
*
* COMMENTS:
*       USERS THAT IMPLEMENTS THEIR OWN OS LAYER CAN RETURN SIZE = 0.
*
*******************************************************************************/
typedef GT_STATUS (*SIM_OS_SOCKET_GET_SOCKET_ADDR_SIZE_FUN)
(
    OUT GT_U32*    sockAddrSize
);



/*******************************************************************************
* osSocketShutDown ()
*
* DESCRIPTION:
*       This routine shuts down all, or part, of a connection-based socket.
*
* INPUTS:
*       socketFd    - Socket descriptor
*       operation   - 0 = receives disallowed
*                     1 = sends disallowed
*                     2 = sends and disallowed
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on fail
*
* COMMENTS:
*
*
*******************************************************************************/
typedef GT_STATUS (*SIM_OS_SOCKET_SHUT_DOWN_FUN)
(
    IN GT_SOCKET_FD   socketFd ,
    IN SOCKET_SHUTDOWN_OPERATION operation
);

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*******************************************************************************
* osSelectCreateSet()
*
* DESCRIPTION:
*       Create a set of file descriptors for the select function
*
* INPUTS:
*       none
*
* OUTPUTS:
*       none
*
* RETURNS:
*       Pointer to the set. If unable to create, returns null. Note that the
*       pointer is from void type.
*
* COMMENTS:
*
*******************************************************************************/
typedef GT_VOID * (*SIM_OS_SELECT_CREATE_SET_FUN)
(
    GT_VOID
);


/*******************************************************************************
* osSelectEraseSet()
*
* DESCRIPTION:
*       Erase (delete) a set of file descriptors
*
* INPUTS:
*       set - Pointer to the set.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_VOID (*SIM_OS_SELECT_ERASE_SET_FUN)
(
    IN GT_VOID *  set
);


/*******************************************************************************
* osSelectZeroSet()
*
* DESCRIPTION:
*       Zeros a set of file descriptors
*
* INPUTS:
*       set - Pointer to the set.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_VOID (*SIM_OS_SELECT_ZERO_SET_FUN)
(
    INOUT GT_VOID *  set
);


/*******************************************************************************
* osSelectAddFdToSet()
*
* DESCRIPTION:
*       Add a file descriptor to a specific set
*
* INPUTS:
*       set - Pointer to the set
*       fd  - A file descriptor
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_VOID (*SIM_OS_SELECT_ADD_FD_TO_SET_FUN)
(
    INOUT GT_VOID *  set ,
    IN GT_FD      fd
);


/*******************************************************************************
* osSelectClearFdFromSet()
*
* DESCRIPTION:
*       Remove (clear) a file descriptor from a specific set
*
* INPUTS:
*       set - Pointer to the set
*       fd  - A file descriptor
*
* OUTPUTS:
*       None
*
* RETURNS:
*       None
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_VOID (*SIM_OS_SELECT_CLEAR_FD_FROM_SET_FUN)
(
    INOUT GT_VOID *  set ,
    IN GT_FD      fd
);


/*******************************************************************************
* osSelectIsFdSet()
*
* DESCRIPTION:
*       Test if a specific file descriptor is set in a set
*
* INPUTS:
*       set - Pointer to the set
*       fd  - A file descriptor
*
* OUTPUTS:
*    None
*
* RETURNS:
*       GT_TRUE  (non zero) if set , returned as unsigned int
*       GT_FALSE (zero) if not set , returned as unsigned int
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_BOOL (*SIM_OS_SELECT_IS_FD_SET_FUN)
(
    INOUT GT_VOID *  set ,
    IN GT_FD      fd
);


/*******************************************************************************
* osSelectCopySet()
*
* DESCRIPTION:
*       Duplicate sets (require 2 pointers for sets)
*
* INPUTS:
*       srcSet - Pointer to source set
*       dstSet - Pointer to destination set
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Mone
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_VOID (*SIM_OS_SELECT_COPY_SET_FUN)
(
    IN GT_VOID *  srcSet ,
    OUT GT_VOID *  dstSet
);


/*******************************************************************************
* osSelect()
*
* DESCRIPTION:
*       OS Dependent select function
*
* INPUTS:
*       width       - The highest-numbered descriptor in any of the next three
*                     sets + 1 (if zero, The default length will be taken)
*       readSet     - Pointer to a read operation  descriptor set
*       writeSet    - Pointer to a write operation descriptor set
*       exceptionSet- Pointer to an exception descriptor set (not supported in
*                     all OS, such as VxWorks)
*       timeOut     - Maximum time to wait on in milliseconds. Sending a
*                     negative value will block indefinitely. Zero value cause
*                     no block
*
* OUTPUTS:
*       According to posix, all files descriptors will be zeroed , and only
*       bits that represents file descriptors which are ready will be set.
*
* RETURNS:
*       On success, returns the number of descriptors contained in the
*       descriptor sets,  which  may be zero if the timeout expires before
*       anything interesting happens. On  error, -1 is returned, and errno
*       is set appropriately; the sets and timeout become
*       undefined, so do not rely on their contents after an error.
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_SIZE (*SIM_OS_SELECT_FUN)
(
    IN GT_FD         width ,
    INOUT GT_VOID *  readSet ,
    INOUT GT_VOID *  writeSet ,
    INOUT GT_VOID *  exceptionSet ,
    IN GT_U32     timeOut
);

/*******************************************************************************
* osSocketGetSocketFdSetSize()
*
* DESCRIPTION:
*       Returns the size of sockaddr_in.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       sockAddrSize - to be filled with sockaddr_in size.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on fail
*
* COMMENTS:
*       USERS THAT IMPLEMENTS THEIR OWN OS LAYER CAN RETURN SIZE = 0.
*
*******************************************************************************/
typedef GT_STATUS (*SIM_OS_SOCKET_GET_SOCKET_FD_SET_SIZE_FUN)
(
    OUT GT_U32*    sockFdSize
);

/*******************************************************************************
* simOsSocketSetSocketNoDelay()
*
* DESCRIPTION:
*       Set the socket option to be no delay.
*
* INPUTS:
*       socketFd    - Socket descriptor
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on fail
*
* COMMENTS:
*       None
*
*******************************************************************************/
typedef GT_STATUS (*SIM_OS_SOCKET_SET_SOCKET_NO_DELAY_FUN)
(
    IN  GT_SOCKET_FD     socketFd
);




/* SIM_OS_FUNC_BIND_SOCKETS_STC -
*    structure that hold the "os sockets" functions needed be bound to SIM.
*
*/
typedef struct{
    SIM_OS_SOCKET_TCP_CREATE_FUN        osSocketTcpCreate;
    SIM_OS_SOCKET_UDP_CREATE_FUN        osSocketUdpCreate;
    SIM_OS_SOCKET_TCP_DESTROY_FUN       osSocketTcpDestroy;
    SIM_OS_SOCKET_UDP_DESTROY_FUN       osSocketUdpDestroy;
    SIM_OS_SOCKET_CREATE_ADDR_FUN       osSocketCreateAddr;
    SIM_OS_SOCKET_DESTROY_ADDR_FUN      osSocketDestroyAddr;
    SIM_OS_SOCKET_BIND_FUN              osSocketBind;
    SIM_OS_SOCKET_LISTEN_FUN            osSocketListen;
    SIM_OS_SOCKET_ACCEPT_FUN            osSocketAccept;
    SIM_OS_SOCKET_CONNECT_FUN           osSocketConnect;
    SIM_OS_SOCKET_SET_NON_BLOCK_FUN     osSocketSetNonBlock;
    SIM_OS_SOCKET_SET_BLOCK_FUN         osSocketSetBlock;
    SIM_OS_SOCKET_SEND_FUN              osSocketSend;
    SIM_OS_SOCKET_SEND_TO_FUN           osSocketSendTo;
    SIM_OS_SOCKET_RECV_FUN              osSocketRecv;
    SIM_OS_SOCKET_RECV_FROM_FUN         osSocketRecvFrom;
    SIM_OS_SOCKET_SET_SOCKET_NO_LINGER_FUN osSocketSetSocketNoLinger;
    SIM_OS_SOCKET_EXTRACT_IP_ADDR_FROM_SOCKET_ADDR_FUN osSocketExtractIpAddrFromSocketAddr;
    SIM_OS_SOCKET_GET_SOCKET_ADDR_SIZE_FUN osSocketGetSocketAddrSize;
    SIM_OS_SOCKET_SHUT_DOWN_FUN         osSocketShutDown;

    SIM_OS_SELECT_CREATE_SET_FUN        osSelectCreateSet;
    SIM_OS_SELECT_ERASE_SET_FUN         osSelectEraseSet;
    SIM_OS_SELECT_ZERO_SET_FUN          osSelectZeroSet;
    SIM_OS_SELECT_ADD_FD_TO_SET_FUN     osSelectAddFdToSet;
    SIM_OS_SELECT_CLEAR_FD_FROM_SET_FUN osSelectClearFdFromSet;
    SIM_OS_SELECT_IS_FD_SET_FUN         osSelectIsFdSet;
    SIM_OS_SELECT_COPY_SET_FUN          osSelectCopySet;
    SIM_OS_SELECT_FUN                   osSelect;
    SIM_OS_SOCKET_GET_SOCKET_FD_SET_SIZE_FUN osSocketGetSocketFdSetSize;

    SIM_OS_SOCKET_SET_SOCKET_NO_DELAY_FUN simOsSocketSetSocketNoDelay;
}SIM_OS_FUNC_BIND_SOCKETS_STC;





extern SIM_OS_SOCKET_TCP_CREATE_FUN        SIM_OS_MAC(osSocketTcpCreate);
extern SIM_OS_SOCKET_UDP_CREATE_FUN        SIM_OS_MAC(osSocketUdpCreate);
extern SIM_OS_SOCKET_TCP_DESTROY_FUN       SIM_OS_MAC(osSocketTcpDestroy);
extern SIM_OS_SOCKET_UDP_DESTROY_FUN       SIM_OS_MAC(osSocketUdpDestroy);
extern SIM_OS_SOCKET_CREATE_ADDR_FUN       SIM_OS_MAC(osSocketCreateAddr);
extern SIM_OS_SOCKET_DESTROY_ADDR_FUN      SIM_OS_MAC(osSocketDestroyAddr);
extern SIM_OS_SOCKET_BIND_FUN              SIM_OS_MAC(osSocketBind);
extern SIM_OS_SOCKET_LISTEN_FUN            SIM_OS_MAC(osSocketListen);
extern SIM_OS_SOCKET_ACCEPT_FUN            SIM_OS_MAC(osSocketAccept);
extern SIM_OS_SOCKET_CONNECT_FUN           SIM_OS_MAC(osSocketConnect);
extern SIM_OS_SOCKET_SET_NON_BLOCK_FUN     SIM_OS_MAC(osSocketSetNonBlock);
extern SIM_OS_SOCKET_SET_BLOCK_FUN         SIM_OS_MAC(osSocketSetBlock);
extern SIM_OS_SOCKET_SEND_FUN              SIM_OS_MAC(osSocketSend);
extern SIM_OS_SOCKET_SEND_TO_FUN           SIM_OS_MAC(osSocketSendTo);
extern SIM_OS_SOCKET_RECV_FUN              SIM_OS_MAC(osSocketRecv);
extern SIM_OS_SOCKET_RECV_FROM_FUN         SIM_OS_MAC(osSocketRecvFrom);
extern SIM_OS_SOCKET_SET_SOCKET_NO_LINGER_FUN SIM_OS_MAC(osSocketSetSocketNoLinger);
extern SIM_OS_SOCKET_EXTRACT_IP_ADDR_FROM_SOCKET_ADDR_FUN SIM_OS_MAC(osSocketExtractIpAddrFromSocketAddr);
extern SIM_OS_SOCKET_GET_SOCKET_ADDR_SIZE_FUN SIM_OS_MAC(osSocketGetSocketAddrSize);
extern SIM_OS_SOCKET_SHUT_DOWN_FUN         SIM_OS_MAC(osSocketShutDown);

extern SIM_OS_SELECT_CREATE_SET_FUN        SIM_OS_MAC(osSelectCreateSet);
extern SIM_OS_SELECT_ERASE_SET_FUN         SIM_OS_MAC(osSelectEraseSet);
extern SIM_OS_SELECT_ZERO_SET_FUN          SIM_OS_MAC(osSelectZeroSet);
extern SIM_OS_SELECT_ADD_FD_TO_SET_FUN     SIM_OS_MAC(osSelectAddFdToSet);
extern SIM_OS_SELECT_CLEAR_FD_FROM_SET_FUN SIM_OS_MAC(osSelectClearFdFromSet);
extern SIM_OS_SELECT_IS_FD_SET_FUN         SIM_OS_MAC(osSelectIsFdSet);
extern SIM_OS_SELECT_COPY_SET_FUN          SIM_OS_MAC(osSelectCopySet);
extern SIM_OS_SELECT_FUN                   SIM_OS_MAC(osSelect);
extern SIM_OS_SOCKET_GET_SOCKET_FD_SET_SIZE_FUN SIM_OS_MAC(osSocketGetSocketFdSetSize);

extern SIM_OS_SOCKET_SET_SOCKET_NO_DELAY_FUN SIM_OS_MAC(simOsSocketSetSocketNoDelay);


#endif   /* __simOsBindSocketsh */



