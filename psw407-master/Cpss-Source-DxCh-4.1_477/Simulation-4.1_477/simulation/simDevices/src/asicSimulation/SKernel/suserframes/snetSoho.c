/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetSoho.c
*
* DESCRIPTION:
*       This is a external API definition for SnetSoho module of SKernel.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 62 $
*
*******************************************************************************/
#include <os/simTypes.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/suserframes/snetSoho.h>
#include <asicSimulation/SKernel/smem/smemSoho.h>
#include <asicSimulation/SKernel/sohoCommon/sregSoho.h>
#include <asicSimulation/SKernel/suserframes/snetSohoEgress.h>
#include <asicSimulation/SKernel/skernel.h>
#include <common/Utils/SohoHash/smacHashSoho.h>
#include <asicSimulation/SKernel/sfdb/sfdbSoho.h>
#include <asicSimulation/SLog/simLog.h>

static GT_VOID snetSohoIngressPacketProc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);

static GT_VOID snetSohoSaLearning
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);

static GT_VOID snetSohoVlanAssign
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);

static GT_VOID snetSohoDaTranslation
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);

static GT_VOID snetSohoHeaderProc
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);

static GT_BOOL snetSohoCascadeFrame
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);

static GT_BOOL snetSohoCascadeRubyFrame
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);

static GT_VOID snetSohoVtuAtuLookUp
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);

static GT_VOID snetSohoIpPriorityAssign
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);

static GT_VOID snetSohoIgmpSnoop
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);

static GT_VOID snetSoho2IgmpSnoop
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);

static GT_VOID snetSohoPortMapUpdate
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);

static GT_VOID snetSohoPriorityAssign
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);

static GT_VOID snetSohoL2Decision
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);

static GT_VOID snetSohoL2RubyDecision
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);

static GT_VOID snetSohoStatCountUpdate
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);

static GT_VOID snetSohoAtuEntryGet
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_U32 address,
    OUT SNET_SOHO_ATU_STC * atuEntryPtr
);

static GT_VOID snetSohoQcMonitorIngress
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr,
    OUT SNET_SOHO_MIRROR_STC * mirrPtr
);

static GT_VOID snetSohoQcMonitorEgress
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr,
    OUT SNET_SOHO_MIRROR_STC * mirrPtr
);

static GT_VOID snetSohoFrameProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SBUF_BUF_ID bufferId,
    IN GT_U32 srcPort
);

static GT_VOID snetSohoLinkStateNotify
(
    IN SKERNEL_DEVICE_OBJECT      *     devObjPtr,
    IN GT_U32                           port,
    IN GT_U32                           linkState
);

static GT_VOID snetOpalPvtCollect
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);

static GT_U32 globLearnCnt; /* learn counter value */

#define SOHO_IS_ATU_ENTRY_STATIC(_atu_entry)     \
    ((SGT_MAC_ADDR_IS_MCST_BY_FIRST_BYTE(_atu_entry.macAddr.bytes[0])) ||  \
    (_atu_entry.entryState == 0xe) ||            \
    (_atu_entry.entryState == 0xf) )

/* Retrieve Request Format from frame data buffer */
#define RMT_TPYE(_frame_data_ptr)\
    (GT_U16)((_frame_data_ptr)[16] << 8 | (_frame_data_ptr)[17] )
/* Retrieve Request Format from frame data buffer */
#define REQ_FMT(_frame_data_ptr)\
    (GT_U16)((_frame_data_ptr)[18] << 8 | (_frame_data_ptr)[19] )

/* Retrieve Request Format from frame data buffer */
#define REQ_CDE(_frame_data_ptr)\
    (GT_U16)((_frame_data_ptr)[22] << 8 | (_frame_data_ptr)[23] )


/*******************************************************************************
*   snetSohoProcessInit
*
* DESCRIPTION:
*       Init module.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*
* COMMENTS:
*
*
*******************************************************************************/
void snetSohoProcessInit
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr
)
{
    devObjPtr->descriptorPtr =
        (void *)calloc(1, sizeof(SKERNEL_FRAME_SOHO_DESCR_STC));

    if (devObjPtr->descriptorPtr == 0)
    {
        skernelFatalError("smemSohoInit: allocation error\n");
    }

    /* initiation of internal soho function */
    devObjPtr->devFrameProcFuncPtr = snetSohoFrameProcess;
    devObjPtr->devPortLinkUpdateFuncPtr = snetSohoLinkStateNotify;
    devObjPtr->devFdbMsgProcFuncPtr = sfdbSohoMsgProcess;
    devObjPtr->devMacTblAgingProcFuncPtr = sfdbSohoMacTableAging ;
}

/*******************************************************************************
*   snetSohoLinkStateNotify
*
* DESCRIPTION:
*       Notify devices database that link state changed
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       port        - port number.
*       linkState   - link state (0 - down, 1 - up)
*
*******************************************************************************/
static GT_VOID snetSohoLinkStateNotify
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U32 port,
    IN GT_U32 linkState
)
{
    GT_U32 regAddress,swPortRegAddr;      /* Register's address */
    GT_U32 data ;
    SOHO_PORT_SPEED_MODE_E spd;         /* speed Number */

    /* Port status register */
    swPortRegAddr = SWITCH_PORT0_STATUS_REG + (port * 0x10000);
    /* Link */
    smemRegFldSet(devObjPtr, swPortRegAddr, 11, 1, linkState);

    /* PHY status */
    regAddress = PHY_STATUS_REG + (port * 0x10000);
    /* Link */
    smemRegFldSet(devObjPtr, regAddress, 2, 1, linkState);

    /* PHY port specific status */
    regAddress = PHY_PORT_STATUS_REG + (port * 0x10000);
    /* Link */
    smemRegFldSet(devObjPtr, regAddress, 10, 1, linkState);

    /* Port Speed */
    smemRegFldGet(devObjPtr, swPortRegAddr, 8, 2, (void *)&spd);

    /* The feature is applicable for Ruby and Opal only */
    if ( ((devObjPtr->deviceType != SKERNEL_RUBY) &&
          (devObjPtr->deviceType != SKERNEL_OPAL)) ||
          (spd == SOHO_PORT_SPEED_1000_MBPS_E) )
        return ;

    /* PHY port interrupt status */
    regAddress = PHY_INTERRUPT_STATUS_REG + (port * 0x10000);
    /* Interrupt */
    smemRegFldSet(devObjPtr, regAddress, 10, 1, 1);

    /* PHY port interrupt summary */
    /* port summary the same for all ports in HW, in SW used for port0 */
    smemRegFldSet(devObjPtr, PHY_INTERRUPT_PORT_SUM_REG, port, 1, 1);

    /* Global interrupt cause */
    smemRegFldSet(devObjPtr, GLB_STATUS_REG, 1, 1, 1);

    /* CALL INTERRUPT */

    /* Interrupt if enabled - global mask*/
    smemRegFldGet(devObjPtr, GLB_CTRL_REG, 1, 1, &data);
    if (data == 0)
        return ;

    /* Interrupt if enabled - PHY events mask*/
    regAddress = PHY_INTERRUPT_ENABLE_REG + (port * 0x10000);
    smemRegFldGet(devObjPtr, regAddress, 10, 1, &data);
    if (data == 0)
        return ;

    scibSetInterrupt(devObjPtr->deviceId);
}

/*******************************************************************************
*   snetSohoSaLearning
*
* DESCRIPTION:
*        SA learning process
* INPUTS:
*        devObjPtr  -  pointer to device object.
*        descrPtr   - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
static void snetSohoSaLearning
(
    IN SKERNEL_DEVICE_OBJECT   * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSohoSaLearning);

    GT_STATUS status;                   /* return status */
    SNET_SOHO_ATU_STC  atuEntry;        /* ATU database entry */
    GT_U8 isEntryMc = 0, isStatic = 0;  /* ATU database entry state*/
    GT_U32 regAddress;                  /* register's address */
    GT_U32 atuAddress = 0;              /* register's address */
    GT_U32 fldValue;                    /* register's field value */
    GT_U8 saOk = 0, doSa = 0;           /* SA learn flags */
    GT_U32 port;                        /* source port id */
    GT_U8 * srcMacPtr;                  /* src mac pointer */
    GT_U8 violation = 0;                /* ATU violation flag */
    GT_U8 updateDpv = 0;                /* update DPV flag */
    GT_U32 ports;                       /* device ports number */
    SOHO_DEV_MEM_INFO * memInfoPtr;     /* device's memory pointer */
    GT_U32 trunkIdx;                    /* trunk mask index */
    GT_U32 dpv;                         /* destination port vector */
    GT_U32  trunkId = 0;/* 1 based number , if 0 -- not trunk */
    GT_U32 portCtrlRegData;             /* Port Control Register's Data */
    GT_U32 pavRegData;                  /* Port Association Register's Data */
    GT_U32 portBaseVlanRegData;         /* Port Based vlan Register's Data */
    GT_U32 portAtuControlData;         /* Port Atu Control Register's Data */
    GT_U32 learnInterruptEn;
    GT_U32 refreshLock;                 /* refresh lock bit in SA learning */

    ports = devObjPtr->portsNumber;

    /* Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);

    srcMacPtr = SRC_MAC_FROM_DSCR(descrPtr);
    port = descrPtr->srcPort;

    regAddress = PORT_ASSOC_VECTOR_REG + (port * 0x10000);
    smemRegGet(devObjPtr, regAddress, &pavRegData);

    regAddress  = PORT_CTRL1_REG + port * 0x10000;
    smemRegGet(devObjPtr, regAddress, &portCtrlRegData);

    regAddress  = PORT_BASED_VLAN_MAP_REG + port * 0x10000;
    smemRegGet(devObjPtr, regAddress, &portBaseVlanRegData);

    regAddress  = PORT_ATU_CONTROL + port * 0x10000;
    smemRegGet(devObjPtr, regAddress, &portAtuControlData);

    /* enable interrupt NA message :    */
    learnInterruptEn = SMEM_U32_GET_FIELD(pavRegData, 0, ports);

    status = sfdbSohoAtuEntryAddress(devObjPtr, srcMacPtr, descrPtr->dbNum,
                                    &atuAddress);
    /* DPV */
    dpv = SMEM_U32_GET_FIELD(pavRegData, 0, ports);

    if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        /* Get load balancing vector */
        __LOG(("Get load balancing vector"));
        trunkIdx = (srcMacPtr[5] & 0x7) ^ (descrPtr->dstMacPtr[5] & 0x7);
        descrPtr->tpv = memInfoPtr->trunkMaskMem.trunkTblMem[trunkIdx] & 0x7ff;

        /* Trunk Port */
        if (descrPtr->srcTrunk)
        {
            trunkId = SMEM_U32_GET_FIELD(portCtrlRegData, 4, 4);
            dpv = trunkId;
        }
    }

    /* correct location for MC SA test FIGURE 16 ATU SA Processing */

    if (SGT_MAC_ADDR_IS_MCST(srcMacPtr))
    {
        return;
    }

    /* Was SA found */
    __LOG(("Was SA found"));
    if (status == GT_OK)
    {
        descrPtr->saHit = 1;

        snetSohoAtuEntryGet(devObjPtr, atuAddress, &atuEntry);

        if (SOHO_IS_ATU_ENTRY_STATIC(atuEntry))
        {
            doSa = 1;
            if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
            {/* Iis entry 's DPV all zeros */
                if (atuEntry.atuData == 0)
                {
                   descrPtr->saNoDpv = GT_TRUE;
                }
            }
        }
        else
        {
            fldValue = SMEM_U32_GET_FIELD(pavRegData, 13, 1);
             /* Locked port */
            __LOG(("Locked port"));
            if (fldValue)
            {
                if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
                {
                    refreshLock = SMEM_U32_GET_FIELD(pavRegData,11, 1);
                    if (refreshLock)
                    {
                        atuEntry.entryState = 0x7;
                    }
                }
                doSa = 1;
            }
            else
            {
                updateDpv = 1;
            }
        }

        if (doSa == 1)
        {
            /* Is this port's bit set */
            if ((atuEntry.atuData & (1 << port)) == 0)
            {
                /* Ignore wrong data */
                fldValue = SMEM_U32_GET_FIELD(pavRegData, 12, 1);
                if (!fldValue)
                {
                    /* Set member violation */
                    memInfoPtr->macDbMem.violation = SOHO_SRC_ATU_PORT_E;
                    violation = 1;
                }
                else
                {
                    saOk = 1;
                }
            }
            else
            {
                saOk = 1;
            }
        }
    }
    else  /* entry was not found */
    {

        /* init atuEntry  */
        memset(&atuEntry , 0 , sizeof(atuEntry));

        if (learnInterruptEn == 0)
        {
            /* the interrupts are not allowed !!! */
            violation = 0;
            /* the update of dpv is not allowed */
            updateDpv = 0;
        }
        else
        {
            fldValue = SMEM_U32_GET_FIELD(pavRegData, 13, 1);
            /* Locked port */
            if (fldValue)
            {
                /* Set miss violation */
                memInfoPtr->macDbMem.violation = SOHO_MISS_ATU_E;
                violation = 1;
            }


           if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
           {/* flag over limit reached address for opal plus */

               fldValue = SMEM_U32_GET_FIELD(pavRegData, 14, 1);
               if (fldValue)
               {/* flag over limit  address for opal plus */
                   descrPtr->overLimit = 0;
               }
           }


            status = sfdbSohoFreeBinGet(devObjPtr, srcMacPtr,
                                        descrPtr->dbNum, &atuAddress);
            /*  All entries static */
            if (status == GT_OK)
            {
                /* Set miss violation */
                memInfoPtr->macDbMem.violation = SOHO_FULL_ATU_E;
                violation = 1;
            }
            else
            {
                updateDpv = 1;
            }
        }
    }

    if (violation)
    {
        GT_U16 * wordDataPtr;

        wordDataPtr = memInfoPtr->macDbMem.violationData;
        memset(wordDataPtr, 0, 5 * sizeof(GT_U16));

        /* DBNum */
        wordDataPtr[0] = (GT_U16)descrPtr->dbNum;

        /* SPID, even if the packet is received on trunk - the incoming port is*
         *          value that should be configured                           */
        wordDataPtr[1] = (GT_U16)descrPtr->srcPort;

        /* SA set - force swapped bytes order as documented */
        wordDataPtr[2] = srcMacPtr[1] | (srcMacPtr[0] << 8);

        wordDataPtr[3] = srcMacPtr[3] | (srcMacPtr[2] << 8);

        wordDataPtr[4] = srcMacPtr[5] | (srcMacPtr[4] << 8);


        /* set ATU Violation - no matter if interrupt enabled,
        * status register is updated
        */
        smemRegFldSet(devObjPtr, GLB_STATUS_REG, 3, 1, 1);
        /* check that interrupt enabled */
        smemRegFldGet(devObjPtr, GLB_CTRL_REG, 3, 1, &fldValue);

        if (fldValue)
        {
            /* ATU problem interrupt */
            scibSetInterrupt(devObjPtr->deviceId);
        }
        return;
    }


    /* Update DPV for station move */
    if (updateDpv)
    {
        fldValue = SMEM_U32_GET_FIELD(pavRegData, 13, 1);
         /* Locked port */
        if (fldValue == 0)
        {
            /* PAV */
            if (dpv == 0)
            {
                atuEntry.entryState = 0;
            }
            else
            {
                /* Update PAV for ATU learning */
                atuEntry.entryState = 0x7;
            }
            /* Set SA and Entry_State */
            /* word 0 */
            regAddress = atuAddress;
            fldValue = (srcMacPtr[5]) |
                       (srcMacPtr[4] << 8) |
                       (srcMacPtr[3] << 16) |
                       (srcMacPtr[2] << 24);

            descrPtr->atuEntry[0] = fldValue;
            descrPtr->atuEntryAddr = regAddress;
           /*
            It should be stored only in the end of ingress pipe
            after all bridge decisions
            smemRegSet(devObjPtr, regAddress, fldValue);
            */

            /* word 1
              regAddress = atuAddress + 0x4;
            */

            /* SA bytes */
            fldValue = srcMacPtr[1] |
                       (srcMacPtr[0] << 8);

            /* Entries state */
            fldValue |= (atuEntry.entryState << 16);

            /* Destination Ports Vector */
            fldValue |= (dpv << 20);

            descrPtr->atuEntry[1] = fldValue;
         /* It should be stored only in the end of ingress pipe
            after all bridge decisions
            smemRegSet(devObjPtr, regAddress, fldValue);
        */
        }
        if ((devObjPtr->deviceType == SKERNEL_RUBY) ||
            (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType)))
        {
            /* New data */
            if (descrPtr->saHit == 0)
            {
                smemRegFldGet(devObjPtr, GLB_ATU_CTRL_REG, 3, 1, &fldValue);
                /* Learn2All */
                if (fldValue)
                {
                    descrPtr->saUpdate = 1;
                }
            }
        }
        saOk = 1;
    }

    if (saOk)
    {
        isEntryMc = atuEntry.entryState;

        if (isEntryMc)
        {
            /* Use DA ATU priority */
            if (descrPtr->Mgmt)
            {
                descrPtr->priorityInfo.sa_pri.useSaPriority =
                    (atuEntry.entryState == 0xE);
            }
            else
            {
                descrPtr->priorityInfo.sa_pri.useSaPriority =
                    (atuEntry.entryState == 0xF);
            }
            descrPtr->notRateLimit = (atuEntry.entryState == 0x5);
        }
        else
        if (descrPtr->Mgmt)
        {
            descrPtr->priorityInfo.sa_pri.useSaPriority =
                  (atuEntry.entryState == 0xD);
        }
        else
        if (isStatic)
        {

            if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
            {/* Use SA ATU priority */
            descrPtr->priorityInfo.sa_pri.useSaPriority =
                (atuEntry.entryState == 0);
            }
            else
            /* Use SA ATU priority */
            descrPtr->priorityInfo.sa_pri.useSaPriority =
                (atuEntry.entryState == 0xF);
        }
        else
        {
            descrPtr->priorityInfo.sa_pri.useSaPriority = 0;
        }
        if (descrPtr->priorityInfo.sa_pri.useSaPriority != 0)
        {
            descrPtr->priorityInfo.sa_pri.useSaPriority = 1;
            descrPtr->priorityInfo.sa_pri.saPriority = atuEntry.priority;
        }
    }
}

