/*уровень работы со свич контроллером для igmp*/

#include <stdio.h>
#include <string.h>
#include "stm32f4xx.h"
#include "stm32f4x7_eth.h"
#include "SMIApi.h"
#include "salsa2.h"
#include "Salsa2Regs.h"
#include "msApiDefs.h"
#include "msApiTypes.h"
#include "h/driver/gtHwCntl.h"
#include "h/driver/gtDrvSwRegs.h"
#include "board.h"
#include "settings.h"
#include "VLAN.h"
#include "igmp_mv.h"
#include "FlowCtrl.h"
#include "QoS.h"
#include "FreeRTOS.h"
#include "task.h"
#include "debug.h"

/* The following macro converts a boolean   */
/* value to a binary one (of 1 bit).        */
/* GT_FALSE --> 0                           */
/* GT_TRUE --> 1                            */
#define BOOL_2_BIT(boolVal,binVal)                                  \
            (binVal) = (((boolVal) == GT_TRUE) ? 1 : 0)

#define BIT_2_BOOL(binVal,boolVal)                                  \
            (boolVal) = (((binVal) == 0) ? GT_FALSE : GT_TRUE)



/*******************************************************************************
* gsysSetForceSnoopPri
*
* DESCRIPTION:
*        Force Snooping Priority. The priority on IGMP or MLD Snoop frames are
*        set to the SnoopPri value (gsysSetSnoopPri API) when Force Snooping
*       Priority is enabled. When it's disabled, the priority on these frames
*        is not modified.
*
* INPUTS:
*        en - GT_TRUE to use defined PRI bits, GT_FALSE otherwise.
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK   - on success
*        GT_FAIL - on error
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None.
*
*******************************************************************************/
GT_STATUS gsysSetForceSnoopPri
(
    IN GT_QD_DEV    *dev,
    IN GT_BOOL        en
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16            data;

    DEBUG_MSG(IGMP_DEBUG,"gsysSetForceSnoopPri Called.\r\n");

    BOOL_2_BIT(en,data);

    /* Set related bit */
    retVal = hwSetGlobal2RegField(QD_REG_PRIORITY_OVERRIDE, 7, 1, data);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(IGMP_DEBUG,"Failed.\r\n");
        return retVal;
    }

    DEBUG_MSG(IGMP_DEBUG,"OK.\r\n");
    return GT_OK;
}

/*******************************************************************************
* gsysGetForceSnoopPri
*
* DESCRIPTION:
*        Force Snooping Priority. The priority on IGMP or MLD Snoop frames are
*        set to the SnoopPri value (gsysSetSnoopPri API) when Force Snooping
*       Priority is enabled. When it's disabled, the priority on these frames
*        is not modified.
*
* INPUTS:
*        None.
*
* OUTPUTS:
*        en - GT_TRUE to use defined PRI bits, GT_FALSE otherwise.
*
* RETURNS:
*        GT_OK   - on success
*        GT_FAIL - on error
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None.
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gsysGetForceSnoopPri
(
    IN  GT_QD_DEV    *dev,
    OUT GT_BOOL      *en
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16          data;           /* The register's read data.    */
    DEBUG_MSG(IGMP_DEBUG,"gsysGetForceSnoopPri Called.\r\n");

    /* Check if Switch supports this feature. */
    if (/*!IS_IN_DEV_GROUP(dev,DEV_SNOOP_PRI)*/0)
    {
    	DEBUG_MSG(IGMP_DEBUG,"GT_NOT_SUPPORTED\r\n");
        return GT_NOT_SUPPORTED;
    }

    /* Get related bit */
    retVal = hwGetGlobal2RegField(QD_REG_PRIORITY_OVERRIDE,7,1,&data);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(IGMP_DEBUG,"Failed.\r\n");
        return retVal;
    }

    BIT_2_BOOL(data,*en);
    DEBUG_MSG(IGMP_DEBUG,"OK.\r\n");
    return GT_OK;
}

/*******************************************************************************
* gsysSetSnoopPri
*
* DESCRIPTION:
*        Snoop Priority. When ForceSnoopPri (gsysSetForceSnoopPri API) is enabled,
*       this priority is used as the egressing frame's PRI[2:0] bits on generated
*       Marvell Tag To_CPU Snoop frames and higher 2 bits of the priority are
*       used as the internal Queue Priority to use on IGMP/MLD snoop frames.
*
* INPUTS:
*        pri - PRI[2:0] bits (should be less than 8)
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK   - on success
*        GT_FAIL - on error
*        GT_BAD_PARAM - If pri is not less than 8.
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None.
*
*******************************************************************************/
GT_STATUS gsysSetSnoopPri
(
    IN GT_QD_DEV    *dev,
    IN GT_U16        pri
)
{
    GT_STATUS       retVal;         /* Functions return value.      */

    DEBUG_MSG(IGMP_DEBUG,"gsysSetSnoopPri Called.\r\n");

    /* Check if Switch supports this feature. */
    if (/*!IS_IN_DEV_GROUP(dev,DEV_SNOOP_PRI)*/0)
    {
    	DEBUG_MSG(IGMP_DEBUG,"GT_NOT_SUPPORTED\r\n");
        return GT_NOT_SUPPORTED;
    }

    if (pri > 0x7)
    {
    	DEBUG_MSG(IGMP_DEBUG,"GT_BAD_PARAM\r\n");
        return GT_BAD_PARAM;
    }

    /* Set related bit */
    retVal = hwSetGlobal2RegField(QD_REG_PRIORITY_OVERRIDE, 4, 3, pri);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(IGMP_DEBUG,"Failed.\r\n");
        return retVal;
    }

    DEBUG_MSG(IGMP_DEBUG,"OK.\r\n");
    return GT_OK;
}

