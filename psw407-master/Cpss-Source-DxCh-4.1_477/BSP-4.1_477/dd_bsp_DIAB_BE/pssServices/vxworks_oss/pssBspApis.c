/*******************************************************************************
*                   Copyright 2002, GALILEO TECHNOLOGY, LTD.                   *
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
* (MJKK), GALILEO TECHNOLOGY LTD. (GTL) AND GALILEO TECHNOLOGY, INC. (GTI).    *
********************************************************************************
* pssBspApis.h - bsp APIs
*
* DESCRIPTION:
*       Enable managment of cache memory
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 4 $
*
*******************************************************************************/

#include "vxworks.h"
#include "config.h"
#include "intLib.h"
#include "cacheLib.h"
#include "db78XX0.h"
#include "drv/pci/pciConfigLib.h"
#include "syslib.h"
#include "mvTypes.h"
#include "mvIdma.h"
#include "mvTwsi.h"
#include "pssBspApis.h"
#include "sysPciIntCtrl.h"
#include "mvEthPhy.h" /* smi driver */
#include "mvBoardEnvSpec.h"


#undef MV_DEBUG         
#ifdef MV_DEBUG         
    #define DB(x) x
#else
    #define DB(x)
#endif


/*** reset ***/
/*******************************************************************************
* bspResetInit
*
* DESCRIPTION:
*       This routine calls in init to do system init config for reset.
*
* INPUTS:
*       none.
*
* OUTPUTS:
*       none.
*
* RETURNS:
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspResetInit
(
    MV_VOID
)
{
    return MV_OK;
}


/*******************************************************************************
* bspReset
*
* DESCRIPTION:
*       This routine calls to reset of CPU.
*
* INPUTS:
*       none.
*
* OUTPUTS:
*       none.
*
* RETURNS:
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspReset
(
    MV_VOID
)
{
    sysToMonitor(0);

    return  MV_OK;
}

/*** cache ***/
/*******************************************************************************
* bspCacheFlush
*
* DESCRIPTION:
*       Flush to RAM content of cache
*
* INPUTS:
*       type        - type of cache memory data/intraction
*       address_PTR - starting address of memory block to flush
*       size        - size of memory block
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspCacheFlush
(
    IN bspCacheType_ENT         cacheType,
    IN void                     *address_PTR,
    IN size_t                   size
)
{
    CACHE_TYPE cache;

    switch (cacheType)
    {
    case bspCacheType_InstructionCache_E:
        cache = INSTRUCTION_CACHE;
        break;

    case bspCacheType_DataCache_E:
        cache = DATA_CACHE;
        break;

    default:
        return MV_BAD_PARAM;
    }

    return cacheFlush(cache, address_PTR, size) ? MV_OK : MV_FAIL;
}

/*******************************************************************************
* bspCacheInvalidate
*
* DESCRIPTION:
*       Invalidate current content of cache
*
* INPUTS:
*       type        - type of cache memory data/intraction
*       address_PTR - starting address of memory block to flush
*       size        - size of memory block
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspCacheInvalidate
(
    IN bspCacheType_ENT         cacheType,
    IN void                     *address_PTR,
    IN size_t                   size
)
{
    CACHE_TYPE cache;

    switch (cacheType)
    {
    case bspCacheType_InstructionCache_E:
        cache = INSTRUCTION_CACHE;
        break;

    case bspCacheType_DataCache_E:
        cache = DATA_CACHE;
        break;

    default:
        return MV_BAD_PARAM;
    }

    return cacheInvalidate(cache, address_PTR, size) ? MV_OK : MV_FAIL;
}

/*** DMA ***/
/*******************************************************************************
* bspDmaWrite
*
* DESCRIPTION:
*       Write a given buffer to the given address using the Dma.
*
* INPUTS:
*       address     - The destination address to write to.
*       buffer      - The buffer to be written.
*       length      - Length of buffer in words.
*       burstLimit  - Number of words to be written on each burst.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*       1.  The given buffer is allways 4 bytes aligned, any further allignment
*           requirements should be handled internally by this function.
*       2.  The given buffer may be allocated from an uncached memory space, and
*           it's to the function to handle the cache flushing.
*       3.  The Prestera Driver assumes that the implementation of the DMA is
*           blocking, otherwise the Driver functionality might be damaged.
*
*******************************************************************************/
MV_STATUS bspDmaWrite
(
    IN  MV_U32  address,
    IN  MV_U32  *buffer,
    IN  MV_U32  length,
    IN  MV_U32  burstLimit
)
{
    MV_U32  sourceAddr;     /* The source address to begin the transfer from*/
    MV_U32  destAddr;       /* Destination address for the data to be       */
                            /* transfered.                                  */
    MV_U32    dmaEngine;  /* The Dma engine to perform the burst through. */
    MV_STATUS dmaStatus;

    MV_U32  numOfBursts = 0;

    /* Set the dma function parameters.     */
    dmaEngine   = 0;
    sourceAddr  = (MV_U32)buffer;
    destAddr    = address;

    /*osPrintf("src = 0x%x, dst = 0x%x, length = %d.\n",sourceAddr,destAddr,
             length);*/

    /* Wait until the Dma is Idle.          */
    while(mvDmaStateGet(dmaEngine) != MV_IDLE);

    /* Flush the buffer data    */
    bspCacheFlush(bspCacheType_DataCache_E, buffer, length * sizeof(MV_U32));

    numOfBursts = length / burstLimit;

    while ( numOfBursts )
    {
        dmaStatus = mvDmaTransfer(dmaEngine,sourceAddr,destAddr,burstLimit * 4, 0);
        if(dmaStatus != MV_OK)
        {
            return MV_FAIL;
        }

        sourceAddr += (burstLimit * 4);
        destAddr += (burstLimit * 4);
        numOfBursts--;

        /* Wait until the Dma is Idle.          */
        while(mvDmaStateGet(dmaEngine) != MV_IDLE);
    }

    return MV_OK;
}

