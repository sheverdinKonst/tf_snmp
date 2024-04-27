/* самодельные упрощенные Api функции для работы с Marvel 88E6095*/

//#include <eth_LWIP.h>
//#include "stm32_eth.h"
//#include "flash/spiflash.h"
#include <string.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"
#include "board.h"
#include "settings.h"
#include "SMIApi.h"
#include "../igmp/igmp_mv.h"
#include "h/driver/gtDrvSwRegs.h"
#include "VLAN.h"
#include "FlowCtrl.h"
#include "SpeedDuplex.h"
#include "QoS.h"
#include "AdvVCT.h"

#include "h/driver/gtDrvSwRegs.h"
#include "h/driver/gtHwCntl.h"
#include "gtPortRateCtrl.h"

#include "stm32f4x7_eth.h"
#include "stm32f4xx_rcc.h"
#include "inc/Salsa2Regs.h"

#include "stm32f4xx_gpio.h"
#include "../stp/bridgestp.h"
#include "salsa2.h"
#include "../igmp/igmp_mv.h"
#include "../events/events_handler.h"
//#include "../stp/stp_oslayer.h"
#include "../uip/timer.h"
#include "debug.h"

#define PAGE_ADDR_REG	22

//extern struct status_t status;

static u8  net_port_sett_change_t=0;


u8 port_statistics_start=0;

//struct mac_entry_t mac_entry[MAX_CPU_ENTRIES_NUM] __attribute__ ((section (".ccmram")));

u8 net_port_sett_change(void){
	if(net_port_sett_change_t == 1)
		return 1;
	else
		return 0;
}

void net_port_sett_change_set(u8 state){
	net_port_sett_change_t = state;
}

uint8_t ETH_InitSMI(void){
	  RCC_ClocksTypeDef  rcc_clocks;
	  uint32_t tmpreg = 0;     //RegValue = 0,
	  uint32_t hclk = 60000000;

tmpreg = ETH->MACMIIAR;
  /* Clear CSR Clock Range CR[2:0] bits */
  tmpreg &= MACMIIAR_CR_MASK;
  /* Get hclk frequency value */
  RCC_GetClocksFreq(&rcc_clocks);
  hclk = rcc_clocks.HCLK_Frequency;
  /* Set CR bits depending on hclk value */
  if((hclk >= 20000000)&&(hclk < 35000000))
  {
    /* CSR Clock Range between 20-35 MHz */
    tmpreg |= (uint32_t)ETH_MACMIIAR_CR_Div16;
  }
  else if((hclk >= 35000000)&&(hclk < 60000000))
  {
    /* CSR Clock Range between 35-60 MHz */
    tmpreg |= (uint32_t)ETH_MACMIIAR_CR_Div26;
  }
  else /* ((hclk >= 60000000)&&(hclk <= 72000000)) */
  {
    /* CSR Clock Range between 60-72 MHz */
    tmpreg |= (uint32_t)ETH_MACMIIAR_CR_Div42;
  }
  /* Write to ETHERNET MAC MIIAR: Configure the ETHERNET CSR Clock Range */
  ETH->MACMIIAR = (uint32_t)tmpreg;

  return 1;
}

void SmiInit(void){
	ETH_InitSMI();
}

void smi_allport_discard(void){
  uint16_t tmp;
  if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
		  (get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
	for(int i=0;i<MV_PORT_NUM;i++){
		tmp = ETH_ReadPHYRegister(0x10+i,0x04);
		tmp&=~0x03;
		ETH_WritePHYRegister(0x10+i, 0x04, tmp);
	}
  }
  else if(get_marvell_id() == DEV_98DX316){
	  for(int i=0;i<ALL_PORT_NUM;i++){
		  stp_set_port_state(i,BSTP_IFSTATE_DISCARDING);
	  }
  }
}

uint8_t marvell_freeze_control(void){
uint16_t tmp;
//u8 ret=0;
#ifndef PPU_DIS
tmp = ETH_ReadPHYRegister(GlobalRegisters,0x00);
if(tmp & 0x4000)
	return 1;
else
	return 0;
#else
	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
		(get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
		/*зависание свича определяем по состоянию bloking на порту процессора*/
		tmp = ETH_ReadPHYRegister(0x10+CPU_PORT,0x04);
		if((tmp & 0x03)!=0x3){
			return 1;
		}else{
			return 0;
		}
	}
	//принцип работы на Salsa2:
	//если сработал WatchDog timer на CPU порту и других портах, перезагружаем коммутатор
	else if(get_marvell_id()==DEV_98DX316){
		if(Salsa2_ReadRegField(0x01800080,1,15)){
			return 1;
		}
		for(u8 i=0;i<ALL_PORT_NUM;i++){
			Salsa2_WritePhyReg(i,Salsa2_get_phyAddr(i),22,4);
			if(Salsa2_ReadPhyReg(i,Salsa2_get_phyAddr(i),19) & 0x100)
				return 1;
		}
		return 0;

	}
	return 0;
#endif
}


void smi_flush_db(int dbnum){
u16 tmp;
  tmp = ETH_ReadPHYRegister(0x1b, 0x0a);
  tmp &= ~0xF000;
  ETH_WritePHYRegister(0x1b, 0x0a, tmp);
  ETH_WritePHYRegister(0x1b, 0x0c, 0);
  ETH_WritePHYRegister(0x1b, 0x0b, (1<<15)|(6<<12)|dbnum);
}

void smi_flush_all(void){
  DEBUG_MSG(STP_DEBUG,"smi_flush_all\r\n");
  if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
	(get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
	  ETH_WritePHYRegister(0x1b, 0x0c, 0);
	  ETH_WritePHYRegister(0x1b, 0x0b, (1<<15)|(2<<12));
  }
  else if(get_marvell_id() == DEV_98DX316){
	  for(u8 i=0;i<ALL_PORT_NUM;i++){
		  Salsa2_WriteRegField(PORT0_TRANSMIT_CFG_REG+0x1000*L2F_port_conv(i),12,1,1);
	  }
	  Salsa2_WriteRegField(PORT0_TRANSMIT_CFG_REG+0x1F000,12,1,1);
	  vTaskDelay(10);
	  for(u8 i=0;i<ALL_PORT_NUM;i++){
		  Salsa2_WriteRegField(PORT0_TRANSMIT_CFG_REG+0x1000*L2F_port_conv(i),12,0,1);
	  }
	  Salsa2_WriteRegField(PORT0_TRANSMIT_CFG_REG+0x1F000,12,0,1);


	  //алгоритм выше не работает
	  /*for(u16 i=0;i<4096;i++){
		  //set valid = 0
		  Salsa2_WriteRegField(MAC_TABLE_ENTRY_W0+0x10*i,0,0,1);
	 }*/
  }
}

void smi_set_port_dbnum_low(int port, int num){
uint16_t tmp;
	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
		(get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
 		tmp = ETH_ReadPHYRegister(0x10+port, 0x06);
		tmp&=~(0xf<<12);
		tmp|=((num&0x0f)<<12);
		ETH_WritePHYRegister(0x10+port, 0x06, tmp);
	}
}


/*read atu value
 * num - DBNum
 * start - set 1 if first call, 0 - for other
 * mac,port - returned mac and port*/
//for 6095
int read_atu(int num, int start, struct mac_entry_t *entry){
int end;
uint16_t *buf;
uint16_t tmp;
u32 temp;
uint8_t addr;
static u32 cnt;

if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){
	if (start){
		tmp = ETH_ReadPHYRegister(0x1b, 0x0a);
		tmp &= ~0xF000;
		ETH_WritePHYRegister(0x1b, 0x0a, tmp);
		ETH_WritePHYRegister(0x1b, 0x0d, 0xffff);
	    ETH_WritePHYRegister(0x1b, 0x0e, 0xffff);
		ETH_WritePHYRegister(0x1b, 0x0f, 0xffff);
	}

	//get next
	ETH_WritePHYRegister(0x1b, 0x0b, (1<<15)|(4<<12)|num);
	//wait if bussy
	cnt = 1000;
	tmp = ETH_ReadPHYRegister(0x1b,0x0b);
	while((tmp&(1<<15)) && cnt){
		tmp = ETH_ReadPHYRegister(0x1b,0x0b);
		cnt--;
	}
	tmp = ETH_ReadPHYRegister(0x1b, 0x0c);
	entry->port_vect = ((tmp>>4) & 0x7FF);

	//read mac
	buf = (uint16_t *)entry->mac;
	end = 1;
	addr = 0x0d;
	for(int i=0;i<3;i++){
		tmp = ETH_ReadPHYRegister(0x1b, addr);
		if(tmp!=0xffff)
			end = 0;
		*buf++ = tmp;
		addr++;
	}

	//ETH_WritePHYRegister(0x1b, 0x0b, (1<<15)|(4<<12)|num);
	//wait if bussy
	//while(ETH_ReadPHYRegister(0x1b,0x0b)&(1<<15));
	//tmp = ETH_ReadPHYRegister(0x1b, 0x0c);
	//*port = ((tmp>>4) & 0x7FF);

	DEBUG_MSG(DEBUG_QD,"read atu: mac %x:%x:%x:%x:%x:%x port vect %lu\r\n",
			entry->mac[0],entry->mac[1],entry->mac[2],entry->mac[3],entry->mac[4],entry->mac[5],entry->port_vect);
	return end;
}
else if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
	GT_STATUS status = 0;
	static GT_ATU_ENTRY tmpMacEntry;

		if(start){
			if((status = gfdbGetAtuEntryFirst(&tmpMacEntry))!= GT_OK){
				return status;
			}
		}
		else{
			if((status = gfdbGetAtuEntryNext(&tmpMacEntry)) != GT_OK)
			{
				return status;
			}
		}

		entry->mac[0] = tmpMacEntry.macAddr.arEther[1];
		entry->mac[1] = tmpMacEntry.macAddr.arEther[0];
		entry->mac[2] = tmpMacEntry.macAddr.arEther[3];
		entry->mac[3] = tmpMacEntry.macAddr.arEther[2];
		entry->mac[4] = tmpMacEntry.macAddr.arEther[5];
		entry->mac[5] = tmpMacEntry.macAddr.arEther[4];
		entry->port_vect = tmpMacEntry.portVec;
		return status;
	}
else if(get_marvell_id() == DEV_98DX316){
	if(start==1){
		cnt = 0;
	}

	//find firs valid entry
	while(Salsa2_ReadRegField(MAC_TABLE_ENTRY_W0+0x10*cnt,0,1) == 0){
		cnt++;
		if(cnt>4094)
			return 1;
	}

	temp = Salsa2_ReadReg(MAC_TABLE_ENTRY_W0+0x10*cnt);
	if((temp & 0x01) && ((temp & 2)==0) && ((temp & 8) == 0)){//check valid, dont skip, dont trunk
		entry->vid = (u16)((temp>>4)&0x0FFF);
		entry->mac[0] = (u8)(temp>>16);
		entry->mac[1] = (u8)(temp>>24);

		temp = Salsa2_ReadReg(MAC_TABLE_ENTRY_W1+0x10*cnt);
		entry->mac[2] = (u8)temp;
		entry->mac[3] = (u8)(temp>>8);
		entry->mac[4] = (u8)(temp>>16);
		entry->mac[5] = (u8)(temp>>24);

		temp = Salsa2_ReadReg(MAC_TABLE_ENTRY_W2+0x10*cnt);
		entry->port_vect = temp & 0x1F;
		cnt++;
		return 0;
	}
}

return 0;
}

/*
//pereodical read ATU (FDB) for Salsa PP
void read_all_atu(void){
u16 cnt;//all atu
u8 cnt2;//cpu atu
u32 temp;

	if(get_dev_type()==DEV_SWU16){
		for(u8 i=0;i<MAX_CPU_ENTRIES_NUM;i++){
			mac_entry[i].valid = 0;
		}
		cnt = 0;
		cnt2 = 0;

		taskENTER_CRITICAL();

		DEBUG_MSG(1,"read_all_atu start\r\n");

		while((cnt<8192) && (cnt2<MAX_CPU_ENTRIES_NUM)){
			temp = Salsa2_ReadReg(MAC_TABLE_ENTRY_W0+0x10*cnt);
			if((temp & 0x01) && ((temp & 0x02)==0)){//check valid, dont skip

				DEBUG_MSG(0,"read_all_atu: %d\r\n",cnt);


				mac_entry[cnt2].vid = (u16)((temp>>4)&0x0FFF);
				mac_entry[cnt2].mac[4] = (u8)(temp>>16);
				mac_entry[cnt2].mac[5] = (u8)(temp>>24);
				if(temp & (1<<3))
					mac_entry[cnt2].is_trunk = 1;
				else
					mac_entry[cnt2].is_trunk = 0;

				temp = Salsa2_ReadReg(MAC_TABLE_ENTRY_W1+0x10*cnt);
				mac_entry[cnt2].mac[2] = (u8)temp;
				mac_entry[cnt2].mac[3] = (u8)(temp>>8);
				mac_entry[cnt2].mac[0] = (u8)(temp>>16);
				mac_entry[cnt2].mac[1] = (u8)(temp>>24);

				temp = Salsa2_ReadReg(MAC_TABLE_ENTRY_W2+0x10*cnt);
				mac_entry[cnt2].port_vect = (temp & 0x1F);

				if(mac_entry[cnt2].mac[0]==0xC0 && mac_entry[cnt2].mac[1]==0x11 && mac_entry[cnt2].mac[2]==0xA6){
					if(!(temp & 0x3C000)){
						temp |=0x3C000;//set priority to maximum
						Salsa2_WriteReg(MAC_TABLE_ENTRY_W2+0x10*cnt,temp);
					}
				}

				mac_entry[cnt2].valid = 1;
				cnt2++;

				vTaskDelay(1);//хз зачем
			}
			else
				mac_entry[cnt2].valid = 0;

			cnt++;
		}

		DEBUG_MSG(2,"read_all_atu stop\r\n");

		taskEXIT_CRITICAL();

	}
}
*/

