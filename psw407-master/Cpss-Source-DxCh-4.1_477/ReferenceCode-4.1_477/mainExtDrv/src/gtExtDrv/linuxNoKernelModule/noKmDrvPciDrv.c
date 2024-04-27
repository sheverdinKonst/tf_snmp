/*******************************************************************************
*                Copyright 2015, MARVELL SEMICONDUCTOR, LTD.                   *
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
********************************************************************************
* noKmDrvPciDrv.c
*
* DESCRIPTION:
*       Includes PCI functions wrappers implementation.
*
* DEPENDENCIES:
*       -   sysfs
*
* FILE REVISION NUMBER:
*       $Revision: 1 $
*
*******************************************************************************/
#if 0

Description:
    This driver use pci-sysfs driver which allocates entries in
    /sys/bus/pci/devices/
    For each devices the following allocated:
        deviceId, vendorId, class, config, irq, resource%d

    So, the driver access PP using this feature
    (mmap resources, read/write config, irq, IDs...)




AC3, BC2, Cetus, Caelum devices:
These devices require confuguration of MBUS <=> PCIe -
    resource <=> bar{1,2} offset XXX =>  PCIe BAR{2,4}

Here:
    resource    - switch, dfx, RAM, dragonite
    bar1        - a resource mapped to PCI BAR2
    bar2        - a resource mapped to PCI BAR4

These devices are configured by extDrvInitDrv():
1. Scan all PCI devices
2. For matching device:
    2.1. map PCI BAR0 (control and management) to userspace
    2.2. resolve size of bar1, bar2
    2.3. configire size of bar1, bar2
    2.4. unmap PCI BAR0, rescan device, mmap it again
    2.5. reset all windows
    2.6. configure and enable windows

#endif


#define _BSD_SOURCE
#define _POSIX_SOURCE
#include <gtExtDrv/drivers/gtPciDrv.h>
#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <pthread.h>

/*
 * Based on
 * Bobcat2 Control and Management Subsystem
 */

static GT_U32 mv_window_ctl_reg(int win)
{
    return (win < 5) ? (0x41820+win*0x10) : 0x41880;
}
static GT_U32 mv_window_base_reg(int win)
{
    return mv_window_ctl_reg(win) + 4;
}
static GT_U32 mv_window_remap_reg(int win)
{
    return mv_window_ctl_reg(win) + 0xc;
}







/* heplful macros */
#define _64K 0x00010000
#define _1M  0x00100000
#define _2M  0x00200000
#define _4M  0x00400000
#define _16M 0x01000000
#define _64M 0x04000000
#define SIZE_TO_BAR_REG(_size) ((((_size)>>16)-1)<<16)
/* to simplify code bus,dev,func*/
#define BDF pciBus, pciDev, pciFunc
#define BDF_DECL \
    IN  GT_U32  pciBus, \
    IN  GT_U32  pciDev, \
    IN  GT_U32  pciFunc


#define UNIT_ID_SWITCH          0x3
#define UNIT_ID_DFX             0x8
#define UNIT_ID_DRAM            0x0
#define UNIT_ID_DRAGONITE       0xa


struct pci_decoding_window {
	uint8_t     win;         /* window number 0..5 */
	uint8_t     bar;         /* pci bar map to 1..2, map to PCI BAR2, BAR4 */
	unsigned    base_offset; /* offset from BAR base */
	unsigned    size;        /* window size, 64KB granularity, see A.2.9.1.*/
	unsigned    remap;       /* window remap */
	uint8_t     target_id;   /* A.2.9.1. */
	uint8_t     attr;
	uint8_t     enable;
};

static struct pci_decoding_window ac3_pci_sysmap[] = {
    /*win  bar offset    size  remap       target_id            attr  enable*/
/* BAR 1*/
    {0,    1,  0x0,      _64M, 0x0,        UNIT_ID_SWITCH,      0x0,  1},
/* BAR 2*/
    {1,    2,  0x0,      _1M,  0x0,        UNIT_ID_DFX,         0x0,  1},
    {2,    2,  _2M,      _2M,  0xFFE00000, UNIT_ID_DRAM,        0x3E, 1},
    {3,    2,  _4M,      _64K, 0x0,        UNIT_ID_DRAGONITE,   0x0,  1},
    {4,    2,  _4M+_64K, _64K, 0x04000000, UNIT_ID_DRAGONITE,   0x0,  1},
    {0xff, 0,  0x0,      0,    0x0,        0,                   0x0,  0},
};

