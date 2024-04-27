/*******************************************************************************
*              (c), Copyright 2008, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* slanLib.c
*
* DESCRIPTION:
*       SLAN library for UNIX-like OSes
*
* DEPENDENCIES:
*
* FILE REVISION NUMBER:
*       $Revision 2 $
*
*******************************************************************************/
#define _GNU_SOURCE
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>


#include "slanLib.h"

/************ Defines  ********************************************************/
#define SLAN slans[slanId-1]
#define CHECK_SLAN(err) \
    if (slans == NULL) \
    { \
                fprintf(stderr, "slanLib not initialized\n"); \
        return err; \
    } \
    if (slanId <= 0 || slanId > slansMax) \
        return err; \
    if (SLAN.state == SLAN_STATE_NULL || SLAN.state == SLAN_STATE_CONNECTING) \
        return err;

/************* Typedefs *******************************************************/


typedef enum {
    SLAN_STATE_NULL = 0,
    SLAN_STATE_CONNECTING,
    SLAN_STATE_OPENED,
    SLAN_STATE_CLOSED,
    SLAN_STATE_RESET
} SLAN_STATE_ENUM;
/*
 * Typedef: struct slanSTC
 *
 * Description:
 *      This structure represent a single SLAN connection
 *
 * Fields:
 *      state        - current slan state
 *      fd           - socket file descriptor
 *      handler      - event handler pointer passed to bind
 *      userData     - a custom pointer passed to bind
 *      buffer       - buffer for input
 *      bufferLength - number of bytes already in buffer
 */
typedef struct {
    SLAN_STATE_ENUM                 state;
    int                             fd;
    char                            *name;
    SLAN_LIB_EVENT_HANDLER_FUNC     handler;
    void*                           userData;
    char                            buffer[SLAN_INPUT_BUFFER_SIZE];
    int                             bufferLength;
} slanSTC;


/************* Global data ****************************************************/

/* Maxumum number of SLANs */
static int slansMax = 0;
/* SLAN array */
static slanSTC* slans = NULL;
/* SLAN client name: asic, smbsim, ether, etc */
static char *slanClientName = NULL;
/* signal pipe required to awake slanLibMainLoop() */
static int sigPipe[2];

static pthread_mutex_t slanMtx = PTHREAD_MUTEX_INITIALIZER;

/************* Functions ******************************************************/
static char *s_strdup(const char *s)
{
#ifdef SHARED_MEMORY
/* Don't call strdup because it allocates memory with LIBC internal malloc() */
    char *dest;
    size_t len = strlen(s);
    dest = malloc(len+1);
    if (!dest)
        return NULL;
    strcpy(dest,s);
    return dest;
#else
    return strdup(s);
#endif
}

/*******************************************************************************
* slanLibInit
*
* DESCRIPTION:
*       Initialize library
*
* INPUTS:
*       slansMax    - Maximum number of SLANS
*       clientName  - Client name for debug tricks in slanConnector
*                     Can be NULL. The values will be
*                     "asic", "smbsim", "ethernet", etc
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if success
*       Non zero if error occured:
*           EALREADY
*           EINVAL
*           ENOMEM
*
* COMMENTS:
*
*******************************************************************************/
int slanLibInit
(
    IN  int slansMaxParam,
    IN  const char *clientName
)
{
    if (slans != NULL)
        return EALREADY;

    if (slansMaxParam < 1)
        return EINVAL;


    pthread_mutex_lock(&slanMtx);

    slans = (slanSTC*)calloc(slansMaxParam, sizeof(slanSTC));
    if (slans == NULL)
    {
        perror("Can't allocate memory for SLANs");
        pthread_mutex_unlock(&slanMtx);
        return ENOMEM;
    }

    slansMax = slansMaxParam;

    if (socketpair(PF_LOCAL, SOCK_STREAM, 0, sigPipe) < 0)
    {
        int er = errno;
        perror("Can't create socketpair");
        free(slans);
        slans = NULL;
        slansMax = 0;
        pthread_mutex_unlock(&slanMtx);
        return er;
    }

    slanClientName = s_strdup(clientName ? clientName : "");

    pthread_mutex_unlock(&slanMtx);

    return 0;
}

