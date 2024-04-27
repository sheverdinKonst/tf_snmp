/*
 * eth_LWIP.h
 *
 *  Created on: 21.07.2011
 *      Author: Belyaew alexandr
 */

#ifndef ETH_LWIP_H_
#define ETH_LWIP_H_


#include <stdio.h>
//#include "stm32f10x_lib.h"

#include "FreeRTOS.h"
#include "task.h"

#include "croutine.h"
#include "semphr.h"
//#include "flash/blbx.h"

//#include "usb.h"
//#include "i2c.h"
//#include "perh_init.h"
#include "stm32f4x7_eth.h"

#include "../deffines.h"

#define ArpCashSize 32
typedef struct
{
	uint8_t	TimeToLive;
	uint8_t	IpAddr[4];
	uint8_t	MacAddr[6];
} ARP_cash_t;

typedef struct
{
	uint8_t	THA[6];//transmit mac addres
	uint8_t	SHA[6];//source mac addres
#if DSA_TAG_ENABLE
	uint8_t DSA_TAG[4];
#endif
	uint8_t	TYPE_h;
	uint8_t	TYPE_l;
	uint8_t	PAYLOAD[1518];
} ENET_frame_t;

typedef struct
{   uint8_t	HTYPE_h;
    uint8_t	HTYPE_l;
	uint8_t	PTYPE_h;
    uint8_t	PTYPE_l;
	uint8_t	HLEN;
	uint8_t PLEN;
	uint8_t OPER_h;
	uint8_t OPER_l;
	uint8_t	SHA[6];//source mac addres
	uint8_t SPA[4];//source IP addres
	uint8_t	THA[6];//transmit mac addres
	uint8_t TPA[4];//transmit IP addres
} ARP_t;

// IP-�����
typedef struct
{
    uint8_t ver_head_len; // ������ � ����� ��������� =0x45
    uint8_t tos; //��� �������
    uint16_t total_len; //����� ����� ������
    uint16_t fragment_id; //������������� ���������
    uint16_t flags_framgent_offset; //�������� ���������
    uint8_t ttl; //TTL
    uint8_t protocol; //��� ���������
    uint16_t cksum; //����������� ����� ���������
    uint8_t from_addr[4]; //IP-����� �����������
    uint8_t to_addr[4]; //IP-����� ����������
    uint8_t data[4];
} ip_packet_t;

//ENET
#define ENET_THA0    0
#define ENET_THA1    1
#define ENET_THA2    2
#define ENET_THA3    3
#define ENET_THA4    4
#define ENET_THA5    5
#define ENET_SHA0    6
#define ENET_SHA1    7
#define ENET_SHA2    8
#define ENET_SHA3    9
#define ENET_SHA4    10
#define ENET_SHA5    11

#if DSA_TAG_ENABLE
	#define ENET_TYPE_h  12+4
	#define ENET_TYPE_l  13+4
	#define ENET_PAYLOAD 14+4
	//ARP 0x806
	#define ARP_HTYPE_h 14+4
	#define ARP_HTYPE_l 15+4
	#define ARP_PTYPE_h 16+4
	#define ARP_PTYPE_l 17+4
	#define ARP_HLEN    18+4
	#define ARP_PLEN    19+4
	#define ARP_OPER_h  20+4
	#define ARP_OPER_l  21+4

	#define ARP_SHA0    22+4
	#define ARP_SHA1    23+4
	#define ARP_SHA2    24+4
	#define ARP_SHA3    25+4
	#define ARP_SHA4    26+4
	#define ARP_SHA5    27+4
	#define ARP_SPA0    28+4
	#define ARP_SPA1    29+4
	#define ARP_SPA2    30+4
	#define ARP_SPA3    31+4

	#define ARP_THA0    32+4
	#define ARP_THA1    33+4
	#define ARP_THA2    34+4
	#define ARP_THA3    35+4
	#define ARP_THA4    36+4
	#define ARP_THA5    37+4
	#define ARP_TPA0    38+4
	#define ARP_TPA1    39+4
	#define ARP_TPA2    40+4
	#define ARP_TPA3    41+4
	//IP v4 0x800
	#define Version              14+4  //0
	#define TypeOfService        15+4  //1
	#define IpTotalLength        16+4  //2
	#define Identification       18+4  //4
	//#define Offset               20+4  //6
	#define TTL                  22+4  //8
	#define Protocol             23+4  //9
	#define IpHeaderChecksum     24+4  //10
	#define SourceIpAddress      26+4  //12
	#define DestinationIpAddress 30+4  //16
	//UDP
	#define SourcePort           34+4  //20
	#define DestinationPort      36+4
	#define UdpTotalLength       38+4
	#define UdpHeaderChecksum    40+4
