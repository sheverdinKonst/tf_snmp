#ifndef SPIFLASH_H
#define SPIFLASH_H

#include "bbdefines.h"
#include "stm32f4xx.h"

#define CMD_WRITE_PAGE	0x02
#define CMD_READ_ARRAY	0x03
#define CMD_READ_ID	0x9F

struct spi_flash_prop {
	u8		idcode1;
	u8		idcode2;
	u8		idcode3;
	u8		idcode4;
	/* Log2 of page size in power-of-two mode */
	u8		l2_page_size;
	u16		page_size;
	u16		pages_per_sector;
	u16     num_sectors;
};


struct spi_flash_params {
	/* Log2 of page size in power-of-two mode */
	u8		l2_page_size;
	u8      page_erasable;
	u16		page_size;
	u16		pages_per_sector;
	u16     num_sectors;
};

struct spi_flash {
	u32		size;
	struct spi_flash_params params;
	u8 manufacture;
	i8		(*read)(struct spi_flash *flash, u32 offset,
				u16 len, void *buf);
	i8		(*write)(struct spi_flash *flash, u32 offset,
				u16 len, const void *buf);
	i8		(*erase)(struct spi_flash *flash, u32 offset,
				u32 len);
};

//struct spi_flash *at45dbxx_probe (u8 *idcode, struct spi_flash *sf);
struct spi_flash *spansion_probe (u8 *idcode, struct spi_flash *sf);
struct spi_flash *macronyx_probe (u8 *idcode, struct spi_flash *sf);


u8 spi_flash_probe(void);
i8 spi_flash_properties(u32 *size, u16 *page_size, u32 *erase_size);
i8 spi_flash_read(u32 offset,u16 len, void *buf);
i8 spi_flash_write(u32 offset,u16 len, void *buf);
i8 spi_flash_erase(u32 offset,u32 len);
void TFTP2Flash(u32 *buff,u32 offset,u16 len);
u8 spi_flash_manufacture(void);

#endif
