#include <string.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "stm32f4x7_eth.h"
#include "stm32f4xx_iwdg.h"

#include "../deffines.h"

//#include "stm32f4xx_lib.h"
//#include "usb.h"
//#include "eth_LWIP.h"

#include "../uip/uip.h"
#include "../uip/timer.h"
#include "../uip/uip_arp.h"
#include "eeprom.h"

#include "command.h" 
#include "board.h"
#include "SMIApi.h"
#include "Salsa2Regs.h"
#include "salsa2.h"
#include "settings.h"

extern uint8_t MyPORT[2];
extern uint8_t IDbuff[6];
//extern char data_PSW2G_settings_backup_bak[];
extern const uint32_t image_version[1];
extern u8 MyIP[4];
extern u8 dev_addr[6];

uint8_t RemIpAdd[4];
uint8_t  AppBuffer[80];
void ComandProcessing( uint8_t  *buffer,uint16_t len);


struct uip_udp_conn *command_conn;
struct uip_udp_conn *command_tftp_conn;

void command_init(void){
    command_conn = uip_udp_new(NULL, 0);
    if (command_conn!=NULL) uip_udp_bind(command_conn, (MyPORT[0]|(MyPORT[1]<<8)));
    
    //command_tftp_conn = uip_udp_new(NULL, 0);
    //if (command_tftp_conn!=NULL) uip_udp_bind(command_tftp_conn, HTONS(69));
}

static void MACSETProcessing( uint8_t  *buffer,uint16_t len){
	if(buffer[11]=='w'){//если запрос на подключение
uint8_t mac[6];
		/*проверяем ключ*/
		if((buffer[18]=='K')&&(buffer[19]=='r')&&(buffer[20]=='2')){
			mac[0]=buffer[12];
			mac[1]=buffer[13];
			mac[2]=buffer[14];
			mac[3]=buffer[15];
			mac[4]=buffer[16];
			mac[5]=buffer[17];

			set_net_def_mac(mac);
			set_net_mac(mac);
			settings_save();

		// отправляем обратно в качестве подтверждения
		buffer[11]='r';

		uip_udp_send (21);
	 }

	}
}


void ConnectProcessing( uint8_t  *buffer,uint16_t len){

	if(buffer[11]=='2'){//если запрос на подключение
		buffer[10]='C';
		buffer[11]='1';
		buffer[12]=(uint8_t)(image_version[0]>>24);// отправляем версию прошивки
		buffer[13]=(uint8_t)(image_version[0]>>16);
		buffer[14]=(uint8_t)(image_version[0]>>8);
		buffer[15]=(uint8_t)(image_version[0]);
		buffer[16]=dev_addr[0];// и МАС адресс
		buffer[17]=dev_addr[1];
		buffer[18]=dev_addr[2];
		buffer[19]=dev_addr[3];
		buffer[20]=dev_addr[4];
		buffer[21]=dev_addr[5];
		// ну и IP заодним
		buffer[22]=MyIP[0];
		buffer[23]=MyIP[1];
		buffer[24]=MyIP[2];
		buffer[25]=MyIP[3];
		// и порт
		buffer[26]=MyPORT[0];
		buffer[27]=MyPORT[1];

		uip_udp_send (30);
	}
}

void ComandProcessing(u8 *buffer,u16 len){
	 switch (buffer[10]){
		 case 'C':ConnectProcessing(buffer,len);break;
		 case 'm':MACSETProcessing(buffer,len);break;//установка МАС
		 //case 'n':ExtNameSet(buffer,len);break; // установка поддержки имени сторонего производителя
	 }
}

struct uip_udp_conn *mac_set_conn;
void cmd_appcall(void){
  unsigned char *appdata;

  if(uip_hostaddr[0] == 0 && uip_hostaddr[1]==0)
	  return;


    if (uip_newdata()){
      appdata = uip_appdata;
      if (uip_datalen()>10){
			if (!memcmp(IDbuff, appdata, sizeof(IDbuff))){
				ComandProcessing(appdata,uip_datalen());
			}
      }
    }
}




