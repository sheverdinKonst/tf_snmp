#if 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"

#include "stm32f4xx_flash.h"
#include "stm32f4xx_rcc.h"

#include "board.h"
#include "eeprom.h"
#include "settings.h"

struct settings_t settings __attribute__ ((section (".ccmram")));


#define EEPROM_NUM_BANKS    ((EEPROM_BANKS_END - EEPROM_BANKS_START) / EEPROM_BANK_SIZE)//2 banks

#define EEPROM_VALID_BANK	0xAAAA
#define EEPROM_BACK_BANK	0xBBBB
#define EEPROM_OLD_BANK		0x0000
#define EEPROM_FREE_BANK	0xFFFF

#define EEPROM_START(bank)  (EEPROM_BANKS_START + (bank) * EEPROM_BANK_SIZE)
#define EEPROM_END			(EEPROM_BANKS_START + (cur_bank+1) * EEPROM_BANK_SIZE)

static uint16_t shadow_ram[EEPROM_BANK_SIZE/2] __attribute__ ((section (".ccmram")));
uint16_t cur_bank __attribute__ ((section (".ccmram")));

uint16_t need_update_flag[4]   __attribute__ ((section ("._boot2main")));





/*
 * Address и Length обязательно должны быть кратны двум
*/
int8_t Write_Eeprom(uint16_t Address, void *Data, uint16_t Length)
{
  uint32_t eeaddr = EEPROM_START(cur_bank) + Address + EEPROM_BANK_OFFSET;
  //uint16_t cnt,i, *eep, *dp;
  FLASH_Status FLASHStatus = FLASH_COMPLETE;

  if (eeaddr>=EEPROM_END || (eeaddr+Length)>=EEPROM_END || (eeaddr&1) || (Length&1))
     return -1;

  portENTER_CRITICAL();
  memcpy(&shadow_ram[Address/2],Data,Length);
  portEXIT_CRITICAL();


  return FLASHStatus;
}


//перезаписываем содержимое
int8_t Verify_Flash(void){
  uint16_t i;
  FLASH_Status FLASHStatus = FLASH_COMPLETE, status_bank0,status_bank1;


  	  portENTER_CRITICAL();
	  FLASH_Unlock();


	  /* Сбрасываем флаги */
	  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
	                  FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);

	  FLASHStatus = FLASH_EraseSector(FLASH_Sector_2,VoltageRange_3);
	  FLASHStatus = FLASH_EraseSector(FLASH_Sector_3,VoltageRange_3);

	  //пишем в банк 0
	  cur_bank = 0;
	  for (i=0;i<(EEPROM_BANK_SIZE-EEPROM_BANK_OFFSET)&&FLASHStatus == FLASH_COMPLETE;i+=2){
	     FLASHStatus = FLASH_ProgramHalfWord(EEPROM_START(cur_bank) + EEPROM_BANK_OFFSET + i, shadow_ram[ i/2 ]);
	  }
	  status_bank0 = FLASHStatus;

	  //пишем в банк 1
	  cur_bank = 1;
	  for (i=0;i<(EEPROM_BANK_SIZE-EEPROM_BANK_OFFSET)&&FLASHStatus == FLASH_COMPLETE;i+=2){
	     FLASHStatus = FLASH_ProgramHalfWord(EEPROM_START(cur_bank) + EEPROM_BANK_OFFSET + i, shadow_ram[ i/2 ]);
	  }
	  status_bank1 = FLASHStatus;


	  //если записалось нормально, делаем основным банком 0
	  if(status_bank0 == FLASH_COMPLETE){
		  FLASHStatus = FLASH_ProgramHalfWord(EEPROM_START(0), EEPROM_VALID_BANK);
		  FLASHStatus = FLASH_ProgramHalfWord(EEPROM_START(1), EEPROM_BACK_BANK);
	  }else if(status_bank1 == FLASH_COMPLETE){
	 //если нет, то делаем основным банком 1
		  FLASHStatus = FLASH_ProgramHalfWord(EEPROM_START(0), EEPROM_BACK_BANK);
		  FLASHStatus = FLASH_ProgramHalfWord(EEPROM_START(1), EEPROM_VALID_BANK);
	  }
	  else{
	//если нет, то делаем основным банком 0, даже если он тоже записал с ошибками
		  FLASHStatus = FLASH_ProgramHalfWord(EEPROM_START(0), EEPROM_VALID_BANK);
		  FLASHStatus = FLASH_ProgramHalfWord(EEPROM_START(1), EEPROM_BACK_BANK);
	  }


	  FLASH_Lock();
	  portEXIT_CRITICAL();
	  return FLASHStatus;
}

/*
 * Address и Length обязательно должны быть кратны двум
*/
int8_t Read_Eeprom(uint16_t Address, void *Data, uint16_t Length)
{
FLASH_Status FLASHStatus = FLASH_COMPLETE;

  if (Address>=EEPROM_BANK_SIZE || (Address+Length)>=EEPROM_BANK_SIZE || (Address&1)){
     return -1;
  }
  portENTER_CRITICAL();

  memcpy(Data, (const void*)&shadow_ram[Address/2], Length);
  portEXIT_CRITICAL();
  return FLASHStatus;
}

void Get_Eeprom (void)
{
  u16 i;
  u32 eeaddr;

  //если оба банка пусты, пишем дефолтные настройки
  if(((*(__IO uint16_t*)EEPROM_START(0)) == EEPROM_FREE_BANK) &&
		  ((*(__IO uint16_t*)EEPROM_START(1)) == EEPROM_FREE_BANK)){
	  settings_default(0);
  }

  portENTER_CRITICAL();


  //определяем активный банк
  if((*(__IO uint16_t*)EEPROM_START(0)) == EEPROM_VALID_BANK){
	  cur_bank = 0;
  }else if(((*(__IO uint16_t*)EEPROM_START(1)) == EEPROM_VALID_BANK)){
	  cur_bank = 1;
  }else
	  cur_bank = 0;




  eeaddr = EEPROM_START(cur_bank) + EEPROM_BANK_OFFSET;

  for(i = 0;i<(EEPROM_BANK_SIZE-EEPROM_BANK_OFFSET);i+=2){
	  shadow_ram[i/2] = (*(__IO uint16_t*)(eeaddr + i));
  }
  portEXIT_CRITICAL();
}

int8_t Clear_Eeprom (void)
{
  FLASH_Status FLASHStatus = FLASH_COMPLETE;

  FLASH_Unlock();
  /* Сбрасываем флаги */
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                  FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);	

  FLASHStatus = FLASH_EraseSector(FLASH_Sector_2,VoltageRange_3);
  FLASHStatus = FLASH_EraseSector(FLASH_Sector_3,VoltageRange_3);

  cur_bank = 0;
  FLASH_ProgramHalfWord(EEPROM_START(cur_bank), EEPROM_VALID_BANK);
 
  FLASH_Lock();
  memset (shadow_ram,0xFFFF,EEPROM_BANK_SIZE/2);

  return FLASHStatus;
}















