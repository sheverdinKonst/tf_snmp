#include <string.h>
#include "eth_LWIP.h"
#include "stm32f4x7_eth.h"
#include "../flash/spiflash.h"
#include "../deffines.h"
///#include <net/uip/uipopt.h>
#include "../uip/uip_arp.h"
#include "board.h"
#include "SMIApi.h"

//#include "fifo.h"
//#include "usb.h"

#define USE_UIP_TCP

uint8_t IDbuff[6]={'C','O','N','F','I','G'};
extern uint8_t MyIP[4];
extern uint8_t MyPORT[2];
extern uint8_t dev_addr[6];
extern uint16_t LedPeriod;
//extern struct arp_entry arp_table[UIP_ARPTAB_SIZE];

uint16_t  TftpSeqNum=0;
uint8_t ENET_TEMP[256];
uint16_t IDENTIFICATION;


//--------------------------------------------------------------------
uint8_t ARP_req(uint8_t *ENET,uint16_t *ln,uint8_t *Tpa){
  ENET_frame_t *ENETframe;
  ARP_t *ARPframe;
  uint8_t j;  //i,


	ENETframe=(ENET_frame_t *)ENET;
	for(j=0;j<6;j++)
		ENETframe->THA[j]=0xFF;
	for(j=0;j<6;j++)
		ENETframe->SHA[j]=dev_addr[j];

#if DSA_TAG_ENABLE

	if(get_marvell_id() == DEV_98DX316){
		ENETframe->DSA_TAG[0]=0x41;
		ENETframe->DSA_TAG[1]=0x00;
		ENETframe->DSA_TAG[2]=0x60;
		ENETframe->DSA_TAG[3]=0x00;
	}
	else{
		//c0000000
		ENETframe->DSA_TAG[0]=0xC0;
		ENETframe->DSA_TAG[1]=0;
		ENETframe->DSA_TAG[2]=0;
		ENETframe->DSA_TAG[3]=0;
	}
#endif

	ENETframe->TYPE_h=0x08;
	ENETframe->TYPE_l=0x06;
	
	ARPframe=(ARP_t *)&ENETframe->PAYLOAD[0];
	ARPframe->HTYPE_h=0x00;
	ARPframe->HTYPE_l=0x01;
	ARPframe->PTYPE_h=0x08;
	ARPframe->PTYPE_l=00;
    ARPframe->HLEN=0x06;
	ARPframe->PLEN=0x04;
	ARPframe->OPER_h=0x00;
	ARPframe->OPER_l=0x01;	

	for(j=0;j<6;j++)
	  ARPframe->SHA[j]=dev_addr[j];

//	uip_draddr
//	uip_netmask

	ARPframe->SPA[0]=(u8)(uip_hostaddr[0]);
	ARPframe->SPA[1]=(u8)(uip_hostaddr[0]>>8);
	ARPframe->SPA[2]=(u8)(uip_hostaddr[1]);
	ARPframe->SPA[3]=(u8)(uip_hostaddr[1]>>8);


	for(j=0;j<6;j++)
	  ARPframe->THA[j]=0xff;	
	for(j=0;j<4;j++)
	  ARPframe->TPA[j]=Tpa[j];	
#ifdef DSA_TAG_ENABLE
	*ln=42+4;
#else
	*ln=42;
#endif

  return 1;
}


/***************************************************************************
Function: ip_sum_calc()
Description: Calculate IP checksum
****************************************************************************/
void ip_sum_calc(u8 *buff)
{
	uint16_t word16,i;
	uint32_t sum;
	
	//initialize sum to zero
	sum=0;
	buff[IpHeaderChecksum]=0;
	buff[IpHeaderChecksum+1]=0;
	
	for (i=0;i<20;i=i+2){
		word16 =(((	uint16_t)buff[Version+i]<<8)&0xFF00)+((	uint16_t)buff[Version+i+1]&0xFF);
		sum += (unsigned long)word16;
	}	
	
	// keep only the last 16 bits of the 32 bit calculated sum and add the carries
    while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);
	
	// Take the one's complement of sum
	sum = ~sum;
	buff[IpHeaderChecksum]=sum>>8;
	buff[IpHeaderChecksum+1]=sum;
}

