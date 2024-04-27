
#ifndef INC_EEPROM_H
#define INC_EEPROM_H


#define EEPROM_BANKS_START	0x08008000UL
#define EEPROM_BANKS_END	0x08010000UL
#define EEPROM_BANK_SIZE	0x4000//0x1000 // 16k
#define EEPROM_BANK_OFFSET	4

#define EEPROM_SISE (EEPROM_BANK_SIZE - EEPROM_BANK_OFFSET) //размер eeprom

extern uint16_t need_update_flag[4];

#define reboot_flag 		need_update_flag[1]
#define bootloader_version  need_update_flag[2]
#define need_default 		need_update_flag[3]



//int8_t Init_Eeprom(void);
int8_t Write_Eeprom(uint16_t Address, void *Data, uint16_t Length);
int8_t Read_Eeprom(uint16_t Address, void *Data, uint16_t Length);

int8_t Verify_Flash(void);
void  Get_Eeprom(void);

int8_t Clear_Eeprom (void);


int settings_save_(void);
int settings_load_(void);

#endif
