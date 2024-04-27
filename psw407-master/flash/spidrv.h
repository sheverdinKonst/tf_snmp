#ifndef SPIDRV_H
#define SPIDRV_H

#include "bbdefines.h"

void spi_drv_init(void);
u8 spi_drv_xfer(u8 op,u8 end);
u16 spi_drv_cmd_maxlen(void);
void spi_drv_cmd(u8 *cmd, u16 cmd_len, const u8 *buf, u16 buf_len);
u8 spi_drv_isbusy(void);
u8 *spi_drv_result(u8 offset);

#endif
