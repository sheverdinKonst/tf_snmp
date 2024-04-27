/*******************************************************************************
*              (c), Copyright 2001, Marvell International Ltd.                 *
* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL SEMICONDUCTOR, INC.   *
* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT  *
* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE        *
* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL.     *
* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,       *
* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE.   *
********************************************************************************
* prvCpssHwRegisters.c
*
* DESCRIPTION:
*       Private API definition for register access of the DXCH devices.
*
*       Those APIs gives next benefit :
*       for Lion (multi-port group device) with TXQ ver 1 (or above)
*       - write of 'global config' into the TXQ may need to we written only in
*       some of the port groups (not all)
*       - write of 'per port' may need to we written to other port group then
*       specified according to 'dest port'
*
*       still SOME functionality MUST NOT use those APIs , like:
*       - interrupts handling --> needed per specific port group
*       - prvCpssPortGroupsCounterSummary(...) , prvCpssPortGroupBusyWait(...)
*          and maybe others...
*       - maybe others
*
*
* FILE REVISION NUMBER:
*       $Revision: 33 $
*
*******************************************************************************/

#include <cpss/generic/config/private/prvCpssConfigTypes.h>
#include <cpssDriver/pp/prvCpssDrvPpDefs.h>

/* BMP for Lion , for TXQ SHT sub unit , that need to be duplicated to port
   groups 0,1*/
#define LION_TXQ_SHT_UNIT_PORT_GROUPS_BMP_CNS   0x3

/*
   BMP for Lion , for TXQ EGR sub unit , must be duplicated to all port groups
   because serve traffic according to INGRESS port
*/
#define LION_TXQ_EGR_UNIT_PORT_GROUPS_BMP_CNS   CPSS_PORT_GROUP_UNAWARE_MODE_CNS

/*
   BMP for Lion , for BMA sub unit , single port group 0
*/
#define LION_BMA_UNIT_PORT_GROUPS_BMP_CNS     0x1

/*
   BMP for Lion , for CTU sub unit , single port group 0
*/
#define LION_CTU_UNIT_PORT_GROUPS_BMP_CNS     0x1

/*
   BMP for Lion , for TXQ queue sub unit , single port group 0
*/
#define LION_TXQ_QUEUE_UNIT_PORT_GROUPS_BMP_CNS     0x1

/*
   BMP for Lion , for TXQ LL sub unit , single port group 0
*/
#define LION_TXQ_LL_UNIT_PORT_GROUPS_BMP_CNS     0x1

/* open this for start of debug Puma3*/
/*#define SPECIAL_PUMA3_DEBUG*/

#if ((defined ASIC_SIMULATION) && (defined EXMXPM_FAMILY) && (defined SPECIAL_PUMA3_DEBUG))
    #define ALLOW_EMPTY_PORT_GROUPS
#endif


#ifdef ALLOW_EMPTY_PORT_GROUPS
        /* the read/write register of 'non-valid' will try to loop on 0 port groups -->
            will not fail the function , but do nothing
            --> for puma3 initialization DEBUG ONLY */
    #define RETURN_VALUE_OF_MULTI_PORT_GROUPS_FAIL_CNS          GT_OK
#else  /*!ALLOW_EMPTY_PORT_GROUPS*/
    #define RETURN_VALUE_OF_MULTI_PORT_GROUPS_FAIL_CNS          GT_FAIL
#endif /*!ALLOW_EMPTY_PORT_GROUPS*/

/*******************************************************************************
* prvCpssLionDuplicatedMultiPortGroupsGet
*
* DESCRIPTION:
*       Check if register address is duplicated in multi-port groups device
*       for Lion B device
*
* APPLICABLE DEVICES:
*        Lion.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2.
*
* INPUTS:
*       regAddr - register address to access
*
* OUTPUTS:
*       portGroupsBmpPtr  - (pointer to) the updated port groups
*                       relevant only when function returns GT_TRUE
*
* RETURNS:
*       GT_TRUE     - the caller need to use the updated 'port groups bmp'
*       GT_FALSE    - the caller NOT need to use the updated 'port groups bmp'
*
* COMMENTS:
*
*
*******************************************************************************/
GT_BOOL prvCpssLionDuplicatedMultiPortGroupsGet
(
    IN GT_U32                   regAddr,
    OUT  GT_PORT_GROUPS_BMP     *portGroupsBmpPtr
)
{
    switch(U32_GET_FIELD_MAC(regAddr , 23 , 6))
    {
        /* sub-unit index */
        case 0x03:      /* address 0x01800000 */   /* TXQ EGR */
            *portGroupsBmpPtr = LION_TXQ_EGR_UNIT_PORT_GROUPS_BMP_CNS;
            break;
        case 0x07:      /* address 0x03800000 */    /* BMA */
            *portGroupsBmpPtr = LION_BMA_UNIT_PORT_GROUPS_BMP_CNS;
            break;
        case 0x0E:      /* address 0x07000000 */   /* CTU */
            *portGroupsBmpPtr = LION_CTU_UNIT_PORT_GROUPS_BMP_CNS;
            break;
        case 0x14:      /* address 0x0a000000 */   /* TXQ Queue */
            *portGroupsBmpPtr = LION_TXQ_QUEUE_UNIT_PORT_GROUPS_BMP_CNS;
            break;
        case 0x21:      /* address 0x10800000 */   /* TXQ LL */
            *portGroupsBmpPtr = LION_TXQ_LL_UNIT_PORT_GROUPS_BMP_CNS;
            break;
        case 0x23:      /* address 0x11800000 */   /* TXQ SHT */
            *portGroupsBmpPtr = LION_TXQ_SHT_UNIT_PORT_GROUPS_BMP_CNS;
            break;
       default:
           return GT_FALSE;
    }

    return GT_TRUE;
}

/* BMPs for Lion2 , for TXQ SHT sub unit per hemisphere */
#define LION2_TXQ_SHT_UNIT_HEM0_PORT_GROUPS_BMP_CNS   0xf
#define LION2_TXQ_SHT_UNIT_HEM1_PORT_GROUPS_BMP_CNS   0xf0

/* BMPs for Lion2 , for TXQ EGR0 sub unit per hemisphere */
#define LION2_TXQ_EGR_UNIT_HEM0_PORT_GROUPS_BMP_CNS   0xf
#define LION2_TXQ_EGR_UNIT_HEM1_PORT_GROUPS_BMP_CNS   0xf0

/* BMPs for Lion2 , for TXQ queue sub unit per hemisphere */
#define LION2_TXQ_QUEUE_UNIT_HEM0_PORT_GROUPS_BMP_CNS     0x1
#define LION2_TXQ_QUEUE_UNIT_HEM1_PORT_GROUPS_BMP_CNS     0x10

/* BMPs for Lion2 , for TXQ LL sub unit per hemisphere */
#define LION2_TXQ_LL_UNIT_HEM0_PORT_GROUPS_BMP_CNS     0x1
#define LION2_TXQ_LL_UNIT_HEM1_PORT_GROUPS_BMP_CNS     0x10

/* BMP for Lion2 , for TXQ DIST sub unit , single port group 0 */
#define LION2_TXQ_DIST_UNIT_PORT_GROUPS_BMP_CNS     0x1

/* BMP for Lion2 , for BMA sub unit , single port group 0 */
#define LION2_BMA_UNIT_PORT_GROUPS_BMP_CNS     0x1

/* BMP for Lion2 , for DFX Server sub unit , single port group 1 */
#define LION2_DFX_SERVER_UNIT_PORT_GROUPS_BMP_CNS     0x2

/* BMP for Lion2 , for CPFC sub unit , single port group 0 */
#define LION2_CPFC_UNIT_PORT_GROUPS_BMP_CNS     0x1

/* BMPs for Lion2 , for CTU sub unit per hemisphere */
#define LION2_CTU_UNIT_HEM0_PORT_GROUPS_BMP_CNS     0x1
#define LION2_CTU_UNIT_HEM1_PORT_GROUPS_BMP_CNS     0x10

#define PRV_CPSS_DXCH_PORT_GROUPS_NUM_IN_HEMISPHERE_CNS 4

#define  LION2_DFX_SERVER_INDEX   0x03/*0x01800000*/
#define  LION2_BMA_INDEX          0x07/*0x03800000*/
#define  LION2_TXQ_QUEUE_INDEX    0x20/*0x10000000*/
#define  LION2_TXQ_LL_INDEX       0x21/*0x10800000*/
#define  LION2_TXQ_SHT_INDEX      0x23/*0x11800000*/
#define  LION2_TXQ_EGR0_INDEX     0x24/*0x12000000*/
#define  LION2_TXQ_EGR1_INDEX     0x25/*0x12800000*/
#define  LION2_TXQ_DIST_INDEX     0x26/*0x13000000*/
#define  LION2_CPFC_INDEX         0x27/*0x13800000*/
#define  LION2_CTU0_INDEX         0x0E/*0x07000000*/
#define  LION2_CTU1_INDEX         0x15/*0x0A800000*/

/* BMP for Lion3 , for FDB sub unit , single port group 0 */
#define LION3_FDB_UNIT_PORT_GROUPS_BMP_CNS     0x1
/*BMP of every 2 port groups share memory (0,2,4,6,8,10,..30)*/
#define SHARED_EVERY_2_PORT_GROUPS_CNS    0x55555555
/*BMP of every 4 port groups share memory (0,4,8,12,..28)*/
#define SHARED_EVERY_4_PORT_GROUPS_CNS    0x11111111

#define LION3_SHIFT_UNIT_CNS  23