/***************************************************************************
Function: icmp_sum_calc()
Description: Calculate ICMP checksum
****************************************************************************/
void icmp_sum_calc(u8 *ENET)
{
	uint16_t word16,i;
	uint32_t sum;

	//initialize sum to zero
	sum=0;
	ENET[ICMP_ChSum]=0;
	ENET[ICMP_ChSum+1]=0;

	for (i=0;i<40;i=i+2){
		word16 =(((	uint16_t)ENET[ICMP_Type+i]<<8)&0xFF00)+((uint16_t)ENET[ICMP_Type+i+1]&0xFF);
		sum += (unsigned long)word16;
	}

	// keep only the last 16 bits of the 32 bit calculated sum and add the carries
    while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);

	// Take the one's complement of sum
	sum = ~sum;
	ENET[ICMP_ChSum]=sum>>8;
	ENET[ICMP_ChSum+1]=sum;
}

/***************************************************************************
Function: udp_sum_calc()
Description: Calculate UDP checksum
****************************************************************************/
void udp_sum_calc(u8 buff[])
{
	uint16_t len_udp;
	uint16_t prot_udp=17;
	uint16_t padd=0;
	uint16_t word16,i;
	uint32_t sum;
	
    len_udp=(((	uint16_t)buff[UdpTotalLength]<<8)&0xFF00)+((uint16_t)buff[UdpTotalLength+1]&0xFF);
	// Find out if the length of data is even or odd number. If odd,
	// add a padding byte = 0 at the end of packet
	if ((len_udp&1)==1){
		padd=1;
		buff[len_udp]=0;
	}
	
	//initialize sum to zero
	sum=0;
	buff[UdpHeaderChecksum]=0;
	buff[UdpHeaderChecksum+1]=0;
	// make 16 bit words out of every two adjacent 8 bit words and 
	// calculate the sum of all 16 vit words
	for (i=0;i<len_udp+padd;i=i+2){
		word16 =(((u16)buff[SourcePort+i]<<8)&0xFF00)+((u16)buff[SourcePort+i+1]&0xFF);
		sum = sum + (unsigned long)word16;
	}	
	
	// add the UDP pseudo header which contains the IP source and destinationn addresses
	for (i=0;i<8;i=i+2){
		word16 =(((	uint16_t)buff[SourceIpAddress+i]<<8)&0xFF00)+((	uint16_t)buff[SourceIpAddress+i+1]&0xFF);
		sum=sum+word16;	
	}
	// the protocol number and the length of the UDP packet
	sum = sum + prot_udp + len_udp;

	// keep only the last 16 bits of the 32 bit calculated sum and add the carries
    while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);
		
	// Take the one's complement of sum
	sum = ~sum; 
	buff[UdpHeaderChecksum]=sum>>8;
	buff[UdpHeaderChecksum+1]=sum;
}


