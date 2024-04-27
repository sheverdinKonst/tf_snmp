/*
 * sfp_cmd.h
 *
 *  Created on: 30.10.2012
 *      Author: Alex
 */

#ifndef SFP_CMD_H_
#define SFP_CMD_H_

#define SFP_EE_SIZE	128

struct sfp_state_t{
	u8 state;
	char vendor[16];//vendor name
	char OUI[3];//The vendor organizationally unique identifier field
	char PN[16];//The vendor part number
	char rev[4];//The vendor revision numbe
	u8 identifier;//Type of serial transceiver
	u8 connector;//Code for connector type
	u8 type;//Ethernet Compliance Codes zB:1000BASE-T
	u8 link_len;//Fibre Channel Link Length
	u8 fibre_tech;//Fibre Channel Technology
	u8 media;//Fibre Channel Transmission Media
	u8 speed;//Fibre Channel Speed
	u8 encoding;//serial encoding mechanism
	u16 wavelen;
	u16 nbr;//nominal bitrate
	u16 len9;//length  (9µm )
	u16 len50;//length  (50µm )
	u16 len62;//length  (62.5µm )
	u16 lenc;//length  (cooper )
	u8 dm_type;
	i16 dm_temper;
	u16 dm_voltage;
	u16 dm_current;
	//u16 dm_outbias;
	u16 dm_txpwr;
	u16 dm_rxpwr;
};


void sfp_get_info(u8 port,struct sfp_state_t *sfp_state);
void sfp_set_addr(u8 addr);
u8 get_sfp_present(u8 port);
u8 get_sfp_set(void);
u8 get_sfp_los(u8 port);
void sfp_set_line_init(void);
void sfp_los_line_init(void);

u8  sfp_ee_write(u8 addr,u8 *buff);
u8 sfp_reprog(u8 port,u8 index);

u8 get_sfp_fw_num(void);
u8 get_sfp_fw_name(u8 index,char *name);

u8 sfp_reprog_file(u8 addr);

void sfp_set_write(u8 state);
#endif /* SFP_CMD_H_ */
