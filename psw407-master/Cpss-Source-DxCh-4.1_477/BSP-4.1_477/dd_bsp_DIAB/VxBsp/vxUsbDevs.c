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

#include "mvOs.h"
#include "mvDebug.h"
#include "BoardEnv/mvBoardEnvLib.h"
#include "gpp/mvGpp.h"
#include "vxGppIntCtrl.h"
#include "usb/mvUsb.h"
#include "boardEnv/mvBoardEnvSpec.h"
#include "ctrlenv/mvCtrlEnvSpec.h"
#include "usb/api/mvUsbDevApi.h"
#include "usb/examples/disk.h"
#include "usb/examples/mouse.h"

#include "config.h"
#include "intLib.h"
#include "logLib.h"
#include "wdLib.h"
#include "sysLib.h"

#define USB_DISK_DEFAULT_SIZE   2048
#define USB_TASK_PRIO           10
#define MV_BOARD_MAX_USB		MV_USB_MAX_PORTS

#if ((MV_USB_VERSION >= 1) || defined(USB_UNDERRUN_WA))
static int streaming = 1;  
#else
static int streaming = 0;  
#endif

extern MV_BOOL	mvUsbCtrlIsHost[];

#if defined(USB_UNDERRUN_WA)

int     usbWaIdma = 3;
int     usbWaSramDescr = 0;


#define USB_IDMA_CTRL_LOW_VALUE       ICCLR_DST_BURST_LIM_128BYTE   \
                                    | ICCLR_SRC_BURST_LIM_128BYTE   \
                                    | ICCLR_BLOCK_MODE              \
                                    | ICCLR_DESC_MODE_16M

static char*    sramBase = (char*)NULL;
static int      sramSize = 0;

extern int      global_wa_sram_parts;
extern int      global_wa_threshold; 

MV_U32     mvUsbSramGet(MV_U32* pSize)
{
    char*   pBuf;

    /* Align address to 64 bytes */
    pBuf = (char*)MV_ALIGN_UP((MV_U32)sramBase, 64);

    if(pSize != NULL)
    {
        *pSize = sramSize - (pBuf - sramBase);
    }
/*
    mvOsPrintf("mvUsbSramGet: Base=%p (%p), Size=%d (%d)\n", 
                sramBase, pBuf, sramSize, *pSize);
*/
    return (MV_U32)pBuf;
}

void        mvUsbIdmaToSramCopy(void* sram_buf, void* src_buf, unsigned int size)
{
    unsigned long phys_addr;

/*
    mvOsPrintf("IdmaToSramCopy: idma=%d, sram=%p, src=%p (0x%x)\n", 
                    usbWaIdma, sram_buf, src_buf, phys_addr);
*/
    if( (usbWaIdma > 0) && (usbWaIdma < MV_IDMA_MAX_CHAN) )
    {
        phys_addr = mvOsCacheFlush(NULL, src_buf, size);

        /* !!!! SRAM Uncached */
        /*mvOsCacheInvalidate(NULL, sram_buf, size);*/

        mvDmaTransfer(usbWaIdma, (MV_U32)phys_addr, 
                      (MV_U32)sram_buf, size, 0);

        /* Wait until copy is finished */
        while( mvDmaStateGet(usbWaIdma) != MV_IDLE );
    }
    else
    {
        memcpy(sram_buf, src_buf, size);
        /* !!!! SRAM Uncached */
        /* mvOsCacheFlush(NULL, sram_buf, size);*/
    }
}

USB_WA_FUNCS    usbWaFuncs = 
{
    mvUsbSramGet,
    mvUsbIdmaToSramCopy
};

#endif /* USB_UNDERRUN_WA */

extern MV_U32   mvUsbGetCapRegAddr(int devNo);