#define LION3_FDB_INDEX_CNS           (PRV_CPSS_DXCH_LION3_BASE_ADDRESS_FDB_CNS >> LION3_SHIFT_UNIT_CNS)/*0x06000000*/
#define LION3_MG_INDEX_CNS            (0x00000000 >> LION3_SHIFT_UNIT_CNS)/*0x00000000*/
#define LION3_EGF_EFT_INDEX_CNS        (PRV_CPSS_DXCH_LION3_BASE_ADDRESS_EGF_EFT_CNS >> LION3_SHIFT_UNIT_CNS)
#define LION3_EGF_QAG_INDEX_MAC(id)    ((PRV_CPSS_DXCH_LION3_BASE_ADDRESS_EGF_QAG_CNS >> LION3_SHIFT_UNIT_CNS) + (id))
#define LION3_EGF_SHT_INDEX_MAC(id)    ((PRV_CPSS_DXCH_LION3_BASE_ADDRESS_EGF_SHT_CNS >> LION3_SHIFT_UNIT_CNS) + (id))
#define LION3_TTI_INDEX_CNS            (PRV_CPSS_DXCH_LION3_BASE_ADDRESS_TTI_CNS >> LION3_SHIFT_UNIT_CNS)
#define LION3_L2I_INDEX_CNS            (PRV_CPSS_DXCH_LION3_BASE_ADDRESS_L2I_CNS >> LION3_SHIFT_UNIT_CNS)
#define LION3_EQ_INDEX_MAC(id)         ((PRV_CPSS_DXCH_LION3_BASE_ADDRESS_EQ_CNS >> LION3_SHIFT_UNIT_CNS) + (id))
#define LION3_HA_INDEX_CNS             ((PRV_CPSS_DXCH_LION3_BASE_ADDRESS_HA_CNS >> LION3_SHIFT_UNIT_CNS))
#define LION3_MLL_INDEX_CNS            ((PRV_CPSS_DXCH_LION3_BASE_ADDRESS_MLL_CNS >> LION3_SHIFT_UNIT_CNS))
#define LION3_PCL_INDEX_CNS            (PRV_CPSS_DXCH_LION3_BASE_ADDRESS_PCL_CNS >> LION3_SHIFT_UNIT_CNS)
#define LION3_TCAM_INDEX_CNS            (PRV_CPSS_DXCH_LION3_BASE_ADDRESS_TCAM_CNS >> LION3_SHIFT_UNIT_CNS)
#define LION3_LPM_INDEX_CNS(id)        ((PRV_CPSS_DXCH_LION3_BASE_ADDRESS_LPM_CNS >> LION3_SHIFT_UNIT_CNS) + (id))
#define LION3_EPCL_INDEX_CNS           (PRV_CPSS_DXCH_LION3_BASE_ADDRESS_EPCL_CNS >> LION3_SHIFT_UNIT_CNS)
#define LION3_IPVX_INDEX_CNS           (PRV_CPSS_DXCH_LION3_BASE_ADDRESS_IPVX_CNS >> LION3_SHIFT_UNIT_CNS)
#define LION3_IPLR_INDEX_CNS           (PRV_CPSS_DXCH_LION3_BASE_ADDRESS_IPLR_CNS >> LION3_SHIFT_UNIT_CNS)
#define LION3_IPLR_1_INDEX_CNS           (PRV_CPSS_DXCH_LION3_BASE_ADDRESS_IPLR_1_CNS >> LION3_SHIFT_UNIT_CNS)
#define LION3_EPLR_INDEX_CNS           (PRV_CPSS_DXCH_LION3_BASE_ADDRESS_EPLR_CNS >> LION3_SHIFT_UNIT_CNS)
#define LION3_CNC_0_INDEX_CNS          (PRV_CPSS_DXCH_LION3_BASE_ADDRESS_CNC_0_CNS >> LION3_SHIFT_UNIT_CNS)
#define LION3_CNC_1_INDEX_CNS          (PRV_CPSS_DXCH_LION3_BASE_ADDRESS_CNC_1_CNS >> LION3_SHIFT_UNIT_CNS)

#define LION3_TXQ_QUEUE_INDEX_CNS      (PRV_CPSS_DXCH_LION3_BASE_ADDRESS_TXQ_QUEUE_CNS >> LION3_SHIFT_UNIT_CNS)
#define LION3_TXQ_LL_INDEX_CNS         (PRV_CPSS_DXCH_LION3_BASE_ADDRESS_TXQ_LL_CNS    >> LION3_SHIFT_UNIT_CNS)
#define LION3_TXQ_PFC_INDEX_CNS        (PRV_CPSS_DXCH_LION3_BASE_ADDRESS_TXQ_PFC_CNS   >> LION3_SHIFT_UNIT_CNS)
#define LION3_TXQ_QCN_INDEX_CNS        (PRV_CPSS_DXCH_LION3_BASE_ADDRESS_TXQ_QCN_CNS   >> LION3_SHIFT_UNIT_CNS)
#define LION3_TXQ_DQ_INDEX_CNS         (PRV_CPSS_DXCH_LION3_BASE_ADDRESS_TXQ_DQ_CNS    >> LION3_SHIFT_UNIT_CNS)


/*******************************************************************************
* prvCpssLion2DuplicatedMultiPortGroupsGet
*
* DESCRIPTION:
*       Check if register address is duplicated in multi-port groups device
*       for Lion2,3 device.
*
* APPLICABLE DEVICES:
*        Lion2; Lion3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Bobcat2.
*
* INPUTS:
*       portGroupId - The port group id. relevant only to 'multi-port-groups'
*                     devices. Supports value CPSS_PORT_GROUP_UNAWARE_MODE_CNS
*       regAddr     - register address to access.
*
* OUTPUTS:
*       portGroupsBmpPtr  - (pointer to) the updated port groups
*                       relevant only when function returns GT_TRUE
*       isAdditionalRegDupNeededPtr - (pointer to)
*             GT_TRUE - the caller need to duplicate the value to additional
*                       register in all 'port groups bmp'.
*             GT_FALSE - the caller NOT need to duplicate the value to additional
*                       register.
*       additionalRegDupPtr - (pointer to)
*                       register address of additional register duplication.
*                       Only relevant when isAdditionalRegDupNeeded is GT_TRUE.
*
* RETURNS:
*       GT_TRUE     - the caller need to use the updated 'port groups bmp'
*       GT_FALSE    - the caller NOT need to use the updated 'port groups bmp'
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_BOOL prvCpssLion2DuplicatedMultiPortGroupsGet
(
    IN GT_U32                   portGroupId,
    IN GT_U32                   regAddr,
    OUT  GT_PORT_GROUPS_BMP     *portGroupsBmpPtr,
    OUT  GT_BOOL                *isAdditionalRegDupNeededPtr,
    OUT  GT_U32                 *additionalRegDupPtr
)
{
    GT_U32  unitIndex = U32_GET_FIELD_MAC(regAddr , 23 , 6);
    GT_U32  unitIndexAdditional;/* unit index of the additional register*/

    *isAdditionalRegDupNeededPtr = GT_FALSE;
    switch(unitIndex)
    {
        case LION2_TXQ_EGR0_INDEX:/*0x12000000*/    /*PRV_CPSS_DXCH_LION2_TXQ_EGR0_BASE_ADDRESS_CNS*/
        case LION2_TXQ_EGR1_INDEX:/*0x12800000*/    /*PRV_CPSS_DXCH_LION2_TXQ_EGR1_BASE_ADDRESS_CNS*/
            if (portGroupId == CPSS_PORT_GROUP_UNAWARE_MODE_CNS)
            {
                *portGroupsBmpPtr = LION2_TXQ_EGR_UNIT_HEM0_PORT_GROUPS_BMP_CNS |
                    LION2_TXQ_EGR_UNIT_HEM1_PORT_GROUPS_BMP_CNS;
            }
            else if (portGroupId < PRV_CPSS_DXCH_PORT_GROUPS_NUM_IN_HEMISPHERE_CNS)
            {
                *portGroupsBmpPtr = LION2_TXQ_EGR_UNIT_HEM0_PORT_GROUPS_BMP_CNS;
            }
            else
            {
                *portGroupsBmpPtr = LION2_TXQ_EGR_UNIT_HEM1_PORT_GROUPS_BMP_CNS;
            }

            if(unitIndex == LION2_TXQ_EGR0_INDEX)
            {
                /* when access egr0 , need also to access egr1 */
                unitIndexAdditional = LION2_TXQ_EGR1_INDEX;
            }
            else
            {
                /* when access egr1 , need also to access egr0 */
                unitIndexAdditional = LION2_TXQ_EGR0_INDEX;
            }

            *isAdditionalRegDupNeededPtr = GT_TRUE;
            *additionalRegDupPtr = regAddr;
            U32_SET_FIELD_MASKED_MAC((*additionalRegDupPtr),23,6,unitIndexAdditional);
            break;
        case LION2_TXQ_QUEUE_INDEX: /*0x10000000*//*PRV_CPSS_DXCH_LION2_TXQ_QUEUE_BASE_ADDRESS_CNS*/
            if (portGroupId == CPSS_PORT_GROUP_UNAWARE_MODE_CNS)
            {
                *portGroupsBmpPtr = LION2_TXQ_QUEUE_UNIT_HEM0_PORT_GROUPS_BMP_CNS |
                    LION2_TXQ_QUEUE_UNIT_HEM1_PORT_GROUPS_BMP_CNS;
            }
            else if (portGroupId < PRV_CPSS_DXCH_PORT_GROUPS_NUM_IN_HEMISPHERE_CNS)
            {
                *portGroupsBmpPtr = LION2_TXQ_QUEUE_UNIT_HEM0_PORT_GROUPS_BMP_CNS;
            }
            else
            {
                *portGroupsBmpPtr = LION2_TXQ_QUEUE_UNIT_HEM1_PORT_GROUPS_BMP_CNS;
            }
            break;
        case LION2_TXQ_LL_INDEX: /*0x10800000*/ /*PRV_CPSS_DXCH_LION2_TXQ_LL_BASE_ADDRESS_CNS*/
            if (portGroupId == CPSS_PORT_GROUP_UNAWARE_MODE_CNS)
            {
                *portGroupsBmpPtr = LION2_TXQ_LL_UNIT_HEM0_PORT_GROUPS_BMP_CNS |
                    LION2_TXQ_LL_UNIT_HEM1_PORT_GROUPS_BMP_CNS;
            }
            else if (portGroupId < PRV_CPSS_DXCH_PORT_GROUPS_NUM_IN_HEMISPHERE_CNS)
            {
                *portGroupsBmpPtr = LION2_TXQ_LL_UNIT_HEM0_PORT_GROUPS_BMP_CNS;
            }
            else
            {
                *portGroupsBmpPtr = LION2_TXQ_LL_UNIT_HEM1_PORT_GROUPS_BMP_CNS;
            }
            break;
        case LION2_TXQ_SHT_INDEX: /*0x11800000*/ /*PRV_CPSS_DXCH_LION2_TXQ_SHT_BASE_ADDRESS_CNS*/
            if (portGroupId == CPSS_PORT_GROUP_UNAWARE_MODE_CNS)
            {
                *portGroupsBmpPtr = LION2_TXQ_SHT_UNIT_HEM0_PORT_GROUPS_BMP_CNS |
                                    LION2_TXQ_SHT_UNIT_HEM1_PORT_GROUPS_BMP_CNS;
            }
            else if (portGroupId < PRV_CPSS_DXCH_PORT_GROUPS_NUM_IN_HEMISPHERE_CNS)
            {
                *portGroupsBmpPtr = LION2_TXQ_SHT_UNIT_HEM0_PORT_GROUPS_BMP_CNS;
            }
            else
            {
                *portGroupsBmpPtr = LION2_TXQ_SHT_UNIT_HEM1_PORT_GROUPS_BMP_CNS;
            }
            break;
        case LION2_TXQ_DIST_INDEX: /*0x13000000*/ /*PRV_CPSS_DXCH_LION2_TXQ_DIST_BASE_ADDRESS_CNS*/
            *portGroupsBmpPtr = LION2_TXQ_DIST_UNIT_PORT_GROUPS_BMP_CNS;
            break;
        case LION2_BMA_INDEX: /*0x03800000*//*PRV_CPSS_DXCH_LION2_BMA_BASE_ADDRESS_CNS*/
            *portGroupsBmpPtr = LION2_BMA_UNIT_PORT_GROUPS_BMP_CNS;
            break;
        case LION2_CPFC_INDEX: /*0x13800000*/
            *portGroupsBmpPtr = LION2_CPFC_UNIT_PORT_GROUPS_BMP_CNS;
            break;
        case LION2_DFX_SERVER_INDEX: /*0x01800000*//*PRV_CPSS_DXCH_LION2_DFX_SERVER_BASE_ADDRESS_CNS*/
            *portGroupsBmpPtr = LION2_DFX_SERVER_UNIT_PORT_GROUPS_BMP_CNS;
            break;
        case LION2_CTU0_INDEX: /*0x07000000*/
        case LION2_CTU1_INDEX: /*0x0A800000*/
            if (portGroupId == CPSS_PORT_GROUP_UNAWARE_MODE_CNS)
            {
                *portGroupsBmpPtr = LION2_CTU_UNIT_HEM0_PORT_GROUPS_BMP_CNS |
                    LION2_CTU_UNIT_HEM1_PORT_GROUPS_BMP_CNS;
            }
            else if (portGroupId < PRV_CPSS_DXCH_PORT_GROUPS_NUM_IN_HEMISPHERE_CNS)
            {
                *portGroupsBmpPtr = LION2_CTU_UNIT_HEM0_PORT_GROUPS_BMP_CNS;
            }
            else
            {
                *portGroupsBmpPtr = LION2_CTU_UNIT_HEM1_PORT_GROUPS_BMP_CNS;
            }
            break;
       default:
           return GT_FALSE;
    }

    return GT_TRUE;
}