/*******************************************************************************
* gsysGetSnoopPri
*
* DESCRIPTION:
*        Snoop Priority. When ForceSnoopPri (gsysSetForceSnoopPri API) is enabled,
*       this priority is used as the egressing frame's PRI[2:0] bits on generated
*       Marvell Tag To_CPU Snoop frames and higher 2 bits of the priority are
*       used as the internal Queue Priority to use on IGMP/MLD snoop frames.
*
* INPUTS:
*        None.
*
* OUTPUTS:
*        pri - PRI[2:0] bits (should be less than 8)
*
* RETURNS:
*        GT_OK   - on success
*        GT_FAIL - on error
*        GT_NOT_SUPPORTED - if current device does not support this feature.
*
* COMMENTS:
*        None.
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gsysGetSnoopPri
(
    IN  GT_QD_DEV    *dev,
    OUT GT_U16      *pri
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    DEBUG_MSG(IGMP_DEBUG,"gsysGetSnoopPri Called.\r\n");

    /* Check if Switch supports this feature. */
    if (/*!IS_IN_DEV_GROUP(dev,DEV_SNOOP_PRI)*/0)
    {
    	DEBUG_MSG(IGMP_DEBUG,"GT_NOT_SUPPORTED\r\n");
        return GT_NOT_SUPPORTED;
    }

    /* Get related bit */
    retVal = hwGetGlobal2RegField(QD_REG_PRIORITY_OVERRIDE,4,3,pri);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(IGMP_DEBUG,"Failed.\r\n");
        return retVal;
    }

    DEBUG_MSG(IGMP_DEBUG,"OK.\r\n");
    return GT_OK;
}


/* the following two APIs are added to support fullsail and clippership */

/*******************************************************************************
* gprtSetIGMPSnoop
*
* DESCRIPTION:
*         This routine set the IGMP Snoop. When set to one and this port receives
*        IGMP frame, the frame is switched to the CPU port, overriding all other
*        switching decisions, with exception for CPU's Trailer.
*        CPU port is determined by the Ingress Mode bits. A port is considered
*        the CPU port if its Ingress Mode are either GT_TRAILER_INGRESS or
*        GT_CPUPORT_INGRESS.
*
* INPUTS:
*        port - the logical port number.
*        mode - GT_TRUE for IGMP Snoop or GT_FALSE otherwise
*
* OUTPUTS:
*        None.
*
* RETURNS:
*        GT_OK   - on success
*        GT_FAIL - on error
*
* COMMENTS:
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gprtSetIGMPSnoop
(
    IN GT_LPORT     port,
    IN GT_BOOL      mode
)
{
    GT_U16          data;
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           hwPort;         /* the physical port number     */

    DEBUG_MSG(IGMP_DEBUG,"gprtSetIGMPSnoop Called.\r\n");

    /* translate LPORT to hardware port */
    hwPort = lport2port(port);

    /* check if device supports this feature */
    //if(retVal = IS_VALID_API_CALL(dev,hwPort, DEV_IGMP_SNOOPING)) != GT_OK)
    //  return retVal;

    /* translate BOOL to binary */
    BOOL_2_BIT(mode, data);

    /* Set the IGMP Snooping mode.            */
    retVal = hwSetPortRegField(hwPort, QD_REG_PORT_CONTROL,10,1, data);

    if(retVal != GT_OK)
    {
    	DEBUG_MSG(IGMP_DEBUG,"Failed.\r\n");
    }
    else
    {
    	DEBUG_MSG(IGMP_DEBUG,"OK.\r\n");
    }
    return retVal;
}



/*******************************************************************************
* gprtGetIGMPSnoop
*
* DESCRIPTION:
*       This routine get the IGMP Snoop mode.
*
* INPUTS:
*       port  - the logical port number.
*
* OUTPUTS:
*       mode - GT_TRUE: IGMP Snoop enabled
*           GT_FALSE otherwise
*
* RETURNS:
*       GT_OK   - on success
*       GT_FAIL - on error
*
* COMMENTS:
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gprtGetIGMPSnoop
(
    IN  GT_QD_DEV   *dev,
    IN  GT_LPORT    port,
    OUT GT_BOOL     *mode
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U8           hwPort;         /* the physical port number     */
    GT_U16          data;           /* to keep the read valve       */

    DEBUG_MSG(IGMP_DEBUG,"gprtGetIGMPSnoop Called.\r\n");

    /* translate LPORT to hardware port */
    hwPort = lport2port(port);

    /* check if device supports this feature */
    //if(retVal = IS_VALID_API_CALL(dev,hwPort, DEV_IGMP_SNOOPING)) != GT_OK)
    //  return retVal;

    /* Get the Ingress Mode.            */
    retVal = hwGetPortRegField(hwPort, QD_REG_PORT_CONTROL, 10, 1, &data);

    /* translate binary to BOOL  */
    BIT_2_BOOL(data, *mode);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(IGMP_DEBUG,"Failed.\r\n");
    }
    else
    {
    	DEBUG_MSG(IGMP_DEBUG,"OK.\r\n");
    }

    return retVal;
}