/*******************************************************************************
*   snetSohoMacHashCalc
*
* DESCRIPTION:
*        Calculates the hash index for the mac address table
*
* INPUTS:
*        macAddrPtr  -  pointer to the first byte of MAC address.
*
* RETURNS:
*        The hash index
*******************************************************************************/
GT_U32 snetSohoMacHashCalc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN GT_U8 * macAddrPtr
)
{
    GT_U32  hushIdx;

    sohoMacHashCalc((GT_ETHERADDR *)macAddrPtr, &hushIdx);
    if (!SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        /* the SOHO-1 devices support only 10 bits of bucket index */
        /* the SOHO-2 devices support 11 bits of bucket index */

        hushIdx &= 0x3ff;/* mask with 10 bits */
    }

    return hushIdx;
}

/*******************************************************************************
*   snetSohoVlanAssign
*
* DESCRIPTION:
*       Ingress VLAN processing
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       descrPtr        - pointer to the frame's descriptor.
*
* OUTPUTS:
*
* RETURNS:
*
* COMMENTS: 0 = Port is a member of this VLAN and frames are to egress unmodified
*           1 = Port is a member of this VLAN and frames are to egress Untagged
*           2 = Port is a member of this VLAN and frames are to egress Tagged
*           3 = Port is not a member of this VLAN*
*
*******************************************************************************/
static GT_VOID snetSohoVlanAssign
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSohoVlanAssign);

    GT_U32  regAddr, fldValue,fldValue2;/* register's address and field value */
    SNET_SOHO_VTU_STC vtuEntry;         /* VTU database entry */
    GT_STATUS status;                   /* return status */
    GT_U8 violation = 0;                /* ATU violation flag */
    SOHO_DEV_MEM_INFO * memInfoPtr;     /* device's memory pointer */

    /*Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);

    regAddr = PORT_BASED_VLAN_MAP_REG + descrPtr->srcPort * 0x10000;
    smemRegFldGet(devObjPtr, regAddr, 12, 4, &fldValue);
    /* Start with default database */
    if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        regAddr = PORT_CTRL1_REG + descrPtr->srcPort * 0x10000;
        if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
            smemRegFldGet(devObjPtr, regAddr, 0, 8, &fldValue2);
        else
            smemRegFldGet(devObjPtr, regAddr, 0, 4, &fldValue2);
        fldValue = (fldValue <<4) | fldValue2;

    }
    descrPtr->dbNum = fldValue;
    /* Start with default VTU vector */
    __LOG(("Start with default VTU vector"));
    descrPtr->vtuHit = 0;

    if (!SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        regAddr = PORT_CTRL_2_REG + descrPtr->srcPort * 0x10000;
        smemRegFldGet(devObjPtr, regAddr, 10, 2, &fldValue);
        /* 802.1q disabled */
        if (fldValue == 0)
        {
            return;
        }
    }
    else
    {
        descrPtr->vtuVector = 0x7ff;
    }

    status = smemSohoVtuEntryGet(devObjPtr, descrPtr->vid, &vtuEntry);
    if (status == GT_NOT_FOUND)
    {
        descrPtr->vtuMiss = 1;
        /* Set miss violation */
        memInfoPtr->vlanDbMem.violation = SOHO_MISS_VTU_VID_E;
        violation = 1;
    }
    else
    {
        /* Is SPID member */
        if ((vtuEntry.portsMap[descrPtr->srcPort] & 0x3) == 3)
        {
            /* Set member violation */
            memInfoPtr->vlanDbMem.violation = SOHO_SRC_VTU_PORT_E;
            violation = 1;
        }
        else
            descrPtr->spOk = 1;

        descrPtr->vtuHit = 1;

        if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
        {
             descrPtr->dbNum = vtuEntry.dbNum;
             /* CALCULATE SBIT VALUE */
             descrPtr->sBit = vtuEntry.sid;

             descrPtr->policyVid = vtuEntry.vidPolicy;
        }
        else
        {
            descrPtr->dbNum = vtuEntry.dbNum;
        }

        SOHO_MEMBER_TAG_2_VTU_VECTOR(vtuEntry.portsMap, descrPtr->vtuVector);

        regAddr  = PORT_CTRL_REG + descrPtr->srcPort * 0x10000;
        smemRegFldGet(devObjPtr, regAddr, 0, 2, &fldValue);
        /* PortState */
        if (fldValue == 0)
        {
            descrPtr->vtuVector &= ~(1 << descrPtr->srcPort);
        }
        descrPtr->priorityInfo.vtu_pri.useVtuPriority = (GT_U8)(vtuEntry.pri & 0x8);
        descrPtr->priorityInfo.vtu_pri.vtuPriority = (GT_U8)(vtuEntry.pri & 0x7);
    }

    if (violation)
    {
        GT_U16 * wordDataPtr;

        wordDataPtr = &memInfoPtr->vlanDbMem.violationData[0];
        memset(wordDataPtr, 0, 5 * sizeof(GT_U16));

        /* SPID */
        wordDataPtr[0] = (GT_U16)descrPtr->srcPort;

        /* VID */
        wordDataPtr[1] = descrPtr->vid;

        /* check that interrupt enabled */
        smemRegFldGet(devObjPtr, GLB_CTRL_REG, 5, 1, &fldValue);
        if (fldValue)
        {
            /* VTU problem interrupt */
            smemRegFldSet(devObjPtr, GLB_STATUS_REG, 5, 1, 1);
            scibSetInterrupt(devObjPtr->deviceId);
        }
        return;
    }
}

/*******************************************************************************
*   snetSohoFrameProcess
*
* DESCRIPTION:
*       Process the frame, get and do actions for a frame
*
* INPUTS:
*       devObjPtr    - pointer to device object.
*       bufferId     - frame data buffer Id
*       srcPort      - source port number
*
*
*******************************************************************************/
static GT_VOID snetSohoFrameProcess
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SBUF_BUF_ID bufferId,
    IN GT_U32 srcPort
)
{
    DECLARE_FUNC_NAME(snetSohoFrameProcess);

    SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr; /* pointer to frame's descriptor */
    SNET_SOHO_MIRROR_STC mirrStc;

    descrPtr = (SKERNEL_FRAME_SOHO_DESCR_STC *)devObjPtr->descriptorPtr;
    memset(descrPtr, 0, sizeof(SKERNEL_FRAME_SOHO_DESCR_STC));
    memset(&mirrStc, 0, sizeof(SNET_SOHO_MIRROR_STC));

    descrPtr->frameBuf = bufferId;
    descrPtr->srcPort = srcPort;
    descrPtr->byteCount = (GT_U16)bufferId->actualDataSize;

    /* Ingress packet rules for incoming packet */
    __LOG(("Ingress packet rules for incoming packet"));
    snetSohoIngressPacketProc(devObjPtr, descrPtr);

    /* QC Monitor - Ingress and Egress */
    snetSohoQcMonitorIngress(devObjPtr, descrPtr , &mirrStc);
    snetSohoQcMonitorEgress(devObjPtr, descrPtr  , &mirrStc);
}

/*******************************************************************************
*   snetSohoIngressPacketProc
*
* DESCRIPTION:
*       Ingress packet rules for incoming packets
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static GT_VOID snetSohoIngressPacketProc
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    snetSohoHeaderProc(devObjPtr, descrPtr);
    snetSohoIpPriorityAssign(devObjPtr, descrPtr);
    snetSohoIgmpSnoop(devObjPtr, descrPtr);
    snetSohoPortMapUpdate(devObjPtr, descrPtr);
    snetSohoPriorityAssign(devObjPtr, descrPtr);
    snetSohoL2Decision(devObjPtr, descrPtr);
    snetSohoStatCountUpdate(devObjPtr, descrPtr);
}
/*******************************************************************************
*   snetSohoDaTranslation
*
* DESCRIPTION:
*       Destination address translation process
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static GT_VOID snetSohoDaTranslation
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSohoDaTranslation);

    GT_U8   isEntryMng;                 /* Is ATU entry management */
    GT_STATUS status;                   /* return status */
    SNET_SOHO_ATU_STC  atuEntry;        /* ATU database entry */
    GT_U8 isEntryMc = 0, isStatic = 0;  /* ATU database entry state*/
    GT_U32 regAddress;                  /* register's address */
    GT_U32 fldValue;                    /* register's field value */
    GT_U32 ports;                       /* device ports number */
    SOHO_DEV_MEM_INFO * memInfoPtr;     /* device's memory pointer */
    GT_U32 atuData = 0;                 /* Destination port vector/TrunkID */
    GT_U32 trunkIdx;                    /* trunk mask index */

    ports = devObjPtr->portsNumber;
    /* Get pointer to the device memory */
    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);

    status = sfdbSohoAtuEntryAddress(devObjPtr, descrPtr->dstMacPtr,
                                     descrPtr->dbNum, &regAddress);
    if (status != GT_OK)
    {
        return;
    }

    descrPtr->daHit = 1;
    snetSohoAtuEntryGet(devObjPtr, regAddress, &atuEntry);


    /* DA found */
    __LOG(("DA found"));
    isEntryMng = ( ((SGT_MAC_ADDR_IS_MCST_BY_FIRST_BYTE(atuEntry.macAddr.bytes[0]) &&
                    ((atuEntry.entryState & 0x7) == 0x6)) ) ||
                 (((atuEntry.entryState == 0xc) ||
                   (atuEntry.entryState == 0xd)) &&
                  (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType)) ));


    atuData = atuEntry.atuData;

    if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        if ((atuEntry.trunk) &&
            (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType)))
        {
            /* TRNUK MAP */
            __LOG(("TRNUK MAP"));
            smemRegFldGet(devObjPtr, GLB2_TRUNK_MASK_REG,0,11, &trunkIdx);
            smemRegFldGet(devObjPtr, GLB2_TRUNK_ROUT_REG,0,11, &atuData);
            atuData = atuData & trunkIdx;

        }
        else if (atuEntry.trunk)
        {
            trunkIdx = atuData & 0xf;
            atuData = memInfoPtr->trunkRouteMem.trouteTblMem[trunkIdx] & 0x7ff;
        }
    }

    if (isEntryMng)
    {
        descrPtr->Mgmt = 1;
    }
    else
    {
        /* Is entry MC */
        __LOG(("Is entry MC"));
        if (SGT_MAC_ADDR_IS_MCST_BY_FIRST_BYTE(atuEntry.macAddr.bytes[0]))
        {
            isEntryMc = 1;
        }
        else
        /* Is entry static */
        if (atuEntry.entryState & 0xe)
        {
            isStatic = 1;
            descrPtr->daStatic = 1;
        }
    }
    if (isEntryMc)
    {
        if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
        {
            /* Use DA ATU priority */
            __LOG(("Use DA ATU priority"));
            if (descrPtr->Mgmt)
            {
                descrPtr->priorityInfo.da_pri.useDaPriority =
                    (atuEntry.entryState == 0x6);
            }
            else
            {
                descrPtr->priorityInfo.da_pri.useDaPriority = 0;
            }

                descrPtr->policyDa = (atuEntry.entryState == 0x4);

        }
        else
        {
            /* Use DA ATU priority */
            __LOG(("Use DA ATU priority"));
            if (descrPtr->Mgmt)
            {
                descrPtr->priorityInfo.da_pri.useDaPriority =
                    (atuEntry.entryState == 0xF);
            }
            else
            {
                descrPtr->priorityInfo.da_pri.useDaPriority = 0;
            }

        }
        descrPtr->notRateLimit = (atuEntry.entryState == 0x5);
    }
    else if (descrPtr->Mgmt)
    {
        descrPtr->priorityInfo.da_pri.useDaPriority =
              (atuEntry.entryState == 0xD);
    }
    if (isStatic)
    {
        /* Use DA ATU priority */
        descrPtr->priorityInfo.da_pri.useDaPriority =
            (atuEntry.entryState == 0xE);

        if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
        {
            /* Use DA ATU priority */
            if (descrPtr->Mgmt)
            {
                descrPtr->priorityInfo.da_pri.useDaPriority =
                    (atuEntry.entryState == 0x6);
            }
            else
            {
                descrPtr->priorityInfo.da_pri.useDaPriority = 0;
            }

                descrPtr->policyDa = (atuEntry.entryState == 0x4);
                descrPtr->notRateLimit = (atuEntry.entryState == 0x5);

        }
    }
    else
    {
        descrPtr->priorityInfo.da_pri.useDaPriority = 0;
    }
    if (descrPtr->priorityInfo.da_pri.useDaPriority != 0)
    {
        descrPtr->priorityInfo.da_pri.daPriority =
         (GT_U8)(SMEM_U32_GET_FIELD(atuEntry.atuData, 11, 1) |
                 SMEM_U32_GET_FIELD(atuEntry.atuData, 10, 2) << 1);
    }
    if (!SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        regAddress = PORT_ASSOC_VECTOR_REG + (descrPtr->srcPort * 0x10000);
        smemRegFldGet(devObjPtr, regAddress, 15, 1, &fldValue);
         /* SPID ingress monitor */
        if (fldValue)
        {
            /* Mask the SPID's bit in the SPID's PAV */
            atuData &= ~(1 << descrPtr->srcPort);
            smemRegFldGet(devObjPtr, regAddress, 0, ports, &fldValue);
            fldValue &= ~(1 << descrPtr->srcPort);
            atuData |= fldValue;
        }
    }

    descrPtr->destPortVector = atuData;
}