/*******************************************************************************
* bspDmaRead
*
* DESCRIPTION:
*       Read a memory block from a given address.
*
* INPUTS:
*       address     - The address to read from.
*       length      - Length of the memory block to read (in words).
*       burstLimit  - Number of words to be read on each burst.
*
* OUTPUTS:
*       buffer  - The read data.
*
* RETURNS:
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*       1.  The given buffer is allways 4 bytes aligned, any further allignment
*           requirements should be handled internally by this function.
*       2.  The given buffer may be allocated from an uncached memory space, and
*           it's to the function to handle the cache flushing.
*       3.  The Prestera Driver assumes that the implementation of the DMA is
*           blocking, otherwise the Driver functionality might be damaged.
*
*******************************************************************************/
MV_STATUS bspDmaRead
(
    IN  MV_U32  address,
    IN  MV_U32  length,
    IN  MV_U32  burstLimit,
    OUT MV_U32  *buffer
)
{
    MV_U32  sourceAddr;     /* The source address to begin the transfer from*/
    MV_U32  destAddr;       /* Destination address for the data to be       */
                            /* transfered.                                  */
    MV_U32    dmaEngine;  /* The Dma engine to perform the burst through. */
    MV_STATUS dmaStatus;

    MV_U32  numOfBursts = 0;

    /* Set the dma function parameters.     */
    dmaEngine   = 1;
    sourceAddr  = address;
    destAddr    = (MV_U32)buffer;

    /*osPrintf("src = 0x%x, dst = 0x%x, length = %d.\n",sourceAddr,destAddr,
             length);*/

    /* Wait until the Dma is Idle.          */
    while(mvDmaStateGet(dmaEngine) != MV_IDLE);

    bspCacheFlush(bspCacheType_DataCache_E, buffer, sizeof(MV_U32) * length);
    bspCacheInvalidate(bspCacheType_DataCache_E, buffer, sizeof(MV_U32) * length);

    numOfBursts = length / burstLimit;

    while ( numOfBursts )
    {
        dmaStatus = mvDmaTransfer(dmaEngine,sourceAddr,destAddr,burstLimit * 4, 0);
        if(dmaStatus != MV_OK)
        {
            return MV_FAIL;
        }

        sourceAddr += (burstLimit * 4);
        destAddr += (burstLimit * 4);
        numOfBursts--;

        /* Wait until the Dma is Idle.          */
        while(mvDmaStateGet(dmaEngine) != MV_IDLE);
    }

    return MV_OK;
}



