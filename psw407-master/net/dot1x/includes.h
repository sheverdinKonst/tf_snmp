/*
 * wpa_supplicant/hostapd - Default include files
 * Copyright (c) 2005-2006, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * This header file is included into all C files so that commonly used header
 * files can be selected with OS specific ifdef blocks in one place instead of
 * having to have OS/C library specific selection in many files.
 */


#ifndef INCLUDES_H
#define INCLUDES_H

/* Include possible build time configuration before including anything else */
#include "utils/build_config.h"
#include "utils/common.h"
#include "utils/list.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifndef _WIN32_WCE
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#endif /* _WIN32_WCE */
#include <ctype.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif /* _MSC_VER */



//todo my
//#define AF_INET	 0
#define AF_INET6 1
//#define AF_UNSPEC	2


#define NID_X9_62_prime256v1	415
#define NID_secp384r1			715
#define NID_secp521r1			716
#define NID_X9_62_prime192v1	409
#define NID_secp224r1			713




typedef unsigned long socklen_t;

typedef struct WOLFSSL_BIGNUM {
    int   neg;              /* openssh deference */
    void* internal;         /* our big num */
} WOLFSSL_BIGNUM;

typedef struct WOLFSSL_BN_CTX WOLFSSL_BN_CTX;

typedef WOLFSSL_BIGNUM BIGNUM;
typedef WOLFSSL_BN_CTX BN_CTX;



//todo//my

#define EC_GROUP long int
#define EC_POINT long int
/*#define BN_CTX	long int
BIGNUM *BN_new(void);
void BN_init(BIGNUM *);
void BN_clear(BIGNUM *a);
void BN_free(BIGNUM *a);
BN_CTX *BN_CTX_new(void);*/
EC_POINT *EC_POINT_new(const EC_GROUP *group);
//void BN_CTX_free(BN_CTX *c);
void EC_POINT_clear_free(EC_POINT *point);
void EC_GROUP_free(EC_GROUP *group);
/*void BN_clear_free(BIGNUM *a);
int BN_num_bits(const BIGNUM *a);
int BN_hex2bn(BIGNUM **a, const char *str);
int BN_bn2bin(const BIGNUM *a, unsigned char *to);
BIGNUM *BN_bin2bn(const unsigned char *s, int len,
 BIGNUM *ret);
*/
//int recvfrom(int socket, void *buffer, size_t size, int flags, struct sockaddr *addr, socklen_t *length_ptr);
//ssize_t send(int sockfd, const void *buf, size_t len, int flags);
//uint32_t inet_addr (const char *name);
//ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
//               const struct sockaddr *dest_addr, socklen_t addrlen);
//int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
//int connect(int sockfd, const struct sockaddr *addr,  socklen_t addrlen);

unsigned int alarm (unsigned int seconds);
//int select (int nfds, fd_set *read_fds, fd_set *write_fds, fd_set *except_fds, struct timeval *timeout);
int pipe (int filedes[2]);
//int getsockname (int socket, struct sockaddr *addr, socklen_t *length_ptr);
#define inet_ntoa(x) "1"

#define inet_aton(x,y)0

#define __must_check
/*
int BN_rshift(BIGNUM *r, BIGNUM *a, int n);
int BN_cmp(BIGNUM *a, BIGNUM *b);
int BN_ucmp(BIGNUM *a, BIGNUM *b);
int BN_is_odd(BIGNUM *a);
const BIGNUM *BN_value_one(void);
int BN_num_bytes(const BIGNUM *a);
*/
//uint32_t htonl (uint32_t hostlong);
//uint32_t ntohl (uint32_t hostlong);



int EC_GROUP_get_order(const EC_GROUP *group, BIGNUM *order, BN_CTX *ctx);
int EC_GROUP_get_cofactor(const EC_GROUP *group, BIGNUM *cofactor, BN_CTX *ctx);
int EC_POINT_set_compressed_coordinates_GFp(const EC_GROUP *group, EC_POINT *p,
	const BIGNUM *x, int y_bit, BN_CTX *ctx);