/*******************************************************************************
*   snetSohoAtuEntryGet
*
* DESCRIPTION:
*       Format 64-bit ATU entry to  SNET_SOHO_ATU_STC structure
*
* INPUTS:
*       devObjPtr       - pointer to device object.
*       atuAddress      - ATU entry address in external SRAM
*       atuEntryPtr     - pointer to SNET_SOHO_ATU_STC structure.
*
*
*******************************************************************************/
static GT_VOID snetSohoAtuEntryGet
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN GT_U32 atuAddress,
    OUT SNET_SOHO_ATU_STC * atuEntryPtr
)
{
    GT_U32 * atuWordPtr;                /* ATU word pointer */

    ASSERT_PTR(atuEntryPtr);

    /* Pointer to ATU word in SRAM */
    atuWordPtr = smemMemGet(devObjPtr, atuAddress);
    SOHO_MSG_2_MAC((GT_U8 *)atuWordPtr, atuEntryPtr->macAddr);
    atuEntryPtr->entryState =
        (GT_U8) SMEM_U32_GET_FIELD(atuWordPtr[1], 16, 4);

    /* Sapphire/Ruby */
    if (devObjPtr->deviceType == SKERNEL_SAPPHIRE)
    {
        atuEntryPtr->atuData =
            (GT_U16)SMEM_U32_GET_FIELD(atuWordPtr[1], 20, 10);
        atuEntryPtr->priority =
            (GT_U8) SMEM_U32_GET_FIELD(atuWordPtr[1], 30, 1) << 1 |
            (GT_U8) SMEM_U32_GET_FIELD(atuWordPtr[2],  0, 1);
    }
    else if(SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        /* 11 bits of ports bmp / trunkId(0 based) */
        atuEntryPtr->atuData =
            (GT_U16)SMEM_U32_GET_FIELD(atuWordPtr[1], 20, 11);

        atuEntryPtr->priority =
                (GT_U8) SMEM_U32_GET_FIELD(atuWordPtr[1], 31, 1) |      /* LSB */
                (GT_U8) SMEM_U32_GET_FIELD(atuWordPtr[2], 0, 2) << 1;   /* MSB */
        atuEntryPtr->trunk =
                (GT_U8) SMEM_U32_GET_FIELD(atuWordPtr[2], 2, 1) ;
        if ((SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType)))
        {
            atuEntryPtr->fid =
                (GT_U8) SMEM_U32_GET_FIELD(atuWordPtr[2], 3, 1) ;
        }
    }
    else
    {
        atuEntryPtr->atuData =
            (GT_U16)SMEM_U32_GET_FIELD(atuWordPtr[1], 20, 11);
        atuEntryPtr->priority =
            (GT_U8) SMEM_U32_GET_FIELD(atuWordPtr[1], 31, 1) |      /* LSB */
            (GT_U8) SMEM_U32_GET_FIELD(atuWordPtr[2], 0, 2) << 1;   /* MSB */
        atuEntryPtr->trunk = 0;

    }
}


/*******************************************************************************
*   snetSohoCreateMarvellTag
*
* DESCRIPTION:
*       Get Marvell tag from descriptor
*
* INPUTS:
*       devObjPtr     - pointer to device object.
*       descrPtr      - description pointer
*
* OUTPUTS:
*       Returns 32 bits Marvell TAG
*
*******************************************************************************/
GT_U32 snetSohoCreateMarvellTag
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    INOUT SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSohoCreateMarvellTag);

    GT_U32 mrvlTag;
    GT_U32 mrvlTagCmd = 0;
    GT_U8  cpu_code=0;

    if (descrPtr->rxSnif)
    {
        descrPtr->srcTrunk = 1;
        mrvlTagCmd =  TAG_CMD_TO_TARGET_SNIFFER_E;
    }
    else
    if (descrPtr->txSnif)
    {
        mrvlTagCmd =  TAG_CMD_TO_TARGET_SNIFFER_E;
    }
    else
    if (descrPtr->pktCmd == SKERNEL_PKT_FORWARD_E)
    {
       mrvlTagCmd = TAG_CMD_FORWARD_E;
    }
    else
    if (descrPtr->pktCmd == SKERNEL_PKT_TRAP_CPU_E)
    {
       mrvlTagCmd = TAG_CMD_TO_CPU_E;
       if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
       {
           if (descrPtr->Mgmt)
           {
                if (descrPtr->arp)
                {
                    cpu_code = 0x4;
                }
                else
                {
                    cpu_code = 0x0;
                }
           }
           else if (descrPtr->daType.igmp)
           {
                cpu_code = 0x2;
           }
       }
       else
       {
           if (descrPtr->Mgmt)
           {
                cpu_code=0x2;
           }
           else if (descrPtr->daType.igmp)
           {
                cpu_code=0x6;
           }
       }
    }

    mrvlTag = mrvlTagCmd << 30;

    if (mrvlTagCmd == TAG_CMD_FORWARD_E)
    {
        /* Src_Tagged */
        __LOG(("Src_Tagged"));
        mrvlTag |= ((descrPtr->srcVlanTagged & 0x01) << 29);

        /* Src Dev */
        mrvlTag |= ((descrPtr->srcDevice & 0x1F) << 24);
        if (descrPtr->srcTrunk)
        {
            mrvlTag |= (1 << 18);
            /* Src_trunk */
            mrvlTag |= ((descrPtr->trunkId & 0x0F) << 19);
        }
        else
        {
            /* Src_Port */
            mrvlTag |= ((descrPtr->srcPort & 0x1F) << 19);
        }
    }

    if (mrvlTagCmd == TAG_CMD_TO_CPU_E)
    {


        if (descrPtr->rmtMngmt)
        {
            /* Src_Tagged replace with 0! */
            mrvlTag |= ( 0x00 << 29);
            /* Src Dev */
            mrvlTag |= ((descrPtr->srcDevice & 0x1F) << 24);
            /* Second octet = 0 */
            mrvlTag |= (0  << 16);
            /* remote management bit  */
            mrvlTag |= ( 0x1 << 12);
            /*   Sequnence + tag DSA 0XF vid and pri will be added later */
        }
        else
        {
        /* Src_Tagged */
        mrvlTag |= ((descrPtr->srcVlanTagged & 0x01) << 29);

        /* Src Dev */
        mrvlTag |= ((descrPtr->srcDevice & 0x1F) << 24);

        /* Src_Port */
        mrvlTag |= ((descrPtr->srcPort & 0x1F) << 19);
        if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
        {
            /* CPU_Code[2:1] : The '>>' operator is a must in the
                               expression '(cpu_code >> 1) & 0x3)' .
                               Only bits 1 , 2 are saved in the 3rd
                               Octet of the marvell tag .
                               The first bit(0) is saved on the 4th octet.
                               Tuvia(21-Aug-05)
            */
            mrvlTag |= ( ((cpu_code >> 1) & 0x3) << 17);
        }
        else
        {
            /* CPU_Code[3:1] */
            mrvlTag |= ( ((cpu_code >> 1) & 0x7) << 16);
        }
        /* CPU_Code[0] **** vid and pri will be added later */
        mrvlTag |= ( (cpu_code & 0x1) << 12);
    }
    }

    if (mrvlTagCmd == TAG_CMD_TO_TARGET_SNIFFER_E)
    {
        /* rx_sniff */
        mrvlTag |= ((descrPtr->rxSnif & 0x01) << 18);
    }

    if ((SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType) &&
        (descrPtr->rmtMngmt == 0)))
    {
        /* Frame's CFI */
        __LOG(("Frame's CFI"));
        mrvlTag |= (descrPtr->mcfi & 0x1) << 16;
    }
    /* add vlan tag with vpt.
      if descrPtr->rmtMngmt is TRUE then the vid field is of the form 0xfXX
      where XX is the RMU 8 bits sequence number.
    */
    mrvlTag |= ((descrPtr->vid & 0xfff) | ((descrPtr->fPri & 0x7) << 13));

    return SGT_LIB_SWAP_BYTES_AND_WORDS_MAC(mrvlTag);
}

/*******************************************************************************
*   snetSohoHeaderProc
*
* DESCRIPTION:
*       Header processing, DA and SA Capture and Double Tag Removal
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
* OUTPUT:
*
* COMMENT:
*******************************************************************************/
static GT_VOID snetSohoHeaderProc
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSohoHeaderProc);

    GT_U32 regAddress;                  /* Register's address */
    GT_U32 headerState;
    SBUF_BUF_STC * frameBufPtr;
    GT_U32  cascadedPort;
    GT_U32  dbNum=0;
    GT_U32  learnDis=0;
    GT_U32  ignoreFcs=0;
    GT_U32  vlanTable=0;
    GT_U8 * headerPtr;
    GT_U32  regAddr;                    /* Register's address */
    GT_U32* regValPtr;                  /* Register's value pointer */
    GT_U32  fldValue;                   /* Register's field value */
    GT_U32 ports;                       /* ports number */
    GT_U8  doubleTag = 0;               /* double tag flag */

    ports = devObjPtr->portsNumber;

    regAddress  = PORT_CTRL_REG + descrPtr->srcPort * 0x10000;
    regValPtr = smemMemGet(devObjPtr, regAddress);

    /* port in header mode */
    headerState = SMEM_U32_GET_FIELD(regValPtr[0], 11, 1);
    frameBufPtr = descrPtr->frameBuf;
    headerPtr = frameBufPtr->actualDataPtr;

    /* Set byte count from actual buffer's length */
    descrPtr->byteCount = (GT_U16)frameBufPtr->actualDataSize;
    if (headerState)
    {
        /* write the header to the port based vlan register */
        __LOG(("write the header to the port based vlan register"));
        if ( (headerPtr[0] != 0) && (headerPtr[1] != 0) )
        {
            dbNum =  (frameBufPtr->actualDataPtr[0] >> 4) & 0xf;
            SMEM_U32_SET_FIELD(regValPtr[0], 12,  4, dbNum);

            learnDis = (headerPtr[0] >> 3) & 0x1;
            SMEM_U32_SET_FIELD(regValPtr[0], 11,  1, learnDis);

            if (devObjPtr->deviceType == SKERNEL_SAPPHIRE)
            {
                ignoreFcs = (headerPtr[0] >> 2) & 0x1;
                SMEM_U32_SET_FIELD(regValPtr[0], 10,  1, ignoreFcs);
                vlanTable = ( (headerPtr[0] & 0x3) << 8) | headerPtr[1];
            }
            else
            {
                vlanTable = ( (headerPtr[0] & 0x7) << 8) | headerPtr[1];
            }

            regAddr = PORT_BASED_VLAN_MAP_REG + descrPtr->srcPort * 0x10000;
            smemRegFldSet(devObjPtr, regAddr,  0, ports, vlanTable);
        }
        /* remove ingress header */
        headerPtr+=2;
        descrPtr->byteCount-=2;
    }

    /* Set destination MAC pointer */
    descrPtr->dstMacPtr = frameBufPtr->actualDataPtr;
    if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        if (SGT_MAC_ADDR_IS_MGMT(descrPtr->dstMacPtr))
        {
            smemRegFldGet(devObjPtr, GLB2_MNG_REG, 3, 1, &fldValue);
            /* Rsvd2CPU */
            __LOG(("Rsvd2CPU"));
            if (fldValue)
            {
                GT_U8 daBits;
                daBits = descrPtr->dstMacPtr[5] & 0xf;
                smemRegFldGet(devObjPtr, GLB2_MGMT_EN_REG, 0, 16, &fldValue);
                if (fldValue & (1 << daBits))
                {
                    descrPtr->Mgmt = 1;
                }
                else if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
                {
                    smemRegFldGet(devObjPtr, GLB2_MGMT_EN_REG_2X, 0, 16, &fldValue);
                    if (fldValue & (1 << daBits))
                    {
                        descrPtr->Mgmt = 1;
                    }
                }
            }
            if (SGT_MAC_ADDR_IS_PAUSE(descrPtr->dstMacPtr))
            {
              descrPtr->daType.pause = 1;
            }
        }
    }
    else
    {
        GT_U32 etherType, etherTypeOffset;

        /* check if the frame is igmp/mld */
        if (SGT_MAC_ADDR_IS_MLD(descrPtr->dstMacPtr))
        {
          descrPtr->daType.mld = 1;
        }
        /* ------- check for IGMP frame ------- */
        __LOG(("------- check for IGMP frame -------"));
        if (descrPtr->srcVlanTagged == 1)
        {
            etherTypeOffset = 16;
        }
        else
        {
            etherTypeOffset = 12;
        }
        etherType = descrPtr->dstMacPtr[etherTypeOffset] << 8 |
                    descrPtr->dstMacPtr[etherTypeOffset + 1];

        if(etherType == 0x0800)
        {/* if it's IP frame */
            if (SGT_MAC_ADDR_IS_IPMC(descrPtr->dstMacPtr))
            {/* if it's IP multicast frame */
                if(descrPtr->dstMacPtr[etherTypeOffset+11] == 2)
                {/* if IP protocol is IGMP */
                    descrPtr->daType.igmp = 1;
                }
            }
        }

        if (SGT_MAC_ADDR_IS_PAUSE(descrPtr->dstMacPtr))
        {
          descrPtr->daType.pause = 1;
        }
    }

    if (descrPtr->daType.pause)
    {
        if (!((headerPtr[12] == 0x88) && (headerPtr[13] == 0x08)))
        {
               descrPtr->daType.pause = 0;
        }
        else if (!((headerPtr[14] == 0x00) && (headerPtr[15] == 0x01)))
        {
               descrPtr->daType.pause = 0;
        }
        if (descrPtr->daType.pause)
        {
            descrPtr->pauseTime = headerPtr[16] & headerPtr[17];
            return;
        }
    }

    /* The packet is not pause */
    smemRegFldGet(devObjPtr, regAddress, 8, 1, &cascadedPort);
    if (cascadedPort)
    {
        descrPtr->cascade = 1;
        descrPtr->srcVlanTagged = (frameBufPtr->actualDataPtr[12] >> 5) & 0x1;
        descrPtr->qPri = (headerPtr[14] >> 6) & 0x3;
        descrPtr->marvellTagCmd = (headerPtr[12] >> 6) & 0x3;
        if (descrPtr->marvellTagCmd == TAG_CMD_FROM_CPU_E)
        {
            descrPtr->qPri = (headerPtr[14] & 0x3);
        }
        else if (descrPtr->marvellTagCmd == TAG_CMD_TO_TARGET_SNIFFER_E)
        {
            if ((devObjPtr->deviceType == SKERNEL_RUBY) ||
                (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType)))
            {
                descrPtr->rxSnif = (headerPtr[13] >> 2) & 0x1;
            }
        }
        else if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
        {
            if(descrPtr->marvellTagCmd == TAG_CMD_FORWARD_E)
            {
                descrPtr->pktCmd = SKERNEL_PKT_FORWARD_E;
                /* bit 18 in mrvl tag */
                descrPtr->srcTrunk = (headerPtr[13] >> 2) & 0x1;
                if (descrPtr->srcTrunk)
                {
                    /* bit 19-23 (5 bits) in mrvl tag --
                       but Jade/Opal support only 4 bits of trunk */
                    descrPtr->trunkId = (headerPtr[13] >> 3) & 0x0F;

                }
                else
                {
                    regAddr = PORT_CTRL1_REG + descrPtr->srcPort * 0x10000;
                    smemRegFldGet(devObjPtr, regAddr,  14, 1, &fldValue);
                    /* Trunk port */
                    descrPtr->srcTrunk = (GT_U8)fldValue;
                    smemRegFldGet(devObjPtr, regAddr,  4, 4, &fldValue);
                    /* Trunk ID */
                    descrPtr->trunkId = (GT_U8)fldValue;
                }
            }
        }

        if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
        {
            snetOpalPvtCollect(devObjPtr, descrPtr);
        }

        return;
    }

    if ((devObjPtr->deviceType == SKERNEL_RUBY) ||
        (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType)))
    {
        descrPtr->tagVal = 0x8100;
        if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
        {
            regAddr = PORT_CTRL1_REG + descrPtr->srcPort * 0x10000;
            smemRegFldGet(devObjPtr, regAddr,  14, 1, &fldValue);
            /* Trunk port */
            descrPtr->srcTrunk = (GT_U8)fldValue;
            smemRegFldGet(devObjPtr, regAddr,  4, 4, &fldValue);
            /* Trunk ID */
            descrPtr->trunkId = (GT_U8)fldValue;
        }
        smemRegFldGet(devObjPtr, regAddress, 15, 1, &fldValue);
        /* Use core tag */
        if (fldValue)
        {
            smemRegGet(devObjPtr, GLB_CORE_TAG_TYPE_REG, &fldValue);
            /* Core Tag Type */
            descrPtr->tagVal = (GT_U16)fldValue;
        }
        else
        {
            regAddr = PORT_CTRL_REG + descrPtr->srcPort * 0x10000;
            smemRegFldGet(devObjPtr, regAddr,  9, 1, &fldValue);
            /* DoubleTag */
            if (fldValue)
            {
                if ((headerPtr[12] << 8 | headerPtr[13]) == descrPtr->tagVal)
                {
                    doubleTag = 1;
                }
            }
        }
        if (doubleTag)
        {
            if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
            {
                smemRegFldGet(devObjPtr, GLB2_MNG_REG, 15, 1, &fldValue);
                /* Use double tag data */
                if (fldValue)
                {
                    descrPtr->dtPri = (headerPtr[14] >> 5) & 0x7;
                    descrPtr->dtVid = headerPtr[14] & 0xf;
                    descrPtr->useDt = 1;
                }
            }
            /* Remove bytes 13 to 16 */
            memcpy(&headerPtr[12], &headerPtr[15], descrPtr->byteCount - 15);
            descrPtr->byteCount-= 4;
            descrPtr->modified = 1;
            descrPtr->tagVal = 0x8100;
        }
    }
}