//получение записи из MAC таблицы
//num - номер записи
//entry - указатель на структуру записи
//ret 1-запись существует
u8 get_salsa2_fdb_entry(u16 num, struct mac_entry_t *entry){
u32 temp;

	if(num >= SALSA2_FDB_MAX)
		return 0;

	temp = Salsa2_ReadReg(MAC_TABLE_ENTRY_W0+0x10*num);
	entry->valid = 0;
	if((temp & 0x01) && ((temp & 0x02)==0)){//check valid, dont skip
		DEBUG_MSG(DEBUG_QD,"read_all_atu: %d %lX\r\n",num,temp);

		entry->vid = (u16)((temp>>4)&0x0FFF);
		entry->mac[5] = (u8)(temp>>16);
		entry->mac[4] = (u8)(temp>>24);
		if(temp & (1<<3))
			entry->is_trunk = 1;
		else
			entry->is_trunk = 0;

		temp = Salsa2_ReadReg(MAC_TABLE_ENTRY_W1+0x10*num);
		entry->mac[3] = (u8)temp;
		entry->mac[2] = (u8)(temp>>8);
		entry->mac[1] = (u8)(temp>>16);
		entry->mac[0] = (u8)(temp>>24);

		temp = Salsa2_ReadReg(MAC_TABLE_ENTRY_W2+0x10*num);
		entry->port_vect = (temp & 0x1F);

		entry->valid = 1;
	}
	vTaskDelay(1);
	IWDG_ReloadCounter();
	return entry->valid;
}

u32 read_atu_agetime(void){
u16 tmp;
	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){
		tmp = ETH_ReadPHYRegister(0x1B, 0x0A);
		tmp = (tmp>>4)&0xFF;
		return tmp*15;
	}else	if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
		tmp = ETH_ReadPHYRegister(0x1B, 0x0A);
		tmp = (tmp>>4)&0xFF;
		return tmp*15;
	}
	else{
		return SALSA2_AGE_TIME;
	}
	return 0;
}

u32 get_atu_port_vect(u8 *mac_s){
int start = 1 ;
struct mac_entry_t entry;

	while(read_atu(0, start, &entry)==0){
		//if(uip_macaddr_cmp(mac,mac_s)){
		//		return port;
		//}
		if(mac_s[0]==entry.mac[1] && mac_s[1]==entry.mac[0] && mac_s[2]==entry.mac[3]
		         && mac_s[3]==entry.mac[2] && mac_s[4]==entry.mac[5] && mac_s[5] == entry.mac[4])
			return entry.port_vect;
		start = 0;
	}
	return 0;
}




/*******************************************************************************
* gfdbGetAtuEntryNext
*
* DESCRIPTION:
*       Gets next lexicographic MAC address from the specified Mac Addr.
*
* INPUTS:
*       atuEntry - the Mac Address to start the search.
*
* OUTPUTS:
*       atuEntry - match Address translate unit entry.
*
* RETURNS:
*       GT_OK      - on success.
*       GT_FAIL    - on error or entry does not exist.
*       GT_NO_SUCH - no more entries.
*
* COMMENTS:
*       Search starts from atu.macAddr[xx:xx:xx:xx:xx:xx] specified by the
*       user.
*
*        DBNum in atuEntry -
*            ATU MAC Address Database number. If multiple address
*            databases are not being used, DBNum should be zero.
*            If multiple address databases are being used, this value
*            should be set to the desired address database number.
*
*******************************************************************************/
GT_STATUS gfdbGetAtuEntryNext
(
    GT_ATU_ENTRY  *atuEntry
)
{
    GT_STATUS       retVal;
    GT_ATU_ENTRY    entry;
    GT_U32 			data;

    DEBUG_MSG(DEBUG_QD,"gfdbGetAtuEntryNext Called.\r\n");

    if(IS_BROADCAST_MAC(atuEntry->macAddr))
    {
           return GT_NO_SUCH;
    }

    memcpy(entry.macAddr.arEther,atuEntry->macAddr.arEther,6);

    entry.DBNum = atuEntry->DBNum;
    DEBUG_MSG(DEBUG_QD,"DBNum : %i\r\n",entry.DBNum);

    retVal = atuOperationPerform(GET_NEXT_ENTRY,NULL,&entry);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(DEBUG_QD,"Failed (atuOperationPerform returned GT_FAIL).\r\n");
        return retVal;
    }

    if(IS_BROADCAST_MAC(entry.macAddr))
    {
        //if(IS_IN_DEV_GROUP(dev,DEV_BROADCAST_INVALID))
    	if(0)
        {
    		DEBUG_MSG(DEBUG_QD,"Failed (Invalid Mac).\r\n");
            return GT_NO_SUCH;
        }
        else if(entry.entryState.ucEntryState == 0)
        {
        	DEBUG_MSG(DEBUG_QD,"Failed (Invalid Mac).\r\n");
            return GT_NO_SUCH;
        }
    }

    memcpy(atuEntry->macAddr.arEther,entry.macAddr.arEther,6);
    atuEntry->portVec   = GT_PORTVEC_2_LPORTVEC(entry.portVec);
    atuEntry->prio      = entry.prio;
    atuEntry->trunkMember = entry.trunkMember;
    atuEntry->exPrio.useMacFPri = entry.exPrio.useMacFPri;
    atuEntry->exPrio.macFPri = entry.exPrio.macFPri;
    atuEntry->exPrio.macQPri = entry.exPrio.macQPri;

    if(IS_MULTICAST_MAC(entry.macAddr))
    {
    	//if(dev->deviceId == GT_88E6051)
        if(0)
    	{
        	DEBUG_MSG(DEBUG_QD,"Failed.\r\n");
            return GT_FAIL;
        }

        atuStateDevToApp(GT_FALSE,(GT_U32)entry.entryState.ucEntryState, &data);
        atuEntry->entryState.mcEntryState = data;
    }
    else
    {
        atuStateDevToApp(GT_TRUE,(GT_U32)entry.entryState.ucEntryState,&data);
        atuEntry->entryState.ucEntryState = data;
    }

    DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    return GT_OK;
}


/*******************************************************************************
* gfdbGetAtuEntryFirst
*
* DESCRIPTION:
*       Gets first lexicographic MAC address entry from the ATU.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       atuEntry - match Address translate unit entry.
*
* RETURNS:
*       GT_OK      - on success
*       GT_FAIL    - on error
*       GT_NO_SUCH - table is empty.
*
* COMMENTS:
*       Search starts from Mac[00:00:00:00:00:00]
*
*        DBNum in atuEntry -
*            ATU MAC Address Database number. If multiple address
*            databases are not being used, DBNum should be zero.
*            If multiple address databases are being used, this value
*            should be set to the desired address database number.
*
*******************************************************************************/
GT_STATUS gfdbGetAtuEntryFirst
(
    GT_ATU_ENTRY    *atuEntry
)
{
    GT_STATUS       retVal;
    GT_ATU_ENTRY    entry;
    GT_U32 data;

    DEBUG_MSG(DEBUG_QD,"gfdbGetAtuEntryFirst Called.\r\n");

    if(/*IS_IN_DEV_GROUP(dev,DEV_BROADCAST_INVALID)*/0)
        memset(entry.macAddr.arEther,0,sizeof(GT_ETHERADDR));
    else
        memset(entry.macAddr.arEther,0xFF,sizeof(GT_ETHERADDR));

    entry.DBNum = atuEntry->DBNum;

    DEBUG_MSG(DEBUG_QD,"DBNum : %i\n",entry.DBNum);

    retVal = atuOperationPerform(GET_NEXT_ENTRY,NULL,&entry);
    if(retVal != GT_OK)
    {
    	DEBUG_MSG(DEBUG_QD,"Failed (atuOperationPerform returned GT_FAIL).\r\n");
        return retVal;
    }

    if(IS_BROADCAST_MAC(entry.macAddr))
    {
        if(/*IS_IN_DEV_GROUP(dev,DEV_BROADCAST_INVALID)*/0)
        {
        	DEBUG_MSG(DEBUG_QD,"Failed (Invalid Mac).\r\n");
            return GT_NO_SUCH;
        }
        else if(entry.entryState.ucEntryState == 0)
        {
        	DEBUG_MSG(DEBUG_QD,"Failed (Invalid Mac).\r\n");
            return GT_NO_SUCH;
        }
    }

    memcpy(atuEntry->macAddr.arEther,entry.macAddr.arEther,6);
    atuEntry->portVec   = GT_PORTVEC_2_LPORTVEC(entry.portVec);
    atuEntry->prio      = entry.prio;
    atuEntry->trunkMember = entry.trunkMember;
    atuEntry->exPrio.useMacFPri = entry.exPrio.useMacFPri;
    atuEntry->exPrio.macFPri = entry.exPrio.macFPri;
    atuEntry->exPrio.macQPri = entry.exPrio.macQPri;

    if(IS_MULTICAST_MAC(entry.macAddr))
    {
        /*if(dev->deviceId == GT_88E6051)
        {
            DBG_INFO(("Failed.\n"));
            return GT_FAIL;
        }*/

        atuStateDevToApp(GT_FALSE,entry.entryState.ucEntryState, &data);
        atuEntry->entryState.mcEntryState = data;
    }
    else
    {
        atuStateDevToApp(GT_TRUE,entry.entryState.ucEntryState,&data);
        atuEntry->entryState.ucEntryState = data;
    }

    DEBUG_MSG(DEBUG_QD,"OK.\r\n");
    return GT_OK;
}

GT_STATUS atuStateDevToApp
(
    GT_BOOL        unicast,//IN
    GT_U32        state,//IN
    GT_U32        *newOne//OUT
)
{
    GT_U32    newState;
    GT_STATUS    retVal = GT_OK;

    if(unicast)
    {
        if (state == 0)
        {
            newState = (GT_U32)GT_UC_INVALID;
        }
        else if (state <= 7)
        {
            newState = (GT_U32)GT_UC_DYNAMIC;
        }
        else if ((state <= 0xE) && (!/*IS_IN_DEV_GROUP(dev,DEV_UC_7_DYNAMIC)*/1))
        {
            newState = (GT_U32)GT_UC_DYNAMIC;
        }
        else
        {
            newState = state;
        }
    }
    else
    {
        newState = state;
    }

    *newOne = newState;
    return retVal;
}

/*читаем значения */
/*u8 port - логический порт 0-4*/
/*u8 type:
 0 - RX good
 2 - Rx bad
 0x0E - tx good
 0x1E - collision
 */