static GT_STATUS atuStateAppToDev
(
    //IN  GT_QD_DEV    *dev,
    IN  GT_BOOL        unicast,
    IN  GT_U32        state,
    OUT GT_U32        *newOne
)
{
    GT_U32    newState;
    GT_STATUS    retVal = GT_OK;

    if(unicast)
    {
        switch ((GT_ATU_UC_STATE)state)
        {
            case GT_UC_INVALID:
                newState = state;
                break;

            case GT_UC_DYNAMIC:
                if (/*IS_IN_DEV_GROUP(dev,DEV_UC_7_DYNAMIC)*/1)
                {
                    newState = 7;
                }
                else
                {
                    newState = 0xE;
                }
                break;

            case GT_UC_NO_PRI_TO_CPU_STATIC_NRL:
                if (/*IS_IN_DEV_GROUP(dev,DEV_UC_NO_PRI_TO_CPU_STATIC_NRL)*/0)
                {
                    newState = state;
                }
                else
                {
                    newState = (GT_U32)GT_UC_STATIC;
                    retVal = GT_BAD_PARAM;
                }
                break;

            case GT_UC_TO_CPU_STATIC_NRL:
                if (/*IS_IN_DEV_GROUP(dev,DEV_UC_TO_CPU_STATIC_NRL)*/0)
                {
                    newState = state;
                }
                else
                {
                    newState = (GT_U32)GT_UC_STATIC;
                    retVal = GT_BAD_PARAM;
                }
                break;

            case GT_UC_NO_PRI_STATIC_NRL:
                if (/*IS_IN_DEV_GROUP(dev,DEV_UC_NO_PRI_STATIC_NRL)*/0)
                {
                    newState = state;
                }
                else
                {
                    newState = (GT_U32)GT_UC_STATIC;
                    retVal = GT_BAD_PARAM;
                }
                break;

            case GT_UC_STATIC_NRL:
                if (/*IS_IN_DEV_GROUP(dev,DEV_UC_STATIC_NRL)*/0)
                {
                    newState = state;
                }
                else
                {
                    newState = (GT_U32)GT_UC_STATIC;
                    retVal = GT_BAD_PARAM;
                }
                break;

            case GT_UC_NO_PRI_TO_CPU_STATIC:
                if (/*IS_IN_DEV_GROUP(dev,DEV_UC_NO_PRI_TO_CPU_STATIC)*/0)
                {
                    newState = state;
                }
                else
                {
                    newState = (GT_U32)GT_UC_STATIC;
                    retVal = GT_BAD_PARAM;
                }
                break;

            case GT_UC_TO_CPU_STATIC:
                if (/*IS_IN_DEV_GROUP(dev,DEV_UC_TO_CPU_STATIC)*/0)
                {
                    newState = state;
                }
                else
                {
                    newState = (GT_U32)GT_UC_STATIC;
                    retVal = GT_BAD_PARAM;
                }
                break;

            case GT_UC_NO_PRI_STATIC:
                if (/*IS_IN_DEV_GROUP(dev,DEV_UC_NO_PRI_STATIC)*/0)
                {
                    newState = state;
                }
                else
                {
                    newState = (GT_U32)GT_UC_STATIC;
                    retVal = GT_BAD_PARAM;
                }
                break;

            case GT_UC_STATIC:
                if (/*IS_IN_DEV_GROUP(dev,DEV_UC_STATIC)*/1)
                {
                    newState = state;
                }
                else
                {
                    newState = (GT_U32)GT_UC_STATIC;
                    retVal = GT_BAD_PARAM;
                }
                break;

            default:
                if (/*IS_IN_DEV_GROUP(dev,DEV_UC_7_DYNAMIC)*/1)
                {
                    newState = 7;
                }
                else
                {
                    newState = 0xE;
                }
                retVal = GT_BAD_PARAM;
                break;

        }
    }
    else
    {
        switch (/*(GT_ATU_UC_STATE)*/state)
        {
            case GT_MC_INVALID:
                newState = state;
                break;

            case GT_MC_MGM_STATIC_UNLIMITED_RATE:
                if (/*IS_IN_DEV_GROUP(dev,DEV_MC_MGM_STATIC_UNLIMITED_RATE)*/1)
                {
                    newState = state;
                }
                else
                {
                    newState = (GT_U32)GT_MC_STATIC;
                    retVal = GT_BAD_PARAM;
                }
                break;

            case GT_MC_STATIC_UNLIMITED_RATE:
                if (/*IS_IN_DEV_GROUP(dev,DEV_MC_STATIC_UNLIMITED_RATE)*/1)
                {
                    newState = state;
                }
                else
                {
                    newState = (GT_U32)GT_MC_STATIC;
                    retVal = GT_BAD_PARAM;
                }
                break;

            case GT_MC_MGM_STATIC:
                if (/*IS_IN_DEV_GROUP(dev,DEV_MC_MGM_STATIC)*/0)
                {
                    newState = state;
                }
                else
                {
                    newState = (GT_U32)GT_MC_STATIC;
                    retVal = GT_BAD_PARAM;
                }
                break;

            case GT_MC_STATIC:
                if (/*IS_IN_DEV_GROUP(dev,DEV_MC_STATIC)*/1)
                {
                    newState = state;
                }
                else
                {
                    newState = (GT_U32)GT_MC_STATIC;
                    retVal = GT_BAD_PARAM;
                }
                break;

            case GT_MC_PRIO_MGM_STATIC_UNLIMITED_RATE:
                if (/*IS_IN_DEV_GROUP(dev,DEV_MC_PRIO_MGM_STATIC_UNLIMITED_RATE)*/0)
                {
                    newState = state;
                }
                else
                {
                    newState = (GT_U32)GT_MC_STATIC;
                    retVal = GT_BAD_PARAM;
                }
                break;

            case GT_MC_PRIO_STATIC_UNLIMITED_RATE:
                if (/*IS_IN_DEV_GROUP(dev,DEV_MC_PRIO_STATIC_UNLIMITED_RATE)*/1)
                {
                    newState = state;
                }
                else
                {
                    newState = (GT_U32)GT_MC_STATIC;
                    retVal = GT_BAD_PARAM;
                }
                break;

            case GT_MC_PRIO_MGM_STATIC:
                if (/*IS_IN_DEV_GROUP(dev,DEV_MC_PRIO_MGM_STATIC)*/1)
                {
                    newState = state;
                }
                else
                {
                    newState = (GT_U32)GT_MC_STATIC;
                    retVal = GT_BAD_PARAM;
                }
                break;

            case GT_MC_PRIO_STATIC:
                if (/*IS_IN_DEV_GROUP(dev,DEV_MC_PRIO_STATIC)*/1)
                {
                    newState = state;
                }
                else
                {
                    newState = (GT_U32)GT_MC_STATIC;
                    retVal = GT_BAD_PARAM;
                }
                break;

            default:
                newState = (GT_U32)GT_MC_STATIC;
                retVal = GT_BAD_PARAM;
                break;

        }
    }

    *newOne = newState;
    return retVal;
}