/*******************************************************************************
*   snetSohoCascadeFrame
*
* DESCRIPTION:
*       Cascade Frame Port Mapping
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
* RETURNS:
*       GT_TRUE       - Skip PORT_VEC and PRI processing
*       GT_FALSE      - Perform Normal Packet Processing
*
*******************************************************************************/
static GT_BOOL snetSohoCascadeFrame
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSohoCascadeFrame);

    GT_U32 regAddr;                     /* register's address */
    GT_U32 trgPort;                     /* target port */
    GT_U32 fieldValue;                  /* Register's field value */
    GT_BOOL retVal;                     /* Return status */
    GT_U32 mrvlTag;                     /* Marvell tag */
    GT_U32 trgDev;                      /* Target device */

    /* build the MARVELL tag */
    mrvlTag = DSA_TAG(descrPtr->frameBuf->actualDataPtr);

    retVal = GT_TRUE;

    if (descrPtr->marvellTagCmd == TAG_CMD_FROM_CPU_E)
    {
        trgDev = SMEM_U32_GET_FIELD(mrvlTag, 24, 5);
        if (trgDev == descrPtr->srcDevice)
        {
            /* Target port from Marvell Tag */
            __LOG(("Target port from Marvell Tag"));
            trgPort = SMEM_U32_GET_FIELD(mrvlTag, 19, 5);
            descrPtr->destPortVector = (1 << trgPort);
        }
        else
        {
            smemRegFldGet(devObjPtr, GLB_CTRL_2_REG, 12, 4, &fieldValue);
            /* Cascade port */
            if (fieldValue == 0xf)
            {
                descrPtr->destPortVector = (1 << trgDev);
            }
            else
            {
                trgPort = fieldValue;
                descrPtr->destPortVector = (1 << trgPort);
            }
        }
        descrPtr->Mgmt = 1;
    }
    else
    if (descrPtr->marvellTagCmd == TAG_CMD_TO_CPU_E)
    {
        descrPtr->srcDevice = (GT_U8)SMEM_U32_GET_FIELD(mrvlTag, 24, 5);
        regAddr = PORT_CTRL_2_REG + descrPtr->srcPort * 0x10000;
        smemRegFldGet(devObjPtr, regAddr, 0, 4, &fieldValue);
        /*  CPU port */
        __LOG(("CPU port"));
        trgPort = fieldValue;
        descrPtr->destPortVector = (1 << trgPort);
        descrPtr->Mgmt = 1;
    }
    else
    if (descrPtr->marvellTagCmd == TAG_CMD_TO_TARGET_SNIFFER_E)
    {
        descrPtr->srcDevice = (GT_U8)SMEM_U32_GET_FIELD(mrvlTag, 24, 5);
        descrPtr->destPortVector = 0;
        descrPtr->Mgmt = 1;
    }
    else
    {
        descrPtr->srcDevice = (GT_U8)SMEM_U32_GET_FIELD(mrvlTag, 24, 5);
        retVal = GT_FALSE;
    }

    return retVal;
}

/*******************************************************************************
*   snetSohoCascadeRubyFrame
*
* DESCRIPTION:
*       Cascade Frame Port Mapping
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
* RETURNS:
*       GT_TRUE       - Skip PORT_VEC and PRI processing
*       GT_FALSE      - Perform Normal Packet Processing
*
*******************************************************************************/
static GT_BOOL snetSohoCascadeRubyFrame
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSohoCascadeRubyFrame);

    GT_U32      trgPort;                    /* target port */
    GT_U32      trgDev;                     /* Target device */
    GT_U32      srcDevice;                  /* Source device */
    GT_U32      regAddr;                    /* register's address */
    GT_U32      fieldValue;                 /* register's field value */
    GT_BOOL     retVal;                     /* return value */
    SOHO_DEV_MEM_INFO * memInfoPtr;         /* device's memory pointer */
    GT_U32      mrvlTag;                    /* Marvell tag */
    GT_U32      mrvlTagCpuCode;             /* Marvell tag CPU code */
    GT_U32      flowCtrl;                   /* Flow control */


    memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);

    /* build the MARVELL tag */
    mrvlTag = DSA_TAG(descrPtr->frameBuf->actualDataPtr);

    retVal = GT_TRUE;

    if (descrPtr->marvellTagCmd == TAG_CMD_TO_CPU_E)
    {
        descrPtr->srcDevice =
            (GT_U8)SMEM_U32_GET_FIELD(mrvlTag, 24, 5);

        descrPtr->origSrcPortOrTrnk =
            (GT_U8)SMEM_U32_GET_FIELD(mrvlTag, 19, 5);

        if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
        {
            /*Monitor Control register Offset: 0x1A or decimal 26
             the field CPU Dest - bits 4:7*/
            smemRegFldGet(devObjPtr, GLB_MON_CTRL_REG, 4, 4, &fieldValue);
        }
        else
        {
            regAddr = PORT_CTRL_2_REG + descrPtr->srcPort * 0x10000;
            smemRegFldGet(devObjPtr, regAddr, 0, 4, &fieldValue);
        }
        /*  CPU port */
        __LOG(("CPU port"));
        trgPort = fieldValue;
        descrPtr->destPortVector = (1 << trgPort);
        if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
        {
            /* build the CPU code from 3 and 1 bits */
            mrvlTagCpuCode = (SMEM_U32_GET_FIELD(mrvlTag, 16, 3) << 1) |
                              SMEM_U32_GET_FIELD(mrvlTag, 12, 1);
            /* ARP */
            if (mrvlTagCpuCode == 0x4)
            {
                smemRegFldGet(devObjPtr, GLB_MON_CTRL_REG, 4, 4, &fieldValue);
                /* ARP monitor destination */
                __LOG(("ARP monitor destination"));
                trgPort = fieldValue;
                descrPtr->destPortVector = (1 << trgPort);
            }
        }

        descrPtr->Mgmt = 1;
    }
    else
    if (descrPtr->marvellTagCmd == TAG_CMD_FROM_CPU_E)
    {
        if (descrPtr->rmtMngmt)
        {
            descrPtr->destPortVector = (1 <<  descrPtr->srcPort);
        }
        else
        {

            trgDev = SMEM_U32_GET_FIELD(mrvlTag, 24, 5);
            if (trgDev == descrPtr->srcDevice)
            {
                /* Target port from Marvell Tag */
                __LOG(("Target port from Marvell Tag"));
                trgPort = SMEM_U32_GET_FIELD(mrvlTag, 19, 5);
                descrPtr->destPortVector = (1 << trgPort);
            }
            else
            {
                if ( SKERNEL_DEVICE_FAMILY_SOHO_PLUS((devObjPtr)->deviceType))
                {/* For cascade FROM_CPU forwarding the OPAL+ does not look at this filed
                    and it behaves like OPAL when the field contain the value 0xf. */
                    fieldValue = 0xf;
                }
                else
                    smemRegFldGet(devObjPtr, GLB_CTRL_2_REG, 12, 4, &fieldValue);
                 /* Cascade port */
                if (fieldValue == 0xf)
                {
                    if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
                    {
                        trgPort = memInfoPtr->trgDevMem.deviceTblMem[trgDev] & 0xf;
                        if (trgPort == 0xf)
                        {
                            descrPtr->destPortVector = 0;
                        }
                        else
                        {
                            descrPtr->destPortVector = (1 << trgPort);
                        }
                    }
                }
                else
                {
                    trgPort = fieldValue;
                    if (trgPort >= 0xb && trgPort <= 0xe)
                    {
                        descrPtr->destPortVector = 0;
                    }
                    else
                    {
                        descrPtr->destPortVector = (1 << trgPort);
                    }
                }
            }
        }
        descrPtr->Mgmt = 1;
    }
    else
    if (descrPtr->marvellTagCmd == TAG_CMD_TO_TARGET_SNIFFER_E)
    {
        flowCtrl = GT_FALSE;

        if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
        {
            /* Flow control from marvell tag */
            flowCtrl = SMEM_U32_GET_FIELD(mrvlTag, 17, 1);
            if (flowCtrl)
            {
                srcDevice = SMEM_U32_GET_FIELD(mrvlTag, 24, 5);
                if (srcDevice == descrPtr->srcDevice)
                {
                    /* Egress port's speed */
                    descrPtr->fcSpd = (GT_U8)SMEM_U32_GET_FIELD(mrvlTag, 10, 2);
                    descrPtr->fcIn = 1;
                }
                else
                {
                    trgDev = SMEM_U32_GET_FIELD(mrvlTag, 24, 5);
                    trgPort = memInfoPtr->trgDevMem.deviceTblMem[trgDev] & 0xf;
                    if (trgPort == 0xf)
                    {
                        descrPtr->destPortVector = 0;
                    }
                    else
                    {
                        descrPtr->destPortVector = (1 << trgPort);
                    }
                }
            }
        }
        /* Process Rx/Tx sniffer frame */
        if (flowCtrl == GT_FALSE)
        {
            if (descrPtr->rxSnif)
            {
                smemRegFldGet(devObjPtr, GLB_MON_CTRL_REG, 12, 4, &fieldValue);
                /* Ingress monitor destination */
                trgPort = fieldValue;
                descrPtr->Mgmt = 1;
            }
            else
            {
                smemRegFldGet(devObjPtr, GLB_MON_CTRL_REG, 8, 4, &fieldValue);
                /* Egress monitor destination */
                trgPort = fieldValue;
                if (descrPtr->daHit)
                {
                    descrPtr->Mgmt = 1;
                }
            }

            if (trgPort >= 0xb && trgPort <= 0xf)
            {
                descrPtr->destPortVector = 0;
            }
            else
            {
                descrPtr->destPortVector = (1 << trgPort);
            }
        }

        descrPtr->srcDevice =
            (GT_U8)SMEM_U32_GET_FIELD(mrvlTag, 24, 5);
        descrPtr->origSrcPortOrTrnk =
            (GT_U8)SMEM_U32_GET_FIELD(mrvlTag, 19, 5);
    }
    else /* forward DSA tag */
    {
        descrPtr->srcDevice =
            (GT_U8)SMEM_U32_GET_FIELD(mrvlTag, 24, 5);
        descrPtr->origSrcPortOrTrnk =
            (GT_U8)SMEM_U32_GET_FIELD(mrvlTag, 19, 5);

        retVal = GT_FALSE;
    }

    return retVal;
}

/*******************************************************************************
*   snetSohoVtuAtuLookUp
*
* DESCRIPTION:
*       DA and SA lookups with the determined DBNum
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static GT_VOID snetSohoVtuAtuLookUp
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSohoVtuAtuLookUp);

    if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
    {/* Start VTU lookup after VID is determine */
        snetOpalPvtCollect(devObjPtr, descrPtr);
    }
    /* Start VTU lookup after VID is determine */
    __LOG(("Start VTU lookup after VID is determine"));
    snetSohoVlanAssign(devObjPtr, descrPtr);
    /* DA Translation Process */
    __LOG(("DA Translation Process"));
    snetSohoDaTranslation(devObjPtr, descrPtr);
    /* SA Learning Process */
    __LOG(("SA Learning Process"));
    snetSohoSaLearning(devObjPtr, descrPtr);
}


/*******************************************************************************
*   snetOpalPvtCollect
*
* DESCRIPTION:
*       collect values for PVT
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static GT_VOID snetOpalPvtCollect
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{

    GT_U32 fld5BitPort;                 /* register's field first bit */
    SOHO_DEV_MEM_INFO * memInfoPtr;         /* device's memory pointer */
    SMEM_REGISTER * pvtTblMemPtr;       /* PVT table memory pointer */
    GT_32 index,offset;

    if(descrPtr->cascade)
    {
        /* Get pointer to the device memory */
        memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);
        pvtTblMemPtr = memInfoPtr->pvtDbMem.pvtTblMem;

        smemRegFldGet(devObjPtr, GLB2_DEST_POLARITY_REG, 14, 1,&fld5BitPort);


       /* When this bit fld5BitPort is zero the 9 bits
          used to access the PVT memory is:
          Addr[8:4] = Source Device[4:0] or 0x1F
          Addr[3:0] = Source Port/Trunk[3:0]              */
       /* When this bit is one the 9 bits used to access the PVT memory is:
          Addr[8:5] = Source Device[3:0] or 0xF
          Addr[4:0] = Source Port/Trunk[4:0]             */

        if(fld5BitPort == 0)
        {
            offset = 4;

        }
        else
        {
            offset = 5;
        }



        if (descrPtr->srcTrunk == 1)
        {

            index = descrPtr->trunkId  | (descrPtr->srcTrunk << offset);

        }
        else
        {

            index = descrPtr->srcPort | (descrPtr->srcDevice << offset);

        }
        descrPtr->pvt = (GT_UINTPTR)&pvtTblMemPtr[index];

    }

}

