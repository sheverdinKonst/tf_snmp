#ifndef _RC4_H
#define _RC4_H

#include "stm32f4xx.h"
struct rc4_state {
	u8	perm[256];
	u8	index1;
	u8	index2;
};

void rc4_init(struct rc4_state *state, const u8 *key, int keylen);
void rc4_crypt(struct rc4_state *state,	const u8 *inbuf, u8 *outbuf, int buflen);

void RC4_Init(u8* buf, u8* key, int key_len);
u8 RC4_GetByte(u8* buf, int* x, int* y);
void RC4_Crypt(u8* input, int len, u8* key, int key_len);


#endif