/*******************************************************************************
* atuOperationPerform
*
* DESCRIPTION:
*       This function is used by all ATU control functions, and is responsible
*       to write the required operation into the ATU registers.
*
* INPUTS:
*       atuOp       - The ATU operation bits to be written into the ATU
*                     operation register.
*       DBNum       - ATU Database Number for CPU accesses
*       entryPri    - The EntryPri field in the ATU Data register.
*       portVec     - The portVec field in the ATU Data register.
*       entryState  - The EntryState field in the ATU Data register.
*       atuMac      - The Mac address to be written to the ATU Mac registers.
*
* OUTPUTS:
*       entryPri    - The EntryPri field in case the atuOp is GetNext.
*       portVec     - The portVec field in case the atuOp is GetNext.
*       entryState  - The EntryState field in case the atuOp is GetNext.
*       atuMac      - The returned Mac address in case the atuOp is GetNext.
*
* RETURNS:
*       GT_OK on success,
*       GT_FAIL otherwise.
*
* COMMENTS:
*       1.  if atuMac == NULL, nothing needs to be written to ATU Mac registers.
*
*******************************************************************************/
GT_STATUS atuOperationPerform
(
    GT_ATU_OPERATION    atuOp,
    GT_EXTRA_OP_DATA    *opData,
    GT_ATU_ENTRY        *entry
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16          data;           /* Data to be set into the      */
                                    /* register.                    */
    GT_U16          opcodeData;           /* Data to be set into the      */
                                    /* register.                    */
    GT_U8           i;
    GT_U16            portMask;

    //gtSemTake(dev,dev->atuRegsSem,OS_WAIT_FOREVER);

    portMask = (1 << MV_PORT_NUM) - 1;

    /* Wait until the ATU in ready. */
#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 1;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_ATU_OPERATION;
      regAccess.rw_reg_list[0].data = 15;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        gtSemGive(dev,dev->atuRegsSem);
        return retVal;
      }
    }
#else
    data = 1;
    while(data == 1)
    {
        retVal = hwGetGlobalRegField(QD_REG_ATU_OPERATION,15,1,&data);
        if(retVal != GT_OK)
        {
               return retVal;
        }
    }
#endif

    opcodeData = 0;

    switch (atuOp)
    {
        case LOAD_PURGE_ENTRY:

            DEBUG_MSG(DEBUG_QD,"LOAD_PURGE_ENTRY entry->trunkMember %d\r\n",entry->trunkMember);


				if (1 && entry->trunkMember)
				{
					/* portVec represents trunk ID */
					data = (GT_U16)( 0x8000 | (((entry->portVec) & 0xF) << 4) |
						 (((entry->entryState.ucEntryState) & 0xF)) );
				}
				else
				{
					data = (GT_U16)( (((entry->portVec) & portMask) << 4) |
						 (((entry->entryState.ucEntryState) & 0xF)) );
				}
				opcodeData |= (entry->prio & 0x7) << 8;


                retVal = hwWriteGlobalReg(QD_REG_ATU_DATA_REG,data);
                if(retVal != GT_OK)
                {
                    return retVal;
                }
                /* pass thru */

        case GET_NEXT_ENTRY:
#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 3;

      for(i = 0; i < 3; i++)
      {
        data=(entry->macAddr.arEther[2*i] << 8)|(entry->macAddr.arEther[1 + 2*i]);
        regAccess.rw_reg_list[i].cmd = HW_REG_WRITE;
        regAccess.rw_reg_list[i].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
        regAccess.rw_reg_list[i].reg = QD_REG_ATU_MAC_BASE+i;
        regAccess.rw_reg_list[i].data = data;
      }
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        gtSemGive(dev,dev->atuRegsSem);
        return retVal;
      }
    }