static struct pci_decoding_window bc2_pci_sysmap[] = {
    /*win  bar offset    size  remap       target_id            attr  enable*/
/* BAR 1*/
    {0,    1,  0x0,      _64M, 0x0,        UNIT_ID_SWITCH,      0x0,  1},
/* BAR 2*/
    {1,    2,  0x0,      _1M,  0x0,        UNIT_ID_DFX,         0x0,  1},
    {2,    2,  _2M,      _2M,  0xFFE00000, UNIT_ID_DRAM,        0x3E, 1},
#ifdef CONFIG_MV_SERVICECPU_FIRMWARE_IN_DRAM
/* In order to use a new index for DRAM window, add two disabled windows */
    {3,    2,  0x0,      0,    0x0,        0,                   0x0,  0},
    {4,    2,  0x0,      0,    0x0,        0,                   0x0,  0},
    {5,    2,  _16M,     _16M, 0x0,        UNIT_ID_DRAM,        0x1E, 1},
#endif
    {0xff, 0,  0x0,      0,    0x0,        0,                   0x0,  0},
};

static struct pci_decoding_window bobk_pci_sysmap[] = {
    /*win  bar offset    size  remap       target_id            attr  status*/
/* BAR 1*/
    {0,	   1,  0x0,      _64M, 0x0,        UNIT_ID_SWITCH,      0x0,  1},
/* BAR 2*/
    {1,    2,  0x0,      _1M,  0x0,        UNIT_ID_DFX,         0x0,  1},
    {2,    2,  _2M,      _2M,  0xFFE00000, UNIT_ID_DRAM,        0x3E, 1},
    {3,    2,  _4M,      _64K, 0x0,        UNIT_ID_DRAGONITE,   0x0,  1},
    {4,    2,  _4M+_64K, _64K, 0x04000000, UNIT_ID_DRAGONITE,   0x0,  1},
    {0xff, 0,  0x0,      0,    0x0,        0,                   0x0,  0},
};

struct {
    const char *name;
    unsigned devId;
    unsigned devIdMask;
    struct pci_decoding_window* sysmap;
} prestera_sysmap[] = {
    { "bobcat2",   0xfc00, ~0xff, bc2_pci_sysmap },
    { "alleycat3", 0xf400, ~0xff, ac3_pci_sysmap },
    { "cetus",     0xbe00, ~0xff, bobk_pci_sysmap },
    { "caelum",    0xbc00, ~0xff, bobk_pci_sysmap },

    { NULL,0,0,NULL }
};


/***************************************************/
/*   register read/write definitions               */
/***************************************************/
static GT_U32 mv_reg_read(GT_UINTPTR regsBase, GT_U32 regAddr)
{
    GT_U32 val = *((volatile GT_U32*)(regsBase+regAddr));
    return le32toh(val);
}
static void mv_reg_write(GT_UINTPTR regsBase, GT_U32 regAddr, GT_U32 value)
{
    *((volatile GT_U32*)(regsBase+regAddr)) = htole32(value);
}
static void mv_reg_write_field(GT_UINTPTR regsBase, GT_U32 regAddr, GT_U32 mask, GT_U32 value)
{
    GT_U32 val;
    val = mv_reg_read(regsBase, regAddr);
    val &= ~mask;
    val |= value;
    mv_reg_write(regsBase, regAddr, val);
}

/***************************************************/
/*   resize PCI bar (AC3, BC2, BobK)               */
/***************************************************/
static void mv_resize_bar(GT_UINTPTR regsBase, GT_U32 ctrlReg, unsigned size)
{
    /* Disable BAR before reconfiguration */
    mv_reg_write_field(regsBase, ctrlReg, 0x1, 0x0);
    /* Resize */
    mv_reg_write(regsBase, ctrlReg, SIZE_TO_BAR_REG(size));
    /* Enable BAR */
    mv_reg_write_field(regsBase, ctrlReg, 0x1, 0x1);

}
/***************************************************/
/*   calculate total bar size (AC3, BC2, BobK)     */
/*   must be power of 2                            */
/***************************************************/
static unsigned mv_calc_bar_size(struct pci_decoding_window *win_map, uint8_t bar)
{
	uint8_t target;
	unsigned size = 0;

	for (target = 0; win_map[target].win != 0xff; target++) {
		if (!win_map[target].enable)
			continue;

		if (win_map[target].bar != bar)
			continue;

        while (size < win_map[target].base_offset + win_map[target].size)
            size = (size ? (size << 1) : 0x10000);

	}
	return size;
}