/*******************************************************************************
*   snetSohoIpPriorityAssign
*
* DESCRIPTION:
*       IP priority extraction
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static GT_VOID snetSohoIpPriorityAssign
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSohoIpPriorityAssign);

    GT_U32 regAddr, fldValue;           /* registers address and value */
    GT_U32 fldFirstBit;                 /* register's field first bit */
    GT_U16 etherType;                   /* frame's ethernet type */
    GT_U8 * dataPtr;                    /* pointer to frame's actual data */
    GT_U32 port;                        /* source port */
    GT_U8 byte;
    GT_BOOL vlanModeEn = GT_TRUE;       /* IEEE 802.1 mode enable */
    GT_U8   etherTypeOffset=0;
    GT_U32 portEtypeRegData;            /* Port Policy Register's Data */

    GT_U32      remoteMgmt = 0;         /* Remote Management bit  */
    GT_U32      dsa18_23;               /* Must be 0x3e Remote Management */
    GT_U32      dsa29;                  /* Must be 1 in Remote Management */
    GT_U32      dsa16;                  /* Must be 1 in Remote Management */
    GT_U32      dsa8_12;                /* Must be 0xf Remote Management */

    GT_BIT      f2Renable;              /* Frame to Register enable */
    GT_BIT      f2Rp10;                 /* port 10 bit              */
    GT_BIT      daCheck;                /* check the DA in RRU validate static */

    dataPtr = descrPtr->frameBuf->actualDataPtr;
    port = descrPtr->srcPort;
    descrPtr->ipPriority.useIpvxPriority = 0;

    etherType = (dataPtr[12] << 8 | dataPtr[13]);

    if (descrPtr->cascade == 0)
    {
        if (etherType != 0x8100)
        {
            regAddr = PORT_DFLT_VLAN_PRI_REG + (port * 0x10000);
            smemRegFldGet(devObjPtr, regAddr, 0, 12, &fldValue);
            /* Default VID */
            descrPtr->vid = (GT_U16)fldValue;
            if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
            {
                if (SGT_MAC_ADDR_IS_IPMC(descrPtr->dstMacPtr))
                {/* if it's IP multicast frame */
                    etherTypeOffset = 12;
                    if(descrPtr->dstMacPtr[etherTypeOffset+11] == 2)
                    {/* if IP protocol is IGMP */
                        descrPtr->daType.igmp = 1;
                    }
                }
            }
        }
        else
        {
            descrPtr->srcVlanTagged = 1;

            if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
            {
                regAddr = PORT_CTRL_2_REG + (port * 0x10000);
                smemRegFldGet(devObjPtr, regAddr, 10, 2, &fldValue);
                /* 802.1Q mode */
                if (fldValue)
                {
                    descrPtr->mcfi = (dataPtr[14] >> 4) & 0x1;
                    descrPtr->tagOut = 1;
                }
                else
                {
                    /* Use default Port VID for 802.1Q disabled */
                    regAddr = PORT_DFLT_VLAN_PRI_REG + (port * 0x10000);
                    smemRegFldGet(devObjPtr, regAddr, 0, 12, &fldValue);

                    /* Default VID */
                    descrPtr->vid = (GT_U16)fldValue;
                    vlanModeEn = GT_FALSE;
                }
                if (SGT_MAC_ADDR_IS_IPMC(descrPtr->dstMacPtr))
                {/* if it's IP multicast frame */
                    etherTypeOffset = 16;
                    if(descrPtr->dstMacPtr[etherTypeOffset+11] == 2)
                    {/* if IP protocol is IGMP */
                        descrPtr->daType.igmp = 1;
                    }
                }
            }
        }
    }
    else
    {
        /* Cascade port frame */
        __LOG(("Cascade port frame"));
        descrPtr->srcVlanTagged = (dataPtr[12] >> 5) & 0x1;
        if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
        {
            descrPtr->mcfi = dataPtr[13] & 0x1;
            descrPtr->tagOut = 1;
        }


        if(SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
            remoteMgmt = dataPtr[13] & 0x02;

        if (remoteMgmt == 2)
        {
            dsa18_23 = dataPtr[13] & 0xf8;
            dsa29 =   dataPtr[12] & 0x20;
            dsa16 = dataPtr[13] & 0x1;
            dsa8_12 = dataPtr[14] & 0x1f;

            smemRegFldGet(devObjPtr, GLB_CTRL_2_REG, 12,1, &f2Renable);
            smemRegFldGet(devObjPtr, GLB_CTRL_2_REG, 13,1, &f2Rp10);

            if ((dsa18_23 == 0xf8) && (dsa29 == 0) &&
                (dsa16 == 0)       && (dsa8_12 == 0xf) && (f2Renable) &&
                (((f2Rp10) && (port == 10)) ||((!(f2Rp10)) && (port == 9))))
            {
               descrPtr->rmtMngmt = 1;
            }
            else

                descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
        }
    }

    if (descrPtr->srcVlanTagged || descrPtr->cascade)
    {
        if (vlanModeEn == GT_TRUE)
        {
            descrPtr->vid = (dataPtr[14] & 0xf) << 8 | dataPtr[15];
            regAddr = PORT_DFLT_VLAN_PRI_REG + (port * 0x10000);

            if (descrPtr->vid == 0)
            {
                smemRegFldGet(devObjPtr, regAddr, 0, 12, &fldValue);
                /* Default VID */
                descrPtr->vid = (GT_U16)fldValue;
                 if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType)
                     && (descrPtr->vid == 0))
                     descrPtr->priOnlyTag = GT_TRUE;
            }
            else
            {
                smemRegFldGet(devObjPtr, regAddr, 12, 11, &fldValue);
                /* Force default VID */
                if (fldValue)
                {
                    smemRegFldGet(devObjPtr, regAddr, 0, 12, &fldValue);
                    /* Default VID */
                    __LOG(("Default VID"));
                    descrPtr->vid = (GT_U16)fldValue;
                }
            }
        }

        /* IEEE priority */
        descrPtr->fPri = (dataPtr[14] >> 5) & 0x7;
        if (descrPtr->useDt)
        {
            descrPtr->fPri = descrPtr->dtPri;
        }

        /* Map IEEE Priority Per Port to fpri */
        /* calculate base address */
        regAddr = PORT_IEEE_PRIO_REMAP_REG + (port * 0x10000);

        regAddr += ((descrPtr->fPri / 4) * 16);

        fldFirstBit = (descrPtr->fPri % 4) * 4;

        smemRegFldGet(devObjPtr, regAddr, fldFirstBit, 3, &fldValue);

        descrPtr->fPri = (GT_U8)fldValue;

        /* Map IEEE priority to global priority */
        fldFirstBit = fldValue * 2;
        smemRegFldGet(devObjPtr, GLB_IEEE_PRI_REG, fldFirstBit, 2,
                      &fldValue);

        descrPtr->ipPriority.ieeePiority = (GT_U8)fldValue;
    }

    if (descrPtr->useDt)
    {
        descrPtr->vid = descrPtr->dtVid;
        if ((SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType)) &&
             (descrPtr->vid == 0))
            descrPtr->priOnlyTag = GT_TRUE;
    }



    if  (descrPtr->rmtMngmt == 1)
    {/* check the DA in RMU  */
        smemRegFldGet(devObjPtr, GLB_CTRL_2_REG, 14,1, &daCheck);
        if  ((daCheck == 1) && (descrPtr->daStatic == 0))
            descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
    }

    else
    {
        /* Start VTU/DA/SA Lookup */
        snetSohoVtuAtuLookUp(devObjPtr, descrPtr);
    }


    /* Assign IP Priority to Tagged packet or Cascade packet */
    if (descrPtr->srcVlanTagged || descrPtr->cascade)
    {
        etherType = (dataPtr[16] << 8 | dataPtr[17]);
        if (etherType == 0x0800)
        {
            /* Tagged IPV4 Classifying */
            byte = (dataPtr[18] >> 4) & 0xf;
            if (byte == 0x4)
            {
                descrPtr->ipPriority.useIpvxPriority = 1; /*IPV4_E;*/
                byte = (dataPtr[19] >> 2) & 0x3f;
                descrPtr->ipPriority.ipPriority = byte;
                if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
                    descrPtr->iHl = dataPtr[14] & 0xf;
            }
        }
        else
        if (etherType == 0x86dd)
        {
            /* Tagged IPV6 Classifying */
            byte = (dataPtr[18] >> 4) & 0xf;
            if (byte == 0x6)
            {
                descrPtr->ipPriority.useIpvxPriority = 2; /*IPV6_E;*/
                byte = (dataPtr[19] >> 6) & 0x3;
                descrPtr->ipPriority.ipPriority = byte;
                byte = dataPtr[18] & 0xf;
                descrPtr->ipPriority.ipPriority |= byte << 2;
                if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
                    descrPtr->iHl = dataPtr[14] & 0xf;
            }
        }
        else
        if (etherType == 0x0806)
        {
            if (SGT_MAC_ADDR_IS_BCST(descrPtr->dstMacPtr))
            {
                if (descrPtr->cascade == 0)
                {
                    descrPtr->arp = 1;
                }
            }
        }
        if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
        {
            if (etherType == 0x8863)
            {
                    if (descrPtr->cascade == 0)
                    {
                        descrPtr->pppOE = 1;
                    }
                }
            else if (etherType == 0x8200)
            {
                    if (descrPtr->cascade == 0)
                    {
                        descrPtr->vBas = 1;
                    }

            }

            portEtypeRegData = PORT_ETYPE + (port * 0x10000);
            smemRegFldGet(devObjPtr, portEtypeRegData, 0, 16, &fldValue);

            if  (etherType == fldValue)
            {

                    if (descrPtr->cascade == 0)
                    {
                        descrPtr->eType = 1;
                    }

            }
        }
    }
    else
    {
        if (etherType == 0x0800)
        {
            /* Untagged IPV4 Classifying */
            byte = (dataPtr[14] >> 4) & 0xf;
            if (byte == 0x4)
            {
                descrPtr->ipPriority.useIpvxPriority = 1; /*IPV4_E;*/
                byte = (dataPtr[15] >> 2) & 0x3f;
                descrPtr->ipPriority.ipPriority = byte;
                if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
                    descrPtr->iHl = dataPtr[14] & 0xf;
            }
        }
        else
        if (etherType == 0x86dd)
        {
            /* Untagged IPV6 Classifying */
            byte = (dataPtr[14] >> 4) & 0xf;
            if (byte == 0x6)
            {
                descrPtr->ipPriority.useIpvxPriority = 2; /*IPV6_E;*/
                byte = (dataPtr[15] >> 6) & 0x3;
                descrPtr->ipPriority.ipPriority = byte;
                byte = dataPtr[14] & 0xf;
                descrPtr->ipPriority.ipPriority |= byte << 2;
            }
        }
        else
        if (etherType == 0x0806)
        {
            if (SGT_MAC_ADDR_IS_BCST(descrPtr->dstMacPtr))
            {
                descrPtr->arp = 1;
            }
        }
        if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
        {
            if (etherType == 0x8863)
            {
                if (SGT_MAC_ADDR_IS_BCST(descrPtr->dstMacPtr))
                {
                    if (descrPtr->cascade == 0)
                    {
                        descrPtr->pppOE = 1;
                    }
                }
            }
            else if (etherType == 0x8200)
            {
                if (SGT_MAC_ADDR_IS_BCST(descrPtr->dstMacPtr))
                {
                    if (descrPtr->cascade == 0)
                    {
                        descrPtr->vBas = 1;
                    }
                }
            }
            /*reading portEtype value register 0xf */

            portEtypeRegData = PORT_ETYPE + (port * 0x10000);
            smemRegFldGet(devObjPtr, portEtypeRegData, 0, 16, &fldValue);
            if  (etherType == fldValue)
            {
                    if (descrPtr->cascade == 0)
                    {
                        descrPtr->eType = 1;


                }
            }
        }
    }

    if (descrPtr->ipPriority.useIpvxPriority)
    {
        /* Remap the IP Priority according to IP_QPRI map */
        regAddr = GLB_IP_QPRI_MAP_REG +
            ((descrPtr->ipPriority.ipPriority / 8) * 16);

        fldFirstBit = ((descrPtr->ipPriority.ipPriority % 8) * 2);

        smemRegFldGet(devObjPtr, regAddr, fldFirstBit, 2, &fldValue);

        descrPtr->ipPriority.ipPriority = (GT_U8)fldValue;
    }
}

/*******************************************************************************
*   snetSohoPortState
*
* DESCRIPTION:
*       Control frame's processing
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static GT_VOID snetSohoIgmpSnoop
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSohoIgmpSnoop);

    GT_U8 * dataPtr;                    /* pointer to frame's actual data */
    GT_U32 port;                        /* source port */
    GT_U8 nextHeaderProt;               /* frame's protocol/next header */
    GT_U32 regAddr, fldValue;           /* registers address and value */
 /*   GT_U32 packetSize;                   packet size */
    GT_U32 portControl2Data;            /* Port Control 2 Register's Data */



    port = descrPtr->srcPort;

    regAddr = PORT_CTRL_2_REG + (port * 0x10000);
    smemRegGet(devObjPtr, regAddr, &portControl2Data);


    dataPtr = descrPtr->frameBuf->actualDataPtr;
    port = descrPtr->srcPort;

    if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        /* Change the IGMP Frame check logic in Ruby2 */
        __LOG(("Change the IGMP Frame check logic in Ruby2"));
        snetSoho2IgmpSnoop(devObjPtr, descrPtr);
        return;
    }

    /* IPv4 IGMP snooping */
    if (descrPtr->ipPriority.useIpvxPriority == 1 &&
        descrPtr->daType.igmp)
    {
        if (descrPtr->srcVlanTagged)
        {
            nextHeaderProt = dataPtr[27] & 0xff;
        }
        else
        {
            nextHeaderProt = dataPtr[23] & 0xff;
        }
        if (nextHeaderProt == 0x02)
        {
            regAddr = PORT_CTRL_REG + port * 0x10000;
            smemRegFldGet(devObjPtr, regAddr, 10, 1, &fldValue);
            /* IGMP snooping */
            __LOG(("IGMP snooping"));
            if (fldValue)
            {
                descrPtr->igmpSnoop = 1;
            }
        }
    }
    else
    /* IPv4 IGMP snooping */
    if (descrPtr->ipPriority.useIpvxPriority == 2 &&
         descrPtr->daType.mld)
    {
        if (descrPtr->srcVlanTagged)
        {
            nextHeaderProt = dataPtr[24] & 0xff;
        }
        else
        {
            nextHeaderProt = dataPtr[20] & 0xff;
        }
        if (nextHeaderProt == 0x01)
        {
            regAddr = PORT_CTRL_REG + port * 0x10000;
            smemRegFldGet(devObjPtr, regAddr, 10, 1, &fldValue);
            /* IGMP snooping */
            __LOG(("IGMP snooping"));
            if (fldValue)
            {
                descrPtr->igmpSnoop = 1;
            }
        }
    }
}

/*******************************************************************************
*   snetSoho2PortState
*
* DESCRIPTION:
*       Ruby2 specific snooping processing
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static GT_VOID snetSoho2IgmpSnoop
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSoho2IgmpSnoop);

    GT_U8 * dataPtr;                    /* pointer to frame's actual data */
    GT_U32 port;                        /* source port */
    GT_U32 regAddr, fldValue;           /* registers address and value */
    GT_U8 nextHeaderProt, hdr2;         /* frame's protocol/next header */
    GT_U8 error;                        /* error flag */

    dataPtr = descrPtr->frameBuf->actualDataPtr;
    port = descrPtr->srcPort;

    /* IPv4 IGMP snooping */
    if (descrPtr->ipPriority.useIpvxPriority == 1)
    {
        if (descrPtr->srcVlanTagged)
        {
            nextHeaderProt = dataPtr[27] & 0xff;
        }
        else
        {
            nextHeaderProt = dataPtr[23] & 0xff;
        }
        if (nextHeaderProt == 0x02)
        {
            regAddr = PORT_CTRL_REG + port * 0x10000;
            smemRegFldGet(devObjPtr, regAddr, 10, 1, &fldValue);
            /* IGMP snooping */
            __LOG(("IGMP snooping"));
            if (fldValue)
            {
                descrPtr->igmpSnoop = 1;
            }
        }
    }
    else
    /* IPv6 MLD snooping */
    if (descrPtr->ipPriority.useIpvxPriority == 2)
    {
        if (descrPtr->srcVlanTagged)
        {
            nextHeaderProt = dataPtr[24];
            hdr2 = dataPtr[64];
            error = (dataPtr[65] >> 0x7);
        }
        else
        {
            nextHeaderProt = dataPtr[20];
            hdr2 = dataPtr[60];
            error = (dataPtr[61] >> 0x7);
        }
        if (nextHeaderProt == 0x0 &&
            hdr2 == 0x3a &&
            error == 1)
        {
            regAddr = PORT_CTRL_REG + port * 0x10000;
            smemRegFldGet(devObjPtr, regAddr, 10, 1, &fldValue);
            /* IGMP snooping */
            if (fldValue)
            {
                descrPtr->igmpSnoop = 1;
            }
        }
    }
}

