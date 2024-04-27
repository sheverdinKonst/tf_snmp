/******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* sregxCat.h
*
* DESCRIPTION:
*       Defines for xCat3 memory registers access.
*
* DEPENDENCIES:
*       None.
*
* FILE REVISION NUMBER:
*       $Revision: 3 $
*
*******************************************************************************/
#ifndef __sregXCat3h
#define __sregXCat3h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* egress and txq - IEEE 802.1BR – global config  */
#define SMEM_XCAT3_EGRESS_AND_TXQ_802_1_BR_GLOBAL_CONFIG_REG(dev)                      \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.br8021Config.globalConfig : \
    SMAIN_NOT_VALID_CNS)

/* egress and txq - IEEE 802.1BR – PCID of Trg Port p  */
#define SMEM_XCAT3_EGRESS_AND_TXQ_PCID_TARGET_PORT_P_REG(dev, port)                      \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.br8021Config.pcidOfTrgPort[(port == SNET_CHT_CPU_PORT_CNS) ? 31 : port]: \
    SMAIN_NOT_VALID_CNS)

/* egress and txq - IEEE 802.1BR – DP2CFI1 Enable Register */
#define SMEM_XCAT3_EGRESS_AND_TXQ_DP2CFI1_ENABLE_REG(dev)\
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->egrAndTxqVer0.br8021Config.dp2cfiEnablePerPortReg : \
    SMAIN_NOT_VALID_CNS)

/* egress and txq - IEEE 802.1BR – Ingress E-CID table  */
#define SMEM_XCAT3_EGRESS_AND_TXQ_ECID_TBL_MEM(dev, entry_index) \
    SMEM_TABLE_ENTRY_INDEX_GET_MAC(dev,egressAndTxqIngressEcid,entry_index)

/* TTI - IEEE 802.1BR – Src Port BPE Enable  */
#define SMEM_XCAT3_TTI_SOURCE_PORT_BPE_ENABLE_REG(dev)                      \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->TTI.SrcPortBpeEnable : \
    SMAIN_NOT_VALID_CNS)

/* HA - IEEE 802.1BR – Per Port BPE Enable  */
#define SMEM_XCAT3_HA_PER_PORT_BPE_ENABLE_REG(dev)                      \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.PerPortBpeEnable : \
    SMAIN_NOT_VALID_CNS)

/* HA - IEEE 802.1BR – BPE Internal Configuration  */
#define SMEM_XCAT3_HA_BPE_INTERNAL_CONFIG_REG(dev)                      \
    ((dev)->supportRegistersDb ? (SMEM_CHT_MAC_REG_DB_GET(dev))->HA.BpeInternalConfig : \
    SMAIN_NOT_VALID_CNS)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __sregXCat3h */