/*******************************************************************************
* prvCpssLion3DuplicatedMultiPortGroupsGet
*
* DESCRIPTION:
*       Check if register address is duplicated in multi-port groups device
*       for Lion3 device.
*
* APPLICABLE DEVICES:
*        Lion3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion; Lion2; Bobcat2.
*
* INPUTS:
*       portGroupId - The port group id. relevant only to 'multi-port-groups'
*                     devices. Supports value CPSS_PORT_GROUP_UNAWARE_MODE_CNS
*       regAddr     - register address to access.
*
* OUTPUTS:
*       portGroupsBmpPtr  - (pointer to) the updated port groups
*                       relevant only when function returns GT_TRUE
*       isAdditionalRegDupNeededPtr - (pointer to)
*             GT_TRUE - the caller need to duplicate the value to additional
*                       register in all 'port groups bmp'.
*             GT_FALSE - the caller NOT need to duplicate the value to additional
*                       register.
*       additionalRegDupPtr - (pointer to)
*                       register address of additional register duplication.
*                       Only relevant when isAdditionalRegDupNeeded is GT_TRUE.
*
* RETURNS:
*       GT_TRUE     - the caller need to use the updated 'port groups bmp'
*       GT_FALSE    - the caller NOT need to use the updated 'port groups bmp'
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_BOOL prvCpssLion3DuplicatedMultiPortGroupsGet
(
    IN GT_U32                   portGroupId,
    IN GT_U32                   regAddr,
    OUT  GT_PORT_GROUPS_BMP     *portGroupsBmpPtr,
    OUT  GT_BOOL                *isAdditionalRegDupNeededPtr,
    OUT  GT_U32                 *additionalRegDupPtr
)
{
    GT_U32  unitIndex;

    unitIndex = U32_GET_FIELD_MAC(regAddr , LION3_SHIFT_UNIT_CNS , 9);/* use ALL 9 BITS */
    *isAdditionalRegDupNeededPtr = GT_FALSE;
    switch(unitIndex)
    {
        case LION3_FDB_INDEX_CNS:
            *portGroupsBmpPtr = LION3_FDB_UNIT_PORT_GROUPS_BMP_CNS;
            return GT_TRUE;
        case LION3_MG_INDEX_CNS:
            if(regAddr == 0x000000c0 ||
               regAddr == 0x000000c4)
            {
                #if 0
                /* this #if 0 is to allow the 'references' to find those registers */
                addrPtr->globalRegs.auQBaseAddr is 0x000000c0
                addrPtr->globalRegs.auQControl  is 0x000000c4
                #endif /*0*/
                /* those registers are for the AUQ management. and since we have
                   only single FDB and single AUQ, we need to access single unit */
                *portGroupsBmpPtr = LION3_FDB_UNIT_PORT_GROUPS_BMP_CNS;
                return GT_TRUE;
            }
            break;
        case LION3_EGF_EFT_INDEX_CNS:
        case LION3_EGF_SHT_INDEX_MAC(0):
        case LION3_EGF_SHT_INDEX_MAC(1):
        case LION3_EGF_SHT_INDEX_MAC(2):
        case LION3_EGF_SHT_INDEX_MAC(3):
        case LION3_EGF_SHT_INDEX_MAC(4):
        case LION3_EGF_SHT_INDEX_MAC(5):
        case LION3_EGF_SHT_INDEX_MAC(6):
        case LION3_EGF_SHT_INDEX_MAC(7):
        case LION3_EGF_SHT_INDEX_MAC(8):
        case LION3_EGF_SHT_INDEX_MAC(9):
        case LION3_EGF_SHT_INDEX_MAC(10):
        case LION3_EGF_SHT_INDEX_MAC(11):
        case LION3_EGF_SHT_INDEX_MAC(12):
        case LION3_LPM_INDEX_CNS(0):
        case LION3_LPM_INDEX_CNS(1):
            /* shared per 2 port groups -- used in memory of ingress pipe */
            *portGroupsBmpPtr = SHARED_EVERY_2_PORT_GROUPS_CNS;
            return GT_TRUE;

        case LION3_EGF_QAG_INDEX_MAC(0):
        case LION3_EGF_QAG_INDEX_MAC(1):
            /* shared per 4 port groups -- used in memory of egress pipe */
            *portGroupsBmpPtr = SHARED_EVERY_4_PORT_GROUPS_CNS;
            return GT_TRUE;

        case LION3_MLL_INDEX_CNS:
            /* shared per 2 port groups */
            *portGroupsBmpPtr = SHARED_EVERY_2_PORT_GROUPS_CNS;
            return GT_TRUE;

        case LION3_TCAM_INDEX_CNS:
            /* shared per 4 port groups -- used in memory of ingress pipe for IPCL,TTI and egress pipe for EPCL */
            *portGroupsBmpPtr = SHARED_EVERY_4_PORT_GROUPS_CNS;
            return GT_TRUE;

        /* other Lion3 units that we covered and not want to go to Lion2 implementation */
        case LION3_TTI_INDEX_CNS:
        case LION3_L2I_INDEX_CNS:
        case LION3_EQ_INDEX_MAC(0):
        case LION3_EQ_INDEX_MAC(1):
        case LION3_HA_INDEX_CNS:
        case LION3_PCL_INDEX_CNS:
        case LION3_EPCL_INDEX_CNS:
        case LION3_IPVX_INDEX_CNS:
        case LION3_IPLR_INDEX_CNS:
        case LION3_IPLR_1_INDEX_CNS:
        case LION3_EPLR_INDEX_CNS:
        case LION3_CNC_0_INDEX_CNS:
        case LION3_CNC_1_INDEX_CNS:
            return GT_FALSE;

        case LION3_TXQ_PFC_INDEX_CNS:
            *portGroupsBmpPtr = LION2_TXQ_LL_UNIT_HEM0_PORT_GROUPS_BMP_CNS;
            return GT_TRUE;

        case LION3_TXQ_QUEUE_INDEX_CNS:
        case LION3_TXQ_LL_INDEX_CNS:
        case LION3_TXQ_QCN_INDEX_CNS:
        /*case LION3_TXQ_DQ_INDEX_CNS:*/
            if (portGroupId == CPSS_PORT_GROUP_UNAWARE_MODE_CNS)
            {
                *portGroupsBmpPtr = SHARED_EVERY_4_PORT_GROUPS_CNS;
            }
            else if (portGroupId < PRV_CPSS_DXCH_PORT_GROUPS_NUM_IN_HEMISPHERE_CNS)
            {
                *portGroupsBmpPtr = LION2_TXQ_LL_UNIT_HEM0_PORT_GROUPS_BMP_CNS;
            }
            else
            {
                CPSS_TBD_BOOKMARK_EARCH
                /* should be changed to 0x4 when
                   PRV_CPSS_DXCH_PORT_GROUPS_NUM_IN_HEMISPHERE_CNS
                   will be reduced to 2 */
                *portGroupsBmpPtr = LION2_TXQ_LL_UNIT_HEM1_PORT_GROUPS_BMP_CNS;
            }

            return GT_TRUE;

       default:
            break;
    }

    /* continue with Lion2 memory space ... because I don't want to copy all it's cases */
    return prvCpssLion2DuplicatedMultiPortGroupsGet(
            portGroupId,regAddr,portGroupsBmpPtr,
            isAdditionalRegDupNeededPtr,additionalRegDupPtr);
}

