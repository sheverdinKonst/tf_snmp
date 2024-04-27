/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.

*******************************************************************************/

#include "mv_prestera_switch_test.h"
#include "presteraSwitchEnd.h"
#include "cntmr\mvCntmr.h"
#include "mvPrestera.h"
#include "mvPresteraPriv.h"

#if 0
#define DB(x) x
#else
#define DB(x)
#endif

/*
 * How to run the test
 * 1. Run with no cached buffers - Add to the makefile -
 *    EXTRA_DEFINE += -DUNCACHED_TX_BUFFERS -DUNCACHED_RX_BUFFERS
 * 2. To run test, when the cpu update the MAC entry only:
 *    add - #define UPDATE_MAC
 *    To run test, when switch add DSA TAG, and CPU update the DSA tag:
 *    add - #define DSA_TAG
 *    To run with DSA tag and with update message - use the both defines.
 * 3. To run test - add #define BI_DIRECTIONAL
 * 4. Test process:
 *    run - switchTestInit(int rxPort, int txPort)
 *    send packets from rxPort with MAC DA -
 *    {0x00,0x45,0x78,0x14,0x59,<send port>}.
 */

MV_U8 switchTestUserDefMacAddr[6] = {0x00, 0x45, 0x78, 0x14, 0x59, 0x00};
extern SDMA_INTERRUPT_CTRL G_sdmaInterCtrl[SYS_CONF_MAX_DEV];
RX_DESC_LIST   *rxDescList;
TX_DESC_LIST   *txDescList;
MV_BOOL         alreadyInit = MV_FALSE;

MV_U32 G_measureLoopbackTimeRxTs = 0;
MV_U32 G_measureLoopbackTimeTxTs = 0;

#define MAX_PORT                28
#define MRVL_TAG_PORT_BIT       19
#define MRVL_TAG_EXT_PORT_BIT   10

#define TEST_RX_DESC_NUM        PRESTERA_RXQ_LEN * 100
#define TEST_TX_DESC_NUM        PRESTERA_TXQ_LEN * 100

#define PRESTERA_TXQ_MAX_FREE_DESC (NUM_OF_TX_QUEUES * PRESTERA_TXQ_LEN)

#define UPDATE_MAC
#define DSA_TAG
#undef  BI_DIRECTIONAL

#ifdef BI_DIRECTIONAL
#  ifndef UPDATE_MAC
#    define UPDATE_MAC
#  endif
#endif

#undef  UPDATE_MAC

/*
 * functions prototype
 */
MV_BOOL switchTestInit(MV_U32 rxPort, MV_U32 txPort);
MV_U32 mvSwitchRxTest(MV_U32 devNum, MV_U32 rxPort, MV_U32 txPort);
MV_U32 mvSwitchLoopbackTest(MV_U32 devNum, MV_U32 rxPort, MV_U32 txPort);

MV_STATUS mvSwitchInit_test(MV_U32 dev)
{
    /* disable Rx queues - before enable */
    CHECK_STATUS(mvSwitchWriteReg(dev, 0x2680, 0xFF00));

    /* SDMA - disable retransmit on resource error */
    CHECK_STATUS(mvSwitchReadModWriteReg(dev, 0x2800, 0xff00, 0xff00));

    CHECK_STATUS(mvSwitchBitSet(dev, CASCADE_AND_HEADER_CONFIG_REG,
                                                         CPU_PORT_DSA_EN_BIT));
    /* CHECK_STATUS(mvSwitchBitSet(dev, CASCADE_AND_HEADER_CONFIG_REG,
                                            PREPEND_TWO_BYTES_HEADER_BIT)); */

    if (setCpuAsVLANMember(dev, 1 /* vlan */) != MV_OK)
    {
        mvOsPrintf("Error: (Prestera) Unable to set CPU as VLAN member.\n");
        return MV_FAIL;
    }

    /* init the hal:
     * create internal port control structure and descriptor rings
     */
    if (mvInitSdmaNetIfDev(dev, 20,
                           NUM_OF_TX_QUEUES * PRESTERA_TXQ_LEN,
                           NUM_OF_RX_QUEUES * PRESTERA_RXQ_LEN,
                           SWITCH_MTU) != MV_OK)
    {
        mvOsPrintf( "Error: (Prestera) Unable to initialize SDMA\n");
    }

    return MV_OK;
}

MV_BOOL switchTestInit(MV_U32 rxPort, MV_U32 txPort)
{
    MV_U8 macAddr[18];
    int dev;

    if (alreadyInit)
    {
        mvOsPrintf("switchTestInit aleady called");
        return MV_FALSE;
    }

    alreadyInit = MV_TRUE;

    /*
     * init the both devices anyway (may be not needed)
     */
    for (dev = 0; dev < mvSwitchGetDevicesNum(); dev++)
    {
        mvSwitchOpenMemWinAsKirkwood(dev);
        mvSwitchInit_test(dev);
    }

    mvPpCascadePortCfg();
    mvSwitchVidxCfg();
    mvSwitchEgressFilterCfg();

    /* all ports are members of VIDX 1 except [dev 0, port 26] */
    if (mvSwitchWriteReg(PRESTERA_DEFAULT_DEV, 0xA100010, 0x1FFFFFFF) != MV_OK)
    {
        mvOsPrintf("%s: mvSwitchWriteReg failed.\n", __func__);
        return MV_FALSE;
    }

    if (mvSwitchBitReset(PRESTERA_DEFAULT_DEV, 0xA100010,
                                               BIT_27 /* port 26 */) != MV_OK)
    {
        mvOsPrintf("%s: mvSwitchWriteReg failed.\n", __func__);
        return MV_FALSE;
    }

    /*
     * Set default MAC address
     */
    memcpy(macAddr, switchTestUserDefMacAddr, MV_MAC_ADDR_SIZE);
    macAddr[5] = rxPort;

    if (setCPUAddressInMACTAble(PRESTERA_DEFAULT_DEV, macAddr, 1 /* vid */) != MV_OK)
    {
        mvOsPrintf("Error: (Prestera) Unable to teach CPU MAC address.\n");
        return MV_FALSE;
    }

#ifdef BI_DIRECTIONAL
#  ifndef DSA_TAG
    macAddr[5] = rxPort + MAX_PORT;
    if (setMACAddressInMACTAble(PRESTERA_DEFAULT_DEV, macAddr, 1, txPort) != MV_OK)
    {
        mvOsPrintf("Error: (Prestera) Unable to teach CPU MAC address.\n");
        return MV_FALSE;
    }

    macAddr[5] = txPort + MAX_PORT;
    if (setMACAddressInMACTAble(PRESTERA_DEFAULT_DEV, macAddr, 1, rxPort) != MV_OK)
    {
        mvOsPrintf("Error: (Prestera) Unable to teach CPU MAC address.\n");
        return MV_FALSE;
    }
#  endif
#endif

    /*
     * unreachable code
     */
    mvOsPrintf("unreachable code.\n");
    return MV_FALSE;
}

