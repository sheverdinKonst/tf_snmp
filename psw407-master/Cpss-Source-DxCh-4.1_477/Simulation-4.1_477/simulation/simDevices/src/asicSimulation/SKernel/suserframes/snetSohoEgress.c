/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* snetSohoEgress.c
*
* DESCRIPTION:
*      API definition for Soho Egress Processing.
*
* DEPENDENCIES:
*      None.
*
* FILE REVISION NUMBER:
*      $Revision: 34 $
*
*******************************************************************************/
#include <asicSimulation/SKernel/suserframes/snetSohoEgress.h>
#include <asicSimulation/SKernel/sohoCommon/sregSoho.h>
#include <asicSimulation/SKernel/smem/smem.h>
#include <asicSimulation/SKernel/suserframes/snetSoho.h>
#include <asicSimulation/SKernel/smem/smemSoho.h>
#include <asicSimulation/SKernel/suserframes/snet.h>
#include <asicSimulation/SKernel/skernel.h>
#include <asicSimulation/SLog/simLog.h>

static GT_VOID snetSohoFrwrdPacket
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);
static GT_BOOL snetSohoVtuTables
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr,
    INOUT SOHO_PORT_INFO_STC * port_infoPtr
);
static GT_VOID snetSohoSendFrame
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr,
    IN SOHO_PORT_INFO_STC * port_infoPtr
);
static GT_VOID snetSohoInterSwitchEgress
(
    INOUT SKERNEL_DEVICE_OBJECT *  devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr,
    IN SOHO_PORT_INFO_STC * port_infoPtr
);
static GT_BOOL snetSohoTx2Port
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr,
    IN SOHO_PORT_INFO_STC * port_infoPtr,
    IN GT_U8 command
);
static GT_VOID snetSohoDiscardPacket
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
);
static GT_VOID snetSohoSwitchToNetworkPort
(
    INOUT SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr,
    IN SOHO_PORT_INFO_STC * port_infoPtr
);
static GT_VOID snetSohoStatEgressCountUpdate
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr,
    IN GT_U32 port
);
static GT_VOID snetSohoUpdateOutFilteredCount
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr,
    IN GT_U32 port
);

#define NO_CHANGE            0
#define ADD_MARVELL_TAG      1
#define ADD_VLAN_TAG         2
#define REMOVE_MARVELL_TAG   3
#define REMOVE_VLAN_TAG      4
/*******************************************************************************
*   snetSohoEgressPacketProc
*
* DESCRIPTION:
*       Egress frame process. Start point
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
* COMMENT :
*
*
*******************************************************************************/
GT_VOID snetSohoEgressPacketProc
(
    IN SKERNEL_DEVICE_OBJECT        *     devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC *     descrPtr
)
{

    ASSERT_PTR(devObjPtr);
    ASSERT_PTR(descrPtr);

    switch(descrPtr->pktCmd){
    case SKERNEL_PKT_FORWARD_E:
    case SKERNEL_PKT_TRAP_CPU_E:
        snetSohoFrwrdPacket(devObjPtr, descrPtr);
        break;
    case SKERNEL_PKT_DROP_E:
        snetSohoDiscardPacket(devObjPtr, descrPtr);
        break;
    }
}

/*******************************************************************************
* snetSohoFrwrdPacket
*
* DESCRIPTION:
*        Forward frame process
* INPUTS:
*        devObjPtr - pointer to device object.
*        descrPtr  - pointer to the frame's descriptor.
* OUTPUTS:
* COMMENT:
*        Egress Processing A in 886E813 data sheet specification
*******************************************************************************/
static GT_VOID snetSohoFrwrdPacket
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    DECLARE_FUNC_NAME(snetSohoFrwrdPacket);

    GT_U32 targetPortBmp;
    GT_U8  portInx=0;
    SOHO_PORT_INFO_STC port_info[24];
    GT_U32 fldValue;
    GT_U32 portRegAddr;
    GT_U32 portRegAddr2;
    GT_U8  check_egress_filter = 1 ;

    /* the target port bitmap is bitmap which i get from ingress process */
    targetPortBmp = descrPtr->destPortVector;
    if ( (descrPtr->Mgmt) ||
         ((descrPtr->igmpSnoop) &&
          (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))) )
    {
            check_egress_filter = 0;
    }

    /* Find the port state and the tag mode for every egress port in pp */
    __LOG(("Find the port state and the tag mode for every egress port in pp"));
    for (; portInx<devObjPtr->portsNumber; ++portInx)
    {
        portRegAddr2 = PORT_CTRL_2_REG+ (portInx * 0x10000);
        portRegAddr = PORT_CTRL_REG + (portInx * 0x10000);

        /* port is DPID ? */
        if ((targetPortBmp & (1 << portInx))==0)
                                 continue;

        /* port state  */
        smemRegFldGet(devObjPtr, portRegAddr, 0, 2, &fldValue);
        port_info[portInx].state  = (SOHO_PORT_STATE_ENT)fldValue;

        if (port_info[portInx].state == SNET_SOHO_DISABLED_STATE)
            continue;

        port_info[portInx].num = portInx;
        /* the port is enabled,check tag(egress) mode ,           *
        * to determine tagging .                                 */
        smemRegFldGet(devObjPtr, portRegAddr, 12, 2, &fldValue);
        port_info[portInx].tag_mode = (SOHO_PORT_TAG_EGRESS_MODE_ENT)fldValue;

        /*  don't perform egress filtering for management packet and *
         *  for igmp snoop (unless it is sapphire device            */
        if (check_egress_filter)
        {
            /* if enables 802.1q mode to the vtu table */
            smemRegFldGet(devObjPtr, portRegAddr2, 10, 2, &fldValue);
            port_info[portInx].p_8021q_mode = fldValue;
            if (port_info[portInx].p_8021q_mode != SNET_SOHO_8021Q_MODE_DISABLE)
            {
                if (snetSohoVtuTables(devObjPtr, descrPtr, &port_info[portInx])
                                      == GT_FALSE)
                {   /* egress filtering */
                    snetSohoUpdateOutFilteredCount(devObjPtr, descrPtr, portInx);
                    continue;
                }
            }
        }
        snetSohoSendFrame(devObjPtr,descrPtr,&port_info[portInx]);
    }/* port loop*/
}