/*******************************************************************************
*   snetSohoPortMapUpdate
*
* DESCRIPTION:
*       Frame Mapping Policy
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static GT_VOID snetSohoPortMapUpdate
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSohoPortMapUpdate);

    GT_U32 regAddr, fldValue;           /* registers address and value */
    GT_U8  enforcePortBaseVlan = 0;     /* enforce port based VLAN */
    GT_U32 portVector = 0;              /* destination ports vector */
    GT_U32 ports = 0;                   /* device ports number */
    GT_U16 i,ii,jj;
    GT_U32 portBaseVlanBmp;
    GT_U32 port;
    GT_U32 portCtrlRegData;             /* Port Control Register's Data */
    GT_U32 portCtrl1RegData;            /* Port Control Register's Data */
    GT_U32 portCtrl2RegData;            /* Port Control Register's Data */
    GT_U8  mgmt = 0;                    /* Management frame */
    GT_U32  myDevNum;                   /* my device number */
    GT_BOOL enableLoopBackFilter;       /* Prevent loopback filter */
    GT_U32 lastData;
    GT_U8 *dataPtr;

    ports = devObjPtr->portsNumber;
    port = descrPtr->srcPort;

    regAddr = PORT_CTRL_REG + (port * 0x10000);
    smemRegGet(devObjPtr, regAddr, &portCtrlRegData);

    regAddr = PORT_CTRL1_REG + (port * 0x10000);
    smemRegGet(devObjPtr, regAddr, &portCtrl1RegData);

    regAddr = PORT_CTRL_2_REG + (port * 0x10000);
    smemRegGet(devObjPtr, regAddr, &portCtrl2RegData);

    smemRegFldGet(devObjPtr, GLB_CTRL_2_REG, 0, 5, &myDevNum);
    /* Source Device */
    descrPtr->srcDevice = (GT_U8)myDevNum;

    if (descrPtr->cascade)
    {
        if(devObjPtr->deviceType == SKERNEL_SAPPHIRE)
        {
            if(snetSohoCascadeFrame(devObjPtr, descrPtr))
            {/* if it's marvel tag frame skip port vector and priority definition */
                __LOG(("if it's marvel tag frame skip port vector and priority definition"));
                return;
            }
        }
        else
        {
            if(snetSohoCascadeRubyFrame(devObjPtr, descrPtr))
            {


                /* layer 3 identification the kind of remote command */
                if ((descrPtr->rmtMngmt) &&
                    (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType)))
                {
                    /* get the ethertype */
                    descrPtr->remethrtype = RMT_TPYE(descrPtr->frameBuf->actualDataPtr);

                    /* get the request format */
                    descrPtr->reqFormat = REQ_FMT(descrPtr->frameBuf->actualDataPtr);

                    /* get the request code */
                    descrPtr->reqCode = REQ_CDE(descrPtr->frameBuf->actualDataPtr);


                    dataPtr = descrPtr->frameBuf->actualDataPtr;

                    switch (descrPtr->reqCode)
                    {
                        case SNET_OPAL_DUMP_MIB:
                        case SNET_OPAL_DUMP_ATU_STATE:
                        case SNET_OPAL_GETID:
                           for (ii=0; ii<8;ii++ )
                           { /* 2*(request format ,pad ,request code ,data) */

                               descrPtr->reqData[ii] =
                               dataPtr[18 + ii];
                           }
                       break;

                       case SNET_OPAL_READ_WRITE_STATE:
                           for (ii=0; ii<6;ii++ )
                           { /* 2*(request format ,request code  */
                               descrPtr->reqData[ii] =
                               dataPtr[18 + ii];
                           }


                           for (ii=0; ii<121;ii++ )
                           {
                               for (jj=0; jj<4;jj++)
                               {
                                  descrPtr->reqData[6 +4*ii +jj] =
                                  dataPtr[24 + 4*ii +jj];
                               }
                               lastData =  REQ_END_LIST(descrPtr->frameBuf->actualDataPtr,ii);
                               if (lastData == 0xffffffff)
                               { /* last data */
                                   break;
                               }
                           }

                      break;
                    }

                }

                /* if it's marvel tag frame skip port vector and priority definition */
                return;
            }
        }
        /* if we are here : we got forward DSA tag */
    }

    /* Limit Mode Flood UC */
    if ((descrPtr->daHit == 0) &&
        (0 == SGT_MAC_ADDR_IS_MCST(descrPtr->dstMacPtr)))
    {
        /* flood of unicast */
        for (i = 0; i < ports; i++)
        {
            regAddr = PORT_CTRL_REG + (i * 0x10000);
            if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
            {
                smemRegFldGet(devObjPtr, regAddr, 3,1,&fldValue);
            }
            else
            {
                smemRegFldGet(devObjPtr, regAddr, 2,1,&fldValue);
            }
            /* Forward unknown for all soho devices */
            if (fldValue)
            {
                portVector |=  1 << i;
            }

    }
    }
    else if((SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType)) &&
            (descrPtr->daHit == 0))
    {
        smemRegFldGet(devObjPtr, GLB2_MNG_REG, 12,1,&fldValue);
        if((1 == SGT_MAC_ADDR_IS_BCST(descrPtr->dstMacPtr)) && (fldValue == 1) )
        {
            /* Flood BC */
            portVector = 0x3ff;
        }
        else if(1 == SGT_MAC_ADDR_IS_MCST(descrPtr->dstMacPtr))
        { /* Limit Mode Flood MC / BC*/
            for (i = 0; i < ports; i++)
            {
                regAddr = PORT_CTRL_REG + (i * 0x10000);
                smemRegFldGet(devObjPtr, regAddr, 2,1,&fldValue);

                /* Default forward */
                if (fldValue == 1)
                {
                    portVector |=  1 << i;
                }
            }
        }
    }

    else if ((SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
              &&(descrPtr->daHit == 0) &&
             (1 == SGT_MAC_ADDR_IS_MCST(descrPtr->dstMacPtr)))
    {
        /* Limit Mode Flood MC */
        for (i = 0; i < ports; i++)
        {
            regAddr = PORT_CTRL_2_REG + (i * 0x10000);
            smemRegFldGet(devObjPtr, regAddr, 6,1,&fldValue);

            /* Default forward only MULTICAST N*/
            if (fldValue)
            {
                portVector |=  1 << i;
            }
        }

    }

    else
    {
        /* Start with default */
        if (devObjPtr->deviceType == SKERNEL_SAPPHIRE)
        {
            /* Sapphire don't have this bit */
            portVector = 0x3ff;
        }
        else
        {
            for (i = 0; i < ports; i++)
            {
                regAddr = PORT_CTRL_2_REG + (i * 0x10000);
                smemRegFldGet(devObjPtr, regAddr, 6,1,&fldValue);

                /* Default forward */
                if (fldValue)
                {
                    portVector |=  1 << i;
                }
            }
        }
    }

    if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        /* Is DA a MGMT DA? */
        if (descrPtr->Mgmt == 1)
        {
            /* Switch Management Register */
            smemRegFldGet(devObjPtr, GLB2_MNG_REG, 0, 3, &fldValue);
            /* MGMT Pri */
            descrPtr->fPri = descrPtr->qPri = (GT_U8)fldValue;

            if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
            {
                /*Monitor Control register Offset: 0x1A or decimal 26
                 the field CPU Dest - bits 4:7*/
                smemRegFldGet(devObjPtr, GLB_MON_CTRL_REG, 4, 4, &fldValue);
            }
            else
            {
                /* Port control 2 register */
                fldValue = SMEM_U32_GET_FIELD(portCtrl2RegData, 0, 4);
            }

            /* CPUport */
            descrPtr->destPortVector = 1 << fldValue;
            return;
        }
    }

    if (descrPtr->daHit)
    {
        if (descrPtr->Mgmt)
        {
            portVector = descrPtr->destPortVector;
            mgmt = 1;
        }
        else
        {
            fldValue = SMEM_U32_GET_FIELD(portCtrl2RegData, 7, 1);
            /* Map Using DA Hits */
            __LOG(("Map Using DA Hits"));
            if (fldValue)
            {
                portVector = descrPtr->destPortVector;
            }
        }
    }

    /* VLAN table */
    regAddr = PORT_BASED_VLAN_MAP_REG + port * 0x10000;
    smemRegFldGet(devObjPtr, regAddr, 0, ports, &portBaseVlanBmp);

    if (mgmt == 0)
    {
        if (descrPtr->igmpSnoop)
        {
            if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
            {
                /*Monitor Control register Offset: 0x1A or decimal 26
                 the field CPU Dest - bits 4:7*/
                smemRegFldGet(devObjPtr, GLB_MON_CTRL_REG, 4, 4, &fldValue);
            }
            else
            {
                /* Port control 2 register */
                fldValue = SMEM_U32_GET_FIELD(portCtrl2RegData, 0, 4);
            }

            /* CPU port */
            portVector = 1 << fldValue;
        }

        if (descrPtr->saUpdate &&
            devObjPtr->deviceType != SKERNEL_SAPPHIRE)
        {
            for (i = 0; i < ports; i++)
            {
                if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
                {
                    /*  Message Port */
                    fldValue = SMEM_U32_GET_FIELD(portCtrl1RegData, 15, 1);
                }
                else
                {
                    /* Marvell Tag */
                    fldValue = SMEM_U32_GET_FIELD(portCtrlRegData, 8, 1);
                }
                /* Message Port/Marvell Tag */
                if (fldValue)
                {
                    portVector |= (1 << i);
                }
            }
        }

        if (descrPtr->vtuHit &&
            !SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
        {
            portVector &= descrPtr->vtuVector;
            fldValue = SMEM_U32_GET_FIELD(portCtrlRegData, 3, 1);
            /* Protected port */
            if (fldValue)
            {
                enforcePortBaseVlan = 1;
            }
        }
        else
        {
            enforcePortBaseVlan = 1;

            fldValue = SMEM_U32_GET_FIELD(portCtrlRegData, 7, 1);
            /* VLAN tunnel */
            if (fldValue)
            {
                if (descrPtr->daStatic == 1)
                {
                    enforcePortBaseVlan = 0;
                }
            }

            if ((SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType)))
            {/* 802.1Q MODE */
                fldValue = SMEM_U32_GET_FIELD(portCtrl2RegData, 10, 2);
                if (fldValue == 0)
                {  /* USE VTU */
                    fldValue = SMEM_U32_GET_FIELD(GLB2_MNG_REG, 8, 1);
                    if(fldValue)
                    {
                        descrPtr->policyVid = 0;
                    }
                }
                portVector &= descrPtr->vtuVector;


            }
        }

        if (enforcePortBaseVlan)
        {
            portVector &= portBaseVlanBmp;
            if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
            {
                /* VTU vector */
                portVector &= descrPtr->vtuVector;
                /* Load Balancing is done -- by trunk designated ports */
                portVector &= descrPtr->tpv;
            }

            if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
            {/* IGMP/MLD Snooping */
                fldValue = SMEM_U32_GET_FIELD(GLB_CTRL_REG, 10, 1);
                if ((fldValue)  && (descrPtr->igmpSnoop))
                { /* DPV vectored CPUDest */
                     fldValue = SMEM_U32_GET_FIELD(GLB_MON_CTRL_REG, 4, 4);
                     portVector &= ~(1 << fldValue);

                }
            }
        }
    }

    if (((portBaseVlanBmp >> port) & 0x1) == 0)
    {
        portVector &= ~(1 << port);
    }

    if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType) &&
        descrPtr->srcTrunk)
    {
        /* don't allow traffic to return to src trunk */
        for (i = 0; i < ports; i++)
        {
            regAddr = PORT_CTRL1_REG + (i * 0x10000);
            smemRegFldGet(devObjPtr, regAddr, 14,1,&fldValue);
            /* Trunk port */
            if (fldValue)
            {
                smemRegFldGet(devObjPtr, regAddr, 4, 4, &fldValue);
                if (descrPtr->trunkId == fldValue)
                {
                    portVector &= ~(1 << i);
                }
            }
        }


        if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
        {
            /* mask out members of trunk port members */
            for (i = 0; i < ports; i++)
            {
                 regAddr = GLB2_TRUNK_ROUT_REG + (i * 0x10000);
                 smemRegFldGet(devObjPtr, regAddr, 14,1,&fldValue);
                 /* Trunk port */
                 if (fldValue)
                 {
                     smemRegFldGet(devObjPtr, regAddr, 4, 4, &fldValue);
                     if (descrPtr->trunkId == fldValue)
                     {
                         portVector |=  1 << i;
                     }
                 }
             }
        }
    }
    if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        /* Loopback Filter */
        enableLoopBackFilter =
            (SMEM_U32_GET_FIELD(portCtrlRegData, 3, 1)) ? GT_TRUE : GT_FALSE;
    }
    else
    if (devObjPtr->deviceType == SKERNEL_RUBY)
    {
        /* Cascade port */
        enableLoopBackFilter = (descrPtr->cascade) ? GT_TRUE : GT_FALSE;
    }
    else
    {
        enableLoopBackFilter = GT_FALSE;
    }

    if (enableLoopBackFilter)
    {
        if (descrPtr->srcDevice == myDevNum)
        {
            smemRegFldGet(devObjPtr, GLB2_MNG_REG, 14, 1, &fldValue);
            /* Prevent loop */
            if (fldValue)
            {
                descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
                return;
            }

            portVector &= ~(1 << descrPtr->origSrcPortOrTrnk);
        }
    }
    descrPtr->destPortVector = portVector;
}