//-------------------------------------------------------------------------
void TFTP_server(uint8_t *ENET,uint16_t *ln){
  uint16_t len,tmp;
  uint16_t Length;
  static uint8_t Number;

  if (ENET[TftpOpcodH]==0){
     switch(ENET[TftpOpcodL]){
        case RRQ_pkt: 
            ENET[TftpOpcodL]=ERR_pkt;
            ENET[TftpOpcodL+1]=0;
            ENET[TftpOpcodL+2]=ACC_DEN;	      
            *ln=4; 
	    break;
	    
        case WRQ_pkt: 
	    ENET[TftpOpcodL]=ACK_pkt;
	    ENET[TftpTransID]=0;
	    ENET[TftpTransID+1]=0;
            *ln=4;	
	    TftpSeqNum=1;	    
	    break;
	    
        case DATA_pkt: 
	    tmp=(uint16_t)ENET[TftpTransID]<<8|(uint16_t)ENET[TftpTransID+1]; 
	    if (TftpSeqNum==tmp) {
	      ENET[TftpOpcodL]=ACK_pkt;
#ifndef USE_UIP_TCP
	      len=((uint16_t )ENET[UdpTotalLength]<<8)|(uint16_t )ENET[UdpTotalLength+1];
	      len -=12;
#else /*USE_UIP_TCP*/
	      len =*ln-4;
#endif /*USE_UIP_TCP*/


	     if(len==512){Number++;}
	      else{ //���� ������ ��������� �����
	      Length=Number*512+len;
	      //����� ���� � ����� � ������ ��������
	      //Write_Eeprom(LenAddrE,&Length,2);
	      }

	      //TFTP2Flash(ENET,len);
	      TftpSeqNum++;
	    }
	    else{
	       if((TftpSeqNum-1)==tmp){
	    	  ENET[TftpOpcodL]=ACK_pkt;
		     *ln=4;
	          break;      
	       }
	       ENET[TftpOpcodL]=ERR_pkt;
	       ENET[TftpOpcodL+1]=0;
           ENET[TftpOpcodL+2]=WRONG_ID;
	    }
	    *ln=4;	
	    break;
	    
        case ACK_pkt: 
            ENET[TftpOpcodL]=ERR_pkt;
            ENET[TftpOpcodL+1]=0;
            ENET[TftpOpcodL+2]=UNCORR_OPCODE;	      
            *ln=4; 	
	    break;
	    
        case ERR_pkt: 
	    return;

        default:
            ENET[TftpOpcodL]=ERR_pkt;
            ENET[TftpOpcodL+1]=0;
            ENET[TftpOpcodL+2]=UNCORR_OPCODE;	      
           *ln=4; 	
     }
  }
  else{
    ENET[TftpOpcodL]=ERR_pkt;
    ENET[TftpOpcodL+1]=0;
    ENET[TftpOpcodL+2]=UNCORR_OPCODE;	      
    *ln=4; 
  }
#ifndef USE_UIP_TCP
  HeadMaker(ENET,ln);
  ETH_HandleTxPkt(ENET, *ln);
#endif
}
//#ifndef USE_UIP_TCP
//-------------------------------------------------------------------------
void ICMPAns(uint8_t *ENET,uint16_t *ln){
    uint8_t t;
    uint16_t l;

    for (l=0;l<6;l++){
 	 t=ENET[ENET_THA0+l];
 	 ENET[ENET_THA0+l]=ENET[ENET_SHA0+l];
 	 ENET[ENET_SHA0+l]=t;
    }


    ENET[Identification]=IDENTIFICATION>>8;
    ENET[Identification+1]=IDENTIFICATION;
    IDENTIFICATION++;
//    ENET[Offset]=0;
//    ENET[Offset+1]=0;
    ENET[20+4]=0;
    ENET[20+4+1]=0;

    ENET[TTL]=64;

    l=60;
    ENET[IpTotalLength]=l>>8;
    ENET[IpTotalLength+1]=l;

    for (l=0;l<4;l++){
  	 t=ENET[DestinationIpAddress+l];
  	 ENET[DestinationIpAddress+l]=ENET[SourceIpAddress+l];
  	 ENET[SourceIpAddress+l]=t;
     }

    ENET[ICMP_Type]=0;


	ENET[ICMP_ChSum]=0;
	ENET[ICMP_ChSum+1]=0;
    ip_sum_calc(ENET);
    icmp_sum_calc(ENET);
}