/*** PCI ***/

/*******************************************************************************
* bspPciConfigWriteReg
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
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspPciConfigWriteReg
(
    IN  MV_U32  busNo,
    IN  MV_U32  devSel,
    IN  MV_U32  funcNo,
    IN  MV_U32  regAddr,
    IN  MV_U32  data
)
{
    MV_STATUS ret;

    /* call bsp write function */
    ret = pciConfigOutLong(busNo, devSel, funcNo, regAddr, data);
    /* check whether success */
    if(ret != 0)
    {
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
* bspPciConfigReadReg
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
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspPciConfigReadReg
(
    IN  MV_U32  busNo,
    IN  MV_U32  devSel,
    IN  MV_U32  funcNo,
    IN  MV_U32  regAddr,
    OUT MV_U32  *data
)
{
    MV_STATUS ret;
    /* call bsp read function */
    ret = pciConfigInLong(busNo, devSel, funcNo, regAddr, (MV_U32 *)data);
    /* check whether success */
    if(ret != 0)
    {
        return MV_FAIL;
    }

    return MV_OK;
}

/*******************************************************************************
* bspPciFindDev
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
*       MV_OK   - on success,
*       MV_FAIL - othersise.
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspPciFindDev
(
    IN  MV_U16  vendorId,
    IN  MV_U16  devId,
    IN  MV_U32  instance,
    OUT MV_U32  *busNo,
    OUT MV_U32  *devSel,
    OUT MV_U32  *funcNo
)
{
    STATUS ret;
    /* call bsp read function */
    ret = pciFindDevice(vendorId, devId, instance, (int*)busNo,
                        (int*)devSel, (int*)funcNo);
    /* check whether success */
    if(ret != OK)
    {
        return MV_FAIL;
    }
    return MV_OK;
}

/*******************************************************************************
* bspPciGetIntVec
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
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/

extern int    have_idt_switch;
extern UINT16 lion_dev_id;

MV_STATUS bspPciGetIntVec
(
    IN  bspPciInt_PCI_INT  pciInt,
    OUT void               **intVec
)

{
#ifdef LION_RD

      switch (pciInt)
      {
      case     bspPciInt_PCI_INT_D:
        *intVec = (void *)INT_LVL_PEX00_INTA;
        break;
      case     bspPciInt_PCI_INT_C:
        *intVec = (void *)INT_LVL_PEX01_INTA;
        break;
      case     bspPciInt_PCI_INT_B:
        *intVec = (void *)INT_LVL_PEX02_INTA;
        break;
      case     bspPciInt_PCI_INT_A:
        *intVec = (void *)INT_LVL_PEX03_INTA;
        break;
      default:
        return MV_FAIL;
        break;
      }

#else
  int pp_core_number;
  UINT8 irq;
  int b, d, f;

  /* check parameters */
    if(intVec == NULL)
    {
        return MV_BAD_PARAM;
    }
    /* get the PCI interrupt vector */

	if (have_idt_switch)
    {
      /*
	    first pin of pex interface 4: 96 + 16 = 112  
		therefore we swizzle the interrupts accoring to the formula:

		irq_num = 112 + (PEX switch device nume)%4
      */
      
      pp_core_number = 4 - pciInt;
      bspPciFindDev(0x11ab, lion_dev_id, pp_core_number, 
                    (MV_U32 *)&b,
                    (MV_U32 *)&d,
                    (MV_U32 *)&f);

      /* interrupt line is set in mv_fix_pex */
      pciConfigInByte(b, d, f, PCI_CFG_DEV_INT_LINE, &irq);
      *intVec = (void *)(int)irq;
    }

    else
    {   
      switch (pciInt)
      {
      case     bspPciInt_PCI_INT_D:
        *intVec = (void *)112;
        break;
      case     bspPciInt_PCI_INT_C:
        *intVec = (void *)108;
        break;
      case     bspPciInt_PCI_INT_B:
        *intVec = (void *)104;
        break;
      case     bspPciInt_PCI_INT_A:
        *intVec = (void *)100;
        break;
      default:
        *intVec = (void *)96 ;
        break;
      }
    }
    DB(osPrintf(">>> in bspPciGetIntVec, pciInt=%d, returning %d\n", pciInt, (int)*intVec));
