/*
 * s25fl128p.h
 *
 *  Created on: 06.11.2012
 *      Author: Alex
 */

#ifndef S25FL128P_H_
#define S25FL128P_H_

#define SPANSION_FLASH 0
#define MACRONYX_FLASH 1

i8 spansion_erase_all(struct spi_flash *flash, u32 offset, u32 len);
void spansion_address (u8 *cmd, u16 page_addr, u8 byte_addr, struct spi_flash_params *params);
i8 spansion_read(struct spi_flash *flash, u32 offset, u16 len, void *buf);
i8 spansion_write(struct spi_flash *flash, u32 offset, u16 len, /*const*/ void *buf);
i8 spansion_erase(struct spi_flash *flash, u32 offset, u32 len);
#endif /* S25FL128P_H_ */