int EC_POINT_is_on_curve(const EC_GROUP *group, const EC_POINT *point, BN_CTX *ctx);
int EC_POINT_add(const EC_GROUP *group, EC_POINT *r, const EC_POINT *a, const EC_POINT *b, BN_CTX *ctx);
int EC_POINT_dbl(const EC_GROUP *group, EC_POINT *r, const EC_POINT *a, BN_CTX *ctx);
int EC_POINT_invert(const EC_GROUP *group, EC_POINT *a, BN_CTX *ctx);
int EC_POINT_is_at_infinity(const EC_GROUP *group, const EC_POINT *p);
int EC_POINT_is_on_curve(const EC_GROUP *group, const EC_POINT *point, BN_CTX *ctx);
int EC_POINT_cmp(const EC_GROUP *group, const EC_POINT *a, const EC_POINT *b, BN_CTX *ctx);
int EC_POINT_make_affine(const EC_GROUP *group, EC_POINT *point, BN_CTX *ctx);
int EC_POINTs_make_affine(const EC_GROUP *group, size_t num, EC_POINT *points[], BN_CTX *ctx);
int EC_POINTs_mul(const EC_GROUP *group, EC_POINT *r, const BIGNUM *n, size_t num, const EC_POINT *p[], const BIGNUM *m[], BN_CTX *ctx);
int EC_POINT_mul(const EC_GROUP *group, EC_POINT *r, const BIGNUM *n, const EC_POINT *q, const BIGNUM *m, BN_CTX *ctx);
int EC_GROUP_precompute_mult(EC_GROUP *group, BN_CTX *ctx);
int EC_GROUP_have_precompute_mult(const EC_GROUP *group);
int EC_POINT_get_affine_coordinates_GFp(const EC_GROUP *group,
                                        const EC_POINT *p, BIGNUM *x,
										BIGNUM *y, BN_CTX *ctx);
int EC_POINT_set_affine_coordinates_GFp(const EC_GROUP *group, EC_POINT *p,
	const BIGNUM *x, const BIGNUM *y, BN_CTX *ctx);




int BN_rand_range(BIGNUM *rnd, BIGNUM *range);
int BN_add(BIGNUM *r, const BIGNUM *a, const BIGNUM *b);



#define WOLFSSL_API

WOLFSSL_API WOLFSSL_BN_CTX* wolfSSL_BN_CTX_new(void);
WOLFSSL_API void           wolfSSL_BN_CTX_init(WOLFSSL_BN_CTX*);
WOLFSSL_API void           wolfSSL_BN_CTX_free(WOLFSSL_BN_CTX*);

WOLFSSL_API WOLFSSL_BIGNUM* wolfSSL_BN_new(void);
WOLFSSL_API void           wolfSSL_BN_free(WOLFSSL_BIGNUM*);
WOLFSSL_API void           wolfSSL_BN_clear_free(WOLFSSL_BIGNUM*);


WOLFSSL_API int wolfSSL_BN_sub(WOLFSSL_BIGNUM*, const WOLFSSL_BIGNUM*,
	                         const WOLFSSL_BIGNUM*);
WOLFSSL_API int wolfSSL_BN_mod(WOLFSSL_BIGNUM*, const WOLFSSL_BIGNUM*,
	                         const WOLFSSL_BIGNUM*, const WOLFSSL_BN_CTX*);

WOLFSSL_API const WOLFSSL_BIGNUM* wolfSSL_BN_value_one(void);


WOLFSSL_API int wolfSSL_BN_num_bytes(const WOLFSSL_BIGNUM*);
WOLFSSL_API int wolfSSL_BN_num_bits(const WOLFSSL_BIGNUM*);

WOLFSSL_API int wolfSSL_BN_is_zero(const WOLFSSL_BIGNUM*);
WOLFSSL_API int wolfSSL_BN_is_one(const WOLFSSL_BIGNUM*);
WOLFSSL_API int wolfSSL_BN_is_odd(const WOLFSSL_BIGNUM*);

WOLFSSL_API int wolfSSL_BN_cmp(const WOLFSSL_BIGNUM*, const WOLFSSL_BIGNUM*);

WOLFSSL_API int wolfSSL_BN_bn2bin(const WOLFSSL_BIGNUM*, unsigned char*);
WOLFSSL_API WOLFSSL_BIGNUM* wolfSSL_BN_bin2bn(const unsigned char*, int len,
	                            WOLFSSL_BIGNUM* ret);

WOLFSSL_API int wolfSSL_mask_bits(WOLFSSL_BIGNUM*, int n);

WOLFSSL_API int wolfSSL_BN_rand(WOLFSSL_BIGNUM*, int bits, int top, int bottom);
WOLFSSL_API int wolfSSL_BN_is_bit_set(const WOLFSSL_BIGNUM*, int n);
WOLFSSL_API int wolfSSL_BN_hex2bn(WOLFSSL_BIGNUM**, const char* str);