#endif /* LION_RD */
    return MV_OK;
}

/*******************************************************************************
* bspPciGetIntMask
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
*       MV_OK      - on success.
*       MV_FAIL    - otherwise.
*
* COMMENTS:
*       PCI interrupt mask should be used for interrupt disable/enable.
*
*******************************************************************************/
MV_STATUS bspPciGetIntMask
(
    IN  bspPciInt_PCI_INT  pciInt,
    OUT MV_U32             *intMask
)
{
    void        *intVec;

    /* check parameters */
    if(intMask == NULL)
    {
        return MV_BAD_PARAM;
    }
    /* get the PCI interrupt vector */
    bspPciGetIntVec(pciInt, &intVec);

    *intMask = (MV_U32)intVec;

    return MV_OK;
}

/*******************************************************************************
* bspPciEnableCombinedAccess
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
*       enWrCombine - MV_TRUE enables write requests combining.
*       enRdCombine - MV_TRUE enables read requests combining.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on sucess,
*       MV_NOT_SUPPORTED    - if the controller does not support this feature,
*       MV_FAIL             - otherwise.
*
* COMMENTS:
*       1.  Example for combined write scenario:
*           The controller is required to write a 32-bit data to address 0x8000,
*           while this transaction is still in progress, a request for a write
*           operation to address 0x8004 arrives, in this case the two writes are
*           combined into a single burst of 8-bytes.
*
*******************************************************************************/
MV_STATUS bspPciEnableCombinedAccess
(
    IN  MV_BOOL     enWrCombine,
    IN  MV_BOOL     enRdCombine
)
{
    if((enWrCombine == MV_TRUE) || (enRdCombine == MV_TRUE))
    {
        return MV_NOT_SUPPORTED;
    }

    return MV_OK;
}

extern void mv_symAdd(char *name, void *value);

void bspSymAdd(char *name, void *value)
{
  /* add name, value to symbol table */
  mv_symAdd(name, value);
}

/*******************************************************************************
* bspIntConnect
*
* DESCRIPTION:
*       Connect a specified C routine to a specified interrupt vector.
*
* INPUTS:
*       vector    - interrupt vector number to attach to
*       routine   - routine to be called
*       parameter - parameter to be passed to routine
*
* OUTPUTS:
*       None
*
* RETURNS:
*       MV_OK   - on success
*       MV_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
MV_STATUS bspIntConnect
(
    IN  MV_U32           vector,
    IN  MV_VOIDFUNCPTR   routine,
    IN  MV_U32           parameter
)
{
   int rc;
   DB(osPrintf(">>> in bspIntConnect, vector=%d\n", vector));
   rc = sysPciIntConnect(vector, routine, parameter);
   return (OK == rc) ? MV_OK : MV_FAIL;
}

/*******************************************************************************
* bspIntEnable
*
* DESCRIPTION:
*       Enable corresponding interrupt bits
*
* INPUTS:
*       intMask - new interrupt bits
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success
*       MV_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
MV_STATUS bspIntEnable
(
    IN MV_U32   intMask
)
{
  int rc;
    
   rc = sysPciIntEnable(intMask);
   
   return (OK == rc) ? MV_OK : MV_FAIL;
}

/*******************************************************************************
* bspIntDisable
*
* DESCRIPTION:
*       Disable corresponding interrupt bits.
*
* INPUTS:
*       intMask - new interrupt bits
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK   - on success
*       MV_FAIL - on error
*
* COMMENTS:
*       None
*
*******************************************************************************/
MV_STATUS bspIntDisable
(
    IN MV_U32   intMask
)
{
    int rc;
    rc = sysPciIntDisable(intMask);

    return (OK == rc) ? MV_OK : MV_FAIL;
}