u32 read_stats_cnt(u8 port, u8 type){
GT_STATUS retVal;
u8 hw_port;
//u16 data;
u32 ret;
//u16 cnt;

//logical to physical port
hw_port=L2F_port_conv(port);

if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240) ||
		(get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){


	switch(type){
		case RX_GOOD:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_InGoodOctetsLo,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;
		case  TX_GOOD:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_OutOctetsLo,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;



		case COLLISION:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_Collisions,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;

		case RX_UNICAST:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_InUnicasts,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;

		case TX_UNICAST:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_OutUnicasts,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;



		case RX_MULTICAST:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_InMulticasts,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;

		case TX_MULTICAST:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_OutMulticasts,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;

		case RX_BROADCAST:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_InBroadcasts,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;


		case TX_BROADCAST:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_OutBroadcasts,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;
//new
		case TX_FCSERR:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_OutFCSErr,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;

		case DEFERRED:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_Deferred,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;

		case EXCESSIVE:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_Excessive,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;

		case SINGLE:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_Single,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;

		case TX_PAUSE:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_OutPause,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;
		case RX_PAUSE:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_InPause,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;
		case MULTIPLE:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_Multiple,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;
		case RX_UNDERSIZE:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_Undersize,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;
		case RX_FARGMENTS:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_Fragments,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;
		case RX_OVERSIZE:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_Oversize,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;
		case RX_JABBER:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_Jabber,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;
		case RX_RXERR:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_InMACRcvErr,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;
		case RX_FCSERR:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_InFCSErr,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;
		case LATE:
			retVal = statsOperationPerform(STATS_READ_COUNTER,hw_port,STATS3_Late,(GT_VOID *)&ret);
			if(retVal != GT_OK)
			{
				DEBUG_MSG(DEBUG_QD,"Failed (statsOperationPerform returned GT_FAIL).\r\n");
				return 0;
			}
			return ret;


	}
}

	if(get_marvell_id() == DEV_98DX316){

		switch(type){
			case RX_GOOD:
				retVal = Salsa2_Read_Counter(SALSA2_RX_GOOD,hw_port,&ret);
				if(retVal != GT_OK)
				{
					DEBUG_MSG(DEBUG_QD,"Failed (Salsa2_Read_Counter returned GT_FAIL).\r\n");
					return 0;
				}
				return ret;
			case TX_GOOD:

				Salsa2_Read_Counter(SALSA2_TX_GOOD,hw_port,&ret);
				retVal = Salsa2_Read_Counter(SALSA2_TX_GOOD,hw_port,&ret);
				if(retVal != GT_OK)
				{
					DEBUG_MSG(DEBUG_QD,"Failed (Salsa2_Read_Counter returned GT_FAIL).\r\n");
					return 0;
				}
				return ret;
			case COLLISION:
				retVal = Salsa2_Read_Counter(SALSA2_COLLISION,hw_port,&ret);
				if(retVal != GT_OK)
				{
					DEBUG_MSG(DEBUG_QD,"Failed (Salsa2_Read_Counter returned GT_FAIL).\r\n");
					return 0;
				}
				return ret;
			case RX_UNICAST:
				return 0;
			case TX_UNICAST:
				return 0;
			case RX_MULTICAST:
				retVal = Salsa2_Read_Counter(SALSA2_RX_MULTICAST,hw_port,&ret);
				if(retVal != GT_OK)
				{
					DEBUG_MSG(DEBUG_QD,"Failed (Salsa2_Read_Counter returned GT_FAIL).\r\n");
					return 0;
				}
				return ret;
			case TX_MULTICAST:
				retVal = Salsa2_Read_Counter(SALSA2_TX_MULTICAST,hw_port,&ret);
				if(retVal != GT_OK)
				{
					DEBUG_MSG(DEBUG_QD,"Failed (Salsa2_Read_Counter returned GT_FAIL).\r\n");
					return 0;
				}
				return ret;
			case RX_BROADCAST:
				retVal = Salsa2_Read_Counter(SALSA2_RX_BROADCAST,hw_port,&ret);
				if(retVal != GT_OK)
				{
					DEBUG_MSG(DEBUG_QD,"Failed (Salsa2_Read_Counter returned GT_FAIL).\r\n");
					return 0;
				}
				return ret;
			case TX_BROADCAST:
				retVal = Salsa2_Read_Counter(SALSA2_TX_BROADCAST,hw_port,&ret);
				if(retVal != GT_OK)
				{
					DEBUG_MSG(DEBUG_QD,"Failed (Salsa2_Read_Counter returned GT_FAIL).\r\n");
					return 0;
				}
				return ret;

			case RX_BAD:
				retVal = Salsa2_Read_Counter(SALSA2_RX_BAD,hw_port,&ret);
				if(retVal != GT_OK)
				{
					DEBUG_MSG(DEBUG_QD,"Failed (Salsa2_Read_Counter returned GT_FAIL).\r\n");
					return 0;
				}
				return ret;


			default: return 0;
		}
	}

    return 0;

}


/*******************************************************************************
* statsOperationPerform
*
* DESCRIPTION:
*       This function is used by all stats control functions, and is responsible
*       to write the required operation into the stats registers.
*
* INPUTS:
*       statsOp       - The stats operation bits to be written into the stats
*                     operation register.
*       port        - port number
*       counter     - counter to be read if it's read operation
*
* OUTPUTS:
*       statsData   - points to the data storage where the MIB counter will be saved.
*
* RETURNS:
*       GT_OK on success,
*       GT_FAIL otherwise.
*
* COMMENTS:
*
*******************************************************************************/

GT_STATUS statsOperationPerform
(
    GT_STATS_OPERATION   statsOp,
    GT_U8                port,
    GT_STATS_COUNTERS    counter,
    GT_VOID              *statsData
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16          data,histoData; /* Data to be set into the      */
                                    /* register.                    */
    GT_U32 statsCounter;
    GT_U32 lastCounter;
    GT_U16            portNum;

    //if (!((IS_IN_DEV_GROUP(dev,DEV_GIGABIT_SWITCH)) ||
    //    (IS_IN_DEV_GROUP(dev,DEV_RMON_REALTIME_SUPPORT))))
    if (0)
    {
      /*if (IS_IN_DEV_GROUP(dev,DEV_MELODY_SWITCH))
        lastCounter = (GT_U32)STATS2_Late;
      else
        lastCounter = (GT_U32)STATS_OutDiscards;*/
    }
    else
    {
        lastCounter = (GT_U32)STATS3_Late;
    }

    //if (IS_IN_DEV_GROUP(dev,DEV_RMON_PORT_BITS))
    if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240))
    {
        portNum = (port + 1) << 5;
    }
    else
    {
        portNum = (GT_U16)port;
    }

    /* Wait until the stats in ready. */
#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 2;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_STATS_OPERATION;
      regAccess.rw_reg_list[0].data = 15;
      regAccess.rw_reg_list[1].cmd = HW_REG_READ;
      regAccess.rw_reg_list[1].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[1].reg = QD_REG_STATS_OPERATION;
      regAccess.rw_reg_list[1].data = 0;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        gtSemGive(dev,dev->statsRegsSem);
        return retVal;
      }
      histoData = qdLong2Short(regAccess.rw_reg_list[1].data);
    }
#else
    data = 1;
    while(data == 1)
    {
        retVal = hwGetGlobalRegField(QD_REG_STATS_OPERATION,15,1,&data);
        if(retVal != GT_OK)
        {
              return retVal;
        }
    }

    /* Get the Histogram mode bit.                */
    retVal = hwReadGlobalReg(QD_REG_STATS_OPERATION,&histoData);
    if(retVal != GT_OK)
    {
        return retVal;
    }

#endif
    histoData &= 0xC00;

    /* Set the STAT Operation register */
    switch (statsOp)
    {
        case STATS_FLUSH_ALL:
            data = (1 << 15) | (GT_STATS_FLUSH_ALL << 12) | histoData;
            retVal = hwWriteGlobalReg(QD_REG_STATS_OPERATION,data);
            return retVal;

        case STATS_FLUSH_PORT:
            data = (1 << 15) | (GT_STATS_FLUSH_PORT << 12) | portNum | histoData;
            retVal = hwWriteGlobalReg(QD_REG_STATS_OPERATION,data);
            return retVal;

        case STATS_READ_COUNTER:
            retVal = statsCapture(port);
            if(retVal != GT_OK)
            {
                return retVal;
            }

            retVal = statsReadCounter(counter,(GT_U32*)statsData);
            if(retVal != GT_OK)
            {
                return retVal;
            }
            break;

        case STATS_READ_REALTIME_COUNTER:
            retVal = statsReadRealtimeCounter(port,counter,(GT_U32*)statsData);
            if(retVal != GT_OK)
            {
               return retVal;
            }

            break;

        case STATS_READ_ALL:
            retVal = statsCapture(port);
            if(retVal != GT_OK)
            {
               return retVal;
            }

            for(statsCounter=0; statsCounter<=lastCounter; statsCounter++)
            {
                retVal = statsReadCounter(statsCounter,((GT_U32*)statsData + statsCounter));
                if(retVal != GT_OK)
                {
                   return retVal;
                }
            }
            break;

        default:
            return GT_FAIL;
    }
    return GT_OK;
}


/*******************************************************************************
* statsCapture
*
* DESCRIPTION:
*       This function is used to capture all counters of a port.
*
* INPUTS:
*       port        - port number
*
* OUTPUTS:
*        None.
*
* RETURNS:
*       GT_OK on success,
*       GT_FAIL otherwise.
*
* COMMENTS:
*        If Semaphore is used, Semaphore should be acquired before this function call.
*******************************************************************************/
GT_STATUS statsCapture
(
    GT_U8             port
)
{
    GT_STATUS       retVal;         /* Functions return value.      */
    GT_U16          data, histoData;/* Data to be set into the      */
                                    /* register.                    */
    GT_U16            portNum;

    //if (IS_IN_DEV_GROUP(dev,DEV_RMON_PORT_BITS))
    if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240))
    {
        portNum = (port + 1) << 5;
    }
    else
    {
        portNum = (GT_U16)port;
    }

    /* Get the Histogram mode bit.                */
    retVal = hwReadGlobalReg(QD_REG_STATS_OPERATION,&histoData);
    if(retVal != GT_OK)
    {
        return retVal;
    }

    histoData &= 0xC00;

#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 1;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_STATS_OPERATION;
      regAccess.rw_reg_list[0].data = 15;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        return retVal;
      }
    }
#else
    data = 1;
       while(data == 1)
    {
        retVal = hwGetGlobalRegField(QD_REG_STATS_OPERATION,15,1,&data);
        if(retVal != GT_OK)
           {
               return retVal;
        }
       }
#endif

    data = (1 << 15) | (GT_STATS_CAPTURE_PORT << 12) | portNum | histoData;
    retVal = hwWriteGlobalReg(QD_REG_STATS_OPERATION,data);
    if(retVal != GT_OK)
    {
        return retVal;
    }

    return GT_OK;
}


/*******************************************************************************
* statsReadCounter
*
* DESCRIPTION:
*       This function is used to read a captured counter.
*
* INPUTS:
*       counter     - counter to be read if it's read operation
*
* OUTPUTS:
*       statsData   - points to the data storage where the MIB counter will be saved.
*
* RETURNS:
*       GT_OK on success,
*       GT_FAIL otherwise.
*
* COMMENTS:
*        If Semaphore is used, Semaphore should be acquired before this function call.
*******************************************************************************/
GT_STATUS statsReadCounter
(
    GT_U32            counter,
    GT_U32            *statsData
)
{
    GT_STATUS   retVal;         /* Functions return value.            */
    GT_U16      data, histoData;/* Data to be set into the  register. */
#ifndef GT_RMGMT_ACCESS
    GT_U16    counter3_2;     /* Counter Register Bytes 3 & 2       */
    GT_U16    counter1_0;     /* Counter Register Bytes 1 & 0       */
#endif

    /* Get the Histogram mode bit.                */
    retVal = hwReadGlobalReg(QD_REG_STATS_OPERATION,&histoData);
    if(retVal != GT_OK)
    {
        return retVal;
    }

    histoData &= 0xC00;

#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 1;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_STATS_OPERATION;
      regAccess.rw_reg_list[0].data = 15;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        return retVal;
      }
    }
#else
    data = 1;
       while(data == 1)
    {
        retVal = hwGetGlobalRegField(QD_REG_STATS_OPERATION,15,1,&data);
        if(retVal != GT_OK)
        {
               return retVal;
        }
       }
#endif

    data = (GT_U16)((1 << 15) | (GT_STATS_READ_COUNTER << 12) | counter | histoData);
    retVal = hwWriteGlobalReg(QD_REG_STATS_OPERATION,data);
    if(retVal != GT_OK)
    {
        return retVal;
    }

#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 3;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_STATS_OPERATION;
      regAccess.rw_reg_list[0].data = 15;
      regAccess.rw_reg_list[1].cmd = HW_REG_READ;
      regAccess.rw_reg_list[1].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[1].reg = QD_REG_STATS_COUNTER3_2;
      regAccess.rw_reg_list[1].data = 0;
      regAccess.rw_reg_list[2].cmd = HW_REG_READ;
      regAccess.rw_reg_list[2].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[2].reg = QD_REG_STATS_COUNTER1_0;
      regAccess.rw_reg_list[2].data = 0;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        return retVal;
      }
      *statsData = (regAccess.rw_reg_list[1].data << 16) | regAccess.rw_reg_list[2].data;
    }
#else
    data = 1;
       while(data == 1)
    {
        retVal = hwGetGlobalRegField(QD_REG_STATS_OPERATION,15,1,&data);
        if(retVal != GT_OK)
        {
               return retVal;
        }
    }

    retVal = hwReadGlobalReg(QD_REG_STATS_COUNTER3_2,&counter3_2);
    if(retVal != GT_OK)
    {
        return retVal;
    }

    retVal = hwReadGlobalReg(QD_REG_STATS_COUNTER1_0,&counter1_0);
    if(retVal != GT_OK)
    {
        return retVal;
    }

    *statsData = (counter3_2 << 16) | counter1_0;
#endif

    return GT_OK;

}


/*******************************************************************************
* statsReadRealtimeCounter
*
* DESCRIPTION:
*       This function is used to read a realtime counter.
*
* INPUTS:
*       port     - port to be accessed
*       counter  - counter to be read if it's read operation
*
* OUTPUTS:
*       statsData   - points to the data storage where the MIB counter will be saved.
*
* RETURNS:
*       GT_OK on success,
*       GT_FAIL otherwise.
*
* COMMENTS:
*        If Semaphore is used, Semaphore should be acquired before this function call.
*******************************************************************************/
GT_STATUS statsReadRealtimeCounter
(
    IN   GT_U8             port,
    IN   GT_U32            counter,
    OUT  GT_U32            *statsData
)
{
    GT_STATUS   retVal;         /* Functions return value.            */
    GT_U16      data, histoData;/* Data to be set into the  register. */
    GT_U16    counter3_2;     /* Counter Register Bytes 3 & 2       */
    GT_U16    counter1_0;     /* Counter Register Bytes 1 & 0       */

    /* Get the Histogram mode bit.                */
    retVal = hwReadGlobalReg(QD_REG_STATS_OPERATION,&histoData);
    if(retVal != GT_OK)
    {
        return retVal;
    }

    histoData &= 0xC00;

#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 1;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_STATS_OPERATION;
      regAccess.rw_reg_list[0].data = 15;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        return retVal;
      }
    }