/*******************************************************************************
*  snetSohoVltTables
*
* DESCRIPTION:
*   Check with the VTU table , if the port is tagged/untagged in the vlan .
* INPUTS:
*   devObjPtr - pointer to device object.
*   descrPtr  - pointer to the frame's descriptor.
*
* OUTPUTS:
*   port_info - update the vtu_tag_mode of the port info structure .
* COMMENT:
*
*******************************************************************************/
static GT_BOOL snetSohoVtuTables
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr,
    INOUT SOHO_PORT_INFO_STC * port_infoPtr
)
{
    GT_U16  vid = descrPtr->vid ;
    GT_BOOL status = GT_TRUE;
    GT_U32  portMap;
    GT_U32  portState = 0;
    SNET_SOHO_VTU_STC vtuEntry;

    if (smemSohoVtuEntryGet(devObjPtr, vid, &vtuEntry) == GT_OK)
    {
        portMap = vtuEntry.portsMap[port_infoPtr->num];
        if (SKERNEL_DEVICE_FAMILY_SOHO2(devObjPtr->deviceType))
        {
            port_infoPtr->vtu_tag_mode = portMap & 0x3;
            portState = (portMap >> 2) & 0x3;
            if (portState != SNET_SOHO_DISABLED_STATE)
            {
                port_infoPtr->state = portState;
                if (port_infoPtr->state != SNET_SOHO_FORWARD_STATE)
                {
                    status = GT_FALSE;
                }
            }
        }
        else
        {
            if (portMap != SNET_SOHO_8021Q_NOTMEMBER_STATE)
            {
                port_infoPtr->vtu_tag_mode = portMap;
            }
            else
            {
                status = GT_FALSE;
            }
        }
    }
    else
    {
        /* if the state of p_8021q_mode register is
           SNET_SOHO_8021Q_MODE_FALLBACK , then use the port based vlan
           value , don't discard packet*/
        if ( (port_infoPtr->p_8021q_mode==SNET_SOHO_8021Q_MODE_CHECK) ||
             (port_infoPtr->p_8021q_mode==SNET_SOHO_8021Q_MODE_SECURE) )
        {
           status = GT_FALSE;
        }
    }

    return status;
}

/*******************************************************************************
* snetSohoInterSwitchEgress
*
* DESCRIPTION:
*        Send the packet to cascade target port(can be cpu port also).
* INPUTS:
*        devObjPtr      - pointer to device object.
*        descrPtr       - pointer to the frame's descriptor.
*        port_infoPtr   - information on port
* OUTPUTS:
*
*
*******************************************************************************/
static GT_VOID snetSohoInterSwitchEgress
(
    INOUT SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr,
    IN SOHO_PORT_INFO_STC * port_infoPtr
)
{
    GT_BOOL status;
    GT_U8   command;

    if (descrPtr->cascade)      /* interswitch process*/
    {
        command = NO_CHANGE;
        if (descrPtr->rxSnif || descrPtr->txSnif)
        {
            descrPtr->srcTrunk = 0;
            descrPtr->frameBuf->actualDataPtr[15] = 0x00;
            descrPtr->frameBuf->actualDataPtr[16] = 0x0b;
            command = ADD_MARVELL_TAG;
        }
        else if (descrPtr->rmtMngmt)
        {
            command = ADD_MARVELL_TAG;
        }
    }
    else
    {
        command = ADD_MARVELL_TAG;
    }
    if (descrPtr->Mgmt)
    {
        descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
    }
    else if (descrPtr->igmpSnoop)
    {
        descrPtr->pktCmd = SKERNEL_PKT_TRAP_CPU_E;
    }
    status = snetSohoTx2Port(devObjPtr, descrPtr, port_infoPtr, command);
    if (status != GT_TRUE)
    {
        printf("snetSohoInterSwitchEgress: failed to send frame to port=%d\n",\
                port_infoPtr->num);
    }

    return;
}

