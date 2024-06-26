#ifndef SALSA2REGS_H_
#define SALSA2REGS_H_

#define SALSA2_MAX_CNT 	10//максимальное число повторений цикла ожидания

#define SALSA2_FDB_MAX	8192//размер таблицы коммутации
#define SALSA2_AGE_TIME	300//время жизни записи в таблице FDB

/*SMI Interface registers*/
#define SMI_RW_STATUS		0x1F
#define SMI_WRITE_ADDR_MSB	0x00
#define SMI_WRITE_ADDR_LSB	0x01
#define SMI_WRITE_DATA_MSB  0x02
#define SMI_WRITE_DATA_LSB	0x03
#define SMI_READ_ADDR_MSB	0x04
#define SMI_READ_ADDR_LSB	0x05
#define SMI_READ_DATA_MSB  	0x06
#define SMI_READ_DATA_LSB	0x07

//addresses of external PHY
#define PHY_ADDR_REG0		0x04004030 //(ports 0-5)
#define PHY_ADDR_REG1		0x04804030 //ports 6-11
#define PHY_ADDR_REG2		0x05004030 //ports 12-17
#define PHY_ADDR_REG3		0x05804030 //ports 18-23

#define SMI0_MNGT_REG		0x04004054  //ports 0-11
#define SMI1_MNGT_REG		0x05004054  //ports 12-23

#define PORT_STATUS_REG0	0x10000010 //port 0 status register

#define MIB_CNT_CTRL_REG0_P0	0x04004020//ports 0-5
#define MIB_CNT_CTRL_REG0_P1	0x04804020//ports 6-11
#define MIB_CNT_CTRL_REG0_P2	0x05004020//ports 12-17
#define MIB_CNT_CTRL_REG0_P3	0x05804020//ports 18-23

#define MIB_COUNTRERS_REG_P0	0x04010000//port0

//
#define SALSA2_RX_GOOD 			0x00
#define SALSA2_RX_BAD 			0x08
#define SALSA2_RX_GOOD_FRAMES	0x10
#define SALSA2_TX_MAC_ERROR		0x0C
#define SALSA2_RX_BAD_FRAMES	0x14
#define SALSA2_RX_BROADCAST		0x18
#define SALSA2_RX_MULTICAST		0x1C
#define SALSA2_64_FRAMES		0x20
#define SALSA2_65_127_FRAMES	0x24
#define SALSA2_127_255_FRAMES	0x28
#define SALSA2_256_511_FRAMES	0x2C
#define SALSA2_512_1023_FRAMES 	0x30
#define SALSA2_1024_MAX_FRAMES 	0x34
#define SALSA2_TX_GOOD 			0x38
#define SALSA2_TX_GOOD_FRAMES	0x40
#define SALSA2_MAC_COLLISION 	0x44
#define SALSA2_TX_MULTICAST		0x48
#define SALSA2_TX_BROADCAST		0x4C
#define SALSA2_RX_UNRECOGN_MAC	0x50
#define SALSA2_TX_FCR			0x54
#define SALSA2_RX_GOOD_FCR		0x58
#define SALSA2_RX_BAD_FCR		0x5C
#define SALSA2_RX_UNDERSIZE		0x60
#define SALSA2_RX_FRAGMENTS		0x64
#define SALSA2_RX_OVERSIZE		0x68
#define SALSA2_RX_JABBER		0x6C
#define SALSA2_RX_MAC_ERR		0x70
#define SALSA2_BAD_CRC			0x74
#define SALSA2_COLLISION		0x78
#define SALSA2_LATE_COLLISION	0x7C



#define MAC_TABLE_CONROL		0x06000000
#define MESSAGE_FROM_CPU_REG0	0x06000040
#define MESSAGE_FROM_CPU_REG1	0x06000044
#define MESSAGE_FROM_CPU_REG2	0x06000048
#define MESSAGE_FROM_CPU_REG3	0x0600004C


#define MAC_TABLE_ENTRY_W0		0x06100000
#define MAC_TABLE_ENTRY_W1		0x06100004
#define MAC_TABLE_ENTRY_W2		0x06100008

#define CPU_PORT_CTRL_REG		0x000000A0
#define CPU_PORT_STATUS_REG		0x000000A4
#define CPU_PORT_SA_MID_REG		0x000000AC
#define CPU_PORT_SA_HI_REG		0x000000B0
#define VLAN_ETHER_TYPE_REG		0x000000BC
#define CPU_PORT_ETHER_TYPE_REG	0x000000B4
#define CPU_PORT_GOOD_FRAMES_TX	0x00000060
#define CPU_PORT_MAC_ERR		0x00000064
#define CPU_PORT_GOOD_OCTETS_TX	0x00000068
#define CPU_PORT_GOOD_FRAMES_RX	0x00000070
#define CPU_PORT_BAD_FRAMES_RX	0x00000074
#define CPU_PORT_GOOD_OCTETS_RX	0x00000078
#define CPU_PORT_BAD_OCTETS_RX	0x0000007C