#else
    data = 1;
       while(data == 1)
    {
        retVal = hwGetGlobalRegField(QD_REG_STATS_OPERATION,15,1,&data);
        if(retVal != GT_OK)
           {
               return retVal;
        }
       }
#endif

    data = (GT_U16)((1 << 15) | (GT_STATS_READ_COUNTER << 12) | ((port+1) << 5) | counter | histoData);
    retVal = hwWriteGlobalReg(QD_REG_STATS_OPERATION,data);
    if(retVal != GT_OK)
    {
        return retVal;
    }

#ifdef GT_RMGMT_ACCESS
    {
      HW_DEV_REG_ACCESS regAccess;

      regAccess.entries = 1;

      regAccess.rw_reg_list[0].cmd = HW_REG_WAIT_TILL_0;
      regAccess.rw_reg_list[0].addr = CALC_SMI_DEV_ADDR(dev, 0, GLOBAL_REG_ACCESS);
      regAccess.rw_reg_list[0].reg = QD_REG_STATS_OPERATION;
      regAccess.rw_reg_list[0].data = 15;
      retVal = hwAccessMultiRegs(dev, &regAccess);
      if(retVal != GT_OK)
      {
        return retVal;
      }
    }
#else
    data = 1;
       while(data == 1)
    {
        retVal = hwGetGlobalRegField(QD_REG_STATS_OPERATION,15,1,&data);
        if(retVal != GT_OK)
           {
               return retVal;
        }
       }
#endif

    retVal = hwReadGlobalReg(QD_REG_STATS_COUNTER3_2,&counter3_2);
    if(retVal != GT_OK)
    {
        return retVal;
    }

    retVal = hwReadGlobalReg(QD_REG_STATS_COUNTER1_0,&counter1_0);
    if(retVal != GT_OK)
    {
        return retVal;
    }

    *statsData = (counter3_2 << 16) | counter1_0;

    return GT_OK;

}


static u8 start_vct_test;
static u8 port_vct_test;
static u8 length_vct_test;

u8 set_cable_test(u8 state,u8 port,u8 length){
	start_vct_test = state;
	if(port<COOPER_PORT_NUM)
		port_vct_test = port;
	if(state == VCT_CALLIBRATE)
		length_vct_test = length;
	DEBUG_MSG(DEBUG_QD,"length_vct_test %d\r\n",length_vct_test);
	return 0;
}

u8 get_vct_port(void){
	return port_vct_test;
}

u8 get_cable_test(void){
	return start_vct_test;
}

/*получение статистики по портам*/
void port_statistics_processing(void){
	//get stats for all ports
	for(u8 i= 0;i<(ALL_PORT_NUM);i++){

		//if(dev.port_stat[i].link == 1){
			//save last value
			dev.port_stat[i].rx_good_last = dev.port_stat[i].rx_good;
			dev.port_stat[i].tx_good_last = dev.port_stat[i].tx_good;

			dev.port_stat[i].rx_good 	= read_stats_cnt(i,RX_GOOD);
			if(dev.port_stat[i].rx_good == 0)
				dev.port_stat[i].rx_good = dev.port_stat[i].rx_good_last;

			dev.port_stat[i].tx_good 	= read_stats_cnt(i,TX_GOOD);
			if(dev.port_stat[i].tx_good == 0)
				dev.port_stat[i].tx_good = dev.port_stat[i].tx_good_last;


			dev.port_stat[i].rx_unicast = read_stats_cnt(i,RX_UNICAST);
			dev.port_stat[i].tx_unicast = read_stats_cnt(i,TX_UNICAST);
			dev.port_stat[i].rx_broadcast = read_stats_cnt(i,RX_BROADCAST);
			dev.port_stat[i].tx_broadcast = read_stats_cnt(i,TX_BROADCAST);
			dev.port_stat[i].rx_multicast = read_stats_cnt(i,RX_MULTICAST);
			dev.port_stat[i].tx_multicast = read_stats_cnt(i,TX_MULTICAST);


			//calculate speed
			if(dev.port_stat[i].rx_good>dev.port_stat[i].rx_good_last)
				dev.port_stat[i].rx_speed = (dev.port_stat[i].rx_good - dev.port_stat[i].rx_good_last)/SPEED_STAT_PERIOD;
			if(dev.port_stat[i].tx_good > dev.port_stat[i].tx_good_last)
				dev.port_stat[i].tx_speed = (dev.port_stat[i].tx_good - dev.port_stat[i].tx_good_last)/SPEED_STAT_PERIOD;

		//}
		//else{
		//	dev.port_stat[i].rx_speed = 0;
		//	dev.port_stat[i].tx_speed = 0;
		//}

		//заполняем и сдвигаем массив статистики для скорости
		//новые складываем в конец, постепенно сдвигая вперед
		for(u8 j=0;j<(SPEED_STAT_LEN-1);j++){
			dev.port_stat[i].rx_speed_stat[j] = dev.port_stat[i].rx_speed_stat[j+1];
			dev.port_stat[i].tx_speed_stat[j] = dev.port_stat[i].tx_speed_stat[j+1];
		}
		dev.port_stat[i].rx_speed_stat[SPEED_STAT_LEN-1] = (u8)(dev.port_stat[i].rx_speed/131072);
		dev.port_stat[i].tx_speed_stat[SPEED_STAT_LEN-1] = (u8)(dev.port_stat[i].tx_speed/131072);
	}
}

void vct_processing(void){
u8 coef1,coef2;
	if(get_cable_test() == VCT_TEST){
		if(port_vct_test<COOPER_PORT_NUM){
			simpleCableTest(port_vct_test);
			set_cable_test(0,0,0);
		}
	}
	if(get_cable_test() == VCT_CALLIBRATE){
		if(port_vct_test<COOPER_PORT_NUM){
			if(simpleCableTest(port_vct_test) == 0){
				//make callibrate koeff
				DEBUG_MSG(DEBUG_QD,"length_vct_test %d\r\n",length_vct_test);
				if(dev.port_stat[port_vct_test].rx_len){
					coef1 = (length_vct_test*100)/(dev.port_stat[port_vct_test].rx_len);
					DEBUG_MSG(DEBUG_QD,"coeff1 %d\r\n",coef1);
					set_callibrate_koef_1(port_vct_test,coef1);
				}
				if(dev.port_stat[port_vct_test].tx_len){
					coef2 = (length_vct_test*100)/(dev.port_stat[port_vct_test].tx_len);
					DEBUG_MSG(DEBUG_QD,"coeff2 %d\r\n",coef2);
					set_callibrate_koef_2(port_vct_test,coef2);
				}
				if((dev.port_stat[port_vct_test].rx_len)||(dev.port_stat[port_vct_test].tx_len)){
					set_callibrate_len(port_vct_test,length_vct_test);
					settings_save();
				}

				dev.port_stat[port_vct_test].vct_compleat_ok = 0;
			}
			set_cable_test(0,0,0);
		}
	}
}

/*This sample shows how to run Virtual Cable Test and how to use the
test result.													    */
uint32_t simpleCableTest(uint8_t port){
uint16_t TempReg;
uint16_t TempReg1;
uint32_t Temp=0;
uint16_t RXST;
uint16_t TXST;
uint8_t LengthRX=0;
uint8_t LengthTX=0;
u16 phyPort;
u8 phyAddr;

DEBUG_MSG(VCT_DBG,"simpleCableTest\r\n");

phyPort = L2F_port_conv(port);


if(get_marvell_id() == DEV_98DX316){


	phyPort = L2F_port_conv(port+FIBER_PORT_NUM);
	phyAddr = Salsa2_get_phyAddr(port+FIBER_PORT_NUM);
	Salsa2_WritePhyReg(phyPort,phyAddr,22,7);
	TempReg = Salsa2_ReadPhyReg(phyPort,phyAddr,21);
	TempReg |= 1<<15;//start measurment
	Salsa2_WritePhyReg(phyPort,phyAddr,21,TempReg);

	vTaskDelay(100*MSEC);

	while(TempReg & (1<<15)){
		TempReg = Salsa2_ReadPhyReg(phyPort,phyAddr,21);
	}

	vTaskDelay(1000*MSEC);


	TempReg = Salsa2_ReadPhyReg(phyPort,phyAddr,20);
	for(u8 i=0;i<4;i++){
		//pair 0 status
		switch((TempReg>>i*4)&0x0F){
			case 0: dev.port_stat[port].vct_status[i] = VCT_BAD;break;
			case 1: dev.port_stat[port].vct_status[i] = VCT_GOOD;break;
			case 2: dev.port_stat[port].vct_status[i] = VCT_OPEN;break;
			case 3: dev.port_stat[port].vct_status[i] = VCT_SAME_PAIR_SHORT;break;
			case 4: dev.port_stat[port].vct_status[i] = VCT_CROSS_PAIR_SHORT;break;
			default:dev.port_stat[port].vct_status[i] = VCT_PAIR_BUSY;break;
		}
	}

	for(u8 i=0;i<4;i++){
		dev.port_stat[port].vct_len[i] = Salsa2_ReadPhyReg(phyPort,phyAddr,16+i);
		DEBUG_MSG(VCT_DBG,"len %d: 0x%X\r\n",i,dev.port_stat[port].vct_len[i]);
	}

	dev.port_stat[port].vct_compleat_ok = 1;
	DEBUG_MSG(VCT_DBG,"simple Cable Test end 0x%X\r\n",TempReg);

	//Restart ANEg
	//set page 0
	Salsa2_WritePhyReg(phyPort,phyAddr,22,0);
	TempReg = Salsa2_ReadPhyReg(phyPort,phyAddr,0);
	TempReg |= 1<<9;//set Restart Cooper aneg bit
	Salsa2_WritePhyReg(phyPort,phyAddr,0,TempReg);

	return 0;
}


if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
	//DEBUG_MSG(VCT_DBG,"Reading QD_REG_ADV_VCT_CONTROL_5\r\n"/*ETH_ReadIndirectPHYReg(0,PAGE5,23)*/);

	DEBUG_MSG(VCT_DBG,"Advanced Cable Test\r\n");
	//advVctTest(phyPort);


	simpleVctTest(port);

	DEBUG_MSG(VCT_DBG,"simple Cable Test end\r\n");
	return 0;
}

if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){
	//Start:
	DEBUG_MSG(VCT_DBG,"start cable test on port %d \r\n",phyPort);
	TempReg1=ETH_ReadPHYRegister(phyPort,0);//Phy Control Registr // запоминаем
	ETH_WritePHYRegister(phyPort,0,1<<15 |1<<13 | 1<<8);

	/* Enable Virtual Cable Tester*/
	ETH_WritePHYRegister(phyPort,0x1A,ENABLE_VCT_MASK);

	/*ждем результат*/
	//таймаут 5 сек
	Temp = 50;
	do
	{
		DEBUG_MSG(VCT_DBG,"vct wait \r\n");
		TempReg=ETH_ReadPHYRegister(phyPort,0x1A);
		vTaskDelay(100*MSEC);
		Temp--;
		if(Temp==0){
			//* восстанавливаем педыдущее значение*/
			//NVIC_EnableIRQ(EXTI15_10_IRQn);
			ETH_WritePHYRegister(phyPort,0,TempReg1);
			dev.port_stat[port].rx_status = VCT_BAD;
			dev.port_stat[port].tx_status = VCT_BAD;
			DEBUG_MSG(VCT_DBG,"vct bad result\r\n");
			return 1;//0x303FFFF;
		}
	}while(TempReg & VCTCompleatMask);



	vTaskDelay(500*MSEC);
	DEBUG_MSG(VCT_DBG,"vct ready \r\n ");
	TXST=ETH_ReadPHYRegister(phyPort,0x1A);
	RXST=ETH_ReadPHYRegister(phyPort,0x1B);


	LengthTX=(u8)(((786*(TXST & 0xFF)-18862))/1000);
	LengthRX=(u8)(((786*(RXST & 0xFF)-18862))/1000);


	TXST = ((TXST)>>13) & 0x03;
	RXST = ((RXST)>>13) & 0x03;

	DEBUG_MSG(VCT_DBG,"vct ststus RX %d  TX %d \r\n",RXST,TXST);

	/*00 && len<0xFF impedance mismatch
	 *00 && len==0xFF good termination
	 *01 short
	 *10 open
	 *11 unable to perform МСЕ to this pair
	 */
	/*
	 * rx_status
	 * 0 - test not run / bad test
	 * 1 - short
	 * 2 - open
	 * 3 - good
	*/
	/*
	#define VCT_BAD 	0
	#define VCT_SHORT 	1
	#define VCT_OPEN	2
	#define VCT_GOOD	3
	*/

	vTaskDelay(500*MSEC);
	/* восстанавливаем педыдущее значение*/
	//NVIC_EnableIRQ(EXTI15_10_IRQn);
	ETH_WritePHYRegister(phyPort,0,TempReg1);

	switch(RXST){
		case 0:
			//if(LengthRX == 0xFF)
				dev.port_stat[port].rx_status = VCT_GOOD;
			//else
			//	dev.port_stat[port].rx_status = VCT_BAD;
			break;
		case 1:
			dev.port_stat[port].rx_status = VCT_SHORT;
			break;
		case 2:
			dev.port_stat[port].rx_status = VCT_OPEN;
			break;
		case 3:
		default:
			dev.port_stat[port].rx_status = VCT_BAD;
	}

	switch(TXST){
		case 0:
			//if(LengthTX == 0xFF)
				dev.port_stat[port].tx_status = VCT_GOOD;
			//else
			//	dev.port_stat[port].tx_status = VCT_BAD;
			break;
		case 1:
			dev.port_stat[port].tx_status = VCT_SHORT;
			break;
		case 2:
			dev.port_stat[port].tx_status = VCT_OPEN;
			break;
		case 3:
		default:
			dev.port_stat[port].tx_status = VCT_BAD;
	}

	dev.port_stat[port].rx_len  = LengthRX;
	dev.port_stat[port].tx_len  = LengthTX;

	dev.port_stat[port].vct_compleat_ok = 1;

	DEBUG_MSG(VCT_DBG,"vct len RX %d  TX %d \r\n",LengthRX,LengthTX);

	return 0;
	}