/*******************************************************************************
* prvCpssPuma3DuplicatedMultiPortGroupsGet
*
* DESCRIPTION:
*       Check if register address is duplicated in multi-port groups device
*       for Puma3 device.
*
* APPLICABLE DEVICES:
*        ExMxPm3.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion.
*        ExMxPm.
*
* INPUTS:
*       portGroupId - The port group id. relevant only to 'multi-port-groups'
*                     devices. Supports value CPSS_PORT_GROUP_UNAWARE_MODE_CNS
*       regAddr     - register address to access.
*
* OUTPUTS:
*       portGroupsBmpPtr  - (pointer to) the updated port groups
*                       relevant only when function returns GT_TRUE
*       isAdditionalRegDupNeededPtr - (pointer to)
*             GT_TRUE - the caller need to duplicate the value to additional
*                       register in all 'port groups bmp'.
*             GT_FALSE - the caller NOT need to duplicate the value to additional
*                       register.
*       additionalRegDupPtr - (pointer to)
*                       register address of additional register duplication.
*                       Only relevant when isAdditionalRegDupNeeded is GT_TRUE.
*
* RETURNS:
*       GT_TRUE     - the caller need to use the updated 'port groups bmp'
*       GT_FALSE    - the caller NOT need to use the updated 'port groups bmp'
*
* COMMENTS:
*
*
*******************************************************************************/
GT_BOOL prvCpssPuma3DuplicatedMultiPortGroupsGet
(
    IN GT_U32                   portGroupId,
    IN GT_U32                   regAddr,
    OUT  GT_PORT_GROUPS_BMP     *portGroupsBmpPtr,
    OUT  GT_BOOL                *isAdditionalRegDupNeededPtr,
    OUT  GT_U32                 *additionalRegDupPtr
)
{
    GT_U32  unitIndex = U32_GET_FIELD_MAC(regAddr , 20 , 8);
    GT_U32  unitIndexAdditional;/* unit index of the additional register*/

    *isAdditionalRegDupNeededPtr = GT_FALSE;

    /* allow CPSS ExMxPm API to 'think' that there are 4 port groups
    but there are only 2 'MG units' (management) units */
    switch(portGroupId)
    {
        case CPSS_PORT_GROUP_UNAWARE_MODE_CNS:
            /* support 'loop' on port groups --> remove redundancy --> MG 0,1 */
            *portGroupsBmpPtr = (BIT_0 | BIT_1);
            break;
        case 0:
            /* support access to port group 0 --> fabric port group on MG 0 */
        case 2:
            /* support access to port group 2 --> fabric port group on MG 0 */
            *portGroupsBmpPtr = BIT_0;
            break;
        case 1:
            /* support access to port group 3 --> fabric port group on MG 1 */
        case 3:
            /* support access to port group 3 --> fabric port group on MG 1 */
            *portGroupsBmpPtr = BIT_1;
            break;
        default:
            /* should not get here */
            return GT_FALSE;
    }

    switch(U32_GET_FIELD_MAC(regAddr , 28 , 4))/* bits 28..31*/
    {
        case 0:
            switch(unitIndex)/* bits 20..27*/
            {
                case 45 :       /* BMA                     */
                case 48 :       /* Lookup_EMC              */
                case 49 :       /* Lookup_EMC_Memory       */
                case 50 :       /* Forwarding_EMC          */
                case 51 :       /* Forwarding_EMC_Memory   */
                case 52 :       /* FCU                     */
                case 57 :       /* EGF_SHT0                */
                case 58 :       /* EGF_SHT1                */
                case 59 :       /* EGF_QAG                 */
                case 60 :       /* TXQ_Queue               */
                case 61 :       /* TXQ_LL                  */
                case 62 :       /* TXQ_PFC                 */
                case 63 :       /* TXQ_QCN                 */
                case 64 :       /* TXQ_DQ0                 */
                case 65 :       /* TXQ_DQ1                 */
                case 66 :       /* TXQ_DQ2                 */
                case 67 :       /* TXQ_DQ3                 */
                case 68 :       /* DFX_Server              */
                case 69 :       /* Action_Table            */
                case 70 :       /* TCAM_    _IF_UNIT       */
                case 71 :       /* MPPM0_BANK0             */
                case 72 :       /* MPPM0_BANK1             */
                case 73 :       /* MPPM1_BANK0             */
                case 74 :       /* MPPM1_BANK1             */
                case 75 :       /* MPPM2_BANK0             */
                case 76 :       /* MPPM2_BANK1             */
                case 77 :       /* MPPM3_BANK0             */
                case 78 :       /* MPPM3_BANK1             */
                    *portGroupsBmpPtr = BIT_0;
                    break;
                case 53 :       /* EGF_EFT0                */
                case 54 :       /* EGF_EFT1                */
                case 55 :       /* EGF_EFT2                */
                case 56 :       /* EGF_EFT3                */
                    /*
                        unitIndex       unitIndexAdditional
                        53                  54
                        54                  53
                        55                  56
                        56                  55

                        Duplication due to performance requirements:
                        EFT 0 and EFT 1 must be set with the same values.
                        EFT 2 and EFT 3 must be set with the same values.

                        could use next formula :
                        unitIndexAdditional = (((1 + unitIndex) ^ 0x1) - 1);

                        but to get more 'clear' understanding using next calculation:
                    */
                    if(unitIndex & 1)
                    {
                        /*53,55 convert to 54,56 */
                        unitIndexAdditional = unitIndex + 1;
                    }
                    else
                    {
                        /*54,56 convert to 53,55 */
                        unitIndexAdditional = unitIndex - 1;
                    }


                    *isAdditionalRegDupNeededPtr = GT_TRUE;
                    *additionalRegDupPtr = regAddr;
                    U32_SET_FIELD_MASKED_MAC((*additionalRegDupPtr),20,8,unitIndexAdditional);

                    *portGroupsBmpPtr = BIT_0;
                    break;

                default:
                    break;
            }

            break;
        case 3:
        case 4:
        case 5:
            *portGroupsBmpPtr = BIT_0;
            break;
        default:
            break;
    }

    if(regAddr == PRV_CPSS_SW_PTR_ENTRY_UNUSED)
    {
        #ifdef ALLOW_EMPTY_PORT_GROUPS
            /* Allow current debug level for 'Bring up' to avoid those fails */
            *portGroupsBmpPtr = 0;
        #endif /*PUMA2_AS_PUMA3*/
    }

    return GT_TRUE;
}