/*** TWSI ***/
/*******************************************************************************
* bspTwsiInitDriver
*
* DESCRIPTION:
*       Init the TWSI interface 
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspTwsiInitDriver
(
    MV_VOID
)
{
  return MV_OK; 
}

/*******************************************************************************
* bspTwsiWaitNotBusy
*
* DESCRIPTION:
*       Wait for TWSI interface not BUSY
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspTwsiWaitNotBusy
(
    MV_VOID
)
{
	return MV_OK; /* Done by read-write operation */
}

/*******************************************************************************
* bspTwsiMasterReadTrans
*
* DESCRIPTION:
*       do TWSI interface Transaction 
*
* INPUTS:
*    devId - I2c slave ID                               
*    pData - Pointer to array of chars (address / data)
*    len   - pData array size (in chars).              
*    stop  - Indicates if stop bit is needed.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspTwsiMasterReadTrans
(
    IN MV_U8           devId,       /* I2c slave ID                              */ 
    IN MV_U8           *pData,      /* Pointer to array of chars (address / data)*/
    IN MV_U8           len,         /* pData array size (in chars).              */
    IN MV_BOOL         stop         /* Indicates if stop bit is needed in the end  */
)
{
    MV_TWSI_SLAVE twsiSlave;

    twsiSlave.slaveAddr.address = devId;
    twsiSlave.slaveAddr.type    = ADDR7_BIT;
    twsiSlave.validOffset       = FALSE;
    twsiSlave.offset            = 0;
    twsiSlave.moreThen256       = FALSE;

    return (mvTwsiRead(0, &twsiSlave, pData, len));
}

/*******************************************************************************
* bspTwsiMasterWriteTrans
*
* DESCRIPTION:
*       do TWSI interface Transaction 
*
* INPUTS:
*    devId - I2c slave ID                               
*    pData - Pointer to array of chars (address / data)
*    len   - pData array size (in chars).              
*    stop  - Indicates if stop bit is needed.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspTwsiMasterWriteTrans
(
    IN MV_U8           devId,       /* I2c slave ID                              */ 
    IN MV_U8           *pData,      /* Pointer to array of chars (address / data)*/
    IN MV_U8           len,         /* pData array size (in chars).              */
    IN MV_BOOL         stop         /* Indicates if stop bit is needed in the end  */
)
{
    MV_TWSI_SLAVE twsiSlave;

    twsiSlave.slaveAddr.address = devId;
    twsiSlave.slaveAddr.type    = ADDR7_BIT;
    twsiSlave.validOffset       = FALSE;
    twsiSlave.offset            = 0;
    twsiSlave.moreThen256       = FALSE;

    return (mvTwsiWrite(0, &twsiSlave, pData, len));
}



/************* SMI services *****************/