return 0;
}




uint32_t InDiscardsFrameCount(uint8_t Port){
uint32_t InCnt;
	//ETH_WritePHYRegister(0x1B,0x04,0x03);// disable PPU
	//Tmp=ETH_ReadPHYRegister(GlobalRegisters,0x1D);
	//ETH_WritePHYRegister(GlobalRegisters,0x1D,((Tmp & NoFlushAllMAsk) | NoFlushAllMAsk2));

	InCnt=ETH_ReadPHYRegister(0x10+Port,0x11)<<16 | ETH_ReadPHYRegister(0x10+Port,0x10);
	//ETH_WritePHYRegister(GlobalRegisters,0x1D,Tmp);
	return InCnt;
}

uint16_t InFilteredFrameCount(uint8_t Port){
uint16_t InCnt;
	//ETH_WritePHYRegister(0x1B,0x04,0x8000);// disable PPU
	InCnt=ETH_ReadPHYRegister(0x10+Port,0x12);
	return InCnt;
}

uint16_t OutFilteredFrameCount(uint8_t Port){
uint16_t OutCnt/*,Tmp*/;
	//Tmp=ETH_ReadPHYRegister(GlobalRegisters,0x1D);
	//ETH_WritePHYRegister(GlobalRegisters,0x1D,((Tmp & NoFlushAllMAsk)/* | NoFlushAllMAsk2*/));

	//ETH_WritePHYRegister(0x1B,0x04,0x8000);// disable PPU
	OutCnt=ETH_ReadPHYRegister(0x10+Port,0x13);
	//ETH_WritePHYRegister(GlobalRegisters,0x1D,Tmp);
	return OutCnt;
}


/*status returns function*/
/*возвращаем значения параметров порта*/
uint8_t  PortStateInfo(uint8_t Port){
uint16_t Tmp;
	if(get_marvell_id()==DEV_98DX316){
		return Salsa2_ReadRegField(PORT_MAC_CTRL_REG_P0+0x400*Port,0,1);
	}
	else{
		Tmp=ETH_ReadPHYRegister(0x10+Port,0x04);
		//Tmp &=0xFFFC;
		Tmp &=0x03;
		if(Tmp==0x03)
			return 1;//1-enable
		if(Tmp==0x02)
			return 2;//1-learning
		if(Tmp==0x01)
			return 3;//1-blocked
		if(Tmp==0x00)
			return 0;//1-disable
		else
			return 0;
	}

}

//get port duplex state
uint8_t PortDuplexInfo(uint8_t Port){
uint16_t Tmp;
	if(get_marvell_id()==DEV_98DX316){
		if(Salsa2_ReadRegField(PORT_STATUS_REG0+0x400*Port,3,1))
			return 1;
		else
			return 0;
	}
	else{
		Tmp=ETH_ReadPHYRegister(0x10+Port,0x00);
		Tmp &= 0x400;
		if(Tmp)
			return 1; // 1- full duplex
		else
			return 0;	// 0-half duplex
	}
}

uint8_t PortSpeedInfo(uint8_t Port){
uint16_t Tmp;
	if(get_marvell_id()==DEV_98DX316){
		Tmp = Salsa2_ReadRegField(PORT_STATUS_REG0+0x400*Port,1,2);
		if(Tmp==0x00)
			return 1;//10mb
		else if(Tmp==0x02)
			return 2;//100mb
		else if(Tmp==0x01)
			return 3;//1000mb
		else
			return 0;
	}
	else{
		Tmp=ETH_ReadPHYRegister(0x10+Port,0x00)>>8;
		Tmp &= 0x03;
		if(Tmp==0) return 1; // 1- 10m
		if(Tmp==1) return 2; // 1- 100m
		if(Tmp==2) return 3; // 1- 1000m
		else return 0;	// 0-error
	}
}

uint8_t PortFlowControlInfo(uint8_t Port){
uint16_t Tmp=0;
//uint8_t Flow[10];
	if(get_marvell_id()==DEV_98DX316){
		Tmp = Salsa2_ReadRegField(PORT_STATUS_REG0+0x400*Port,6,2);
		if(Tmp == 0)
			return 0;
		else
			return 1;
	}
	else{
		Tmp=ETH_ReadPHYRegister(0x10+Port,0x00);
		Tmp &= 0x10;
		if(Tmp==0) return 0;
		else
			return 1;	// disable
	}
}





/************************************************************************************/
/*          начальная установка настроек портов при включении                       */
/************************************************************************************/
void SwitchPortSet(void){
u16 tmp;
int hport = 0;
port_sett_t port_sett;
	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){

		/*настройка скорости, дуплекса и FC*/
		for(u8 i=0;i<(ALL_PORT_NUM);i++){
			get_port_config(i,&port_sett);
			switch_port_config(&port_sett);
		}

		/*Port Based VLAN Set*/
		pbvlan_setup();

		/*speed limit*/
		RateLimitConfig();

		/**** QoS ****/
		qos_set();

		/*802.1Q vlan*/
		VLAN_setup();


		set_mac_filtering();

		for(uint8_t i=0;i<MV_PORT_NUM;i++){
			smi_set_port_dbnum_low(i, 0);
		}
		smi_flush_db(0);

		/*настройка SFP портов*/
		for(u8 i=0;i<(ALL_PORT_NUM);i++){
			if(is_fiber(i)){
				hport = L2F_port_conv(i);
				if(get_port_sett_state(i)==DISABLE){
					tmp = 0x413;//link forced down
					ETH_WritePHYRegister(0x10+hport,0x01,tmp);

				}
				else{
					tmp = 0x403;//link auto
					ETH_WritePHYRegister(0x10+hport,0x01,tmp);
					//printf("fiber up %x %x\r\n",0x10+hport,tmp);
				}
			}
		}
	}
	else if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
		/*настройка скорости, дуплекса и FC*/

		//set cooper speed
		for(u8 i=0;i<(ALL_PORT_NUM);i++){
			get_port_config(i,&port_sett);
			switch_port_config(&port_sett);
		}


		/*Port Based VLAN Set*/
		pbvlan_setup();

		//set fiber speed
		if(speed_select() == 1){
			psw1g_fiber_speed(100);
		}
		else
		{
			psw1g_fiber_speed(1000);
		}

		/*speed limit*/
		PSW1G_RateLimitConfig();

		/**** QoS ****/
		qos_set();

		/*802.1Q vlan*/
		if(get_vlan_sett_state() == 1){
			VLAN_setup();
		}

		set_mac_filtering();

		for(uint8_t i=0;i<MV_PORT_NUM;i++){
			smi_set_port_dbnum_low(i, 0);
			smi_flush_db(i);
		}

	}

	else if	(get_marvell_id() == DEV_98DX316){


		//disable device
		//Salsa2_WriteReg(0x00000000,0x00006002);
		//Salsa2_WriteReg(0x00000000,0x0000600A);
		vTaskDelay(1000*MSEC);


		//set VlanLookupMode
		Salsa2_WriteRegField(MAC_TABLE_CONROL,4,1,1);//IVL

		//set cooper speed
		for(u8 i=0;i<(ALL_PORT_NUM);i++){
			get_port_config(i,&port_sett);
			switch_port_config(&port_sett);
		}

		/*speed limit*/
		SWU_RateLimitConfig();

		/**** QoS ****/
		SWU_qos_set();

		/*802.1Q vlan*/
		SWU_VLAN_setup();

		/*Port Based VLAN Set*/
		//SWU_pbvlan_setup();

		//Link Aggregation
		//SWU_link_aggregation_config();

		//Port Mirroring
		SWU_port_mirroring_config();

		//enable device
		Salsa2_WriteReg(0x00000000,0x00006003);
	}
}


/*перевод из логического обозначения портов в физическое
 * 0->0
 * 1->2
 * 2->4
 * 3->6
 * 4->8
 * 5->9
 * */
int L2F_port_conv(uint8_t port){
uint8_t f_port=0;
	if((get_dev_type() == DEV_PSW2G)||(get_dev_type() == DEV_PSW2GUPS)||(get_dev_type() == DEV_PSW2GPLUS)||(get_dev_type() == DEV_PSW2G4F)||(get_dev_type() == DEV_PSW2G4FUPS)){
		switch(port){
			case 0: f_port=0; break;
			case 1: f_port=2; break;
			case 2: f_port=4; break;
			case 3: f_port=6; break;
			case 4: f_port=8; break;
			case 5: f_port=9; break;
			default: return -1;
		}
	}
	else if(get_dev_type() == DEV_PSW2G6F){
		switch(port){
			case 0: f_port=0; break;
			case 1: f_port=2; break;
			case 2: f_port=4; break;
			case 3: f_port=5; break;
			case 4: f_port=6; break;
			case 5: f_port=7; break;
			case 6: f_port=8; break;
			case 7: f_port=9; break;
			default: return -1;
		}
	}
	else if(get_dev_type() == DEV_PSW2G8F){
		switch(port){
			case 0: f_port=0; break;
			case 1: f_port=1; break;
			case 2: f_port=2; break;
			case 3: f_port=3; break;
			case 4: f_port=4; break;
			case 5: f_port=5; break;
			case 6: f_port=6; break;
			case 7: f_port=7; break;
			case 8: f_port=8; break;
			case 9: f_port=9; break;
			default: return -1;
		}
	}
	else if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
		switch(port){
			case 0: f_port=0; break;
			case 1: f_port=1; break;
			case 2: f_port=2; break;
			case 3: f_port=3; break;
			case 4: f_port=4; break;
			case 5: f_port=5; break;
			default: return -1;
		}
	}
	else if((get_dev_type() == DEV_PSW2G2FPLUS)||(get_dev_type() == DEV_PSW2G2FPLUSUPS)){
		switch(port){
			case 0: f_port=4; break;
			case 1: f_port=6; break;
			case 2: f_port=8; break;
			case 3: f_port=9; break;
			default: return -1;
		}
	}
	//for SWU
	else if(get_marvell_id() == DEV_98DX316){
		switch(port){
			case 0: f_port=0; break;
			case 1: f_port=1; break;
			case 2: f_port=2; break;
			case 3: f_port=3; break;
			case 4: f_port=4; break;
			case 5: f_port=5; break;
			case 6: f_port=6; break;
			case 7: f_port=7; break;
			case 8: f_port=8; break;
			case 9: f_port=9; break;
			case 10: f_port=10; break;
			case 11: f_port=11; break;
			case 12: f_port=12; break;
			case 13: f_port=13; break;
			case 14: f_port=14; break;
			case 15: f_port=15; break;

			default: return port;
		}
	}
return f_port;
}

/*перевод из физического обозначения портов в логическое*/

int F2L_port_conv(uint8_t f_port){
uint8_t port=0;
 if((get_dev_type() == DEV_PSW2G)||(get_dev_type() == DEV_PSW2GUPS)||(get_dev_type() == DEV_PSW2GPLUS)||(get_dev_type() == DEV_PSW2G4F)||(get_dev_type() == DEV_PSW2G4FUPS)){
	switch(f_port){
		case 0: port=0; break;
		case 2: port=1; break;
		case 4: port=2; break;
		case 6: port=3; break;
		case 8: port=4; break;
		case 9: port=5; break;
		default: return -1;
	}
 }
	 if(get_dev_type() == DEV_PSW2G6F){
		switch(f_port){
			case 0: port=0; break;
			case 2: port=1; break;
			case 4: port=2; break;
			case 5: port=3; break;
			case 6: port=4; break;
			case 7: port=5; break;
			case 8: port=6; break;
			case 9: port=7; break;
			default: return -1;
		}
	 }
	 if(get_dev_type() == DEV_PSW2G8F){
		switch(f_port){
			case 0: port=0; break;
			case 1: port=1; break;
			case 2: port=2; break;
			case 3: port=3; break;
			case 4: port=4; break;
			case 5: port=5; break;
			case 6: port=6; break;
			case 7: port=7; break;
			case 8: port=8; break;
			case 9: port=9; break;
			default: return -1;
		}
	 }
	if((get_dev_type() == DEV_PSW1G4F)||(get_dev_type() == DEV_PSW1G4FUPS)){
		switch(f_port){
			case 0: port=0; break;
			case 1: port=1; break;
			case 2: port=2; break;
			case 3: port=3; break;
			case 4: port=4; break;
			case 5: port=5; break;
			default: return -1;
		}
	}
	if((get_dev_type() == DEV_PSW2G2FPLUS)||(get_dev_type() == DEV_PSW2G2FPLUSUPS)){
		switch(f_port){
			case 4: port=0; break;
			case 6: port=1; break;
			case 8: port=2; break;
			case 9: port=3; break;
			default: return -1;
		}
	 }


	if(get_marvell_id() == DEV_98DX316){
		switch(f_port){
			case 0: port=0; break;
			case 1: port=1; break;
			case 2: port=2; break;
			case 3: port=3; break;
			case 4: port=4; break;
			case 5: port=5; break;
			case 6: port=6; break;
			case 7: port=7; break;
			case 8: port=8; break;
			case 9: port=9; break;
			case 10: port=10; break;
			case 11: port=11; break;
			case 12: port=12; break;
			case 13: port=13; break;
			case 14: port=14; break;
			case 15: port=15; break;
			default: return -1;
		}
	 }
return port;
}