/* open file in /sys/bus/pci/devices/0000:$bus:$dev.$func/ */
static GT_STATUS sysfs_pci_open(
    BDF_DECL,
    IN  const char *name,
    IN  int     flags,
    OUT int     *fd
)
{
    char fname[128];
    if (pciBus > 255 || pciDev > 31 || pciFunc > 7)
        return GT_BAD_PARAM;
    sprintf(fname, "/sys/bus/pci/devices/0000:%02x:%02x.%x/%s",
            pciBus, pciDev, pciFunc, name);
    *fd = open(fname, flags);
    if (*fd < 0)
        return GT_FAIL;
    return GT_OK;
}

static GT_STATUS sysfs_pci_readNum(
    BDF_DECL,
    IN  const char *name,
    OUT unsigned *val
)
{
    GT_STATUS ret;
    int fd, n;
    char buf[64];

    ret = sysfs_pci_open(BDF, name, O_RDONLY, &fd);
    if (ret != GT_OK)
        return ret;
    n=read(fd, buf, sizeof(buf));
    close(fd);
    if (n <= 0)
        return GT_FAIL;
    buf[n] = 0;
    if (sscanf(buf, "%i", val) != 1)
        return GT_FAIL;
    return GT_OK;
}


/* open file to read/write register */
static GT_STATUS sysfs_pci_open_regs_and_lseek(
    BDF_DECL,
    IN  GT_U32  regAddr,
    IN  int     flags,
    OUT int     *fd
)
{
    if (regAddr & 3 || regAddr >= 64)
        return GT_BAD_PARAM;
    if (sysfs_pci_open(BDF, "config", flags, fd) != GT_OK)
        return GT_FAIL;
    if (lseek(*fd, regAddr, SEEK_SET) < 0)
    {
        close(*fd);
        return GT_FAIL;
    }
    return GT_OK;
}

/*******************************************************************************
* extDrvPciConfigWriteReg
*
* DESCRIPTION:
*       This routine write register to the PCI configuration space.
*
* INPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*       regAddr  - Register offset in the configuration space.
*       data     - data to write.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS extDrvPciConfigWriteReg
(
    IN  GT_U32  busNo,
    IN  GT_U32  devSel,
    IN  GT_U32  funcNo,
    IN  GT_U32  regAddr,
    IN  GT_U32  data
)
{
    int fd;
    GT_STATUS ret;
    ret = sysfs_pci_open_regs_and_lseek(busNo, devSel, funcNo, regAddr, O_RDWR, &fd);
    if (ret != GT_OK)
        return ret;
    /* data to LE */
    data = htole32(data);
    write(fd, &data, 4);
    close(fd);
    return GT_OK;
}


/*******************************************************************************
* extDrvPciConfigReadReg
*
* DESCRIPTION:
*       This routine read register from the PCI configuration space.
*
* INPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*       regAddr  - Register offset in the configuration space.
*
* OUTPUTS:
*       data     - the read data.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS extDrvPciConfigReadReg
(
    IN  GT_U32  busNo,
    IN  GT_U32  devSel,
    IN  GT_U32  funcNo,
    IN  GT_U32  regAddr,
    OUT GT_U32  *data
)
{
    int fd;
    GT_STATUS ret;
    ret = sysfs_pci_open_regs_and_lseek(busNo, devSel, funcNo, regAddr, O_RDONLY, &fd);
    if (ret != GT_OK)
        return ret;
    read(fd, data, 4);
    /* LE to CPU */
    *data = le32toh(*data);

    close(fd);
    return GT_OK;
}


/***************************************************/
/*   read physical addresses of PCI BAR2, BAR4     */
/***************************************************/
static GT_STATUS sysfs_read_resource_phys(
    BDF_DECL,
    OUT unsigned long long *res1,
    OUT unsigned long long *res2
)
{
    int fd;
    FILE *f;
    int i = 0;
    unsigned long long start, end, flags;
    GT_STATUS rc;

    rc = sysfs_pci_open(BDF, "resource", O_RDONLY, &fd);
    if (rc != GT_OK)
        return rc;
    f = fdopen(fd, "r");
    if (f == NULL)
        return GT_FAIL;
    while (!feof(f))
    {
        if (fscanf(f, "%lli %lli %lli", &start, &end, &flags) != 3)
            break;
        if (i == 2)
            *res1 = start;
        if (i == 4)
            *res2 = start;
        i++;
    }
    fclose(f);
    return (i > 4) ? GT_OK : GT_FAIL;
}