#else
                for(i = 0; i < 3; i++)
                {
                    data=(entry->macAddr.arEther[2*i] << 8)|(entry->macAddr.arEther[1 + 2*i]);
                    retVal = hwWriteGlobalReg((GT_U8)(QD_REG_ATU_MAC_BASE+i),data);
                    if(retVal != GT_OK)
                    {
                       return retVal;
                    }
                }
#endif
                break;

        case FLUSH_ALL:
        case FLUSH_UNLOCKED:
        case FLUSH_ALL_IN_DB:
        case FLUSH_UNLOCKED_IN_DB:
                if (entry->entryState.ucEntryState == 0xF)
                {
                    data = (GT_U16)(0xF | ((opData->moveFrom & 0xF) << 4) | ((opData->moveTo & 0xF) << 8));
                }
                else
                {
                    data = 0;
                }
                retVal = hwWriteGlobalReg(QD_REG_ATU_DATA_REG,data);
                   if(retVal != GT_OK)
                {
                       //gtSemGive(dev,dev->atuRegsSem);
                    return retVal;
                   }
                break;

        case SERVICE_VIOLATIONS:

                break;

        default :
                return GT_FAIL;
    }



    /* Set DBNum */

    //if(IS_IN_DEV_GROUP(dev,DEV_FID_REG))
    if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240))
    {
        retVal = hwSetGlobalRegField(QD_REG_ATU_FID_REG,0,12,(GT_U16)(entry->DBNum & 0xFFF));
        if(retVal != GT_OK)
        {
           return retVal;
        }
    }

    if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){
    	retVal = hwSetGlobalRegField(QD_REG_ATU_CONTROL,12,4,(GT_U16)((entry->DBNum & 0xF0) >> 4));
		if(retVal != GT_OK)
		{
			//gtSemGive(dev,dev->atuRegsSem);
			return retVal;
		}
    }


    /* Set the ATU Operation register in addtion to DBNum setup  */
    //if(IS_IN_DEV_GROUP(dev,DEV_FID_REG))
    if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240))
    	opcodeData |= ((1 << 15) | (atuOp << 12));
    if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097))
    	opcodeData |= ((1 << 15) | (atuOp << 12) | (entry->DBNum & 0xF));


    retVal = hwWriteGlobalReg(QD_REG_ATU_OPERATION,opcodeData);
    if(retVal != GT_OK)
    {
        //gtSemGive(dev,dev->atuRegsSem);
        return retVal;
    }

    /* If the operation is to service violation operation wait for the response   */
    if(atuOp == SERVICE_VIOLATIONS)
    {
        /* Wait until the VTU in ready. */
#ifdef GT_RMGMT_ACCESS
        {
          HW_DEV_REG_ACCESS regAccess;

          regAccess.entries = 1;

          regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
          regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
          regAccess.rw_reg_list[0].reg = QD_REG_ATU_OPERATION;
          regAccess.rw_reg_list[0].data = 15;
          retVal = hwAccessMultiRegs(dev, &regAccess);
          if(retVal != GT_OK)
          {
            gtSemGive(dev,dev->atuRegsSem);
          return retVal;
          }
        }
#else
        data = 1;
        while(data == 1)
        {
            retVal = hwGetGlobalRegField(QD_REG_ATU_OPERATION,15,1,&data);
            if(retVal != GT_OK)
            {
                //gtSemGive(dev,dev->atuRegsSem);
                return retVal;
            }
        }
#endif

        /* get the Interrupt Cause */
        retVal = hwGetGlobalRegField(QD_REG_ATU_OPERATION,4,4,&data);
        if(retVal != GT_OK)
        {
            //gtSemGive(dev,dev->atuRegsSem);
            return retVal;
        }
        /*if (!IS_IN_DEV_GROUP(dev,DEV_AGE_OUT_INT))*/
        if(!((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)))
        {
            data &= 0x7;    /* only 3 bits are valid for non age_out_int group */
        }

        switch (data)
        {
            case 8:    /* Age Interrupt */
                opData->intCause = GT_AGE_VIOLATION;
                break;
            case 4:    /* Member Violation */
                opData->intCause = GT_MEMBER_VIOLATION;
                break;
            case 2:    /* Miss Violation */
                opData->intCause = GT_MISS_VIOLATION;
                break;
            case 1:    /* Full Violation */
                opData->intCause = GT_FULL_VIOLATION;
                break;
            default:
                opData->intCause = 0;
                //gtSemGive(dev,dev->atuRegsSem);
                return GT_OK;
        }

        /* get the DBNum that was involved in the violation */

        entry->DBNum = 0;

        if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240))
		//if(IS_IN_DEV_GROUP(dev,DEV_FID_REG))
		{
			retVal = hwGetGlobalRegField(QD_REG_ATU_FID_REG,0,12,&data);
			if(retVal != GT_OK)
			{
				//gtSemGive(dev,dev->atuRegsSem);
				return retVal;
			}
			entry->DBNum = (GT_U16)data;
		}


        if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097))
        //if (IS_IN_DEV_GROUP(dev,DEV_DBNUM_256))
        {
            retVal = hwGetGlobalRegField(QD_REG_ATU_CONTROL,12,4,&data);
            if(retVal != GT_OK)
            {
                //gtSemGive(dev,dev->atuRegsSem);
                return retVal;
            }
            entry->DBNum = (GT_U16)data << 4;
        }


        //if(!IS_IN_DEV_GROUP(dev,DEV_FID_REG))
        if(!((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)))
        {
            retVal = hwGetGlobalRegField(QD_REG_ATU_OPERATION,0,4,&data);
            if(retVal != GT_OK)
            {
                //gtSemGive(dev,dev->atuRegsSem);
                return retVal;
            }

            entry->DBNum |= (GT_U8)(data & 0xF);
        }

        /* get the Source Port ID that was involved in the violation */

        retVal = hwReadGlobalReg(QD_REG_ATU_DATA_REG,&data);
        if(retVal != GT_OK)
        {
            //gtSemGive(dev,dev->atuRegsSem);
            return retVal;
        }

        entry->entryState.ucEntryState = data & 0xF;

        /* Get the Mac address  */
        for(i = 0; i < 3; i++)
        {
            retVal = hwReadGlobalReg((GT_U8)(QD_REG_ATU_MAC_BASE+i),&data);
            if(retVal != GT_OK)
            {
                //gtSemGive(dev,dev->atuRegsSem);
                return retVal;
            }
            entry->macAddr.arEther[2*i] = data >> 8;
            entry->macAddr.arEther[1 + 2*i] = data & 0xFF;
        }



    } /* end of service violations */
    /* If the operation is a gen next operation wait for the response   */
    if(atuOp == GET_NEXT_ENTRY)
    {
        entry->trunkMember = GT_FALSE;
        entry->exPrio.useMacFPri = GT_FALSE;
        entry->exPrio.macFPri = 0;
        entry->exPrio.macQPri = 0;

        /* Wait until the ATU in ready. */
        data = 1;
        while(data == 1)
        {
            retVal = hwGetGlobalRegField(QD_REG_ATU_OPERATION,15,1,&data);
            if(retVal != GT_OK)
            {
                //gtSemGive(dev,dev->atuRegsSem);
                return retVal;
            }
        }

        /* Get the Mac address  */
        for(i = 0; i < 3; i++)
        {
            retVal = hwReadGlobalReg((GT_U8)(QD_REG_ATU_MAC_BASE+i),&data);
            if(retVal != GT_OK)
            {
                //gtSemGive(dev,dev->atuRegsSem);
                return retVal;
            }
            entry->macAddr.arEther[2*i] = data >> 8;
            entry->macAddr.arEther[1 + 2*i] = data & 0xFF;
        }

        retVal = hwReadGlobalReg(QD_REG_ATU_DATA_REG,&data);
        if(retVal != GT_OK)
        {
            //gtSemGive(dev,dev->atuRegsSem);
            return retVal;
        }


        /* Get the Atu data register fields */
        if (/*(IS_IN_DEV_GROUP(dev,DEV_88E6093_FAMILY) &&
            (!((IS_IN_DEV_GROUP(dev,DEV_88EC000_FAMILY))||
               (IS_IN_DEV_GROUP(dev,DEV_88ESPANNAK_FAMILY))))) ||
            IS_IN_DEV_GROUP(dev,DEV_TRUNK)*/1)
        {
            if (/*IS_IN_DEV_GROUP(dev,DEV_TRUNK)*/1)
            {
                entry->trunkMember = (data & 0x8000)?GT_TRUE:GT_FALSE;
            }

            entry->portVec = (data >> 4) & portMask;
            entry->entryState.ucEntryState = data & 0xF;
            retVal = hwGetGlobalRegField(QD_REG_ATU_OPERATION,8,3,&data);
            if(retVal != GT_OK)
            {
                //gtSemGive(dev,dev->atuRegsSem);
                return retVal;
            }
            entry->prio = (GT_U8)data;
        }
    }

    //gtSemGive(dev,dev->atuRegsSem);
    return GT_OK;
}