u16 get_marvell_id(void){
static u16 vers = 0;
	if((vers != DEV_88E095)&&(vers != DEV_88E097)&&(vers != DEV_88E096)&&(vers != DEV_88E6176)&&(vers != DEV_88E6240)&&(vers!=DEV_98DX316)){
		hwGetPortRegField(0,QD_REG_SWITCH_ID,4,12,&vers);
		if((vers == DEV_88E095)||(vers == DEV_88E097)||(vers == DEV_88E096)||(vers == DEV_88E6176)||(vers == DEV_88E6240))
			return vers;
		else{
			//if managment interface
			if(Salsa2_ReadRegField(DEVICE_ID_REG,4,16)==0x0D14){
				vers = DEV_98DX316;
				return vers;
			}
		}


		return 1;
	}
	else
		return vers;
}


void ppu_disable(void){
u16 ln;
	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){
		//ppu_disable
		ln=ETH_ReadPHYRegister(0x1B,0x04);
		ln &=~0x4000;//PPu disable
		ETH_WritePHYRegister(0x1B,0x04,ln);
	}
}

void wait_mv_ready(void){
u32 TimeOut;
	TimeOut=100000;
	while(get_marvell_id() == 1){
		TimeOut--;
		if(!TimeOut)
			break;
	}
	if(get_marvell_id() == 1){
		//не работает мнтерфейс
		ADD_ALARM(ERROR_MARVEL_START);
		DEBUG_MSG(PRINTF_DEB,"Error! Switch ID detection fail\r\n");
	}

}

//настройка прерываний от MARVELL
void marvell_int_cfg(void){
uint16_t tmp=0;

	//set ports interrupt
	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){
		//set global interrupts
		tmp = (GT_ATU_PROB | GT_PHY_INTERRUPT);//AtuProbIntEn
		ETH_WritePHYRegister(GlobalRegister,0x04,tmp);//phy int enable

		for(u8 i=0;i<(COOPER_PORT_NUM);i++){
			tmp = GT_LINK_STATUS_CHANGED;
			ETH_WritePHYRegister(L2F_port_conv(i),QD_PHY_INT_ENABLE_REG, tmp);
		}
	}
	else if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){

		//set global interrupts
		tmp = ETH_ReadPHYRegister(0x1B,0x04);
		tmp |= (GT_ATU_PROB | GT_DEVICE_INT);//AtuProbIntEn + Device Int
		ETH_WritePHYRegister(0x1B,0x04,tmp);//phy int enable

		//set phy int
		tmp = ETH_ReadPHYRegister(0x1C,0x01);
		tmp |= (0x1F | (1<<11));//0-5 ports + Serdes
		ETH_WritePHYRegister(0x1C,0x01,tmp);//phy int enable


		for(u8 i=0;i<(COOPER_PORT_NUM);i++){
			tmp = GT_LINK_STATUS_CHANGED;
			ETH_WriteIndirectPHYReg(L2F_port_conv(i), PAGE0, QD_PHY_INT_ENABLE_REG,tmp);
		}
		//for fiber port
		//for(u8 i=COOPER_PORT_NUM;i<(ALL_PORT_NUM);i++){
			tmp = GT_LINK_STATUS_CHANGED;
			ETH_WriteIndirectPHYReg(/*L2F_port_conv(i)*/0x0F, PAGE1, QD_PHY_INT_ENABLE_REG,tmp);
		//}
	}
	else if(get_marvell_id() == DEV_98DX316){
		//настройка прерываний по изменению линка
		//for(u8 i=0;i<(ALL_PORT_NUM);i++){
		//	Salsa2_WriteRegField(PORT_INTERRUPT_MASK_P0+0x400*L2F_port_conv(i),1,0x7FFE,15);
		//}
	}


	//clear interrupts bit
	/*ETH_ReadPHYRegister(GlobalRegister,0x00);
	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){
		for(uint8_t i=0;i<11;i++)
			ETH_ReadPHYRegister(i,QD_PHY_INT_STATUS_REG);
	}
	if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
		for(uint8_t i=0;i<7;i++)
			ETH_ReadIndirectPHYReg(i,PAGE0,QD_PHY_INT_STATUS_REG);
	}*/

}

void ETH_LINK_int_clear(void){
	ETH_ReadPHYRegister(GlobalRegister,0x00);//clear interrupt bit
	if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){
		for(uint8_t i=0;i<11;i++)
			ETH_ReadPHYRegister(i,QD_PHY_INT_STATUS_REG);
	}
	if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
		for(uint8_t i=0;i<7;i++)
			ETH_ReadIndirectPHYReg(i,PAGE0,QD_PHY_INT_STATUS_REG);
	}
}




GT_STATUS hwGetGlobal2RegField
(
    IN  GT_U8    regAddr,
    IN  GT_U8    fieldOffset,
    IN  GT_U8    fieldLength,
    OUT GT_U16   *data
)
{
    GT_U16 mask;            /* Bits mask to be read */
    GT_U16 tmpData;
    GT_STATUS   retVal;
    GT_U8       phyAddr;

    phyAddr = CALC_SMI_DEV_ADDR(0, GLOBAL2_REG_ACCESS);
    if (phyAddr == 0xFF)
    {
        return GT_BAD_PARAM;
    }

    retVal = miiSmiIfReadRegister(phyAddr,regAddr,&tmpData);

     if(retVal != GT_OK)
        return retVal;

    CALC_MASK(fieldOffset,fieldLength,mask);
    tmpData = (tmpData & mask) >> fieldOffset;
    *data = tmpData;
    DEBUG_MSG(DEBUG_QD,"Read from global 2 register: regAddr 0x%x, fOff %d, fLen %d, data 0x%x.\r\n",
    		regAddr,fieldOffset,fieldLength,*data);

    return GT_OK;
}


/*******************************************************************************
* hwSetGlobal2RegField
*
* DESCRIPTION:
*       This function writes to specified field in a switch's global 2 register.
*
* INPUTS:
*       regAddr     - The register's address.
*       fieldOffset - The field start bit index. (0 - 15)
*       fieldLength - Number of bits to write.
*       data        - Data to be written.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       GT_OK on success, or
*       GT_FAIL otherwise.
*
* COMMENTS:
*       1.  The sum of fieldOffset & fieldLength parameters must be smaller-
*           equal to 16.
*
*******************************************************************************/
GT_STATUS hwSetGlobal2RegField
(
    IN  GT_U8    regAddr,
    IN  GT_U8    fieldOffset,
    IN  GT_U8    fieldLength,
    IN  GT_U16   data
)
{
    GT_U16 mask;
    GT_U16 tmpData;
    GT_STATUS   retVal;
    GT_U8       phyAddr;

    phyAddr = CALC_SMI_DEV_ADDR(0, GLOBAL2_REG_ACCESS);
    if (phyAddr == 0xFF)
    {
        return GT_BAD_PARAM;
    }

    retVal = miiSmiIfReadRegister(phyAddr,regAddr,&tmpData);

    if(retVal != GT_OK)
    {
        return retVal;
    }

    CALC_MASK(fieldOffset,fieldLength,mask);

    /* Set the desired bits to 0.                       */
    tmpData &= ~mask;
    /* Set the given data into the above reset bits.    */
    tmpData |= ((data << fieldOffset) & mask);

    DEBUG_MSG(DEBUG_QD,"Write to global 2 register: regAddr 0x%x, fieldOff %d, fieldLen %d, data 0x%x.\r\n",
    		regAddr,fieldOffset,fieldLength,data);

    //retVal = miiSmiIfWriteRegister(phyAddr,regAddr,tmpData);
    ETH_WritePHYRegister(phyAddr,regAddr,tmpData);

    return retVal;
}


GT_STATUS hwReadGlobalReg
(
   IN  GT_U8    regAddr,
    OUT GT_U16   *data
)
{
    GT_U8       phyAddr;
    GT_STATUS   retVal;

    phyAddr = CALC_SMI_DEV_ADDR(0, GLOBAL_REG_ACCESS);

    //gtSemTake(dev,dev->multiAddrSem,OS_WAIT_FOREVER);

    retVal = miiSmiIfReadRegister(phyAddr,regAddr,data);

    //gtSemGive(dev,dev->multiAddrSem);

    DEBUG_MSG(DEBUG_QD,"read from global register: phyAddr 0x%x, regAddr 0x%x, data 0x%x.\r\n",
    		phyAddr,regAddr,*data);
    return retVal;
}



GT_STATUS hwWriteGlobalReg
(
    IN  GT_U8    regAddr,
    IN  GT_U16   data
)
{
    GT_U8   phyAddr;
    GT_STATUS   retVal;

    phyAddr = CALC_SMI_DEV_ADDR(0, GLOBAL_REG_ACCESS);

    DEBUG_MSG(DEBUG_QD,"Write to global register: phyAddr 0x%x, regAddr 0x%x, data 0x%x.\r\n",
    		phyAddr,regAddr,data);

    //gtSemTake(dev,dev->multiAddrSem,OS_WAIT_FOREVER);

    //retVal = miiSmiIfWriteRegister(phyAddr,regAddr,data);
    ETH_WritePHYRegister(phyAddr,regAddr,data);
    //gtSemGive(dev,dev->multiAddrSem);
    retVal = 0;
    return retVal;
}

void * gtMemSet
(
    IN void * start,
    IN int    symbol,
    IN GT_U32 size
)
{
    GT_U32 i;
    char* buf;

    buf = (char*)start;

    for(i=0; i<size; i++)
    {
        *buf++ = (char)symbol;
    }

    return start;
}

GT_STATUS hwWriteGlobal2Reg
(
    IN  GT_U8    regAddr,
    IN  GT_U16   data
)
{
    GT_U8   phyAddr;
    GT_STATUS   retVal;
    phyAddr = CALC_SMI_DEV_ADDR(0, GLOBAL2_REG_ACCESS);

    DEBUG_MSG(DEBUG_QD,"Write to global 2 register: phyAddr 0x%x, regAddr 0x%x, data 0x%x.\r\n",
    		phyAddr,regAddr,data);
    ETH_WritePHYRegister(phyAddr,regAddr,data);
    retVal = 0;
    return retVal;
}


GT_STATUS hwReadGlobal2Reg
(
   IN  GT_U8    regAddr,
    OUT GT_U16   *data
)
{
    GT_U8       phyAddr;
    GT_STATUS   retVal;

    phyAddr = CALC_SMI_DEV_ADDR(0, GLOBAL2_REG_ACCESS);


    retVal = miiSmiIfReadRegister(phyAddr,regAddr,data);

   DEBUG_MSG(DEBUG_QD,"read from global 2 register: phyAddr 0x%x, regAddr 0x%x, data 0x%x.\r\n",
    		phyAddr,regAddr,*data);
    return retVal;
}

//void port_statistics_processing(void){
//u8 i;
////static u8 run_timer=0;
//
////	if(run_timer == 0){
////		timer_set(&timer_, 10000 * MSEC  );
////		run_timer = 1;
////	}
//
//	//if (/*(timer_expired(&timer_))&&(run_timer == 1) ||*/ port_statistics_start == 1){
//		for(i=0;i<(COOPER_PORT_NUM+FIBER_PORT_NUM);i++){
//			if(get_port_link(i)){
//				dev.port_stat[i].rx_good 	= read_stats_cnt(i,RX_GOOD);
//				dev.port_stat[i].rx_bad 		= read_stats_cnt(i,RX_BAD);
//				dev.port_stat[i].tx_good 	= read_stats_cnt(i,TX_GOOD);
//				dev.port_stat[i].rx_discard 	= InDiscardsFrameCount(L2F_port_conv(i));
//				dev.port_stat[i].rx_filtered =InFilteredFrameCount(L2F_port_conv(i));
//				dev.port_stat[i].tx_filtered = OutFilteredFrameCount(L2F_port_conv(i));
//			}
//		}
//		//timer_set(&timer_, 10000 * MSEC  );
//		//port_statistics_start = 0;
//	//}
//
//}


