/* usrScsi.c - SCSI initialization */

/* Copyright 1992-1996 Wind River Systems, Inc. */

/*
modification history
--------------------
02c,17nov96,jdi  doc: made sysScsiConfig() NOMANUAL, since it's in generic BSP.
02b,16sep96,dat  new macro SYS_SCSI_CONFIG, for rtn sysScsiConfig
02a,25jul96,jds  major modification of this file. Most of the previous code
                 is commented out.
01f,09may96,jds  fixed modeData warning with SCSI_AUTO_CONFIG definition
01e,31aug94,ism  added a #ifdef to keep the RT11 file system from being
		 included just because SCSI is included. (SPR #3572)
01d,11jan93,ccc  removed 2nd parameter from call to scsiAutoConfig().
01c,24dec92,jdi  removed note about change of 4th param in scsiPhysDevCreate();
		 mentioned where usrScsiConfig() is found.
01b,24sep92,ccc  note about change of 4th parameter in scsiPhysDevCreate().
01a,18sep92,jcf  written.
*/

/*
DESCRIPTION
This file is used to configure and initialize the VxWorks SCSI support.
This file is included by usrConfig.c.

There are many examples that are provided in this file which demonstrate
use of routines to configure various SCSI devices. Such a configuration is
user specific. Therefore, the examples provided in this file should be 
used as a model and customized to the user requirements.

MAKE NO CHANGES TO THIS FILE.

All BSP specific code should be added to
the sysLib.c file or a sysScsi.c file in the BSP directory.  Define the
macro SYS_SCSI_CONFIG to generate the call to sysScsiConfig().
See usrScsiConfig for more info on the SYS_SCSI_CONFIG macro.

SEE ALSO: usrExtra.c

NOMANUAL
*/
#include <vxWorks.h>
#include "config.h"
#include "mvOs.h"
#include <scsiLib.h>
#include <dosFsLib.h>
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 2) 
#include <xbdBlkDev.h>
#endif


#if (defined(MV_INCLUDE_SATA) && defined (INCLUDE_PCI)) 


/*******************************************************************************
*
* sysScsiConfig - Example SCSI device setup code
*
* This is an example of a SCSI routine, that could be added to the BSP sysLib.c
* file to perform SCSI disk/tape setup.  Define SYS_SCSI_CONFIG to have
* the kernel call this routine as part of usrScsiConfig().
*
* Most of the code found here is an example of how to declare a SCSI
* peripheral configuration.  You must edit this routine to reflect the
* actual configuration of your SCSI bus.  This example is found in
* src/config/usrScsi.c.
*
* If you are just getting started, you can test your hardware configuration
* by defining SCSI_AUTO_CONFIG in config.h, which will probe the bus and
* display all devices found.  No device should have the same SCSI bus ID as
* your VxWorks SCSI port (default = 7), or the same as any other device.
* Check for proper bus termination.
*
* There are three configuration examples here that 
* demostrate configuration of a SCSI hard disk (any type), an OMTI 3500 floppy 
* disk, and a tape drive (any type).
*
* The hard disk is divided into two 32-Mbyte partitions and a third
* partition with the remainder of the disk.  The first partition is initialized
* as a dosFs device.  The second and third partitions are initialized as
* rt11Fs devices, each with 256 directory entries.
*
* It is recommended that the first partition (BLK_DEV) on a block device be
* a dosFs device, if the intention is eventually to boot VxWorks from the
* device.  This will simplify the task considerably.
*
* The floppy, since it is a removable medium device, is allowed to have only a
* single partition, and dosFs is the file system of choice for this device,
* since it facilitates media compatibility with IBM PC machines.
*
* While the hard disk configuration is fairly straightforward, the floppy
* setup in this example is a bit intricate.  Note that the
* scsiPhysDevCreate() call is issued twice.  The first time is merely to get
* a "handle" to pass to the scsiModeSelect() function, since the default
* media type is sometimes inappropriate (in the case of generic
* SCSI-to-floppy cards).  After the hardware is correctly configured, the
* handle is discarded via the scsiPhysDevDelete() call, after which a
* second scsiPhysDevCreate() call correctly configures the peripheral.
* (Before the scsiModeSelect() call, the configuration information was
* incorrect.) Also note that following the scsiBlkDevCreate() call, the
* correct values for <sectorsPerTrack> and <nHeads> must be set via the
* scsiBlkDevInit() call.  This is necessary for IBM PC compatibility.
* Similarly, the tape configuration is also a little involved because 
* certain device parameters need to turned off within VxWorks and the
* tape fixed block size needs to be defined, assuming that the tape
* supports fixed blocks.
*
* The last parameter to the dosFsDevInit() call is a pointer to a
* DOS_VOL_CONFIG structure.  By specifying NULL, you are asking
* dosFsDevInit() to read this information off the disk in the drive.  This
* may fail if no disk is present or if the disk has no valid dosFs
* directory.  Should this be the case, you can use the dosFsMkfs() command to
* create a new directory on a disk.  This routine uses default parameters
* (see dosFsLib) that may not be suitable for your application, in which
* case you should use dosFsDevInit() with a pointer to a valid DOS_VOL_CONFIG
* structure that you have created and initialized.  If dosFsDevInit() is
* used, a diskInit() call should be made to write a new directory on the
* disk, if the disk is blank or disposable.
*
* NOTE
* The variable <pSbdFloppy> is global to allow the above calls to be
* made from the VxWorks shell, i.e.:
* .CS
*     -> dosFsMkfs "/fd0/", pSbdFloppy
* .CE
* If a disk is new, use diskFormat() to format it.
*
* INTERNAL
* The fourth parameter passed to scsiPhysDevCreate() is now
* <reqSenseLength> (previously <selTimeout>).
*
* NOMANUAL
*/