/*******************************************************************************
* slanLibMainLoop
*
* DESCRIPTION:
*       Mail loop function. slanLibInit() doesn't create thread because
*       thread can require to be created by specific API
*       Must be called after slanLinInit()
*
* INPUTS:
*       timeoutSec  - return if no input more than timeoutSec
*                     Zero means wait forewer
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0   if timeout
*       Non zero if error
*
* COMMENTS:
*
*******************************************************************************/
int slanLibMainLoop(int timeoutSec)
{
    fd_set  rfds;
    int     r;
    int     maxFd;
    SLAN_ID slanId;

    if (slans == NULL) /* not initialized */
        return -1;

    while (1)
    {
        struct timeval tv;
        FD_ZERO(&rfds);
        FD_SET(sigPipe[0], &rfds);

        maxFd = sigPipe[0];
        for (slanId = 1; slanId <= slansMax; slanId++)
        {
            if (SLAN.state == SLAN_STATE_OPENED)
            {
                FD_SET(SLAN.fd, &rfds);
                if (maxFd < SLAN.fd)
                    maxFd = SLAN.fd;
            }
        }

        if (timeoutSec)
        {
            tv.tv_sec = timeoutSec;
            tv.tv_usec = 0;
        }
        r = select(maxFd+1, &rfds, (fd_set*)NULL, (fd_set*)NULL, timeoutSec?&tv:(struct timeval*)NULL);
        if (r < 0)
        {
            if (errno == EINTR)
                continue;

            fprintf(stderr,"ERROR : slanLibMainLoop breaks [%s][%d] \n",__FILE__,__LINE__);
            break;
        }
        if (r == 0)
        {
            fprintf(stderr,"ERROR : slanLibMainLoop exit [%s][%d] \n",__FILE__,__LINE__);
            return 0;
        }
        if (FD_ISSET(sigPipe[0], &rfds))
        {
            char ch;
            r = read(sigPipe[0], &ch, 1);
            if (r <= 0)
            {
                /* critical error, will never happen */
                perror("failed to read from signal pipe");
            }
        }
        for (slanId = 1; slanId <= slansMax; slanId++)
        {
            if (SLAN.state != SLAN_STATE_OPENED)
                continue;

            if (!FD_ISSET(SLAN.fd, &rfds))
                continue;

            r = recv(SLAN.fd, SLAN.buffer + SLAN.bufferLength,
                    sizeof(SLAN.buffer) - SLAN.bufferLength, 0);
            if (r < 0)
            {
                /* handle error */
                continue;
            }
            if (r == 0)
            {
                SLAN.state = SLAN_STATE_CLOSED;
                if (SLAN.handler != NULL)
                    SLAN.handler(slanId, SLAN.userData,
                            SLAN_LIB_EV_CLOSED, NULL, 0);
                continue;
            }
            SLAN.bufferLength += r;
            r = 0;
#define BYTES_LEFT  (SLAN.bufferLength - r)
#define CURR_PTR    (SLAN.buffer + r)
            while (BYTES_LEFT >= SLAN_PKT_HDR_SIZE)
            {
                int dataSize;
                SLAN_LIB_EVENT_TYPE evType;
                char *dataPtr;

                SLAN_LIB_UNPACK_HEADER(CURR_PTR, dataSize, evType);
                if (dataSize < SLAN_PKT_HDR_SIZE || dataSize > SLAN_INPUT_BUFFER_SIZE)
                {
                    /* protocol error */
                    SLAN.state = SLAN_STATE_RESET;
                    if (SLAN.handler != NULL)
                        SLAN.handler(slanId, SLAN.userData,
                                SLAN_LIB_EV_CLOSED, NULL, 0);
                    r = SLAN.bufferLength;
                    close(SLAN.fd);
                    break;
                }
                if (dataSize > BYTES_LEFT)
                    break;

                /* got packet */
                dataPtr = CURR_PTR;
                r += dataSize;

                /* handle packet */
                if (SLAN.handler != NULL)
                    SLAN.handler(slanId, SLAN.userData,
                            evType, dataPtr + SLAN_PKT_HDR_SIZE,
                            dataSize - SLAN_PKT_HDR_SIZE);
            }
            if (BYTES_LEFT)
                memcpy(SLAN.buffer, CURR_PTR, BYTES_LEFT);
            SLAN.bufferLength = BYTES_LEFT;
#undef BYTES_LEFT
#undef CURR_PTR
        }
    }

    fprintf(stderr,"ERROR : slanLibMainLoop returns [%s][%d] \n",__FILE__,__LINE__);
    return -1;
}