WOLFSSL_API WOLFSSL_BIGNUM* wolfSSL_BN_dup(const WOLFSSL_BIGNUM*);
WOLFSSL_API WOLFSSL_BIGNUM* wolfSSL_BN_copy(WOLFSSL_BIGNUM*, const WOLFSSL_BIGNUM*);

WOLFSSL_API int wolfSSL_BN_set_word(WOLFSSL_BIGNUM*, unsigned long w);

WOLFSSL_API int   wolfSSL_BN_dec2bn(WOLFSSL_BIGNUM**, const char* str);
WOLFSSL_API char* wolfSSL_BN_bn2dec(const WOLFSSL_BIGNUM*);





#define BN_CTX_new        wolfSSL_BN_CTX_new
#define BN_CTX_init       wolfSSL_BN_CTX_init
#define BN_CTX_free       wolfSSL_BN_CTX_free

#define BN_new        wolfSSL_BN_new
#define BN_free       wolfSSL_BN_free
#define BN_clear_free wolfSSL_BN_clear_free

#define BN_num_bytes wolfSSL_BN_num_bytes
#define BN_num_bits  wolfSSL_BN_num_bits

#define BN_is_zero  wolfSSL_BN_is_zero
#define BN_is_one   wolfSSL_BN_is_one
#define BN_is_odd   wolfSSL_BN_is_odd

#define BN_cmp    wolfSSL_BN_cmp

#define BN_bn2bin  wolfSSL_BN_bn2bin
#define BN_bin2bn  wolfSSL_BN_bin2bn

#define BN_mod       wolfSSL_BN_mod
#define BN_sub       wolfSSL_BN_sub
#define BN_value_one wolfSSL_BN_value_one

#define BN_mask_bits wolfSSL_mask_bits

#define BN_rand       wolfSSL_BN_rand
#define BN_is_bit_set wolfSSL_BN_is_bit_set
#define BN_hex2bn     wolfSSL_BN_hex2bn

#define BN_dup  wolfSSL_BN_dup
#define BN_copy wolfSSL_BN_copy

#define BN_set_word wolfSSL_BN_set_word

#define BN_dec2bn wolfSSL_BN_dec2bn
#define BN_bn2dec wolfSSL_BN_bn2dec



struct hostapd_data {
	struct hostapd_iface *iface;
	struct hostapd_config *iconf;
	struct hostapd_bss_config *conf;
	int interface_added; /* virtual interface added for this BSS */
	unsigned int started:1;
	unsigned int disabled:1;
	unsigned int reenable_beacon:1;

	u8 own_addr[ETH_ALEN];

	int num_sta; /* number of entries in sta_list */
	struct sta_info *sta_list; /* STA info list head */
#define STA_HASH_SIZE 256
#define STA_HASH(sta) (sta[5])
	struct sta_info *sta_hash[STA_HASH_SIZE];

	/*
	 * Bitfield for indicating which AIDs are allocated. Only AID values
	 * 1-2007 are used and as such, the bit at index 0 corresponds to AID
	 * 1.
	 */
#define AID_WORDS ((2008 + 31) / 32)
	u32 sta_aid[AID_WORDS];

	const struct wpa_driver_ops *driver;
	void *drv_priv;

	void (*new_assoc_sta_cb)(struct hostapd_data *hapd,
				 struct sta_info *sta, int reassoc);

	void *msg_ctx; /* ctx for wpa_msg() calls */
	void *msg_ctx_parent; /* parent interface ctx for wpa_msg() calls */

	struct radius_client_data *radius;
	u64 acct_session_id;
	struct radius_das_data *radius_das;

	struct iapp_data *iapp;

	struct hostapd_cached_radius_acl *acl_cache;
	struct hostapd_acl_query_data *acl_queries;

	struct wpa_authenticator *wpa_auth;
	struct eapol_authenticator *eapol_auth;

	struct rsn_preauth_interface *preauth_iface;
	struct os_reltime michael_mic_failure;
	int michael_mic_failures;
	int tkip_countermeasures;

	int ctrl_sock;
//	struct dl_list ctrl_dst;

	void *ssl_ctx;
	void *eap_sim_db_priv;
	struct radius_server_data *radius_srv;
	//struct dl_list erp_keys; /* struct eap_server_erp_key */

	int parameter_set_count;

	/* Time Advertisement */
	u8 time_update_counter;
	struct wpabuf *time_adv;

#ifdef CONFIG_FULL_DYNAMIC_VLAN
	struct full_dynamic_vlan *full_dynamic_vlan;
#endif /* CONFIG_FULL_DYNAMIC_VLAN */

