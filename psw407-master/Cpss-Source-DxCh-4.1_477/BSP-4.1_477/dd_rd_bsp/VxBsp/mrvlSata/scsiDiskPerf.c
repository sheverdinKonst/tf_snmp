#define INCLUDE_SCSI

#define INCLUDE_SCSI2

#include <vxWorks.h>        /* our OS */
#include <taskLib.h>        /* task control */
#include <stdio.h>          /* printf */
#include <sysLib.h>         /* sysClkRateGet */
#include <timers.h>         /* timers */
#include <logLib.h>         /* logMsg */
#include <tickLib.h>
#include <rawFsLib.h>
#include <cacheLib.h>
#include <errnoLib.h>
#include <string.h>
#include <scsiLib.h>
#include <dosFsLib.h>
#include <scsiLib.h>


#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 2) 
#include <xbdBlkDev.h>
#endif
#include "mvCommon.h"




#define DEFAULT_BUFFER_LEN      (64*1024)
#define DEFAULT_TEST_DURATION   (20)
#define DEFAULT_OFFSET          (0)

int forever=0;
char *rawDefName="raw0:";

int scsiDiskPerf(int bufferLen, int ttg, int iswrite,
			 SCSI_PHYS_DEV *	pPhysDev, char* pRawName, int DiskOffset)

{
    RAW_VOL_DESC *volDesc = NULL;
	BLK_DEV * pBlk = NULL;
    int startTick = 0, stopTick = 0;
    int fd = ERROR; 
    unsigned int totalBytes = 0, bytesPerSecond;
    char* buffer = NULL;
    int retval = ERROR, res;
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 2) 
	device_t	xbdDevice;
#endif

	if ((int)pRawName == 0)
	{
		pRawName = rawDefName;
	}
	if ((int)pPhysDev == 0)
	{
        logMsg("pPhysDev is NULL\n", 1, 2, 3, 4, 5, 6);
        pBlk = NULL;
        goto finish;
	}
    if (NULL == pBlk)
    {
        rawFsInit(4);
        pBlk = scsiBlkDevCreate (pPhysDev, pPhysDev->numBlocks, 0);
		if (NULL == pBlk)
        {
            printf("scsiPhysDevCreate() failed\n");
            goto finish;
        }
    }
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 2) 
	xbdDevice = xbdBlkDevCreate (pBlk, pRawName); 
	printf("xbdBlkDevCreate(0x%X,%s) return xbdDevice=0x%x\n",pBlk, pRawName, xbdDevice);
    volDesc = rawFsDevInit(pRawName, xbdDevice);
#else
    volDesc = rawFsDevInit(pRawName, pBlk);
#endif
    if (NULL == volDesc)
    {
        int _errno = errnoOfTaskGet(taskIdSelf());
        if (_errno != 0xD0006)
        {       
            printf("rawFsDevInit() failed with 0x%X\n", 
                    errnoOfTaskGet( taskIdSelf() ));
            goto finish;
        }
    }

    if (0 == bufferLen) 
    {
        bufferLen = DEFAULT_BUFFER_LEN;
    }
    buffer = cacheDmaMalloc(bufferLen);
    if (!buffer)
    {
        printf("cacheDmaMalloc() failed.\n");  

        goto finish;
    }        printf("buffer: %p\n", buffer);

    memset(buffer, 0x5A, bufferLen);
    if (0 == ttg) 
    {
        ttg = DEFAULT_TEST_DURATION;
    }
    ttg *= sysClkRateGet(); 
    
    fd = open(pRawName, O_RDWR, 0666);

    res = ioctl (fd, FIOSEEK, DiskOffset);
    startTick = tickGet();
	do  
	{
		if (iswrite)
		{
			res = write(fd, buffer, bufferLen);
			if (res < 0)
			{
				printf("write() failed errno=0x%X\n", 
						errnoOfTaskGet( taskIdSelf() ) );  
				goto finish;
			}           
		}
		else
		{
			res = read(fd, buffer, bufferLen);
			if (res < 0)
			{
				printf("read() failed with errno=0x%X\n", 
						errnoOfTaskGet(taskIdSelf()));  
				goto finish;
			}
		}       
		stopTick = tickGet();
		totalBytes += (unsigned int)res;
		ioctl (fd, FIOSEEK, DiskOffset);
	} while (abs(stopTick - startTick) < ttg);

    bytesPerSecond = totalBytes/(abs(stopTick - startTick)/sysClkRateGet());    

    printf("%s transfer rate is %d MByte per second\n", 
                            (char*)((iswrite)? "Write" : "Read"),
                            (bytesPerSecond)/1024/1024);
    retval = OK;
finish:
    if (buffer)
    {
        cacheDmaFree(buffer);
    }    printf("close file\n");

    if (fd != ERROR)
    {   
        close(fd);
    }
    if (volDesc)
    {
        rawFsVolUnmount(volDesc);
    }    
    return retval;
}

#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 2) 
#else
void doForever(int bufferLen, int ttg, int iswrite,SCSI_PHYS_DEV *	pPhysDev,char* pRawName)
{
	printf("doForever is Started \n");

	while(forever)
	{
		if (scsiDiskPerf(bufferLen,ttg,iswrite,pPhysDev,pRawName,0) != OK) break;
	}

	printf("doForever is Ended \n");
}