/*******************************************************************************
* slanLibBind
*
* DESCRIPTION:
*       This function connects to SLAN Connector and send slanName identifier.
*       Connection is added to slanLibMainLoop() pool
*
* INPUTS:
*       slanName    - SLAN name
*       handler     - event handler function
*       userData    - pointer to user data
*
* OUTPUTS:
*       slanId      - SLAN id
*
* RETURNS:
*       Zero if success
*       Non zero if error occured
*
* COMMENTS:
*       $SLAN_PREFIX environment variable will be added to slanName
*
*******************************************************************************/
int slanLibBind
(
    IN  const char*                     slanName,
    IN  SLAN_LIB_EVENT_HANDLER_FUNC     handler,
    IN  void*                           userData,
    OUT SLAN_ID                         *slanIdPtr
)
{
        struct addrinfo        hints;
        struct addrinfo        *ai;
        int        er, k;
    int dataSize;
    SLAN_LIB_EVENT_TYPE evType;
    char slanNameU[SLAN_NAME_LEN];
    char *e;
    int retry_write_max = 1000;/* support busy sockets */
    int retry_write = retry_write_max;/* support busy sockets */
    int uSleepTime = 20*1000;/* x millisec*/
    int allowPrintError = 0;
    SLAN_ID slanId;

    /* different sleep between threads */
    /*uSleepTime *= 1 + ((pthread_self() + getpid()) % 7);*/

    if (slans == NULL)
    {
                fprintf(stderr, "slanLib not initialized\n");
        return -1;
    }

    slanNameU[0] = 0;
    if ((e = getenv("SLAN_PREFIX")) != NULL)
    {
        if (strlen(e)+1 >= sizeof(slanNameU))
        {
            fprintf(stderr, "slan prefix '%s' too long\n", e);
            return -1;
        }
        strcpy(slanNameU, e);
        strcat(slanNameU, "_");
    }
    if (strlen(slanNameU) + strlen(slanName) >= sizeof(slanNameU))
    {
        fprintf(stderr, "slan name '%s%s' too long\n",slanNameU, slanName);
        return -1;
    }
    strcat(slanNameU, slanName);

    pthread_mutex_lock(&slanMtx);
    for (slanId = 1; slanId <= slansMax; slanId++)
        if (SLAN.state == SLAN_STATE_NULL)
            break;
    if (slanId > slansMax)
    {
        pthread_mutex_unlock(&slanMtx);
                fprintf(stderr, "Too many SLANs\n");
        return -1;
    }
    SLAN.state = SLAN_STATE_CONNECTING;
    pthread_mutex_unlock(&slanMtx);

retry_write_lbl:

    /* connect to SLAN Connector */
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;/* AF_UNSPEC */
        hints.ai_socktype = SOCK_STREAM;

        er = getaddrinfo(SLAN_CONNECTOR_ADDRESS, SLAN_CONNECTOR_PORT, &hints, &ai);
        if (er) {
                fprintf(stderr, "can't getaddrinfo(): %s\n", gai_strerror(er));
        SLAN.state = SLAN_STATE_NULL;
                return -1;
        }

        SLAN.fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);

        if (SLAN.fd < 0) {
                perror("can't socket()");
                freeaddrinfo(ai);
        SLAN.state = SLAN_STATE_NULL;
                return -1;
        }
        if (connect(SLAN.fd,  ai->ai_addr,ai->ai_addrlen) < 0) {
                perror("Can't connect() to slanConnector");
                freeaddrinfo(ai);
                close(SLAN.fd);
        SLAN.state = SLAN_STATE_NULL;
                return -1;
        }
        freeaddrinfo(ai);

    if(retry_write_max != retry_write &&
       (0==(retry_write%250)))
    {
        allowPrintError = 1;
    }
    else
    {
        allowPrintError = 0;
    }

    if(allowPrintError)
    {
        fprintf(stderr,"slanLibBind: retry_write = %d \n",retry_write_max - retry_write);
    }

    /* send identification packet */
    dataSize = sprintf(SLAN.buffer+SLAN_PKT_HDR_SIZE, "%s%c%s",
            slanNameU, 0, slanClientName);
    dataSize += 1 + SLAN_PKT_HDR_SIZE;
    SLAN_LIB_PACK_HEADER(SLAN.buffer,dataSize,SLAN_LIB_EV_HANDSHAKE);

    {
        int writeValue = write(SLAN.fd, SLAN.buffer, dataSize);
        if(writeValue == 0 || writeValue == -1)
        {
            if(allowPrintError)
            {
                fprintf(stderr, "slanLibBind : write: got error [%d] bytes ... retry ... \n" , writeValue);
            }
            retry_write--;
            usleep(uSleepTime);
            close(SLAN.fd);

            if(retry_write == 0)
            {
                goto bad_ack;
            }

            goto retry_write_lbl;
        }
        if (writeValue < 0)
        {
            fprintf(stderr, "slanLibBind : write() : %s\n", strerror(errno));
        }
    }

    /* wait for ack */
    k = read(SLAN.fd, SLAN.buffer, SLAN_PKT_HDR_SIZE);
    if(k == 0 || k == -1)
    {
        if(allowPrintError)
        {
            fprintf(stderr, "slanLibBind : read: got error [%d] bytes ... retry ... \n" , k);
        }

        retry_write--;
        usleep(uSleepTime);
        close(SLAN.fd);

        if(retry_write == 0)
        {
            goto bad_ack;
        }

        goto retry_write_lbl;
    }
    else
    if (k < SLAN_PKT_HDR_SIZE)
    {
        /* failed to read ack */
        fprintf(stderr, "slanLibBind : Failed to connect: no handshake ack received (got only %d bytes)\n" , k);

bad_ack:
        fprintf(stderr,"slanLibBind : Error : slan [%s] bind failed [%s][%d]\n",
            slanNameU,__FILE__,__LINE__);


        close(SLAN.fd);
        SLAN.state = SLAN_STATE_NULL;
        return -1;
    }
    SLAN_LIB_UNPACK_HEADER(SLAN.buffer, dataSize, evType);
    if (dataSize < SLAN_PKT_HDR_SIZE || dataSize >= SLAN_INPUT_BUFFER_SIZE)
    {
        fprintf(stderr,"slanLibBind : Error : slan bind case 1 [%s][%d]\n",__FILE__,__LINE__);

        goto bad_ack;
    }
    if (dataSize > SLAN_PKT_HDR_SIZE)
    {
        k = read(SLAN.fd, SLAN.buffer + SLAN_PKT_HDR_SIZE, dataSize - SLAN_PKT_HDR_SIZE);
        if (k < dataSize - SLAN_PKT_HDR_SIZE)
        if (dataSize < SLAN_PKT_HDR_SIZE || dataSize >= SLAN_INPUT_BUFFER_SIZE)
        {
            fprintf(stderr,"slanLibBind : Error : slan bind case 2 [%s][%d]\n",__FILE__,__LINE__);

            goto bad_ack;
        }
    }
    /* got ack */
    switch (evType) {
        case SLAN_LIB_EV_HANDSHAKE_SUCCESS:
            break;
        case SLAN_LIB_EV_HANDSHAKE_FAILED:
            /* connection failed, see reason */
            SLAN.buffer[dataSize] = 0;
            fprintf(stderr, "slanLibBind : Failed to connect: %s\n", SLAN.buffer + SLAN_PKT_HDR_SIZE);
            goto bad_ack;
        default:
            /* bad answer */
            fprintf(stderr, "slanLibBind : Failed to connect: bad message type\n");
            goto bad_ack;
    }

    /* connected */
    SLAN.name = s_strdup(slanNameU);
    SLAN.handler = handler;
    SLAN.userData = userData;
    SLAN.bufferLength = 0;
    SLAN.state = SLAN_STATE_OPENED;
    *slanIdPtr = slanId;

    /* signal to mainLoop thread */
    if (write(sigPipe[1],"1",1) < 0)
    {
        fprintf(stderr, "slanLibBind : write() : %s\n", strerror(errno));
    }

    /*printf("slan bind with actual name [%s] \n",slanNameU);*/

    return 0;
}