	struct l2_packet_data *l2;
	struct wps_context *wps;

	int beacon_set_done;
	struct wpabuf *wps_beacon_ie;
	struct wpabuf *wps_probe_resp_ie;
#ifdef CONFIG_WPS
	unsigned int ap_pin_failures;
	unsigned int ap_pin_failures_consecutive;
	struct upnp_wps_device_sm *wps_upnp;
	unsigned int ap_pin_lockout_time;

	struct wps_stat wps_stats;
#endif /* CONFIG_WPS */

	struct hostapd_probereq_cb *probereq_cb;
	size_t num_probereq_cb;

	void (*public_action_cb)(void *ctx, const u8 *buf, size_t len,
				 int freq);
	void *public_action_cb_ctx;
	void (*public_action_cb2)(void *ctx, const u8 *buf, size_t len,
				  int freq);
	void *public_action_cb2_ctx;

	int (*vendor_action_cb)(void *ctx, const u8 *buf, size_t len,
				int freq);
	void *vendor_action_cb_ctx;

	void (*wps_reg_success_cb)(void *ctx, const u8 *mac_addr,
				   const u8 *uuid_e);
	void *wps_reg_success_cb_ctx;

	void (*wps_event_cb)(void *ctx, enum wps_event event,
			     union wps_event_data *data);
	void *wps_event_cb_ctx;

	void (*sta_authorized_cb)(void *ctx, const u8 *mac_addr,
				  int authorized, const u8 *p2p_dev_addr);
	void *sta_authorized_cb_ctx;

	void (*setup_complete_cb)(void *ctx);
	void *setup_complete_cb_ctx;

	void (*new_psk_cb)(void *ctx, const u8 *mac_addr,
			   const u8 *p2p_dev_addr, const u8 *psk,
			   size_t psk_len);
	void *new_psk_cb_ctx;

	/* channel switch parameters */
//	struct hostapd_freq_params cs_freq_params;
	u8 cs_count;
	int cs_block_tx;
	unsigned int cs_c_off_beacon;
	unsigned int cs_c_off_proberesp;
	int csa_in_progress;
	unsigned int cs_c_off_ecsa_beacon;
	unsigned int cs_c_off_ecsa_proberesp;

	/* BSS Load */
	unsigned int bss_load_update_timeout;

#ifdef CONFIG_P2P
	struct p2p_data *p2p;
	struct p2p_group *p2p_group;
	struct wpabuf *p2p_beacon_ie;
	struct wpabuf *p2p_probe_resp_ie;

	/* Number of non-P2P association stations */
	int num_sta_no_p2p;

	/* Periodic NoA (used only when no non-P2P clients in the group) */
	int noa_enabled;
	int noa_start;
	int noa_duration;
#endif /* CONFIG_P2P */
#ifdef CONFIG_INTERWORKING
	size_t gas_frag_limit;
#endif /* CONFIG_INTERWORKING */
#ifdef CONFIG_PROXYARP
	struct l2_packet_data *sock_dhcp;
	struct l2_packet_data *sock_ndisc;
#endif /* CONFIG_PROXYARP */
#ifdef CONFIG_MESH
	int num_plinks;
	int max_plinks;
	void (*mesh_sta_free_cb)(struct hostapd_data *hapd,
				 struct sta_info *sta);
	struct wpabuf *mesh_pending_auth;
	struct os_reltime mesh_pending_auth_time;
	u8 mesh_required_peer[ETH_ALEN];
#endif /* CONFIG_MESH */

#ifdef CONFIG_SQLITE
	struct hostapd_eap_user tmp_eap_user;
#endif /* CONFIG_SQLITE */

#ifdef CONFIG_SAE
	/** Key used for generating SAE anti-clogging tokens */
	u8 sae_token_key[8];
	struct os_reltime last_sae_token_key_update;
	int dot11RSNASAERetransPeriod; /* msec */
#endif /* CONFIG_SAE */

#ifdef CONFIG_TESTING_OPTIONS
	unsigned int ext_mgmt_frame_handling:1;
	unsigned int ext_eapol_frame_io:1;

	struct l2_packet_data *l2_test;
#endif /* CONFIG_TESTING_OPTIONS */

#ifdef CONFIG_MBO
	unsigned int mbo_assoc_disallow;
#endif /* CONFIG_MBO */

//todo
//	struct dl_list nr_db;

	u8 lci_req_token;
	u8 range_req_token;
	unsigned int lci_req_active:1;
	unsigned int range_req_active:1;
};

int ieee802_1x_init(struct hostapd_data *);
#endif