MV_U32 mvSwitchRxTest(MV_U32 devNum, MV_U32 rxPort, MV_U32 txPort)
{
    STRUCT_SW_TX_DESC *currSwTxDescP;
    STRUCT_SW_RX_DESC *swRxDescP;
    STRUCT_RX_DESC    *rxDescP;
    /* STRUCT_TX_DESC    *txDescP; */
    STRUCT_TX_DESC     txDesc;
    MV_U32             rxDsaWord0, rxDsaWord1, txDsaWord0, txDsaWord1;
    MV_U32             word1, bufPhysAddr;
#ifdef UPDATE_MAC
    MV_U8              tmp;
#endif

#if defined(DSA_TAG) && defined(BI_DIRECTIONAL)
    MV_U32 dsaWord0, dsaWord1;
#endif

    mvOsMemset(&txDesc, 0, sizeof(STRUCT_TX_DESC));
    TX_DESC_SET_RECALC_CRC_BIT(&txDesc, 1);
    TX_DESC_SET_LAST_BIT      (&txDesc, 1);
    TX_DESC_SET_INT_BIT       (&txDesc, 1);
    TX_DESC_SET_FIRST_BIT     (&txDesc, 1);
    TX_DESC_SET_OWN_BIT       (&txDesc, MV_OWNERSHIP_DMA);

    txDsaWord0 = calcFromCpuDsaTagWord0(devNum, txPort);
    txDsaWord1 = calcFromCpuDsaTagWord1(txPort);

    rxDsaWord0 = calcFromCpuDsaTagWord0(devNum, rxPort);
    rxDsaWord1 = calcFromCpuDsaTagWord1(rxPort);

    rxDescList = &(G_sdmaInterCtrl[0].rxDescList[0]);
    txDescList = &(G_sdmaInterCtrl[0].txDescList[0]);

    while (1)
    {
        if (txDescList->freeDescNum != PRESTERA_TXQ_MAX_FREE_DESC)
        {
            if (IS_TX_DESC_DMA_OWNED(txDescList->next2Free->txDesc))
            {
                /* ETH_DESCR_FLUSH_INV(NULL, txDescList->next2Free->txDesc); */
            }
            else
            {
                rxDescList->rxFreeDescNum++;
                rxDescP = rxDescList->next2Return->rxDesc;
                rxDescP->buffPointer = hwByteSwap((MV_U32)txDescList->next2Free->txDesc->buffPointer);
                RX_DESC_RESET(rxDescP);
                rxDescList->next2Return = rxDescList->next2Return->swNextDesc;
                txDescList->freeDescNum++;
                txDescList->next2Free = txDescList->next2Free->swNextDesc;
            }
        }

        if (rxDescList->rxFreeDescNum == 0)
        {
            TX_DESC_SET_OWN_BIT(txDescList->next2Free->txDesc, MV_OWNERSHIP_CPU);
            continue;
        }

        swRxDescP = rxDescList->next2Receive;
        word1 = hwByteSwap(swRxDescP->rxDesc->word1);

        if (word1 & 0x80000000)
        {
            continue;
        }

        rxDescList->next2Receive = swRxDescP->swNextDesc;
        rxDescList->rxFreeDescNum--;
        bufPhysAddr = hwByteSwap(swRxDescP->rxDesc->buffPointer);

        /*
         * good rx - send the packet back to the switch
         */

#ifdef UPDATE_MAC
        tmp = ((MV_U8 *)bufPhysAddr)[5];
        ((MV_U8 *)bufPhysAddr)[5]  = ((MV_U8 *)bufPhysAddr)[11];
        ((MV_U8 *)bufPhysAddr)[11] = tmp;
#endif

#ifdef DSA_TAG
#  ifndef BI_DIRECTIONAL
        /*
         * DSA & uni-direction -
         * update to FROM CPU packet with target port = txPort
         */
        ((MV_U32*)bufPhysAddr)[3]  = txDsaWord0;
        ((MV_U32*)bufPhysAddr)[4]  = txDsaWord1;
#  else
        /*
         * DSA & bi-direction - update to FROM CPU packet with target port =
         * according to the source port
         */
        dsaWord0 = ((MV_U32*)bufPhysAddr)[3];
        dsaWord1 = ((MV_U32*)bufPhysAddr)[4];
        if (DSA_GET_SRC_PORT(MV_BYTE_SWAP_32BIT(dsaWord0),
                             MV_BYTE_SWAP_32BIT(dsaWord1)) == rxPort)
        {
            ((MV_U32*)bufPhysAddr)[3] = txDsaWord0;
            ((MV_U32*)bufPhysAddr)[4] = txDsaWord1;
        }
        else
        {
            ((MV_U32*)bufPhysAddr)[3] = rxDsaWord0;
            ((MV_U32*)bufPhysAddr)[4] = rxDsaWord1;
        }
#  endif
#endif
        currSwTxDescP = txDescList->next2Feed;

        /* neta ETH_PACKET_CACHE_FLUSH(bufPhysAddr,bufSize); */

        currSwTxDescP->txDesc->buffPointer = hwByteSwap(bufPhysAddr);

        txDescList->freeDescNum--;
        txDescList->next2Feed = currSwTxDescP->swNextDesc;

        currSwTxDescP->txDesc->word1 = txDesc.word1;
        TX_DESC_SET_BYTE_CNT(currSwTxDescP->txDesc,
                             RX_DESC_GET_BYTE_COUNT_FIELD(swRxDescP->rxDesc));

        *((volatile MV_U32 *)(0xf4000000+ 0x2868)) = (MV_U32)MV_32BIT_LE_FAST(0x1);
    }


#ifndef _DIAB_TOOL
    return 0; /* make the compiler happy */
#endif
}

/*******************************************************************************
 * PHY 88E1340 MANAGEMENT FUNCTIONS --- START
 *
 *******************************************************************************/