/*******************************************************************************
*   snetSohoPriorityAssign
*
* DESCRIPTION:
*       Priority Selection
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static GT_VOID snetSohoPriorityAssign
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSohoPriorityAssign);

    GT_U32 regAddr, fldValue;           /* registers address and value */
    GT_U32 port;                        /* source port */
    GT_U32 fldFirstBit;                 /* register's field first bit */
    GT_BOOL useDefaultUserPrio;         /* use port default user priority */

    if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        /* If DA a MGMT, priority has been assigned */
        __LOG(("If DA a MGMT, priority has been assigned"));
        if (descrPtr->Mgmt)
        {
            return;
        }
    }

    useDefaultUserPrio = GT_FALSE;

    port = descrPtr->srcPort;
    regAddr = PORT_CTRL_REG + port * 0x10000;


    smemRegFldGet(devObjPtr, regAddr, 4, 1, &fldValue);

    if (fldValue)
    {
        /* Use Tag priority */
        if (descrPtr->srcVlanTagged)
        {


           /* packet is Tagged */
            __LOG(("packet is Tagged"));
            smemRegFldGet(devObjPtr, regAddr, 5, 1, &fldValue);

            /* Use Ip Priority */
            if (fldValue)
            {
                /* check if packet is IPv4 or IPv6*/
                if (descrPtr->ipPriority.useIpvxPriority != 0)
                {
                    smemRegFldGet(devObjPtr, regAddr, 6, 1, &fldValue);

                    /* Use Tag if both */
                    __LOG(("Use Tag if both"));
                    if (fldValue)
                    {
                        /* IEEE_QPRI */
                        descrPtr->qPri = descrPtr->ipPriority.ieeePiority;
                    }
                    else
                    {
                        /* IP_QPRI */
                        descrPtr->qPri = descrPtr->ipPriority.ipPriority;
                    }
                }
                else
                {
                    /* packet is not IPv4 or IPv6 */
                    __LOG(("packet is not IPv4 or IPv6"));
                    descrPtr->qPri = descrPtr->ipPriority.ieeePiority;
                }
            }
        }
        else
        {
            /* packet is not Tagged */
            useDefaultUserPrio = GT_TRUE;
        }
    }
    else
    {
        /* don't use Tag Priority */
        useDefaultUserPrio = GT_TRUE;
    }

    if (useDefaultUserPrio == GT_TRUE)
    {
        /* getting here means that packet isn't Tagged
           and/or Use Tag Priority bit is disabled */

        regAddr = PORT_DFLT_VLAN_PRI_REG + (port * 0x10000);
        /* DefPri */
        smemRegFldGet(devObjPtr, regAddr, 13, 3, &fldValue);


        if  (descrPtr->cascade == 0)
        {

            descrPtr->fPri = (GT_U8)fldValue;
        }
        /* Remap the default to global priority */
        __LOG(("Remap the default to global priority"));
        fldFirstBit = (fldValue * 2);
        smemRegFldGet(devObjPtr, GLB_IEEE_PRI_REG, fldFirstBit, 2,
                      &fldValue);

        descrPtr->ipPriority.ieeePiority = (GT_U8)fldValue;

        /* queue default */
        descrPtr->qPri = descrPtr->ipPriority.ieeePiority;

        /* get Use_IP_Priority bit */
        regAddr = PORT_CTRL_REG + port * 0x10000;
        smemRegFldGet(devObjPtr, regAddr, 5, 1, &fldValue);

        if (fldValue)
        {
            /* Use IP Priority */
            __LOG(("Use IP Priority"));
            if (descrPtr->ipPriority.useIpvxPriority != 0)
            {
                /* packet is IPv4 or IPv6 */

                /* IP_QPRI */
                descrPtr->qPri = descrPtr->ipPriority.ipPriority;

                /*descrPtr->fPri |= (descrPtr->qPri & 0x3) << 1;*/
                descrPtr->fPri = (descrPtr->fPri & 0x1) | (descrPtr->qPri << 1);

                useDefaultUserPrio = GT_FALSE;
            }
        }
    }

    /* Get Port Control Register2 Address */
    regAddr = PORT_CTRL_2_REG + (port * 0x10000);

    if (descrPtr->vtuHit)
    {
        /* VTU Hit */
        smemRegFldGet(devObjPtr, regAddr, 14, 1, &fldValue);

        if (fldValue)
        {
            /* VTU Priority Override */
            if (descrPtr->priorityInfo.vtu_pri.useVtuPriority)
            {
                /* Use VTU's PRI */
                __LOG(("Use VTU's PRI"));
                descrPtr->fPri = descrPtr->priorityInfo.vtu_pri.vtuPriority & 0x7;
                descrPtr->qPri = (descrPtr->fPri >> 1) & 0x3;
            }
        }
    }

    if (descrPtr->saHit)
    {
        /* SA Hit */
        smemRegFldGet(devObjPtr, regAddr, 13, 1, &fldValue);

        if (fldValue)
        {
            /* SA Priority Override */
            if (descrPtr->priorityInfo.sa_pri.useSaPriority)
            {
                /* Use SA's PRI */
                __LOG(("Use SA's PRI"));
                descrPtr->fPri = descrPtr->priorityInfo.sa_pri.saPriority & 0x7;
                descrPtr->qPri = (descrPtr->fPri >> 1) & 0x3;
            }
        }
    }

    if (descrPtr->daHit)
    {
        /* DA Hit */
        smemRegFldGet(devObjPtr, regAddr, 12, 1, &fldValue);

        if (fldValue)
        {
            /* DA Priority Override */
            if (descrPtr->priorityInfo.da_pri.useDaPriority)
            {
                /* Use DA's PRI */
                __LOG(("Use DA's PRI"));
                descrPtr->fPri = descrPtr->priorityInfo.da_pri.daPriority & 0x7;
                descrPtr->qPri = (descrPtr->fPri >> 1) & 0x3;
            }
        }
    }
}

/*******************************************************************************
*   snetSohoL2Decision
*
* DESCRIPTION:
*       Check frame for VLAN violation, port authentication and size errors
*       and apply decision on the frame
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
*       Check frame for VLAN violation, port authentication and size errors
*       and apply decision on the frame
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
********************************************************************************/
static GT_VOID snetSohoL2Decision
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSohoL2Decision);

    GT_U32 regAddr, fldValue;           /* registers address and value */
    GT_U32 packetSize;                  /* actual packet's data size */
    GT_U32 portState,vtuPortState;
    SNET_SOHO_VTU_STC vtuEntry;
    GT_U8   portMap=0;
    GT_U32  port;                       /* source Port */
    GT_U32 readLearnCnt;                /* limit cnt /learn option */
    GT_U32 learnCnt;                    /* counter */
    GT_U32  entryState_uni;               /* ATU  static policy unicast  enrty*/
    GT_U32  entryState_multi;             /* ATU  static policy multicast  enrty*/

    GT_BIT  dpvMember;                      /*port member */


    port = descrPtr->srcPort;

    if (devObjPtr->deviceType == SKERNEL_RUBY)
    {
        snetSohoL2RubyDecision(devObjPtr, descrPtr);
        return;
    }

    if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
    {
       /* DA policy */
        entryState_uni = SMEM_U32_GET_FIELD(descrPtr->atuEntry[1], 17, 3);
        entryState_multi = SMEM_U32_GET_FIELD(descrPtr->atuEntry[1], 16, 3);

        if ((entryState_uni == 4) || (entryState_multi == 4))
        {

            /* Get Port Rate Override Address */
            __LOG(("Get Port Rate Override Address"));
            regAddr = PORT_PRIORITY_OVERRIDE + (port * 0x10000);

            if (descrPtr->daHit == 1)
            {
                smemRegFldGet(devObjPtr, regAddr, 14, 2, &fldValue);

                if (fldValue == 1)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_MIRROR_CPU_E;
                }
                else if (fldValue == 2)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
                }
                else if (fldValue == 3)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
                }
                else
                    descrPtr->pktCmd = SKERNEL_PKT_FORWARD_E;

            }

            /* SA policy*/

            else if (descrPtr->saHit == 1)
            {
                smemRegFldGet(devObjPtr, regAddr, 12, 2, &fldValue);

                if (fldValue == 1)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_MIRROR_CPU_E;
                }
                else if (fldValue == 2)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
                }
                else if (fldValue == 3)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
                }
                else
                    descrPtr->pktCmd = SKERNEL_PKT_FORWARD_E;

            }


            /* VTU policy*/

            else if ((smemSohoVtuEntryGet(devObjPtr, descrPtr->vid, &vtuEntry) == GT_OK) &&
                 (vtuEntry.vidPolicy))
            {
                /* Get Port Policy control Address */
                regAddr = PORT_POLICY_CONTROL + (port * 0x10000);
                smemRegFldGet(devObjPtr, regAddr, 10, 2, &fldValue);

                if (fldValue == 1)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_MIRROR_CPU_E;
                }
                else if (fldValue == 2)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
                }
                else if (fldValue == 3)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
                }
                else
                    descrPtr->pktCmd = SKERNEL_PKT_FORWARD_E;

            }
            /* Etype policy*/
            else if (descrPtr->eType)
            {
                /* Get Port Policy control Address */
                regAddr = PORT_POLICY_CONTROL + (port * 0x10000);
                smemRegFldGet(devObjPtr, regAddr, 8, 2, &fldValue);

                if (fldValue == 1)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_MIRROR_CPU_E;
                }
                else if (fldValue == 2)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
                }
                else if (fldValue == 3)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
                }
                else
                    descrPtr->pktCmd = SKERNEL_PKT_FORWARD_E;

            }
            /* PPPoE policy*/
            else if (descrPtr->pppOE)
            {
                /* Get Port Policy control Address */
                regAddr = PORT_POLICY_CONTROL + (port * 0x10000);
                smemRegFldGet(devObjPtr, regAddr, 6, 2, &fldValue);

                if (fldValue == 1)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_MIRROR_CPU_E;
                }
                else if (fldValue == 2)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
                }
                else if (fldValue == 3)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
                }
                else
                    descrPtr->pktCmd = SKERNEL_PKT_FORWARD_E;

            }

                /* vBas policy*/
           else if (descrPtr->vBas)
           {
                /* Get Port Policy control Address */
                regAddr = PORT_POLICY_CONTROL + (port * 0x10000);
                smemRegFldGet(devObjPtr, regAddr, 4, 2, &fldValue);

                if (fldValue == 1)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_MIRROR_CPU_E;
                }
                else if (fldValue == 2)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
                }
                else if (fldValue == 3)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
                }
                else
                    descrPtr->pktCmd = SKERNEL_PKT_FORWARD_E;

           }
        }
        switch(descrPtr->pktCmd)
        {
            case SKERNEL_PKT_TRAP_CPU_E:
                 smemRegFldGet(devObjPtr, GLB_MON_CTRL_REG, 4, 4, &fldValue);
                 /* CPUport */
                 descrPtr->destPortVector = 1 << fldValue;
                 break;

            case SKERNEL_PKT_MIRROR_CPU_E:
                 smemRegFldGet(devObjPtr, GLB_MON_CTRL_REG, 4, 4, &fldValue);
                 /* CPUport */
                 descrPtr->destPortVector |= 1 << fldValue;
                 break;

            case SKERNEL_PKT_DROP_E:
                 descrPtr->destPortVector = 0;
                 break;

            default:
                 break;
        }

    }
    if (descrPtr->Mgmt == 0)
    {
        regAddr = PORT_CTRL_REG + (descrPtr->srcPort * 0x10000);
        if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
        {

            smemRegFldGet(devObjPtr, regAddr, 14, 2, &fldValue);
            /* Drop on lock */
            __LOG(("Drop on lock"));
            if (descrPtr->saHit == 0)
            {
                if ((fldValue == 1) && (descrPtr->saNoDpv))
                {
                    descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
                    return;
                }
            }


        }
        else
        {

            smemRegFldGet(devObjPtr, regAddr, 14, 1, &fldValue);
            /* Drop on lock */
            __LOG(("Drop on lock"));
            if (fldValue)
            {
                if (descrPtr->saHit == 0)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
                    return;
                }
            }
        }
        regAddr = PORT_CTRL_2_REG + (descrPtr->srcPort * 0x10000);
        smemRegFldGet(devObjPtr, regAddr, 10, 2, &fldValue);
        /* 802.1Q mode check/secured */
        __LOG(("802.1Q mode check/secured"));
        if (fldValue == 2 || fldValue == 3)
        {
            if (descrPtr->vtuMiss)
            {
                descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
                return;
            }
        }
        /* 802.1Q mode secured  */
        __LOG(("802.1Q mode secured"));
        if (fldValue == 3)
        {
            if (descrPtr->spOk == 0)
            {
                descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
                return;
            }
        }
    }
    if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
    {
        if (descrPtr->srcVlanTagged == 0)
        {
            if (descrPtr->priOnlyTag)
            {
                descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
                return;
            }
        }
    }
    /* Error frame check and port state */
    packetSize = descrPtr->frameBuf->actualDataSize;
    /*  The simulation accepted small sized packets
        to be backward compatible with RTG
    if (packetSize < 64)
    {
        descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
        return;
    }
    */

    smemRegFldGet(devObjPtr, GLB_CTRL_REG, 10, 1, &fldValue);
    if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
    {
        /* Max frame size */
        __LOG(("Max frame size"));
        if (fldValue == 1)
        {
            if (packetSize > 1632)
            {
                descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
                return;
            }
        }
    }
    else
    {
        /* Max frame size */
        if (fldValue == 1)
        {
            if (packetSize > 1532)
            {
                descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
                return;
            }
        }
    }
        if (descrPtr->srcVlanTagged == 0)
        {
            if (packetSize > 1518)
            {
                 descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
                 return;
             }
        }
        else
        if (packetSize > 1522)
        {
            descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
            return;
        }

    if (descrPtr->rxSnif)
    {
        descrPtr->pktCmd = SKERNEL_PKT_MIRROR_CPU_E;
        return;
    }

    /* Analyse of Port state */
    regAddr = PORT_CTRL_REG + (descrPtr->srcPort * 0x10000);
    smemRegFldGet(devObjPtr, regAddr, 0, 2, &portState);
    if (portState == 0)
    {
        descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
        return;
    }

    if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        if (smemSohoVtuEntryGet(devObjPtr, descrPtr->vid, &vtuEntry) == GT_OK)
        {
            portMap = vtuEntry.portsMap[descrPtr->srcPort];
            vtuPortState = (portMap >> 2) & 0x3;
            if (vtuPortState != SNET_SOHO_DISABLED_STATE)
            {
                portState = vtuPortState;
            }
        }
    }

    /* check the if packet can be learned according to the port STP state */
    if (portState == SKERNEL_STP_LEARN_E ||
        portState == SKERNEL_STP_FORWARD_E)
    {
        regAddr = PORT_BASED_VLAN_MAP_REG + (descrPtr->srcPort * 0x10000);
        smemRegFldGet(devObjPtr, regAddr, 11, 1, &fldValue);
        /* Learn disable */
        __LOG(("Learn disable"));
        if (fldValue == 0)
        {
            /* store ATU */
            if (descrPtr->Mgmt == 1)
            {
                descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
            }
            else if (descrPtr->rxSnif == 0)
            {
                descrPtr->pktCmd = SKERNEL_PKT_FORWARD_E;
            }
            else
            {
                descrPtr->pktCmd = SKERNEL_PKT_MIRROR_CPU_E;
            }

            if (descrPtr->rxSnif == 0)
            {
                smemRegSet(devObjPtr, descrPtr->atuEntryAddr,descrPtr->atuEntry[0]);
                smemRegSet(devObjPtr, (descrPtr->atuEntryAddr + 4),descrPtr->atuEntry[1]);
            }
        }
    }

    /* check the STP state for non management packets */
    if (descrPtr->Mgmt == 0)
    {
        if (portState != SKERNEL_STP_FORWARD_E)
        {
            descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
        }
    }

    /* hardware address learn limit for non trunk packets */
    if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
    {
        if (!(descrPtr->srcTrunk))
        {
            for (port = 0 ; port < 11 ; port++)
            {
                    dpvMember =  SMEM_U32_GET_FIELD(descrPtr->destPortVector, port, 1);
                    regAddr = PORT_ATU_CONTROL + (port * 0x10000);
                    smemRegFldGet(devObjPtr, regAddr, 15, 1, &readLearnCnt);
                    if  ((readLearnCnt == 0) && (dpvMember == 1))
                    {
                        /* PUT VALUE IN GLOBAL */
                        smemRegFldGet(devObjPtr, regAddr, 0, 8, &learnCnt);
                        learnCnt++;
                        globLearnCnt = learnCnt;
                        /* increment port learn limit counter */
                        smemMemSet(devObjPtr, regAddr, &learnCnt, 8);
                    }

            }

            for (port = 0 ; port < 11 ; port++)
            {
                    dpvMember =  SMEM_U32_GET_FIELD(descrPtr->destPortVector, port, 1);
                    regAddr = PORT_ATU_CONTROL + (port * 0x10000);
                    smemRegFldGet(devObjPtr, regAddr, 15, 1, &readLearnCnt);
                    smemRegFldGet(devObjPtr, regAddr, 0, 8, &learnCnt);
                    if  ((readLearnCnt == 0) && (dpvMember == 1)&& learnCnt == globLearnCnt )
                    {
                      /* interrupt */
                        smemRegFldSet(devObjPtr, regAddr, 14, 1, 1);
                    }

            }
        }
    }
}