/*******************************************************************************
*   snetSohoSwitchToNetworkPort
*
* DESCRIPTION:
*        interswitch ingress to network egress
* INPUTS:
*        devObjPtr      - pointer to device object.
*        descrPtr       - pointer to the frame's descriptor.
*        port_infoPtr   - information on port
* OUTPUTS:
*
* COMMENT:
*******************************************************************************/
static GT_VOID snetSohoSwitchToNetworkPort
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr,
    IN SOHO_PORT_INFO_STC * port_infoPtr
)
{
    DECLARE_FUNC_NAME(snetSohoSwitchToNetworkPort);

    GT_BOOL status;
    GT_U8   command;
    GT_U8   can_modify = 0;

    if (descrPtr->Mgmt)/* frame from cpu */
    {
        if (descrPtr->srcVlanTagged)
        {
            command = ADD_VLAN_TAG;
        }
        else
        {
            command = REMOVE_MARVELL_TAG;
        }
        status = snetSohoTx2Port(devObjPtr,descrPtr,port_infoPtr,command);
        if (status==GT_FALSE)
        {
           printf(" snetSohoSwitchToNetworkPort : failed to send frame"\
                  " to port=%d\n",port_infoPtr->num);
        }
        return;
    }
    if (port_infoPtr->p_8021q_mode!=SNET_SOHO_8021Q_MODE_DISABLE)
    {
        can_modify = port_infoPtr->vtu_tag_mode;
    }
    else if (port_infoPtr->tag_mode != SNET_SOHO_EGRESS_MODE_UNMODIFIED)
    {
        can_modify = port_infoPtr->tag_mode;
    }
    if (can_modify != SNET_SOHO_EGRESS_MODE_UNMODIFIED)
    {
       /* the frame can be modify */
        __LOG(("the frame can be modify"));
       if (can_modify == SNET_SOHO_EGRESS_MODE_TAGGED_STATE)
       {
            command =  ADD_VLAN_TAG;
       }
       else
       {
            command =  REMOVE_VLAN_TAG;
       }
    }
    else /* the frame can not be modified  , keep the frame as is */
    {
        if (descrPtr->srcVlanTagged)
        {
            command =  ADD_VLAN_TAG;
        }
        else
        {
            command =  REMOVE_VLAN_TAG;
        }
    }
    status = snetSohoTx2Port(devObjPtr,descrPtr,port_infoPtr,command);
    if (status==GT_FALSE)
    {
        printf(" snetSohoSwitchToNetworkPort : failed to send"\
               " frame to port=%d\n",port_infoPtr->num);
    }
}