#define PHY88E1340_RD_OP             (BIT_26)
#define PHY88E1340_WR_OP             (0) /* bit 26 is 0 */
#define PHY88E1340_PAGE_ADDR_REG     (22)
#define SMI_REG_BUSY_BIT             (28)
#define SMI_OP_DONE_MAX_TRIALS       (1000)

INLINE MV_U32 PRESTERA_SMI_REG_ADDR(MV_U32 smiId);
INLINE MV_U32 PRESTERA_PHY_ADDR_REG0(MV_U32 smiId);
INLINE MV_U32 PRESTERA_PHY_ADDR_REG1(MV_U32 smiId);
INLINE MV_U32 PRESTERA_PHY_ADDR_REG2(MV_U32 smiId);
INLINE MV_U32 PRESTERA_PHY_ADDR_REG3(MV_U32 smiId);
MV_STATUS mvSwitchGetPhyAddr(MV_U32 devId, MV_U32 smiId, MV_U32 portId, MV_U32 *pPhyAddr);
INLINE MV_STATUS mvSwitchSmiDoOpWait(MV_U32 devId, MV_U32 smiId, MV_U32 value);
MV_STATUS mvSwitchSmiReadRegWait(MV_U32 devId, MV_U32 smiId,
                                 MV_U32 value, MV_U32 *pReadVal);
MV_STATUS mvSwitchSmiWriteRegWait(MV_U32 devId, MV_U32 smiId, MV_U32 value);
MV_STATUS mvSwitchSmiIsOpDone(MV_U32 devId, MV_U32 smiId);

MV_STATUS setExtLoopback_Phy88E1340(MV_U32 devId, MV_U32 smiId, MV_U32 portId);
MV_STATUS setLoopback_Phy88E1340(MV_U32 devId, MV_U32 smiId, MV_U32 portId);
MV_STATUS unsetLoopback_Phy88E1340(MV_U32 devId, MV_U32 smiId, MV_U32 portId);
MV_STATUS setPage_Phy88E1340(MV_U32 devId, MV_U32 smiId, MV_U32 portId, MV_U32 pageId);
MV_STATUS readRegPaged_Phy88E1340(MV_U32 devId, MV_U32 smiId, MV_U32 portId,
                                  MV_U32 regId, MV_U32 *readVal);
MV_STATUS writeRegPaged_Phy88E1340(MV_U32 devId, MV_U32 smiId, MV_U32 portId,
                                   MV_U32 regId, MV_U32 valToWrite);
MV_STATUS readReg_Phy88E1340(MV_U32 devId, MV_U32 smiId, MV_U32 portId,
                             MV_U32 regId, MV_U32 pageId, MV_U32 *readVal);
MV_STATUS writeReg_Phy88E1340(MV_U32 devId, MV_U32 smiId, MV_U32 portId,
                              MV_U32 regId, MV_U32 pageId, MV_U32 value);
MV_STATUS readModWriteReg_Phy88E1340(MV_U32 devId, MV_U32 smiId, MV_U32 portId,
                                     MV_U32 regId, MV_U32 pageId, MV_U32 mask,
                                     MV_U32 value);

INLINE MV_STATUS mvSwitchSmiDoOpWait(MV_U32 devId, MV_U32 smiId, MV_U32 value)
{
    MV_STATUS rc;
    MV_U32 addr;

    addr = PRESTERA_SMI_REG_ADDR(smiId);

    CHECK_STATUS(mvSwitchWriteReg(devId, addr, value));

    rc = mvSwitchSmiIsOpDone(devId, smiId);
    if (rc != MV_OK)
    {
        mvOsPrintf("mvSwitchSmiDoOpWait() - mvSwitchSmiIsOpDone() failed.\n");
        return rc;
    }

    return MV_OK;
}

MV_STATUS mvSwitchSmiReadRegWait(MV_U32 devId, MV_U32 smiId,
                                 MV_U32 value, MV_U32 *pReadVal)
{
    MV_STATUS rc;

    rc = mvSwitchSmiDoOpWait(devId, smiId, value | PHY88E1340_RD_OP);
    if (rc != MV_OK)
    {
        mvOsPrintf("mvSwitchSmiReadRegWait() - mvSwitchSmiDoOpWait() failed.\n");
        return rc;
    }

    CHECK_STATUS(mvSwitchReadReg(devId, PRESTERA_SMI_REG_ADDR(smiId), pReadVal));
    *pReadVal &= 0xFFFF; /* data is stored in 16 LSB */

    return MV_OK;
}

MV_STATUS mvSwitchSmiWriteRegWait(MV_U32 devId, MV_U32 smiId, MV_U32 value)
{
    MV_STATUS rc;

    rc = mvSwitchSmiDoOpWait(devId, smiId, value | PHY88E1340_WR_OP);
    if (rc != MV_OK)
    {
        mvOsPrintf("mvSwitchSmiWriteRegWait() - mvSwitchSmiDoOpWait() failed.\n");
        return rc;
    }

    return MV_OK;
}

MV_STATUS setPage_Phy88E1340(MV_U32 devId, MV_U32 smiId, MV_U32 portId, MV_U32 pageId)
{
    MV_STATUS rc;
    MV_U32 value, regId, phyAddr;

    rc = mvSwitchGetPhyAddr(devId, smiId, portId, &phyAddr);
    if (rc != MV_OK)
    {
        mvOsPrintf("setPage_Phy88E1340() - mvSwitchGetPhyAddr() failed.\n");
        return rc;
    }

    regId   = PHY88E1340_PAGE_ADDR_REG;
    value   = ((phyAddr & 0x1F) << 16) | ((regId & 0x1F) << 21) | (pageId & 0xFFFF);

    return mvSwitchSmiWriteRegWait(devId, smiId, value);
}