/*******************************************************************************
* slanLibUnbind
*
* DESCRIPTION:
*       Remove connection from slanLibMainLoop() pool.
*       Then connection to SLAN Connector closed
*
* INPUTS:
*       slanId      - SLAN id
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Zero if success
*       Non zero if error occured
*
* COMMENTS:
*
*******************************************************************************/
int slanLibUnbind
(
    IN  SLAN_ID     slanId
)
{
    CHECK_SLAN(-1);

    pthread_mutex_lock(&slanMtx);
    if (SLAN.state != SLAN_STATE_RESET)
        close(SLAN.fd);
    SLAN.state = SLAN_STATE_NULL;
    if (SLAN.name)
        free(SLAN.name);
    SLAN.name = NULL;

    /* signal to mainLoop thread */
    if (write(sigPipe[1],"1",1) < 0)
    {
        fprintf(stderr, "write() : %s\n", strerror(errno));
    }

    pthread_mutex_unlock(&slanMtx);

    return 0;
}


/*******************************************************************************
* slanLibGetUserData
*
* DESCRIPTION:
*       Get custom pointer passed to bind function
*
* INPUTS:
*       slanId      - SLAN id
*
* OUTPUTS:
*       None
*
* RETURNS:
*       User data
*       NULL if error occured
*
* COMMENTS:
*
*******************************************************************************/
void* slanLibGetUserData
(
    IN  SLAN_ID     slanId
)
{
    CHECK_SLAN(NULL);

    return SLAN.userData;
}