/*******************************************************************************
* gfdbAddMacEntry
*
* DESCRIPTION:
*       Creates the new entry in MAC address table.
*
* INPUTS:
*       macEntry    - mac address entry to insert to the ATU.
*
* OUTPUTS:
*       None
*
* RETURNS:
*       GT_OK          - on success
*       GT_FAIL        - on error
*       GT_BAD_PARAM   - on invalid port vector
*
* COMMENTS:
*        DBNum in atuEntry -
*            ATU MAC Address Database number. If multiple address
*            databases are not being used, DBNum should be zero.
*            If multiple address databases are being used, this value
*            should be set to the desired address database number.
*
* GalTis:
*
*******************************************************************************/
GT_STATUS gfdbAddMacEntry
(
    IN GT_ATU_ENTRY *macEntry
)
{
    GT_STATUS       retVal;
    GT_ATU_ENTRY    entry;
    GT_U32 data;


    DEBUG_MSG(DEBUG_QD,"gfdbAddMacEntry Called. \r\n");

    /*gtMemCpy*/
    memcpy(entry.macAddr.arEther,macEntry->macAddr.arEther,6);
    entry.DBNum        = macEntry->DBNum;
    entry.portVec     = GT_LPORTVEC_2_PORTVEC(macEntry->portVec);
    if(entry.portVec == GT_INVALID_PORT_VEC)
    {
        return GT_BAD_PARAM;
    }


	entry.exPrio.useMacFPri = 0;
	entry.exPrio.macFPri = 0;
	entry.exPrio.macQPri = 0;
	entry.prio        = macEntry->prio;


    entry.trunkMember = macEntry->trunkMember;

    DEBUG_MSG(DEBUG_QD,"entry.macAddr %x:%x:%x:%x:%x:%x\r\n",
        		entry.macAddr.arEther[0],entry.macAddr.arEther[1],entry.macAddr.arEther[2],entry.macAddr.arEther[3],entry.macAddr.arEther[4],entry.macAddr.arEther[5]);


    DEBUG_MSG(DEBUG_QD,"entry %d\r\n",entry.entryState.ucEntryState);
    DEBUG_MSG(DEBUG_QD,"macEntry %d %d\r\n",macEntry->entryState.ucEntryState,macEntry->entryState.mcEntryState);



    if(IS_MULTICAST_MAC(entry.macAddr))
    {
    	DEBUG_MSG(DEBUG_QD,"is multicast addr\r\n");
    	atuStateAppToDev(GT_FALSE,(GT_U32)macEntry->entryState.mcEntryState,&data);
        entry.entryState.ucEntryState = data;
    }
    else
    {
    	DEBUG_MSG(DEBUG_QD,"is unicast addr\r\n");
        atuStateAppToDev(GT_TRUE,(GT_U32)macEntry->entryState.ucEntryState,&data);
        entry.entryState.ucEntryState = data;
    }

    if (entry.entryState.ucEntryState == 0)
    {
    	DEBUG_MSG(DEBUG_QD,"Entry State should not be ZERO.\r\n");
        return GT_BAD_PARAM;
    }

    retVal = atuOperationPerform(LOAD_PURGE_ENTRY,NULL,&entry);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(DEBUG_QD,"Failed.\r\n");
        return retVal;
    }

    DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    return GT_OK;
}

