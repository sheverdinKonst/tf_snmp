
#ifndef _COMMAND_H_
#define _COMMAND_H_

struct command_state{
  
};

void command_init(void);
void cmd_appcall(void);

void SetParamToSave(void);
uint32_t Write2EEPROM(void);

void salsa_command(char *Buf);
void salsa2_phy_reg_dump(u8 phy_port,u8 phy_addr);
#endif /*_COMMAND_H_*/