/*******************************************************************************
* prvCpssBobkDuplicatedMultiPortGroupsGet
*
* DESCRIPTION:
*       Check if register address is duplicated in bobk device.
*
* APPLICABLE DEVICES:
*        Bobcat2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion.
*        ExMxPm.
*
* INPUTS:
*       portGroupId - The port group id. relevant only to 'multi-port-groups'
*                     devices. Supports value CPSS_PORT_GROUP_UNAWARE_MODE_CNS
*       regAddr     - register address to access.
*
* OUTPUTS:
*       portGroupsBmpPtr  - (pointer to) the updated port groups
*                       relevant only when function returns GT_TRUE
*       isAdditionalRegDupNeededPtr - (pointer to)
*             GT_TRUE - the caller need to duplicate the value to additional
*                       register in all 'port groups bmp'.
*             GT_FALSE - the caller NOT need to duplicate the value to additional
*                       register.
*       additionalRegDupPtr - (pointer to)
*                       register address of additional register duplication.
*                       Only relevant when isAdditionalRegDupNeeded is GT_TRUE.
*
* RETURNS:
*       GT_TRUE     - the caller need to use the updated 'port groups bmp'
*       GT_FALSE    - the caller NOT need to use the updated 'port groups bmp'
*
* COMMENTS:
*
*
*******************************************************************************/
static GT_BOOL prvCpssBobkDuplicatedMultiPortGroupsGet_byDevNum
(
    IN GT_U8                    devNum,
    IN GT_U32                   portGroupId,
    IN GT_U32                   regAddr,
    OUT  GT_PORT_GROUPS_BMP     *portGroupsBmpPtr,
    OUT  GT_BOOL                *isAdditionalRegDupNeededPtr,
    OUT  GT_U32                 *additionalRegDupPtr
)
{
    PRV_CPSS_DXCH_UNIT_ENT  unitId;
    GT_U32  additionalBaseAddr;

    unitId = prvCpssDxChHwRegAddrToUnitIdConvert(devNum, regAddr);
    switch(unitId)
    {
        case PRV_CPSS_DXCH_UNIT_RXDMA_E:
            additionalBaseAddr = prvCpssDxChHwUnitBaseAddrGet(devNum,PRV_CPSS_DXCH_UNIT_RXDMA1_E,NULL);
            break;
        case PRV_CPSS_DXCH_UNIT_TXDMA_E:
            additionalBaseAddr = prvCpssDxChHwUnitBaseAddrGet(devNum,PRV_CPSS_DXCH_UNIT_TXDMA1_E,NULL);
            break;
        case PRV_CPSS_DXCH_UNIT_TX_FIFO_E:
            additionalBaseAddr = prvCpssDxChHwUnitBaseAddrGet(devNum,PRV_CPSS_DXCH_UNIT_TX_FIFO1_E,NULL);
            break;
        default:
            /* we not need to duplicate the address */
            return GT_FALSE;

    }

    /* the device not supports multi cores ... so state 'unaware' */
    *portGroupsBmpPtr = CPSS_PORT_GROUP_UNAWARE_MODE_CNS;

    *isAdditionalRegDupNeededPtr = GT_TRUE;
    /* base address is 8 MSBits added with 24 LSBits of original address */
    *additionalRegDupPtr = additionalBaseAddr | (regAddr & 0x00FFFFFF);

    (void)portGroupId;

    return GT_TRUE;
}

/*******************************************************************************
* prvCpssBobkDuplicatedMultiPortGroupsGet
*
* DESCRIPTION:
*       Check if register address is duplicated in bobk device.
*
* APPLICABLE DEVICES:
*        Bobcat2.
*
* NOT APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2; Lion.
*        ExMxPm.
*
* INPUTS:
*       portGroupId - The port group id. relevant only to 'multi-port-groups'
*                     devices. Supports value CPSS_PORT_GROUP_UNAWARE_MODE_CNS
*       regAddr     - register address to access.
*
* OUTPUTS:
*       portGroupsBmpPtr  - (pointer to) the updated port groups
*                       relevant only when function returns GT_TRUE
*       isAdditionalRegDupNeededPtr - (pointer to)
*             GT_TRUE - the caller need to duplicate the value to additional
*                       register in all 'port groups bmp'.
*             GT_FALSE - the caller NOT need to duplicate the value to additional
*                       register.
*       additionalRegDupPtr - (pointer to)
*                       register address of additional register duplication.
*                       Only relevant when isAdditionalRegDupNeeded is GT_TRUE.
*
* RETURNS:
*       GT_TRUE     - the caller need to use the updated 'port groups bmp'
*       GT_FALSE    - the caller NOT need to use the updated 'port groups bmp'
*
* COMMENTS:
*
*
*******************************************************************************/
GT_BOOL prvCpssBobkDuplicatedMultiPortGroupsGet
(
    IN GT_U32                   portGroupId,
    IN GT_U32                   regAddr,
    OUT  GT_PORT_GROUPS_BMP     *portGroupsBmpPtr,
    OUT  GT_BOOL                *isAdditionalRegDupNeededPtr,
    OUT  GT_U32                 *additionalRegDupPtr
)
{
    GT_U8   devNum = (GT_U8)BOBK_DEVICE_INDICATION_CNS;

    return
        prvCpssBobkDuplicatedMultiPortGroupsGet_byDevNum(devNum,
            portGroupId,regAddr,
            portGroupsBmpPtr,isAdditionalRegDupNeededPtr,additionalRegDupPtr);
}


/*******************************************************************************
* prvCpssDuplicatedMultiPortGroupsGet
*
* DESCRIPTION:
*       Check if register address is duplicated in multi-port groups device
*
* APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2 ; Lion; Lion2.
*        ExMxPm ; ExMxPm3.
*
* NOT APPLICABLE DEVICES:
*       None.
*
* INPUTS:
*       devNum      - the device number.
*       portGroupId - The port group id. relevant only to 'multi-port-groups'
*                     devices. Supports value CPSS_PORT_GROUP_UNAWARE_MODE_CNS
*       regAddr     - register address to access.
*
* OUTPUTS:
*       portGroupsBmpPtr  - (pointer to) the updated port groups
*                       relevant only when function returns GT_TRUE
*       isAdditionalRegDupNeededPtr - (pointer to)
*             GT_TRUE - the caller need to duplicate the value to additional
*                       register in all 'port groups bmp'.
*             GT_FALSE - the caller NOT need to duplicate the value to additional
*                       register.
*       additionalRegDupPtr - (pointer to)
*                       register address of additional register duplication.
*                       Only relevant when isAdditionalRegDupNeeded is GT_TRUE.
*
* RETURNS:
*       GT_TRUE     - the caller need to use the updated 'port groups bmp'
*       GT_FALSE    - the caller NOT need to use the updated 'port groups bmp'
*
* COMMENTS:
*
*
*******************************************************************************/
GT_BOOL prvCpssDuplicatedMultiPortGroupsGet
(
    IN GT_U8                    devNum,
    IN GT_U32                   portGroupId,
    IN GT_U32                   regAddr,
    OUT  GT_PORT_GROUPS_BMP     *portGroupsBmpPtr,
    OUT  GT_BOOL                *isAdditionalRegDupNeededPtr,
    OUT  GT_U32                 *additionalRegDupPtr
)
{
    GT_BOOL     didBmpUpgrade;

    *isAdditionalRegDupNeededPtr = GT_FALSE;

    /* NOTE : use prvCpssDrvPpConfig[devNum] instead of PRV_CPSS_PP_MAC(devNum)
        because the first call is done from the cpssDriver before the
        PRV_CPSS_PP_MAC(devNum) was allocated and initialized ! */

    switch(prvCpssDrvPpConfig[devNum]->devFamily)
    {
        case CPSS_PP_FAMILY_DXCH_LION_E:
            didBmpUpgrade = prvCpssLionDuplicatedMultiPortGroupsGet(regAddr,portGroupsBmpPtr);
            break;
        case CPSS_PP_FAMILY_DXCH_LION2_E:
            didBmpUpgrade = prvCpssLion2DuplicatedMultiPortGroupsGet(portGroupId,regAddr,portGroupsBmpPtr,
                   isAdditionalRegDupNeededPtr,additionalRegDupPtr);
            break;
        case CPSS_PP_FAMILY_DXCH_LION3_E:
            didBmpUpgrade = prvCpssLion3DuplicatedMultiPortGroupsGet(portGroupId,regAddr,portGroupsBmpPtr,
                   isAdditionalRegDupNeededPtr,additionalRegDupPtr);
            break;
        case CPSS_PP_FAMILY_PUMA3_E:
            didBmpUpgrade = prvCpssPuma3DuplicatedMultiPortGroupsGet(portGroupId,regAddr,portGroupsBmpPtr,
                   isAdditionalRegDupNeededPtr,additionalRegDupPtr);
            break;
        case CPSS_PP_FAMILY_DXCH_BOBCAT2_E:
            if(prvCpssDrvPpConfig[devNum]->devSubFamily == CPSS_PP_SUB_FAMILY_BOBCAT2_BOBK_E)
            {
                didBmpUpgrade = prvCpssBobkDuplicatedMultiPortGroupsGet_byDevNum(devNum,
                       portGroupId,regAddr,portGroupsBmpPtr,
                       isAdditionalRegDupNeededPtr,additionalRegDupPtr);
            }
            else
            {
                didBmpUpgrade = GT_FALSE;
            }
            break;
        case CPSS_PP_FAMILY_DXCH_BOBCAT3_E:
            CPSS_TBD_BOOKMARK_BOBCAT3
            didBmpUpgrade = GT_FALSE;
            break;
        default:
            didBmpUpgrade = GT_FALSE;
            break;
    }

    if (didBmpUpgrade == GT_FALSE)
    {
        return GT_FALSE;
    }

    /* mask the active ports only */
    *portGroupsBmpPtr &= prvCpssDrvPpConfig[devNum]->portGroupsInfo.activePortGroupsBmp;

    return GT_TRUE;
}