static void* mvUsbIoUncachedMalloc( void* pDev, MV_U32 size, MV_U32 alignment, 
                                    MV_ULONG* pPhyAddr )
{
#if (defined(MV_88F5182) || defined(MV_88F5181L)) && defined(USB_UNDERRUN_WA)
    if(usbWaSramDescr != 0)
    {
        char*   pBuf;

        pBuf = (char*)MV_ALIGN_UP((MV_U32)sramBase, alignment);
        size += (pBuf - sramBase);
        if( (sramSize < size) )
        {
            mvOsPrintf("SRAM malloc failed: Required %d bytes - Free %d bytes\n", 
                        size, sramSize);
            return NULL;
        }
        if(pPhyAddr != NULL)
            *pPhyAddr = (MV_ULONG)sramBase;

        pBuf = sramBase;

        sramSize -= size;
        sramBase += size;

        return pBuf;
    }
#endif 
    return mvOsIoUncachedMalloc( pDev, size+alignment, pPhyAddr,NULL);
}

static void mvUsbIoUncachedFree( void* pDev, MV_U32 size, MV_ULONG phyAddr, void* pVirtAddr )
{
#if (defined(MV_88F5182) || defined(MV_88F5181L)) && defined(USB_UNDERRUN_WA)
    if(usbWaSramDescr != 0)
    {
        return;
    }
#endif 
    return mvOsIoUncachedFree( pDev, size, phyAddr, pVirtAddr);
} 

static MV_ULONG mvUsbCacheInvalidate( void* pDev, void* p, int size )
{
#if (defined(MV_88F5182) || defined(MV_88F5181L)) && defined(USB_UNDERRUN_WA)
    if( ((char*)p >= sramBase) && 
        ((char*)p < (sramBase + sramSize)) )
        return (unsigned long)p;
#endif

    return mvOsCacheInvalidate( pDev, p, size);
}

static MV_ULONG mvUsbCacheFlush( void* pDev, void* p, int size )
{
#if (defined(MV_88F5182) || defined(MV_88F5181L)) && defined(USB_UNDERRUN_WA)
    if( ((char*)p >= sramBase) && 
        ((char*)p < (sramBase + sramSize)) )
        return (unsigned long)p;
#endif

    return mvOsCacheFlush( pDev, p, size);
}

static MV_ULONG  usbMemVirtToPhy(void* pDev, void* pVirt)
{
    return mvOsIoVirtToPhy(pDev, pVirt);
}

static void   usbDevResetComplete(int devNo)
{
    MV_U32  regVal; 

    regVal = MV_USB_CORE_MODE_DEVICE | MV_USB_CORE_SETUP_LOCK_DISABLE_MASK;
    if(streaming == 0)    
        regVal |= MV_USB_CORE_STREAM_DISABLE_MASK; 

    /* Set USB_MODE register */
    MV_REG_WRITE(MV_USB_CORE_MODE_REG(devNo), regVal); 
}

USB_IMPORT_FUNCS    usbImportFuncs = 
{
    mvOsPrintf,
    mvOsSPrintf,
    mvUsbIoUncachedMalloc,
    mvUsbIoUncachedFree,
    mvOsMalloc,
    mvOsFree,
    memset,
    memcpy,
    mvUsbCacheFlush,
    mvUsbCacheInvalidate,
    usbMemVirtToPhy,
    NULL,
    NULL,
    mvUsbGetCapRegAddr,
    usbDevResetComplete
};



typedef struct
{
    unsigned long       taskId;
    SEM_ID              semId;
    MV_BOOL             isActive;
    int                 isrCount;
    int                 taskCount;
    _usb_device_handle  handle;
    MV_U8               gppNo;
    int                 gppIsrCount;
    int                 gppPollCount;
    MV_BOOL             isMouse;
    int                 diskSize;
    int                 mouseResumeCount;

} MV_USB_DEVICE;

MV_BOOL         mvUsbIsFirst = MV_TRUE;
MV_USB_DEVICE   mvUsbDevice[MV_BOARD_MAX_USB];

MV_BOOL         mvUsbDevIsISR = MV_TRUE;

WDOG_ID         mvUsbMouseWd = NULL;
int             mvUsbMouseTimeout = 10;  /* wakeup after 10 sec */

/* Local functions */
static void    mvUsbMouseResume(void);
void    mvUsbDevIsr(int dev);


#ifdef MV_USB_VOLTAGE_FIX

/* 0 - disable, 1 - polling by Watch Dog timer, 2 - Interrupt */
int                 mvUsbGppMode = 0;
WDOG_ID             mvUsbGppWd = NULL;
/* Time in system ticks */
int                 mvUsbGppWdTime = 1;