/*******************************************************************************
*   snetSohoTx2Port
*
* DESCRIPTION:
*                Send the packet to the egress port.
* INPUTS:
*        devObjPtr              - pointer to device object.
*        descrPtr               - pointer to the frame's descriptor.
*        port_infoPtr           -
*        command                - no change , add tag , add marvell tag...
* OUTPUTS:
*
*
*******************************************************************************/
static GT_BOOL snetSohoTx2Port
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    INOUT SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr,
    IN SOHO_PORT_INFO_STC * port_infoPtr,
    IN GT_U8 command
)
{
    GT_U8_PTR egrBufPtr = 0;
    GT_U8_PTR dataPtr;
    GT_U32_PTR mrvlTagPtr;
    GT_U32 mrvlTag;
    GT_U32 egressBufLen, copySize;
    GT_U8 tagData[4];
    SBUF_BUF_ID frameBuf;
    GT_U32 fldValue;
    GT_U32 regAddr;
    GT_U8  priority_tag = 0;


    GT_U32 getIdrev;                       /* Product Num/Rev port 0 offset 3 */
    GT_U8 getIdData[6];                    /* Title for each response         */
    GT_U16 continueCode;                   /* ATU index a 13 bit index in 8K  */
    GT_U8 num_of_dump_atu;                 /* between 0 .. 48 valid entries   */
    GT_U16 ii,jj;                          /* index                           */
    GT_U8 respRdWr[480];                   /* Data for response in RD/WRITE   */
#define DUMP_ATU_WIDTH_CNS  10             /* 10 bytes per ATU dump entry     */
#define DUMP_ATU_MAX_ENTRIES_CNS 48        /* 48 entries max in ATU dump frame*/
    GT_U8 dumpAtu[DUMP_ATU_MAX_ENTRIES_CNS*DUMP_ATU_WIDTH_CNS];/* Data for response in DumpATU    */
    GT_U8 dumpMib[146];                    /* Data for response in DumpMib    */
    SOHO_DEV_MEM_INFO * memInfoPtr;        /* device's memory pointer         */
    GT_U32 * atuEntryPtr;                  /* ATU entry pointer               */
    GT_U32 atuSize;                        /* ATU database size               */
    GT_U32 entryState;                     /* ATU entry state                 */
    GT_U32 sizedumpAtu;                    /* size of ATU bytes to response   */
    GT_U32  currentTimeStamp;              /* time stamp for MIB operation    */
    GT_U32  counter;                       /* counter addi for MIB operation  */
    GT_U8 port;                            /* from Request for MIB operation  */
    GT_U32 lastData;                       /* end indication for Read/Write   */
    GT_U8 firstOct,secondOct,              /* four octets for Read            */
          thirdOct,fourthOct;              /*   write operation               */
    GT_U8 comand ;                         /* 1st Octet operation type         */
    GT_U8 opCode;                          /* Opcode in Read/Write            */
    GT_U8 smidev;                          /* SMI device address              */
    GT_U8 offsetregAddr;                   /* SMI register offset             */
    GT_U8 mibAndClear;                     /*  Dump Mib & Clear registers     */
    GT_U16 etherType;                      /* Ethertype in RMU packet         */
    GT_U16_PTR etherTypePtr;               /* Ethertype ptr in RMU packet     */

    regAddr = PORT_CTRL_REG + port_infoPtr->num * 0x10000;

    priority_tag = descrPtr->fPri ;

    /* Egress buffer pointer to copy to */
    egrBufPtr = devObjPtr->egressBuffer;
    /* check egress header mode */
    smemRegFldGet(devObjPtr, regAddr, 11, 1, &fldValue);
    if (fldValue)
    {
      /* Egress buffer pointer to copy the 1st byte */
      *egrBufPtr = (descrPtr->Mgmt >0) ? 1 : 0;

      *egrBufPtr = priority_tag << 1;
      *egrBufPtr = (GT_U8)descrPtr->dbNum << 4;

      /* 2nd byte */
      egrBufPtr++;
      *egrBufPtr =  (GT_U8)descrPtr->srcPort ;

    }
    /* Actual data pointer to copy from */
    frameBuf = descrPtr->frameBuf;
    dataPtr = frameBuf->actualDataPtr;
    copySize = 2 * sizeof(SGT_MAC_ADDR_TYP);
    /* Append DA and SA to the egress buffer */
    MEM_APPEND(egrBufPtr, dataPtr, copySize);
    dataPtr += copySize;

    if (command == ADD_MARVELL_TAG)
    {
       mrvlTag = snetSohoCreateMarvellTag(devObjPtr, descrPtr);
       mrvlTagPtr = (GT_U32_PTR) &mrvlTag;
       copySize = sizeof(mrvlTag);
       /* Copy Marvell tag */
       MEM_APPEND(egrBufPtr, mrvlTagPtr, copySize);

       if (descrPtr->srcVlanTagged == 1)
       {
            /* Skip VLAN tag , because it was overwritten by marvell tag */
            dataPtr += 4;
       }
       if  (descrPtr->rmtMngmt)
       {   /* add Ethertype */
           etherType = descrPtr->remethrtype;
           etherTypePtr = (GT_U16_PTR) &etherType;
           copySize = sizeof(etherType);
           /* Copy Ethertype  */
           MEM_APPEND(egrBufPtr, etherTypePtr, copySize);
           dataPtr += 2;
       }
    }
    else if (command == ADD_VLAN_TAG)
    {
        snetTagDataGet(priority_tag,descrPtr->vid,GT_FALSE,tagData);
        copySize = sizeof(tagData);
        MEM_APPEND(egrBufPtr, tagData, copySize);
        if (descrPtr->srcVlanTagged == 1)
        {
            /* Skip VLAN tag , because it was overwritten by vlan tag */
            dataPtr += 4;
        }
        else if (descrPtr->cascade == 1)
        {
            /* Skip Marvell tag , because it was overwritten by vlan tag */
            dataPtr += 4;
        }
    }
    else if (command == REMOVE_VLAN_TAG)
    {
         /* Skip VLAN tag , because it was deleted */
         dataPtr += 4;
    }
    else if (command == REMOVE_MARVELL_TAG)
    {
      /* Skip MARVELL tag , because it needs to be stripped                   */
      if (descrPtr->srcVlanTagged == 0) /* 0 means that the packet should be  *
                                         *   egressed without marvell tag     */
      {
         dataPtr += 4;
      }
    }

    /* RMU packet. */
    if(descrPtr->rmtMngmt == 1)
    { /* building L3 response */

        /* response format */
        getIdData[0] = descrPtr->reqData[0];

        if (descrPtr->reqCode == SNET_OPAL_GETID)
        {
            getIdData[1] = 0x01;
        }
        else
            getIdData[1] = descrPtr->reqData[1];
        /* response code */
        getIdData[4] = descrPtr->reqData[4];
        getIdData[5] = descrPtr->reqData[5];

        /* Product Num/Rev */
        smemRegFldGet(devObjPtr, PORT_SWTC_ID_REG, 0, 16, &getIdrev);
        getIdData[2]= (GT_U8)((getIdrev & 0xff00) >> 8);
        getIdData[3]= (GT_U8)getIdrev;

        copySize = sizeof(getIdData);
        dataPtr += 6;

        MEM_APPEND(egrBufPtr, getIdData, copySize);

        switch (descrPtr->reqCode)
        {
        case SNET_OPAL_DUMP_MIB:

                port = (GT_U8)descrPtr->reqData[7];
                mibAndClear  = (GT_U8)descrPtr->reqData[6] & 0x10;

            ii = 0;
            dumpMib[ii++] = descrPtr->srcDevice;
            dumpMib[ii++] = port << 3;

            currentTimeStamp = SIM_OS_MAC(simOsTime)();
            dumpMib[ii++] = (GT_U8)(currentTimeStamp >> 24) ;
            dumpMib[ii++] = (GT_U8)(currentTimeStamp >> 16) ;
            dumpMib[ii++] = (GT_U8)(currentTimeStamp >> 8) ;
            dumpMib[ii++] = (GT_U8)currentTimeStamp;

            /* first counter address */
               regAddr = CNT_IN_GOOD_OCTETS_LO_REG + port * 0x10000;
            for (jj = 0; jj <= 0x1f ; jj++ , regAddr += 0x10)
            {
                smemRegGet(devObjPtr, regAddr, &counter);
                if (mibAndClear)
                { /* Mib and Clear OPERATION */
                    smemRegSet(devObjPtr, regAddr, 0);
                }
                dumpMib[ii++] = (GT_U8)(counter >> 24);
                dumpMib[ii++] = (GT_U8)(counter >> 16);
                dumpMib[ii++] = (GT_U8)(counter >> 8);
                dumpMib[ii++] = (GT_U8)counter;
            }

            /* high 16 bits of IN DISCARD  */
            regAddr = PORT_INDISCARDHGH_CNTR_REG + (port * 0x10000);
            smemRegGet(devObjPtr, regAddr, &counter);

            dumpMib[ii++] = (GT_U8)(counter >> 8);
            dumpMib[ii++] = (GT_U8)counter;

            if (mibAndClear)
            { /* Mib and Clear OPERATION */
                smemRegSet(devObjPtr, regAddr, 0);
            }

            /* low 16 bits of IN DISCARD */
            regAddr = PORT_INDISCARDLOW_CNTR_REG + (port * 0x10000);
            smemRegGet(devObjPtr, regAddr, &counter);

            dumpMib[ii++] = (GT_U8)(counter >> 8);
            dumpMib[ii++] = (GT_U8)counter;

            if (mibAndClear)
            { /* Mib and Clear OPERATION */
                smemRegSet(devObjPtr, regAddr, 0);
            }

            dumpMib[ii++] = 0;
            dumpMib[ii++] = 0;

            /* low 16 bits of IN FILTERED */
            regAddr = PORT_INFILTERED_CNTR_REG + (port * 0x10000);
            smemRegGet(devObjPtr, regAddr, &counter);

            dumpMib[ii++] = (GT_U8)(counter >> 8);
            dumpMib[ii++] = (GT_U8)counter;

            if (mibAndClear)
            { /* Mib and Clear OPERATION */
                smemRegSet(devObjPtr, regAddr, 0);
            }

            dumpMib[ii++] = 0;
            dumpMib[ii++] = 0;

            /* low 16 bits of OUT FILTERED */
            regAddr = PORT_OUTFILTERED_CNTR_REG + (port * 0x10000);
            smemRegGet(devObjPtr, regAddr, &counter);

            dumpMib[ii++] = (GT_U8)(counter >> 8);
            dumpMib[ii++] = (GT_U8)counter;

            if (mibAndClear)
            { /* Mib and Clear OPERATION */
                smemRegSet(devObjPtr, regAddr, 0);
            }

            MEM_APPEND(egrBufPtr, dumpMib, ii/*number of bytes*/);

            dataPtr += ii;/*number of bytes*/

            break;

        case SNET_OPAL_DUMP_ATU_STATE:

               num_of_dump_atu = 0;

               /* Get pointer to the device memory */
               memInfoPtr = (SOHO_DEV_MEM_INFO *)(devObjPtr->deviceMemory);

               atuSize = memInfoPtr->macDbMem.macTblMemSize /
                            SOHO_ATU_ENTRY_SIZE_IN_WORDS;

               continueCode = descrPtr->reqData[6] << 8 | descrPtr->reqData[7];
               for ( ii = continueCode; ii < atuSize ; ii ++)
               {
                   jj = ii * SOHO_ATU_ENTRY_SIZE_IN_WORDS;
                   /* Get ATU database entry  */
                   atuEntryPtr = &memInfoPtr->macDbMem.macTblMem[jj];

                   entryState = SMEM_U32_GET_FIELD(atuEntryPtr[1], 16, 4);
                   if (entryState)
                   {
                       GT_U32 bucket;                      /* bucket for current mac addr */
                       GT_U32 dbNum;
                       GT_U32  dumpEntryByte = num_of_dump_atu * DUMP_ATU_WIDTH_CNS;/* byte index in dumpAtu[]
                                for current entry */

                       dumpAtu[0+dumpEntryByte] = (GT_U8)
                           ((entryState << 4) |                               /* entry state */
                           ((SMEM_U32_GET_FIELD(atuEntryPtr[2], 2, 1)) << 3) |/* trunk bit */
                           SMEM_U32_GET_FIELD(atuEntryPtr[1], 28, 3));        /*DPV bits 8..10*/

                       dumpAtu[1+dumpEntryByte] = (GT_U8)
                           SMEM_U32_GET_FIELD(atuEntryPtr[1], 20, 8);         /*DPV bits 0..7*/

                       dumpAtu[2+dumpEntryByte] = (GT_U8)
                           SMEM_U32_GET_FIELD(atuEntryPtr[1], 8, 8);          /*macAddr[0]*/

                       dumpAtu[3+dumpEntryByte] = (GT_U8)
                           SMEM_U32_GET_FIELD(atuEntryPtr[1], 0, 8);          /*macAddr[1]*/

                       dumpAtu[4+dumpEntryByte] = (GT_U8)
                           SMEM_U32_GET_FIELD(atuEntryPtr[0], 24, 8);         /*macAddr[2]*/

                       dumpAtu[5+dumpEntryByte] = (GT_U8)
                           SMEM_U32_GET_FIELD(atuEntryPtr[0], 16, 8);         /*macAddr[3]*/

                       dumpAtu[6+dumpEntryByte] = (GT_U8)
                           SMEM_U32_GET_FIELD(atuEntryPtr[0], 8, 8);          /*macAddr[4]*/

                       dumpAtu[7+dumpEntryByte] = (GT_U8)
                           SMEM_U32_GET_FIELD(atuEntryPtr[0], 0, 8);          /*macAddr[5]*/

                       /* calc bucket for dbNum = 0 of current entry */
                       bucket  = snetSohoMacHashCalc(devObjPtr, &dumpAtu[2+dumpEntryByte]);
                       /* calc the dbNum --> bits 0..10
                          NOTE: bit 11 comes from the entry itself */
                       /* the dbNum is the bucket offset from where it would be
                          for dbNum 0 */
                       dbNum = (((atuSize + ii)/SOHO_ATU_BUCKET_SIZE_IN_BINS) - bucket) %
                                (atuSize/SOHO_ATU_BUCKET_SIZE_IN_BINS);
                       /* add bit 11 to the calc */
                       dbNum |= SMEM_U32_GET_FIELD(atuEntryPtr[2],  3, 1) << 11;

                       dumpAtu[8+dumpEntryByte] = (GT_U8)
                            ((SMEM_U32_GET_FIELD(atuEntryPtr[1], 31, 1) << 4) | /* prio bit 0 */
                             (SMEM_U32_GET_FIELD(atuEntryPtr[2],  0, 2)  << 5) | /* prio bits 1,2 */
                              SMEM_U32_GET_FIELD(dbNum, 8, 4) ); /* FID (dbNum) bits 8..11 */

                       dumpAtu[9+dumpEntryByte] = (GT_U8)
                             (SMEM_U32_GET_FIELD(dbNum, 0, 8)); /* FID (dbNum) bits 0..7  */

                       num_of_dump_atu++;
                       if (num_of_dump_atu == DUMP_ATU_MAX_ENTRIES_CNS)
                       {
                           ii++;/* the next entry that caller need to use */
                           break;
                       }
                   }
               }

               ii %= atuSize;

               sizedumpAtu  = num_of_dump_atu * DUMP_ATU_WIDTH_CNS ;

               if(sizedumpAtu + 1 >= DUMP_ATU_MAX_ENTRIES_CNS * DUMP_ATU_WIDTH_CNS)
               {
                   skernelFatalError("Overrun of static array 'dumpAtu'\n");
               }

               dumpAtu[sizedumpAtu]     = (GT_U8)(ii >> 8);
               dumpAtu[sizedumpAtu + 1] = (GT_U8)ii ;


               sizedumpAtu += 2;

               if(sizedumpAtu >= DUMP_ATU_MAX_ENTRIES_CNS * DUMP_ATU_WIDTH_CNS)
               {
                   skernelFatalError("Overrun of static array 'dumpAtu'\n");
               }

               MEM_APPEND(egrBufPtr, dumpAtu, sizedumpAtu );

               dataPtr += sizedumpAtu;
            break;

        case   SNET_OPAL_READ_WRITE_STATE:

                for (ii=1; ii<481;ii++ )
                {
                    if((ii*4 + 2 + 3) >= 480)
                    {
                        skernelFatalError("Overrun of static array 'descrPtr->reqData'\n");
                    }

                    firstOct  = descrPtr->reqData[ii*4 + 2];
                    secondOct = descrPtr->reqData[ii*4 + 2 + 1];
                    thirdOct  = descrPtr->reqData[ii*4 + 2 + 2];
                    fourthOct = descrPtr->reqData[ii*4 + 2 + 3];

                    comand = (firstOct & 0xf0) >> 4;
                    opCode = (firstOct & 0x0c) >> 2;
                    smidev = (firstOct & 0x03) << 3 | (secondOct & 0xe0) >> 5;
                    offsetregAddr = (secondOct & 0x1f);

                    lastData =   firstOct << 24 | secondOct << 16 |
                                 thirdOct << 8  | fourthOct;
                    if (lastData == 0xffffffff)
                    { /* last data */
                       if(((ii-1)*4 + 3) >= 480)
                       {
                          skernelFatalError("Overrun of static array 'respRdWr'\n");
                       }

                       respRdWr[(ii-1)*4]     = firstOct;
                       respRdWr[(ii-1)*4 + 1] = secondOct;
                       respRdWr[(ii-1)*4 + 2] = thirdOct;
                       respRdWr[(ii-1)*4 + 3] = fourthOct;

                       break;
                    }

                    switch (smidev)
                    {   /* port register */
                        case 0x10:
                            regAddr =   SWITCH_PORT0_STATUS_REG + 0x10 * offsetregAddr
                                                  + 0x10000 * descrPtr->srcPort;
                        break;
                        /* global 1 register */
                        case 0x1b:
                            regAddr =   GLB_STATUS_REG + 0x10 * offsetregAddr;
                        break;
                        /* global 2 register */
                        case 0x1c:
                            regAddr =   GLB2_INTERUPT_SOURCE + 0x10 * offsetregAddr;
                        break;
                    }


                    respRdWr[(ii-1)*4]     = firstOct;
                    respRdWr[(ii-1)*4 + 1] = secondOct;
                    if (comand == 0)
                    {   /* read/write operation */
                        if (opCode == 2)
                        {   /* read operation */
                            smemRegFldGet(devObjPtr, regAddr, 0, 16, &fldValue);
                            respRdWr[(ii-1)*4 + 2] = (GT_U8)((fldValue  & 0x0000ff00) >> 8);
                            respRdWr[(ii-1)*4 + 3] = (GT_U8)fldValue;
                        }
                        else if (opCode == 1)
                        {   /* write operation */
                            respRdWr[(ii-1)*4 + 2] = thirdOct;
                            respRdWr[(ii-1)*4 + 3] = fourthOct;
                            fldValue = thirdOct << 8  | fourthOct;
                            smemRmuReadWriteMem(SKERNEL_MEMORY_WRITE_E,devObjPtr,
                                                 regAddr, 1, &fldValue);
                        }
                    }
                    else if (comand == 1)
                    {   /* wait until bit  operation */
                        smemRegFldGet(devObjPtr, regAddr, thirdOct, 1, &fldValue);
                          if (((fldValue == 1) && (opCode == 0)) ||
                            ((fldValue == 0) && (opCode == 0)))

                        {
                            respRdWr[(ii-1)*4 + 2] = thirdOct;
                            respRdWr[(ii-1)*4 + 3] = fourthOct;

                        }
                        else
                            skernelFatalError("wait command  \
                                           bit not ready to be sent \n");
                    }
                    else /* end of list if command != 0,1 */
                    {
                        break;
                    }

                }

                if((ii * 4) >= 480)
                {
                    skernelFatalError("Overrun of static array 'respRdWr'\n");
                }

                MEM_APPEND(egrBufPtr, respRdWr, ii * 4);

                dataPtr += ii * 4;

            break;

        }

    }
    else /*  (descrPtr->rmtMngmt == 1) */
    {
        if (descrPtr->frameBuf->actualDataSize <
            (GT_U32)(dataPtr - frameBuf->actualDataPtr))
        return GT_FALSE;


        copySize = descrPtr->frameBuf->actualDataSize -
                (dataPtr - frameBuf->actualDataPtr);

        /* Copy the rest of the frame data */
        MEM_APPEND(egrBufPtr, dataPtr, copySize);
    }
    egressBufLen = egrBufPtr - devObjPtr->egressBuffer;

    if(egressBufLen > 0)
    {

        if (egressBufLen <= 0x3c)
        {
          memset((&devObjPtr->egressBuffer[egressBufLen]), 0, 0x3c-egressBufLen);
          egressBufLen = 0x3c;
        }
        smainFrame2PortSend(devObjPtr,
                            port_infoPtr->num,
                            devObjPtr->egressBuffer,
                            egressBufLen,
                            GT_FALSE);
        snetSohoStatEgressCountUpdate(devObjPtr,descrPtr,port_infoPtr->num);
    }
    else
    {
       return GT_FALSE;
    }

    return GT_TRUE;
}