//vid to internal pointer entry
#define VID_TO_INT_ENTRY0		0x0A000000
#define VID0_ENTRY_WORD0		0x0A002000
#define VID0_ENTRY_WORD1		0x0A002004
#define VID0_ENTRY_WORD2		0x0A002008

#define BRIDGE_CPU_PORT_CTRL	0x02018000

#define PORT0_VID_REG			0x02000004

#define INGRESS_CTRL_REG		0x02040000

#define CPU2CODE_PRIO_REG		0x02040028

#define CPU_PORT_VID			0x0204013C

#define BROADCAST_RL_CTRL		0x02040140
#define BROADCAST_RL_WIND_REG	0x02040144

#define UNKNOWN_RATE_LIMIT		0x02040168
#define UNKNOWN_RATE_LIMIT_WIND	0x0204016C

#define TRUNK_TABLE_REG0		0x02040170
#define TRUNK_TABLE_REG1		0x02040174
#define TRUNK_TABLE_REG2		0x02040178
#define TRUNK_TABLE_REG3		0x0204017C

#define DSCP_TO_COS_TABLE		0x02040200


//span port state (for group 0)
#define SPAN_STATE_GROUP0_W0	0x0A006000
#define SPAN_STATE_GROUP0_W1	0x0A006004

#define PORT_MAC_CTRL_REG_P0	0x10000000
#define PORT_MAC_CTRL_REG2P0	0x10000008
#define PORT_ANEG_CONFIG_REG	0x1000000C
#define PORT_STATUS_REG0		0x10000010
#define PORT_INTERRUPT_CAUSE_P0	0x10000020
#define PORT_INTERRUPT_MASK_P0	0x10000024

#define GLOBAL_CONTROL_REG		0x00000000
#define DEVICE_ID_REG			0x0000004C

#define BRIDGE_PORT0_CTRL_REG	0x02000000//bridge port 0 control register



#define TRUNK1_MEMBERS_TABLE_WORD0	0x06000100

#define TRANSMIT_QUEUE_CTRL_REG	0x01800004
#define TRANSMIT_SNIFFER_REG	0x01800008
#define EGRESS_BRIDGING_REG		0x0180000C
#define INGRESS_MIRRORING_REG	0x06000060
#define STATISTIC_SNIFF_REG		0x06000064

#define TRUNK_DESIGNATED_PORTS_HASH0	0x01800288
#define TRUNK_DESIGNATED_PORTS_HASH1	0x01801288
#define TRUNK_DESIGNATED_PORTS_HASH2	0x01802288
#define TRUNK_DESIGNATED_PORTS_HASH3	0x01803288
#define TRUNK_DESIGNATED_PORTS_HASH4	0x01804288
#define TRUNK_DESIGNATED_PORTS_HASH5	0x01805288
#define TRUNK_DESIGNATED_PORTS_HASH6	0x01806288
#define TRUNK_DESIGNATED_PORTS_HASH7	0x01807288


#define TRUNK0_NON_TRUNK_MEMB_REG	0x01800280
#define TRUNK1_NON_TRUNK_MEMB_REG	0x01801280
#define TRUNK2_NON_TRUNK_MEMB_REG	0x01802280
#define TRUNK3_NON_TRUNK_MEMB_REG	0x01803280
#define TRUNK4_NON_TRUNK_MEMB_REG	0x01804280

#define PORT0_TRANSMIT_CFG_REG	0x01800000
#define PROFILE0_WRR_SP_CFG_REG	0x01800504

#define SERDES_EXT_CFG2_REG		0x18000004

#define CALC_MIB_CNT_OFFSET(port,offset) switch(port){\
											case 0: offset = 0x04010000;break;\
											case 1: offset = 0x04010080;break;\
											case 2: offset = 0x04010100;break;\
											case 3: offset = 0x04010180;break;\
											case 4: offset = 0x04010200;break;\
											case 5: offset = 0x04010280;break;\
											case 6: offset = 0x04810000;break;\
											case 7: offset = 0x04810080;break;\
											case 8: offset = 0x04810100;break;\
											case 9: offset = 0x04810180;break;\
											case 10:offset = 0x04810200;break;\
											case 11:offset = 0x04810280;break;\
											case 12:offset = 0x05010000;break;\
											case 13:offset = 0x05010080;break;\
											case 14:offset = 0x05010100;break;\
											case 15:offset = 0x05010180;break;\
											case 16:offset = 0x05010200;break;\
											case 17:offset = 0x05010280;break;\
										}


//MAC Address Control
#define NEW_ADDR_MSG		0
#define QUERY_REQUEST_MSG	1
#define QUERY_RESPOND_MSG	2


#define LINK_DISABLED	8
#define LINK_FORCED_ON	9
#define LINK_NORMAL		1



//PHY
#define FIBER_CTRL_REG		0
#endif /* SALSA2REGS_H_ */