/* Data for example code */



STATUS
sysScsiConfigDisk (SCSI_PHYS_DEV *	pPhysDev,
				   char *pDevName, BOOL nfsExported)
    {

	BLK_DEV *	pBlk;
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 2) 
	device_t	xbdDevice;
#endif
    /*
     * NOTE: Either of the following global variables may be set or reset
     * from the VxWorks shell. Under 5.0, they should NOT both be set at the
     * same time, or output will be interleaved and hard to read!! They are
     * intended as an aid to trouble-shooting SCSI problems.
     */

    /*
     * The following section of code provides sample configurations within
     * VxWorks of SCSI peripheral devices and VxWorks file systems. It 
     * should however be noted that the actual parameters provided to
     * scsiPhysDevCreate(), scsiBlkDevCreate(), dosFsDevInit() etc., are
     * highly dependent upon the user environment and should therefore be 
     * modified accordingly.
     */

    /*
     * HARD DRIVE CONFIGURATION
     *
     * In order to configure a hard disk and initialise both dosFs and rt11Fs
     * file systems, the following initialisation code will serve as an
     * example.
     */

    /* configure a SCSI hard disk at busId = 2, LUN = 0 */

	if ((int)pPhysDev == 0)
	{
		return ERROR;
	}

	{
	/* create block devices */

        if ((pBlk = scsiBlkDevCreate (pPhysDev, pPhysDev->numBlocks, 0)) == NULL)
	    {

			printErr("usrScsiConfig: scsiPhysDevCreate failed.\n",
				0, 0, 0, 0, 0, 0);

				return (ERROR);
	    }
#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 2) 
		xbdDevice = xbdBlkDevCreate (pBlk, pDevName); 
		mvOsPrintf("xbdBlkDevCreate return xbdDevice=0x%x\n", xbdDevice);
#else
#ifdef MV_NFS_SUPPORT
        if (nfsExported) 
            dosFsDevInitOptionsSet (DOS_OPT_EXPORT);
#endif /* MV_NFS_SUPPORT */

        if ((dosFsDevInit  (pDevName, pBlk, NULL) == NULL) )
	    {
			return (ERROR);
	    }
#endif /* (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR > 2)  */

#ifdef MV_NFS_SUPPORT
	if (nfsExported) 
        nfsExport (pDevName, 0, FALSE, 0);
#endif /* MV_NFS_SUPPORT */

	}


    return (OK);
    }


#endif