static void    mvUsbGppIsr(int dev)
{
    mvUsbDevice[dev].gppIsrCount++;

    mvUsbBackVoltageUpdate(dev, mvUsbDevice[dev].gppNo);
}

static void    mvUsbGppPoll(void)
{
    int     dev;

    for(dev=0; dev<mvCtrlUsbMaxGet(); dev++)
    {
        if(mvUsbDevice[dev].handle != NULL)
        {
            mvUsbBackVoltageUpdate(dev, mvUsbDevice[dev].gppNo);
            mvUsbDevice[dev].gppPollCount++;
        }
        /* restart timer */
        if(mvUsbGppWd != NULL)
            wdStart(mvUsbGppWd, mvUsbGppWdTime, (FUNCPTR)mvUsbGppPoll, 0);
    }
}
#endif /* MV_USB_VOLTAGE_FIX */

void     mvUsbDevStatsClear(int dev)
{
    if( (dev < 0) || dev >= mvCtrlUsbMaxGet() )
    {
        mvOsPrintf("USB devNo=%d is out of range. Valid range is 0...%d\n", 
                    dev, mvCtrlUsbMaxGet());
        return;
    }

    mvUsbDevice[dev].isrCount = 0;
    mvUsbDevice[dev].taskCount = 0;
    mvUsbDevice[dev].mouseResumeCount = 0;
    mvUsbDevice[dev].gppIsrCount = 0;
}

void    mvUsbDevStatus(int dev)
{
    if( (dev < 0) || dev >= mvCtrlUsbMaxGet() )
    {
        mvOsPrintf("USB devNo=%d is out of range. Valid range is 0...%d\n", 
                    dev, mvCtrlUsbMaxGet());
        return;
    }

    mvOsPrintf("\n\t USB Device #%d (%s) - %s\n\n", 
        dev, mvUsbDevice[dev].isMouse ? "Mouse" : "Disk",
        mvUsbDevice[dev].isActive ? "Active" : "Inactive");

    mvOsPrintf("semId=0x%x, gppNo=%d, handle=0x%x\n",
                mvUsbDevice[dev].semId, mvUsbDevice[dev].gppNo,
                mvUsbDevice[dev].handle);
    mvOsPrintf("isr=%d, task=%d, gppIsr=%d, gppPoll=%d\n",
                mvUsbDevice[dev].isrCount, mvUsbDevice[dev].taskCount,
                mvUsbDevice[dev].gppIsrCount, 
                mvUsbDevice[dev].gppPollCount);
    if(mvUsbDevice[dev].isMouse)
        mvOsPrintf("mouseResume=%d\n", mvUsbDevice[dev].mouseResumeCount);
}

void    mvUsbDevShow(int dev, unsigned int mode)
{
    int     i;

    if( (dev < 0) || dev >= mvCtrlUsbMaxGet() )
    {
        mvOsPrintf("USB devNo=%d is out of range. Valid range is 0...%d\n", 
                    dev, mvCtrlUsbMaxGet());
        return;
    }

    if(mvUsbDevice[dev].handle == NULL)
    {
        mvOsPrintf("USB-%d Device is not initialized\n", dev);
        return;
    }

    if( MV_BIT_CHECK(mode, 0) )
        _usb_regs(mvUsbDevice[dev].handle);

    if( MV_BIT_CHECK(mode, 1) )
        _usb_status(mvUsbDevice[dev].handle);

    if( MV_BIT_CHECK(mode, 2) )
        _usb_stats(mvUsbDevice[dev].handle);

    if( MV_BIT_CHECK(mode, 3) )
        _usb_debug_print_trace_log();

    for(i=0; i<_usb_device_get_max_endpoint(mvUsbDevice[dev].handle); i++)
    {
        if( MV_BIT_CHECK(mode, (8+i)) )
        {
            _usb_ep_status(mvUsbDevice[dev].handle, i, ARC_USB_RECV);
            _usb_ep_status(mvUsbDevice[dev].handle, i, ARC_USB_SEND);
        }
    }
}