/*******************************************************************************
* gfdbDelMacEntry
*
* DESCRIPTION:
*       Deletes MAC address entry. If DBNum or FID is used, gfdbDelAtuEntry API
*        would be the better choice to delete an entry in ATU.
*
* INPUTS:
*       macAddress - mac address.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK           - on success
*       GT_FAIL         - on error
*       GT_NO_RESOURCE  - failed to allocate a t2c struct
*       GT_NO_SUCH      - if specified address entry does not exist
*
* COMMENTS:
*
*******************************************************************************/
GT_STATUS gfdbDelMacEntry
(
    IN GT_ETHERADDR  *macAddress
)
{
    GT_STATUS retVal;
    GT_ATU_ENTRY    entry;

    DEBUG_MSG(DEBUG_QD,"gfdbDelMacEntry Called.\r\n");
    /* check if device supports this feature */
    if (/*!IS_IN_DEV_GROUP(dev,DEV_STATIC_ADDR)*/0)
    {
    	DEBUG_MSG(DEBUG_QD,"GT_NOT_SUPPORTED\r\n");
        return GT_NOT_SUPPORTED;
    }

    //gtMemCpy
    memcpy(entry.macAddr.arEther,macAddress->arEther,6);
    entry.DBNum = 0;
    entry.prio = 0;
    entry.portVec = 0;
    entry.entryState.ucEntryState = 0;
    entry.trunkMember = GT_FALSE;
    entry.exPrio.useMacFPri = GT_FALSE;
    entry.exPrio.macFPri = 0;
    entry.exPrio.macQPri = 0;

    retVal = atuOperationPerform(LOAD_PURGE_ENTRY,NULL,&entry);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(DEBUG_QD,"Failed.\r\n");
        return retVal;
    }
    DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    return GT_OK;
}





/*
 *    Add a multicast MAC address into the QuaterDeck MAC Address database,
 *    where address is 01-00-18-1a-00-00 and frames with this destination has
 *    to be forwarding to Port 1, Port 2 and Port 4 (port starts from Port 0)
 *    Input - None
*/
GT_STATUS sampleAddMulticastAddr(u8 *mac, u8 *ports, u8 add_cpu)
{
    GT_STATUS status;
    GT_ATU_ENTRY macEntry;

    for(u8 i=0;i<6;i++){
    	macEntry.macAddr.arEther[i] = mac[i];
    }

    //if exist
    macEntry.portVec = get_atu_port_vect(mac);


    for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
    	if(ports[i] == 1){
    		macEntry.portVec |= 1<<L2F_port_conv(i);
		}
    }


    // and add cpu port
    if(add_cpu)
    	macEntry.portVec |=  (1<<CPU_PORT);



    macEntry.DBNum = 0;


    macEntry.prio = 0;            /* Priority (2bits). When these bits are used they override
                                any other priority determined by the frame's data. This value is
                                meaningful only if the device does not support extended priority
                                information such as MAC Queue Priority and MAC Frame Priority */

    macEntry.exPrio.macQPri = 0;    /* If device doesnot support MAC Queue Priority override,
                                    this field is ignored. */
    macEntry.exPrio.macFPri = 0;    /* If device doesnot support MAC Frame Priority override,
                                    this field is ignored. */
    macEntry.exPrio.useMacFPri = 0;    /* If device doesnot support MAC Frame Priority override,
                                    this field is ignored. */

    macEntry.trunkMember = 0;

    macEntry.entryState.ucEntryState = GT_MC_STATIC;
    macEntry.entryState.mcEntryState = GT_MC_STATIC;

                                /* This address is locked and will not be aged out.
                                Refer to GT_ATU_MC_STATE in msApiDefs.h for other option.*/

    DEBUG_MSG(DEBUG_QD,"sampleAddMulticastAddr: mac %x:%x:%x:%x:%x:%x  port vect %lu\r\n",
    		mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],macEntry.portVec);

    DEBUG_MSG(DEBUG_QD,"hello \r\n");

    /*
     *    Add the MAC Address.
     */

    status = gfdbAddMacEntry(&macEntry);
    if(status != GT_OK)
    {
    	DEBUG_MSG(DEBUG_QD,"gfdbAddMacEntry returned fail.\r\n");
        return status;
    }

    return GT_OK;
}



