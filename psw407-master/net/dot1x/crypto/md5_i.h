/*
 * MD5 internal definitions
 * Copyright (c) 2003-2005, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef MD5_I_H
#define MD5_I_H

struct MD5Context_ {
	u32 buf[4];
	u32 bits[2];
	u8 in[64];
};

void MD5Init_(struct MD5Context_ *context);
void MD5Update_(struct MD5Context_ *context, unsigned char const *buf,
	       unsigned len);
void MD5Final_(unsigned char digest[16], struct MD5Context_ *context);

#endif /* MD5_I_H */