static void     mvUsbDevsInit(void)
{
    int dev;

    for(dev=0; dev<mvCtrlUsbMaxGet(); dev++)
    {
        mvUsbDevice[dev].handle = NULL;
        mvUsbDevice[dev].gppNo = (MV_U8)N_A;
        mvUsbDevice[dev].isActive = MV_FALSE;
        mvUsbDevice[dev].isMouse = MV_FALSE;
        mvUsbDevice[dev].semId = NULL;
    }
    /* First of all. */
    _usb_device_set_bsp_funcs(&usbImportFuncs);

#if defined(USB_UNDERRUN_WA)

    global_wa_funcs = &usbWaFuncs;

    sramBase = (char*)(CRYPT_ENG_BASE + (2 * 1024) );
    sramSize = (8 - 2) * 1024;

    memset(sramBase, 0, sramSize);

    if( (usbWaIdma >= 0) && (usbWaIdma < MV_IDMA_MAX_CHAN) )
    {
        MV_REG_WRITE(IDMA_CAUSE_REG, 0);
        MV_REG_WRITE(IDMA_MASK_REG, 0);
        MV_REG_WRITE(IDMA_BYTE_COUNT_REG(usbWaIdma), 0);
        MV_REG_WRITE(IDMA_CURR_DESC_PTR_REG(usbWaIdma), 0);
        MV_REG_WRITE(IDMA_CTRL_HIGH_REG(usbWaIdma), ICCHR_ENDIAN_LITTLE | ICCHR_DESC_BYTE_SWAP_EN);
        MV_REG_WRITE(IDMA_CTRL_LOW_REG(usbWaIdma), USB_IDMA_CTRL_LOW_VALUE);
        mvOsPrintf("Use IDMA channel #%d for Underrun WA\n", usbWaIdma);
    }

#endif /* USB_UNDERRUN_WA */
}

static int    mvUsbDevLoad(int dev)
{
    STATUS  status;

    /* Create semaphore */
    mvUsbDevice[dev].semId = semBCreate(SEM_Q_PRIORITY, SEM_EMPTY);
    if(mvUsbDevice[dev].semId == NULL)
    {
        mvOsPrintf("mvUsbDevTask: Can't create semaphore\n");
        return 1;
    }

#ifdef MV_USB_VOLTAGE_FIX
    if(mvUsbGppMode != 0)
    {
        mvUsbDevice[dev].gppNo = mvUsbGppInit(dev);
        if(mvUsbDevice[dev].gppNo != (MV_U8)N_A)
        {
            if(mvUsbGppMode == 2)
            {
                /* Interrupt mode */
                status = vxGppIntConnect(mvUsbDevice[dev].gppNo, mvUsbGppIsr, 
                                         mvUsbDevice[dev].gppNo, 0);
                if (status != OK)
                {
                    mvOsPrintf("usbDev-%d: Can't connect to GPP_%d interrupt (%d), status=0x%x\n", 
                                dev, mvUsbDevice[dev].gppNo, status);
                    return (4);
                }

                status = vxGppIntEnable(mvUsbDevice[dev].gppNo);
                if (status != OK)
                {
                    mvOsPrintf("usbDev-%d: Can't enable GPP_%d interrupt (%d), status=0x%x\n", 
                                dev, mvUsbDevice[dev].gppNo, status);
                    return (5);
                }
            }
            else
            {
                /* Polling mode - one watchdog for all USB devices */
                if(mvUsbGppWd == NULL)
                {
                    mvUsbGppWd = wdCreate();
                    if(mvUsbGppWd == NULL)
                    {
                        mvOsPrintf ("Can't create watchdog for VBUS Polling\n");  
                        return 2;
                    }
                    /* Start watchdog on each tick */
                    wdStart(mvUsbGppWd, mvUsbGppWdTime, (FUNCPTR)mvUsbGppPoll, 0);
                }
            }
        }
    }
#endif /* MV_USB_VOLTAGE_FIX */

    if(mvUsbDevice[dev].isMouse)
    {
        mvUsbDevice[dev].handle = usbMouseLoad(dev);
        if(mvUsbDevice[dev].handle == NULL)
        {
            mvOsPrintf("Load USB-%d device is FAILED\n", dev);
            return 1;
        }
        /* Start wdog timer for self resume - one for all USB devices */        
        if(mvUsbMouseWd == NULL)
        {
            mvUsbMouseWd = wdCreate();
            if(mvUsbMouseWd == NULL)
            {
                mvOsPrintf("USB-%d device: Can't create watchdog for Mouse resume\n", dev);
                return 2;
            }
            wdStart(mvUsbMouseWd, (sysClkRateGet()*mvUsbMouseTimeout), 
                                    (FUNCPTR)mvUsbMouseResume, 0);
        }
    }
    else
    {
        mvUsbDevice[dev].handle = usbDiskLoad(dev, mvUsbDevice[dev].diskSize);
        if(mvUsbDevice[dev].handle == NULL)
        {
            mvOsPrintf("Load USB-%d device is FAILED\n", dev);
            return 1;
        }
    }
    _usb_device_start(mvUsbDevice[dev].handle);

    /* Connect interrupt handler and enable interrupt */
    status = intConnect((VOIDFUNCPTR * )INT_LVL_USB_CNT(dev), mvUsbDevIsr, (int)dev);
    if (status != OK)
    {
        mvOsPrintf("usbDev-%d: Can't connect to USB interrupt (%d), status=0x%x \n", 
                    dev, INT_LVL_USB_CNT(dev), status);
        return (status);
    }
    intEnable(INT_LVL_USB_CNT(dev));
    return 0;
}

