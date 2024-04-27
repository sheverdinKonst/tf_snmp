/*
 * 123.c
 *
 *  Created on: 26 дек. 2017 г.
 *      Author: BelyaevAlex
 */
#include "dot1x/includes.h"
#include <sys/types.h>
#include "lwip/sockets.h"
//
//
//void BN_init(/*BIGNUM*/int *a){}
//void BN_clear(/*BIGNUM*/int *a){}
BIGNUM *wolfSSL_BN_new(void){}
void wolfSSL_BN_clear_free(WOLFSSL_BIGNUM *a){}
WOLFSSL_BN_CTX *wolfSSL_BN_CTX_new(void){}
int wolfSSL_BN_cmp(const WOLFSSL_BIGNUM* a, const WOLFSSL_BIGNUM* b){}
int wolfSSL_BN_mod(WOLFSSL_BIGNUM* a, const WOLFSSL_BIGNUM *b,
	                         const WOLFSSL_BIGNUM *c, const WOLFSSL_BN_CTX *d){}


EC_POINT *EC_POINT_new(const EC_GROUP *group){}
void BN_CTX_free(BN_CTX *c){}
void EC_POINT_clear_free(EC_POINT *point){}
void EC_GROUP_free(EC_GROUP *group){}
//int BN_num_bits(const /*BIGNUM*/int *a){}
int BN_num_bytes(const BIGNUM *a){}
int BN_hex2bn(BIGNUM **a, const char *str){}
int BN_bn2bin(const BIGNUM *a, unsigned char *to){}
BIGNUM *BN_bin2bn(const unsigned char *s, int len, BIGNUM *ret){}
int EC_GROUP_get_cofactor(const EC_GROUP *group, BIGNUM *cofactor, BN_CTX *ctx){}
int EC_POINT_mul(const EC_GROUP *group, EC_POINT *r, const BIGNUM *n, const EC_POINT *q, const BIGNUM *m, BN_CTX *ctx){}
int EC_POINT_is_at_infinity(const EC_GROUP *group, const EC_POINT *p){}
int EC_POINT_add(const EC_GROUP *group, EC_POINT *r, const EC_POINT *a, const EC_POINT *b, BN_CTX *ctx){}
const BIGNUM *BN_value_one(void){}
int EC_POINT_get_affine_coordinates_GFp(const EC_GROUP *group,
                                        const EC_POINT *p, BIGNUM *x,
										BIGNUM *y, BN_CTX *ctx){}
int EC_POINT_set_affine_coordinates_GFp(const EC_GROUP *group, EC_POINT *p,
	const BIGNUM *x, const BIGNUM *y, BN_CTX *ctx){}
int EC_POINT_invert(const EC_GROUP *group, EC_POINT *a, BN_CTX *ctx){}
int BN_rand_range(BIGNUM *rnd, BIGNUM *range){}
int BN_add(BIGNUM *r, const BIGNUM *a, const BIGNUM *b){}
/*int recvfrom(int socket, void *buffer, size_t size, int flags, struct sockaddr *addr, socklen_t *length_ptr){

}*/
/*ssize_t send(int sockfd, const void *buf, size_t len, int flags){

}*/
/*uint32_t inet_addr (const char *name){

}*/
/*ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen){

}*/
/*int connect(int sockfd, const struct sockaddr *addr,  socklen_t addrlen){

}*/
/*int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen){

}*/
/*int getsockname (int socket, struct sockaddr *addr, socklen_t *length_ptr){

}*/
unsigned int alarm (unsigned int seconds){

}
/*int select (int nfds, fd_set *read_fds, fd_set *write_fds, fd_set *except_fds, struct timeval *timeout){

}*/
int pipe (int filedes[2]){

}
int os_file_exists(const char *fname){

}

/*uint32_t htonl (uint32_t hostlong){

}
uint32_t ntohl (uint32_t hostlong){

}*/