/*******************************************************************************
* mv_configure_win_bar
*   Configure PP mapping windows (PP unit <=> PCI BAR)
*   several windows can be mapped to the same PCI BAR
*   For example:
*       first window(2M) mapped to PCI BAR phys+0
*       second window(1M) will be mapped to PCI BAR phys+2M
*
*******************************************************************************/
static GT_STATUS mv_configure_win_bar(
    BDF_DECL,
    GT_UINTPTR regsBase,
    struct pci_decoding_window *win_map
)
{
    unsigned long long res1, res2;
    GT_STATUS rc;

    /* get bar2 and bar4 phys from .../resource : lines 2, 4 */
    rc = sysfs_read_resource_phys(BDF, &res1, &res2);
    if (rc != GT_OK)
    {
        printf("  Failed to read bar2 & bar4 phys addresses\n");
        return rc;
    }

    /* configire windows */
    for (; win_map->win != 0xff; win_map++)
    {
        GT_U32 win_ctl, win_base, win_remap, ctl_val, phys;

        if (!win_map->enable)
            continue;

        if (win_map->win >= 6)
        {
            printf("  bad window number\n");
            return GT_FAIL;
        }

        win_ctl = mv_window_ctl_reg(win_map->win);
        win_base = mv_window_base_reg(win_map->win);
        win_remap = mv_window_remap_reg(win_map->win);
        ctl_val =   SIZE_TO_BAR_REG(win_map->size) |
                    (win_map->target_id << 4) |
                    (win_map->attr << 8) |
                    (((win_map->bar == 1) ? 0 : 1) << 1) |
                    0x1 /* enable window */;
        phys = (GT_U32)((win_map->bar == 1) ? res1 : res2);
        phys += win_map->base_offset;

        mv_reg_write(regsBase, win_base, phys);
        mv_reg_write(regsBase, win_remap, win_map->remap | 0x1/*remap_enable*/);
        mv_reg_write(regsBase, win_ctl, ctl_val);

		printf("  BAR%d: win%d_ctrl = 0x%08x, win_base = 0x%08x\n",
                win_map->bar, win_map->win,
                mv_reg_read(regsBase, win_ctl),
                mv_reg_read(regsBase, win_base));

    }

	return GT_OK;
}

static GT_STATUS sysfs_pci_map_resource(
    BDF_DECL,
    IN  const char  *resname,
    IN  GT_UINTPTR   maxSize,
    OUT GT_UINTPTR  *mappedBase,
    OUT GT_UINTPTR  *mappedSize,
    OUT int         *fdPtr
)
{
    int fd;
    GT_STATUS ret;
    struct stat st;
    void *vaddr;

    ret = sysfs_pci_open(BDF, resname, O_RDWR, &fd);
    if (ret != GT_OK)
        return ret;
    if (fstat(fd, &st) < 0)
    {
        close(fd);
        return GT_FAIL;
    }
    if (maxSize > st.st_size)
        maxSize = st.st_size;

    vaddr = mmap(NULL,
                maxSize,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                fd,
                (off_t)0);
    if (MAP_FAILED == vaddr)
    {
        close(fd);
        return GT_FAIL;
    }
    *mappedBase = (GT_UINTPTR)vaddr;
    *mappedSize = (GT_UINTPTR)maxSize;
    if (fdPtr != NULL)
        *fdPtr = fd;
    return GT_OK;
}