MV_STATUS mvSwitchGetPhyAddr(MV_U32 devId, MV_U32 smiId, MV_U32 portId_in, MV_U32 *pPhyAddr)
{
    MV_U32 addr, value;
    volatile MV_U32 portId = portId_in;

    if (portId <= 5)
    {
        addr = PRESTERA_PHY_ADDR_REG0(smiId);
        CHECK_STATUS(mvSwitchReadReg(devId, addr, &value));
        value = (value & (0x1F << portId)) >> portId;
        *pPhyAddr = value;
        return MV_OK;
    }

    if (portId >= 6 && portId <= 11)
    {
        addr = PRESTERA_PHY_ADDR_REG1(smiId);
        CHECK_STATUS(mvSwitchReadReg(devId, addr, &value));
        portId -= 6;
        value = (value & (0x1F << portId)) >> portId;
        *pPhyAddr = value;
        return MV_OK;
    }

    if (portId >= 12 && portId <= 17)
    {
        addr = PRESTERA_PHY_ADDR_REG2(smiId);
        CHECK_STATUS(mvSwitchReadReg(devId, addr, &value));
        portId -= 12;
        value = (value & (0x1F << portId)) >> portId;
        *pPhyAddr = value;
        return MV_OK;
    }

    if (portId >= 18 && portId <= 23)
    {
        addr = PRESTERA_PHY_ADDR_REG2(smiId);
        CHECK_STATUS(mvSwitchReadReg(devId, addr, &value));
        portId -= 18;
        value = (value & (0x1F << portId)) >> portId;
        *pPhyAddr = value;
        return MV_OK;
    }

    return MV_OK;
}

INLINE MV_U32 PRESTERA_PHY_ADDR_REG0(MV_U32 smiId)
{
    MV_U32 phyAddrReg0;

    if (smiId == 0)
        phyAddrReg0 = 0x4004030;
    else
        phyAddrReg0 = 0x5004030;

    return phyAddrReg0;
}

INLINE MV_U32 PRESTERA_PHY_ADDR_REG1(MV_U32 smiId)
{
    MV_U32 phyAddrReg1;

    if (smiId == 0)
        phyAddrReg1 = 0x4804030;
    else
        phyAddrReg1 = 0x5804030;

    return phyAddrReg1;
}

INLINE MV_U32 PRESTERA_PHY_ADDR_REG2(MV_U32 smiId)
{
    MV_U32 phyAddrReg2;

    if (smiId == 0)
        phyAddrReg2 = 0x5004030;
    else
        phyAddrReg2 = 0x5804030;

    return phyAddrReg2;
}

INLINE MV_U32 PRESTERA_PHY_ADDR_REG3(MV_U32 smiId)
{
    MV_U32 phyAddrReg3;

    if (smiId == 0)
        phyAddrReg3 = 0x5804030;
    else
        phyAddrReg3 = 0x5804030;

    return phyAddrReg3;
}

INLINE MV_U32 PRESTERA_SMI_REG_ADDR(MV_U32 smiId)
{
    MV_U32 smiRegAddr;

    if (smiId == 0)
        smiRegAddr = 0x4004054;
    else
        smiRegAddr = 0x5004054;

    return smiRegAddr;
}

MV_STATUS mvSwitchSmiIsOpDone(MV_U32 devId, MV_U32 smiId)
{
    MV_U32 retryCnt, value, addr;

    addr = PRESTERA_SMI_REG_ADDR(smiId);

    for (retryCnt = 0; retryCnt < SMI_OP_DONE_MAX_TRIALS; retryCnt++)
    {
        CHECK_STATUS(mvSwitchReadReg(devId, addr, &value));
        if ((value & SMI_REG_BUSY_BIT) == 0) /* not busy */
            break;
    }

    if (retryCnt == SMI_OP_DONE_MAX_TRIALS)
        return MV_FAIL;

    return MV_OK;
}

/*
 * assumes that PHY page is preconfigured
 */
MV_STATUS readRegPaged_Phy88E1340(MV_U32 devId, MV_U32 smiId, MV_U32 portId,
                                  MV_U32 regId, MV_U32 *pReadVal)
{
    MV_STATUS rc;
    MV_U32 value, phyAddr;

    rc = mvSwitchGetPhyAddr(devId, smiId, portId, &phyAddr);
    if (rc != MV_OK)
    {
        mvOsPrintf("readRegPaged_Phy88E1340() - mvSwitchGetPhyAddr() failed.\n");
        return rc;
    }

    value = ((phyAddr & 0x1F) << 16) | ((regId & 0x1F) << 21);

    rc = mvSwitchSmiReadRegWait(devId, smiId, value, pReadVal);
    if (rc != MV_OK)
    {
        mvOsPrintf("readRegPaged_Phy88E1340() - mvSwitchSmiWriteRegWait() failed.\n");
        return rc;
    }

    CHECK_STATUS(mvSwitchReadReg(devId, PRESTERA_SMI_REG_ADDR(smiId), pReadVal));
    *pReadVal = (*pReadVal & 0xFFFF);
    return MV_OK;
}

/*
 * assumes that PHY page is preconfigured
 */
MV_STATUS writeRegPaged_Phy88E1340(MV_U32 devId, MV_U32 smiId, MV_U32 portId,
                                   MV_U32 regId, MV_U32 valToWrite)
{
    MV_STATUS rc;
    MV_U32 value, phyAddr;

    rc = mvSwitchGetPhyAddr(devId, smiId, portId, &phyAddr);
    if (rc != MV_OK)
    {
        mvOsPrintf("writeRegPaged_Phy88E1340() - mvSwitchGetPhyAddr() failed.\n");
        return rc;
    }

    value = ((phyAddr & 0x1F) << 16) | ((regId & 0x1F) << 21) | (valToWrite & 0xFFFF);

    rc = mvSwitchSmiWriteRegWait(devId, smiId, value);
    if (rc != MV_OK)
    {
        mvOsPrintf("readRegPaged_Phy88E1340() - mvSwitchSmiWriteRegWait() failed.\n");
        return rc;
    }

    return MV_OK;
}

MV_STATUS readReg_Phy88E1340(MV_U32 devId, MV_U32 smiId, MV_U32 portId,
                             MV_U32 regId, MV_U32 pageId, MV_U32 *readVal)
{
    MV_STATUS rc;

    rc = setPage_Phy88E1340(devId, smiId, portId, pageId);
    if (rc != MV_OK)
    {
        mvOsPrintf("readReg_Phy88E1340() - setPage_Phy88E1340() failed.\n");
        return rc;
    }

    rc = readRegPaged_Phy88E1340(devId, smiId, portId, regId, readVal);
    if (rc != MV_OK)
    {
        mvOsPrintf("readReg_Phy88E1340() - readRegPaged_Phy88E1340() failed.\n");
        return rc;
    }

    return MV_OK;
}