/*******************************************************************************
* prvCpssHwPpReadRegister
*
* DESCRIPTION:
*       Read a register value from the given PP. - generic (non-driver) API
*
* APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2 ; Lion; Lion2.
*        ExMxPm ; ExMxPm3.
*
* NOT APPLICABLE DEVICES:
*       None.
*
* INPUTS:
*       devNum  - The PP to read from.
*       regAddr - The register's address to read from.
*
* OUTPUTS:
*       dataPtr - (pointer to) memory for the register value.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on hardware error
*       GT_NOT_INITIALIZED - if the driver was not initialized
*       GT_HW_ERROR - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvCpssHwPpReadRegister
(
    IN GT_U8    devNum,
    IN GT_U32   regAddr,
    OUT GT_U32  *dataPtr
)
{
    return prvCpssHwPpPortGroupGetRegField(devNum,
        CPSS_PORT_GROUP_UNAWARE_MODE_CNS,regAddr,
        0,32,
        dataPtr);
}



/*******************************************************************************
* prvCpssHwPpWriteRegister
*
* DESCRIPTION:
*       Write to a PP's given register. - generic (non-driver) API
*
* APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2 ; Lion; Lion2.
*        ExMxPm ; ExMxPm3.
*
* NOT APPLICABLE DEVICES:
*       None.
*
* INPUTS:
*       devNum  - The PP to write to.
*       regAddr - The register's address to write to.
*       value   - The value to be written.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on hardware error
*       GT_NOT_INITIALIZED - if the driver was not initialized
*       GT_HW_ERROR - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvCpssHwPpWriteRegister
(
    IN GT_U8 devNum,
    IN GT_U32 regAddr,
    IN GT_U32 value
)
{
    return prvCpssHwPpPortGroupSetRegField(devNum,
        CPSS_PORT_GROUP_UNAWARE_MODE_CNS,regAddr,
        0,32,
        value);
}



/*******************************************************************************
* prvCpssHwPpGetRegField
*
* DESCRIPTION:
*       Read a selected register field. - generic (non-driver) API
*
* APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2 ; Lion; Lion2.
*        ExMxPm ; ExMxPm3.
*
* NOT APPLICABLE DEVICES:
*       None.
*
* INPUTS:
*       devNum  - The PP device number to read from.
*       regAddr - The register's address to read from.
*       fieldOffset - The start bit number in the register.
*       fieldLength - The number of bits to be read.
*
* OUTPUTS:
*       fieldDataPtr   - (pointer to) Data to read from the register.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on hardware error
*       GT_NOT_INITIALIZED - if the driver was not initialized
*       GT_HW_ERROR - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvCpssHwPpGetRegField
(
    IN GT_U8    devNum,
    IN GT_U32   regAddr,
    IN GT_U32   fieldOffset,
    IN GT_U32   fieldLength,
    OUT GT_U32  *fieldDataPtr
)
{
    return prvCpssHwPpPortGroupGetRegField(devNum,
        CPSS_PORT_GROUP_UNAWARE_MODE_CNS,regAddr,
        fieldOffset,fieldLength,fieldDataPtr);
}



/*******************************************************************************
* prvCpssHwPpSetRegField
*
* DESCRIPTION:
*       Write value to selected register field. - generic (non-driver) API
*
* APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2 ; Lion; Lion2.
*        ExMxPm ; ExMxPm3.
*
* NOT APPLICABLE DEVICES:
*       None.
*
* INPUTS:
*       devNum  - The PP device number to write to.
*       regAddr - The register's address to write to.
*       fieldOffset - The start bit number in the register.
*       fieldLength - The number of bits to be written to register.
*       fieldData   - Data to be written into the register.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on hardware error
*       GT_NOT_INITIALIZED - if the driver was not initialized
*       GT_HW_ERROR - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvCpssHwPpSetRegField
(
    IN GT_U8 devNum,
    IN GT_U32 regAddr,
    IN GT_U32 fieldOffset,
    IN GT_U32 fieldLength,
    IN GT_U32 fieldData
)
{
    return prvCpssHwPpPortGroupSetRegField(devNum,
        CPSS_PORT_GROUP_UNAWARE_MODE_CNS,regAddr,
        fieldOffset,fieldLength,fieldData);
}



/*******************************************************************************
* prvCpssHwPpReadRegBitMask
*
* DESCRIPTION:
*       Reads the unmasked bits of a register. - generic (non-driver) API
*
* APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2 ; Lion; Lion2.
*        ExMxPm ; ExMxPm3.
*
* NOT APPLICABLE DEVICES:
*       None.
*
* INPUTS:
*       devNum  - PP device number to read from.
*       regAddr - Register address to read from.
*       mask    - Mask for selecting the read bits.
*
* OUTPUTS:
*       dataPtr    - Data read from register.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on hardware error
*       GT_NOT_INITIALIZED - if the driver was not initialized
*       GT_HW_ERROR - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       The bits in value to be read are the masked bit of 'mask'.
*
*******************************************************************************/
GT_STATUS prvCpssHwPpReadRegBitMask
(
    IN GT_U8    devNum,
    IN GT_U32   regAddr,
    IN GT_U32   mask,
    OUT GT_U32  *dataPtr
)
{
    return prvCpssHwPpPortGroupReadRegBitMask(devNum,
        CPSS_PORT_GROUP_UNAWARE_MODE_CNS,regAddr,
        mask,dataPtr);
}




/*******************************************************************************
* prvCpssHwPpWriteRegBitMask
*
* DESCRIPTION:
*       Writes the unmasked bits of a register. - generic (non-driver) API
*
* APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2 ; Lion; Lion2.
*        ExMxPm ; ExMxPm3.
*
* NOT APPLICABLE DEVICES:
*       None.
*
* INPUTS:
*       devNum  - PP device number to write to.
*       regAddr - Register address to write to.
*       mask    - Mask for selecting the written bits.
*       value   - Data to be written to register.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on hardware error
*       GT_NOT_INITIALIZED - if the driver was not initialized
*       GT_HW_ERROR - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       The bits in value to be written are the masked bit of 'mask'.
*
*******************************************************************************/
GT_STATUS prvCpssHwPpWriteRegBitMask
(
    IN GT_U8 devNum,
    IN GT_U32 regAddr,
    IN GT_U32 mask,
    IN GT_U32 value
)
{
    return prvCpssHwPpPortGroupWriteRegBitMask(devNum,
        CPSS_PORT_GROUP_UNAWARE_MODE_CNS,regAddr,
        mask,value);
}