/*******************************************************************************
* bspSmiInitDriver
*
* DESCRIPTION:
*       Init the SMI interface
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       smiAccessMode - direct/indirect mode
*
* RETURNS:
*       MV_OK               - on success
*       MV_FAIL   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspSmiInitDriver
(
   bspSmiAccessMode_ENT  *smiAccessMode
)
{
    return MV_FAIL;
}

/*******************************************************************************
* bspSmiReadReg
*
* DESCRIPTION:
*       Reads a register from SMI slave.
*
* INPUTS:
*       devSlvId - Slave Device ID
*      actSmiAddr - actual smi addr to use (relevant for SX PPs)
*       regAddr - Register address to read from.
*
* OUTPUTS:
*       valuePtr     - Data read from register.
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspSmiReadReg
(
    IN  MV_U32  devSlvId,
    IN  MV_U32  actSmiAddr,
    IN  MV_U32  regAddr,
  OUT MV_U32 *valuePtr
)
{
  return MV_FAIL;
}

/*******************************************************************************
* bspSmiWriteReg
*
* DESCRIPTION:
*       Writes a register to an SMI slave.
*
* INPUTS:
*       devSlvId - Slave Device ID
*      actSmiAddr - actual smi addr to use (relevant for SX PPs)
*       regAddr - Register address to read from.
*       value   - data to be written.
*
* OUTPUTS:
*        None,
*
* RETURNS:
*       MV_OK               - on success
*       MV_ERROR   - on hardware error
*
* COMMENTS:
*
*******************************************************************************/
MV_STATUS bspSmiWriteReg
(
    IN MV_U32 devSlvId,
    IN MV_U32 actSmiAddr,
    IN MV_U32 regAddr,
  IN MV_U32 value
)
{
  return MV_FAIL;
}
/*** Ethernet Driver ***/
/*******************************************************************************
* bspEthPortRxInit
*
* DESCRIPTION: Init the ethernet port Rx interface
*
* INPUTS:
*       rxBufPoolSize   - buffer pool size
*       rxBufPool_PTR   - the address of the pool
*       rxBufSize       - the buffer requested size
*       numOfRxBufs_PTR - number of requested buffers, and actual buffers created
*       headerOffset    - packet header offset size
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthPortRxInit
(
    IN MV_U32           rxBufPoolSize,
    IN MV_U8*           rxBufPool_PTR,
    IN MV_U32           rxBufSize,
    INOUT MV_U32        *numOfRxBufs_PTR,
    IN MV_U32           headerOffset,
    IN MV_U32           rxQNum,
    IN MV_U32           rxQbufPercentage[]
)
{

    return MV_FAIL;
}

/*******************************************************************************
* bspEthPortTxInit
*
* DESCRIPTION: Init the ethernet port Tx interface
*
* INPUTS:
*       numOfTxBufs - number of requested buffers
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthPortTxInit
(
    IN MV_U32           numOfTxBufs
)
{
   return MV_FAIL;
}

/*******************************************************************************
* bspEthPortEnable
*
* DESCRIPTION: Enable the ethernet port interface
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthPortEnable
(
    MV_VOID
)
{
   return MV_FAIL;
}

/*******************************************************************************
* bspEthPortDisable
*
* DESCRIPTION: Disable the ethernet port interface
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthPortDisable
(
    MV_VOID
)
{
   return MV_FAIL;
}


/*******************************************************************************
* bspEthPortTx
*
* DESCRIPTION:
*       This function is called after a TxEnd event has been received, it passes
*       the needed information to the Tapi part.
*
* INPUTS:
*       segmentList     - A list of pointers to the packets segments.
*       segmentLen      - A list of segment length.
*       numOfSegments   - The number of segment in segment list.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthPortTx
(
    IN MV_U8*           segmentList[],
    IN MV_U32           segmentLen[],
    IN MV_U32           numOfSegments
)
{
   return MV_FAIL;
}

/*******************************************************************************
* bspEthInputHookAdd
*
* DESCRIPTION:
*       This bind the user Rx callback
*
* INPUTS:
*       userRxFunc - the user Rx callback function
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthInputHookAdd
(
    IN BSP_RX_CALLBACK_FUNCPTR    userRxFunc
)
{
   return MV_FAIL;
}

/*******************************************************************************
* bspEthTxCompleteHookAdd
*
* DESCRIPTION:
*       This bind the user Tx complete callback
*
* INPUTS:
*       userTxFunc - the user Tx callback function
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthTxCompleteHookAdd
(
    IN BSP_TX_COMPLETE_CALLBACK_FUNCPTR userTxFunc
)
{
   return MV_FAIL;
}

/*******************************************************************************
* bspEthRxPacketFree
*
* DESCRIPTION:
*       This routine frees the received Rx buffer. 
*
* INPUTS:
*       segmentList     - A list of pointers to the packets segments.
*       numOfSegments   - The number of segment in segment list.
*       queueNum        - Receive queue number
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       MV_OK if successful, or
*       MV_FAIL otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
MV_STATUS bspEthRxPacketFree
(
    IN MV_U8*           segmentList[],
    IN MV_U32           numOfSegments,
    IN MV_U32           queueNum
)
{
   return MV_FAIL;
}