MV_STATUS writeReg_Phy88E1340(MV_U32 devId, MV_U32 smiId, MV_U32 portId,
                              MV_U32 regId, MV_U32 pageId, MV_U32 value)
{
    MV_STATUS rc;

    rc = setPage_Phy88E1340(devId, smiId, portId, pageId);
    if (rc != MV_OK)
    {
        mvOsPrintf("writeReg_Phy88E1340() - setPage_Phy88E1340() failed.\n");
        return rc;
    }

    rc = writeRegPaged_Phy88E1340(devId, smiId, portId, regId, value);
    if (rc != MV_OK)
    {
        mvOsPrintf("writeReg_Phy88E1340() - readRegPaged_Phy88E1340() failed.\n");
        return rc;
    }

    return MV_OK;
}

MV_STATUS readModWriteReg_Phy88E1340(MV_U32 devId, MV_U32 smiId, MV_U32 portId,
                                     MV_U32 regId, MV_U32 pageId, MV_U32 mask,
                                     MV_U32 value)
{
    MV_STATUS rc;
    MV_U32 regData;

    rc = setPage_Phy88E1340(devId, smiId, portId, 0 /* page */);
    if (rc != MV_OK)
    {
        mvOsPrintf("readModWriteReg_Phy88E1340() - setPage_Phy88E1340() failed.\n");
        return rc;
    }

    rc = readRegPaged_Phy88E1340(devId, smiId, portId, regId, &regData);
    if (rc != MV_OK)
    {
        mvOsPrintf("readModWriteReg_Phy88E1340() - setPage_Phy88E1340() failed.\n");
        return rc;
    }

    regData = (regData & ~mask) | (value & mask);

    rc = writeRegPaged_Phy88E1340(devId, smiId, portId, regId, regData);
    if (rc != MV_OK)
    {
        mvOsPrintf("readModWriteReg_Phy88E1340() - writeReg_Phy88E1340() failed.\n");
        return rc;
    }

    return MV_OK;
}

MV_STATUS setExtLoopback_Phy88E1340(MV_U32 devId, MV_U32 smiId, MV_U32 portId)
{
    MV_STATUS rc;

    rc = readModWriteReg_Phy88E1340(devId, smiId, portId, 16 /* reg */, 6 /* page */,
                                    BIT_5 /* mask */, BIT_5 /* value */);
    if (rc != MV_OK)
    {
        mvOsPrintf("setLoopback_Phy88E1340() - 1st readModWriteReg_Phy88E1340() failed.\n");
        return rc;
    }

    return MV_OK;
}

MV_STATUS setLoopback_Phy88E1340(MV_U32 devId, MV_U32 smiId, MV_U32 portId)
{
    MV_STATUS rc;

    rc = readModWriteReg_Phy88E1340(devId, smiId, portId, 0 /* reg */, 0 /* page */,
                                    BIT_14 /* mask */, BIT_14 /* value */);
    if (rc != MV_OK)
    {
        mvOsPrintf("setLoopback_Phy88E1340() - 1st readModWriteReg_Phy88E1340() failed.\n");
        return rc;
    }

    rc = readModWriteReg_Phy88E1340(devId, smiId, portId, 0 /* reg */, 1 /* page */,
                                    BIT_14 /* mask */, BIT_14 /* value */);
    if (rc != MV_OK)
    {
        mvOsPrintf("setLoopback_Phy88E1340() - 2nd readModWriteReg_Phy88E1340() failed.\n");
        return rc;
    }

    rc = readModWriteReg_Phy88E1340(devId, smiId, portId, 0 /* reg */, 4 /* page */,
                                    BIT_14 /* mask */, BIT_14 /* value */);
    if (rc != MV_OK)
    {
        mvOsPrintf("setLoopback_Phy88E1340() - 3rd readModWriteReg_Phy88E1340() failed.\n");
        return rc;
    }

    return MV_OK;
}

MV_STATUS unsetLoopback_Phy88E1340(MV_U32 devId, MV_U32 smiId, MV_U32 portId)
{
    MV_STATUS rc;

    rc = readModWriteReg_Phy88E1340(devId, smiId, portId, 0 /* reg */, 0 /* page */,
                                    BIT_14 /* mask */, 0 /* value */);
    if (rc != MV_OK)
    {
        mvOsPrintf("setLoopback_Phy88E1340() - 1st readModWriteReg_Phy88E1340() failed.\n");
        return rc;
    }

    rc = readModWriteReg_Phy88E1340(devId, smiId, portId, 0 /* reg */, 1 /* page */,
                                    BIT_14 /* mask */, 0 /* value */);
    if (rc != MV_OK)
    {
        mvOsPrintf("setLoopback_Phy88E1340() - 2nd readModWriteReg_Phy88E1340() failed.\n");
        return rc;
    }

    rc = readModWriteReg_Phy88E1340(devId, smiId, portId, 0 /* reg */, 4 /* page */,
                                    BIT_14 /* mask */, 0 /* value */);
    if (rc != MV_OK)
    {
        mvOsPrintf("setLoopback_Phy88E1340() - 3rd readModWriteReg_Phy88E1340() failed.\n");
        return rc;
    }

    return MV_OK;
}

/*******************************************************************************
 * PHY 88E1340 MANAGEMENT FUNCTIONS --- END
 *
 *******************************************************************************/

MV_U32 testTimer_mvCntmrRead()
{
    MV_U32 t1, t2;

    t1 = mvCntmrRead(SYS_TIMER_NUM);
    mvOsUDelay(0x1000);
    t2 = mvCntmrRead(SYS_TIMER_NUM);

    mvOsPrintf("t1 = 0x%08X.\n", t1);
    mvOsPrintf("t2 = 0x%08X.\n", t2);

    return MV_OK;
}

#define PRESTERA_PORT_AN_CFG_REG(portNum)          (0x0A80000C + portNum * 0x400)
#define PRESTERA_PORT_MAC_CTRL_REG0(portNum)       (0x0A800000 + portNum * 0x400)
#define PRESTERA_PORT_MAC_CTRL_REG1(portNum)       (0x0A800004 + portNum * 0x400)
#define PRESTERA_PORT_MAC_CTRL_REG2(portNum)       (0x0A800008 + portNum * 0x400)
#define PRESTERA_PORT_MAC_CTRL_REG3(portNum)       (0x0A800048 + portNum * 0x400)