/*******************************************************************************
* prvCpssHwPpReadRam
*
* DESCRIPTION:
*       Read from PP's RAM. - generic (non-driver) API
*
* APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2 ; Lion; Lion2.
*        ExMxPm ; ExMxPm3.
*
* NOT APPLICABLE DEVICES:
*       None.
*
* INPUTS:
*       devNum  - The PP device number to read from.
*       addr    - Address offset to read from.
*       length  - Number of Words (4 byte) to read.
*
* OUTPUTS:
*       dataPtr    - (pointer to) An array containing the read data.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on hardware error
*       GT_NOT_INITIALIZED - if the driver was not initialized
*       GT_HW_ERROR - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvCpssHwPpReadRam
(
    IN  GT_U8   devNum,
    IN  GT_U32  addr,
    IN  GT_U32  length,
    OUT GT_U32  *dataPtr
)
{
    return prvCpssHwPpPortGroupReadRam(
        devNum,CPSS_PORT_GROUP_UNAWARE_MODE_CNS,
        addr,length,dataPtr);
}


/*******************************************************************************
* prvCpssHwPpWriteRam
*
* DESCRIPTION:
*       Writes to PP's RAM. - generic (non-driver) API
*
* APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2 ; Lion; Lion2.
*        ExMxPm ; ExMxPm3.
*
* NOT APPLICABLE DEVICES:
*       None.
*
* INPUTS:
*       devNum  - The PP device number to write to.
*       addr    - Address offset to write to.
*       length  - Number of Words (4 byte) to write.
*       dataPtr - (pointer to) the array containing the data to be written.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on hardware error
*       GT_NOT_INITIALIZED - if the driver was not initialized
*       GT_HW_ERROR - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvCpssHwPpWriteRam
(
    IN GT_U8   devNum,
    IN GT_U32  addr,
    IN GT_U32  length,
    IN GT_U32  *dataPtr
)
{
    return prvCpssHwPpPortGroupWriteRam(devNum,CPSS_PORT_GROUP_UNAWARE_MODE_CNS,
        addr,length,dataPtr);
}

/*******************************************************************************
* prvCpssHwPpPortGroupReadRegister
*
* DESCRIPTION:
*       Read a register value from the given PP. - generic (non-driver) API
*       in the specific port group in the device
*
* APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2 ; Lion; Lion2.
*        ExMxPm ; ExMxPm3.
*
* NOT APPLICABLE DEVICES:
*       None.
*
* INPUTS:
*       devNum      - The PP to read from.
*       portGroupId - The port group id. relevant only to 'multi-port-groups'
*                     devices. Supports value CPSS_PORT_GROUP_UNAWARE_MODE_CNS
*       regAddr     - The register's address to read from.
*
* OUTPUTS:
*       dataPtr - (pointer to) memory for the register value.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on hardware error
*       GT_NOT_INITIALIZED - if the driver was not initialized
*       GT_HW_ERROR - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvCpssHwPpPortGroupReadRegister
(
    IN GT_U8    devNum,
    IN GT_U32   portGroupId,
    IN GT_U32   regAddr,
    OUT GT_U32  *dataPtr
)
{
    return prvCpssHwPpPortGroupGetRegField(
        devNum, portGroupId,
        regAddr, 0, 32, dataPtr);
}

/*******************************************************************************
* prvCpssHwPpPortGroupWriteRegister
*
* DESCRIPTION:
*       Write to a PP's given register. - generic (non-driver) API
*       in the specific port group in the device
*
* APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2 ; Lion; Lion2.
*        ExMxPm ; ExMxPm3.
*
* NOT APPLICABLE DEVICES:
*       None.
*
* INPUTS:
*       devNum      - The PP to write to.
*       portGroupId - The port group id. relevant only to 'multi-port-groups'
*                     devices. Supports value CPSS_PORT_GROUP_UNAWARE_MODE_CNS
*       regAddr     - The register's address to write to.
*       value       - The value to be written.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on hardware error
*       GT_NOT_INITIALIZED - if the driver was not initialized
*       GT_HW_ERROR - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvCpssHwPpPortGroupWriteRegister
(
    IN GT_U8    devNum,
    IN GT_U32   portGroupId,
    IN GT_U32   regAddr,
    IN GT_U32   value
)
{
    return prvCpssHwPpPortGroupSetRegField(devNum,portGroupId,
        regAddr,
        0,32,
        value);
}




/*******************************************************************************
* prvCpssHwPpPortGroupGetRegField
*
* DESCRIPTION:
*       Read a selected register field. - generic (non-driver) API
*       in the specific port group in the device
*
* APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2 ; Lion; Lion2.
*        ExMxPm ; ExMxPm3.
*
* NOT APPLICABLE DEVICES:
*       None.
*
* INPUTS:
*       devNum      - The PP device number to read from.
*       portGroupId - The port group id. relevant only to 'multi-port-groups'
*                     devices. Supports value CPSS_PORT_GROUP_UNAWARE_MODE_CNS
*       regAddr     - The register's address to read from.
*       fieldOffset - The start bit number in the register.
*       fieldLength - The number of bits to be read.
*
* OUTPUTS:
*       fieldDataPtr   - (pointer to) Data to read from the register.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on hardware error
*       GT_NOT_INITIALIZED - if the driver was not initialized
*       GT_HW_ERROR - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvCpssHwPpPortGroupGetRegField
(
    IN  GT_U8    devNum,
    IN  GT_U32   portGroupId,
    IN  GT_U32   regAddr,
    IN  GT_U32   fieldOffset,
    IN  GT_U32   fieldLength,
    OUT GT_U32   *fieldDataPtr
)
{
    GT_STATUS       rc;
    GT_PORT_GROUPS_BMP  portGroupsBmp;/*port groups bmp */
    GT_BOOL isAdditionalRegDupNeeded; /* not used */
    GT_U32  additionalRegDup; /* not used */

    PRV_CPSS_DEV_CHECK_MAC(devNum);

    if(fieldOffset == 0 && fieldLength == 32)
    {
        /* read register */

        if(GT_FALSE ==
            prvCpssDuplicatedMultiPortGroupsGet(
                devNum,portGroupId,regAddr,&portGroupsBmp,
                &isAdditionalRegDupNeeded,&additionalRegDup))
        {
            return prvCpssDrvHwPpPortGroupReadRegister(
                devNum,portGroupId,regAddr,fieldDataPtr);
        }

#ifdef ALLOW_EMPTY_PORT_GROUPS
        if(portGroupsBmp == 0)
        {
            return GT_OK;
        }
#endif /*ALLOW_EMPTY_PORT_GROUPS*/

        /* get first port group in the BMP */
        PRV_CPSS_MULTI_PORT_GROUPS_BMP_GET_FIRST_ACTIVE_MAC(
            devNum,portGroupsBmp,portGroupId);
        rc = prvCpssDrvHwPpPortGroupReadRegister(
            devNum,portGroupId,regAddr,fieldDataPtr);

        return rc;
    }

    if(GT_FALSE ==
        prvCpssDuplicatedMultiPortGroupsGet(
                devNum,portGroupId,regAddr,&portGroupsBmp,
                &isAdditionalRegDupNeeded,&additionalRegDup))
    {
        return prvCpssDrvHwPpPortGroupGetRegField(
            devNum,portGroupId,
            regAddr,fieldOffset,fieldLength,fieldDataPtr);
    }

#ifdef ALLOW_EMPTY_PORT_GROUPS
    if(portGroupsBmp == 0)
    {
        return GT_OK;
    }
#endif /*ALLOW_EMPTY_PORT_GROUPS*/

    /* get first port group in the BMP */
    PRV_CPSS_MULTI_PORT_GROUPS_BMP_GET_FIRST_ACTIVE_MAC(
        devNum,portGroupsBmp,portGroupId);

    rc = prvCpssDrvHwPpPortGroupGetRegField(
        devNum,portGroupId,
        regAddr,fieldOffset,fieldLength,fieldDataPtr);

    return rc;
}



/*******************************************************************************
* prvCpssHwPpPortGroupSetRegField
*
* DESCRIPTION:
*       Write value to selected register field. - generic (non-driver) API
*       in the specific port group in the device
*
* APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2 ; Lion; Lion2.
*        ExMxPm ; ExMxPm3.
*
* NOT APPLICABLE DEVICES:
*       None.
*
* INPUTS:
*       devNum      - The PP device number to write to.
*       portGroupId - The port group id. relevant only to 'multi-port-groups'
*                     devices. Supports value CPSS_PORT_GROUP_UNAWARE_MODE_CNS
*       regAddr     - The register's address to write to.
*       fieldOffset - The start bit number in the register.
*       fieldLength - The number of bits to be written to register.
*       fieldData   - Data to be written into the register.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on hardware error
*       GT_NOT_INITIALIZED - if the driver was not initialized
*       GT_HW_ERROR - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvCpssHwPpPortGroupSetRegField
(
    IN GT_U8    devNum,
    IN GT_U32   portGroupId,
    IN GT_U32   regAddr,
    IN GT_U32   fieldOffset,
    IN GT_U32   fieldLength,
    IN GT_U32   fieldData
)
{
    GT_STATUS       rc = RETURN_VALUE_OF_MULTI_PORT_GROUPS_FAIL_CNS;
    GT_PORT_GROUPS_BMP  portGroupsBmp;/*port groups bmp */
    GT_BOOL isAdditionalRegDupNeeded;
    GT_U32  additionalRegDup;
    GT_U32 addrArr[2];
    GT_U32 dataArr[2];
    GT_U32 arrLen = 1;

    PRV_CPSS_DEV_CHECK_MAC(devNum);

    if(fieldOffset == 0 && fieldLength == 32)
    {
        /* write register */
        if(GT_FALSE ==
            prvCpssDuplicatedMultiPortGroupsGet(devNum,portGroupId,regAddr,&portGroupsBmp,
                &isAdditionalRegDupNeeded,&additionalRegDup))
        {
            return prvCpssDrvHwPpPortGroupWriteRegister(devNum,portGroupId,regAddr,fieldData);
        }
        addrArr[0] = regAddr;
        dataArr[0] = fieldData;

        if (isAdditionalRegDupNeeded == GT_TRUE)
        {
            addrArr[1] = additionalRegDup;
            dataArr[1] = fieldData;
            arrLen = 2 ;
        }

        PRV_CPSS_GEN_PP_START_LOOP_PORT_GROUPS_IN_BMP_MAC(devNum,portGroupsBmp,portGroupId)
        {
            rc = prvCpssDrvHwPpPortGroupWriteVec(devNum,portGroupId,addrArr,dataArr,arrLen);
            if(rc != GT_OK)
            {
                return rc;
            }
        }
        PRV_CPSS_GEN_PP_END_LOOP_PORT_GROUPS_IN_BMP_MAC(devNum,portGroupsBmp,portGroupId)

        return rc;
    }


    if(GT_FALSE ==
        prvCpssDuplicatedMultiPortGroupsGet(devNum,portGroupId,regAddr,&portGroupsBmp,
                &isAdditionalRegDupNeeded,&additionalRegDup))
    {
        return prvCpssDrvHwPpPortGroupSetRegField(devNum,portGroupId,regAddr,fieldOffset,fieldLength,fieldData);
    }

    PRV_CPSS_GEN_PP_START_LOOP_PORT_GROUPS_IN_BMP_MAC(devNum,portGroupsBmp,portGroupId)
    {
        rc = prvCpssDrvHwPpPortGroupSetRegField(devNum,portGroupId,regAddr,fieldOffset,fieldLength,fieldData);
        if(rc != GT_OK)
        {
            return rc;
        }
        if (isAdditionalRegDupNeeded == GT_TRUE)
        {
            rc = prvCpssDrvHwPpPortGroupSetRegField(devNum,portGroupId,additionalRegDup,fieldOffset,fieldLength,fieldData);
            if(rc != GT_OK)
            {
                return rc;
            }
        }
    }
    PRV_CPSS_GEN_PP_END_LOOP_PORT_GROUPS_IN_BMP_MAC(devNum,portGroupsBmp,portGroupId)

    return rc;
}