//получаем настройки из eeprom
int settings_load_(void){
uip_ipaddr_t ip;
u8 tmp[128];
u16 temp;
u32 temp32;
stp_port_sett_t stp_temp;
vlan_t temp_vlan;
engine_id_t eid;
snmp12_community_t community_temp;

	Get_Eeprom();

	Read_Eeprom(IPADDRESS_ADDR,tmp,4);
	uip_ipaddr(ip,tmp[0],tmp[1],tmp[2],tmp[3]);
	set_net_ip(ip);

	//mask
	Read_Eeprom(NETMASK_ADDR,tmp,4);
	uip_ipaddr(ip,tmp[0],tmp[1],tmp[2],tmp[3]);
	set_net_mask(ip);

	Read_Eeprom(GATEWAY_ADDR,tmp,4);
	uip_ipaddr(ip,tmp[0],tmp[1],tmp[2],tmp[3]);
	set_net_gate(ip);


	Read_Eeprom(DNS_ADDR,tmp,4);
	uip_ipaddr(ip,tmp[0],tmp[1],tmp[2],tmp[3]);
	set_net_dns(ip);

	Read_Eeprom(USER_MAC_ADDR,tmp,6);
	set_net_mac(tmp);

	Read_Eeprom(DEFAULT_MAC_ADDR,tmp,6);
	set_net_def_mac(tmp);

	Read_Eeprom(DHCPMODE_ADDR,&temp,2);
	set_dhcp_mode((u8)temp);

	Read_Eeprom(NET_GRAT_ARP_STATE_ADDR,&temp,2);
	set_gratuitous_arp_state((u8)temp);

	Read_Eeprom(LANG_ADDR,&temp,2);
	set_interface_lang((u8)temp);


	//set_dhcp_server_addr
	//set_dhcp_hops
	//set_dhcp_opt82

	//port state
	Read_Eeprom(PORT1_STATE_ADDR,&temp,2);
	set_port_state(0,(u8)temp);

	Read_Eeprom(PORT2_STATE_ADDR,&temp,2);
	set_port_state(1,(u8)temp);

	Read_Eeprom(PORT3_STATE_ADDR,&temp,2);
	set_port_state(2,(u8)temp);

	Read_Eeprom(PORT4_STATE_ADDR,&temp,2);
	set_port_state(3,(u8)temp);

	Read_Eeprom(PORT5_STATE_ADDR,&temp,2);
	set_port_state(4,(u8)temp);

	Read_Eeprom(PORT6_STATE_ADDR,&temp,2);
	set_port_state(5,(u8)temp);

	Read_Eeprom(PORT7_STATE_ADDR,&temp,2);
	set_port_state(6,(u8)temp);

	Read_Eeprom(PORT8_STATE_ADDR,&temp,2);
	set_port_state(7,(u8)temp);

	Read_Eeprom(PORT9_STATE_ADDR,&temp,2);
	set_port_state(8,(u8)temp);

	Read_Eeprom(PORT10_STATE_ADDR,&temp,2);
	set_port_state(9,(u8)temp);

	Read_Eeprom(PORT11_STATE_ADDR,&temp,2);
	set_port_state(10,(u8)temp);

	Read_Eeprom(PORT12_STATE_ADDR,&temp,2);
	set_port_state(11,(u8)temp);

	Read_Eeprom(PORT13_STATE_ADDR,&temp,2);
	set_port_state(12,(u8)temp);

	Read_Eeprom(PORT14_STATE_ADDR,&temp,2);
	set_port_state(13,(u8)temp);

	Read_Eeprom(PORT15_STATE_ADDR,&temp,2);
	set_port_state(14,(u8)temp);

	Read_Eeprom(PORT16_STATE_ADDR,&temp,2);
	set_port_state(15,(u8)temp);

	//speed/duplex
	Read_Eeprom(PORT1_SPEEDDPLX_ADDR,&temp,2);
	set_port_speed_dplx(0,(u8)temp);

	Read_Eeprom(PORT2_SPEEDDPLX_ADDR,&temp,2);
	set_port_speed_dplx(1,(u8)temp);

	Read_Eeprom(PORT3_SPEEDDPLX_ADDR,&temp,2);
	set_port_speed_dplx(2,(u8)temp);

	Read_Eeprom(PORT4_SPEEDDPLX_ADDR,&temp,2);
	set_port_speed_dplx(3,(u8)temp);

	Read_Eeprom(PORT5_SPEEDDPLX_ADDR,&temp,2);
	set_port_speed_dplx(4,(u8)temp);

	Read_Eeprom(PORT6_SPEEDDPLX_ADDR,&temp,2);
	set_port_speed_dplx(5,(u8)temp);

	Read_Eeprom(PORT7_SPEEDDPLX_ADDR,&temp,2);
	set_port_speed_dplx(6,(u8)temp);

	Read_Eeprom(PORT8_SPEEDDPLX_ADDR,&temp,2);
	set_port_speed_dplx(7,(u8)temp);

	Read_Eeprom(PORT9_SPEEDDPLX_ADDR,&temp,2);
	set_port_speed_dplx(8,(u8)temp);

	Read_Eeprom(PORT10_SPEEDDPLX_ADDR,&temp,2);
	set_port_speed_dplx(9,(u8)temp);

	Read_Eeprom(PORT11_SPEEDDPLX_ADDR,&temp,2);
	set_port_speed_dplx(10,(u8)temp);

	Read_Eeprom(PORT12_SPEEDDPLX_ADDR,&temp,2);
	set_port_speed_dplx(11,(u8)temp);

	Read_Eeprom(PORT13_SPEEDDPLX_ADDR,&temp,2);
	set_port_speed_dplx(12,(u8)temp);

	Read_Eeprom(PORT14_SPEEDDPLX_ADDR,&temp,2);
	set_port_speed_dplx(13,(u8)temp);

	Read_Eeprom(PORT15_SPEEDDPLX_ADDR,&temp,2);
	set_port_speed_dplx(14,(u8)temp);

	Read_Eeprom(PORT16_SPEEDDPLX_ADDR,&temp,2);
	set_port_speed_dplx(15,(u8)temp);


	//flow control
	Read_Eeprom(PORT1_FLOWCTRL_ADDR,&temp,2);
	set_port_flow(0,(u8)temp);

	Read_Eeprom(PORT2_FLOWCTRL_ADDR,&temp,2);
	set_port_flow(1,(u8)temp);

	Read_Eeprom(PORT3_FLOWCTRL_ADDR,&temp,2);
	set_port_flow(2,(u8)temp);

	Read_Eeprom(PORT4_FLOWCTRL_ADDR,&temp,2);
	set_port_flow(3,(u8)temp);

	Read_Eeprom(PORT5_FLOWCTRL_ADDR,&temp,2);
	set_port_flow(4,(u8)temp);

	Read_Eeprom(PORT6_FLOWCTRL_ADDR,&temp,2);
	set_port_flow(5,(u8)temp);

	Read_Eeprom(PORT7_FLOWCTRL_ADDR,&temp,2);
	set_port_flow(6,(u8)temp);

	Read_Eeprom(PORT8_FLOWCTRL_ADDR,&temp,2);
	set_port_flow(7,(u8)temp);

	Read_Eeprom(PORT9_FLOWCTRL_ADDR,&temp,2);
	set_port_flow(8,(u8)temp);

	Read_Eeprom(PORT10_FLOWCTRL_ADDR,&temp,2);
	set_port_flow(9,(u8)temp);

	Read_Eeprom(PORT11_FLOWCTRL_ADDR,&temp,2);
	set_port_flow(10,(u8)temp);

	Read_Eeprom(PORT12_FLOWCTRL_ADDR,&temp,2);
	set_port_flow(11,(u8)temp);

	Read_Eeprom(PORT13_FLOWCTRL_ADDR,&temp,2);
	set_port_flow(12,(u8)temp);

	Read_Eeprom(PORT14_FLOWCTRL_ADDR,&temp,2);
	set_port_flow(13,(u8)temp);

	Read_Eeprom(PORT15_FLOWCTRL_ADDR,&temp,2);
	set_port_flow(14,(u8)temp);

	Read_Eeprom(PORT16_FLOWCTRL_ADDR,&temp,2);
	set_port_flow(15,(u8)temp);

	//wdt
	Read_Eeprom(PORT1_WDT_ADDR,&temp,2);
	set_port_wdt(0,(u8)temp);

	Read_Eeprom(PORT2_WDT_ADDR,&temp,2);
	set_port_wdt(1,(u8)temp);

	Read_Eeprom(PORT3_WDT_ADDR,&temp,2);
	set_port_wdt(2,(u8)temp);

	Read_Eeprom(PORT4_WDT_ADDR,&temp,2);
	set_port_wdt(3,(u8)temp);

	Read_Eeprom(PORT5_WDT_ADDR,&temp,2);
	set_port_wdt(4,(u8)temp);

	Read_Eeprom(PORT6_WDT_ADDR,&temp,2);
	set_port_wdt(5,(u8)temp);

	Read_Eeprom(PORT1_IPADDR_ADDR,ip,4);
	set_port_wdt_ip(0,ip);

	Read_Eeprom(PORT2_IPADDR_ADDR,ip,4);
	set_port_wdt_ip(1,ip);

	Read_Eeprom(PORT3_IPADDR_ADDR,ip,4);
	set_port_wdt_ip(2,ip);

	Read_Eeprom(PORT4_IPADDR_ADDR,ip,4);
	set_port_wdt_ip(3,ip);

	Read_Eeprom(PORT5_IPADDR_ADDR,ip,4);
	set_port_wdt_ip(4,ip);

	Read_Eeprom(PORT6_IPADDR_ADDR,ip,4);
	set_port_wdt_ip(5,ip);

	//wdt ar speed
	Read_Eeprom(PORT1_WDT_SPEED_ADDR,&temp,2);
	set_port_wdt_speed_down(0,(u16)temp);

	Read_Eeprom(PORT2_WDT_SPEED_ADDR,&temp,2);
	set_port_wdt_speed_down(1,(u16)temp);

	Read_Eeprom(PORT3_WDT_SPEED_ADDR,&temp,2);
	set_port_wdt_speed_down(2,(u16)temp);

	Read_Eeprom(PORT4_WDT_SPEED_ADDR,&temp,2);
	set_port_wdt_speed_down(3,(u16)temp);

	Read_Eeprom(PORT5_WDT_SPEED_ADDR,&temp,2);
	set_port_wdt_speed_down(4,(u16)temp);

	Read_Eeprom(PORT6_WDT_SPEED_ADDR,&temp,2);
	set_port_wdt_speed_down(5,(u16)temp);

	Read_Eeprom(PORT7_WDT_SPEED_ADDR,&temp,2);
	set_port_wdt_speed_down(6,(u16)temp);

	Read_Eeprom(PORT8_WDT_SPEED_ADDR,&temp,2);
	set_port_wdt_speed_down(7,(u16)temp);

	Read_Eeprom(PORT9_WDT_SPEED_ADDR,&temp,2);
	set_port_wdt_speed_down(8,(u16)temp);

	Read_Eeprom(PORT10_WDT_SPEED_ADDR,&temp,2);
	set_port_wdt_speed_down(9,(u16)temp);

	Read_Eeprom(PORT11_WDT_SPEED_ADDR,&temp,2);
	set_port_wdt_speed_down(10,(u16)temp);

	Read_Eeprom(PORT12_WDT_SPEED_ADDR,&temp,2);
	set_port_wdt_speed_down(11,(u16)temp);

	Read_Eeprom(PORT13_WDT_SPEED_ADDR,&temp,2);
	set_port_wdt_speed_down(12,(u16)temp);

	Read_Eeprom(PORT14_WDT_SPEED_ADDR,&temp,2);
	set_port_wdt_speed_down(13,(u16)temp);

	Read_Eeprom(PORT15_WDT_SPEED_ADDR,&temp,2);
	set_port_wdt_speed_down(14,(u16)temp);

	Read_Eeprom(PORT16_WDT_SPEED_ADDR,&temp,2);
	set_port_wdt_speed_down(15,(u16)temp);

	//wdt ar speed up
	Read_Eeprom(PORT1_WDT_SPEED_UP_ADDR,&temp,2);
	set_port_wdt_speed_up(0,(u16)temp);

	Read_Eeprom(PORT2_WDT_SPEED_UP_ADDR,&temp,2);
	set_port_wdt_speed_up(1,(u16)temp);

	Read_Eeprom(PORT3_WDT_SPEED_UP_ADDR,&temp,2);
	set_port_wdt_speed_up(2,(u16)temp);

	Read_Eeprom(PORT4_WDT_SPEED_UP_ADDR,&temp,2);
	set_port_wdt_speed_up(3,(u16)temp);

	Read_Eeprom(PORT5_WDT_SPEED_UP_ADDR,&temp,2);
	set_port_wdt_speed_up(4,(u16)temp);

	Read_Eeprom(PORT6_WDT_SPEED_UP_ADDR,&temp,2);
	set_port_wdt_speed_up(5,(u16)temp);

	Read_Eeprom(PORT7_WDT_SPEED_UP_ADDR,&temp,2);
	set_port_wdt_speed_up(6,(u16)temp);

	Read_Eeprom(PORT8_WDT_SPEED_UP_ADDR,&temp,2);
	set_port_wdt_speed_up(7,(u16)temp);

	Read_Eeprom(PORT9_WDT_SPEED_ADDR,&temp,2);
	set_port_wdt_speed_up(8,(u16)temp);

	Read_Eeprom(PORT10_WDT_SPEED_ADDR,&temp,2);
	set_port_wdt_speed_up(9,(u16)temp);

	Read_Eeprom(PORT11_WDT_SPEED_UP_ADDR,&temp,2);
	set_port_wdt_speed_up(10,(u16)temp);

	Read_Eeprom(PORT12_WDT_SPEED_UP_ADDR,&temp,2);
	set_port_wdt_speed_up(11,(u16)temp);

	Read_Eeprom(PORT13_WDT_SPEED_UP_ADDR,&temp,2);
	set_port_wdt_speed_up(12,(u16)temp);

	Read_Eeprom(PORT14_WDT_SPEED_UP_ADDR,&temp,2);
	set_port_wdt_speed_up(13,(u16)temp);

	Read_Eeprom(PORT15_WDT_SPEED_UP_ADDR,&temp,2);
	set_port_wdt_speed_up(14,(u16)temp);

	Read_Eeprom(PORT16_WDT_SPEED_UP_ADDR,&temp,2);
	set_port_wdt_speed_up(15,(u16)temp);



	//softstart
	Read_Eeprom(PORT1_SOFTSTART_ADDR,&temp,2);
	set_port_soft_start(0,(u8)temp);

	Read_Eeprom(PORT2_SOFTSTART_ADDR,&temp,2);
	set_port_soft_start(1,(u8)temp);

	Read_Eeprom(PORT3_SOFTSTART_ADDR,&temp,2);
	set_port_soft_start(2,(u8)temp);

	Read_Eeprom(PORT4_SOFTSTART_ADDR,&temp,2);
	set_port_soft_start(3,(u8)temp);

	Read_Eeprom(PORT5_SOFTSTART_ADDR,&temp,2);
	set_port_soft_start(4,(u8)temp);

	Read_Eeprom(PORT6_SOFTSTART_ADDR,&temp,2);
	set_port_soft_start(5,(u8)temp);

	//poe state
	Read_Eeprom(PORT1_POE_ADDR,&temp,2);
	set_port_poe(0,(u8)temp);
	//set_port_poe_b(0,(u8)(temp>>8));

	Read_Eeprom(PORT2_POE_ADDR,&temp,2);
	set_port_poe(1,(u8)temp);
	//set_port_poe_b(1,(u8)(temp>>8));

	Read_Eeprom(PORT3_POE_ADDR,&temp,2);
	set_port_poe(2,(u8)temp);
	//set_port_poe_b(2,(u8)(temp>>8));

	Read_Eeprom(PORT4_POE_ADDR,&temp,2);
	set_port_poe(3,(u8)temp);
	//set_port_poe_b(3,(u8)(temp>>8));

	Read_Eeprom(PORT5_POE_ADDR,&temp,2);
	set_port_poe(4,(u8)temp);
	//set_port_poe_b(4,(u8)(temp>>8));

	Read_Eeprom(PORT6_POE_ADDR,&temp,2);
	set_port_poe(5,(u8)temp);
	//set_port_poe_b(5,(u8)(temp>>8));

	//poe limit
	Read_Eeprom(PORT1_POE_A_LIM,&temp,2);
	set_port_pwr_lim_a(0,(u8)temp);

	Read_Eeprom(PORT2_POE_A_LIM,&temp,2);
	set_port_pwr_lim_a(1,(u8)temp);

	Read_Eeprom(PORT3_POE_A_LIM,&temp,2);
	set_port_pwr_lim_a(2,(u8)temp);

	Read_Eeprom(PORT4_POE_A_LIM,&temp,2);
	set_port_pwr_lim_a(3,(u8)temp);

	Read_Eeprom(PORT5_POE_A_LIM,&temp,2);
	set_port_pwr_lim_a(4,(u8)temp);

	Read_Eeprom(PORT6_POE_A_LIM,&temp,2);
	set_port_pwr_lim_a(5,(u8)temp);


	Read_Eeprom(PORT1_POE_B_LIM,&temp,2);
	set_port_pwr_lim_b(0,(u8)temp);

	Read_Eeprom(PORT2_POE_B_LIM,&temp,2);
	set_port_pwr_lim_b(1,(u8)temp);

	Read_Eeprom(PORT3_POE_B_LIM,&temp,2);
	set_port_pwr_lim_b(2,(u8)temp);

	Read_Eeprom(PORT4_POE_B_LIM,&temp,2);
	set_port_pwr_lim_b(3,(u8)temp);

	Read_Eeprom(PORT5_POE_B_LIM,&temp,2);
	set_port_pwr_lim_b(4,(u8)temp);

	Read_Eeprom(PORT6_POE_B_LIM,&temp,2);
	set_port_pwr_lim_b(5,(u8)temp);

	Read_Eeprom(PORT_SFP1_MODE,&temp,2);
	set_port_sfp_mode(GE1,(u8)temp);

	Read_Eeprom(PORT_SFP2_MODE,&temp,2);
	set_port_sfp_mode(GE2,(u8)temp);


	//user 0
	Read_Eeprom(HTTP_USERNAME_ADDR,tmp,64);
	//set_interface_login((char *)tmp);
	set_interface_users_username(0,(char *)tmp);

	Read_Eeprom(HTTP_PASSWD_ADDR,tmp,64);
	//set_interface_passwd((char *)tmp);
	set_interface_users_password(0,(char *)tmp);
	set_interface_users_rule(0,ADMIN_RULE);

	/*add to user list*/
	//user 1
	Read_Eeprom(USER1_USERNAME_ADDR,tmp,64);
	set_interface_users_username(1,(char *)tmp);

	Read_Eeprom(USER1_PASSWORD_ADDR,tmp,64);
	set_interface_users_password(1,(char *)tmp);

	Read_Eeprom(USER1_RULE_ADDR,&temp,2);
	set_interface_users_rule(1,(u8)temp);

	//user 2
	Read_Eeprom(USER2_USERNAME_ADDR,tmp,64);
	set_interface_users_username(2,(char *)tmp);

	Read_Eeprom(USER2_PASSWORD_ADDR,tmp,64);
	set_interface_users_password(2,(char *)tmp);

	Read_Eeprom(USER2_RULE_ADDR,&temp,2);
	set_interface_users_rule(2,(u8)temp);

	//user 3
	Read_Eeprom(USER3_USERNAME_ADDR,tmp,64);
	set_interface_users_username(3,(char *)tmp);

	Read_Eeprom(USER3_PASSWORD_ADDR,tmp,64);
	set_interface_users_password(3,(char *)tmp);

	Read_Eeprom(USER3_RULE_ADDR,&temp,2);
	set_interface_users_rule(3,(u8)temp);


	//description
	Read_Eeprom(SYSTEM_NAME_ADDR,tmp,128);
	set_interface_name((char *)tmp);

	Read_Eeprom(SYSTEM_LOCATION_ADDR,tmp,128);
	set_interface_location((char *)tmp);

	Read_Eeprom(SYSTEM_CONTACT_ADDR,tmp,128);
	set_interface_contact((char *)tmp);

	//port description
	Read_Eeprom(PORT1_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	set_port_descr(0,(char *)tmp);
	Read_Eeprom(PORT2_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	set_port_descr(1,(char *)tmp);
	Read_Eeprom(PORT3_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	set_port_descr(2,(char *)tmp);
	Read_Eeprom(PORT4_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	set_port_descr(3,(char *)tmp);
	Read_Eeprom(PORT5_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	set_port_descr(4,(char *)tmp);
	Read_Eeprom(PORT6_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	set_port_descr(5,(char *)tmp);
	Read_Eeprom(PORT7_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	set_port_descr(6,(char *)tmp);
	Read_Eeprom(PORT8_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	set_port_descr(7,(char *)tmp);
	Read_Eeprom(PORT9_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	set_port_descr(8,(char *)tmp);
	Read_Eeprom(PORT10_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	set_port_descr(9,(char *)tmp);
	Read_Eeprom(PORT11_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	set_port_descr(10,(char *)tmp);
	Read_Eeprom(PORT12_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	set_port_descr(11,(char *)tmp);
	Read_Eeprom(PORT13_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	set_port_descr(12,(char *)tmp);
	Read_Eeprom(PORT14_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	set_port_descr(13,(char *)tmp);
	Read_Eeprom(PORT15_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	set_port_descr(14,(char *)tmp);
	Read_Eeprom(PORT16_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	set_port_descr(15,(char *)tmp);


	//smtp
	Read_Eeprom(SMTP_STATE_ADDR,&temp,2);
	set_smtp_state((u8)temp);

	Read_Eeprom(SMTP_SERV_IP_ADDR,ip,4);
	set_smtp_server(ip);

	Read_Eeprom(SMTP_TO1_ADDR,tmp,64);
	set_smtp_to((char *)tmp);

	Read_Eeprom(SMTP_TO2_ADDR,tmp,64);
	set_smtp_to2((char *)(char *)tmp);

	Read_Eeprom(SMTP_TO3_ADDR,tmp,64);
	set_smtp_to3((char *)tmp);

	Read_Eeprom(SMTP_FROM_ADDR,tmp,64);
	set_smtp_from((char *)tmp);

	Read_Eeprom(SMTP_SUBJ_ADDR,tmp,64);
	set_smtp_subj((char *)tmp);

	Read_Eeprom(SMTP_LOGIN_ADDR,tmp,32);
	set_smtp_login((char *)tmp);

	Read_Eeprom(SMTP_PASS_ADDR,tmp,32);
	set_smtp_pass((char *)tmp);

	Read_Eeprom(SMTP_PORT_ADDR,&temp32,4);
	set_smtp_port((u16)temp32);

	Read_Eeprom(SMTP_DOMAIN_NAME2_ADDR,tmp,32);
	set_smtp_domain((char *)tmp);

	Read_Eeprom(SNTP_STATE_ADDR,&temp,2);
	set_sntp_state((u8)temp);

	Read_Eeprom(SNTP_SETT_SERV_ADDR,ip,4);
	set_sntp_serv(ip);

	Read_Eeprom(SNTP_SETT_SERV_NAME_ADDR,tmp,64);
	set_sntp_serv_name((char *)tmp);


	Read_Eeprom(SNTP_TIMEZONE_ADDR,&temp,2);
	set_sntp_timezone((i8)temp);

	Read_Eeprom(SNTP_PERIOD_ADDR,&temp,2);
	set_sntp_period((u8)temp);

	Read_Eeprom(SYSLOG_STATE_ADDR,&temp,2);
	set_syslog_state((u8)temp);

	Read_Eeprom(SYSLOG_SERV_IP_ADDR,ip,4);
	set_syslog_serv(ip);

	Read_Eeprom(EVENT_LIST_BASE_S_ADDR,&temp,2);
	set_event_base_s(0x01&(temp>>3),temp&SMASK);

	Read_Eeprom(EVENT_LIST_PORT_S_ADDR,&temp,2);
	set_event_port_s(0x01&(temp>>3),temp&SMASK);

	Read_Eeprom(EVENT_LIST_VLAN_S_ADDR,&temp,2);
	set_event_vlan_s(0x01&(temp>>3),temp&SMASK);

	Read_Eeprom(EVENT_LIST_STP_S_ADDR,&temp,2);
	set_event_stp_s(0x01&(temp>>3),temp&SMASK);

	Read_Eeprom(EVENT_LIST_QOS_S_ADDR,&temp,2);
	set_event_qos_s(0x01&(temp>>3),temp&SMASK);

	Read_Eeprom(EVENT_LIST_OTHER_S_ADDR,&temp,2);
	set_event_other_s(0x01&(temp>>3),temp&SMASK);

	Read_Eeprom(EVENT_LIST_LINK_T_ADDR,&temp,2);
	set_event_port_link_t(0x01&(temp>>3),temp&SMASK);

	Read_Eeprom(EVENT_LIST_POE_T_ADDR,&temp,2);
	set_event_port_poe_t(0x01&(temp>>3),temp&SMASK);

	Read_Eeprom(EVENT_LIST_STP_T_ADDR,&temp,2);
	set_event_stp_t(0x01&(temp>>3),temp&SMASK);

	Read_Eeprom(EVENT_LIST_ARLINK_T_ADDR,&temp,2);
	set_event_spec_link_t(0x01&(temp>>3),temp&SMASK);

	Read_Eeprom(EVENT_LIST_ARPING_T_ADDR,&temp,2);
	set_event_spec_ping_t(0x01&(temp>>3),temp&SMASK);

	Read_Eeprom(EVENT_LIST_ARSPEED_T_ADDR,&temp,2);
	set_event_spec_speed_t(0x01&(temp>>3),temp&SMASK);


	Read_Eeprom(EVENT_LIST_SYSTEM_T_ADDR,&temp,2);
	set_event_system_t(0x01&(temp>>3),temp&SMASK);

	Read_Eeprom(EVENT_LIST_UPS_T_ADDR,&temp,2);
	set_event_ups_t(0x01&(temp>>3),temp&SMASK);

	Read_Eeprom(EVENT_LIST_ACCESS_T_ADDR,&temp,2);
	set_event_alarm_t(0x01&(temp>>3),temp&SMASK);

	Read_Eeprom(EVENT_LIST_MAC_T_ADDR,&temp,2);
	set_event_mac_t(0x01&(temp>>3),temp&SMASK);

	Read_Eeprom(DRY_CONT0_STATE_ADDR,&temp,2);
	set_alarm_state(0,(u8)temp);

	Read_Eeprom(DRY_CONT1_STATE_ADDR,&temp,2);
	set_alarm_state(1,(u8)temp);

	Read_Eeprom(DRY_CONT1_LEVEL_ADDR,&temp,2);
	set_alarm_front(1,(u8)temp);

	Read_Eeprom(DRY_CONT2_STATE_ADDR,&temp,2);
	set_alarm_state(2,(u8)temp);

	Read_Eeprom(DRY_CONT2_LEVEL_ADDR,&temp,2);
	set_alarm_front(2,(u8)temp);

	IWDG_ReloadCounter();

	//rate limit rx
	Read_Eeprom(PORT1_RATE_LIMIT_RX_ADDR,&temp32,4);
	set_rate_limit_rx(0,temp32);

	Read_Eeprom(PORT2_RATE_LIMIT_RX_ADDR,&temp32,4);
	set_rate_limit_rx(1,temp32);

	Read_Eeprom(PORT3_RATE_LIMIT_RX_ADDR,&temp32,4);
	set_rate_limit_rx(2,temp32);

	Read_Eeprom(PORT4_RATE_LIMIT_RX_ADDR,&temp32,4);
	set_rate_limit_rx(3,temp32);

	Read_Eeprom(PORT5_RATE_LIMIT_RX_ADDR,&temp32,4);
	set_rate_limit_rx(4,temp32);

	Read_Eeprom(PORT6_RATE_LIMIT_RX_ADDR,&temp32,4);
	set_rate_limit_rx(5,temp32);

	Read_Eeprom(PORT7_RATE_LIMIT_RX_ADDR,&temp32,4);
	set_rate_limit_rx(6,temp32);

	Read_Eeprom(PORT8_RATE_LIMIT_RX_ADDR,&temp32,4);
	set_rate_limit_rx(7,temp32);

	Read_Eeprom(PORT9_RATE_LIMIT_RX_ADDR,&temp32,4);
	set_rate_limit_rx(8,temp32);

	Read_Eeprom(PORT10_RATE_LIMIT_RX_ADDR,&temp32,4);
	set_rate_limit_rx(9,temp32);

	Read_Eeprom(PORT11_RATE_LIMIT_RX_ADDR,&temp32,4);
	set_rate_limit_rx(10,temp32);

	Read_Eeprom(PORT12_RATE_LIMIT_RX_ADDR,&temp32,4);
	set_rate_limit_rx(11,temp32);

	Read_Eeprom(PORT13_RATE_LIMIT_RX_ADDR,&temp32,4);
	set_rate_limit_rx(12,temp32);

	Read_Eeprom(PORT14_RATE_LIMIT_RX_ADDR,&temp32,4);
	set_rate_limit_rx(13,temp32);

	Read_Eeprom(PORT15_RATE_LIMIT_RX_ADDR,&temp32,4);
	set_rate_limit_rx(14,temp32);

	Read_Eeprom(PORT16_RATE_LIMIT_RX_ADDR,&temp32,4);
	set_rate_limit_rx(15,temp32);


	//rate limit tx
	Read_Eeprom(PORT1_RATE_LIMIT_TX_ADDR,&temp32,4);
	set_rate_limit_tx(0,temp32);

	Read_Eeprom(PORT2_RATE_LIMIT_TX_ADDR,&temp32,4);
	set_rate_limit_tx(1,temp32);

	Read_Eeprom(PORT3_RATE_LIMIT_TX_ADDR,&temp32,4);
	set_rate_limit_tx(2,temp32);

	Read_Eeprom(PORT4_RATE_LIMIT_TX_ADDR,&temp32,4);
	set_rate_limit_tx(3,temp32);

	Read_Eeprom(PORT5_RATE_LIMIT_TX_ADDR,&temp32,4);
	set_rate_limit_tx(4,temp32);

	Read_Eeprom(PORT6_RATE_LIMIT_TX_ADDR,&temp32,4);
	set_rate_limit_tx(5,temp32);

	Read_Eeprom(PORT7_RATE_LIMIT_TX_ADDR,&temp32,4);
	set_rate_limit_tx(6,temp32);

	Read_Eeprom(PORT8_RATE_LIMIT_TX_ADDR,&temp32,4);
	set_rate_limit_tx(7,temp32);

	Read_Eeprom(PORT9_RATE_LIMIT_TX_ADDR,&temp32,4);
	set_rate_limit_tx(8,temp32);

	Read_Eeprom(PORT10_RATE_LIMIT_TX_ADDR,&temp32,4);
	set_rate_limit_tx(9,temp32);

	Read_Eeprom(PORT11_RATE_LIMIT_TX_ADDR,&temp32,4);
	set_rate_limit_tx(10,temp32);

	Read_Eeprom(PORT12_RATE_LIMIT_TX_ADDR,&temp32,4);
	set_rate_limit_tx(11,temp32);

	Read_Eeprom(PORT13_RATE_LIMIT_TX_ADDR,&temp32,4);
	set_rate_limit_tx(12,temp32);

	Read_Eeprom(PORT14_RATE_LIMIT_TX_ADDR,&temp32,4);
	set_rate_limit_tx(13,temp32);

	Read_Eeprom(PORT15_RATE_LIMIT_TX_ADDR,&temp32,4);
	set_rate_limit_tx(14,temp32);

	Read_Eeprom(PORT16_RATE_LIMIT_TX_ADDR,&temp32,4);
	set_rate_limit_tx(15,temp32);

	//cos
	Read_Eeprom(PORT1_COS_STATE_ADDR,&temp,2);
	set_qos_port_cos_state(0,(u8)temp);

	Read_Eeprom(PORT2_COS_STATE_ADDR,&temp,2);
	set_qos_port_cos_state(1,(u8)temp);

	Read_Eeprom(PORT3_COS_STATE_ADDR,&temp,2);
	set_qos_port_cos_state(2,(u8)temp);

	Read_Eeprom(PORT4_COS_STATE_ADDR,&temp,2);
	set_qos_port_cos_state(3,(u8)temp);

	Read_Eeprom(PORT5_COS_STATE_ADDR,&temp,2);
	set_qos_port_cos_state(4,(u8)temp);

	Read_Eeprom(PORT6_COS_STATE_ADDR,&temp,2);
	set_qos_port_cos_state(5,(u8)temp);

	Read_Eeprom(PORT7_COS_STATE_ADDR,&temp,2);
	set_qos_port_cos_state(6,(u8)temp);

	Read_Eeprom(PORT8_COS_STATE_ADDR,&temp,2);
	set_qos_port_cos_state(7,(u8)temp);

	Read_Eeprom(PORT9_COS_STATE_ADDR,&temp,2);
	set_qos_port_cos_state(8,(u8)temp);

	Read_Eeprom(PORT10_COS_STATE_ADDR,&temp,2);
	set_qos_port_cos_state(9,(u8)temp);

	Read_Eeprom(PORT11_COS_STATE_ADDR,&temp,2);
	set_qos_port_cos_state(10,(u8)temp);

	Read_Eeprom(PORT12_COS_STATE_ADDR,&temp,2);
	set_qos_port_cos_state(11,(u8)temp);

	Read_Eeprom(PORT13_COS_STATE_ADDR,&temp,2);
	set_qos_port_cos_state(12,(u8)temp);

	Read_Eeprom(PORT14_COS_STATE_ADDR,&temp,2);
	set_qos_port_cos_state(13,(u8)temp);

	Read_Eeprom(PORT15_COS_STATE_ADDR,&temp,2);
	set_qos_port_cos_state(14,(u8)temp);

	Read_Eeprom(PORT16_COS_STATE_ADDR,&temp,2);
	set_qos_port_cos_state(15,(u8)temp);


	//tos
	Read_Eeprom(PORT1_TOS_STATE_ADDR,&temp,2);
	set_qos_port_tos_state(0,(u8)temp);

	Read_Eeprom(PORT2_TOS_STATE_ADDR,&temp,2);
	set_qos_port_tos_state(1,(u8)temp);

	Read_Eeprom(PORT3_TOS_STATE_ADDR,&temp,2);
	set_qos_port_tos_state(2,(u8)temp);

	Read_Eeprom(PORT4_TOS_STATE_ADDR,&temp,2);
	set_qos_port_tos_state(3,(u8)temp);

	Read_Eeprom(PORT5_TOS_STATE_ADDR,&temp,2);
	set_qos_port_tos_state(4,(u8)temp);

	Read_Eeprom(PORT6_TOS_STATE_ADDR,&temp,2);
	set_qos_port_tos_state(5,(u8)temp);

	Read_Eeprom(PORT7_TOS_STATE_ADDR,&temp,2);
	set_qos_port_tos_state(6,(u8)temp);

	Read_Eeprom(PORT8_TOS_STATE_ADDR,&temp,2);
	set_qos_port_tos_state(7,(u8)temp);

	Read_Eeprom(PORT9_TOS_STATE_ADDR,&temp,2);
	set_qos_port_tos_state(8,(u8)temp);

	Read_Eeprom(PORT10_TOS_STATE_ADDR,&temp,2);
	set_qos_port_tos_state(9,(u8)temp);

	Read_Eeprom(PORT11_TOS_STATE_ADDR,&temp,2);
	set_qos_port_tos_state(10,(u8)temp);

	Read_Eeprom(PORT12_TOS_STATE_ADDR,&temp,2);
	set_qos_port_tos_state(11,(u8)temp);

	Read_Eeprom(PORT13_TOS_STATE_ADDR,&temp,2);
	set_qos_port_tos_state(12,(u8)temp);

	Read_Eeprom(PORT14_TOS_STATE_ADDR,&temp,2);
	set_qos_port_tos_state(13,(u8)temp);

	Read_Eeprom(PORT15_TOS_STATE_ADDR,&temp,2);
	set_qos_port_tos_state(14,(u8)temp);

	Read_Eeprom(PORT16_TOS_STATE_ADDR,&temp,2);
	set_qos_port_tos_state(15,(u8)temp);

	//
	Read_Eeprom(PORT1_QOS_RULE_ADDR,&temp,2);
	set_qos_port_rule(0,(u8)temp);

	Read_Eeprom(PORT2_QOS_RULE_ADDR,&temp,2);
	set_qos_port_rule(1,(u8)temp);

	Read_Eeprom(PORT3_QOS_RULE_ADDR,&temp,2);
	set_qos_port_rule(2,(u8)temp);

	Read_Eeprom(PORT4_QOS_RULE_ADDR,&temp,2);
	set_qos_port_rule(3,(u8)temp);

	Read_Eeprom(PORT5_QOS_RULE_ADDR,&temp,2);
	set_qos_port_rule(4,(u8)temp);

	Read_Eeprom(PORT6_QOS_RULE_ADDR,&temp,2);
	set_qos_port_rule(5,(u8)temp);

	Read_Eeprom(PORT7_QOS_RULE_ADDR,&temp,2);
	set_qos_port_rule(6,(u8)temp);

	Read_Eeprom(PORT8_QOS_RULE_ADDR,&temp,2);
	set_qos_port_rule(7,(u8)temp);

	Read_Eeprom(PORT9_QOS_RULE_ADDR,&temp,2);
	set_qos_port_rule(8,(u8)temp);

	Read_Eeprom(PORT10_QOS_RULE_ADDR,&temp,2);
	set_qos_port_rule(9,(u8)temp);

	Read_Eeprom(PORT11_QOS_RULE_ADDR,&temp,2);
	set_qos_port_rule(10,(u8)temp);

	Read_Eeprom(PORT12_QOS_RULE_ADDR,&temp,2);
	set_qos_port_rule(11,(u8)temp);

	Read_Eeprom(PORT13_QOS_RULE_ADDR,&temp,2);
	set_qos_port_rule(12,(u8)temp);

	Read_Eeprom(PORT14_QOS_RULE_ADDR,&temp,2);
	set_qos_port_rule(13,(u8)temp);

	Read_Eeprom(PORT15_QOS_RULE_ADDR,&temp,2);
	set_qos_port_rule(14,(u8)temp);

	Read_Eeprom(PORT16_QOS_RULE_ADDR,&temp,2);
	set_qos_port_rule(15,(u8)temp);



	Read_Eeprom(PORT1_COS_PRI_ADDR,&temp,2);
	set_qos_port_def_pri(0,(u8)temp);

	Read_Eeprom(PORT2_COS_PRI_ADDR,&temp,2);
	set_qos_port_def_pri(1,(u8)temp);

	Read_Eeprom(PORT3_COS_PRI_ADDR,&temp,2);
	set_qos_port_def_pri(2,(u8)temp);

	Read_Eeprom(PORT4_COS_PRI_ADDR,&temp,2);
	set_qos_port_def_pri(3,(u8)temp);

	Read_Eeprom(PORT5_COS_PRI_ADDR,&temp,2);
	set_qos_port_def_pri(4,(u8)temp);

	Read_Eeprom(PORT6_COS_PRI_ADDR,&temp,2);
	set_qos_port_def_pri(5,(u8)temp);

	Read_Eeprom(PORT7_COS_PRI_ADDR,&temp,2);
	set_qos_port_def_pri(6,(u8)temp);

	Read_Eeprom(PORT8_COS_PRI_ADDR,&temp,2);
	set_qos_port_def_pri(7,(u8)temp);

	Read_Eeprom(PORT9_COS_PRI_ADDR,&temp,2);
	set_qos_port_def_pri(8,(u8)temp);

	Read_Eeprom(PORT10_COS_PRI_ADDR,&temp,2);
	set_qos_port_def_pri(9,(u8)temp);

	Read_Eeprom(PORT11_COS_PRI_ADDR,&temp,2);
	set_qos_port_def_pri(10,(u8)temp);

	Read_Eeprom(PORT12_COS_PRI_ADDR,&temp,2);
	set_qos_port_def_pri(11,(u8)temp);

	Read_Eeprom(PORT13_COS_PRI_ADDR,&temp,2);
	set_qos_port_def_pri(12,(u8)temp);

	Read_Eeprom(PORT14_COS_PRI_ADDR,&temp,2);
	set_qos_port_def_pri(13,(u8)temp);

	Read_Eeprom(PORT15_COS_PRI_ADDR,&temp,2);
	set_qos_port_def_pri(14,(u8)temp);

	Read_Eeprom(PORT16_COS_PRI_ADDR,&temp,2);
	set_qos_port_def_pri(15,(u8)temp);


	Read_Eeprom(QOS_STATE_ADDR,&temp,2);
	set_qos_state((u8)temp);

	Read_Eeprom(QOS_POLICY_ADDR,&temp,2);
	set_qos_policy((u8)temp);

	Read_Eeprom(QOS_COS_ADDR,tmp,8);
	for(u8 i=0;i<8;i++){
		set_qos_cos(i,tmp[i]);
	}

	Read_Eeprom(QOS_TOS_ADDR,tmp,64);
	for(u8 i=0;i<64;i++){
		set_qos_tos(i,tmp[i]);
	}

	Read_Eeprom(UC_RATE_LIM_ADDR,&temp,2);
	set_uc_rate_limit((u8)temp);
	Read_Eeprom(MC_RATE_LIM_ADDR,&temp,2);
	set_mc_rate_limit((u8)temp);
	Read_Eeprom(BC_RATE_LIM_ADDR,&temp,2);
	set_bc_rate_limit((u8)temp);
	Read_Eeprom(LIM_RATE_LIM_ADDR,&temp,2);
	set_bc_limit((u8)temp);




	Read_Eeprom(RATE_LIM_MODE_P1_ADDR,&temp,2);
	set_rate_limit_mode(0,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P2_ADDR,&temp,2);
	set_rate_limit_mode(1,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P3_ADDR,&temp,2);
	set_rate_limit_mode(2,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P4_ADDR,&temp,2);
	set_rate_limit_mode(3,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P5_ADDR,&temp,2);
	set_rate_limit_mode(4,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P6_ADDR,&temp,2);
	set_rate_limit_mode(5,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P7_ADDR,&temp,2);
	set_rate_limit_mode(6,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P8_ADDR,&temp,2);
	set_rate_limit_mode(7,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P9_ADDR,&temp,2);
	set_rate_limit_mode(8,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P10_ADDR,&temp,2);
	set_rate_limit_mode(9,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P11_ADDR,&temp,2);
	set_rate_limit_mode(10,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P12_ADDR,&temp,2);
	set_rate_limit_mode(11,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P13_ADDR,&temp,2);
	set_rate_limit_mode(12,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P14_ADDR,&temp,2);
	set_rate_limit_mode(13,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P15_ADDR,&temp,2);
	set_rate_limit_mode(14,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P16_ADDR,&temp,2);
	set_rate_limit_mode(15,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P17_ADDR,&temp,2);
	set_rate_limit_mode(16,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P18_ADDR,&temp,2);
	set_rate_limit_mode(17,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P19_ADDR,&temp,2);
	set_rate_limit_mode(18,(u8)temp);
	Read_Eeprom(RATE_LIM_MODE_P20_ADDR,&temp,2);
	set_rate_limit_mode(19,(u8)temp);








	Read_Eeprom(PB_VLAN_STATE_ADDR,&temp,2);
	set_pb_vlan_state((u8)temp);

	Read_Eeprom(PORT1_PB_VLAN_ADDR,&temp,2);
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		if(temp & 1<<i)
			set_pb_vlan_port(0,i,1);
		else
			set_pb_vlan_port(0,i,0);
	}

	Read_Eeprom(PORT2_PB_VLAN_ADDR,&temp,2);
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		if(temp & 1<<i)
			set_pb_vlan_port(1,i,1);
		else
			set_pb_vlan_port(1,i,0);
	}

	Read_Eeprom(PORT3_PB_VLAN_ADDR,&temp,2);
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		if(temp & 1<<i)
			set_pb_vlan_port(2,i,1);
		else
			set_pb_vlan_port(2,i,0);
	}

	Read_Eeprom(PORT4_PB_VLAN_ADDR,&temp,2);
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		if(temp & 1<<i)
			set_pb_vlan_port(3,i,1);
		else
			set_pb_vlan_port(3,i,0);
	}

	Read_Eeprom(PORT5_PB_VLAN_ADDR,&temp,2);
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		if(temp & 1<<i)
			set_pb_vlan_port(4,i,1);
		else
			set_pb_vlan_port(4,i,0);
	}

	Read_Eeprom(PORT6_PB_VLAN_ADDR,&temp,2);
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		if(temp & 1<<i)
			set_pb_vlan_port(5,i,1);
		else
			set_pb_vlan_port(5,i,0);
	}

	Read_Eeprom(PORT7_PB_VLAN_ADDR,&temp,2);
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		if(temp & 1<<i)
			set_pb_vlan_port(6,i,1);
		else
			set_pb_vlan_port(6,i,0);
	}

	Read_Eeprom(PORT8_PB_VLAN_ADDR,&temp,2);
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		if(temp & 1<<i)
			set_pb_vlan_port(7,i,1);
		else
			set_pb_vlan_port(7,i,0);
	}

	Read_Eeprom(PORT9_PB_VLAN_ADDR,&temp,2);
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		if(temp & 1<<i)
			set_pb_vlan_port(8,i,1);
		else
			set_pb_vlan_port(8,i,0);
	}

	Read_Eeprom(PORT10_PB_VLAN_ADDR,&temp,2);
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		if(temp & 1<<i)
			set_pb_vlan_port(9,i,1);
		else
			set_pb_vlan_port(9,i,0);
	}

	Read_Eeprom(PORT11_PB_VLAN_ADDR,&temp,2);
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		if(temp & 1<<i)
			set_pb_vlan_port(10,i,1);
		else
			set_pb_vlan_port(10,i,0);
	}

	Read_Eeprom(PORT12_PB_VLAN_ADDR,&temp,2);
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		if(temp & 1<<i)
			set_pb_vlan_port(11,i,1);
		else
			set_pb_vlan_port(11,i,0);
	}

	Read_Eeprom(PORT13_PB_VLAN_ADDR,&temp,2);
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		if(temp & 1<<i)
			set_pb_vlan_port(12,i,1);
		else
			set_pb_vlan_port(12,i,0);
	}

	Read_Eeprom(PORT14_PB_VLAN_ADDR,&temp,2);
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		if(temp & 1<<i)
			set_pb_vlan_port(13,i,1);
		else
			set_pb_vlan_port(13,i,0);
	}

	Read_Eeprom(PORT15_PB_VLAN_ADDR,&temp,2);
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		if(temp & 1<<i)
			set_pb_vlan_port(14,i,1);
		else
			set_pb_vlan_port(14,i,0);
	}

	Read_Eeprom(PORT16_PB_VLAN_ADDR,&temp,2);
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		if(temp & 1<<i)
			set_pb_vlan_port(15,i,1);
		else
			set_pb_vlan_port(15,i,0);
	}


	//port based vlan for swu-16
	Read_Eeprom(PORT1_PB_VLAN_SWU_ADDR,&temp,2);
	set_pb_vlan_swu_port(0,(u8)temp);

	Read_Eeprom(PORT2_PB_VLAN_SWU_ADDR,&temp,2);
	set_pb_vlan_swu_port(1,(u8)temp);

	Read_Eeprom(PORT3_PB_VLAN_SWU_ADDR,&temp,2);
	set_pb_vlan_swu_port(2,(u8)temp);

	Read_Eeprom(PORT4_PB_VLAN_SWU_ADDR,&temp,2);
	set_pb_vlan_swu_port(3,(u8)temp);

	Read_Eeprom(PORT5_PB_VLAN_SWU_ADDR,&temp,2);
	set_pb_vlan_swu_port(4,(u8)temp);

	Read_Eeprom(PORT6_PB_VLAN_SWU_ADDR,&temp,2);
	set_pb_vlan_swu_port(5,(u8)temp);

	Read_Eeprom(PORT7_PB_VLAN_SWU_ADDR,&temp,2);
	set_pb_vlan_swu_port(6,(u8)temp);

	Read_Eeprom(PORT8_PB_VLAN_SWU_ADDR,&temp,2);
	set_pb_vlan_swu_port(7,(u8)temp);

	Read_Eeprom(PORT9_PB_VLAN_SWU_ADDR,&temp,2);
	set_pb_vlan_swu_port(8,(u8)temp);

	Read_Eeprom(PORT10_PB_VLAN_SWU_ADDR,&temp,2);
	set_pb_vlan_swu_port(9,(u8)temp);

	Read_Eeprom(PORT11_PB_VLAN_SWU_ADDR,&temp,2);
	set_pb_vlan_swu_port(10,(u8)temp);

	Read_Eeprom(PORT12_PB_VLAN_SWU_ADDR,&temp,2);
	set_pb_vlan_swu_port(11,(u8)temp);

	Read_Eeprom(PORT13_PB_VLAN_SWU_ADDR,&temp,2);
	set_pb_vlan_swu_port(12,(u8)temp);

	Read_Eeprom(PORT14_PB_VLAN_SWU_ADDR,&temp,2);
	set_pb_vlan_swu_port(13,(u8)temp);

	Read_Eeprom(PORT15_PB_VLAN_SWU_ADDR,&temp,2);
	set_pb_vlan_swu_port(14,(u8)temp);

	Read_Eeprom(PORT16_PB_VLAN_SWU_ADDR,&temp,2);
	set_pb_vlan_swu_port(15,(u8)temp);


	Read_Eeprom(VLAN_MVID_ADDR,&temp,2);
	set_vlan_sett_mngt(temp);

	Read_Eeprom(VLAN_TRUNK_STATE_ADDR,&temp,2);
	set_vlan_trunk_state((u8)temp);

	Read_Eeprom(PORT1_VLAN_ST_ADDR,&temp,2);
	set_vlan_sett_port_state(0,(u8)temp);

	Read_Eeprom(PORT2_VLAN_ST_ADDR,&temp,2);
	set_vlan_sett_port_state(1,(u8)temp);

	Read_Eeprom(PORT3_VLAN_ST_ADDR,&temp,2);
	set_vlan_sett_port_state(2,(u8)temp);

	Read_Eeprom(PORT4_VLAN_ST_ADDR,&temp,2);
	set_vlan_sett_port_state(3,(u8)temp);

	Read_Eeprom(PORT5_VLAN_ST_ADDR,&temp,2);
	set_vlan_sett_port_state(4,(u8)temp);

	Read_Eeprom(PORT6_VLAN_ST_ADDR,&temp,2);
	set_vlan_sett_port_state(5,(u8)temp);

	Read_Eeprom(PORT7_VLAN_ST_ADDR,&temp,2);
	set_vlan_sett_port_state(6,(u8)temp);

	Read_Eeprom(PORT8_VLAN_ST_ADDR,&temp,2);
	set_vlan_sett_port_state(7,(u8)temp);

	Read_Eeprom(PORT9_VLAN_ST_ADDR,&temp,2);
	set_vlan_sett_port_state(8,(u8)temp);

	Read_Eeprom(PORT10_VLAN_ST_ADDR,&temp,2);
	set_vlan_sett_port_state(9,(u8)temp);

	Read_Eeprom(PORT11_VLAN_ST_ADDR,&temp,2);
	set_vlan_sett_port_state(10,(u8)temp);

	Read_Eeprom(PORT12_VLAN_ST_ADDR,&temp,2);
	set_vlan_sett_port_state(11,(u8)temp);

	Read_Eeprom(PORT13_VLAN_ST_ADDR,&temp,2);
	set_vlan_sett_port_state(12,(u8)temp);

	Read_Eeprom(PORT14_VLAN_ST_ADDR,&temp,2);
	set_vlan_sett_port_state(13,(u8)temp);

	Read_Eeprom(PORT15_VLAN_ST_ADDR,&temp,2);
	set_vlan_sett_port_state(14,(u8)temp);

	Read_Eeprom(PORT16_VLAN_ST_ADDR,&temp,2);
	set_vlan_sett_port_state(15,(u8)temp);


	Read_Eeprom(PORT1_DEF_VID_ADDR,&temp,2);
	set_vlan_sett_dvid(0,temp);

	Read_Eeprom(PORT2_DEF_VID_ADDR,&temp,2);
	set_vlan_sett_dvid(1,temp);

	Read_Eeprom(PORT3_DEF_VID_ADDR,&temp,2);
	set_vlan_sett_dvid(2,temp);

	Read_Eeprom(PORT4_DEF_VID_ADDR,&temp,2);
	set_vlan_sett_dvid(3,temp);

	Read_Eeprom(PORT5_DEF_VID_ADDR,&temp,2);
	set_vlan_sett_dvid(4,temp);

	Read_Eeprom(PORT6_DEF_VID_ADDR,&temp,2);
	set_vlan_sett_dvid(5,temp);

	Read_Eeprom(PORT7_DEF_VID_ADDR,&temp,2);
	set_vlan_sett_dvid(6,temp);

	Read_Eeprom(PORT8_DEF_VID_ADDR,&temp,2);
	set_vlan_sett_dvid(7,temp);

	Read_Eeprom(PORT9_DEF_VID_ADDR,&temp,2);
	set_vlan_sett_dvid(8,temp);

	Read_Eeprom(PORT10_DEF_VID_ADDR,&temp,2);
	set_vlan_sett_dvid(9,temp);

	Read_Eeprom(PORT11_DEF_VID_ADDR,&temp,2);
	set_vlan_sett_dvid(10,temp);

	Read_Eeprom(PORT12_DEF_VID_ADDR,&temp,2);
	set_vlan_sett_dvid(11,temp);

	Read_Eeprom(PORT13_DEF_VID_ADDR,&temp,2);
	set_vlan_sett_dvid(12,temp);

	Read_Eeprom(PORT14_DEF_VID_ADDR,&temp,2);
	set_vlan_sett_dvid(13,temp);

	Read_Eeprom(PORT15_DEF_VID_ADDR,&temp,2);
	set_vlan_sett_dvid(14,temp);

	Read_Eeprom(PORT16_DEF_VID_ADDR,&temp,2);
	set_vlan_sett_dvid(15,temp);



	Read_Eeprom(VLAN_NUM_ADDR,&temp,2);
	set_vlan_sett_vlannum(temp);

	//первые 20
	for(u8 i=0;i<20;i++){
		Read_Eeprom(VLAN1_ADDR+i*80,&temp_vlan,sizeof(vlan_t));
		set_vlan_state(i,temp_vlan.state);
		set_vlan_vid(i,temp_vlan.VID);
		set_vlan_name(i,temp_vlan.VLANNAme);
		for(u8 j=0;j<(COOPER_PORT_NUM+FIBER_PORT_NUM);j++)
			set_vlan_port(i,j,temp_vlan.Ports[j]);
	}
	//остальные
	for(u8 i=0;i<MAXVlanNum-20;i++){
		Read_Eeprom(VLAN21_ADDR+i*80,&temp_vlan,sizeof(vlan_t));
		set_vlan_state(i+20,temp_vlan.state);
		set_vlan_vid(i+20,temp_vlan.VID);
		set_vlan_name(i+20,temp_vlan.VLANNAme);
		for(u8 j=0;j<(ALL_PORT_NUM);j++)
			set_vlan_port(i+20,j,temp_vlan.Ports[j]);
	}



	IWDG_ReloadCounter();

	Read_Eeprom(STP_STATE_ADDR,&temp,2);
	set_stp_state((u8)temp);

	Read_Eeprom(STP_MAGIC_ADDR,&temp,2);
	set_stp_magic(temp);

	Read_Eeprom(STP_PROTO_ADDR,&temp,2);
	set_stp_proto((u8)temp);

	Read_Eeprom(STP_BRIDGE_PRIOR_ADDR,&temp,2);
	set_stp_bridge_priority(temp);

	Read_Eeprom(STP_MAX_AGE_ADDR,&temp,2);
	set_stp_bridge_max_age((u8)temp);

	Read_Eeprom(STP_HELLO_TIME_ADDR,&temp,2);
	set_stp_bridge_htime((u8)temp);

	Read_Eeprom(STP_FORW_DELAY_ADDR,&temp,2);
	set_stp_bridge_fdelay((u8)temp);

	Read_Eeprom(STP_MIGRATE_DELAY_ADDR,&temp,2);
	set_stp_bridge_mdelay((u8)temp);

	Read_Eeprom(STP_TX_HCOUNT_ADDR,&temp,2);
	set_stp_txholdcount((u8)temp);



	for(u8 i=0;i<(ALL_PORT_NUM);i++){
		switch(i){
			case 0:Read_Eeprom(STP_PORT1_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 1:Read_Eeprom(STP_PORT2_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 2:Read_Eeprom(STP_PORT3_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 3:Read_Eeprom(STP_PORT4_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 4:Read_Eeprom(STP_PORT5_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 5:Read_Eeprom(STP_PORT6_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 6:Read_Eeprom(STP_PORT7_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 7:Read_Eeprom(STP_PORT8_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 8:Read_Eeprom(STP_PORT9_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 9:Read_Eeprom(STP_PORT10_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 10:Read_Eeprom(STP_PORT11_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 11:Read_Eeprom(STP_PORT12_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 12:Read_Eeprom(STP_PORT13_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 13:Read_Eeprom(STP_PORT14_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 14:Read_Eeprom(STP_PORT15_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 15:Read_Eeprom(STP_PORT16_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
		}
		set_stp_port_enable(i,stp_temp.enable);
		set_stp_port_state(i,stp_temp.state);
		set_stp_port_priority(i,stp_temp.priority);
		set_stp_port_cost(i,stp_temp.path_cost);

		if(stp_temp.flags & BSTP_PORTCFG_FLAG_ADMCOST)
			set_stp_port_autocost(i,1);
		else
			set_stp_port_autocost(i,0);

		if(stp_temp.flags & BSTP_PORTCFG_FLAG_AUTOEDGE)
			set_stp_port_autoedge(i,1);
		else
			set_stp_port_autoedge(i,0);

		if(stp_temp.flags & BSTP_PORTCFG_FLAG_EDGE)
			set_stp_port_edge(i,1);
		else
			set_stp_port_edge(i,0);

		if(stp_temp.flags & BSTP_PORTCFG_FLAG_AUTOPTP)
			set_stp_port_autoptp(i,1);
		else
			set_stp_port_autoptp(i,0);

		if(stp_temp.flags & BSTP_PORTCFG_FLAG_PTP)
			set_stp_port_ptp(i,1);
		else
			set_stp_port_ptp(i,0);
	}

	Read_Eeprom(STP_FWBPDU_ADDR,&temp,2);
	set_stp_bpdu_fw((u8)temp);


	//VCT
	Read_Eeprom(PORT1_VCT_ADJ_ADDR,&temp,2);
	set_callibrate_koef_1(0,temp);

	Read_Eeprom(PORT2_VCT_ADJ_ADDR,&temp,2);
	set_callibrate_koef_1(1,temp);

	Read_Eeprom(PORT3_VCT_ADJ_ADDR,&temp,2);
	set_callibrate_koef_1(2,temp);

	Read_Eeprom(PORT4_VCT_ADJ_ADDR,&temp,2);
	set_callibrate_koef_1(3,temp);

	Read_Eeprom(PORT5_VCT_ADJ_ADDR,&temp,2);
	set_callibrate_koef_1(4,temp);

	Read_Eeprom(PORT6_VCT_ADJ_ADDR,&temp,2);
	set_callibrate_koef_1(5,temp);

	Read_Eeprom(PORT7_VCT_ADJ_ADDR,&temp,2);
	set_callibrate_koef_1(6,temp);

	Read_Eeprom(PORT8_VCT_ADJ_ADDR,&temp,2);
	set_callibrate_koef_1(7,temp);

	Read_Eeprom(PORT9_VCT_ADJ_ADDR,&temp,2);
	set_callibrate_koef_1(8,temp);

	Read_Eeprom(PORT10_VCT_ADJ_ADDR,&temp,2);
	set_callibrate_koef_1(9,temp);

	Read_Eeprom(PORT11_VCT_ADJ_ADDR,&temp,2);
	set_callibrate_koef_1(10,temp);

	Read_Eeprom(PORT12_VCT_ADJ_ADDR,&temp,2);
	set_callibrate_koef_1(11,temp);

	Read_Eeprom(PORT13_VCT_ADJ_ADDR,&temp,2);
	set_callibrate_koef_1(12,temp);

	Read_Eeprom(PORT14_VCT_ADJ_ADDR,&temp,2);
	set_callibrate_koef_1(13,temp);

	Read_Eeprom(PORT15_VCT_ADJ_ADDR,&temp,2);
	set_callibrate_koef_1(14,temp);

	Read_Eeprom(PORT16_VCT_ADJ_ADDR,&temp,2);
	set_callibrate_koef_1(15,temp);



	Read_Eeprom(PORT1_VCT_LEN_ADDR,&temp,2);
	set_callibrate_len(0,(u8)temp);

	Read_Eeprom(PORT2_VCT_LEN_ADDR,&temp,2);
	set_callibrate_len(1,(u8)temp);

	Read_Eeprom(PORT3_VCT_LEN_ADDR,&temp,2);
	set_callibrate_len(2,(u8)temp);

	Read_Eeprom(PORT4_VCT_LEN_ADDR,&temp,2);
	set_callibrate_len(3,(u8)temp);

	Read_Eeprom(PORT5_VCT_LEN_ADDR,&temp,2);
	set_callibrate_len(4,(u8)temp);

	Read_Eeprom(PORT6_VCT_LEN_ADDR,&temp,2);
	set_callibrate_len(5,(u8)temp);

	Read_Eeprom(PORT7_VCT_LEN_ADDR,&temp,2);
	set_callibrate_len(6,(u8)temp);

	Read_Eeprom(PORT8_VCT_LEN_ADDR,&temp,2);
	set_callibrate_len(7,(u8)temp);

	Read_Eeprom(PORT9_VCT_LEN_ADDR,&temp,2);
	set_callibrate_len(8,(u8)temp);

	Read_Eeprom(PORT10_VCT_LEN_ADDR,&temp,2);
	set_callibrate_len(9,(u8)temp);

	Read_Eeprom(PORT11_VCT_LEN_ADDR,&temp,2);
	set_callibrate_len(10,(u8)temp);

	Read_Eeprom(PORT12_VCT_LEN_ADDR,&temp,2);
	set_callibrate_len(11,(u8)temp);

	Read_Eeprom(PORT13_VCT_LEN_ADDR,&temp,2);
	set_callibrate_len(12,(u8)temp);

	Read_Eeprom(PORT14_VCT_LEN_ADDR,&temp,2);
	set_callibrate_len(13,(u8)temp);

	Read_Eeprom(PORT15_VCT_LEN_ADDR,&temp,2);
	set_callibrate_len(14,(u8)temp);

	Read_Eeprom(PORT16_VCT_LEN_ADDR,&temp,2);
	set_callibrate_len(15,(u8)temp);



	Read_Eeprom(SNMP_STATE_ADDR,&temp,2);
	set_snmp_state((u8)temp);

	Read_Eeprom(SNMP_MODE_ADDR,&temp,2);
	set_snmp_mode((u8)temp);

	Read_Eeprom(SNMP_SERVER_ADDR,ip,4);
	set_snmp_serv(ip);

	Read_Eeprom(SNMP_VERS_ADDR,&temp,2);
	set_snmp_vers((u8)temp);

	Read_Eeprom(SNMP_COMMUNITY1_ADDR,&community_temp,68);
	set_snmp1_read_communitie(community_temp.community);

	Read_Eeprom(SNMP_COMMUNITY2_ADDR,&community_temp,68);
	set_snmp1_write_communitie(community_temp.community);

	Read_Eeprom(SNMP_COMMUNITY2_ADDR,&community_temp,68);
	set_snmp1_write_communitie(community_temp.community);

	//snmp3
	Read_Eeprom(SNMP3_USER1_LEVEL,&temp,2);
	set_snmp3_level(0,(u8)temp);

	Read_Eeprom(SNMP3_USER1_USER_NAME,tmp,64);
	set_snmp3_user_name(0,(char *)tmp);

	Read_Eeprom(SNMP3_USER1_AUTH_PASS,tmp,64);
	set_snmp3_auth_pass(0,(char *)tmp);

	Read_Eeprom(SNMP3_USER1_PRIV_PASS,tmp,64);
	set_snmp3_priv_pass(0,(char *)tmp);

	Read_Eeprom(SNMP3_USER2_LEVEL,&temp,2);
	set_snmp3_level(1,(u8)temp);

	Read_Eeprom(SNMP3_USER2_USER_NAME,tmp,64);
	set_snmp3_user_name(1,(char *)tmp);

	Read_Eeprom(SNMP3_USER2_AUTH_PASS,tmp,64);
	set_snmp3_auth_pass(1,(char *)tmp);

	Read_Eeprom(SNMP3_USER2_PRIV_PASS,tmp,64);
	set_snmp3_priv_pass(1,(char *)tmp);

	Read_Eeprom(SNMP3_USER3_LEVEL,&temp,2);
	set_snmp3_level(2,(u8)temp);

	Read_Eeprom(SNMP3_USER3_USER_NAME,tmp,64);
	set_snmp3_user_name(2,(char *)tmp);

	Read_Eeprom(SNMP3_USER3_AUTH_PASS,tmp,64);
	set_snmp3_auth_pass(2,(char *)tmp);

	Read_Eeprom(SNMP3_USER3_PRIV_PASS,tmp,64);
	set_snmp3_priv_pass(2,(char *)tmp);

	Read_Eeprom(SNMP3_ENGINE_ID_PTR,tmp,64);
	Read_Eeprom(SNMP3_ENGINE_ID_LEN,&temp,2);
	eid.len=temp;
	for(u8 i=0;i<64;i++)
		eid.ptr[i] = tmp[i];
	set_snmp3_engine_id(&eid);

	Read_Eeprom(PORT_SOFTSTART_TIME_ADDR,&temp,2);
	set_softstart_time(temp);

	//igmp settings
	Read_Eeprom(IGMP_STATE_ADDR,&temp,2);
	set_igmp_snooping_state((u8)temp);


	Read_Eeprom(IGMP_PORT_1_STATE_ADDR,&temp,2);
	set_igmp_port_state(0,(u8) temp);

	Read_Eeprom(IGMP_PORT_2_STATE_ADDR,&temp,2);
	set_igmp_port_state(1,(u8) temp);

	Read_Eeprom(IGMP_PORT_3_STATE_ADDR,&temp,2);
	set_igmp_port_state(2,(u8) temp);

	Read_Eeprom(IGMP_PORT_4_STATE_ADDR,&temp,2);
	set_igmp_port_state(3,(u8) temp);

	Read_Eeprom(IGMP_PORT_5_STATE_ADDR,&temp,2);
	set_igmp_port_state(4,(u8) temp);

	Read_Eeprom(IGMP_PORT_6_STATE_ADDR,&temp,2);
	set_igmp_port_state(5,(u8) temp);

	Read_Eeprom(IGMP_PORT_7_STATE_ADDR,&temp,2);
	set_igmp_port_state(6,(u8) temp);

	Read_Eeprom(IGMP_PORT_8_STATE_ADDR,&temp,2);
	set_igmp_port_state(7,(u8) temp);

	Read_Eeprom(IGMP_PORT_9_STATE_ADDR,&temp,2);
	set_igmp_port_state(8,(u8) temp);

	Read_Eeprom(IGMP_PORT_10_STATE_ADDR,&temp,2);
	set_igmp_port_state(9,(u8) temp);

	Read_Eeprom(IGMP_PORT_11_STATE_ADDR,&temp,2);
	set_igmp_port_state(10,(u8) temp);

	Read_Eeprom(IGMP_PORT_12_STATE_ADDR,&temp,2);
	set_igmp_port_state(11,(u8) temp);

	Read_Eeprom(IGMP_PORT_13_STATE_ADDR,&temp,2);
	set_igmp_port_state(12,(u8) temp);

	Read_Eeprom(IGMP_PORT_14_STATE_ADDR,&temp,2);
	set_igmp_port_state(13,(u8) temp);

	Read_Eeprom(IGMP_PORT_15_STATE_ADDR,&temp,2);
	set_igmp_port_state(14,(u8) temp);

	Read_Eeprom(IGMP_PORT_16_STATE_ADDR,&temp,2);
	set_igmp_port_state(15,(u8) temp);



	Read_Eeprom(IGMP_QUERY_INTERVAL_ADDR,&temp,2);
	set_igmp_query_int((u8) temp);

	Read_Eeprom(IGMP_QUERY_MODE_ADDR,&temp,2);
	set_igmp_query_mode((u8) temp);

	Read_Eeprom(IGMP_QUERY_RESP_INTERVAL_ADDR,&temp,2);
	set_igmp_max_resp_time((u8) temp);

	Read_Eeprom(IGMP_GROUP_MEMB_TIME_ADDR,&temp,2);
	set_igmp_group_membership_time((u8) temp);

	Read_Eeprom(IGMP_OTHER_QUERIER_ADDR,&temp,2);
	set_igmp_other_querier_time((u8) temp);

	Read_Eeprom(TELNET_STATE_ADDR,&temp,2);
	set_telnet_state((u8)temp);

	Read_Eeprom(TELNET_ECHO_ADDR,&temp,2);
	set_telnet_echo((u8)temp);

	Read_Eeprom(TFTP_STATE_ADDR,&temp,2);
	set_tftp_state((u8)temp);

	Read_Eeprom(TFTP_MODE_ADDR,&temp,2);
	set_tftp_mode((u8)temp);

	Read_Eeprom(TFTP_PORT_ADDR,&temp,2);
	set_tftp_port(temp);


	Read_Eeprom(DOWNSHIFT_STATE_ADDR,&temp,2);
	set_downshifting_mode((u8)temp);

	Read_Eeprom(PLC_OUT1_STATE_ADDR,&temp,2);
	set_plc_out_state(0,(u8)temp);

	Read_Eeprom(PLC_OUT2_STATE_ADDR,&temp,2);
	set_plc_out_state(1,(u8)temp);

	Read_Eeprom(PLC_OUT3_STATE_ADDR,&temp,2);
	set_plc_out_state(2,(u8)temp);

	Read_Eeprom(PLC_OUT4_STATE_ADDR,&temp,2);
	set_plc_out_state(3,(u8)temp);

	Read_Eeprom(PLC_OUT1_ACTION_ADDR,&temp,2);
	set_plc_out_action(0,(u8)temp);

	Read_Eeprom(PLC_OUT2_ACTION_ADDR,&temp,2);
	set_plc_out_action(1,(u8)temp);

	Read_Eeprom(PLC_OUT3_ACTION_ADDR,&temp,2);
	set_plc_out_action(2,(u8)temp);

	Read_Eeprom(PLC_OUT4_ACTION_ADDR,&temp,2);
	set_plc_out_action(3,(u8)temp);


	Read_Eeprom(PLC_OUT1_EVENT1_ADDR,&temp,2);
	set_plc_out_event(0,0,(u8)temp);

	Read_Eeprom(PLC_OUT1_EVENT2_ADDR,&temp,2);
	set_plc_out_event(0,1,(u8)temp);

	Read_Eeprom(PLC_OUT1_EVENT3_ADDR,&temp,2);
	set_plc_out_event(0,2,(u8)temp);

	Read_Eeprom(PLC_OUT1_EVENT4_ADDR,&temp,2);
	set_plc_out_event(0,3,(u8)temp);

	Read_Eeprom(PLC_OUT1_EVENT5_ADDR,&temp,2);
	set_plc_out_event(0,4,(u8)temp);

	Read_Eeprom(PLC_OUT1_EVENT6_ADDR,&temp,2);
	set_plc_out_event(0,5,(u8)temp);

	Read_Eeprom(PLC_OUT1_EVENT7_ADDR,&temp,2);
	set_plc_out_event(0,6,(u8)temp);



	Read_Eeprom(PLC_OUT2_EVENT1_ADDR,&temp,2);
	set_plc_out_event(1,0,(u8)temp);

	Read_Eeprom(PLC_OUT2_EVENT2_ADDR,&temp,2);
	set_plc_out_event(1,1,(u8)temp);

	Read_Eeprom(PLC_OUT2_EVENT3_ADDR,&temp,2);
	set_plc_out_event(1,2,(u8)temp);

	Read_Eeprom(PLC_OUT2_EVENT4_ADDR,&temp,2);
	set_plc_out_event(1,3,(u8)temp);

	Read_Eeprom(PLC_OUT2_EVENT5_ADDR,&temp,2);
	set_plc_out_event(1,4,(u8)temp);

	Read_Eeprom(PLC_OUT2_EVENT6_ADDR,&temp,2);
	set_plc_out_event(1,5,(u8)temp);

	Read_Eeprom(PLC_OUT2_EVENT7_ADDR,&temp,2);
	set_plc_out_event(1,6,(u8)temp);


	Read_Eeprom(PLC_OUT3_EVENT1_ADDR,&temp,2);
	set_plc_out_event(2,0,(u8)temp);

	Read_Eeprom(PLC_OUT3_EVENT2_ADDR,&temp,2);
	set_plc_out_event(2,1,(u8)temp);

	Read_Eeprom(PLC_OUT3_EVENT3_ADDR,&temp,2);
	set_plc_out_event(2,2,(u8)temp);

	Read_Eeprom(PLC_OUT3_EVENT4_ADDR,&temp,2);
	set_plc_out_event(2,3,(u8)temp);

	Read_Eeprom(PLC_OUT3_EVENT5_ADDR,&temp,2);
	set_plc_out_event(2,4,(u8)temp);

	Read_Eeprom(PLC_OUT3_EVENT6_ADDR,&temp,2);
	set_plc_out_event(2,5,(u8)temp);

	Read_Eeprom(PLC_OUT3_EVENT7_ADDR,&temp,2);
	set_plc_out_event(2,6,(u8)temp);


	Read_Eeprom(PLC_OUT4_EVENT1_ADDR,&temp,2);
	set_plc_out_event(3,0,(u8)temp);

	Read_Eeprom(PLC_OUT4_EVENT2_ADDR,&temp,2);
	set_plc_out_event(3,1,(u8)temp);

	Read_Eeprom(PLC_OUT4_EVENT3_ADDR,&temp,2);
	set_plc_out_event(3,2,(u8)temp);

	Read_Eeprom(PLC_OUT4_EVENT4_ADDR,&temp,2);
	set_plc_out_event(3,3,(u8)temp);

	Read_Eeprom(PLC_OUT4_EVENT5_ADDR,&temp,2);
	set_plc_out_event(3,4,(u8)temp);

	Read_Eeprom(PLC_OUT4_EVENT6_ADDR,&temp,2);
	set_plc_out_event(3,5,(u8)temp);

	Read_Eeprom(PLC_OUT4_EVENT7_ADDR,&temp,2);
	set_plc_out_event(3,6,(u8)temp);



	Read_Eeprom(PLC_IN1_STATE_ADDR,&temp,2);
	set_plc_in_state(0,(u8)temp);

	Read_Eeprom(PLC_IN2_STATE_ADDR,&temp,2);
	set_plc_in_state(1,(u8)temp);

	Read_Eeprom(PLC_IN3_STATE_ADDR,&temp,2);
	set_plc_in_state(2,(u8)temp);

	Read_Eeprom(PLC_IN4_STATE_ADDR,&temp,2);
	set_plc_in_state(3,(u8)temp);



	Read_Eeprom(PLC_IN1_ALARM_ADDR,&temp,2);
	set_plc_in_alarm_state(0,(u8)temp);

	Read_Eeprom(PLC_IN2_ALARM_ADDR,&temp,2);
	set_plc_in_alarm_state(1,(u8)temp);

	Read_Eeprom(PLC_IN3_ALARM_ADDR,&temp,2);
	set_plc_in_alarm_state(2,(u8)temp);

	Read_Eeprom(PLC_IN4_ALARM_ADDR,&temp,2);
	set_plc_in_alarm_state(3,(u8)temp);



	Read_Eeprom(PLC_EM_MODEL_ADDR,&temp,2);
	set_plc_em_model((u16)temp);

	Read_Eeprom(PLC_EM_BAUDRATE_ADDR,&temp,2);
	set_plc_em_rate((u8)temp);

	Read_Eeprom(PLC_EM_PARITY_ADDR,&temp,2);
	set_plc_em_parity((u8)temp);

	Read_Eeprom(PLC_EM_DATABITS_ADDR,&temp,2);
	set_plc_em_databits((u8)temp);

	Read_Eeprom(PLC_EM_STOPBITS_ADDR,&temp,2);
	set_plc_em_stopbits((u8)temp);

	Read_Eeprom(PLC_EM_PASS_ADDR,tmp,10);
	set_plc_em_pass((char *)tmp);

	Read_Eeprom(PLC_EM_ID_ADDR,tmp,32);
	set_plc_em_id((char *)tmp);

	//mac learning on port
	Read_Eeprom(PORT1_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(0,(u8)temp);

	Read_Eeprom(PORT2_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(1,(u8)temp);

	Read_Eeprom(PORT3_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(2,(u8)temp);

	Read_Eeprom(PORT4_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(3,(u8)temp);

	Read_Eeprom(PORT5_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(4,(u8)temp);

	Read_Eeprom(PORT6_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(5,(u8)temp);

	Read_Eeprom(PORT7_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(6,(u8)temp);

	Read_Eeprom(PORT8_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(7,(u8)temp);

	Read_Eeprom(PORT9_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(8,(u8)temp);

	Read_Eeprom(PORT10_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(9,(u8)temp);

	Read_Eeprom(PORT11_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(10,(u8)temp);

	Read_Eeprom(PORT12_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(11,(u8)temp);

	Read_Eeprom(PORT13_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(12,(u8)temp);

	Read_Eeprom(PORT14_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(13,(u8)temp);

	Read_Eeprom(PORT15_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(14,(u8)temp);

	Read_Eeprom(PORT16_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(15,(u8)temp);

	Read_Eeprom(PORT17_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(16,(u8)temp);

	Read_Eeprom(PORT18_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(17,(u8)temp);

	Read_Eeprom(PORT19_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(18,(u8)temp);

	Read_Eeprom(PORT20_MACFILT_ADDR,&temp,2);
	set_mac_filter_state(19,(u8)temp);


	//cpu port
	Read_Eeprom(CPU_MACLEARN_ADDR,&temp,2);
	set_mac_learn_cpu((u8)temp);


	//MAC FILTERING TABLE
	Read_Eeprom(MAC_BIND_ENRTY1_ACTIVE,&temp,2);
	set_mac_bind_entry_active(0,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY1_MAC,tmp,6);
	set_mac_bind_entry_mac(0,tmp);
	Read_Eeprom(MAC_BIND_ENRTY1_PORT,&temp,2);
	set_mac_bind_entry_port(0,temp);

	Read_Eeprom(MAC_BIND_ENRTY2_ACTIVE,&temp,2);
	set_mac_bind_entry_active(1,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY2_MAC,tmp,6);
	set_mac_bind_entry_mac(1,tmp);
	Read_Eeprom(MAC_BIND_ENRTY2_PORT,&temp,2);
	set_mac_bind_entry_port(1,temp);

	Read_Eeprom(MAC_BIND_ENRTY3_ACTIVE,&temp,2);
	set_mac_bind_entry_active(2,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY3_MAC,tmp,6);
	set_mac_bind_entry_mac(2,tmp);
	Read_Eeprom(MAC_BIND_ENRTY3_PORT,&temp,2);
	set_mac_bind_entry_port(2,temp);

	Read_Eeprom(MAC_BIND_ENRTY4_ACTIVE,&temp,2);
	set_mac_bind_entry_active(3,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY4_MAC,tmp,6);
	set_mac_bind_entry_mac(3,tmp);
	Read_Eeprom(MAC_BIND_ENRTY4_PORT,&temp,2);
	set_mac_bind_entry_port(3,temp);

	Read_Eeprom(MAC_BIND_ENRTY5_ACTIVE,&temp,2);
	set_mac_bind_entry_active(4,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY5_MAC,tmp,6);
	set_mac_bind_entry_mac(4,tmp);
	Read_Eeprom(MAC_BIND_ENRTY5_PORT,&temp,2);
	set_mac_bind_entry_port(4,temp);

	Read_Eeprom(MAC_BIND_ENRTY6_ACTIVE,&temp,2);
	set_mac_bind_entry_active(5,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY6_MAC,tmp,6);
	set_mac_bind_entry_mac(5,tmp);
	Read_Eeprom(MAC_BIND_ENRTY6_PORT,&temp,2);
	set_mac_bind_entry_port(5,temp);

	Read_Eeprom(MAC_BIND_ENRTY7_ACTIVE,&temp,2);
	set_mac_bind_entry_active(6,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY7_MAC,tmp,6);
	set_mac_bind_entry_mac(6,tmp);
	Read_Eeprom(MAC_BIND_ENRTY7_PORT,&temp,2);
	set_mac_bind_entry_port(6,temp);

	Read_Eeprom(MAC_BIND_ENRTY8_ACTIVE,&temp,2);
	set_mac_bind_entry_active(7,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY8_MAC,tmp,6);
	set_mac_bind_entry_mac(7,tmp);
	Read_Eeprom(MAC_BIND_ENRTY8_PORT,&temp,2);
	set_mac_bind_entry_port(7,temp);

	Read_Eeprom(MAC_BIND_ENRTY9_ACTIVE,&temp,2);
	set_mac_bind_entry_active(8,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY9_MAC,tmp,6);
	set_mac_bind_entry_mac(8,tmp);
	Read_Eeprom(MAC_BIND_ENRTY9_PORT,&temp,2);
	set_mac_bind_entry_port(8,temp);

	Read_Eeprom(MAC_BIND_ENRTY10_ACTIVE,&temp,2);
	set_mac_bind_entry_active(9,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY10_MAC,tmp,6);
	set_mac_bind_entry_mac(9,tmp);
	Read_Eeprom(MAC_BIND_ENRTY10_PORT,&temp,2);
	set_mac_bind_entry_port(9,temp);

	Read_Eeprom(MAC_BIND_ENRTY11_ACTIVE,&temp,2);
	set_mac_bind_entry_active(10,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY11_MAC,tmp,6);
	set_mac_bind_entry_mac(10,tmp);
	Read_Eeprom(MAC_BIND_ENRTY11_PORT,&temp,2);
	set_mac_bind_entry_port(10,temp);

	Read_Eeprom(MAC_BIND_ENRTY12_ACTIVE,&temp,2);
	set_mac_bind_entry_active(11,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY12_MAC,tmp,6);
	set_mac_bind_entry_mac(11,tmp);
	Read_Eeprom(MAC_BIND_ENRTY12_PORT,&temp,2);
	set_mac_bind_entry_port(11,temp);

	Read_Eeprom(MAC_BIND_ENRTY13_ACTIVE,&temp,2);
	set_mac_bind_entry_active(12,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY13_MAC,tmp,6);
	set_mac_bind_entry_mac(12,tmp);
	Read_Eeprom(MAC_BIND_ENRTY13_PORT,&temp,2);
	set_mac_bind_entry_port(12,temp);

	Read_Eeprom(MAC_BIND_ENRTY14_ACTIVE,&temp,2);
	set_mac_bind_entry_active(13,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY14_MAC,tmp,6);
	set_mac_bind_entry_mac(13,tmp);
	Read_Eeprom(MAC_BIND_ENRTY14_PORT,&temp,2);
	set_mac_bind_entry_port(13,temp);

	Read_Eeprom(MAC_BIND_ENRTY15_ACTIVE,&temp,2);
	set_mac_bind_entry_active(14,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY15_MAC,tmp,6);
	set_mac_bind_entry_mac(14,tmp);
	Read_Eeprom(MAC_BIND_ENRTY15_PORT,&temp,2);
	set_mac_bind_entry_port(14,temp);

	Read_Eeprom(MAC_BIND_ENRTY16_ACTIVE,&temp,2);
	set_mac_bind_entry_active(15,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY16_MAC,tmp,6);
	set_mac_bind_entry_mac(15,tmp);
	Read_Eeprom(MAC_BIND_ENRTY16_PORT,&temp,2);
	set_mac_bind_entry_port(15,temp);

	Read_Eeprom(MAC_BIND_ENRTY17_ACTIVE,&temp,2);
	set_mac_bind_entry_active(16,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY17_MAC,tmp,6);
	set_mac_bind_entry_mac(16,tmp);
	Read_Eeprom(MAC_BIND_ENRTY17_PORT,&temp,2);
	set_mac_bind_entry_port(16,temp);

	Read_Eeprom(MAC_BIND_ENRTY18_ACTIVE,&temp,2);
	set_mac_bind_entry_active(17,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY18_MAC,tmp,6);
	set_mac_bind_entry_mac(17,tmp);
	Read_Eeprom(MAC_BIND_ENRTY18_PORT,&temp,2);
	set_mac_bind_entry_port(17,temp);

	Read_Eeprom(MAC_BIND_ENRTY19_ACTIVE,&temp,2);
	set_mac_bind_entry_active(18,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY19_MAC,tmp,6);
	set_mac_bind_entry_mac(18,tmp);
	Read_Eeprom(MAC_BIND_ENRTY19_PORT,&temp,2);
	set_mac_bind_entry_port(18,temp);

	Read_Eeprom(MAC_BIND_ENRTY20_ACTIVE,&temp,2);
	set_mac_bind_entry_active(19,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY20_MAC,tmp,6);
	set_mac_bind_entry_mac(19,tmp);
	Read_Eeprom(MAC_BIND_ENRTY20_PORT,&temp,2);
	set_mac_bind_entry_port(19,temp);

	Read_Eeprom(MAC_BIND_ENRTY21_ACTIVE,&temp,2);
	set_mac_bind_entry_active(20,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY21_MAC,tmp,6);
	set_mac_bind_entry_mac(20,tmp);
	Read_Eeprom(MAC_BIND_ENRTY21_PORT,&temp,2);
	set_mac_bind_entry_port(20,temp);

	Read_Eeprom(MAC_BIND_ENRTY22_ACTIVE,&temp,2);
	set_mac_bind_entry_active(21,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY22_MAC,tmp,6);
	set_mac_bind_entry_mac(21,tmp);
	Read_Eeprom(MAC_BIND_ENRTY22_PORT,&temp,2);
	set_mac_bind_entry_port(21,temp);

	Read_Eeprom(MAC_BIND_ENRTY23_ACTIVE,&temp,2);
	set_mac_bind_entry_active(22,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY23_MAC,tmp,6);
	set_mac_bind_entry_mac(22,tmp);
	Read_Eeprom(MAC_BIND_ENRTY23_PORT,&temp,2);
	set_mac_bind_entry_port(22,temp);

	Read_Eeprom(MAC_BIND_ENRTY24_ACTIVE,&temp,2);
	set_mac_bind_entry_active(23,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY24_MAC,tmp,6);
	set_mac_bind_entry_mac(23,tmp);
	Read_Eeprom(MAC_BIND_ENRTY24_PORT,&temp,2);
	set_mac_bind_entry_port(23,temp);

	Read_Eeprom(MAC_BIND_ENRTY25_ACTIVE,&temp,2);
	set_mac_bind_entry_active(24,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY25_MAC,tmp,6);
	set_mac_bind_entry_mac(24,tmp);
	Read_Eeprom(MAC_BIND_ENRTY25_PORT,&temp,2);
	set_mac_bind_entry_port(24,temp);

	Read_Eeprom(MAC_BIND_ENRTY26_ACTIVE,&temp,2);
	set_mac_bind_entry_active(25,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY26_MAC,tmp,6);
	set_mac_bind_entry_mac(25,tmp);
	Read_Eeprom(MAC_BIND_ENRTY26_PORT,&temp,2);
	set_mac_bind_entry_port(25,temp);

	Read_Eeprom(MAC_BIND_ENRTY27_ACTIVE,&temp,2);
	set_mac_bind_entry_active(26,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY27_MAC,tmp,6);
	set_mac_bind_entry_mac(26,tmp);
	Read_Eeprom(MAC_BIND_ENRTY27_PORT,&temp,2);
	set_mac_bind_entry_port(26,temp);

	Read_Eeprom(MAC_BIND_ENRTY28_ACTIVE,&temp,2);
	set_mac_bind_entry_active(27,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY28_MAC,tmp,6);
	set_mac_bind_entry_mac(27,tmp);
	Read_Eeprom(MAC_BIND_ENRTY28_PORT,&temp,2);
	set_mac_bind_entry_port(27,temp);

	Read_Eeprom(MAC_BIND_ENRTY29_ACTIVE,&temp,2);
	set_mac_bind_entry_active(28,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY29_MAC,tmp,6);
	set_mac_bind_entry_mac(28,tmp);
	Read_Eeprom(MAC_BIND_ENRTY29_PORT,&temp,2);
	set_mac_bind_entry_port(28,temp);

	Read_Eeprom(MAC_BIND_ENRTY30_ACTIVE,&temp,2);
	set_mac_bind_entry_active(29,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY30_MAC,tmp,6);
	set_mac_bind_entry_mac(29,tmp);
	Read_Eeprom(MAC_BIND_ENRTY30_PORT,&temp,2);
	set_mac_bind_entry_port(29,temp);

	Read_Eeprom(MAC_BIND_ENRTY31_ACTIVE,&temp,2);
	set_mac_bind_entry_active(30,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY31_MAC,tmp,6);
	set_mac_bind_entry_mac(30,tmp);
	Read_Eeprom(MAC_BIND_ENRTY31_PORT,&temp,2);
	set_mac_bind_entry_port(30,temp);

	Read_Eeprom(MAC_BIND_ENRTY32_ACTIVE,&temp,2);
	set_mac_bind_entry_active(31,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY32_MAC,tmp,6);
	set_mac_bind_entry_mac(31,tmp);
	Read_Eeprom(MAC_BIND_ENRTY32_PORT,&temp,2);
	set_mac_bind_entry_port(31,temp);

	Read_Eeprom(MAC_BIND_ENRTY33_ACTIVE,&temp,2);
	set_mac_bind_entry_active(32,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY33_MAC,tmp,6);
	set_mac_bind_entry_mac(32,tmp);
	Read_Eeprom(MAC_BIND_ENRTY33_PORT,&temp,2);
	set_mac_bind_entry_port(32,temp);

	Read_Eeprom(MAC_BIND_ENRTY34_ACTIVE,&temp,2);
	set_mac_bind_entry_active(33,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY34_MAC,tmp,6);
	set_mac_bind_entry_mac(33,tmp);
	Read_Eeprom(MAC_BIND_ENRTY34_PORT,&temp,2);
	set_mac_bind_entry_port(33,temp);

	Read_Eeprom(MAC_BIND_ENRTY35_ACTIVE,&temp,2);
	set_mac_bind_entry_active(34,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY35_MAC,tmp,6);
	set_mac_bind_entry_mac(34,tmp);
	Read_Eeprom(MAC_BIND_ENRTY35_PORT,&temp,2);
	set_mac_bind_entry_port(34,temp);

	Read_Eeprom(MAC_BIND_ENRTY36_ACTIVE,&temp,2);
	set_mac_bind_entry_active(35,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY36_MAC,tmp,6);
	set_mac_bind_entry_mac(35,tmp);
	Read_Eeprom(MAC_BIND_ENRTY36_PORT,&temp,2);
	set_mac_bind_entry_port(35,temp);

	Read_Eeprom(MAC_BIND_ENRTY37_ACTIVE,&temp,2);
	set_mac_bind_entry_active(36,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY37_MAC,tmp,6);
	set_mac_bind_entry_mac(36,tmp);
	Read_Eeprom(MAC_BIND_ENRTY37_PORT,&temp,2);
	set_mac_bind_entry_port(36,temp);

	Read_Eeprom(MAC_BIND_ENRTY38_ACTIVE,&temp,2);
	set_mac_bind_entry_active(37,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY38_MAC,tmp,6);
	set_mac_bind_entry_mac(37,tmp);
	Read_Eeprom(MAC_BIND_ENRTY38_PORT,&temp,2);
	set_mac_bind_entry_port(37,temp);

	Read_Eeprom(MAC_BIND_ENRTY39_ACTIVE,&temp,2);
	set_mac_bind_entry_active(38,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY39_MAC,tmp,6);
	set_mac_bind_entry_mac(38,tmp);
	Read_Eeprom(MAC_BIND_ENRTY39_PORT,&temp,2);
	set_mac_bind_entry_port(38,temp);

	Read_Eeprom(MAC_BIND_ENRTY40_ACTIVE,&temp,2);
	set_mac_bind_entry_active(39,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY40_MAC,tmp,6);
	set_mac_bind_entry_mac(39,tmp);
	Read_Eeprom(MAC_BIND_ENRTY40_PORT,&temp,2);
	set_mac_bind_entry_port(39,temp);

	Read_Eeprom(MAC_BIND_ENRTY41_ACTIVE,&temp,2);
	set_mac_bind_entry_active(40,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY41_MAC,tmp,6);
	set_mac_bind_entry_mac(40,tmp);
	Read_Eeprom(MAC_BIND_ENRTY41_PORT,&temp,2);
	set_mac_bind_entry_port(40,temp);

	Read_Eeprom(MAC_BIND_ENRTY42_ACTIVE,&temp,2);
	set_mac_bind_entry_active(41,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY42_MAC,tmp,6);
	set_mac_bind_entry_mac(41,tmp);
	Read_Eeprom(MAC_BIND_ENRTY42_PORT,&temp,2);
	set_mac_bind_entry_port(41,temp);

	Read_Eeprom(MAC_BIND_ENRTY43_ACTIVE,&temp,2);
	set_mac_bind_entry_active(42,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY43_MAC,tmp,6);
	set_mac_bind_entry_mac(42,tmp);
	Read_Eeprom(MAC_BIND_ENRTY43_PORT,&temp,2);
	set_mac_bind_entry_port(42,temp);

	Read_Eeprom(MAC_BIND_ENRTY44_ACTIVE,&temp,2);
	set_mac_bind_entry_active(43,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY44_MAC,tmp,6);
	set_mac_bind_entry_mac(43,tmp);
	Read_Eeprom(MAC_BIND_ENRTY44_PORT,&temp,2);
	set_mac_bind_entry_port(43,temp);

	Read_Eeprom(MAC_BIND_ENRTY45_ACTIVE,&temp,2);
	set_mac_bind_entry_active(44,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY45_MAC,tmp,6);
	set_mac_bind_entry_mac(44,tmp);
	Read_Eeprom(MAC_BIND_ENRTY45_PORT,&temp,2);
	set_mac_bind_entry_port(44,temp);

	Read_Eeprom(MAC_BIND_ENRTY46_ACTIVE,&temp,2);
	set_mac_bind_entry_active(45,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY46_MAC,tmp,6);
	set_mac_bind_entry_mac(45,tmp);
	Read_Eeprom(MAC_BIND_ENRTY46_PORT,&temp,2);
	set_mac_bind_entry_port(45,temp);

	Read_Eeprom(MAC_BIND_ENRTY47_ACTIVE,&temp,2);
	set_mac_bind_entry_active(46,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY47_MAC,tmp,6);
	set_mac_bind_entry_mac(46,tmp);
	Read_Eeprom(MAC_BIND_ENRTY47_PORT,&temp,2);
	set_mac_bind_entry_port(46,temp);

	Read_Eeprom(MAC_BIND_ENRTY48_ACTIVE,&temp,2);
	set_mac_bind_entry_active(47,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY48_MAC,tmp,6);
	set_mac_bind_entry_mac(47,tmp);
	Read_Eeprom(MAC_BIND_ENRTY48_PORT,&temp,2);
	set_mac_bind_entry_port(47,temp);

	Read_Eeprom(MAC_BIND_ENRTY49_ACTIVE,&temp,2);
	set_mac_bind_entry_active(48,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY49_MAC,tmp,6);
	set_mac_bind_entry_mac(48,tmp);
	Read_Eeprom(MAC_BIND_ENRTY49_PORT,&temp,2);
	set_mac_bind_entry_port(48,temp);

	Read_Eeprom(MAC_BIND_ENRTY50_ACTIVE,&temp,2);
	set_mac_bind_entry_active(49,(u8)temp);
	Read_Eeprom(MAC_BIND_ENRTY50_MAC,tmp,6);
	set_mac_bind_entry_mac(49,tmp);
	Read_Eeprom(MAC_BIND_ENRTY50_PORT,&temp,2);
	set_mac_bind_entry_port(49,temp);




	//ups settings
	Read_Eeprom(UPS_DELAYED_START_ADDR,&temp,2);
	set_ups_delayed_start((u8)temp);


	//link aggregation
	Read_Eeprom(LAG_ENTRY1_VALID_ADDR,&temp,2);
	set_lag_valid(0,(u8)temp);
	Read_Eeprom(LAG_ENTRY1_STATE_ADDR,&temp,2);
	set_lag_state(0,(u8)temp);
	Read_Eeprom(LAG_ENTRY1_MASTER_ADDR,&temp,2);
	set_lag_master_port(0,(u8)temp);
	Read_Eeprom(LAG_ENTRY1_PORTS_ADDR,tmp,32);
	for(u8 i=0;i<PORT_NUM;i++)
		set_lag_port(0,i,tmp[i]);
	Read_Eeprom(LAG_ENTRY2_VALID_ADDR,&temp,2);
	set_lag_valid(1,(u8)temp);
	Read_Eeprom(LAG_ENTRY2_STATE_ADDR,&temp,2);
	set_lag_state(1,(u8)temp);
	Read_Eeprom(LAG_ENTRY2_MASTER_ADDR,&temp,2);
	set_lag_master_port(1,(u8)temp);
	Read_Eeprom(LAG_ENTRY2_PORTS_ADDR,tmp,32);
	for(u8 i=0;i<PORT_NUM;i++)
		set_lag_port(1,i,tmp[i]);
	Read_Eeprom(LAG_ENTRY3_VALID_ADDR,&temp,2);
	set_lag_valid(2,(u8)temp);
	Read_Eeprom(LAG_ENTRY3_STATE_ADDR,&temp,2);
	set_lag_state(2,(u8)temp);
	Read_Eeprom(LAG_ENTRY3_MASTER_ADDR,&temp,2);
	set_lag_master_port(2,(u8)temp);
	Read_Eeprom(LAG_ENTRY3_PORTS_ADDR,tmp,32);
	for(u8 i=0;i<PORT_NUM;i++)
		set_lag_port(2,i,tmp[i]);
	Read_Eeprom(LAG_ENTRY4_VALID_ADDR,&temp,2);
	set_lag_valid(3,(u8)temp);
	Read_Eeprom(LAG_ENTRY4_STATE_ADDR,&temp,2);
	set_lag_state(3,(u8)temp);
	Read_Eeprom(LAG_ENTRY4_MASTER_ADDR,&temp,2);
	set_lag_master_port(3,(u8)temp);
	Read_Eeprom(LAG_ENTRY4_PORTS_ADDR,tmp,32);
	for(u8 i=0;i<PORT_NUM;i++)
		set_lag_port(3,i,tmp[i]);
	Read_Eeprom(LAG_ENTRY5_VALID_ADDR,&temp,2);
	set_lag_valid(4,(u8)temp);
	Read_Eeprom(LAG_ENTRY5_STATE_ADDR,&temp,2);
	set_lag_state(4,(u8)temp);
	Read_Eeprom(LAG_ENTRY5_MASTER_ADDR,&temp,2);
	set_lag_master_port(4,(u8)temp);
	Read_Eeprom(LAG_ENTRY5_PORTS_ADDR,tmp,32);
	for(u8 i=0;i<PORT_NUM;i++)
		set_lag_port(4,i,tmp[i]);

	//port mirroring
	Read_Eeprom(MIRROR_ENTRY_STATE_ADDR,&temp,2);
	set_mirror_state((u8)temp);
	Read_Eeprom(MIRROR_ENTRY_TARGET_ADDR,&temp,2);
	set_mirror_target_port((u8)temp);
	Read_Eeprom(MIRROR_ENTRY_PORTS_ADDR,tmp,32);
	for(u8 i=0;i<PORT_NUM;i++)
		set_mirror_port(i,tmp[i]);

	//teleport
	Read_Eeprom(INPUT1_STATE_ADDR,&temp,2);
	set_input_state(0,(u8)temp);
	Read_Eeprom(INPUT2_STATE_ADDR,&temp,2);
	set_input_state(1,(u8)temp);
	Read_Eeprom(INPUT3_STATE_ADDR,&temp,2);
	set_input_state(2,(u8)temp);

	Read_Eeprom(INPUT1_INVERSE_ADDR,&temp,2);
	set_input_inverse(0,(u8)temp);
	Read_Eeprom(INPUT2_INVERSE_ADDR,&temp,2);
	set_input_inverse(1,(u8)temp);
	Read_Eeprom(INPUT3_INVERSE_ADDR,&temp,2);
	set_input_inverse(2,(u8)temp);

	Read_Eeprom(INPUT1_REMDEV_ADDR,&temp,2);
	set_input_remdev(0,(u8)temp);
	Read_Eeprom(INPUT2_REMDEV_ADDR,&temp,2);
	set_input_remdev(1,(u8)temp);
	Read_Eeprom(INPUT3_REMDEV_ADDR,&temp,2);
	set_input_remdev(2,(u8)temp);

	Read_Eeprom(INPUT1_REMPORT_ADDR,&temp,2);
	set_input_remport(0,(u8)temp);
	Read_Eeprom(INPUT2_REMPORT_ADDR,&temp,2);
	set_input_remport(1,(u8)temp);
	Read_Eeprom(INPUT3_REMPORT_ADDR,&temp,2);
	set_input_remport(2,(u8)temp);

	Read_Eeprom(TLP_EVENT1_STATE_ADDR,&temp,2);
	set_tlp_event_state(0,(u8)temp);
	Read_Eeprom(TLP_EVENT2_STATE_ADDR,&temp,2);
	set_tlp_event_state(1,(u8)temp);

	Read_Eeprom(TLP_EVENT1_INVERSE_ADDR,&temp,2);
	set_tlp_event_inverse(0,(u8)temp);
	Read_Eeprom(TLP_EVENT2_INVERSE_ADDR,&temp,2);
	set_tlp_event_inverse(1,(u8)temp);

	Read_Eeprom(TLP_EVENT1_REMDEV_ADDR,&temp,2);
	set_tlp_event_remdev(0,(u8)temp);
	Read_Eeprom(TLP_EVENT2_REMDEV_ADDR,&temp,2);
	set_tlp_event_remdev(1,(u8)temp);

	Read_Eeprom(TLP_EVENT1_REMPORT_ADDR,&temp,2);
	set_tlp_event_remport(0,(u8)temp);
	Read_Eeprom(TLP_EVENT2_REMPORT_ADDR,&temp,2);
	set_tlp_event_remport(1,(u8)temp);

	Read_Eeprom(TLP1_VALID,&temp,2);
	set_tlp_remdev_valid(0,(u8)temp);
	Read_Eeprom(TLP2_VALID,&temp,2);
	set_tlp_remdev_valid(1,(u8)temp);
	Read_Eeprom(TLP3_VALID,&temp,2);
	set_tlp_remdev_valid(2,(u8)temp);
	Read_Eeprom(TLP4_VALID,&temp,2);
	set_tlp_remdev_valid(3,(u8)temp);

	Read_Eeprom(TLP1_TYPE,&temp,2);
	set_tlp_remdev_type(0,(u8)temp);
	Read_Eeprom(TLP2_TYPE,&temp,2);
	set_tlp_remdev_type(1,(u8)temp);
	Read_Eeprom(TLP3_TYPE,&temp,2);
	set_tlp_remdev_type(2,(u8)temp);
	Read_Eeprom(TLP4_TYPE,&temp,2);
	set_tlp_remdev_type(3,(u8)temp);

	Read_Eeprom(TLP1_DESCR,tmp,64);
	set_tlp_remdev_name(0,(char *)tmp);
	Read_Eeprom(TLP2_DESCR,tmp,64);
	set_tlp_remdev_name(1,(char *)tmp);
	Read_Eeprom(TLP3_DESCR,tmp,64);
	set_tlp_remdev_name(2,(char *)tmp);
	Read_Eeprom(TLP4_DESCR,tmp,64);
	set_tlp_remdev_name(3,(char *)tmp);

	Read_Eeprom(TLP1_IP,&ip,4);
	set_tlp_remdev_ip(0,&ip);
	Read_Eeprom(TLP2_IP,&ip,4);
	set_tlp_remdev_ip(1,&ip);
	Read_Eeprom(TLP3_IP,&ip,4);
	set_tlp_remdev_ip(2,&ip);
	Read_Eeprom(TLP4_IP,&ip,4);
	set_tlp_remdev_ip(3,&ip);

	Read_Eeprom(TLP1_MASK,&ip,4);
	set_tlp_remdev_mask(0,&ip);
	Read_Eeprom(TLP2_MASK,&ip,4);
	set_tlp_remdev_mask(1,&ip);
	Read_Eeprom(TLP3_MASK,&ip,4);
	set_tlp_remdev_mask(2,&ip);
	Read_Eeprom(TLP4_MASK,&ip,4);
	set_tlp_remdev_mask(3,&ip);

	Read_Eeprom(TLP1_GATE,&ip,4);
	set_tlp_remdev_gate(0,&ip);
	Read_Eeprom(TLP2_GATE,&ip,4);
	set_tlp_remdev_gate(1,&ip);
	Read_Eeprom(TLP3_GATE,&ip,4);
	set_tlp_remdev_gate(2,&ip);
	Read_Eeprom(TLP4_GATE,&ip,4);
	set_tlp_remdev_gate(3,&ip);


	Read_Eeprom(LLDP_STATE_ADDR,&temp,2);
	set_lldp_state(temp);
	Read_Eeprom(LLDP_TX_INT_ADDR,&temp,2);
	set_lldp_transmit_interval(temp);
	Read_Eeprom(LLDP_HOLD_TIME_ADDR,&temp,2);
	set_lldp_hold_multiplier(temp);

	Read_Eeprom(LLDP_PORT1_STATE_ADDR,&temp,2);
	set_lldp_port_state(0,temp);
	Read_Eeprom(LLDP_PORT2_STATE_ADDR,&temp,2);
	set_lldp_port_state(1,temp);
	Read_Eeprom(LLDP_PORT3_STATE_ADDR,&temp,2);
	set_lldp_port_state(2,temp);
	Read_Eeprom(LLDP_PORT4_STATE_ADDR,&temp,2);
	set_lldp_port_state(3,temp);
	Read_Eeprom(LLDP_PORT5_STATE_ADDR,&temp,2);
	set_lldp_port_state(4,temp);
	Read_Eeprom(LLDP_PORT6_STATE_ADDR,&temp,2);
	set_lldp_port_state(5,temp);
	Read_Eeprom(LLDP_PORT7_STATE_ADDR,&temp,2);
	set_lldp_port_state(6,temp);
	Read_Eeprom(LLDP_PORT8_STATE_ADDR,&temp,2);
	set_lldp_port_state(7,temp);
	Read_Eeprom(LLDP_PORT9_STATE_ADDR,&temp,2);
	set_lldp_port_state(8,temp);
	Read_Eeprom(LLDP_PORT10_STATE_ADDR,&temp,2);
	set_lldp_port_state(9,temp);
	Read_Eeprom(LLDP_PORT11_STATE_ADDR,&temp,2);
	set_lldp_port_state(10,temp);
	Read_Eeprom(LLDP_PORT12_STATE_ADDR,&temp,2);
	set_lldp_port_state(11,temp);
	Read_Eeprom(LLDP_PORT13_STATE_ADDR,&temp,2);
	set_lldp_port_state(12,temp);
	Read_Eeprom(LLDP_PORT14_STATE_ADDR,&temp,2);
	set_lldp_port_state(13,temp);
	Read_Eeprom(LLDP_PORT15_STATE_ADDR,&temp,2);
	set_lldp_port_state(14,temp);
	Read_Eeprom(LLDP_PORT16_STATE_ADDR,&temp,2);
	set_lldp_port_state(15,temp);



	//mark settings loaded flag
	settings_loaded(1);
	return 0;
}


/*****************************************************/
/*     сохраниение настроек    */
/*****************************************************/

//сохранеиние настроек в eeprom
int settings_save_(void){
uip_ipaddr_t ip;
char tmp[128];
u16 temp;
u32 temp32;
stp_port_sett_t stp_temp;
vlan_t temp_vlan;
engine_id_t eid;
//snmp12_community_t community_temp;

//FLASH_Unlock();

IWDG_ReloadCounter();


	uip_ipaddr_copy(ip,settings.net_sett.ip);
	Write_Eeprom(IPADDRESS_ADDR,ip,4);

	uip_ipaddr_copy(ip,settings.net_sett.mask);
	Write_Eeprom(NETMASK_ADDR,ip,4);

	uip_ipaddr_copy(ip,settings.net_sett.gate);
	Write_Eeprom(GATEWAY_ADDR,ip,4);

	uip_ipaddr_copy(ip,settings.net_sett.dns);
	Write_Eeprom(DNS_ADDR,ip,4);

	uip_macaddr_copy(tmp,settings.net_sett.mac);
	Write_Eeprom(USER_MAC_ADDR,tmp,6);

	uip_macaddr_copy(tmp,settings.net_sett.default_mac);
	Write_Eeprom(DEFAULT_MAC_ADDR,tmp,6);

	temp = get_dhcp_mode();
	Write_Eeprom(DHCPMODE_ADDR,&temp,2);

	temp = get_gratuitous_arp_state();
	Write_Eeprom(NET_GRAT_ARP_STATE_ADDR,&temp,2);


	//set_dhcp_server_addr
	//set_dhcp_hops
	//set_dhcp_opt82

	//port state
	temp = get_port_sett_state(0);
	Write_Eeprom(PORT1_STATE_ADDR,&temp,2);

	temp = get_port_sett_state(1);
	Write_Eeprom(PORT2_STATE_ADDR,&temp,2);

	temp = get_port_sett_state(2);
	Write_Eeprom(PORT3_STATE_ADDR,&temp,2);

	temp = get_port_sett_state(3);
	Write_Eeprom(PORT4_STATE_ADDR,&temp,2);

	temp = get_port_sett_state(4);
	Write_Eeprom(PORT5_STATE_ADDR,&temp,2);

	temp = get_port_sett_state(5);
	Write_Eeprom(PORT6_STATE_ADDR,&temp,2);

	temp = get_port_sett_state(6);
	Write_Eeprom(PORT7_STATE_ADDR,&temp,2);

	temp = get_port_sett_state(7);
	Write_Eeprom(PORT8_STATE_ADDR,&temp,2);

	temp = get_port_sett_state(8);
	Write_Eeprom(PORT9_STATE_ADDR,&temp,2);

	temp = get_port_sett_state(9);
	Write_Eeprom(PORT10_STATE_ADDR,&temp,2);

	temp = get_port_sett_state(10);
	Write_Eeprom(PORT11_STATE_ADDR,&temp,2);

	temp = get_port_sett_state(11);
	Write_Eeprom(PORT12_STATE_ADDR,&temp,2);

	temp = get_port_sett_state(12);
	Write_Eeprom(PORT13_STATE_ADDR,&temp,2);

	temp = get_port_sett_state(13);
	Write_Eeprom(PORT14_STATE_ADDR,&temp,2);

	temp = get_port_sett_state(14);
	Write_Eeprom(PORT15_STATE_ADDR,&temp,2);

	temp = get_port_sett_state(15);
	Write_Eeprom(PORT16_STATE_ADDR,&temp,2);

	//speed / duplex
	temp = get_port_sett_speed_dplx(0);
	Write_Eeprom(PORT1_SPEEDDPLX_ADDR,&temp,2);

	temp = get_port_sett_speed_dplx(1);
	Write_Eeprom(PORT2_SPEEDDPLX_ADDR,&temp,2);

	temp = get_port_sett_speed_dplx(2);
	Write_Eeprom(PORT3_SPEEDDPLX_ADDR,&temp,2);

	temp = get_port_sett_speed_dplx(3);
	Write_Eeprom(PORT4_SPEEDDPLX_ADDR,&temp,2);

	temp = get_port_sett_speed_dplx(4);
	Write_Eeprom(PORT5_SPEEDDPLX_ADDR,&temp,2);

	temp = get_port_sett_speed_dplx(5);
	Write_Eeprom(PORT6_SPEEDDPLX_ADDR,&temp,2);

	temp = get_port_sett_speed_dplx(6);
	Write_Eeprom(PORT7_SPEEDDPLX_ADDR,&temp,2);

	temp = get_port_sett_speed_dplx(7);
	Write_Eeprom(PORT8_SPEEDDPLX_ADDR,&temp,2);

	temp = get_port_sett_speed_dplx(8);
	Write_Eeprom(PORT9_SPEEDDPLX_ADDR,&temp,2);

	temp = get_port_sett_speed_dplx(9);
	Write_Eeprom(PORT10_SPEEDDPLX_ADDR,&temp,2);

	temp = get_port_sett_speed_dplx(10);
	Write_Eeprom(PORT11_SPEEDDPLX_ADDR,&temp,2);

	temp = get_port_sett_speed_dplx(11);
	Write_Eeprom(PORT12_SPEEDDPLX_ADDR,&temp,2);

	temp = get_port_sett_speed_dplx(12);
	Write_Eeprom(PORT13_SPEEDDPLX_ADDR,&temp,2);

	temp = get_port_sett_speed_dplx(13);
	Write_Eeprom(PORT14_SPEEDDPLX_ADDR,&temp,2);

	temp = get_port_sett_speed_dplx(14);
	Write_Eeprom(PORT15_SPEEDDPLX_ADDR,&temp,2);

	temp = get_port_sett_speed_dplx(15);
	Write_Eeprom(PORT16_SPEEDDPLX_ADDR,&temp,2);

	//flow control
	temp  = get_port_sett_flow(0);
	Write_Eeprom(PORT1_FLOWCTRL_ADDR,&temp,2);

	temp  = get_port_sett_flow(1);
	Write_Eeprom(PORT2_FLOWCTRL_ADDR,&temp,2);

	temp  = get_port_sett_flow(2);
	Write_Eeprom(PORT3_FLOWCTRL_ADDR,&temp,2);

	temp  = get_port_sett_flow(3);
	Write_Eeprom(PORT4_FLOWCTRL_ADDR,&temp,2);

	temp  = get_port_sett_flow(4);
	Write_Eeprom(PORT5_FLOWCTRL_ADDR,&temp,2);

	temp  = get_port_sett_flow(5);
	Write_Eeprom(PORT6_FLOWCTRL_ADDR,&temp,2);

	temp  = get_port_sett_flow(6);
	Write_Eeprom(PORT7_FLOWCTRL_ADDR,&temp,2);

	temp  = get_port_sett_flow(7);
	Write_Eeprom(PORT8_FLOWCTRL_ADDR,&temp,2);

	temp  = get_port_sett_flow(8);
	Write_Eeprom(PORT9_FLOWCTRL_ADDR,&temp,2);

	temp  = get_port_sett_flow(9);
	Write_Eeprom(PORT10_FLOWCTRL_ADDR,&temp,2);

	temp  = get_port_sett_flow(10);
	Write_Eeprom(PORT11_FLOWCTRL_ADDR,&temp,2);

	temp  = get_port_sett_flow(11);
	Write_Eeprom(PORT12_FLOWCTRL_ADDR,&temp,2);

	temp  = get_port_sett_flow(12);
	Write_Eeprom(PORT13_FLOWCTRL_ADDR,&temp,2);

	temp  = get_port_sett_flow(13);
	Write_Eeprom(PORT14_FLOWCTRL_ADDR,&temp,2);

	temp  = get_port_sett_flow(14);
	Write_Eeprom(PORT15_FLOWCTRL_ADDR,&temp,2);

	temp  = get_port_sett_flow(15);
	Write_Eeprom(PORT16_FLOWCTRL_ADDR,&temp,2);




	temp = get_port_sett_wdt(0);
	Write_Eeprom(PORT1_WDT_ADDR,&temp,2);

	temp = get_port_sett_wdt(1);
	Write_Eeprom(PORT2_WDT_ADDR,&temp,2);

	temp = get_port_sett_wdt(2);
	Write_Eeprom(PORT3_WDT_ADDR,&temp,2);

	temp = get_port_sett_wdt(3);
	Write_Eeprom(PORT4_WDT_ADDR,&temp,2);

	temp = get_port_sett_wdt(4);
	Write_Eeprom(PORT5_WDT_ADDR,&temp,2);

	temp = get_port_sett_wdt(5);
	Write_Eeprom(PORT6_WDT_ADDR,&temp,2);

	temp = get_port_sett_wdt(6);
	Write_Eeprom(PORT7_WDT_ADDR,&temp,2);

	temp = get_port_sett_wdt(7);
	Write_Eeprom(PORT8_WDT_ADDR,&temp,2);

	//ar - speed
	temp = get_port_sett_wdt_speed_down(0);
	Write_Eeprom(PORT1_WDT_SPEED_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_down(1);
	Write_Eeprom(PORT2_WDT_SPEED_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_down(2);
	Write_Eeprom(PORT3_WDT_SPEED_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_down(3);
	Write_Eeprom(PORT4_WDT_SPEED_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_down(4);
	Write_Eeprom(PORT5_WDT_SPEED_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_down(5);
	Write_Eeprom(PORT6_WDT_SPEED_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_down(6);
	Write_Eeprom(PORT7_WDT_SPEED_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_down(7);
	Write_Eeprom(PORT8_WDT_SPEED_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_down(8);
	Write_Eeprom(PORT9_WDT_SPEED_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_down(9);
	Write_Eeprom(PORT10_WDT_SPEED_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_down(10);
	Write_Eeprom(PORT11_WDT_SPEED_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_down(11);
	Write_Eeprom(PORT12_WDT_SPEED_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_down(12);
	Write_Eeprom(PORT13_WDT_SPEED_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_down(13);
	Write_Eeprom(PORT14_WDT_SPEED_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_down(14);
	Write_Eeprom(PORT15_WDT_SPEED_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_down(15);
	Write_Eeprom(PORT16_WDT_SPEED_ADDR,&temp,2);

	//ar - speed
	temp = get_port_sett_wdt_speed_up(0);
	Write_Eeprom(PORT1_WDT_SPEED_UP_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_up(1);
	Write_Eeprom(PORT2_WDT_SPEED_UP_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_up(2);
	Write_Eeprom(PORT3_WDT_SPEED_UP_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_up(3);
	Write_Eeprom(PORT4_WDT_SPEED_UP_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_up(4);
	Write_Eeprom(PORT5_WDT_SPEED_UP_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_up(5);
	Write_Eeprom(PORT6_WDT_SPEED_UP_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_up(6);
	Write_Eeprom(PORT7_WDT_SPEED_UP_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_up(7);
	Write_Eeprom(PORT8_WDT_SPEED_UP_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_up(8);
	Write_Eeprom(PORT9_WDT_SPEED_UP_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_up(9);
	Write_Eeprom(PORT10_WDT_SPEED_UP_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_up(10);
	Write_Eeprom(PORT11_WDT_SPEED_UP_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_up(11);
	Write_Eeprom(PORT12_WDT_SPEED_UP_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_up(12);
	Write_Eeprom(PORT13_WDT_SPEED_UP_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_up(13);
	Write_Eeprom(PORT14_WDT_SPEED_UP_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_up(14);
	Write_Eeprom(PORT15_WDT_SPEED_UP_ADDR,&temp,2);

	temp = get_port_sett_wdt_speed_up(15);
	Write_Eeprom(PORT16_WDT_SPEED_UP_ADDR,&temp,2);


#if 1

	get_port_sett_wdt_ip(0,&ip);
	Write_Eeprom(PORT1_IPADDR_ADDR,ip,4);

	get_port_sett_wdt_ip(1,&ip);
	Write_Eeprom(PORT2_IPADDR_ADDR,ip,4);

	get_port_sett_wdt_ip(2,&ip);
	Write_Eeprom(PORT3_IPADDR_ADDR,ip,4);

	get_port_sett_wdt_ip(3,&ip);
	Write_Eeprom(PORT4_IPADDR_ADDR,ip,4);

	get_port_sett_wdt_ip(4,&ip);
	Write_Eeprom(PORT5_IPADDR_ADDR,ip,4);

	get_port_sett_wdt_ip(5,&ip);
	Write_Eeprom(PORT6_IPADDR_ADDR,ip,4);

	get_port_sett_wdt_ip(6,&ip);
	Write_Eeprom(PORT7_IPADDR_ADDR,ip,4);

	get_port_sett_wdt_ip(7,&ip);
	Write_Eeprom(PORT8_IPADDR_ADDR,ip,4);

	//get_port_sett_wdt_ip(8,&ip);
	//Write_Eeprom(PORT9_IPADDR_ADDR,ip,4);

	//get_port_sett_wdt_ip(9,&ip);
	//Write_Eeprom(PORT10_IPADDR_ADDR,ip,4);


	temp = get_port_sett_soft_start(0);
	Write_Eeprom(PORT1_SOFTSTART_ADDR,&temp,2);

	temp = get_port_sett_soft_start(1);
	Write_Eeprom(PORT2_SOFTSTART_ADDR,&temp,2);

	temp = get_port_sett_soft_start(2);
	Write_Eeprom(PORT3_SOFTSTART_ADDR,&temp,2);

	temp = get_port_sett_soft_start(3);
	Write_Eeprom(PORT4_SOFTSTART_ADDR,&temp,2);

	temp = get_port_sett_soft_start(4);
	Write_Eeprom(PORT5_SOFTSTART_ADDR,&temp,2);

	temp = get_port_sett_soft_start(5);
	Write_Eeprom(PORT6_SOFTSTART_ADDR,&temp,2);

	temp = get_port_sett_soft_start(6);
	Write_Eeprom(PORT7_SOFTSTART_ADDR,&temp,2);

	temp = get_port_sett_soft_start(7);
	Write_Eeprom(PORT8_SOFTSTART_ADDR,&temp,2);

	//temp = get_port_sett_soft_start(8);
	//Write_Eeprom(PORT9_SOFTSTART_ADDR,&temp,2);

	//temp = get_port_sett_soft_start(9);
	//Write_Eeprom(PORT10_SOFTSTART_ADDR,&temp,2);

	temp = (u16)(get_port_sett_poe_b(0)<<8 | get_port_sett_poe(0));
	Write_Eeprom(PORT1_POE_ADDR,&temp,2);

	temp = (u16)(get_port_sett_poe_b(1)<<8 | get_port_sett_poe(1));
	Write_Eeprom(PORT2_POE_ADDR,&temp,2);

	temp = (u16)(get_port_sett_poe_b(2)<<8 | get_port_sett_poe(2));
	Write_Eeprom(PORT3_POE_ADDR,&temp,2);

	temp = (u16)(get_port_sett_poe_b(3)<<8 | get_port_sett_poe(3));
	Write_Eeprom(PORT4_POE_ADDR,&temp,2);

	temp = (u16)(get_port_sett_poe_b(4)<<8 | get_port_sett_poe(4));
	Write_Eeprom(PORT5_POE_ADDR,&temp,2);

	temp = (u16)(get_port_sett_poe_b(5)<<8 | get_port_sett_poe(5));
	Write_Eeprom(PORT6_POE_ADDR,&temp,2);

	temp = (u16)(get_port_sett_poe_b(6)<<8 | get_port_sett_poe(6));
	Write_Eeprom(PORT7_POE_ADDR,&temp,2);

	temp = (u16)(get_port_sett_poe_b(7)<<8 | get_port_sett_poe(7));
	Write_Eeprom(PORT8_POE_ADDR,&temp,2);

	temp = get_port_sett_pwr_lim_a(0);
	Write_Eeprom(PORT1_POE_A_LIM,&temp,2);

	temp = get_port_sett_pwr_lim_a(1);
	Write_Eeprom(PORT2_POE_A_LIM,&temp,2);

	temp = get_port_sett_pwr_lim_a(2);
	Write_Eeprom(PORT3_POE_A_LIM,&temp,2);

	temp = get_port_sett_pwr_lim_a(3);
	Write_Eeprom(PORT4_POE_A_LIM,&temp,2);

	temp = get_port_sett_pwr_lim_a(4);
	Write_Eeprom(PORT5_POE_A_LIM,&temp,2);

	temp = get_port_sett_pwr_lim_a(5);
	Write_Eeprom(PORT6_POE_A_LIM,&temp,2);

	temp = get_port_sett_pwr_lim_a(6);
	Write_Eeprom(PORT7_POE_A_LIM,&temp,2);

	temp = get_port_sett_pwr_lim_a(7);
	Write_Eeprom(PORT8_POE_A_LIM,&temp,2);


	//temp = get_port_sett_pwr_lim_a(8);
	//Write_Eeprom(PORT9_POE_A_LIM,&temp,2);

	//temp = get_port_sett_pwr_lim_a(9);
	//Write_Eeprom(PORT10_POE_A_LIM,&temp,2);


	temp = get_port_sett_pwr_lim_b(0);
	Write_Eeprom(PORT1_POE_B_LIM,&temp,2);

	temp = get_port_sett_pwr_lim_b(1);
	Write_Eeprom(PORT2_POE_B_LIM,&temp,2);

	temp = get_port_sett_pwr_lim_b(2);
	Write_Eeprom(PORT3_POE_B_LIM,&temp,2);

	temp = get_port_sett_pwr_lim_b(3);
	Write_Eeprom(PORT4_POE_B_LIM,&temp,2);

	temp = get_port_sett_pwr_lim_b(4);
	Write_Eeprom(PORT5_POE_B_LIM,&temp,2);

	temp = get_port_sett_pwr_lim_b(5);
	Write_Eeprom(PORT6_POE_B_LIM,&temp,2);


	temp = get_port_sett_sfp_mode(GE1);
	Write_Eeprom(PORT_SFP1_MODE,&temp,2);

	temp = get_port_sett_sfp_mode(GE2);
	Write_Eeprom(PORT_SFP2_MODE,&temp,2);


	temp = get_interface_lang();
	Write_Eeprom(LANG_ADDR,&temp,2);


	strcpy(tmp,settings.interface_sett.http_login64);
	Write_Eeprom(HTTP_USERNAME_ADDR,tmp,64);

	strcpy(tmp,settings.interface_sett.http_passwd64);
	Write_Eeprom(HTTP_PASSWD_ADDR,tmp,64);


	get_interface_users_username(1,tmp);
	Write_Eeprom(USER1_USERNAME_ADDR,tmp,64);
	get_interface_users_password(1,tmp);
	Write_Eeprom(USER1_PASSWORD_ADDR,tmp,64);
	temp = get_interface_users_rule(1);
	Write_Eeprom(USER1_RULE_ADDR,&temp,2);

	get_interface_users_username(2,tmp);
	Write_Eeprom(USER2_USERNAME_ADDR,tmp,64);
	get_interface_users_password(2,tmp);
	Write_Eeprom(USER2_PASSWORD_ADDR,tmp,64);
	temp = get_interface_users_rule(2);
	Write_Eeprom(USER2_RULE_ADDR,&temp,2);

	get_interface_users_username(3,tmp);
	Write_Eeprom(USER3_USERNAME_ADDR,tmp,64);
	get_interface_users_password(3,tmp);
	Write_Eeprom(USER3_PASSWORD_ADDR,tmp,64);
	temp = get_interface_users_rule(3);
	Write_Eeprom(USER3_RULE_ADDR,&temp,2);

	//description
	strcpy(tmp,settings.interface_sett.system_name);
	Write_Eeprom(SYSTEM_NAME_ADDR,tmp,128);

	strcpy(tmp,settings.interface_sett.system_location);
	Write_Eeprom(SYSTEM_LOCATION_ADDR,tmp,128);

	strcpy(tmp,settings.interface_sett.system_contact);
	Write_Eeprom(SYSTEM_CONTACT_ADDR,tmp,128);

	//port description
	get_port_descr(0,tmp);
	Write_Eeprom(PORT1_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	get_port_descr(1,tmp);
	Write_Eeprom(PORT2_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	get_port_descr(2,tmp);
	Write_Eeprom(PORT3_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	get_port_descr(3,tmp);
	Write_Eeprom(PORT4_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	get_port_descr(4,tmp);
	Write_Eeprom(PORT5_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	get_port_descr(5,tmp);
	Write_Eeprom(PORT6_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	get_port_descr(6,tmp);
	Write_Eeprom(PORT7_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	get_port_descr(7,tmp);
	Write_Eeprom(PORT8_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	get_port_descr(8,tmp);
	Write_Eeprom(PORT9_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	get_port_descr(9,tmp);
	Write_Eeprom(PORT10_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	get_port_descr(10,tmp);
	Write_Eeprom(PORT11_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	get_port_descr(11,tmp);
	Write_Eeprom(PORT12_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	get_port_descr(12,tmp);
	Write_Eeprom(PORT13_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	get_port_descr(13,tmp);
	Write_Eeprom(PORT14_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	get_port_descr(14,tmp);
	Write_Eeprom(PORT15_DESCR_ADDR,tmp,PORT_DESCR_LEN);
	get_port_descr(15,tmp);
	Write_Eeprom(PORT16_DESCR_ADDR,tmp,PORT_DESCR_LEN);


	temp = get_smtp_state();
	Write_Eeprom(SMTP_STATE_ADDR,&temp,2);

	get_smtp_server(&ip);
	Write_Eeprom(SMTP_SERV_IP_ADDR,ip,4);

	strcpy(tmp,settings.smtp_sett.to);
	Write_Eeprom(SMTP_TO1_ADDR,tmp,64);

	strcpy(tmp,settings.smtp_sett.to2);
	Write_Eeprom(SMTP_TO2_ADDR,tmp,64);

	strcpy(tmp,settings.smtp_sett.to3);
	Write_Eeprom(SMTP_TO3_ADDR,tmp,64);

	strcpy(tmp,settings.smtp_sett.from);
	Write_Eeprom(SMTP_FROM_ADDR,tmp,64);

	strcpy(tmp,settings.smtp_sett.subj);
	Write_Eeprom(SMTP_SUBJ_ADDR,tmp,64);

	strcpy(tmp,settings.smtp_sett.login);
	Write_Eeprom(SMTP_LOGIN_ADDR,tmp,32);

	strcpy(tmp,settings.smtp_sett.pass);
	Write_Eeprom(SMTP_PASS_ADDR,tmp,32);

	temp32 = get_smtp_port();
	Write_Eeprom(SMTP_PORT_ADDR,&temp32,4);

	strcpy(tmp,settings.smtp_sett.domain_name);
	Write_Eeprom(SMTP_DOMAIN_NAME2_ADDR,tmp,32);
#endif


	temp = get_sntp_state();
	Write_Eeprom(SNTP_STATE_ADDR,&temp,2);

	uip_ipaddr_copy(ip,settings.sntp_sett.addr);
	Write_Eeprom(SNTP_SETT_SERV_ADDR,ip,4);

	get_sntp_serv_name(tmp);
	Write_Eeprom(SNTP_SETT_SERV_NAME_ADDR,tmp,64);

	temp = (u16)get_sntp_timezone();
	Write_Eeprom(SNTP_TIMEZONE_ADDR,&temp,2);

	temp = get_sntp_period();
	Write_Eeprom(SNTP_PERIOD_ADDR,&temp,2);


	temp = get_syslog_state();
	Write_Eeprom(SYSLOG_STATE_ADDR,&temp,2);

	get_syslog_serv(&ip);
	Write_Eeprom(SYSLOG_SERV_IP_ADDR,ip,4);

	temp = get_event_base_s_st()<<3 | get_event_base_s_level();
	Write_Eeprom(EVENT_LIST_BASE_S_ADDR,&temp,2);

	temp = get_event_port_s_st()<<3 | get_event_port_s_level();
	Write_Eeprom(EVENT_LIST_PORT_S_ADDR,&temp,2);

	temp = get_event_vlan_s_st()<<3 | get_event_vlan_s_level();
	Write_Eeprom(EVENT_LIST_VLAN_S_ADDR,&temp,2);

	temp = get_event_stp_s_st()<<3 | get_event_stp_s_level();
	Write_Eeprom(EVENT_LIST_STP_S_ADDR,&temp,2);

	temp = get_event_qos_s_st()<<3 | get_event_qos_s_level();
	Write_Eeprom(EVENT_LIST_QOS_S_ADDR,&temp,2);

	temp = get_event_other_s_st()<<3 | get_event_other_s_level();
	Write_Eeprom(EVENT_LIST_OTHER_S_ADDR,&temp,2);

	temp = get_event_port_link_t_st()<<3 | get_event_port_link_t_level();
	Write_Eeprom(EVENT_LIST_LINK_T_ADDR,&temp,2);

	temp = get_event_port_poe_t_st()<<3 | get_event_port_poe_t_level();
	Write_Eeprom(EVENT_LIST_POE_T_ADDR,&temp,2);

	temp = get_event_port_stp_t_st()<<3 | get_event_port_stp_t_level();
	Write_Eeprom(EVENT_LIST_STP_T_ADDR,&temp,2);

	temp = get_event_port_spec_link_t_st()<<3 | get_event_port_spec_link_t_level();
	Write_Eeprom(EVENT_LIST_ARLINK_T_ADDR,&temp,2);

	temp = get_event_port_spec_ping_t_st()<<3 | get_event_port_spec_ping_t_level();
	Write_Eeprom(EVENT_LIST_ARPING_T_ADDR,&temp,2);

	temp = get_event_port_spec_speed_t_st()<<3 | get_event_port_spec_speed_t_level();
	Write_Eeprom(EVENT_LIST_ARSPEED_T_ADDR,&temp,2);

	temp = get_event_port_system_t_st()<<3 | get_event_port_system_t_level();
	Write_Eeprom(EVENT_LIST_SYSTEM_T_ADDR,&temp,2);

	temp = get_event_port_ups_t_st()<<3 | get_event_port_ups_t_level();
	Write_Eeprom(EVENT_LIST_UPS_T_ADDR,&temp,2);

	temp = get_event_port_alarm_t_st()<<3 | get_event_port_alarm_t_level();
	Write_Eeprom(EVENT_LIST_ACCESS_T_ADDR,&temp,2);

	temp = get_event_port_mac_t_st()<<3 | get_event_port_mac_t_level();
	Write_Eeprom(EVENT_LIST_MAC_T_ADDR,&temp,2);

	temp = get_alarm_state(0);
	Write_Eeprom(DRY_CONT0_STATE_ADDR,&temp,2);

	temp = get_alarm_state(1);
	Write_Eeprom(DRY_CONT1_STATE_ADDR,&temp,2);

	temp = get_alarm_front(1);
	Write_Eeprom(DRY_CONT1_LEVEL_ADDR,&temp,2);

	temp = get_alarm_state(2);
	Write_Eeprom(DRY_CONT2_STATE_ADDR,&temp,2);

	temp = get_alarm_front(2);
	Write_Eeprom(DRY_CONT2_LEVEL_ADDR,&temp,2);

	temp32 = get_rate_limit_rx(0);
	Write_Eeprom(PORT1_RATE_LIMIT_RX_ADDR,&temp32,4);

	temp32 = get_rate_limit_rx(1);
	Write_Eeprom(PORT2_RATE_LIMIT_RX_ADDR,&temp32,4);

	temp32 = get_rate_limit_rx(2);
	Write_Eeprom(PORT3_RATE_LIMIT_RX_ADDR,&temp32,4);

	temp32 = get_rate_limit_rx(3);
	Write_Eeprom(PORT4_RATE_LIMIT_RX_ADDR,&temp32,4);

	temp32 = get_rate_limit_rx(4);
	Write_Eeprom(PORT5_RATE_LIMIT_RX_ADDR,&temp32,4);

	temp32 = get_rate_limit_rx(5);
	Write_Eeprom(PORT6_RATE_LIMIT_RX_ADDR,&temp32,4);

	temp32 = get_rate_limit_rx(6);
	Write_Eeprom(PORT7_RATE_LIMIT_RX_ADDR,&temp32,4);

	temp32 = get_rate_limit_rx(7);
	Write_Eeprom(PORT8_RATE_LIMIT_RX_ADDR,&temp32,4);

	temp32 = get_rate_limit_rx(8);
	Write_Eeprom(PORT9_RATE_LIMIT_RX_ADDR,&temp32,4);

	temp32 = get_rate_limit_rx(9);
	Write_Eeprom(PORT10_RATE_LIMIT_RX_ADDR,&temp32,4);

	temp32 = get_rate_limit_rx(10);
	Write_Eeprom(PORT11_RATE_LIMIT_RX_ADDR,&temp32,4);

	temp32 = get_rate_limit_rx(11);
	Write_Eeprom(PORT12_RATE_LIMIT_RX_ADDR,&temp32,4);

	temp32 = get_rate_limit_rx(12);
	Write_Eeprom(PORT13_RATE_LIMIT_RX_ADDR,&temp32,4);

	temp32 = get_rate_limit_rx(13);
	Write_Eeprom(PORT14_RATE_LIMIT_RX_ADDR,&temp32,4);

	temp32 = get_rate_limit_rx(14);
	Write_Eeprom(PORT15_RATE_LIMIT_RX_ADDR,&temp32,4);

	temp32 = get_rate_limit_rx(15);
	Write_Eeprom(PORT16_RATE_LIMIT_RX_ADDR,&temp32,4);


	temp32 = get_rate_limit_tx(0);
	Write_Eeprom(PORT1_RATE_LIMIT_TX_ADDR,&temp32,4);

	temp32 = get_rate_limit_tx(1);
	Write_Eeprom(PORT2_RATE_LIMIT_TX_ADDR,&temp32,4);

	temp32 = get_rate_limit_tx(2);
	Write_Eeprom(PORT3_RATE_LIMIT_TX_ADDR,&temp32,4);

	temp32 = get_rate_limit_tx(3);
	Write_Eeprom(PORT4_RATE_LIMIT_TX_ADDR,&temp32,4);

	temp32 = get_rate_limit_tx(4);
	Write_Eeprom(PORT5_RATE_LIMIT_TX_ADDR,&temp32,4);

	temp32 = get_rate_limit_tx(5);
	Write_Eeprom(PORT6_RATE_LIMIT_TX_ADDR,&temp32,4);

	temp32 = get_rate_limit_tx(6);
	Write_Eeprom(PORT7_RATE_LIMIT_TX_ADDR,&temp32,4);

	temp32 = get_rate_limit_tx(7);
	Write_Eeprom(PORT8_RATE_LIMIT_TX_ADDR,&temp32,4);

	temp32 = get_rate_limit_tx(7);
	Write_Eeprom(PORT8_RATE_LIMIT_TX_ADDR,&temp32,4);

	temp32 = get_rate_limit_tx(8);
	Write_Eeprom(PORT9_RATE_LIMIT_TX_ADDR,&temp32,4);

	temp32 = get_rate_limit_tx(9);
	Write_Eeprom(PORT10_RATE_LIMIT_TX_ADDR,&temp32,4);

	temp32 = get_rate_limit_tx(10);
	Write_Eeprom(PORT11_RATE_LIMIT_TX_ADDR,&temp32,4);

	temp32 = get_rate_limit_tx(11);
	Write_Eeprom(PORT12_RATE_LIMIT_TX_ADDR,&temp32,4);

	temp32 = get_rate_limit_tx(12);
	Write_Eeprom(PORT13_RATE_LIMIT_TX_ADDR,&temp32,4);

	temp32 = get_rate_limit_tx(13);
	Write_Eeprom(PORT14_RATE_LIMIT_TX_ADDR,&temp32,4);

	temp32 = get_rate_limit_tx(14);
	Write_Eeprom(PORT15_RATE_LIMIT_TX_ADDR,&temp32,4);

	temp32 = get_rate_limit_tx(15);
	Write_Eeprom(PORT16_RATE_LIMIT_TX_ADDR,&temp32,4);


	IWDG_ReloadCounter();

	temp = get_qos_port_cos_state(0);
	Write_Eeprom(PORT1_COS_STATE_ADDR,&temp,2);

	temp = get_qos_port_cos_state(1);
	Write_Eeprom(PORT2_COS_STATE_ADDR,&temp,2);

	temp = get_qos_port_cos_state(2);
	Write_Eeprom(PORT3_COS_STATE_ADDR,&temp,2);

	temp = get_qos_port_cos_state(3);
	Write_Eeprom(PORT4_COS_STATE_ADDR,&temp,2);

	temp = get_qos_port_cos_state(4);
	Write_Eeprom(PORT5_COS_STATE_ADDR,&temp,2);

	temp = get_qos_port_cos_state(5);
	Write_Eeprom(PORT6_COS_STATE_ADDR,&temp,2);

	temp = get_qos_port_cos_state(6);
	Write_Eeprom(PORT7_COS_STATE_ADDR,&temp,2);

	temp = get_qos_port_cos_state(7);
	Write_Eeprom(PORT8_COS_STATE_ADDR,&temp,2);

	temp = get_qos_port_cos_state(8);
	Write_Eeprom(PORT9_COS_STATE_ADDR,&temp,2);

	temp = get_qos_port_cos_state(9);
	Write_Eeprom(PORT10_COS_STATE_ADDR,&temp,2);

	temp = get_qos_port_cos_state(10);
	Write_Eeprom(PORT11_COS_STATE_ADDR,&temp,2);

	temp = get_qos_port_cos_state(11);
	Write_Eeprom(PORT12_COS_STATE_ADDR,&temp,2);

	temp = get_qos_port_cos_state(12);
	Write_Eeprom(PORT13_COS_STATE_ADDR,&temp,2);

	temp = get_qos_port_cos_state(13);
	Write_Eeprom(PORT14_COS_STATE_ADDR,&temp,2);

	temp = get_qos_port_cos_state(14);
	Write_Eeprom(PORT15_COS_STATE_ADDR,&temp,2);

	temp = get_qos_port_cos_state(15);
	Write_Eeprom(PORT16_COS_STATE_ADDR,&temp,2);




	temp = get_qos_port_tos_state(0);
	Write_Eeprom(PORT1_TOS_STATE_ADDR,&temp,2);

	temp = get_qos_port_tos_state(1);
	Write_Eeprom(PORT2_TOS_STATE_ADDR,&temp,2);

	temp = get_qos_port_tos_state(2);
	Write_Eeprom(PORT3_TOS_STATE_ADDR,&temp,2);

	temp = get_qos_port_tos_state(3);
	Write_Eeprom(PORT4_TOS_STATE_ADDR,&temp,2);

	temp = get_qos_port_tos_state(4);
	Write_Eeprom(PORT5_TOS_STATE_ADDR,&temp,2);

	temp = get_qos_port_tos_state(5);
	Write_Eeprom(PORT6_TOS_STATE_ADDR,&temp,2);

	temp = get_qos_port_tos_state(6);
	Write_Eeprom(PORT7_TOS_STATE_ADDR,&temp,2);

	temp = get_qos_port_tos_state(7);
	Write_Eeprom(PORT8_TOS_STATE_ADDR,&temp,2);

	temp = get_qos_port_tos_state(8);
	Write_Eeprom(PORT9_TOS_STATE_ADDR,&temp,2);

	temp = get_qos_port_tos_state(9);
	Write_Eeprom(PORT10_TOS_STATE_ADDR,&temp,2);

	temp = get_qos_port_tos_state(10);
	Write_Eeprom(PORT11_TOS_STATE_ADDR,&temp,2);

	temp = get_qos_port_tos_state(11);
	Write_Eeprom(PORT12_TOS_STATE_ADDR,&temp,2);

	temp = get_qos_port_tos_state(12);
	Write_Eeprom(PORT13_TOS_STATE_ADDR,&temp,2);

	temp = get_qos_port_tos_state(13);
	Write_Eeprom(PORT14_TOS_STATE_ADDR,&temp,2);

	temp = get_qos_port_tos_state(14);
	Write_Eeprom(PORT15_TOS_STATE_ADDR,&temp,2);

	temp = get_qos_port_tos_state(15);
	Write_Eeprom(PORT16_TOS_STATE_ADDR,&temp,2);


	temp = get_qos_port_rule(0);
	Write_Eeprom(PORT1_QOS_RULE_ADDR,&temp,2);

	temp = get_qos_port_rule(1);
	Write_Eeprom(PORT2_QOS_RULE_ADDR,&temp,2);

	temp = get_qos_port_rule(2);
	Write_Eeprom(PORT3_QOS_RULE_ADDR,&temp,2);

	temp = get_qos_port_rule(3);
	Write_Eeprom(PORT4_QOS_RULE_ADDR,&temp,2);

	temp = get_qos_port_rule(4);
	Write_Eeprom(PORT5_QOS_RULE_ADDR,&temp,2);

	temp = get_qos_port_rule(5);
	Write_Eeprom(PORT6_QOS_RULE_ADDR,&temp,2);

	temp = get_qos_port_rule(6);
	Write_Eeprom(PORT7_QOS_RULE_ADDR,&temp,2);

	temp = get_qos_port_rule(7);
	Write_Eeprom(PORT8_QOS_RULE_ADDR,&temp,2);

	temp = get_qos_port_rule(8);
	Write_Eeprom(PORT9_QOS_RULE_ADDR,&temp,2);

	temp = get_qos_port_rule(9);
	Write_Eeprom(PORT10_QOS_RULE_ADDR,&temp,2);

	temp = get_qos_port_rule(10);
	Write_Eeprom(PORT11_QOS_RULE_ADDR,&temp,2);

	temp = get_qos_port_rule(11);
	Write_Eeprom(PORT12_QOS_RULE_ADDR,&temp,2);

	temp = get_qos_port_rule(12);
	Write_Eeprom(PORT13_QOS_RULE_ADDR,&temp,2);

	temp = get_qos_port_rule(13);
	Write_Eeprom(PORT14_QOS_RULE_ADDR,&temp,2);

	temp = get_qos_port_rule(14);
	Write_Eeprom(PORT15_QOS_RULE_ADDR,&temp,2);

	temp = get_qos_port_rule(15);
	Write_Eeprom(PORT16_QOS_RULE_ADDR,&temp,2);




	temp = get_qos_port_def_pri(0);
	Write_Eeprom(PORT1_COS_PRI_ADDR,&temp,2);

	temp = get_qos_port_def_pri(1);
	Write_Eeprom(PORT2_COS_PRI_ADDR,&temp,2);

	temp = get_qos_port_def_pri(2);
	Write_Eeprom(PORT3_COS_PRI_ADDR,&temp,2);

	temp = get_qos_port_def_pri(3);
	Write_Eeprom(PORT4_COS_PRI_ADDR,&temp,2);

	temp = get_qos_port_def_pri(4);
	Write_Eeprom(PORT5_COS_PRI_ADDR,&temp,2);

	temp = get_qos_port_def_pri(5);
	Write_Eeprom(PORT6_COS_PRI_ADDR,&temp,2);

	temp = get_qos_port_def_pri(6);
	Write_Eeprom(PORT7_COS_PRI_ADDR,&temp,2);

	temp = get_qos_port_def_pri(7);
	Write_Eeprom(PORT8_COS_PRI_ADDR,&temp,2);

	temp = get_qos_port_def_pri(8);
	Write_Eeprom(PORT9_COS_PRI_ADDR,&temp,2);

	temp = get_qos_port_def_pri(9);
	Write_Eeprom(PORT10_COS_PRI_ADDR,&temp,2);

	temp = get_qos_port_def_pri(10);
	Write_Eeprom(PORT11_COS_PRI_ADDR,&temp,2);

	temp = get_qos_port_def_pri(11);
	Write_Eeprom(PORT12_COS_PRI_ADDR,&temp,2);

	temp = get_qos_port_def_pri(12);
	Write_Eeprom(PORT13_COS_PRI_ADDR,&temp,2);

	temp = get_qos_port_def_pri(13);
	Write_Eeprom(PORT14_COS_PRI_ADDR,&temp,2);

	temp = get_qos_port_def_pri(14);
	Write_Eeprom(PORT15_COS_PRI_ADDR,&temp,2);

	temp = get_qos_port_def_pri(15);
	Write_Eeprom(PORT16_COS_PRI_ADDR,&temp,2);



	temp = get_qos_state();
	Write_Eeprom(QOS_STATE_ADDR,&temp,2);

	temp = get_qos_policy();
	Write_Eeprom(QOS_POLICY_ADDR,&temp,2);

	for(u8 i=0;i<8;i++){
		tmp[i] = get_qos_cos_queue(i);
	}
	Write_Eeprom(QOS_COS_ADDR,tmp,8);

	for(u8 i=0;i<64;i++){
		tmp[i] = get_qos_tos_queue(i);
	}
	Write_Eeprom(QOS_TOS_ADDR,tmp,64);

	temp = get_uc_rate_limit();
	Write_Eeprom(UC_RATE_LIM_ADDR,&temp,2);
	temp = get_mc_rate_limit();
	Write_Eeprom(MC_RATE_LIM_ADDR,&temp,2);
	temp = get_bc_rate_limit();
	Write_Eeprom(BC_RATE_LIM_ADDR,&temp,2);
	temp = get_bc_limit();
	Write_Eeprom(LIM_RATE_LIM_ADDR,&temp,2);

	temp = get_rate_limit_mode(0);
	Write_Eeprom(RATE_LIM_MODE_P1_ADDR,&temp,2);
	temp = get_rate_limit_mode(1);
	Write_Eeprom(RATE_LIM_MODE_P2_ADDR,&temp,2);
	temp = get_rate_limit_mode(2);
	Write_Eeprom(RATE_LIM_MODE_P3_ADDR,&temp,2);
	temp = get_rate_limit_mode(3);
	Write_Eeprom(RATE_LIM_MODE_P4_ADDR,&temp,2);
	temp = get_rate_limit_mode(4);
	Write_Eeprom(RATE_LIM_MODE_P5_ADDR,&temp,2);
	temp = get_rate_limit_mode(5);
	Write_Eeprom(RATE_LIM_MODE_P6_ADDR,&temp,2);
	temp = get_rate_limit_mode(6);
	Write_Eeprom(RATE_LIM_MODE_P7_ADDR,&temp,2);
	temp = get_rate_limit_mode(7);
	Write_Eeprom(RATE_LIM_MODE_P8_ADDR,&temp,2);
	temp = get_rate_limit_mode(8);
	Write_Eeprom(RATE_LIM_MODE_P9_ADDR,&temp,2);
	temp = get_rate_limit_mode(9);
	Write_Eeprom(RATE_LIM_MODE_P10_ADDR,&temp,2);
	temp = get_rate_limit_mode(10);
	Write_Eeprom(RATE_LIM_MODE_P11_ADDR,&temp,2);
	temp = get_rate_limit_mode(11);
	Write_Eeprom(RATE_LIM_MODE_P12_ADDR,&temp,2);
	temp = get_rate_limit_mode(12);
	Write_Eeprom(RATE_LIM_MODE_P13_ADDR,&temp,2);
	temp = get_rate_limit_mode(13);
	Write_Eeprom(RATE_LIM_MODE_P14_ADDR,&temp,2);
	temp = get_rate_limit_mode(14);
	Write_Eeprom(RATE_LIM_MODE_P15_ADDR,&temp,2);
	temp = get_rate_limit_mode(15);
	Write_Eeprom(RATE_LIM_MODE_P16_ADDR,&temp,2);
	temp = get_rate_limit_mode(16);
	Write_Eeprom(RATE_LIM_MODE_P17_ADDR,&temp,2);
	temp = get_rate_limit_mode(17);
	Write_Eeprom(RATE_LIM_MODE_P18_ADDR,&temp,2);
	temp = get_rate_limit_mode(18);
	Write_Eeprom(RATE_LIM_MODE_P19_ADDR,&temp,2);
	temp = get_rate_limit_mode(19);
	Write_Eeprom(RATE_LIM_MODE_P20_ADDR,&temp,2);


	temp = get_pb_vlan_state();
	Write_Eeprom(PB_VLAN_STATE_ADDR,&temp,2);

	temp = 0;
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		temp |= get_pb_vlan_port(0,i)<<i;
	}
	Write_Eeprom(PORT1_PB_VLAN_ADDR,&temp,2);

	temp = 0;
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		temp |= get_pb_vlan_port(1,i)<<i;
	}
	Write_Eeprom(PORT2_PB_VLAN_ADDR,&temp,2);

	temp = 0;
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		temp |= get_pb_vlan_port(2,i)<<i;
	}
	Write_Eeprom(PORT3_PB_VLAN_ADDR,&temp,2);

	temp = 0;
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		temp |= get_pb_vlan_port(3,i)<<i;
	}
	Write_Eeprom(PORT4_PB_VLAN_ADDR,&temp,2);

	temp = 0;
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		temp |= get_pb_vlan_port(4,i)<<i;
	}
	Write_Eeprom(PORT5_PB_VLAN_ADDR,&temp,2);

	temp = 0;
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		temp |= get_pb_vlan_port(5,i)<<i;
	}
	Write_Eeprom(PORT6_PB_VLAN_ADDR,&temp,2);

	temp = 0;
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		temp |= get_pb_vlan_port(6,i)<<i;
	}
	Write_Eeprom(PORT7_PB_VLAN_ADDR,&temp,2);

	temp = 0;
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		temp |= get_pb_vlan_port(7,i)<<i;
	}
	Write_Eeprom(PORT8_PB_VLAN_ADDR,&temp,2);

	temp = 0;
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		temp |= get_pb_vlan_port(8,i)<<i;
	}
	Write_Eeprom(PORT9_PB_VLAN_ADDR,&temp,2);

	temp = 0;
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		temp |= get_pb_vlan_port(9,i)<<i;
	}
	Write_Eeprom(PORT10_PB_VLAN_ADDR,&temp,2);

	temp = 0;
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		temp |= get_pb_vlan_port(10,i)<<i;
	}
	Write_Eeprom(PORT11_PB_VLAN_ADDR,&temp,2);

	temp = 0;
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		temp |= get_pb_vlan_port(11,i)<<i;
	}
	Write_Eeprom(PORT12_PB_VLAN_ADDR,&temp,2);

	temp = 0;
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		temp |= get_pb_vlan_port(12,i)<<i;
	}
	Write_Eeprom(PORT13_PB_VLAN_ADDR,&temp,2);

	temp = 0;
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		temp |= get_pb_vlan_port(13,i)<<i;
	}
	Write_Eeprom(PORT14_PB_VLAN_ADDR,&temp,2);

	temp = 0;
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		temp |= get_pb_vlan_port(14,i)<<i;
	}
	Write_Eeprom(PORT15_PB_VLAN_ADDR,&temp,2);

	temp = 0;
	for(u8 i=0;i<ALL_PORT_NUM;i++){
		temp |= get_pb_vlan_port(15,i)<<i;
	}
	Write_Eeprom(PORT16_PB_VLAN_ADDR,&temp,2);


	//port based vlan for swu-16
	temp = get_pb_vlan_swu_port(0);
	Write_Eeprom(PORT1_PB_VLAN_SWU_ADDR,&temp,2);

	temp = get_pb_vlan_swu_port(1);
	Write_Eeprom(PORT2_PB_VLAN_SWU_ADDR,&temp,2);

	temp = get_pb_vlan_swu_port(2);
	Write_Eeprom(PORT3_PB_VLAN_SWU_ADDR,&temp,2);

	temp = get_pb_vlan_swu_port(3);
	Write_Eeprom(PORT4_PB_VLAN_SWU_ADDR,&temp,2);

	temp = get_pb_vlan_swu_port(4);
	Write_Eeprom(PORT5_PB_VLAN_SWU_ADDR,&temp,2);

	temp = get_pb_vlan_swu_port(5);
	Write_Eeprom(PORT6_PB_VLAN_SWU_ADDR,&temp,2);

	temp = get_pb_vlan_swu_port(6);
	Write_Eeprom(PORT7_PB_VLAN_SWU_ADDR,&temp,2);

	temp = get_pb_vlan_swu_port(7);
	Write_Eeprom(PORT8_PB_VLAN_SWU_ADDR,&temp,2);

	temp = get_pb_vlan_swu_port(8);
	Write_Eeprom(PORT9_PB_VLAN_SWU_ADDR,&temp,2);

	temp = get_pb_vlan_swu_port(9);
	Write_Eeprom(PORT10_PB_VLAN_SWU_ADDR,&temp,2);

	temp = get_pb_vlan_swu_port(10);
	Write_Eeprom(PORT11_PB_VLAN_SWU_ADDR,&temp,2);

	temp = get_pb_vlan_swu_port(11);
	Write_Eeprom(PORT12_PB_VLAN_SWU_ADDR,&temp,2);

	temp = get_pb_vlan_swu_port(12);
	Write_Eeprom(PORT13_PB_VLAN_SWU_ADDR,&temp,2);

	temp = get_pb_vlan_swu_port(13);
	Write_Eeprom(PORT14_PB_VLAN_SWU_ADDR,&temp,2);

	temp = get_pb_vlan_swu_port(14);
	Write_Eeprom(PORT15_PB_VLAN_SWU_ADDR,&temp,2);

	temp = get_pb_vlan_swu_port(15);
	Write_Eeprom(PORT16_PB_VLAN_SWU_ADDR,&temp,2);



	temp = get_vlan_sett_state();
	Write_Eeprom(VLAN_STATE_ADDR,&temp,2);

	IWDG_ReloadCounter();

	temp = get_vlan_sett_mngt();
	Write_Eeprom(VLAN_MVID_ADDR,&temp,2);

	temp = get_vlan_trunk_state();
	Write_Eeprom(VLAN_TRUNK_STATE_ADDR,&temp,2);

	temp = get_vlan_sett_port_state(0);
	Write_Eeprom(PORT1_VLAN_ST_ADDR,&temp,2);

	temp = get_vlan_sett_port_state(1);
	Write_Eeprom(PORT2_VLAN_ST_ADDR,&temp,2);

	temp = get_vlan_sett_port_state(2);
	Write_Eeprom(PORT3_VLAN_ST_ADDR,&temp,2);

	temp = get_vlan_sett_port_state(3);
	Write_Eeprom(PORT4_VLAN_ST_ADDR,&temp,2);

	temp = get_vlan_sett_port_state(4);
	Write_Eeprom(PORT5_VLAN_ST_ADDR,&temp,2);

	temp = get_vlan_sett_port_state(5);
	Write_Eeprom(PORT6_VLAN_ST_ADDR,&temp,2);

	temp = get_vlan_sett_port_state(6);
	Write_Eeprom(PORT7_VLAN_ST_ADDR,&temp,2);

	temp = get_vlan_sett_port_state(7);
	Write_Eeprom(PORT8_VLAN_ST_ADDR,&temp,2);

	temp = get_vlan_sett_port_state(8);
	Write_Eeprom(PORT9_VLAN_ST_ADDR,&temp,2);

	temp = get_vlan_sett_port_state(9);
	Write_Eeprom(PORT10_VLAN_ST_ADDR,&temp,2);

	temp = get_vlan_sett_port_state(10);
	Write_Eeprom(PORT11_VLAN_ST_ADDR,&temp,2);

	temp = get_vlan_sett_port_state(11);
	Write_Eeprom(PORT12_VLAN_ST_ADDR,&temp,2);

	temp = get_vlan_sett_port_state(12);
	Write_Eeprom(PORT13_VLAN_ST_ADDR,&temp,2);

	temp = get_vlan_sett_port_state(13);
	Write_Eeprom(PORT14_VLAN_ST_ADDR,&temp,2);

	temp = get_vlan_sett_port_state(14);
	Write_Eeprom(PORT15_VLAN_ST_ADDR,&temp,2);

	temp = get_vlan_sett_port_state(15);
	Write_Eeprom(PORT16_VLAN_ST_ADDR,&temp,2);



	temp = get_vlan_sett_dvid(0);
	Write_Eeprom(PORT1_DEF_VID_ADDR,&temp,2);

	temp = get_vlan_sett_dvid(1);
	Write_Eeprom(PORT2_DEF_VID_ADDR,&temp,2);

	temp = get_vlan_sett_dvid(2);
	Write_Eeprom(PORT3_DEF_VID_ADDR,&temp,2);

	temp = get_vlan_sett_dvid(3);
	Write_Eeprom(PORT4_DEF_VID_ADDR,&temp,2);

	temp = get_vlan_sett_dvid(4);
	Write_Eeprom(PORT5_DEF_VID_ADDR,&temp,2);

	temp = get_vlan_sett_dvid(5);
	Write_Eeprom(PORT6_DEF_VID_ADDR,&temp,2);

	temp = get_vlan_sett_dvid(6);
	Write_Eeprom(PORT7_DEF_VID_ADDR,&temp,2);

	temp = get_vlan_sett_dvid(7);
	Write_Eeprom(PORT8_DEF_VID_ADDR,&temp,2);

	temp = get_vlan_sett_dvid(8);
	Write_Eeprom(PORT9_DEF_VID_ADDR,&temp,2);

	temp = get_vlan_sett_dvid(9);
	Write_Eeprom(PORT10_DEF_VID_ADDR,&temp,2);

	temp = get_vlan_sett_dvid(10);
	Write_Eeprom(PORT11_DEF_VID_ADDR,&temp,2);

	temp = get_vlan_sett_dvid(11);
	Write_Eeprom(PORT12_DEF_VID_ADDR,&temp,2);

	temp = get_vlan_sett_dvid(12);
	Write_Eeprom(PORT13_DEF_VID_ADDR,&temp,2);

	temp = get_vlan_sett_dvid(13);
	Write_Eeprom(PORT14_DEF_VID_ADDR,&temp,2);

	temp = get_vlan_sett_dvid(14);
	Write_Eeprom(PORT15_DEF_VID_ADDR,&temp,2);

	temp = get_vlan_sett_dvid(15);
	Write_Eeprom(PORT16_DEF_VID_ADDR,&temp,2);


	temp = get_vlan_sett_vlannum();
	Write_Eeprom(VLAN_NUM_ADDR,&temp,2);


	for(u8 i=0;i<MAXVlanNum;i++){
		temp_vlan.state = get_vlan_state(i);
		temp_vlan.VID = get_vlan_vid(i);
		strncpy(temp_vlan.VLANNAme,get_vlan_name(i),17);
		for(u8 j=0;j<ALL_PORT_NUM;j++)
			temp_vlan.Ports[j] = get_vlan_port_state(i,j);
		//первые 20
		if(i<20)
			Write_Eeprom(VLAN1_ADDR+i*80,&temp_vlan,80);
		else
			Write_Eeprom(VLAN21_ADDR+(i-20)*80,&temp_vlan,80);
	}




	temp = get_stp_state();
	Write_Eeprom(STP_STATE_ADDR,&temp,2);

	temp = get_stp_magic();
	Write_Eeprom(STP_MAGIC_ADDR,&temp,2);

	temp = get_stp_proto();
	Write_Eeprom(STP_PROTO_ADDR,&temp,2);

	temp = get_stp_bridge_priority();
	Write_Eeprom(STP_BRIDGE_PRIOR_ADDR,&temp,2);

	temp = get_stp_bridge_max_age();
	Write_Eeprom(STP_MAX_AGE_ADDR,&temp,2);

	temp = get_stp_bridge_htime();
	Write_Eeprom(STP_HELLO_TIME_ADDR,&temp,2);

	temp = get_stp_bridge_fdelay();
	Write_Eeprom(STP_FORW_DELAY_ADDR,&temp,2);

	temp = get_stp_bridge_mdelay();
	Write_Eeprom(STP_MIGRATE_DELAY_ADDR,&temp,2);

	temp = get_stp_txholdcount();
	Write_Eeprom(STP_TX_HCOUNT_ADDR,&temp,2);

	for(u8 i=0;i<(ALL_PORT_NUM);i++){
		stp_temp.enable = get_stp_port_enable(i);
		stp_temp.state = get_stp_port_state(i);
		stp_temp.priority = get_stp_port_priority(i);
		stp_temp.path_cost = get_stp_port_cost(i);
		stp_temp.flags = settings.stp_port_sett[i].flags;

		switch(i){
			case 0:Write_Eeprom(STP_PORT1_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 1:Write_Eeprom(STP_PORT2_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 2:Write_Eeprom(STP_PORT3_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 3:Write_Eeprom(STP_PORT4_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 4:Write_Eeprom(STP_PORT5_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 5:Write_Eeprom(STP_PORT6_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 6:Write_Eeprom(STP_PORT7_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 7:Write_Eeprom(STP_PORT8_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 8:Write_Eeprom(STP_PORT9_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 9:Write_Eeprom(STP_PORT10_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 10:Write_Eeprom(STP_PORT11_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 11:Write_Eeprom(STP_PORT12_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 12:Write_Eeprom(STP_PORT13_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 13:Write_Eeprom(STP_PORT14_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 14:Write_Eeprom(STP_PORT15_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
			case 15:Write_Eeprom(STP_PORT16_CFG_ADDR,&stp_temp,sizeof(stp_port_sett_t));break;
		}
	}

	//port BPDU forwarding
	temp = get_stp_bpdu_fw();
	Write_Eeprom(STP_FWBPDU_ADDR,&temp,2);


	temp = get_callibrate_koef_1(0);
	Write_Eeprom(PORT1_VCT_ADJ_ADDR,&temp,2);

	temp = get_callibrate_koef_1(1);
	Write_Eeprom(PORT2_VCT_ADJ_ADDR,&temp,2);

	temp = get_callibrate_koef_1(2);
	Write_Eeprom(PORT3_VCT_ADJ_ADDR,&temp,2);

	temp = get_callibrate_koef_1(3);
	Write_Eeprom(PORT4_VCT_ADJ_ADDR,&temp,2);

	temp = get_callibrate_koef_1(4);
	Write_Eeprom(PORT5_VCT_ADJ_ADDR,&temp,2);

	temp = get_callibrate_koef_1(5);
	Write_Eeprom(PORT6_VCT_ADJ_ADDR,&temp,2);

	temp = get_callibrate_koef_1(6);
	Write_Eeprom(PORT7_VCT_ADJ_ADDR,&temp,2);

	temp = get_callibrate_koef_1(7);
	Write_Eeprom(PORT8_VCT_ADJ_ADDR,&temp,2);

	temp = get_callibrate_koef_1(8);
	Write_Eeprom(PORT9_VCT_ADJ_ADDR,&temp,2);

	temp = get_callibrate_koef_1(9);
	Write_Eeprom(PORT10_VCT_ADJ_ADDR,&temp,2);

	temp = get_callibrate_koef_1(10);
	Write_Eeprom(PORT11_VCT_ADJ_ADDR,&temp,2);

	temp = get_callibrate_koef_1(11);
	Write_Eeprom(PORT12_VCT_ADJ_ADDR,&temp,2);

	temp = get_callibrate_koef_1(12);
	Write_Eeprom(PORT13_VCT_ADJ_ADDR,&temp,2);

	temp = get_callibrate_koef_1(13);
	Write_Eeprom(PORT14_VCT_ADJ_ADDR,&temp,2);

	temp = get_callibrate_koef_1(14);
	Write_Eeprom(PORT15_VCT_ADJ_ADDR,&temp,2);

	temp = get_callibrate_koef_1(15);
	Write_Eeprom(PORT16_VCT_ADJ_ADDR,&temp,2);




	temp  = get_callibrate_len(0);
	Write_Eeprom(PORT1_VCT_LEN_ADDR,&temp,2);

	temp  = get_callibrate_len(1);
	Write_Eeprom(PORT2_VCT_LEN_ADDR,&temp,2);

	temp  = get_callibrate_len(2);
	Write_Eeprom(PORT3_VCT_LEN_ADDR,&temp,2);

	temp  = get_callibrate_len(3);
	Write_Eeprom(PORT4_VCT_LEN_ADDR,&temp,2);

	temp  = get_callibrate_len(4);
	Write_Eeprom(PORT5_VCT_LEN_ADDR,&temp,2);

	temp  = get_callibrate_len(5);
	Write_Eeprom(PORT6_VCT_LEN_ADDR,&temp,2);

	temp  = get_callibrate_len(6);
	Write_Eeprom(PORT7_VCT_LEN_ADDR,&temp,2);

	temp  = get_callibrate_len(7);
	Write_Eeprom(PORT8_VCT_LEN_ADDR,&temp,2);

	temp  = get_callibrate_len(8);
	Write_Eeprom(PORT9_VCT_LEN_ADDR,&temp,2);

	temp  = get_callibrate_len(9);
	Write_Eeprom(PORT10_VCT_LEN_ADDR,&temp,2);

	temp  = get_callibrate_len(10);
	Write_Eeprom(PORT11_VCT_LEN_ADDR,&temp,2);

	temp  = get_callibrate_len(11);
	Write_Eeprom(PORT12_VCT_LEN_ADDR,&temp,2);

	temp  = get_callibrate_len(12);
	Write_Eeprom(PORT13_VCT_LEN_ADDR,&temp,2);

	temp  = get_callibrate_len(13);
	Write_Eeprom(PORT14_VCT_LEN_ADDR,&temp,2);

	temp  = get_callibrate_len(14);
	Write_Eeprom(PORT15_VCT_LEN_ADDR,&temp,2);

	temp  = get_callibrate_len(15);
	Write_Eeprom(PORT16_VCT_LEN_ADDR,&temp,2);


	temp = get_snmp_state();
	Write_Eeprom(SNMP_STATE_ADDR,&temp,2);

	temp = get_snmp_mode();
	Write_Eeprom(SNMP_MODE_ADDR,&temp,2);

	get_snmp_serv(&ip);
	Write_Eeprom(SNMP_SERVER_ADDR,ip,4);

	temp = get_snmp_vers();
	Write_Eeprom(SNMP_VERS_ADDR,&temp,2);

	temp = get_softstart_time();
	Write_Eeprom(PORT_SOFTSTART_TIME_ADDR,&temp,2);

	//get_snmp_communitie()
	Write_Eeprom(SNMP_COMMUNITY1_ADDR,settings.snmp_sett.snmp1_read_commun.community,68);

	Write_Eeprom(SNMP_COMMUNITY2_ADDR,settings.snmp_sett.snmp1_write_commun.community,68);


	//snmp3
	//1
	temp = get_snmp3_level(0);
	Write_Eeprom(SNMP3_USER1_LEVEL,&temp,2);

	get_snmp3_user_name(0,tmp);
	Write_Eeprom(SNMP3_USER1_USER_NAME,tmp,64);

	get_snmp3_auth_pass(0,tmp);
	Write_Eeprom(SNMP3_USER1_AUTH_PASS,tmp,64);

	get_snmp3_priv_pass(0,tmp);
	Write_Eeprom(SNMP3_USER1_PRIV_PASS,tmp,64);

	//2
	temp = get_snmp3_level(1);
	Write_Eeprom(SNMP3_USER2_LEVEL,&temp,2);

	get_snmp3_user_name(1,tmp);
	Write_Eeprom(SNMP3_USER2_USER_NAME,tmp,64);

	get_snmp3_auth_pass(1,tmp);
	Write_Eeprom(SNMP3_USER2_AUTH_PASS,tmp,64);

	get_snmp3_priv_pass(1,tmp);
	Write_Eeprom(SNMP3_USER2_PRIV_PASS,tmp,64);

	//3
	temp = get_snmp3_level(2);
	Write_Eeprom(SNMP3_USER3_LEVEL,&temp,2);

	get_snmp3_user_name(2,tmp);
	Write_Eeprom(SNMP3_USER3_USER_NAME,tmp,64);

	get_snmp3_auth_pass(2,tmp);
	Write_Eeprom(SNMP3_USER3_AUTH_PASS,tmp,64);

	get_snmp3_priv_pass(2,tmp);
	Write_Eeprom(SNMP3_USER3_PRIV_PASS,tmp,64);

	get_snmp3_engine_id(&eid);
	Write_Eeprom(SNMP3_ENGINE_ID_PTR,eid.ptr,64);
	Write_Eeprom(SNMP3_ENGINE_ID_LEN,&eid.len,2);

	//igmp settings
	temp = get_igmp_snooping_state();
	Write_Eeprom(IGMP_STATE_ADDR,&temp,2);

	//igmp port state settings
	temp = get_igmp_port_state(0);
	Write_Eeprom(IGMP_PORT_1_STATE_ADDR,&temp,2);

	temp = get_igmp_port_state(1);
	Write_Eeprom(IGMP_PORT_2_STATE_ADDR,&temp,2);

	temp = get_igmp_port_state(2);
	Write_Eeprom(IGMP_PORT_3_STATE_ADDR,&temp,2);

	temp = get_igmp_port_state(3);
	Write_Eeprom(IGMP_PORT_4_STATE_ADDR,&temp,2);

	temp = get_igmp_port_state(4);
	Write_Eeprom(IGMP_PORT_5_STATE_ADDR,&temp,2);

	temp = get_igmp_port_state(5);
	Write_Eeprom(IGMP_PORT_6_STATE_ADDR,&temp,2);

	temp = get_igmp_port_state(6);
	Write_Eeprom(IGMP_PORT_7_STATE_ADDR,&temp,2);

	temp = get_igmp_port_state(7);
	Write_Eeprom(IGMP_PORT_8_STATE_ADDR,&temp,2);

	temp = get_igmp_port_state(8);
	Write_Eeprom(IGMP_PORT_9_STATE_ADDR,&temp,2);

	temp = get_igmp_port_state(9);
	Write_Eeprom(IGMP_PORT_10_STATE_ADDR,&temp,2);

	temp = get_igmp_port_state(10);
	Write_Eeprom(IGMP_PORT_11_STATE_ADDR,&temp,2);

	temp = get_igmp_port_state(11);
	Write_Eeprom(IGMP_PORT_12_STATE_ADDR,&temp,2);

	temp = get_igmp_port_state(12);
	Write_Eeprom(IGMP_PORT_13_STATE_ADDR,&temp,2);

	temp = get_igmp_port_state(13);
	Write_Eeprom(IGMP_PORT_14_STATE_ADDR,&temp,2);

	temp = get_igmp_port_state(14);
	Write_Eeprom(IGMP_PORT_15_STATE_ADDR,&temp,2);

	temp = get_igmp_port_state(15);
	Write_Eeprom(IGMP_PORT_16_STATE_ADDR,&temp,2);





	temp = get_igmp_query_int();
	Write_Eeprom(IGMP_QUERY_INTERVAL_ADDR,&temp,2);

	temp = get_igmp_query_mode();
	Write_Eeprom(IGMP_QUERY_MODE_ADDR,&temp,2);

	temp = get_igmp_max_resp_time();
	Write_Eeprom(IGMP_QUERY_RESP_INTERVAL_ADDR,&temp,2);

	temp = get_igmp_group_membership_time();
	Write_Eeprom(IGMP_GROUP_MEMB_TIME_ADDR,&temp,2);

	temp = get_igmp_other_querier_time();
	Write_Eeprom(IGMP_OTHER_QUERIER_ADDR,&temp,2);


	temp = get_telnet_state();
	Write_Eeprom(TELNET_STATE_ADDR,&temp,2);

	temp = get_telnet_echo();
	Write_Eeprom(TELNET_ECHO_ADDR,&temp,2);

	temp = get_downshifting_mode();
	Write_Eeprom(DOWNSHIFT_STATE_ADDR,&temp,2);

	temp = get_tftp_state();
	Write_Eeprom(TFTP_STATE_ADDR,&temp,2);

	temp = get_tftp_mode();
	Write_Eeprom(TFTP_MODE_ADDR,&temp,2);

	temp = get_tftp_port();
	Write_Eeprom(TFTP_PORT_ADDR,&temp,2);

	temp = get_plc_out_state(0);
	Write_Eeprom(PLC_OUT1_STATE_ADDR,&temp,2);

	temp = get_plc_out_state(1);
	Write_Eeprom(PLC_OUT2_STATE_ADDR,&temp,2);

	temp = get_plc_out_state(2);
	Write_Eeprom(PLC_OUT3_STATE_ADDR,&temp,2);

	temp = get_plc_out_state(3);
	Write_Eeprom(PLC_OUT4_STATE_ADDR,&temp,2);

	temp = get_plc_out_action(0);
	Write_Eeprom(PLC_OUT1_ACTION_ADDR,&temp,2);

	temp = get_plc_out_action(1);
	Write_Eeprom(PLC_OUT2_ACTION_ADDR,&temp,2);

	temp = get_plc_out_action(2);
	Write_Eeprom(PLC_OUT3_ACTION_ADDR,&temp,2);

	temp = get_plc_out_action(3);
	Write_Eeprom(PLC_OUT4_ACTION_ADDR,&temp,2);

	temp = get_plc_out_event(0,0);
	Write_Eeprom(PLC_OUT1_EVENT1_ADDR,&temp,2);

	temp = get_plc_out_event(0,1);
	Write_Eeprom(PLC_OUT1_EVENT2_ADDR,&temp,2);

	temp = get_plc_out_event(0,2);
	Write_Eeprom(PLC_OUT1_EVENT3_ADDR,&temp,2);

	temp = get_plc_out_event(0,3);
	Write_Eeprom(PLC_OUT1_EVENT4_ADDR,&temp,2);

	temp = get_plc_out_event(0,4);
	Write_Eeprom(PLC_OUT1_EVENT5_ADDR,&temp,2);

	temp = get_plc_out_event(0,5);
	Write_Eeprom(PLC_OUT1_EVENT6_ADDR,&temp,2);

	temp = get_plc_out_event(0,6);
	Write_Eeprom(PLC_OUT1_EVENT7_ADDR,&temp,2);


	temp = get_plc_out_event(1,0);
	Write_Eeprom(PLC_OUT2_EVENT1_ADDR,&temp,2);

	temp = get_plc_out_event(1,1);
	Write_Eeprom(PLC_OUT2_EVENT2_ADDR,&temp,2);

	temp = get_plc_out_event(1,2);
	Write_Eeprom(PLC_OUT2_EVENT3_ADDR,&temp,2);

	temp = get_plc_out_event(1,3);
	Write_Eeprom(PLC_OUT2_EVENT4_ADDR,&temp,2);

	temp = get_plc_out_event(1,4);
	Write_Eeprom(PLC_OUT2_EVENT5_ADDR,&temp,2);

	temp = get_plc_out_event(1,5);
	Write_Eeprom(PLC_OUT2_EVENT6_ADDR,&temp,2);

	temp = get_plc_out_event(1,6);
	Write_Eeprom(PLC_OUT2_EVENT7_ADDR,&temp,2);


	temp = get_plc_out_event(2,0);
	Write_Eeprom(PLC_OUT3_EVENT1_ADDR,&temp,2);

	temp = get_plc_out_event(2,1);
	Write_Eeprom(PLC_OUT3_EVENT2_ADDR,&temp,2);

	temp = get_plc_out_event(2,2);
	Write_Eeprom(PLC_OUT3_EVENT3_ADDR,&temp,2);

	temp = get_plc_out_event(2,3);
	Write_Eeprom(PLC_OUT3_EVENT4_ADDR,&temp,2);

	temp = get_plc_out_event(2,4);
	Write_Eeprom(PLC_OUT3_EVENT5_ADDR,&temp,2);

	temp = get_plc_out_event(2,5);
	Write_Eeprom(PLC_OUT3_EVENT6_ADDR,&temp,2);

	temp = get_plc_out_event(2,6);
	Write_Eeprom(PLC_OUT3_EVENT7_ADDR,&temp,2);


	temp = get_plc_out_event(3,0);
	Write_Eeprom(PLC_OUT4_EVENT1_ADDR,&temp,2);

	temp = get_plc_out_event(3,1);
	Write_Eeprom(PLC_OUT4_EVENT2_ADDR,&temp,2);

	temp = get_plc_out_event(3,2);
	Write_Eeprom(PLC_OUT4_EVENT3_ADDR,&temp,2);

	temp = get_plc_out_event(3,3);
	Write_Eeprom(PLC_OUT4_EVENT4_ADDR,&temp,2);

	temp = get_plc_out_event(3,4);
	Write_Eeprom(PLC_OUT4_EVENT5_ADDR,&temp,2);

	temp = get_plc_out_event(3,5);
	Write_Eeprom(PLC_OUT4_EVENT6_ADDR,&temp,2);

	temp = get_plc_out_event(3,6);
	Write_Eeprom(PLC_OUT4_EVENT7_ADDR,&temp,2);


	temp = get_plc_in_state(0);
	Write_Eeprom(PLC_IN1_STATE_ADDR,&temp,2);
	temp = get_plc_in_state(1);
	Write_Eeprom(PLC_IN2_STATE_ADDR,&temp,2);
	temp = get_plc_in_state(2);
	Write_Eeprom(PLC_IN3_STATE_ADDR,&temp,2);
	temp = get_plc_in_state(3);
	Write_Eeprom(PLC_IN4_STATE_ADDR,&temp,2);


	temp = get_plc_in_alarm_state(0);
	Write_Eeprom(PLC_IN1_ALARM_ADDR,&temp,2);
	temp = get_plc_in_alarm_state(1);
	Write_Eeprom(PLC_IN2_ALARM_ADDR,&temp,2);
	temp = get_plc_in_alarm_state(2);
	Write_Eeprom(PLC_IN3_ALARM_ADDR,&temp,2);
	temp = get_plc_in_alarm_state(3);
	Write_Eeprom(PLC_IN4_ALARM_ADDR,&temp,2);


	temp = get_plc_em_model();
	Write_Eeprom(PLC_EM_MODEL_ADDR,&temp,2);

	temp = get_plc_em_rate();
	Write_Eeprom(PLC_EM_BAUDRATE_ADDR,&temp,2);

	temp = get_plc_em_parity();
	Write_Eeprom(PLC_EM_PARITY_ADDR,&temp,2);


	temp = get_plc_em_databits();
	Write_Eeprom(PLC_EM_DATABITS_ADDR,&temp,2);

	temp = get_plc_em_stopbits();
	Write_Eeprom(PLC_EM_STOPBITS_ADDR,&temp,2);

	memset(tmp,0,10);
	get_plc_em_pass(tmp);
	Write_Eeprom(PLC_EM_PASS_ADDR,tmp,10);

	memset(tmp,0,32);
	get_plc_em_id(tmp);
	Write_Eeprom(PLC_EM_ID_ADDR,tmp,32);

	//mac learning on port
	temp = get_mac_filter_state(0);
	Write_Eeprom(PORT1_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(1);
	Write_Eeprom(PORT2_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(2);
	Write_Eeprom(PORT3_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(3);
	Write_Eeprom(PORT4_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(4);
	Write_Eeprom(PORT5_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(5);
	Write_Eeprom(PORT6_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(6);
	Write_Eeprom(PORT7_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(7);
	Write_Eeprom(PORT8_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(8);
	Write_Eeprom(PORT9_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(9);
	Write_Eeprom(PORT10_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(10);
	Write_Eeprom(PORT11_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(11);
	Write_Eeprom(PORT12_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(12);
	Write_Eeprom(PORT13_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(13);
	Write_Eeprom(PORT14_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(14);
	Write_Eeprom(PORT15_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(15);
	Write_Eeprom(PORT16_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(16);
	Write_Eeprom(PORT17_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(17);
	Write_Eeprom(PORT18_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(18);
	Write_Eeprom(PORT19_MACFILT_ADDR,&temp,2);

	temp = get_mac_filter_state(19);
	Write_Eeprom(PORT20_MACFILT_ADDR,&temp,2);


	//cpu port
	temp = get_mac_learn_cpu();
	Write_Eeprom(CPU_MACLEARN_ADDR,&temp,2);

	//MAC FILTERING TABLE
	temp = get_mac_bind_entry_active(0);
	Write_Eeprom(MAC_BIND_ENRTY1_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(0,i);
	Write_Eeprom(MAC_BIND_ENRTY1_MAC,tmp,6);
	temp = get_mac_bind_entry_port(0);
	Write_Eeprom(MAC_BIND_ENRTY1_PORT,&temp,2);

	temp = get_mac_bind_entry_active(1);
	Write_Eeprom(MAC_BIND_ENRTY2_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(1,i);
	Write_Eeprom(MAC_BIND_ENRTY2_MAC,tmp,6);
	temp = get_mac_bind_entry_port(1);
	Write_Eeprom(MAC_BIND_ENRTY2_PORT,&temp,2);

	temp = get_mac_bind_entry_active(2);
	Write_Eeprom(MAC_BIND_ENRTY3_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(2,i);
	Write_Eeprom(MAC_BIND_ENRTY3_MAC,tmp,6);
	temp = get_mac_bind_entry_port(2);
	Write_Eeprom(MAC_BIND_ENRTY3_PORT,&temp,2);

	temp = get_mac_bind_entry_active(3);
	Write_Eeprom(MAC_BIND_ENRTY4_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(3,i);
	Write_Eeprom(MAC_BIND_ENRTY4_MAC,tmp,6);
	temp = get_mac_bind_entry_port(3);
	Write_Eeprom(MAC_BIND_ENRTY4_PORT,&temp,2);

	temp = get_mac_bind_entry_active(4);
	Write_Eeprom(MAC_BIND_ENRTY5_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(4,i);
	Write_Eeprom(MAC_BIND_ENRTY5_MAC,tmp,6);
	temp = get_mac_bind_entry_port(4);
	Write_Eeprom(MAC_BIND_ENRTY5_PORT,&temp,2);

	temp = get_mac_bind_entry_active(5);
	Write_Eeprom(MAC_BIND_ENRTY6_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(5,i);
	Write_Eeprom(MAC_BIND_ENRTY6_MAC,tmp,6);
	temp = get_mac_bind_entry_port(5);
	Write_Eeprom(MAC_BIND_ENRTY6_PORT,&temp,2);

	temp = get_mac_bind_entry_active(6);
	Write_Eeprom(MAC_BIND_ENRTY7_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(6,i);
	Write_Eeprom(MAC_BIND_ENRTY7_MAC,tmp,6);
	temp = get_mac_bind_entry_port(6);
	Write_Eeprom(MAC_BIND_ENRTY7_PORT,&temp,2);

	temp = get_mac_bind_entry_active(7);
	Write_Eeprom(MAC_BIND_ENRTY8_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(7,i);
	Write_Eeprom(MAC_BIND_ENRTY8_MAC,tmp,6);
	temp = get_mac_bind_entry_port(7);
	Write_Eeprom(MAC_BIND_ENRTY8_PORT,&temp,2);

	temp = get_mac_bind_entry_active(8);
	Write_Eeprom(MAC_BIND_ENRTY9_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(8,i);
	Write_Eeprom(MAC_BIND_ENRTY9_MAC,tmp,6);
	temp = get_mac_bind_entry_port(8);
	Write_Eeprom(MAC_BIND_ENRTY9_PORT,&temp,2);

	temp = get_mac_bind_entry_active(9);
	Write_Eeprom(MAC_BIND_ENRTY10_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(9,i);
	Write_Eeprom(MAC_BIND_ENRTY10_MAC,tmp,6);
	temp = get_mac_bind_entry_port(9);
	Write_Eeprom(MAC_BIND_ENRTY10_PORT,&temp,2);

	temp = get_mac_bind_entry_active(10);
	Write_Eeprom(MAC_BIND_ENRTY11_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(10,i);
	Write_Eeprom(MAC_BIND_ENRTY11_MAC,tmp,6);
	temp = get_mac_bind_entry_port(10);
	Write_Eeprom(MAC_BIND_ENRTY11_PORT,&temp,2);

	temp = get_mac_bind_entry_active(11);
	Write_Eeprom(MAC_BIND_ENRTY12_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(11,i);
	Write_Eeprom(MAC_BIND_ENRTY12_MAC,tmp,6);
	temp = get_mac_bind_entry_port(11);
	Write_Eeprom(MAC_BIND_ENRTY12_PORT,&temp,2);

	temp = get_mac_bind_entry_active(12);
	Write_Eeprom(MAC_BIND_ENRTY13_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(12,i);
	Write_Eeprom(MAC_BIND_ENRTY13_MAC,tmp,6);
	temp = get_mac_bind_entry_port(12);
	Write_Eeprom(MAC_BIND_ENRTY13_PORT,&temp,2);

	temp = get_mac_bind_entry_active(13);
	Write_Eeprom(MAC_BIND_ENRTY14_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(13,i);
	Write_Eeprom(MAC_BIND_ENRTY14_MAC,tmp,6);
	temp = get_mac_bind_entry_port(13);
	Write_Eeprom(MAC_BIND_ENRTY14_PORT,&temp,2);

	temp = get_mac_bind_entry_active(14);
	Write_Eeprom(MAC_BIND_ENRTY15_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(14,i);
	Write_Eeprom(MAC_BIND_ENRTY15_MAC,tmp,6);
	temp = get_mac_bind_entry_port(14);
	Write_Eeprom(MAC_BIND_ENRTY15_PORT,&temp,2);

	temp = get_mac_bind_entry_active(15);
	Write_Eeprom(MAC_BIND_ENRTY16_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(15,i);
	Write_Eeprom(MAC_BIND_ENRTY16_MAC,tmp,6);
	temp = get_mac_bind_entry_port(15);
	Write_Eeprom(MAC_BIND_ENRTY16_PORT,&temp,2);

	temp = get_mac_bind_entry_active(16);
	Write_Eeprom(MAC_BIND_ENRTY17_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(16,i);
	Write_Eeprom(MAC_BIND_ENRTY17_MAC,tmp,6);
	temp = get_mac_bind_entry_port(16);
	Write_Eeprom(MAC_BIND_ENRTY17_PORT,&temp,2);

	temp = get_mac_bind_entry_active(17);
	Write_Eeprom(MAC_BIND_ENRTY18_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(17,i);
	Write_Eeprom(MAC_BIND_ENRTY18_MAC,tmp,6);
	temp = get_mac_bind_entry_port(17);
	Write_Eeprom(MAC_BIND_ENRTY18_PORT,&temp,2);

	temp = get_mac_bind_entry_active(18);
	Write_Eeprom(MAC_BIND_ENRTY19_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(18,i);
	Write_Eeprom(MAC_BIND_ENRTY19_MAC,tmp,6);
	temp = get_mac_bind_entry_port(18);
	Write_Eeprom(MAC_BIND_ENRTY19_PORT,&temp,2);

	temp = get_mac_bind_entry_active(19);
	Write_Eeprom(MAC_BIND_ENRTY20_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(19,i);
	Write_Eeprom(MAC_BIND_ENRTY20_MAC,tmp,6);
	temp = get_mac_bind_entry_port(19);
	Write_Eeprom(MAC_BIND_ENRTY20_PORT,&temp,2);

	temp = get_mac_bind_entry_active(20);
	Write_Eeprom(MAC_BIND_ENRTY21_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(20,i);
	Write_Eeprom(MAC_BIND_ENRTY21_MAC,tmp,6);
	temp = get_mac_bind_entry_port(20);
	Write_Eeprom(MAC_BIND_ENRTY21_PORT,&temp,2);

	temp = get_mac_bind_entry_active(21);
	Write_Eeprom(MAC_BIND_ENRTY22_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(21,i);
	Write_Eeprom(MAC_BIND_ENRTY22_MAC,tmp,6);
	temp = get_mac_bind_entry_port(21);
	Write_Eeprom(MAC_BIND_ENRTY22_PORT,&temp,2);

	temp = get_mac_bind_entry_active(22);
	Write_Eeprom(MAC_BIND_ENRTY23_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(22,i);
	Write_Eeprom(MAC_BIND_ENRTY23_MAC,tmp,6);
	temp = get_mac_bind_entry_port(22);
	Write_Eeprom(MAC_BIND_ENRTY23_PORT,&temp,2);

	temp = get_mac_bind_entry_active(23);
	Write_Eeprom(MAC_BIND_ENRTY24_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(23,i);
	Write_Eeprom(MAC_BIND_ENRTY24_MAC,tmp,6);
	temp = get_mac_bind_entry_port(23);
	Write_Eeprom(MAC_BIND_ENRTY24_PORT,&temp,2);

	temp = get_mac_bind_entry_active(24);
	Write_Eeprom(MAC_BIND_ENRTY25_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(24,i);
	Write_Eeprom(MAC_BIND_ENRTY25_MAC,tmp,6);
	temp = get_mac_bind_entry_port(24);
	Write_Eeprom(MAC_BIND_ENRTY25_PORT,&temp,2);

	temp = get_mac_bind_entry_active(25);
	Write_Eeprom(MAC_BIND_ENRTY26_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(25,i);
	Write_Eeprom(MAC_BIND_ENRTY26_MAC,tmp,6);
	temp = get_mac_bind_entry_port(25);
	Write_Eeprom(MAC_BIND_ENRTY26_PORT,&temp,2);

	temp = get_mac_bind_entry_active(26);
	Write_Eeprom(MAC_BIND_ENRTY27_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(26,i);
	Write_Eeprom(MAC_BIND_ENRTY27_MAC,tmp,6);
	temp = get_mac_bind_entry_port(26);
	Write_Eeprom(MAC_BIND_ENRTY27_PORT,&temp,2);

	temp = get_mac_bind_entry_active(27);
	Write_Eeprom(MAC_BIND_ENRTY28_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(27,i);
	Write_Eeprom(MAC_BIND_ENRTY28_MAC,tmp,6);
	temp = get_mac_bind_entry_port(27);
	Write_Eeprom(MAC_BIND_ENRTY28_PORT,&temp,2);

	temp = get_mac_bind_entry_active(28);
	Write_Eeprom(MAC_BIND_ENRTY29_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(28,i);
	Write_Eeprom(MAC_BIND_ENRTY29_MAC,tmp,6);
	temp = get_mac_bind_entry_port(28);
	Write_Eeprom(MAC_BIND_ENRTY29_PORT,&temp,2);

	temp = get_mac_bind_entry_active(29);
	Write_Eeprom(MAC_BIND_ENRTY30_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(29,i);
	Write_Eeprom(MAC_BIND_ENRTY30_MAC,tmp,6);
	temp = get_mac_bind_entry_port(29);
	Write_Eeprom(MAC_BIND_ENRTY30_PORT,&temp,2);

	temp = get_mac_bind_entry_active(30);
	Write_Eeprom(MAC_BIND_ENRTY31_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(30,i);
	Write_Eeprom(MAC_BIND_ENRTY31_MAC,tmp,6);
	temp = get_mac_bind_entry_port(30);
	Write_Eeprom(MAC_BIND_ENRTY31_PORT,&temp,2);

	temp = get_mac_bind_entry_active(31);
	Write_Eeprom(MAC_BIND_ENRTY32_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(31,i);
	Write_Eeprom(MAC_BIND_ENRTY32_MAC,tmp,6);
	temp = get_mac_bind_entry_port(31);
	Write_Eeprom(MAC_BIND_ENRTY32_PORT,&temp,2);

	temp = get_mac_bind_entry_active(32);
	Write_Eeprom(MAC_BIND_ENRTY33_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(32,i);
	Write_Eeprom(MAC_BIND_ENRTY33_MAC,tmp,6);
	temp = get_mac_bind_entry_port(32);
	Write_Eeprom(MAC_BIND_ENRTY33_PORT,&temp,2);

	temp = get_mac_bind_entry_active(33);
	Write_Eeprom(MAC_BIND_ENRTY34_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(33,i);
	Write_Eeprom(MAC_BIND_ENRTY34_MAC,tmp,6);
	temp = get_mac_bind_entry_port(33);
	Write_Eeprom(MAC_BIND_ENRTY34_PORT,&temp,2);

	temp = get_mac_bind_entry_active(34);
	Write_Eeprom(MAC_BIND_ENRTY35_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(34,i);
	Write_Eeprom(MAC_BIND_ENRTY35_MAC,tmp,6);
	temp = get_mac_bind_entry_port(34);
	Write_Eeprom(MAC_BIND_ENRTY35_PORT,&temp,2);

	temp = get_mac_bind_entry_active(35);
	Write_Eeprom(MAC_BIND_ENRTY36_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(35,i);
	Write_Eeprom(MAC_BIND_ENRTY36_MAC,tmp,6);
	temp = get_mac_bind_entry_port(35);
	Write_Eeprom(MAC_BIND_ENRTY36_PORT,&temp,2);

	temp = get_mac_bind_entry_active(36);
	Write_Eeprom(MAC_BIND_ENRTY37_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(36,i);
	Write_Eeprom(MAC_BIND_ENRTY37_MAC,tmp,6);
	temp = get_mac_bind_entry_port(36);
	Write_Eeprom(MAC_BIND_ENRTY37_PORT,&temp,2);

	temp = get_mac_bind_entry_active(37);
	Write_Eeprom(MAC_BIND_ENRTY38_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(37,i);
	Write_Eeprom(MAC_BIND_ENRTY38_MAC,tmp,6);
	temp = get_mac_bind_entry_port(37);
	Write_Eeprom(MAC_BIND_ENRTY38_PORT,&temp,2);

	temp = get_mac_bind_entry_active(38);
	Write_Eeprom(MAC_BIND_ENRTY39_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(38,i);
	Write_Eeprom(MAC_BIND_ENRTY39_MAC,tmp,6);
	temp = get_mac_bind_entry_port(38);
	Write_Eeprom(MAC_BIND_ENRTY39_PORT,&temp,2);

	temp = get_mac_bind_entry_active(39);
	Write_Eeprom(MAC_BIND_ENRTY40_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(39,i);
	Write_Eeprom(MAC_BIND_ENRTY40_MAC,tmp,6);
	temp = get_mac_bind_entry_port(39);
	Write_Eeprom(MAC_BIND_ENRTY40_PORT,&temp,2);

	temp = get_mac_bind_entry_active(40);
	Write_Eeprom(MAC_BIND_ENRTY41_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(40,i);
	Write_Eeprom(MAC_BIND_ENRTY41_MAC,tmp,6);
	temp = get_mac_bind_entry_port(40);
	Write_Eeprom(MAC_BIND_ENRTY41_PORT,&temp,2);

	temp = get_mac_bind_entry_active(41);
	Write_Eeprom(MAC_BIND_ENRTY42_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(41,i);
	Write_Eeprom(MAC_BIND_ENRTY42_MAC,tmp,6);
	temp = get_mac_bind_entry_port(41);
	Write_Eeprom(MAC_BIND_ENRTY42_PORT,&temp,2);

	temp = get_mac_bind_entry_active(42);
	Write_Eeprom(MAC_BIND_ENRTY43_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(42,i);
	Write_Eeprom(MAC_BIND_ENRTY43_MAC,tmp,6);
	temp = get_mac_bind_entry_port(42);
	Write_Eeprom(MAC_BIND_ENRTY43_PORT,&temp,2);

	temp = get_mac_bind_entry_active(43);
	Write_Eeprom(MAC_BIND_ENRTY44_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(43,i);
	Write_Eeprom(MAC_BIND_ENRTY44_MAC,tmp,6);
	temp = get_mac_bind_entry_port(43);
	Write_Eeprom(MAC_BIND_ENRTY44_PORT,&temp,2);

	temp = get_mac_bind_entry_active(44);
	Write_Eeprom(MAC_BIND_ENRTY45_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(44,i);
	Write_Eeprom(MAC_BIND_ENRTY45_MAC,tmp,6);
	temp = get_mac_bind_entry_port(44);
	Write_Eeprom(MAC_BIND_ENRTY45_PORT,&temp,2);

	temp = get_mac_bind_entry_active(45);
	Write_Eeprom(MAC_BIND_ENRTY46_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(45,i);
	Write_Eeprom(MAC_BIND_ENRTY46_MAC,tmp,6);
	temp = get_mac_bind_entry_port(45);
	Write_Eeprom(MAC_BIND_ENRTY46_PORT,&temp,2);

	temp = get_mac_bind_entry_active(46);
	Write_Eeprom(MAC_BIND_ENRTY47_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(46,i);
	Write_Eeprom(MAC_BIND_ENRTY47_MAC,tmp,6);
	temp = get_mac_bind_entry_port(46);
	Write_Eeprom(MAC_BIND_ENRTY47_PORT,&temp,2);

	temp = get_mac_bind_entry_active(47);
	Write_Eeprom(MAC_BIND_ENRTY48_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(47,i);
	Write_Eeprom(MAC_BIND_ENRTY48_MAC,tmp,6);
	temp = get_mac_bind_entry_port(47);
	Write_Eeprom(MAC_BIND_ENRTY48_PORT,&temp,2);

	temp = get_mac_bind_entry_active(48);
	Write_Eeprom(MAC_BIND_ENRTY49_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(48,i);
	Write_Eeprom(MAC_BIND_ENRTY49_MAC,tmp,6);
	temp = get_mac_bind_entry_port(48);
	Write_Eeprom(MAC_BIND_ENRTY49_PORT,&temp,2);

	temp = get_mac_bind_entry_active(49);
	Write_Eeprom(MAC_BIND_ENRTY50_ACTIVE,&temp,2);
	for(u8 i=0;i<6;i++)	tmp[i] = get_mac_bind_entry_mac(49,i);
	Write_Eeprom(MAC_BIND_ENRTY50_MAC,tmp,6);
	temp = get_mac_bind_entry_port(49);
	Write_Eeprom(MAC_BIND_ENRTY50_PORT,&temp,2);


	//ups settings
	temp = get_ups_delayed_start();
	Write_Eeprom(UPS_DELAYED_START_ADDR,&temp,2);

	//link aggregation
	temp = get_lag_valid(0);
	Write_Eeprom(LAG_ENTRY1_VALID_ADDR,&temp,2);
	temp = get_lag_state(0);
	Write_Eeprom(LAG_ENTRY1_STATE_ADDR,&temp,2);
	temp = get_lag_master_port(0);
	Write_Eeprom(LAG_ENTRY1_MASTER_ADDR,&temp,2);
	for(u8 i=0;i<PORT_NUM;i++)
		tmp[i] = get_lag_port(0,i);
	Write_Eeprom(LAG_ENTRY1_PORTS_ADDR,tmp,32);

	temp = get_lag_valid(1);
	Write_Eeprom(LAG_ENTRY2_VALID_ADDR,&temp,2);
	temp = get_lag_state(1);
	Write_Eeprom(LAG_ENTRY2_STATE_ADDR,&temp,2);
	temp = get_lag_master_port(1);
	Write_Eeprom(LAG_ENTRY2_MASTER_ADDR,&temp,2);
	for(u8 i=0;i<PORT_NUM;i++)
		tmp[i] = get_lag_port(1,i);
	Write_Eeprom(LAG_ENTRY2_PORTS_ADDR,tmp,32);

	temp = get_lag_valid(2);
	Write_Eeprom(LAG_ENTRY3_VALID_ADDR,&temp,2);
	temp = get_lag_state(2);
	Write_Eeprom(LAG_ENTRY3_STATE_ADDR,&temp,2);
	temp = get_lag_master_port(2);
	Write_Eeprom(LAG_ENTRY3_MASTER_ADDR,&temp,2);
	for(u8 i=0;i<PORT_NUM;i++)
		tmp[i] = get_lag_port(2,i);
	Write_Eeprom(LAG_ENTRY3_PORTS_ADDR,tmp,32);

	temp = get_lag_valid(3);
	Write_Eeprom(LAG_ENTRY4_VALID_ADDR,&temp,2);
	temp = get_lag_state(3);
	Write_Eeprom(LAG_ENTRY4_STATE_ADDR,&temp,2);
	temp = get_lag_master_port(3);
	Write_Eeprom(LAG_ENTRY4_MASTER_ADDR,&temp,2);
	for(u8 i=0;i<PORT_NUM;i++)
		tmp[i] = get_lag_port(3,i);
	Write_Eeprom(LAG_ENTRY4_PORTS_ADDR,tmp,32);

	temp = get_lag_valid(4);
	Write_Eeprom(LAG_ENTRY5_VALID_ADDR,&temp,2);
	temp = get_lag_state(3);
	Write_Eeprom(LAG_ENTRY5_STATE_ADDR,&temp,2);
	temp = get_lag_master_port(4);
	Write_Eeprom(LAG_ENTRY5_MASTER_ADDR,&temp,2);
	for(u8 i=0;i<PORT_NUM;i++)
		tmp[i] = get_lag_port(4,i);
	Write_Eeprom(LAG_ENTRY5_PORTS_ADDR,tmp,32);


	//port mirroring
	temp = get_mirror_state();
	Write_Eeprom(MIRROR_ENTRY_STATE_ADDR,&temp,2);
	temp = get_mirror_target_port();
	Write_Eeprom(MIRROR_ENTRY_TARGET_ADDR,&temp,2);
	for(u8 i=0;i<PORT_NUM;i++)
		tmp[i] = get_mirror_port(i);
	Write_Eeprom(MIRROR_ENTRY_PORTS_ADDR,tmp,32);


	//teleport
	temp = get_input_state(0);
	Write_Eeprom(INPUT1_STATE_ADDR,&temp,2);
	temp = get_input_state(1);
	Write_Eeprom(INPUT2_STATE_ADDR,&temp,2);
	temp = get_input_state(2);
	Write_Eeprom(INPUT3_STATE_ADDR,&temp,2);

	temp = get_input_inverse(0);
	Write_Eeprom(INPUT1_INVERSE_ADDR,&temp,2);
	temp = get_input_inverse(1);
	Write_Eeprom(INPUT2_INVERSE_ADDR,&temp,2);
	temp = get_input_inverse(2);
	Write_Eeprom(INPUT3_INVERSE_ADDR,&temp,2);

	temp = get_input_rem_dev(0);
	Write_Eeprom(INPUT1_REMDEV_ADDR,&temp,2);
	temp = get_input_rem_dev(1);
	Write_Eeprom(INPUT2_REMDEV_ADDR,&temp,2);
	temp = get_input_rem_dev(2);
	Write_Eeprom(INPUT3_REMDEV_ADDR,&temp,2);

	temp = get_input_rem_port(0);
	Write_Eeprom(INPUT1_REMPORT_ADDR,&temp,2);
	temp = get_input_rem_port(1);
	Write_Eeprom(INPUT2_REMPORT_ADDR,&temp,2);
	temp = get_input_rem_port(2);
	Write_Eeprom(INPUT3_REMPORT_ADDR,&temp,2);


	temp = get_tlp_event_state(0);
	Write_Eeprom(TLP_EVENT1_STATE_ADDR,&temp,2);
	temp = get_tlp_event_state(1);
	Write_Eeprom(TLP_EVENT2_STATE_ADDR,&temp,2);


	temp = get_tlp_event_inverse(0);
	Write_Eeprom(TLP_EVENT1_INVERSE_ADDR,&temp,2);
	temp = get_tlp_event_inverse(1);
	Write_Eeprom(TLP_EVENT2_INVERSE_ADDR,&temp,2);

	temp = get_tlp_event_rem_dev(0);
	Write_Eeprom(TLP_EVENT1_REMDEV_ADDR,&temp,2);
	temp = get_tlp_event_rem_dev(1);
	Write_Eeprom(TLP_EVENT2_REMDEV_ADDR,&temp,2);


	temp = get_tlp_event_rem_port(0);
	Write_Eeprom(TLP_EVENT1_REMPORT_ADDR,&temp,2);
	temp = get_tlp_event_rem_port(1);
	Write_Eeprom(TLP_EVENT2_REMPORT_ADDR,&temp,2);


	temp = get_tlp_remdev_valid(0);
	Write_Eeprom(TLP1_VALID,&temp,2);
	temp = get_tlp_remdev_valid(1);
	Write_Eeprom(TLP2_VALID,&temp,2);
	temp = get_tlp_remdev_valid(2);
	Write_Eeprom(TLP3_VALID,&temp,2);
	temp = get_tlp_remdev_valid(3);
	Write_Eeprom(TLP4_VALID,&temp,2);

	temp = get_tlp_remdev_type(0);
	Write_Eeprom(TLP1_TYPE,&temp,2);
	temp = get_tlp_remdev_type(1);
	Write_Eeprom(TLP2_TYPE,&temp,2);
	temp = get_tlp_remdev_type(2);
	Write_Eeprom(TLP3_TYPE,&temp,2);
	temp = get_tlp_remdev_type(3);
	Write_Eeprom(TLP4_TYPE,&temp,2);

	get_tlp_remdev_name(0,(char *)tmp);
	Write_Eeprom(TLP1_DESCR,tmp,64);
	get_tlp_remdev_name(1,(char *)tmp);
	Write_Eeprom(TLP2_DESCR,tmp,64);
	get_tlp_remdev_name(2,(char *)tmp);
	Write_Eeprom(TLP3_DESCR,tmp,64);
	get_tlp_remdev_name(3,(char *)tmp);
	Write_Eeprom(TLP4_DESCR,tmp,64);

	get_tlp_remdev_ip(0,&ip);
	Write_Eeprom(TLP1_IP,&ip,4);
	get_tlp_remdev_ip(1,&ip);
	Write_Eeprom(TLP2_IP,&ip,4);
	get_tlp_remdev_ip(2,&ip);
	Write_Eeprom(TLP3_IP,&ip,4);
	get_tlp_remdev_ip(3,&ip);
	Write_Eeprom(TLP4_IP,&ip,4);

	get_tlp_remdev_mask(0,&ip);
	Write_Eeprom(TLP1_MASK,&ip,4);
	get_tlp_remdev_mask(1,&ip);
	Write_Eeprom(TLP2_MASK,&ip,4);
	get_tlp_remdev_mask(2,&ip);
	Write_Eeprom(TLP3_MASK,&ip,4);
	get_tlp_remdev_mask(3,&ip);
	Write_Eeprom(TLP4_MASK,&ip,4);

	get_tlp_remdev_gate(0,&ip);
	Write_Eeprom(TLP1_GATE,&ip,4);
	get_tlp_remdev_gate(0,&ip);
	Write_Eeprom(TLP1_GATE,&ip,4);
	get_tlp_remdev_gate(0,&ip);
	Write_Eeprom(TLP1_GATE,&ip,4);
	get_tlp_remdev_gate(0,&ip);
	Write_Eeprom(TLP1_GATE,&ip,4);

	temp = get_lldp_state();
	Write_Eeprom(LLDP_STATE_ADDR,&temp,2);
	temp = get_lldp_transmit_interval();
	Write_Eeprom(LLDP_TX_INT_ADDR,&temp,2);
	temp = get_lldp_hold_multiplier();
	Write_Eeprom(LLDP_HOLD_TIME_ADDR,&temp,2);

	temp = get_lldp_port_state(0);
	Write_Eeprom(LLDP_PORT1_STATE_ADDR,&temp,2);
	temp = get_lldp_port_state(1);
	Write_Eeprom(LLDP_PORT2_STATE_ADDR,&temp,2);
	temp = get_lldp_port_state(2);
	Write_Eeprom(LLDP_PORT3_STATE_ADDR,&temp,2);
	temp = get_lldp_port_state(3);
	Write_Eeprom(LLDP_PORT4_STATE_ADDR,&temp,2);
	temp = get_lldp_port_state(4);
	Write_Eeprom(LLDP_PORT5_STATE_ADDR,&temp,2);
	temp = get_lldp_port_state(5);
	Write_Eeprom(LLDP_PORT6_STATE_ADDR,&temp,2);
	temp = get_lldp_port_state(6);
	Write_Eeprom(LLDP_PORT7_STATE_ADDR,&temp,2);
	temp = get_lldp_port_state(7);
	Write_Eeprom(LLDP_PORT8_STATE_ADDR,&temp,2);
	temp = get_lldp_port_state(8);
	Write_Eeprom(LLDP_PORT9_STATE_ADDR,&temp,2);
	temp = get_lldp_port_state(9);
	Write_Eeprom(LLDP_PORT10_STATE_ADDR,&temp,2);
	temp = get_lldp_port_state(10);
	Write_Eeprom(LLDP_PORT11_STATE_ADDR,&temp,2);
	temp = get_lldp_port_state(11);
	Write_Eeprom(LLDP_PORT12_STATE_ADDR,&temp,2);
	temp = get_lldp_port_state(12);
	Write_Eeprom(LLDP_PORT13_STATE_ADDR,&temp,2);
	temp = get_lldp_port_state(13);
	Write_Eeprom(LLDP_PORT14_STATE_ADDR,&temp,2);
	temp = get_lldp_port_state(14);
	Write_Eeprom(LLDP_PORT15_STATE_ADDR,&temp,2);
	temp = get_lldp_port_state(15);
	Write_Eeprom(LLDP_PORT16_STATE_ADDR,&temp,2);




	Verify_Flash();
	set_poe_init(0);//изменились настройки poe
	return 0;
}


#endif