MV_STATUS mvSwitchSetPortMacLoopback(MV_U32 devId, MV_U32 portId)
{
    CHECK_STATUS(mvSwitchWriteReg(devId, PRESTERA_PORT_AN_CFG_REG(portId), 0x906A));
    CHECK_STATUS(mvSwitchWriteReg(devId, PRESTERA_PORT_MAC_CTRL_REG0(portId), 0x8BE5));
    CHECK_STATUS(mvSwitchWriteReg(devId, PRESTERA_PORT_MAC_CTRL_REG1(portId), 0x0023));
    CHECK_STATUS(mvSwitchWriteReg(devId, PRESTERA_PORT_MAC_CTRL_REG2(portId), 0xC008));
    CHECK_STATUS(mvSwitchWriteReg(devId, PRESTERA_PORT_MAC_CTRL_REG3(portId), 0x0300));

    return MV_OK;
}

MV_U8 *G_SwitchLoopbackTest_txPktP = NULL;
#define MEASURE_LOOPBACK_TIME_TEST_PACKET_LEN 60
MV_U8 measure_loopback_time_test_packet[MEASURE_LOOPBACK_TIME_TEST_PACKET_LEN] = {
                  0x00, 0x00, 0x00, 0x00,             /* place for word 0*/
                  0x00, 0x00, 0x00, 0x00,             /* place for word 1*/
                  0x00, 0x45, 0x78, 0x14, 0x59, 0x99, /* dst mac         */
                  0x00, 0x45, 0x78, 0x14, 0x59, 0x00, /* src mac         */
                  0x08, 0x00,                         /*                 */
                  0x00, 0x01,                         /* arp: eth type   */
                  0x08, 0x00,                         /* arp: IP proto   */
                  0x06, 0x04, 0x00, 0x01,             /* arp: ...        */
                  0x00, 0x45, 0x78, 0x14, 0x59, 0x00, /* arp: sender mac */
                  0x0A, 0X04, 0X32, 0X2E,             /* arp: sender IP  */
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* arp: target mac */
                  0x0A, 0X04, 0X32, 0X78,             /* arp: target IP  */ };

MV_U8 *measure_loopback_time_test_packet_ptr = measure_loopback_time_test_packet + EXTEND_DSA_TAG_SIZE;

MV_BOOL measure_loopback_time_test_init()
{
    MV_U8 macAddr[18];
    MV_U32 devId;

    if (alreadyInit)
    {
        mvOsPrintf("switchTestInit aleady called");
        return MV_FALSE;
    }

    alreadyInit = MV_TRUE;

    for (devId = 0; devId < mvSwitchGetDevicesNum(); devId++)
    {
        mvSwitchOpenMemWinAsKirkwood(devId);
        mvSwitchInit_test(devId);
    }

    mvPpCascadePortCfg();
    mvSwitchVidxCfg();
    mvSwitchEgressFilterCfg();

    /* all ports are members of VIDX 1 except [dev 0, port 26] */
    if (mvSwitchWriteReg(PRESTERA_DEFAULT_DEV, 0xA100010, 0x1FFFFFFF) != MV_OK)
    {
        mvOsPrintf("%s: mvSwitchWriteReg failed.\n", __func__);
        return MV_FALSE;
    }

    if (mvSwitchBitReset(PRESTERA_DEFAULT_DEV, 0xA100010,
                                            BIT_27 /* port 26 */) != MV_OK)
    {
        mvOsPrintf("%s: mvSwitchBitReset failed.\n", __func__);
        return MV_FALSE;
    }

    devId = PRESTERA_DEFAULT_DEV;

    /*
     * Set default MAC address
     */
    memcpy(macAddr, switchTestUserDefMacAddr, MV_MAC_ADDR_SIZE);

    if (setCPUAddressInMACTAble(devId, macAddr, 1 /* vid */) != MV_OK)
    {
        mvOsPrintf("Error: (Prestera) Unable to teach CPU MAC address.\n");
        return MV_FALSE;
    }

    if (setLoopback_Phy88E1340(devId, 0, /* smiId */ 0 /* portId */) != MV_OK)
    {
        mvOsPrintf("measure_loopback_time_test_init() - setLoopback_Phy88E1340() failed.\n");
        return MV_FAIL;
    }

    mvOsPrintf("Now call to measure_loopback_time().\n");
    return MV_FALSE;
}

extern MV_U8 switchPortNum;

int measure_loopback_time_take2(MV_U32 switchPort)
{
    MV_PKT_DESC     packetDesc;
    MV_STATUS       status;
    MV_ULONG        physAddr;
    MV_U8          *txPktP;
    /* STRUCT_SW_RX_DESC *swRxDescP; */
    /* MV_U32          word1; */
    /* MV_U32          loopCnt, loopMaxCnt = 10000; */

    rxDescList = &(G_sdmaInterCtrl[0].rxDescList[0]);

    txPktP = mvOsIoUncachedMalloc(NULL, MEASURE_LOOPBACK_TIME_TEST_PACKET_LEN,
                                  &physAddr, NULL);
    if (!txPktP)
    {
        mvOsPrintf("%s: alloc failed.\n", __func__);
        return 1;
    }
    memcpy(txPktP, measure_loopback_time_test_packet_ptr,
           MEASURE_LOOPBACK_TIME_TEST_PACKET_LEN);

    memset(&packetDesc, 0, sizeof(MV_PKT_DESC));
    packetDesc.pcktData[0]    = (MV_U8 *)txPktP;
    packetDesc.pcktPhyData[0] = (MV_U8 *)txPktP;

    /*
     * coverity shouts here

    if (pcktSize >= MIN_PCKT_SIZE - CRC_SIZE)
        pcktSize += CRC_SIZE;
    else
        pcktSize =  MIN_PCKT_SIZE;
    */

    status = mvSwitchBuildPacket(PRESTERA_DEFAULT_DEV,
                                 switchPort,             /* port num  */
                                 0,                      /* entryId   */
                                 1,                      /* appendCrc */
                                 1,                      /* pcktsNum  */
                                 0,                      /* gap       */
                                 txPktP,       /* with place for DSA tag */
                                 MEASURE_LOOPBACK_TIME_TEST_PACKET_LEN,
                                &packetDesc);

    G_SwitchLoopbackTest_txPktP = packetDesc.pcktData[0];
    mvSwitchLoopbackTest(PRESTERA_DEFAULT_DEV, 0, 0);

    return OK;
}

/*
 * Assumes that Prestera BSP switch END driver performs the init.
 * After the test the system continues to functions.
 */