static void sysfs_pci_configure(
    BDF_DECL
)
{
    unsigned pciVendorId, pciDeviceId, pciClass;
    int i, fd;
    unsigned size;
    struct pci_decoding_window* sysmap = NULL;
    GT_UINTPTR regsBase, regsSize;

    if (sysfs_pci_readNum(BDF, "vendor", &pciVendorId) != GT_OK)
        return;
    if (pciVendorId != 0x11ab)
        return;
    if (sysfs_pci_readNum(BDF, "class", &pciClass) != GT_OK)
        return;
    if (pciClass != 0x020000)
        return;
    if (sysfs_pci_readNum(BDF, "device", &pciDeviceId) != GT_OK)
        return;
    printf("Found marvell device %04x:%04x\n", pciVendorId, pciDeviceId);

    for (i = 0; prestera_sysmap[i].sysmap; i++)
    {
        if ((pciDeviceId & prestera_sysmap[i].devIdMask) ==
                prestera_sysmap[i].devId)
        {
            sysmap = prestera_sysmap[i].sysmap;
            printf("  Configuring %s...\n", prestera_sysmap[i].name);
            break;
        }
    }
    if (!sysmap) /* no matching entries, nothing to do */
        return;

    /***************************************************/
    /** Configure PCI BARs                            **/
    /***************************************************/
    /* map pci bar0 */
    if (sysfs_pci_map_resource(BDF, "resource0", _1M, &regsBase, &regsSize, &fd) != GT_OK)
    {
        printf("  Failed to map resource0, device not configured\n");
        return;
    }
    /* configure BAR1 size */
    size = mv_calc_bar_size(sysmap, 1);
    mv_resize_bar(regsBase, 0x41804, size);
    /* configure BAR2 size */
    size = mv_calc_bar_size(sysmap, 2);
    mv_resize_bar(regsBase, 0x41808, size);
    /* unmap pci bar 0 */
    munmap((void*)regsBase, regsSize);
    close(fd);
    /* rescan device */
    if (sysfs_pci_open(BDF, "rescan", O_WRONLY, &fd) != GT_OK)
    {
        printf("  Failed to open 'rescan' for write\n");
        return;
    }
    write(fd,"1\n",2);
    close(fd);


    /***************************************************/
    /** Configure PP windows BARs                     **/
    /***************************************************/
    /* map pci bar 0 again */
    if (sysfs_pci_map_resource(BDF, "resource0", _1M, &regsBase, &regsSize, &fd) != GT_OK)
    {
        printf("  Failed to map resource0, device not configured\n");
        return;
    }
    /* disable all windows which points bar2 & bar4 */
    for (i = 0; i < 6; i++)
    {
        mv_reg_write(regsBase, mv_window_ctl_reg(i), 0);
        mv_reg_write(regsBase, mv_window_base_reg(i), 0);
        mv_reg_write(regsBase, mv_window_remap_reg(i), 0);
    }
    /* configure windows */
    mv_configure_win_bar(BDF, regsBase, sysmap);

    /* Done, unmap pci bar 0 */
    munmap((void*)regsBase, regsSize);
    close(fd);
}

/*******************************************************************************
* extDrvInitDrv
*
* DESCRIPTION:
*       If first client initialize it
*
* INPUTS:
*       None
*
* OUTPUTS:
*       isFirstClient
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - othersise.
*
* COMMENTS:
*       Linux only
*
*******************************************************************************/
GT_STATUS extDrvInitDrv
(
    OUT GT_BOOL  *isFirstClient
)
{
    DIR *dir;
    struct dirent *d;
    unsigned pciBus,pciDev,pciFunc;

    if (isFirstClient != NULL)
    {
        *isFirstClient = GT_FALSE;
    }


    dir = opendir("/sys/bus/pci/devices");
    if (dir == NULL)
    {
        return GT_FAIL;
    }

    while (1)
    {
        d = readdir(dir);
        if (!d)
            break;
        if (sscanf(d->d_name,"0000:%x:%x.%x",&pciBus, &pciDev, &pciFunc) < 3)
            continue;

        sysfs_pci_configure(BDF);
    }
    closedir(dir);

    return GT_OK;
}

#ifdef NOKM_DRV_EMULATE_INTERRUPTS
#define DEVICES_MAX 8
static struct {
    GT_U32 intNum;
    GT_EXT_DRV_PCI_MAP_STC map;
} noKmMappingsList[DEVICES_MAX];
GT_U32 noKmMappingsNum = 0;
#endif