static void    mvUsbDevUnload(int dev)
{
    intDisable(INT_LVL_USB_CNT(dev));

#ifdef MV_USB_VOLTAGE_FIX
    if(mvUsbGppMode == 2)
    {
        vxGppIntDisable(mvUsbDevice[dev].gppNo);
    }
    else if(mvUsbGppWd != NULL)
    {
        wdCancel(mvUsbGppWd);
        wdDelete(mvUsbGppWd);
        mvUsbGppWd = NULL;
    }
#endif /* MV_USB_VOLTAGE_FIX */

    /* Unload USB Device */
    if(mvUsbDevice[dev].isMouse)    
    {
        usbMouseUnload(mvUsbDevice[dev].handle);
        /* If never of devices use mvUsbMouseWd - close it */
        if(mvUsbMouseWd != NULL)
        {
            wdCancel(mvUsbMouseWd);
            wdDelete(mvUsbMouseWd);
            mvUsbMouseWd = NULL;
        }
    }
    else 
    {    
        usbDiskUnload(mvUsbDevice[dev].handle);
    }

    mvUsbDevice[dev].taskId = 0;
    mvUsbDevice[dev].diskSize = 0;
    mvUsbDevice[dev].handle = NULL;
    mvUsbDevice[dev].isMouse = MV_FALSE;

    if(mvUsbDevice[dev].semId != NULL)
    {
        semDelete(mvUsbDevice[dev].semId);
        mvUsbDevice[dev].semId = NULL;
    }
}

void    mvUsbDevIsr(int dev)
{
    mvUsbDevice[dev].isrCount++;

    if(mvUsbDevIsISR)
    {
        if(mvUsbDevice[dev].handle == NULL)
        {
            logMsg("usbDev-%d ISR: USB device is not initialized\n", 
                    dev, 0, 0, 0, 0, 0);
            return;
        }
        _usb_dci_vusb20_isr(mvUsbDevice[dev].handle);
    }
    else
    {
        intDisable(INT_LVL_USB_CNT(dev));
        semGive(mvUsbDevice[dev].semId);
    }
}

static void    mvUsbMouseResume(void)
{
    int     dev, keyLock;

    for(dev=0; dev<mvCtrlUsbMaxGet(); dev++)
    {
        if( (mvUsbDevice[dev].handle != NULL) &&
            (mvUsbDevice[dev].isMouse) )
        {
            mvUsbDevice[dev].mouseResumeCount++;

            keyLock = intLock();
            usbMousePeriodicResume(mvUsbDevice[dev].handle);
            intUnlock(keyLock);
        }
    }
    /* restart timer */
    wdStart(mvUsbMouseWd, (sysClkRateGet()*mvUsbMouseTimeout), 
                                    (FUNCPTR)mvUsbMouseResume, 0);
}