#endif


int 	scsiSp (FUNCPTR func, int x1,int x2,int x3,int x4,int x5,int x6,int x7,int x8,int x9)
{
	return taskSpawn("tDiskPerf", SCSI_DEF_TASK_PRIORITY, 0, 8*4000,
					func,  x1, x2, x3, x4, x5,
					x6, x7, x8, x9,0);
	
}




#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 2) 

/*-------------------------------------------------*/
int scsiDiskPerfInit(SCSI_PHYS_DEV *	pPhysDev, char* pRawName)

{
	device_t	xbdDevice;

	BLK_DEV * pBlk = NULL;

	if ((int)pRawName == 0)
	{
		pRawName = rawDefName;
	}
	if ((int)pPhysDev == 0)
	{
        logMsg("pPhysDev is NULL\n", 1, 2, 3, 4, 5, 6);
        return 1;
	}
    rawFsInit(5);
    pBlk = scsiBlkDevCreate (pPhysDev, pPhysDev->numBlocks, 0);
	if (NULL == pBlk)
    {
        printf("scsiPhysDevCreate() failed\n");
        return 2;
    }

	xbdDevice = xbdBlkDevCreate (pBlk, pRawName); 
	printf("xbdBlkDevCreate(0x%X,%s) return xbdDevice=0x%x\n",pBlk, pRawName, xbdDevice);
    return 0;
}
/*--------------------------------------------------*/
int scsiDiskPerf2(int 		bufferLen, 
				  int 		ttg, 
				  int 		iswrite, 
				  int 		DiskOffset, 
				  device_t	xbdDevice, 
				  char    	*pRawName)
{
    int startTick = 0, stopTick = 0;
    int fd = ERROR; 
    unsigned int totalBytes = 0, bytesPerSecond;
    char* buffer = NULL;
    int retval = ERROR, res;
	RAW_VOL_DESC *volDesc = NULL;

	if (0  == xbdDevice)
	{
		printf("Error xbdDevice is NULL \n");
		return 1;
	}
	if ((int)pRawName == 0)
	{
		printf("Error disk name is NULL \n");
		return 1;
		
	}
    volDesc = rawFsDevInit(pRawName, xbdDevice);
    if (NULL == volDesc)
    {
        int _errno = errnoOfTaskGet(taskIdSelf());
        if (_errno != 0xD0006)
        {       
            printf("rawFsDevInit() failed with 0x%X\n", 
                    errnoOfTaskGet( taskIdSelf() ));
            return 2;
        }
    }

    if (0 == bufferLen) 
    {
        bufferLen = DEFAULT_BUFFER_LEN;
    }
    buffer = cacheDmaMalloc(bufferLen);
    if (!buffer)
    {
        printf("cacheDmaMalloc() failed.\n");  
        return 3;
    }
	printf("buffer: %p\n", buffer);

    memset(buffer, 0x5A, bufferLen);
    if (0 == ttg) 
    {
        ttg = DEFAULT_TEST_DURATION;
    }
    ttg *= sysClkRateGet(); 
    
    fd = open(pRawName, O_RDWR, 0666);

    res = ioctl (fd, FIOSEEK, DiskOffset);
    startTick = tickGet();
    retval = OK;
	do  
	{
		if (iswrite)
		{
			res = write(fd, buffer, bufferLen);
			if (res < 0)
			{
				printf("write() failed errno=0x%X\n", 
						errnoOfTaskGet( taskIdSelf() ) );  
				retval = 5;
				break;
			}           
		}
		else
		{
			res = read(fd, buffer, bufferLen);
			if (res < 0)
			{
				printf("read() failed with errno=0x%X\n", 
						errnoOfTaskGet(taskIdSelf()));  
				retval = 6;
				break;
			}
		}       
		stopTick = tickGet();
		totalBytes += (unsigned int)res;
		ioctl (fd, FIOSEEK, DiskOffset);
	} while (abs(stopTick - startTick) < ttg);

    if (OK == retval)
	{
		printf("totalBytes = %d, startTick=%d, stopTick=%d, sysClkRateGet=%d\n", 
			   totalBytes, startTick, stopTick,  sysClkRateGet());
		bytesPerSecond = totalBytes/(abs(stopTick - startTick)/sysClkRateGet());    
		printf("%s transfer rate is %d MByte per second\n", 
								(char*)((iswrite)? "Write" : "Read"),
								(bytesPerSecond)/_1M);
	}
    if (buffer)
    {
        cacheDmaFree(buffer);
    }    
	printf("close file\n");
    if (fd != ERROR)
    {   
        close(fd);
    }
    if (volDesc)
    {
        rawFsVolUnmount(volDesc);
    }
    return retval;
}


void doForever(int 		bufferLen, 
			   int 		ttg, 
			   int 		iswrite, 
			   int      DiskOffset,
			   device_t xbdDevice,
			   char		*pRawName)
{
	printf("doForever is Started \n");

	while(forever)
	{
		if (scsiDiskPerf2(bufferLen,
						  ttg,
						  iswrite, 
						  DiskOffset,
						  xbdDevice,
						  pRawName) != OK) 
			break;
	}

	printf("doForever is Ended \n");
}


#endif /* (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 2)  */