#else
	#define ENET_TYPE_h  12
	#define ENET_TYPE_l  13
	#define ENET_PAYLOAD 14
	//ARP 0x806
	#define ARP_HTYPE_h 14
	#define ARP_HTYPE_l 15
	#define ARP_PTYPE_h 16
	#define ARP_PTYPE_l 17
	#define ARP_HLEN    18
	#define ARP_PLEN    19
	#define ARP_OPER_h  20
	#define ARP_OPER_l  21

	#define ARP_SHA0    22
	#define ARP_SHA1    23
	#define ARP_SHA2    24
	#define ARP_SHA3    25
	#define ARP_SHA4    26
	#define ARP_SHA5    27
	#define ARP_SPA0    28
	#define ARP_SPA1    29
	#define ARP_SPA2    30
	#define ARP_SPA3    31

	#define ARP_THA0    32
	#define ARP_THA1    33
	#define ARP_THA2    34
	#define ARP_THA3    35
	#define ARP_THA4    36
	#define ARP_THA5    37
	#define ARP_TPA0    38
	#define ARP_TPA1    39
	#define ARP_TPA2    40
	#define ARP_TPA3    41
	//IP v4 0x800
	#define Version              14  //0
	#define TypeOfService        15  //1
	#define IpTotalLength        16  //2
	#define Identification       18  //4
	#define Offset               20  //6
	#define TTL                  22  //8
	#define Protocol             23  //9
	#define IpHeaderChecksum     24  //10
	#define SourceIpAddress      26  //12
	#define DestinationIpAddress 30  //16
	//UDP
	#define SourcePort           34  //20
	#define DestinationPort      36
	#define UdpTotalLength       38
	#define UdpHeaderChecksum    40
#endif

#ifndef USE_UIP_TCP
#define UdpPayload           (42)
#define TftpOpcodH           (42)
#define TftpOpcodL           (43)
#define TftpTransID          (44)
#define TftpData             (46)
//TFTP
#define Cmd_App              42
#define OPCOD_App            43
#define DATA_App             44
#else /*USE_UIP_TCP*/
#define UdpPayload           (0)
#define TftpOpcodH           (0)
#define TftpOpcodL           (1)
#define TftpTransID          (2)
#define TftpData             (4)

#define Cmd_App              0
#define OPCOD_App            1
#define DATA_App             2
#endif /*USE_UIP_TCP*/

#define UdpCommand           52// ��� �������� ������

//ICMP
#if DSA_TAG_ENABLE
	#define ICMP_Type 34+4
	#define ICMP_Code 35+4
	#define ICMP_ChSum 36+4//-37
	#define ICMP_Id 38+4//-39
	#define ICMP_SeqNum 40+4//-41
	#define ICMP_Data 42+4
#else
	#define ICMP_Type 34
	#define ICMP_Code 35
	#define ICMP_ChSum 36//-37
	#define ICMP_Id 38//-39
	#define ICMP_SeqNum 40//-41
	#define ICMP_Data 42
#endif
//TFTP----------------------------------------------------------------
#define RRQ_pkt  1  // � Read Request (������ �� ������ �����.
#define WRQ_pkt  2  // � Write Request (������ �� ������ �����.
#define DATA_pkt 3  // �Data ( ������, ������������ ����� TFTP.
#define ACK_pkt  4  // � Acknowledgment (������������� ������.
#define ERR_pkt  5  // � Error (������.

#define NO_CODE   0	//��� ������������� ����, ��. ����� ������
#define NO_FILE   1	//���� �� ������
#define ACC_DEN   2	//������ ��������
#define NO_SPACE  3	//���������� �������� ����� �� �����
#define UNCORR_OPCODE 4	//������������ TFTP-��������
#define WRONG_ID  5	//������������ Transfer ID
#define FILE_EXIST 6	//���� ��� ����������
#define NO_USER   7	//������������ �� ����������
#define WRONG_OPT 8	//������������ �����

/*
 ip         0     IP           # Internet protocol
icmp       1     ICMP         # Internet control message protocol
ggp        3     GGP          # Gateway-gateway protocol
tcp        6     TCP          # Transmission control protocol
egp        8     EGP          # Exterior gateway protocol
pup        12    PUP          # PARC universal packet protocol
udp        17    UDP          # User datagram protocol
hmp        20    HMP          # Host monitoring protocol
xns-idp    22    XNS-IDP      # Xerox NS IDP
rdp        27    RDP          # "reliable datagram" protocol
ipv6       41    IPv6         # Internet protocol IPv6
ipv6-route 43    IPv6-Route   # Routing header for IPv6
ipv6-frag  44    IPv6-Frag    # Fragment header for IPv6
esp        50    ESP          # Encapsulating security payload
ah         51    AH           # Authentication header
ipv6-icmp  58    IPv6-ICMP    # ICMP for IPv6
ipv6-nonxt 59    IPv6-NoNxt   # No next header for IPv6
ipv6-opts  60    IPv6-Opts    # Destination options for IPv6
rvd        66    RVD          # MIT remote virtual disk
*/
    
#define UDP_Prot             17
#define TCP_Prot             6
#define ICMP_Prot             1

#define Port_TFTP            69

#define SYSLOG_PORT		     514


uint8_t ToNet(uint8_t *ENET,uint8_t *AppBuff,uint16_t *len,uint8_t *DIPA,uint8_t *UDPP);
uint8_t  FromEnet(uint8_t *ENET,uint16_t *ln);
void ARP_cash_init(void);
void ARP_TimeToLive(void);
void Send_ARP_ans(uint8_t *ENET,uint16_t *ln);
uint8_t ARP_req(uint8_t *ENET,uint16_t *ln,uint8_t*Tpa);
void set_ARP_cash(uint8_t*HA,uint8_t*PA);
uint8_t get_ARP_cash(uint8_t*HA,uint8_t*PA);
void ip_sum_calc(u8 *buff);
void udp_sum_calc(u8 buff[]);
void TFTP_server(uint8_t *ENET,uint16_t *ln);
//uint8_t ICMPReq(uint8_t *ENET,uint8_t *IP,uint16_t *ln);
void icmp_sum_calc(u8 *ENET);
#endif /* ETH_LWIP_H_ */