/*******************************************************************************
* slanLibGetSlanName
*
* DESCRIPTION:
*       Get slan name
*
* INPUTS:
*       slanId      - SLAN id
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Slan name
*       NULL if error occured
*
* COMMENTS:
*
*******************************************************************************/
const char* slanLibGetSlanName
(
    IN  SLAN_ID     slanId
)
{
    CHECK_SLAN(NULL);

    return SLAN.userData;
}

/*******************************************************************************
* slanLibTransmit
*
* DESCRIPTION:
*       Transmit packet to SLAN
*
* INPUTS:
*       slanId      - SLAN id
*       packetData  - pointer to packet data
*       packetLen   - pointer to packet length
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Number or bytes sent
*       < 0 if error occured
*
* COMMENTS:
*
*******************************************************************************/
int slanLibTransmit
(
    IN  SLAN_ID     slanId,
    IN  const char  *packetData,
    IN  int         packetLen
)
{
    char hdr[SLAN_PKT_HDR_SIZE];
    int  len;

    /* cut a packet if it exceeds input buffer size to avoid invinite loops */
    if (packetLen + SLAN_PKT_HDR_SIZE > SLAN_INPUT_BUFFER_SIZE)
        packetLen = SLAN_INPUT_BUFFER_SIZE - SLAN_PKT_HDR_SIZE;

    SLAN_LIB_PACK_HEADER(hdr,packetLen + SLAN_PKT_HDR_SIZE,SLAN_LIB_EV_PACKET);

    CHECK_SLAN(-1);

    pthread_mutex_lock(&slanMtx);
    if (write(SLAN.fd, hdr, SLAN_PKT_HDR_SIZE) < 0)
    {
        fprintf(stderr, "write() : %s\n", strerror(errno));
    }

    len = write(SLAN.fd, packetData, packetLen);
    if (len < 0)
    {
        fprintf(stderr, "write() : %s\n", strerror(errno));
    }

    pthread_mutex_unlock(&slanMtx);

    return len;
}

/*******************************************************************************
* slanLibSetLinkStatus
*
* DESCRIPTION:
*       Change SLANs link status
*
* INPUTS:
*       slanId      - SLAN id
*       linkStatus  - new link status
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Number or bytes sent
*       < 0 if error occured
*
* COMMENTS:
*
*******************************************************************************/
int slanLibSetLinkStatus
(
    IN  SLAN_ID                     slanId,
    IN  SLAN_LIB_LINK_STATUS_TYPE   linkStatus
)
{
    char hdr[SLAN_PKT_HDR_SIZE];
    SLAN_LIB_EVENT_TYPE ev;

    switch (linkStatus)
    {
        case SLAN_LIB_LINK_UP:
            ev = SLAN_LIB_EV_LINKUP;
            break;
        case SLAN_LIB_LINK_DOWN:
            ev = SLAN_LIB_EV_LINKDOWN;
            break;
        default:
            return -1;
    }
    SLAN_LIB_PACK_HEADER(hdr,SLAN_PKT_HDR_SIZE,ev);

    CHECK_SLAN(-1);
    pthread_mutex_lock(&slanMtx);
    if (write(SLAN.fd, hdr, SLAN_PKT_HDR_SIZE) < 0)
    {
        fprintf(stderr, "write() : %s\n", strerror(errno));
    }
    pthread_mutex_unlock(&slanMtx);
    return 0;
}



#ifdef  TEST

void* rx_thread(void* unused)
{
    slanLibMainLoop();
    return NULL;
}

void ev_handler_func
(
    IN  SLAN_ID             slanId,
    IN  void*               userData,
    IN  SLAN_LIB_EVENT_TYPE eventType,
    IN  char*               pktData,
    IN  int                 pktLen
)
{
    printf("Got event for slan %d, userData=%p, evType=%d pktLen=%d\n",
            slanId, userData, eventType, pktLen);
}

int main()
{
    pthread_t rxthreadId;
    SLAN_ID slanA;
    SLAN_ID slanB;

    slanLibInit(10,"selftest");

    pthread_create(&rxthreadId, NULL, rx_thread, NULL);

    slanLibBind("slan00", ev_handler_func, (void*)0xa, &slanA);
    slanLibBind("slan00", ev_handler_func, (void*)0xb, &slanB);

    slanLibTransmit(slanA, "12345", 5);

    sleep(4);
    printf("unbinding...\n");
    slanLibUnbind(slanB);

    while (1)
        sleep(10);

    return 0;
}
#endif /* TEST */