/*******************************************************************************
* snetSohoSendFrame
*
* DESCRIPTION:
*    Execute the decision that was taken in the function of snetSohoFrwrdPacket
* INPUTS:
*    devObjPtr      - pointer to device object.
*    descrPtr       - pointer to the frame's descriptor.
*    port_infoPtr   - information on port.
* OUTPUTS:
*
* COMMENT : Defined as Processing B in Link Street 88E6183 Datasheet
*******************************************************************************/
static GT_VOID snetSohoSendFrame
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr,
    IN SOHO_PORT_INFO_STC * port_infoPtr
)
{
    DECLARE_FUNC_NAME(snetSohoSendFrame);

    GT_U32 cascadedPortValue;
    GT_U32 regAddr;
    GT_STATUS status;
    GT_U8 command = NO_CHANGE;

    regAddr = PORT_CTRL_REG + (port_infoPtr->num * 0x10000);

    /* check if the egress port is cascaded port */
    __LOG(("check if the egress port is cascaded port"));
    smemRegFldGet(devObjPtr, regAddr, 8, 1, &cascadedPortValue);
    if (cascadedPortValue)
    {
        snetSohoInterSwitchEgress(devObjPtr,descrPtr,port_infoPtr);
        return;
    }
    regAddr = PORT_CTRL_REG + (descrPtr->srcPort * 0x10000);
    /* check if the ingress port is cascaded port */
    __LOG(("check if the ingress port is cascaded port"));
    smemRegFldGet(devObjPtr, regAddr, 8, 1, &cascadedPortValue);
    if (cascadedPortValue)
    {
        snetSohoSwitchToNetworkPort(devObjPtr,descrPtr,port_infoPtr);
        return;
    }
    /* if the frame is management , then don't modify it . Send the packet */
    __LOG(("if the frame is management , then don't modify it . Send the packet"));
    if (descrPtr->Mgmt)
    {
        status = snetSohoTx2Port(devObjPtr,descrPtr,port_infoPtr,NO_CHANGE);
        if (status!=GT_TRUE)
        {
          printf(" snetSohoFrameChange : failed to send frame to port=%d\n"\
                   ,port_infoPtr->num);
        }
        return;
    }
    /* port to port bridge
    add a tag to the frame if needed .
    VTU table is stronger then the port control.*/
    if (port_infoPtr->vtu_tag_mode!=SNET_SOHO_8021Q_UNMODIFIED_STATE)
    {
         if (port_infoPtr->vtu_tag_mode==SNET_SOHO_8021Q_TAGGED_STATE)
         {
             command = ADD_VLAN_TAG;
         }
         else
         {
             if (descrPtr->srcVlanTagged == 1) /* if packet is tagged then remove tag*/
             {
                 command = REMOVE_VLAN_TAG;
             }
         }
    }
    else if (port_infoPtr->tag_mode != SNET_SOHO_EGRESS_MODE_UNMODIFIED)
    {
        if ( (port_infoPtr->tag_mode==SNET_SOHO_EGRESS_MODE_TAGGED_STATE) ||
             (port_infoPtr->tag_mode==SNET_SOHO_EGRESS_MODE_DOUBLE_TAG_STATE))
        {
              command = ADD_VLAN_TAG ;
        }
        else/* if packet is tagged then remove tag*/
        {
            if (descrPtr->srcVlanTagged == 1) /* if packet is tagged then remove tag*/
            {
                command = REMOVE_VLAN_TAG;
            }
        }
    }
    if (port_infoPtr->state==SNET_SOHO_FORWARD_STATE)
    {
        if (snetSohoTx2Port(devObjPtr,descrPtr,port_infoPtr,command)!=GT_TRUE)
        {
            printf(" snetSohoFrameChange : failed to send frame to port=%d\n",\
                    port_infoPtr->num);
        }
    }
}

