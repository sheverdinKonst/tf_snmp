#ifndef IGMP_H_
#define IGMP_H_

 /*      Linux NET3:     Internet Group Management Protocol  [IGMP]
   *
   *      This code implements the IGMP protocol as defined in RFC1112. There has
   *      been a further revision of this protocol since which is now supported.
   *
   *      If you have trouble with this module be careful what gcc you have used,
   *      the older version didn't come out right using gcc 2.5.8, the newer one
   *      seems to fall out with gcc 2.6.2.
   *
   *      Version: $Id: igmp.c,v 1.46 2001/07/27 09:27:29 davem Exp $
   *
   *      Authors:
   *              Alan Cox <Alan.Cox@linux.org>;  1 /*
  2  *      Linux NET3:     Internet Group Management Protocol  [IGMP]
  3  *
  4  *      Authors:
  5  *              Alan Cox <Alan.Cox@linux.org>
  6  *
  7  *      Extended to talk the BSD extended IGMP protocol of mrouted 3.6
  8  *
  9  *
 10  *      This program is free software; you can redistribute it and/or
 11  *      modify it under the terms of the GNU General Public License
 12  *      as published by the Free Software Foundation; either version
 13  *      2 of the License, or (at your option) any later version.
 14  */
 /*
 *      IGMP protocol structures
 */



 struct iphdr {
        u8    ihl:4,
              version:4;

        u8    tos;
        u16   tot_len;
        u16   id;
        u16   frag_off;
        u8    ttl;
        u8    protocol;
        u16   check;
        u32   saddr;
        u32   daddr;
};

 /*
 *      Header in on cable format
 */

 struct igmphdr
 {
         u8 type;
         u8 code;              /* For newer IGMP */
         u16 csum;
         u32 group;
 };


 /* V3 group record types [grec_type] */
 #define IGMPV3_MODE_IS_INCLUDE          1
 #define IGMPV3_MODE_IS_EXCLUDE          2
 #define IGMPV3_CHANGE_TO_INCLUDE        3
 #define IGMPV3_CHANGE_TO_EXCLUDE        4
 #define IGMPV3_ALLOW_NEW_SOURCES        5
 #define IGMPV3_BLOCK_OLD_SOURCES        6

 struct igmpv3_grec {
         u8    grec_type;
         u8    grec_auxwords;
         u16   grec_nsrcs;
         u32   grec_mca;
         u32   grec_src[0];
 };

 struct igmpv3_report {
         u8 type;
         u8 resv1;
         u16 csum;
         u16 resv2;
         u16 ngrec;
         struct igmpv3_grec grec[0];
 };

 struct igmpv3_query {
         u8 type;
         u8 code;
         u16 csum;
         u32 group;
 #if /*defined(__LITTLE_ENDIAN_BITFIELD)*/1
         u8 qrv:3,
              suppress:1,
              resv:4;
 #elif defined(__BIG_ENDIAN_BITFIELD)
         u8 resv:4,
              suppress:1,
              qrv:3;
 #else
 #error "Please fix <asm/byteorder.h>"
 #endif
         u8 qqic;
         u16 nsrcs;
         u32 srcs[0];
 };

 #define IGMP_HOST_MEMBERSHIP_QUERY      0x11    /* From RFC1112 */
 #define IGMP_HOST_MEMBERSHIP_REPORT     0x12    /* Ditto */
 #define IGMP_DVMRP                      0x13    /* DVMRP routing */
 #define IGMP_PIM                        0x14    /* PIM routing */
 #define IGMP_TRACE                      0x15
 #define IGMPV2_HOST_MEMBERSHIP_REPORT   0x16    /* V2 version of 0x11 */
 #define IGMP_HOST_LEAVE_MESSAGE         0x17
 #define IGMPV3_HOST_MEMBERSHIP_REPORT   0x22    /* V3 version of 0x11 */

 #define IGMP_MTRACE_RESP                0x1e
 #define IGMP_MTRACE                     0x1f


 /*
  *      Use the BSD names for these for compatibility
  */

#define IGMP_DELAYING_MEMBER            0x01
#define IGMP_IDLE_MEMBER                0x02
#define IGMP_LAZY_MEMBER                0x03
#define IGMP_SLEEPING_MEMBER            0x04
#define IGMP_AWAKENING_MEMBER           0x05

#define IGMP_MINLEN                     8

#define IGMP_MAX_HOST_REPORT_DELAY      10      /* max delay for response to */
                                                 /* query (in seconds)   */

#define IGMP_TIMER_SCALE                10      /* denotes that the igmphdr->timer field */
                                                 /* specifies time in 10th of seconds     */

#define IGMP_AGE_THRESHOLD              400     /* If this host don't hear any IGMP V1  */
                                                /* message in this period of time,      */
                                                /* revert to IGMP v2 router.            */