MV_STATUS measure_loopback_time_take4(MV_U32 devId, MV_U32 portId, MV_U32 numOfPkts)
{
    /* MV_U8 dev; */
    MV_U32 tmp;

    mvSwitchSetPortMacLoopback(devId, portId);

    /*
     * configure Private VLAN Edge (PVE)
     */

    /* enable PVE globally */
    CHECK_STATUS(mvSwitchReadReg (devId, 0x2040000, &tmp));
    tmp |= BIT_30;
    CHECK_STATUS(mvSwitchWriteReg(devId, 0x2040000, tmp));

    /* enable PVE for specific port */
    CHECK_STATUS(mvSwitchReadReg (devId, 0x2000000 + portId * 0x1000, &tmp));
    tmp |= BIT_23;
    CHECK_STATUS(mvSwitchWriteReg(devId, 0x2000000 + portId * 0x1000, tmp));

    /* enable PVE for CPU port */
    CHECK_STATUS(mvSwitchReadReg (devId, 0x203F000, &tmp));
    tmp |= BIT_23;
    CHECK_STATUS(mvSwitchWriteReg(devId, 0x203F000, tmp));

    /*
     * configure target PVE port
     */

    /* configure taget PVE port for specific port to CPU port */
    CHECK_STATUS(mvSwitchReadReg (devId, 0x2000000 + portId * 0x1000, &tmp));
    tmp |= (63 /* CPU port num */ << 25);
    CHECK_STATUS(mvSwitchWriteReg(devId, 0x2000000 + portId * 0x1000, tmp));

    /* configure target PVE port for CPU port to specific port */
    CHECK_STATUS(mvSwitchReadReg (devId, 0x203F000, &tmp));
    tmp |= (portId << 25);
    CHECK_STATUS(mvSwitchWriteReg(devId, 0x203F000, tmp));

    measure_loopback_time_take2(portId);
    CHECK_STATUS(mvSwitchSetPortMacLoopback(devId, portId));

    return MV_OK;
}

int measure_loopback_time(MV_U32 switchPort)
{
    STRUCT_SW_TX_DESC *currSwTxDescP;
    STRUCT_SW_RX_DESC *swRxDescP;
    /* STRUCT_RX_DESC    *rxDescP; */
    /* STRUCT_TX_DESC    *txDescP; */
    STRUCT_TX_DESC     txDesc;
    MV_U32             /*rxDsaWord0, rxDsaWord1, */txDsaWord0, txDsaWord1;
    MV_U32             word1/*, bufPhysAddr, i*/;
    /* MV_U8              tmp; */
    MV_U32             txTs;
    MV_U32             devNum = PRESTERA_DEFAULT_DEV;
    MV_U32 bufPhysAddr = (MV_U32)measure_loopback_time_test_packet;

#if defined(DSA_TAG) && defined(BI_DIRECTIONAL)
    MV_U32 dsaWord0, dsaWord1;
#endif

    mvOsMemset(&txDesc, 0, sizeof(STRUCT_TX_DESC));
    TX_DESC_SET_RECALC_CRC_BIT(&txDesc, 1);
    TX_DESC_SET_LAST_BIT      (&txDesc, 1);
    TX_DESC_SET_INT_BIT       (&txDesc, 1);
    TX_DESC_SET_FIRST_BIT     (&txDesc, 1);
    TX_DESC_SET_OWN_BIT       (&txDesc, MV_OWNERSHIP_DMA);

    txDsaWord0 = calcFromCpuDsaTagWord0(devNum, switchPort);
    txDsaWord1 = calcFromCpuDsaTagWord1(switchPort);

    /*
    txDsaWord0 = mvSwitchFromCpuMultiTargetDsaTagWord0();
    txDsaWord1 = mvSwitchFromCpuMultiTargetDsaTagWord1();

    rxDsaWord0 = mvSwitchFromCpuMultiTargetDsaTagWord0();
    rxDsaWord1 = mvSwitchFromCpuMultiTargetDsaTagWord1();
    */

    rxDescList = &(G_sdmaInterCtrl[0].rxDescList[0]);
    txDescList = &(G_sdmaInterCtrl[0].txDescList[0]);

    /***************************************************************************
     * tx the packet
     */
    currSwTxDescP = txDescList->next2Feed;

    /* neta ETH_PACKET_CACHE_FLUSH(bufPhysAddr,bufSize); */

    ((MV_U32*)bufPhysAddr)[3]  = txDsaWord0;
    ((MV_U32*)bufPhysAddr)[4]  = txDsaWord1;
    currSwTxDescP->txDesc->buffPointer = hwByteSwap((MV_U32)measure_loopback_time_test_packet);

    txDescList->freeDescNum--;
    txDescList->next2Feed = currSwTxDescP->swNextDesc;

    currSwTxDescP->txDesc->word1 = txDesc.word1;
    TX_DESC_SET_BYTE_CNT(currSwTxDescP->txDesc, MEASURE_LOOPBACK_TIME_TEST_PACKET_LEN);

    /* take tx timestamp */
    txTs = mvCntmrRead(SYS_TIMER_NUM);

    *((volatile MV_U32 *)(0xf4000000+ 0x2868)) = (MV_U32)MV_32BIT_LE_FAST(0x1);

    /***************************************************************************
     * wait for the packet to be back
     */
    while (1)
    {
        swRxDescP = rxDescList->next2Receive;
        word1 = hwByteSwap(swRxDescP->rxDesc->word1);

        if (word1 & 0x80000000)
        {
            continue;
        }
    }

#ifndef _DIAB_TOOL
    return 0; /* make the compiler happy */
#endif
}