/*обращение к регистрам PHY через косвенную адресацию*/
uint16_t ETH_ReadIndirectPHYReg(uint16_t PHYAddress,uint16_t PHYPage, uint16_t PHYReg){
u16 tmp;
	//1. write page num
	//wait if busy
	tmp  = 0x8000;
	while(tmp & 0x8000){
		tmp = ETH_ReadPHYRegister(0x1C,0x18);
	}
	//use clause 22
	ETH_WritePHYRegister(0x1C,0x19,PHYPage);
	tmp = 0;
	tmp = (1<<15) | (1<<12) | (0x01<<10) | ((PHYAddress & 0x1F) <<5) | (PAGE_ADDR_REG &0x1F);
	ETH_WritePHYRegister(0x1C,0x18,tmp);

	//2. read data
	//wait if busy
	tmp  = 0x8000;
	while(tmp & 0x8000){
		tmp = ETH_ReadPHYRegister(0x1C,0x18);
	}
	//use clause 22
	tmp = 0;
	tmp = 1<<15 | 1<<12 | 0x02<<10 | (PHYAddress & 0x1F) <<5 | (PHYReg &0x1F);
	ETH_WritePHYRegister(0x1C,0x18,tmp);
	//wait if busy
	tmp  = 0x8000;
	while(tmp & 0x8000){
		tmp = ETH_ReadPHYRegister(0x1C,0x18);
	}
	//read result
	tmp = ETH_ReadPHYRegister(0x1C,0x19);

    DEBUG_MSG(DEBUG_QD,"read from phy register: pageAddr 0x%x, phyAddr 0x%x, regAddr 0x%x, data 0x%x.\r\n",
    		PHYPage,PHYAddress,PHYReg,tmp);

	return tmp;
}

/*обращение к регистрам PHY через косвенную адресацию*/
void ETH_WriteIndirectPHYReg(uint16_t PHYAddress,uint16_t PHYPage, uint16_t PHYReg, uint16_t Data){
	u16 tmp;
	//1. write page num
	//wait if busy
	tmp  = 0x8000;
	while(tmp & 0x8000){
		tmp = ETH_ReadPHYRegister(0x1C,0x18);
	}
	//use clause 22
	ETH_WritePHYRegister(0x1C,0x19,PHYPage);
	tmp = 1<<15 | 1<<12 | 0x01<<10 | (PHYAddress & 0x1F) <<5 | (PAGE_ADDR_REG & 0x1F);
	ETH_WritePHYRegister(0x1C,0x18,tmp);

	//2. write data
	//wait if busy
	tmp  = 0x8000;
	while(tmp & 0x8000){
		tmp = ETH_ReadPHYRegister(0x1C,0x18);
	}
	//set data
	ETH_WritePHYRegister(0x1C,0x19,Data);
	//use clause 22
	//set addr and start
	tmp = 1<<15 | 1<<12 | 0x01<<10 | (PHYAddress & 0x1F) <<5 | (PHYReg & 0x1F);
	ETH_WritePHYRegister(0x1C,0x18,tmp);
	//wait if busy
	tmp  = 0x8000;
	while(tmp & 0x8000){
		tmp = ETH_ReadPHYRegister(0x1C,0x18);
	}
    DEBUG_MSG(DEBUG_QD,"Write to phy register: pageAddr 0x%x, phyAddr 0x%x, regAddr 0x%x, data 0x%x.\r\n",
    		PHYPage,PHYAddress,PHYReg,Data);
}


/*обращение к регистрам Salsa2*/

/*write data to internal reg
 * addr - reg addr
 * data - data
 * return - operation result*/
u8 Salsa2_WriteReg(u32 addr,u32 data){
u32 max_cnt;
taskENTER_CRITICAL();
	ETH_WritePHYRegister(SALSA2_PHY_ADDR,SMI_WRITE_ADDR_MSB,(u16)(addr>>16));
	ETH_WritePHYRegister(SALSA2_PHY_ADDR,SMI_WRITE_ADDR_LSB,(u16)(addr));
	ETH_WritePHYRegister(SALSA2_PHY_ADDR,SMI_WRITE_DATA_MSB,(u16)(data>>16));
	ETH_WritePHYRegister(SALSA2_PHY_ADDR,SMI_WRITE_DATA_LSB,(u16)(data));
	max_cnt = SALSA2_MAX_CNT;
	while((ETH_ReadPHYRegister(SALSA2_PHY_ADDR,SMI_RW_STATUS) & 2) == 0 && max_cnt){
		max_cnt--;
	}
	if(max_cnt == 0)
		DEBUG_MSG(DEBUG_QD,"Salsa2_WriteReg: Timeout\r\n");
taskEXIT_CRITICAL();
	//DEBUG_MSG(DEBUG_QD,"Salsa2_WriteReg: phyAddr: 0x%lX, data: 0x%lX\r\n", addr,data);
	return 0;
}


/*read data to internal reg
 * addr - reg addr
 * return - data*/
u32 Salsa2_ReadReg(u32 addr){
u16 data_lsb,data_msb;
u32 max_cnt;
taskENTER_CRITICAL();
	ETH_WritePHYRegister(SALSA2_PHY_ADDR,SMI_READ_ADDR_MSB,(u16)(addr>>16));
	ETH_WritePHYRegister(SALSA2_PHY_ADDR,SMI_READ_ADDR_LSB,(u16)(addr));
	max_cnt = SALSA2_MAX_CNT;
	while((ETH_ReadPHYRegister(SALSA2_PHY_ADDR,SMI_RW_STATUS) & (1<<0)) == 0 && max_cnt){
		max_cnt--;
	}
	if(max_cnt == 0)
		DEBUG_MSG(DEBUG_QD,"Salsa2_ReadReg: Timeout\r\n");
	data_msb = ETH_ReadPHYRegister(SALSA2_PHY_ADDR,SMI_READ_DATA_MSB);
	data_lsb = ETH_ReadPHYRegister(SALSA2_PHY_ADDR,SMI_READ_DATA_LSB);
taskEXIT_CRITICAL();
	//DEBUG_MSG(DEBUG_QD,"Salsa2_ReadReg: phyAddr 0x%x, data 0x%x\r\n",
	//		 addr,(u32)(data_msb<<16 | data_lsb));


	return (data_msb<<16 | data_lsb);
}

/*write reg field*/
u8 Salsa2_WriteRegField(u32 addr,u8 offset, u32 data,u8 len){
u32 temp,mask;
	temp = Salsa2_ReadReg(addr);
	CALC_MASK32(offset,len,mask);
    /* Set the desired bits to 0.                       */
    temp &= ~mask;
    /* Set the given data into the above reset bits.    */
    temp |= ((data << offset) & mask);
    temp = Salsa2_WriteReg(addr,temp);
    DEBUG_MSG(DEBUG_QD,"Salsa2_WriteRegField: phy: 0x%lX, offset:0x%X data: 0x%lX len:%d\r\n", addr,offset,data,len);
    return 0;
}

/*read reg field*/
u32 Salsa2_ReadRegField(u32 addr,u8 offset,u8 len){
u32 temp,mask;
	temp = Salsa2_ReadReg(addr);
    CALC_MASK32(offset,len,mask);
    return (temp & mask) >> offset;
}


/*config PHY Address for external PHY*/
/* port - physycal port 0-23
 * addr - 5 bit phy addr*/
void Salsa2_configPhyAddres(u8 port,u32 addr){
u32 phy_addr;
u8 offset;
	//calculate reg addr
	phy_addr = PHY_ADDR_REG0 + (port/6)*0x800000;
	//calculate offset
	offset = (port%6)*5;
	//write
	Salsa2_WriteRegField(phy_addr,offset,addr,5);
}

/*Write to external phy reg*/
void Salsa2_WritePhyReg(u8 port,u8 PhyAddr, u8 PhyReg, u16 data){
u32 reg,reg_addr;
u32 max_cnt;
	if(port<12)
		reg_addr = SMI0_MNGT_REG;
	else
		reg_addr = SMI1_MNGT_REG;

	//wait if bussy
	reg = Salsa2_ReadReg(reg_addr);
	max_cnt = SALSA2_MAX_CNT;
	while((reg & 0x20000000) && max_cnt){
		reg = Salsa2_ReadReg(reg_addr);
		max_cnt--;
	}
	//set opcode
	//0 - write
	reg = 0;
	//set phy addr
	reg |= ((PhyAddr & 0x1F)<<16);
	//set reg addr
	reg |= ((PhyReg & 0x1F)<<21);
	//set data
	reg |= data;
	Salsa2_WriteReg(reg_addr,reg);

	//DEBUG_MSG(DEBUG_QD,"Salsa2_WritePhyReg: phyAddr 0x%x,phyRegs 0x%x, data 0x%x\r\n",
	//		PhyAddr,PhyReg,data);
}

/*Read external phy reg*/
u16 Salsa2_ReadPhyReg(u8 port,u8 PhyAddr, u8 PhyReg){
u32 reg,reg_addr;
u32 max_cnt;
	if(port<12)
		reg_addr = SMI0_MNGT_REG;
	else
		reg_addr = SMI1_MNGT_REG;
	//wait if bussy
	reg = Salsa2_ReadReg(reg_addr);
	max_cnt = SALSA2_MAX_CNT;
	while((reg & 0x10000000) && max_cnt){
		reg = Salsa2_ReadReg(reg_addr);
		max_cnt--;
	}
	//set opcode 1 - read
	reg = 0x4000000;
	//set phy addr
	reg |= ((PhyAddr & 0x1F)<<16);
	//set reg addr
	reg |= ((PhyReg & 0x1F)<<21);
	Salsa2_WriteReg(reg_addr,reg);
	//wait if ReadVAlid==0
	reg = Salsa2_ReadReg(reg_addr);
	while((reg & 0x8000000) == 0){
		reg = Salsa2_ReadReg(reg_addr);
	}

	//bug fix!!!!
	//reg = reg>>1;

	//DEBUG_MSG(DEBUG_QD,"Salsa2_ReadPhyReg: phyAddr 0x%x,phyRegs 0x%x, data 0x%x\r\n",
	//		PhyAddr,PhyReg,(u16)reg);

	return (u16)(reg & 0xFFFF);
}




void SWU_start_process(void){
	//GPIO_InitTypeDef GPIO_InitStructure;

	DEBUG_MSG(PRINTF_DEB,"SWU_start_process\r\n");

	//config lines

	/*//CPU_EN - CPU_TXD[3]
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOB, &GPIO_InitStructure);


	//SD ref sel - CPU_TXD[2]
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOB, &GPIO_InitStructure);


	//SD ref sel - CPU_TXD[1]
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOB, &GPIO_InitStructure);


	//сброс чипа
	DEBUG_MSG(PRINTF_DEB,"SWU_start_process: reboot...\r\n");
	GPIO_ResetBits(LINE_SW_RESET_GPIO, LINE_SW_RESET_PIN);
	vTaskDelay(100*MSEC);
	GPIO_SetBits(LINE_SW_RESET_GPIO, LINE_SW_RESET_PIN);
*/
	//ждем установки
	//DEBUG_MSG(PRINTF_DEB,"SWU_start_process: wait DEV_INIT_DONE...\r\n");
	//while(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_9)==Bit_RESET);
	//DEBUG_MSG(PRINTF_DEB,"SWU_start_process: DEV_INIT_DONE ok!\r\n");
}



//установка Port MAC Learning и MAC Address Binding Table
void set_mac_filtering(void){
u8 hw_port;//,port;
u16 tmp;
//u8 mac[6];
//	//Read Globasl Status Reg
//	ETH_ReadPHYRegister(0x1B,0x00);
//
//	//set interrupt
//	tmp = ETH_ReadPHYRegister(0x1B,0x04);//Global Control reg
//	tmp |= 0x08;//AtuProbIntEn
//	ETH_WritePHYRegister(0x1B,0x04,tmp);

	/*if(get_mac_learn_cpu() == DISABLE){
		//
		tmp = ETH_ReadPHYRegister(0x10+CPU_PORT,0x04);
		tmp &=~0x04;
		ETH_WritePHYRegister(0x10+CPU_PORT,0x04,tmp);

		tmp = ETH_ReadPHYRegister(0x10+CPU_PORT,0x08);
		tmp &=~0x40;
		ETH_WritePHYRegister(0x10+CPU_PORT, 0x08, tmp);
	}
	else{*/
		tmp = ETH_ReadPHYRegister(0x10+CPU_PORT,0x04);
		tmp |= 0x04;
		ETH_WritePHYRegister(0x10+CPU_PORT,0x04,tmp);

		tmp = ETH_ReadPHYRegister(0x10+CPU_PORT,0x08);
		tmp |= 0x40;
		ETH_WritePHYRegister(0x10+CPU_PORT, 0x08, tmp);
	/*}*/


	//set mac address learning
	for(u8 i=0;i<(ALL_PORT_NUM);i++){
		hw_port = L2F_port_conv(i);
		//если отключено обучение, то mac filtering
		if(get_mac_filter_state(i)!=DISABLE){
			//disable forward unknown unicast
			if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
					(get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
				tmp = ETH_ReadPHYRegister(0x10+hw_port,0x04);
				//tmp &=~0x04;
				tmp |= 0x4000;//enable drop on lock
				ETH_WritePHYRegister(0x10+hw_port,0x04,tmp);

				//Locked Port
				tmp = ETH_ReadPHYRegister(0x10+hw_port,0x0B);
				tmp |= 0x2000;//set locked port
				ETH_WritePHYRegister(0x10+hw_port,0x0B,tmp);
			}

			//disable forwarding unknown multicast
			/*if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){
				//disable forwarding unknown multicast
				//tmp = ETH_ReadPHYRegister(0x10+hw_port,0x08);
				//tmp &=~0x40;
				//ETH_WritePHYRegister(0x10+hw_port, 0x08, tmp);
			}
			else if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
				//disable forwarding unknown multicast
				//tmp = ETH_ReadPHYRegister(0x10+hw_port,0x04);
				//tmp &=~0x08;
				//ETH_WritePHYRegister(0x10+hw_port, 0x04, tmp);
			}*/
		}
		else{
			//включено

			if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)
					||(get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
				//enable forward unknown unicast
				if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
						(get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
					tmp = ETH_ReadPHYRegister(0x10+hw_port,0x04);
					//tmp &=~0x04;
					tmp &=~0x4000;//disable drop on lock
					ETH_WritePHYRegister(0x10+hw_port,0x04,tmp);

					//Locked Port disable
					tmp = ETH_ReadPHYRegister(0x10+hw_port,0x0B);
					tmp &=~0x2000;//reset locked port
					ETH_WritePHYRegister(0x10+hw_port,0x0B,tmp);
				}
			}

/*
			if((get_igmp_snooping_state() == DISABLE)||(get_igmp_port_state(i) == DISABLE)){
				//enable forwarding unknown multicast
				if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)){
					//enable forwarding unknown multicast
					tmp = ETH_ReadPHYRegister(0x10+hw_port,0x08);
					tmp |= 0x40;
					ETH_WritePHYRegister(0x10+hw_port, 0x08, tmp);
				}
				else if((get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){
					//enable forwarding unknown multicast
					tmp = ETH_ReadPHYRegister(0x10+hw_port,0x04);
					tmp |= 0x08;
					ETH_WritePHYRegister(0x10+hw_port, 0x04, tmp);
				}
			}
			//PS если igmp snooping включен, то там и будем запрещать unknown multicast
			 */
		}
	}
}