/*******************************************************************************
* prvCpssHwPpPortGroupReadRegBitMask
*
* DESCRIPTION:
*       Reads the unmasked bits of a register. - generic (non-driver) API
*       in the specific port group in the device
*
* APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2 ; Lion; Lion2.
*        ExMxPm ; ExMxPm3.
*
* NOT APPLICABLE DEVICES:
*       None.
*
* INPUTS:
*       devNum      - PP device number to read from.
*       portGroupId - The port group id. relevant only to 'multi-port-groups'
*                     devices. Supports value CPSS_PORT_GROUP_UNAWARE_MODE_CNS
*       regAddr     - Register address to read from.
*       mask        - Mask for selecting the read bits.
*
* OUTPUTS:
*       dataPtr    - Data read from register.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on hardware error
*       GT_NOT_INITIALIZED - if the driver was not initialized
*       GT_HW_ERROR - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       The bits in value to be read are the masked bit of 'mask'.
*
*******************************************************************************/
GT_STATUS prvCpssHwPpPortGroupReadRegBitMask
(
    IN GT_U8    devNum,
    IN GT_U32   portGroupId,
    IN GT_U32   regAddr,
    IN GT_U32   mask,
    OUT GT_U32  *dataPtr
)
{
    GT_STATUS       rc;
    GT_PORT_GROUPS_BMP  portGroupsBmp;/*port groups bmp */
    GT_BOOL isAdditionalRegDupNeeded; /* not used */
    GT_U32  additionalRegDup; /* not used */

    PRV_CPSS_DEV_CHECK_MAC(devNum);

    if(GT_FALSE ==
        prvCpssDuplicatedMultiPortGroupsGet(devNum,portGroupId,regAddr,&portGroupsBmp,
                &isAdditionalRegDupNeeded,&additionalRegDup))
    {
        return prvCpssDrvHwPpPortGroupReadRegBitMask(devNum,portGroupId,regAddr,mask,dataPtr);
    }

#ifdef ALLOW_EMPTY_PORT_GROUPS
    if(portGroupsBmp == 0)
    {
        return GT_OK;
    }
#endif /*ALLOW_EMPTY_PORT_GROUPS*/

    /* get first port group in the BMP */
    PRV_CPSS_MULTI_PORT_GROUPS_BMP_GET_FIRST_ACTIVE_MAC(devNum,portGroupsBmp,portGroupId);

    rc = prvCpssDrvHwPpPortGroupReadRegBitMask(devNum,portGroupId,regAddr,mask,dataPtr);

    return rc;
}




/*******************************************************************************
* prvCpssHwPpPortGroupWriteRegBitMask
*
* DESCRIPTION:
*       Writes the unmasked bits of a register. - generic (non-driver) API
*       in the specific port group in the device
*
* APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2 ; Lion; Lion2.
*        ExMxPm ; ExMxPm3.
*
* NOT APPLICABLE DEVICES:
*       None.
*
* INPUTS:
*       devNum      - PP device number to write to.
*       portGroupId - The port group id. relevant only to 'multi-port-groups'
*                     devices. Supports value CPSS_PORT_GROUP_UNAWARE_MODE_CNS
*       regAddr     - Register address to write to.
*       mask        - Mask for selecting the written bits.
*       value       - Data to be written to register.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on hardware error
*       GT_NOT_INITIALIZED - if the driver was not initialized
*       GT_HW_ERROR - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       The bits in value to be written are the masked bit of 'mask'.
*
*******************************************************************************/
GT_STATUS prvCpssHwPpPortGroupWriteRegBitMask
(
    IN GT_U8    devNum,
    IN GT_U32   portGroupId,
    IN GT_U32   regAddr,
    IN GT_U32   mask,
    IN GT_U32   value
)
{
    GT_STATUS       rc = RETURN_VALUE_OF_MULTI_PORT_GROUPS_FAIL_CNS;
    GT_PORT_GROUPS_BMP  portGroupsBmp;/*port groups bmp */
    GT_BOOL isAdditionalRegDupNeeded;
    GT_U32  additionalRegDup;

    PRV_CPSS_DEV_CHECK_MAC(devNum);

    if(GT_FALSE ==
        prvCpssDuplicatedMultiPortGroupsGet(devNum,portGroupId,regAddr,&portGroupsBmp,
                &isAdditionalRegDupNeeded,&additionalRegDup))
    {
        return prvCpssDrvHwPpPortGroupWriteRegBitMask(devNum,portGroupId,regAddr,mask,value);
    }

    PRV_CPSS_GEN_PP_START_LOOP_PORT_GROUPS_IN_BMP_MAC(devNum,portGroupsBmp,portGroupId)
    {
        rc = prvCpssDrvHwPpPortGroupWriteRegBitMask(devNum,portGroupId,regAddr,mask,value);
        if(rc != GT_OK)
        {
            return rc;
        }
        if (isAdditionalRegDupNeeded == GT_TRUE)
        {
            rc = prvCpssDrvHwPpPortGroupWriteRegBitMask(devNum,portGroupId,additionalRegDup,mask,value);
            if(rc != GT_OK)
            {
                return rc;
            }
        }
    }
    PRV_CPSS_GEN_PP_END_LOOP_PORT_GROUPS_IN_BMP_MAC(devNum,portGroupsBmp,portGroupId)

    return rc;
}

/*******************************************************************************
* prvCpssHwPpPortGroupReadRam
*
* DESCRIPTION:
*       Read from PP's RAM. - generic (non-driver) API
*       in the specific port group in the device
*
* APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2 ; Lion; Lion2.
*        ExMxPm ; ExMxPm3.
*
* NOT APPLICABLE DEVICES:
*       None.
*
* INPUTS:
*       devNum      - The PP device number to read from.
*       portGroupId - The port group id. relevant only to 'multi-port-groups'
*                     devices. Supports value CPSS_PORT_GROUP_UNAWARE_MODE_CNS
*       addr        - Address offset to read from.
*       length      - Number of Words (4 byte) to read.
*
* OUTPUTS:
*       dataPtr     - (pointer to) An array containing the read data.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on hardware error
*       GT_NOT_INITIALIZED - if the driver was not initialized
*       GT_HW_ERROR - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvCpssHwPpPortGroupReadRam
(
    IN GT_U8    devNum,
    IN GT_U32   portGroupId,
    IN GT_U32   addr,
    IN GT_U32   length,
    OUT GT_U32  *dataPtr
)
{
    GT_STATUS       rc;
    GT_PORT_GROUPS_BMP  portGroupsBmp;/*port groups bmp */
    GT_BOOL isAdditionalRegDupNeeded; /* not used */
    GT_U32  additionalRegDup; /* not used */

    PRV_CPSS_DEV_CHECK_MAC(devNum);

    if(GT_FALSE ==
        prvCpssDuplicatedMultiPortGroupsGet(
            devNum,portGroupId,addr,&portGroupsBmp,
            &isAdditionalRegDupNeeded,&additionalRegDup))
    {
        return prvCpssDrvHwPpPortGroupReadRam(
            devNum,portGroupId,addr,length,dataPtr);
    }

#ifdef ALLOW_EMPTY_PORT_GROUPS
    if(portGroupsBmp == 0)
    {
        return GT_OK;
    }
#endif /*ALLOW_EMPTY_PORT_GROUPS*/
    /* get first port group in the BMP */
    PRV_CPSS_MULTI_PORT_GROUPS_BMP_GET_FIRST_ACTIVE_MAC(
        devNum,portGroupsBmp,portGroupId);

    rc = prvCpssDrvHwPpPortGroupReadRam(
        devNum,portGroupId,addr,length,dataPtr);

    return rc;
}

/*******************************************************************************
* prvCpssHwPpPortGroupWriteRam
*
* DESCRIPTION:
*       Writes to PP's RAM. - generic (non-driver) API
*       in the specific port group in the device
*
* APPLICABLE DEVICES:
*        DxCh1; DxCh1_Diamond; DxCh2; DxCh3; xCat; xCat3; xCat2 ; Lion; Lion2.
*        ExMxPm ; ExMxPm3.
*
* NOT APPLICABLE DEVICES:
*       None.
*
* INPUTS:
*       devNum      - The PP device number to write to.
*       portGroupId - The port group id. relevant only to 'multi-port-groups'
*                     devices. Supports value CPSS_PORT_GROUP_UNAWARE_MODE_CNS
*       addr        - Address offset to write to.
*       length      - Number of Words (4 byte) to write.
*       dataPtr     - (pointer to) An array containing the data to be written.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on hardware error
*       GT_NOT_INITIALIZED - if the driver was not initialized
*       GT_HW_ERROR - on hardware error
*       GT_NOT_APPLICABLE_DEVICE - on not applicable device
*
* COMMENTS:
*       None.
*
*******************************************************************************/
GT_STATUS prvCpssHwPpPortGroupWriteRam
(
    IN GT_U8    devNum,
    IN GT_U32   portGroupId,
    IN GT_U32   addr,
    IN GT_U32   length,
    IN GT_U32   *dataPtr
)
{
    GT_STATUS       rc = RETURN_VALUE_OF_MULTI_PORT_GROUPS_FAIL_CNS;
    GT_PORT_GROUPS_BMP  portGroupsBmp;/*port groups bmp */
    GT_BOOL isAdditionalRegDupNeeded;
    GT_U32  additionalRegDup;

    PRV_CPSS_DEV_CHECK_MAC(devNum);

    if(GT_FALSE ==
        prvCpssDuplicatedMultiPortGroupsGet(
            devNum,portGroupId,addr,&portGroupsBmp,
            &isAdditionalRegDupNeeded,&additionalRegDup))
    {
        return prvCpssDrvHwPpPortGroupWriteRam(
            devNum,portGroupId,addr,length,dataPtr);
    }

    PRV_CPSS_GEN_PP_START_LOOP_PORT_GROUPS_IN_BMP_MAC(
        devNum,portGroupsBmp,portGroupId)
    {
        rc = prvCpssDrvHwPpPortGroupWriteRam(
            devNum,portGroupId,addr,length,dataPtr);
        if(rc != GT_OK)
        {
            return rc;
        }
        if (isAdditionalRegDupNeeded == GT_TRUE)
        {
            rc = prvCpssDrvHwPpPortGroupWriteRam(
                devNum,portGroupId,additionalRegDup,length,dataPtr);
            if(rc != GT_OK)
            {
                return rc;
            }
        }
    }
    PRV_CPSS_GEN_PP_END_LOOP_PORT_GROUPS_IN_BMP_MAC(
        devNum,portGroupsBmp,portGroupId)

    return rc;
}