#define IGMP_ALL_HOSTS          htonl(0xE0000001L)
#define IGMP_ALL_ROUTER         htonl(0xE0000002L)
#define IGMPV3_ALL_MCR          htonl(0xE0000016L)
#define IGMP_LOCAL_GROUP        htonl(0xE0000000L)
#define IGMP_LOCAL_GROUP_MASK   htonl(0xFFFFFF00L)


#define MCAST_EXCLUDE   0
#define MCAST_INCLUDE   1

#define IP_DF           0x4000          /* Flag: "Don't Fragment"       */

struct sk_buff {
	 struct sk_buff  * next;                 /* Next buffer in list                          */
	 struct sk_buff  * prev;                 /* Previous buffer in list                      */
	 struct net_device       *dev;           /* Device we arrived on/are leaving by          */
};


/*
 * struct for keeping the multicast list in
  */


 struct ip_sf_socklist
 {
         unsigned int            sl_max;
         unsigned int            sl_count;
         u32                   sl_addr[0];
 };

 #define IP_SFLSIZE(count)       (sizeof(struct ip_sf_socklist) + \
         (count) * sizeof(u32))

 #define IP_SFBLOCK      10      /* allocate this many at once */

 /* ip_mc_socklist is real list now. Speed is not argument;
    this list never used in fast path code
  */

 struct ip_mc_socklist
 {
         struct ip_mc_socklist   *next;
         int                     count;
         struct ip_mreqn         multi;
         unsigned int            sfmode;         /* MCAST_{INCLUDE,EXCLUDE} */
         struct ip_sf_socklist   *sflist;
 };

 struct ip_sf_list
 {
        struct ip_sf_list       *sf_next;
         u32                   sf_inaddr;
         unsigned long           sf_count[2];    /* include/exclude counts */
         unsigned char           sf_gsresp;      /* include in g & s response? */
         unsigned char           sf_oldin;       /* change state */
         unsigned char           sf_crcount;     /* retrans. left to send */
 };

 struct ip_mc_list{
         struct in_device        *interface;
         unsigned long           multiaddr;
         struct ip_sf_list       *sources;
         struct ip_sf_list       *tomb;
         unsigned int            sfmode;
         unsigned long           sfcount[2];
         struct ip_mc_list       *next;
         struct timer_list       timer;
         int                     users;
         atomic_t                refcnt;
         spinlock_t              lock;
         char                    tm_running;
         char                    reporter;
         char                    unsolicit_count;
         char                    loaded;
         unsigned char           gsquery;        /* check source marks? */
         unsigned char           crcount;
 };

 /* V3 exponential field decoding */
 #define IGMPV3_MASK(value, nb) ((nb)>=32 ? (value) : ((1<<(nb))-1) & (value))
 #define IGMPV3_EXP(thresh, nbmant, nbexp, value) \
         ((value) < (thresh) ? (value) : \
         ((IGMPV3_MASK(value, nbmant) | (1<<(nbmant+nbexp))) << \
          (IGMPV3_MASK((value) >> (nbmant), nbexp) + (nbexp))))

 #define IGMPV3_QQIC(value) IGMPV3_EXP(0x80, 4, 3, value)
 #define IGMPV3_MRC(value) IGMPV3_EXP(0x80, 4, 3, value)

 extern int ip_check_mc(struct in_device *dev, u32 mc_addr, u32 src_addr);
 extern int igmp_rcv(struct sk_buff *);
 extern int ip_mc_join_group(struct sock *sk, struct ip_mreqn *imr);
 extern int ip_mc_leave_group(struct sock *sk, struct ip_mreqn *imr);
 extern void ip_mc_drop_socket(struct sock *sk);
 extern int ip_mc_source(int add, int omode, struct sock *sk,
                 struct ip_mreq_source *mreqs, int ifindex);
 extern int ip_mc_msfilter(struct sock *sk, struct ip_msfilter *msf,int ifindex);
 extern int ip_mc_msfget(struct sock *sk, struct ip_msfilter *msf,
                 struct ip_msfilter *optval, int *optlen);
 extern int ip_mc_gsfget(struct sock *sk, struct group_filter *gsf,
                 struct group_filter *optval, int *optlen);
 extern int ip_mc_sf_allow(struct sock *sk, u32 local, u32 rmt, int dif);
 extern void ip_mr_init(void);
 extern void ip_mc_init_dev(struct in_device *);
 extern void ip_mc_destroy_dev(struct in_device *);
 extern void ip_mc_up(struct in_device *);
 extern void ip_mc_down(struct in_device *);
 extern void ip_mc_dec_group(struct in_device *in_dev, u32 addr);
 extern void ip_mc_inc_group(struct in_device *in_dev, u32 addr);

#endif /* IGMP_H_ */