void salsa_command(char *Buf){
char temp[256];
u32 addr;
u32 data;
u8 phy_port;
u8 phy_addr;
u8 phy_page;
u8 phy_reg;
u16 phy_data;


	//Salsa 2 operation
	if(Buf[0] == 's'){
		//read operation
		if(Buf[1] == 'r'){
			strncpy(temp,(char *)Buf+2,8);
			temp[8] = 0;
			addr = strtoul(temp,NULL,16);
			data = Salsa2_ReadReg(addr);
			printf("sr%08lx%08lx\r\n",addr,data);
		}
		//write operation
		if(Buf[1] == 'w'){
			strncpy(temp,(char *)Buf+2,8);
			temp[8] = 0;
			addr = strtoul(temp,NULL,16);
			strncpy(temp,(char *)Buf+10,8);
			temp[8] = 0;
			data = strtoul(temp,NULL,16);

			Salsa2_WriteReg(addr,data);
			printf("sw%08lx%08lx\r\n",addr,data);
		}

		//start operation
		if(Buf[1] == 's'){
			salsa2_config_processing();
			config_cpu_port();
			printf("ss\r\n",addr,data);
		}
	}

	//phy operation
	if(Buf[0] == 'p'){
		//read operation
		if(Buf[1]=='r'){
			strncpy(temp,(char *)Buf+2,2);
			temp[2] = 0;
			phy_port = (u8)strtoul(temp,NULL,16);

			strncpy(temp,(char *)Buf+4,2);
			temp[2] = 0;
			phy_addr = (u8)strtoul(temp,NULL,16);

			strncpy(temp,(char *)Buf+6,2);
			temp[2] = 0;
			phy_page = (u8)strtoul(temp,NULL,16);

			strncpy(temp,(char *)Buf+8,2);
			temp[2] = 0;
			phy_reg = (u8)strtoul(temp,NULL,16);

			Salsa2_WritePhyReg(phy_port,phy_addr,22,phy_page);
			phy_data = Salsa2_ReadPhyReg(phy_port,phy_addr,phy_reg);
			printf("pr%02x%02x%02x%02x%04x\r\n",phy_port,phy_addr,phy_page,phy_reg,phy_data);
		}
		//write operation
		if(Buf[1]=='w'){
			strncpy(temp,(char *)Buf+2,2);
			temp[2] = 0;
			phy_port = (u8)strtoul(temp,NULL,16);

			strncpy(temp,(char *)Buf+4,2);
			temp[2] = 0;
			phy_addr = (u8)strtoul(temp,NULL,16);

			strncpy(temp,(char *)Buf+6,2);
			temp[2] = 0;
			phy_page = (u8)strtoul(temp,NULL,16);

			strncpy(temp,(char *)Buf+8,2);
			temp[2] = 0;
			phy_reg = (u8)strtoul(temp,NULL,16);

			strncpy(temp,(char *)Buf+10,4);
			temp[4] = 0;
			phy_data = (u16)strtoul(temp,NULL,16);

			Salsa2_WritePhyReg(phy_port,phy_addr,22,phy_page);
			Salsa2_WritePhyReg(phy_port,phy_addr,phy_reg,phy_data);
			printf("pw%02x%02x%02x%02x%04x\r\n",phy_port,phy_addr,phy_page,phy_reg,phy_data);
		}
		//dump
		if(Buf[1]=='d'){
			strncpy(temp,(char *)Buf+2,2);
			temp[2] = 0;
			phy_port = (u8)strtoul(temp,NULL,16);

			strncpy(temp,(char *)Buf+4,2);
			temp[2] = 0;
			phy_addr = (u8)strtoul(temp,NULL,16);

			salsa2_phy_reg_dump(phy_port,phy_addr);
		}
	}

	//INTn State
	if(Buf[0] == 'i' || Buf[1] == 'n' ){
		printf("in%d\r\n",GPIO_ReadInputDataBit(GPIOE,LINE_SW_INT_PIN));
	}
}



void salsa2_phy_reg_dump(u8 phy_port,u8 phy_addr){
	//0-7
	for(u8 i=0;i<8;i++){
		Salsa2_WritePhyReg(phy_port,phy_addr,22,i);
		for(u8 j=0;j<32;j++){
			printf("P%02d R%02d 0x%04X\r\n",i,j,Salsa2_ReadPhyReg(phy_port,phy_addr,j));
			vTaskDelay(100*MSEC);
		}
	}
	//18
	Salsa2_WritePhyReg(phy_port,phy_addr,22,18);
	for(u8 j=0;j<32;j++){
		printf("P18 R%02d 0x%04X\r\n",j,Salsa2_ReadPhyReg(phy_port,phy_addr,j));
		vTaskDelay(100*MSEC);
	}
	printf("end\r\n");
}