MV_U32 mvSwitchLoopbackTest(MV_U32 devNum, MV_U32 rxPort, MV_U32 txPort)
{
    STRUCT_SW_TX_DESC *currSwTxDescP;
    STRUCT_SW_RX_DESC *swRxDescP;
    STRUCT_RX_DESC    *rxDescP;
    /* STRUCT_TX_DESC    *txDescP; */
    STRUCT_TX_DESC     txDesc;
    MV_U32             rxDsaWord0, rxDsaWord1, txDsaWord0, txDsaWord1;
    MV_U32             word1, bufPhysAddr, txPkt;
#ifdef UPDATE_MAC
    MV_U8              tmp;
#endif

#if defined(DSA_TAG) && defined(BI_DIRECTIONAL)
    MV_U32 dsaWord0, dsaWord1;
#endif

    mvOsMemset(&txDesc, 0, sizeof(STRUCT_TX_DESC));
    TX_DESC_SET_RECALC_CRC_BIT(&txDesc, 1);
    TX_DESC_SET_LAST_BIT      (&txDesc, 1);
    TX_DESC_SET_INT_BIT       (&txDesc, 1);
    TX_DESC_SET_FIRST_BIT     (&txDesc, 1);
    TX_DESC_SET_OWN_BIT       (&txDesc, MV_OWNERSHIP_DMA);

    txDsaWord0 = calcFromCpuDsaTagWord0(devNum, txPort);
    txDsaWord1 = calcFromCpuDsaTagWord1(txPort);

    rxDsaWord0 = calcFromCpuDsaTagWord0(devNum, rxPort);
    rxDsaWord1 = calcFromCpuDsaTagWord1(rxPort);

    /*
    txDsaWord0 = mvSwitchFromCpuMultiTargetDsaTagWord0();
    txDsaWord1 = mvSwitchFromCpuMultiTargetDsaTagWord1();

    rxDsaWord0 = mvSwitchFromCpuMultiTargetDsaTagWord0();
    rxDsaWord1 = mvSwitchFromCpuMultiTargetDsaTagWord1();
    */

    rxDescList = &(G_sdmaInterCtrl[0].rxDescList[0]);
    txDescList = &(G_sdmaInterCtrl[0].txDescList[0]);

    /*
     * send packet for loopback test
     */
    currSwTxDescP = txDescList->next2Feed;

    /* neta ETH_PACKET_CACHE_FLUSH(bufPhysAddr,bufSize); */

    txPkt = (MV_U32)G_SwitchLoopbackTest_txPktP;
    currSwTxDescP->txDesc->buffPointer = hwByteSwap(txPkt);

    txDescList->freeDescNum--;
    txDescList->next2Feed = currSwTxDescP->swNextDesc;

    currSwTxDescP->txDesc->word1 = txDesc.word1;
    TX_DESC_SET_BYTE_CNT(currSwTxDescP->txDesc, MEASURE_LOOPBACK_TIME_TEST_PACKET_LEN);
                         /* RX_DESC_GET_BYTE_COUNT_FIELD(swRxDescP->rxDesc)); */

    /*
     * wait for the packet to get back
     */
    swRxDescP = rxDescList->next2Receive;
    rxDescP   = swRxDescP->rxDesc;

    /* send the packet yahoooo */
    *((volatile MV_U32 *)(0xf4000000+ 0x2868)) = (MV_U32)MV_32BIT_LE_FAST(0x1);

    do {
        word1 = hwByteSwap(rxDescP->word1);
    } while (word1 & 0x80000000);

    /*
     * take timestamp for the loopbacked packet
     */

    mvOsPrintf("%s: the packet is BACK !!!\n", __func__);

    while (1)
    {
        if (txDescList->freeDescNum != PRESTERA_TXQ_MAX_FREE_DESC)
        {
            if (IS_TX_DESC_DMA_OWNED(txDescList->next2Free->txDesc))
            {
                /* ETH_DESCR_FLUSH_INV(NULL, txDescList->next2Free->txDesc); */
            }
            else
            {
                rxDescList->rxFreeDescNum++;
                rxDescP = rxDescList->next2Return->rxDesc;
                rxDescP->buffPointer = hwByteSwap((MV_U32)txDescList->next2Free->txDesc->buffPointer);
                RX_DESC_RESET(rxDescP);
                rxDescList->next2Return = rxDescList->next2Return->swNextDesc;
                txDescList->freeDescNum++;
                txDescList->next2Free = txDescList->next2Free->swNextDesc;
            }
        }

        if (rxDescList->rxFreeDescNum == 0)
        {
            TX_DESC_SET_OWN_BIT(txDescList->next2Free->txDesc, MV_OWNERSHIP_CPU);
            continue;
        }

        swRxDescP = rxDescList->next2Receive;
        word1 = hwByteSwap(swRxDescP->rxDesc->word1);

        if (word1 & 0x80000000)
        {
            continue;
        }

        rxDescList->next2Receive = swRxDescP->swNextDesc;
        rxDescList->rxFreeDescNum--;
        bufPhysAddr = hwByteSwap(swRxDescP->rxDesc->buffPointer);

        /*
         * good rx - send the packet back to the switch
         */

#ifdef UPDATE_MAC
        tmp = ((MV_U8 *)bufPhysAddr)[5];
        ((MV_U8 *)bufPhysAddr)[5]  = ((MV_U8 *)bufPhysAddr)[11];
        ((MV_U8 *)bufPhysAddr)[11] = tmp;
#endif

#ifdef DSA_TAG
#  ifndef BI_DIRECTIONAL
        /*
         * DSA & uni-direction -
         * update to FROM CPU packet with target port = txPort
         */
        ((MV_U32*)bufPhysAddr)[3]  = txDsaWord0;
        ((MV_U32*)bufPhysAddr)[4]  = txDsaWord1;
#  else
        /*
         * DSA & bi-direction - update to FROM CPU packet with target port =
         * according to the source port
         */
        dsaWord0 = ((MV_U32*)bufPhysAddr)[3];
        dsaWord1 = ((MV_U32*)bufPhysAddr)[4];
        if (DSA_GET_SRC_PORT(MV_BYTE_SWAP_32BIT(dsaWord0),
                             MV_BYTE_SWAP_32BIT(dsaWord1)) == rxPort)
        {
            ((MV_U32*)bufPhysAddr)[3] = txDsaWord0;
            ((MV_U32*)bufPhysAddr)[4] = txDsaWord1;
        }
        else
        {
            ((MV_U32*)bufPhysAddr)[3] = rxDsaWord0;
            ((MV_U32*)bufPhysAddr)[4] = rxDsaWord1;
        }
#  endif
#endif
        currSwTxDescP = txDescList->next2Feed;

        currSwTxDescP->txDesc->buffPointer = hwByteSwap(bufPhysAddr);

        txDescList->freeDescNum--;
        txDescList->next2Feed = currSwTxDescP->swNextDesc;

        currSwTxDescP->txDesc->word1 = txDesc.word1;
        TX_DESC_SET_BYTE_CNT(currSwTxDescP->txDesc,
                             RX_DESC_GET_BYTE_COUNT_FIELD(swRxDescP->rxDesc));

        *((volatile MV_U32 *)(0xf4000000+ 0x2868)) = (MV_U32)MV_32BIT_LE_FAST(0x1);
    }


#ifndef _DIAB_TOOL
    return 0; /* make the compiler happy */
#endif
}