static unsigned __TASKCONV mvUsbDevTask(void* arglist)
{
    int     rc, dev = (int)arglist;

    rc = mvUsbDevLoad(dev);
    if(rc != 0)
    {
        return rc;
    }
    mvUsbDevice[dev].isActive = MV_TRUE;
        
    while (mvUsbDevice[dev].isActive) 
    {
        semTake(mvUsbDevice[dev].semId, 1000);
        
        if(!mvUsbDevIsISR)
        {
            _usb_dci_vusb20_isr(mvUsbDevice[dev].handle);
            mvUsbDevice[dev].taskCount++;
            intEnable(INT_LVL_USB_CNT(dev));
        }
    } /* Endwhile */

    mvUsbDevUnload(dev);

    return 0;
}

int     usbDiskStart(int dev, int diskSize)
{
    long    rc;
    char    taskName[20];

    if(mvUsbIsFirst == MV_TRUE)
    {
        mvUsbDevsInit();
        mvUsbIsFirst = MV_FALSE;
    }

    if( (dev < 0) || dev >= mvCtrlUsbMaxGet() )
    {
        mvOsPrintf("USB devNo=%d is out of range. Valid range is 0...%d\n", 
                    dev, mvCtrlUsbMaxGet());
        return 1;
    }
    if(mvUsbCtrlIsHost[dev])
    {
        mvOsPrintf("USB devNo=%d is configured to work as USB Host\n", dev);
        return 3;
    }
    if(mvUsbDevice[dev].handle != NULL)
    {
        mvOsPrintf("USB-%d Device already initialized\n", dev);
        return 2;
    }
    mvUsbDevStatsClear(dev);

    if(diskSize == 0)
        diskSize = USB_DISK_DEFAULT_SIZE;

    mvUsbDevice[dev].diskSize = diskSize;

    mvOsPrintf("USB-%d Disk example: %d Kbytes\n", dev, diskSize);

    sprintf(taskName, "tUsb%d", dev);
    rc = mvOsTaskCreate(taskName, USB_TASK_PRIO, 4*1024, mvUsbDevTask, 
                        (void*)dev, &mvUsbDevice[dev].taskId);
    if (rc != MV_OK) 
    {
        mvOsPrintf("Can't create %s task, rc = %ld\n", taskName, rc);
        return rc;
    }

    return 0;
}

int     usbMouseStart(int dev)
{
    long    rc;
    char    taskName[20];

    if(mvUsbIsFirst == MV_TRUE)
    {
        mvUsbDevsInit();
        mvUsbIsFirst = MV_FALSE;
    }

    if( (dev < 0) || dev >= mvCtrlUsbMaxGet() )
    {
        mvOsPrintf("USB devNo=%d is out of range. Valid range is 0...%d\n", 
                    dev, mvCtrlUsbMaxGet());
        return 1;
    }
    if(mvUsbCtrlIsHost[dev])
    {
        mvOsPrintf("USB devNo=%d is configured to work as USB Host\n", dev);
        return 3;
    }
    if(mvUsbDevice[dev].handle != NULL)
    {
        mvOsPrintf("USB-%d Device already initialized\n", dev);
        return 2;
    }
    mvUsbDevStatsClear(dev);

    mvUsbDevice[dev].isMouse = MV_TRUE;
    mvOsPrintf("USB-%d Mouse example\n", dev);

    sprintf(taskName, "tUsb%d", dev);
    rc = mvOsTaskCreate(taskName, USB_TASK_PRIO, 4*1024, mvUsbDevTask, 
                        (void*)dev, &mvUsbDevice[dev].taskId);
    if (rc != MV_OK) 
    {
        mvOsPrintf("Can't create %s task, rc=%ld\n", taskName, rc);
        return rc;
    }
    return 0;
}

int     usbDevStop(int dev)
{
    if( (dev < 0) || dev >= mvCtrlUsbMaxGet() )
    {
        mvOsPrintf("USB devNo=%d is out of range. Valid range is 0...%d\n", 
                    dev, mvCtrlUsbMaxGet());
        return 1;
    }
    if(mvUsbDevice[dev].handle == NULL)
    {
        mvOsPrintf("USB-%d Device is not active\n", dev);
        return 2;
    }

    mvUsbDevice[dev].isActive = MV_FALSE;
    semGive(mvUsbDevice[dev].semId);

    return 0;
}