/*******************************************************************************
*   snetSohoDiscardPacket
*
* DESCRIPTION:
*        Drop packet.
*
* INPUTS:
*        devObjPtr                - pointer to device object.
*        descrPtr                - pointer to the frame's descriptor.
* OUTPUTS:
*
*
*******************************************************************************/
static GT_VOID snetSohoDiscardPacket
(
    IN SKERNEL_DEVICE_OBJECT * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr
)
{
    return;
}

/*******************************************************************************
*   snetSohoStatEgressCountUpdate
*
* DESCRIPTION:
*       Update Stats Counter Egress Group
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*
*
*******************************************************************************/
static GT_VOID snetSohoStatEgressCountUpdate
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr,
    IN GT_U32 port
)
{
    DECLARE_FUNC_NAME(snetSohoStatEgressCountUpdate);

    GT_U32 regAddr;                     /* registers address and value */
    SMEM_REGISTER  counter=0;             /* counter's value */
    GT_U32  fldValue=0;


    if (SGT_MAC_ADDR_IS_BCST(descrPtr->dstMacPtr))
    {
        regAddr = CNT_OUT_BCST_REG + port * 0x10000;
        smemRegGet(devObjPtr, regAddr, &counter);
        counter++;
        smemRegSet(devObjPtr, regAddr, counter);
    }
    if (SGT_MAC_ADDR_IS_MCST(descrPtr->dstMacPtr))
    {
        regAddr = CNT_OUT_MCST_REG + port * 0x10000;
        smemRegGet(devObjPtr, regAddr, &counter);
        counter++;
        smemRegSet(devObjPtr, regAddr, counter);
    }
    else
    {   /* Good frames sent */
        __LOG(("Good frames sent"));
        regAddr = CNT_OUT_UNICAST_FRAMES_REG + port * 0x10000;
        smemRegGet(devObjPtr, regAddr, &counter);
        counter++;
        smemRegSet(devObjPtr, regAddr, counter);
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

    regAddr = CNT_OUT_OCTETS_LO_REG + port * 0x10000;
    smemRegGet(devObjPtr, regAddr, &counter);
    counter+= descrPtr->byteCount;
    smemRegSet(devObjPtr, regAddr, counter);

}

/*******************************************************************************
*   snetSohoUpdateOutFilteredCount
*
* DESCRIPTION:
*       Update outfiltered counter
*
* INPUTS:
*       devObjPtr   - pointer to device object.
*       descrPtr    - pointer to the frame's descriptor.
*       port        - egress port.
*
*
*******************************************************************************/
static GT_VOID snetSohoUpdateOutFilteredCount
(
    IN SKERNEL_DEVICE_OBJECT  * devObjPtr,
    IN SKERNEL_FRAME_SOHO_DESCR_STC * descrPtr,
    IN GT_U32 port
)
{
    GT_U32      regAddress;
    GT_U32      counter;

    regAddress = PORT_OUTFILTERED_CNTR_REG + (port * 0x10000);
    smemRegGet(devObjPtr, regAddress, &counter);
    ++counter;
    smemRegSet(devObjPtr, regAddress, counter);
}

