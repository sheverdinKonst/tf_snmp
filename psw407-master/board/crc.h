#ifndef _CRC_H
#define _CRC_H


uint32_t Crc32(u32 offset,uint32_t len);
uint32_t BuffCrc(u8 *buff,uint32_t len);
u32 check_crc(u32 offset,u32 len);

uint32_t CryptCrc32(u32 offset,u8 *crypt_key,uint32_t len);

#endif