#if 0
uint8_t ICMPReq(uint8_t *ENET,uint8_t *IP,uint16_t *ln){
    uint16_t l;
    uint8_t mac1[6];
    uint8_t IPTemp[4]={255,255,255,255};
    uint8_t NoOneSubnetwork=0;
    static uint16_t SeqNum;
    uint8_t i;

    //xUsb_Print("ICMPReq IP %d.%d.%d.%d\r\n",IP[0],IP[1],IP[2],IP[3]);
    l=0;

	for(i=0;i<2;i++){
		if(((IP[i*2+1]<<8 | IP[i*2])  & uip_netmask[i])==(uip_hostaddr[i] & uip_netmask[i]))
			NoOneSubnetwork=0;
		else
		{
			//xUsb_Print("No One Subnetwork\r\n");
			NoOneSubnetwork=1;
			break;
		}
	}

	if(NoOneSubnetwork==1)
	{
		memcpy(IPTemp,uip_draddr,4);
	}
	else
	{
		memcpy(IPTemp,IP,4);
	}

	//xUsb_Print("IPTemp IP %d.%d.%d.%d\r\n",IPTemp[0],IPTemp[1],IPTemp[2],IPTemp[3]);

	if((IPTemp[0]!=0xFF)&&(IPTemp[1]!=0xFF)&&(IPTemp[2]!=0xFF)&&(IPTemp[3]!=0xFF))
		uip_search_mac(IPTemp,mac1);
	else{
		//xUsb_Print("no valid IP\r\n");
		return 1;
	}

	if((mac1[0]==255)&&(mac1[1]==255)&&(mac1[2]==255)&&(mac1[3]==255)&&(mac1[4]==255)&&(mac1[5]==255)){
		  //xUsb_Print("no find mac\r\n");
		return 1;
	}

	for (l=0;l<6;l++){
		ENET[ENET_SHA0+l]=dev_addr[l];//source
		ENET[ENET_THA0+l]=mac1[l];//destination
	}

#ifdef DSA_TAG_ENABLE

	if(get_marvell_id() == DEV_98DX316){
		//20000000
		ENET[12]=0x40;
		ENET[13]=0;
		ENET[14]=0;
		ENET[15]=0;
	}
	else{
		//c0000000
		ENET[12]=0xC0;
		ENET[13]=0;
		ENET[14]=0;
		ENET[15]=0;
	}
#endif

 	ENET[ENET_TYPE_h]=0x08;
	ENET[ENET_TYPE_l]=0x00;

	ENET[Version]=0x45;
	ENET[TypeOfService]=0;
	ENET[Identification]=IDENTIFICATION>>8;
	ENET[Identification+1]=IDENTIFICATION;
	IDENTIFICATION++;
	ENET[Offset]=0;
	ENET[Offset+1]=0;
	ENET[TTL]=128;
	ENET[Protocol]=1;//ICMP

    l=60;

    ENET[IpTotalLength]=l>>8;
    ENET[IpTotalLength+1]=l;

    for (l=0;l<4;l++){
    	ENET[DestinationIpAddress+l]=IP[l];
    }

    ENET[SourceIpAddress]=(u8)(uip_hostaddr[0]);
    ENET[SourceIpAddress+1]=(u8)(uip_hostaddr[0]>>8);
    ENET[SourceIpAddress+2]=(u8)(uip_hostaddr[1]);
    ENET[SourceIpAddress+3]=(u8)(uip_hostaddr[1]>>8);


    ENET[ICMP_Type]=0x08;//request
    ENET[ICMP_Code]=0;

	ENET[ICMP_ChSum]=0;
	ENET[ICMP_ChSum+1]=0;

	ENET[ICMP_Id]=0;
	ENET[ICMP_Id+1]=1;

	ENET[ICMP_SeqNum]=SeqNum>>8;
	ENET[ICMP_SeqNum+1]=SeqNum;
	SeqNum++;

	for(uint8_t i=0;i<32;i++){
	if(i<23)
		ENET[ICMP_Data+i]=0x61+i;
	else
		ENET[ICMP_Data+i]=0x61+i-23;
	}

	ENET[ICMP_ChSum]=0;

    ip_sum_calc(ENET);
    icmp_sum_calc(ENET);

    return 0;
}
#endif