/*
 *    Add a multicast MAC address into the QuaterDeck MAC Address database,
 *    where address is 01-00-18-1a-00-00 and frames with this destination has
 *    to be forwarding to Port 1, Port 2 and Port 4 (port starts from Port 0)
 *    Input - None
*/
GT_STATUS AddMacAddr(u8 *mac, u8 port)
{
    GT_STATUS status;
    GT_ATU_ENTRY macEntry;

    for(u8 i=0;i<6;i++){
    	macEntry.macAddr.arEther[i] = mac[i];
    }

    //if exist
    //macEntry.portVec = get_atu_port_vect(mac);



    macEntry.portVec = 1<<L2F_port_conv(port);



    // and add cpu port
    //if(add_cpu)
    //macEntry.portVec |=  (1<<CPU_PORT);


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

    macEntry.entryState.ucEntryState = GT_UC_STATIC;
    macEntry.entryState.mcEntryState = GT_MC_STATIC;

                                /* This address is locked and will not be aged out.
                                Refer to GT_ATU_MC_STATE in msApiDefs.h for other option.*/

    DEBUG_MSG(MAC_FILTR_DEBUG,"AddMacAddr: mac %x:%x:%x:%x:%x:%x  port vect %lu\r\n",
    		mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],macEntry.portVec);



    /*
     *    Add the MAC Address.
     */

    status = gfdbAddMacEntry(&macEntry);
    if(status != GT_OK)
    {
    	DEBUG_MSG(MAC_FILTR_DEBUG,"gfdbAddMacEntry returned fail.\r\n");
        return status;
    }

    return GT_OK;
}



GT_STATUS DelMacAddr(u8 *mac, u8 port)
{
    GT_STATUS status;
    GT_ATU_ENTRY macEntry;

    for(u8 i=0;i<6;i++){
    	macEntry.macAddr.arEther[i] = mac[i];
    }

    //if exist
    // get exist port vect
    macEntry.portVec = get_atu_port_vect(mac);;

    macEntry.portVec &= ~(1<<L2F_port_conv(port));


    //if(del_cpu)
    // and add cpu port
    macEntry.portVec &= ~(1<<CPU_PORT);


    //if only CPU port, delete entry
    if(macEntry.portVec == (1<<CPU_PORT)){
        for(u8 i=0;i<6;i++){
        	macEntry.macAddr.arEther[i] = mac[i];
        }
        DEBUG_MSG(MAC_FILTR_DEBUG,"sampleDelMulticastAddr: mac %x:%x:%x:%x:%x:%x\r\n",
        		mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
        /*
         *    Delete the given Multicast Address.
         */
        if((status = gfdbDelMacEntry(&macEntry.macAddr)) != GT_OK)
        {
        	DEBUG_MSG(MAC_FILTR_DEBUG,"gfdbDelMacEntry returned fail.\r\n");
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

    DEBUG_MSG(MAC_FILTR_DEBUG,"DelMacAddr: mac %x:%x:%x:%x:%x:%x  port vect %lu\r\n",
    		mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],macEntry.portVec);


    /*
     *    Add the MAC Address.
     */

    if(macEntry.portVec == 0){
        if((status = gfdbDelMacEntry(&macEntry.macAddr)) != GT_OK)
        {
        	DEBUG_MSG(MAC_FILTR_DEBUG,"gfdbDelMacEntry returned fail.\r\n");
            return status;
        }
        return GT_OK;
    }

    status = gfdbAddMacEntry(&macEntry);
    if(status != GT_OK)
    {
    	DEBUG_MSG(MAC_FILTR_DEBUG,"gfdbAddMacEntry returned fail.\r\n");
        return status;
    }

    return GT_OK;
}


void mac_filtring_processing(void){
u8 port;
u8 mac[6];
u16 temp;
static port_sett_t port_sett;
u8 identy;

 	//разблокировка error-disabled порта
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		if(get_mac_filter_state(i) == PORT_FILT_TEMP && dev.port_stat[i].error_dis==1){
			if(timer_expired(&dev.port_stat[i].err_dis_timer)){
				dev.port_stat[i].error_dis = 0;
				for(u8 j=0;j<MAX_BLOCKED_MAC;j++){
					if(dev.mac_blocked[j].port==i){
						dev.mac_blocked[j].age_time = 0;
					}
				}

				//restore old state
				set_port_state(i, ENABLE);
				get_port_config(i,&port_sett);
				switch_port_config(&port_sett);
				send_events_mac(EVENTS_UP_PORT1+i,0);
				return;
			}
		}
	}



	//если было прерывание
	if (GPIO_ReadInputDataBit(GPIOE,LINE_SW_INT_PIN)==Bit_RESET){

		if((get_marvell_id() == DEV_88E095)||(get_marvell_id() == DEV_88E097)||
		   (get_marvell_id() == DEV_88E6176)||(get_marvell_id() == DEV_88E6240)){

			temp = ETH_ReadPHYRegister(0x1B, 0x00);
			if(temp & (1<<3)){//ATU Problem Interrupt is active
				//read atu
				temp = ETH_ReadPHYRegister(0x1B, 0x0D);
				mac[0] = (u8)(temp>>8);
				mac[1] = (u8)(temp);

				temp = ETH_ReadPHYRegister(0x1B, 0x0E);
				mac[2] = (u8)(temp>>8);
				mac[3] = (u8)(temp);

				temp = ETH_ReadPHYRegister(0x1B, 0x0F);
				mac[4] = (u8)(temp>>8);
				mac[5] = (u8)(temp);

				temp = ETH_ReadPHYRegister(0x1B, 0x0C);
				port = F2L_port_conv((u8)(temp)&(0x0F));

				DEBUG_MSG(MAC_FILTR_DEBUG,"ATU INT %X:%X:%X:%X:%X:%X %d\r\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],port);



				//проверяем что запрос с разрешенного МАС
				identy = 0;
				for(u8 i=0;i<MAC_BIND_MAX_ENTRIES;i++){
					identy = 1;
					for(u8 j=0;j<6;j++)
						if(mac[j] != get_mac_bind_entry_mac(i,j))
							identy = 0;


					if(port != get_mac_bind_entry_port(i))
						continue;

					if(identy){
						if(get_mac_bind_entry_active(i)){
							AddMacAddr(mac,get_mac_bind_entry_port(i));
							ETH_WritePHYRegister(0x1B, 0x0B, 0xF000);//Clear Violation Data
							del_blocked_mac(mac,port);//удаляем из списка заблокированных
							return;
						}
						else{
							DelMacAddr(mac,get_mac_bind_entry_port(i));
							ETH_WritePHYRegister(0x1B, 0x0B, 0xF000);//Clear Violation Data
							return;
						}
					}
				}
				DEBUG_MSG(MAC_FILTR_DEBUG,"MAC no found\r\n");
				ETH_WritePHYRegister(0x1B, 0x0B, 0xF000);//Clear Violation Data
				ETH_LINK_int_clear();

				//если не нашли в списке с разрешенными, то добавляем в список запрещенных
				if(get_mac_filter_state(port) == MAC_FILT){
					add_blocked_mac(mac,port);
					ETH_WritePHYRegister(0x1B, 0x0B, 0xF000);//Clear Violation Data
				}
				else if((get_mac_filter_state(port) == PORT_FILT ||
						get_mac_filter_state(port) == PORT_FILT_TEMP)&&
						dev.port_stat[port].error_dis == 0){
					ETH_WritePHYRegister(0x1B, 0x0B, 0xF000);//Clear Violation Data
					add_blocked_mac(mac,port);
					dev.port_stat[port].error_dis = 1;
					send_events_u32(EVENTS_ERROR_DISABLED_PORT1+port,0);

					//disable port
					set_port_state(port, DISABLE);
					get_port_config(port,&port_sett);
					switch_port_config(&port_sett);

					if(get_mac_filter_state(port) == PORT_FILT_TEMP){
						timer_set(&dev.port_stat[port].err_dis_timer,  MAX_BLOCKED_PORT_TIME* MSEC  );
					}
				}
			}
		}

	}
}

//добавляем в список заблокированных
void add_blocked_mac(u8 *mac,u8 port){
u8
age_min,age_min_pos;

	DEBUG_MSG(MAC_FILTR_DEBUG,"add_blocked_mac %x:%x:%x:%x:%x:%x port\r\n",
			mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],port);

	//при каждом вызове уменьшаем age time
	for(u8 i=0;i<MAX_BLOCKED_MAC;i++){
		if(dev.mac_blocked[i].age_time)
			dev.mac_blocked[i].age_time--;
	}

	for(u8 i=0;i<MAX_BLOCKED_MAC;i++){
		//смотрим по существующим записям
		if(dev.mac_blocked[i].age_time){
			//если запись найдена, просто актуализируем
			if(dev.mac_blocked[i].mac[0]==mac[0] && dev.mac_blocked[i].mac[1]==mac[1] &&
			  dev.mac_blocked[i].mac[2]==mac[2] && dev.mac_blocked[i].mac[3]==mac[3] &&
			  dev.mac_blocked[i].mac[4]==mac[4] && dev.mac_blocked[i].mac[5]==mac[5] &&
			  dev.mac_blocked[i].port==port){
				if(dev.mac_blocked[i].age_time<254)
					dev.mac_blocked[i].age_time++;
				return;
			}
		}
	}


	//ищем запись с мин age time
	age_min_pos = 0;
	age_min = 255;
	for(u8 i=0;i<MAX_BLOCKED_MAC;i++){
		if(dev.mac_blocked[i].age_time<=age_min){
			age_min = dev.mac_blocked[i].age_time;
			age_min_pos = i;
		}
	}

	if(mac[0]==0&&mac[1]==0&&mac[2]==0&&mac[3]==0&&mac[4]==0&&mac[5]==0)
		return;

	//добавляем
	dev.mac_blocked[age_min_pos].port = port;
	dev.mac_blocked[age_min_pos].mac[0] = mac[0];
	dev.mac_blocked[age_min_pos].mac[1] = mac[1];
	dev.mac_blocked[age_min_pos].mac[2] = mac[2];
	dev.mac_blocked[age_min_pos].mac[3] = mac[3];
	dev.mac_blocked[age_min_pos].mac[4] = mac[4];
	dev.mac_blocked[age_min_pos].mac[5] = mac[5];
	dev.mac_blocked[age_min_pos].age_time = 255;
	send_events_mac(EVENTS_MAC_UNALLOWED_PORT1+port,mac);
}

//удаляем из списка заблокированных
void del_blocked_mac(u8 *mac,u8 port){

	DEBUG_MSG(MAC_FILTR_DEBUG,"del_blocked_mac %x:%x:%x:%x:%x:%x port\r\n",
			mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],port);

	for(u8 i=0;i<MAX_BLOCKED_MAC;i++){
		//смотрим по существующим записям
		if(dev.mac_blocked[i].age_time){
			//если запись найдена, просто актуализируем
			if(dev.mac_blocked[i].mac[0]==mac[0] && dev.mac_blocked[i].mac[1]==mac[1] &&
			  dev.mac_blocked[i].mac[2]==mac[2] && dev.mac_blocked[i].mac[3]==mac[3] &&
			  dev.mac_blocked[i].mac[4]==mac[4] && dev.mac_blocked[i].mac[5]==mac[5] &&
			  dev.mac_blocked[i].port==port){
				dev.mac_blocked[i].age_time = 0;
				return;
			}
		}
	}
}