/*******************************************************************************
*   snetSohoL2RubyDecision
*
* DESCRIPTION:
*       Check frame for VLAN violation, port authentication and size errors
*       and apply decision on the frame
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
********************************************************************************/
static GT_VOID snetSohoL2RubyDecision
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSohoL2RubyDecision);

    GT_U32 regAddr, fldValue;           /* registers address and value */
    GT_U32 packetSize;                  /* actual packet's data size */
    GT_U32 maxPacketSize;               /* max packet's data size */

    if (descrPtr->Mgmt == 0)
    {
        regAddr = PORT_CTRL_REG + (descrPtr->srcPort * 0x10000);
        smemRegFldGet(devObjPtr, regAddr, 14, 1, &fldValue);
        /* Drop on lock */
        if (fldValue)
        {
            if (descrPtr->saHit == 0)
            {
                descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
                descrPtr->xdrop = 1;
                descrPtr->filtered = 1;
                descrPtr->destPortVector = 0;
                return;
            }
        }
        regAddr = PORT_CTRL_2_REG + (descrPtr->srcPort * 0x10000);
        smemRegFldGet(devObjPtr, regAddr, 10, 2, &fldValue);
        /* 802.1Q mode check/secured */
        if (fldValue == 2 || fldValue == 3)
        {
            if (descrPtr->vtuMiss)
            {
                descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
                descrPtr->filtered = 1;
                descrPtr->destPortVector = 0;
                return;
            }
        }

        /* 802.1Q mode secured  */
        if (fldValue == 3)
        {
            if (descrPtr->spOk == 0)
            {
                descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
                descrPtr->filtered = 1;
                descrPtr->destPortVector = 0;
                return;
            }
        }

        /* Discard tagged frames
           According to the Don Pannel the implementation of Opal
           is such that the untagged/tagged packet checking is done
            even if the port is in Secure mode (tuvia,8/8/05) */
        regAddr = PORT_CTRL_2_REG + (descrPtr->srcPort * 0x10000);
        if (descrPtr->srcVlanTagged)
        {
            smemRegFldGet(devObjPtr, regAddr, 9, 1, &fldValue);
            /* Discard tagged */
            __LOG(("Discard tagged"));
            if (fldValue)
            {
                descrPtr->filtered = 1;
                descrPtr->destPortVector = 0;
            }
        }
        else
        {
            smemRegFldGet(devObjPtr, regAddr, 8, 1, &fldValue);
            /* Discard untagged */
            __LOG(("Discard untagged"));
            if (fldValue)
            {
                descrPtr->filtered = 1;
                descrPtr->destPortVector = 0;
            }
        }
    }

    /* Error frame check and port state */
    packetSize = descrPtr->frameBuf->actualDataSize;
#if 0 /* not check packteSize for RTG compatibility */
    if (packetSize < 64)
    {
        descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
        return;
    }
#endif
    smemRegFldGet(devObjPtr, GLB_CTRL_REG, 10, 1, &fldValue);
    /* Max frame size */
    if (fldValue == 1)
    {
        maxPacketSize =
            SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType) ? 1632 : 1536;
        if (packetSize > maxPacketSize)
        {
            descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
            return;
        }
    }
    else
    if (descrPtr->srcVlanTagged == 0)
    {
        if (packetSize > 1522)
        {
            descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
            return;
        }
    }
    else
    if (packetSize > 1522)
    {
        descrPtr->pktCmd = SKERNEL_PKT_DROP_E;
        return;
    }

    regAddr = PORT_CTRL_REG + (descrPtr->srcPort * 0x10000);
    smemRegFldGet(devObjPtr, regAddr, 0, 2, &fldValue);
    /* Port state */
    __LOG(("Port state"));
    if (fldValue == SKERNEL_STP_LEARN_E ||
        fldValue == SKERNEL_STP_FORWARD_E)
    {
        regAddr = PORT_BASED_VLAN_MAP_REG + (descrPtr->srcPort * 0x10000);
        smemRegFldGet(devObjPtr, regAddr, 11, 1, &fldValue);
        /* Learn disable */
        __LOG(("Learn disable"));
        if (fldValue == 0)
        {
            if (descrPtr->rxSnif == 0)
            {
                smemRegSet(devObjPtr, descrPtr->atuEntryAddr,
                           descrPtr->atuEntry[0]);

                smemRegSet(devObjPtr, (descrPtr->atuEntryAddr + 4),
                           descrPtr->atuEntry[1]);

                if (descrPtr->Mgmt == 1)
                {
                    descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
                }
                else
                {
                    descrPtr->pktCmd = SKERNEL_PKT_FORWARD_E;
                }
            }
            else
            {
                descrPtr->pktCmd = SKERNEL_PKT_MIRROR_CPU_E;
            }
        }
    }
}

/*******************************************************************************
*   snetSohoStatCountUpdate
*
* DESCRIPTION:
*       Update Stats Counter Ingress Group
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static GT_VOID snetSohoStatCountUpdate
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSohoStatCountUpdate);

    GT_U32 regAddr;                     /* registers address */
    SMEM_REGISTER  counter;             /* counter's value */
    GT_U32 port;                        /* source port id */
    GT_U32 fldValue;

    if (descrPtr->pktCmd == SKERNEL_PKT_DROP_E)
        return;

    port = descrPtr->srcPort;

    /* Good frames received */
    __LOG(("Good frames received"));
    if (SGT_MAC_ADDR_IS_BCST(descrPtr->dstMacPtr))
    {
        regAddr = CNT_IN_BCST_REG + port * 0x10000;

        smemRegGet(devObjPtr, regAddr, &counter);
        smemRegSet(devObjPtr, regAddr, ++counter);
    }
    else
    if (SGT_MAC_ADDR_IS_MCST(descrPtr->dstMacPtr))
    {
        regAddr = CNT_IN_MCST_REG + port * 0x10000;
        smemRegGet(devObjPtr, regAddr, &counter);
        smemRegSet(devObjPtr, regAddr, ++counter);
    }
    else
    {
        regAddr = CNT_IN_UCAST_FRAMES_REG + port * 0x10000;
        smemRegGet(devObjPtr, regAddr, &counter);
        smemRegSet(devObjPtr, regAddr, ++counter);
    }

    switch (SNET_GET_NUM_OCTETS_IN_FRAME(descrPtr->byteCount))
    {
        case SNET_FRAMES_1024_TO_MAX_OCTETS:
            regAddr = CNT_1024_OCTETS_REG + port * 0x10000;
            smemRegGet(devObjPtr, regAddr, &fldValue);
            fldValue++;
            smemRegSet(devObjPtr, regAddr, fldValue);
        break;
        case SNET_FRAMES_512_TO_1023_OCTETS:
            regAddr = CNT_512_TO_1023_OCTETS_REG + port * 0x10000;
            smemRegGet(devObjPtr, regAddr, &fldValue);
            fldValue++;
            smemRegSet(devObjPtr, regAddr, fldValue);
        break;
        case SNET_FRAMES_256_TO_511_OCTETS:
            regAddr = CNT_256_TO_511_OCTETS_REG + port * 0x10000;
            smemRegGet(devObjPtr, regAddr, &fldValue);
            fldValue++;
            smemRegSet(devObjPtr, regAddr, fldValue);
        break;
        case SNET_FRAMES_128_TO_255_OCTETS:
            regAddr = CNT_128_TO_255_OCTETS_REG + port * 0x10000;
            smemRegGet(devObjPtr, regAddr, &fldValue);
            fldValue++;
            smemRegSet(devObjPtr, regAddr, fldValue);
        break;
        case SNET_FRAMES_65_TO_127_OCTETS:
            regAddr = CNT_65_TO_127_OCTETS_REG + port * 0x10000;
            smemRegGet(devObjPtr, regAddr, &fldValue);
            fldValue++;
            smemRegSet(devObjPtr, regAddr, fldValue);
        break;
        case SNET_FRAMES_64_OCTETS:
            regAddr = CNT_64_OCTETS_REG + port * 0x10000;
            smemRegGet(devObjPtr, regAddr, &fldValue);
            fldValue++;
            smemRegSet(devObjPtr, regAddr, fldValue);
        break;
    }

    regAddr = CNT_IN_GOOD_OCTETS_LO_REG + port * 0x10000;
    smemRegGet(devObjPtr, regAddr, &counter);
    counter += descrPtr->byteCount;
    smemRegSet(devObjPtr, regAddr, counter);


}

/*******************************************************************************
*   snetSohoQcMonitorIngress
*
* DESCRIPTION:
*       Monitoring support in the QC Ingress
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       mirrPtr     - tx/rx mirror analyser flags.
*
*******************************************************************************/
static GT_VOID snetSohoQcMonitorIngress
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr,
    OUT SNET_SOHO_MIRROR_STC * mirrPtr
)
{
    DECLARE_FUNC_NAME(snetSohoQcMonitorIngress);

    GT_U32 regAddr;                     /* registers address */
    GT_U32 fldValue;                    /* registers field value */
    GT_U32 arpValue=0;                  /* registers arp value */
    GT_U8 inMonitorDest=0;              /* Ingress monitor dest */
    GT_U8 outMonitorDest=0;             /* Egress monitor dest */
    GT_U32 destRegAddr;                 /* destination register address */
    GT_U8 destPortInd;                  /* counter for destination port loop */

    if ((devObjPtr->deviceType != SKERNEL_RUBY) &&
        !SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        return;
    }

    memset(mirrPtr,0,sizeof(SNET_SOHO_MIRROR_STC));
    /* QC Monitor handling - Egress  */
    regAddr = PORT_CTRL_2_REG + descrPtr->srcPort * 0x10000;
    smemRegFldGet(devObjPtr, regAddr, 4, 1, &fldValue);
    /* Ingress Monitor Source */
    if (fldValue)
    {
        smemRegFldGet(devObjPtr, GLB_MON_CTRL_REG, 12, 4, &fldValue);
        /* Ingress monitor destination */
        __LOG(("Ingress monitor destination"));
        inMonitorDest = (GT_U8)fldValue;
        if (descrPtr->destPortVector & inMonitorDest)
        {
            mirrPtr->monFwd = 1;
        }
        else
        {
            descrPtr->destPortVector |= 1 << inMonitorDest;
        }
        mirrPtr->inMon = 1;
    }

    /* Egress Monitor Source */
    __LOG(("Egress Monitor Source"));
    for (destPortInd = 0; destPortInd < devObjPtr->portsNumber; destPortInd++)
    {
       if ((1 << destPortInd) & descrPtr->destPortVector)
       {
            destRegAddr =  PORT_CTRL_2_REG + destPortInd * 0x10000;
            smemRegFldGet(devObjPtr, destRegAddr, 5, 1, &fldValue);

            if (fldValue != 0)
            {
                smemRegFldGet(devObjPtr, GLB_MON_CTRL_REG, 8, 4, &fldValue);
                /* Egress monitor destination */
                outMonitorDest = (GT_U8)fldValue;
                if (descrPtr->destPortVector & outMonitorDest)
                {
                    mirrPtr->monFwd = 1;
                }
                else
                {
                    descrPtr->destPortVector |= 1 << outMonitorDest;
                }
                mirrPtr->outMon = 1;
            }
        }
    }

    if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        if (descrPtr->arp)
        {
            if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
            {/* ARP MIRROR */
                regAddr = PORT_CTRL_2_REG + descrPtr->srcPort * 0x10000;
                smemRegFldGet(devObjPtr, regAddr, 6, 1, &fldValue);
            }

            if  (arpValue == 0)
            {
                smemRegFldGet(devObjPtr, GLB_MON_CTRL_REG, 4, 4, &fldValue);
                /* ARP monitor destination */
                __LOG(("ARP monitor destination"));
                mirrPtr->arpMonitorDest = (GT_U8)fldValue;
                if (mirrPtr->arpMonitorDest != 0xf)
                {
                    if ((descrPtr->destPortVector >> mirrPtr->arpMonitorDest) & 0x1)
                    {
                        mirrPtr->monFwd = 1;
                    }
                    else
                    {
                        descrPtr->destPortVector |= (1 << mirrPtr->arpMonitorDest);
                    }
                    mirrPtr->outMon = 1;
                }
                else
                {
                    descrPtr->arp = 0;
                }
            }
        }
        if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
        {

            if ((descrPtr->uDPpolicy == 1) || (descrPtr->oPt82policy == 1) ||
                (descrPtr->vBaspolicy == 1) || (descrPtr->pPppolicy == 1) ||
                (descrPtr->eTypepolicy == 1) || (descrPtr->vTupolicy == 1) ||
                (descrPtr->sApolicy == 1) || (descrPtr->oPt82policy == 1))
                {
                    descrPtr->polMirror = 1;
                    smemRegFldGet(devObjPtr, GLB_MON_CTRL_REG, 0, 4, &fldValue);

                    if ( fldValue != 0xf)
                    {
                        if ((descrPtr->destPortVector >> mirrPtr->arpMonitorDest) & 0x1)
                        {
                            mirrPtr->monFwd = 1;
                        }
                        else
                        {
                            descrPtr->destPortVector |= (1 << mirrPtr->arpMonitorDest);
                        }
                    }
                    else
                        descrPtr->polMirror = 0;
                }

            else
                descrPtr->polMirror = 0;
        }
    }
}

/*******************************************************************************
*   snetSohoQcMonitorEgress
*
* DESCRIPTION:
*       Monitoring support in the QC Egress
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       mirrPtr     - tx/rx mirror analyser flags.
*
*******************************************************************************/
static GT_VOID snetSohoQcMonitorEgress
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr,
    IN SNET_SOHO_MIRROR_STC * mirrPtr
)
{
    DECLARE_FUNC_NAME(snetSohoQcMonitorEgress);

    /* QC Monitor handling - Egress  */
    __LOG(("QC Monitor handling - Egress"));
    if (mirrPtr->monFwd || (mirrPtr->inMon == mirrPtr->outMon))
    {
        descrPtr->txSnif = 0;
        descrPtr->rxSnif = 0;
        if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
            descrPtr->polMirror = 0;
        snetSohoEgressPacketProc(devObjPtr,descrPtr);
    }
    if (mirrPtr->inMon == 1)
    {
        descrPtr->rxSnif = 1;
        descrPtr->txSnif = 0;
        descrPtr->Mgmt = 1;
        if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
            descrPtr->polMirror = 0;
        snetSohoEgressPacketProc(devObjPtr,descrPtr);
    }
    if (mirrPtr->outMon == 1)
    {
        descrPtr->rxSnif = 0;
        descrPtr->txSnif = 1;
        snetSohoEgressPacketProc(devObjPtr,descrPtr);
        if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
            descrPtr->polMirror = 0;
    }
    if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
    {
        if (descrPtr->destPortVector & mirrPtr->arpMonitorDest)
        {
            if (descrPtr->arp)
            {
                if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
                        descrPtr->polMirror = 0;
                descrPtr->rxSnif = 0;
                descrPtr->txSnif = 0;
                descrPtr->Mgmt = 1;
                snetSohoEgressPacketProc(devObjPtr,descrPtr);
            }
        }
        if (SKERNEL_DEVICE_FAMILY_SOHO_PLUS(devObjPtr->deviceType))
        {
            if (descrPtr->polMirror)
            {
                descrPtr->rxSnif = 0;
                descrPtr->txSnif = 0;
                descrPtr->Mgmt = 0;
                snetSohoEgressPacketProc(devObjPtr,descrPtr);
            }
        }
    }
}