/*******************************************************************************
* extDrvPciMap
*
* DESCRIPTION:
*       This routine maps PP registers and PCI registers into userspace
*
* INPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*       regsSize - registers size (64M for PCI)
*
* OUTPUTS:
*       mapPtr   - The pointer to all mappings
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS extDrvPciMap
(
    IN  GT_U32                  busNo,
    IN  GT_U32                  devSel,
    IN  GT_U32                  funcNo,
    IN  GT_UINTPTR              regsSize,
    OUT GT_EXT_DRV_PCI_MAP_STC  *mapPtr
)
{
    GT_STATUS rc;

    /* TODO: get devVend for quirks */

    if (mapPtr == NULL)
        return GT_BAD_PARAM;
    mapPtr->regs.base = 0;
    mapPtr->regs.size = 0;
    mapPtr->config = mapPtr->dfx = mapPtr->regs;

    /* config: BAR0*/
    /* TODO: apply quirks to get length and offset */
    rc = sysfs_pci_map_resource(busNo, devSel, funcNo,
            "resource0",
            1024*1024, /* _1M */
            &(mapPtr->config.base),
            &(mapPtr->config.size), NULL);
    if (rc != GT_OK)
        return rc;


    /* regs: BAR2 */
    rc = sysfs_pci_map_resource(busNo, devSel, funcNo,
            "resource2",
            regsSize,
            &(mapPtr->regs.base),
            &(mapPtr->regs.size), NULL);
    if (rc != GT_OK)
        return rc;

    
    /* dfx: BAR4, ignore error if no such */
    sysfs_pci_map_resource(busNo, devSel, funcNo,
            "resource4",
            1024*1024, /* _1M */
            &(mapPtr->dfx.base),
            &(mapPtr->dfx.size), NULL);

#ifdef NOKM_DRV_EMULATE_INTERRUPTS
    if (noKmMappingsNum < DEVICES_MAX)
    {
        noKmMappingsList[noKmMappingsNum].intNum=0x11ab0000 |
            busNo << 8 | devSel << 3 | funcNo;
        noKmMappingsList[noKmMappingsNum].map = *mapPtr;
        noKmMappingsNum++;
    }
#endif
    return GT_OK;
}

#ifdef NOKM_DRV_EMULATE_INTERRUPTS
/*******************************************************************************
* prvExtDrvNokmGetMappingByIntNum
*
* DESCRIPTION:
*       This routine returns userspace mapping for interrupt emulation
*
* INPUTS:
*       intNum   - a generated interrupt
*                  0x11ab0000 | (bus<<8) | (dev<<3) | (func)
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Pointer to structure or NULL
*
* COMMENTS:
*
*******************************************************************************/
GT_EXT_DRV_PCI_MAP_STC* prvExtDrvNokmGetMappingByIntNum(GT_U32 intNum)
{
    GT_U32 i;
    for (i = 0; i < noKmMappingsNum; i++)
    {
        if (noKmMappingsList[i].intNum == intNum)
            return &(noKmMappingsList[i].map);
    }
    return NULL;
}
#endif /* NOKM_DRV_EMULATE_INTERRUPTS */

/*******************************************************************************
* extDrvPciFindDev
*
* DESCRIPTION:
*       This routine returns the next instance of the given device (defined by
*       vendorId & devId).
*
* INPUTS:
*       vendorId - The device vendor Id.
*       devId    - The device Id.
*       instance - The requested device instance.
*
* OUTPUTS:
*       busNo    - PCI bus number.
*       devSel   - the device devSel.
*       funcNo   - function number.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS extDrvPciFindDev
(
    IN  GT_U16  vendorId,
    IN  GT_U16  devId,
    IN  GT_U32  instance,
    OUT GT_U32  *busNo,
    OUT GT_U32  *devSel,
    OUT GT_U32  *funcNo
)
{
    DIR *dir;
    struct dirent *d;
    unsigned pciBus,pciDev,pciFunc;
    unsigned pciDevId, pciVendId;
    GT_U32 i = 0;

    dir = opendir("/sys/bus/pci/devices");
    if (dir == NULL)
    {
        return GT_FAIL;
    }

    while (1)
    {
        d = readdir(dir);
        if (!d)
            break;
        if (sscanf(d->d_name,"0000:%x:%x.%x",&pciBus, &pciDev, &pciFunc) < 3)
            continue;
        if (sysfs_pci_readNum(BDF, "vendor", &pciVendId) != GT_OK)
            continue;
        if (vendorId != pciVendId)
            continue;
        if (sysfs_pci_readNum(BDF, "device", &pciDevId) != GT_OK)
            continue;
        if (devId != pciDevId)
            continue;
        if (instance == i)
        {
            /* found */
            *busNo = pciBus;
            *devSel = pciDev;
            *funcNo = pciFunc;
            closedir(dir);
            return GT_OK;
        }
        instance++;
    }
    closedir(dir);
    return GT_FAIL;
}