#if 0
/*
 *    Delete the Multicast MAC address of 01-00-18-1a-00-00.
 *    Input - None
*/
GT_STATUS sampleDelMulticastAddr(u8 *mac,u8 *ports)
{
    GT_STATUS status;
    GT_ATU_ENTRY macEntry;

    /*
     *    Assume that Ethernet address for the CPU MAC is
     *    01-50-43-00-01-02.
    */
    /*
    macEntry.macAddr.arEther[0] = 0x01;
    macEntry.macAddr.arEther[1] = 0x50;
    macEntry.macAddr.arEther[2] = 0x43;
    macEntry.macAddr.arEther[3] = 0x00;
    macEntry.macAddr.arEther[4] = 0x01;
    macEntry.macAddr.arEther[5] = 0x02;*/

    for(u8 i=0;i<6;i++){
    	macEntry.macAddr.arEther[i] = mac[i];
    }

    DBG_INFO("sampleDelMulticastAddr: mac %x:%x:%x:%x:%x:%x\r\n",
    		mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

    /*
     *    Delete the given Multicast Address.
     */
    if((status = gfdbDelMacEntry(&macEntry.macAddr)) != GT_OK)
    {
        MSG_PRINT("gfdbDelMacEntry returned fail.\r\n");
        return status;
    }

    return GT_OK;
}

#endif

GT_STATUS sampleDelMulticastAddr(u8 *mac, u8 *ports,u8 del_cpu)
{
    GT_STATUS status;
    GT_ATU_ENTRY macEntry;

    for(u8 i=0;i<6;i++){
    	macEntry.macAddr.arEther[i] = mac[i];
    }

    //if exist
    // get exist port vect
    macEntry.portVec = get_atu_port_vect(mac);;

    for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
    	if(ports[i] == 1){
    		macEntry.portVec &= ~(1<<L2F_port_conv(i));
		}
    }

    if(del_cpu)
    // and add cpu port
    	macEntry.portVec &= ~(1<<CPU_PORT);

    //if only CPU port, delete entry
    if(macEntry.portVec == (1<<CPU_PORT)){
        for(u8 i=0;i<6;i++){
        	macEntry.macAddr.arEther[i] = mac[i];
        }
        DEBUG_MSG(DEBUG_QD,"sampleDelMulticastAddr: mac %x:%x:%x:%x:%x:%x\r\n",
        		mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
        /*
         *    Delete the given Multicast Address.
         */
        if((status = gfdbDelMacEntry(&macEntry.macAddr)) != GT_OK)
        {
        	DEBUG_MSG(DEBUG_QD,"gfdbDelMacEntry returned fail.\r\n");
            return status;
        }
        return GT_OK;
    }

    //else modify current

    macEntry.DBNum = 0;


    macEntry.prio = 0;            /* Priority (2bits). When these bits are used they override
                                any other priority determined by the frame's data. This value is
                                meaningful only if the device does not support extended priority
                                information such as MAC Queue Priority and MAC Frame Priority */

    macEntry.exPrio.macQPri = 0;    /* If device doesnot support MAC Queue Priority override,
                                    this field is ignored. */
    macEntry.exPrio.macFPri = 0;    /* If device doesnot support MAC Frame Priority override,
                                    this field is ignored. */
    macEntry.exPrio.useMacFPri = 0;    /* If device doesnot support MAC Frame Priority override,
                                    this field is ignored. */

    macEntry.trunkMember = 0;

    macEntry.entryState.ucEntryState = GT_MC_STATIC;
    macEntry.entryState.mcEntryState = GT_MC_STATIC;

                                /* This address is locked and will not be aged out.
                                Refer to GT_ATU_MC_STATE in msApiDefs.h for other option.*/

    DEBUG_MSG(DEBUG_QD,"sampleAddMulticastAddr: mac %x:%x:%x:%x:%x:%x  port vect %lu\r\n",
    		mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],macEntry.portVec);

    /*
     *    Add the MAC Address.
     */

    if(macEntry.portVec == 0){
        if((status = gfdbDelMacEntry(&macEntry.macAddr)) != GT_OK)
        {
        	DEBUG_MSG(DEBUG_QD,"gfdbDelMacEntry returned fail.\r\n");
            return status;
        }
        return GT_OK;
    }

    status = gfdbAddMacEntry(&macEntry);
    if(status != GT_OK)
    {
    	DEBUG_MSG(DEBUG_QD,"gfdbAddMacEntry returned fail.\r\n");
        return status;
    }

    return GT_OK;
}

/*добавляем broadcast MAC в ATU таблицу, т.к. он тоже считается мультикастом*/
void add_broadcast_to_atu(void){
	u8 mac[6];
	u8 ports[PORT_NUM];
	for(u8 i=0;i<6;i++)
		mac[i] = 0xFF;
	for(u8 i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
		ports[i] = 1;
	}

	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
		(get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
		sampleAddMulticastAddr(mac,ports,1);
	}
	else{
		//Trap Unknow Multicast to CPU
		//Salsa2_WriteRegField(EGRESS_BRIDGING_REG,3,2,2);
	}


}