/*******************************************************************************
* extDrvGetPciIntVec
*
* DESCRIPTION:
*       This routine return the PCI interrupt vector.
*
* INPUTS:
*       pciInt - PCI interrupt number.
*
* OUTPUTS:
*       intVec - PCI interrupt vector.
*
* RETURNS:
*       GT_OK      - on success.
*       GT_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS extDrvGetPciIntVec
(
    IN  GT_PCI_INT  pciInt,
    OUT void        **intVec
)
{
    return GT_FAIL;
}


/*******************************************************************************
* extDrvGetIntMask
*
* DESCRIPTION:
*       This routine return the PCI interrupt vector.
*
* INPUTS:
*       pciInt - PCI interrupt number.
*
* OUTPUTS:
*       intMask - PCI interrupt mask.
*
* RETURNS:
*       GT_OK      - on success.
*       GT_FAIL    - otherwise.
*
* COMMENTS:
*       PCI interrupt mask should be used for interrupt disable/enable.
*
*******************************************************************************/
GT_STATUS extDrvGetIntMask
(
    IN  GT_PCI_INT  pciInt,
    OUT GT_UINTPTR  *intMask
)
{
    return GT_FAIL;
}

/*******************************************************************************
* extDrvPcieGetInterruptNumber
*
* DESCRIPTION:
*       This routine returns interrupt vector/mask for PCIe device
*
* INPUTS:
*       busNo      - PCI bus number.
*       devSel     - the device devSel.
*       funcNo     - function number.
*
* OUTPUTS:
*       intNumPtr  - Interrupt number value
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS extDrvPcieGetInterruptNumber
(
    IN  GT_U32      busNo,
    IN  GT_U32      devSel,
    IN  GT_U32      funcNo,
    OUT GT_U32     *intNumPtr
)
{
#ifdef NOKM_DRV_EMULATE_INTERRUPTS
    *intNumPtr = 0x11ab0000 | busNo << 8 | devSel << 3 | funcNo;
    return GT_OK;
#else
    unsigned pciInt;
    GT_STATUS rc;

    rc = sysfs_pci_readNum(busNo, devSel, funcNo, "irq", &pciInt);
    *intNumPtr = (GT_U32)pciInt;
    return rc;
#endif
}

/*******************************************************************************
* extDrvEnableCombinedWrites
*
* DESCRIPTION:
*       This function enables / disables the Pci writes / reads combining
*       feature.
*       Some system controllers support combining memory writes / reads. When a
*       long burst write / read is required and combining is enabled, the master
*       combines consecutive write / read transactions, if possible, and
*       performs one burst on the Pci instead of two. (see comments)
*
* INPUTS:
*       enWrCombine - GT_TRUE enables write requests combining.
*       enRdCombine - GT_TRUE enables read requests combining.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK               - on sucess,
*       GT_NOT_SUPPORTED    - if the controller does not support this feature,
*       GT_FAIL             - otherwise.
*
* COMMENTS:
*       1.  Example for combined write scenario:
*           The controller is required to write a 32-bit data to address 0x8000,
*           while this transaction is still in progress, a request for a write
*           operation to address 0x8004 arrives, in this case the two writes are
*           combined into a single burst of 8-bytes.
*
*******************************************************************************/
GT_STATUS extDrvEnableCombinedPciAccess
(
    IN  GT_BOOL     enWrCombine,
    IN  GT_BOOL     enRdCombine
)
{
    return GT_OK;
}

/*******************************************************************************
* extDrvPciDoubleWrite
*
* DESCRIPTION:
*        This routine will write a 64-bit data  to given address
*
* INPUTS:
*        address - address to write to
*       word1 - the first half of double word to write (MSW)
*       word2 - the second half of double word to write (LSW)
*
* OUTPUTS:
*      none
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS extDrvPciDoubleWrite
(
    IN  GT_U32 address,
    IN  GT_U32 word1, 
    IN  GT_U32 word2
)
{
	return GT_NOT_SUPPORTED;
}

/*******************************************************************************
* extDrvPciDoubleRead
*
* DESCRIPTION:
*        This routine will read a 64-bit data  from given address
*
* INPUTS:
*        address - address to read from
*
* OUTPUTS:
*       data     -  pointer for the received data.
*
* RETURNS:
*       GT_OK   - on success,
*       GT_FAIL - otherwise.
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS extDrvPciDoubleRead
(
    IN  GT_U32  address,
    OUT GT_U64  *dataPtr
)
{
	return GT_NOT_SUPPORTED;
}
